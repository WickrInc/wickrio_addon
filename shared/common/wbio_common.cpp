#include <QStandardPaths>
#include <QDir>

#include "wbio_common.h"
#include "wickrbotsettings.h"

/**
 * @brief WBIOCommon::makeDirectory
 * This function will create the input directory, if it does not already exist
 * @param dirname
 * @return
 */
bool WBIOCommon::makeDirectory(QString dirname)
{
    QDir tmp(dirname);
    if (!tmp.exists()) {
        if (!tmp.mkpath(".")) {
            return false;
        }
    }
    return true;
}

QString
WBIOCommon::getClientProcessName(QString name)
{
    QString processName;
    if (name.isEmpty()) {
        processName = WBIO_CLIENT_PROCESS;
    } else {
        processName = QString("%1.%2").arg(WBIO_CLIENT_PROCESS).arg(name);
    }
    return processName;
}
