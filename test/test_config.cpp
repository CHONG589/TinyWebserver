/**
 * @file test_config.cpp
 * @brief 配置模块测试
 * @version 0.1
 * @date 2026-03-22
 */

#include "zchlog.h"
#include "base/config.h"

ConfigVar<int>::ptr g_int = Config::Lookup("global.int", (int)8080, "global int");

ConfigVar<float>::ptr g_float = Config::Lookup("global.float", (float)10.2f, "global float");

// 字符串需显示构造，不能传字符串常量
ConfigVar<std::string>::ptr g_string = Config::Lookup("global.string", std::string("helloworld"), "global string");

ConfigVar<std::vector<int>>::ptr g_int_vec = Config::Lookup("global.int_vec", std::vector<int>{1, 2, 3}, "global int vec");

ConfigVar<std::list<int>>::ptr g_int_list = Config::Lookup("global.int_list", std::list<int>{1, 2, 3}, "global int list");

ConfigVar<std::set<int>>::ptr g_int_set = Config::Lookup("global.int_set", std::set<int>{1, 2, 3}, "global int set");

ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set = 
        Config::Lookup("global.int_unordered_set", std::unordered_set<int>{1, 2, 3}, "global int unordered_set");

ConfigVar<std::map<std::string, int>>::ptr g_map_string2int = 
        Config::Lookup("global.map_string2int", std::map<std::string, int>{{"key1", 1}, {"key2", 2}}, "global map string2int");

ConfigVar<std::unordered_map<std::string, int>>::ptr g_unordered_map_string2int = 
        Config::Lookup("global.unordered_map_string2int", std::unordered_map<std::string, int>{{"key1", 1}, {"key2", 2}}, "global unordered_map string2int");

template<class T>
std::string formatArray(const T &v) {
    std::stringstream ss;
    ss << "[";
    for(const auto &i:v) {
        ss << " " << i;
    }
    ss << " ]";
    return ss.str();
}

template<class T>
std::string formatMap(const T &m) {
    std::stringstream ss;
    ss << "{";
    for(const auto &i:m) {
        ss << " {" << i.first << ":" << i.second << "}";
    }
    ss << " }";
    return ss.str();
}

void test_config() {
    LOG_INFO() << "g_int value: " << g_int->GetValue();
    LOG_INFO() << "g_float value: " << g_float->GetValue();
    LOG_INFO() << "g_string value: " << g_string->GetValue();
    LOG_INFO() << "g_int_vec value: " << formatArray<std::vector<int>>(g_int_vec->GetValue());
    LOG_INFO() << "g_int_list value: " << formatArray<std::list<int>>(g_int_list->GetValue());
    LOG_INFO() << "g_int_set value: " << formatArray<std::set<int>>(g_int_set->GetValue());
    LOG_INFO() << "g_int_unordered_set value: " << formatArray<std::unordered_set<int>>(g_int_unordered_set->GetValue());
    LOG_INFO() << "g_int_map value: " << formatMap<std::map<std::string, int>>(g_map_string2int->GetValue());
    LOG_INFO() << "g_int_unordered_map value: " << formatMap<std::unordered_map<std::string, int>>(g_unordered_map_string2int->GetValue());

    // 自定义配置项
    //test_class();
}

int main() {
    // 初始化日志系统 (加载配置文件)
    zch::InitLogFromJson("/home/zch/Project/TinyWebserver/config/log_config.json");

    // 设置g_int的配置变更回调函数
    g_int->AddListener([](const int &old_value, const int &new_value) {
        LOG_INFO() << "g_int value changed, old_value: " << old_value << ", new_value: " << new_value;
    });

    LOG_INFO() << "before============================";

    test_config();

    Config::LoadFromConfDir("/home/zch/Project/TinyWebserver/config");

    LOG_INFO() << "after============================";

    test_config();
}
