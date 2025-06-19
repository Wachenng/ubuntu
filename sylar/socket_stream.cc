#include "socket_stream.h"

namespace sylar {

SocketStream::SocketStream(Socket::ptr sock, bool owner)
    :m_socket(sock)
    ,m_owner(owner) {
}

SocketStream::~SocketStream() {
    if(m_owner && m_socket) {
        m_socket->close();
    }
}

bool SocketStream::isConnected() const {
    return m_socket && m_socket->isConnected();
}

/**
* @brief 读取数据
* @param[out] buffer 待接收数据的内存
* @param[in] length 待接收数据的内存长度
* @return
*      @retval >0 返回实际接收到的数据长度
*      @retval =0 socket被远端关闭
*      @retval <0 socket错误
*/
int SocketStream::read(void* buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_socket->recv(buffer, length);
}

int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);              // 拿到iovec写缓存
    int rt = m_socket->recv(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);    // 更新当前操作位置
    }
    return rt;
}

/**
* @brief 写入数据
* @param[in] buffer 待发送数据的内存
* @param[in] length 待发送数据的内存长度
* @return
*      @retval >0 返回实际接收到的数据长度
*      @retval =0 socket被远端关闭
*      @retval <0 socket错误
*/
int SocketStream::write(const void* buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_socket->send(buffer, length);
}

int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    int rt = m_socket->send(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

void SocketStream::close() {
    if(m_socket) {
        m_socket->close();
    }
}

}