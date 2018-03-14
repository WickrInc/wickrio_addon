#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdconsole.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleClientHandler.h"

CmdConsole::CmdConsole(CmdOperation *operation) :
    m_operation(operation)
{
    m_consoleServer = NULL;
}

CmdConsole::~CmdConsole()
{
    if (m_consoleServer != NULL) {
        delete m_consoleServer;
    }
}

/**
 * @brief CmdConsole::runConsoleCommands
 * This function will handle user input associated with the Console commands.
 */
bool CmdConsole::runCommands(QString commands)
{
    if (m_consoleServer == NULL) {
        m_consoleServer = new ConsoleServer(m_operation->m_ioDB);
    }
    QTextStream input(stdin);

    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    // Get the current SSL Settings
    if (m_operation->m_settings->childGroups().contains(WBSETTINGS_SSL_HEADER)) {
        // Get the email settings from the settings file
        m_operation->m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
        m_sslSettings.readFromSettings(m_operation->m_settings);
        m_operation->m_settings->endGroup();
    } else {
        m_sslSettings.sslKeyFile = "";
        m_sslSettings.sslCertFile = "";
    }

    while (true) {
        qDebug() << "CONSOLE:Enter console command:";
        QString line = input.readLine();

        line = line.trimmed();
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();
            int cUserIndex;

            // Convert the second argument to an integer, for the console user index commands
            if (args.size() > 1) {
                bool ok;
                cUserIndex = args.at(1).toInt(&ok);
                if (!ok) {
                    qDebug() << "CONSOLE:Console User Index is not a number!";
                    continue;
                }
            } else {
                cUserIndex = -1;
            }

            if (cmd == "?" || cmd == "help") {
                qDebug() << "CONSOLE:Console Commands:";
                qDebug() << "CONSOLE:  add        - adds a new console user";
                qDebug() << "CONSOLE:  back       - leave the console commands";
                qDebug() << "CONSOLE:  config     - configures the console service";
                qDebug() << "CONSOLE:  delete <#> - deletes the console user by index";
                qDebug() << "CONSOLE:  help or ?  - shows supported commands";
                qDebug() << "CONSOLE:  list       - shows the list of console users";
                qDebug() << "CONSOLE:  modify <#> - modifies the console user by index";
                qDebug() << "CONSOLE:  quit       - leaves the program";
                qDebug() << "CONSOLE:  status     - shows the state of the console service";

                if (m_consoleServer->isRunning()) {
                    qDebug() << "CONSOLE:  stop       - stops the console service";
                } else {
                    qDebug() << "CONSOLE:  start      - starts the console service";
                }
            } else if (cmd == "add") {
                addConsoleUser();
            } else if (cmd == "back") {
                break;
            } else if (cmd == "config") {
                configConsole();
            } else if (cmd == "delete") {
                if (cUserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: delete <index>";
                } else {
                    deleteConsoleUser(cUserIndex);
                }
            } else if (cmd == "list") {
                listConsoleUsers();
            } else if (cmd == "modify") {
                if (cUserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: modify <index>";
                } else {
                    modifyConsoleUser(cUserIndex);
                }
            } else if (cmd == "status") {
                status();
            } else if (cmd == "start") {
                m_consoleServer->setState(true);
            } else if (cmd == "stop") {
                m_consoleServer->setState(false);
            } else if (cmd == "quit") {
                return false;
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
    return true;
}

/**
 * @brief CmdConsole::status
 * This is a public function that will print out all of the status information for the
 * console record(s).
 */
void CmdConsole::status()
{
    m_operation->m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
    QString type = m_operation->m_settings->value(WBSETTINGS_CONSOLESVR_TYPE,"http").toString();
    QString iface = m_operation->m_settings->value(WBSETTINGS_CONSOLESVR_IF,"localhost").toString();
    int portNumber = m_operation->m_settings->value(WBSETTINGS_CONSOLESVR_PORT,0).toInt();
    m_operation->m_settings->endGroup();

    QString clientState = WickrIOConsoleClientHandler::getActualProcessState(WBIO_CONSOLESERVER_TARGET, m_operation->m_ioDB);
    qDebug() << "CONSOLE:Console Server status:";
    qDebug() << "CONSOLE:  Type       :" << type;
    qDebug() << "CONSOLE:  Interface  :" << iface;
    qDebug() << "CONSOLE:  Port       :" << portNumber;
    qDebug() << "CONSOLE:  State      :" << clientState;
    listConsoleUsers();
}

/**
 * @brief CmdConsole::configConsole
 * This function will configure the console settings
 */
void CmdConsole::configConsole()
{
    bool quit = false;

    m_operation->m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
    QString type = m_operation->m_settings->value(WBSETTINGS_CONSOLESVR_TYPE,"http").toString();
    QString iface = m_operation->m_settings->value(WBSETTINGS_CONSOLESVR_IF,"localhost").toString();
    int portNumber = m_operation->m_settings->value(WBSETTINGS_CONSOLESVR_PORT,0).toInt();
    m_operation->m_settings->endGroup();

    bool isHttps = (type.toLower() == "https");

    // Get the list of possible interfaces
    QStringList ifaceList = WickrIOConsoleClientHandler::getNetworkInterfaceList();

    // Get a unique interface and port pair
    while (true) {
        if (ifaceList.size() == 1) {
            iface = ifaceList.at(0);
        } else {
            QString temp = getNewValue(iface, tr("Enter the interface"));

            // Check if the user wants to quit the action
            if (handleQuit(temp, &quit) && quit) {
                return;
            }

            if (temp.toLower() == "list" || temp == "?") {
                foreach (QString i, ifaceList) {
                    qDebug() << "CONSOLE:" << i;
                }
                continue;
            }

            if (!ifaceList.contains(temp)) {
                qDebug() << "CONSOLE:" << temp << "not found in the list of interfaces!";
                continue;
            }

            iface =  temp;
        }

        QString port = getNewValue(QString::number(portNumber), tr("Enter the IP port number"), CHECK_INT);

        // Check if the user wants to quit the action
        if (handleQuit(port, &quit) && quit) {
            return;
        }

        portNumber = port.toInt();

        if (chkInterfaceExists(iface, portNumber)) {
            qDebug() << "CONSOLE:Interface and port combination used already!";
        } else {
            break;
        }
    }

        // Get the interface type (HTTP or HTTPS)
    while (true) {
        QString ifacetype;
        ifacetype = getNewValue(type, tr("Enter the interface type"));
        // Check if the user wants to quit the action
        if (handleQuit(ifacetype, &quit) && quit) {
            return;
        }

        if (ifacetype.toLower() == "https") {
            type = "https";
            isHttps = true;
        } else if (ifacetype.toLower() == "http") {
            type = "http";
            isHttps = false;
        } else {
            qDebug() << "CONSOLE:Invalid interface type, enter either HTTP or HTTPS";
            continue;
        }
        break;
    }

    // Get the SSL Key and Cert values
    if (isHttps) {
        if (m_sslSettings.sslKeyFile.isEmpty() || m_sslSettings.sslCertFile.isEmpty()) {
            qDebug() << "CONSOLE:WARNING: SSL has not been setup! Go to the Advanced settings!";
        }
    }


    // Update the values in the settings file
    m_operation->m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
    m_operation->m_settings->setValue(WBSETTINGS_CONSOLESVR_TYPE, type);
    // If using localhost interface then do not need the host entry in the settings
    if (iface == "localhost") {
        m_operation->m_settings->remove(WBSETTINGS_CONSOLESVR_IF);
    } else {
        m_operation->m_settings->setValue(WBSETTINGS_CONSOLESVR_IF, iface);
    }
    m_operation->m_settings->setValue(WBSETTINGS_CONSOLESVR_PORT, portNumber);
    if (isHttps) {
        m_operation->m_settings->setValue(WBSETTINGS_CONSOLESVR_SSLKEY, m_sslSettings.sslKeyFile);
        m_operation->m_settings->setValue(WBSETTINGS_CONSOLESVR_SSLCERT, m_sslSettings.sslCertFile);
    } else {
        m_operation->m_settings->remove(WBSETTINGS_CONSOLESVR_SSLKEY);
        m_operation->m_settings->remove(WBSETTINGS_CONSOLESVR_SSLCERT);
    }
    m_operation->m_settings->endGroup();
    m_operation->m_settings->sync();
}


bool CmdConsole::validateIndex(int consoleUserIndex)
{
    if (consoleUserIndex >= m_consoleUsers.length() || consoleUserIndex < 0) {
        qDebug() << "CONSOLE:The input console user index is out of range!";
        return false;
    }
    return true;
}

/**
 * @brief CmdConsole::getConsoleUserValues
 * Prompt the user for the console user values. Do checking on the values as they
 * are entered to make sure they are valid.
 * @param cuser
 * @return
 */
bool CmdConsole::getConsoleUserValues(WickrIOConsoleUser *cuser)
{
    bool quit = false;
    QString temp;
    WickrIOConsoleUser newCUser = *cuser;

    // Get the User name value.  Does not currently have to be unique
    while (true) {
        temp = getNewValue(cuser->user, tr("Enter the User name"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        if (temp.isEmpty() || temp.length() < 4) {
            qDebug() << "CONSOLE:User name should be at least 4 characters long!";
        } else if (temp == cuser->user) {
            break;
        }

        WickrIOConsoleUser newUser;
        if (m_operation->m_ioDB->getConsoleUser(temp, &newUser)) {
            qDebug() << "CONSOLE:User name is already in use!";
        } else {
            break;
        }
    }
    newCUser.user = temp;

    // Get the password value.  Does not currently have to be unique
    while (true) {
        temp = getNewValue(cuser->password, tr("Enter the user's password"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        if (temp.isEmpty() || temp.length() < 4) {
            qDebug() << "CONSOLE:Password should be at least 4 characters long!";
        } else {
            break;
        }
    }
    newCUser.password = temp;

    // get max number of clients
    QString maxClients = getNewValue(QString::number(cuser->maxclients), tr("Enter max number of clients (0 for unlimited)"), CHECK_INT);
    // Check if the user wants to quit the action
    if (handleQuit(maxClients, &quit) && quit) {
        return false;
    }
    newCUser.maxclients = maxClients.toInt();

    // Get the permissions
    QString yesString;

    yesString = cuser->isAdmin() ? "yes" : "no";
    temp = getNewValue(yesString, tr("Is this an administrator?"), CHECK_BOOL);
    if (handleQuit(temp, &quit) && quit) {
        return false;
    }
    newCUser.setAdmin(temp == "yes");

    yesString = cuser->canEdit() ? "yes" : "no";
    temp = getNewValue(yesString, tr("Can edit Bot configurations?"), CHECK_BOOL);
    if (handleQuit(temp, &quit) && quit) {
        return false;
    }
    newCUser.setEdit(temp == "yes");

    // Must be able to edit to have create capability
    if (cuser->canEdit()) {
        yesString = cuser->canCreate() ? "yes" : "no";
        temp = getNewValue(yesString, tr("Can create bots?"), CHECK_BOOL);
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
        newCUser.setCreate(temp == "yes");
    } else {
        newCUser.setCreate(false);
    }

    yesString = cuser->rxEvents() ? "yes" : "no";
    temp = getNewValue(yesString, tr("Can receive event messages?"), CHECK_BOOL);
    if (handleQuit(temp, &quit) && quit) {
        return false;
    }
    newCUser.setRxEvents(temp == "yes");

    // Authentication Settings

    // Get the authentication type (Basic or Email)
    while (true) {
        QString authType;
        authType = getNewValue(cuser->getAuthTypeStr(), tr("Enter the authentication type"));
        // Check if the user wants to quit the action
        if (handleQuit(maxClients, &quit) && quit) {
            return false;
        }
        if (authType == "?") {
            qDebug() << "CONSOLE:Valid authentication types, enter either Basic or Email";
            continue;
        } else if (authType.toLower() == "basic") {
            newCUser.authType = CUSER_AUTHTYPE_BASIC;
        } else if (authType.toLower() == "email") {
            newCUser.authType = CUSER_AUTHTYPE_EMAIL;
        } else {
            qDebug() << "CONSOLE:Invalid authentication type, enter either Basic or Email";
            continue;
        }
        break;
    }

    // Get the Email value.  Does not currently have to be unique
    while (true) {
        temp = getNewValue(cuser->email, tr("Enter the user's email address"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        // Must enter an email if auttype is EMail
        if (newCUser.authType == CUSER_AUTHTYPE_EMAIL && (temp.isEmpty() || temp.length() < 4)) {
            qDebug() << "CONSOLE:Email should be at least 4 characters long!";
        } else {
            break;
        }
    }
    newCUser.email = temp;

    QString tokenStr;
    if (cuser->token.isEmpty()) {
        tokenStr = WickrIOTokens::getRandomString(TOKEN_LENGTH);
    } else {
        tokenStr = cuser->token;
    }
    // Get the Token value
    while (true) {
        temp = getNewValue(tokenStr, tr("Enter the Token for this user"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        if (temp.isEmpty() || temp.length() < 10) {
            qDebug() << "CONSOLE:Token should be at least 10 characters long!";
        } else {
            break;
        }
    }
    newCUser.token = temp;


    *cuser = newCUser;
    return true;
}

/**
 * @brief CmdConsole::addConsoleUser
 * Add a new console user to the database
 */
void CmdConsole::addConsoleUser()
{
    WickrIOConsoleUser *cuser = new WickrIOConsoleUser();

    while (true) {
        if (!getConsoleUserValues(cuser)) {
            break;
        }

        if (m_operation->m_ioDB->insertConsoleUser(cuser)) {
            qDebug() << "CONSOLE:Successfully added record to the database!";
            if (!m_operation->m_ioDB->getConsoleUser(cuser->user, cuser)) {
                qDebug() << "CONSOLE:Failed to retrieve Console User record from the database!";
            } else if (! m_operation->m_ioDB->insertToken(cuser->token, cuser->id, "*")) {
                qDebug() << "CONSOLE:Failed to add Token record to the database!";
            }
            break;
        } else {
            qDebug() << "CONSOLE:Could not add console user!";
            // If the record was not added to the database then ask the user to try again
            QString response = getNewValue("", tr("Failed to add record, try again?"));
            if (response.isEmpty() || response.toLower() == "n") {
                delete cuser;
                return;
            }
        }
    }
    delete cuser;
}

void CmdConsole::deleteConsoleUser(int consoleUserIndex)
{
    if (validateIndex(consoleUserIndex)) {
        WickrIOConsoleUser *cuser = m_consoleUsers.at(consoleUserIndex);

        // Make sure the user wants to continue
        QString prompt = QString(tr("Do you really want to remove the console user %1")).arg(cuser->user);
        QString response = getNewValue("", prompt);
        if (response.toLower() != "y") {
            return;
        }

        if (!m_operation->m_ioDB->deleteConsoleUser(cuser->user)) {
            qDebug() << tr("Failed to delete console user's record for") << cuser->user;
        } else {
            QString prompt = QString(tr("Delete clients linked to console user %1")).arg(cuser->user);
            QString response = getNewValue("", prompt);
            if (response.toLower() == "y") {
                m_operation->m_ioDB->deleteClientsOfConsoleUser(cuser->id);
            } else {
                m_operation->m_ioDB->clearClientsOfConsoleUser(cuser->id);
            }
        }

        m_consoleUsers = m_operation->m_ioDB->getConsoleUsers();
    }
}

void CmdConsole::listConsoleUsers()
{
    // Update the list of console users
    m_consoleUsers = m_operation->m_ioDB->getConsoleUsers();

    if (m_consoleUsers.length() == 0) {
        qDebug() << "CONSOLE:There are currently no Console Users configured!";
    } else {
        qDebug() << "CONSOLE:Console Users list:";
    }

    int cnt=0;
    for (WickrIOConsoleUser *cuser : m_consoleUsers) {
        QString permissions;
        permissions = cuser->isAdmin() ? "admin" : "user";
        permissions += cuser->canCreate() ? ",create" : "";
        permissions += cuser->canEdit() ? ",edit" : "";
        permissions += cuser->rxEvents() ? ",events" : "";

        // Get any associated token(s)
        WickrIOTokens token;
        QString tokenStr;
        if (m_operation->m_ioDB->getToken(cuser->id, &token)) {
            tokenStr = token.token;
        } else {
            tokenStr = "Not set!";
        }

        QString data = QString("CONSOLE:  user[%1] User=%2, Permissons=[%3], MaxClients=%4, Token=%5")
            .arg(cnt++)
            .arg(cuser->user)
            .arg(permissions)
            .arg(cuser->maxclients)
            .arg(tokenStr);
        qDebug() << qPrintable(data);
    }
}

void CmdConsole::modifyConsoleUser(int consoleUserIndex)
{
    if (validateIndex(consoleUserIndex)) {
        WickrIOConsoleUser *cuser;
        cuser = m_consoleUsers.at(consoleUserIndex);
        WickrIOTokens token;
        if (m_operation->m_ioDB->getConsoleUserToken(cuser->id, &token)) {
            cuser->token = token.token;
        }

        WickrIOConsoleUser newCUser;
        newCUser = *cuser;

        while (true) {
            if (!getConsoleUserValues(&newCUser)) {
                break;
            }

            if (! m_operation->m_ioDB->updateConsoleUser(&newCUser)) {
                // If the record was not updated to the database then ask the user to try again
                QString response = getNewValue("", tr("Failed to update record, try again?"));
                if (response.isEmpty() || response.toLower() == "n") {
                    return;
                }
            } else {
                // If the tokens changed then update the token record
                if (newCUser.token != cuser->token) {
                    WickrIOTokens token;
                    if (! m_operation->m_ioDB->getConsoleUserToken(cuser->id, &token)) {
                        qDebug() << "CONSOLE:Could not get old Token record!";
                        if (! m_operation->m_ioDB->insertToken(newCUser.token, newCUser.id, "*")) {
                            qDebug() << "CONSOLE:Could not insert new Token record!";
                        }
                    } else {
                        token.token = newCUser.token;
                        if (! m_operation->m_ioDB->updateToken(&token)) {
                            qDebug() << "CONSOLE:Could not update old Token record!";
                        }
                    }
                }
                qDebug() << "CONSOLE:Successfully updated record to the database!";
                break;
            }
        }
        m_consoleUsers = m_operation->m_ioDB->getConsoleUsers();
    }
}

bool CmdConsole::chkInterfaceExists(const QString& iface, int port)
{
#if 0
    for (WickrBotClients *client : m_clients) {
        if (client->iface == iface && client->port == port) {
            qDebug() << "CONSOLE:The input interface and port are NOT unique!";
            return true;
        }
    }
#else
    Q_UNUSED(iface);
    Q_UNUSED(port);
#endif
    return false;
}
