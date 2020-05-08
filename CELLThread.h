//
// Created by 孙卓文 on 2020/5/8.
//

#ifndef EASYTCPSERVER_CELLTHREAD_H
#define EASYTCPSERVER_CELLTHREAD_H

#include <functional>
#include <mutex>
#include "CELLSemaphore.h"
class CELLThread
{
private:
    using EventCall = std::function<void(CELLThread*)>;
public:
    //启动线程
    void Start(const EventCall& onCreate = nullptr,
               const EventCall& onRun = nullptr,
               const EventCall& onClose = nullptr) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_isRun) {
            _isRun = true;

            if (onCreate) _onCreate = onCreate;
            if (onRun) _onRun = onRun;
            if (onClose) _onDestroy = onClose;
            //线程
            std::thread t(std::mem_fn(&CELLThread::OnWork), this);
            t.detach();
        }
    }
    //关闭线程
    void Close() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_isRun) {
            _isRun = false;
            _sem.wait();
        }
    }
    //自己线程关闭自己 不需要使用信号量，如果使用信号量导致阻塞
    void Exit() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_isRun) {
            _isRun = false;
        }
    }
    //线程是否处于运行状态
    bool isRun() {
        return _isRun;
    }
protected:
    //线程运行时的工作函数
    void OnWork() {
        if (_onCreate) {
            _onCreate(this);
        }
        if (_onRun) {
            _onRun(this);
        }
        if (_onDestroy) {
            _onDestroy(this);
        }
        _sem.wakeUp();
    }

private:
    EventCall _onCreate;
    EventCall _onDestroy;
    EventCall _onRun;
    //线程是否启动运行中
    bool _isRun = false;
    //控制线程的终止退出
    CELLSemaphore _sem;
    //不同线程中改变数据时需要加锁
    std::mutex _mutex;

};

#endif //EASYTCPSERVER_CELLTHREAD_H
