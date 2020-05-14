//
// Created by 孙卓文 on 2020/5/14.
//

#ifndef EASYTCPCLIENT_CELLMSGSTREAM_H
#define EASYTCPCLIENT_CELLMSGSTREAM_H

#include "CELLStream.h"
class CELLRecvStream: public CELLStream
{
public:
    explicit CELLRecvStream(DataHeader* header, bool bDelete = false)
    :CELLStream((char*)header, header->dataLength, bDelete) {
        push(header->dataLength);
    }

    uint16_t getNetCmd() {
        uint16_t cmd = CMD_ERROR;
        Read<uint16_t>(cmd);
        return cmd;
    }
    uint16_t getNetLen() {
        uint16_t len = 0;
        Read<uint16_t>(len);
        return len;
    }

private:
};

class CELLSendStream: public CELLStream
{
public:
    explicit CELLSendStream(int nSize = 1024):CELLStream(nSize) {
        //预先占领消息长度所需要的空间
        Write<uint16_t>(0);
    }
    //读数据的时候 数据流是由外部传进来的，第三参数表示该数据流是否由我释放
    explicit CELLSendStream(char* pData, int nSize, bool bDelete = false)
    :CELLStream(pData, nSize, bDelete) {
        //预先占领消息长度所需要的空间
        Write<uint16_t>(0);
    }

    void setNetCmd(uint16_t cmd) {
        Write<uint16_t>(cmd);
    }

    //当数据写完之后，把数据的总长写在预留的消息的头部
    void finish() {
        int pos = getWritePos();
        setWritePos(0);
        Write<uint16_t>(pos);
        setWritePos(pos);
    }

private:
};


#endif //EASYTCPCLIENT_CELLMSGSTREAM_H
