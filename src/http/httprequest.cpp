#include "http/httprequest.h"

// 网页名称，和一般的前端跳转不同，这里需要将请求信息放到后
// 端来验证一遍再上传i（和小组成员还起过争执）
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

// 登录/注册
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/login.html", 1}, {"/register.html", 0}
};

/**
 * @brief 初始化 HttpRequest 对象
 */
void HttpRequest::Init() {
    state_ = REQUEST_LINE;  // 初始状态
    method_ = path_ = version_= body_ = "";
    header_.clear();
    post_.clear();
}

/**
 * @brief 解析 HTTP 请求
 * @param[in] buff 读缓冲区
 * @return bool 解析是否成功
 */
bool HttpRequest::parse(Buffer& buff) {
    const char END[] = "\r\n";
    if(buff.ReadableBytes() == 0) {
        LOG_WARN() << "没有可读的字节";
        return false;
    }
        
    // 读取数据开始
    while(buff.ReadableBytes() && state_ != FINISH) {
        // 从buff中的读指针开始到读指针结束，这块区域是未读取得数据并去处"\r\n"。
        // 第一，二个参数是查找范围，三，四个是要查找的序列,const char END[] = "\r\n";
        // 查找成功返回指向查找到的子序列中的第一个元素
        const char *lineend = std::search(buff.Peek(), buff.BeginWriteConst(), END, END+2);
        std::string line(buff.Peek(), lineend);
        switch (state_) {
            case REQUEST_LINE:
                // 解析错误
                if(!ParseRequestLine_(line)) {
                    return false;
                }
                // 解析路径
                ParsePath_();   
                break;
            case HEADERS:
                ParseHeader_(line);
                if(buff.ReadableBytes() <= 2) { 
                    //说明是空行，get请求，后面为\r\n 
                    //可读数据已经没了，说明解析完头部就已经没了,是 GET 请求
                    //请求数据不在GET方法中使用，提前结束
                    state_ = FINISH; 
                }
                break;
            case BODY:
                ParseBody_(line);
                break;
            default:
                break;
        }
        if(lineend == buff.BeginWrite()) {
            //如果缓存中的数据读完了，就跳出循环，表示
            //全部解析完。  
            buff.RetrieveAll();
            break;
        }
        // 跳过回车换行
        buff.RetrieveUntil(lineend + 2);        
    }

    return true;
}

/**
 * @brief 解析请求行
 * @param[in] line 请求行字符串
 * @return bool 解析是否成功
 */
