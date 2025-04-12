#include "NovelRecommender.h"
#include <QHash>
#include <QPair>
#include <QDebug>
#include <algorithm>

NovelRecommender::NovelRecommender(QObject *parent)
    : QObject(parent)
{
    m_favManager = FavoriteManager::instance();
    m_allNovelManager = AllNovelManager::instance();
}

NovelRecommender::~NovelRecommender()
{
}

QVector<NovelItem> NovelRecommender::recommendNovels(int count) const {
    QVector<NovelItem> favItems = m_favManager->favorites();
    QHash<QString, int> typeFrequency;
    for (const NovelItem &item : favItems) {
        typeFrequency[item.type] += 1;
    }

    QVector<NovelItem> allNovels = m_allNovelManager->novels();

    QSet<QString> favNames;
    for (const NovelItem &item : favItems) {
        favNames.insert(item.name);
    }
    
    QVector<QPair<NovelItem, int>> candidateScores;
    for (const NovelItem &novel : allNovels) {
        if (favNames.contains(novel.name))
            continue;

        int score = typeFrequency.value(novel.type, 0);
        candidateScores.append(qMakePair(novel, score));
    }
    
    std::sort(candidateScores.begin(), candidateScores.end(), [](const QPair<NovelItem, int> &a,
                                                                   const QPair<NovelItem, int> &b) {
        return a.second > b.second;
    });
    
    QVector<NovelItem> recommended;
    for (int i = 0; i < candidateScores.size() && recommended.size() < static_cast<uint>(count); ++i) {
        recommended.append(candidateScores[i].first);
    }
    
    if (recommended.size() < static_cast<uint>(count)) {
        for (const NovelItem &novel : allNovels) {
            if (favNames.contains(novel.name))
                continue;
            bool alreadyIncluded = false;
            for (const NovelItem &rec : recommended) {
                if (rec.name == novel.name) {
                    alreadyIncluded = true;
                    break;
                }
            }
            if (!alreadyIncluded) {
                recommended.append(novel);
                if (recommended.size() >= static_cast<uint>(count))
                    break;
            }
        }
    }
    
    return recommended;
}
