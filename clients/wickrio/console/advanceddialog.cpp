#include <QPushButton>

#include "advanceddialog.h"
#include "ui_advanceddialog.h"
#include "wickrbotmessagebox.h"

AdvancedDialog::AdvancedDialog(WickrIOEmailSettings *emailIN, WickrIOSSLSettings *sslIN, QWidget *parent) :
    QDialog(parent),
    email(emailIN),
    ssl(sslIN),
    success(false),
    emailSuccess(false),
    sslSuccess(false),
    ui(new Ui::AdvancedDialog)
{
    ui->setupUi(this);

    ui->typeComboBox->addItem("SMTP");
    ui->typeComboBox->addItem("SSL");
    ui->typeComboBox->addItem("TLS");

    dataToScreen();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, [=]() {
        success = true;
        screenToData();
        this->close();
        emit finished(this);
    });

    connect(ui->buttonBox, &QDialogButtonBox::rejected, [=]() {
        this->close();
        emit finished(this);
    });

    connect(ui->serverLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });
    connect(ui->portLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });
    connect(ui->typeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));
    connect(ui->accountLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });
    connect(ui->passwordLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });
    connect(ui->senderLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });
    connect(ui->recipientLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });

    connect(ui->sslKeyFileLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });
    connect(ui->sslCertFileLineEdit, &QLineEdit::textChanged, [=]() { updateScreen(); });

}

AdvancedDialog::~AdvancedDialog()
{
    delete ui;
}

void
AdvancedDialog::slotComboChanged(int)
{
    updateScreen();
}

/**
 * @brief AdvancedDialog::updateScreen
 * This function will update the screen based on what the user has entered.
 * The OK button will be enabled or disabled based on what the user entered.
 */
void
AdvancedDialog::updateScreen()
{
    bool okEnabled = true;

    if (ui->serverLineEdit->text().isEmpty() ||
        ui->accountLineEdit->text().isEmpty() ||
        ui->passwordLineEdit->text().isEmpty() ||
        ui->senderLabel->text().isEmpty() ||
        ui->recipientLineEdit->text().isEmpty()) {
        okEnabled = false;
    } else if (ui->portLineEdit->text().toInt() <= 0) {
        okEnabled = false;
    }

    if (! ssl->validateSSLKey(ui->sslKeyFileLineEdit->text())) {
        // TODO: Need to identify when the file does not exist!
    } else if (! ssl->validateSSLCert(ui->sslCertFileLineEdit->text())) {
        // TODO: Need to identify when the fildoes not exist
    } else if (!okEnabled && (!ui->sslKeyFileLineEdit->text().isEmpty() && !ui->sslCertFileLineEdit->text().isEmpty())) {
    // If the user has entered the SSL settings then that is fine to allow
        okEnabled = true;
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
}

/**
 * @brief AdvancedDialog::dataToScreen
 * This function will take the data from the input objects and display it on the screen.
 */
void
AdvancedDialog::dataToScreen()
{
    ui->serverLineEdit->setText(email->server);
    ui->accountLineEdit->setText(email->account);
    ui->passwordLineEdit->setText(email->password);
    ui->senderLineEdit->setText(email->sender);
    ui->recipientLineEdit->setText(email->recipient);

    ui->portLineEdit->setText(QString::number(email->port));
    if (email->type.isEmpty() || email->type.toLower() == "smtp") {
        ui->typeComboBox->setCurrentIndex(0);
    } else if (email->type.toLower() == "ssl") {
        ui->typeComboBox->setCurrentIndex(1);
    } else if (email->type.toLower() == "tls") {
        ui->typeComboBox->setCurrentIndex(2);
    }

    ui->sslKeyFileLineEdit->setText(ssl->sslKeyFile);
    ui->sslCertFileLineEdit->setText(ssl->sslCertFile);

    updateScreen();
}

/**
 * @brief AdvancedDialog::screenToData
 * This function will do several things; validate the data, if valid then set
 * the appropriate success flag (email or ssl). Then move the data from the GUI
 * to the appropriate object.
 */
void
AdvancedDialog::screenToData()
{
    // Deal with the Email settings
    WickrIOEmailSettings scrnEmail;
    scrnEmail.server = ui->serverLineEdit->text();
    scrnEmail.port = ui->portLineEdit->text().toInt();
    if (ui->typeComboBox->currentIndex() == 2) {
        scrnEmail.type = "tls";
    } else if (ui->typeComboBox->currentIndex() == 1) {
        scrnEmail.type = "ssl";
    } else {
        scrnEmail.type = "smtp";
    }
    scrnEmail.account = ui->accountLineEdit->text();
    scrnEmail.password = ui->passwordLineEdit->text();
    scrnEmail.sender = ui->senderLineEdit->text();
    scrnEmail.recipient = ui->recipientLineEdit->text();

    // Validate the Email changes. If no changes then email success is false.
    if (scrnEmail.server != email->server || scrnEmail.port != email->port ||
        scrnEmail.type != email->type || scrnEmail.account != email->account ||
        scrnEmail.password != email->password || scrnEmail.sender != email->sender ||
        scrnEmail.recipient != email->recipient) {
        *email = scrnEmail;
        emailSuccess = true;
    } else {
        emailSuccess = false;
    }

    // Handle the SSL settings
    WickrIOSSLSettings scrnSSL;
    scrnSSL.sslKeyFile = ui->sslKeyFileLineEdit->text();
    scrnSSL.sslCertFile = ui->sslCertFileLineEdit->text();

    if (scrnSSL.sslKeyFile != ssl->sslKeyFile || scrnSSL.sslCertFile != ssl->sslCertFile) {
        *ssl = scrnSSL;
        sslSuccess = true;

        // Warn the user!
        QString keyFile = ui->sslKeyFileLineEdit->text();
        QString crtFile = ui->sslCertFileLineEdit->text();
        if (keyFile.isEmpty() && crtFile.isEmpty()) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("SSL not setup!\nYou will not be able to use HTTPS!");
            msg->exec();
        } else if (keyFile.isEmpty()  || ! ssl->validateSSLKey(keyFile)) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("SSL Key file not found!\nYou will not be able to use HTTPS!");
            msg->exec();
        } else if (crtFile.isEmpty()  || ! ssl->validateSSLKey(crtFile)) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText("SSL Certificate file not found!\nYou will not be able to use HTTPS!");
            msg->exec();
        }
    } else {
        sslSuccess = false;
    }
}
