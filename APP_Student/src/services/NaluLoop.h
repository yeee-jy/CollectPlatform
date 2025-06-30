//
// Created by Lenovo on 25-7-4.
//

#ifndef NALULOOP_H
#define NALULOOP_H

#include <mutex>
#include<deque>
#include"mediabase.h"
#include<thread>
#include "./semaphore.h"
class NaluLoop {
public:
    NaluLoop();
    NaluLoop(int queue_nalu_len);
    ~NaluLoop();
    void addmsg(LooperMessage* msg, bool flush);
    void Post(int what, void* data, bool flush = false);
    void loop();
    virtual void handle(int what, void* obj);
    void Stop();
protected:
    std::deque< LooperMessage* > msg_queue_;
    std::mutex queue_mutex_;
    int max_nalu_;
    //这是自己定义的互斥量
    Semaphore* head_data_available_;
    std::thread *worker_ = NULL;
    bool running_ = false;
};



#endif //NALULOOP_H
