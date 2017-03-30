#ifndef CLIENTVERSIONINFO_H
#define CLIENTVERSIONINFO_H

#include <QString>

class ClientVersionInfo {
public:
    static QString getAppName();
    static QString getOrgName();
    static QString getPlatform();
    static int getProductType();

    static QString getVersionString(); // e.g. "v1.2.3"
    static QString getBuildString();   // e.g. "build 4"
    static int versionForLogin();      // e.g. 1,020,304

    static QString getAppVersion()   { return getPlatform() + " " + getVersionString() + "\r\n" + getBuildString(); }
    static QString getVersionBuild() { return getVersionString() + " " + getBuildString(); }
};

#endif // CLIENTVERSIONINFO_H
