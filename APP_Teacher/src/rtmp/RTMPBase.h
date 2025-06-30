//
// Created by Lenovo on 25-7-16.
//

#ifndef RTMPBASE_H
#define RTMPBASE_H



#pragma once
#include<iostream>
#include<string>

#include <Winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include "librtmp/rtmp.h"


enum RTMP_BASE_TYPE
{
    RTMP_BASE_TYPE_UNKNOW,
    RTMP_BASE_TYPE_PLAY,
    RTMP_BASE_TYPE_PUSH
};

class RTMPBase
{
public:
    // RTMP Server   //由于crtmpserver是每个一段时间(默认8s)发送数据包,需大于发送间隔才行
    bool Connect(std::string url);
    //必须确保已经设置过url
    bool Connect();
    bool IsConnect();

    RTMPBase();
    ~RTMPBase();
private:
    RTMP_BASE_TYPE rtmp_obj_type_ = RTMP_BASE_TYPE_PLAY;//表示这是拉流

protected:
    RTMP* rtmp_ = NULL;
    std::string url_;
    bool enable_video_;      //是否打开视频
    bool enable_audio_;      //是否打开音频
};



#endif //RTMPBASE_H
