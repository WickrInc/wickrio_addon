#include <QDebug>
#include <QProcess>

#include "supportBotInterface.h"

QProcess *supportBotProcess;

bool start(QString location, QString script)
{
    QString scriptFullPath = QString("%1/%1").arg(location).arg(script);

    // Create a process to run the configure
    supportBotProcess = new QProcess();

#if 0
    connect(supportBotProcess, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        qDebug() << "CONSOLE:error enum val = " << error;
        supportBotProcess->deleteLater();
        supportBotProcess = nullptr;
    });
#endif

    supportBotProcess->setProcessChannelMode(QProcess::MergedChannels);
    supportBotProcess->setWorkingDirectory(location);
    supportBotProcess->start(scriptFullPath, QIODevice::ReadWrite);

    // Wait for it to start
    if(!supportBotProcess->waitForStarted()) {
        qDebug() << QString("CONSOLE:Failed to run %1").arg(scriptFullPath);
        supportBotProcess->deleteLater();
        supportBotProcess = nullptr;
        return false;
    }

    return true;
}

QString processInput(QString input)
{
    if (supportBotProcess == nullptr) {
        return QString();
    }

    bool sentrequest=false;

    while(supportBotProcess->waitForReadyRead(-1)) {
        while(supportBotProcess->canReadLine()) {
            QString bytes = QString(supportBotProcess->readLine());
            if (!bytes.isEmpty()) {
                if (sentrequest){
                    return bytes;
                }

                if (bytes.toLower().startsWith("prompt:")) {
                    QByteArray output = (QString("%1\n").arg(input)).toLatin1();
                    supportBotProcess->write(output);
                    sentrequest = true;
                }
            }
        }
    }

    qDebug() << "Process stopped " << input;
    return QString("Process stopped!");
}

void quit()
{
    if (supportBotProcess != nullptr) {
        supportBotProcess->write("quite\n");
        supportBotProcess->close();
        supportBotProcess->deleteLater();
        supportBotProcess = nullptr;
    }
}
