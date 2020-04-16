//
// Created by 孙卓文 on 2020/4/5.
//
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#include <thread>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};

struct DataHeader
{
    short dataLength;  //数据长度
    short cmd;         //命令
};

struct Login: public DataHeader
{
    Login(){
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char passWord[32];
};
struct LoginResult: public DataHeader
{
    LoginResult(){
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 1;
    }
    int result;
};
struct LogOut: public DataHeader
{
    LogOut(){
        dataLength = sizeof(LogOut);
        cmd = CMD_LOGOUT;
    }
    char userName[32];
};
struct LoginOutResult: public DataHeader
{
    LoginOutResult(){
        dataLength = sizeof(LoginOutResult);
        cmd = CMD_LOGOUT_RESULT;
        result = 1;
    }
    int result;
};
struct NewUserJoin : public DataHeader
{
    NewUserJoin()
    {
        dataLength = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        scok = 0;
    }
    int scok;
};

int process(SOCKET sock)  {

    //5 接收客户端数据
    //5.1 接受数据头
    //使用缓冲区来接受数据
    char szRecv[4096] = {};
    int nLen = recv(sock, szRecv, sizeof(DataHeader), 0);
    auto* header = (DataHeader *)szRecv;
    if (nLen <= 0) {
        printf("与服务器断开连接, 任务结束\n");
        return -1;
    }

    switch (header->cmd) {
        case CMD_LOGIN_RESULT:
        {
            recv(sock, szRecv+sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LoginResult* login = (LoginResult *)szRecv;
            printf("收到服务器<Socket = %d>消息：CMD_LOGIN_RESULT, 数据长度: %d，用户名称: %d\n",
                   sock, login->dataLength, login->result);
        }
            break;
        case CMD_LOGOUT_RESULT:
        {
            recv(sock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            LoginOutResult* loginOut = (LoginOutResult *)szRecv;
            printf("收到服务器<Socket = %d>消息：CMD_LOGOUT_RESULT, 数据长度: %d\n",
                   sock ,loginOut->dataLength);
        }
            break;
        case CMD_NEW_USER_JOIN:
        {
            recv(sock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            NewUserJoin* newJoin = (NewUserJoin *)szRecv;
            printf("收到服务器<Socket = %d>消息：NEW_USER_JOIN, 数据长度: %d\n",
                   sock ,newJoin->dataLength);
        }
            break;
        default:
        {
            header->cmd = CMD_ERROR;
            header->dataLength = 0;
            send(sock, &header, sizeof(DataHeader), 0);
        }
            break;
    }
    return 0;
}
bool g_bRun = true;
void cmdThread(SOCKET sock) {
    while (true) {
        // 3 输入请求命令
        char sendBuf[256] = {};
        sleep(1);
        printf("请输入命令：");
        scanf("%s", sendBuf);

        // 4 处理请求
        if (0 == strcmp(sendBuf, "exit")) {
            printf("退出cmdThread线程！\n");
            g_bRun = false;
            break;
        } else if (0 == strcmp(sendBuf, "login")) {
            Login login;
            strcpy(login.userName, "lyd");
            strcpy(login.passWord, "lydmima");
            send(sock, &login, sizeof(login), 0);

        } else if (0 == strcmp(sendBuf, "logout")) {
            LogOut logout;
            strcpy(logout.userName, "lyd");
            send(sock, &logout, sizeof(logout), 0);
        } else {
            printf("收到不支持的命令，请重新输入！\n");
        }
    }

}

int main_fun(){
    // 1 建立一个socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("建立套接字失败...\n");
    }else {
        printf("建立套接字成功...\n");
    }
    // 2 连接服务器  connect
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(8000);
    _sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    int ret = connect(sock, (sockaddr *)&_sin, sizeof(sockaddr));
    if (ret == SOCKET_ERROR) {
        printf("连接失败...\n");
    }else {
        printf("连接成功...\n");
    }

//    启动线程函数
    std::thread t1(cmdThread, sock);
//    t1.detach();


    while (g_bRun) {

        fd_set fdReader;
        FD_ZERO(&fdReader);
        FD_SET(sock, &fdReader);
        timeval t = {1, 0};
        int selectRet = select(sock+1, &fdReader, nullptr, nullptr, &t);
        if (selectRet < 0) {
            printf("select发生错误, 任务结束\n");
            break;
        }
        if (FD_ISSET(sock, &fdReader)) {
            FD_CLR(sock, &fdReader);
            if (-1 == process(sock)) {
                printf("select任务结束!\n");
            }
        }
//        std::cout << "空闲时间处理其他业务" << std::endl;


//        // 3 输入请求命令
//        char sendBuf[256] = {};
//        printf("请输入命令：");
//        scanf("%s", sendBuf);
///        scanf是阻塞操作，有碍于测试

//
//        // 4 处理请求
//        if (0 == strcmp(sendBuf, "exit")) {
//            printf("收到exit命令，任务结束！");
//            break;
//        }
//        else if (0 == strcmp(sendBuf, "login")) {
//            Login login;
//            strcpy(login.userName, "lyd");
//            strcpy(login.passWord, "lydmima");
//            send(sock, &login, sizeof(login), 0);
//            //接受服务器返回的数据
//            LoginResult loginRet = {};
//            recv(sock, &loginRet, sizeof(loginRet), 0);
//            printf("LoginResult: %d\n", loginRet.result);
//
//        }
//        else if (0 == strcmp(sendBuf, "logout")) {
//            LogOut logout;
//            strcpy(logout.userName, "lyd");
//            send(sock, &logout, sizeof(logout), 0);
//            //接受服务器返回的数据
//            LoginOutResult logoutRet = {};
//            recv(sock, &logoutRet, sizeof(logoutRet), 0);
//            printf("LogoutResult: %d\n", logoutRet.result);
//        }
//        else {
//            printf("收到不支持的命令，请重新输入！\n");
//        }
//        // 5 向服务器发送请求命令
//        send(sock, sendBuf, strlen(sendBuf)+1, 0);
//        // 6 接受服务器信息 recv
//        DataPackage dp = {};
//        int msyLen = recv(sock, &dp, sizeof(dp), 0); //返回接收到数据的长度
//        if (msyLen > 0)
//            std::cout << "name:" << dp.name << "\n" << "age:" <<  dp.age << std::endl;
//        else {
//            std::cout << "No message!" << std::endl;
//            break;
//        }
    }
    // 4 关闭socket
    close(sock);
    t1.join();
    return 0;
}