bool HttpRequest::ParseRequestLine_(const std::string& line) {
    //([^ ]*):表示任意除了空格外的元素
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    // 用来匹配patten得到结果
    std::smatch Match;   
    if(std::regex_match(line, Match, patten)) {  
        // 匹配指定字符串整体是否符合
        // 在匹配规则中，以括号()的方式来划分组别 一共三个括号 [0]表示整体
        // 即Match[0]表示匹配的整个请求行。下标1，2，3分别表示对应的三种类别
        method_ = Match[1];
        path_ = Match[2];
        version_ = Match[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR() << "RequestLine Error: " << line;
    return false;
}

/**
 * @brief 处理请求路径
 */
void HttpRequest::ParsePath_() {
    if(path_ == "/") {
        path_ = "/index.html";
    } else {
        // 如果访问 /login，自动补全为 /login.html
        if(DEFAULT_HTML.find(path_) != DEFAULT_HTML.end()) {
            path_ += ".html";
        }
    }
}

/**
 * @brief 解析请求头
 * @param[in] line 请求头字符串
 */
void HttpRequest::ParseHeader_(const std::string& line) {
    //第一组([^:]*)：表示所有非冒号的字符，即类型
    //然后后面紧跟一个冒号，
    //然后一个空格和?，表示空格可有可无，即冒号后面可有可无空格
    //(.*)$:末尾表示值，任意非\n，即换行的元素，值里面是可以有空格的。
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch Match;
    if(std::regex_match(line, Match, patten)) {
        //解析完一行，继续请求头部，因为头部可能
        //有好几行。
        header_[Match[1]] = Match[2];
    } else { 
        // 匹配失败说明首部行匹配完了，状态变化   
        state_ = BODY;
    }
}

/**
 * @brief 从 urlencoded 格式中解析参数
 */
void HttpRequest::ParseFromUrlencoded_() {
    // 从url中解析编码
    // 如：http://localhost/index.html?key1=value1&key2=value2

    if(body_.size() == 0) return ; 
    
    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            //第一个参数是开始位置，第二个是获取子串的长度
            //成功则返回[j, i - j] 中的字符串
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG() << key << " = " << value;
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

/**
 * @brief 处理 Post 请求
 */
void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        // 从url中解析编码
        ParseFromUrlencoded_();
        if(DEFAULT_HTML_TAG.count(path_)) { 
            // 如果是登录/注册的path
            int tag = DEFAULT_HTML_TAG.find(path_)->second; 
            LOG_DEBUG() << "Tag:" << tag;
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);  // 为1则是登录
                if(UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}

/**
 * @brief 解析请求体
 * @param[in] line 请求体字符串
 */
void HttpRequest::ParseBody_(const std::string& line) {
    body_ = line;
    //因为有 body，所以是 post请求，会更改服务器中的数据，这里
    //用另外一个函数来处理。
    ParsePost_();
    state_ = FINISH;    // 状态转换为下一个状态
    LOG_DEBUG() << "Body:" << line << ", len:" << line.size();
}

/**
 * @brief 16进制转换为10进制
 * @param[in] ch 16进制字符
 * @return int 10进制数值
 */
int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') 
        return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') 
        return ch -'a' + 10;
    return ch;
}

/**
 * @brief 用户验证
 * @param[in] name 用户名
 * @param[in] pwd 密码
 * @param[in] isLogin 是否为登录操作
 * @return bool 验证是否通过
 */
bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if(name.empty() || pwd.empty()) { 
        return false; 
    }

    LOG_INFO() << "Verify name:" << name << " pwd:" << pwd;
    
    // 获取数据库连接
    std::shared_ptr<Connection> conn = ConnectionPool::GetConnectionPool().GetConnection();
    if (!conn) {
        LOG_ERROR() << "Get database connection failed!";
        return false;
    }
    
    bool flag = false;
    // unsigned int j = 0;
    char order[256] = { 0 };
    // MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!isLogin) {
        flag = true; 
    }

    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG() << order;

    res = conn->Query(order);
    if(res == nullptr) { 
        return false; 
    }

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG() << "MYSQL ROW: " << row[0] << " " << row[1];
        std::string password(row[1]);
        /* 注册行为且用户名未被使用*/
        if(isLogin) {
            if(pwd == password) { 
                flag = true; 
            } else {
                flag = false;
                LOG_INFO() << "pwd error!";
            }
        } else { 
            flag = false; 
            LOG_INFO() << "user used!";
        }
    }
    mysql_free_result(res);

    /* 注册行为 且用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG() << "regirster!";
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG() << order;
        if(!conn->Update(order)) { 
            LOG_ERROR() << "Insert error!";
            flag = false; 
        }
        flag = true;
    }

    LOG_INFO() << "UserVerify success!!";
    return flag;
}

/**
 * @brief 获取请求路径
 * @return std::string 请求路径
 */
std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}

/**
 * @brief 获取请求方法
 * @return std::string 请求方法
 */
std::string HttpRequest::method() const {
    return method_;
}

/**
 * @brief 获取 HTTP 版本
 * @return std::string HTTP 版本
 */
std::string HttpRequest::version() const {
    return version_;
}

/**
 * @brief 获取 POST 请求参数
 * @param[in] key 参数名
 * @return std::string 参数值
 */
std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

/**
 * @brief 判断是否保持连接
 * @return bool 是否保持连接
 */
bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}
