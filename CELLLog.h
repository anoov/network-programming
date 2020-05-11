//
// Created by 孙卓文 on 2020/5/11.
//

#ifndef EASYTCPSERVER_CELLLOG_H
#define EASYTCPSERVER_CELLLOG_H

#include <cstdio>
#include "CELLTask.h"
#include <ctime>
class CELLLog
{
public:
    ~CELLLog() {
        _taskServer.close();
        if (_logFile) {
            Info("CELLLog close(_logFile)\n");
            fclose(_logFile);
            _logFile = nullptr;
        }
    }

public:
    static CELLLog& Instance() {
        static CELLLog sLog;
        return sLog;
    }
    //Info
    //Debug
    //Warring
    //Error
    static void Info(const char* pStr) {
        CELLLog* pLog = &Instance();
        pLog->_taskServer.addTask([pLog, pStr](){
            if (pLog->_logFile) {
                auto t = system_clock::now();
                auto tNow = system_clock::to_time_t(t);
                //fprintf(pLog->_logFile, "%s", ctime(&tNow));
                fprintf(pLog->_logFile, "%s" , "Info ");
                std::tm* now = std::gmtime(&tNow);
                fprintf(pLog->_logFile, "[%d-%d-%d, %d:%d:%d] ",
                        now->tm_year+1990, now->tm_mon+1, now->tm_mday, now->tm_hour+8, now->tm_min, now->tm_sec);
                fprintf(pLog->_logFile, "%s" , pStr);
                fflush(pLog->_logFile);
            }

            printf("%s", pStr);
        });

    }
    template <typename ...Args>
    static void Info(const char* pFormat, Args ...args) {
        CELLLog* pLog = &Instance();
        pLog->_taskServer.addTask([pLog, pFormat, args...](){
            if (pLog->_logFile) {
                auto t = system_clock::now();
                auto tNow = system_clock::to_time_t(t);
                //fprintf(pLog->_logFile, "%s", ctime(&tNow));
                std::tm* now = std::gmtime(&tNow);
                fprintf(pLog->_logFile, "%s" , "Info ");
                fprintf(pLog->_logFile, "[%d-%d-%d, %d:%d:%d] ",
                        now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour+8, now->tm_min, now->tm_sec);
                fprintf(pLog->_logFile, pFormat , args...);
                fflush(pLog->_logFile);
            }
            printf(pFormat, args...);
        });

    }
    //设置路径和模式
    void SetLogPath(const char* logPath, const char* mode) {
        if (_logFile) {
            Info("CELLLog::setLogPath _logFile != nullptr\n");
            fclose(_logFile);
            _logFile = nullptr;
        }
        _logFile = fopen(logPath, mode);
        if (_logFile)
        {
            Info("CELLLog::setLogPath success <%s, %s>\n", logPath, mode);
        } else {
            Info("CELLLog::setLogPath failed <%s, %s>\n", logPath, mode);
        }
    }
private:
    CELLLog() {
        _taskServer.Start();
    }

private:
    FILE* _logFile = nullptr;
    CellTaskServer _taskServer;
};

#endif //EASYTCPSERVER_CELLLOG_H
