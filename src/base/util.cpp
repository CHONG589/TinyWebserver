#include <string.h>
#include <time.h>
#include <dirent.h>
#include <signal.h> // for kill()
#include <sys/syscall.h>
#include <sys/stat.h>
#include <execinfo.h> // for backtrace()
#include <cxxabi.h>   // for abi::__cxa_demangle()
#include <algorithm>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
#else
    #include <unistd.h>
#endif

#include "base/util.h"

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint64_t GetElapsedMS() {
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void FSUtil::ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix) {
    if (access(path.c_str(), 0) != 0) {
        return;
    }
    
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }
    struct dirent *dp = nullptr;
    while ((dp = readdir(dir)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        } else if (dp->d_type == DT_REG) {
            std::string filename(dp->d_name);
            if (subfix.empty()) {
                files.push_back(path + "/" + filename);
            } else {
                if (filename.size() < subfix.size()) {
                    continue;
                }
                if (filename.substr(filename.length() - subfix.size()) == subfix) {
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    closedir(dir);
}

/** 
 * @brief 判断文件或目录是否存在
 * @param[in] path 路径
 * @return bool 存在返回 true，否则 false
 */
bool FSUtil::IsExist(const std::string& path) {
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
 * @brief 确保路径存在，存在则跳过，不存在则创建
 * @param[in] path 目录路径
 * @return void
 */
void FSUtil::MakeSurePathExist(const std::string& path) {
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


