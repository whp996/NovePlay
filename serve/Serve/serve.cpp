#include "serve.h"
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <sqlite3.h>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <regex>
#include <netinet/tcp.h>

// 全局在线客户端列表：存储用户名及其对应的连接描述符（TCP socket）
static std::unordered_map<std::string, std::shared_ptr<ClientConnection>> onlineClients;
// 保护在线列表的互斥锁
static std::mutex clientsMutex;

static void FlushSocketBuffer(int sock_fd) {
    char flushBuffer[1024];
    int flags = fcntl(sock_fd, F_GETFL, 0);
    // 将 socket 设置为非阻塞模式
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
    while (true) {
        ssize_t n = recv(sock_fd, flushBuffer, sizeof(flushBuffer), 0);
        if (n <= 0) break;
    }
    // 恢复原有非非阻塞设置
    fcntl(sock_fd, F_SETFL, flags);
} 

Serve::Serve()
{
    listen_port_ = 4567;
    db_ = nullptr;
}

Serve::~Serve()
{
    if(db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Serve::start()
{
    // 初始化并连接 SQLite 数据库（数据库文件名：ChatServe.db，如果不存在则自动创建）
    int rc = sqlite3_open("ChatServe.db", &db_);
    if(rc != SQLITE_OK)
    {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }
    std::cout << "成功打开数据库." << std::endl;

    // 创建 users 表（若不存在则创建），注意字段定义按 SQLite 语法写
    const char* sql_create = "CREATE TABLE IF NOT EXISTS users ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                             "username TEXT NOT NULL UNIQUE, "
                             "password TEXT NOT NULL, "
                             "create_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
    char* errMsg = nullptr;
    rc = sqlite3_exec(db_, sql_create, nullptr, nullptr, &errMsg);
    if(rc != SQLITE_OK)
    {
        std::cerr << "创建表出错: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }

    // 创建 TCP 监听套接字
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0)
    {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(listen_port_);

    if(bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(sock_fd);
        return;
    }

    if(listen(sock_fd, 5) < 0)
    {
        perror("listen");
        close(sock_fd);
        return;
    }

    std::cout << "server is listening on port " << listen_port_ << " ..." << std::endl;
    
    // 主线程持续监听客户端连接请求
    while(true)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);
        std::cout << "client_fd: " << client_fd << std::endl;
        if(client_fd < 0)
        {
            perror("accept");
            continue;
        }

        // 将初始握手处理放入独立线程中，避免阻塞 accept() 循环
        std::thread t(&Serve::HandleInitialConnection, this, client_fd);
        t.detach();
    }
    // 关闭监听套接字（正常流程一般不会执行到这里）
    close(sock_fd);
}

void Serve::stop()
{
    for(auto &client : onlineClients)
    {
        // 关闭每个连接的socket fd
        close(client.second->fd);
    }
    std::cout << "停止服务..." << std::endl;
}

void Serve::restart()
{
    stop();
    start();
}

void Serve::HandleMessage(MessageType msgType, std::string& dataStr, int client_fd)
{
    std::string response = "";
    int rc = 0; // 添加 rc 变量，便于 SQLite API 调用
    // 根据消息类型分别处理
    switch(msgType) {
        case MessageType::register_user:
            {
                // 预期数据格式： "username|password"
                std::vector<std::string> tokens;
                size_t pos = 0;
                while((pos = dataStr.find("|")) != std::string::npos)
                {
                    tokens.push_back(dataStr.substr(0, pos));
                    dataStr.erase(0, pos + 1);
                }
                tokens.push_back(dataStr);
                
                if(tokens.size() >= 2)
                {
                    std::string username = tokens[0];
                    std::string password = tokens[1];

                    // 限制1：用户名只能为中文或英文或数字
                    // 限制2：密码只能由英文大小写字母和数字组成
                    std::regex usernameRegex("^[A-Za-z0-9一-龥]+$");
                    std::regex passwordRegex("^[A-Za-z0-9]+$");
                    if(!std::regex_match(username, usernameRegex))
                    {
                        response = "register:invalid_username";
                        std::vector<char> returnMsg = GenerateReturnMsg(response);
                        send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                        close(client_fd);
                        return;
                    }
                    if(!std::regex_match(password, passwordRegex))
                    {
                        response = "register:invalid_password";
                        std::vector<char> returnMsg = GenerateReturnMsg(response);
                        send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                        close(client_fd);
                        return;
                    }
                    std::cout << "username: " << username << std::endl;
                    std::cout << "password: " << password << std::endl;
                    // 限制3：注册用户总数不超过20个
                    std::string count_sql = "SELECT COUNT(*) FROM users;";
                    sqlite3_stmt* stmt = nullptr;
                    rc = sqlite3_prepare_v2(db_, count_sql.c_str(), -1, &stmt, nullptr);
                    int userCount = 0;
                    if(rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
                        userCount = sqlite3_column_int(stmt, 0);
                    }
                    sqlite3_finalize(stmt);
                    if(userCount >= 20) {
                        response = "register:user_limit_reached";
                        std::cout << "user limit reached" << std::endl;
                        std::vector<char> returnMsg = GenerateReturnMsg(response);
                        send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                        close(client_fd);
                        return;
                    }

                    // 插入用户信息（对输入进行转义建议使用 SQLite API 自带功能或手工构造）
                    std::string insert_sql = "INSERT INTO users (username, password) VALUES ('" +
                                                username + "', '" + password + "');";
                    char* errMsg = nullptr;
                    rc = sqlite3_exec(db_, insert_sql.c_str(), nullptr, nullptr, &errMsg);
                    if(rc != SQLITE_OK)
                    {
                        std::string error = errMsg;
                        std::cerr << "用户注册失败: " << error << std::endl;
                        sqlite3_free(errMsg);
                        if(error.find("UNIQUE constraint failed") != std::string::npos) {
                            response = "register:username_exists";
                        } else {
                            response = "register:register_failed";
                        }
                        std::vector<char> returnMsg = GenerateReturnMsg(response);
                        send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                        close(client_fd);
                        return;
                    }
                    else
                    {
                        std::cout << "用户注册成功: " << username << std::endl;
                        response = "register:register_success";
                        std::vector<char> returnMsg = GenerateReturnMsg(response);
                        send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                        close(client_fd);
                        return;
                    }
                }
                else
                {
                    std::cerr << "注册消息格式错误。" << std::endl;
                    response = "register:register_failed";
                    std::vector<char> returnMsg = GenerateReturnMsg(response);
                    send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                    close(client_fd);
                    return;
                }
            }
            break;

        case MessageType::login:
            {
                // 预期数据格式： "username|password"
                std::vector<std::string> tokens;
                size_t pos = 0;
                while((pos = dataStr.find("|")) != std::string::npos)
                {
                    tokens.push_back(dataStr.substr(0, pos));
                    dataStr.erase(0, pos + 1);
                }
                tokens.push_back(dataStr);
                if(tokens.size() >= 2)
                {
                    std::string username = tokens[0];
                    std::string password = tokens[1];

                    std::string select_sql = "SELECT password FROM users WHERE username = '" + username + "';";
                    sqlite3_stmt* stmt = nullptr;
                    rc = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &stmt, nullptr);
                    if(rc != SQLITE_OK)
                    {
                        response = "login:login_failed, user_not_exist";
                        std::cout << "用户登录失败，用户不存在: " << username << std::endl;
                        std::vector<char> returnMsg = GenerateReturnMsg(response);
                        send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                        close(client_fd);
                        return;
                    }
                    else
                    {
                        if(sqlite3_step(stmt) == SQLITE_ROW)
                        {
                            if(password == reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)))
                            {
                                if(onlineClients.find(username) != onlineClients.end()) {
                                    response = "login:login_failed, user_already_online";
                                    std::cout << "用户登录失败，用户已在线: " << username << std::endl;
                                    std::vector<char> returnMsg = GenerateReturnMsg(response);
                                    send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                                    close(client_fd);
                                    return;
                                }
                                std::cout << "用户登录成功: " << username << std::endl;
                                response = "login:login_success";
                                std::vector<char> returnMsg = GenerateReturnMsg(response);
                                send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                                {
                                    auto conn = std::make_shared<ClientConnection>();
                                    conn->fd = client_fd;
                                    {
                                        std::lock_guard<std::mutex> lock(clientsMutex);
                                        // 存入在线客户端列表
                                        onlineClients[username] = conn;
                                    }
                                    // 启动处理线程，该线程负责收发消息
                                    std::thread(&Serve::HandleClient, this, conn, username).detach();
                                }
                                std::vector<char> notification = GenerateReturnMsg("user_online:" + username);
                                BroadcastNotification(notification);
                                std::cout << "用户登录成功，通知所有在线用户" << std::endl;
                                return;
                            }
                            else
                            {
                                std::cout << "用户登录失败，密码错误: " << username << std::endl;
                                response = "login:login_failed, password_error"; 
                                std::vector<char> returnMsg = GenerateReturnMsg(response);
                                send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                                close(client_fd);
                                return;
                            }
                        }
                        else
                        {
                            std::cout << "用户登录失败，用户不存在: " << username << std::endl;
                            response = "login:login_failed, user_not_exist";
                            std::vector<char> returnMsg = GenerateReturnMsg(response);
                            send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                            close(client_fd);
                            return;
                        }
                    }
                    sqlite3_finalize(stmt);
                }
                else
                {
                    std::cerr << "登录消息格式错误。" << std::endl;
                    response = "login:login_failed, format_error";
                    std::vector<char> returnMsg = GenerateReturnMsg(response);
                    send(client_fd, returnMsg.data(), returnMsg.size(), 0);
                    close(client_fd);
                    return;
                }
            }
            break;
    }
    return;
}

