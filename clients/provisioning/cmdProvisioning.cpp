#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "cmdProvisioning.h"

CmdProvisioning::CmdProvisioning()
{
}

/**
 * @brief CmdProvisioning::runCommands
 */
bool CmdProvisioning::runCommands()
{
    bool quitcheck;

    // Get the client type
    while (true) {
        QString clienttype;
        clienttype = getNewValue(m_clientType, tr("Enter the client type"));
        if (clienttype.toLower() == "cloud") {
            m_clientType = clienttype;
        } else if (clienttype.toLower() == "onprem") {
            m_clientType = clienttype;
        } else {
            qDebug() << "CONSOLE:Invalid client type, enter either cloud or onprem";
            continue;
        }
        break;
    }

    // For onprem get the config file information
    if (m_clientType == "onprem") {
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

        m_username = CmdBase::getNewValue(m_username, "Enter user name", CHECK_NONE);
        m_regToken = CmdBase::getNewValue(m_invitation, "Enter registration token", CHECK_NONE);
    } else {
        m_username = CmdBase::getNewValue(m_username, "Enter user name", CHECK_NONE);
        m_invitation = CmdBase::getNewValue(m_invitation, "Enter invitation", CHECK_NONE);
    }

    return true;
}

void CmdProvisioning::status()
{
}

