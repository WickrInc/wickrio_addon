#ifndef WICKRBOTMESSAGEBOX_H
#define WICKRBOTMESSAGEBOX_H

#include <QLabel>
#include <QEventLoop>
#include <QBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include "wickrbotguilib.h"

class DECLSPEC WickrBotMessageBox : public QWidget
   // QMessageBox inherits severe styling limitations from QDialog!
{
    Q_OBJECT

    static int padding;  // for message box (pixels), on all sides
    static int buttonPadding;  // padding (pixels) on either side of button text

    static QList<WickrBotMessageBox *> list;
    QEventLoop* eventLoop;
    QBoxLayout* msgBoxlayout;
    QLabel noticeArea;
    QLabel textArea;
    QLabel passwordHeader;
    QLineEdit passwordInput;
    QHBoxLayout *passwordLayout;

    QLabel idHeader;
    QLineEdit idInput;

    QGridLayout *idpwGrid;

    QBoxLayout* buttonLayout;
    QHBoxLayout* customLayout;
    QWidget * customAddedWidget;
    QHash< QPushButton*, int > buttonMap;
    int buttonReturned;
    int parentWidth;
    int mMaxWidth;  // for message box (pixels)

    void closeEvent(QCloseEvent *event);

    QWidget* parentScreen;

public:
    enum mb_type { TYPE_NORMAL, TYPE_PASSWORD, TYPE_ID_PASSWORD };

    WickrBotMessageBox(QWidget *parent, mb_type type = TYPE_NORMAL, int maxWidth = 420);
    virtual ~WickrBotMessageBox();
    void setText( const QString& text );
    void setNotification( const QString& notification );
    void addButton( QString text, int returnValue = 0 );
    bool addWidget( QWidget * container );
    int exec();  // displays message box and returns returnValue of pushed button
    static void flush();

    void setWordWrap(bool enable) {
        textArea.setWordWrap(enable);
    }

    QString getID() {
        return idInput.text();
    }

    QString getPassword() {
        return passwordInput.text();
    }

#if defined(Q_OS_LINUX)
    static void postMoveEvent(QWidget * screen, QPoint center);

public slots:

    void deferredPostMoveEvent();
#endif

signals:
    void execDone();
    void closing();

public slots:
    void onMoved(QWidget *parent);

protected slots:
    void onClicked();
    void onDone();
};

#endif
