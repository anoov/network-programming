//
// Created by 孙卓文 on 2020/4/18.
//

#ifndef EASYTCPSERVER_EASYTCPSERVER_H
#define EASYTCPSERVER_EASYTCPSERVER_H
#include <iostream>
#include <cstdio>
#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
//#include <sys/socket.h>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include <vector>
#include "DataStruct.h"

class EasyTCPServer
{
public:
    explicit EasyTCPServer() {
        _sock = INVALID_SOCKET;
    }
    virtual ~EasyTCPServer() {

    }
    //初始化socket
    int InitSocket();
    //绑定IP, 端口号
    int BindPort(const char *ip, unsigned short port);
    //监听端口号
    int ListenPort(int n);
    //接收客户端连接
    SOCKET Accept();
    //处理网络消息
    bool OnRun();
    //是否工作中
    bool IsRun();
    //接收数据  处理粘包 拆分包
    int RecvData(SOCKET clientSock);
    //响应网络消息
    void OnNetMsg(SOCKET clientSock, DataHeader *header, char *szRecv);
    //发送指定socket数据
    int SendData(SOCKET client, DataHeader *header);
    //群发
    void SendDataToAll(DataHeader *header);
    //关闭socket
    void Close();

private:
    SOCKET _sock;
    std::vector<SOCKET> clients;

};

int EasyTCPServer::InitSocket() {
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
    return _sock;
}

int EasyTCPServer::BindPort(const char *ip, unsigned short port) {
    if (_sock == INVALID_SOCKET) {
        InitSocket();
    }
    sockaddr_in _sin{};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(port);    //host to net unsigned short
    _sin.sin_addr.s_addr = inet_addr(ip);
    int ret = bind(_sock, (sockaddr*)& _sin, sizeof(_sin));
    if (ret == SOCKET_ERROR) {
        printf("绑定网络端口号<%d>失败...\n", port);
    } else {
        printf("绑定网络端口号<%d>成功...\n", port);
    }
    return ret;
}

int EasyTCPServer::ListenPort(int n) {
    int ret = listen(_sock, n);
    if (ret == SOCKET_ERROR){
        printf("<Socket=%d>监听网络端口失败...\n", _sock);
    } else {
        printf("<Socket=%d>监听网络端口成功...\n", _sock);
    }
    return ret;
}

void EasyTCPServer::Close() {
    if (_sock != INVALID_SOCKET) {
        for (int& client : clients) {
            close(client);
            client = INVALID_SOCKET;
        }
        close(_sock);
        _sock = INVALID_SOCKET;
    }

}

SOCKET EasyTCPServer::Accept() {
    //4 accept 等待接受客户连接
    sockaddr_in clientAddr = {};
    socklen_t nAddrLen = sizeof(sockaddr_in);
    SOCKET clientSock = INVALID_SOCKET;

    clientSock = accept(_sock, (sockaddr *) &clientAddr, &nAddrLen);
    if (clientSock == INVALID_SOCKET) {
        printf("<Socket=%d>接受客户连接失败...\n", _sock);
    } else {
        NewUserJoin userJoin{};
        SendDataToAll(&userJoin);
        clients.push_back(clientSock);
        printf("<Socket=%d>接受新客户连接成功: socket = %d, IP = %s\n",
                _sock, (int)(clientSock), inet_ntoa(clientAddr.sin_addr));
    }
    return clientSock;
}

