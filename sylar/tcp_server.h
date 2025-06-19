#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <memory>
#include <functional>
#include "address.h"
#include "iomanager.h"
#include "socket.h"
#include "noncopyable.h"

namespace sylar {

class TcpServer : public std::enable_shared_from_this<TcpServer>
                    , Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;


    TcpServer(sylar::IOManager* woker = sylar::IOManager::GetThis()
              ,sylar::IOManager* accept_woker = sylar::IOManager::GetThis());
    virtual ~TcpServer();


    virtual bool bind(sylar::Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout;}
    std::string getName() const { return m_name;}
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}
    void setName(const std::string& v) { m_name = v;}

    bool isStop() const { return m_isStop;}
protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);
private:
    /// 存储listen socket 
    std::vector<Socket::ptr> m_socks;
    /// 当作工作线程池
    IOManager* m_worker;

    IOManager* m_acceptWorker;
    /// 接收超时时间
    uint64_t m_recvTimeout;
    /// 服务器名称
    std::string m_name;
    /// 是否停止
    bool m_isStop;
};

}

#endif