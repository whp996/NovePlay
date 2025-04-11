#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainterPath>
#include <QRegion>
#include <QTimer>
#include <QMessageBox>

#if (QT_VERSION > QT_VERSION_CHECK(6,3,0))
#include <QFileDialog>
#endif


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->centralwidget->setMouseTracking(true);
#ifdef Q_OS_LINUX
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    cornerRadius = 0;
#endif
    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [=](){Init();});
    t->setSingleShot(true);
    t->start(10);

    connect(ui->adjSizeBtn, &QPushButton::clicked, this, [=](){controlWindowScale();});
    timeout = new QTimer(this);
    connect(timeout, &QTimer::timeout, this, [=](){
        if(socket != nullptr){
            delete socket;
            socket = nullptr;
        }
        timeout->stop();
        QMessageBox::warning(this, "错误", "连接超时");
    });
}

void MainWindow::Init(){
    /* Create main widget and set mask, style sheet and shadow */
#ifdef Q_OS_LINUX
    QPainterPath path;
    path.addRect(ui->mainWidget->rect());
#else
    QPainterPath path;
    path.addRoundedRect(ui->mainWidget->rect(), cornerRadius - 1, cornerRadius - 1);
#endif
    QRegion mask(path.toFillPolygon().toPolygon());
    ui->mainWidget->setMask(mask);

    QString mainStyle;
    ui->mainWidget->setObjectName("mainWidget");
    mainStyle = "QWidget#mainWidget{background-color:" + mainBackGround.name() + QString::asprintf(";border-radius:%dpx", cornerRadius) + "}";
    ui->mainWidget->setStyleSheet(mainStyle);
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
#ifdef Q_OS_WINDOWS
    windowShadow = new QGraphicsDropShadowEffect(this);
    windowShadow->setBlurRadius(30);
    windowShadow->setColor(QColor(0, 0, 0));
    windowShadow->setOffset(0, 0);
    ui->mainWidget->setGraphicsEffect(windowShadow);
#endif
#endif
    /**********************************************************/

    /* Create border in order to cover the zigzag edge of the region */
#ifdef Q_OS_WINDOWS
    border = new QWidget(this);
    border->move(ui->mainWidget->pos() - QPoint(1, 1));
    border->resize(ui->mainWidget->size() + QSize(2, 2));
    QString borderStyle;
    borderStyle = "background-color:#00FFFFFF;border:1.5px solid #686868; border-radius:" + QString::asprintf("%d",cornerRadius) + "px";
    border->setStyleSheet(borderStyle);
    border->setAttribute(Qt::WA_TransparentForMouseEvents);
    border->show();
#endif
    /*****************************************************************/

    /* Create about page */
    defaultSettingsPage = new SlidePage(cornerRadius, "ABOUT", ui->mainWidget);
    textInputItem *version = new textInputItem("version", defaultSettingsPage);
    version->setValue("1.3-beta");
    version->setEnabled(false);
    textInputItem *updateDate = new textInputItem("last-upd", defaultSettingsPage);
    updateDate->setValue("2021/12/6 10:14");
    updateDate->setEnabled(false);
    textInputItem *Author = new textInputItem("author", defaultSettingsPage);
    Author->setValue("Linloir | Made with love");
    Author->setEnabled(false);
    textInputItem *lic = new textInputItem("lic", defaultSettingsPage);
    lic->setValue("MIT License");
    lic->setEnabled(false);
    textInputItem *GitHub = new textInputItem("git", defaultSettingsPage);
    GitHub->setValue("github.com/Linloir");
    GitHub->setEnabled(false);
    defaultSettingsPage->AddContent(GitHub);
    defaultSettingsPage->AddContent(lic);
    defaultSettingsPage->AddContent(Author);
    defaultSettingsPage->AddContent(updateDate);
    defaultSettingsPage->AddContent(version);
    curSettingsPage = defaultSettingsPage;
    defaultSettingsPage->show();
    pageList.push_back(defaultSettingsPage);

    /************************/

    /* Initialize display area */
    QFont titleFont = QFont("Corbel Light", 24);
    QFontMetrics titleFm(titleFont);
    canvasTitle = new QLineEdit(this);
    canvasTitle->setFont(titleFont);
    canvasTitle->setText("START");
    canvasTitle->setMaxLength(20);
    canvasTitle->setReadOnly(true);
    canvasTitle->setMinimumHeight(titleFm.height());
    canvasTitle->setMaximumWidth(titleFm.size(Qt::TextSingleLine, "START").width() + 10);
    canvasTitle->setStyleSheet("background-color:#00000000;border-style:none;border-width:0px;margin-left:1px;");
    connect(canvasTitle, &QLineEdit::textEdited, canvasTitle, [=](QString text){canvasTitle->setMaximumWidth(titleFm.size(Qt::TextSingleLine, text).width());});

    QFont descFont = QFont("Corbel Light", 12);
    QFontMetrics descFm(descFont);
    canvasDesc = new QLineEdit(this);
    canvasDesc->setFont(descFont);
    canvasDesc->setText("Begin your encrypted call");
    canvasDesc->setMaxLength(128);
    canvasDesc->setReadOnly(true);
    canvasDesc->setMinimumHeight(descFm.lineSpacing());
    canvasDesc->setStyleSheet("background-color:#00000000;border-style:none;border-width:0px;");

    settingsIcon = new customIcon(":/icons/icons/settings.svg", "settings", 5, this);
    settingsIcon->setMinimumHeight(canvasTitle->height() * 0.7);
    settingsIcon->setMaximumWidth(canvasTitle->height() * 0.7);
    connect(settingsIcon, &customIcon::clicked, this, [=](){
        QPropertyAnimation *rotate = new QPropertyAnimation(settingsIcon, "rotationAngle", this);
        rotate->setDuration(750);
        rotate->setStartValue(0);
        rotate->setEndValue(90);
        rotate->setEasingCurve(QEasingCurve::InOutExpo);
        rotate->start();
        curSettingsPage->slideIn();
    });
    layersIcon = new customIcon(":/icons/icons/layers.svg", "layers", 5, this);
    layersIcon->setMinimumHeight(canvasTitle->height() * 0.7);
    layersIcon->setMaximumWidth(canvasTitle->height() * 0.7);
    connect(layersIcon, &customIcon::clicked, [this](){
        curLayersPage->slideIn();
    });

    /* create title */

    QWidget *titleInnerWidget = new QWidget(this);
    titleInnerWidget->setFixedHeight(canvasTitle->height());
    QHBoxLayout *innerLayout = new QHBoxLayout(titleInnerWidget);
    titleInnerWidget->setLayout(innerLayout);
    innerLayout->setContentsMargins(0, 0, 0, 0);
    innerLayout->setSpacing(10);
    innerLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    innerLayout->addWidget(canvasTitle);
    innerLayout->addWidget(settingsIcon);
    innerLayout->addWidget(layersIcon);

    QWidget *titleWidget = new QWidget(this);
    titleWidget->setMaximumHeight(canvasTitle->height() + canvasDesc->height());
    QVBoxLayout *outerLayout = new QVBoxLayout(titleWidget);
    titleWidget->setLayout(outerLayout);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    outerLayout->addWidget(titleInnerWidget);
    outerLayout->addWidget(canvasDesc);

    /* create default page */

    defaultPage = new QWidget(ui->mainWidget);
    defaultPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    bigIconButton *login = new bigIconButton(":/icons/icons/create.png", "登录", 10, this);
    login->setScale(0.9);
    bigIconButton *regbigbt = new bigIconButton(":/icons/icons/open.png", "注册", 10, this);
    regbigbt->setScale(0.9);
    connect(regbigbt, &bigIconButton::clicked, this, [=](){
        registerPage->slideIn();
    });
    QHBoxLayout *defaultPageLayout = new QHBoxLayout(defaultPage);
    defaultPage->setLayout(defaultPageLayout);
    defaultPageLayout->setContentsMargins(50, 30, 50, 80);
    defaultPageLayout->setSpacing(20);
    defaultPageLayout->addWidget(login);
    defaultPageLayout->addWidget(regbigbt);

    /* create layers page */
    //for login page

    defaultLayersPage = new SlidePage(cornerRadius, "聊天管理", ui->mainWidget);
    defaultLayersPage->stackUnder(loginPage);
    defaultLayersPage->stackUnder(registerPage);
    textButton *registerbtn = new textButton("注册", defaultLayersPage);
    textButton *loginBtn = new textButton("登录", defaultLayersPage);
    defaultLayersPage->AddContent(loginBtn);
    defaultLayersPage->AddContent(registerbtn);
    connect(loginBtn, &textButton::myClicked, this, [=](){ loginPage->slideIn();});
    connect(registerbtn, &textButton::myClicked, this, [=](){ registerPage->slideIn();});
    defaultLayersPage->show();
    pageList.push_back(defaultLayersPage);
    curLayersPage = defaultLayersPage;

    /* create add new slide page */
    loginPage = new SlidePage(cornerRadius, "用户登录", ui->mainWidget);
    registerPage = new SlidePage(cornerRadius, "用户注册", ui->mainWidget);
    QLineEdit *canvasName = new QLineEdit(this);
    QLineEdit *username = new QLineEdit(this);
    canvasName->setMaximumHeight(20);
    username->setMaximumHeight(20);
    QLineEdit *canvasDesc = new QLineEdit(this);
    QLineEdit *password = new QLineEdit(this);
    canvasDesc->setMaximumHeight(20);
    password->setMaximumHeight(20);

    QWidget *whiteSpace = new QWidget(loginPage);
    QWidget *whiteSpace2 = new QWidget(registerPage);
    whiteSpace->setFixedHeight(30);
    whiteSpace2->setFixedHeight(30);
    textInputItem *usernameSel = new textInputItem("用户名:", loginPage);
    textInputItem *registernameSel = new textInputItem("用户名:", registerPage);
    usernameSel->setValue("test");
    registernameSel->setValue("test");
    textInputItem *passwordSel = new textInputItem("密码:", loginPage);
    textInputItem *registerpasswordSel = new textInputItem("密码:", registerPage);
    passwordSel->setValue("123456");
    registerpasswordSel->setValue("123456");
    textButton *submit = new textButton("登录!", loginPage);
    textButton *registerSubmit = new textButton("注册!", registerPage);
    connect(submit, &textButton::myClicked, this, [=](){
        submit->setEnabled(false);
        if(socket == nullptr){
            socket = new socketlearn();
        }else{
            delete socket;
        }
        try {
            socket = new socketlearn();
        } catch (const std::exception &e) {
            QMessageBox::warning(this, "错误", QString::fromStdString(e.what()));
            return;
        }
        socket->setReceiveCallback([this](const std::string &msg, MessageType type){
         emit receiveMessage(QString::fromStdString(msg), type);
        });
        connect(this, &MainWindow::receiveMessage, this, &MainWindow::on_receiveMessage);
        QString username = usernameSel->value();
        QString password = passwordSel->value();
        g_links.CurrentLogin = username;

        if(username.isEmpty() || password.isEmpty()) {
         QMessageBox::warning(this, "错误", "用户名或密码不能为空");
         return;
        }
        MessageType type = MessageType::login;
        QString loginPayload = username + "|" + password;
        vector<char> data = socket->GenerateMessage(type, loginPayload.toStdString());
        socket->SendData(data);
        if(!socket->SendData(data)) {
            QMessageBox::warning(this, "错误", QString::fromStdString(socket->GetLastError()));
            return;
        }
        socket->startListening();
        timeout->start(10000);
        QTimer *delay = new QTimer(this);
        connect(delay, &QTimer::timeout, this, [=](){
            submit->setEnabled(true);
        });
        delay->setSingleShot(true);
        delay->start(10000);
    });
    connect(registerSubmit, &textButton::myClicked, this, [=](){
        if(socket != nullptr){
            delete socket;
            socket = nullptr;
        }
        try {
            socket = new socketlearn();
        } catch (const std::exception &e) {
            QMessageBox::warning(this, "错误", QString::fromStdString(e.what()));
            return;
        }
        QString username = registernameSel->value();
        QString password = registerpasswordSel->value();
        if(username.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, "错误", "用户名或密码不能为空");
            return;
        }
        MessageType type = MessageType::register_user;
        QString registerPayload = username + "|" + password;
        vector<char> data = socket->GenerateMessage(type, registerPayload.toStdString());
        socket->SendData(data);
        if(!socket->SendData(data)) {
            QMessageBox::warning(this, "错误", QString::fromStdString(socket->GetLastError()));
            return;
        }
        socket->setReceiveCallback([this](const std::string &msg, MessageType type){
            emit receiveMessage(QString::fromStdString(msg), type);
        });
        connect(this, &MainWindow::receiveMessage, this, &MainWindow::on_receiveMessage);
        socket->startListening();
        timeout->start(20000);
    });
    loginPage->AddContent(submit);
    loginPage->AddContent(passwordSel);
    loginPage->AddContent(usernameSel);
    loginPage->AddContent(whiteSpace);
    registerPage->AddContent(registerSubmit);
    registerPage->AddContent(registerpasswordSel);
    registerPage->AddContent(registernameSel);
    registerPage->AddContent(whiteSpace2);
    connect(login, &bigIconButton::clicked, loginPage, [=](){loginPage->slideIn();});
    connect(regbigbt, &bigIconButton::clicked, registerPage, [=](){registerPage->slideIn();});
    loginPage->show();
    registerPage->show();
    pageList.push_back(loginPage);
    pageList.push_back(registerPage);

    ui->displayLayout->addWidget(titleWidget);
    ui->displayLayout->addWidget(defaultPage);
    ui->displayLayout->setAlignment(Qt::AlignTop);
}

