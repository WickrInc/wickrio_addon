#include <QtWidgets>
#include <QtNetwork>
#include <QString>

#include "wbio_common.h"

#include "client.h"
#include "wickrbotipc.h"
#include "wickrbotmessagebox.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrioconsoleclienthandler.h"
#include "wickrioappsettings.h"
#include "wickrioconsoleuser.h"

#include "ui_console_dialog.h"
#include "wbio_common.h"
#include "server_common.h"

extern bool isVERSIONDEBUG();

Client::Client(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConsoleDialog),
    m_webServer(NULL),
    m_ioDB(NULL),
    m_clientsSelectedRow(-1),
    m_consoleUsersSelectedRow(-1),
    m_updateTimer(NULL)
{
    m_appNm = WBIO_CLIENTSERVER_TARGET;
    m_settings = WBIOServerCommon::getSettings();
    dbLocation = WBIOServerCommon::getDBLocation();

    m_webServer = new WebServer(dbLocation, m_settings, this);

    ui->setupUi(this);

#if defined(WICKR_DEBUG)
    this->setWindowTitle("WickrIO Server Console (Debug)");
#elif defined(WICKR_QA)
    this->setWindowTitle("WickrIO Server Console");
#endif

    ipc = new WickrBotIPC();

    setupConsoleArea();

    // Get the SSL settings
    m_sslSettings.readFromSettings(m_settings);

    connect(ui->quitButton, &QPushButton::clicked, this, &Client::close);

    connect(ui->serverStartButton, &QPushButton::clicked, [=]() {
        QString message;
        if (m_consoleServer->isRunning(WBIO_CLIENTSERVER_TARGET)) {
            message = tr("Stopping client server...");
        } else {
            message = tr("Starting client server...");
        }
        QProgressDialog progress(this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        progress.setLabelText(message);
        progress.setCancelButton(0);
        progress.setRange(0,0);
        progress.setMinimumDuration(0);
        progress.show();

        m_consoleServer->toggleState(WBIO_CLIENTSERVER_TARGET);

        progress.cancel();

        updateServerInformation();
    });

    // Handle the clicking of the Delete client button
    connect(ui->clientDeletePushButton, &QPushButton::clicked, [=]() {
        if (m_clientsSelectedRow != -1) {
            QAbstractItemModel *model = ui->clientsTableView->model();

            QModelIndex mycell = model->index(m_clientsSelectedRow, CLIENT_MODEL_NAME_IDX);
            QString name = mycell.data().toString();

            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->addButton(tr("Cancel"), 1);
            msg->setText(tr("Are you sure you want to delete the record for %1").arg(name));
            if (msg->exec() == 0) {
                // Delete the entry
                if (! m_ioDB->deleteClientUsingName(name)) {
                    WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                    msg->addButton(tr("OK"), 0);
                    msg->setText(tr("Failed to delete client's record for %1").arg(name));
                    msg->exec();
                }

                WickrIOClients *updateClient = m_ioDB->getClientUsingName(name);

                // Delete the process_state entry
                QString processName = WBIOServerCommon::getClientProcessName(updateClient);
                if (! m_ioDB->deleteProcessState(processName)) {
                    WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                    msg->addButton(tr("OK"), 0);
                    msg->setText(tr("Failed to delete client's process state record for %1").arg(name));
                    msg->exec();
                }

                updateClientsList();
                updateClientButtons();
            }
        }
    });

    // Handle the clicking of the Add a Client button
    connect(ui->clientAddPushButton, &QPushButton::clicked, [=]() {
        addClientDialog = new AddClientDialog(m_ioDB, &m_sslSettings, NULL, this);
        addClientDialog->setWindowModality(Qt::WindowModal);

        // This handles when a new client is added
        connect(addClientDialog, &AddClientDialog::finished, [=]() {
            addClientDialog->deleteLater();
            updateClientsList();
        });

        addClientDialog->show();
    });

    // Handle when the user clicks on an entry in the Clients Table
    connect(ui->clientsTableView, &QTableView::clicked, [=](const QModelIndex &index) {
       m_clientsSelectedRow = index.row();
       QAbstractItemModel *model = ui->clientsTableView->model();

       QModelIndex mycell = model->index(index.row(), CLIENT_MODEL_NAME_IDX);
       updateClientButtons();
    });

    ui->clientsTableView->installEventFilter(this);

    // If an entry is double clicked then modify an entry
    connect(ui->clientsTableView, &QTableView::doubleClicked, [=](const QModelIndex &index) {
        m_clientsSelectedRow = index.row();
        QAbstractItemModel *model = ui->clientsTableView->model();

        QModelIndex mycell = model->index(index.row(), CLIENT_MODEL_NAME_IDX);
        updateClientButtons();

        QString name = mycell.data().toString();

        // Start up the AddClientDialog to allow user to change the values of this client
        updateClient = m_ioDB->getClientUsingName(name);

        // send the stop command to the client
        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getClientProcessName(updateClient);
        if (m_ioDB->getProcessState(processName, &state)) {
            if (state.state != PROCSTATE_RUNNING) {
                if (!getClientSettings(updateClient)) {
                    qDebug() << "COuld not get the .ini/registr information for" << name;
                }

                addClientDialog = new AddClientDialog(m_ioDB, &m_sslSettings, updateClient, this);
                addClientDialog->setWindowModality(Qt::WindowModal);

                // This handles when a new client is added
                connect(addClientDialog, &AddClientDialog::finished, [=]() {
                    addClientDialog->deleteLater();
                });

                addClientDialog->show();
            }
        }
    });

    connect(ui->clientStartPushButton, &QPushButton::clicked, [=]() {
        if (m_clientsSelectedRow != -1) {
            QAbstractItemModel *model = ui->clientsTableView->model();
            QModelIndex mycell = model->index(m_clientsSelectedRow, CLIENT_MODEL_NAME_IDX);
            QString name = mycell.data().toString();

            WickrIOClients *updateClient = m_ioDB->getClientUsingName(name);

            // send the stop command to the client
            WickrBotProcessState state;
            QString processName = WBIOServerCommon::getClientProcessName(updateClient);
            if (m_ioDB->getProcessState(processName, &state)) {
                // If the process is running then stop it
                if (state.state == PROCSTATE_RUNNING) {
                    WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                    msg->addButton(tr("OK"), 0);
                    msg->addButton(tr("Cancel"), 1);
                    msg->setText(tr("Are you sure you want to pause the client for %1").arg(name));
                    if (msg->exec() == 0) {
                        if (state.ipc_port != 0) {
                            ipc->sendMessage(state.ipc_port, WBIO_IPCCMDS_PAUSE);
                        }
                    }
                } else {
                    if (state.state == PROCSTATE_PAUSED) {
                        WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                        msg->addButton(tr("OK"), 0);
                        msg->addButton(tr("Cancel"), 1);
                        msg->setText(tr("Are you sure you want to start the client for %1").arg(name));
                        if (msg->exec() == 0) {
                            // To start the client change the state of the client to DOWN and
                            // allow the service to start the client.
                            m_ioDB->updateProcessState(processName, 0, PROCSTATE_DOWN);
                        }
                    }
                }
                updateServerInformation();
                updateClientsList();
                updateClientButtons();
            }
        }
    });

    // Handle the clicking of the Advanced Settings button
    connect(ui->advancedButton, &QPushButton::clicked, [=]() {
        WickrIOEmailSettings *email = new WickrIOEmailSettings();

        m_settings->beginGroup(WBSETTINGS_EMAIL_HEADER);
        email->readFromSettings(m_settings);
        m_settings->endGroup();;

        m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
        m_sslSettings.readFromSettings(m_settings);
        m_settings->endGroup();

        m_advancedDialog = new AdvancedDialog(email, &m_sslSettings, this);
        m_advancedDialog->setWindowModality(Qt::WindowModal);
        m_advancedDialog->show();

        connect(m_advancedDialog, &AdvancedDialog::finished, [=](AdvancedDialog *dlg) {
            if (dlg->success) {
                if (dlg->emailSuccess) {
                    m_settings->beginGroup(WBSETTINGS_EMAIL_HEADER);
                    email->saveToSettings(m_settings);
                    m_settings->endGroup();
                }

                // TODO: Need to propagate the changes to the console server and clients
                if (dlg->sslSuccess) {
                    m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
                    m_sslSettings.saveToSettings(m_settings);
                    m_settings->endGroup();

                    // Update the console server
                    if (m_consoleServer->setSSL(&m_sslSettings)) {
                        // Prompt the user to restart the Console Server
                        WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                        msg->addButton(tr("OK"), 0);
                        msg->addButton(tr("Cancel"), 1);
                        msg->setText(tr("The Console Server needs to restart for changes to take effect. Restart?"));
                        if (msg->exec() == 0) {
                            QProgressDialog progress(this);
                            progress.setWindowModality(Qt::WindowModal);
                            progress.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
                            progress.setLabelText("Restarting Console Server");
                            progress.setCancelButton(0);
                            progress.setRange(0,0);
                            progress.setMinimumDuration(0);
                            progress.show();

                            m_consoleServer->restart();
                            progress.cancel();
                        }
                    }
                 }
            }
            delete dlg->email;
            dlg->deleteLater();
        });
    });

    ui->clientsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->clientsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->clientsTableView->verticalHeader()->setVisible(false);

    // Make the table Read only
    ui->clientsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    updateClientButtons();
    openDatabase();

    m_consoleServer = new ConsoleServer(m_ioDB);

    // Setup a timer which will update the screen
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, [=]() {
        updateConsoleUsersList();
        updateClientsList();
        updateConsoleUsersButtons();
        updateClientButtons();
        updateServerInformation();
    });
    m_updateTimer->setInterval(1000);
    m_updateTimer->start();
}

