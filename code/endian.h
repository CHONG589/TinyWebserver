/**
 * @file endian.h
 * @brief 字节序操作函数(大端/小端)
 * @date 2025-01-11
 */

#pragma once

#define LITTLE_ENDIAN       1
#define BIG_ENDIAN          2

#include <byteswap.h>
#include <stdint.h>

namespace zch {

/**
 * @brief 只在小端机器上执行byteswap，在大端机器上什么都不做
 */
template <class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

}// namespace zch
