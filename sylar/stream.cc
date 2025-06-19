#include "stream.h"

namespace sylar {

 /**
 * @brief 读固定长度的数据
 * @param[out] buffer 接收数据的内存
 * @param[in] length 接收数据的内存大小
 * @return
 *      @retval >0 返回接收到的数据的实际大小
 *      @retval =0 被关闭
 *      @retval <0 出现流错误
 */
int Stream::readFixSize(void* buffer, size_t length) {
    size_t offset = 0;          // 偏移量
    size_t left = length;       // 剩余读的数据
    while(left > 0) {
        size_t len = read((char*)buffer + offset, left);
        if(len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    size_t left = length;
    while(left > 0) {
        size_t len = read(ba, left);
        if(len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

/**
* @brief 写固定长度的数据
* @param[in] buffer 写数据的内存
* @param[in] length 写入数据的内存大小
* @return
*      @retval >0 返回写入到的数据的实际大小
*      @retval =0 被关闭
*      @retval <0 出现流错误
*/
int Stream::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0;
    size_t left = length;
    while(left > 0) {
        size_t len = write((const char*)buffer + offset, left);
        if(len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;

}

int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
    size_t left = length;
    while(left > 0) {
        size_t len = write(ba, left);
        if(len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

}