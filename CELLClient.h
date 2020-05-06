//
// Created by 孙卓文 on 2020/5/6.
//

#ifndef EASYTCPSERVER_CELLCLIENT_H
#define EASYTCPSERVER_CELLCLIENT_H

#include "CELLPublicHeader.h"

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

        while (true) {
            //如何要发送的数据长度远大于缓冲区，需要循环发送
            if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE) {
                //计算可拷贝的数据长度
                int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
                //拷贝数据
                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
                //计算剩余数据位置
                pSendData += nCopyLen;
                //计算剩余数据长度
                nSendLen -= nCopyLen;
                //发送数据
                ret = send(_sockFd, _szSendBuf, SEND_BUFF_SIZE, 0);
                //数据尾部指针清零
                _lastSendPos = 0;
                //如何发生错误
                if (SOCKET_ERROR == ret) {
                    return ret;
                }
            } else {
                //将要发送的数据拷贝到发送缓冲区尾部
                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
                _lastSendPos += nSendLen;
                break;
            }
        }
        return ret;
    }
private:
    SOCKET _sockFd;
    //以下解决粘包和拆分包需要的变量
    //接收缓冲区
    static const int RECV_BUFF_SIZE = 10240 * 10;
    //第二缓冲区  消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE] = {};
    int _lastPos;                        //指向缓冲区有数据的末尾位置

    //发送缓冲区
    static const int SEND_BUFF_SIZE = 10240 * 10;
    char _szSendBuf[SEND_BUFF_SIZE] = {};
    int _lastSendPos;                        //指向缓冲区有数据的末尾位置


};
#endif //EASYTCPSERVER_CELLCLIENT_H
