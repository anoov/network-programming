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
    explicit CELLClient(SOCKET s = INVALID_SOCKET):_recvBuff(RECV_BUFF_SIZE), _sendBuff(SEND_BUFF_SIZE){
        _sockFd = s;

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
    //发送数据, 将数据写到缓冲区
    int SendData(DataHeader *header) {
        //要发送数据的长度
        int nSendLen = header->dataLength;
        //要发送的数据
        const char *pSendData = (const char *) header;

        if (_sendBuff.Push((const char *) pSendData, nSendLen)) {
            return nSendLen;    //添加到缓冲区成功
        }
        return SOCKET_ERROR;
    }
    //立即发送数据
    int SendDataNow() {
        int ret = _sendBuff.Write2Socket(_sockFd);
        if (ret) resetDTSend();
        return ret;
    }

    //接收数据 成功返回收发字节数
    int RecvData() {
        return _recvBuff.ReadFromSocket(_sockFd);
    }
    //判断缓冲区中是否有完整的数据
    bool hasMsg() {
        return _recvBuff.hasMsg();
    }
    //取出缓冲区中头的完整数据
    DataHeader* frontMsg() {
        return (DataHeader*)_recvBuff.GetBuf();
    }
    //移除头完整数据
    void popFrontMsg() {
        if (hasMsg())
            _recvBuff.Pop(frontMsg()->dataLength);
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
    //定时发送，重置发送时间
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


private:
    SOCKET _sockFd;
    //以下解决粘包和拆分包需要的变量
    //接收缓冲区
    //第二缓冲区  消息缓冲区
//    char _szMsgBuf[RECV_BUFF_SIZE] = {};
//    int _lastPos;                        //指向缓冲区有数据的末尾位置

    //接收缓冲区
    CELLBuffer _recvBuff;

    //发送缓冲区
    CELLBuffer _sendBuff;

    //心跳死亡计时
    time_t _dtHeart;

    //上次发送消息数据的时间
    time_t _dtSend;
    //发送缓冲区写满的次数
    int _sendBuffFullCount = 0;


};
using CELLClientPtr = std::shared_ptr<CELLClient>;
#endif //EASYTCPSERVER_CELLCLIENT_H
