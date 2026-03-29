#include <unistd.h>
#include <fstream>

#include "coroutine/iomanager.h"
#include "base/address.h"
#include "base/http_server.h"
#include "http/httprequest.h"
#include "base/config.h"

static zch::Logger::ptr g_logger = LOG_NAME("system");

static zch::ConfigVar<size_t>::ptr g_thread_num =
    zch::Config::Lookup("server.thread_num", (size_t)4, "thread number");

static zch::ConfigVar<std::string>::ptr g_ip =
    zch::Config::Lookup("server.ip", std::string("0.0.0.0"), "ip address");

static zch::ConfigVar<int>::ptr g_port =
    zch::Config::Lookup("server.port", (int)8000, "port");

void run() {
    
    LOG_INFO(g_logger) << "Server starting...";

    // 绑定所有网卡的指定端口
    std::string strIp = g_ip->GetValue();
    LOG_INFO(g_logger) << "绑定 IP 地址：" << strIp << ", 端口：" << g_port->GetValue();
    Address::ptr addr = IPv4Address::Create(strIp.c_str(), g_port->GetValue());
    if(!addr) {
        LOG_ERROR(g_logger) << "Create address failed";
        return;
    }

    // 创建 HTTP 服务器，传入资源路径
    HttpServer::ptr server = std::make_shared<HttpServer>(true);
    
    // 绑定地址
    while(!server->bind(addr)) {
        LOG_ERROR(g_logger) << "Bind failed, retrying in 2s...";
        sleep(2);
    }
    
    // 启动服务器
    LOG_INFO(g_logger) << "Bind success " << *addr;
    server->start();
}

int main() {

    // 加载配置文件
    zch::Config::LoadFromConfDir("/home/zch/Project/TinyWebserver/config", false);

    // 启动 IOManager
    size_t thread_num = g_thread_num->GetValue();
    LOG_INFO(g_logger) << "线程数量为：" << thread_num;
    IOManager::ptr manager = std::make_shared<IOManager>(thread_num, true);
    manager->schedule(run);
    
    // IOManager 析构时会调用 stop()，等待所有任务完成
    return 0;
} 
