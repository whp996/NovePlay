#include "customWidgets.h"

//*********************************************************//
//CustomIcon class implementation
//*********************************************************//

customIcon::customIcon(QString iconPath, QString hint, int r, QWidget *parent):
    QPushButton(parent),
    radius(r),
    iconHint(hint){
    QSvgRenderer renderer;
    renderer.load(iconPath);
    QSize size = renderer.defaultSize();
    iconImg = new QPixmap(size);
    iconImg->fill(Qt::transparent);
    QPainter painter(iconImg);
    painter.setRenderHints(QPainter::Antialiasing);
    renderer.render(&painter);

    widgetRatio = iconImg->height() / iconImg->width();
    bgColor = defaultColor;
}

customIcon::customIcon(const QPixmap &icon, QString hint, int r, QWidget *parent):
    QPushButton(parent),
    radius(r),
    iconHint(hint){
    iconImg = new QPixmap(icon);

    widgetRatio = iconImg->height() / iconImg->width();
    bgColor = defaultColor;
}

void customIcon::paintEvent(QPaintEvent *event){
    resize(height() / widgetRatio, height());

    QPainter bgPainter(this);
    bgPainter.setRenderHints(QPainter::Antialiasing);
    bgPainter.setPen(Qt::NoPen);
    bgPainter.setBrush(bgColor);
    bgPainter.drawRoundedRect(this->rect(), radius, radius);

    QPainter pixmapPainter(this);
    pixmapPainter.setRenderHints(QPainter::Antialiasing);
    pixmapPainter.translate(width() / 2, height() / 2);
    pixmapPainter.rotate(rotation);
    pixmapPainter.translate(-width() / 2, -height() / 2);
    int w = iconSizeRate * width();
    int h = iconSizeRate * height();
    pixmapPainter.drawPixmap(width() / 2 - w / 2, height() / 2 - h / 2, w, h, *iconImg);
}

void customIcon::enterEvent(QEnterEvent *event){
    bgColor = hoverColor;
    update();
}

void customIcon::leaveEvent(QEvent *event){
    bgColor = defaultColor;
    update();
}

void customIcon::mousePressEvent(QMouseEvent *event){
    emit clicked();
    setFocus();
    iconSizeRate -= 0.1;
    update();
}

void customIcon::mouseReleaseEvent(QMouseEvent *event){
    iconSizeRate += 0.1;
    update();
}


//*********************************************************//
//selectionItem class implementation
//*********************************************************//

selectionItem::selectionItem(QString name, QString info, QWidget *parent) :
    QWidget(parent){
    /* set labels */
    QFont titleFont = QFont("Corbel", 13);
    QFontMetrics fm(titleFont);
    qreal height = fm.lineSpacing();
    title = new QLabel(this);
    title->setText(name);
    title->setFont(titleFont);
    title->setMinimumHeight(height);
    title->setStyleSheet("color:#2c2c2c");
    title->setAlignment(Qt::AlignLeft | Qt::AlignBottom);

    QFont descFont = QFont("Corbel Light", 11);
    fm = QFontMetrics(descFont);
    height = fm.lineSpacing();
    description = new QLabel(this);
    description->setText(info);
    description->setFont(descFont);
    description->setMinimumHeight(height);
    description->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    description->setStyleSheet("color:#707070");
    description->setWordWrap(false);

    QScrollArea *descScrollArea = new QScrollArea(this);
    descScrollArea->setWidget(description);
    descScrollArea->setWidgetResizable(true);
    descScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    descScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    descScrollArea->setFrameShape(QFrame::NoFrame);
    descScrollArea->viewport()->setStyleSheet("background-color: #00000000");
    int hScrollBarHeight = descScrollArea->horizontalScrollBar()->sizeHint().height();
    descScrollArea->setFixedHeight(description->minimumHeight() + hScrollBarHeight + 2);
    
    indicator = new QWidget(this);

    /* set minimum height and layout */
    setFixedHeight(title->height() + (info == "" ? 0 : descScrollArea->height() + 5));
    
    indicator->resize(6, 0.4 * this->height());
    indicator->move(4, 0.3 * this->height());
    indicator->setStyleSheet("border-radius:3px;background-color:#0078D4");
    opac = new QGraphicsOpacityEffect(indicator);
    opac->setOpacity(0);
    indicator->setGraphicsEffect(opac);

    QVBoxLayout *contentLayout = new QVBoxLayout(this);
    contentLayout->setContentsMargins(20, 0, 0, 0);
    contentLayout->setSpacing(2);
    this->setLayout(contentLayout);
    contentLayout->addWidget(title);
    if(info != "")
        contentLayout->addWidget(descScrollArea);
    contentLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    /* set background widget */
    bgWidget = new QWidget(this);
    bgWidget->resize(this->size());
    bgWidget->setStyleSheet("border-radius:5px;background-color:#00000000");
    bgWidget->lower();
    bgWidget->show();

    this->setMouseTracking(true);
}

