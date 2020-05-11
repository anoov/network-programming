#include <iostream>
#include "EasyTCPServer.h"
#include <thread>
bool g_bRun = true;
void cmdThread() {
    while (true) {
        // 3 输入请求命令
        char sendBuf[256] = {};
        //CELLLog::Info("请输入命令：");
        scanf("%s", sendBuf);

        // 4 处理请求
        if (0 == strcmp(sendBuf, "exit")) {
            g_bRun = false;
            CELLLog::Info("退出cmdThread线程！\n");
            break;
        } else {
            CELLLog::Info("收到不支持的命令，请重新输入！\n");
        }
    }
}

class MyServer : public EasyTCPServer
{
public:
    //cellServer 4 多个线程触发 不安全
    void OnLeave(CELLClientPtr pClient) override {
        EasyTCPServer::OnLeave(pClient);
        //std::cout << "客户端离开: " << pClient->GetSock()  << std::endl;
    }
    //只会被主线程触发 安全
    void OnJoin(CELLClientPtr clientSocket) override {
        EasyTCPServer::OnJoin(clientSocket);
        //std::cout << "客户端加入: " << clientSocket->GetSock()  << std::endl;
    }
    void OnNetRecv(CELLClientPtr pClient) override {
        EasyTCPServer::OnNetRecv(pClient);
    }
    //cellServer 4 多个线程触发 不安全
    void OnNetMsg(CELLServer* pCellServer, CELLClientPtr clientSock, DataHeader *header) override {
        EasyTCPServer::OnNetMsg(pCellServer, clientSock, header);
        switch (header->cmd) {
            case CMD_LOGIN:
            {
                //收到登录消息也心跳了一次
                clientSock->resetDTHeart();
                Login* login = (Login *)header;
                //CELLLog::Info("收到<Socket = %3d>请求：CMD_LOGIN, 数据长度: %d，用户名称: %s, 用户密码: %s\n",
                //       clientSock, login->dataLength, login->userName, login->passWord);
                //忽略判断用户名和密码
                LoginResult ret;
                if (SOCKET_ERROR ==  clientSock->SendData(&ret)) {
                    //发送缓冲区满了，消息没有发出去
                    CELLLog::Info("socket<%d> send full\n", clientSock->GetSock());
                    CELLLog::Info("socket<%d> send full\n", clientSock->GetSock());
                }
                ////使用收发分离
                //auto* ret = new LoginResult();
                //pCellServer->addSendTask(clientSock, ret);
            }
                break;
            case CMD_LOGOUT:
            {
                LogOut* loginOut = (LogOut *)header;
                //CELLLog::Info("收到<Socket = %3d>请求：CMD_LOGOUT, 数据长度: %d，用户名称: %s\n",
                //       clientSock ,loginOut->dataLength, loginOut->userName);
                //忽略判断用户名和密码
                auto* ret = new LoginOutResult();
                //clientSock->SendData(&ret);
                pCellServer->addSendTask(clientSock, ret);
            }
                break;

            case CMD_c2s_HEART:
            {
                //有收到心跳数据就重置心跳，说明该客户还活着
                clientSock->resetDTHeart();
                Heart_s2c_Test ret;
                clientSock->SendData(&ret);
            }
                break;
            default:
            {
                CELLLog::Info("收到<socket = %3d>未定义的消息，数据长度为: %d\n", clientSock->GetSock(), header->dataLength);
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

    CELLLog::Instance().SetLogPath("serverLog.txt", "w");

    MyServer server;
    server.InitSocket();
    //server.Bind("10.211.55.4", 4567);
    server.Bind("127.0.0.1", 4567);
    server.Listen(50);

    //启动消费者模型
    server.Start(4);


    //在主线程中等待用户输入命令
    while (true) {
        // 3 输入请求命令
        char sendBuf[256] = {};
        //CELLLog::Info("请输入命令：");
        scanf("%s", sendBuf);

        // 4 处理请求
        if (0 == strcmp(sendBuf, "exit")) {
            server.Close();
            CELLLog::Info("退出cmdThread线程！\n");
            break;
        } else {
            CELLLog::Info("收到不支持的命令，请重新输入！\n");
        }
    }


//    CELLServer task(1, 1);
//    task.Start();
//    std::chrono::milliseconds t(1000);
//    std::this_thread::sleep_for(t);
//    task.Close();

    return 0;
}
