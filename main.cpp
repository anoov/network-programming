#include <iostream>
#include "EasyTCPServer.h"
#include <thread>
bool g_bRun = true;
void cmdThread() {
    while (true) {
        // 3 输入请求命令
        char sendBuf[256] = {};
        //printf("请输入命令：");
        scanf("%s", sendBuf);

        // 4 处理请求
        if (0 == strcmp(sendBuf, "exit")) {
            g_bRun = false;
            printf("退出cmdThread线程！\n");
            break;
        } else {
            printf("收到不支持的命令，请重新输入！\n");
        }
    }
}

class MyServer : public EasyTCPServer
{
public:
    //cellServer 4 多个线程触发 不安全
    void OnLeave(ClientSocket* pClient) override {
        EasyTCPServer::OnLeave(pClient);
        //std::cout << "客户端离开: " << pClient->GetSock()  << std::endl;
    }
    //只会被主线程触发 安全
    void OnJoin(ClientSocket *clientSocket) override {
        EasyTCPServer::OnJoin(clientSocket);
        //std::cout << "客户端加入: " << clientSocket->GetSock()  << std::endl;
    }
    void OnNetRecv(ClientSocket* pClient) override {
        EasyTCPServer::OnNetRecv(pClient);
    }
    //cellServer 4 多个线程触发 不安全
    void OnNetMsg(CellServer* pCellServer, ClientSocket* clientSock, DataHeader *header) override {
        EasyTCPServer::OnNetMsg(pCellServer, clientSock, header);
        switch (header->cmd) {
            case CMD_LOGIN:
            {
                Login* login = (Login *)header;
                //printf("收到<Socket = %3d>请求：CMD_LOGIN, 数据长度: %d，用户名称: %s, 用户密码: %s\n",
                //       clientSock, login->dataLength, login->userName, login->passWord);
                //忽略判断用户名和密码
                auto* ret = new LoginResult();
                //clientSock->SendData(&ret);
                pCellServer->addSendTask(clientSock, ret);
            }
                break;
            case CMD_LOGOUT:
            {
                LogOut* loginOut = (LogOut *)header;
                //printf("收到<Socket = %3d>请求：CMD_LOGOUT, 数据长度: %d，用户名称: %s\n",
                //       clientSock ,loginOut->dataLength, loginOut->userName);
                //忽略判断用户名和密码
                auto* ret = new LoginOutResult();
                //clientSock->SendData(&ret);
                pCellServer->addSendTask(clientSock, ret);
            }
                break;
            default:
            {
                printf("收到<socket = %3d>未定义的消息，数据长度为: %d\n", clientSock->GetSock(), header->dataLength);
                header->cmd = CMD_ERROR;
                header->dataLength = 0;
                //clientSock->SendData(header);
                //pCellServer->addSendTask(clientSock, header);
            }
                break;
        }
    }
};

int main() {
    MyServer server;
    server.InitSocket();
//    server.Bind("10.211.55.4", 4567);
    server.Bind("127.0.0.1", 4567);
    server.Listen(50);

    //启动消费者模型
    server.Start(4);

    //启动线程函数
    std::thread t1(cmdThread);
    t1.detach();
    while (g_bRun) {
        server.OnRun();
    }
    server.Close();
    return 0;
}
