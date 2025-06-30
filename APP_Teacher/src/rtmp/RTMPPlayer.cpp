//
// Created by Lenovo on 25-7-16.
//

#include "RTMPPlayer.h"

//转换为小写后比较
// 比较两个字符串是否相等（不区分大小写）
bool iequals_simple(const std::string& a, const std::string& b) {
    // 长度不同直接返回 false
    if (a.size() != b.size()) {
        return false;
    }

    // 逐字符比较（忽略大小写）
    return std::equal(a.begin(), a.end(), b.begin(),
        [](unsigned char c1, unsigned char c2) {
            return std::tolower(c1) == std::tolower(c2);
        });
}
RTMPPlayer::RTMPPlayer()
{
}

RTMPPlayer::~RTMPPlayer()
{
}

RET_CODE RTMPPlayer::Start()
{
    if (!worker_)
    {
        worker_ = new std::thread(std::bind(&RTMPPlayer::readPacketThread, this));
        if (worker_ == NULL)
        {
            printf("new std::thread failed");
            return RET_FAIL;
        }
    }
    return RET_OK;
}

//解读rtmp的metadata数据
void RTMPPlayer::parseScriptTag(RTMPPacket& packet)
{
    AMFObject obj;
    AVal val;
    AMFObjectProperty* property;
    AMFObject subObject;

    //解码成AMF对象
    if (AMF_Decode(&obj, packet.m_body, packet.m_nBodySize, FALSE) < 0)
    {
        printf("%s, error decoding invoke packet\n", __FUNCTION__);
    }

    for (int n = 0; n < obj.o_num; n++)
    {
        property = AMF_GetProp(&obj, NULL, n);

        if (property != NULL)
        {
            //AMF_OBJECT：键值对对象（类似 JSON 对象）。
            if (property->p_type == AMF_OBJECT)
            {
                //从当前的对象中提取出嵌套类型的对象
                AMFProp_GetObject(property, &subObject);

                for (int m = 0; m < subObject.o_num; m++)
                {
                    property = AMF_GetProp(&subObject, NULL, m);
                    if (property != NULL)
                    {
                        if (property->p_type == AMF_BOOLEAN)
                        {
                            //目前所有的参数都是数值类型
                        }
                        //数值类型（AMF_NUMBER） 属性
                        else if (property->p_type == AMF_NUMBER)
                        {
                            double dVal = AMFProp_GetNumber(property);
                            if (iequals_simple("width", property->p_name.av_val))
                            {
                                video_width = (int)dVal;
                                printf("parse widht %d\n", video_width);
                            }
                            else if (iequals_simple("height", property->p_name.av_val))
                            {
                                video_height = (int)dVal;
                                printf("parse Height %d\n", video_height);
                            }
                            else if (iequals_simple("framerate", property->p_name.av_val))
                            {
                                video_frame_rate = (int)dVal;
                                printf("parse frame_rate %d\n", video_frame_rate);
                                if (video_frame_rate > 0) {
                                    video_frame_duration_ = 1000 / video_frame_rate;
                                }
                            }
                            else if (iequals_simple("videocodecid", property->p_name.av_val))
                            {
                                video_codec_id = (int)dVal;
                                printf("parse video_codec_id %d\n", video_codec_id);
                            }
                        }
                        //这个不是必要的参数，打印出来看看
                        else if (property->p_type == AMF_STRING)
                        {
                            AMFProp_GetString(property, &val);
                        }
                    }
                }
            }
            else
            {
                //从property对象中提取 字符串类型的值 并打印出来
                AMFProp_GetString(property, &val);
                printf("val = %s\n", val.av_val);
            }
        }
    }
}