bool EasyTCPServer::OnRun() {
    if (IsRun()) {
        //伯克利 socket
        //第一个参数：集合中所有描述符的范围而不是数量，集合中最大值加一
        //第二个参数：读描述符集合，告诉内核需要查询的需要读的套接字的集合
        //第三个参数：写描述符集合
        //第三个参数：异常描述符集合
        //第四个参数：时间，在该时间内没有返回，则返回
        fd_set fdRead;
        fd_set fdWrite;
        fd_set fdExcept;
        FD_ZERO(&fdRead);   //清空集合中的数据
        FD_ZERO(&fdWrite);   //清空集合中的数据
        FD_ZERO(&fdExcept);   //清空集合中的数据

        FD_SET(_sock, &fdRead);      //让内核代理查看socket有没有读操作
        FD_SET(_sock, &fdWrite);     //让内核代理查看socket有没有写操作
        FD_SET(_sock, &fdExcept);    //让内核代理查看socket有没有异常操作

        SOCKET maxSock = _sock;
        for (int i = (int) clients.size() - 1; i >= 0; i--) {
            FD_SET(clients[i], &fdRead);//有没有客户需要接收
            maxSock = std::max(maxSock, clients[i]);
        }

        timeval t = {1, 0};
        //若最后一个参数设置为null，则程序会阻塞到select这里，一直等到有数据可处理
        //select监视三个集合中的所有描述符，在这里是套接字
        //例如select的第二个参数读集合，
        //若集合中的某一个socket有读操作，则保持该操作位，否则该操作位清零
        int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExcept, &t);

        if (ret < 0) {
            printf("select发生错误, 任务结束\n");
            Close();
            return false;
        }

        //若本socket有读操作，意味着有客户机连进来
        if (FD_ISSET(_sock, &fdRead)) {
            FD_CLR(_sock, &fdRead);

            //4 accept 等待接受客户连接
            Accept();

        }

        for (int n = (int) clients.size() - 1; n >= 0; n--) {
            //如何客户socket数组中有读组操作，则说明有消息进来
            if (FD_ISSET(clients[n], &fdRead)) {
                if (-1 == RecvData(clients[n])) {
                    auto iter = clients.begin() + n;//std::vector<SOCKET>::iterator
                    if (iter != clients.end()) {
                        clients.erase(iter);
                    }
                }
            }
        }
        return true;
    }
    return false ;
}

bool EasyTCPServer::IsRun() {
    return _sock != INVALID_SOCKET;
}

int EasyTCPServer::RecvData(SOCKET clientSock) {
    char szRecv[4096] = {};
    int nLen = (int)recv(clientSock, szRecv, sizeof(DataHeader), 0);
    auto* header = (DataHeader *)szRecv;
    if (nLen <= 0) {
        printf("客户端<Socket = %d>退出, 任务结束\n", clientSock);
        return -1;
    }
    recv(clientSock, szRecv+sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
    OnNetMsg(clientSock, header, szRecv);
    return 0;
}

void EasyTCPServer::OnNetMsg(SOCKET clientSock, DataHeader *header, char *szRecv) {
    switch (header->cmd) {
        case CMD_LOGIN:
        {
            Login* login = (Login *)szRecv;
            printf("收到<Socket = %d>请求：CMD_LOGIN, 数据长度: %d，用户名称: %s, 用户密码: %s\n",
                   clientSock, login->dataLength, login->userName, login->passWord);
            //忽略判断用户名和密码
            LoginResult ret;
            send(clientSock, &ret, sizeof(LoginResult), 0);
        }
            break;
        case CMD_LOGOUT:
        {
            LogOut* loginOut = (LogOut *)szRecv;
            printf("收到<Socket = %d>请求：CMD_LOGOUT, 数据长度: %d，用户名称: %s\n",
                   clientSock ,loginOut->dataLength, loginOut->userName);
            //忽略判断用户名和密码
            LoginOutResult ret;
            send(clientSock, &ret, sizeof(LoginOutResult), 0);

        }
            break;
        default:
        {
            header->cmd = CMD_ERROR;
            header->dataLength = 0;
            send(clientSock, &header, sizeof(DataHeader), 0);
        }
            break;
    }

}

int EasyTCPServer::SendData(SOCKET client, DataHeader *header) {
    if (IsRun() && header)
        send(client, header, header->dataLength, 0);
    return SOCKET_ERROR;
}

void EasyTCPServer::SendDataToAll(DataHeader *header) {
    for (auto client: clients) {
        SendData(client, header);
    }
}

#endif //EASYTCPSERVER_EASYTCPSERVER_H
