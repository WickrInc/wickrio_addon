#include <QPushButton>

#include "addconsoleuserdialog.h"
#include "ui_addconsoleuserdialog.h"
#include "wickrbotmessagebox.h"

AddConsoleUserDialog::AddConsoleUserDialog(WickrIOClientDatabase *ioDB, WickrIOConsoleUser *consoleUser, QWidget *parent) :
    QDialog(parent),
    m_ioDB(ioDB),
    m_consoleUser(consoleUser),
    ui(new Ui::AddConsoleUserDialog)
{
    ui->setupUi(this);

    // Make sure the max clients only allows numbers up to 100
    QIntValidator *qiv = new QIntValidator(0, 100);
    ui->maxClientsLineEdit->setValidator(qiv);

    ui->permissionsComboBox->addItem("User");
    ui->permissionsComboBox->addItem("Administrator");
    QString tokenStr = WickrIOTokens::getRandomString(TOKEN_LENGTH);

    ui->authTypeComboBox->addItem("Basic");
    ui->authTypeComboBox->addItem("Email Token");

    // Place data from input object to the screen
    if (m_consoleUser != NULL) {
        setWindowTitle(tr("Modify User"));

        ui->userLineEdit->setText(m_consoleUser->user);
        ui->passwordLineEdit->setText(m_consoleUser->password);
        ui->maxClientsLineEdit->setText(QString::number(m_consoleUser->maxclients));
        if (m_consoleUser->permissions & CUSER_PERM_ADMIN_FLAG) {
            ui->permissionsComboBox->setCurrentIndex(1);
        } else {
            ui->permissionsComboBox->setCurrentIndex(0);
        }

        ui->authEmailLineEdit->setText(m_consoleUser->email);
        if (m_consoleUser->authType == CUSER_AUTHTYPE_EMAIL) {
            ui->authTypeComboBox->setCurrentIndex(1);
        } else {
            ui->authTypeComboBox->setCurrentIndex(0);
        }
        WickrIOTokens token;
        if (ioDB->getConsoleUserToken(m_consoleUser->id, &token)) {
            tokenStr = token.token;
        }
    } else {
        setWindowTitle(tr("Add User"));

        ui->userLineEdit->setText("");
        ui->passwordLineEdit->setText("");
        ui->maxClientsLineEdit->setText("1");
        ui->permissionsComboBox->setCurrentIndex(0);

        ui->authTypeComboBox->setCurrentIndex(0);
        ui->authEmailLineEdit->setText("");
    }
    ui->authTokenLineEdit->setText(tokenStr);

    // If the user clicks the cancel button then close the dialog and finish
    connect(ui->buttonBox, &QDialogButtonBox::rejected, [=]() {
        close();
        emit finished();
    });

    // User clicked the OK button, process the data
    connect(ui->buttonBox, &QDialogButtonBox::accepted, [=]() {
        if (saveScreenValues()) {
            close();
            emit finished();
        }
    });

    connect(ui->authTokenRefreshButton, &QPushButton::clicked, [=]() {
        QString tokenStr = WickrIOTokens::getRandomString(TOKEN_LENGTH);
        ui->authTokenLineEdit->setText(tokenStr);
    });

    connect(ui->userLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->passwordLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->maxClientsLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->permissionsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));

    connect(ui->authEmailLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->authTokenLineEdit, &QLineEdit::textChanged, [=]() { updateButtons(); });
    connect(ui->authTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));
}

AddConsoleUserDialog::~AddConsoleUserDialog()
{
    delete ui;
}

void
AddConsoleUserDialog::updateButtons()
{
    int maxClients;

    if (ui->maxClientsLineEdit->text().isEmpty()) {
        maxClients = 0;
    } else {
        maxClients = ui->maxClientsLineEdit->text().toInt();
    }

    bool okEnabled = true;
    if (ui->userLineEdit->text().isEmpty() || ui->passwordLineEdit->text().isEmpty() || maxClients < 0) {
        okEnabled = false;
    }
    if (ui->authTypeComboBox->currentIndex() == 1 && ui->authEmailLineEdit->text().isEmpty()) {
        okEnabled = false;
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
}

void
AddConsoleUserDialog::slotComboChanged(int)
{
    updateButtons();
}

bool
AddConsoleUserDialog::saveScreenValues()
{
    // Get the values from the screen
    QString user = ui->userLineEdit->text().toLower();
    QString password = ui->passwordLineEdit->text();
    int maxClients = ui->maxClientsLineEdit->text().toInt();
    int permissions;
    if (ui->permissionsComboBox->currentIndex() == 1) {
        permissions = 1;
    } else {
        permissions = 0;
    }
    QString tokenStr = ui->authTokenLineEdit->text();
    QString email = ui->authEmailLineEdit->text();
    int authType;
    if (ui->authTypeComboBox->currentIndex() == 1) {
        authType = CUSER_AUTHTYPE_EMAIL;
    } else {
        authType = CUSER_AUTHTYPE_BASIC;
    }

    // if this is a change to an existing user then modify that user
    if (m_consoleUser != NULL) {
        // If the updated has a different User name then check if the new one exists already!
        if (user != m_consoleUser->user.toLower()) {
            WickrIOConsoleUser cUser;
            if (m_ioDB->getConsoleUser(user, &cUser)) {
                WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                msg->addButton(tr("OK"), 0);
                msg->setText("User name is already in use!");
                msg->exec();
                return false;
            }
            m_consoleUser->user = user;
        }
        m_consoleUser->password = password;
        m_consoleUser->maxclients = maxClients;
        m_consoleUser->permissions = permissions;
        m_consoleUser->authType = authType;
        m_consoleUser->email = email;

        if (! m_ioDB->updateConsoleUser(m_consoleUser)) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("Failed to update Console User in the database!");
            msg->exec();
            return false;
        }

        // Check the token
        WickrIOTokens token;
        if (m_ioDB->getConsoleUserToken(m_consoleUser->id, &token)) {
            if (tokenStr != token.token) {
                token.token = tokenStr;
                if (! m_ioDB->updateToken(&token)) {
                    WickrBotMessageBox *msg = new WickrBotMessageBox(this);
                    msg->addButton(tr("OK"), 0);
                    msg->setText("Failed to update Token in the database!");
                    msg->exec();
                    return false;
                }
            }
        }

    } else {
        WickrIOConsoleUser cUser;
        QString error("");

        // See if the console user already exists
        if (m_ioDB->getConsoleUser(user, &cUser)) {
            error = tr("User name is already in use!");
        }
        // Insert the new Console user into the database
        else if (! m_ioDB->insertConsoleUser(user, password, permissions, maxClients, authType, email)) {
            error = tr("Failed to add Console User to the database!");
        }
        // Verify that the console user was added to the database, also need the ID
        else if (! m_ioDB->getConsoleUser(user, &cUser)) {
            error = tr("Failed to retrieve Console User from the database!");
        }
        // Add the token for this consoler user to the database
        else if (! m_ioDB->insertToken(tokenStr, cUser.id, "*")) {
            error = tr("Failed to add Token to the database!");
        }

        if (!error.isEmpty()) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText(error);
            msg->exec();
            return false;
        }
    }
    return true;
}
