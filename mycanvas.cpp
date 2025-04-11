#include "mycanvas.h"

MyCanvas::MyCanvas(int radius, QString name, socketlearn* socket, QWidget *parent) :
    QWidget(parent),
    canvasName(name),
    socket(socket)
{
    /* create canvas */
    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    this->setLayout(mainLayout);
    view = new TransparentNavTextBrowser(this);
    view->setReadOnly(true);
    player = Player::getInstance(nullptr);
    mainLayout->addWidget(view);
    this->setFocusPolicy(Qt::ClickFocus);

    crawler = new WebCrawler(this);
    crawler->setOutputWidget(view);
    // 加载本地bqg.html文件
    QFile localFile("bqg.html");
    if (localFile.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(localFile.readAll());
        view->setHtml(content);
        localFile.close();
    } else {
        qDebug() << "无法打开本地文件 bqg.html";
    }
    // crawler->fetchArticle(QUrl("https://www.biq01.cc/index/126865/1.html"));

    CreateSettings(radius);
    CreateLayerPage(radius);

    // 在构造函数中，关闭默认打开链接，然后连接信号
    view->setOpenLinks(false);
    connect(view, &QTextBrowser::anchorClicked, this, [=](const QUrl &url) mutable {
        qDebug() << "anchorClicked" << url;
        QString urlnew = url.toString() + "1.html";
        crawler->fetchArticle(QUrl(urlnew));
        qDebug() << "anchorClicked" << urlnew;
    });

    socketTimerout = new QTimer(this);
    connect(socketTimerout, &QTimer::timeout, this, [=](){
        socketTimerout->stop();
        QMessageBox::warning(this, "错误", "连接超时,请重新登录");
        emit setDel(this);
    });
}

