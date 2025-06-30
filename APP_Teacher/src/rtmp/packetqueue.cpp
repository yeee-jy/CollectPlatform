//
// Created by Lenovo on 25-7-16.
//
#include "packetqueue.h"

AVPacket* flush_pkt = NULL;//所有成员都初始化为0

void PacketQueue::packet_queue_start()
{
    std::unique_lock<std::mutex> lock(*mutex_);
    abort_request_ = 0;
    packet_queue_put_private(flush_pkt); // flush_pkt就是用来刷空包的
    // 通知等待线程队列状态已更新
    cond_->notify_all();
}

int PacketQueue::packet_queue_init()
{
    //有问题就返回错误码
	mutex_ = new std::mutex;
    if (!mutex_) {
        printf("new mutex is error!");
        return -1;
    }
    cond_ = new std::condition_variable();
    if (!cond_) {
        printf("new condition_variable is error");
        delete mutex_;
        mutex_ = NULL;
        return -1;
    }
    is_init_ = 1;
    abort_request_ = 1;
    return 0;
}

//把解码前的包放到解码队列中
int PacketQueue::packet_queue_put_private(AVPacket* pkt)
{
    MyAVPacketList* pkt1;

    if (abort_request_)
        return -1;

    pkt1 = (MyAVPacketList*)av_malloc(sizeof(MyAVPacketList));
    if (!pkt1)
        return -1;

    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    //如果是需要清空解码器，说明seek了，播放序列需要加1
    if (pkt == flush_pkt)
    {
        serial_++;
        printf("serial = %d\n", serial_);
    }
    pkt1->serial = serial_;

    //如果队列为空，就直接的加入，最后让尾指针指向
    //如果队列不空，就加到尾部
    if (!last_pkt_)
        first_pkt_ = pkt1;
    else
        last_pkt_->next = pkt1;
    last_pkt_ = pkt1;

    nb_packets_++;
    //AVPacket的大小加上里面指针的大小
    size_ += pkt1->pkt.size + sizeof(*pkt1);
    duration_ += pkt1->pkt.duration;

    //发送一个信号量，表示队列中有数据了
    cond_->notify_one();
    return 0;
}

int PacketQueue::packet_queue_get(AVPacket* pkt, int block, int* serial) {
    MyAVPacketList* pkt1 = nullptr;
    int ret = 0;

    std::unique_lock<std::mutex> lock(*mutex_);

    // 循环等待直到有数据或退出请求
    for (;;) {
        if (abort_request_) {
            ret = -1;  // 退出请求，返回错误
            break;
        }

        pkt1 = first_pkt_;  // 取队首包
        if (pkt1) {
            // 更新队列头指针
            first_pkt_ = pkt1->next;
            if (!first_pkt_) {
                last_pkt_ = nullptr;  // 队列为空时，尾指针也置空
            }

            // 更新队列统计信息
            nb_packets_--;
            size_ -= pkt1->pkt.size + sizeof(*pkt1);
            duration_ -= pkt1->pkt.duration;

            // 关键：使用av_packet_ref增加引用计数
            if (av_packet_ref(pkt, &pkt1->pkt) < 0) {
                ret = -1;
                break;
            }

            if (serial) {
                *serial = pkt1->serial;  // 传递序列号（用于同步）
            }

            // 释放节点前解除引用（减少计数）
            av_packet_unref(&pkt1->pkt);
            av_free(pkt1);  // 释放结构体本身

            ret = 1;  // 成功获取数据包
            break;
        }
        else if (!block) {
            // 非阻塞模式，队列为空时直接返回0
            ret = 0;
            break;
        }
        else {
            // 阻塞等待条件变量（替代SDL_CondWait）
            // 等待时会自动释放锁，被唤醒后重新获取锁
            cond_->wait(lock);
        }
    }
    return ret;
}

int PacketQueue::serial()
{
    return this->serial_;
}

int PacketQueue::duration()
{
    return duration_;
}

int PacketQueue::get_nb_packets()
{
    return nb_packets_;
}

int PacketQueue::packet_queue_put(AVPacket* pkt)
{
    std::lock_guard<std::mutex> lock(*mutex_);
    // 真正把包放到解码前队列中的函数
    int ret = packet_queue_put_private(pkt);

    // 如果不是空包，就释放掉
    if (pkt != flush_pkt && ret < 0)
        av_packet_unref(pkt);

    return ret;
}