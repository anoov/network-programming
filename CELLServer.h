//
// Created by 孙卓文 on 2020/5/6.
//

#ifndef EASYTCPSERVER_CELLSERVER_H
#define EASYTCPSERVER_CELLSERVER_H

#include <vector>
#include "CELLPublicHeader.h"
#include "INetEvent.h"
#include "CELLClient.h"

//网络消息发送任务
class CellSendMsg2ClientTask : public CellTask
{
public:
    CellSendMsg2ClientTask(CELLClient* c, DataHeader* h): _pClient(c), _pHeader(h){}
    //virtual ~CellSendMsg2ClientTask();

    int doTask() override {
        int ret = _pClient->SendData(_pHeader);
        delete _pHeader;
        return ret;
    }
private:
    CELLClient* _pClient;
    DataHeader* _pHeader;
};

//网络消息接收处理服务类
class CELLServer
{
public:
    explicit CELLServer(SOCKET sock = INVALID_SOCKET) {
        _sock = sock;
        _pThread = nullptr;
        _pNetEvent = nullptr;
    }
    ~CELLServer() {
        Close();
        _sock = INVALID_SOCKET;
        //delete _pThread;
    }

    bool OnRun();

    bool IsRun();

    void Close();

    int RecvData(CELLClient* clientSock);

    virtual void OnNetMsg(CELLClient* clientSock, DataHeader *header);

    void AddClient(CELLClient* pClient);

    void Start();
    //提供缓冲客户队列的大小
    size_t GetClientCount() const {
        return _clients.size() +  _clientsBuff.size();
    }

    void SetNetEvent(INETEvent* event) {
        _pNetEvent = event;
    }

    void addSendTask(CELLClient* pClient, DataHeader* data) {
        auto* task = new CellSendMsg2ClientTask(pClient, data);
        _taskServer.addTask(task);
    }

private:
    SOCKET _sock;
    //正式客户队列
    std::vector<CELLClient*> _clients;
    //缓冲客户队列 应用与生产者消费者模型
    std::vector<CELLClient*> _clientsBuff;
    std::mutex _mutex;

    static const int RECV_BUFF_SIZE = 10240;
    char _szRecv[RECV_BUFF_SIZE] = {};

    std::shared_ptr<std::thread>  _pThread;

    INETEvent* _pNetEvent;

    //用于改进 2020 04 29 CELLServer::OnRun
    //备份客户socket fd_set
    fd_set _fdRead_back;
    //客户列表是否有变化
    bool _clients_change;
    SOCKET _maxSock;

    //任务服务类型，
    CellTaskServer _taskServer;

};

bool CELLServer::OnRun() {
    _clients_change = true;
    while (IsRun()) {
        if (!_clientsBuff.empty()) {
            //从缓冲队列中取出客户数据
            std::lock_guard<std::mutex> lockGuard(_mutex);
            for (auto pClient: _clientsBuff) {
                _clients.push_back(pClient);
            }
            _clientsBuff.clear();
            _clients_change = true;
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
        if (_clients_change) {
            _clients_change = false;
            _maxSock = _clients[0]->GetSock();
            for (int i = (int) _clients.size() - 1; i >= 0; i--) {
                FD_SET(_clients[i]->GetSock(), &fdRead);//有没有客户需要接收
                _maxSock = std::max(_maxSock, _clients[i]->GetSock());
            }
            memcpy(&_fdRead_back, &fdRead, sizeof(_fdRead_back));
        } else {
            memcpy(&fdRead, &_fdRead_back, sizeof(_fdRead_back));
        }

        timeval t = {1, 0};
        //若最后一个参数设置为null，则程序会阻塞到select这里，一直等到有数据可处理
        //select监视三个集合中的所有描述符，在这里是套接字
        //例如select的第二个参数读集合，
        //若集合中的某一个socket有读操作，则保持该操作位，否则该操作位清零
        int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr); //每个线程的任务只是查询消息，采用阻塞方式更好

        if (ret < 0) {
            printf("select发生错误, 任务结束\n");
            Close();
            return false;
        } else if (ret == 0) {
            //ret == 0说明没有可处理的数据，下面的代码跳过
            continue;
        }

        for (int n = (int) _clients.size() - 1; n >= 0; n--) {
            //如何客户socket数组中有读组操作，则说明有消息进来
            if (FD_ISSET(_clients[n]->GetSock(), &fdRead)) {
                if (-1 == RecvData(_clients[n])) {
                    auto iter = _clients.begin() + n;//std::vector<SOCKET>::iterator
                    if (iter != _clients.end()) {
                        _clients_change = true;
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

bool CELLServer::IsRun() {
    return _sock != INVALID_SOCKET;
}

void CELLServer::Close() {
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

int CELLServer::RecvData(CELLClient* clientSock) {
//    int nLen = (int)recv(clientSock->GetSock(), _szRecv, RECV_BUFF_SIZE, 0);
//    _pNetEvent->OnNetRecv(clientSock);
//    if (nLen <= 0) {
//        //printf("客户端<Socket = %d>退出, 任务结束\n", clientSock->GetSock());
//        return -1;
//    }
//    //将收取到的数据拷贝到消息缓冲区
//    memcpy(clientSock->GetMsg() + clientSock->GetPos(), _szRecv, nLen);
//    clientSock->SetPos(clientSock->GetPos()+nLen);

    //取消第一缓冲区，即取消水舀子，留下水缸
    char* szRecv = clientSock->GetMsg() + clientSock->GetPos();
    int nLen = (int)recv(clientSock->GetSock(), szRecv, RECV_BUFF_SIZE - clientSock->GetPos(), 0);
    _pNetEvent->OnNetRecv(clientSock);
    if (nLen <= 0) {
        //printf("客户端<Socket = %d>退出, 任务结束\n", clientSock->GetSock());
        return -1;
    }
    clientSock->SetPos(clientSock->GetPos() + nLen);
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

void CELLServer::OnNetMsg(CELLClient* clientSock, DataHeader *header) {
    _pNetEvent->OnNetMsg(this, clientSock, header);
}

void CELLServer::AddClient(CELLClient* pClient) {
    std::lock_guard<std::mutex> lock(_mutex);
    _clientsBuff.push_back(pClient);
}

void CELLServer::Start() {
    _pThread = std::shared_ptr<std::thread>(new std::thread(std::mem_fn(&CELLServer::OnRun), this));
    _taskServer.Start(); //启动消息发送线程
    //_pThread->detach();
}

#endif //EASYTCPSERVER_CELLSERVER_H
