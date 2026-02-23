# 项目描述

该项目是在 TinyWebServer 的基础上，参考 sylar 的项目增加了协程部分，以及地址封装、Socket封装、fd 封装等。

TinyWebserver 参考：[Modern C++ 风格的 TinyWebServer](https://github.com/JehanRio/TinyWebServer)

sylar：[C++高性能分布式服务器框架](https://github.com/sylar-yin/sylar)

后续还会继续优化项目，已实现的模块如下：

- [x] 线程模块
- [x] 协程模块
- [x] 协程调度模块
- [x] IO 协程调度模块
- [x] 定时器模块
- [x] Address 模块
- [x] Socket 模块
- [x] TcpServer 类
- [ ] 流式日志实现
- [ ] 配置模块
- [ ] HTTP (重新封装)
- [ ] Hook 模块
- [ ] Stream 模块
- [ ] ByteArray 模块
- [ ] 环境变量模块
- [ ] 守护进程

# 运行

```
cd TinyWebserver

make

./server
```

# 测试结果

在 CPU i5，内存 4G 的机器上运行。

- 在第一版本没有协程，双 ET 模式下，用 webbench 实现了 8K 的并发量，运行 10s，QPS 为 1731。

![V1_0](docs/images/V1_0.png)

- 在第二版本有协程，但是没有采用 ET 模式 (还未实现)，用 webbench 实现了 8K 的并发量，运行 10s，QPS 为 1524。

![V1_1](docs/images/V1_1.png)

怎么性能还下降了？是不是因为没有用到 ET 模式？(待优化的地方)

# 遇到的问题记录

stat 函数不能解析中文字符串，否则会出错，自己在解析路径的时候，由于路径中有中文，导致解析失败，项目运行不了，导致找了好久好久。

# 优化

- 用 CMake 代替 Makefile。

- 关于 ET 模式的处理，在各种读写中，如：

```Cpp
ssize_t HttpConn::read(int* saveErrno) {
    LOG_INFO("start Read");
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET); // ET:边沿触发要一次性全部读出
    return len;
}
```

- 项目中实际是没有用到 iomanager 这个模块的，所以对于 ET 模式、定时事件、Hook 都是没有用到的，项目中只是直接将任务用 `iom_schedule()` 添加到调度器中的，也没有用到 iomanager 的 epoll 。 

# 待优化的目录结构

```
TinyWebserver/
├── CMakeLists.txt          # 顶层构建文件
├── README.md
├── build/                  # 构建目录 (已忽略)
├── config/                 # [新增] 配置文件目录
│   ├── log_config.json     # 日志配置
│   └── db_config.json      # 数据库配置
├── deps/                   # [重命名] 第三方依赖
│   ├── zchlog/             # [移动] 日志库作为子模块
│   └── webbench/           # [移动] 压测工具
├── include/                # [新增] 公共头文件 (对外暴露的接口)
│   ├── tinyweb/            # 项目命名空间
│   │   ├── http/
│   │   ├── db/
│   │   └── base/
├── src/                    # [重命名 code] 核心源码
│   ├── main.cpp            # 程序入口
│   ├── base/               # 基础组件
│   │   ├── buffer.cpp
│   │   ├── socket.cpp
│   │   ├── address.cpp
│   │   └── tcp_server.cpp
│   ├── coroutine/          # 协程核心
│   │   ├── fiber.cpp
│   │   ├── scheduler.cpp
│   │   └── iomanager.cpp
│   ├── http/               # HTTP 业务
│   │   ├── http_server.cpp
│   │   ├── httpconn.cpp
│   │   ├── httprequest.cpp
│   │   └── httpresponse.cpp
│   └── db/                 # [合并] 数据库相关
│       ├── sqlconnpool.cpp # 现有的简单连接池
│       └── connect_pool/   # [可选] 复杂的 ConnectPool 代码移到这里
└── tests/                  # [新增] 单元测试
    ├── test_fiber.cpp
    └── test_http.cpp
```
