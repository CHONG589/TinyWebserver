/**
 * @file noncopyable.h
 * @brief 不可拷贝对象封装
 * @author zch 
 * @date 2025-03-29
 */

#ifndef NONCOPYABLE_H__
#define NONCOPYABLE_H__

/**
 * @brief 对象无法拷贝,赋值
 */
class Noncopyable {
public:
    /**
     * @brief 默认构造函数
     */
    Noncopyable() = default;

    /**
     * @brief 默认析构函数
     */
    ~Noncopyable() = default;

    /**
     * @brief 拷贝构造函数(禁用)
     */
    Noncopyable(const Noncopyable&) = delete;

    /**
     * @brief 赋值函数(禁用)
     */
    Noncopyable& operator=(const Noncopyable&) = delete;
};

#endif
