#include <QDebug>

#include "wickrbotsettings.h"
#include "configureconsoleserverdialog.h"
#include "ui_configureconsoleserverdialog.h"
#include "wickrbotmessagebox.h"

#include "wickrioconsoleclienthandler.h"

ConfigureConsoleServerDialog::ConfigureConsoleServerDialog(QSettings *settings, WickrIOSSLSettings *sslSettings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigureConsoleServerDialog),
    m_settings(settings),
    m_sslSettings(sslSettings)
{
    ui->setupUi(this);
    setWindowTitle(tr("Setup Console Server"));

    // Setup the initial values for the Console settings
    QString type = m_settings->value(WBSETTINGS_CONSOLESVR_TYPE,"http").toString();
    QString iface = m_settings->value(WBSETTINGS_CONSOLESVR_IF,"localhost").toString();
    int port = m_settings->value(WBSETTINGS_CONSOLESVR_PORT,0).toInt();

    // Setup the supported network types
    ui->typeComboBox->addItem("HTTP");
    ui->typeComboBox->addItem("HTTPS");
    if (type == "https") {
        ui->typeComboBox->setCurrentIndex(1);
    } else {
        ui->typeComboBox->setCurrentIndex(0);
    }

    QStringList ifaces = WickrIOConsoleClientHandler::getNetworkInterfaceList();
    ui->interfaceComboBox->addItems(ifaces);
    int index = ui->interfaceComboBox->findText(iface);
    if (index == -1) {
        index = 0;
        qDebug() << "Could not find interface in the list" << iface;
    }
    ui->interfaceComboBox->setCurrentIndex(index);

    ui->portLineEdit->setText(QString::number(port));

    connect(ui->typeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));
    connect(ui->portLineEdit, &QLineEdit::textChanged, [=]() { updateDialog(); });
    connect(ui->interfaceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboChanged(int)));

    // If the user clicks the cancel button then close the dialog and finish
    connect(ui->buttonBox, &QDialogButtonBox::rejected, [=]() {
        close();
        emit finished();
    });

    // User clicked the OK button, process the data
    connect(ui->buttonBox, &QDialogButtonBox::accepted, [=]() {
        save();
        close();
        emit finished();
    });

    updateDialog();
}

ConfigureConsoleServerDialog::~ConfigureConsoleServerDialog()
{
    delete ui;
}

void
ConfigureConsoleServerDialog::slotComboChanged(int)
{
    updateDialog();
}

void
ConfigureConsoleServerDialog::updateDialog()
{
    bool isOK = true;

    if (ui->portLineEdit->text().isEmpty()) {
        isOK = false;
    } else if (ui->portLineEdit->text().toInt() <= 0) {
        isOK = false;
    }

    if (isOK) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

bool
ConfigureConsoleServerDialog::save()
{
    // Get the values from the screen
    QString type;
    bool isHttps;
    if (ui->typeComboBox->currentIndex() == 0) {
        type = "http";
        isHttps = false;
    } else {
        type = "https";
        isHttps = true;
    }
    QString iface = ui->interfaceComboBox->currentText();
    QString port = ui->portLineEdit->text();

    // Make sure the values are correct
    if (iface.isEmpty()) {
        WickrBotMessageBox *msg = new WickrBotMessageBox(this);
        msg->addButton(tr("OK"), 0);
        msg->setText(tr("Interface must have a value"));
        msg->exec();
        return false;
    }

    // Update the values in the settings file
    m_settings->setValue(WBSETTINGS_CONSOLESVR_TYPE, type);
    if (iface == "localhost") {
        m_settings->remove(WBSETTINGS_CONSOLESVR_IF);
    } else {
        m_settings->setValue(WBSETTINGS_CONSOLESVR_IF, iface);
    }
    m_settings->setValue(WBSETTINGS_CONSOLESVR_PORT, port);
    if (isHttps) {
        if (m_sslSettings->sslKeyFile.isEmpty() || m_sslSettings->sslCertFile.isEmpty()) {
            WickrBotMessageBox *msg = new WickrBotMessageBox(this);
            msg->addButton(tr("OK"), 0);
            msg->setText(tr("WARNING: SSL settings have not been setup.\nGo to the Advanced Settings"));
            msg->exec();
        }
        m_settings->setValue(WBSETTINGS_CONSOLESVR_SSLKEY, m_sslSettings->sslKeyFile);
        m_settings->setValue(WBSETTINGS_CONSOLESVR_SSLCERT, m_sslSettings->sslCertFile);
    } else {
        m_settings->remove(WBSETTINGS_CONSOLESVR_SSLKEY);
        m_settings->remove(WBSETTINGS_CONSOLESVR_SSLCERT);
    }
    m_settings->sync();

    return true;
}
