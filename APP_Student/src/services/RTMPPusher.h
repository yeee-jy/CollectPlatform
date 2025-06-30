//
// Created by Lenovo on 25-7-4.
//

#ifndef RTMPPUSHER_H
#define RTMPPUSHER_H


#pragma once
#include "rtmpbase.h"
#include "NaluLoop.h"

class RTMPPusher : public rtmpbase,public NaluLoop
{
public:
    RTMPPusher() :rtmpbase(RTMP_BASE_TYPE_PUSH), NaluLoop(30){ printf("RTMPPusher create!\n"); }
    ~RTMPPusher();

public:
    //  MetaData
    bool SendMetadata(FLVMetadataMsg* metadata);

    //发送packet
    int sendPacket(unsigned int packet_type, unsigned char* data, unsigned int size, unsigned int timestamp);

    //发送视频配置信息
    bool sendH264SequenceHeader(VideoSequenceHeaderMsg* seq_header);

    //发送视频数据
    bool sendH264Packet(char* data, int size, bool is_keyframe, unsigned int timestamp);
private:
    virtual void handle(int what, void* data);
    // 发送metadata ,这样音视频的信息只需要发送一次即，所以再第一次的时候打印
    bool is_first_metadata_ = false;
    enum
    {
        FLV_CODECID_H264 = 7,
        FLV_CODECID_AAC = 10,
    };

    enum RTMPChannel
    {
        RTMP_NETWORK_CHANNEL = 2,   //用于传输网络相关的控制消息
        RTMP_SYSTEM_CHANNEL,        //用于传输系统级控制消息
        RTMP_AUDIO_CHANNEL,         // channel for audio data
        RTMP_VIDEO_CHANNEL = 6,   // channel for video data
        RTMP_SOURCE_CHANNEL = 8,   //用于传输音视频源相关的调用消息
    };
};



#endif //RTMPPUSHER_H
