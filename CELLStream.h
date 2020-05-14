//
// Created by 孙卓文 on 2020/5/14.
//

#ifndef EASYTCPCLIENT_CELLSTREAM_H
#define EASYTCPCLIENT_CELLSTREAM_H

#include <cstdint>
class CELLStream
{
public:
    explicit CELLStream(int nSize = 1024) {
        _nSize = nSize;
        _pBuf = new char[_nSize];
        _bDelete = true;
    }
    //读数据的时候 数据流是由外部传进来的，第三参数表示该数据流是否由我释放
    explicit CELLStream(char* pData, int nSize, bool bDelete = false) {
        _nSize = nSize;
        _pBuf = pData;
        _bDelete = bDelete;
    }
    virtual ~CELLStream() {
        if (_pBuf && _bDelete) {
            delete[] _pBuf;
            _pBuf = nullptr;
        }
    }

    char* data() {
        return _pBuf;
    }

    int length() {
        return _nWritePos;
    }

    void push(int size) {
        _nWritePos += size;
    }

    void pop(int size) {
        _nReadPos -= size;
    }

    void setWritePos(int n) {
        _nWritePos = n;
    }

    int getWritePos() {
        return _nWritePos;
    }


    //读取
    template <typename T>
    bool Read(T&, bool offset = true);
    int8_t ReadInt8();
    int16_t ReadInt16();
    int32_t ReadInt32();
    float ReadFloat();
    double ReadDouble();
    template <typename T>
    //返回读取数组的元素个数，返回0读取失败
    int ReadArray(T* pArr, uint32_t len);

    //写入
    template <typename T>
    bool Write(T);
    bool WriteInt8(int8_t);//char
    bool WriteInt16(int16_t);//short
    bool WriteInt32(int32_t);//int
    bool WriteFloat(float);
    bool WriteDouble(double);
    //先写入数组的长度（32位int） 再写入数组
    template <typename T>
    bool WriteArray(T* pData, uint32_t len);

private:
    //数据缓冲区
    char* _pBuf = nullptr;
    //数据缓冲区的大小
    int _nSize = 0;
    //已写入数据的位置
    int _nWritePos = 0;
    //已读取数据的位置
    int _nReadPos = 0;
    //_pBuff是否应该被由我释放
    bool _bDelete = true;

};

template <typename T>
bool CELLStream::Write(T n) {
    //计算要写入数据的大小
    size_t nLen = sizeof(T);
    //判断能不能写入
    if (_nWritePos + nLen <= _nSize) {
        //将写入的数据拷贝到缓冲区的尾部
        memcpy(_pBuf + _nWritePos, &n, nLen);
        _nWritePos += nLen;
        return true;
    }
    return false;
}

bool CELLStream::WriteInt8(int8_t n) {
    return Write(n);
}

bool CELLStream::WriteInt16(int16_t n) {
    return Write(n);
}

bool CELLStream::WriteInt32(int32_t n) {
    return Write(n);
}

bool CELLStream::WriteFloat(float n) {
    return Write(n);
}

bool CELLStream::WriteDouble(double n) {
    return Write(n);
}

template <typename T>
bool CELLStream::WriteArray(T *pData, uint32_t len) {
    auto nLen = sizeof(T) * len;
    if (_nWritePos + nLen + sizeof(uint32_t) <= _nSize) {
        //先写入数组的长度，数组的元素个数
        WriteInt32(len);
        memcpy(_pBuf+_nWritePos, pData, nLen);
        _nWritePos += nLen;
        return true;
    }
    return false;
}

template<typename T>
bool CELLStream::Read(T& n, bool offset) {
    //int8_t n = 0;      //记录成功读取的数
    size_t nLen = sizeof(T);
    //保证有数据，但不保证数据是否有效
    if (_nReadPos + nLen <= _nSize) {
        memcpy(&n, _pBuf + _nReadPos, nLen);
        if (offset) _nReadPos += nLen;
        return true;
    }
    return false;
}

int8_t CELLStream::ReadInt8() {
    int8_t n = 0;
    Read(n);
    return n;
}

int16_t CELLStream::ReadInt16() {
    int16_t n = 0;
    Read(n);
    return n;
}

int32_t CELLStream::ReadInt32() {
    int32_t n = 0;
    Read(n);
    return n;
}

float CELLStream::ReadFloat() {
    float n = 0;
    Read(n);
    return n;
}

double CELLStream::ReadDouble() {
    double n = 0;
    Read(n);
    return n;
}

template<typename T>
int CELLStream::ReadArray(T *pArr, uint32_t len) {
    //读取元素的个数
    uint32_t len1 = 0;
    //问题：若传入的数组放不下，则没读出数组，可是数组的长度已经读出，缓冲区中数组的长度已经没有了？
    Read(len1, false);  //读取元素个数但不偏移读取位置
    //判断传入的数组是否放下？
    if (len1 <= len) {
        //计算数组的字节长度
        auto nLen = len1 * sizeof(T);
        //判断能不能读出
        if (_nReadPos + nLen + sizeof(uint32_t) <= _nSize) {
            //读取成功后，补上读取数组的偏移
            _nReadPos += sizeof(uint32_t);
            memcpy(pArr, _pBuf + _nReadPos, nLen);
            _nReadPos += nLen;
            return len1;
        }
    }

    return 0;
}

#endif //EASYTCPCLIENT_CELLSTREAM_H
