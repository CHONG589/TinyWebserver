# 遍历当前目录下的所有源文件
aux_source_directory(. TEST_SRC_LIST)

# 为每个源文件生成一个单独的可执行文件
foreach(TEST_SRC ${TEST_SRC_LIST})
    # 获取文件名（不带扩展名）作为目标名称
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    
    # 添加可执行文件
    add_executable(${TEST_NAME} ${TEST_SRC})
    
    # 链接必要的库
    target_link_libraries(${TEST_NAME}
        PRIVATE
        TinyWebServerLib  # 链接核心库
        zchlog
        Threads::Threads
    )
endforeach()