void MyCanvas::CreateSettings(int radius){
    /* create settings page */
    settings = new SlidePage(radius, "SETTINGS", this->parentWidget());
    singleSelectGroup *structureSetting = new singleSelectGroup("小说源", this);
    connect(structureSetting, &singleSelectGroup::itemChange, this, [=](){settings->UpdateContents();});
    selectionItem *bqg = new selectionItem("笔趣阁", "笔趣阁", this);
    structureSetting->AddItem(bqg);
    structureSetting->SetSelection(bqg);
    connect(structureSetting, &singleSelectGroup::selectedItemChange, this, [=](int id){
        if(id == 0){
            QFile localFile("bqg.html");
            if (localFile.open(QIODevice::ReadOnly)) {
                QString content = QString::fromUtf8(localFile.readAll());
                view->setHtml(content);
                localFile.close();
            } else {
                qDebug() << "无法打开本地文件 bqg.html";
            }
        }
    });
    singleSelectGroup *dirSetting = new singleSelectGroup("推荐", this);
    connect(dirSetting, &singleSelectGroup::itemChange, this, [=](){settings->UpdateContents();});
    selectionItem *setDG = new selectionItem("DG", "Directed graph", this);
    selectionItem *setUDG = new selectionItem("UDG", "Undirected graph", this);
    dirSetting->AddItem(setDG);
    dirSetting->AddItem(setUDG);
    dirSetting->SetSelection(type == DG ? setDG : setUDG);
    connect(dirSetting, &singleSelectGroup::selectedItemChange, this, [=](int id){
        // g->ConvertType(id == 0 ? AbstractGraph::DG : AbstractGraph::UDG);
        // view->setType(id == 0 ? MyGraphicsView::DG : MyGraphicsView::UDG);
        // type = id == 0 ? DG : UDG;
    });

    singleSelectGroup *collectSetting = new singleSelectGroup("收藏列表", this);
    connect(collectSetting, &singleSelectGroup::itemChange, this, [=](){settings->UpdateContents();});
    QFile file(QDir::currentPath() + "/Favorite.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        QJsonArray favorites = obj["favorite"].toArray();
        
        for (const QJsonValue &value : favorites) {
            QJsonObject book = value.toObject();
            QString name = book["name"].toString();
            QString desc = book["description"].toString();
            QString url = book["url"].toString();
            
            selectionItem *item = new selectionItem(name, desc, this);
            collectSetting->AddItem(item);
            connect(item, &selectionItem::selected, this, [=](){
                crawler->fetchArticle(QUrl(url));
            });
        }
    }
    file.close();
    connect(collectSetting, &singleSelectGroup::selectedItemChange, this, [=](int id){
        // g->ConvertType(id == 0 ? AbstractGraph::DG : AbstractGraph::UDG);
        // view->setType(id == 0 ? MyGraphicsView::DG : MyGraphicsView::UDG);
        // type = id == 0 ? DG : UDG;
    });

    singleSelectGroup *dfsSetting = new singleSelectGroup("音色", this);
    connect(dfsSetting, &singleSelectGroup::itemChange, this, [=](){settings->UpdateContents();});
    for(const auto &voice : player->getVoices()){
        QString desc = QString("年龄：%1 性别：%2 语言：%3").arg(voice.age()).arg(voice.gender()).arg(voice.language());
        selectionItem *item = new selectionItem(voice.name(), desc, this);
        dfsSetting->AddItem(item);
        connect(item, &selectionItem::selected, this, [=](selectionItem *item){
            player->setVoice(voice);
        });
    }
    QWidget *whiteSpace = new QWidget(this);
    whiteSpace->setFixedHeight(30);
    horizontalValueAdjuster *aniSpeed = new horizontalValueAdjuster("播放速度", -1, 1, 0.1, this);
    aniSpeed->setValue(0);
    connect(aniSpeed, &horizontalValueAdjuster::valueChanged, view, [=](qreal value){
        player->Setrate(value);
    });
    textInputItem *rename = new textInputItem("Name", this);
    rename->setValue(canvasName);
    connect(rename, &textInputItem::textEdited, this, [=](QString text){canvasName = text; emit nameChanged(text);});
    textInputItem *redesc = new textInputItem("Detail", this);
    redesc->setValue(canvasDescription);
    connect(redesc, &textInputItem::textEdited, this, [=](QString text){canvasDescription = text; emit descChanged(text);});
    textButton *showBtn = new textButton("暂停", this);
    connect(showBtn, &textButton::myClicked, this, [=](){
        if(showBtn->value() == "暂停") {player->stopNovelReading(); showBtn->setValue("继续");}
        else {player->ContinueNovelReading(); showBtn->setValue("暂停");}
    });
    textButton *hideBtn = new textButton("播放", this);
    connect(hideBtn, &textButton::myClicked, this, [=](){
        player->startNovelReading(view->toPlainText().toUtf8());
        showBtn->setValue("暂停");
    });
    QWidget *whiteSpace2 = new QWidget(this);
    whiteSpace2->setFixedHeight(30);
    textButton *saveBtn = new textButton("收藏", this);
    connect(saveBtn, &textButton::myClicked, this, [=](){
        QString savePath = QFileDialog::getSaveFileName(this, tr("Save map"), " ", tr("Map file(*.map)"));
        if(!savePath.isEmpty())
            SaveToFile(savePath);
    });
    textButton *delBtn = new textButton("取消收藏", "#0acb1b45","#1acb1b45","#2acb1b45",this);
    connect(delBtn, &textButton::myClicked, this, [=](){emit setDel(this);});
    settings->AddContent(delBtn);
    settings->AddContent(saveBtn);
    settings->AddContent(whiteSpace2);
    settings->AddContent(showBtn);
    settings->AddContent(hideBtn);
    settings->AddContent(dfsSetting);
    settings->AddContent(dirSetting);
    settings->AddContent(collectSetting);
    settings->AddContent(structureSetting);
    settings->AddContent(aniSpeed);
    settings->AddContent(whiteSpace);
    settings->AddContent(redesc);
    settings->AddContent(rename);
    settings->show();

}

void MyCanvas::CreateLayerPage(int r){
    layerPage = new SlidePage(r, "聊天管理", this->parentWidget());
    layerPage->stackUnder(settings);
    textButton *logoutBtn = new textButton("退出登录", this);
    connect(logoutBtn, &textButton::myClicked, this, [=](){
        uint32_t instruction = static_cast<uint32_t>(InstructionType::logout);
        vector<char> data = socket->GenerateMessage(MessageType::instruction, instruction);
        socket->SendData(data);
        emit setDel(this);
    });
    textButton *deleteBtn = new textButton("删除账号", "#0acb1b45","#1acb1b45","#2acb1b45", this);
    connect(deleteBtn, &textButton::myClicked, this, [=](){
        uint32_t instruction = static_cast<uint32_t>(InstructionType::delete_self);
        vector<char> data = socket->GenerateMessage(MessageType::instruction, instruction);
        socket->SendData(data);
        socketTimerout->start(1000);
        emit setDel(this);
    });
    userList = new singleSelectGroup("在线用户", this);
    QWidget *whiteSpace = new QWidget(this);
    whiteSpace->setFixedHeight(30);
    connect(userList, &singleSelectGroup::itemChange, this, [=](){layerPage->UpdateContents();});
    layerPage->AddContent(deleteBtn);
    layerPage->AddContent(whiteSpace);
    layerPage->AddContent(logoutBtn);
    layerPage->AddContent(userList);
    layerPage->show();

    QTimer *delay = new QTimer(this);
    connect(delay, &QTimer::timeout, this, [=](){Init();});
    delay->setSingleShot(true);
    delay->start(10);
}

