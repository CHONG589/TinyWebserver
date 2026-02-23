cmake_minimum_required(VERSION 3.10)
project(jsoncpp)

# 源文件
file(GLOB JSONCPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/lib_json/*.cpp")

# 创建静态库
add_library(jsoncpp_lib STATIC ${JSONCPP_SOURCES})

# 包含目录
target_include_directories(jsoncpp_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/lib_json>
)
