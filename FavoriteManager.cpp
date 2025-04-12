#include "FavoriteManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

FavoriteManager* FavoriteManager::m_instance = nullptr;

FavoriteManager* FavoriteManager::instance(const QString &filePath) {
    if (!m_instance) {
        m_instance = new FavoriteManager(filePath);
    }
    return m_instance;
}


FavoriteManager::FavoriteManager(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
    loadFavorites();
}

FavoriteManager::~FavoriteManager() {
    saveFavorites();
}

bool FavoriteManager::loadFavorites() {
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Last_Error = "无法打开文件读取:" + m_filePath;
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        Last_Error = "解析 JSON 出错:" + error.errorString();
        return false;
    }
    if (!doc.isObject()) {
        Last_Error = "JSON 格式错误，期望一个对象。";
        return false;
    }
    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("favorite") || !rootObj["favorite"].isArray()) {
        Last_Error = "JSON 缺少 'favorite' 数组。";
        return false;
    }
    QJsonArray array = rootObj["favorite"].toArray();
    m_favorites.clear();
    for (int i = 0; i < array.size(); ++i) {
        if (array[i].isObject()) {
            NovelItem item = NovelItem::fromJson(array[i].toObject());
            m_favorites.append(item);
        }
    }
    return true;
}

bool FavoriteManager::saveFavorites() {
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Last_Error = "无法打开文件写入:" + m_filePath;
        return false;
    }
    QJsonObject rootObj;
    QJsonArray array;
    for (const NovelItem &item : m_favorites) {
        array.append(item.toJson());
    }
    rootObj["favorite"] = array;
    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QVector<NovelItem> FavoriteManager::favorites() const {
    return m_favorites;
}

bool FavoriteManager::addFavorite(const NovelItem &item) {
    for (const NovelItem &fav : m_favorites) {
        if (fav.name == item.name) {
            Last_Error = "收藏项已存在:" + item.name;
            return false;
        }
    }
    m_favorites.append(item);
    return saveFavorites();
}

bool FavoriteManager::removeFavorite(const QString &name) {
    for (int i = 0; i < m_favorites.size(); ++i) {
        if (m_favorites[i].name == name) {
            m_favorites.removeAt(i);
            return saveFavorites();
        }
    }
    Last_Error = "未找到收藏项:" + name;
    return false;
} 
