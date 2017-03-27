#include <QCoreApplication>
#include "wickrIOClientMain.h"
#include "wickrbotutils.h"
#include "operationdata.h"
#include "wickrIOClientRuntime.h"

void
WickrIOClientMain::noDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

/** Search the configuration file */
QString
WickrIOClientMain::searchConfigFile() {
#ifdef Q_OS_WIN
    return QString(WBIO_SERVER_SETTINGS_FORMAT)
            .arg(QCoreApplication::organizationName())
            .arg(QCoreApplication::applicationName());
#else
    // Setup the list of locations to search for the ini file
    QString filesdir = QStandardPaths::writableLocation( QStandardPaths::DataLocation );

    QStringList searchList;
    searchList.append(filesdir);

    // Look for the ini file with the application name
    QString appName=QCoreApplication::applicationName();
    QString fileName(appName+".ini");

    QString retFile = WickrBotUtils::fileInList(fileName, searchList);

    if (retFile.isEmpty()) {
        qFatal("Cannot find config file %s",qPrintable(fileName));
    }
    return retFile;
#endif
}

void
WickrIOClientMain::redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    //in this function, you can write the message to any stream!
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        WickrIOClientRuntime::operationData()->output(str);
        break;
    case QtFatalMsg:
        WickrIOClientRuntime::operationData()->output(str);
        abort();
    }
}

#include "wickr-sdk/wickr-core-c/localRepo/wickr-crypto/unix/include/crypto_engine.h"
QString
WickrIOClientMain::decryptData(const QByteArray& filedata, const QString& passphrase)
{
    wickr_buffer_t config_buffer = { filedata.length(), filedata.data() };
    wickr_buffer_t pass_buffer = { passphrase.length(), passphrase.toLatin1().data() };

    wickr_crypto_engine_t engine = wickr_crypto_engine_get_default();

    wickr_buffer_t *decrypted = wickr_crypto_engine_kdf_decipher(&engine, &config_buffer, &pass_buffer);

    QString returnString = QString(decrypted->bytes, decrypted->length);
    return returnString;
}

void
WickrIOClientMain::loadBootstrapFile(const QString& fileName, const QString& passphrase)
{
    qDebug() << "The bootstrap file " << fileName << " with the passphrase " << passphrase << " was loaded";

    QFile bootstrap(QUrl(fileName).toLocalFile());

    // Try to decrypt the configuration file
    if (!bootstrap.open(QIODevice::ReadOnly))
    {
        qDebug() << "can't open bootstrap file";
        return;
    }
    QByteArray bootstrapBlob(bootstrap.readAll());
    bootstrap.close();
    if (bootstrapBlob.size() == 0)
        return;

#if 0
    QString bootstrapStr = cryptoDecryptConfiguration(passphrase, bootstrapBlob);
#else
    QString bootstrapStr = decryptData(bootstrapBlob, passphrase);
#endif

    if (bootstrapStr == NULL || bootstrapStr.isEmpty()) {
        if (!bootstrap.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "can't open bootstrap file";
            return;
        }
        // try reading the file as a text file
        bootstrapStr = QString(bootstrap.readAll());
        bootstrap.close();
        if (bootstrapStr == NULL || bootstrapStr.isEmpty())
            return;
    }

    QJsonDocument d;
    d = d.fromJson(bootstrapStr.toUtf8());
    if(WickrCore::WickrRuntime::getEnvironmentMgr()->loadBootStrapJson(d))
    {
        WickrCore::WickrRuntime::getEnvironmentMgr()->storeNetworkConfig(controller->getNetworkSettings());
        bootStrapDone();
    } else {
        showMessageBox(tr("Incorrect credentials - please try again."), tr("Configuration file error"));
    }
}

