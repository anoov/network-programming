//
// Created by 孙卓文 on 2020/4/5.
//
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#include <vector>
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
//有新客户端加入，群发给所有的客户端
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

std::vector<SOCKET> g_clients;

int process(SOCKET clientSock)  {

    //5 接收客户端数据
    //5.1 接受数据头
    //使用缓冲区来接受数据
    char szRecv[4096] = {};
    int nLen = recv(clientSock, szRecv, sizeof(DataHeader), 0);
    auto* header = (DataHeader *)szRecv;
    if (nLen <= 0) {
        printf("客户端<Socket = %d>退出, 任务结束\n", clientSock);
        return -1;
    }

    switch (header->cmd) {
        case CMD_LOGIN:
        {
            recv(clientSock, szRecv+sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            Login* login = (Login *)szRecv;
            printf("收到<Socket = %d>请求：CMD_LOGIN, 数据长度: %d，用户名称: %s, 用户密码: %s\n",
                   clientSock, login->dataLength, login->userName, login->passWord);
            //忽略判断用户名和密码
            LoginResult ret;
            send(clientSock, &ret, sizeof(LoginResult), 0);
        }
            break;
        case CMD_LOGOUT:
        {
            recv(clientSock, szRecv+sizeof(DataHeader), header->dataLength-sizeof(DataHeader), 0);
            LogOut* loginOut = (LogOut *)szRecv;
            printf("收到<Socket = %d>请求：CMD_LOGOUT, 数据长度: %d，用户名称: %s\n",
                   clientSock ,loginOut->dataLength, loginOut->userName);
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
    return 0;
}

int main_fun2() {
    //1 建立一个socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //2 bind 绑定用于接受客户端的连接和网络端口
    sockaddr_in _sin{};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(8000);    //host to net unsigned short
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

    while (true) {

        //伯克利 socket
        //第一个参数：集合中所有描述符的范围而不是数量，集合中最大值加一
        //第二个参数：读描述符集合，告诉内核需要查询的需要读的套接字的集合
        //第三个参数：写描述符集合
        //第三个参数：异常描述符集合
        //第四个参数：时间，在该时间内没有返回，则返回
        fd_set fdRead;
        fd_set fdWrite;
        fd_set fdExcept;
        FD_ZERO(&fdRead);   //清空集合中的数据
        FD_ZERO(&fdWrite);   //清空集合中的数据
        FD_ZERO(&fdExcept);   //清空集合中的数据

        FD_SET(sock, &fdRead);      //让内核代理查看socket有没有读操作
        FD_SET(sock, &fdWrite);     //让内核代理查看socket有没有写操作
        FD_SET(sock, &fdExcept);    //让内核代理查看socket有没有异常操作

        SOCKET maxSock = sock;
        for (int i = (int)g_clients.size() - 1; i >= 0; i--) {
            FD_SET(g_clients[i], &fdRead);//有没有客户需要接收
            maxSock = std::max(maxSock, g_clients[i]);
        }

        timeval t = {1, 0};
        //若最后一个参数设置为null，则程序会阻塞到select这里，一直等到有数据可处理
        //select监视三个集合中的所有描述符，在这里是套接字
        //例如select的第二个参数读集合，
        //若集合中的某一个socket有读操作，则保持该操作位，否则该操作位清零
        int ret = select(maxSock+1, &fdRead, &fdWrite, &fdExcept, &t);

        if (ret < 0) {
            printf("select发生错误, 任务结束\n");
            break;
        }

        //若本socket有读操作，意味着有客户机连进来
        if (FD_ISSET(sock, &fdRead)) {
            FD_CLR(sock, &fdRead);

            //4 accept 等待接受客户连接
            sockaddr_in clientAddr = {};
            socklen_t nAddrLen = sizeof(sockaddr_in);
            SOCKET clientSock = INVALID_SOCKET;

            clientSock = accept(sock, (sockaddr *) &clientAddr, &nAddrLen);
            if (clientSock == INVALID_SOCKET) {
                printf("接受客户连接失败...\n");
            } else {
                for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
                    NewUserJoin userJoin{};
                    send(g_clients[n], &userJoin, sizeof(NewUserJoin), 0);
                }
                g_clients.push_back(clientSock);
                printf("新客户连接成功: socket = %d, IP = %s\n",(int)(clientSock), inet_ntoa(clientAddr.sin_addr));
            }

        }

        for (int n = (int)g_clients.size() - 1; n >= 0; n--)
        {
            //如何客户socket数组中有读组操作，则说明有消息进来
            if (FD_ISSET(g_clients[n], &fdRead))
            {
                if (-1 == process(g_clients[n]))
                {
                    auto iter = g_clients.begin() + n;//std::vector<SOCKET>::iterator
                    if (iter != g_clients.end())
                    {
                        g_clients.erase(iter);
                    }
                }
            }
        }
//        std::cout << "空闲时间处理其他业务" << std::endl;

    }
    //6 关闭套接字
    for (size_t n = 0; n < g_clients.size(); n++) {
        close(g_clients[n]);
    }
    return 0;
}
