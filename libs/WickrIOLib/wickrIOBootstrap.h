#ifndef WICKRIOBOOTSTRAP_H
#define WICKRIOBOOTSTRAP_H

#include <QString>

class WickrIOBootstrap {
public:
    WickrIOBootstrap() {}

    static bool encryptAndSave(const QString& bstrapStr, const QString& bstrapFilename, const QString& passphrase);
    static QString readFile(const QString& bstrapFilename, const QString& passphrase);

};


#endif // WICKRIOBOOTSTRAP_H
