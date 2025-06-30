//
// Created by Lenovo on 25-7-4.
//

#pragma once
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
#include <condition_variable>
#include <algorithm>
using namespace std;

class Semaphore
{
public:
    explicit Semaphore(unsigned int initial = 0) {
        count_ = 0;

    }
    ~Semaphore() {

    }
    void post(unsigned int n = 1) {

        unique_lock<mutex> lock(mutex_);
        count_ += n;
        if (n == 1) {
            condition_.notify_one();
        }
        else {
            condition_.notify_all();
        }

    }
    void wait() {
        unique_lock<mutex> lock(mutex_);
        while (count_ == 0) {
            condition_.wait(lock);
        }
        --count_;
    }
private:
    int count_;
    mutex mutex_; //保证对count的互斥访问
    condition_variable_any condition_;  //条件变量，用于线程间的等待和通知
};

#endif // SEMAPHORE_H
