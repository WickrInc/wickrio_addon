#include <QDebug>
#include <QString>

#include "inittest.h"
#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"
#include "common/wickrUtil.h"
#include "Wickr/WickrProduct.h"
#include "session/wickrAppClock.h"
#include "wickrapplication.h"
#include "common/wickrRuntime.h"

#ifdef Q_OS_MAC
#include "platforms/mac/extras/WickrAppDelegate-C-Interface.h"
#endif

InitTest::InitTest(QObject *parent) :
      QObject(parent)
{
}

void InitTest::initTestCase()
{
    WickrApplication *app = NULL;

    QByteArray secureJson;
    bool isDebug;
    if (isVERSIONDEBUG()) {
        isDebug = true;
        secureJson = "secex_json2:Fq3&M1[d^,2P";
    } else {
        isDebug = false;
        secureJson = "secex_json:8q$&M4[d^;2R";
    }

    wickrProductSetProductType(ClientVersionInfo::getProductType());

    QString username, appname = ClientVersionInfo::getAppName(), orgname = "Wickr, LLC";
    QString bootTime = WickrUtil::formatTimestamp(WickrAppClock::getBootTime());
    qDebug() <<  appname << "System was booted" << bootTime;

#ifdef Q_OS_MAC
    WickrAppDelegateInitialize();
    //wickrAppDelegateRegisterNotifications();
    //QString pushid = wickrAppDelegateGetNotificationID();
#endif

    int argc=1;
    char **argv=NULL;
    app = new WickrApplication(argc, argv, true);

    // Wickr Runtime Environment (all applications include this line)
    WickrCore::WickrRuntime::init(secureJson, isDebug);

}

void InitTest::testArea()
{
    QCOMPARE(2.0, 3.14);
    QCOMPARE(314.0, 314.0);
}

void InitTest::testRadius()
{
    QCOMPARE(2.0, 1.0);
    QCOMPARE(10.0, 10.0);
}

void InitTest::cleanupTestCase()
{
}
