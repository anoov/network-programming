//
// Created by 孙卓文 on 2020/5/7.
//

#ifndef EASYTCPSERVER_CELLSEMAPHORE_H
#define EASYTCPSERVER_CELLSEMAPHORE_H

#include <thread>
#include <chrono>
class CELLSemaphore
{
public:
    CELLSemaphore() {

    }
    ~CELLSemaphore() {

    }
    void wait() {
        _isWaitExit = true;
        while (_isWaitExit) {
            std::chrono::milliseconds t(1);
            std::this_thread::sleep_for(t);
        }
    }
    void wakeUp() {
        _isWaitExit = false;
    }

private:
    bool _isWaitExit = false;
};

#endif //EASYTCPSERVER_CELLSEMAPHORE_H
