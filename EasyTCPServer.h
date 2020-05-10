//
// Created by 孙卓文 on 2020/4/18.
//

#ifndef EASYTCPSERVER_EASYTCPSERVER_H
#define EASYTCPSERVER_EASYTCPSERVER_H

//#define FD_SETSIZE      3000
#include "CELLPublicHeader.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include "CELLServer.h"
#include "CELLClient.h"
#include "CELLThread.h"
#include "INetEvent.h"


//接收客户端服务
class EasyTCPServer : public INETEvent
{
public:
    explicit EasyTCPServer() {
        _sock = INVALID_SOCKET;
        _msgCount = 0;
        _clientCount = 0;
        _recvCount = 0;
    }
    virtual ~EasyTCPServer() {
        if (SOCKET_ERROR != _sock) {
            Close();
        }
    }
    //初始化socket
    int InitSocket();
    //绑定IP, 端口号
    int Bind(const char *ip, unsigned short port);
    //监听端口号
    int Listen(int n);
    //接收客户端连接
    SOCKET Accept();
    //
    void Start(int threadCount = 1);
    //关闭socket
    void Close();

    void addClientToCellServer(CELLClientPtr pClient);

    void OnLeave(CELLClientPtr pClient) override;

    void OnNetMsg(CELLServer* pCellServer, CELLClientPtr clientSock, DataHeader* header) override ;

    void OnJoin(CELLClientPtr CellClient) override ;

    void OnNetRecv(CELLClientPtr pClient) override ;

private:
    //处理网络消息
    bool OnRun(CELLThread* pThread);
    //计算并输出每秒收到的网络消息
    void time4msg();
private:
    SOCKET _sock;
    //消息处理对象，内部创建线程
    std::vector<CELLServer*> _cellServer;

    //每秒消息计时
    CELLTimeStamp _tTime;

    CELLThread _thread;

protected:
    std::atomic_int _msgCount{};
    //客户端进入计数
    std::atomic_int _clientCount{};

    std::atomic_int _recvCount{};

};

int EasyTCPServer::InitSocket() {
#ifndef _win32
    ///在Linux中，若客户端或者服务器端断开连接时会接收到一个信号，这个信号会触发进程终止
    ///在收到该信号时无作为
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return 1;
#endif
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
    printf("EasyTCPServer close start\n");
    _thread.Close();
    if (_sock != INVALID_SOCKET) {
        for (auto s : _cellServer) {
            delete s;
        }
        _cellServer.clear();
        close(_sock);
        _sock = INVALID_SOCKET;
    }
    printf("EasyTCPServer close end\n");

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
        //将新加入来的客户加入到队列中
        auto newClient = std::make_shared<CELLClient>(clientSock);
        addClientToCellServer(newClient);
    }
    return clientSock;
}

void EasyTCPServer::addClientToCellServer(CELLClientPtr pClient) {

    //查找消费者中队列size最小的，将新客户加入
    auto pMinServer = _cellServer[0];
    for (auto& pCellServer: _cellServer) {
        if (pMinServer->GetClientCount() > pCellServer->GetClientCount())
            pMinServer = pCellServer;
    }
    pMinServer->AddClient(pClient);
    OnJoin(pClient);
}

bool EasyTCPServer::OnRun(CELLThread* pThread) {
    while (pThread->isRun()) {
        time4msg();
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
        //FD_ZERO(&fdWrite);   //清空集合中的数据
        //FD_ZERO(&fdExcept);   //清空集合中的数据

        FD_SET(_sock, &fdRead);      //让内核代理查看socket有没有读操作
        //FD_SET(_sock, &fdWrite);     //让内核代理查看socket有没有写操作
        //FD_SET(_sock, &fdExcept);    //让内核代理查看socket有没有异常操作

        timeval t = {0, 10};
        //若最后一个参数设置为null，则程序会阻塞到select这里，一直等到有数据可处理
        //select监视三个集合中的所有描述符，在这里是套接字
        //例如select的第二个参数读集合，
        //若集合中的某一个socket有读操作，则保持该操作位，否则该操作位清零
        int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);

        if (ret < 0) {
            printf("EasyTCPServer<%d>.Accept select发生错误, 任务结束\n", _sock);
            //Close();
            pThread->Exit();
            break;
        }

        //若本socket有读操作，意味着有客户机连进来
        if (FD_ISSET(_sock, &fdRead)) {
            FD_CLR(_sock, &fdRead);

            //4 accept 等待接受客户连接
            Accept();
            //return true;
        }
        //return true;
    }
    return false ;
}

void EasyTCPServer::time4msg() {
    auto t1 = _tTime.getElapsedSecond();
    if (t1 >= 1.0) {

        printf("thread<%lu>, time<%lf>, socket<%d>, clients<%d>, recvCount<%d>, msgCount<%d>\n",
                _cellServer.size(), t1, _sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount/t1));
        _tTime.update();
        _msgCount = 0;
        _recvCount = 0;
    }

}

void EasyTCPServer::Start(int threadCount) {
    for (int i = 0; i < threadCount; i++) {
        auto ser = new CELLServer(i+1, _sock);
        _cellServer.push_back(ser);
        //注册网络事件接收对象
        ser->SetNetEvent(this);
        //启动消息接收处理线程
        ser->Start();
    }

    _thread.Start(nullptr,
                  [this](CELLThread* pThread){OnRun(pThread);},
                  nullptr);

}

//cellServer 4 多个线程触发 不安全
void EasyTCPServer::OnLeave(CELLClientPtr pClient) {
    _clientCount--;
}
//cellServer 4 多个线程触发 不安全
void EasyTCPServer::OnNetMsg(CELLServer* pCellServer, CELLClientPtr clientSock, DataHeader *header) {
    _msgCount++;
//    switch (header->cmd) {
//        case CMD_LOGIN:
//        {
//            Login* login = (Login *)header;
//            //printf("收到<Socket = %3d>请求：CMD_LOGIN, 数据长度: %d，用户名称: %s, 用户密码: %s\n",
//            //       clientSock, login->dataLength, login->userName, login->passWord);
//            //忽略判断用户名和密码
//            LoginResult ret;
//            clientSock->SendData(&ret);
//        }
//            break;
//        case CMD_LOGOUT:
//        {
//            LogOut* loginOut = (LogOut *)header;
//            //printf("收到<Socket = %3d>请求：CMD_LOGOUT, 数据长度: %d，用户名称: %s\n",
//            //       clientSock ,loginOut->dataLength, loginOut->userName);
//            //忽略判断用户名和密码
//            LoginOutResult ret;
//            clientSock->SendData(&ret);
//        }
//            break;
//        default:
//        {
//            printf("收到<socket = %3d>未定义的消息，数据长度为: %d\n", clientSock->GetSock(), header->dataLength);
//            header->cmd = CMD_ERROR;
//            header->dataLength = 0;
//            clientSock->SendData(header);
//        }
//            break;
//    }
}
//只会被主线程触发 安全
void EasyTCPServer::OnJoin(CELLClientPtr CellClient) {
    _clientCount++;
}

void EasyTCPServer::OnNetRecv(CELLClientPtr pClient) {
    _recvCount++;
}


#endif //EASYTCPSERVER_EASYTCPSERVER_H
