//
// Created by Lenovo on 25-7-16.
//

#ifndef VIDEODECODELOOP_H
#define VIDEODECODELOOP_H
#include<iostream>
#include<thread>
#include "PacketQueue.h"
#include "framequeue.h"
#include "mediabase.h"
// #include "AVSync.h"
#include<functional>
#include "H264Decoder.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include <libavutil/time.h>
};
class VideoDecodeLoop
{
public:
    VideoDecodeLoop(PacketQueue* packetq = NULL, FrameQueue* frameq = NULL);
    ~VideoDecodeLoop();
    RET_CODE Init(const Properties& properties);
    int start();
    void Loop();
    int Post(void* pkt);

    const int YUV_BUF_MAX_SIZE = int(1920 * 1080 * 1.5); //先写死最大支持, fixme

    int		pkt_serial;         // 包序列
    // =0，解码器处于工作状态；=非0，解码器处于空闲状态
    int		finished_;

    int queue_picture(AVFrame* src_frame, double pts, double duration, int serial);
private:
    PacketQueue* pkt_queue_;
    FrameQueue* frame_queue_;

    //视频解码器
    H264Decoder* h264_decoder_ = NULL;
    //缓存和大小
    uint8_t* yuv_buf_;
    int32_t yuv_buf_size_;
    /// 初始化时是stream的start time
    int64_t		start_pts_;
    int request_exit_ = 0;//用户退出标志
    //开始解码线程
    std::thread *worker_ = NULL;

};



#endif //VIDEODECODELOOP_H
