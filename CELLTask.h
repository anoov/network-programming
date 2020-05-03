//
// Created by 孙卓文 on 2020/5/3.
//

#ifndef EASYTCPSERVER_CELLTASK_H
#define EASYTCPSERVER_CELLTASK_H

#include <thread>
#include <mutex>
#include <list>
//任务类型-基类
class CellTask
{
public:
    CellTask() = default;
    virtual ~CellTask() = default;
    //执行任务
    virtual int doTask() = 0;

private:

};

//执行任务的服务类型
class CellTaskServer
{
public:
    CellTaskServer() = default;
    ~CellTaskServer() = default;
    //添加任务
    void addTask(CellTask* task);
    //启动服务
    void Start();

private:
    //循环执行工作函数
    void OnRun();

private:
    //真实任务数据
    std::list<CellTask*> _tasks;
    //任务数据缓冲区
    std::list<CellTask*> _tasksBuf;
    //改变数据缓冲区时需要加锁
    std::mutex _mutex;
};

void CellTaskServer::addTask(CellTask* task) {
    std::lock_guard<std::mutex> lock(_mutex);
    _tasksBuf.push_back(task);
}

void CellTaskServer::Start() {
    std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
    t.detach();
}

void CellTaskServer::OnRun() {
    while (true) {
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
                pTask->doTask();
                delete pTask;
            }
            _tasks.clear();
        }
    }
}

#endif //EASYTCPSERVER_CELLTASK_H
