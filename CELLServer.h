//
// Created by 孙卓文 on 2020/5/6.
//

#ifndef EASYTCPSERVER_CELLSERVER_H
#define EASYTCPSERVER_CELLSERVER_H

#include <vector>
#include "CELLPublicHeader.h"
#include "INetEvent.h"
#include "CELLClient.h"
#include "CELLSemaphore.h"
#include "CELLThread.h"


//网络消息接收处理服务类
class CELLServer
{
public:
    explicit CELLServer(int id, SOCKET sock = INVALID_SOCKET) {
        _id = id;
        _sock = sock;
        _pNetEvent = nullptr;

        _taskServer.serverId = id;
    }
    ~CELLServer() {
        printf("CELLServer<%d> ~CELLServer start\n", _id);
        //if(SOCKET_ERROR != _sock) {

            Close();
            //_sock = INVALID_SOCKET;
            //delete _pThread;
        //}
        printf("CELLServer<%d> ~CELLServer end\n", _id);


    }

    bool OnRun(CELLThread* pThread);

    bool IsRun();

    void Close();

    int RecvData(CELLClientPtr clientSock);

    void OnNetMsg(CELLClientPtr clientSock, DataHeader *header);

    void AddClient(CELLClientPtr pClient);

    void Start();
    //提供缓冲客户队列的大小
    size_t GetClientCount() const {
        return _clients.size() +  _clientsBuff.size();
    }

    void SetNetEvent(INETEvent* event) {
        _pNetEvent = event;
    }

    void addSendTask(CELLClientPtr pClient, DataHeader* data) {
        //auto* task = new CellSendMsg2ClientTask(pClient, data);
        _taskServer.addTask([pClient, data] () {
            pClient->SendData(data);
            //delete data;
        });
    }

    void OnClientLeave(int index) {
        auto iter = _clients.begin() + index;//std::vector<SOCKET>::iterator
        if (iter != _clients.end()) {
            _clients_change = true;
            if (_pNetEvent)
                _pNetEvent->OnLeave(_clients[index]);
            //delete _clients[n];
            //close((*iter)->GetSock());
            _clients.erase(iter);
        }
    }

    void ReadData(fd_set& fdRead) {
        for (int n = (int) _clients.size() - 1; n >= 0; n--) {
            //如何客户socket数组中有读组操作，则说明有消息进来
            if (FD_ISSET(_clients[n]->GetSock(), &fdRead)) {
                if (-1 == RecvData(_clients[n])) {
                    OnClientLeave(n);
                }
            }
        }
    }

    void WriteData(fd_set& fdWrite) {
        for (int n = (int) _clients.size() - 1; n >= 0; n--) {
            //如何客户socket数组中有读组操作，则说明有消息进来
            if (FD_ISSET(_clients[n]->GetSock(), &fdWrite)) {
                if (-1 == _clients[n]->SendDataNow()) {     //直接将缓冲区的数据发出去
                    OnClientLeave(n);
                }
            }
        }
    }

    void CheckTime() {
        auto nowTime = CELLTime::getNowInMilliSec();
        auto dt = nowTime - _oldTime;
        _oldTime = nowTime;

        for (auto iter = _clients.begin(); iter != _clients.end();) {
            //心跳检测
            if ((*iter)->checkHeart(dt)) {
                if (_pNetEvent)
                    _pNetEvent->OnLeave(*iter);
                _clients_change = true;
                //delete *iter;
                //close((*iter)->GetSock());
                iter = _clients.erase(iter);
                continue;
            }
//            //定时发送检测
//            (*iter)->checkSend(dt);
            iter ++;

        }

    }

private:
    SOCKET _sock;
    //正式客户队列
    std::vector<CELLClientPtr> _clients;
    //缓冲客户队列 应用与生产者消费者模型
    std::vector<CELLClientPtr> _clientsBuff;
    std::mutex _mutex;

    INETEvent* _pNetEvent;

    //用于改进 2020 04 29 CELLServer::OnRun
    //备份客户socket fd_set
    fd_set _fdRead_back;

    SOCKET _maxSock;

