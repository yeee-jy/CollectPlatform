//
// Created by Lenovo on 25-7-4.
//

#ifndef H264ENCODER_H
#define H264ENCODER_H

#include "mediabase.h"
#include<iostream>
#include<string>
extern "C"
{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/opt.h"
    #include "libavutil/channel_layout.h"
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
};


class H264Encoder {
public:
    H264Encoder();
    ~H264Encoder();

public:
    int init(Properties properties);
    int Encode(AVFrame* in, int in_samples, uint8_t* out, int& out_size);
private:
    int width_;
    int height_;
    int fps_;
    int b_frame_;
    int bitrate_;
    int gop_;

    const AVCodec *codec_ = NULL;
    AVCodecContext *ctx_ = NULL;
    AVDictionary* param = NULL;//用来配置h264的参数

    std::vector<uint8_t> sps_;
    std::vector<uint8_t> pps_;

    AVFrame *yuv_frame = NULL;
    AVFrame *frame_ = NULL;
    AVPacket *packet_ = NULL;

    int data_size_;//平面大小
    int count = 0;//计数器
    int framecnt = 0;//计数器，表示发送了多少帧
    //申请一个用于存放视频帧的buf
    uint8_t* picture_buf_ = NULL;

    SwsContext* sws_ctx_;           // 格式转换上下文
public:
    uint8_t* get_sps_data()
    {
        return sps_.data();
    }

    size_t get_sps_size()
    {
        return sps_.size();
    }
    uint8_t* get_pps_data()
    {
        return pps_.data();
    }

    size_t get_pps_size()
    {
        return pps_.size();
    }
};



#endif //H264ENCODER_H
