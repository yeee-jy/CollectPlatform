//
// Created by Lenovo on 25-7-4.
//

#include "NaluLoop.h"
NaluLoop::NaluLoop()
{
}
NaluLoop::NaluLoop(int queue_nalu_len): max_nalu_(queue_nalu_len)
{
    head_data_available_ = new Semaphore(0);
    worker_ = new std::thread(&NaluLoop::loop, this);
    running_ = true;
}
NaluLoop::~NaluLoop() {
    if (running_)
    {
        Stop();
    }
};
void NaluLoop::Stop()
{
    if (running_)
    {
        printf("Stop\n");
        LooperMessage* msg = new LooperMessage();
        msg->what = 0;
        msg->obj = NULL;
        msg->quit = true;
        addmsg(msg, true);
        //停止线程
        if (worker_)
        {
            worker_->join();
            delete worker_;
            worker_ = NULL;
        }
        if (head_data_available_)
            delete head_data_available_;

        running_ = false;
    }
}
void NaluLoop::addmsg(LooperMessage* msg, bool flush)
{
    queue_mutex_.lock();
    if (flush)
    {
        msg_queue_.clear();
    }
    if (msg_queue_.size() >= max_nalu_)  //移除消息,直到下一个I帧，或者队列为空
    {
        while (msg_queue_.size() > 0)
        {
            LooperMessage* tempMsg = msg_queue_.front();
            // 从I帧开始
            if (tempMsg->what == RTMP_BODY_VID_RAW && ((NaluStruct*)tempMsg->obj)->type == 5)
            {
                // printf("drop msg, now have %d msg", msg_queue_.size());
                break;
            }
            msg_queue_.pop_front();

            delete tempMsg->obj;
            delete tempMsg;
        }
    }
    msg_queue_.push_back(msg);

    queue_mutex_.unlock();
    head_data_available_->post();
}
void NaluLoop::Post(int what, void* data, bool flush)
{
    LooperMessage* msg = new LooperMessage();
    msg->what = what;
    msg->obj = (MsgBaseObj*)data;
    msg->quit = false;
    addmsg(msg, flush);
}
void NaluLoop::loop()
{
    LooperMessage* msg; //利用这个读取消息
    while (true)
    {
        queue_mutex_.lock();
        int size = msg_queue_.size();
        if (size > 0)
        {
            msg = msg_queue_.front();
            msg_queue_.pop_front();
            queue_mutex_.unlock();
            //quit 退出
            if (msg->quit)
            {
                break;
            }
            handle(msg->what, msg->obj);
            delete msg;
        }
        else {
            queue_mutex_.unlock();
            head_data_available_->wait();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    delete msg->obj;
    delete msg;
    while (msg_queue_.size() > 0)
    {
        msg = msg_queue_.front();
        msg_queue_.pop_front();
        delete msg->obj;
        delete msg;
    }
}

void NaluLoop::handle(int what, void* obj)
{
}