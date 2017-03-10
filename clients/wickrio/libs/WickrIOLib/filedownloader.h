#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include "wickrbotlib.h"

class DECLSPEC FileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit FileDownloader(QObject *parent = 0);

    virtual ~FileDownloader();

    void getFile(QUrl imageUrl, bool wait = false);

    QByteArray downloadedData() const;

signals:
    void downloaded();
    void downloadFailed();

private slots:

    void fileDownloaded(QNetworkReply* pReply);

    void onDownloadProgress(qint64,qint64);
    void onReadyRead();
    void onReplyFinished();

private:

    QNetworkAccessManager *m_pManager;
    QNetworkReply *reply;

    bool wait_for_response;
    QEventLoop *request_event_loop;

    QByteArray m_DownloadedData;

};

#endif // FILEDOWNLOADER_H
