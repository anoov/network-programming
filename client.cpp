//
// Created by 孙卓文 on 2020/4/17.
//

#ifndef EASYTCPCLIENT_EASYTCPCLIENT_H
#define EASYTCPCLIENT_EASYTCPCLIENT_H

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SOCKET  int
#define INVALID_SOCKET  (~(unsigned)0)
#define SOCKET_ERROR            (-1)

#include "DataStruct.h"

class EasyTCPClient
{
public:
    explicit EasyTCPClient() {
        _sock = INVALID_SOCKET;
    }
    virtual ~EasyTCPClient() {
        Close();
    }
    //初始化socket
    void InitSocket();
    //连接服务器
    int Connect(char* ip, unsigned short port);
    //关闭socket
    void Close();
    //发送数据
    int SendData(DataHeader *header);
    //接收数据 处理粘包 拆分包
    int RecvData();
    //响应网络消息
    virtual void OnNetMsg(DataHeader *data);
    bool OnRun();
    //是否工作
    bool isRun();
    SOCKET Get() {
        return _sock;
    }
private:
    SOCKET _sock;
    //以下解决粘包和拆分包需要的变量
    //接收缓冲区
    static const int RECV_BUFF_SIZE = 10240;
    char _szRecv[RECV_BUFF_SIZE] = {};
    //第二缓冲区  消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE*10] = {};
    int _lastPos = 0;                        //指向缓冲区有数据的末尾位置
};

void EasyTCPClient::Close() {
    //关闭socket
    if (_sock != INVALID_SOCKET) {
        close(_sock);
        _sock = INVALID_SOCKET;
    }
}

void EasyTCPClient::InitSocket() {
    // 1 建立一个socket
    if (_sock != INVALID_SOCKET) {
        CELLLog::Info("<socket = %d>关闭之前的连接...\n", _sock);
        Close();
    }
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    if (_sock == INVALID_SOCKET) {
        CELLLog::Info("建立套接字失败...\n");
    } else {
        CELLLog::Info("建立套接字成功...\n");
    }
}

int EasyTCPClient::Connect(char *ip, unsigned short port) {
    if (_sock == INVALID_SOCKET)
        InitSocket();
    // 2 连接服务器  connect
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(port);
    _sin.sin_addr.s_addr = inet_addr(ip);
    int ret = connect(_sock, (sockaddr *)&_sin, sizeof(sockaddr));
    if (ret == SOCKET_ERROR) {
        CELLLog::Info("连接失败...\n");
    }else {
        CELLLog::Info("<socket = %d>连接成功...\n", _sock);
    }
    return ret;
}

bool EasyTCPClient::OnRun() {
    if (isRun()) {
        fd_set fdReader;
        FD_ZERO(&fdReader);
        FD_SET(_sock, &fdReader);
        timeval t = {1, 0};
        int selectRet = select(_sock + 1, &fdReader, nullptr, nullptr, &t);
        if (selectRet < 0) {
            CELLLog::Info("<Socket = %d>select发生错误, 任务结束1!\n", _sock);
            Close();
            return false;
        }
        if (FD_ISSET(_sock, &fdReader)) {
            FD_CLR(_sock, &fdReader);
            if (-1 == RecvData()) {
                CELLLog::Info("<Socket = %d>select发生错误, 任务结束2!\n", _sock);
                Close();
                return false;
            }
        }
        return true;
    }
    return false;
}

//接收数据
int EasyTCPClient::RecvData() {
    //5 接收客户端数据
    //5.1 接受数据头
    //使用缓冲区来接受数据
    int nLen = recv(_sock, _szRecv, RECV_BUFF_SIZE, 0);
    if (nLen <= 0) {
        CELLLog::Info("<socket = %d>与服务器断开连接, 任务结束\n", _sock);
        return -1;
    }
    //将收取到的数据拷贝到消息缓冲区
    memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
    _lastPos += nLen;
    //判断收到消息的长度是否大于消息头的长度，若大于消息头的长度，就可以取出消息体的长度
    while (_lastPos >= sizeof(DataHeader)) {
        auto *header = (DataHeader*)_szMsgBuf;
        //判断收到消息的长度是否大于消息体的长度，若大于消息体的长度，就可以取出消息体
        if (_lastPos >= header->dataLength) {
            //此时消息缓冲中有两部分数据，一部分是待处理的数据，第二部分是下一次处理的数据
            //消息缓冲区中未处理消息的长度
            int nSize = _lastPos - header->dataLength;  //消息缓冲中收到的数据总长度减去将要处理的数据的长度
            OnNetMsg(header);
            //通过掩盖的方法将处理过的数据清除
            memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
            _lastPos = nSize;
        } else {
            //剩余的消息虽然大于消息头但是小于消息体
            break;
        }
    }
    return 0;
}

bool EasyTCPClient::isRun() {
    return _sock != INVALID_SOCKET;
}

void EasyTCPClient::OnNetMsg(DataHeader *data) {
    switch (data->cmd) {
        case CMD_LOGIN_RESULT:
        {
            LoginResult* login = (LoginResult *)data;
            CELLLog::Info("<Socket = %d>收到服务器消息：CMD_LOGIN_RESULT, 数据长度: %d，结果: %d\n",
                   _sock, login->dataLength, login->result);
        }
            break;
        case CMD_LOGOUT_RESULT:
        {
            LoginOutResult* loginOut = (LoginOutResult *)data;
            CELLLog::Info("<Socket = %d>收到服务器消息：CMD_LOGOUT_RESULT, 数据长度: %d\n",
                   _sock ,loginOut->dataLength);
        }
            break;
        case CMD_NEW_USER_JOIN:
        {
            NewUserJoin* newJoin = (NewUserJoin *)data;
            CELLLog::Info("<Socket = %d>收到服务器消息：NEW_USER_JOIN, 数据长度: %d\n",
                   _sock ,newJoin->dataLength);
        }
            break;
        default:
        {
            CELLLog::Info("<Socket = %d>收到未定义消息：NEW_ERROR, 数据长度: %d\n",
                   _sock ,data->dataLength);
        }
            break;
    }
}

int EasyTCPClient::SendData(DataHeader *header) {
    if (isRun() && header)
        send(_sock, header, header->dataLength, 0);
    return SOCKET_ERROR;
}

#endif //EASYTCPCLIENT_EASYTCPCLIENT_H
