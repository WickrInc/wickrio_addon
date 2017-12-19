#include "clientversioninfo.h"

#include "wickrBuildNumbers.h"
#include "WickrProduct.h"

QString ClientVersionInfo::getPlatform()
{
    return
#if defined(Q_OS_MAC)
        "MacOS"
#elif defined(Q_OS_LINUX)
        "Linux"
#elif defined(__WIN32__)
        "Windows"
#else
#error Unsupported Platform
#endif
        ;
}

int ClientVersionInfo::versionForLogin()
{
    return BUILD_NUMBER;
}

QString ClientVersionInfo::getVersionString()
{
    int bvnum = BUILD_NUMBER;
    QString nicever = QString::number(bvnum/1000000);
    bvnum %= 1000000;
    nicever += "." + QString::number(bvnum/10000);
    bvnum %= 10000;
    nicever += "." + QString::number(bvnum/100);
    return (QString("v") + nicever);
}

QString ClientVersionInfo::getBuildString()
{
    return (QStringLiteral("build ") + QString::number(BUILD_NUMBER%100));
}

QString ClientVersionInfo::getOrgName()
{
    return "Wickr, LLC";
}