void selectionItem::enterEvent(QEnterEvent *event){
    bgWidget->setStyleSheet("border-radius:5px;background-color:#0a000000");
    QParallelAnimationGroup *enter = new QParallelAnimationGroup(this);
    QPropertyAnimation *longer = new QPropertyAnimation(indicator, "geometry", this);
    longer->setStartValue(indicator->geometry());
    longer->setEndValue(QRectF(4, 0.25 * this->height(), 6, this->height() * 0.5));
    longer->setDuration(150);
    longer->setEasingCurve(QEasingCurve::OutBack);
    QPropertyAnimation *fadeIn = new QPropertyAnimation(opac, "opacity", this);
    fadeIn->setStartValue(opac->opacity());
    fadeIn->setEndValue(0.99);
    fadeIn->setDuration(100);
    enter->addAnimation(longer);
    enter->addAnimation(fadeIn);
    enter->start();
}

void selectionItem::leaveEvent(QEvent *event){
    bgWidget->setStyleSheet("border-radius:5px;background-color:#00000000");
    QParallelAnimationGroup *leave = new QParallelAnimationGroup(this);
    QPropertyAnimation *shorter = new QPropertyAnimation(indicator, "geometry", this);
    shorter->setStartValue(indicator->geometry());
    shorter->setEndValue(QRectF(4, 0.3 * this->height(), 6, this->height() * 0.4));
    shorter->setDuration(150);
    shorter->setEasingCurve(QEasingCurve::OutBack);
    QPropertyAnimation *fadeOut = new QPropertyAnimation(opac, "opacity", this);
    fadeOut->setStartValue(opac->opacity());
    fadeOut->setEndValue(onSelected ? 0.99 : 0);
    fadeOut->setDuration(100);
    leave->addAnimation(shorter);
    leave->addAnimation(fadeOut);
    leave->start();

    if(mousePressed)
        mousePressed = false;
}

void selectionItem::mousePressEvent(QMouseEvent *event){
    bgWidget->setStyleSheet("border-radius:5px;background-color:#1a000000");
    QPropertyAnimation *shorter = new QPropertyAnimation(indicator, "geometry", this);
    shorter->setStartValue(indicator->geometry());
    shorter->setEndValue(QRectF(4, 0.4 * this->height(), 6, this->height() * 0.2));
    shorter->setDuration(100);
    shorter->setEasingCurve(QEasingCurve::OutBack);
    shorter->start();

    mousePressed = true;
}

void selectionItem::mouseReleaseEvent(QMouseEvent *event){
    if(mousePressed){
        bgWidget->setStyleSheet("border-radius:5px;background-color:#0a000000");
        QPropertyAnimation *longer = new QPropertyAnimation(indicator, "geometry", this);
        longer->setStartValue(indicator->geometry());
        longer->setEndValue(QRectF(4, 0.25 * this->height(), 6, this->height() * 0.5));
        longer->setDuration(150);
        longer->setEasingCurve(QEasingCurve::OutBack);
        longer->start();

        if(!onSelected){
            onSelected = true;
            title->setStyleSheet("color:#005FB8");
            description->setStyleSheet("color:#3a8fb7");
            setFocus();
        }
        emit selected(this);
        mousePressed = false;
    }
}

void selectionItem::resizeEvent(QResizeEvent *event){
    bgWidget->resize(this->size());
}

void selectionItem::Select(){
    if(!onSelected){
        onSelected = true;
        title->setStyleSheet("color:#005FB8");
        description->setStyleSheet("color:#3a8fb7");
        indicator->setGeometry(4, 0.5 * this->height(), 6, 0);

        QParallelAnimationGroup *sel = new QParallelAnimationGroup(this);
        QPropertyAnimation *longer = new QPropertyAnimation(indicator, "geometry", this);
        longer->setStartValue(indicator->geometry());
        longer->setEndValue(QRectF(4, 0.3 * this->height(), 6, this->height() * 0.4));
        longer->setDuration(150);
        longer->setEasingCurve(QEasingCurve::OutBack);
        QPropertyAnimation *fadeIn = new QPropertyAnimation(opac, "opacity", this);
        fadeIn->setStartValue(opac->opacity());
        fadeIn->setEndValue(0.99);
        fadeIn->setDuration(100);
        sel->addAnimation(longer);
        sel->addAnimation(fadeIn);
        sel->start();

        emit selected(this);
    }
}

void selectionItem::Deselect(){
    if(onSelected){
        onSelected = false;
        title->setStyleSheet("color:#2c2c2c");
        description->setStyleSheet("color:#707070");

        QPropertyAnimation *fadeOut = new QPropertyAnimation(opac, "opacity", this);
        fadeOut->setStartValue(opac->opacity());
        fadeOut->setEndValue(0);
        fadeOut->setDuration(100);
        fadeOut->start();
    }
}

//*********************************************************//
// singleSelectGroup 类实现
//*********************************************************//

singleSelectGroup::singleSelectGroup(QString name, QWidget *parent) :
    QWidget(parent){
    QFont titleFont = QFont("Corbel", 16);
    QFontMetrics fm(titleFont);
    qreal height = fm.lineSpacing();
    groupName = new QLabel(this);
    groupName->setMinimumHeight(height);
    groupName->setFont(titleFont);
    groupName->setText(name);

    QWidget *spacingLine = new QWidget(this);
    spacingLine->setFixedHeight(1);
    spacingLine->setStyleSheet("background-color:#0a000000");

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(middleSpacing);
    mainLayout->addWidget(groupName);
    mainLayout->addWidget(spacingLine);

    itemsContainer = new ScrollAreaCustom(this);
    itemsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addWidget(itemsContainer);
    updateScrollAreaHeight();
}