void MyCanvas::Init(){
    /* Create info widget */
    infoWidget = new QWidget(this);
    mainLayout->addWidget(infoWidget);
    mainLayout->setStretch(0, 7);
    mainLayout->setStretch(1, 3);
    infoWidget->setMinimumWidth(250);
    infoWidget->setMaximumWidth(500);

    //Set basic layout
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
    infoWidget->setLayout(infoLayout);
    infoLayout->setContentsMargins(10, 0, 0, 0);
    infoLayout->setAlignment(Qt::AlignTop);

    QFont titleFont = QFont("Corbel", 20);

    QWidget *upper = new QWidget(infoWidget);
    QVBoxLayout *upperLayout = new QVBoxLayout(upper);
    upper->setLayout(upperLayout);
    upperLayout->setContentsMargins(0, 0, 0, 0);
    upper->setContentsMargins(0, 0, 0, 0);
    pageName = new QLabel(infoWidget);
    pageName->setText("消息");
    pageName->setFont(titleFont);
    pageName->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    pageName->setStyleSheet("color:#2c2c2c");
    QWidget *upperSplitter = new QWidget(upper);
    upperSplitter->setFixedSize(30, 6);
    upperSplitter->setStyleSheet("background-color:#3c3c3c;border-radius:3px;");
    upperLayout->addWidget(pageName);
    upperLayout->addWidget(upperSplitter);

    QWidget *lower = new QWidget(infoWidget);
    QVBoxLayout *lowerLayout = new QVBoxLayout(lower);
    lower->setLayout(lowerLayout);
    lowerLayout->setContentsMargins(0, 0, 0, 0);
    QWidget *lowerSplitter = new QWidget(lower);
    lowerSplitter->setFixedSize(30, 6);
    lowerSplitter->setStyleSheet("background-color:#3c3c3c;border-radius:3px;");

    lowerLayout->addWidget(lowerSplitter);
    infoLayout->addWidget(upper);
    infoLayout->addWidget(lower);

    //Add specific items and connections
    //Default page
    QWidget *defInfoPage = new QWidget(infoWidget);
    QVBoxLayout *defInfoLayout = new QVBoxLayout(defInfoPage);
    defInfoPage->setLayout(defInfoLayout);
    defInfoLayout->setContentsMargins(0, 0, 0, 0);
    defInfoLayout->setAlignment(Qt::AlignTop);
    QWidget *defTextItems = new QWidget(defInfoPage);
    defTextItems->setObjectName("DefTextItems");
    defTextItems->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    defTextItems->setStyleSheet("QWidget#DefTextItems{border:1px solid #cfcfcf;border-radius:5px;}");
    defTextLayout = new QVBoxLayout(defTextItems);
    defTextItems->setLayout(defTextLayout);
    defTextLayout->setContentsMargins(0, 5, 0, 5);
    MessageDisplay *messageDisplay = new MessageDisplay(defInfoPage);
    messageDisplay->addMessage(new MessageItem("系统", "你好，我是小明，很高兴认识你！", "2024-04-06 10:00:00", MessageItem::Left));
    messageDisplay->addMessage(new MessageItem("我", "你好，小明，很高兴认识你！", "2024-04-06 10:00:01", MessageItem::Right));
    currentMessageDisplay = messageDisplay;
    defTextLayout->addWidget(messageDisplay);
    defInfoLayout->addWidget(defTextItems);
    upperLayout->addWidget(defInfoPage);
    defInfoPage->show();

    QWidget *inputPage = new QWidget(lower);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputPage);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setAlignment(Qt::AlignTop);
    inputPage->setLayout(inputLayout);
    textInputItem *textInput = new textInputItem("我：", inputPage);
    textInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    textButton *sendBtn = new textButton("发送", inputPage);
    sendBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sendBtn->setFixedWidth(100);
    connect(sendBtn, &textButton::myClicked, this, [=](){
        QString text = textInput->value();
        if(!text.isEmpty()){
            Message msg;
            msg.type = fileType::Text;
            msg.data = text.toUtf8();
            msg.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QByteArray msgdata = Message::serialize(msg);
            if(g_links.ConnectName.isEmpty()){
                QMessageBox::warning(this, "错误", "请先选择一个用户");
                return;
            }
            std::string sedmsg = g_links.ConnectName.toStdString() + "|" + msgdata.toStdString();
            vector<char> data = socket->GenerateMessage(MessageType::forward_msg, sedmsg);
            socket->SendData(data);
            socketTimerout->start(1000);
            currentMessageDisplay->addMessage(new MessageItem("我", text, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), MessageItem::Right));
            textInput->setValue("");
        }
    });
    inputLayout->addWidget(textInput);
    inputLayout->addWidget(sendBtn);
    lowerLayout->addWidget(inputPage);
}