    //任务服务类型，
    CellTaskServer _taskServer;

    //旧时间戳
    time_t _oldTime = CELLTime::getNowInMilliSec();

    //自定义线程ID；
    int _id = -1;

    //客户列表是否有变化
    bool _clients_change;

    //线程
    CELLThread _thread;


};

bool CELLServer::OnRun(CELLThread* pThread) {
    _clients_change = true;
    while (pThread->isRun()) {
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
            //旧的时间戳
            _oldTime = CELLTime::getNowInMilliSec();

            continue;
        }
        //伯克利 socket
        //第一个参数：集合中所有描述符的范围而不是数量，集合中最大值加一
        //第二个参数：读描述符集合，告诉内核需要查询的需要读的套接字的集合
        //第三个参数：写描述符集合
        //第三个参数：异常描述符集合
        //第四个参数：时间，在该时间内没有返回，则返回
        fd_set fdRead;
        fd_set fdWrite;
        //fd_set fdExc;
        if (_clients_change) {
            _clients_change = false;
            FD_ZERO(&fdRead);   //清空集合中的数据
            _maxSock = _clients[0]->GetSock();
            for (int i = (int) _clients.size() - 1; i >= 0; i--) {
                FD_SET(_clients[i]->GetSock(), &fdRead);//将客户端集合放在读描述符集合中
                _maxSock = std::max(_maxSock, _clients[i]->GetSock());
            }
            memcpy(&_fdRead_back, &fdRead, sizeof(_fdRead_back));
        } else {
            memcpy(&fdRead, &_fdRead_back, sizeof(_fdRead_back));
        }
        //因为可写和异常集合和可读集合是同样的，在可读做计算后可直接拿来用
        memcpy(&fdWrite, &_fdRead_back, sizeof(_fdRead_back));
        //memcpy(&fdExc, &_fdRead_back, sizeof(_fdRead_back));


        timeval t = {0, 1};
        //若最后一个参数设置为null，则程序会阻塞到select这里，一直等到有数据可处理
        //select监视三个集合中的所有描述符，在这里是套接字
        //例如select的第二个参数读集合，
        //若集合中的某一个socket有读操作，则保持该操作位，否则该操作位清零
        int ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t); //每个线程的任务只是查询消息，采用阻塞方式更好

        if (ret < 0) {
            printf("CELLServer<%d>.OnRun.select Error, exit\n", _id);
            //Close();  //OnRun内部调用Close会调用信号量等待函数，而此时OnRun并一直阻塞不回退出，也就不回调用唤醒函数
            pThread->Exit();
            break;
        } else if (ret == 0) {
            //ret == 0说明没有可处理的数据，下面的代码跳过
            continue;
        }

        //接收数据
        ReadData(fdRead);
        //写数据
        WriteData(fdWrite);
        //通过写操作将异常表现出来
        //WriteData(fdExc);
        CheckTime();


    }
    printf("CELLServer<%d> OnRun exit\n", _id);

    return false ;
}

bool CELLServer::IsRun() {
    return _sock != INVALID_SOCKET;
}

void CELLServer::Close() {
    printf("CELLServer<%d> close start\n", _id);
    _taskServer.close();
    _thread.Close();
    printf("CELLServer<%d> close end\n", _id);
}

int CELLServer::RecvData(CELLClientPtr clientSock) {
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
    int nLen = (int)recv(clientSock->GetSock(), szRecv, clientSock->RECV_BUFF_SIZE - clientSock->GetPos(), 0);
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

void CELLServer::OnNetMsg(CELLClientPtr clientSock, DataHeader *header) {
    _pNetEvent->OnNetMsg(this, clientSock, header);
}

void CELLServer::AddClient(CELLClientPtr pClient) {
    std::lock_guard<std::mutex> lock(_mutex);
    _clientsBuff.push_back(pClient);
}

void CELLServer::Start() {
    _taskServer.Start(); //启动消息发送线程
    _thread.Start(nullptr,
                  [this](CELLThread* pThread){OnRun(pThread);},
                  nullptr);
}

#endif //EASYTCPSERVER_CELLSERVER_H
