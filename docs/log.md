# TinyWebserver 日志系统

## 概述

日志系统位于 `include/base/log.h` 和 `src/base/log.cpp` 中，提供了完整的日志功能，包括：

- **8 个日志级别**：FATAL → DEBUG，按严重程度从低到高（数值从 0 到 700）
- **可插拔的日志模板**：支持丰富的格式占位符，可自定义输出格式
- **多种输出方式（Appender）**：控制台输出、文件输出（按天回滚）
- **线程安全**：使用 Spinlock 保证多线程环境下的日志安全
- **与 YAML 配置系统深度集成**：支持通过配置文件动态管理日志器和输出目标

## 架构总览

```
LoggerManager (单例，管理所有 Logger)
    │
    ├── Logger ("root")          ← 根日志器
    │     ├── Appender 1         ← 输出目标（控制台、文件等）
    │     ├── Appender 2
    │     └── ...
    ├── Logger ("system")        ← 自定义日志器
    │     └── Appender ...
    └── Logger (...)             ← 其他日志器
```

每条日志的写入流程：

```
LOG_XXX(logger) << "message"
       │
       ▼
  LogEventWrap 构造 → LogEvent 创建，获取 stringstream 引用，LogEvent 包含时间，日志等级，实际内容(stringstream)等
       │
       ▼
  << "message" → 内容写入 stringstream
       │
       ▼
  LogEventWrap 析构 → logger->Log(event)   根据这个 logger 已有的落地方向，写入到里面
       │
       ▼
  Logger 遍历所有 Appender → appender->Log(event)  appener中根据(std::cout 或者文件流) 进行格式化写入
       │
       ▼
  Appender 使用 LogFormatter 格式化 → 输出到目标
```

## 核心类详解

### 1. LogLevel - 日志级别

定义在 `log.h:75`，数值越小严重程度越高：

| 级别    | 数值  | 含义                     |
| ------- | ----  | ------------------------ |
| FATAL   | 0     | 致命错误，系统不可用     |
| ALERT   | 100   | 高优先级告警             |
| CRIT    | 200   | 严重错误                 |
| ERROR   | 300   | 错误                     |
| WARN    | 400   | 警告                     |
| NOTICE  | 500   | 正常但值得注意           |
| INFO    | 600   | 一般信息                 |
| DEBUG   | 700   | 调试信息                 |
| NOTSET  | 800   | 未设置（表示无效/禁用）   |

提供 `ToString()` 和 `FromString()` 两个静态方法进行字符串与枚举值的双向转换。`FromString` 不区分大小写。

### 2. LogEvent - 日志事件

定义在 `log.h:117`，承载单条日志的全部上下文信息：

| 字段          | 类型              | 说明                              |
| ------------- | ----------------- | --------------------------------- |
| `m_loggerName`| `std::string`     | 日志器名称                        |
| `m_level`     | `LogLevel::Level` | 日志级别                          |
| `m_file`      | `const char*`     | 源文件名（通过 `__FILENAME__` 宏获取，只保留文件名，去除路径） |
| `m_line`      | `int32_t`         | 源文件行号                        |
| `m_elapse`    | `int64_t`         | 从日志器创建到当前的累计运行毫秒数 |
| `m_time`      | `time_t`          | UTC 时间戳                        |
| `m_ss`        | `std::stringstream`| 日志内容，支持 `<<` 流式写入      |

### 3. LogFormatter - 日志格式化器

定义在 `log.h:193`，通过**状态机**解析格式模板字符串（pattern），将其拆分为一个 `FormatItem` 列表。

#### 格式占位符

