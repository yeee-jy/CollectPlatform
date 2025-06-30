//
// Created by Lenovo on 25-7-16.
//

#ifndef RTMPPLAYER_H
#define RTMPPLAYER_H



#pragma once
#include "RTMPBase.h"
#include "mediabase.h"
#include<thread>
#include<functional>
#include<string>
#include "TimesUtil.h"
extern "C"
{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/opt.h"
    #include "libavutil/channel_layout.h"
    #include <libavutil/time.h>
};

class RTMPPlayer : public RTMPBase
{
public:
	RTMPPlayer();
	~RTMPPlayer();

    RET_CODE Start();

    void* readPacketThread();

    //用于解读头部文件
    void parseScriptTag(RTMPPacket& packet);
public:
    //是否退出线程
    bool request_exit_thread_ = false;
    std::thread* worker_ = NULL;
public:
	//绑定各种回调函数，和发送一样，分别解读音视频的参数和数据
    // 收到视频数据包调用回调
    void AddVideoInfoCallback(std::function<void(int what, MsgBaseObj* data, bool flush)> callback)
    {
        video_info_callback_ = callback;
    }

    void AddVideoPacketCallback(std::function<void(void*)> callback)
    {
        video_packet_callable_object_ = callback;
    }

    //绑定的四个回调函数
    std::function<void(int what, MsgBaseObj* data, bool flush)> video_info_callback_ = NULL;
    std::function<void(void*)> video_packet_callable_object_ = NULL;

private:
    //存储sps和pps
    std::vector<std::string> sps_vector_;    // 可以存储多个sps
    std::vector<std::string> pps_vector_;    // 可以存储多个pps
    //视频参数
    int video_width;
    int video_height;
    int video_frame_rate;//帧率
    int video_frame_duration_;    //通过帧率求出帧间隔，ms
    int video_codec_id;     //视频编码格式
    int64_t video_pre_pts_ = -1;    //视频pts

    //表示是否处理完成pps和sps的参数
    bool firt_entry = false;
    //是否得到matadata信息
    bool is_got_metadta_ = false;
    //是否得到了video的packet数据
    bool is_got_video_iframe_ = false;
    //是否得到了video的配置信息
    bool is_got_video_sequence_ = false;
};




#endif //RTMPPLAYER_H