void singleSelectGroup::AddItem(selectionItem *item){
    selections.push_back(item);
    itemsContainer->addWidget(item, false);
    if(selectedID == -1){
        item->Select();
        selectedID = selections.indexOf(item);
    }
    connect(item, SIGNAL(selected(selectionItem*)), this, SLOT(changeSelection(selectionItem*)));
    updateScrollAreaHeight();
    emit itemChange();
}

void singleSelectGroup::RemoveItem(selectionItem *item){
    int id = selections.indexOf(item);
    if(id < 0)  return;
    selections.erase(selections.begin() + id);
    itemsContainer->removeWidget(item);
    item->setParent(nullptr);
    item->deleteLater();
    if(selections.size() == 0)
        selectedID = -1;
    else{
        selectedID = id < selections.size() ? id : id - 1;
        selections[selectedID]->Select();
    }
    emit selectedItemChange(selectedID);
    emit itemChange();
    updateScrollAreaHeight();
}

void singleSelectGroup::SetSelection(selectionItem *item){
    int id = selections.indexOf(item);
    selections[id]->Select();
}

void singleSelectGroup::changeSelection(selectionItem *item){
    int id = selections.indexOf(item);
    for(int i = 0; i < selections.size(); i++){
        if(i == id) continue;
        selections[i]->Deselect();
    }
    selectedID = id;
    emit selectedItemChange(id);
}

void singleSelectGroup::updateScrollAreaHeight() {
    int newHeight = 0;
    if(selections.size() > 0)
        newHeight = selections.size() * selections[0]->height();
    else
        newHeight = 0;
    if(newHeight > 500)
        newHeight = 500;

    int totalHeight = groupName->height() + middleSpacing + 1 + bottomSpacing + newHeight;
    this->setFixedHeight(totalHeight);
}

selectionItem *singleSelectGroup::GetSelection(QString name){
    for(int i = 0; i < selections.size(); i++){
        if(selections[i]->name() == name)
            return selections[i];
    }
    return nullptr;
}
//*********************************************************//
// horizontalValueAdjuster 类实现
//*********************************************************//

horizontalValueAdjuster::horizontalValueAdjuster(QString name, qreal min, qreal max, qreal step, QWidget *parent) :
    QWidget(parent),
    curValue(min),
    minValue(min),
    maxValue(max),
    stepValue(step)
{
    QFont titleFont = QFont("Corbel", 16);
    QFontMetrics fm(titleFont);
    qreal height = fm.lineSpacing();
    title = new QLabel(this);
    title->setMinimumHeight(height);
    title->setFont(titleFont);
    title->setText(name);

    QWidget *spacingLine = new QWidget(this);
    spacingLine->setFixedHeight(1);
    spacingLine->setStyleSheet("background-color:#0a000000");

    slider = new QSlider(Qt::Horizontal, this);
    slider->setMinimum(0);
    slider->setMaximum((max - min) / step + 1);
    slider->setPageStep(1);
    QString grooveStyle = "QSlider::groove:horizontal{height:6px; border-radius:3px;} ";
    QString sliderStyle = "QSlider::handle:horizontal{width:12px; margin-bottom:-3px; margin-top:-3px; background:#c2c2c2; border-radius:6px;} ";
    QString sliderHStyle = "QSlider::handle:horizontal:hover{width:12px; margin-bottom:-3px; margin-top:-3px; background:#3a8fb7; border-radius:6px;} ";
    QString sliderPStyle = "QSlider::handle:horizontal:pressed{width:12px; margin-bottom:-3px; margin-top:-3px; background:#005fb8; border-radius:6px;} ";
    QString subStyle = "QSlider::sub-page:horizontal{background:#0078D4; border-radius:3px} ";
    QString addStyle = "QSlider::add-page:horizontal{background:#1a000000; border-radius:3px} ";
    slider->setStyleSheet(grooveStyle+sliderStyle+sliderHStyle+sliderPStyle+subStyle+addStyle);


    QFont valueFont = QFont("Corbel", 13);
    fm = QFontMetrics(titleFont);
    height = fm.lineSpacing();
    valueLabel = new QLabel(this);
    valueLabel->setMinimumHeight(height);
    valueLabel->setFont(valueFont);
    valueLabel->setText(QString::asprintf("%g", min));
    valueLabel->setMinimumWidth(30);
    valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    valueLabel->setStyleSheet("margin-bottom:5px");

    QWidget *content = new QWidget(this);
    content->setMinimumHeight(valueLabel->height() < slider->height() ? valueLabel->height() : slider->height());
    QHBoxLayout *contentLayout = new QHBoxLayout(content);
    contentLayout->setAlignment(Qt::AlignVCenter);
    content->setLayout(contentLayout);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(10);
    contentLayout->addWidget(valueLabel);
    contentLayout->addWidget(slider);

    this->setMinimumHeight(title->height() + 2 * middleSpacing + 1 + content->height() + bottomSpacing);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    this->setLayout(mainLayout);
    mainLayout->setContentsMargins(10, 0, 10, bottomSpacing);
    mainLayout->setSpacing(middleSpacing);
    mainLayout->addWidget(title);
    mainLayout->addWidget(spacingLine);
    mainLayout->addWidget(content);

    connect(slider, &QSlider::valueChanged, this, [=](qreal value){
        valueLabel->setText(QString::asprintf("%g", value * stepValue + minValue));
        curValue = value * stepValue + minValue;
        emit valueChanged(curValue);
    });
}

