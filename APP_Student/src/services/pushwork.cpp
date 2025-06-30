//
// Created by Lenovo on 25-7-4.
//
#pragma once

#include "pushwork.h"
pushwork::pushwork() {
}

pushwork::~pushwork() {
}

int pushwork::initwork(Properties  properties) {
    //初始化 Winsock 库
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return RET_FAIL;
    }

    encoder_ =  new H264Encoder;
    width_ = properties.GetProperty("width",640);
    height_ = properties.GetProperty("height",480);
    fps_ = properties.GetProperty("fps",30);
    b_frame_ = properties.GetProperty("height",0);
    bitrate_ = properties.GetProperty("bitrate",500 *1024);
    gop_ = properties.GetProperty("gop",fps_);

    if (encoder_->init(properties) != 0) {
        std::cerr << "encoder init is fail" << std::endl;
        return -1;
    }
    AVPublishTime::GetInstance()->Rest();   // 推流打时间戳的问题

    rtmp_pusher = new RTMPPusher;
    if (rtmp_pusher == NULL)
    {
        printf("new RTMPPusher() failed\n");
        return -1;
    }

    //调用连接类进行连接
    if (!rtmp_pusher->connect(rtmp_url_))
    {
        printf("rtmp_pusher connect() failed\n");
        return -1;
    }


    video_nalu_buf = new uint8_t[VIDEO_NALU_BUF_MAX_SIZE];
    //开始设置RTMP流格式
    // 这是发送头
    //RTMP中的文件类型，Metadata 的主要作用传输一些音视频的参数文件
    metadata = new FLVMetadataMsg();

    // 设置视频相关
    metadata->has_video = 1;
    metadata->width = width_;
    metadata->height = height_;
    metadata->framerate = fps_;
    metadata->videodatarate = bitrate_;
    //把某个消息推送到消息队列中
    rtmp_pusher->Post(RTMP_BODY_METADATA, metadata, false);


    return 0;
}
void pushwork::YuvCallback(AVFrame* yuv)
{
    if (need_send_video_config)
    {
        need_send_video_config = false;
        VideoSequenceHeaderMsg* vid_config_msg = new VideoSequenceHeaderMsg(
            encoder_->get_sps_data(),
            encoder_->get_sps_size(),
            encoder_->get_pps_data(),
            encoder_->get_pps_size()
        );
        //
        vid_config_msg->nWidth = width_;
        vid_config_msg->nHeight = height_;
        vid_config_msg->nFrameRate = fps_;
        vid_config_msg->nVideoDataRate = bitrate_;
        vid_config_msg->pts_ = 0;

        rtmp_pusher->Post(RTMP_BODY_VID_CONFIG, vid_config_msg);

    }
    int video_nalu_size_ = VIDEO_NALU_BUF_MAX_SIZE;
    // 进行编码
    if (encoder_->Encode(yuv, 0, video_nalu_buf, video_nalu_size_) == 0)
    {
        // // 获取到编码数据
        NaluStruct* nalu = new NaluStruct(video_nalu_buf, video_nalu_size_);
        //这里是为了提取NALU的类型
        nalu->type = video_nalu_buf[0] & 0x1f;
        nalu->pts = AVPublishTime::GetInstance()->get_video_pts();
        rtmp_pusher->Post(RTMP_BODY_VID_RAW, nalu);
    }

}