| 占位符   | 含义         | 说明                                            |
| -------- | -----------  | ----------------------------------------------- |
| `%m`     | 消息体       | 通过 `<<` 流式写入的内容                        |
| `%p`     | 日志级别     | 如 FATAL, INFO, DEBUG                           |
| `%c`     | 日志器名称   | 如 "root", "system"                             |
| `%d`     | 日期时间     | 支持 `%d{...}` 指定 strftime 格式               |
| `%r`     | 累计运行毫秒 | 从日志器创建至今的毫秒数                        |
| `%f`     | 文件名       | 日志语句所在的源文件名                          |
| `%l`     | 行号         | 日志语句所在的行号                              |
| `%t`     | 线程 id      | 当前线程的 tid                                  |
| `%F`     | 协程 id      | 当前协程 id（目前未实现，始终为 0）             |
| `%N`     | 线程名称     | 当前线程名称                                    |
| `%%`     | 百分号       | 输出 `%`                                        |
| `%T`     | 制表符       | 输出 `\t`                                       |
| `%n`     | 换行符       | 输出换行                                        |

默认 pattern：
```
[%d{%Y-%m-%d %H:%M:%S}][%rms][%p][%c][%f:%l] %m%n
```

#### 状态机解析流程

解析过程在 `LogFormatter::Init()`（`log.cpp:184`）中实现。状态机有两种状态：

1. **解析常规字符串**（`parsing_string = true`）：遇到 `%` 之前的所有字符视为常规字符串，生成 `StringFormatItem`
2. **解析模板字符**（`parsing_string = false`）：遇到 `%` 后紧跟的字符被视为模板标识符，生成对应的 `FormatItem` 子类实例

状态转换图：

```
常规字符 ──%──▶ 模板字符
模板字符 ──任意──▶ 常规字符
```

特殊处理：
- `%` 后紧跟 `%`：模板字符状态遇到 `%`，表示这里是一个 `%` 转义，生成 `PercentSignFormatItem`
- `%d` 后紧接 `{...}`：将大括号内的内容作为 strftime 格式字符串提取，生成 `DateTimeFormatItem`

以下是关键代码的流程示意：

```cpp
while (i < pattern.size()) {
    if (c == "%") {
        if (parsing_string) {
            // 结束常规字符串，进入模板字符状态
            if (!tmp.empty()) patterns.push_back({0, tmp});
            tmp.clear();
            parsing_string = false;
        } else {
            // 模板字符状态遇到 % → % 转义
            patterns.push_back({1, "%"});
            parsing_string = true;
        }
    } else {
        if (parsing_string) {
            tmp += c;        // 常规字符累积
        } else {
            patterns.push_back({1, c});  // 模板字符
            parsing_string = true;
            if (c == "d" && 后面有 {...}) {
                提取 dateformat;
            }
        }
    }
}
```

解析完成后，通过静态映射表 `s_format_items` 将模板字符查找为对应的 `FormatItem` 子类：

| 字符 | 类                      |
| ---- | ----------------------- |
| `m`  | `MessageFormatItem`     |
| `p`  | `LevelFormatItem`       |
| `c`  | `LoggerNameFormatItem`  |
| `d`  | `DateTimeFormatItem`    |
| `r`  | `ElapseFormatItem`      |
| `f`  | `FileNameFormatItem`    |
| `l`  | `LineFormatItem`        |
| `%`  | `PercentSignFormatItem` |
| `T`  | `TabFormatItem`         |
| `n`  | `NewLineFormatItem`     |

`Format()` 方法遍历 `m_items` 列表，依次调用每个 `FormatItem::Format(os, event)`，将格式化结果写入输出流。

### 4. LogAppender - 日志输出地

定义在 `log.h:283`，抽象基类，包含：
- `m_formatter` / `m_defaultFormatter`：日志格式器，每个 Appender 可以有自己的格式器
- `m_mutex`：Spinlock 互斥锁，保证线程安全
- `Log(LogEvent::ptr event)`：纯虚函数，由子类实现具体输出逻辑
- `ToYamlString()`：纯虚函数，将 Appender 配置转为 YAML 字符串

#### StdoutLogAppender - 控制台输出

`log.cpp:337`，直接调用 `m_formatter->Format(std::cout, event)` 输出到标准输出。

