#include "bytearray.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>

#include "endian.h"
#include "log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/**
 * @brief 分配大小为s的内存块，next为nullptr
 */
ByteArray::Node::Node(size_t s)
    :ptr(new char[s])
    ,next(nullptr)
    ,size(s) {
}

/**
 * @brief 空节点
 */
ByteArray::Node::Node()
    :ptr(nullptr)
    ,next(nullptr)
    ,size(0) {
}

/**
 * @brief 析构函数
 */
ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size)
    :m_baseSize(base_size)
    ,m_position(0)
    ,m_capacity(base_size)
    ,m_size(0)
    ,m_endian(SYLAR_BIG_ENDIAN)
    ,m_root(new Node(base_size))
    ,m_cur(m_root) {
}

ByteArray::~ByteArray() {
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

bool ByteArray::isLittleEndian() const {
    return m_endian == SYLAR_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = SYLAR_LITTLE_ENDIAN;
    } else {
        m_endian = SYLAR_BIG_ENDIAN;
    }
}

void ByteArray::writeFint8  (int8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8 (uint8_t value) {
    write(&value, sizeof(value));
}
void ByteArray::writeFint16 (int16_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint32 (int32_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint64 (int64_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

/**
 * @brief Zigzag编码常用于将有符号整数映射到无符号整数，使得绝对值小的整数（无论正负）都对应一个较小的无符号整数
 * @param v 一个32位有符号整数的引用，即需要被转换的整数
 * @return 经过Zigzag编码后的32位无符号整数
 *         通过这个映射，原始的有符号整数被重新排序，使得0在中间，然后向正负两个方向交替扩展（这也是Zigzag名称的由来）
            | 有符号整数（v） | 无符号整数（返回值） |
            |----------------|---------------------|
            | 0             | 0                   |
            | -1            | 1                   |
            | 1             | 2                   |
            | -2            | 3                   |
            | 2             | 4                   |
            | -3            | 5                   |
            | 3             | 6                   |
            | ...           | ...                 |
           这样，原始整数的绝对值越小，编码后的值也越小。这对于后续使用变长整数编码（如Varint）压缩数据非常有利，因为小的数值占用的字节数更少
 */
static uint32_t EncodeZigzag32(const int32_t& v) {
    if(v < 0) { // 当输入为负数（v < 0）时
        // 先取负值（得到正数），然后转换为无符号整数，接着乘以2，最后再减1
        // -1 -> (1)*2-1 = 1        -2 -> (2)*2-1 = 3       -3 -> (3)*2-1 = 5   这样负数部分就映射到了奇数（1, 3, 5, ...）
        return ((uint32_t)(-v)) * 2 - 1;
    } else { // 当输入为非负数（v >= 0）时
        // 直接乘以2 -- eg: 0 -> 0   1 -> 2  2 -> 4                              这样非负数部分就映射到了偶数（0, 2, 4, ...）
        return v * 2;
    }
}

/**
 * @brief 同上 只不过是64位
 */
static uint64_t EncodeZigzag64(const int64_t& v) {
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

/*
    举个例子：设原始数 n=-1。
    - 编码：Zigzag编码：`(-1)*2*(-1) - 1`? 不对，重新用正确公式：编码公式应该是：`(n << 1) ^ (n >> 31)`（32位）。
    具体：n=-1，二进制（补码）为：0xFFFFFFFF。
    左移一位：0xFFFFFFFE。
    算术右移31位：0xFFFFFFFF（因为符号位为1，右移填充1）。
    异或：0xFFFFFFFE ^ 0xFFFFFFFF = 0x00000001（即1）。
    - 所以，v=1。
    - 解码：`v >> 1` 得到 0（即0x00000000）。
    `v & 1` 得到1，所以 `-1` 的补码是0xFFFFFFFF。
    0x00000000 ^ 0xFFFFFFFF = 0xFFFFFFFF，即-1（补码）。正确。
    再举一个例子：n=-2。
    - 编码：n=-2，补码：0xFFFFFFFE。
    左移一位：0xFFFFFFFC。
    算术右移31位：0xFFFFFFFF。
    异或：0xFFFFFFFC ^ 0xFFFFFFFF = 0x00000003（即3）。
    - 解码：v=3。
    `v >> 1` 得到1（即0x00000001）。
    `v & 1` 得到1，所以 `-(1)` 是0xFFFFFFFF。
    0x00000001 ^ 0xFFFFFFFF = 0xFFFFFFFE，即-2。正确。
    非负数的例子：n=1。
    - 编码：n=1，补码：0x00000001。
    左移一位：0x00000002。
    算术右移31位：0x00000000（因为符号位0，右移填充0）。
    异或：0x00000002 ^ 0x00000000 = 0x00000002（即2）。
    - 解码：v=2。
    `v >> 1` 得到1（0x00000001）。
    `v & 1` 得到0，所以 `-0` 是0。
    1 ^ 0 = 1。正确。
    另一个非负例子：n=0。
    - 编码：0左移1位还是0，右移31位也是0，异或后0。
    - 解码：v=0。
    `v >> 1` 是0，`v & 1` 是0，结果是0。正确。
*/

/**
 * @brief 使用Zigzag编码的32位无符号整数（`uint32_t`）解码为一个有符号整数（`int32_t`）
    ### Zigzag编码原理
                        Zigzag编码通过以下方式将有符号整数映射为无符号整数：
                        - 对于非负数（包括0）：使用偶数表示，即 `n * 2`。
                        - 对于负数：使用奇数表示，即 `(-n) * 2 - 1`。
 */
static int32_t DecodeZigzag32(const uint32_t& v) {
    // **`v >> 1`**：将无符号整数 `v` 右移一位。这相当于除以2（取整数部分）。
    //  在Zigzag编码中：
    //                  - 如果原始数是非负数（n≥0），编码为 `2n`，右移一位得到 `n`。
    //                  - 如果原始数是负数（n<0），编码为 `-2n-1`，右移一位得到 `(-2n-1)/2`，即 `-n - 0.5`

    // **`v & 1`**：取 `v` 的最低位。
    //   根据Zigzag编码，最低位是符号标志：
    //                                - 如果最低位为0，表示原始数是非负数。
    //                                - 如果最低位为1，表示原始数是负数。

    // **`-(v & 1)`**：这是一个关键操作。这里利用了负数的二进制表示（补码）。注意，`-(v & 1)` 的值只有两种可能：
    //                                - 当 `v & 1` 为0时，结果为0（因为-0还是0）。
    //                                - 当 `v & 1` 为1时，结果为-1（因为-1的补码表示是全1，即0xFFFFFFFF）

    //  **异或操作 `(v >> 1) ^ -(v & 1)`**：
    //           - 情况1：`v` 是偶数（即原始非负数），则 `v & 1 = 0`，所以 `-(v & 1)=0`，那么结果就是 `v >> 1`（即原始的非负数）。
    //           - 情况2：`v` 是奇数（即原始负数），则 `v & 1 = 1`，所以 `-(v & 1) = -1`（即32位全1）。异或一个全1的数，相当于按位取反。那么表达式变为：`(v >> 1) ^ 0xFFFFFFFF`。
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}


void ByteArray::writeInt32  (int32_t value) {
    writeUint32(EncodeZigzag32(value));
}

/**
 * @brief 将一个32位无符号整数（`uint32_t value`）以变长方式编码（通常称为Varints）并写入到字节数组中。
 *        Varints是一种用可变长度字节表示整数的方法，较小的整数占用较少的字节
 */

/**
    ### 举例说明：
    假设我们要写入的整数是`300`（二进制为：`1 0010 1100`，即16进制0x12C）。
    - 第一轮循环：
    - `300 >= 0x80` -> 进入循环。
    - 取低7位：`300 & 0x7F` -> `0x2C`（即44）。
    - 设置最高位为1：`0x2C | 0x80` -> `0xAC`。
    - 存入`tmp[0]`。
    - 右移7位：`300 >> 7` -> 2（因为300/(2^7)=2余44）。
    - 第二轮循环：
    - `2 < 0x80` -> 不进入循环。
    - 然后，将2存入`tmp[1]`。
    - 最后，调用`write(tmp, 2)`，写入两个字节：0xAC 和 0x02。
        实际存储的字节序列是：0xAC, 0x02。注意，在Varints编码中，最低有效组在前面（即小端序）。所以解码时，先读到0xAC（最高位为1，表示还有下一个字节），
        然后0x02（最高位为0，结束）。去掉最高位后，得到两个7位组：0x2C和0x02，然后重新组合：0x02 << 7 | 0x2C = 2*128 + 44 = 256+44=300。
 */
void ByteArray::writeUint32 (uint32_t value) {
    // `tmp`是一个大小为5的字节数组。为什么是5？因为对于32位整数，使用7位一组进行编码，最多需要5个字节（因为5*7=35>32）
    uint8_t tmp[5];
    // `i`是索引，用于跟踪`tmp`数组中已使用的字节数
    uint8_t i = 0;
    // 当`value`大于或等于`0x80`（即128）时，说明还有至少7位需要编码（因为0x80是第8位为1，其余为0，所以如果value小于128，则可以用一个字节表示）
    while(value >= 0x80) {
        // `value & 0x7F`：取`value`的最低7位。
        // `| 0x80`：将最高位（第8位）设置为1，表示后续还有字节（这是Varints的规则：如果最高位为1，则表示下一个字节还有有效数据；为0则表示结束）
        // 将结果存入`tmp[i]`，然后`i`自增
        tmp[i++] = (value & 0x7F) | 0x80;
        // `value >>= 7`：将`value`右移7位，处理下一个7位组
        value >>= 7;
    }
    // 处理最后一个字节
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeInt64  (int64_t value) {
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64 (uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeFloat  (float value) {
    // 因为`float`类型通常也是4字节（根据IEEE 754标准），所以我们可以用一个32位整数来存储浮点数的二进制表示
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

void ByteArray::writeDouble (double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string& value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string& value) {
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string& value) {
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringVint(const std::string& value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLength(const std::string& value) {
    write(value.c_str(), value.size());
}

int8_t   ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t  ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == SYLAR_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

int16_t  ByteArray::readFint16() {
    XX(int16_t);
}
uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}

int32_t  ByteArray::readFint32() {
    XX(int32_t);
}

uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}

int64_t  ByteArray::readFint64() {
    XX(int64_t);
}

uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}

#undef XX

int32_t  ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}

/**
 * @brief 用于从字节数组中读取一个32位无符号整数（uint32_t）。该函数使用了一种变长编码（类似于LEB128，但这里是每7位一组，最多32位）来读取数据
 */

/*
    ### 示例
    假设我们要读取一个变长编码的整数，字节序列为：`0x81, 0x82, 0x03`（十六进制）。
    1. 读取第一个字节 `0x81`（二进制 `10000001`）：
    - 最高位为1，所以取低7位：`0x01`。
    - 左移0位：`0x01`。
    - 此时 `result = 0x01`。
    - 继续循环。
    2. 读取第二个字节 `0x82`（二进制 `10000010`）：
    - 最高位为1，取低7位：`0x02`。
    - 左移7位：`0x02 << 7 = 0x100`。
    - 与result按位或：`0x01 | 0x100 = 0x101`。
    - 继续循环。
    3. 读取第三个字节 `0x03`（二进制 `00000011`）：
    - 最高位为0，所以直接取 `0x03`。
    - 左移14位：`0x03 << 14 = 0xC000`。
    - 与result按位或：`0x101 | 0xC000 = 0xC101`。
    - 跳出循环。
    最终结果 `result = 0xC101`（十进制为49409）。
    ### 注意
    这种编码方式可以节省空间，因为较小的数值可以用更少的字节表示。
    但是，对于大于等于 `2^28` 的数值，需要5个字节（而固定长度的uint32_t只需要4字节），所以它并不是对所有情况都节省空间，
    但在实际应用中，如果数值普遍较小，则整体上可以节省空间。
*/
uint32_t ByteArray::readUint32() {
    // 初始化一个32位无符号整数变量 `result` 为0，用于存储最终结果。
    uint32_t result = 0;
    // 开始一个循环，循环变量 `i` 从0开始，每次增加7，直到达到或超过32（因为32位整数最多需要5个7位的组，因为5*7=35，
    // 但这里循环条件为`i<32`，实际上会循环5次：i=0,7,14,21,28）
    for(int i = 0; i < 32; i += 7) {
        // 调用 `readFuint8()` 函数读取一个字节（8位）的数据，存储到 `uint8_t` 类型的变量 `b` 中。
        uint8_t b = readFuint8();
        // 判断读取的字节 `b` 的最高位（第7位）是否为0。在变长编码中，每个字节的最高位用作标志位：
        // 如果为0，表示这是最后一个字节；如果为1，表示后面还有字节。因为 `0x80` 的二进制是 `10000000`，所以 `b < 0x80` 即最高位为0
        if(b < 0x80) {
            // 如果最高位为0，则将当前字节 `b` 的值（低7位有效）左移 `i` 位，然后与 `result` 进行按位或操作，
            // 跳出循环，因为遇到结束字节将结果合并到 `result` 中。由于 `b` 小于0x80，所以它本身的值就是低7位，不需要额外处理
            result |= ((uint32_t)b) << i;
            // 跳出循环，因为遇到结束字节
            break;
        } else { // 如果字节 `b` 的最高位为1（即 `b >= 0x80`），则执行else分支
            // 将字节 `b` 的低7位（通过 `b & 0x7f` 得到）转换为32位无符号整数，然后左移 `i` 位，再与 `result` 进行按位或操作，将低7位合并到 `result` 的相应位置
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

int64_t  ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

float    ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double   ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint() {
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear() {
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = NULL;
}

/**
 * @brief 用于将指定缓冲区中的数据写入到字节数组中
 * @param buf 指向要写入数据的缓冲区的指针
 * @param size 要写入的数据大小（字节数）  
 */
void ByteArray::write(const void* buf, size_t size) {
    // 如果要写入的数据大小为0，则直接返回
    if(size == 0) {
        return;
    }
    // 调用`addCapacity`函数，确保当前字节数组有足够的空间容纳要写入的`size`字节数据。
    // 这个函数内部会处理扩容逻辑（如果需要）
    addCapacity(size);

    // npos: 当前节点（`m_cur`）中可写入数据的起始位置（偏移量）。它通过当前写入位置`m_position`对每个节点的大小（`m_baseSize`）取模得到
    size_t npos = m_position % m_baseSize;
    // ncap: 当前节点剩余的可写入空间（从`npos`到节点末尾）
    size_t ncap = m_cur->size - npos;
    // bpos: 在源缓冲区（`buf`）中已经写入的数据位置（偏移量），初始为0
    size_t bpos = 0;

    //循环写入数据
    while(size > 0) {
        // 当前节点的剩余空间足够容纳剩余数据
        if(ncap >= size) {
            //将剩余数据（`size`字节）从源缓冲区（`buf + bpos`）复制到当前节点的剩余空间（`m_cur->ptr + npos`）
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            //如果复制后当前节点正好被写满（即`npos + size`等于节点大小），则将当前节点指针`m_cur`移动到下一个节点（`m_cur->next`）
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            //更新当前写入位置`m_position`（增加`size`）
            m_position += size;
            //更新源缓冲区偏移`bpos`（增加`size`）
            bpos += size;
            //将剩余数据大小`size`置为0，退出循环
            size = 0;
        } else { //当前节点的剩余空间不足以容纳剩余数据
            //将当前节点剩余空间（`ncap`字节）用源缓冲区中相应位置的数据填满
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            //更新当前写入位置`m_position`（增加`ncap`）
            m_position += ncap;
            //更新源缓冲区偏移`bpos`（增加`ncap`）
            bpos += ncap;
            //更新剩余要写入的数据大小（`size -= ncap`）
            size -= ncap;
            //将当前节点指针移动到下一个节点（`m_cur = m_cur->next`）
            m_cur = m_cur->next;
            //重置`ncap`为新节点的总大小（因为新节点还没有写入任何数据，所以剩余空间就是节点大小），并将`npos`重置为0（从新节点的起始位置写入）
            ncap = m_cur->size;
            //更新字节数组的总大小
            npos = 0;
        }
    }

    //写入完成后，如果当前写入位置`m_position`超过了字节数组原有的数据大小`m_size`，则将`m_size`更新为`m_position`。这表示字节数组的有效数据扩展到了`m_position`的位置。
    if(m_position > m_size) {
        m_size = m_position;
    }
}

/**
 * @brief 从字节数组中读取数据到指定的缓冲区
 * @param buf 目标缓冲区的指针，读取的数据将拷贝到这里
 * @param size 要读取的字节数
 */
void ByteArray::read(void* buf, size_t size) {
    // 首先检查要读取的字节数`size`是否大于当前可读的字节数（通过`getReadSize()`获取）。
    // 如果大于，则抛出`std::out_of_range`异常
    if(size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    // 计算当前块（`m_cur`）中的读取位置：`npos = m_position % m_baseSize`。
    // 这里`m_position`是当前字节数组的全局读取位置（从开始到当前位置的字节数），所以取余得到在当前块内的偏移。
    size_t npos = m_position % m_baseSize;
    // 计算当前块内从`npos`开始还有多少字节可读：`ncap = m_cur->size - npos`。
    // 注意，这里`m_cur->size`可能小于`m_baseSize`（例如最后一个块可能没有写满）
    size_t ncap = m_cur->size - npos;
    // 初始化目标缓冲区的偏移`bpos`为0
    size_t bpos = 0;
    // 循环读取，直到读取完`size`个字节
    while(size > 0) {
        // 如果当前块的剩余可读字节数（`ncap`）大于等于剩余需要读取的字节数（`size`）
        if(ncap >= size) {
            // 则将剩余需要读取的数据从当前块的`npos`位置开始拷贝到目标缓冲区偏移`bpos`处
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            // 如果拷贝后当前块刚好被读完（即`m_cur->size == npos + size`），那么将当前块指针`m_cur`指向下一个块（`m_cur = m_cur->next`）
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            // 更新全局读取位置`m_position`增加`size`
            m_position += size;
            // 更新目标缓冲区偏移`bpos`增加`size`（虽然之后会退出循环，但这里逻辑正确）
            bpos += size;
            // 将剩余需要读取的字节数`size`置为0，然后退出循环
            size = 0;
        } else { // 当前块的剩余可读字节数不够
            // 将当前块剩余的所有字节（`ncap`个）拷贝到目标缓冲区偏移`bpos`处
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            // 更新全局读取位置`m_position`增加`ncap`
            m_position += ncap;
            // 更新目标缓冲区偏移`bpos`增加`ncap`
            bpos += ncap;
            // 剩余需要读取的字节数`size`减去`ncap`
            size -= ncap;
            // 将当前块指针`m_cur`指向下一个块（`m_cur = m_cur->next`）
            m_cur = m_cur->next;
            // 更新下一个块的可读字节数`ncap`为`m_cur->size`（因为下一个块从头开始读，所以`npos`置0）
            ncap = m_cur->size;
            // 将`npos`置0（因为下一个块从0偏移开始读）
            npos = 0;
        }
    }
}

/**
 * @brief 函数从指定的位置（`position`）开始读取一定大小（`size`）的数据到提供的缓冲区（`buf`）中
 * @param buf 目标缓冲区，读取的数据将复制到这里。
 * @param size 要读取的字节数。
 * @param position 从字节数组的哪个位置开始读取。
 */
void ByteArray::read(void* buf, size_t size, size_t position) const {
    // 首先检查要读取的字节数是否超过了可读范围（通过`getReadSize()`获取）。如果超过，抛出`std::out_of_range`异常。
    if(size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    // npos: 计算起始位置在第一个节点内的偏移。`m_baseSize`可能是每个节点的基础大小（比如节点内存块的大小），通过取模得到在节点内的偏移
    size_t npos = position % m_baseSize;
    // ncap: 当前节点（`m_cur`）从偏移`npos`开始到节点末尾的剩余字节数。
    size_t ncap = m_cur->size - npos;
    // bpos: 目标缓冲区的当前写入位置，初始为0
    size_t bpos = 0;
    // cur: 当前节点指针，初始为`m_cur`（当前操作位置的节点）
    Node* cur = m_cur;
    // 剩下代码与上边一致
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

/**
 * @brief 设置当前的位置指针
 *        将当前的位置指针设置到字节数组的第`v`个字节处。如果`v`超出数组范围，抛出异常。
 *        同时更新内部指针`m_cur`（指向当前块的指针）和`m_position`（当前全局位置）
 * @param v 要设置的位置（索引）
 */
void ByteArray::setPosition(size_t v) {
    // 如果传入的位置`v`大于整个字节数组的大小`m_size`，则抛出`std::out_of_range`异常
    if(v > m_size) {
        throw std::out_of_range("set_position out of range");
    }
    // 更新成员变量`m_position`为`v`，表示当前全局位置
    m_position = v;
    // m_root：链表的头节点指针。这里将当前块指针`m_cur`重置为链表开头
    m_cur = m_root;
    // 遍历链表，定位到目标块 循环条件：当前剩余的位置`v`大于当前块的大小（`m_cur->size`）
    while(v > m_cur->size) {
        // 如果条件成立，说明目标位置不在当前块内，则从`v`中减去当前块的大小，并将`m_cur`移动到下一个块
        v -= m_cur->size;
        // 循环继续，直到`v`小于等于当前块的大小，此时目标位置就在当前块内
        m_cur = m_cur->next;
    }
    // 处理刚好位于块末尾的情况
    if(v == m_cur->size) {
        // 如果经过上面的循环后，`v`正好等于当前块的大小，说明目标位置位于当前块的末尾（即下一个块的起始位置）。
        // 因此，将当前块指针`m_cur`移动到下一个块。注意，此时位置`v`在该块内的偏移量应为0（即块的开头）
        m_cur = m_cur->next;
    }
}

/**
 * @brief 将`ByteArray`内部存储的数据写入到指定的文件中
 * @param name 文件名
 */
bool ByteArray::writeToFile(const std::string& name) const {
    // 创建了一个输出文件流对象`ofs`
    std::ofstream ofs;
    // 使用`open`成员函数打开文件，文件名为参数`name`
    // std::ios::trunc：如果文件已存在，则先清空文件内容
    // std::ios::binary：以二进制模式打开文件，确保数据原样写入
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        SYLAR_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // getReadSize()`：获取当前可读取的数据大小（即从当前读取位置到有效数据末尾的长度）。
    // 注意，虽然函数名是`getReadSize`，但在这个上下文中，它表示的是待写入的数据量
    int64_t read_size = getReadSize();
    // m_position：当前读取位置的偏移量（在字节数组中的位置），将其赋值给`pos`
    int64_t pos = m_position;
    // m_cur：指向当前读取位置所在的节点（`ByteArray`是由多个节点组成的链表结构），赋值给`cur`
    Node* cur = m_cur;

    // 循环写入数据 只要还有待写入数据（`read_size > 0`），就继续循环
    while(read_size > 0) {
        // 计算当前节点内的起始位置
        // diff：表示当前节点内已经处理过的偏移量（即从节点起始位置到当前读取位置的偏移）
        int diff = pos % m_baseSize;
        // 如果剩余待写入数据大于一个节点大小（`m_baseSize`），则本次最多写一个节点大小（减去`diff`后的部分）
        // 否则，本次写剩余的数据量（同样减去`diff`）
        // 注意：`len`必须确保不会超过当前节点剩余的长度（即`m_baseSize - diff`）
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        // 将当前节点（`cur`）的数据缓冲区（`ptr`）从偏移`diff`处开始，写入`len`个字节到文件流
        ofs.write(cur->ptr + diff, len);
        // 移动到下一个节点
        cur = cur->next;
        // 更新位置（在字节数组中的绝对位置）
        pos += len;
        // 减少剩余待写入数据量
        read_size -= len;
    }

    return true;
}

/**
 * @brief 从文件中读取数据并写入到当前对象中
 * @param name 文件名
 */
bool ByteArray::readFromFile(const std::string& name) {
    // 创建了一个文件输入流对象`ifs`
    std::ifstream ifs;
    // 以二进制模式打开文件，这样可以确保读取的内容不会被解释（例如，不会进行换行符转换）
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        SYLAR_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // 使用`std::shared_ptr`管理动态分配的字符数组，大小为`m_baseSize`（可能是类的一个成员变量，表示每次读取的块大小）
    // 自定义删除器：`[](char* ptr) { delete[] ptr; }`，确保数组被正确释放（使用`delete[]`）
    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr;});
    // 循环读取文件 -- 循环直到文件结束（`eof()`返回`true`）
    while(!ifs.eof()) {
        // 每次读取最多`m_baseSize`字节的数据到缓冲区`buff`中
        // buff.get()：获取`shared_ptr`管理的原始指针     ifs.gcount()：返回上一次读取操作实际读取的字节数。
        ifs.read(buff.get(), m_baseSize);
        // 调用`write(buff.get(), ifs.gcount())`将读取到的数据写入到当前对象中（`write`可能是该类的另一个成员函数，用于将数据追加到内部缓冲区）
        write(buff.get(), ifs.gcount());
    }
    return true;
}

/**
 * @brief 为字节数组增加容量
 * @param size 需要增加的容量大小
 */
void ByteArray::addCapacity(size_t size) {
    // 如果请求增加的容量为0，则直接返回，无需操作
    if(size == 0) {
        return;
    }
    // 获取当前总容量 `old_cap`（通过 `getCapacity()` 成员函数）
    size_t old_cap = getCapacity();
    // 如果当前容量已经大于或等于需要增加的容量（注意：这里应该是需要达到的总容量？），则直接返回
    if(old_cap >= size) {
        return;
    }

    // --计算需要新增的块数-- 
    // 计算还需要增加的容量：`size = size - old_cap`（此时 `size` 表示额外需要增加的字节数）
    size = size - old_cap;
    // 然后计算需要分配多少个新的块（每个块大小为 `m_baseSize`）
    size_t count = (size / m_baseSize) + (((size % m_baseSize) > old_cap) ? 1 : 0);
    // 遍历到链表末尾
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }

    // first：用于记录新增的第一个节点（块）
    Node* first = NULL;
    // 循环 `count` 次
    for(size_t i = 0; i < count; ++i) {
        // 在链表末尾添加一个新节点（块），大小为 `m_baseSize`
        tmp->next = new Node(m_baseSize);
        //  如果是新增的第一个节点，则将其记录在 `first` 中
        if(first == NULL) {
            first = tmp->next;
        }
        // 更新 `tmp` 为新节点，以便下一次循环继续在末尾添加
        tmp = tmp->next;
        // 总容量 `m_capacity` 每次增加一个块的大小（`m_baseSize`）
        m_capacity += m_baseSize;
    }

    // 如果原始容量为0（即原本没有数据块），那么将当前指针 `m_cur` 指向新增的第一个块。这可能是为了后续写入数据时从第一个块开始
    if(old_cap == 0) {
        m_cur = first;
    }
}

std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}

/**
 * @brief 将当前可读的数据填充到传入的`iovec`向量中，以便进行分散读（scatter read）
 * @param buffers 用于存放分散的内存块
 * @param len 想要读取的长度
 * @return 实际读取的长度
 */
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    // 首先，将`len`调整为不超过当前可读数据大小（通过`getReadSize()`获取）的值
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }
    // ---初始化变量---
    // 保存实际要读取的长度
    uint64_t size = len;

    // 当前节点中数据的起始位置（偏移）
    size_t npos = m_position % m_baseSize;
    // 当前节点中剩余可读数据量
    size_t ncap = m_cur->size - npos;
    // `iovec`向量
    struct iovec iov;
    // 当前节点指针
    Node* cur = m_cur;

    // 只要还有需要读取的数据（`len>0`）就继续
    while(len > 0) {
        // 当前节点的剩余容量（`ncap`）大于等于剩余需要读取的长度（`len`）
        if(ncap >= len) {
            // 只需将当前节点中从`npos`开始长度为`len`的数据块加入`buffers`，并将`len`置0（表示读取完成）
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else { // 当前节点的剩余容量小于剩余需要读取的长度
            // 此时，将当前节点剩余的所有数据（`ncap`）加入`buffers`
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            // 然后更新剩余需要读取的长度（`len -= ncap`）
            len -= ncap;
            // 并将当前节点指向下一个节点（`cur = cur->next`）
            cur = cur->next;
            // 同时更新`ncap`为新节点的总容量（因为新节点从0开始）
            ncap = cur->size;
            // `npos`重置为0
            npos = 0;
        }
        // 每次循环都会将一个`iovec`结构加入`buffers`向量
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers
                                ,uint64_t len, uint64_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if(len == 0) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

}