#ifndef WICKRIOCALLBACKTHREAD_H
#define WICKRIOCALLBACKTHREAD_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "wickriothread.h"
#include "operationdata.h"
#include "wickrioappsettings.h"
#include "SmtpMime"


class WickrIOCallbackThread : public WickrIOThread
{
    Q_OBJECT
public:
    WickrIOCallbackThread(OperationData *operation);
    ~WickrIOCallbackThread();

private:
    OperationData *m_operation;
    SmtpClient *m_smtp;

    QNetworkReply *reply;
    QNetworkAccessManager *mgr;
    int postedMsgID;
    QString m_url;

protected:
    void processStarted();
    void onTimerAction();

private:
    void startEmailCallback(WickrIOEmailSettings *email);
    bool sendMessages(WickrIOEmailSettings *email);
    MimeFile *getAttachmentFile(const QString &filename);

    void startUrlCallback(QString url);
    bool sendMessage();

private slots:
    void msgSendCallbackResponse(QNetworkReply *thereply, QByteArray);
    void msgSendCallbackError(QNetworkReply *thereply, QByteArray bytes);
    void gotReply(QNetworkReply *thereply);

signals:
    void sendReply(QNetworkReply *reply, QByteArray replyBytes);
    void sendError(QNetworkReply *reply, QByteArray replyBytes);

};

#endif // WICKRIOCALLBACKTHREAD_H
