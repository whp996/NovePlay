#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include "ChatApperro.h"
#include <fcntl.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

struct ClientConnection {
    int fd; // 客户端 socket 描述符
    std::mutex sendMutex; // 保护发送队列的互斥锁
    std::queue<std::vector<char>> sendQueue; // 待发送的消息队列
    std::condition_variable cv; // 通知等待发送线程新的消息到来
};

enum class MessageType {
    login = 0,
    register_user = 1,
    forward_msg = 2,
    instruction = 3,
    return_msg = 4
};

enum class InstructionType {
    delete_self = 0,
    change_password = 1,
    get_all_user = 2,
    get_all_online_users = 3,
    heartbeat_ACK = 4,
    logout = 5
};

class Serve {
public:
    Serve();
    ~Serve();

    void start();
    void stop();
    void restart();

private:
    int listen_port_;
    sqlite3* db_;

    void HandleMessage(MessageType msgType, std::string& dataStr, int client_fd);
    void HandleClient(std::shared_ptr<ClientConnection> conn, const std::string &username);
    void HandleInitialConnection(int client_fd);
    std::vector<char> GenerateReturnMsg(const std::string &msg);
    void HandleClientMessage(std::shared_ptr<ClientConnection> conn, uint8_t msgTypeByte, const std::string &username, bool &exitLoop);
    void BroadcastNotification(const std::vector<char> &notification);
};