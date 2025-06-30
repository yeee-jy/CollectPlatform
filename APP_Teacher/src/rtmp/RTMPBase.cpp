//
// Created by Lenovo on 25-7-16.
//

#include "RTMPBase.h"

bool RTMPBase::Connect(std::string url)
{
    url_ = url;
    return Connect();
}

bool RTMPBase::Connect()
{
    //断线重连必须执行次操作，才能重现连上
    RTMP_Free(rtmp_);
    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);

    printf("base begin connect\n");
    //set connection timeout,default 10s
    rtmp_->Link.timeout = 10;
    if (RTMP_SetupURL(rtmp_, (char*)url_.c_str()) < 0)
    {
        printf("RTMP_SetupURL failed!\n");
        return FALSE;
    }
    rtmp_->Link.lFlags |= RTMP_LF_LIVE;
    RTMP_SetBufferMS(rtmp_, 3600 * 1000);//1hour

    if (!RTMP_Connect(rtmp_, NULL))
    {
        printf("RTMP_Connect failed!\n");
        return FALSE;
    }
    //建立链接之后，服务器可能有多个流，所以需要根据你的rtmp_上下文选择自己流
    //服务器可能同时有多个流（如 live / stream1、live / stream2）
    if (!RTMP_ConnectStream(rtmp_, 0))
    {
        printf("RTMP_ConnectStream failed\n");
        return FALSE;
    }

    return true;
}

bool RTMPBase::IsConnect()
{
    return RTMP_IsConnected(rtmp_);
}

RTMPBase::RTMPBase()
{
}

RTMPBase::~RTMPBase()
{
}