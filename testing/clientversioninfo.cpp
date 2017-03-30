#include "clientversioninfo.h"

#include "Wickr/WickrProduct.h"
#include "common/wickrUtil.h"

#define BUILD_NUMBER  3000001

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

int ClientVersionInfo::getProductType()
{
#ifdef WICKR_COMPLIANCE_BOT
    return PRODUCT_TYPE_BOT;
#elif WICKR_SCIF
    return PRODUCT_TYPE_SKIF;
#else
    return PRODUCT_TYPE_PRO;
#endif
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

QString ClientVersionInfo::getAppName()
{
    return "WickrIO";
}
