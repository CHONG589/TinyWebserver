#include <unistd.h>

#include "coroutine/iomanager.h"
#include "base/address.h"
#include "base/http_server.h"
#include "zchlog.h"

void run() {
    // 初始化日志
    zch::InitLogFromJson("/home/zch/Project/TinyWebserver/config/log_config.json");
    LOG_INFO() << "Server starting...";

    // 绑定所有网卡的 8000 端口
    Address::ptr addr = IPv4Address::Create("0.0.0.0", 8000);
    if(!addr) {
        LOG_ERROR() << "Create address failed";
        return;
    }

    // 创建 HTTP 服务器
    HttpServer::ptr server = std::make_shared<HttpServer>(true);
    
    // 绑定地址
    while(!server->bind(addr)) {
        LOG_ERROR() << "Bind failed, retrying in 2s...";
        sleep(2);
    }
    
    // 启动服务器
    LOG_INFO() << "Bind success " << *addr;
    server->start();
}

int main() {
    // 4个 IO 线程，启用 caller 线程参与调度
    IOManager::ptr manager = std::make_shared<IOManager>(4, true);
    manager->schedule(run);
    
    // IOManager 析构时会调用 stop()，等待所有任务完成
    return 0;
} 
