//
// Created by Lenovo on 25-7-10.
//

#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#pragma once
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
};
#include<thread>
#include<mutex>
#include<condition_variable>
//只声明，让编译器自己取找
extern AVPacket *flush_pkt;

//真正存放视频帧的结构
typedef struct MyAVPacketList {
    AVPacket		pkt;
    struct MyAVPacketList* next;          // 指向下一个元素
    int			serial;
} MyAVPacketList;


//存放解码之前的队列
class PacketQueue
{
public:
    MyAVPacketList* first_pkt_ = NULL;
    MyAVPacketList* last_pkt_ = NULL;  // 队首，队尾指针
    int		nb_packets_ = 0;                 // 包数量，也就是队列元素数量
    int		size_ = 0;                       // 队列所有元素的数据大小总和
    int		duration_ = 0;               // 队列所有元素的数据播放持续时间
    int		abort_request_ = 0;              // 用户退出请求标志
    int		serial_ = 0;                     //
    std::mutex *mutex_ = NULL;                 // 互斥量
    std::condition_variable *cond_ = NULL;                  // 条件变量
    int is_init_ = 0;
public:
    PacketQueue() {}
    ~PacketQueue() {}
public:
    static void InitFlushPacket()
    {
        flush_pkt = av_packet_alloc();
        flush_pkt->data = (uint8_t*)&flush_pkt; // 初始化为数据指向自己本身
    }
    void packet_queue_start();
    int packet_queue_init();

    //真正往队列里面加入packet的函数
    int packet_queue_put_private(AVPacket *pkt);

    //从队列中取一帧
    int packet_queue_get(AVPacket* pkt, int block, int* serial);

    //返回当前解码前的序列
    int serial();

    //返回当前解码器前队列的总市场
    int duration();

    //返回队列帧长度
    int get_nb_packets();

    //把packet包放到队列中取
    int packet_queue_put(AVPacket* pkt);
};


#endif //PACKETQUEUE_H
