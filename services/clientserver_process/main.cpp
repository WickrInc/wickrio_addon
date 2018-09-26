#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QThread>
#include <QSettings>
#include <QStandardPaths>
#include <QProcess>
#include <QtPlugin>

#include <QSqlQuery>
#include <QSqlError>

#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <readline/readline.h>
#include <iostream>
#include <termios.h>

#include "operationdata.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotutils.h"
#include "wickrbotsettings.h"
#include "loghandler.h"

#define DO_AS_SERVICE 1

#include "wickrioclientserverprocess.h"
#include "wickrioprocesscommand.h"
#include "wickrIOIPCRuntime.h"

#ifdef Q_OS_LINUX
WickrIOClientServerProcess *curService;
#endif

LogHandler logs;

void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    if (type == QtMsgType::QtDebugMsg && str.startsWith("CONSOLE:")) {
        QString outstr = str.right(str.length()-8);
        QTextStream(stdout) << outstr << endl;
    } else {

        logs.output(str);
        if (type == QtFatalMsg) {
            abort();
        }
    }
}

#ifdef Q_OS_LINUX
void catchUnixSignals(const std::vector<int>& quitSignals,
                      const std::vector<int>& ignoreSignals = std::vector<int>()) {

    auto handler = [](int sig) ->void {
        qDebug() << "\nquit the application: user request signal = " << sig;
        QCoreApplication::quit();
    };

    // all these signals will be ignored.
    for ( int sig : ignoreSignals )
        signal(sig, SIG_IGN);

    // each of these signals calls the handler (quits the QCoreApplication).
    for ( int sig : quitSignals )
        signal(sig, handler);
}
#endif


/**
 * @brief acceptLicense
 * This function will display the associated license if one exists
 * @return true if the license is accepted
 */

bool acceptLicense()
{
    bool acceptLicense = false;

    // Before proceeding make sure the license has been approved
    QString licenseFileName = "/usr/share/doc/wickr/Wickr_api_bot_license.txt";
    QFile   licenseFile(licenseFileName);
    if (!licenseFile.exists())
        return true;

    if (licenseFile.open(QIODevice::ReadOnly))
    {
        QString savedLine;
        struct termios oldTermios, newTermios;
        tcgetattr(STDIN_FILENO, &oldTermios);
        newTermios = oldTermios;
        cfmakeraw(&newTermios);


        QTextStream in(&licenseFile);
        while (!in.atEnd()) {
            // Calculate the number of lines that can be displayed
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            int lines = w.ws_row - 1;   // Account for the continue statement
            int cols = w.ws_col;

            bool done=false;

            // If there are any lines left over from the previous buffer
            if (!savedLine.isEmpty()) {
                qDebug().noquote() << savedLine;
                lines -= (savedLine.length() / cols) + 1;
                savedLine.clear();
            }

            for (int i=0; i<lines; i++) {
                if (in.atEnd()) {
                    done=true;
                    break;
                }
                QString line = in.readLine();

                if (line.length() > cols) {
                    int useLines = line.length() / cols;
                    if (i+useLines >= lines) {
                        savedLine=line;
                        break;
                    }
                    i += useLines;
                }
                qDebug().noquote() << line;
            }

            if (!done) {
                tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
                fflush(stdin);
                std::cout << "Enter any character to continue:";
                char c = getchar();
                std::cout << "\r                                \r";
                tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
            }
        }
        licenseFile.close();

        // See if the user accepts the license agreement
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
        bool badResponse = true;
        while (badResponse) {
            fflush(stdin);
            std::cout << "\n\rDo you accept the Terms and Conditions set forth above? (Y/N)";
            char c = (char)getchar();
            std::cout << "\n\r";
            QChar qchar = c;
            if (qchar.toLower() == 'y') {
                acceptLicense = true;
                break;
            }
            if (qchar.toLower() == 'n') {
                acceptLicense = false;
                break;
            }
        }
        fflush(stdin);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
    }
    return acceptLicense;
}

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    QString dirname = QString("/opt/%1/logs").arg(WBIO_GENERAL_TARGET);
    QString filename = QString("%1/%2.output").arg(dirname).arg(WBIO_CLIENTSERVER_TARGET);
    QString logname =  QString("%1/%2.log").arg(dirname).arg(WBIO_CLIENTSERVER_TARGET);

    QSettings *settings = WBIOServerCommon::getSettings();
    settings->beginGroup(WBSETTINGS_LICENSE_HEADER);
    bool licenseAccpeted = settings->value(WBSETTINGS_LICENSE_ACCEPTED, false).toBool();
    settings->endGroup();

    if (!licenseAccpeted) {
        if (!acceptLicense()) {
            qDebug().noquote() << "Cannot continue!";
            exit(1);
        }
        settings->beginGroup(WBSETTINGS_LICENSE_HEADER);
        settings->setValue(WBSETTINGS_LICENSE_ACCEPTED, true);
        settings->endGroup();
        settings->sync();
    }

    bool debugOutput = false;
    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if (cmd == "-debug") {
            debugOutput = true;
        }
    }

    WBIOCommon::makeDirectory(dirname);
    logs.setupLog(logname);
    logs.logSetOutput(filename);

    if (!debugOutput)
        qInstallMessageHandler(redirectedOutput);
    int svcret;

    catchUnixSignals({SIGTSTP, SIGQUIT, SIGTERM});

    QCoreApplication *app = new QCoreApplication(argc, argv);

    OperationData *pOperation = new OperationData();
    pOperation->processName = WBIO_CLIENTSERVER_TARGET;

    WickrIOIPCRuntime::init(WBIO_CLIENTSERVER_TARGET, false);

    WICKRIOCLIENTSERVERPROCESS = new WickrIOClientServerProcess(pOperation);
    WICKRIOCLIENTSERVERPROCESS->start();
    WICKRIOPROCESSCOMMAND = new WickrIOProcessCommand(pOperation);
    WICKRIOPROCESSCOMMAND->start();

    QObject::connect(WICKRIOPROCESSCOMMAND, &WickrIOProcessCommand::signalQuit,
                     WICKRIOCLIENTSERVERPROCESS, &WickrIOClientServerProcess::processFinished,
                     Qt::QueuedConnection);

    svcret = app->exec();

    WickrIOIPCRuntime::shutdown();

    qDebug() << "Leaving Client Service";
    return svcret;
}
