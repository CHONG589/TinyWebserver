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

/**
 * @brief 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
 */
template <class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::list<T>)
 */
template <class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::list<T> 转换成 YAML String)
 */
template <class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
 */
template <class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 */
template <class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
 */
template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
 */
template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T> &v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
 */
template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin();
             it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
 */
template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T> &v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string, T>)
 */
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin();
             it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                      LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML String)
 */
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T> &v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto &i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 配置参数模板子类,保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从std::string转换成T类型的仿函数
 *          ToStr 从T转换成std::string的仿函数
 *          std::string 为YAML格式的字符串
 */
template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

    /**
     * @brief 通过参数名,参数值,描述构造ConfigVar
     * @param[in] name 参数名称有效字符为[0-9a-z_.]
     * @param[in] default_value 参数的默认值
     * @param[in] description 参数的描述
     */
    ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
        : ConfigVarBase(name, description)
        , m_val(default_value) {
    }

    /**
     * @brief 将参数值转换成YAML String
     * @exception 当转换失败抛出异常
     */
    std::string toString() override {
        try {
            //return boost::lexical_cast<std::string>(m_val);
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
                                              << e.what() << " convert: " << TypeToName<T>() << " to string"
                                              << " name=" << m_name;
        }
        return "";
    }
};

}//namespace zch

#endif
