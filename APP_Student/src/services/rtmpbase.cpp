//
// Created by Lenovo on 25-7-3.
//

#include "rtmpbase.h"

rtmpbase::rtmpbase() {

}
rtmpbase::~rtmpbase() {

}
rtmpbase::rtmpbase(RTMP_BASE_TYPE rtmp_base_type):rtmp_base_type_(rtmp_base_type)
{
    init();
}
bool rtmpbase::init() {
    rtmp_ = RTMP_Alloc();
    if (rtmp_ == NULL)
        return false;

    RTMP_Init(rtmp_);
    return true;
}

bool rtmpbase::connect() {
    RTMP_Free(rtmp_);
    rtmp_ = RTMP_Alloc();
    if (rtmp_ == NULL)
        return false;
    RTMP_Init(rtmp_);

    rtmp_->Link.timeout = 10;//设置超时的时间

    if (RTMP_SetupURL(rtmp_,(char*)url_.c_str()) < 0) {
        std::cerr<< "RTMP_SetupURL is fail" << std::endl;
        return false;
    }

    rtmp_->Link.lFlags |= RTMP_LF_LIVE;//设置为直播模式

    RTMP_SetBufferMS(rtmp_,3600 * 1000);//缓冲区，1h

    if (rtmp_base_type_ == RTMP_BASE_TYPE_PUSH) {
        RTMP_EnableWrite(rtmp_);//开启推流
    }

    if (RTMP_Connect(rtmp_, NULL)) {
        printf("RTMP is susses\n");
    } else {
        printf("RTMP is fail\n");
        return false;
    }

    if (!RTMP_ConnectStream(rtmp_,0)) {
        std::cerr << "RTMP_ConnectStream is fail" << std::endl;
        return false;
    }

    return true;
}

bool rtmpbase::connect(std::string url) {
    url_ = url;
    return connect();
}

bool rtmpbase::isConnect() {
    return RTMP_IsConnected(rtmp_);
}


