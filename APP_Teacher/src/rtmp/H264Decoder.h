//
// Created by Lenovo on 25-7-16.
//

#ifndef H264DECODER_H
#define H264DECODER_H

#include "mediabase.h"
//真正的解码器
extern "C"
{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/opt.h"
    #include "libavutil/channel_layout.h"
};
class H264Decoder
{
public:
    H264Decoder();
    ~H264Decoder();
    RET_CODE ReceiveFrame(AVFrame* frame);//接受参数
    RET_CODE SendPacket(AVPacket* pkt);

    //清空解码器
    void FlushBuffers();
public:
    RET_CODE Init(const Properties& properties);
private:
    const AVCodec* codec_;
    AVCodecContext* ctx_;
    AVFrame* picture_;
};




#endif //H264DECODER_H
