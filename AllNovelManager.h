#ifndef ALLNOVELMANAGER_H
#define ALLNOVELMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include "links.h"

class AllNovelManager : public QObject {
    Q_OBJECT
public:
    static AllNovelManager* instance(const QString &filePath = QString(PROJECT_DIR) + "/allnovel.json");

    bool loadNovels();
    bool saveNovels();
    QVector<NovelItem> novels() const;
    bool addNovel(const NovelItem &novel);
    bool removeNovel(const QString &name);
    QString getLastError() const{return Last_Error;}


private:
    explicit AllNovelManager(const QString &filePath, QObject *parent = nullptr);
    ~AllNovelManager();

    Q_DISABLE_COPY(AllNovelManager)

    static AllNovelManager* m_instance;
    QString Last_Error = "";

    QString m_filePath;       // allnovel.json 文件的路径
    QVector<NovelItem> m_novels; // 内存中的小说列表
};

#endif // ALLNOVELMANAGER_H 
