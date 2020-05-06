//
// Created by 孙卓文 on 2020/5/6.
//

#ifndef EASYTCPSERVER_INETEVENT_H
#define EASYTCPSERVER_INETEVENT_H

#include "CELLPublicHeader.h"
#include "CELLClient.h"
class CELLServer;
//网络事件接口
class INETEvent
{
public:
    //客户端离开事件
    virtual void OnLeave(CELLClient* pClient) = 0;
    //客户端消息事件
    virtual void OnNetMsg(CELLServer* pCellServer, CELLClient* clientSock, DataHeader* header) = 0;
    //客户端加入时间
    virtual void OnJoin(CELLClient* CellClient) = 0;
    //recv事件
    virtual void OnNetRecv(CELLClient* pClient) = 0;

private:
};
#endif //EASYTCPSERVER_INETEVENT_H
