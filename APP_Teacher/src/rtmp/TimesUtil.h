//
// Created by Lenovo on 25-7-16.
//

#ifndef TIMESUTIL_H
#define TIMESUTIL_H
#pragma once
#include<iostream>
#include <winsock2.h>
//#include <ws2tcpip.h>
class TimesUtil
{
public:
    TimesUtil();
    ~TimesUtil();

    //用于获取当前系统时间（毫秒级）。
    static inline int64_t GetTimeMillisecond()
    {
        return (int64_t)GetTickCount64();
    }
};


#endif //TIMESUTIL_H
