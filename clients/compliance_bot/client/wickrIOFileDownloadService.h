#ifndef WICKRIOFILEDOWNLOADSERVICE_H
#define WICKRIOFILEDOWNLOADSERVICE_H

#include "messaging/wickrInbox.h"
#include "operationdata.h"


// Forward declaration
class WickrIOFileDownloadThread;

class WickrIORxDownloadFile
{
public:
    WickrIORxDownloadFile(WickrCore::WickrInbox *msg, WickrCore::FileInfo fileinfo, QString attachFilename, QString realFilename, QJsonObject&  json) :
        m_msg(msg),
        m_fileInfo(fileinfo),
        m_attachmentFileName(attachFilename),
        m_realFileName(realFilename),
        m_downloaded(false),
        m_downloading(false),
        m_json(json){
    }

    WickrCore::WickrInbox   *m_msg;
    WickrCore::FileInfo     m_fileInfo;
    QString                 m_attachmentFileName;
    QString                 m_realFileName;
    bool                    m_downloaded;
    bool                    m_downloading;
    QJsonObject             m_json;
};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOFileDownloadService : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOFileDownloadService();
    virtual ~WickrIOFileDownloadService();

    void downloadFile(WickrIORxDownloadFile *dload);


private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    WickrServiceState           m_state;
    QThread                     m_thread;
    WickrIOFileDownloadThread   *m_fdThread;

    void startThreads();
    void stopThreads();

signals:
    void signalDownloadFile(WickrIORxDownloadFile *dload);

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOFileDownloadThread : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOFileDownloadThread(QThread *thread, WickrIOFileDownloadService *svc);
    virtual ~WickrIOFileDownloadThread();

    void downloadFile(WickrIORxDownloadFile *dload);


private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock      m_lock;

    WickrIOFileDownloadService  *m_parent;
    bool                        m_running;
    OperationData               *m_operation;

    // File Download definitions
    QMap<QString, WickrIORxDownloadFile *> m_activeDownloads;

public slots:
    void slotDownloadFile(WickrIORxDownloadFile *dload);
    void slotSendFileStatusChange(const QString& uuid, const QString& status, float progress, const QString& finalFileName);


};



#endif // WICKRIOFILEDOWNLOADSERVICE_H
