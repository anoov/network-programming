//
// Created by 孙卓文 on 2020/4/18.
//

#ifndef EASYTCPSERVER_EASYTCPSERVER_H
#define EASYTCPSERVER_EASYTCPSERVER_H
//#define FD_SETSIZE      3000
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
#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <functional>
#include "DataStruct.h"
#include "CELLTimestamp.h"

//客户端数据类型
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
    void SetPos(int pos) {_lastPos = pos;}
    int SendData(DataHeader *header) {
        if (header) {
            return send(_sockFd, header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }
private:
    SOCKET _sockFd;
    //以下解决粘包和拆分包需要的变量
    //接收缓冲区
    static const int RECV_BUFF_SIZE = 10240;
    //第二缓冲区  消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE*10] = {};
    int _lastPos;                        //指向缓冲区有数据的末尾位置
};

//网络事件接口
class INETEvent
{
public:
    //客户端离开事件
    virtual void OnLeave(ClientSocket* pClient) = 0;
    //客户端消息事件
    virtual void OnNetMsg(ClientSocket* clientSock, DataHeader* header) = 0;
    //客户端加入时间
    virtual void OnJoin(ClientSocket* clientSocket) = 0;
private:
};

class CellServer
{
public:
    explicit CellServer(SOCKET sock = INVALID_SOCKET) {
        _sock = sock;
        _pThread = nullptr;
        _pNetEvent = nullptr;
    }
    ~CellServer() {
        Close();
        _sock = INVALID_SOCKET;
        //delete _pThread;
    }

    bool OnRun();

    bool IsRun();

    void Close();

    int RecvData(ClientSocket* clientSock);

    virtual void OnNetMsg(ClientSocket* clientSock, DataHeader *header);

    void AddClient(ClientSocket* pClient);

    void Start();
    //提供缓冲客户队列的大小
    size_t GetClientCount() const {
        return _clients.size() +  _clientsBuff.size();
    }

    void SetNetEvent(INETEvent* event) {
        _pNetEvent = event;
    }

private:
    SOCKET _sock;
    //正式客户队列
    std::vector<ClientSocket*> _clients;
    //缓冲客户队列 应用与生产者消费者模型
    std::vector<ClientSocket*> _clientsBuff;
    std::mutex _mutex;

    static const int RECV_BUFF_SIZE = 10240;
    char _szRecv[RECV_BUFF_SIZE] = {};

    std::shared_ptr<std::thread>  _pThread;

    INETEvent* _pNetEvent;

};

bool CellServer::OnRun() {
    while (IsRun()) {
        if (!_clientsBuff.empty()) {
            //从缓冲队列中取出客户数据
            std::lock_guard<std::mutex> lockGuard(_mutex);
            for (auto pClient: _clientsBuff) {
                _clients.push_back(pClient);
            }
            _clientsBuff.clear();
        }
        //如何没有需要处理的客户端，跳过以下代码
        if (_clients.empty()) {
            std::chrono::milliseconds t(1);
            std::this_thread::sleep_for(t);
            continue;
        }
        //伯克利 socket
        //第一个参数：集合中所有描述符的范围而不是数量，集合中最大值加一
        //第二个参数：读描述符集合，告诉内核需要查询的需要读的套接字的集合
        //第三个参数：写描述符集合
        //第三个参数：异常描述符集合
        //第四个参数：时间，在该时间内没有返回，则返回
        fd_set fdRead;
        FD_ZERO(&fdRead);   //清空集合中的数据

//        FD_SET(_sock, &fdRead);      //让内核代理查看socket有没有读操作
//        FD_SET(_sock, &fdWrite);     //让内核代理查看socket有没有写操作
//        FD_SET(_sock, &fdExcept);    //让内核代理查看socket有没有异常操作

        SOCKET maxSock = _clients[0]->GetSock();
        for (int i = (int) _clients.size() - 1; i >= 0; i--) {
            FD_SET(_clients[i]->GetSock(), &fdRead);//有没有客户需要接收
            maxSock = std::max(maxSock, _clients[i]->GetSock());
        }

        timeval t = {1, 0};
        //若最后一个参数设置为null，则程序会阻塞到select这里，一直等到有数据可处理
        //select监视三个集合中的所有描述符，在这里是套接字
        //例如select的第二个参数读集合，
        //若集合中的某一个socket有读操作，则保持该操作位，否则该操作位清零
        int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);

        if (ret < 0) {
            printf("select发生错误, 任务结束\n");
            Close();
            return false;
        }

        for (int n = (int) _clients.size() - 1; n >= 0; n--) {
            //如何客户socket数组中有读组操作，则说明有消息进来
            if (FD_ISSET(_clients[n]->GetSock(), &fdRead)) {
                if (-1 == RecvData(_clients[n])) {
                    auto iter = _clients.begin() + n;//std::vector<SOCKET>::iterator
                    if (iter != _clients.end()) {
                        if (_pNetEvent)
                            _pNetEvent->OnLeave(_clients[n]);
                        delete _clients[n];
                        _clients.erase(iter);
                    }
                }
            }
        }
    }
    return false ;
}

