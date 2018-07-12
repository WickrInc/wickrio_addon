#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "cmdProvisioning.h"

#include "wickrIOCommon.h"
#include "wickriotokens.h"

CmdProvisioning::CmdProvisioning(WickrBotClients *client) :
    m_client(client)
{
}

/**
 * @brief CmdProvisioning::runCommands
 */
bool CmdProvisioning::runCommands(int argc, char *argv[])
{

    bool quitcheck;

    // Need to have a valid wickrIO bot running


#if defined(WICKR_ENTERPRISE)
    // Force client type to onprem for now
    m_client->onPrem = true;
#elif defined(WICKR_MESSENGER)
    // Force client type to onprem for now
    m_client->onPrem = false;
#else
    // Force client type to onprem for now
    m_client->onPrem = false;
#endif

    // Some default WIO stuff
    m_client->apiKey = WickrIOTokens::getRandomString(16);

    // See if the command line contains the values
    int argidx;
    for( argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);
        if (!cmd.startsWith("-")) {
            break;
        }

        //TBD: Parse options here
    }

    if (argidx < argc) {
        int numargs = argc - argidx;
        if (m_client->onPrem) {
            if (numargs != 4) {
                qDebug() << "CONSOLE:Not enough arguments for onPrem!";
                qDebug() << "CONSOLE:Usage: wickrio_botProv <config File> <config pw> <username> <password>";
                return false;
            }
            m_configFileName = QString(argv[argidx]);
            m_configPassword = QString(argv[argidx+1]);
            m_client->user = QString(argv[argidx+2]);
            m_client->password = QString(argv[argidx+3]);
            m_client->name = m_client->user;
            m_client->name.replace("@", "_");
        } else {
            if (numargs != 2) {
                qDebug() << "CONSOLE:Not enough arguments for user provisioning!";
                qDebug() << "CONSOLE:Usage: wickrio_botProv <username> <password>";
                return false;
            }
            m_client->user = QString(argv[argidx]);
            m_client->password = QString(argv[argidx+1]);
            m_client->name = m_client->user;
            m_client->name.replace("@", "_");
        }
        qDebug() << "CONSOLE:Creating user: " << m_client->name;
    } else {

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
                qDebug() << "CONSOLE:Config file does not exist!";
            }

            m_configPassword = CmdBase::getNewValue(m_configPassword, "Enter config password", CHECK_NONE);

            m_client->user = CmdBase::getNewValue(m_client->user, "Enter user name", CHECK_NONE);
            m_client->password = CmdBase::getNewValue(m_client->password, "Enter initial password", CHECK_NONE);

            // For now use the user name as the local name
            m_client->name = m_client->user;
            m_client->name.replace("@", "_");
        } else {
            if (m_client->user.isEmpty())
                m_client->user = CmdBase::getNewValue(m_client->user, "Enter user name", CHECK_NONE);
            if (m_client->password.isEmpty())
                m_client->password = CmdBase::getNewValue(m_client->password, "Enter password", CHECK_NONE);

            // For now use the user name as the local name
            m_client->name = m_client->user;
            m_client->name.replace("@", "_");
        }
    }

    return true;
}

void CmdProvisioning::status()
{
}

