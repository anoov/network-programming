//
// Created by 孙卓文 on 2020/4/17.
//

#ifndef EASYTCPCLIENT_DATASTRUCT_H
#define EASYTCPCLIENT_DATASTRUCT_H
enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_c2s_HEART,
    CMD_s2c_HEART,
    CMD_ERROR
};

struct DataHeader
{
    uint16_t dataLength;  //数据长度
    uint16_t cmd;         //命令
};

struct Login: public DataHeader
{
    Login(){
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char passWord[32];
    char data[32];
};
struct LoginResult: public DataHeader
{
    LoginResult(){
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 1;
    }
    int result;
    char data[92];
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

struct Heart_c2s_Test: public DataHeader
{
    Heart_c2s_Test(){
        dataLength = sizeof(Heart_c2s_Test);
        cmd = CMD_c2s_HEART;
    }
};

struct Heart_s2c_Test: public DataHeader
{
    Heart_s2c_Test(){
        dataLength = sizeof(Heart_s2c_Test);
        cmd = CMD_s2c_HEART;
    }
};
#endif //EASYTCPCLIENT_DATASTRUCT_H
