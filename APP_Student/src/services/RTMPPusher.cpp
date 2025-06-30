//
// Created by Lenovo on 25-7-4.
//
#include "rtmppusher.h"

//写入一个字节的数据 8位
char* put_byte(char* output, uint8_t nVal)
{
    output[0] = nVal;
    return output + 1;
}

//写入两个字节,还进行了大小端转化，RTMP 等网络协议通常使用大端序
//数据高位存储在低地址，就是高八位存储在output[0]的位置
char* put_be16(char* output, uint16_t nVal)
{
    output[1] = nVal & 0xff;//oxff是八位全1的数，高八位全0，与之后高八位就是0了，就直接变成八位地址了
    output[0] = nVal >> 8;//高八位
    return output + 2;
}
char* put_amf_double(char* c, double d)
{
    *c++ = AMF_NUMBER;  /* type: Number */
    {
        unsigned char* ci, * co;
        ci = (unsigned char*)&d;
        co = (unsigned char*)c;
        co[0] = ci[7];
        co[1] = ci[6];
        co[2] = ci[5];
        co[3] = ci[4];
        co[4] = ci[3];
        co[5] = ci[2];
        co[6] = ci[1];
        co[7] = ci[0];
    }
    return c + 8;
}
//写入一个字符串
char* put_amf_string(char* c, const char* str)
{
    uint16_t len = strlen(str);
    c = put_be16(c, len);//先把长度写进去，然后写入字符串
    memcpy(c, str, len);
    return c + len;
}
RTMPPusher::~RTMPPusher()
{
}

bool RTMPPusher::SendMetadata(FLVMetadataMsg* metadata)
{
    if (metadata == NULL)
    {
        return false;
    }
    char body[1024] = { 0 };

    //这是在写matedata头部的固定字段

    //AMF_STRING 是一个宏定义，通常为 0x02，表示后续数据是字符串。
    //AMF_OBJECT 表示后续是一个键值对对象，类型标识为 0x03。
    char* p = (char*) body;
    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "@setDataFrame");
    //printf("Before: p = %p\n", p);
    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "onMetaData");

    p = put_byte(p, AMF_OBJECT);
    p = put_amf_string(p, "copyright");

    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "firehood");
    if (metadata->has_video)
    {
        p = put_amf_string(p, "width");
        p = put_amf_double(p, metadata->width);

        p = put_amf_string(p, "height");
        p = put_amf_double(p, metadata->height);

        p = put_amf_string(p, "framerate");
        p = put_amf_double(p, metadata->framerate);

        p = put_amf_string(p, "videodatarate");
        p = put_amf_double(p, metadata->videodatarate);

        p = put_amf_string(p, "videocodecid");
        p = put_amf_double(p, FLV_CODECID_H264);
    }
    p = put_amf_string(p, "");
    p = put_byte(p, AMF_OBJECT_END);
    return sendPacket(RTMP_PACKET_TYPE_INFO, (unsigned char*)body, p - body, 0);
}

int RTMPPusher::sendPacket(unsigned int packet_type, unsigned char* data,unsigned int size, unsigned int timestamp)
{
    if (rtmp_ == NULL)
    {
        return FALSE;
    }

    //包装发送的帧
    RTMPPacket packet;
    RTMPPacket_Reset(&packet); //防御性编程，每次使用之前都需要重置
    RTMPPacket_Alloc(&packet, size);

    packet.m_packetType = packet_type;
    if (packet_type == RTMP_PACKET_TYPE_AUDIO)
    {
        packet.m_nChannel = RTMP_AUDIO_CHANNEL;
    }
    else if (packet_type == RTMP_PACKET_TYPE_VIDEO)
    {
        packet.m_nChannel = RTMP_VIDEO_CHANNEL;
    }
    else
    {
        //metabase就是这个用于网络链接的类型
        packet.m_nChannel = RTMP_NETWORK_CHANNEL;
    }

    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;   //chunk的大小
    packet.m_nTimeStamp = timestamp;
    packet.m_nInfoField2 = rtmp_->m_stream_id;  //这个id是服务器返回的id
    packet.m_nBodySize = size;

    //实际上无论是音频原始数据还是配置信息，都是存储在data中
    memcpy(packet.m_body, data, size);

    int nRet = RTMP_SendPacket(rtmp_, &packet, 0);
    if (nRet != 1)
    {
        printf("RTMP_SendPacket fail %d\n", nRet);
    }

    RTMPPacket_Free(&packet);

    return nRet;
}

