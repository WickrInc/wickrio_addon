#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdadvanced.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleClientHandler.h"

CmdAdvanced::CmdAdvanced(CmdOperation *operation) :
    m_operation(operation)
{
}

/**
 * @brief CmdAdvanced::runCommands
 * This function is used to maintain the email and SSL settings.  Email is used to send
 * statistics and status information about the running processes. SSL settings are used
 * for HTTPS connections, and are required to be setup.
 */
bool CmdAdvanced::runCommands(QString commands)
{
    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    while (true) {
        QString line = getCommand("Enter advanced command:");
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();

            if (cmd == "?" || cmd == "help") {
                qDebug() << "CONSOLE:Advanced Commands:";
                qDebug() << "CONSOLE:  back   - leave the advanced settings";
                qDebug() << "CONSOLE:  config - configures the advanced settings";
                qDebug() << "CONSOLE:  delete - deletes the current advanced settings";
                qDebug() << "CONSOLE:  status - shows the current advanced settings";
                qDebug() << "CONSOLE:  quit   - leaves this program";
            } else if (cmd == "back") {
                break;
            } else if (cmd == "config") {
                configure();
            } else if (cmd == "delete") {
                qDebug() << "CONSOLE:Deleting the email settings";
                m_operation->m_settings->remove(WBSETTINGS_EMAIL_HEADER);
                m_operation->m_settings->sync();
            } else if (cmd == "status") {
                status();
            } else if (cmd == "quit") {
                return false;
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
    return true;
}

void CmdAdvanced::status()
{
    WickrIOEmailSettings email;

    qDebug() << "CONSOLE:Email Settings:";
    if (m_operation->m_settings->childGroups().contains(WBSETTINGS_EMAIL_HEADER)) {
        // Get the email settings from the settings file
        m_operation->m_settings->beginGroup(WBSETTINGS_EMAIL_HEADER);
        email.readFromSettings(m_operation->m_settings);
        m_operation->m_settings->endGroup();

        qDebug() << "CONSOLE:  server    :" << qPrintable(email.server);
        qDebug() << "CONSOLE:  port      :" << email.port;
        qDebug() << "CONSOLE:  type      :" << qPrintable(email.type);
        qDebug() << "CONSOLE:  account   :" << qPrintable(email.account);
        qDebug() << "CONSOLE:  sender    :" << qPrintable(email.sender);
        qDebug() << "CONSOLE:  recipient :" << qPrintable(email.recipient);
    } else {
        qDebug() << "CONSOLE:  NOT currently setup!";
    }

    WickrIOSSLSettings ssl;
    qDebug() << "CONSOLE:SSL Settings:";
    if (m_operation->m_settings->childGroups().contains(WBSETTINGS_SSL_HEADER)) {
        // Get the SSL settings from the settings file
        m_operation->m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
        ssl.readFromSettings(m_operation->m_settings);
        m_operation->m_settings->endGroup();

        qDebug() << "CONSOLE:  Key File  :" << qPrintable(ssl.sslKeyFile);
        qDebug() << "CONSOLE:  Cert File :" << qPrintable(ssl.sslCertFile);
    } else {
        qDebug() << "CONSOLE:  NOT currently setup!";
    }

}

void CmdAdvanced::configure()
{
    QString temp;

    temp = getNewValue("y", tr("Configure email settings (y or n)?"));
    if (temp.toLower() == "y") {
        WickrIOEmailSettings email;

        // Get the email settings from the settings file
        m_operation->m_settings->beginGroup(WBSETTINGS_EMAIL_HEADER);
        email.readFromSettings(m_operation->m_settings);
        m_operation->m_settings->endGroup();

        if (configEmail(&email)) {
            m_operation->m_settings->beginGroup(WBSETTINGS_EMAIL_HEADER);
            email.saveToSettings(m_operation->m_settings);
            m_operation->m_settings->endGroup();
        }
    }

    temp = getNewValue("y", tr("Configure SSL settings (y or n)?"));
    if (temp.toLower() == "y") {
        WickrIOSSLSettings ssl;

        // Get the SSL settings from the settings file
        m_operation->m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
        ssl.readFromSettings(m_operation->m_settings);
        m_operation->m_settings->endGroup();

        if (configSSL(&ssl)) {
            m_operation->m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
            ssl.saveToSettings(m_operation->m_settings);
            m_operation->m_settings->endGroup();

            ConsoleServer consoleServer(m_operation->m_ioDB);
            if (consoleServer.setSSL(&ssl)) {
                qDebug() << "CONSOLE:The Console Server needs to restart for changes to take effect.";
                temp = getNewValue("y", tr("Restart Console Server (y or n)?"));
                if (temp.toLower() == "y") {
                    qDebug() << "CONSOLE:Restarting Console Server!";
                    consoleServer.restart();
                    qDebug() << "CONSOLE:Done restarting Console Server!";
                }
            }
        }
    }
}

/**
 * @brief CmdAdvanced::configEmail
 * This function will configure the email settings.
 */
bool CmdAdvanced::configEmail(WickrIOEmailSettings *email)
{
    bool quit = false;

    // Get the server name
    while (true) {
        email->server = getNewValue(email->server, tr("Enter the email server"));

        // Check if the user wants to quit the action
        if (handleQuit(email->server, &quit) && quit) {
            return false;
        }

        if (email->server.isEmpty()) {
            qDebug() << "CONSOLE:Please enter a valid email server!";
        } else {
            break;
        }
    }

    // Get a unique interface and port pair
    while (true) {
        QString port = getNewValue(QString::number(email->port), tr("Enter the port number"), CHECK_INT);

        // Check if the user wants to quit the action
        if (handleQuit(port, &quit) && quit) {
            return false;
        }

        email->port = port.toInt();
        if (email->port > 0)
            break;
        qDebug() << "CONSOLE:Please enter a valid email port number!";
    }

    // Get the Port type value
    while (true) {
        email->type = getNewValue(email->type, tr("Enter the port type (smtp, ssl or tls)"));

        // Check if the user wants to quit the action
        if (handleQuit(email->type, &quit) && quit) {
            return false;
        }

        if (email->type.isEmpty() || (email->type.toLower() != "smtp" &&
            email->type.toLower() != "ssl"  && email->type.toLower() != "tls")) {
            qDebug() << "CONSOLE:Please enter valid port type!";
        } else {
            break;
        }
    }

    // Get the account name
    while (true) {
        email->account = getNewValue(email->account, tr("Enter the email account associated with the outgoing server"));

        // Check if the user wants to quit the action
        if (handleQuit(email->account, &quit) && quit) {
            return false;
        }

        if (email->account.isEmpty()) {
            qDebug() << "CONSOLE:Please enter a valid account!";
        } else {
            break;
        }
    }

    // Get the password
    while (true) {
        email->password = getNewValue(email->password, tr("Enter the account password associated with the outgoing server"));

        // Check if the user wants to quit the action
        if (handleQuit(email->password, &quit) && quit) {
            return false;
        }

        if (email->password.isEmpty()) {
            qDebug() << "CONSOLE:Please enter a valid password!";
        } else {
            break;
        }
    }

    // Get the sender's email address
    while (true) {
        email->sender = getNewValue(email->sender, tr("Enter the sender's email address"));

        // Check if the user wants to quit the action
        if (handleQuit(email->sender, &quit) && quit) {
            return false;
        }

        if (email->sender.isEmpty()) {
            qDebug() << "CONSOLE:Please enter a email address!";
        } else {
            break;
        }
    }

    // Get the recipient's email address
    while (true) {
        email->recipient = getNewValue(email->recipient, tr("Enter the recipient's email address"));

        // Check if the user wants to quit the action
        if (handleQuit(email->recipient, &quit) && quit) {
            return false;
        }

        if (email->recipient.isEmpty()) {
            qDebug() << "CONSOLE:Please enter a valid email address!";
        } else {
            break;
        }
    }

    return true;
}

bool CmdAdvanced::configSSL(WickrIOSSLSettings *ssl)
{
    QString temp;
    bool quit = false;

    // Get the SSL Key file name
    while (true) {
        temp = getNewValue(ssl->sslKeyFile, tr("Enter the SSL Key filename"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        if (! ssl->validateSSLKey(temp)) {
            qDebug() << "CONSOLE:Please enter a valid SSL Key filename!";
        } else {
            ssl->sslKeyFile = temp;
            break;
        }
    }

    // Get the SSL Certificate file name
    while (true) {
        temp = getNewValue(ssl->sslCertFile, tr("Enter the SSL Certificate filename"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        if (! ssl->validateSSLCert(temp)) {
            qDebug() << "CONSOLE:Please enter a valid SSL Certificate filename!";
        } else {
            ssl->sslCertFile = temp;
            break;
        }
    }
    return true;
}
