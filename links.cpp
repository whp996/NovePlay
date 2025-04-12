#include "links.h"

QJsonObject NovelItem::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["author"] = author;
    obj["type"] = type;
    obj["description"] = description;
    obj["url"] = url;
    return obj;
}

NovelItem NovelItem::fromJson(const QJsonObject &obj) {
    NovelItem item;
    item.name = obj["name"].toString();
    item.author = obj["author"].toString();
    item.type = obj["type"].toString();
    item.description = obj["description"].toString();
    item.url = obj["url"].toString();
    return item;
}

Links::Links(QObject *parent)
    : QObject(parent)
{
}

Links::~Links()
{
}

void Links::SetFrontLink(const NovelItem &item)
{
    FrontLink = item;
}

NovelItem Links::GetCurrentLink() const
{
    return CurrentLink;
}

void Links::SetCurrentLink(const NovelItem &item)
{
    CurrentLink = item;
}
