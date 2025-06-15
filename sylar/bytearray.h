#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__

#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

namespace sylar {

/**
 * @brief 模块实现了一个动态扩容的字节数组，支持基本数据类型的序列化/反序列化、变长整数编码、文件 I/O 和零拷贝缓冲区操作
 */
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief 链表节点
     * @param ptr 一个字符指针（指向分配的内存）
     * @param next 指向下一个节点的指针
     * @param size 该节点内存块的大小
     */
    struct Node {
        Node(size_t s);
        Node();
        ~Node();

        char* ptr;
        Node* next;
        size_t size;
    };

    // 初始化成员变量，创建第一个节点（大小为base_size）
    ByteArray(size_t base_size = 4096);
    // 释放整个链表
    ~ByteArray();

    //写入固定长度整数（writeFint8, writeFuint8等）
    void writeFint8  (int8_t value);
    void writeFuint8 (uint8_t value);
    void writeFint16 (int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32 (int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64 (int64_t value);
    void writeFuint64(uint64_t value);

    //变长整数（writeInt32, writeUint32等
    void writeInt32  (int32_t value);
    void writeUint32 (uint32_t value);
    void writeInt64  (int64_t value);
    void writeUint64 (uint64_t value);

    //浮点数
    void writeFloat  (float value);
    void writeDouble (double value);

    //字符串（带长度前缀和不带长度前缀）
    //length:int16 , data
    void writeStringF16(const std::string& value);
    //length:int32 , data
    void writeStringF32(const std::string& value);
    //length:int64 , data
    void writeStringF64(const std::string& value);
    //length:varint, data
    void writeStringVint(const std::string& value);
    //data
    void writeStringWithoutLength(const std::string& value);

    //read - 读取固定长度整数、变长整数、浮点数、字符串
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();

    float    readFloat();
    double   readDouble();

    //length:int16, data
    std::string readStringF16();
    //length:int32, data
    std::string readStringF32();
    //length:int64, data
    std::string readStringF64();
    //length:varint , data
    std::string readStringVint();

    //内部辅助函数：clear（清空）
    void clear();

    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;

    //内部辅助函数：getPosition（获取读写位置）
    size_t getPosition() const { return m_position;}
    //内部辅助函数：setPosition（设置读写位置）
    void setPosition(size_t v);

    //writeToFile（将ByteArray中的数据写入文件）
    bool writeToFile(const std::string& name) const;
    //readFromFile（从文件读取数据到ByteArray）
    bool readFromFile(const std::string& name);

    size_t getBaseSize() const { return m_baseSize;}
    size_t getReadSize() const { return m_size - m_position;}

    //大小端设置
    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    //toString（以字符串形式返回数据）
    std::string toString() const;
    //toHexString（以十六进制字符串形式返回）
    std::string toHexString() const;

    //只获取内容，不修改position ----- getReadBuffers（获取当前可读数据的iovec数组，用于分散读）
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    //增加容量，不修改position   ----- getWriteBuffers（获取当前可写空间的iovec数组，用于分散写）
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    size_t getSize() const { return m_size;}
private:
    //内部辅助函数：addCapacity（扩容）
    void addCapacity(size_t size);
    size_t getCapacity() const { return m_capacity - m_position;}
private:
    // 每个内存块的基本大小（节点的大小）
    size_t m_baseSize;
    // 当前读写位置（相当于游标）
    size_t m_position;
    // 当前总容量（所有节点内存块的总和）
    size_t m_capacity;
    // 当前数据的大小（写入的数据总大小）
    size_t m_size;
    // 字节序（默认大端）
    int8_t m_endian;
    // 链表头节点
    Node* m_root;
    // 当前操作节点（用于优化读写操作，记录当前读写位置所在的节点）
    Node* m_cur;
};

}

#endif