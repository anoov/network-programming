//
// Created by 孙卓文 on 2020/4/6.
//

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)


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
        // std::cin >> sendBuf;
        printf("请输入命令：");
        scanf("%s", sendBuf);
        // 4 处理请求
        if (0 == strcmp(sendBuf, "exit")) {
            break;
        }
        // 5 发送请求
        send(sock, sendBuf, strlen(sendBuf)+1, 0);
        // 6 接受服务器信息 recv
        char recvBuf[256] = {};
        int msyLen = recv(sock, recvBuf, 256, 0); //返回接收到数据的长度
        if (msyLen > 0)
            std::cout << "recvBuf: " << recvBuf << std::endl;
        else {
            std::cout << "No message!" << std::endl;
            break;
        }
    }
    // 4 关闭socket
    close(sock);
    return 0;
};