//
// Created by Lenovo on 25-7-3.
//

#ifndef RTMPBASE_H
#define RTMPBASE_H

#include <Winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include "librtmp/rtmp.h"
#include<iostream>
#include<string>
enum RTMP_BASE_TYPE {
    RTMP_BASE_TYPE_PUSH,
    RTMP_BASE_TYPE_PLAY,
    RTMP_BASE_TYPE_UNKNOW
};

class rtmpbase {
public:
    rtmpbase();
    rtmpbase(RTMP_BASE_TYPE rtmp_base_type);
    ~rtmpbase();

    bool connect(std::string url);
    bool isConnect();
    bool init();
    bool connect();
protected:
    std::string url_;
    RTMP *rtmp_;
    RTMP_BASE_TYPE rtmp_base_type_;

};



#endif //RTMPBASE_H
