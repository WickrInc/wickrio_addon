#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include <readline/readline.h>
#include <readline/history.h>

#include "cmdbase.h"
#include "wickrIOCommon.h"

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

#include <unistd.h>   //_getch*/
#include <termios.h>  //_getch*

char getch(){
    /*#include <unistd.h>   //_getch*/
    /*#include <termios.h>  //_getch*/
    char buf=0;
    struct termios old={0};
    fflush(stdout);
    if(tcgetattr(0, &old)<0)
        perror("tcsetattr()");
    old.c_lflag&=~ICANON;
    old.c_lflag&=~ECHO;
    old.c_cc[VMIN]=1;
    old.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &old)<0)
        perror("tcsetattr ICANON");
    if(read(0,&buf,1)<0)
        perror("read()");
    old.c_lflag|=ICANON;
    old.c_lflag|=ECHO;
    if(tcsetattr(0, TCSADRAIN, &old)<0)
        perror ("tcsetattr ~ICANON");
//    printf("%c\n",buf);
    return buf;
 }

#include    <iostream>

QString CmdBase::getPassword(const QString& prompt)
{
    qDebug("CONSOLE:%s:", qPrintable(prompt));

    QChar   pass[128];
    int     i=0;

    pass[0]=getch();
    while(pass[i]!=13 && pass[i]!=10)
    {
        if (pass[i].isLetterOrNumber()) {
            i++;
            std::cout<<"*";
        } else if (pass[i] == '\b' || pass[i] == 127) {
            if (i > 0) {
                std::cout<<"\b \b";
                i--;
            }
        }

        pass[i]=getch();
    }

    std::cout<<"\n";

    QString password = QString(pass, i);
    return password;
}

QString CmdBase::getCommand(const QString& prompt)
{
    QString lineInput;

    while (true) {
        char *response = readline(prompt.toStdString().c_str());

        if (response != nullptr) {
            lineInput = QString(response);
            free(response);
        }

        lineInput = lineInput.trimmed();

        if (lineInput.length() > 0) {
            add_history(lineInput.toStdString().c_str());
        }

        if (lineInput != "history")
            break;

        // Handle history command
        HIST_ENTRY **historyList = history_list();

        if (historyList) {
            for (int i = 0; historyList[i]; i++)
              printf ("%d: %s\n", i + history_base, historyList[i]->line);
        }
    }
    return lineInput;
}

/**
 * @brief CmdBase::getNewValue
 * Prompt user to imput a value. Display the old value if input.
 * @param oldValue
 * @param prompt
 * @param check
 * @return
 */
QString CmdBase::getNewValue(const QString& oldValue, const QString& prompt, CheckType check, QStringList choices, QString listDesc)
{
    QString newValue("");
    QString lineInput;    

    while (true) {
        QString promptString;
        if (oldValue.isEmpty()) {
            promptString = QString("%1:").arg(prompt);
        } else {
            promptString = QString("%1 (default: %2):").arg(prompt).arg(oldValue);
        }

        char *response = readline(promptString.toStdString().c_str());

        if (response == nullptr || response[0] == NULL) {
            lineInput = QString();
        } else {
            lineInput = QString(response);
        }

        if (response != nullptr)
            free(response);

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
        } else if (check == CHECK_LIST) {
            bool found = false;
            for (QString entry : choices) {
                if (entry.toLower() == lineInput.toLower()) {
                    newValue = entry;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
            if (listDesc.isEmpty()) {
                qDebug() << "CONSOLE:Please enter one of" << choices;
            } else {
                qDebug().noquote().nospace() << "CONSOLE:" << listDesc;
            }
        } else if (check == CHECK_MULTI_LIST) {
            QStringList entries = lineInput.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
            bool allChoicesGood=true;
            for (QString entry : entries) {
                bool goodChoices=false;
                for (QString choice : choices) {
                    if (entry.toLower() == choice.toLower()) {
                        goodChoices=true;
                        break;
                    }
                }
                if (!goodChoices) {
                    qDebug().noquote().nospace() << "CONSOLE:Entry " << entry << " not found!";
                    allChoicesGood = false;
                }
            }
            if (allChoicesGood)
                break;
            if (listDesc.isEmpty()) {
                qDebug() << "CONSOLE:Please enter from list" << choices;
            } else {
                qDebug().noquote().nospace() << "CONSOLE:" << listDesc;
            }
        } else {
            break;
        }
    }
    return newValue;
}
