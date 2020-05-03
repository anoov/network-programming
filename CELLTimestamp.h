//
// Created by 孙卓文 on 2020/4/22.
//

#ifndef EASYTCPSERVER_CELLTIMESTAMP_H
#define EASYTCPSERVER_CELLTIMESTAMP_H

#include <chrono>
using namespace std::chrono;

class CELLTimeStamp
{
public:
    CELLTimeStamp() {
        update();
    }
    void update() {
        _begin = high_resolution_clock::now();
    }
    double getElapsedSecond() {
        return getElapsedTimeInMicroSec() * 0.000001;
    }
    double getElapsedTimeInMilliSec() {
        return getElapsedTimeInMicroSec() * 0.001;
    }
    long long getElapsedTimeInMicroSec() {
        return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
    }

private:
    time_point<high_resolution_clock> _begin;

};

#endif //EASYTCPSERVER_CELLTIMESTAMP_H
