#include "AllNovelManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

AllNovelManager* AllNovelManager::m_instance = nullptr;

AllNovelManager* AllNovelManager::instance(const QString &filePath) {
    if (!m_instance) {
        m_instance = new AllNovelManager(filePath);
    }
    return m_instance;
}

AllNovelManager::AllNovelManager(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
    loadNovels();
}

AllNovelManager::~AllNovelManager() {
    saveNovels();
}

bool AllNovelManager::loadNovels() {
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
    if (!rootObj.contains("allnovel") || !rootObj["allnovel"].isArray()) {
        Last_Error = "JSON 缺少 'allnovel' 数组。";
        return false;
    }
    QJsonArray array = rootObj["allnovel"].toArray();
    m_novels.clear();
    for (int i = 0; i < array.size(); ++i) {
        if (array[i].isObject()) {
            NovelItem item = NovelItem::fromJson(array[i].toObject());
            m_novels.append(item);
        }
    }
    return true;
}

bool AllNovelManager::saveNovels() {
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Last_Error = "无法打开文件写入:" + m_filePath;
        return false;
    }
    QJsonObject rootObj;
    QJsonArray array;
    for (const NovelItem &item : m_novels) {
        array.append(item.toJson());
    }
    rootObj["allnovel"] = array;
    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QVector<NovelItem> AllNovelManager::novels() const {
    return m_novels;
}

bool AllNovelManager::addNovel(const NovelItem &novel) {
    for (const NovelItem &n : m_novels) {
        if (n.name == novel.name) {
            Last_Error = "小说已存在:" + novel.name;
            return false;
        }
    }
    m_novels.append(novel);
    return saveNovels();
}

bool AllNovelManager::removeNovel(const QString &name) {
    for (int i = 0; i < m_novels.size(); ++i) {
        if (m_novels[i].name == name) {
            m_novels.removeAt(i);
            return saveNovels();
        }
    }
    Last_Error = "未找到小说:" + name;
    return false;
} 
