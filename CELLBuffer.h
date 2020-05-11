//
// Created by 孙卓文 on 2020/5/11.
//

#ifndef EASYTCPSERVER_CELLBUFFER_H
#define EASYTCPSERVER_CELLBUFFER_H


class CELLBuffer
{
public:
    explicit CELLBuffer(int size = 8192):_nSize(size) {
        _pBuf = new char[_nSize];
    }
    ~CELLBuffer() {
        if (!_pBuf) {
            delete [] _pBuf;
            _pBuf = nullptr;
        }
    }
    //
    char* GetBuf() {
        return _pBuf;
    }
    //将数据添加到缓冲区
    bool Push(const char* pData, int nLen) {
        //如果新来的数据在缓冲中放不开
        if (_nLast + nLen > _nSize) {
            //需要写入的数据大小大于可用空间
            int n = _nLast + nLen - _nSize;
            //小于8k，扩展buff的大小
            if (n < 8192) {
                n = 8192;

            }
            char* buff = new char[_nSize + n];
            memcpy(buff, _pBuf, _nLast);
            delete [] _pBuf;
            _pBuf = buff;
        }
        if (_nLast + nLen <= _nSize) {
            //将要发送的数据拷贝到发送缓冲区尾部
            memcpy(_pBuf + _nLast, pData, nLen);
            //计算数据尾部的位置
            _nLast += nLen;
            if (_nLast == _nSize) {
                _buffFullCount ++;
            }
            return true;
        } else {
            _buffFullCount++;
        }
        return false;
    }
    //将数据移除缓冲区
    void Pop(int nLen) {
        int n = _nLast - nLen;
        if (n > 0) {
            memcpy(_pBuf, _pBuf + nLen, n);
        }
        _nLast = n;
        if (_buffFullCount > 0) {
            --_buffFullCount;
        }
    }
    //将缓冲区的数据写到socket
    int Write2Socket(SOCKET sockFd) {
        int ret = 0;
        //确保缓冲区有数据
        if (_nLast > 0 && INVALID_SOCKET != sockFd) {
            //发送数据
            ret = send(sockFd, _pBuf, _nLast, 0);
            //数据尾部位置清零
            _nLast = 0;
            //对缓冲区满的计数进行清零
            _buffFullCount = 0;
        }
        return ret;
    }
    //从socket中收到剩余buffer的数据，将buffer写满
    int ReadFromSocket(SOCKET sockFd) {
        if (_nSize - _nLast > 0) {
            char* szRecv = _pBuf + _nLast;
            int nLen = (int)recv(sockFd, szRecv, _nSize - _nLast, 0);
            if (nLen <= 0) {
                //CELLLog::Info("客户端<Socket = %d>退出, 任务结束\n", clientSock->GetSock());
                return nLen;
            }
            //消息缓冲区的数据尾部位置后移
            _nLast += nLen;
            return nLen;
        }
        return 0;
    }
    //判断缓冲区中的数据是否存在一个完整的报文
    bool hasMsg() {
        //判断收到消息的长度是否大于消息头的长度，若大于消息头的长度，就可以取出消息体的长度
        if (_nLast >= sizeof(DataHeader)) {
            auto *header = (DataHeader*)_pBuf;
            //判断收到消息的长度是否大于消息体的长度，若大于消息体的长度，就可以取出消息体
            if (_nLast >= header->dataLength) {
                return true;    //true说明此时的缓冲区中有一个完整的数据
            }
        }
        return false;
    }
private:
    //缓冲区
    char* _pBuf = nullptr;
    //当前缓冲区尾部位置
    int _nLast = 0;
    //缓冲区总的空间大小
    int _nSize = 0;
    //缓冲区写满次数
    int _buffFullCount;
};

#endif //EASYTCPSERVER_CELLBUFFER_H