bool CellServer::IsRun() {
    return _sock != INVALID_SOCKET;
}

void CellServer::Close() {
    if (_sock != INVALID_SOCKET) {
        for (auto&  _client : _clients) {
            close(_client->GetSock());
            delete _client;
        }
        close(_sock);
        _sock = INVALID_SOCKET;
        _clients.clear();
    }
}

int CellServer::RecvData(ClientSocket* clientSock) {
    int nLen = (int)recv(clientSock->GetSock(), _szRecv, RECV_BUFF_SIZE, 0);
    //auto* header = (DataHeader *)szRecv;
    if (nLen <= 0) {
        //printf("客户端<Socket = %d>退出, 任务结束\n", clientSock->GetSock());
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
            OnNetMsg(clientSock, header);
            //_pNetEvent->OnNetMsg(clientSock->GetSock(), header);
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

void CellServer::OnNetMsg(ClientSocket* clientSock, DataHeader *header) {
    _pNetEvent->OnNetMsg(clientSock, header);
}

void CellServer::AddClient(ClientSocket* pClient) {
    std::lock_guard<std::mutex> lock(_mutex);
    _clientsBuff.push_back(pClient);
}

void CellServer::Start() {
    _pThread = std::shared_ptr<std::thread>(new std::thread(std::mem_fn(&CellServer::OnRun), this));
    //_pThread->detach();
}



class EasyTCPServer : public INETEvent
{
public:
    explicit EasyTCPServer() {
        _sock = INVALID_SOCKET;
        _recvCount = 0;
        _clientCount = 0;
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
    //
    void Start(int threadCount = 1);
    //关闭socket
    void Close();
    //处理网络消息
    bool OnRun();
    //是否工作中
    bool IsRun();
    //计算并输出每秒收到的网络消息
    void time4msg();

    void addClientToCellServer(ClientSocket* pClient);

    void OnLeave(ClientSocket* pClient) override;

    void OnNetMsg(ClientSocket* clientSock, DataHeader* header) override ;

    void OnJoin(ClientSocket* clientSocket) override ;

private:
    SOCKET _sock;
    //消息处理对象，内部创建线程
    std::vector<CellServer*> _cellServer;
    //第一缓冲区（第二缓冲区封装在ClientSocket里面）
    static const int RECV_BUFF_SIZE = 10240;
    char _szRecv[RECV_BUFF_SIZE] = {};

    //static const int CELL_SERVER_THREAD_COUNT = 4;

    //每秒消息计时
    CELLTimeStamp _tTime;

protected:
    std::atomic_int _recvCount{};
    //客户端进入计数
    std::atomic_int _clientCount{};

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
        close(_sock);
        //_sock = INVALID_SOCKET;
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
        //将新加入来的客户加入到队列中
        addClientToCellServer(new ClientSocket(clientSock));
    }
    return clientSock;
}

void EasyTCPServer::addClientToCellServer(ClientSocket* pClient) {

    //查找消费者中队列size最小的，将新客户加入
    auto pMinServer = _cellServer[0];
    for (auto& pCellServer: _cellServer) {
        if (pMinServer->GetClientCount() > pCellServer->GetClientCount())
            pMinServer = pCellServer;
    }
    pMinServer->AddClient(pClient);
    OnJoin(pClient);
}

bool EasyTCPServer::OnRun() {
    if (IsRun()) {
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
        return true;
    }
    return false ;
}

bool EasyTCPServer::IsRun() {
    return _sock != INVALID_SOCKET;
}

void EasyTCPServer::time4msg() {
    auto t1 = _tTime.getElapsedSecond();
    if (t1 >= 1.0) {
        printf("thread<%lu>, time<%lf>, socket<%d>, clients<%d>, recvCount<%f>\n",_cellServer.size(), t1, _sock, (int)_clientCount, _recvCount/t1);
        _tTime.update();
        _recvCount = 0;
    }

}


void EasyTCPServer::Start(int threadCount) {
    for (int i = 0; i < threadCount; i++) {
        auto ser = new CellServer(_sock);
        _cellServer.push_back(ser);
        //注册网络事件接收对象
        ser->SetNetEvent(this);
        //启动消息处理线程
        ser->Start();
    }
}
//cellServer 4 多个线程触发 不安全
void EasyTCPServer::OnLeave(ClientSocket* pClient) {
    _clientCount--;
}
//cellServer 4 多个线程触发 不安全
void EasyTCPServer::OnNetMsg(ClientSocket* clientSock, DataHeader *header) {
    _recvCount++;
}
//只会被主线程触发 安全
void EasyTCPServer::OnJoin(ClientSocket *clientSocket) {
    _clientCount++;
}


#endif //EASYTCPSERVER_EASYTCPSERVER_H

