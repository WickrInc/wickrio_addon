#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdoperation.h"
#include "wbio_common.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrioconsoleclienthandler.h"

CmdOperation::CmdOperation(QObject *parent) : QObject(parent)
{
    m_appNm = WBIO_CLIENTSERVER_TARGET;
    m_settings = WBIOCommon::getSettings();
    m_dbLocation = WBIOCommon::getDBLocation();
    m_ipc = new WickrBotIPC();
    m_ioDB = NULL;
}

/**
 * @brief CmdOperation::openDatabase
 * This function will open the database that is located at the directory
 * identified by the m_dbLocation variable.
 * @return true if the database was opened, false if not
 */
bool CmdOperation::openDatabase()
{
    bool retVal = false;

    // If there is a db location then try to open it
    if (!m_dbLocation.isEmpty()) {
        QDir dbDir(m_dbLocation);
        if (!dbDir.exists()) {
            qDebug() << "CONSOLE:DB location does not exist!";
        } else {
            m_ioDB = new WickrIOClientDatabase(m_dbLocation);
            if (!m_ioDB->isOpen()) {
                qDebug() << "CONSOLE:Cannot open database!";
            } else {
                retVal = true;
            }
        }
    } else {
        qDebug() << "CONSOLE:No DB location specified!";
    }

    return retVal;
}
