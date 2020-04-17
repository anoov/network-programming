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
    //接收数据
    int RecvData();
    //处理网络消息
    void OnNetMsg(DataHeader *header, char *szRecv);
    bool OnRun();
    bool isRun();
    SOCKET Get() {
        return _sock;
    }
private:
    SOCKET _sock;

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
        printf("<socket = %d>关闭之前的连接...\n", _sock);
        Close();
    }
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    if (_sock == INVALID_SOCKET) {
        printf("建立套接字失败...\n");
    } else {
        printf("建立套接字成功...\n");
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
        printf("连接失败...\n");
    }else {
        printf("连接成功...\n");
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
            printf("<Socket = %d>select发生错误, 任务结束1!\n", _sock);
            return false;
        }
        if (FD_ISSET(_sock, &fdReader)) {
            FD_CLR(_sock, &fdReader);
            if (-1 == RecvData()) {
                printf("<Socket = %d>select发生错误, 任务结束2!\n", _sock);
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
    char szRecv[4096] = {};
    int nLen = recv(_sock, szRecv, sizeof(DataHeader), 0);
    auto* header = (DataHeader *)szRecv;
    if (nLen <= 0) {
        printf("与服务器断开连接, 任务结束\n");
        return -1;
    }
    recv(_sock, szRecv+sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
    OnNetMsg(header, szRecv);
    return 0;
}

bool EasyTCPClient::isRun() {
    return _sock != INVALID_SOCKET;
}

void EasyTCPClient::OnNetMsg(DataHeader *header, char *szRecv) {
    switch (header->cmd) {
        case CMD_LOGIN_RESULT:
        {
            LoginResult* login = (LoginResult *)szRecv;
            printf("收到服务器<Socket = %d>消息：CMD_LOGIN_RESULT, 数据长度: %d，用户名称: %d\n",
                   _sock, login->dataLength, login->result);
        }
            break;
        case CMD_LOGOUT_RESULT:
        {
            LoginOutResult* loginOut = (LoginOutResult *)szRecv;
            printf("收到服务器<Socket = %d>消息：CMD_LOGOUT_RESULT, 数据长度: %d\n",
                   _sock ,loginOut->dataLength);
        }
            break;
        case CMD_NEW_USER_JOIN:
        {
            NewUserJoin* newJoin = (NewUserJoin *)szRecv;
            printf("收到服务器<Socket = %d>消息：NEW_USER_JOIN, 数据长度: %d\n",
                   _sock ,newJoin->dataLength);
        }
            break;
        default:
        {
            header->cmd = CMD_ERROR;
            header->dataLength = 0;
            send(_sock, &header, sizeof(DataHeader), 0);
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
