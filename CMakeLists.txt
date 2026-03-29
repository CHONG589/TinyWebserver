cmake_minimum_required(VERSION 3.10)
project(TinyWebServer)

# 设置可执行文件输出目录到 bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# 设置库的输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
# add_compile_options(-Wall -g -O2)

# 指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")

# 线程库
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

# 查找 MySQL
# find_package(MySQL REQUIRED) # 如果系统没有安装 standard mysql cmake module，可能需要手动指定路径
# 或者直接链接 mysqlclient (假设库都在标准路径)

# 查找 yaml-cpp
find_package(yaml-cpp REQUIRED)

# 包含头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# 添加子目录
# add_subdirectory(deps/jsoncpp)

# 收集源文件（排除 main.cpp 用于创建库）
file(GLOB_RECURSE LIB_SRC_FILES
    "src/base/*.cpp"
    "src/coroutine/*.cpp"
    "src/http/*.cpp"
    "src/db/*.cpp"
)

# 创建核心库
add_library(TinyWebServerLib SHARED ${LIB_SRC_FILES})
target_link_libraries(TinyWebServerLib
    PUBLIC
#    jsoncpp_lib
    Threads::Threads
    mysqlclient
    yaml-cpp
)

# 生成可执行文件
add_executable(server src/main.cpp)

# 链接库
target_link_libraries(server
    PRIVATE
    TinyWebServerLib
)

# 是否要编译测试程序
option(BUILD_TESTS "Build tests" OFF)

# 添加测试子目录
if(BUILD_TESTS)
    add_subdirectory(test)
endif()