Client::~Client()
{
    if (m_updateTimer != NULL && m_updateTimer->isActive()) {
        m_updateTimer->stop();
        m_updateTimer->deleteLater();
        m_updateTimer = NULL;
    }

    if (m_settings != NULL) {
        m_settings->deleteLater();
        m_settings = NULL;
    }

    if (ipc != NULL) {
        ipc->deleteLater();
        ipc = NULL;
    }

    // Close the database and release the memory associated with it
    if (m_ioDB != NULL) {
        if (m_ioDB->isOpen()) {
            m_ioDB->close();
        }
        m_ioDB->deleteLater();
        m_ioDB = NULL;
    }

    QApplication::processEvents();
}


/**********************************************************************************************
 * Begin Functions specific to the Console Server
 *********************************************************************************************/

void
Client::setupConsoleArea()
{
    // When the user clicks on the Save button then udpate the settings file(s)
    connect(ui->consoleServerSetupButton, &QPushButton::clicked, [=]() {
        // show the Console Server Dialog
        m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
        setupConsoleServerDialog = new ConfigureConsoleServerDialog(m_settings, &m_sslSettings, this);
        setupConsoleServerDialog->setWindowModality(Qt::WindowModal);
        // This handles when a new client is added
        connect(setupConsoleServerDialog, &ConfigureConsoleServerDialog::finished, [=]() {
            m_settings->endGroup();
            updateServerInformation();
            setupConsoleServerDialog->deleteLater();
        });
        setupConsoleServerDialog->show();
    });


    connect(ui->consoleServerStartButton, &QPushButton::clicked, [=]() {
        QString message;
        if (m_consoleServer->isRunning()) {
            message = tr("Stopping console server...");
        } else {
            message = tr("Starting console server...");
        }
        QProgressDialog progress(this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        progress.setLabelText(message);
        progress.setCancelButton(0);
        progress.setRange(0,0);
        progress.setMinimumDuration(0);
        progress.show();

        m_consoleServer->toggleState();

        progress.cancel();

        updateServerInformation();
    });


    // Handle the clicking of the Add a Console User button
    connect(ui->consoleUsersAddButton, &QPushButton::clicked, [=]() {
        addConsoleUserDialog = new AddConsoleUserDialog(m_ioDB, NULL, this);
        addConsoleUserDialog->setWindowModality(Qt::WindowModal);

        // This handles when a new client is added
        connect(addConsoleUserDialog, &AddConsoleUserDialog::finished, [=]() {
            updateConsoleUsersList();
            addConsoleUserDialog->deleteLater();
        });
        addConsoleUserDialog->show();
    });

    // Handle when the user clicks on an entry in the Clients Table
    connect(ui->consoleUsersTableView, &QTableView::clicked, [=](const QModelIndex &index) {
       m_consoleUsersSelectedRow = index.row();
    });

    // If an entry is double clicked then modify an entry
    connect(ui->consoleUsersTableView, &QTableView::doubleClicked, [=](const QModelIndex &index) {
        m_consoleUsersSelectedRow = index.row();
        QAbstractItemModel *model = ui->consoleUsersTableView->model();
        QModelIndex mycell = model->index(index.row(), CONSOLEUSER_MODEL_USER_IDX);
        QString user = mycell.data().toString();

        // Start up the AddClientDialog to allow user to change the values of this client
        if (m_ioDB->getConsoleUser(user, &updateConsoleUser)) {
            addConsoleUserDialog = new AddConsoleUserDialog(m_ioDB, &updateConsoleUser, this);
            addConsoleUserDialog->setWindowModality(Qt::WindowModal);

            // This handles when a new client is added
            connect(addConsoleUserDialog, &AddConsoleUserDialog::finished, [=]() {
                addConsoleUserDialog->deleteLater();
            });

            addConsoleUserDialog->show();
        }
    });

        // Handle the clicking of the Delete client button
    connect(ui->consoleUsersDeleteButton, &QPushButton::clicked, [=]() {
        if (m_consoleUsersSelectedRow != -1) {
            QAbstractItemModel *model = ui->consoleUsersTableView->model();

            QModelIndex mycell = model->index(m_consoleUsersSelectedRow, CONSOLEUSER_MODEL_USER_IDX);
            QString user = mycell.data().toString();

            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->addButton(tr("Cancel"), 1);
            msg->setText(tr("Are you sure you want to delete the record for %1").arg(user));
            if (msg->exec() == 0) {
                WickrIOConsoleUser cUser;
                // Delete the entry
                if (!m_ioDB->getConsoleUser(user, &cUser) || !m_ioDB->deleteConsoleUser(user)) {
                    WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                    msg->addButton(tr("OK"), 0);
                    msg->setText(tr("Failed to delete console user's record for %1").arg(user));
                    msg->exec();
                } else {
                    WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                    msg->addButton(tr("OK"), 0);
                    msg->addButton(tr("NO"), 1);
                    msg->setText(tr("Delete clients linked to console user %1?").arg(user));
                    if (msg->exec() == 0) {
                        m_ioDB->deleteClientsOfConsoleUser(cUser.id);
                    } else {
                        m_ioDB->clearClientsOfConsoleUser(cUser.id);
                    }
                }
                updateConsoleUsersList();
                updateConsoleUsersButtons();
            }
        }
    });

    ui->consoleUsersTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->consoleUsersTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->consoleUsersTableView->verticalHeader()->setVisible(false);

    // Make the table Read only
    ui->consoleUsersTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    updateConsoleUsersButtons();
}

bool
Client::updateConsoleUsersList()
{
    int selectedIndex = -1;

    // Check if there is a row selected yet. If so save that index so highlight can be used.
    QItemSelectionModel *selectModel = ui->consoleUsersTableView->selectionModel();
    if (selectModel != NULL) {
        QModelIndexList selection = selectModel->selectedRows();
        if (selection.count() > 0) {
            QModelIndex index = selection.at(0);
            selectedIndex = index.row();
        }
    }

    if (m_ioDB != NULL && m_ioDB->isOpen()) {
        QList<WickrIOConsoleUser *>cusers = m_ioDB->getConsoleUsers();

        QStandardItemModel *model = new QStandardItemModel(cusers.length(), 3, this);

        ui->consoleUsersTableView->setModel(model);
        QModelIndex index;

        QRect rect = ui->consoleUsersTableView->geometry();
        ui->consoleUsersTableView->setColumnWidth(0, rect.width() * 0.40);
        ui->consoleUsersTableView->setColumnWidth(1, rect.width() * 0.30);
        ui->consoleUsersTableView->setColumnWidth(2, rect.width() * 0.30);

        QStringList headers = QString(CONSOLEUSER_MODEL_HDR).split(",");
        model->setHorizontalHeaderLabels(headers);

        if (cusers.length() > 0) {
            for (int i=0; i<cusers.length(); i++) {
                WickrIOConsoleUser *cuser = cusers.at(i);

                index = model->index(i, CONSOLEUSER_MODEL_USER_IDX, QModelIndex());
                model->setData(index, cuser->user);
                index = model->index(i, CONSOLEUSER_MODEL_PERMISSION_IDX, QModelIndex());
                QString permissions;
                if (cuser->permissions & CUSER_PERM_ADMIN_FLAG)
                    permissions = "Admin";
                else
                    permissions = "User";
                model->setData(index, permissions);
                index = model->index(i, CONSOLEUSER_MODEL_MAXCLIENTS_IDX, QModelIndex());
                model->setData(index, cuser->maxclients);
            }
//            ui->consoleUsersTableView->resizeColumnsToContents();
//            ui->consoleUsersTableView->horizontalHeader()->setStretchLastSection(true);
        } else {
        }

        // If there was a selection, then reselect it
        if (selectedIndex != -1) {
            ui->consoleUsersTableView->selectRow(selectedIndex);
        }

        // Free the memory used by the clients
        while (cusers.length() > 0) {
            WickrIOConsoleUser *cuser = cusers.first();
            cusers.removeFirst();
            delete cuser;
        }
    }
    return true;
}

void
Client::updateConsoleInformation()
{
    bool startServer;
    QString stateString;
    bool allowStart;

    if (m_consoleServer->isRunning()) {
        stateString = tr("The Server is RUNNING");
        startServer = false;
    } else {
        stateString = tr("The Server is DOWN");
        startServer = true;
    }

    ui->consoleServerStatusValue->setText(stateString);

    // Check that the fields are setup
    if (m_ioDB == NULL || ! m_ioDB->isOpen()) {
        allowStart = false;
    } else {
        if (m_ioDB->numConsoleUsers() == 0 || ! m_consoleServer->isConfigured()) {
            allowStart = false;
        } else {
            allowStart = true;
        }
    }


    // If the server needs to be started then set the appropriate buttons
    if (startServer) {
        // The start/stop button should show Start
        ui->consoleServerStartButton->setText(tr("Start"));
        ui->consoleServerStartButton->setEnabled(allowStart);
    } else {
        // The start/stop button should show Stop
        ui->consoleServerStartButton->setText(tr("Stop"));
        ui->consoleServerStartButton->setEnabled(true);
    }
}

void
Client::updateConsoleUsersButtons()
{
    bool showDelete = false;

    if (dbLocation.isEmpty()) {

    } else if (m_ioDB != NULL && m_ioDB->isOpen()){
        QItemSelectionModel *selectModel = ui->consoleUsersTableView->selectionModel();
        if (selectModel != NULL) {
            QModelIndexList selection = selectModel->selectedRows();
            if (selection.count() > 0) {
                showDelete = true;
            }
        }
    }

    ui->consoleUsersDeleteButton->setEnabled(showDelete);
}


/**********************************************************************************************
 * End Functions specific to the Console Server
 *********************************************************************************************/

bool Client::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->clientsTableView) {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mevent;
            mevent = dynamic_cast<QMouseEvent *>(event);
            if (mevent) {
                if (mevent->button() == Qt::RightButton) {
                    qDebug() << "got right mouse click";
                }
            }
        }
    }
    return false;
}