void MainWindow::selectCanvas(MyCanvas *canvas){
    if(!curCanvas){
        ui->displayLayout->removeWidget(defaultPage);
        defaultPage->hide();
        ui->displayLayout->addWidget(canvas);
        canvas->show();
    }
    else{
        ui->displayLayout->removeWidget(curCanvas);
        curCanvas->hide();
        ui->displayLayout->addWidget(canvas);
        canvas->show();
    }
    curCanvas = canvas;
    canvas->settingPage()->setParent(ui->mainWidget);
    curSettingsPage = canvas->settingPage();
    canvasTitle->setText(curCanvas->name());
    canvasTitle->setMaximumWidth(QFontMetrics(QFont("Corbel Light", 24)).size(Qt::TextSingleLine, canvasTitle->text()).width() + 10);
    canvasDesc->setText(curCanvas->description());
}

void MainWindow::deleteCanvas(MyCanvas *canvas){
    int index = canvasList.indexOf(canvas);
    if(index < 0)
        return;
    canvasList.erase(canvasList.begin() + index);
    ui->displayLayout->removeWidget(curCanvas);
    curCanvas->hide();
    if(canvasList.size() > 0){
        selectCanvas(canvasList[0]);
    }
    else{
        ui->displayLayout->addWidget(defaultPage);
        defaultPage->show();
        curCanvas = nullptr;
        canvasTitle->setText("START");
        canvasTitle->setMaximumWidth(QFontMetrics(QFont("Corbel Light", 24)).size(Qt::TextSingleLine, "START").width() + 10);
        canvasDesc->setText("Add your first canvas to start");
        curSettingsPage = defaultSettingsPage;
        curLayersPage = defaultLayersPage;
    }
    pageList.erase(pageList.begin() + pageList.indexOf(canvas->settingPage()));
    pageList.erase(pageList.begin() + pageList.indexOf(canvas->CavlayerPage()));
    delete canvas;
    if(socket != nullptr){
        delete socket;
        socket = nullptr;
    }
    ui->mainWidget->update();
}

