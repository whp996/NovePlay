#ifndef NOVELRECOMMENDER_H
#define NOVELRECOMMENDER_H

#include <QObject>
#include <QVector>
#include <QSet>
#include "FavoriteManager.h"
#include "AllNovelManager.h"
#include "links.h"

class NovelRecommender : public QObject {
    Q_OBJECT
public:
    explicit NovelRecommender(QObject *parent = nullptr);
    ~NovelRecommender();

    QVector<NovelItem> recommendNovels(int count = 5) const;

private:
    FavoriteManager *m_favManager;
    AllNovelManager *m_allNovelManager;
};

#endif // NOVELRECOMMENDER_H
