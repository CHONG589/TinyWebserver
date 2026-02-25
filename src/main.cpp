#include <unistd.h>
#include <fstream>
#include <json/json.h>

#include "coroutine/iomanager.h"
#include "base/address.h"
#include "base/http_server.h"
#include "http/httprequest.h"
#include "zchlog.h"

static std::string ip = "0.0.0.0";
static int port = 8000;
static std::string resources_dir;
static uint64_t timeout = 120000;
static int thread_num = 4;

// 加载服务器配置
bool LoadServerConfig(std::string& ip, int& port, std::string& resources_dir, uint64_t& timeout, int& thread_num) {
    std::ifstream ifs("/home/Project/TinyWebserver/config/server.json");
    if (!ifs.is_open()) {
        LOG_ERROR() << "Open server config failed: /home/Project/TinyWebserver/config/server.json";
        return false;
    }
    
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root)) {
        LOG_ERROR() << "Parse server config failed";
        return false;
    }
    
    if (!root.isMember("server")) {
        LOG_ERROR() << "Config root 'server' not found";
        return false;
    }
    
    const Json::Value& server = root["server"];
    ip = server.get("ip", "0.0.0.0").asString();
    port = server.get("port", 8000).asInt();
    resources_dir = server.get("resources_dir", "./resources").asString();
    timeout = server.get("timeout", 120000).asUInt64();
    thread_num = server.get("thread_num", 4).asInt();

    return true;
}

void run() {
    
    LOG_INFO() << "Server starting...";

    // 绑定所有网卡的指定端口
    Address::ptr addr = IPv4Address::Create(ip.c_str(), port);
    if(!addr) {
        LOG_ERROR() << "Create address failed";
        return;
    }

    // 创建 HTTP 服务器，传入资源路径
    HttpServer::ptr server = std::make_shared<HttpServer>(true, resources_dir);
    server->setRecvTimeout(timeout);
    
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

    // 加载日志配置文件
    zch::InitLogFromJson("/home/Project/TinyWebserver/config/log_config.json");

    // 加载服务器配置
    if (!LoadServerConfig(ip, port, resources_dir, timeout, thread_num)) {
        LOG_WARN() << "Load server config failed, using default";
        resources_dir = "/home/Project/TinyWebserver/resources";
    }
    
    LOG_INFO() << "Config loaded - IP: " << ip << ", Port: " << port 
                << ", Resources: " << resources_dir << ", Timeout: " 
                << timeout << "ms, Threads: " << thread_num;

    // 启动 IOManager
    IOManager::ptr manager = std::make_shared<IOManager>(thread_num, true);
    manager->schedule(run);
    
    // IOManager 析构时会调用 stop()，等待所有任务完成
    return 0;
} 
