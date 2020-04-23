//
// Created by 孙卓文 on 2020/4/18.
//

#ifndef EASYTCPSERVER_EASYTCPSERVER_H
#define EASYTCPSERVER_EASYTCPSERVER_H
#define FD_SETSIZE      3000
#include <iostream>
#include <stdio.h>
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
#include <string.h>
#include "DataStruct.h"
#include "CELLTimestamp.h"

class ClientSocket
{
public:
    explicit ClientSocket(SOCKET s = INVALID_SOCKET){
        _sockFd = s;
        memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
        _lastPos = 0;
    }
    SOCKET GetSock() {return _sockFd;}
    char *GetMsg() {return _szMsgBuf;}
    int GetPos()  {return _lastPos;}
    void SetPos(int pos) {_lastPos = pos;};
private:
    SOCKET _sockFd;
    //以下解决粘包和拆分包需要的变量
    //接收缓冲区
    static const int RECV_BUFF_SIZE = 10240;
    //第二缓冲区  消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE*10] = {};
    int _lastPos;                        //指向缓冲区有数据的末尾位置


};

class EasyTCPServer
{
public:
    explicit EasyTCPServer() {
        _sock = INVALID_SOCKET;
        _recvCount = 0;
    }
    virtual ~EasyTCPServer() {
        Close();
    }
    //初始化socket
    int InitSocket();
    //绑定IP, 端口号
    int Bind(const char *ip, unsigned short port);
    //监听端口号
    int Listen(int n);
    //接收客户端连接
    SOCKET Accept();
    //处理网络消息
    bool OnRun();
    //是否工作中
    bool IsRun();
    //接收数据  处理粘包 拆分包
    int RecvData(ClientSocket* clientSock);
    //响应网络消息
    virtual void OnNetMsg(SOCKET clientSock, DataHeader *header);
    //发送指定socket数据
    int SendData(SOCKET client, DataHeader *header);
    //群发
    void SendDataToAll(DataHeader *header);
    //关闭socket
    void Close();

private:
    SOCKET _sock;
    std::vector<ClientSocket*> _clients;
    //第一缓冲区（第二缓冲区封装在ClientSocket里面）
    static const int RECV_BUFF_SIZE = 10240;
    char _szRecv[RECV_BUFF_SIZE] = {};

    CELLTimeStamp _tTime;
    int _recvCount;

};

int EasyTCPServer::InitSocket() {
    // 1 建立一个socket
    if (_sock != INVALID_SOCKET) {
        printf("<socket = %d>关闭之前的连接...\n", _sock);
        Close();
    }
    _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_sock == INVALID_SOCKET) {
        printf("建立套接字失败...\n");
    } else {
        printf("建立套接字成功...\n");
    }
    return _sock;
}

int EasyTCPServer::Bind(const char *ip, unsigned short port) {
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

int EasyTCPServer::Listen(int n) {
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
        for (auto & _client : _clients) {
            close(_client->GetSock());
            delete _client;
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
        //NewUserJoin userJoin{};
        //SendDataToAll(&userJoin);
        _clients.push_back(new ClientSocket(clientSock));
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
        for (int i = (int) _clients.size() - 1; i >= 0; i--) {
            FD_SET(_clients[i]->GetSock(), &fdRead);//有没有客户需要接收
//            maxSock = std::max(maxSock, _clients[i]->GetSock());
            if (maxSock < _clients[i]->GetSock())
            {
                maxSock = _clients[i]->GetSock();
            }
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
            return true;
        }

        for (int n = (int) _clients.size() - 1; n >= 0; n--) {
            //如何客户socket数组中有读组操作，则说明有消息进来
            if (FD_ISSET(_clients[n]->GetSock(), &fdRead)) {
                if (-1 == RecvData(_clients[n])) {
                    auto iter = _clients.begin() + n;//std::vector<SOCKET>::iterator
                    if (iter != _clients.end()) {
                        delete _clients[n];
                        _clients.erase(iter);
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

int EasyTCPServer::RecvData(ClientSocket* clientSock) {
    int nLen = (int)recv(clientSock->GetSock(), _szRecv, RECV_BUFF_SIZE, 0);
    //auto* header = (DataHeader *)szRecv;
    if (nLen <= 0) {
        printf("客户端<Socket = %d>退出, 任务结束\n", clientSock->GetSock());
        return -1;
    }
    //将收取到的数据拷贝到消息缓冲区
    memcpy(clientSock->GetMsg() + clientSock->GetPos(), _szRecv, nLen);
    clientSock->SetPos(clientSock->GetPos()+nLen);
    //判断收到消息的长度是否大于消息头的长度，若大于消息头的长度，就可以取出消息体的长度
    while (clientSock->GetPos() >= sizeof(DataHeader)) {
        auto *header = (DataHeader*)clientSock->GetMsg();
        //判断收到消息的长度是否大于消息体的长度，若大于消息体的长度，就可以取出消息体
        if (clientSock->GetPos() >= header->dataLength) {
            //此时消息缓冲中有两部分数据，一部分是待处理的数据，第二部分是下一次处理的数据
            //消息缓冲区中未处理消息的长度
            int nSize = clientSock->GetPos() - header->dataLength;  //消息缓冲中收到的数据总长度减去将要处理的数据的长度
            OnNetMsg(clientSock->GetSock(), header);
            //通过掩盖的方法将处理过的数据清除
            memcpy(clientSock->GetMsg(), clientSock->GetMsg() + header->dataLength, nSize);
            clientSock->SetPos(nSize);
        } else {
            //剩余的消息虽然大于消息头但是小于消息体
            break;
        }
    }
    return 0;

}

void EasyTCPServer::OnNetMsg(SOCKET clientSock, DataHeader *header) {
    _recvCount++;
    auto t1 = _tTime.getElapsedSecond();
    if (t1 >= 1.0) {
        printf("time<%lf>, socket<%d> recvCount<%d>\n", t1, clientSock, _recvCount);
        _recvCount = 0;
        _tTime.update();
    }
    switch (header->cmd) {
        case CMD_LOGIN:
        {
            Login* login = (Login *)header;
            //printf("收到<Socket = %3d>请求：CMD_LOGIN, 数据长度: %d，用户名称: %s, 用户密码: %s\n",
            //       clientSock, login->dataLength, login->userName, login->passWord);
            //忽略判断用户名和密码
            //LoginResult ret;
            //send(clientSock, &ret, sizeof(LoginResult), 0);
        }
            break;
        case CMD_LOGOUT:
        {
            LogOut* loginOut = (LogOut *)header;
            //printf("收到<Socket = %3d>请求：CMD_LOGOUT, 数据长度: %d，用户名称: %s\n",
            //       clientSock ,loginOut->dataLength, loginOut->userName);
            //忽略判断用户名和密码
            //LoginOutResult ret;
            //send(clientSock, &ret, sizeof(LoginOutResult), 0);

        }
            break;
        default:
        {
            printf("收到<socket = %3d>未定义的消息，数据长度为: %d\n", clientSock, header->dataLength);
            //header->cmd = CMD_ERROR;
            //header->dataLength = 0;
            //send(clientSock, &header, sizeof(DataHeader), 0);
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
    for (auto client: _clients) {
        SendData(client->GetSock(), header);
    }
}

#endif //EASYTCPSERVER_EASYTCPSERVER_H


