#ifndef __SYLAR_STREAM_H__
#define __SYLAR_STREAM_H__

#include <memory>
#include "bytearray.h"

namespace sylar {

/**
 * @brief 流结构，提供字节流读写接口
 * @brief 所有的流结构都继承自抽象类Stream
 *        Stream类规定了一个流必须具备read/write接口和readFixSize/writeFixSize接口，继承自Stream的类必须实现这些接口
 * 
 *@brief  基类。支持普通读写以及固定长度读写。其中read、write方法是纯虚函数，子类必须重写。
 */
class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;
    virtual ~Stream() {}

    virtual int read(void* buffer, size_t length) = 0;
    virtual int read(ByteArray::ptr ba, size_t length) = 0;

    virtual int readFixSize(void* buffer, size_t length);
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    virtual int write(const void* buffer, size_t length) = 0;
    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    virtual int writeFixSize(const void* buffer, size_t length);
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);
    
    virtual void close() = 0;
};

}

#endif