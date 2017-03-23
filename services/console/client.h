#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QTextEdit>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "wickriodatabase.h"
#include "addclientdialog.h"
#include "addconsoleuserdialog.h"
#include "advanceddialog.h"
#include "wickrbotipc.h"
#include "webserver.h"
#include "configureconsoleserverdialog.h"
#include "consoleserver.h"

#define SETTINGS_DB_LOCATION    "dblocation"

namespace Ui {
    class ConsoleDialog;
} // namespace Ui

class Client : public QDialog
{
    Q_OBJECT

public:
    Client(QWidget *parent = 0);
    ~Client();

private:
    // Functions specific to the Console Server
    void setupConsoleArea();
    void updateConsoleInformation();
    bool updateConsoleUsersList();
    void updateConsoleUsersButtons();

    QString getFilePath(QString filename) {
        QString path = QStandardPaths::writableLocation( QStandardPaths::DataLocation ) + "/" + QCoreApplication::organizationName() + "/" + filename;
        qDebug() << "File path=" << path;
        return path;
    }
    bool saveServerValues();
    void clientSetVisibility(bool visible);
    bool openDatabase();

    bool updateClientsList();
    void updateClientButtons();

    void updateServerInformation();

    bool getClientSettings(WickrIOClients *client);

    bool eventFilter(QObject *object, QEvent *event);

private slots:

private:
    Ui::ConsoleDialog *ui;
    QSettings *m_settings;
    WickrIOSSLSettings m_sslSettings;

    QString dbLocation;

    WebServer *m_webServer;
    ConsoleServer *m_consoleServer;

    WickrIOClientDatabase *m_ioDB;

    AddClientDialog *addClientDialog;
    WickrIOClients *updateClient;

    AddConsoleUserDialog *addConsoleUserDialog;
    WickrIOConsoleUser updateConsoleUser;
    ConfigureConsoleServerDialog *setupConsoleServerDialog;

    int m_clientsSelectedRow;
    int m_consoleUsersSelectedRow;

    QTimer *m_updateTimer;

    WickrBotIPC *ipc;

    QString m_appNm;

    AdvancedDialog *m_advancedDialog;
};

#define CONSOLEUSER_MODEL_HDR "User,Permissions,Max Clients"
#define CONSOLEUSER_MODEL_NUMCOLUMNS      3
#define CONSOLEUSER_MODEL_USER_IDX        0
#define CONSOLEUSER_MODEL_PERMISSION_IDX  1
#define CONSOLEUSER_MODEL_MAXCLIENTS_IDX  2

#define CLIENT_MODEL_HDR "Name,App ID,User,Iface,Port,Type,Status,Console,Msgs"
#define CLIENT_MODEL_NUMCOLUMNS      9
#define CLIENT_MODEL_NAME_IDX        0
#define CLIENT_MODEL_APIKEY_IDX      1
#define CLIENT_MODEL_USER_IDX        2
#define CLIENT_MODEL_IFACE_IDX       3
#define CLIENT_MODEL_PORT_IDX        4
#define CLIENT_MODEL_TYPE_IDX        5
#define CLIENT_MODEL_STATUS_IDX      6
#define CLIENT_MODEL_CONSOLEUSER_IDX 7
#define CLIENT_MODEL_MSGS_IDX        8

#endif