//这个函数才是真正的来处理中间件消息的函数
void RTMPPusher::handle(int what, void* data)
{
    // printf("RTMPPusher::handle into\n");
    //要加是否断开连接逻辑
    if (!isConnect())
    {
        printf("开始断线重连");
        if (!connect())
        {
            printf("重连失败");
            delete data;
            return;
        }
    }

    //开始对传入的消息进行分类
    switch (what)
    {
        //传输头文件了
    case RTMP_BODY_METADATA:
    {
        //if (!is_first_metadata_) {
        //    is_first_metadata_ = true;
        //    LogInfo("%s:t%u", AVPublishTime::GetInstance()->getMetadataTag(),
        //        AVPublishTime::GetInstance()->getCurrenTime());
        //}
        //data的类型是MsgBaseObj ,开始发送数据
        FLVMetadataMsg* metadata = (FLVMetadataMsg*)data;
        if (!SendMetadata(metadata))
        {
            printf("SendMetadata failed\n");
        }
        delete metadata;
        break;
    }
    case RTMP_BODY_VID_CONFIG:
    {
        //if (!is_first_video_sequence_) {
        //    is_first_video_sequence_ = true;
        //    LogInfo("%s:t%u", AVPublishTime::GetInstance()->getAvcHeaderTag(),
        //        AVPublishTime::GetInstance()->getCurrenTime());
        //}
        VideoSequenceHeaderMsg* vid_cfg_msg = (VideoSequenceHeaderMsg*)data;
        if (!sendH264SequenceHeader(vid_cfg_msg))
        {
            printf("sendH264SequenceHeader failed\n");
        }
        else
            printf("sendH264SequenceHeader is susses\n");
        delete vid_cfg_msg;
        break;
    }
    case RTMP_BODY_VID_RAW:
    {
        NaluStruct* nalu = (NaluStruct*)data;
        //通过之前的判断这里是不是关键帧
        if (sendH264Packet((char*)nalu->data, nalu->size, (nalu->type == 0x05) ? true : false,
            nalu->pts))
        {
            //LogInfo("send pack ok");
        }
        else
        {
            printf("at handle send h264 pack fail\n");
        }
        delete nalu;
        break;
    }
    default:
        break;
    }
}


bool RTMPPusher::sendH264SequenceHeader(VideoSequenceHeaderMsg* seq_header)
{
    if (seq_header == NULL)
    {
        return false;
    }

    uint8_t body[1024] = { 0 };

    int i = 0;
    body[i++] = 0x17; //表示关键帧（1）和 H.264 编码（7）
    body[i++] = 0x00; // AVC sequence header

    //这个是时间戳，由于当前发送的是时间头，所以不需要填充时间
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00; // fill in 0;   0

    // AVCDecoderConfigurationRecord.
    body[i++] = 0x01;               //表示当前使用的 AVC 配置版本，这是一个固定值，目前版本为 1
    body[i++] = seq_header->sps_[1]; // AVCProfileIndication
    body[i++] = seq_header->sps_[2]; // profile_compatibility
    body[i++] = seq_header->sps_[3]; // AVCLevelIndication
    body[i++] = 0xff;               // lengthSizeMinusOne

    // sps nums
    body[i++] = 0xE1;                 //SPS的数量
    // sps data length
    body[i++] = (seq_header->sps_size_ >> 8) & 0xff;;
    body[i++] = seq_header->sps_size_ & 0xff;
    // sps data
    memcpy(&body[i], seq_header->sps_, seq_header->sps_size_);
    i = i + seq_header->sps_size_;

    // pps nums
    body[i++] = 0x01; //PPS的数量
    // pps data length
    body[i++] = (seq_header->pps_size_ >> 8) & 0xff;;
    body[i++] = seq_header->pps_size_ & 0xff;
    // sps data
    memcpy(&body[i], seq_header->pps_, seq_header->pps_size_);
    i = i + seq_header->pps_size_;

    //time_ = TimesUtil::GetTimeMillisecond();
    return sendPacket(RTMP_PACKET_TYPE_VIDEO, (unsigned char*)body, i, 0);
}

//发送264的数据
bool RTMPPusher::sendH264Packet(char* data, int size, bool is_keyframe, unsigned int timestamp)
{
    if (data == NULL && size < 11)
    {
        return false;
    }

    unsigned char* body = new unsigned char[size + 9];

    int i = 0;
    //如果是关键帧，第一个4bit就等于1，否则就是2
    if (is_keyframe)
    {
        body[i++] = 0x17;// 1:Iframe  7:AVC
    }
    else
    {
        body[i++] = 0x27;// 2:Pframe  7:AVC
    }

    body[i++] = 0x01;// AVC NALU
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    // NALU size
    //大小端转化，本来的NALU是小端，即高位在高地址，RTMP是大端，高位在低地址
    body[i++] = size >> 24;
    body[i++] = size >> 16;
    body[i++] = size >> 8;
    body[i++] = size & 0xff;;

    // NALU data
    memcpy(&body[i], data, size);

    bool bRet = sendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, timestamp);
    delete[] body;
    return bRet;
}