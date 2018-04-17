#ifndef OPERATIONDATA_H
#define OPERATIONDATA_H

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <QObject>
#include <QSettings>

#include "wickrbotlib.h"
#ifndef DO_WICKRBOT
#include "filedownloader.h"
#endif
#include "wickrbotlog.h"
#include "wickrbotdatabase.h"
#include "wickrbotclients.h"
#include "loghandler.h"

class DECLSPEC OperationData : public QObject
{
Q_OBJECT
public:
    OperationData();
    ~OperationData();

public:
    QString processName;
    bool debug = false;
    bool force = false;

    QString appDataDir;

    QString outputFile;
    bool gotOutputFile;

    QString inputFile;
    bool gotInputFile;

    QString attachmentsDir;
    QString databaseDir;
    QString messagesDir;

    int messageCount = 0;
    int duration = 0;

    WickrBotDatabase *m_botDB;
    WickrBotClients *m_client;

    bool receiveOn;

    int cleanUpSecs;
    int startRcvSecs;
    int m_appTimeOut;   // number of seconds to wait for potentially hung application
    int m_pid;          // The process ID of this application

    bool m_handleInbox; // True if the client should handle inbox messaging

    // Set this to true to clean the messaging database when logging out
    bool m_cleanDBOnLogout = false;

    QSettings *m_settings = nullptr;

private:
#ifndef DO_WICKRBOT
    FileDownloader *m_pImgCtrl;
#endif
    bool m_waiting4image;
    bool m_imageSuccess;
    QString m_imageFileName;
    QList<QString> m_responses;

public:
    LogHandler *log_handler;
#ifndef DO_WICKRBOT
    bool downloadImage(QUrl imageURL, QString fullpath);
#endif
    bool busy();

    void addResponseMessage(QString message) {
        m_responses.append(message);
    }

    QString getReponseMessage() {
        QString response;
        if (m_responses.length() > 0) {
            response = m_responses.first();
            m_responses.removeFirst();
        }
        return response;
    }

    bool setApiKey(const QString &apiKey);
    bool validateApiKey(const QString &apiKey);

    QString getResponseURL();

    bool alreadyActive(bool closeDbOnExit=false);
    bool updateProcessState(int state, bool force=true);

    bool postEvent(QString event, bool critical);

private slots:
#ifndef DO_WICKRBOT
    void loadImage();
    void loadFailed();
#endif
};

#endif // OPERATIONDATA_H
