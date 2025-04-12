// socketlearn.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifdef byte
#undef byte
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <iostream>
#include <vector>
#include "classlist.h"
#include <mutex>
#include <string>
#include <functional>
#include <thread>

using namespace std;

class socketlearn {
public:
    socketlearn();
    ~socketlearn();

    bool SendData(const std::vector<char> Msg);
    bool ReceiveData(string& Msg, MessageType& type);
    string GetLastError() { std::lock_guard<std::mutex> lock(errorMutex); return ErrorMessage; }
    std::vector<char> GenerateMessage(const MessageType& type, const std::string& msg);
    std::vector<char> GenerateMessage(const MessageType& type, const uint32_t& instruction);
    SOCKET GetSocket() { return socket_fd; }

    void setReceiveCallback(std::function<void(const std::string&, MessageType)> callback);
    void startListening();

private:
    SOCKET socket_fd;
    WSADATA wsaData;
    string ErrorMessage;
    mutex errorMutex;
    std::thread m_listenThread;
    bool m_stopThread = false;
    std::function<void(const std::string&, MessageType)> m_receiveCallback;
};
// TODO: 在此处引用程序需要的其他标头。
