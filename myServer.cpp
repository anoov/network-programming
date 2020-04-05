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
    char msyBuf[] = "hello";
    while (true) {
        //循环接入客户端，并发送一个数据
        clientSock = accept(sock, (sockaddr *) &clientAddr, &nAddrLen);
        if (clientSock == INVALID_SOCKET) {
            printf("接受客户连接失败...\n");
            break;
        } else {
            printf("新客户连接成功: IP = %s\n", inet_ntoa(clientAddr.sin_addr));
        }
        //5 send 向客户端发送一条数据
        send(clientSock, msyBuf, strlen(msyBuf) + 1, 0);
    }
    //6 关闭套接字
    close(sock);
    return 0;
}
