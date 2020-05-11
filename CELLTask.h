//
// Created by 孙卓文 on 2020/5/3.
//

#ifndef EASYTCPSERVER_CELLTASK_H
#define EASYTCPSERVER_CELLTASK_H

#include <thread>
#include <mutex>
#include <list>
#include <functional>
#include "CELLSemaphore.h"
#include "CELLThread.h"
//#include "CELLLog.h"


//执行任务的服务类型
class CellTaskServer
{
public:
    //所属server的id
    int serverId = -1;
private:
    using CellTask = std::function<void()>;
public:
    CellTaskServer() = default;
    ~CellTaskServer() = default;
    //添加任务
    void addTask(CellTask task);
    //启动服务
    void Start();

    void close() {
        //CELLLog::Info("CellTaskServer<%d> close start\n", serverId);
        printf("CellTaskServer<%d> close start\n", serverId);
        _thread.Close();
        //CELLLog::Info("CellTaskServer<%d> close end\n", serverId);
        printf("CellTaskServer<%d> close end\n", serverId);
    }
private:
    //循环执行工作函数
    void OnRun(CELLThread* pThread);



private:
    //真实任务数据
    std::list<CellTask> _tasks;
    //任务数据缓冲区
    std::list<CellTask> _tasksBuf;
    //改变数据缓冲区时需要加锁
    std::mutex _mutex;

    //bool _isRun = false;
    //CELLSemaphore _sem;

    CELLThread _thread;

};

void CellTaskServer::addTask(CellTask task) {
    std::lock_guard<std::mutex> lock(_mutex);
    _tasksBuf.push_back(task);
}

void CellTaskServer::Start() {

    _thread.Start(nullptr, [this](CELLThread* pThread){OnRun(pThread);}, nullptr);
}

void CellTaskServer::OnRun(CELLThread* pthread) {
    while (pthread->isRun()) {
        //从缓冲区中取出数据
        if (!_tasksBuf.empty()) {
            std::lock_guard<std::mutex> lock(_mutex);
            for (auto &pTask : _tasksBuf) {
                _tasks.push_back(pTask);
            }
            _tasksBuf.clear();
        }
        //如果没有任务
        if (_tasks.empty()) {
            std::chrono::microseconds t(1);
            std::this_thread::sleep_for(t);
            continue;
        }
        //如果有任务，处理任务
        else {
            for (auto& pTask : _tasks) {
                pTask();
                //delete pTask;
            }
            _tasks.clear();
        }
    }
    //_taskBuf缓冲队列中可能还存在没有执行的任务
    for (auto& pTask : _tasksBuf) {
        pTask();
    }
    //CELLLog::Info("CellTaskServer<%d> OnRun exit\n", serverId);
    printf("CellTaskServer<%d> OnRun exit\n", serverId);

}

#endif //EASYTCPSERVER_CELLTASK_H
