# 运行

```
cd TinyWebserver
make
./server
```

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

# 测试结果

在 CPU i5，内存 4G 的机器上运行。

在第一版本没有协程，双 ET 模式下，用 webbench 实现了 8K 的并发量，运行 10s，QPS 为 1731。

![V1_0](./img/V1_0.png)

在第二版本有协程，但是没有采用 ET 模式 (还未实现)，用 webbench 实现了 8K 的并发量，运行 10s，QPS 为 1524。

![V1_1](./img/V1_1.png)

怎么性能还下降了？是不是因为没有用到 ET 模式？(待优化的地方)
