#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdbase.h"
#include "wbio_common.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrioconsoleclienthandler.h"

CmdBase::CmdBase(QObject *parent) : QObject(parent)
{
}

/**
 * @brief CmdBase::handleQuit
 * This function will parse the input value. If the input value is "quit" then
 * the user will be prompted to verify they want to stop the current activity.
 * If the input value was "quit" a true value will be returned, and the user's
 * response to the prompt will set the input quit pointer based on whether the
 * user responds positively or not.
 * @param value The input value to check
 * @param quit Pointer to a boolean that identifies if the user wants to quit
 * @return Returns true if the input "value" is equal to "quit"
 */
bool CmdBase::handleQuit(const QString& value, bool *quit)
{
    *quit = false;

    // If there is a value input and it is equal to quit then return true
    if (!value.isEmpty() && value.toLower() == "quit") {
        while (true) {
            QString yORn = getNewValue("", "Do you really want to stop the current action? (y or n)");
            if (yORn.toLower() == "y") {
                *quit = true;
                return true;
            } else if (yORn.toLower() == "n") {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief CmdBase::getNewValue
 * Prompt user to imput a value. Display the old value if input.
 * @param oldValue
 * @param prompt
 * @param check
 * @return
 */
QString CmdBase::getNewValue(const QString& oldValue, const QString& prompt, CheckType check)
{
    QString newValue("");

    QTextStream s(stdin);

    QString lineInput;

    while (true) {
        if (oldValue.isEmpty()) {
            qDebug("CONSOLE:%s:", qPrintable(prompt));
        } else {
            qDebug("CONSOLE:%s (default: %s):", qPrintable(prompt), qPrintable(oldValue));
        }
        lineInput = s.readLine();
        if (!oldValue.isEmpty() && lineInput.isEmpty()) {
            newValue = oldValue;
            break;
        }
        newValue = lineInput.trimmed();

        if (newValue.isEmpty()) {
            qDebug() << "CONSOLE:Please enter a value!";
            continue;
        }

        if (check == CHECK_DIR) {
            QDir settingsDir;
            settingsDir.setPath(lineInput);
            if (settingsDir.exists()) {
                break;
            }
            qWarning("Directory does not exist!");
        } else if (check == CHECK_FILE) {
            if (!lineInput.isEmpty())
                break;
        } else if (check == CHECK_INT) {
            bool ok;
            lineInput.toInt(&ok);
            if (ok)
                break;
        } else if (check == CHECK_BOOL) {
            if (lineInput.toLower() == "yes" || lineInput.toLower() == "no") {
                newValue = lineInput.toLower();
                break;
            } else {
                qDebug() << "CONSOLE:Please enter 'yes' or 'no'";
            }
        } else {
            break;
        }
    }
    return newValue;
}
