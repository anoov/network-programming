//
// Created by 孙卓文 on 2020/5/6.
//

#ifndef EASYTCPSERVER_CELLPUBLICHEADER_H
#define EASYTCPSERVER_CELLPUBLICHEADER_H

//socket相关
#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    //#include <sys/socket.h>
    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR            (-1)
#endif
#include <cstdio>
#include <csignal>
#include <cstring>

#define RECV_BUFF_SIZE  1024
#define SEND_BUFF_SIZE  102400

#include "DataStruct.h"
#include "CELLTimestamp.h"
#include "CELLTask.h"
#include "CELLBuffer.h"
#include "CELLLog.h"


#endif //EASYTCPSERVER_CELLPUBLICHEADER_H
