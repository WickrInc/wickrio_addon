#include <QCoreApplication>
#include <QObject>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QThread>
#include <QTimer>

#include <qtservice/QtServiceController>
#include "wickrIOCommon.h"

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    int retVal = 0;

    QCoreApplication app(argc, argv);
    app.setOrganizationName(WBIO_ORGANIZATION);

    if (argc != 3) {
        qFatal("Usage: %s <service> <action>", qPrintable(app.applicationName()));
    }

    QString filename=argv[1];
    QString action=argv[2];

    QtServiceController controller(filename);

    if (action.toLower() == "isup") {
        if (! controller.isRunning())
            retVal = 1;
    } else if (action.toLower() == "stop") {
        if (controller.isRunning()) {
            if (controller.stop()) {
                QTimer timer;
                QEventLoop loop;
                QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
                while (controller.isRunning()) {
                    qDebug() << "Waiting for" << filename << "to stop running";
                    timer.start(10000);
                    loop.exec();
                }
            } else {
                retVal = 1;
            }
        } else {
            retVal = 1;
        }
    } else if (action.toLower() == "install") {
        qDebug() << "not implemented yet";
    } else if (action.toLower() == "start") {
        if (controller.isInstalled()) {
            if (controller.start()) {
                QTimer timer;
                QEventLoop loop;
                QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
                while (! controller.isRunning()) {
                    qDebug() << "Waiting for" << filename << "to start running";
                    timer.start(10000);
                    loop.exec();
                }
            }
        }
    }

    return retVal;
}