//开始接受
void* RTMPPlayer::readPacketThread()
{
    //AVPlayTime* play_time = AVPlayTime::GetInstance();
    RTMPPacket packet = { 0 };
    //cur_time：记录当前操作的开始时间，用于计算单次操作的耗时
    //pre_time：记录上一次循环的时间，用于计算循环间隔
    int64_t cur_time = TimesUtil::GetTimeMillisecond();
    int64_t pre_time = cur_time;

    uint8_t* nalu_buf = (uint8_t*)malloc(1 * 1024 * 1024);

    int total_slice_offset = 0;
    while (!request_exit_thread_)
    {
        //短线重连
        if (!IsConnect())
        {
            printf("短线重连 re connect\n");
            if (!Connect(url_))      //重连失败
            {
                printf("短线重连 reConnect fail %s", url_.c_str());
                //使用thread的休眠函数
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }
        cur_time = TimesUtil::GetTimeMillisecond();
        //t是当前开始的时间和上一次的时间间隔
        int64_t t = cur_time - pre_time;
        pre_time = cur_time;

        //从服务器接受packet数据
        if (!RTMP_ReadPacket(rtmp_, &packet)) {
            printf("RTMP_ReadPacket failed\n");
            continue;
        }
        //diff：表示从开始到完成 RTMP_ReadPacket 操作所花费的时间
        int64_t diff = TimesUtil::GetTimeMillisecond() - cur_time;
        if (RTMPPacket_IsReady(&packet))    // 检测是不是整个包组好了
        {
            //更新
            diff = TimesUtil::GetTimeMillisecond() - cur_time;
            uint8_t nalu_header_4bytes[4] = { 0x00, 0x00, 0x00, 0x01 };
            uint8_t nalu_header_3bytes[3] = { 0x00, 0x00, 0x01 };

            //对于发送过来的packet包，m_nBodySize表示packet的数据大小
            if (!packet.m_nBodySize)
                continue;


            //如果是视频数据
            if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO)
            {

                // 解析完数据再发送给解码器
                // 判断起始字节, 检测是不是spec config, 还原出sps pps等

                //判断是不是关键帧
                bool keyframe = 0x17 == packet.m_body[0] ? true : false;

                //body[1] = 0x00; // AVC sequence header
                //body[i++] = 0x01;// AVC NALU
                bool sequence = 0x00 == packet.m_body[1];

                // SPS/PPS sequence
                if (keyframe && sequence)//全是true的话是avc的头部
                {
                    is_got_video_sequence_ = true;  //说明已经得到video的配置信息

                    //查看发送端，第10个字节是sps的数量
                    uint32_t offset = 10;
                    uint32_t sps_num = packet.m_body[offset++] & 0x1f;
                    if (sps_num > 0)
                    {
                        sps_vector_.clear();    // 先清空原来的缓存
                    }

                    //开始读取sps
                    for (int i = 0; i < sps_num; i++)
                    {
                        //两个字节的长度
                        //查看发送端，是大端存储，需要转化
                        uint8_t ch0 = packet.m_body[offset];
                        uint8_t ch1 = packet.m_body[offset + 1];

                        uint32_t sps_len = ((ch0 << 8) | ch1);
                        offset += 2;//跳过长度，指向数据起始

                        //根据长度读取数据
                        //对于sps和pps，都是使用4B的头部
                        std::string sps;
                        sps.append(nalu_header_4bytes, nalu_header_4bytes + 4); // 存储 start code
                        sps.append(packet.m_body + offset, packet.m_body + offset + sps_len);
                        sps_vector_.push_back(sps);
                        offset += sps_len;//指向下一个sps的长度起始
                    }

                    //读取pps，流程和上面的sps一样
                    uint32_t pps_num = packet.m_body[offset++] & 0x1f;
                    if (pps_num > 0)
                    {
                        pps_vector_.clear();    // 先清空原来的缓存
                    }
                    for (int i = 0; i < pps_num; i++)
                    {
                        uint8_t ch0 = packet.m_body[offset];
                        uint8_t ch1 = packet.m_body[offset + 1];

                        uint32_t pps_len = ((ch0 << 8) | ch1);
                        offset += 2;

                        //开始读取数据
                        std::string pps;
                        pps.append(nalu_header_4bytes, nalu_header_4bytes + 4); // 存储 start code
                        pps.append(packet.m_body + offset, packet.m_body + offset + pps_len);
                        pps_vector_.push_back(pps);
                        offset += pps_len;
                    }

                    /*
                    *   在 FFmpeg 的解码流程中，所有的媒体数据（包括视频帧、音频帧、SPS、PPS 等）
                    *   都需要通过AVPacket结构体传递给解码器。
                    */
                    // 封装成AVPacket
                    AVPacket sps_pkt = { 0 };
                    sps_pkt.size = sps_vector_[0].size();
                    sps_pkt.data = (uint8_t*)av_malloc(sps_pkt.size);

                    if (av_packet_from_data(&sps_pkt, sps_pkt.data, sps_pkt.size) == 0)
                    {
                        memcpy(sps_pkt.data, (uint8_t*)sps_vector_[0].c_str(), sps_vector_[0].size());

                        // if (!rtmp_dump_h264) {
                        //     fopen_s(&rtmp_dump_h264,"rtmp.h264", "wb+");
                        // }
                        // fwrite(sps_pkt.data, sps_pkt.size, 1, rtmp_dump_h264);
                        //
                        // //正常情况下，文件 I/O 会先将数据存入内存缓冲区，
                        // // 达到一定大小或文件关闭时才写入磁盘。fflush()会立即清空缓冲区，确保数据持久化。
                        // fflush(rtmp_dump_h264);

                        //将AVPacket发送给解码器处理
                        // printf("video_packet_callable_object_\n");
                        video_packet_callable_object_(&sps_pkt);
                    }
                    else {
                        printf("av_packet_from_data sps_pkt failed\n");
                    }

                    //pps的封装流程和上面的sps一样
                    AVPacket pps_pkt = { 0 };
                    pps_pkt.size = pps_vector_[0].size();
                    pps_pkt.data = (uint8_t*)av_malloc(pps_pkt.size);
                    if (av_packet_from_data(&pps_pkt, pps_pkt.data, pps_pkt.size) == 0)
                    {
                        memcpy(pps_pkt.data, (uint8_t*)pps_vector_[0].c_str(), pps_vector_[0].size());
                        // if (!rtmp_dump_h264) {
                        //     fopen_s(&rtmp_dump_h264, "rtmp.h264", "wb+");
                        // }
                        // fwrite(pps_pkt.data, pps_pkt.size, 1, rtmp_dump_h264);
                        // fflush(rtmp_dump_h264);
                        video_packet_callable_object_(&pps_pkt);  // 发送包
                    }
                    else
                    {
                        printf("av_packet_from_data pps_pkt failed\n");
                    }
                    firt_entry = true;
                }
                //否则是video的数据信息
                else
                {
                    if (keyframe && !is_got_video_iframe_) {
                        is_got_video_iframe_ = true;
                    }
                    uint32_t duration = video_frame_duration_;

                     //计算pts
                    //第一次进来就直接是开始的时间
                    if (video_pre_pts_ == -1) {
                        //这是推流端传过来的pts
                        video_pre_pts_ = packet.m_nTimeStamp;
                    }
                    else //判断是绝对时间戳，还是相对时间戳，绝对时间戳就用自己的，相对的就累加
                    {
                        if (packet.m_hasAbsTimestamp) {
                            video_pre_pts_ = packet.m_nTimeStamp;
                        }
                        else {
                            duration = packet.m_nTimeStamp;
                            video_pre_pts_ += packet.m_nTimeStamp;
                        }
                    }

                    //根据协议，前五个字节是信息
                    uint32_t offset = 5;
                    total_slice_offset = 0;
                    int first_slice = 1;

                    //对于rtmp而言，data就是一整个NALU
                    //这就是在重构rtmp发送的packet的NALU数据包
                    while (offset < packet.m_nBodySize)
                    {
                        //四个字节的NALU大小数据，大端存储，需要转化
                        uint8_t ch0 = packet.m_body[offset];
                        uint8_t ch1 = packet.m_body[offset + 1];
                        uint8_t ch2 = packet.m_body[offset + 2];
                        uint8_t ch3 = packet.m_body[offset + 3];
                        uint32_t data_len = ((ch0 << 24) | (ch1 << 16) | (ch2 << 8) | ch3);

                        memcpy(&packet.m_body[offset], nalu_header_4bytes, 4);

                        // 跳过data_len占用的4字节
                        offset += 4;

                        //整个data就是一个NALU，跳过前面的5字节配置，4个字节NALU大小，就是真正的NALU
                        uint8_t nalu_type = 0x1f & packet.m_body[offset];
                        //首个 NALU：添加 4 字节起始码（0x00 0x00 0x00 0x01）。
                        //后续 NALU：添加 3 字节起始码（0x00 0x00 0x01）。
                        if (first_slice) {
                            first_slice = 0;
                            memcpy(&nalu_buf[total_slice_offset], nalu_header_4bytes, 4);
                            total_slice_offset += 4;
                        }
                        else {
                            memcpy(&nalu_buf[total_slice_offset], nalu_header_3bytes, 3);
                            total_slice_offset += 3;
                        }

                        memcpy(&nalu_buf[total_slice_offset], &packet.m_body[offset], data_len);
                        offset += data_len; // 跳过data_len
                        total_slice_offset += data_len;
                        first_slice = 0;
                    }

                    //把NALU拷贝进入AVPacket中

                    AVPacket nalu_pkt = { 0 };
                    nalu_pkt.size = total_slice_offset;
                    nalu_pkt.data = (uint8_t*)av_malloc(nalu_pkt.size);

                    if (av_packet_from_data(&nalu_pkt, nalu_pkt.data, nalu_pkt.size) == 0)
                    {
                        memcpy(nalu_pkt.data, (uint8_t*)nalu_buf, total_slice_offset);

                        nalu_pkt.duration = duration;
                        nalu_pkt.dts = video_pre_pts_;

                        //将当前 AVPacket 标记为关键帧（IDR 帧）。
                        if (keyframe)
                            nalu_pkt.flags = AV_PKT_FLAG_KEY;

                        ////把数据写入文件中
                        //if (!rtmp_dump_h264) {
                        //    fopen_s(&rtmp_dump_h264, "rtmp.h264", "wb+");
                        //}
                        //fwrite(nalu_pkt.data, nalu_pkt.size, 1, rtmp_dump_h264);
                        //fflush(rtmp_dump_h264);
                        video_packet_callable_object_(&nalu_pkt);  // 发送包
                    }
                    else
                    {
                        printf("av_packet_from_data nalu_pkt failed\n");
                    }

                }
            }

            //类型信息
            else if (packet.m_packetType == RTMP_PACKET_TYPE_INFO)
            {
                is_got_metadta_ = true;

                parseScriptTag(packet);

                if (video_width > 0 && video_height > 0)
                {
                    FLVMetadataMsg* metadata = new FLVMetadataMsg();
                    metadata->width = video_width;//720;
                    metadata->height = video_height;//480;

                    video_info_callback_(RTMP_BODY_METADATA, metadata, false);
                }
            }
            else
            {
                printf("can't handle it： %d\n", packet.m_packetType);
                RTMP_ClientPacket(rtmp_, &packet);
            }
            //            RTMP_ClientPacket(m_pRtmp, &packet);
            RTMPPacket_Free(&packet);

            memset(&packet, 0, sizeof(RTMPPacket));
        }
        else
        {
            //            LogError("RTMPPacket no Ready");
            continue;
        }
    }
    free(nalu_buf);
    printf("thread exit");
    return NULL;
}
