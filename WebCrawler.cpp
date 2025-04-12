#include "WebCrawler.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QDebug>
#include <QScrollBar>
#include <QRegularExpression>

// 修改辅助函数：提取小说章节内容，仅保留<div id="chaptercontent">...</div>内的部分
static QString extractNovelContent(const QString &html) {
    // 正则表达式匹配<div id="chaptercontent" ...>与</div>之间的内容
    QRegularExpression re("<div\\s+id=[\"']chaptercontent[\"'][^>]*>(.*?)</div>",
                            QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(html);
    if (match.hasMatch()) {
        QString content = match.captured(1);
        int pos = content.indexOf("请收藏本站");
        if (pos != -1) {
            return content.left(pos);
        }
    }
    return html;
}

WebCrawler::WebCrawler(QObject *parent)
    : QObject(parent),
      manager(new QNetworkAccessManager(this)),
      outputWidget(nullptr),
      currentUrl(),
      maxRetries(3),      // 最大重试 3 次
      currentRetry(0)
{
}

void WebCrawler::setOutputWidget(QTextBrowser *output)
{
    outputWidget = output;
}

void WebCrawler::fetchArticle(const QUrl &url, bool returnoriginal)
{
    currentUrl = url;
    currentRetry = 0;
    this->returnoriginal = returnoriginal;
    fetchArticleInternal();
}

void WebCrawler::fetchArticleInternal()
{
    QNetworkRequest request(currentUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Qt WebCrawler)");
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &WebCrawler::onFinished);
}

void WebCrawler::onFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        // 出错时，检查是否还可以重试
        if (currentRetry < maxRetries) {
            currentRetry++;
            // 延时 2000ms 后重试发起请求
            QTimer::singleShot(2000, this, [this](){
                fetchArticleInternal();
            });
        }
        reply->deleteLater();
        return;
    }

    // 成功获取后重置重试计数
    currentRetry = 0;
    QByteArray data = reply->readAll();
    // 网页使用 UTF-8 编码
    QString article = QString::fromUtf8(data);
    // 使用辅助函数提取<div id="chaptercontent">内的内容
    QString novelContent;
    if(returnoriginal){
        novelContent = article;
    }else{
        novelContent = extractNovelContent(article);
    }

    if(novelContent.isEmpty()) return;
    // 发出信号返回爬取内容
    emit articleFetched(novelContent);
    QString inforurl = currentUrl.toString().section('/', 0, -2) + "/";
    fetchInfo(QUrl(inforurl));
    if (outputWidget) {
        outputWidget->setHtml(novelContent);
        // 确保滚动条在最上面
        QTimer::singleShot(0, outputWidget, [this](){
            if (outputWidget->verticalScrollBar())
                outputWidget->verticalScrollBar()->setValue(0);
        });
    }

    reply->deleteLater();
} 

void WebCrawler::fetchInfo(const QUrl &url){
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Qt WebCrawler)");
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &WebCrawler::oninfoFinished);
}

void WebCrawler::oninfoFinished(){
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    QByteArray data = reply->readAll();
    QString html = QString::fromUtf8(data);
    
    // --------------------------
    // 提取书名（书名在 <h1> 标签中）
    QRegularExpression reTitle("<h1[^>]*>([^<]+)</h1>");
    QRegularExpressionMatch match = reTitle.match(html);
    QString bookTitle;
    if (match.hasMatch()) {
        bookTitle = match.captured(1).trimmed();
    } else {
        qDebug() << "未匹配到书名";
    }
    
    // --------------------------
    // 提取作者（格式为 "作者：<作者名>"）
    QRegularExpression reAuthor("作者：\\s*([^<\\s]+(?:\\s+[^<\\s]+)*)");
    match = reAuthor.match(html);
    QString bookAuthor;
    if (match.hasMatch()) {
        bookAuthor = match.captured(1).trimmed();
    } else {
        qDebug() << "未匹配到作者";
    }
    
    // --------------------------
    // 提取简介（简介紧跟在 "内容简介：" 后，直到下一个 HTML 标签）
    QRegularExpression reSynopsis("内容简介：\\s*([\\s\\S]+?)(?:<\\/div>|<br>|<p>)");
    match = reSynopsis.match(html);
    QString bookSynopsis;
    if (match.hasMatch()) {
        bookSynopsis = match.captured(1).trimmed().replace("</dt><dd>", "").replace("</dd></dl>", "");
    } else {
        qDebug() << "未匹配到简介";
    }
    
    // --------------------------
    // 提取分类：根据页面结构在<div class="path wap_none">中提取，
    QRegularExpression reCategory("笔趣阁.*?&gt;\\s*([^&]+?)\\s*&gt;");
    match = reCategory.match(html);
    QString bookCategory;
    if (match.hasMatch()) {
        bookCategory = match.captured(1).trimmed();
    } else {
        qDebug() << "未匹配到分类";
    }

    NovelItem item;
    item.name = bookTitle;
    item.author = bookAuthor;
    item.type = bookCategory;
    item.description = bookSynopsis;
    item.url = currentUrl.toString();
    if(!AllNovelManager::instance()->addNovel(item)){
        qWarning() << "添加小说失败:" << AllNovelManager::instance()->getLastError();
    }
    g_links.SetCurrentLink(item);
    reply->deleteLater();
}


