#ifndef UTIL_H__
#define UTIL_H__

#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>
#include <cxxabi.h>  // for abi::__cxa_demangle()
#include <string>
#include <vector>
#include <iostream>

/**
 * @brief 获取线程id
 * @note 这里不要把pid_t和pthread_t混淆，关于它们之的区别可参考gettid(2)
 */
pid_t GetThreadId();

/**
 * @brief 获取当前启动的毫秒数，参考clock_gettime(2)，使用CLOCK_MONOTONIC_RAW
 */
uint64_t GetElapsedMS();

/**
 * @brief 获取T类型的类型字符串
 */
template <class T>
const char *TypeToName() {
    static const char *s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}

/**
 * @brief 文件系统操作类
 */
class FSUtil {
public:
    /**
     * @brief 递归列举指定目录下所有指定后缀的常规文件，如果不指定后缀，则遍历所有文件，返回的文件名带路径
     * @param[out] files 文件列表 
     * @param[in] path 路径
     * @param[in] subfix 后缀名，比如 ".yml"
     */
    static void ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix);

    /** 
     * @brief 判断文件或目录是否存在
     * @param[in] path 路径
     * @return bool 存在返回 true，否则 false
     */
    static bool IsExist(const std::string& path);

    /** 
     * @brief 确保路径存在，存在则跳过，不存在则创建
     * @param[in] path 目录路径
     * @return void
     */
    static void MakeSurePathExist(const std::string& path);
};

class Date {
public:
    /** 
     * @brief 获取当前时间戳
     * @return time_t 自 1970-01-01 以来的秒数，如果系统没有时间，则返回 -1
     */
    static time_t Now() {
        return time(nullptr);
    }

    /** 
     * @brief 跨平台的时间结构体转换
     * @param[in] time 时间戳指针
     * @param[out] t 输出的时间结构体
     */
    static void LocalTime(const time_t* time, struct tm* t) {
#ifdef _WIN32
        localtime_s(t, time);
#else
        localtime_r(time, t);
#endif
    }

    /** 
     * @brief 获取当前时间的 tm 结构体
     * @return struct tm 时间结构体
     */
    static struct tm GetTimeSet() {
        struct tm t;
        time_t time_stamp = Date::Now();
        LocalTime(&time_stamp, &t);
        return t;
    }
};

#endif
