#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdoperation.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleClientHandler.h"

CmdOperation::CmdOperation(OperationData* operation, QObject *parent) :
    QObject(parent),
    m_operation(operation)
{
    m_appNm = WBIO_CLIENTSERVER_TARGET;
    m_settings = WBIOServerCommon::getSettings();
    m_dbLocation = WBIOServerCommon::getDBLocation();
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

    // If the IODatabase pointer is null but we have an m_operation then use that database
    if (m_ioDB == nullptr) {
        if (m_operation != nullptr && m_operation->m_botDB) {
            // Set a pointer to the WickrIO Database, in the parent class
            m_ioDB = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
        }
    }

    if (m_ioDB != nullptr && m_ioDB->isOpen()) {
        retVal = true;
    } else {
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
    }

    return retVal;
}
