# 未解决的问题

- 在 `InitSocket_()` 中 listen 后，对 listenFd_ 增加了事件，即：`iom_->addEvent()` 后，不会在 idle 中触发，epoll_wait() 返回后的 rt 每次都返回的是 0，rt 表示此次返回的事件数目。这就导致不会触发 `handle_accept()`。

- 要注意 `iom_.reset(new IOManager(4, false));` 现在的位置，不要去动它，它原来在构造函数最前面的位置，但是在前面时，通过看日志发现很多操作没有做？所以我先让前面的初始化操作先完成，再来用协程处理。

# 优化

用 CMake 代替 Makefile
