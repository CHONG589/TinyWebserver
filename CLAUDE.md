# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在此仓库中工作时提供指导。

## 构建与运行

```bash
mkdir -p build && cd build
cmake ..
make -j4
../bin/server          # 服务器二进制文件输出到 bin/
```

CMakeLists.txt 构建两个目标：
- `TinyWebServerLib` — 共享库，源文件来自 `src/base/*.cpp`、`src/coroutine/*.cpp`、`src/http/*.cpp`、`src/db/*.cpp`
- `server` — 可执行文件，链接 TinyWebServerLib

依赖项：`yaml-cpp`、`mysqlclient`、`pthread`。使用 C++14 标准。

编译选项：`-std=c++11 -O0 -ggdb -Wall -Werror`

## 测试

测试文件位于 `test/`。每个 `.cpp` 文件生成一个独立的测试可执行文件。启用方式：

```bash
# 将 CMakeLists.txt 中的 option(BUILD_TESTS "Build tests" ON) 设为 ON
cd build && cmake .. && make -j4
# 测试可执行文件输出到 bin/
```

## 架构

这是一个基于 C++11 开发的高性能 Web 服务器，结合了**协程（Coroutine）** 与 **Epoll Reactor** 模型，参考了 [sylar](https://github.com/sylar-yin/sylar) 框架。

### 核心层次（自底向上）

1. **基础工具层** (`include/base/`, `src/base/`)
   - `log.h` — 完整日志系统。日志级别：FATAL → DEBUG。支持格式模板、控制台输出、文件输出（按天回滚）。LoggerManager 为单例。使用宏：`LOG_INFO(logger)`、`LOG_ERROR(logger)` 等。流式写法：`LOG_INFO(g_logger) << "message";`
   - `config.h` — 基于 YAML 的类型安全配置系统。通过 `zch::Config::Lookup<T>(名称, 默认值, 描述)` 获取/创建配置项。从 `config/` 目录加载 YAML 配置文件。支持变更回调监听。
   - `thread.h` — 线程封装，以及 `Mutex`、`Spinlock`、`RWMutex`、`Semaphore` 同步原语。
   - `timer.h` — Timer + TimerManager，使用 `std::set` 小根堆管理定时器。IOManager 继承 TimerManager。
   - `socket.h` / `address.h` — Socket 和 IPv4/IPv6/Unix 地址抽象。
   - 其他：`buffer.h`、`singleton.h`、`noncopyable.h`、`fd_manager.h`、`util.h`

2. **协程系统** (`include/coroutine/`, `src/coroutine/`)
   - `Fiber` — 基于 `ucontext.h` 的有栈协程。状态：READY → RUNNING → TERM。`resume()`/`yield()` 进行上下文切换。每个线程有一个主协程。
   - `Scheduler` — N:M 协程调度器，内部包含线程池。`schedule(fiber或回调, 线程id)` 添加任务，`start()`/`stop()` 管理生命周期。空闲时执行 idle 协程。
   - `IOManager` — 继承 `Scheduler` 和 `TimerManager`。封装 epoll，将异步 IO 转化为同步的协程代码。注册 fd 事件（READ/WRITE），epoll_wait 返回后通过调度器分发回调。使用 pipe 唤醒 idle 协程。

3. **网络层** (`include/base/`)
   - `TcpServer` — 接收循环将新连接分发给 IOManager 处理。`handleClient()` 由子类重写。
   - `HttpServer` — 继承 TcpServer。为每个客户端创建 `HttpConn`，调用 `process()` 完成读取→解析→响应。

4. **HTTP 层** (`include/http/`, `src/http/`)
   - `HttpConn` — 单个连接处理器。使用 `readv`/`writev` 进行分散读写。包含读缓冲 `Buffer`、写缓冲 `Buffer`、`HttpRequest`、`HttpResponse`。
   - `HttpRequest` — HTTP 请求解析器。状态机：REQUEST_LINE → HEADERS → BODY → FINISH。支持 GET 和 POST（urlencoded）。包含基于 MySQL 的用户验证。
   - `HttpResponse` — HTTP 响应构建器。生成状态行、响应头、响应体。使用 `mmap` 映射静态文件以提升传输效率。

5. **数据库层** (`include/db/`, `src/db/`)
   - `Connection` — MySQL 连接的 RAII 封装。
   - `ConnectionPool` — 单例连接池。支持最小/最大连接数。生产者线程在连接不足时创建连接，回收线程扫描并回收空闲过长的连接。`GetConnection()` 返回 `shared_ptr<Connection>`，通过自定义删除器在析构时自动归还连接。

### 启动流程 (main.cpp)

1. `zch::Config::LoadFromConfDir()` 从 `config/` 目录加载所有 YAML 配置文件
2. 创建 `IOManager`，线程数由 `server.thread_num` 配置（默认 4）
3. 调度 `run()` 函数：创建 `HttpServer` → 绑定地址 → 启动服务开始接收连接
4. IOManager 析构时调用 `stop()`，等待所有任务完成

### 配置文件 (`config/`)

- `server.yml` — IP、端口、线程数、资源文件夹路径、超时时间
- `log_config.yml` — 各日志器的日志级别、输出器（控制台/文件）、格式模板
- `db_config.yml` — MySQL 连接参数（IP、端口、用户名、密码、数据库名）、连接池大小、空闲超时

## 代码风格

Clang-format：基于 Google 风格，4 空格缩进，120 列宽限制，include 排序。

### 函数

- 每个函数的注释都必须按照如下格式添加注释；

```Cpp
/**
 * @brief 获取/创建对应参数名的配置参数
 * @param[in] name 配置参数名称
 * @param[in] default_value 参数默认值
 * @param[in] description 参数描述
 * @details 获取参数名为name的配置参数,如果存在直接返回
 *          如果不存在,创建参数配置并用default_value赋值
 * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
 * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
 */
```

- 函数(包含类中的函数)命名：每个单词首字母大写(大驼峰命名法)；

### 类

- 类名：大驼峰命名；

- 类变量：`m_varName` 这样用小写 `m_` 加小驼峰命名的方式；

### 普通变量命名

采用小驼峰命名；