void horizontalValueAdjuster::setValue(qreal value){
    valueLabel->setText(QString::asprintf("%g", value));
    slider->setValue((value - minValue) / stepValue);
    curValue = value;
    emit valueChanged(value);
}

//*********************************************************//
// DraggableButton 类实现
//*********************************************************//

DraggableButton::DraggableButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent), dragging(false)
{
    setMouseTracking(true);
}

void DraggableButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging = false;
        dragStartPosition = event->globalPos() - this->pos();
        event->accept();
    }
    QPushButton::mousePressEvent(event);
}

void DraggableButton::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint newPos = event->globalPos() - dragStartPosition;
        if (parentWidget()) {
            QRect parentRect = parentWidget()->rect();
            if (newPos.x() < 0)
                newPos.setX(0);
            if (newPos.y() < 0)
                newPos.setY(0);
            if (newPos.x() + width() > parentRect.width())
                newPos.setX(parentRect.width() - width());
            if (newPos.y() + height() > parentRect.height())
                newPos.setY(parentRect.height() - height());
        }
        if ((newPos - this->pos()).manhattanLength() > 5) {
            dragging = true;
        }
        if (dragging) {
            move(newPos);
            event->accept();
        }
    }
    QPushButton::mouseMoveEvent(event);
}

void DraggableButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (dragging) {
            event->accept();
            dragging = false;
            return;
        }
    }
    QPushButton::mouseReleaseEvent(event);
}


//*********************************************************//
// bigIconButton 类实现
//*********************************************************//

bigIconButton::bigIconButton(const QString &iconPath, const QString &description, int radius, QWidget *parent) :
    QWidget(parent),
    cornerRadius(radius)
{
    iconImg = new QPixmap(iconPath);

    /* set icon label and text label */
    icon = new QLabel(this);
    icon->setPixmap(*iconImg);
    icon->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    icon->setAlignment(Qt::AlignCenter);

    QFont textFont = QFont("Corbel", 13);
    QFontMetrics fm(textFont);
    text = new QLabel(this);
    text->setFont(textFont);
    text->setText(description);
    text->setWordWrap(true);
    text->setMinimumHeight(fm.lineSpacing());
    text->setAlignment(Qt::AlignCenter);

    /* set indicator */
    indicator = new QWidget(this);
    indicator->resize(6, 6);
    indicator->move(this->width() - 3, this->height() - 21);
    indicator->setStyleSheet("border-radius:3px;background-color:#afafaf");

    /* set background */
    bgWidget = new QWidget(this);
    bgWidget->resize(this->size());
    radiusStyle = QString::asprintf("border-radius:%dpx;", cornerRadius);
    bgWidget->setStyleSheet(radiusStyle + "background-color:#04000000");
    bgWidget->lower();
    bgWidget->show();

    /* set layout */
    QVBoxLayout *layout = new QVBoxLayout(this);
    this->setLayout(layout);
    layout->setContentsMargins(15, 35, 15, 35);
    layout->setSpacing(15);
    layout->addWidget(icon);
    layout->addWidget(text);
    layout->setAlignment(Qt::AlignCenter);

    this->setMinimumHeight(200);
}

void bigIconButton::resizeEvent(QResizeEvent *event){
    bgWidget->setFixedSize(this->size());
    if(onSelected){
        indicator->resize(this->width() * 0.1, 6);
        indicator->move(this->width() * 0.45, this->height() - 21);
    }
    else{
        indicator->resize(this->width() * 0.1, 6);
        indicator->move(this->width() * 0.45, this->height() - 21);
    }
}

void bigIconButton::enterEvent(QEnterEvent *event){
    bgWidget->setStyleSheet(radiusStyle + "background-color:#0a0078D4");
    QPropertyAnimation *longer = new QPropertyAnimation(indicator, "geometry", this);
    longer->setStartValue(indicator->geometry());
    longer->setEndValue(QRectF(this->width() * 0.2, this->height() - 21, this->width() * 0.6, 6));
    longer->setDuration(150);
    longer->setEasingCurve(QEasingCurve::OutBack);
    longer->start();

    indicator->setStyleSheet("border-radius:3px;background-color:#0078d4");
}

