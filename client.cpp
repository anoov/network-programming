//
// Created by 孙卓文 on 2020/4/5.
//
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
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
    _sin.sin_port = htons(4567);
    _sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    int ret = connect(sock, (sockaddr *)&_sin, sizeof(sockaddr));
    if (ret == SOCKET_ERROR) {
        printf("连接失败...\n");
    }else {
        printf("连接成功...\n");
    }
    while (true) {
        // 3 输入请求命令
        char sendBuf[256] = {};
        printf("请输入命令：");
        scanf("%s", sendBuf);
        // 4 处理请求
        if (0 == strcmp(sendBuf, "exit")) {
            printf("收到exit命令，任务结束！");
            break;
        }
        else if (0 == strcmp(sendBuf, "login")) {
            Login login;
            strcpy(login.userName, "lyd");
            strcpy(login.passWord, "lydmima");
            send(sock, &login, sizeof(login), 0);
            //接受服务器返回的数据
            LoginResult loginRet = {};
            recv(sock, &loginRet, sizeof(loginRet), 0);
            printf("LoginResult: %d\n", loginRet.result);

        }
        else if (0 == strcmp(sendBuf, "logout")) {
            LogOut logout;
            strcpy(logout.userName, "lyd");
            send(sock, &logout, sizeof(logout), 0);
            //接受服务器返回的数据
            LoginOutResult logoutRet = {};
            recv(sock, &logoutRet, sizeof(logoutRet), 0);
            printf("LogoutResult: %d\n", logoutRet.result);
        }
        else {
            printf("收到不支持的命令，请重新输入！\n");
        }
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
    return 0;
};