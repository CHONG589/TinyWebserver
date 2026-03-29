# TinyWebserver

## 简介

这是一个基于 C++11 开发的高性能 Web 服务器，结合了协程（Coroutine）和 Reactor 模型。项目在 [JehanRio/TinyWebServer](https://github.com/JehanRio/TinyWebServer) 的基础上，参考 [sylar](https://github.com/sylar-yin/sylar) 框架重构了核心组件，引入了协程调度和数据库连接池等高级特性。

主要特点：

- **核心架构**：采用 Epoll + 协程 + 线程池 的高并发模型。
- **协程调度**：实现了 N-M 协程调度器，支持协程切换和自动调度，将异步 IO 转化为同步编码风格。
- **IO协程调度**：基于 Epoll 封装了 IOManager，支持 Socket 读写事件的协程调度。
- **日志系统**：支持流式日志、多级别输出、异步写入、文件按天回滚等功能。
- **数据库连接池**：实现了 MySQL 数据库连接池，支持 RAII 机制自动管理连接生命周期，大幅提升数据库并发性能。
- **配置系统**：基于 YAML 的配置管理。

## 已实现模块

- [x] 线程与线程同步模块 (Mutex, Semaphore, Lock)
- [x] 协程核心模块 (Fiber, Scheduler)
- [x] IO协程调度模块 (IOManager, Epoll)
- [x] 网络模块 (Socket, Address, TcpServer)
- [x] HTTP 服务器模块 (HttpServer, HttpConn)，待优化
- [x] 数据库连接池 (ConnectionPool, MySQL)
- [x] 日志模块 (zchlog)
- [x] 配置模块 (JSON)
- [x] 定时器模块 (Timer)
- [ ] Hook 模块 (待完善)

## 环境要求

- Linux 环境
- C++11 及以上编译器
- CMake 3.0+
- MySQL (可选，如果启用数据库功能)

## 编译与运行

### 创建构建目录

```bash
mkdir build && cd build
```

### 编译项目

```bash
cmake ..
make -j4
```

### 运行服务器

```bash
cd .. && cd bin
./server
```

### 运行测试

将项目主目录 CMakeLists.txt 中的 BUILD_TESTS 选项设置为 ON。

```bash
# 是否要编译测试程序
option(BUILD_TESTS "Build tests" ON)
```

## 配置说明

配置文件位于 `config/` 目录下：

- `log_config.yml`: 日志系统配置（日志级别、输出路径、格式等）
- `db_config.yml`: 数据库连接配置（IP、端口、用户名、密码、连接池大小等）
- `server.yml`: 服务器相关的一些配置（ip、port、资源文件夹路径、超时时间、线程数）

## 优化与改进记录

- **构建系统**: 使用 CMake 替代 Makefile，支持多模块编译和依赖管理。
- **日志系统**: 引入 zchlog 替代简易日志，支持更丰富的日志格式和性能优化。
- **数据库**: 实现 ConnectionPool 连接池，支持多线程高并发下的数据库访问，并在测试中验证了显著的性能提升（多线程插入性能提升约 4-5 倍）。
- **IO模型**: 正在完善基于协程的 IO 调度，旨在解决传统回调地狱问题，提供同步视角的异步编程体验。
- **Bug修复**: 修复了中文路径解析问题；修复了 ConnectionPool 析构时的线程安全问题；修复了 HttpServer 编译错误。
- **配置文件优化**: 由原来的 Json 作为配置模块，更改为 yaml 作为配置模块。
- **日志系统完善**: 重构 zchlog 日志系统架构，由原来的各模块分布在不同文件 ==> 整合到一个文件中，部分参照 sylar 的日志进行优化。

## 待完成

- 未提供设置是否用 ET 模式的接口
- isKeepAlive 的也未进行编写

## 参考资料

- [TinyWebServer](https://github.com/JehanRio/TinyWebServer)
- [sylar](https://github.com/sylar-yin/sylar)