bool Client::openDatabase()
{
    // If there is a db location then try to open it
    if (!dbLocation.isEmpty()) {
        QDir dbDir(dbLocation);
        if (!dbDir.exists()) {
//            ui->statusValue->setText("DB location does not exist!");
        } else {
            m_ioDB = new WickrIOClientDatabase(dbLocation);
            if (!m_ioDB->isOpen()) {
//                ui->statusValue->setText("Cannot open database!");
            } else {
                updateConsoleUsersList();
                updateClientsList();
                updateConsoleUsersButtons();
                updateClientButtons();
                return true;
            }
        }
    } else {
//        ui->statusValue->setText("No DB location specified!");
    }

    return false;
}

/**
 * @brief Client::getClientSettings
 * This function will pull any settings for the client that are in the .ini/registry
 * @param newClient
 * @return
 */
bool
Client::getClientSettings(WickrIOClients *client)
{
    QString configFileName;

#ifdef Q_OS_WIN
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT)
            .arg(WBIO_ORGANIZATION)
            .arg(WBIO_GENERAL_TARGET)
            .arg(client->name);
#else
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(dbLocation).arg(client->name);
#endif
    QFile file(configFileName);

    if (!file.exists()) {
        return false;
    }

    QSettings * settings = new QSettings(configFileName, QSettings::NativeFormat);

    settings->beginGroup(WBSETTINGS_USER_HEADER);
    client->password = settings->value(WBSETTINGS_USER_PASSWORD, QString("")).toString();
    settings->endGroup();

    return true;
}