void bigIconButton::leaveEvent(QEvent *event){
    bgWidget->setStyleSheet(radiusStyle + "background-color:#04000000");
    QPropertyAnimation *shorter = new QPropertyAnimation(indicator, "geometry", this);
    shorter->setStartValue(indicator->geometry());
    if(!onSelected)
        shorter->setEndValue(QRectF(this->width() * 0.45, this->height() - 21, this->width() * 0.1, 6));
    else
        shorter->setEndValue(QRectF(this->width() * 0.3, this->height() - 21, this->width() * 0.4, 6));
    shorter->setDuration(250);
    shorter->setEasingCurve(QEasingCurve::OutBack);
    shorter->start();

    if(!onSelected)
        indicator->setStyleSheet("border-radius:3px;background-color:#afafaf");

    if(mousePressed)
        mousePressed = false;
}

void bigIconButton::mousePressEvent(QMouseEvent *event){
    bgWidget->setStyleSheet(radiusStyle + "background-color:#1a0078D4");
    QPropertyAnimation *shorter = new QPropertyAnimation(indicator, "geometry", this);
    shorter->setStartValue(indicator->geometry());
    shorter->setEndValue(QRectF(this->width() * 0.4, this->height() - 21, this->width() * 0.2, 6));
    shorter->setDuration(100);
    shorter->setEasingCurve(QEasingCurve::OutBack);
    shorter->start();

    mousePressed = true;
}

void bigIconButton::mouseReleaseEvent(QMouseEvent *event){
    if(mousePressed){
        bgWidget->setStyleSheet(radiusStyle + "background-color:#0a0078D4");
        QPropertyAnimation *longer = new QPropertyAnimation(indicator, "geometry", this);
        longer->setStartValue(indicator->geometry());
        longer->setEndValue(QRectF(this->width() * 0.2, this->height() - 21, this->width() * 0.6, 6));
        longer->setDuration(150);
        longer->setEasingCurve(QEasingCurve::OutBack);
        longer->start();

        mousePressed = false;
        emit clicked();
        if(selectable){
            emit selected();
            onSelected = true;
        }
    }
}

