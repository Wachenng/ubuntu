#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__

#include "stream.h"
#include "socket.h"

namespace sylar {

/**
 * @brief 继承Stream，针对Socket的读写，本质上就是调用了Socket的读写方法
 */
class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;
    
    /**
     * @brief 构造函数
     * @param owner 表示是否直接接管sock，负责它的close
     */
    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;

    Socket::ptr getSocket() const { return m_socket;}
    bool isConnected() const;
protected:
    /// Socket类
    Socket::ptr m_socket;
    /// 是否主控(主要是是否交由该类来控制关闭)
    bool m_owner;
};

}

#endif