// 用于处理持续连接的客户端
void Serve::HandleClient(std::shared_ptr<ClientConnection> conn, const std::string &username) {
    int client_fd = conn->fd;
    std::cout << "开始处理客户端[" << username << "]的持续连接" << std::endl;
    bool exitLoop = false; // 用于跳出while循环
    time_t heartbeatSent = 0; // 记录发送 heartbeat 的时间，0 表示当前未发送
    while(!exitLoop) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);

        // 设置超时时间为10秒钟
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int activity = select(client_fd + 1, &readfds, NULL, NULL, &timeout);

        if (activity == 0) {
            if (heartbeatSent == 0) {
                // 第一次超时，发送 heartbeat 消息，并启动计时
                std::string heartbeat = "heartbeat";
                std::vector<char> hbMsg = GenerateReturnMsg(heartbeat);
                int sent = send(client_fd, hbMsg.data(), hbMsg.size(), 0);
                if(sent <= 0) {
                    std::cerr << "发送心跳检测给[" << username << "]失败" << std::endl;
                    exitLoop = true;
                    break;
                } else {
                    std::cout << "发送心跳检测给[" << username << "]成功" << std::endl;
                }
                heartbeatSent = time(NULL);
            } else {
                // 已发送 heartbeat，检查是否超过20秒
                if(time(NULL) - heartbeatSent >= 20) {
                    std::cerr << "Heartbeat ACK 超时，断开连接: " << username << std::endl;
                    exitLoop = true;
                    break;
                }
            }
            continue;
        } else if (activity < 0) {
            std::cerr << "select error for " << username << std::endl;
            exitLoop = true;
            break;
        }

        if(FD_ISSET(client_fd, &readfds)) {
            uint8_t msgTypeByte;
            ssize_t ret = read(client_fd, &msgTypeByte, 1);
            if(ret <= 0) {
                std::cout << "客户端[" << username << "]断开连接或读取失败" << std::endl;
                FlushSocketBuffer(client_fd);
                exitLoop = true;
                break;
            }
            // 收到数据时，认为客户端有响应，清零 heartbeat 计时变量
            heartbeatSent = 0;
            HandleClientMessage(conn, msgTypeByte, username, exitLoop);
        }

        // 每次循环检查发送队列中是否有待发送的消息
        {
            std::unique_lock<std::mutex> lock(conn->sendMutex);
            while(!conn->sendQueue.empty()) {
                auto msg = conn->sendQueue.front();
                conn->sendQueue.pop();
                // 发送消息
                send(client_fd, msg.data(), msg.size(), 0);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        onlineClients.erase(username);
    }
    std::vector<char> notification = GenerateReturnMsg("user_offline:" + username);
    BroadcastNotification(notification);
    std::cout << "用户[" << username << "]下线，通知所有在线用户" << std::endl;
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
}

