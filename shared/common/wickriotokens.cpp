#include <QDateTime>

#include "wickriotokens.h"

WickrIOTokens::WickrIOTokens()
{

}

QString
WickrIOTokens::getRandomString(const int length)
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    qsrand((int)(QDateTime::currentMSecsSinceEpoch()));
    QString randomString;
    for(int i=0; i<length; ++i) {
        int index = qrand() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}