#### FileLogAppender - 文件输出

`log.cpp:361`，核心功能：

- **按天回滚**：`Log()` 方法中检测日期变化（`curDay != m_curData`），跨天时自动关闭旧文件并创建新文件
- **定期 reopen**：距离上次写日志超过 3 秒则重新打开文件，防止文件被外部删除
- **文件名策略**：`GetFileName()` 组合基础路径 + `年-月-日.log`，如 `/logs/root/2026-05-10.log`
- **自动创建目录**：`FSUtil::MakeSurePathExist()` 确保日志目录存在

### 5. Logger - 日志器

定义在 `log.h:410`：

- 维护一个 `LogAppender` 列表（`std::list`）
- 持有日志级别，写日志时校验 `event->GetLevel() <= m_level` 才输出（注意：级别数值越小越严重，所以用 `<=`）
- `Log()` 方法遍历所有 Appender，将日志事件分发到每个输出目标

### 6. LogEventWrap - RAII 日志包装器

定义在 `log.h:487`，是宏与 Logger 之间的桥梁：

- **构造时**：持有 Logger 和 LogEvent
- **析构时**：调用 `m_logger->Log(m_event)`，触发实际的日志写入

这种设计使得流式日志写法成为可能：

```cpp
LOG_INFO(g_logger) << "hello " << 123;
// 展开为：
// if (INFO <= logger->GetLevel())
//     LogEventWrap(logger, LogEvent::ptr(new LogEvent(...))).GetLogEvent()->GetSS()
//     << "hello " << 123;
// ;  ← 分号触发 LogEventWrap 析构，日志被写入
```

### 7. 设计分析：为什么不需要自定义 LogStream

在一些日志系统中，会在宏和 `std::stringstream` 之间插入一个自定义的 `LogStream` 类，重载 `operator<<` 来收集日志内容。但本项目的 `LOG_LEVEL` 宏直接返回了 `std::stringstream&`，没有中间层。原因如下：

**`std::stringstream` 本身就是 `std::ostream`**

`std::stringstream` 继承自 `std::ostream`，而 `std::ostream` 已经对所有常见类型（`int`、`double`、`const char*`、`std::string`、`void*` 等）重载了 `operator<<`。宏展开后，用户的 `<< "hello"` 直接走 `ostream::operator<<`，无需任何自定义工作：

```cpp
// 宏展开后的等价代码
LogEventWrap(...).GetLogEvent()->GetSS() << "hello " << 123;
//                                       ^^
//             这是 std::stringstream (即 std::ostream) 的 operator<<
//             数据直接写入 LogEvent 内部的 m_ss 缓冲区
```

**与自定义 LogStream 方案的对比**

| 自定义 LogStream 的目的     | 本项目的处理方式                               |
| -------------------------- | ---------------------------------------------- |
| 收集 `<<` 写入的日志内容    | `std::ostream::operator<<` 天然支持，无需重写  |
| 高效的缓冲区管理            | `std::stringstream` 内部自带缓冲区              |
| 特定类型的格式化输出        | 统一交给 `LogFormatter` 的 `FormatItem` 体系在后端处理，不在收集阶段格式化 |
| 日志级别过滤                | 由宏开头的 `if(level <= logger->GetLevel())` 短路处理 |

**本质是"收集"与"格式化"的职责分离**

- **收集阶段**：`std::stringstream` 只做一件事 — 用 `<<` 原样收集用户写入的所有内容，不做任何加工
- **格式化阶段**：`LogFormatter` 拿到 `LogEvent` 后，将 `m_ss.str()`（日志内容）结合时间、级别、文件名等元信息，按 pattern 模板统一格式化输出

这种设计更简洁：不需要在中间插入一个自定义流层，`std::stringstream` 已经完美完成了"内容收集"这个任务。

### 8. LoggerManager - 日志器管理器（单例）

定义在 `log.h:517`，通过 `typedef Singleton<LoggerManager> LoggerMgr` 暴露为单例：

