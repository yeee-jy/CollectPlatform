//
// Created by Lenovo on 25-7-4.
//

#ifndef PUSHWORK_H
#define PUSHWORK_H
#include "avtimebase.h"
#include<iostream>
#include "./mediabase.h"
#include "./H264Encoder.h"
#include "./services/RTMPPusher.h"
extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
class pushwork {
public:
    pushwork();
    ~pushwork();
    int initwork(Properties  properties);
    void YuvCallback(AVFrame* yuv);
    std::string rtmp_url_ = "rtmp://127.0.0.1:1935/live/stream";
private:
    FLVMetadataMsg *metadata = NULL;
    H264Encoder *encoder_ = NULL;//解码
    RTMPPusher *rtmp_pusher;//发送
    int width_;
    int height_;
    int fps_;
    int b_frame_;
    int gop_;
    int bitrate_;
    //用于发送的数据
    uint8_t* video_nalu_buf = NULL;
    bool need_send_video_config = true;//video的配置
    const int VIDEO_NALU_BUF_MAX_SIZE = 1024 * 1024;	//视频帧的最大大小
};

#endif //RTMPBASE_H