//
// Created by 孙卓文 on 2020/5/7.
//

#ifndef EASYTCPSERVER_CELLSEMAPHORE_H
#define EASYTCPSERVER_CELLSEMAPHORE_H

#include <thread>
#include <chrono>
#include <condition_variable>   //条件变量
class CELLSemaphore
{
public:
    CELLSemaphore() = default;
    ~CELLSemaphore() = default;
    void wait() {
        std::unique_lock<std::mutex> lock(_m);
        if (--_wait < 0) {
            //阻塞等待
            _condition.wait(lock, [this]()->bool { return _wakeUp > 0;});
            --_wakeUp;
        }
    }
    void wakeUp() {
        std::lock_guard<std::mutex> lock(_m);
        if (++_wait <= 0) {
            ++_wakeUp;
            _condition.notify_one();    //通知不用等待了
        }
    }

private:
    //等待计数
    int _wait = 0;
    //唤醒计数
    int _wakeUp = 0;
    std::mutex _m;
    //阻塞等待-条件变量
    std::condition_variable _condition;
};

#endif //EASYTCPSERVER_CELLSEMAPHORE_H
