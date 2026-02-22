/**
 * @file util.hpp
 * @brief 工具函数的封装
 * @author zch
 * @date 2025-11-02
 */

#ifndef ZCH_UTIL_H__
#define ZCH_UTIL_H__

#include <iostream>
#include <string>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
#else
    #include <unistd.h>
#endif

namespace zch {

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

    class File {
	public:
        /** 
         * @brief 判断文件或目录是否存在
         * @param[in] path 路径
         * @return bool 存在返回 true，否则 false
         */
		static bool IsExist(const std::string& path) {
#ifdef _WIN32
            // _access 返回 0 表示存在
            return _access(path.c_str(), 0) == 0;
#else
			struct stat st;
			if (stat(path.c_str(), &st) == 0) {
				return true;
			}
			else {
				return false;
			}
#endif
		}

        /** 
         * @brief 获取文件所在目录路径
         * @param[in] path 文件完整路径
         * @return std::string 目录路径（含末尾分隔符）
         */
		static std::string GetDirPath(const std::string& path) {
			// 如：/home/abc/test/a.txt，目录路径为：
        	// /home/abc/test/ 或者 /home/abc/test
			size_t pos = path.find_last_of("/\\");
			if (pos == std::string::npos) {
				return ".";
			}
			else {
				return path.substr(0, pos + 1);
			}
		}

        /** 
         * @brief 递归创建目录
         * @param[in] path 目录路径
         * @return void
         */
		static void CreateDirectory(const std::string& path) {
			if (path.size() == 0) {
                return;
            }

#ifndef _WIN32
			umask(0);
#endif
			// 测试样例：
			// /home/abc/test/  	/home/abc/test
			// text    				test/
			// ./test  				./test/
			size_t cur = 0, pos = 0;
			std::string parent_dir;

            // cur 是当前位置，pos 是目录分隔符位置
			while (cur < path.size()) {
                // 查找下一个分隔符
				pos = path.find_first_of("/\\", cur);
				if (pos == std::string::npos) {
                    // 没找到分隔符，说明到了最后一层目录（或者是个文件）
                    // 比如 path="logs"，pos=npos，则直接创建 logs
					parent_dir = path;
                    cur = path.size() + 1; // 结束循环
				} else {
                    // 找到了分隔符，比如 path="logs/a.log"，pos=4
                    // parent_dir = "logs"
					parent_dir = path.substr(0, pos + 1);
					cur = pos + 1;
				}
                
                // 检查这一级目录是否存在，不存在则创建
				if (!IsExist(parent_dir)) {
#ifdef _WIN32
                    _mkdir(parent_dir.c_str());
#else
					mkdir(parent_dir.c_str(), 0777);
#endif
				}
			}
		}
	};
}

#endif
