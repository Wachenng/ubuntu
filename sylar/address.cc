#include "address.h"
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>

#include "endian.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


/**
 * @brief 子网掩码的规则 长度 为 4 * 8bit（1字节），由 连续的1 以及 连续的0 两部分组成
 *        例如：11111111.11111111.11111111.00000000，对应十进制：255.255.255.0
 * 
 * @brief 生成一个类型为 T 的位掩码 - 生成的是低位连续1的掩码
 * @param bits 表示掩码中高位连续1的位数
 * @brief 计算总位数： sizeof(T) * 8 获取类型 T 的总位数（例如 sizeof(uint32_t)=4，则总位数为 32）
 * @brief 计算左移位数：sizeof(T) * 8 - bits 确定需要左移的位数。例如，若 T 为 uint32_t（32位），且 bits=4，则左移位数为 32-4=28
 * @brief 生成掩码： 1 << N：将数值1左移 N 位。例如，1 << 28 得到 0x10000000（二进制第28位为1）
 *                     - 1：左移结果减1，将高位连续转换为1。例如：0x10000000 - 1 = 0x0FFFFFFF（高4位为0，低28位为1）
 * @return 生成一个掩码，其中低 (总位数 - bits) 位为1，高 bits 位为0
 * @brief 若需高位连续1的掩码，应使用： ~((1 << (sizeof(T) * 8 - bits)) - 1)
 */
template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

/**
 * @brief 计算一个模板类型 T 变量 value 的二进制表示中 1 的个数
 * @brief 每次操作都会将 value 的二进制表示中 最低位的 1 变成 0
          示例（以 8 位二进制数说明）：
          1.若 value = 0b10100100 (164)：
               value - 1 = 0b10100011 (163)
               value & (value - 1) = 0b10100000 (160)
          2.下一轮循环：
               value = 0b10100000 (160)
               value - 1 = 0b10011111 (159)
               value & (value - 1) = 0b10000000 (128)
          3.继续循环：
               value = 0b10000000 (128)
               value - 1 = 0b01111111 (127)
               value & (value - 1) = 0b00000000 (0) → 循环结束。
 */
