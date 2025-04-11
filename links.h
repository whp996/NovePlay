#ifndef LINKS_H
#define LINKS_H

#include <QObject>
#include <QString>

#define g_links Links::instance()

struct Link
{
    QString name;
    QString url;
    QString category;
    QString desc;
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

    void SetFrontLink(const Link &link);
    Link GetCurrentLink() const;

    Link FrontLink{ "", "", "" };
    Link CurrentLink{ "", "", "" };
    QString CurrentLogin = "";
    QString ConnectName = "";

public slots:
    void SetCurrentLink(const Link &link);

private:
    // 将构造函数设为私有
    explicit Links(QObject *parent = nullptr);
    ~Links();
};

#endif // LINKS_H
