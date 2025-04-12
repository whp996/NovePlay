#ifndef MYCANVAS_H
#define MYCANVAS_H

#include <QWidget>
#include <QGridLayout>
#include "slidepage.h"
#include <QTextBrowser>
#include "WebCrawler.h"
#include "socketlearn.h"
#include "links.h"
#include <QMessageBox>
#include "player.h"
#include "NovelRecommender.h"
#include "FavoriteManager.h"
#include "AllNovelManager.h"
#include <QMutex>

#if (QT_VERSION > QT_VERSION_CHECK(6,3,0))
#include <QFileDialog>
#endif


class MyCanvas : public QWidget
{
    Q_OBJECT
private:
    struct onlineUser{
        MessageDisplay *messageDisplay;
        selectionItem *selectItem;
    };
    QString canvasName;
    QString canvasDescription = "";

    WebCrawler *crawler;
    socketlearn *socket;
    SlidePage *settings;
    SlidePage *layerPage;
    singleSelectGroup *userList;
    QVBoxLayout *defTextLayout;
    QMutex mutex;

    //For display
    TransparentNavTextBrowser *view;
    QHBoxLayout *mainLayout;
    QWidget *infoWidget;
    QLabel *pageName;

    int structure_type;
    int type;
    bool generateForest = false;
    unordered_map<QString, onlineUser> userMap;
    MessageDisplay *currentMessageDisplay;

    // Novel manager
    FavoriteManager *favManager;
    AllNovelManager *allNovelManager;
    NovelRecommender *novelRecommender;

    //For player
    Player *player;
    void CreateSettings(int r);
    void CreateLayerPage(int r);
    void Init();
    void SaveToFile(const QString &path);
    void handleReadyRead(QString message);

public:
    explicit MyCanvas(int radius, QString name = "", socketlearn *socket = nullptr, QWidget *parent = nullptr);
    ~MyCanvas(){ if(player) player->stopNovelReading();}
    QString name(){return canvasName;}
    QString description(){return canvasDescription;}
    SlidePage *settingPage(){return settings;}
    SlidePage *CavlayerPage(){return layerPage;}

    QTimer *socketTimerout;
public slots:
    void on_receiveMessage(const QString &message, const MessageType &type);
signals:
    void nameChanged(QString name);
    void descChanged(QString desc);
    void setDel(MyCanvas* target);
    void receiveMessage(const QString &message, const MessageType &type);
};

#endif // MYCANVAS_H
