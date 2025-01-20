/**
 * @file config.h
 * @brief 配置模块
 * @author zch
 * @date 2025-01-20
 */

#ifndef __ZCH_CONFIG_H__
#define __ZCH_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "mutex.h"
#include "./log/log.h"
#include "util.h"

namespace zch {

/**
 * @brief 配置变量的基类
 */
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    /**
     * @brief 构造函数
     * @param[in] name 配置参数名称[0-9a-z_.]
     * @param[in] description 配置参数描述
     */
    ConfigVarBase(const std::string &name, const std::string &description = "")
                    : m_name(name)
                    , m_description(description) {
        
        //将m_name转换为小写的形式存储
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_description; }

    /**
     * @brief 转成字符串
     */
    virtual std::string toString() const = 0;

    /**
     * @brief 从字符串初始化值
     */
    virtual bool fromString(const std::string &val) = 0;

    /**
     * @brief 返回配置参数值的类型名称
     */
    virtual std::string getTypeName() const = 0;

protected:
    //配置参数的名称
    std::string m_name;
    //配置参数的描述
    std::string m_description;
};

/**
 * @brief 类型转换模板类(F 源类型, T 目标类型)
 */
template<class F, class T>
class LexicalCast {
public:
    /**
     * @brief 类型转换
     * @param[in] v 源类型值
     * @return 返回v转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator()(const F &v) {
        return boost::lexical_cast<T>(V);
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::vector<T>)
 */
template<class T>
class LexicalCast<std::string, std::vector<T>> {
public:F
    std::vector<T> operator()(const std::string &v) {
        //使用 YAML::Load 函数将输入的字符串 v 解析为一个 YAML 节点。
        YAML::Node node = YAML::Load(v);
        //这里定义了一个目标向量 vec，其类型为 std::vector<T>。
        //typename 关键字用于告诉编译器 std::vector<T> 是一个类型。
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

}//namespace zch

#endif