- **构造时**自动创建 `root` 日志器，并添加一个默认的 `StdoutLogAppender`
- `GetLogger(name)`：按名称查找，不存在则新建（但新建的 Logger 不带 Appender，需手动添加）
- `GetRoot()`：返回 root 日志器
- `Init()`：初始化方法，预留用于从配置文件加载（当前为空实现，实际加载逻辑在 log.cpp 末尾的静态注册中）

## 宏定义

```cpp
// 获取 logger
#define LOG_ROOT()    LoggerMgr::GetInstance()->GetRoot()
#define LOG_NAME(name) LoggerMgr::GetInstance()->GetLogger(name)

// 流式写日志（核心宏）
#define LOG_LEVEL(logger, level)
    if(level <= logger->GetLevel())
        LogEventWrap(logger, LogEvent::ptr(new LogEvent(
            logger->GetName(), level, __FILENAME__, __LINE__,
            GetElapsedMS() - logger->GetCreateTime(), time(0)))
        ).GetLogEvent()->GetSS()

// 便捷宏
#define LOG_FATAL(logger)  LOG_LEVEL(logger, LogLevel::FATAL)
#define LOG_DEBUG(logger)  LOG_LEVEL(logger, LogLevel::DEBUG)
// ... ALERT, CRIT, ERROR, WARN, NOTICE, INFO
```

关键细节：
- `__FILENAME__`：通过 `strrchr` 从 `__FILE__` 中截取文件名，去除路径前缀
- **短路求值**：`if(level <= logger->GetLevel())` 确保日志级别不够时不会构造 `LogEventWrap` 和 `LogEvent`，提升性能
- `LogEventWrap` 是临时对象，语句末尾的分号触发析构，在析构函数中完成日志输出

## 与配置系统的集成

### 配置系统相关机制

配置系统（`config.h`）的核心概念：

- **`ConfigVar<T>`**：一个类型安全的配置变量模板类，内部持有值 `T` 和变更回调列表
- **`Config::Lookup<T>(name, default, desc)`**：按名称查找配置项，不存在则创建并注册
- **`Config::LoadFromConfDir(path)`**：扫描 `config/` 目录下所有 `.yml` 文件，解析 YAML 并更新对应配置项
- **`LexicalCast<F, T>`**：类型转换模板，负责 YAML 字符串与 C++ 类型之间的双向转换
- **`AddListener(cb)`**：注册配置变更回调，当 `SetValue()` 检测到值变化时触发

### 日志配置的数据结构

在 `log.cpp:581-610` 中定义了两个配置结构体：

```cpp
struct LogAppenderDefine {
    int type;          // 1 = File, 2 = Stdout
    std::string pattern;
    std::string file;
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::NOTSET;
    std::vector<LogAppenderDefine> appenders;
};
```

### 日志专用的 LexicalCast 偏特化

在 `log.cpp:612-684` 中为 `LogDefine` 提供了 YAML 字符串与 `std::set<LogDefine>` 之间的双向转换偏特化：

- **`LexicalCast<std::string, LogDefine>`**：YAML 字符串 → LogDefine
  - 解析 `name`（必填）、`level`（选填）、`appenders` 数组（选填）
  - appender 的 `type` 字段：`"FileLogAppender"` → type=1，`"StdoutLogAppender"` → type=2
  - 对 FileLogAppender 额外解析 `file` 字段

- **`LexicalCast<LogDefine, std::string>`**：LogDefine → YAML 字符串
  - 将内存结构反向序列化为 YAML，用于调试或持久化

### 配置注册与回调

