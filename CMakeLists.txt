cmake_minimum_required(VERSION 3.10)

project(TinyWebServer)

# C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 线程库
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

add_subdirectory(deps/zchlog)

# 定义测试可执行文件与源文件
add_executable(test_fiber
    test/iomanager_test.cpp
    code/coroutine/thread.cpp
    code/coroutine/fiber.cpp
    code/coroutine/scheduler.cpp
    code/coroutine/iomanager.cpp
    code/mutex.cpp
    code/util.cpp
    code/coroutine/timer.cpp
)

# 链接 zchlog 与线程库
target_link_libraries(test_fiber PRIVATE zchlog Threads::Threads)
