#include "services/wickrService.h"
#include "mywickr/wickrUtil.h"
#include "wickrBuildNumbers.h"

const QString WickrUtil::DefaultBaseURL =  "https://messaging-beta.wickr.com/112/src";

extern QString thePlatform;

int
versionForLogin()
{
    return ENTERPRISE_BUILD_VERSION;
}

QString getPlatform()
{
    return thePlatform;
}

QString getVersionString()
{
    int bvnum = ENTERPRISE_BUILD_VERSION;
    QString nicever = QString::number(bvnum/1000000);
    bvnum %= 1000000;
    nicever += "." + QString::number(bvnum/10000);
    bvnum %= 10000;
    nicever += "." + QString::number(bvnum);
    return (QString("v") + nicever);
}

QString getBuildString()
{
    return (QString("build ") + QString::number(ENTERPRISE_BUILD_NUMBER%100));
}

QString
getAppVersion()
{
    int bvnum = ENTERPRISE_BUILD_VERSION;
    QString nicever = QString::number(bvnum/1000000);
    bvnum %= 1000000;
    nicever += "." + QString::number(bvnum/10000);
    bvnum %= 10000;
    nicever += "." + QString::number(bvnum);

    return nicever;
}
