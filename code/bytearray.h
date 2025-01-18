/**
 * @file bytearray.h
 * @brief 二进制数组(序列化/反序列化)
 * @author zch
 * @date 2025-01-18
 */

#ifndef __ZCH_BYTEARRAY_H__
#define __ZCH_BYTEARRAY_H__

#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

namespace zch {

/**
 * @brief 二进制数组,提供基础类型的序列化,反序列化功能
 */
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief ByteArray的存储节点
     */
    struct Node {
        /**
         * @brief 构造指定大小的内存块
         * @param[in] s 内存块字节数
         */
        Node(size_t s);

        /**
         * 无参构造函数
         */
        Node();

        /**
         * 析构函数,释放内存
         */
        ~Node();

        // 内存块地址指针
        char* ptr;
        // 下一个内存块地址
        Node* next;
        // 内存块大小
        size_t size;
    };

    /**
     * @brief 使用指定长度的内存块构造ByteArray
     * @param[in] base_size 内存块大小
     */
    ByteArray(size_t base_size = 4096);

    ~ByteArray();

    /**
     * @brief 写入固定长度int8_t类型的数据
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFint8(int8_t value);

    /**
     * @brief 写入固定长度uint8_t类型的数据
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFuint8(uint8_t value);

    /**
     * @brief 写入固定长度int16_t类型的数据(大端/小端)
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFint16(int16_t value);

    /**
     * @brief 写入固定长度uint16_t类型的数据(大端/小端)
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFuint16(uint16_t value);

    /**
     * @brief 写入固定长度int32_t类型的数据(大端/小端)
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFint32(int32_t value);

    /**
     * @brief 写入固定长度uint32_t类型的数据(大端/小端)
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFuint32(uint32_t value);

    /**
     * @brief 写入固定长度int64_t类型的数据(大端/小端)
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFint64(int64_t value);

    /**
     * @brief 写入固定长度uint64_t类型的数据(大端/小端)
     * @post m_position += sizeof(value)
     *       如果m_position > m_size 则 m_size = m_position
     */
    void writeFuint64(uint64_t value);
};

}//namespace zch

#endif
