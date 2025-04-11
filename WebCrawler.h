#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QTextBrowser>
#include "links.h"

class QNetworkAccessManager;
class QNetworkReply;

class WebCrawler : public QObject
{
    Q_OBJECT
public:
    explicit WebCrawler(QObject *parent = nullptr);

    // 设置用于展示爬取结果的 QTextBrowser 控件。
    void setOutputWidget(QTextBrowser *output);

    // 发起爬虫请求，输入链接（URL）
    void fetchArticle(const QUrl &url, bool returnoriginal = false);

    void fetchInfo(const QUrl &url);

signals:
    // 当文章爬取完成后发出此信号，返回爬取到的文章内容
    void articleFetched(const QString &article);

private slots:
    // 处理网络请求完成
    void onFinished();

    void oninfoFinished();

private:
    QNetworkAccessManager *manager;
    QTextBrowser *outputWidget; // 用于展示文章内容
    QUrl currentUrl;           // 当前请求的 URL
    int maxRetries;            // 最大重试次数
    int currentRetry;          // 当前重试计数

    bool returnoriginal = false;

    // 新增私有成员函数，用于实际发起请求
    void fetchArticleInternal();
};

#endif // WEBCRAWLER_H 