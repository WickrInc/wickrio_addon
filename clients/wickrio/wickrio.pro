TEMPLATE = subdirs
SUBDIRS += \
    libs \
    callback_listener \
    clientserver \
    console \
    consoleserver \
    enterpriseclient \
    settings_init \
    service_control

CONFIG += ordered

message(*** WickrIO)
message(*** WickrIO: TARGET = $$TARGET)
message(*** WickrIO: OUT_PWD = $$OUT_PWD)

win32 {
    mkinstaller.commands += rm -rf $${TARGET}.deploy;
    mkinstaller.commands += mkdir $${TARGET}.deploy;
    CONFIG(release,release|debug) {
        mkinstaller.commands += cp $${OUT_PWD}/client/release/WickrIOClient.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt -xml $${TARGET}.deploy/WickrIOClient.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/enterpriseclient/release/WickrIOEClient.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt -xml $${TARGET}.deploy/WickrIOEClient.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/clientserver/release/WickrIOSvr.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt $${TARGET}.deploy/WickrIOSvr.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/console/release/WickrIOConsole.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt $${TARGET}.deploy/WickrIOConsole.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/consoleserver/release/WickrIOCSvr.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt $${TARGET}.deploy/WickrIOCSvr.exe ;

        mkinstaller.commands += cp $${PWD}/../../platforms/win/lib32/*.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/../../src/libwickr-sdk.a $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/WickrIOLib/release/WickrBot1.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/WickrIOGUI/release/WickrBotGUI1.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/QtWebApp/release/QtWebApp1.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/QtWebApp/release/QtWebApp1.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/SMTPEmail/release/SMTPEMail.dll $${TARGET}.deploy ;
    }
    else {
        mkinstaller.commands += cp $${OUT_PWD}/callback_listener/debug/WickrIOCallbackListenerBeta.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt $${TARGET}.deploy/WickrIOCallbackListenerBeta.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/client/debug/WickrIOClientBeta.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt -xml $${TARGET}.deploy/WickrIOClientBeta.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/enterpriseclient/debug/WickrIOEClientBeta.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt -xml $${TARGET}.deploy/WickrIOEClientBeta.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/clientserver/debug/WickrIOSvrBeta.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt $${TARGET}.deploy/WickrIOSvrBeta.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/console/debug/WickrIOConsoleBeta.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt $${TARGET}.deploy/WickrIOConsoleBeta.exe ;
        mkinstaller.commands += cp $${OUT_PWD}/consoleserver/debug/WickrIOCSvrBeta.exe $${TARGET}.deploy;
        mkinstaller.commands += windeployqt $${TARGET}.deploy/WickrIOCSvrBeta.exe ;

        mkinstaller.commands += cp $${PWD}/../../platforms/win/lib32/*.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/../../src/libwickr-sdk.a $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/WickrIOLib/debug/WickrBotd1.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/WickrIOGUI/debug/WickrBotGUId1.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/QtWebApp/debug/QtWebAppd1.dll $${TARGET}.deploy ;
        mkinstaller.commands += cp $${OUT_PWD}/libs/SMTPEmail/debug/SMTPEMail.dll $${TARGET}.deploy ;
    }

    QMAKE_EXTRA_TARGETS += mkinstaller
}
