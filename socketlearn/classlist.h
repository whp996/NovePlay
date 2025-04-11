#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QIODevice>
#include <QtCore/QDataStream>

enum class fileType : int {
    Text,
    Image,
    Voice,
    Emoji,
};

struct Message {
    fileType type;
    QByteArray data;
    QString timestamp;
    
    static QByteArray serialize(const Message &msg){
            QByteArray data;
            QDataStream stream(&data, QIODevice::WriteOnly);
            
            // 写入消息类型
            stream << static_cast<qint32>(msg.type);

            // 写入时间戳
            stream << msg.timestamp;

            // 写入数据
            stream << msg.data;

            return data;
    };
    static Message deserialize(const QByteArray &data){
            Message msg;
            QDataStream stream(data);

            // 读取消息类型
            qint32 type;
            stream >> type;
            msg.type = static_cast<fileType>(type);

            // 读取时间戳
            stream >> msg.timestamp;

            // 读取数据
            stream >> msg.data;

            return msg;
    };
};

enum class MessageType {
    login = 0,
    register_user = 1,
    forward_msg = 2,
    instruction = 3,
    return_msg = 4,
    error = 5
};

enum class InstructionType {
    delete_self = 0,
    change_password = 1,
    get_all_user = 2,
    get_all_online_users = 3,
    heartbeat_ACK = 4,
    logout = 5
};
