#include <QFile>
#include <QDebug>

#include "wickrIOBootstrap.h"
#include "libinterface/libwickrcore.h"

bool
WickrIOBootstrap::encryptAndSave(const QString& bstrapString, const QString& fileName, const QString& passphrase)
{
    QByteArray bsBlob = cryptoEncryptConfiguration(passphrase, bstrapString);
    if (bsBlob.isEmpty()) {
        qDebug() << "Cannot encrypt bootstrap file:" << fileName;
        return false;
    }

    QFile bootstrap(fileName);
    if (bootstrap.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
        bootstrap.write(bsBlob, bsBlob.length());
        bootstrap.flush();
        bootstrap.close();
    } else {
        qDebug() << "Cannot open bootstrap file:" << fileName;
        return false;
    }

    return true;
}

/**
 * @brief WickrIOBootstrap::readFile
 * Decrypts the input file using the input passphrase. The resulting string will be passed
 * to the loadBootstrapString() function to be applied as appropriate.
 * @param fileName name of the bootstrap file
 * @param passphrase the passphrase used to decrypt the bootstrap file
 * @return the decrypted bootstrap is returned
 */
QString
WickrIOBootstrap::readFile(const QString& fileName, const QString& passphrase)
{
    QFile bootstrap(fileName);

    // Try to decrypt the configuration file
    if (!bootstrap.open(QIODevice::ReadOnly))
    {
        qDebug() << "can't open bootstrap file";
        return nullptr;
    }
    QByteArray bootstrapBlob(bootstrap.readAll());
    bootstrap.close();
    if (bootstrapBlob.size() == 0)
        return nullptr;

    QString bootstrapStr = cryptoDecryptConfiguration(passphrase, bootstrapBlob);
    if (bootstrapStr == NULL || bootstrapStr.isEmpty()) {
        if (!bootstrap.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "can't open bootstrap file";
            return nullptr;
        }
        // try reading the file as a text file
        bootstrapStr = QString(bootstrap.readAll());
        bootstrap.close();
        if (bootstrapStr == NULL || bootstrapStr.isEmpty())
            return nullptr;
    }

    return bootstrapStr;
}