template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::LookupAny(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        for(auto& i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v) {
                return v;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 解析主机名和服务名，获取对应的网络地址信息（如IPv4或IPv6地址）
 * @brief 该函数支持解析IPv6地址（格式为`[ipv6]:port`）和普通IPv4地址（格式为`host:port`）
 * 
 * @param result 用于存储解析结果的向量
 * @param host 要解析的主机名或IP地址字符串（可能包含端口或服务名）
 * @param family 地址族（如`AF_INET`、`AF_INET6`）
 * @param type  套接字类型（如`SOCK_STREAM`）
 * @param protocol 协议（如`IPPROTO_TCP`）
 */
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family, int type, int protocol) {
    // `hints`用于指定查询条件，这里设置了地址族、套接字类型和协议。其他字段初始化为0或NULL
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;           // 存储IP地址数据，一般转换成struct sockaddr_in或struct sockaddr_in6使用
    hints.ai_next = NULL;

    std::string node;
    const char* service = NULL;

    //检查 ipv6address serivce
    // 如果`host`以`[`开头（如`[::1]:8080`），则查找匹配的`]`，如果找到且后面紧跟冒号，则冒号后面的部分作为服务（端口），方括号内的部分作为节点（IPv6地址）
    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            //TODO check out of range
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2; // 服务（端口）部分在 "]" 后（如 ":80"）
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //检查 node serivce
    // 如果节点仍然为空（即不是IPv6格式），则查找第一个冒号。如果找到且后面没有其他冒号（避免IPv6地址中的多个冒号），则冒号前面的部分作为节点，后面部分作为服务
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    // 如果节点还是空，则整个`host`作为节点（此时没有服务部分）
    if(node.empty()) {
        node = host;
    }
    // 使用前面解析得到的`node`和`service`（可能为NULL），以及`hints`进行地址解析
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        SYLAR_LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << strerror(errno);
        return false;
    }

    next = results;
    while(next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        //SYLAR_LOG_INFO(g_logger) << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return true;
}

/**
 * @brief 用于获取系统中所有网络接口的地址信息，并将结果存储在一个`std::multimap`中
 * @param result 用于存储结果
 * @param family 要获取的网络接口地址族（如`AF_INET`、`AF_INET6`）
 * 
 * 
    struct ifaddrs {                                                
        struct ifaddrs  *ifa_next;                                  ifa_next指向链表中下一个struct ifaddr结构
        char            *ifa_name;                                  ifa_name 网络接口名
        unsigned int     ifa_flags;                                 ifa_flags 网络接口标志，这些标志见下面描述。
        struct sockaddr *ifa_addr;                                  ifa_addr 指向一个包含网络地址的sockaddr结构
        struct sockaddr *ifa_netmask;                               ifa_netmask 指向一个包含网络掩码的结构
        union
        {
            struct sockaddr *ifu_broadaddr; 
            struct sockaddr *ifu_dstaddr; 
        } ifa_ifu;
        #define              ifa_broadaddr ifa_ifu.ifu_broadaddr    ifu_broadaddr 如果(ifa_flags&IFF_BROADCAST)有效，ifu_broadaddr指向一个包含广播地址的结构。
        #define              ifa_dstaddr   ifa_ifu.ifu_dstaddr      ifu_dstaddr 如果(ifa_flags&IFF_POINTOPOINT)有效，ifu_dstaddr指向一个包含p2p目的地址的结构。
        void            *ifa_data;    
    };

    //UNIX
    struct sockaddr {
        ushort  sa_family;
        char    sa_data[14];
    };

    //IPv4
    struct sockaddr_in {
        short            sin_family;   // 网络协议
        unsigned short   sin_port;     // 端口
        struct in_addr   sin_addr;     // ip
        char             sin_zero[8];  
    };

    struct in_addr {
        unsigned long s_addr;  // 使用 inet_aton() 加载
    };

    //IPv6
    struct sockaddr_in6 {
    sa_family_t     sin6_family;    网络协议
    in_port_t       sin6_port;      端口 
    uint32_t        sin6_flowinfo;  IPv6 流信息 
    struct in6_addr sin6_addr;      ip 
    uint32_t        sin6_scope_id; 
    };

    struct in6_addr {
    unsigned char s6_addr[16];     IPv6 address 
    };
 */
bool Address::GetInterfaceAddresses(std::multimap<std::string
                    ,std::pair<Address::ptr, uint32_t> >& result,
                    int family) {
    struct ifaddrs *next, *results;
    // 使用`getifaddrs`函数获取系统中所有网络接口的地址信息，结果存储在`results`链表中
    if(getifaddrs(&results) != 0) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    try {
        // 遍历`ifaddrs`链表，每个节点`next`表示一个网络接口地址
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            // 如果调用者指定了特定的地址族（不是`AF_UNSPEC`），则跳过不匹配的地址族
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch(next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        // 对于IPv4地址，使用`Create`函数（假设是`Address`类的静态方法）创建一个`Address::ptr`指向的IPv4地址对象
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        // 从`ifa_netmask`中提取子网掩码（`sockaddr_in`结构），然后将其转换为`uint32_t`类型的网络字节序的IP地址
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        // 调用`CountBytes`函数计算子网掩码中连续1的位数（即前缀长度）
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        // 对于IPv6地址，同样创建`Address::ptr`对象
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        // 提取IPv6的子网掩码（128位），存储在`in6_addr`结构中
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        // 遍历16个字节（128位），对每个字节调用`CountBytes`计算该字节中1的位数，并累加到`prefix_len`上。这样得到的就是整个子网掩码中1的位数
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }

            // 如果`addr`不为空（即成功创建了地址对象），则将接口名、地址对象和前缀长度组成的键值对插入到`result`中
            if(addr) {
                result.insert(std::make_pair(next->ifa_name,
                            std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return true;
}

/**
 * @brief 用于获取指定网络接口的地址列表
 * @param result 用于存储获取到的地址和前缀长度（子网掩码位数）
 * @param iface  指定网络接口的名称。如果为空或为"*"，则表示所有接口
 * @param family 指定地址族（如IPv4、IPv6等），可以是`AF_INET`（IPv4）、`AF_INET6`（IPv6）或`AF_UNSPEC`（表示任意地址族）
 */
bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family) {
    // 首先检查`iface`是否为空或等于"*"
    if(iface.empty() || iface == "*") {
        // 如果是，则根据`family`参数，向结果向量`result`中添加一个默认的IPv4地址（如果`family`是`AF_INET`或`AF_UNSPEC`）和/或一个默认的IPv6地址（如果`family`是`AF_INET6`或`AF_UNSPEC`）。
        // 这里的默认地址是通过`new IPv4Address()`和`new IPv6Address()`创建的，并且前缀长度（子网掩码位数）设为0（`0u`）。然后返回`true`
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    // 定义一个`std::multimap`，键为接口名称，值为一个地址和前缀长度的对（与`result`中存储的类型相同）。
    // 这个`multimap`用于存储从另一个重载的`GetInterfaceAddresses`函数获取的所有接口的地址信息
    std::multimap<std::string
          ,std::pair<Address::ptr, uint32_t> > results;
    //调用另一个`GetInterfaceAddresses`函数（该函数是重载版本，第一个参数是上面定义的`multimap`，第二个参数是`family`），如果调用失败（返回`false`），则直接返回`false`
    if(!GetInterfaceAddresses(results, family)) {
        return false;
    }
    // 则使用`equal_range`函数在`multimap`中查找所有键等于`iface`的条目（即指定接口的所有地址）
    // 遍历这些条目，将每个条目的值（即`pair<Address::ptr, uint32_t>`）添加到输出向量`result`中
    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return true;
}

int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
    if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

/**
 * @brief 重载小于运算符`<`的实现
 * @brief 优先比较内存内容的字典序（memcmp结果）
 *        若内存内容在公共长度内相同，则长度更小的对象被视为更小
 */
bool Address::operator<(const Address& rhs) const {
    // 获取当前对象和`rhs`对象的地址长度（`getAddrLen()`）中较小的一个，存储在`minlen`中
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    // 使用`memcmp`函数比较两个地址的内容（由`getAddr()`返回的指针指向的内存区域）。比较的长度为`minlen`，即两个地址中较小的长度
    // memcmp`函数按字节比较两个内存区域，如果第一个区域小于第二个，返回负数；相等返回0；大于返回正数
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if(result < 0) {
        // 如果`memcmp`返回负数，表示当前对象的地址内容小于`rhs`的地址内容（在内存的字典序中），那么当前对象应被视为小于`rhs`对象，因此返回`true`。
        return true;
    } else if(result > 0) {
        // 如果`memcmp`返回正数，表示当前对象的地址内容大于`rhs`的地址内容，那么当前对象不小于`rhs`对象，返回`false`。
        return false;
    } else if(getAddrLen() < rhs.getAddrLen()) {
        // 如果`memcmp`返回0，表示在共同长度`minlen`内，两个地址内容完全相同。那么，如果当前对象的地址长度小于`rhs`的地址长度，则当前对象被视为小于`rhs`对象，返回`true`。
        return true;
    }
    return false;
}

bool Address::operator==(const Address& rhs) const {
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
    return !(*this == rhs);
}

/**
 * @brief 根据给定的 IP 地址字符串和端口号创建 IPAddress 对象
 * @param address 字符串形式的 IP 地址（如 "192.168.1.1" 或 "::1"）
 * @param port 端口号
 */
IPAddress::ptr IPAddress::Create(const char* address, uint32_t port) {
    addrinfo hints, *results;
    // memset 清零确保无未初始化数据
    memset(&hints, 0, sizeof(addrinfo));

    // AI_NUMERICHOST：要求地址必须是数字格式（禁止 DNS 解析）
    hints.ai_flags = AI_NUMERICHOST;
    // AF_UNSPEC：同时支持 IPv4 和 IPv6 地址
    hints.ai_family = AF_UNSPEC;

    // 解析 address 字符串（port 参数设为 NULL 暂不处理端口） 结果存储在链表 results 中
    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        SYLAR_LOG_ERROR(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    // 调用 Address::Create 工厂方法，传入 results->ai_addr（地址二进制结构）和 ai_addrlen（结构长度）
    // 使用 dynamic_pointer_cast 将基类指针转为 IPAddress 派生类指针
    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

/**
 * @brief 
 * @param address 字符串形式的 IPv4 地址（如 "192.168.1.1"）
 * @param port 端口号（port）
 */
IPv4Address::ptr IPv4Address::Create(const char* address, uint32_t port) {
    IPv4Address::ptr rt(new IPv4Address);
    // 将主机字节序的 port 转换为网络字节序（大端序）
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    // 使用 inet_pton 将字符串地址转换为二进制形式 -- AF_INET：指定 IPv4 地址族
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address) {
    m_addr = address;
}

IPv4Address::IPv4Address(uint32_t address, uint32_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint32_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

IPv6Address::ptr IPv6Address::Create(const char* address, uint32_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address) {
    m_addr = address;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint32_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint32_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    if(!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) {
    m_addr = addr;
}

const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}

socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

}