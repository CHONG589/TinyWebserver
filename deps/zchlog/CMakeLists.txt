cmake_minimum_required(VERSION 3.10)

project(zchlog)

# 设置编译选项
if(NOT MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -Wall -g")
else()
    # Windows/MSVC 特定选项
    add_compile_options(/W3 /O2)
endif()

# 查找线程库（跨平台）
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

# --- 1. 定义库源文件 ---

# zchlog 核心源文件 (排除 main.cpp)
file(GLOB ZCHLOG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM ZCHLOG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# --- 2. 创建静态库目标 zchlog ---
add_library(zchlog STATIC ${ZCHLOG_SOURCES})

# 设置头文件包含路径 (PUBLIC: 库内部和使用者都能看到)
target_include_directories(zchlog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# 链接线程库 (PRIVATE: 只有库内部需要知道使用了线程库，还是 PUBLIC？通常头文件没暴露线程库类型则 PRIVATE)
target_link_libraries(zchlog PRIVATE Threads::Threads jsoncpp_lib)


# --- 3. 示例程序 (main) ---
# 选项：是否构建示例程序，默认 ON
option(ZCHLOG_BUILD_EXAMPLES "Build zchlog example log_test" OFF)

if(ZCHLOG_BUILD_EXAMPLES)
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

    # 这里的 main.cpp 仅作为库的使用者示例
    add_executable(log_test src/main.cpp)
    
    # 链接 zchlog 库，会自动继承 include 路径
    target_link_libraries(log_test PRIVATE zchlog)

endif()

# # --- 4. 单元测试 (仅在作为主项目时启用) ---
# if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
#     include(FetchContent)
#     FetchContent_Declare(
#       googletest
#       URL https://github.com/google/googletest/archive/refs/heads/main.zip
#     )

#     set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#     FetchContent_MakeAvailable(googletest)

#     enable_testing()

#     # 辅助函数：添加测试用例
#     function(add_zchlog_test name source_file)
#         add_executable(${name} ${source_file})
#         target_link_libraries(${name} PRIVATE zchlog GTest::gtest_main)
#         add_test(NAME ${name} COMMAND ${name})
#     endfunction()

#     add_zchlog_test(test_buffer test/test_buffer.cpp)
#     add_zchlog_test(test_formatter test/test_formatter.cpp)
#     add_zchlog_test(test_rolling test/test_log_rolling.cpp)
# endif()
