#include <QDebug>
#include <QCoreApplication>
#include "filedownloader.h"

FileDownloader::FileDownloader(QObject *parent) :
    QObject(parent)
{
    m_pManager = new QNetworkAccessManager();

    connect(m_pManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileDownloaded(QNetworkReply*)));
}

FileDownloader::~FileDownloader()
{
    if (m_pManager) {
        disconnect(m_pManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileDownloaded(QNetworkReply*)));
        m_pManager->deleteLater();
        m_pManager = NULL;

        // Wait for the delete later to be done
        QCoreApplication::processEvents();
    }
}

void
FileDownloader::getFile(QUrl imageUrl, bool wait4response)
{
    QNetworkRequest request(imageUrl);
    reply = m_pManager->get(request);

    connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(onDownloadProgress(qint64,qint64)));
    connect(reply,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(reply,SIGNAL(finished()),this,SLOT(onReplyFinished()));



    if (reply->isFinished())
        qDebug() << "FileDownloader: web request isFinished()";
    if (reply->isOpen())
        qDebug() << "FileDownloader: web request isOpen()";
    if (reply->isRunning())
        qDebug() << "FileDownloader: web request isRunning()";

    wait_for_response = wait4response;
    if (wait4response) {
        qDebug() << "starting wait for retrieval of image";
        request_event_loop = new QEventLoop();
        request_event_loop->exec();
        if (request_event_loop->isRunning()) {
            request_event_loop->exec();
        }
        request_event_loop->deleteLater();
        request_event_loop = NULL;
        qDebug() << "finished wait for retrieval of image";
    }
}

void FileDownloader::fileDownloaded(QNetworkReply* pReply)
{
    qDebug() << "enter fileDownloaded()";

    if (pReply->error() != QNetworkReply::NoError) {
        qDebug() << "fileDownloaded failed =" << pReply->errorString();
        emit downloadFailed();
    } else {
        m_DownloadedData = pReply->readAll();
        //emit a signal
        emit downloaded();
    }

    if (wait_for_response) {
        qDebug() << "stoping wait for retrieval of image";
        if (request_event_loop != NULL)
            request_event_loop->exit();
    }

}

QByteArray FileDownloader::downloadedData() const
{
    return m_DownloadedData;
}


void FileDownloader::onDownloadProgress(qint64 bytesRead,qint64 bytesTotal)
{
    qDebug() << QString::number(bytesRead).toLatin1() << " - " << QString::number(bytesTotal).toLatin1();
}

void FileDownloader::onReadyRead()
{
    qDebug() << "in onReadyRead()";
}

void FileDownloader::onReplyFinished()
{
    qDebug() << "in onReplyFinished()";
}
