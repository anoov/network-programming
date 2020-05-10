//
// Created by 孙卓文 on 2020/5/6.
//

#ifndef EASYTCPSERVER_CELLCLIENT_H
#define EASYTCPSERVER_CELLCLIENT_H

#include "CELLPublicHeader.h"

//客户端心跳检测死亡计时时间 毫秒
#define CLIENT_HEART_DEAD_TIME 60000
//定时发送时间,指定时间内发送缓冲区的消息
#define CLIENT_SEND_BUFF_TIME 500

//客户端数据类型
class CELLClient
{
public:
    explicit CELLClient(SOCKET s = INVALID_SOCKET){
        _sockFd = s;
        memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
        memset(_szSendBuf, 0, sizeof(_szSendBuf));
        _lastPos = 0;
        _lastSendPos = 0;

        resetDTHeart();
        resetDTSend();
    }

    ~CELLClient() {
        printf("CELLClient<%d> ~CELLClient\n", _sockFd);

        if (INVALID_SOCKET != _sockFd) {
#ifdef win32
            closesocket(_sockFd);
#else
            close(_sockFd);
#endif
            _sockFd = INVALID_SOCKET;
        }
    }

    SOCKET GetSock() {return _sockFd;}
    char *GetMsg() {return _szMsgBuf;}
    int GetPos()  {return _lastPos;}
    void SetPos(int pos) {_lastPos = pos;}
    int SendData(DataHeader *header) {
        int ret = SOCKET_ERROR;
        //要发送数据的长度
        int nSendLen = header->dataLength;
        //要发送的数据
        const char* pSendData = (const char *)header;

        if (_lastSendPos + nSendLen <= SEND_BUFF_SIZE) {
            //将要发送的数据拷贝到发送缓冲区尾部
            memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
            _lastSendPos += nSendLen;
            if (_lastSendPos == SEND_BUFF_SIZE) {
                _sendBuffFullCount ++;
            }
            return nSendLen;
        } else {
            _sendBuffFullCount++;
        }
        return ret;

//        while (true) {
//            //如何要发送的数据长度远大于缓冲区，需要循环发送
//            if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE) {
//                //计算可拷贝的数据长度
//                int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
//                //拷贝数据
//                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
//                //计算剩余数据位置
//                pSendData += nCopyLen;
//                //计算剩余数据长度
//                nSendLen -= nCopyLen;
//                //发送数据
//                ret = send(_sockFd, _szSendBuf, SEND_BUFF_SIZE, 0);
//                //数据尾部指针清零
//                _lastSendPos = 0;
//                //重置发送时间
//                resetDTSend();
//                //如何发生错误
//                if (SOCKET_ERROR == ret) {
//                    return ret;
//                }
//            }
//            else {
//                //将要发送的数据拷贝到发送缓冲区尾部
//                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
//                _lastSendPos += nSendLen;
//                break;
//            }
//        }
//        return ret;
    }

    //立即发送数据
    int SendDataNow() {
        int ret = 0;
        //确保缓冲区有数据
        if (_lastSendPos > 0 && INVALID_SOCKET != _sockFd) {
            //发送数据
            ret = send(_sockFd, _szSendBuf, _lastSendPos, 0);
            //数据尾部位置清零
            _lastSendPos = 0;
            //对缓冲区满的计数进行清零
            _sendBuffFullCount = 0;
            //重置发送计时
            resetDTSend();
        }
        return ret;
    }

    void resetDTHeart() {
        _dtHeart = 0;
    }

    //心跳检测
    bool checkHeart(time_t dt) {
        _dtHeart += dt;
        if (_dtHeart >= CLIENT_HEART_DEAD_TIME) {
            printf("checkHeart dead=%d, time=%ld\n", _sockFd, _dtHeart);
            return true;
        } else {
            return false;
        }
    }

    void resetDTSend() {
        _dtSend = 0;
    }

    //定时发送消息检测
    bool checkSend(time_t dt) {
        _dtSend += dt;
        if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
            //printf("checkSend s=%d, time=%ld\n", _sockFd, _dtSend);
            //立即发送缓冲区中的数据
            SendDataNow();
            //重置发送计时
            resetDTSend();
            return true;
        } else {
            return false;
        }
    }

public:
    static const int RECV_BUFF_SIZE = 1024;
    static const int SEND_BUFF_SIZE = 102400;


private:
    SOCKET _sockFd;
    //以下解决粘包和拆分包需要的变量
    //接收缓冲区
    //第二缓冲区  消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE] = {};
    int _lastPos;                        //指向缓冲区有数据的末尾位置

    //发送缓冲区
    char _szSendBuf[SEND_BUFF_SIZE] = {};
    int _lastSendPos;                        //指向缓冲区有数据的末尾位置

    //心跳死亡计时
    time_t _dtHeart;

    //上次发送消息数据的时间
    time_t _dtSend;
    //发送缓冲区写满的次数
    int _sendBuffFullCount = 0;


};
using CELLClientPtr = std::shared_ptr<CELLClient>;
#endif //EASYTCPSERVER_CELLCLIENT_H
