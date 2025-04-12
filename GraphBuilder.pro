QT       += core gui texttospeech
QT += svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    customScrollContainer.cpp \
    customWidgets.cpp \
    links.cpp \
    main.cpp \
    mainwindow.cpp \
    mycanvas.cpp \
    player.cpp \
    slidepage.cpp \
    WebCrawler.cpp \
    player.cpp \
    NovelRecommender.cpp \
    AllNovelManager.cpp \
    FavoriteManager.cpp

HEADERS += \
    customScrollContainer.h \
    customWidgets.h \
    links.h \
    mainwindow.h \
    mycanvas.h \
    player.h \
    slidepage.h \
    WebCrawler.h \
    socketlearn/socketlearn.h \
    socketlearn/classlist.h \
    player.h \
    NovelRecommender.h \
    AllNovelManager.h \
    FavoriteManager.h
    
FORMS += \
    mainwindow.ui
    
INCLUDEPATH += \
    socketlearn

LIBS += -l$$PWD/socketlearn -lsocketlearn_static
RC_ICONS = logo.ico
DEFINES += PROJECT_DIR=\\\"$$PWD\\\"
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc
