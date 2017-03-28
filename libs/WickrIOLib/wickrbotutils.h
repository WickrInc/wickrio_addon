#ifndef WICKRBOTUTILS_H
#define WICKRBOTUTILS_H

#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotUtils
{
public:
    WickrBotUtils();

    static QString fileInList(const QString &filename, const QStringList &searchList);
    static bool isRunning(const QString &appName, int pid);
    static void killProcess(int pid);
};

#endif // WICKRBOTUTILS_H