std::vector<char> Serve::GenerateReturnMsg(const std::string &msg) {
    uint8_t msgType = static_cast<uint8_t>(MessageType::return_msg);
    uint32_t netMsgLength = htonl(msg.size());
    std::vector<char> buffer(1 + sizeof(netMsgLength) + msg.size());
    buffer[0] = msgType;
    memcpy(buffer.data() + 1, &netMsgLength, sizeof(netMsgLength));
    memcpy(buffer.data() + 1 + sizeof(netMsgLength), msg.c_str(), msg.size());
    return buffer;
}

void Serve::HandleClientMessage(std::shared_ptr<ClientConnection> conn, uint8_t msgTypeByte, const std::string &username, bool &exitLoop) {
    MessageType msgType = static_cast<MessageType>(msgTypeByte);
    ssize_t ret = 0;
    switch(msgType) {
        case MessageType::forward_msg:
            {
                uint32_t netDataLength;
                ret = read(conn->fd, &netDataLength, 4);
                if(ret != 4) {
                    std::cerr << "read netDataLength failed" << std::endl;
                    FlushSocketBuffer(conn->fd);
                    exitLoop = true;
                    return;
                }
                uint32_t dataLength = ntohl(netDataLength);
                std::vector<char> payload(dataLength);
                size_t totalRead = 0;
                while(totalRead < dataLength) {
                    ssize_t n = read(conn->fd, payload.data() + totalRead, dataLength - totalRead);
                    if(n <= 0) return;
                    totalRead += n;
                }
                if(totalRead != dataLength) {
                    std::cerr << "data read incomplete" << std::endl;
                    FlushSocketBuffer(conn->fd);
                    exitLoop = true;
                    return;
                }
                std::string msg(payload.begin(), payload.end());
                // 转发消息格式： "targetUsername|message"
                size_t pos = msg.find("|");
                if(pos != std::string::npos) {
                    std::string target = msg.substr(0, pos);
                    uint8_t msgType = static_cast<uint8_t>(MessageType::forward_msg);
                    std::string forwardMsg = username + "|" + msg.substr(pos + 1);
                    uint32_t netMsgLength = htonl(forwardMsg.size());
                    size_t totalLength = 1 + sizeof(netMsgLength) + forwardMsg.size();
                    std::vector<char> buffer(totalLength);
                    buffer[0] = msgType;
                    memcpy(buffer.data() + 1, &netMsgLength, sizeof(netMsgLength));
                    memcpy(buffer.data() + 1 + sizeof(netMsgLength), forwardMsg.c_str(), forwardMsg.size());
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    if(onlineClients.find(target) != onlineClients.end()) {
                        int target_fd = onlineClients[target]->fd;
                        int totalsend = 0;
                        while(totalsend < totalLength) {
                            int send_len = send(target_fd, buffer.data() + totalsend, totalLength - totalsend, 0);
                            if(send_len <= 0) {
                                break;
                            }
                            totalsend += send_len;
                        }
                        std::string msg = "forward success";
                        std::vector<char> returnMsg = GenerateReturnMsg(msg);
                        std::lock_guard<std::mutex> lock(conn->sendMutex);
                        conn->sendQueue.push(returnMsg);
                    } else {
                        // 如果目标客户端不在线，回馈给发送者提示
                        std::string err = "用户" + target + "不在线";
                        std::vector<char> returnMsg = GenerateReturnMsg(err);
                        std::lock_guard<std::mutex> lock(conn->sendMutex);
                        conn->sendQueue.push(returnMsg);
                    }
                } else {
                    // 收到其它格式消息，可选择进行处理或忽略
                    std::cout << "客户端[" << username << "]发送未知格式消息: " << msg << std::endl;
                    FlushSocketBuffer(conn->fd);
                }
            }
            break;
        case MessageType::instruction:
            {
                uint32_t type;
                ret = read(conn->fd, &type, 4);
                if(ret != 4) {
                    std::cerr << "read instruction type failed" << std::endl;
                    FlushSocketBuffer(conn->fd);
                    exitLoop = true;
                    return;
                }
                switch(static_cast<InstructionType>(type)) {
                    case InstructionType::logout:
                        {
                            std::cout << "收到[" << username << "]退出请求" << std::endl;
                            exitLoop = true;
                            return;
                        }
                        break;
                    case InstructionType::delete_self:
                        {
                            // 构造 SQL 语句，删除当前用户记录
                            std::string delete_sql = "DELETE FROM users WHERE username = '" + username + "';";
                            char* errMsg = nullptr;
                            int rc = sqlite3_exec(db_, delete_sql.c_str(), nullptr, nullptr, &errMsg);
                            if(rc != SQLITE_OK) {
                                std::cerr << "删除用户[" << username << "]失败: " << errMsg << std::endl;
                                sqlite3_free(errMsg);
                                std::vector<char> returnMsg = GenerateReturnMsg("delete failed");
                                std::lock_guard<std::mutex> lock(conn->sendMutex);
                                conn->sendQueue.push(returnMsg);
                            } else {
                                std::cout << "成功删除用户[" << username << "]" << std::endl;
                                std::vector<char> returnMsg = GenerateReturnMsg("delete success");
                                std::lock_guard<std::mutex> lock(conn->sendMutex);
                                conn->sendQueue.push(returnMsg);
                            }
                            exitLoop = true; // 跳出while循环
                            return;
                        }
                        break;
                    case InstructionType::change_password:
                        {
                            // 构造 SQL 语句，更新当前用户密码
                            uint32_t netDataLength;
                            ret = read(conn->fd, &netDataLength, 4);
                            if(ret != 4) {
                                std::cerr << "read netDataLength failed" << std::endl;
                                FlushSocketBuffer(conn->fd);
                                exitLoop = true;
                                return;
                            }
                            uint32_t dataLength = ntohl(netDataLength);
                            std::vector<char> payload(dataLength);
                            size_t totalRead = 0;
                            while(totalRead < dataLength) {
                                ssize_t n = read(conn->fd, payload.data() + totalRead, dataLength - totalRead);
                                if(n <= 0) break;
                                totalRead += n;
                            }
                            if(totalRead != dataLength) {
                                std::cerr << "data read incomplete" << std::endl;
                                FlushSocketBuffer(conn->fd);
                                exitLoop = true;
                                return;
                            }
                            std::string password(payload.begin(), payload.end());
                            size_t pos = password.find("|");
                            std::string old_password;
                            std::string new_password;
                            if(pos != std::string::npos) {
                                old_password = password.substr(0, pos);
                                new_password = password.substr(pos + 1);
                            }

                            std::string get_old_password_sql = "SELECT password FROM users WHERE username = '" + username + "';";
                            sqlite3_stmt* stmt = nullptr;
                            int rc = sqlite3_prepare_v2(db_, get_old_password_sql.c_str(), -1, &stmt, nullptr);
                            if(rc != SQLITE_OK) {
                                std::cerr << "获取旧密码失败: " << sqlite3_errmsg(db_) << std::endl;
                                std::vector<char> returnMsg = GenerateReturnMsg("change password failed");
                                std::lock_guard<std::mutex> lock(conn->sendMutex);
                                conn->sendQueue.push(returnMsg);
                                return;
                            }
                            std::regex passwordRegex("^[A-Za-z0-9]+$");
                            if(!std::regex_match(new_password, passwordRegex)) {
                                std::vector<char> returnMsg = GenerateReturnMsg("new password invalid");
                                std::lock_guard<std::mutex> lock(conn->sendMutex);
                                conn->sendQueue.push(returnMsg);
                                sqlite3_finalize(stmt);
                                return;
                            }
                            if(sqlite3_step(stmt) == SQLITE_ROW) {
                                if(old_password == reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))) {
                                    std::string update_sql = "UPDATE users SET password = '" + new_password + "' WHERE username = '" + username + "';";
                                    char* errMsg = nullptr;
                                    rc = sqlite3_exec(db_, update_sql.c_str(), nullptr, nullptr, &errMsg);
                                    if(rc != SQLITE_OK) {
                                        std::cerr << "更新密码失败: " << errMsg << std::endl;
                                        sqlite3_free(errMsg);
                                        std::vector<char> returnMsg = GenerateReturnMsg("change password failed");
                                        std::lock_guard<std::mutex> lock(conn->sendMutex);
                                        conn->sendQueue.push(returnMsg);
                                    } else {
                                        std::cout << "密码更新成功: " << username << std::endl;
                                        std::vector<char> returnMsg = GenerateReturnMsg("change password success");
                                        std::lock_guard<std::mutex> lock(conn->sendMutex);
                                        conn->sendQueue.push(returnMsg);
                                    }
                                } else {
                                    std::vector<char> returnMsg = GenerateReturnMsg("old password error");
                                    std::lock_guard<std::mutex> lock(conn->sendMutex);
                                    conn->sendQueue.push(returnMsg);
                                    sqlite3_finalize(stmt);
                                    return;
                                }
                            }
                            sqlite3_finalize(stmt);
                        }
                        break;
                    case InstructionType::get_all_user:
                        {
                            std::string get_all_user_sql = "SELECT username FROM users;";
                            sqlite3_stmt* stmt = nullptr;
                            int rc = sqlite3_prepare_v2(db_, get_all_user_sql.c_str(), -1, &stmt, nullptr);
                            std::string all_user = "all_user:";
                            if(rc == SQLITE_OK) {
                                while(sqlite3_step(stmt) == SQLITE_ROW) {
                                    all_user += std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))) + "|";
                                }
                            }
                            sqlite3_finalize(stmt);
                            std::vector<char> returnMsg = GenerateReturnMsg(all_user);
                            std::lock_guard<std::mutex> lock(conn->sendMutex);
                            conn->sendQueue.push(returnMsg);
                        }
                        break;
                    case InstructionType::get_all_online_users:
                        {
                            std::string all_online_users = "all_online_users:";
                            {
                            std::lock_guard<std::mutex> lock(clientsMutex);
                            for(auto &client : onlineClients) {
                                    all_online_users += client.first + "|";
                                }
                            }
                            std::vector<char> returnMsg = GenerateReturnMsg(all_online_users);
                            std::lock_guard<std::mutex> lock2(conn->sendMutex);
                            conn->sendQueue.push(returnMsg);
                            std::cout << "all_online_users: " << all_online_users << std::endl;
                        }
                        break;
                    case InstructionType::heartbeat_ACK:
                        {
                            std::cout << "收到[" << username << "]心跳ACK" << std::endl;
                        }
                        break;
                    default:
                        std::cerr << "未知的instruction类型" << std::endl;
                        {
                            std::vector<char> returnMsg = GenerateReturnMsg("unknown instruction type");
                            std::lock_guard<std::mutex> lock(conn->sendMutex);
                            conn->sendQueue.push(returnMsg);
                            FlushSocketBuffer(conn->fd);
                        }
                        break;
                }
            }
            break;
        default:
            std::cerr << "未知的消息类型" << std::endl;
            std::vector<char> returnMsg = GenerateReturnMsg("unknown message type");
            {
                std::lock_guard<std::mutex> lock(conn->sendMutex);
                conn->sendQueue.push(returnMsg);
            }
            FlushSocketBuffer(conn->fd);
            break;
    }
}

