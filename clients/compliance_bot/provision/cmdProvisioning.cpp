#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "cmdProvisioning.h"

#include "wbio_common.h"
#include "wickriotokens.h"

CmdProvisioning::CmdProvisioning(WickrIOClients *client) :
    m_client(client)
{
}

/**
 * @brief CmdProvisioning::runCommands
 */
bool CmdProvisioning::runCommands()
{

    bool quitcheck;

    // Need to have a valid compliance bot running


#if 1
    // Force client type to onprem for now
    m_client->onPrem = true;
#else
    // Get the client type
    while (true) {
        QString clienttype;
        clienttype = getNewValue(m_clientType, tr("Enter the client type"));
        if (clienttype.toLower() == "cloud") {
            m_clientType = clienttype;
            m_client->onPrem = false;
        } else if (clienttype.toLower() == "onprem") {
            m_clientType = clienttype;
            m_client->onPrem = true;
        } else {
            qDebug() << "CONSOLE:Invalid client type, enter either cloud or onprem";
            continue;
        }
        break;
    }
#endif

    // For onprem get the config file information
    if (m_client->onPrem) {
        while (true) {
            QString line = CmdBase::getNewValue(m_configFileName, "Enter config file", CHECK_FILE);
            if (CmdBase::handleQuit(line, &quitcheck)) {
                return false;
            }

            QFile confFile(line);
            QFileInfo info(confFile);

            if (confFile.exists() && info.isFile() && info.size() > 0) {
                m_configFileName = line;
                break;
            }
            qDebug() << "Config file does not exist!";
        }

        m_configPassword = CmdBase::getNewValue(m_configPassword, "Enter config password", CHECK_NONE);

        m_client->user = CmdBase::getNewValue(m_client->user, "Enter user name", CHECK_NONE);

        // Generate the password
        qDebug() << "********************************************************************";
        qDebug() << "**** GENERATED PASSWORD";
        qDebug() << "**** DO NOT LOOSE THIS PASSWORD, YOU WILL NEED TO ENTER IT EVERY TIME";
        qDebug() << "**** TO START THE BOT";
        qDebug() << "****";
        m_client->password = WickrIOTokens::getRandomString(24);
        qDebug() << m_client->password;
        qDebug() << "********************************************************************";

        // For now use the user name as the local name
        m_client->name = m_client->user;

    } else {
        m_client->user = CmdBase::getNewValue(m_client->user, "Enter user name", CHECK_NONE);
        m_client->password = CmdBase::getNewValue(m_client->password, "Enter user name", CHECK_NONE);
        m_invitation = CmdBase::getNewValue(m_invitation, "Enter invitation", CHECK_NONE);
    }

    return true;
}

void CmdProvisioning::status()
{
}

