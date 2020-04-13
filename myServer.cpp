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

int main_fun2() {
    //1 建立一个socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //2 bind 绑定用于接受客户端的连接和网络端口
    sockaddr_in _sin{};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);    //host to net unsigned short
    _sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(sock, (sockaddr*)& _sin, sizeof(_sin)) == SOCKET_ERROR) {
        printf("绑定网络端口失败...\n");
    } else {
        printf("绑定网络端口成功...\n");
    }

    //3 listen 监听网络端口
    if (listen(sock, 5) == SOCKET_ERROR){
        printf("监听网络端口失败...\n");
    } else {
        printf("监听网络端口成功...\n");
    }

    //4 accept 等待接受客户连接
    sockaddr_in clientAddr = {};
    socklen_t nAddrLen = sizeof(sockaddr_in);
    SOCKET clientSock = INVALID_SOCKET;
    clientSock = accept(sock, (sockaddr *) &clientAddr, &nAddrLen);
    if (clientSock == INVALID_SOCKET) {
        printf("接受客户连接失败...\n");
    } else {
        printf("新客户连接成功: IP = %s\n", inet_ntoa(clientAddr.sin_addr));
    }

    while (true) {
        //5 接收客户端数据
        //5.1 接受数据头
        //使用缓冲区来接受数据
        char szRecv[1024] = {};
        int nLen = recv(clientSock, &szRecv, sizeof(DataHeader), 0);
        DataHeader* header = (DataHeader *)szRecv;
        if (nLen <= 0) {
            printf("客户端退出, 任务结束\n");
            break;
        }

        switch (header->cmd) {
            case CMD_LOGIN:
                {
                    recv(clientSock, szRecv+sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
                    Login* login = (Login *)szRecv;
                    printf("收到命令：CMD_LOGIN, 数据长度: %d，用户名称: %s, 用户密码: %s\n",
                            login->dataLength, login->userName, login->passWord);
                    //忽略判断用户名和密码
                    LoginResult ret;
                    send(clientSock, &ret, sizeof(LoginResult), 0);
                }
                break;
            case CMD_LOGOUT:
                {
                    recv(clientSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
                    LogOut* loginOut = (LogOut *)szRecv;
                    printf("收到命令：CMD_LOGOUT, 数据长度: %d，用户名称: %s\n",
                           loginOut->dataLength, loginOut->userName);
                    //忽略判断用户名和密码
                    LoginOutResult ret;
                    send(clientSock, &ret, sizeof(LoginOutResult), 0);

                }
                break;
            default:
                {
                    header->cmd = CMD_ERROR;
                    header->dataLength = 0;
                    send(clientSock, &header, sizeof(DataHeader), 0);
                }
                break;
        }


    }
    //6 关闭套接字
    close(sock);
    return 0;
}
