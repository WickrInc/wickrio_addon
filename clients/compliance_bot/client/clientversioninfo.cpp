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
#if defined(WICKR_BLACKOUT)                            // == BLACKOUT ==

#  if defined(WICKR_PRODUCTION)
    return QStringLiteral("WickrProOnPrem");
#  elif defined(WICKR_DEBUG)
    return QStringLiteral("WickrProOnPremDebug");
#  endif

#elif defined(WICKR_SCIF)                              // == SCIF ==

#  if defined(WICKR_PRODUCTION)
    return QStringLiteral("WickrSCIF");
#  elif defined(WICKR_DEBUG) && defined(WICKR_BETA)
    return QStringLiteral("WickrCloudBeta");
#  elif defined(WICKR_DEBUG) && defined(WICKR_QA)
    return QStringLiteral("WickrSCIFQA");
#  elif defined(WICKR_DEBUG) && defined(WICKR_ALPHA)
    return QStringLiteral("WickrSCIFDebug");
#  endif

#else                                                  // == PRO ==

#  if defined(WICKR_PRODUCTION)
    return QStringLiteral("WickrPro");
#  elif defined(WICKR_DEBUG) && defined(WICKR_BETA)
    return QStringLiteral("WickrProBeta");
#  elif defined(WICKR_DEBUG) && defined(WICKR_QA)
    return QStringLiteral("WickrProQA");
#  elif defined(WICKR_DEBUG) && defined(WICKR_ALPHA)
    return QStringLiteral("WickrProAlpha");
#  endif

#endif
}
