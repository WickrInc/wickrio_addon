#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdusers.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleClientHandler.h"

CmdUsers::CmdUsers(CmdOperation *operation) :
    m_operation(operation)
{
}

CmdUsers::~CmdUsers()
{
}

/**
 * @brief CmdUsers::runConsoleCommands
 * This function will handle user input associated with the Users commands.
 */
bool CmdUsers::runCommands(QString commands)
{
    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    while (true) {
        QString line = getCommand("Enter users command:");
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();
            int cUserIndex;

            // Convert the second argument to an integer, for the user index commands
            if (args.size() > 1) {
                bool ok;
                cUserIndex = args.at(1).toInt(&ok);
                if (!ok) {
                    qDebug() << "CONSOLE:User Index is not a number!";
                    continue;
                }
            } else {
                cUserIndex = -1;
            }

            if (cmd == "?" || cmd == "help") {
                qDebug() << "CONSOLE:Users Commands:";
                qDebug() << "CONSOLE:  add        - adds a new user";
                qDebug() << "CONSOLE:  back       - leave the users commands";
                qDebug() << "CONSOLE:  delete <#> - deletes a user by index";
                qDebug() << "CONSOLE:  help or ?  - shows supported commands";
                qDebug() << "CONSOLE:  list       - shows the list of users";
                qDebug() << "CONSOLE:  modify <#> - modifies a user by index";
                qDebug() << "CONSOLE:  quit       - leaves the program";
            } else if (cmd == "add") {
                addUser();
            } else if (cmd == "back") {
                break;
            } else if (cmd == "delete") {
                if (cUserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: delete <index>";
                } else {
                    deleteUser(cUserIndex);
                }
            } else if (cmd == "list") {
                listUsers();
            } else if (cmd == "modify") {
                if (cUserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: modify <index>";
                } else {
                    modifyUser(cUserIndex);
                }
            } else if (cmd == "quit") {
                return false;
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
    return true;
}

void CmdUsers::status()
{
    listUsers();
}

bool CmdUsers::validateIndex(int index)
{
    if (index >= m_users.length() || index < 0) {
        qDebug() << "CONSOLE:The input user index is out of range!";
        return false;
    }
    return true;
}

/**
 * @brief CmdUsers::getUserValues
 * Prompt the user for the  user values. Do checking on the values as they
 * are entered to make sure they are valid.
 * @param user
 * @return
 */
bool CmdUsers::getUserValues(WickrIODBUser *user)
{
    bool quit = false;
    QString temp;
    WickrIODBUser newUser = *user;

    // Calculate which clients are Mother Bots (core_bot)
    QList<WickrBotClients *> mbClients = m_operation->m_ioDB->getMotherBotClients(true);
    QStringList motherBots;
    QString mbDesc = "Enter the appropriate number:\n";
    for (WickrBotClients* client : mbClients) {
        motherBots.append(QString::number(client->id));
        mbDesc += QString("  %1 for %2").arg(client->id).arg((client->name));
        delete client;
    }

    if (motherBots.size() == 0) {
        qDebug() << "CONSOLE:There are no Mother Bots currently configured!";
    } else {
        QString curMother = QString::number(user->m_motherBotID);
        temp = getNewValue(curMother, tr("Enter id of Mother Bot Client for this user"), CHECK_LIST, motherBots, mbDesc);

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        // Set the mother bot client id
        newUser.m_motherBotID = temp.toInt();
    }

    // Get the User name value.  Does not currently have to be unique
    while (true) {
        temp = getNewValue(user->m_user, tr("Enter the Wickr ID for this user"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        if (temp.isEmpty() || temp.length() < 4) {
            qDebug() << "CONSOLE:User name should be at least 4 characters long!";
        } else if (temp == user->m_user) {
            break;
        }

        WickrIODBUser newUser;
        if (m_operation->m_ioDB->getUser(temp, newUser.m_motherBotID, &newUser)) {
            qDebug() << "CONSOLE:WIckr ID is already in use!";
        } else {
            break;
        }
    }
    newUser.m_user = temp;

    // get max number of clients
    QString maxClients = getNewValue(QString::number(user->m_clientCount), tr("Enter max number of clients (0 for unlimited)"), CHECK_INT);
    // Check if the user wants to quit the action
    if (handleQuit(maxClients, &quit) && quit) {
        return false;
    }
    newUser.m_clientCount = maxClients.toInt();

    // Get the permissions
    QString yesString;

    yesString = user->isAdmin() ? "yes" : "no";
    temp = getNewValue(yesString, tr("Is this an administrator?"), CHECK_BOOL);
    if (handleQuit(temp, &quit) && quit) {
        return false;
    }
    newUser.setAdmin(temp == "yes");

    yesString = user->canEdit() ? "yes" : "no";
    temp = getNewValue(yesString, tr("Can edit Bot configurations?"), CHECK_BOOL);
    if (handleQuit(temp, &quit) && quit) {
        return false;
    }
    newUser.setEdit(temp == "yes");

    // Must be able to edit to have create capability
    if (user->canEdit()) {
        yesString = user->canCreate() ? "yes" : "no";
        temp = getNewValue(yesString, tr("Can create bots?"), CHECK_BOOL);
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
        newUser.setCreate(temp == "yes");
    } else {
        newUser.setCreate(false);
    }

    yesString = user->rxEvents() ? "yes" : "no";
    temp = getNewValue(yesString, tr("Can receive event messages?"), CHECK_BOOL);
    if (handleQuit(temp, &quit) && quit) {
        return false;
    }
    newUser.setRxEvents(temp == "yes");


    *user = newUser;
    return true;
}

/**
 * @brief CmdUsers::getUserClients
 * Update the list of clients associated with this user
 * @param user
 */
bool CmdUsers::getUserClients(WickrIODBUser *user)
{
    bool quit = false;

    // Calculate which clients are available
    QList<WickrBotClients *> clients = m_operation->m_ioDB->getMotherBotClients(false);
    QStringList clientList;
    QString mbDesc = "Possible clients:\n";
    for (WickrBotClients* client : clients) {
        clientList.append(QString::number(client->id));
        mbDesc += QString("  %1 for %2\n").arg(client->id).arg((client->name));
        delete client;
    }

    if (clientList.size() == 0) {
        qDebug() << "CONSOLE:There are no clients currently configured!";
    } else {
        QStringList curClientListString;
        QList<int> curClientList = m_operation->m_ioDB->getUserClients(user->m_id);
        for (int clientID : curClientList) {
            curClientListString.append(QString::number(clientID));
        }
        QString curClients = curClientListString.join(",");

        QString temp = getNewValue(curClients, tr("Enter IDs for associated clients"), CHECK_MULTI_LIST, clientList, mbDesc);

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        // set the associate clients
        QStringList entries = temp.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
        // First delete all current user to client references
        if (m_operation->m_ioDB->getUserClients(user->m_id).size() > 0 &&
                ! m_operation->m_ioDB->deleteUsersClients(user->m_id)) {
            qDebug() << "CONSOLE:Error deleting current user clients list";
        } else {
            // Add references for the new entries
            for (QString entry : entries) {
                int clientID = entry.toInt();
                m_operation->m_ioDB->insertUserClient(user->m_id, clientID);
            }
        }
    }
    return true;
}

/**
 * @brief CmdUsers::addUser
 * Add a new console user to the database
 */
void CmdUsers::addUser()
{
    WickrIODBUser *user = new WickrIODBUser();

    while (true) {
        if (!getUserValues(user)) {
            break;
        }

        if (m_operation->m_ioDB->insertUser(user)) {
            qDebug() << "CONSOLE:Successfully added record to the database!";
            if (!m_operation->m_ioDB->getUser(user->m_user, user->m_motherBotID, user)) {
                qDebug() << "CONSOLE:Failed to retrieve User record from the database!";
            } else {
                // Add clients for this user
                getUserClients(user);
            }
            break;
        } else {
            qDebug() << "CONSOLE:Could not add user!";
            // If the record was not added to the database then ask the user to try again
            QString response = getNewValue("", tr("Failed to add record, try again?"));
            if (response.isEmpty() || response.toLower() == "n") {
                delete user;
                return;
            }
        }
    }
    delete user;
}

void CmdUsers::deleteUser(int index)
{
    if (validateIndex(index)) {
        WickrIODBUser *user = m_users.at(index);

        // Make sure the user wants to continue
        QString prompt = QString(tr("Do you really want to remove the user %1")).arg(user->m_user);
        QString response = getNewValue("", prompt);
        if (response.toLower() != "y") {
            return;
        }

        if (!m_operation->m_ioDB->deleteUser(user->m_id)) {
            qDebug() << tr("CONSOLE:Failed to delete user's record for") << user->m_user;
        } else {
            qDebug() << tr("CONSOLE:Deleted user's record for") << user->m_user;
        }

        m_users = m_operation->m_ioDB->getUsers();
    }
}

void CmdUsers::listUsers()
{
    // Update the list of console users
    m_users = m_operation->m_ioDB->getUsers();

    if (m_users.length() == 0) {
        qDebug() << "CONSOLE:There are currently no Users configured!";
    } else {
        qDebug() << "CONSOLE:Users list:";
        int cnt=0;
        for (WickrIODBUser *user : m_users) {
            QString permissions;
            permissions = user->isAdmin() ? "admin" : "user";
            permissions += user->canCreate() ? ",create" : "";
            permissions += user->canEdit() ? ",edit" : "";
            permissions += user->rxEvents() ? ",events" : "";

            // Get the associated clients
            QList<int> clients = m_operation->m_ioDB->getUserClients(user->m_id);
            QStringList clientList;
            for (int cid : clients) {
                WickrBotClients *client = m_operation->m_ioDB->getClient(cid);
                QString clientString = QString("%1(%2)").arg(client->name).arg(cid);
                clientList.append(clientString);
                delete client;
            }

            WickrBotClients *client = m_operation->m_ioDB->getClient(user->m_motherBotID);
            QString motherBotString;
            if (client != nullptr) {
                motherBotString = QString("%1(%2)").arg(client->name).arg(user->m_motherBotID);
                delete client;
            } else {
                motherBotString = "not set";
            }

            QString data = QString("CONSOLE:  user[%1] Name=%2\n    Permissions=[%3]\n    MaxClients=%4\n    MotherBot=%5\n    Clients=[%6]")
                    .arg(cnt++)
                    .arg(user->m_user)
                    .arg(permissions)
                    .arg(user->m_clientCount)
                    .arg(motherBotString)
                    .arg(clientList.join(","));
            qDebug() << qPrintable(data);
        }
    }
}

void CmdUsers::modifyUser(int index)
{
    if (validateIndex(index)) {
        WickrIODBUser *user;
        user = m_users.at(index);

        WickrIODBUser newUser;
        newUser = *user;

        while (true) {
            if (!getUserValues(&newUser)) {
                break;
            }

            if (! m_operation->m_ioDB->updateUser(&newUser)) {
                // If the record was not updated to the database then ask the user to try again
                QString response = getNewValue("", tr("Failed to update record, try again?"));
                if (response.isEmpty() || response.toLower() == "n") {
                    return;
                }
            } else {
                qDebug() << "CONSOLE:Successfully updated user record!";

                // Update the list of clients associated with this user
                getUserClients(user);
                break;
            }
        }
        m_users = m_operation->m_ioDB->getUsers();
    }
}
