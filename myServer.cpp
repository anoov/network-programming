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
    CMD_LOGINOUT,
    CMD_ERROR
};

struct DataHead
{
    short dataLength;  //数据长度
    short cmd;         //命令
};

struct Login
{
    char userName[32];
    char passWord[32];
};
struct LoginResult
{
    int result;
};
struct LoginOut
{
    char userName[32];
};
struct LoginOutResult
{
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
        DataHead header = {};
        int nLen = recv(clientSock, &header, sizeof(header), 0);
        if (nLen <= 0) {
            printf("客户端退出, 任务结束\n");
            break;
        }
        printf("收到命令：%d, 数据长度：%d\n", header.cmd, header.dataLength);

        switch (header.cmd) {
            case CMD_LOGIN:
                {
                    Login login = {};
                    recv(clientSock, &login, sizeof(Login), 0);
                    //忽略判断用户名和密码
                    LoginResult ret = {1};
                    send(clientSock, &header, sizeof(DataHead), 0);
                    send(clientSock, &ret, sizeof(LoginResult), 0);
                }
                break;
            case CMD_LOGINOUT:
                {
                    LoginOut loginOut = {};
                    recv(clientSock, &loginOut, sizeof(loginOut), 0);
                    //忽略判断用户名和密码
                    LoginOutResult ret = {2};
                    send(clientSock, &header, sizeof(DataHead), 0);
                    send(clientSock, &ret, sizeof(LoginOutResult), 0);

                }
                break;
            default:
                {
                    header.cmd = CMD_ERROR;
                    header.dataLength = 0;
                    send(clientSock, &header, sizeof(header), 0);
                }
                break;
        }


    }
    //6 关闭套接字
    close(sock);
    return 0;
}
