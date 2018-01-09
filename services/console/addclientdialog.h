#ifndef ADDCLIENTDIALOG_H
#define ADDCLIENTDIALOG_H

#include <QDialog>
#include <QComboBox>

#include "wickrbotclients.h"
#include "wickriodatabase.h"
#include "wickrIOAppSettings.h"

namespace Ui {
class AddClientDialog;
}

class AddClientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddClientDialog(WickrIOClientDatabase *ioDB, WickrIOSSLSettings *sslSettings, WickrBotClients *client=NULL, QWidget *parent = 0);
    ~AddClientDialog();

private:
    void addClient();
    void updateButtons();
    bool chkClientsInterfaceExists(const QString& iface, int port);

signals:
    void finished();

private slots:
    void slotComboChanged(int index);

private:
    Ui::AddClientDialog *ui;
    WickrBotClients *m_client;
    WickrIOClientDatabase *m_ioDB;

    QMap<int, int> consoleUserMap;

    WickrIOSSLSettings *m_sslSettings;
};

#endif // ADDCLIENTDIALOG_H
