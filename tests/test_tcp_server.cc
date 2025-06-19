#include "sylar/tcp_server.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    auto addr = sylar::Address::LookupAny("0.0.0.0:9527");
    SYLAR_LOG_INFO(g_logger) << *addr;    
    // auto addr2 = sylar::UnixAddress::ptr(new sylar::UnixAddress("/tmp/unix_addr"));
    // SYLAR_LOG_INFO(g_logger) << *addr2;   

    std::vector<sylar::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    sylar::TcpServer::ptr tcp_server(new sylar::TcpServer);
    std::vector<sylar::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    sylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

/*
  测试--打开该程序

  每次测试删除 rm /tmp/unix_addr 

  新建终端 
        ubuntu@VM-0-10-ubuntu:~$ ls /tmp/unix_addr -lh
        srwxrwxr-x 1 ubuntu ubuntu 0 Jun 18 03:45 /tmp/unix_addr
        
        ubuntu@VM-0-10-ubuntu:~$ netstat -ap | grep test_
        (Not all processes could be identified, non-owned process info
        will not be shown, you would have to be root to see it all.)
        tcp        0      0 0.0.0.0:8033            0.0.0.0:*               LISTEN      2982842/./bin/test_ 
        unix  2      [ ACC ]     STREAM     LISTENING     250984501 2982842/./bin/test_  /tmp/unix_addr
        
        ubuntu@VM-0-10-ubuntu:~$ ps aux | grep 7940
        ubuntu   2983666  0.0  0.0   6480  2492 pts/4    S+   03:47   0:00 grep --color=auto 7940
        
        ubuntu@VM-0-10-ubuntu:~$ netstat -tnalp | grep test_
        (Not all processes could be identified, non-owned process info
        will not be shown, you would have to be root to see it all.)
        tcp        0      0 0.0.0.0:8033            0.0.0.0:*               LISTEN      2982842/./bin/test_ 
        
        ubuntu@VM-0-10-ubuntu:~$ telnet 127.0.0.1 8033
        Trying 127.0.0.1...
        Connected to 127.0.0.1.
        Escape character is '^]'.
        Connection closed by foreign host.
        
        ubuntu@VM-0-10-ubuntu:~$ telnet 127.0.0.1 8033
        Trying 127.0.0.1...
        Connected to 127.0.0.1.
        Escape character is '^]'.
        Connection closed by foreign host.

 */