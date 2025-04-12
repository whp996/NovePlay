// socketlearn.cpp: 定义应用程序的入口点。
//

#include "socketlearn.h"
#include <WinSock2.h>
#include <cstring>
#include <iostream>
#include <WS2tcpip.h>
#include <stdexcept>
#include "socketlearn.h"
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

socketlearn::socketlearn()
{
    sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        throw std::runtime_error("WSAStartup failed!");
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == INVALID_SOCKET)
    {
        throw std::runtime_error("Socket creation failed");
        WSACleanup();
    }

    server.sin_family = AF_INET;
    server.sin_port = htons("host port");
    if (inet_pton(AF_INET, "host location", &server.sin_addr) <= 0)
    {
        throw std::runtime_error("invalid address/Address not suported");
        closesocket(socket_fd);
        WSACleanup();
    }

    if (connect(socket_fd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        throw std::runtime_error(std::to_string(WSAGetLastError()) + ":Connection failed");
    }
}

socketlearn::~socketlearn()
{
	if (socket_fd != INVALID_SOCKET)
    {
        shutdown(socket_fd, SD_BOTH);
        closesocket(socket_fd);
        WSACleanup();
        std::cout<< "socket close" << std::endl;
    }

    m_stopThread = true;
    if (m_listenThread.joinable())
        m_listenThread.join();
}

bool socketlearn::SendData(const std::vector<char> Msg)
{
    size_t totalSent = 0;
    while (totalSent < Msg.size())
    {
        int res = send(socket_fd, Msg.data() + totalSent, Msg.size() - totalSent, 0);
        if (res == SOCKET_ERROR)
        {
            ErrorMessage = std::to_string(WSAGetLastError());
            return false;
        }
        totalSent += res;
    }
    return true;
}

bool socketlearn::ReceiveData(string& Msg, MessageType& type)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    if(select(0, &readfds, nullptr, nullptr, nullptr) <= 0) {
        ErrorMessage = std::to_string(WSAGetLastError());
        return false;
    }
    if(!FD_ISSET(socket_fd, &readfds)) {
        Msg = "No data";
        return false;
    }
    char Msgtype[1];
    int res = recv(socket_fd, Msgtype, sizeof(Msgtype), 0);
    if (res == SOCKET_ERROR)
    {
        ErrorMessage = std::to_string(WSAGetLastError());
        return false;
    }
    else if (res == 0)
    {
        ErrorMessage = "Connection closed";
        return false;
    }

    MessageType msgtype = static_cast<MessageType>(Msgtype[0]);
    type = msgtype;
    int leng;
    switch (msgtype)
    {
		case MessageType::forward_msg:
		{
			char MessageLength[4];
			int res = recv(socket_fd, MessageLength, sizeof(MessageLength), 0);
			if (res == SOCKET_ERROR)
			{
				ErrorMessage = std::to_string(WSAGetLastError());
				return false;
			}
			uint32_t dataLength = ntohl(*reinterpret_cast<uint32_t*>(MessageLength));
			leng = dataLength;
			int totalread = 0;
			char* buffer = new char[dataLength];
			while (totalread < dataLength)
			{
				res = recv(socket_fd, buffer + totalread, dataLength - totalread, 0);
				if (res == SOCKET_ERROR)
				{
					ErrorMessage = std::to_string(WSAGetLastError());
					delete[] buffer;
					return false;
				}
				totalread += res;
			}
			if(totalread != dataLength)
			{
				ErrorMessage = "Receive data failed";
				delete[] buffer;
				return false;
			}
			Msg = std::string(buffer, dataLength);
			delete[] buffer;
			break;
		}
		case MessageType::return_msg:
		{
			char MsgLength[4];
			int res = recv(socket_fd, MsgLength, sizeof(MsgLength), 0);
			if (res == SOCKET_ERROR)
			{
				ErrorMessage = std::to_string(WSAGetLastError());
				return false;
			}
			uint32_t msgLength = ntohl(*reinterpret_cast<uint32_t*>(MsgLength));
			leng = msgLength;
			char* buffer = new char[msgLength];
			int totalread = 0;
			while (totalread < msgLength)
			{
				res = recv(socket_fd, buffer + totalread, msgLength - totalread, 0);
				if (res == SOCKET_ERROR)
				{
					ErrorMessage = std::to_string(WSAGetLastError());
					delete[] buffer;
					return false;
				}
				totalread += res;
			}
			if(totalread != msgLength)
			{
				ErrorMessage = "Receive data failed";
				delete[] buffer;
				return false;
			}
			Msg = std::string(buffer, msgLength);
            if(Msg == "heartbeat") {
                uint32_t inst = static_cast<uint32_t>(InstructionType::heartbeat_ACK);
                vector<char> data = GenerateMessage(MessageType::instruction, inst);
                SendData(data);
            }
			delete[] buffer;
			break;
		}
    }
    return true;
}

std::vector<char> socketlearn::GenerateMessage(const MessageType& type, const std::string& msg)
{
    uint8_t msgType = static_cast<uint8_t>(type);
    uint32_t msgLength = htonl(static_cast<uint32_t>(msg.length()));
    std::vector<char> data(1 + sizeof(msgLength) + msg.length());
    data[0] = msgType;
    memcpy(data.data() + 1, &msgLength, sizeof(msgLength));
    memcpy(data.data() + 1 + sizeof(msgLength), msg.c_str(), msg.length());
    return data;
}

std::vector<char> socketlearn::GenerateMessage(const MessageType& type, const uint32_t& instruction)
{
    uint8_t msgType = static_cast<uint8_t>(type);
    std::vector<char> data(1 + sizeof(instruction));
    data[0] = msgType;
    memcpy(data.data() + 1, &instruction, sizeof(instruction));
    return data;
} 

void socketlearn::setReceiveCallback(std::function<void(const std::string&, MessageType)> callback)
{
    m_receiveCallback = callback;
}

void socketlearn::startListening()
{
    if(m_listenThread.joinable()) return;
    m_stopThread = false;
    m_listenThread = std::thread([this](){
        while(!m_stopThread && this->GetSocket() != INVALID_SOCKET) {
            std::string msg;
            MessageType type;
            if(!this->ReceiveData(msg, type)) {
				if(msg == "No data") {
					continue;
				}
				m_receiveCallback("disconnect", MessageType::error);
                break;
            }
            if(m_receiveCallback) {
                m_receiveCallback(msg, type);
            }
        }
    });
}

