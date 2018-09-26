#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>

#include "ipcSendMessage.h"

int main(int argc, char *argv[])
{

#ifdef Q_OS_LINUX
#ifdef WICKR_BETA
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-beta");
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-beta/plugins");
#elif defined(WICKR_ALPHA)
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-alpha");
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-alpha/plugins");
#else
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot");
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot/plugins");
#endif
#endif

    QCoreApplication a(argc, argv);
    QString appName;

    a.setOrganizationName("Wickr, LLC");

    IpcSendMessage ipcSend;
    ipcSend.sendMessage("shutdown");

    int retval = a.exec();

    return retval;
}

