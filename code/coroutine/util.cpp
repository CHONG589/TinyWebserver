/**
 * @file util.cpp
 * @author zch
 * @date 2025-01-15
 */

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <signal.h> // for kill()
#include <sys/syscall.h>
#include <sys/stat.h>
#include <execinfo.h> // for backtrace()
#include <cxxabi.h>   // for abi::__cxa_demangle()
#include <algorithm>  // for std::transform()
#include "util.h"
#include "../log/log.h"
#include "fiber.h"

namespace zch {

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint64_t GetElapsedMS() {
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

}//namespace zch
