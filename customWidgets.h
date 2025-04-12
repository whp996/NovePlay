#ifndef CUSTOMWIDGETS_H
#define CUSTOMWIDGETS_H

#include <QPushButton>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSlider>
#include <QString>
#include <QtSvg>
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "customScrollContainer.h"

#if (QT_VERSION > QT_VERSION_CHECK(6,3,0))
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QVBoxLayout>
#endif


class customIcon : public QPushButton{
    Q_OBJECT

    Q_PROPERTY(qreal rotationAngle READ rotationAngle WRITE setRotationAngle NOTIFY rotationAngleChanged)

private:
    int radius;
    qreal widgetRatio;
    qreal iconSizeRate = 0.8;
    qreal rotation = 0;
    QPixmap *iconImg;
    QString iconHint;

    /* for hover and click effects */
    QColor bgColor;
    QColor defaultColor = QColor(0, 0, 0, 0);
    QColor hoverColor = QColor(241, 241, 241, 200);

protected:
    void paintEvent(QPaintEvent* event);
    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

protected:
    qreal rotationAngle() const {return rotation;}

public:
    customIcon(QString iconPath, QString hint = "", int r = 0, QWidget *parent = nullptr);
    customIcon(const QPixmap &icon, QString hint = "", int r = 0, QWidget *parent = nullptr);

    void setRotationAngle(qreal angle = 0){rotation = angle;update();}

signals:
    void rotationAngleChanged();
};

class selectionItem : public QWidget{
    Q_OBJECT

private:
    QLabel *title;
    QLabel *description;
    QWidget *indicator;
    QWidget *mainContent;
    QWidget *bgWidget;
    QGraphicsOpacityEffect *opac;
    bool onSelected = false;
    bool mousePressed = false;

    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    selectionItem(QString name, QString info = "", QWidget *parent = nullptr);
    void Select();
    void Deselect();
    void setTitle(QString titleText){title->setText(titleText);}
    void setDescription(QString descText){description->setText(descText);}
    QString name(){return title->text();}

signals:
    void selected(selectionItem *item);
    //void heightChange();

};

class singleSelectGroup : public QWidget{
    Q_OBJECT

private:
    const int middleSpacing = 5;
    const int bottomSpacing = 30;
    QLabel *groupName;
    QVBoxLayout *mainLayout;
    int selectedID = -1;
    QVector<selectionItem*> selections;
    
    ScrollAreaCustom *itemsContainer;
    QVBoxLayout *itemsLayout;

public:
    singleSelectGroup(QString name = "", QWidget *parent = nullptr);
    void AddItem(selectionItem *item);
    void RemoveItem(selectionItem *item);
    void SetSelection(selectionItem *item);
    selectionItem *GetSelection(QString name);
    qreal value(){return selectedID;}

signals:
    void selectedItemChange(int selectID);
    void itemChange();

private slots:
    void changeSelection(selectionItem *item);
    void updateScrollAreaHeight();
};

class horizontalValueAdjuster : public QWidget{
    Q_OBJECT

private:
    const int middleSpacing = 5;
    const int bottomSpacing = 30;
    QLabel *title;
    qreal curValue;
    qreal minValue;
    qreal maxValue;
    qreal stepValue;
    QWidget *editArea;
    QLabel *valueLabel;
    //QDoubleSpinBox *editLabel;
    customIcon *decreaseBtn;
    customIcon *increaseBtn;
    QSlider *slider;

public:
    horizontalValueAdjuster(QString name, qreal min, qreal max, qreal step, QWidget *parent = nullptr);
    void setValue(qreal value);
    qreal value(){return curValue;}

signals:
    void valueChanged(qreal value);

};

class DraggableButton : public QPushButton
{
    Q_OBJECT
public:
    explicit DraggableButton(const QString &text, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool dragging;
    QPoint dragStartPosition;
};

class bigIconButton : public QWidget{
    Q_OBJECT

private:
    QPixmap *iconImg;
    QLabel *text;
    QLabel *icon;
    QWidget *bgWidget;
    QWidget *indicator;

    int cornerRadius;
    QString radiusStyle;

    bool selectable = false;
    bool mousePressed = false;
    bool onSelected = false;

    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    bigIconButton(const QString &iconPath, const QString &description, int radius, QWidget *parent = nullptr);
    void setSelectable(bool sel = true){selectable = sel;}
    void setScale(qreal scale);

signals:
    void clicked();
    void selected();
};

class textInputItem : public QWidget{
    Q_OBJECT

private:
    const int margin = 10;
    const int spacing = 10;
    QLabel *itemName;
    QLineEdit *editor;
    QWidget *bgWidget;
    QWidget *indicator;
    QGraphicsOpacityEffect *opac;

    QString curText = "";
    bool mousePressed = false;
    bool onEditing = false;

    bool enabled = true;

    void enterEditEffect();
    void leaveEditEffect();

    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    //void focusOutEvent(QFocusEvent *event);

public:
    textInputItem(const QString &name, QWidget *parent = nullptr);
    QLineEdit* lineEditor(){return editor;}
    QString value(){return editor->text();}

    void setValue(const QString &text);
    void setValidator(QValidator *vali){editor->setValidator(vali);}
    void setEnabled(bool enable = true){enabled = enable;}

signals:
    void textEdited(QString text);
};

class textButton : public QWidget{
    Q_OBJECT

private:
    QLabel *btnText;
    QWidget *bgWidget;
    QString defaultColor = "#0a0078d4";
    QString hoverColor = "#1a0078d4";
    QString pressedColor = "#2a0078d4";

    bool mousePressed;

    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    textButton(QString text, QWidget *parent = nullptr, qreal ratio = 0.5);
    textButton(QString text, QString defC, QString hoverC, QString pressedC, QWidget *parent = nullptr, qreal ratio = 0.5);
    void setValue(QString text){btnText->setText(text);}
    QString value(){return btnText->text();}

signals:
    void myClicked();
};

class MessageItem : public QWidget {
    Q_OBJECT
public:
    // 定义消息显示方向，Left表示左侧显示（通常为接收消息），Right表示右侧显示（通常为自己发送的消息）
    enum Side {
        Left,
        Right
    };

    explicit MessageItem(const QString &name, const QString &message, const QString &timestamp, Side side = Left, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *nameLabel;
    QLabel *messageLabel;
    QLabel *timeLabel;
    Side side;
};

class MessageDisplay : public QWidget {
    Q_OBJECT
public:
    explicit MessageDisplay(QWidget *parent = nullptr);
    
    void addMessage(MessageItem *item);

private:
    QScrollArea *scrollArea;
    QWidget *container;
    QVBoxLayout *layout;
};

class TransparentNavTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit TransparentNavTextBrowser(QWidget *parent = nullptr);

    QPushButton *prevButton;
    QPushButton *nextButton;
protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // CUSTOMWIDGETS_H