void
Client::updateClientButtons()
{
    bool showDelete = false;
    bool showStart = false;
    bool showAdd = false;

    if (dbLocation.isEmpty()) {

    } else if (m_ioDB != NULL && m_ioDB->isOpen()){
        QItemSelectionModel *selectModel = ui->clientsTableView->selectionModel();
        if (selectModel != NULL) {
            QModelIndexList selection = selectModel->selectedRows();
            if (selection.count() > 0) {
                QModelIndex index = selection.at(0);

                QAbstractItemModel *model = ui->clientsTableView->model();
                QModelIndex mycell = model->index(index.row(), CLIENT_MODEL_STATUS_IDX);
                QString status = mycell.data().toString();

                if (status.toLower() == "running") {
                    showDelete = false;
                    ui->clientStartPushButton->setText(tr("Pause"));
                } else {
                    showDelete = true;
                    ui->clientStartPushButton->setText(tr("Start"));
                }
                showStart = true;
            }
        }

        showAdd = WBIOServerCommon::getAvailableClientApps().length() > 0;
    }

    ui->clientDeletePushButton->setEnabled(showDelete);
    ui->clientStartPushButton->setEnabled(showStart);
    ui->clientAddPushButton->setEnabled(showAdd);
}

bool
Client::updateClientsList()
{
    int selectedIndex = -1;

    // Check if there is a row selected yet. If so save that index so highlight can be used.
    QItemSelectionModel *selectModel = ui->clientsTableView->selectionModel();
    if (selectModel != NULL) {
        QModelIndexList selection = selectModel->selectedRows();
        if (selection.count() > 0) {
            QModelIndex index = selection.at(0);
            selectedIndex = index.row();
        }
    }

    if (m_ioDB != NULL && m_ioDB->isOpen()) {
        QList<WickrIOClients *>clients = m_ioDB->getClients();

        QStandardItemModel *model = new QStandardItemModel(clients.length(), CLIENT_MODEL_NUMCOLUMNS, this);

        ui->clientsTableView->setModel(model);
        QModelIndex index;

        QStringList headers = QString(CLIENT_MODEL_HDR).split(",");
        model->setHorizontalHeaderLabels(headers);

        if (clients.length() > 0) {
            for (int i=0; i<clients.length(); i++) {
                WickrIOClients *client = clients.at(i);

                // Get the process state for the client
                QString processName = WBIOServerCommon::getClientProcessName(client);
                client->status = WickrIOConsoleClientHandler::getActualProcessState(processName, m_ioDB);

                QString cUserName;
                cUserName = m_ioDB->getClientsConsoleUser(client->id);

                index = model->index(i, CLIENT_MODEL_NAME_IDX, QModelIndex());
                model->setData(index, client->name);
                index = model->index(i, CLIENT_MODEL_STATUS_IDX, QModelIndex());
                model->setData(index, client->status);
                index = model->index(i, CLIENT_MODEL_USER_IDX, QModelIndex());
                model->setData(index, client->user);
                index = model->index(i, CLIENT_MODEL_APIKEY_IDX, QModelIndex());
                model->setData(index, client->apiKey);
                index = model->index(i, CLIENT_MODEL_BINARY_IDX, QModelIndex());
                model->setData(index, client->binary);
                index = model->index(i, CLIENT_MODEL_IFACE_IDX, QModelIndex());
                model->setData(index, client->iface);
                index = model->index(i, CLIENT_MODEL_PORT_IDX, QModelIndex());
                model->setData(index, client->port);
                index = model->index(i, CLIENT_MODEL_TYPE_IDX, QModelIndex());
                model->setData(index, client->getIfaceTypeStr());
                index = model->index(i, CLIENT_MODEL_CONSOLEUSER_IDX, QModelIndex());
                model->setData(index, cUserName);
                index = model->index(i, CLIENT_MODEL_MSGS_IDX, QModelIndex());
                model->setData(index, m_ioDB->getClientsActionCount(client->id));
            }
            ui->clientsTableView->resizeColumnsToContents();
            ui->clientsTableView->horizontalHeader()->setStretchLastSection(true);
        } else {
        }

        // If there was a selection, then reselect it
        if (selectedIndex != -1) {
            ui->clientsTableView->selectRow(selectedIndex);
        }

        // Free the memory used by the clients
        while (clients.length() > 0) {
            WickrIOClients *client = clients.first();
            clients.removeFirst();
            delete client;
        }
    }
    return true;
}


void
Client::updateServerInformation()
{
    updateConsoleInformation();

    bool startServer;
    QString stateString;

    /*
     * Update the Clients Server fields
     */
    if (m_consoleServer->isRunning(WBIO_CLIENTSERVER_TARGET)) {
        stateString = tr("The Server is RUNNING");
        startServer = false;
    } else {
        stateString = tr("The Server is DOWN");
        startServer = true;
    }

    ui->serverStatusValue->setText(stateString);

    // If the server needs to be started then set the appropriate buttons
    if (startServer) {
        // The start/stop button should show Start
        ui->serverStartButton->setText(tr("Start"));
    } else {
        // The start/stop button should show Stop
        ui->serverStartButton->setText(tr("Stop"));
    }
}

/**
 * @brief Client::clientSetVisibility
 * This function will set the visibility of the client widgets. The client should not be visible
 * if the server database is not set.
 * @param visible
 */
void Client::clientSetVisibility(bool visible)
{
    Q_UNUSED(visible);
}
