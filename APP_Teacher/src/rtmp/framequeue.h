//
// Created by Lenovo on 25-7-10.
//

#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H
#pragma once
#include<iostream>
#include "PacketQueue.h"
extern "C"
{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/opt.h"
    #include "libavutil/channel_layout.h"
};
#define VIDEO_PICTURE_QUEUE_SIZE	3       // 图像帧缓存数量
#define SUBPICTURE_QUEUE_SIZE		16      // 字幕帧缓存数量
#define SAMPLE_QUEUE_SIZE           9       // 采样帧缓存数量
#define FRAME_QUEUE_SIZE		SUBPICTURE_QUEUE_SIZE

// 用于缓存解码后的数据
typedef struct Frame {
    AVFrame* frame;         // 指向数据帧
    int		serial;             // 帧序列，在seek的操作时serial会变化
    double		pts;            // 时间戳，单位为秒
    double		duration;       // 该帧持续时间，单位为秒
    int         int_duration;   // 单位为ms，方便统计避免浮点数的问题
    int64_t		pos;            // 该帧在输入文件中的字节位置
    int		width;              // 图像宽度
    int		height;             // 图像高读
    int		format;             // 对于图像为(enum AVPixelFormat)，
    // 对于声音则为(enum AVSampleFormat)
    AVRational	sar;            // 图像的宽高比（16:9，4:3...），如果未知或未指定则为0/1
    int		uploaded;           // 用来记录该帧是否已经显示过？
    int		flip_v;             // =1则旋转180， = 0则正常播放
} Frame;

class FrameQueue
{
public:
    FrameQueue() {};
    ~FrameQueue() {};

    //初始化队列
    int frame_queue_init(PacketQueue* pktq, int max_size, int keep_last);
    //获取当前队列可以写的Frame
    Frame* frame_queue_peek_writable();
    //更新写指针
    void frame_queue_push(int duration_ms);
    //获取可以读的帧
    Frame* frame_queue_peek_readable();
    //获取队列的大小
    int frame_queue_nb_remaining();
    //上一个显示的帧
    Frame* frame_queue_peek_last();
    //读取待显示的帧
    Frame* frame_queue_peek();
    //更新读索引
    void frame_queue_next();

    //释放掉Frame中的AVFrame指针
    void frame_queue_unref_item(Frame* vp)
    {
        av_frame_unref(vp->frame);	/* 释放数据 */
    }

    int duration()
    {
        return total_duration_;
    }
    Frame* frame_queue_peek_next();
public:
    //用户退出标志
    int abort_request_ = 0;
    Frame	queue_[FRAME_QUEUE_SIZE];        // FRAME_QUEUE_SIZE  最大size, 数字太大时会占用大量的内存，需要注意该值的设置
    int		rindex_ = 0;                         // 读指针
    int		windex_ = 0;                         // 写指针
    int		size_ = 0;                           // 当前队列的有效帧数

    int		max_size_ = FRAME_QUEUE_SIZE;       // 当前队列最大的帧数容量

    // = 1说明要在队列里面保持最后一帧的数据不释放，只在销毁队列的时候才将其真正释放
    int		keep_last_ = 0;
    //已显示的帧数量，这里是在队列中缓存1帧，即rindex_shown = 1
    int		rindex_shown_ = 0;                   // 初始化为0，配合keep_last=1使用
    std::mutex* mutex_ = NULL;                     // 互斥量
    std::condition_variable * cond_ = NULL;                      // 条件变量

    PacketQueue* pktq_;                      // 数据包缓冲队列
    int total_duration_ = 0; //解码之后帧总的持续时间
};


#endif //FRAMEQUEUE_H
