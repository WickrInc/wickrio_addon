#include <QDebug>

#include "addclientdialog.h"
#include "ui_addclientdialog.h"
#include "wickrbotmessagebox.h"
#include "wickrioconsoleclienthandler.h"

AddClientDialog::AddClientDialog(WickrIOClientDatabase *ioDB, WickrIOSSLSettings *sslSettings, WickrIOClients *client, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddClientDialog),
    m_client(client),
    m_ioDB(ioDB),
    m_sslSettings(sslSettings)
{
    ui->setupUi(this);

    // Setup the supported network types
    ui->networkTypeComboBox->addItem("HTTP");
    ui->networkTypeComboBox->addItem("HTTPS");

    // Setup the possible network interfaces
    QStringList ifaces = WickrIOConsoleClientHandler::getNetworkInterfaceList();
    ui->interfaceComboBox->addItems(ifaces);

    // Setup the list of possible Console Users
    QList<WickrIOConsoleUser *> cusers = m_ioDB->getConsoleUsers();
    ui->consoleUserComboBox->addItem("No console association");
    int pos=0;
    consoleUserMap.insert(pos++,0);
    for (WickrIOConsoleUser *cuser : cusers) {
        ui->consoleUserComboBox->addItem(cuser->user);
        consoleUserMap.insert(pos++,cuser->id);
        delete cuser;
    }

    if (m_client != NULL) {
        ui->nameLineEdit->setText(m_client->name);
        ui->portLineEdit->setText(QString::number(m_client->port));
        int index = ui->interfaceComboBox->findText(m_client->iface);
        if (index == -1) {
            index = 0;
            qDebug() << "Could not find interface in the list" << m_client->iface;
        }
        ui->interfaceComboBox->setCurrentIndex(index);
        ui->apiKeyLineEdit->setText(m_client->apiKey);
        ui->userNameLineEdit->setText(m_client->user);
        ui->passwordLineEdit->setText(m_client->password);
        if (m_client->isHttps) {
            ui->networkTypeComboBox->setCurrentIndex(1);
        } else {
            ui->networkTypeComboBox->setCurrentIndex(0);
        }
        this->setWindowTitle(tr("Modify Client"));
        ui->addButton->setText("Save");
    } else {
        ui->nameLineEdit->setText("");
        ui->portLineEdit->setText("");
        ui->interfaceComboBox->setCurrentIndex(0);
        ui->apiKeyLineEdit->setText("");
        ui->userNameLineEdit->setText("");
        ui->passwordLineEdit->setText("");
        ui->networkTypeComboBox->setCurrentIndex(0);

        this->setWindowTitle(tr("Add Client"));
        ui->addButton->setText("Add");
    }

    connect(ui->addButton, &QPushButton::clicked, this, &AddClientDialog::addClient);
    connect(ui->cancelButton, &QPushButton::clicked, [=]() {
        close();
        emit finished();
    });

    connect(ui->nameLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->portLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->interfaceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));
    connect(ui->apiKeyLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->userNameLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->passwordLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->networkTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));
    connect(ui->consoleUserComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));

    updateButtons();
}

AddClientDialog::~AddClientDialog()
{
    delete ui;
}

void
AddClientDialog::slotComboChanged(int)
{
    updateButtons();
}

void
AddClientDialog::updateButtons()
{
    if (ui->nameLineEdit->text().isEmpty() ||
        ui->portLineEdit->text().isEmpty() ||
        ui->userNameLineEdit->text().isEmpty() ||
        ui->apiKeyLineEdit->text().isEmpty() ||
        ui->passwordLineEdit->text().isEmpty()) {
        ui->addButton->setEnabled(false);
        return;
    }

    QString name = ui->nameLineEdit->text().toLower();
    int port = ui->portLineEdit->text().toInt();
    QString interface = ui->interfaceComboBox->currentText();
    QString user = ui->userNameLineEdit->text().toLower();
    QString apiKey = ui->apiKeyLineEdit->text();
    QString password = ui->passwordLineEdit->text();
    int cUserID = consoleUserMap.value(ui->consoleUserComboBox->currentIndex(),0);

    if (m_client != NULL) {
        // TODO: Make sure all fields are checked

        // See if there are some values changed
        if (name != m_client->name || port != m_client->port || interface != m_client->iface || user != m_client->user ||
            apiKey != m_client->apiKey || password != m_client->password || cUserID != m_client->console_id) {
            ui->addButton->setEnabled(true);
        } else {
            ui->addButton->setEnabled(false);
        }
    } else {
        ui->addButton->setEnabled(true);
    }
}

