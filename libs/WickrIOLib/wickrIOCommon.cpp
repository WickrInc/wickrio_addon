#include <QStandardPaths>
#include <QDir>

#include "wickrIOCommon.h".h"
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