void Serve::HandleInitialConnection(int client_fd) {
    // 设置接收超时机制，读取超时设为5秒
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
         perror("setsockopt: SO_RCVTIMEO");
    }

    uint8_t msgTypeByte;
    ssize_t ret = read(client_fd, &msgTypeByte, 1);
    if(ret != 1) {
        if(ret < 0)
            perror("read msgTypeByte timeout or error");
        else
            std::cerr << "read msgTypeByte timeout" << std::endl;
        FlushSocketBuffer(client_fd);
        close(client_fd);
        return;
    }
    MessageType msgType = static_cast<MessageType>(msgTypeByte);

    uint32_t netDataLength;
    ret = read(client_fd, &netDataLength, 4);
    if(ret != 4) {
        std::cerr << "read netDataLength failed in HandleInitialConnection" << std::endl;
        close(client_fd);
        return;
    }
    uint32_t dataLength = ntohl(netDataLength);

    std::vector<char> payload(dataLength);
    size_t totalRead = 0;
    while(totalRead < dataLength) {
        ssize_t n = read(client_fd, payload.data() + totalRead, dataLength - totalRead);
        if(n <= 0)
            break;
        totalRead += n;
    }
    if(totalRead != dataLength) {
        std::cerr << "data read incomplete in HandleInitialConnection" << std::endl;
        close(client_fd);
        return;
    }
    std::string dataStr(payload.begin(), payload.end());
    std::cout << "dataStr: " << dataStr << std::endl;
    // 调用原有的消息处理逻辑
    HandleMessage(msgType, dataStr, client_fd);
}

void Serve::BroadcastNotification(const std::vector<char> &notification) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto &pair : onlineClients) {
        auto conn = pair.second;
        {
            std::lock_guard<std::mutex> lock2(conn->sendMutex);
            conn->sendQueue.push(notification);
        }
        conn->cv.notify_one();
    }
}