void
AddClientDialog::addClient()
{
    WickrIOClients client;
    client.name = ui->nameLineEdit->text();
    client.port = ui->portLineEdit->text().toInt();
    client.iface = ui->interfaceComboBox->currentText();
    client.apiKey = ui->apiKeyLineEdit->text();
    client.user = ui->userNameLineEdit->text();
    client.password = ui->passwordLineEdit->text();
    if (ui->networkTypeComboBox->currentIndex() == 0) {
        client.isHttps = false;
        client.sslKeyFile = "";
        client.sslCertFile = "";
    } else {
        client.isHttps = true;
        client.sslKeyFile = m_sslSettings->sslKeyFile;
        client.sslCertFile = m_sslSettings->sslCertFile;
    }
    client.console_id = consoleUserMap.value(ui->consoleUserComboBox->currentIndex(), 0);

    WickrBotClients *test;

    // If Add a new client then check if specific fields are unique
    if (m_client == NULL) {
        test = m_ioDB->getClientUsingName(client.name);
        // If a record is retrieved then show error and leave screen up
        if (test != NULL) {
            qDebug() << "Name field is NOT unique!";
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("Name Field is NOT Unique!");
            msg->exec();
            delete test;
            return;
        }

        test = m_ioDB->getClientUsingUserName(client.user);
        // If a record is retrieved then show error and leave screen up
        if (test != NULL) {
            qDebug() << "User Name field is NOT unique!";
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("User Name Field is NOT Unique!");
            msg->exec();
            delete test;
            return;
        }

        if (chkClientsInterfaceExists(client.iface, client.port)) {
            qDebug() << "Interface and port combination are NOT unique!";
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("Interface and port combination are NOT Unique!");
            msg->exec();
            return;
        }

        // If reached here then all is okay to proceed
        QString errorMsg = WickrIOConsoleClientHandler::addClient(m_ioDB, &client);
        if (!errorMsg.isEmpty()) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText(errorMsg);
            msg->exec();
            return;
        }
    } else {
        // If the user has changed the name then check for uniqueness
        if (m_client->name.toLower() != client.name.toLower()) {
            test = m_ioDB->getClientUsingName(client.name);
            // If a record is retrieved then show error and leave screen up
            if (test != NULL) {
                qDebug() << "Name field is NOT unique!";
                WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                msg->addButton(tr("OK"), 0);
                msg->setText("Name Field is NOT Unique!");
                msg->exec();
                delete test;
                return;
            }
        }

        if (m_client->user.toLower() != client.user.toLower()) {
            test = m_ioDB->getClientUsingUserName(client.user);
            // If a record is retrieved then show error and leave screen up
            if (test != NULL) {
                qDebug() << "User Name field is NOT unique!";
                WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                msg->addButton(tr("OK"), 0);
                msg->setText("User Name Field is NOT Unique!");
                msg->exec();
                delete test;
                return;
            }
        }

        if (((m_client->iface != client.iface) || (m_client->port != client.port)) &&
            chkClientsInterfaceExists(client.iface, client.port)) {
            qDebug() << "Interface and port combination are NOT unique!";
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("Interface and port combination are NOT Unique!");
            msg->exec();
            return;
        }


        // TODO: Need to be able to update a client recordd!!!!!

        // If reached here then all is okay to proceed
        QString errorMsg = WickrIOConsoleClientHandler::addClient(m_ioDB, &client);
        if (!errorMsg.isEmpty()) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText(errorMsg);
            msg->exec();
            return;
        }

        // Update the client record
        client.id = m_client->id;
        if (! m_ioDB->updateClientsRecord(&client, false)) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("Database error couldn not update client record!");
            msg->exec();
            return;
        }
    }

    close();
    emit finished();
}

/**
 * @brief AddClientDialog::chkClientsInterfaceExists
 * This function will check if the input interface and port are unique in the current
 * client database.
 * @param iface
 * @param port
 * @return
 */
bool AddClientDialog::chkClientsInterfaceExists(const QString& iface, int port)
{
    // TODO: Make sure not using the same port as the console server

    bool retVal = false;
    QList <WickrIOClients *>clients;
    clients = m_ioDB->getClients();

    for (WickrIOClients *client : clients) {
        if (client->port == port && (client->iface == iface || client->iface == "localhost" || iface == "localhost" )) {
            retVal = true;
        }
        delete client;
    }
    return retVal;
}