void MyCanvas::SaveToFile(const QString &path){
    QFile output(path);
    output.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream ts(&output);
    ts << "VFdGeWFXUnZaekl3TURJd05ESTE=\n";
    ts << canvasName << "\n";
    ts << canvasDescription << "\n";
    ts << structure_type << " " << type << "\n";
    // view->SaveToFile(ts);
    output.close();
}

void MyCanvas::on_receiveMessage(const QString &message, const MessageType &type){
    socketTimerout->stop();
    if(type == MessageType::return_msg){
        if(message.contains("success")){
            qDebug() << message;
        }else{
            int pos = message.indexOf(":");
            if(pos != -1 && message.left(pos) == "all_online_users"){
                QString users = message.right(message.length() - pos - 1);
                QStringList list = users.split("|", Qt::SkipEmptyParts);
                for(const QString &user : list){
                    if(userMap.find(user) != userMap.end()) continue;
                    selectionItem *item = new selectionItem(user, "", this);
                    userList->AddItem(item);
                    connect(item, &selectionItem::selected, this, [=](selectionItem *item){
                        g_links.ConnectName = item->name();
                        if(userMap.find(item->name()) != userMap.end()){
                            currentMessageDisplay->hide();
                            currentMessageDisplay = userMap[item->name()].messageDisplay;
                            currentMessageDisplay->show();
                        }else{
                            currentMessageDisplay->hide();
                            currentMessageDisplay = new MessageDisplay(this);
                            userMap[item->name()] = {currentMessageDisplay, item};
                            currentMessageDisplay->show();
                            defTextLayout->addWidget(currentMessageDisplay);
                        }
                    });
                    userList->SetSelection(item);
                }
            }else if(pos != -1 && message.left(pos) == "user_online")
            {
                QString user = message.right(message.length() - pos - 1);
                if(userMap.find(user) != userMap.end()) return;
                selectionItem *item = new selectionItem(user, "", this);
                userList->AddItem(item);
                connect(item, &selectionItem::selected, this, [=](selectionItem *item){
                    g_links.ConnectName = item->name();
                    if(userMap.find(item->name()) != userMap.end()){
                        currentMessageDisplay->hide();
                        currentMessageDisplay = userMap[item->name()].messageDisplay;
                        currentMessageDisplay->show();
                    }else{
                        currentMessageDisplay->hide();
                        currentMessageDisplay = new MessageDisplay(this);
                        userMap[item->name()] = {currentMessageDisplay, item};
                        currentMessageDisplay->show();
                        defTextLayout->addWidget(currentMessageDisplay);
                    }
                });
            }else if(pos != -1 && message.left(pos) == "user_offline")
            {
                QString user = message.right(message.length() - pos - 1);
                qDebug() << "user_offline" << user;
                if(userMap.find(user) != userMap.end()){
                    userList->RemoveItem(userMap[user].selectItem);
                    userMap[user].messageDisplay->hide();
                    userMap.erase(user);
                }
            }
        }
    }else if(type == MessageType::forward_msg){
        handleReadyRead(message);
    }else if(type == MessageType::error){
        if(message == "disconnect"){
            delete socket;
            socket = nullptr;
            emit setDel(this);
        }
        QMessageBox::warning(this, "错误", message);
    }
}

void MyCanvas::handleReadyRead(QString message){
    QStringList parts = message.split("|");
    if(parts.size() < 2) return;
    QString from = parts.at(0);
    QString data = parts.at(1);
    QByteArray messageData = data.toUtf8();
    Message msg = Message::deserialize(messageData);
    if(msg.type == fileType::Text){
        MessageDisplay *messageDisplay;
        if(userMap.find(from) == userMap.end()){
            messageDisplay = new MessageDisplay(this);
            selectionItem *item = new selectionItem(from, "", this);
            userList->AddItem(item);
            connect(item, &selectionItem::selected, this, [=](selectionItem *item){
                g_links.ConnectName = item->name();
                if(userMap.find(item->name()) != userMap.end()){
                    currentMessageDisplay->hide();
                    currentMessageDisplay = userMap[item->name()].messageDisplay;
                    currentMessageDisplay->show();
                }
            });
            userMap[from] = {messageDisplay, item};
            messageDisplay->hide();
            defTextLayout->addWidget(messageDisplay);
        }else{
            messageDisplay = userMap[from].messageDisplay;
        }
        messageDisplay->addMessage(new MessageItem(from, QString::fromUtf8(msg.data.data()), msg.timestamp, MessageItem::Left));
    }
}
