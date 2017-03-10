#include <QTimer>

#include "wickrbotmessagebox.h"

int WickrBotMessageBox::padding = 40;
int WickrBotMessageBox::buttonPadding = 25;
QList<WickrBotMessageBox *> WickrBotMessageBox::list;

WickrBotMessageBox::WickrBotMessageBox(QWidget *parent, mb_type type, int maxWidth)
    : QWidget(parent)
    , buttonReturned( -1 )
    , mMaxWidth(maxWidth)
    , parentScreen(NULL)
{
    // Always flush first to avoid contention lock ups
    flush();

    if(parent)
        parentWidth = parent->size().width() - (padding * 4);
    else
        parentWidth = maxWidth;

    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setWindowModality(Qt::ApplicationModal);
    msgBoxlayout = new QBoxLayout( QBoxLayout::TopToBottom, this );
    setLayout( msgBoxlayout );
    customAddedWidget = NULL;
    QPalette mbPalette = this->palette();
    mbPalette.setColor(this->backgroundRole(), Qt::white);
    this->setPalette(mbPalette);
    noticeArea.setText("Notice");
    noticeArea.setAlignment(Qt::AlignHCenter);
    textArea.setAlignment(Qt::AlignHCenter);
    // added color:black, since scrproxy window painted text white. can address later if need to
    noticeArea.setStyleSheet("font-family: Avenir Bold; font-size: 18px; font-style: bold; color: black;");
    textArea  .setStyleSheet("font-size: 16px; color: black;");
    msgBoxlayout->setSizeConstraint(QLayout::SetMinimumSize);

    QSpacerItem *spacer = new QSpacerItem(maxWidth - 70, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

    msgBoxlayout->setContentsMargins(padding,buttonPadding,padding,buttonPadding);
    msgBoxlayout->addWidget( &noticeArea );
    msgBoxlayout->addWidget( &textArea );
    msgBoxlayout->addSpacerItem(spacer);
    textArea.setWordWrap( true );
    textArea.setTextFormat(Qt::RichText);

    // Handle the Wickr ID and Password type of message box
    if (type == TYPE_ID_PASSWORD) {
        idInput.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        idHeader.setText(tr("Wickr ID:"));
        passwordInput.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        passwordInput.setEchoMode(QLineEdit::Password);
        passwordHeader.setText(tr("Password:"));

        idpwGrid = new QGridLayout();
        idpwGrid->addWidget(&idHeader, 0, 0, Qt::AlignRight);
        idpwGrid->addWidget(&idInput, 0, 1, Qt::AlignLeft);
        idpwGrid->addWidget(&passwordHeader, 1, 0, Qt::AlignRight);
        idpwGrid->addWidget(&passwordInput, 1, 1, Qt::AlignLeft);

        passwordLayout = new QHBoxLayout();
        passwordLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        passwordLayout->addLayout(idpwGrid);
        passwordLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        passwordLayout->setStretch(0,2);
        passwordLayout->setStretch(1,10);
        passwordLayout->setStretch(2,3);

        msgBoxlayout->addLayout(passwordLayout);
        msgBoxlayout->addSpacerItem(new QSpacerItem(0, 5, QSizePolicy::Fixed, QSizePolicy::Fixed));
    } else if (type == TYPE_PASSWORD) {
        passwordInput.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        passwordInput.setEchoMode(QLineEdit::Password);
        passwordHeader.setText(tr("Password:"));
        passwordLayout = new QHBoxLayout();
        passwordLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        passwordLayout->addWidget(&passwordHeader);
        passwordLayout->addWidget(&passwordInput);
        passwordLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        msgBoxlayout->addLayout(passwordLayout);
        msgBoxlayout->addSpacerItem(new QSpacerItem(0, 12, QSizePolicy::Fixed, QSizePolicy::Fixed));
    }

    customLayout = new QHBoxLayout();
    msgBoxlayout->addLayout( customLayout );

    buttonLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    msgBoxlayout->addLayout( buttonLayout );

    setFixedWidth(maxWidth);
    setWindowOpacity(1);
    eventLoop = new QEventLoop( this );
    list.append(this);

#if defined(Q_OS_LINUX)
#else
      QPoint origin(parent->x() + (parent->width() - width())/2, parent->y() + (parent->height() - height())/2 - 75);
      move(origin);
#endif
}

WickrBotMessageBox::~WickrBotMessageBox()
{
    delete eventLoop;
//    QHash< QPushButton*, int >::iterator it;
//    for ( it = buttonMap.begin(); it != buttonMap.end(); ++it )
//        delete it.key();
}

void WickrBotMessageBox::closeEvent(QCloseEvent *event)
{
    emit execDone();
    QWidget::closeEvent(event);
}

#if defined(Q_OS_LINUX)
void WickrBotMessageBox::postMoveEvent(QWidget * screen, QPoint center)
{
    foreach(WickrBotMessageBox* mbox, list) {
        if (mbox->parentScreen && mbox->parentScreen == screen) {
            QRect curRect = mbox->rect();
            curRect.moveCenter(center);
            mbox->move(curRect.topLeft());
        }
    }
}

void WickrBotMessageBox::deferredPostMoveEvent()
{
    if (parentScreen != NULL) {
        const QPoint global = parentScreen->mapToGlobal(parentScreen->rect().center());
        postMoveEvent(parentScreen, global);
    }
}
#endif

void WickrBotMessageBox::setText(const QString& text)
{
    textArea.setText( text );

    if(text.isEmpty())
    {
        textArea.hide();
    }
}

void WickrBotMessageBox::setNotification(const QString& notification)
{
    noticeArea.setText(notification);

    if(notification.isEmpty())
    {
        noticeArea.hide();
    }
}

void WickrBotMessageBox::addButton(QString text, int returnValue)
{
    QPushButton* button = new QPushButton( this );

//    button->setFlat( true );

    // If there is no parent screen set then the style sheet should be set
    // no time to fix this in the wickrStyle.css file, could cause problems with other buttons
    if (parentScreen == NULL) {
        button->setStyleSheet("font-family: Avenir Medium; color: #ffffff; background-color: #3c3c3c; font-size: 20px;");
    } else {
        // Make buttons have squared corners
        button->setStyleSheet("border-radius: 0px; background-color: #3c3c3c;");
    }
    button->setText( text );
    button->setMaximumWidth( button->fontMetrics().width( text ) + 2 * buttonPadding );
    buttonMap.insert( button, returnValue );
    buttonLayout->addWidget( button );
    buttonLayout->setSpacing(buttonPadding);
    // Width is fine for up to 2 buttons, but if more we need to adjust the size so they fit
    if (buttonMap.size() > 2)
    {
        setFixedWidth(mMaxWidth + ( buttonMap.size() * buttonPadding * 2 ) + buttonPadding );
    }
    connect( button, SIGNAL(clicked()), this, SLOT( onClicked() ) );
}

/**
 * @brief WickrBotMessageBox::addWidget
 * @param pointer to QWidget that will be added to the container if one does not exist already
 * @return true if the widget was added, false if it was not added
 */
bool WickrBotMessageBox::addWidget( QWidget * container )
{
    if(customAddedWidget == NULL && container != NULL)
    {
        customLayout->addWidget(container);
        customAddedWidget = container;
        return true;
    }

    return false;
}


int WickrBotMessageBox::exec()
{
    connect( this, SIGNAL(execDone()), this, SLOT(onDone()));

    //same way message box is positioned in constructor
#if !defined(Q_OS_LINUX)
    if (parentScreen != NULL) {
        QPoint origin(parentScreen->x() + (parentScreen->width() - width())/2, parentScreen->y() + (parentScreen->height() - height())/2 - 75);
        move(origin);
    }
#endif

    show();

#if defined(Q_OS_LINUX)
    // the Qt::FramelessWindowHint flag may not be respected/working on all window managers.
    // this will set the fixed height so it can not be changed by the user dragging the window handles.
    // the asumption is all text and buttons have been added and their size calculated at this point
    this->setFixedHeight(this->height());
#endif

    eventLoop->exec();

    // if a custom widget was added, clear its parent so it won't get automatically deleted
    // it's up to the creator to get data and manage its destruction
    if(customAddedWidget != NULL)
    {
        customAddedWidget->setParent(0);
    }

    deleteLater();
    return buttonReturned;
}

void WickrBotMessageBox::onClicked()
{
    buttonReturned = buttonMap.find( ( QPushButton* )sender() ).value();
    emit execDone();
}

void WickrBotMessageBox::onDone()
{
    disconnect(this, SIGNAL(execDone()), this, SLOT(onDone()));
    eventLoop->quit();
    hide();
    int pos = list.indexOf(this);
    if(pos > -1)
        list.takeAt(pos);
    emit closing();
    disconnect(this, SIGNAL(closing()));
}

void WickrBotMessageBox::onMoved(QWidget *parent)
{
#if defined(Q_OS_LINUX)
      // verify is a wickrScreen ptr(should always be the case), else use the previous 'origin' QPoint
      //centerPoint = parent->mapToGlobal(parent->rect().center());
      Q_UNUSED(parent);
      QTimer* timer = new QTimer(this);
      connect(timer, &QTimer::timeout, this, &WickrBotMessageBox::deferredPostMoveEvent);
      timer->setSingleShot(true);
      timer->setInterval(10);
      timer->start();
#else
    if (parent != NULL) {
      QPoint origin(parent->x() + (parent->width() - width())/2, parent->y() + (parent->height() - height())/2);
      move(origin);
    }
#endif
}

void WickrBotMessageBox::flush()
{
    foreach(WickrBotMessageBox *mb, list) {
        mb->onDone();
    }
}
