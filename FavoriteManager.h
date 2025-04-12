#ifndef FAVORITEMANAGER_H
#define FAVORITEMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include "links.h"


class FavoriteManager : public QObject {
    Q_OBJECT
public:
    static FavoriteManager* instance(const QString &filePath = QString(PROJECT_DIR) + "/Favorite.json");

    bool loadFavorites();
    bool saveFavorites();
    QVector<NovelItem> favorites() const;
    bool addFavorite(const NovelItem &item);
    bool removeFavorite(const QString &name);
    QString getLastError() const{return Last_Error;}

private:
    explicit FavoriteManager(const QString &filePath, QObject *parent = nullptr);
    ~FavoriteManager();

    Q_DISABLE_COPY(FavoriteManager)

    static FavoriteManager* m_instance;
    QString Last_Error = "";

    QString m_filePath;              // Favorite.json 文件的路径
    QVector<NovelItem> m_favorites; // 内存中保存的收藏列表
};

#endif // FAVORITEMANAGER_H 
