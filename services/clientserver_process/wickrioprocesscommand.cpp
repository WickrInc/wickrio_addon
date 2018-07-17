#include <QTextStream>
#include <QDebug>
#include <QIODevice>

#include "wickrioprocesscommand.h"
#if 1
#include "cmdclient.h"
#else
#include "cmdmain.h"
#endif

WickrIOProcessCommand *WickrIOProcessCommand::theProcessCommand;

WickrIOProcessCommand::WickrIOProcessCommand(OperationData *pOperation) :
    m_operation(pOperation)
{
    moveToThread(this);
    connect(this, &WickrIOProcessCommand::started, this, &WickrIOProcessCommand::processStarted, Qt::QueuedConnection);
    connect(this, &WickrIOProcessCommand::finished, this, &WickrIOProcessCommand::processFinished, Qt::QueuedConnection);
}

unsigned
WickrIOProcessCommand::getVersionNumber(QFile *versionFile)
{
    if (!versionFile->open(QIODevice::ReadOnly))
        return 0;

    QString s;

    QTextStream s1(versionFile);
    s.append(s1.readAll());
    versionFile->close();

    unsigned retVal = 0;
    QStringList slist = s.split(".");
    if (slist.length() == 3) {
        retVal = slist.at(0).toInt() * 1000000 + slist.at(1).toInt() * 1000 + slist.at(2).toInt();
    } else if (slist.length() == 2) {
        retVal = slist.at(0).toInt() * 1000000 + slist.at(1).toInt() * 1000;
    }
    return retVal;
}

void
WickrIOProcessCommand::processStarted()
{
#if 1
    CmdOperation    cmdOperation(m_operation);
    CmdClient       cmdClient(&cmdOperation);
    QStringList     options;

    // Make the client commands the root
    options.append("-root");
    options.append("-basic");

    // Print out a list of the clients
    cmdClient.runCommands(options, "list");

    // Check if there are any clients that have had updated hubot software
    QMap<QString, unsigned> integrationVersions;

//    QDirIterator it(intDir, QStringList() << "*", QDir::Files, QDirIterator::Subdirectories);
    QDirIterator it(WBIO_INTEGRATIONS_DIR, QDir::NoDotAndDotDot | QDir::Dirs);
    while (it.hasNext()) {
        QDir curdir(it.next());

        QFile curHubotVersionFile(QString("%1/%2/VERSION").arg(WBIO_INTEGRATIONS_DIR).arg(curdir.dirName()));
        unsigned curHubotVersion;
        if (curHubotVersionFile.exists()) {
            curHubotVersion = getVersionNumber(&curHubotVersionFile);
            if (curHubotVersion > 0) {
                integrationVersions.insert(curdir.dirName(), curHubotVersion);
            }
        }
    }

    QStringList upgradeMessages;

    // go through all of the clients and see if there are bot integrataions confiigured, check the versions
    QList<WickrBotClients *> clients = m_operation->m_botDB->getClients();
    for (WickrBotClients *client : clients) {
        if (!client->botType.isEmpty()) {
            unsigned newBotVer = integrationVersions.value(client->botType, 0);
            unsigned curBotVer = 0;

            QString destPath = QString(WBIO_CLIENT_BOTDIR_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(client->name)
                    .arg(client->botType);
            QFile curHubotVersionFile(QString("%1/VERSION").arg(destPath));
            if (curHubotVersionFile.exists()) {
                curBotVer = getVersionNumber(&curHubotVersionFile);
                if (curBotVer > 0) {
                    if (curBotVer < newBotVer) {
                        upgradeMessages.append(QString("CONSOLE:Upgrade of %1 available for %2").arg(client->botType).arg(client->user));
                    }
                }
            }
        }
    }

    // Print out upgrade messages if there are any
    if (upgradeMessages.size() > 0) {
        qDebug() << "CONSOLE:=============================================================================";
        qDebug() << "CONSOLE:Upgrades available:";
        for (QString msg : upgradeMessages) {
            qDebug().noquote().nospace() << msg;
        }
        qDebug() << "CONSOLE:=============================================================================";
    }

    // start the command handler
    cmdClient.runCommands(options);
#else
    CmdMain cmdmain(m_operation);

    // Print out a list of the clients
    cmdmain.runCommands("client -basic,list");
    cmdmain.runCommands();
#endif

    emit signalQuit();
}

void
WickrIOProcessCommand::processFinished()
{
}