```cpp
// log.cpp:686 - 注册配置项 "logs"，类型为 std::set<LogDefine>
ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    Config::Lookup("logs", std::set<LogDefine>(), "logs config");

// log.cpp:689-736 - 通过静态全局变量在 main() 之前注册变更回调
struct LogIniter {
    LogIniter() {
        g_log_defines->AddListener([](old_value, new_value) {
            // 1. 遍历新配置，检测新增/修改的 logger
            // 2. 创建/获取 Logger，设置日志级别
            // 3. 清空旧的 Appender，创建新的
            // 4. 遍历旧配置，将已删除的 logger 设为 NOTSET 并清空 Appender
        });
    }
};
static LogIniter __log_init; // 在 main() 之前执行构造函数
```

`LogIniter` 利用 C++ 静态变量初始化机制，在 `main()` 函数之前完成回调注册。无需手动调用初始化。

### 配置加载的完整流程

```
程序启动
    │
    ▼
1. 静态变量初始化阶段
    ├── g_log_defines 注册（Config::Lookup("logs", ...)）
    └── __log_init 构造 → 注册配置变更回调
    │
    ▼
2. main() 执行
    │
    ├── Config::LoadFromConfDir("config/")
    │     ├── 扫描 config/*.yml
    │     ├── 读取 server.yml, log_config.yml, db_config.yml
    │     └── 每个文件调用 LoadFromYaml()
    │           ├── ListAllMember() 递归展平 YAML 树为 key-value 对
    │           ├── 对每个 key，调用 Config::LookupBase(key) 查找已有配置项
    │           └── 找到则调用 ConfigVarBase::FromString(yaml_str)
    │                 └── 触发 LexicalCast 将 YAML → std::set<LogDefine>
    │                       └── 调用 SetValue() → 值变化 → 触发回调
    │                             └── LogIniter 的回调执行：
    │                                   ├── 创建/更新 Logger
    │                                   ├── 设置日志级别
    │                                   ├── 创建 Appender（Stdout/File）
    │                                   └── 添加 Appender 到 Logger
    │
    ▼
3. 日志系统就绪，可直接使用
```

### 配置文件格式示例

```yaml
# config/log_config.yml
logs:
    - name: root
      level: debug
      appenders:
          - type: StdoutLogAppender
            pattern: "[%d{%Y-%m-%d %H:%M:%S}][%rms][%p][%c][%f:%l] %m%n"
          - type: FileLogAppender
            file: /path/to/logs/root/
            pattern: "[%d{%Y-%m-%d %H:%M:%S}][%rms][%p][%c][%f:%l] %m%n"
    - name: system
      level: debug
      appenders:
          - type: StdoutLogAppender
          - type: FileLogAppender
            file: /path/to/logs/system/
```

- `name`：日志器名称，"root" 为根日志器
- `level`：可选值为 `fatal / alert / crit / error / warn / notice / info / debug`（不区分大小写）
- `appenders`：输出目标列表
  - `type`：`StdoutLogAppender` 或 `FileLogAppender`
  - `file`：仅 FileLogAppender 需要，指定日志目录路径
  - `pattern`：可选，不指定则使用默认格式模板
- 支持配置多个日志器（`logs` 是一个数组），每个日志器可以有多个 Appender

## 使用示例

```cpp
// 获取 root 日志器
auto logger = LOG_ROOT();

// 流式写日志
LOG_INFO(logger) << "Server started on port " << port;
LOG_ERROR(logger) << "Connection failed: " << error_msg;
LOG_DEBUG(logger) << "Request body: " << body;

// 获取指定名称的日志器
auto sys_logger = LOG_NAME("system");
LOG_WARN(sys_logger) << "Memory usage high: " << usage;
```

## 线程安全

日志系统在以下层面保证线程安全：

| 组件           | 锁类型    | 锁定范围               |
| -------------- | --------- | ---------------------- |
| LogAppender    | Spinlock  | SetFormatter / GetFormatter / Log |
| Logger         | Spinlock  | AddAppender / DelAppender / ClearAppenders |
| LoggerManager  | Spinlock  | GetLogger（查找和创建新 Logger） |

所有 Appender 的 `Log()` 方法在格式化并输出日志时持有自旋锁，确保同一 Appender 的日志不会交叉输出。
