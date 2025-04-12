#ifndef LINKS_H
#define LINKS_H

#include <QObject>
#include <QString>
#include <QJsonObject>

#define g_links Links::instance()

// 用于描述单个小说项
struct NovelItem {
    QString name;          // 小说名称
    QString author;        // 作者
    QString type;          // 类型，例如玄幻、都市等
    QString description;   // 小说描述
    QString url;           // 小说链接

    // 将小说项转换为 QJsonObject
    QJsonObject toJson() const;

    bool operator==(const NovelItem &other) const {
        return name == other.name && author == other.author && type == other.type && description == other.description && url == other.url;
    }

    // 从 QJsonObject 中解析出小说项
    static NovelItem fromJson(const QJsonObject &obj);
};

class Links : public QObject
{
    Q_OBJECT
public:
    // 返回全局唯一的实例
    static Links& instance() {
        static Links _instance;
        return _instance;
    }

    // 禁止拷贝构造和赋值
    Links(const Links&) = delete;
    Links& operator=(const Links&) = delete;

    void SetFrontLink(const NovelItem &item);
    NovelItem GetCurrentLink() const;

    NovelItem FrontLink{ "", "", "", "", "" };
    NovelItem CurrentLink{ "", "", "", "", "" };
    QString CurrentLogin = "";
    QString ConnectName = "";

public slots:
    void SetCurrentLink(const NovelItem &item);

private:
    // 将构造函数设为私有
    explicit Links(QObject *parent = nullptr);
    ~Links();
};

#endif // LINKS_H
