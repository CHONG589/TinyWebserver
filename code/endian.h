/**
 * @file endian.h
 * @brief 字节序操作函数(大端/小端)
 * @date 2025-01-11
 */

#ifndef __ZCH_ENDIAN_H__
#define __ZCH_ENDIAN_H__

#define ZCH_LITTLE_ENDIAN       1
#define ZCH_BIG_ENDIAN          2

#include <byteswap.h>
#include <stdint.h>

namespace zch {

/**
 * @brief 8字节类型的字节序转化
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

/**
 * @brief 4字节类型的字节序转化
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 2字节类型的字节序转化
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN            //系统的字节序为大端
#define ZCH_BYTE_ORDER ZCH_BIG_ENDIAN   //则用一个别名表示
#else
#define ZCH_BYTE_ORDER ZCH_LITTLE_ENDIAN
#endif

#if ZCH_BYTE_ORDER == ZCH_BIG_ENDIAN

/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template <class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

/**
 * @brief 只在大端机器上执行byteswap, 在小端机器上什么都不做
 */
template <class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

#else

/**
 * @brief 只在小端机器上执行byteswap, 在大端机器上什么都不做
 */
template <class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

/**
 * @brief 只在大端机器上执行byteswap, 在小端机器上什么都不做
 */
template <class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

}// namespace zch

#endif