void bigIconButton::setScale(qreal scale){
    iconImg = new QPixmap(iconImg->scaled(iconImg->size() * scale, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    icon->setPixmap(*iconImg);
}

//*********************************************************//
// textInputItem 类实现
//*********************************************************//

textInputItem::textInputItem(const QString &name, QWidget *parent) :
    QWidget(parent)
{
    QFont nameFont = QFont("Corbel", 12);
    QFontMetrics fm(nameFont);
    qreal height = fm.lineSpacing();
    itemName = new QLabel(this);
    itemName->setText(name);
    itemName->setFont(nameFont);
    itemName->setFixedHeight(height);
    itemName->setStyleSheet("color:#1c1c1c");

    QFont textFont = QFont("Corbel", 12);
    fm = QFontMetrics(textFont);
    editor = new QLineEdit(this);
    editor->setText("");
    editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editor->setFixedHeight(fm.lineSpacing());
    editor->setStyleSheet("color:#5c5c5c;background-color:#00000000;border-style:none;");
    editor->setReadOnly(true);
    editor->setFont(textFont);

    bgWidget = new QWidget(this);
    bgWidget->setStyleSheet("background-color:#00000000;border-radius:5px;");
    bgWidget->lower();
    bgWidget->show();

    indicator = new QWidget(this);
    indicator->setFixedHeight(4);
    indicator->setStyleSheet("background-color:#0078d4;border-radius:2px");

    opac = new QGraphicsOpacityEffect(this);
    opac->setOpacity(0);
    indicator->setGraphicsEffect(opac);

    this->setFixedHeight(itemName->height() + 10);

    connect(editor, &QLineEdit::returnPressed, this, [=](){
        leaveEditEffect();
        onEditing = false;
        editor->setReadOnly(true);
        curText = editor->text();
    });
    connect(editor, &QLineEdit::editingFinished, this, [=](){
        leaveEditEffect();
        onEditing = false;
        editor->setReadOnly(true);
        curText = editor->text();
        QTimer *delay = new QTimer(this);
        connect(delay, &QTimer::timeout, this, [=](){mousePressed = false;});
        delay->setSingleShot(true);
        delay->start(10);
        mousePressed = false;
        emit textEdited(curText);
    });
}

void textInputItem::resizeEvent(QResizeEvent *event){
    itemName->move(margin, this->height() / 2 - itemName->height() / 2);
    itemName->setFixedWidth(qMax( static_cast<int>(this->width() * 0.3 - margin - spacing), itemName->text().length() * 15 ));
    int width = QFontMetrics(editor->font()).size(Qt::TextSingleLine, editor->text()).width() + 3;
    if(!onEditing){
        if(width > this->width() * 0.7 - margin)
            editor->resize(this->width() * 0.7 - margin, editor->height());
        else
            editor->resize(width, editor->height());
        editor->move(this->width() - margin - editor->width(), this->height() / 2 - editor->height() / 2);
        indicator->move(this->width() - margin, this->height() - 7);
    }
    else{
        editor->resize(this->width() * 0.7 - margin, editor->height());
        editor->move(this->width() * 0.3, this->height() / 2 - editor->height() / 2 - 2);
        indicator->move(this->width() * 0.3, this->height() - 7);
    }
    bgWidget->setFixedSize(this->size());
}

void textInputItem::enterEditEffect(){
    editor->setCursorPosition(editor->text().length());
    editor->setStyleSheet("color:#1c1c1c;background-color:#00000000;border-style:none;");
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    QPropertyAnimation *longer = new QPropertyAnimation(indicator, "geometry", this);
    longer->setStartValue(indicator->geometry());
    longer->setEndValue(QRectF(this->width() * 0.3, this->height() - 7, this->width() * 0.7 - margin, 4));
    longer->setDuration(500);
    longer->setEasingCurve(QEasingCurve::InOutExpo);
    QPropertyAnimation *fade = new QPropertyAnimation(opac, "opacity", this);
    fade->setStartValue(opac->opacity());
    fade->setEndValue(0.99);
    fade->setDuration(150);
    QPropertyAnimation *move = new QPropertyAnimation(editor, "geometry", this);
    move->setStartValue(editor->geometry());
    move->setEndValue(QRectF(this->width() * 0.3, this->height() / 2 - editor->height() / 2 - 2, this->width() * 0.7 - margin, editor->height()));
    move->setDuration(500);
    move->setEasingCurve(QEasingCurve::InOutExpo);
    group->addAnimation(longer);
    group->addAnimation(fade);
    group->addAnimation(move);
    group->start();
}

void textInputItem::leaveEditEffect(){
    editor->setCursorPosition(0);
    editor->setStyleSheet("color:#5c5c5c;background-color:#00000000;border-style:none;");
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    QPropertyAnimation *shorter = new QPropertyAnimation(indicator, "geometry", this);
    shorter->setStartValue(indicator->geometry());
    shorter->setEndValue(QRectF(this->width() - margin - 4, this->height() - 7, 4, 4));
    shorter->setDuration(500);
    shorter->setEasingCurve(QEasingCurve::InOutExpo);
    QPropertyAnimation *fade = new QPropertyAnimation(opac, "opacity", this);
    fade->setStartValue(opac->opacity());
    fade->setEndValue(0);
    fade->setDuration(350);
    QPropertyAnimation *move = new QPropertyAnimation(editor, "geometry", this);
    move->setStartValue(editor->geometry());
    int width = QFontMetrics(editor->font()).size(Qt::TextSingleLine, editor->text()).width() + 3;
    if(width > this->width() * 0.7 - margin)
        move->setEndValue(QRectF(this->width() * 0.3, this->height() / 2 - editor->height() / 2, this->width() * 0.7 - margin, editor->height()));
    else
        move->setEndValue(QRectF(this->width() - width - margin, this->height() / 2 - editor->height() / 2, width, editor->height()));
    move->setDuration(500);
    move->setEasingCurve(QEasingCurve::InOutExpo);
    group->addAnimation(shorter);
    group->addAnimation(fade);
    group->addAnimation(move);
    group->start();
}

void textInputItem::enterEvent(QEnterEvent *event){
    bgWidget->setStyleSheet("border-radius:5px;background-color:#0a000000");
}

void textInputItem::leaveEvent(QEvent *event){
    bgWidget->setStyleSheet("border-radius:5px;background-color:#00000000");
}

void textInputItem::mousePressEvent(QMouseEvent *event){
    bgWidget->setStyleSheet("border-radius:5px;background-color:#1a000000");
    mousePressed = true;
}

void textInputItem::mouseReleaseEvent(QMouseEvent *event){
    if(mousePressed){
        bgWidget->setStyleSheet("border-radius:5px;background-color:#0a000000");
        if(onEditing){
            leaveEditEffect();
            onEditing = false;
            curText = editor->text();
            editor->setReadOnly(true);
            emit textEdited(curText);
        }
        else{
            if(enabled){
                enterEditEffect();
                editor->raise();
                onEditing = true;
                editor->setReadOnly(false);
                editor->setText(curText + "");
                editor->setFocus();
            }
        }
        mousePressed = false;
    }
}

void textInputItem::setValue(const QString &text){
    editor->setText(text);
    editor->setCursorPosition(0);
    curText = text;
    int width = QFontMetrics(editor->font()).size(Qt::TextSingleLine, editor->text()).width() + 3;
    if(!onEditing){
        if(width > this->width() * 0.7 - margin)
            editor->resize(this->width() * 0.7 - margin, editor->height());
        else
            editor->resize(width, editor->height());
        editor->move(this->width() - margin - editor->width(), this->height() / 2 - editor->height() / 2);
    }
    else{
        editor->resize(this->width() * 0.7 - margin, editor->height());
        editor->move(this->width() * 0.3, this->height() / 2 - editor->height() / 2 - 2);
    }
}

//*********************************************************//
// textButton 类实现
//*********************************************************//

textButton::textButton(QString text, QWidget *parent, qreal ratio) :
    QWidget(parent)
{
    QFont textFont = QFont("Corbel", 10);
    QFontMetrics fm(textFont);
    qreal height = fm.lineSpacing();
    btnText = new QLabel(this);
    btnText->setText(text);
    btnText->setFont(textFont);
    btnText->setFixedHeight(height);
    btnText->setFixedWidth(fm.size(Qt::TextSingleLine, text).width() + 2);
    btnText->setStyleSheet("color:#1c1c1c");
    btnText->setAlignment(Qt::AlignCenter);

    bgWidget = new QWidget(this);
    bgWidget->setStyleSheet("background-color:"+defaultColor+";border-radius:5px;");

    this->setFixedHeight(btnText->height() / ratio);
}

textButton::textButton(QString text, QString defC, QString hoverC, QString pressedC, QWidget *parent, qreal ratio):
    QWidget(parent),
    defaultColor(defC),
    hoverColor(hoverC),
    pressedColor(pressedC)
{
    QFont textFont = QFont("Corbel", 10);
    QFontMetrics fm(textFont);
    qreal height = fm.lineSpacing();
    btnText = new QLabel(this);
    btnText->setText(text);
    btnText->setFont(textFont);
    btnText->setFixedHeight(height);
    btnText->setFixedWidth(fm.size(Qt::TextSingleLine, text).width() + 2);
    btnText->setStyleSheet("color:#1c1c1c");
    btnText->setAlignment(Qt::AlignCenter);

    bgWidget = new QWidget(this);
    bgWidget->setStyleSheet("background-color:"+defaultColor+";border-radius:5px;");

    this->setFixedHeight(btnText->height() / ratio);
}

void textButton::resizeEvent(QResizeEvent *event){
    bgWidget->resize(this->size());
    btnText->move(this->width() / 2 - btnText->width() / 2, this->height() / 2 - btnText->height() / 2);
}

void textButton::enterEvent(QEnterEvent *event){
    bgWidget->setStyleSheet("background-color:"+hoverColor+";border-radius:5px;");
}

void textButton::leaveEvent(QEvent *event){
    bgWidget->setStyleSheet("background-color:"+defaultColor+";border-radius:5px;");
    if(mousePressed){
        bgWidget->setStyleSheet("background-color:"+pressedColor+";border-radius:5px;");
        QPropertyAnimation *enlarge = new QPropertyAnimation(bgWidget, "geometry", this);
        enlarge->setStartValue(bgWidget->geometry());
        enlarge->setEndValue(QRect(0, 0, this->width(), this->height()));
        enlarge->setDuration(150);
        enlarge->setEasingCurve(QEasingCurve::OutBounce);
        enlarge->start();
        mousePressed = false;
    }
}

void textButton::mousePressEvent(QMouseEvent *event){
    bgWidget->setStyleSheet("background-color:"+pressedColor+";border-radius:5px;");
    QPropertyAnimation *shrink = new QPropertyAnimation(bgWidget, "geometry", this);
    shrink->setStartValue(bgWidget->geometry());
    shrink->setEndValue(QRect(0.05 * this->width(), 0.05 * this->height(), this->width() * 0.9, this->height() * 0.9));
    shrink->setDuration(100);
    shrink->setEasingCurve(QEasingCurve::OutBack);
    shrink->start();
    mousePressed = true;
    setFocus();
}

void textButton::mouseReleaseEvent(QMouseEvent *event){
    if(mousePressed){
        bgWidget->setStyleSheet("background-color:"+hoverColor+";border-radius:5px;");
        QPropertyAnimation *enlarge = new QPropertyAnimation(bgWidget, "geometry", this);
        enlarge->setStartValue(bgWidget->geometry());
        enlarge->setEndValue(QRect(0, 0, this->width(), this->height()));
        enlarge->setDuration(150);
        enlarge->setEasingCurve(QEasingCurve::OutBounce);
        enlarge->start();
        mousePressed = false;
        emit myClicked();
    }
}

//*********************************************************//
// MessageItem 类实现
//*********************************************************//

MessageItem::MessageItem(const QString &name, const QString &message, const QString &timestamp, Side side, QWidget *parent)
    : QWidget(parent), side(side)
{
    nameLabel = new QLabel(this);
    nameLabel->setFixedWidth((name.size() + 2) * 12);
    nameLabel->setAlignment(Qt::AlignCenter);
    if(side == Right){ nameLabel->setText(" :" + name);}
    else{ nameLabel->setText(name + ": "); }
    QFont nameFont("Corbel", 12, QFont::Bold);
    nameLabel->setFont(nameFont);

    messageLabel = new QLabel(this);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setText(message);
    messageLabel->setWordWrap(true);
    QFont msgFont("Corbel", 12);
    messageLabel->setFont(msgFont);

    timeLabel = new QLabel(this);
    if(side == Right){ timeLabel->setAlignment(Qt::AlignRight); }
    timeLabel->setText(timestamp);
    QFont timeFont("Corbel", 10);
    timeLabel->setFont(timeFont);
    timeLabel->setStyleSheet("color:gray;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);

    QWidget *messageWidget = new QWidget(this);
    QHBoxLayout *messageLayout = new QHBoxLayout(messageWidget);
    messageLayout->setContentsMargins(0, 0, 0, 0);
    messageLayout->setSpacing(5);
    if(side == Right){ messageLayout->addWidget(messageLabel); messageLayout->addWidget(nameLabel); }
    else{ messageLayout->addWidget(nameLabel); messageLayout->addWidget(messageLabel); }
    messageWidget->setLayout(messageLayout);

    layout->addWidget(messageWidget);
    layout->addWidget(timeLabel);
    setLayout(layout);

    if(side == Left) {
        messageWidget->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 10px;");
    } else {
        messageWidget->setStyleSheet("background-color: #DCF8C6; border: 1px solid #AED581; border-radius: 10px;");
    }
}

void MessageItem::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

//*********************************************************//
// MessageDisplay 类实现
//*********************************************************//

MessageDisplay::MessageDisplay(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(300);
    this->setStyleSheet("border-radius:5px;background-color:#00000000");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    container = new QWidget;
    layout = new QVBoxLayout(container);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    layout->addStretch();

    container->setLayout(layout);
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);
}

void MessageDisplay::addMessage(MessageItem *item) {
    int count = layout->count();
    if(count > 0) {
        QLayoutItem *stretchItem = layout->itemAt(count - 1);
        if(stretchItem && stretchItem->spacerItem())
            layout->removeItem(stretchItem);
    }
    layout->addWidget(item);
    layout->addStretch();

    QTimer::singleShot(100, this, [=](){
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    });
}

//*********************************************************//
// TransparentNavTextBrowser 类实现
//*********************************************************//

TransparentNavTextBrowser::TransparentNavTextBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
    // 将整体布局方向设置为 RightToLeft，这样滚动条就会出现在左侧
    this->setLayoutDirection(Qt::RightToLeft);
    
    // 如果需要让文本内容保持正常，确保视口或滚动条内容方向不反转
    verticalScrollBar()->setLayoutDirection(Qt::LeftToRight);

    prevButton = new DraggableButton("上一页", this);
    prevButton->setFlat(true);
    prevButton->setStyleSheet("background-color: rgba(0, 0, 0, 0.3); border: none; color: white; font-weight: bold;");

    nextButton = new DraggableButton("下一页", this);
    nextButton->setFlat(true);
    nextButton->setStyleSheet("background-color: rgba(0, 0, 0, 0.3); border: none; color: white; font-weight: bold;");

    prevButton->raise();
    nextButton->raise();
    this->setStyleSheet("background-color: #FFFFFF;border:1px solid #cfcfcf;border-radius:10px;");
    prevButton->setVisible(false);
    nextButton->setVisible(false);
    // 以下为设置滚动条样式的代码（不需要修改，只需保留原有代码即可）
    verticalScrollBar()->setStyleSheet(
        "QScrollBar {"
        "  background:rgba(240, 240, 240, 0.3);"
        "  border-radius: 10px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #0078d4;"
        "  border: none;"
        "  border-radius: 3px;"
        "  min-height: 30px;"
        "}"
        "QScrollBar::sub-line:vertical, QScrollBar::add-line:vertical {"
        "  height: 0px;"
        "}"
    );
    verticalScrollBar()->setFixedWidth(6);
    verticalScrollBar()->installEventFilter(this);
}

void TransparentNavTextBrowser::resizeEvent(QResizeEvent *event)
{
    QTextBrowser::resizeEvent(event);
}

void TransparentNavTextBrowser::showEvent(QShowEvent *event)
{
    QTextBrowser::showEvent(event);
    int btnWidth = 70;
    int btnHeight = 30;
    int margin = 10;
    if (prevButton->pos() == QPoint(0,0))
        prevButton->setGeometry((width() - btnWidth)/2, margin, btnWidth, btnHeight);
    if (nextButton->pos() == QPoint(0,0))
        nextButton->setGeometry((width() - btnWidth)/2, height() - btnHeight - margin, btnWidth, btnHeight);
}

// 事件过滤器,用于处理滚动条的鼠标悬停动画效果
bool TransparentNavTextBrowser::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == verticalScrollBar())
    {
        if (event->type() == QEvent::Enter)
        {
            // 创建动画效果
            QPropertyAnimation *animation = new QPropertyAnimation(verticalScrollBar(), "minimumWidth");
            animation->setDuration(200); // 设置动画持续时间为200毫秒
            animation->setStartValue(6);
            animation->setEndValue(15);
            animation->start(QAbstractAnimation::DeleteWhenStopped);

            QPropertyAnimation *animation2 = new QPropertyAnimation(verticalScrollBar(), "maximumWidth");
            animation2->setDuration(200);
            animation2->setStartValue(6);
            animation2->setEndValue(15);
            animation2->start(QAbstractAnimation::DeleteWhenStopped);
        }
        else if (event->type() == QEvent::Leave)
        {
            // 创建动画效果
            QPropertyAnimation *animation = new QPropertyAnimation(verticalScrollBar(), "minimumWidth");
            animation->setDuration(200);
            animation->setStartValue(10);
            animation->setEndValue(6);
            animation->start(QAbstractAnimation::DeleteWhenStopped);

            QPropertyAnimation *animation2 = new QPropertyAnimation(verticalScrollBar(), "maximumWidth");
            animation2->setDuration(200);
            animation2->setStartValue(10);
            animation2->setEndValue(6);
            animation2->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    return QTextBrowser::eventFilter(watched, event);
}