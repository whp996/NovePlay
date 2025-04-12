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
            
            stream << static_cast<qint32>(msg.type);

            stream << msg.timestamp;

            stream << msg.data;

            return data;
    };
    static Message deserialize(const QByteArray &data){
            Message msg;
            QDataStream stream(data);

            qint32 type;
            stream >> type;
            msg.type = static_cast<fileType>(type);

            stream >> msg.timestamp;

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