MainWindow::~MainWindow()
{
    if(socket != nullptr){
        delete socket;
    }
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event){
    if(event->button() == Qt::LeftButton){
        mousePressed = true;
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
        lastPos = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
#else
        lastPos = event->globalPos() - this->frameGeometry().topLeft();
#endif
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event){
    if(event->buttons() == Qt::NoButton)
        mousePressed = false;
    if(!mousePressed){
        mouseState = 0;
        if(!maximized && abs(event->pos().x() - ui->mainWidget->pos().x()) < 5)
            mouseState |= AT_LEFT;
        if(!maximized && abs(event->pos().y() - ui->mainWidget->pos().y()) < 5)
            mouseState |= AT_TOP;
        if(!maximized && abs(event->pos().x() - ui->mainWidget->pos().x() - ui->mainWidget->width()) < 5)
            mouseState |= AT_RIGHT;
        if(!maximized && abs(event->pos().y() - ui->mainWidget->pos().y() - ui->mainWidget->height()) < 5)
            mouseState |= AT_BOTTOM;
        if(mouseState == AT_TOP_LEFT  || mouseState == AT_BOTTOM_RIGHT)
            setCursor(Qt::SizeFDiagCursor);
        else if(mouseState == AT_TOP_RIGHT || mouseState == AT_BOTTOM_LEFT)
            setCursor(Qt::SizeBDiagCursor);
        else if(mouseState & (AT_LEFT | AT_RIGHT))
            setCursor(Qt::SizeHorCursor);
        else if(mouseState & (AT_TOP | AT_BOTTOM))
            setCursor(Qt::SizeVerCursor);
        else
            unsetCursor();
    }
    else{
        if(mouseState == 0){
            if(maximized){
                qreal wRatio = (double)event->pos().x() / (double)ui->mainWidget->width();
                controlWindowScale();
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
                this->move(QPoint(event->globalPosition().x() - ui->mainWidget->width() * wRatio, -30));
#else
                this->move(QPoint(event->globalPos().x() - ui->mainWidget->width() * wRatio, -30));
#endif
                lastPos = QPoint(ui->mainWidget->width() * wRatio, event->pos().y());
            }
            else
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
                this->move(event->globalPosition().toPoint() - lastPos);
#else
                this->move(event->globalPos() - lastPos);
#endif
        }
        else{
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
            QPoint d = event->globalPosition().toPoint() - frameGeometry().topLeft() - lastPos;
#else
            QPoint d = event->globalPos() - frameGeometry().topLeft() - lastPos;
#endif
            if(mouseState & AT_LEFT){
                this->move(this->frameGeometry().x() + d.x(), this->frameGeometry().y());
                this->resize(this->width() - d.x(), this->height());
            }
            if(mouseState & AT_RIGHT){
                this->resize(this->width() + d.x(), this->height());
            }
            if(mouseState & AT_TOP){
                this->move(this->frameGeometry().x(), this->frameGeometry().y() + d.y());
                this->resize(this->width(), this->height() - d.y());
            }
            if(mouseState & AT_BOTTOM){
                this->resize(this->width(), this->height() + d.y());
            }
        }
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
        lastPos = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
#else
        lastPos = event->globalPos() - this->frameGeometry().topLeft();
#endif
    }
}

void MainWindow::resizeEvent(QResizeEvent *event){
    //Resize border
    if(border)
        border->resize(ui->mainWidget->size() + QSize(2, 2));

    //Resize mask
    QPainterPath path;
#ifdef Q_OS_WINDOWS
    path.addRoundedRect(ui->mainWidget->rect(), cornerRadius - 1, cornerRadius - 1);
#else
    path.addRect(ui->mainWidget->rect());
#endif
    QRegion mask(path.toFillPolygon().toPolygon());
    ui->mainWidget->setMask(mask);

    //Resize all pages
    for(int i = 0; i < pageList.size(); i++){
        pageList[i]->resize(ui->mainWidget->width() * 0.4 < pageList[i]->preferWidth ? pageList[i]->preferWidth - 1 : ui->mainWidget->width() * 0.4 - 1, ui->mainWidget->height());
        pageList[i]->resize(pageList[i]->width() + 1, pageList[i]->height());
    }
}

void MainWindow::controlWindowScale(){
#ifdef Q_OS_WINDOWS
    if(!maximized){
        lastGeometry = this->frameGeometry();
        windowShadow->setEnabled(false);
        ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
        border->hide();
        QString mainStyle = "QWidget#mainWidget{background-color:" + mainBackGround.name() + ";border-radius:0px;}";
        ui->mainWidget->setStyleSheet(mainStyle);
        this->showMaximized();
        maximized = true;
        QPainterPath path;
        path.addRect(ui->mainWidget->rect());
        QRegion mask(path.toFillPolygon().toPolygon());
        ui->mainWidget->setMask(mask);
    }
    else{
        ui->verticalLayout->setContentsMargins(30, 30, 30, 30);
        this->showNormal();
        QString mainStyle = "QWidget#mainWidget{background-color:" + mainBackGround.name() + QString::asprintf(";border-radius:%dpx", cornerRadius) + "}";
        ui->mainWidget->setStyleSheet(mainStyle);
        QPainterPath path;
        path.addRoundedRect(ui->mainWidget->rect(), cornerRadius - 1, cornerRadius - 1);
        QRegion mask(path.toFillPolygon().toPolygon());
        ui->mainWidget->setMask(mask);
        border->show();
        windowShadow->setEnabled(true);
        this->resize(lastGeometry.width(), lastGeometry.height());
        this->move(lastGeometry.x(), lastGeometry.y());
        maximized = false;
    }
#endif
}

MyCanvas* MainWindow::loadCanvas(const QString &path){
    // QFile input(path);
    // input.open(QIODevice::ReadOnly);
    // QTextStream ts(&input);
    // QString magicString = ts.readLine();
    // if(magicString != "VFdGeWFXUnZaekl3TURJd05ESTE=")   return nullptr;
    // MyCanvas *newCanvas = new MyCanvas(ts, cornerRadius, ui->mainWidget);
    // input.close();
    return nullptr;
}

void MainWindow::on_receiveMessage(const QString &message, const MessageType &type){
    if(type == MessageType::return_msg && message == "login:login_success"){
        timeout->stop();
        MyCanvas *newCanvas = new MyCanvas(cornerRadius,
                                           g_links.CurrentLogin,
                                           socket,
                                            ui->mainWidget);
        curLayersPage = newCanvas->CavlayerPage();
        canvasList.push_back(newCanvas);
        pageList.push_back(newCanvas->settingPage());
        pageList.push_back(newCanvas->CavlayerPage());
        selectCanvas(newCanvas);
        loginPage->slideOut();
        connect(this, &MainWindow::receiveMessage, newCanvas, &MyCanvas::on_receiveMessage);
        connect(newCanvas, &MyCanvas::setDel, this, [=](MyCanvas *c){curLayersPage->slideOut();deleteCanvas(c);});
        disconnect(this, &MainWindow::receiveMessage, this, &MainWindow::on_receiveMessage);
        uint32_t instruction = static_cast<uint32_t>(InstructionType::get_all_online_users);
        vector<char> data = socket->GenerateMessage(MessageType::instruction, instruction);
        socket->SendData(data);
        newCanvas->socketTimerout->start(10000);
    }else if(type == MessageType::return_msg && message.contains("login:")){
        timeout->stop();
        QStringList list = message.split(":");
        if(list.size() > 1) QMessageBox::warning(this, "登录失败", list[1]);
    }else if(type == MessageType::return_msg && message == "register:register_success"){
        timeout->stop();
        QStringList list = message.split(":");
        if(list.size() > 1) QMessageBox::information(this, "注册成功", list[1]);
        registerPage->slideOut();
    }else if(type == MessageType::return_msg && message.contains("register:")){
        timeout->stop();
        QStringList list = message.split(":");
        if(list.size() > 1) QMessageBox::warning(this, "注册失败", list[1]);
    }
}
