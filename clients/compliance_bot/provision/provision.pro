#-------------------------------------------------
#
# Project created by QtCreator 2013-12-09T16:47:34
#
#-------------------------------------------------

DEPTH = ../../..

CONFIG += c++11
CONFIG += console
QT -= gui

wickr_compliance:DEFINES += WICKR_COMPLIANCE=1
wickr_compliance_bot {
    DEFINES += WICKR_COMPLIANCE_BOT=1
    DEFINES += WICKR_COMPLIANCE=1
}

CONFIG(release,release|debug) {
    BUILD_TYPE=release

    wickr_beta {
        message(*** WickrIO ComplianceBot Provision Beta.Release Build)
        TARGET = provisionBeta
    } else {
        message(*** WickrIO ComplianceBot Provision Production Build)
        TARGET = provision
    }
} else {
    DEFINES += VERSIONDEBUG
    BUILD_TYPE=debug

    wickr_beta {
        message(*** WickrIO ComplianceBot Provision Beta.Debug Build)
        TARGET = provisionBeta
    }
    else:wickr_qa {
        message(*** WickrIO ComplianceBot Provision QA Build)
        TARGET = provisionQA
    }
    else {
        message(*** WickrIO ComplianceBot Provision Alpha Build)
        TARGET = provisionAlpha
    }
}

QT += sql multimediawidgets xml
QT += network websockets

COMMON = $${DEPTH}/shared/common
INCLUDEPATH += $${COMMON}

wickr_messenger {
    DEFINES += WICKR_MESSENGER=1
}
else:wickr_blackout {
    DEFINES += WICKR_BLACKOUT=1
}
else:wickr_enterprise {
    DEFINES += WICKR_ENTERPRISE=1
}
else:wickr_scif {
    DEFINES += WICKR_SCIF=1
}


CONFIG(release,release|debug) {
    wickr_beta {
        DEFINES += WICKR_BETA
    }
    else {
        DEFINES += WICKR_PRODUCTION
    }
}
else {
    wickr_beta {
        DEFINES += WICKR_BETA
    }
    else:wickr_qa {
        DEFINES += WICKR_QA
    }
    else {
        DEFINES += WICKR_ALPHA
    }

    DEFINES += WICKR_DEBUG
}

# Include the Wickr IO common HTTP files
#
include($${DEPTH}/shared/common_http/common_http.pri)

#
# Include the Wickr Client library
#
include(../../libs/WickrIOClient/WickrIOClient.pri)

INCLUDEPATH += $$DEPTH/wickr-sdk/export
INCLUDEPATH += $$DEPTH/wickr-sdk/src
INCLUDEPATH += $$DEPTH/wickr-sdk/export/Wickr
INCLUDEPATH += $$DEPTH/services/common
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/qbson
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/libbson

#
# Include the Wickr IO Console library
#
include($${DEPTH}/libs/WickrIOConsole/WickrIOConsole.pri)

#
# Include the Wickr IO library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

#
# Include the QtWebAPP library
#
include($${DEPTH}/libs/QtWebApp/QtWebApp.pri)

#
# Include the SMTPEmail library
#
include($${DEPTH}/libs/SMTPEmail/SMTPEmail.pri)

#
# Zero MQ Qt library
#
include($${DEPTH}/libs/nzmqt/nzmqt.pri)

TEMPLATE = app

CONFIG(release,release|debug) {
    SOURCES += $${COMMON}/versiondebugNO.cpp

    macx {
        ICON = $$COMMON/Wickr_prod.icns
        QMAKE_INFO_PLIST = $$DEPTH/ConsumerProd.Info.plist
    }
    win32 {
        RC_FILE = $$COMMON/Wickr.rc
        ICON = $$COMMON/Wickr.ico
    }
}
else {
    SOURCES += $${COMMON}/versiondebugYES.cpp

    APPICON = $$COMMON/Wickr_beta.icns
    INSTICON = $$COMMON/Wickr_beta_installer.icns

    macx {
        ICON = $$COMMON/Wickr_beta.icns
        QMAKE_INFO_PLIST = $$DEPTH/ConsumerBeta.Info.plist
    }
    win32 {
        RC_FILE = $$COMMON/Wickr-beta.rc
        ICON = $$COMMON/Wickr-beta.ico
    }
}

RESOURCES += \
    provisioning.qrc

SOURCES += \
    $${COMMON}/cmdbase.cpp \
    cmdProvisioning.cpp \
    main.cpp \
    wickrioeclientmain.cpp \
    wickrIOLoginHdlr.cpp \
    wickrIOProvisionHdlr.cpp \
    wickrIOClientRuntime.cpp \
    wickrIOBot.cpp \
    cmdMain.cpp \
    wickrIOConsole.cpp

HEADERS += \
    $${COMMON}/cmdbase.h \
    complianceClientConfigInfo.h \
    cmdProvisioning.h \
    wickrioeclientmain.h \
    wickrIOLoginHdlr.h \
    wickrIOProvisionHdlr.h \
    wickrIOClientRuntime.h \
    wickrIOBot.h \
    cmdMain.h \
    wickrIOConsole.h

# qsqlcipher_wickr

win32 {
    CONFIG(debug, debug|release):LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/debug
    else:LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/release
} else {
    LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/
}
LIBS += -lqsqlcipher_wickr
LIBS += -lzmq

# sqlcipher

LIBS += -lsqlcipher

macx {
    QMAKE_LFLAGS += -F$$PWD/$$DEPTH/platforms/mac/lib64
    QMAKE_CXXFLAGS += -Wunused-parameter
    QMAKE_LFLAGS_SONAME  = -Wl,-install_name,@executable_path/../Frameworks/
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/mac/lib64/ -framework IOKit
    LIBS += -framework CoreFoundation -framework Carbon -lobjc
    LIBS += -framework Quincy -framework CrashReporter
    INCLUDEPATH += $$DEPTH/wickr-sdk/platforms/mac/crashdumper
    INCLUDEPATH += $$DEPTH/wickr-sdk/platforms/mac/include
    INCLUDEPATH += $$DEPTH
    LIBS += -lavdevice -lavcodec -lavformat -lavfilter -lswscale -lswresample -lavutil -lcrypto

    LIBS += -lwickrcore -lssl -lphonenumber
    LIBS += -licudata -licui18n -licuuc -lstdc++
    LIBS += -L/opt/local/lib -lprotobuf

    OTHER_FILES += \
        $$DEPTH/wickr-sdk/wickr-sdk/platforms/mac/lib64/libcrypto.a \
        $$DEPTH/wickr-sdk/wickr-sdk/platforms/mac/lib64/libssl.a
}

linux-g++* {
    CONFIG(release,release|debug) {
        QMAKE_RPATHDIR += /usr/lib/wio_compliance_bot
    }
    else {
        wickr_blackout:QMAKE_RPATHDIR = /usr/lib/wio_compliance_bot-onprem
        else:wickr_beta:QMAKE_RPATHDIR = /usr/lib/wio_compliance_bot-beta
        else:wickr_qa:QMAKE_RPATHDIR = /usr/lib/wio_compliance_bot-qa
        else:QMAKE_RPATHDIR = /usr/lib/wio_compliance_bot-alpha
    }

    QMAKE_CXXFLAGS += -Wunused-parameter

    INCLUDEPATH += $$DEPTH/wickr-sdk/platforms/linux/include
    INCLUDEPATH += $$DEPTH
    INCLUDEPATH += /usr/include
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/linux/generic-64
    LIBS += -L$$OUT_PWD/$$DEPTH/wickr-sdk/src -lwickr-sdk
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/linux/generic-64
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/linux/generic-64/$${BUILD_TYPE}
    LIBS += -lWickrCoreC
    LIBS += -lbsd -luuid -ldl
    LIBS += -lssl
    LIBS += -lstdc++
    LIBS += -lprotobuf
    LIBS += -lcrypto
    LIBS += -lwmmigrator -lcjson
    LIBS += -lpsiphontunnel
    LIBS += -lreadline

    LIBS += -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/qbson -lqbson \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/libbson -lbson \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/cloud/qcloud -lQCloud \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/WickrProto -lWickrProto

    LIBS += -L$$OUT_PWD/$${DEPTH}/libs/SMTPEmail -lSMTPEmail

}

win32 {
    INCLUDEPATH += $$DEPTH/wickr-sdk/platforms/win/include
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/win/lib32
    LIBS += -lCFLite

    LIBS += -lwickrcore -lssleay32
    LIBS += -lavdevice -lavcodec -lavformat -lavfilter -lswscale -lswresample -lavutil
    LIBS += -lphonenumber -lprotobuf
    LIBS += -lstdc++ -lws2_32

    DEFINES += I18N_PHONENUMBERS_NO_THREAD_SAFETY

    mkbundle.depends  += all

    mkinstaller.depends += mkbundle
    mkinstaller.commands += rm -fr $${TARGET}.deploy;
    mkinstaller.commands += mkdir $${TARGET}.deploy;
    mkinstaller.commands += cp $${TARGET}.exe $${TARGET}.deploy;
    CONFIG(release,release|debug) {
        mkinstaller.commands += strip $${TARGET}.deploy/$${TARGET}.exe;
    }
    mkinstaller.commands += windeployqt $${TARGET}.deploy/$${TARGET}.exe;
    mkinstaller.commands += cp $$PWD/$$DEPTH/platforms/win/lib32/*.dll $${TARGET}.deploy;

    # Setup the MSI builder commands
    mkmsi.depends += mkinstaller

    mkmsi.commands += mkdir $${OUT_PWD}/$${TARGET}.installer;
    mkmsi.commands += cp -R $${PWD}/$$DEPTH/platforms/win/installer/Prerequisites $${OUT_PWD}/$${TARGET}.installer;
    mkmsi.commands += cp $${PWD}/$$DEPTH/platforms/win/installer/Wickr_msi_builder.aip $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip;
#    mkmsi.commands += AdvancedInstaller.com /newproject $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip -type "professional" -overwrite;
#    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /SetVersion 2.2.1.2;
#    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /SetPackageName \"Wickr - Top Secret Messemger.msi\" -buildname DefaultBuild;
#    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /SetProperty ProductName="Wickr - Top Secret Messenger";
#    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /SetProperty Manufacturer="Wickr Inc.";
    mkmsi.commands += $$PWD/$$DEPTH/platforms/win/installer/bin/CreateAdvancedInstallerFiles.bat $${OUT_PWD}/$${TARGET}.deploy $${OUT_PWD}/$${TARGET}.installer/msi_commands.txt $$system(cmd /c type WICKRIOECLIENT_BUILD_NUMBER);
    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /SetProperty UpgradeCode="{52F2C449-26D7-47CE-9BA0-C6F40E35AB9B}";
    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /NewShortcut -name \"Wickr - Top Secret Messenger\" -dir DesktopFolder -target APPDIR/$${TARGET}.exe -wkdir APPDIR;
    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /NewShortcut -name \"Wickr - Top Secret Messenger\" -dir StartMenuFolder -target APPDIR/$${TARGET}.exe -wkdir APPDIR;
    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /NewShortcut -name \"Wickr - Top Secret Messenger\" -dir QuickLaunch_Dir -target APPDIR/$${TARGET}.exe -wkdir APPDIR;
    mkmsi.commands += AdvancedInstaller.com /edit $${OUT_PWD}/$${TARGET}.installer/$${TARGET}.msi_builder.aip /NewShortcut -name \"Wickr - Top Secret Messenger\" -dir SHORTCUTDIR -target APPDIR/$${TARGET}.exe -wkdir APPDIR;

    mkmsi.commands += cd $${OUT_PWD}/$${TARGET}.installer ;
    mkmsi.commands += AdvancedInstaller.com /execute $${TARGET}.msi_builder.aip msi_commands.txt;

    QMAKE_EXTRA_TARGETS += mkbundle mkinstaller mkmsi
}

macx {
    # Name of the application signing certificate
    APPCERT = \"3rd Party Mac Developer Application: Wickr, LLC (W8RC3R952A)\"

    # Name of the installer signing certificate
    INSTALLERCERT = '3rd Party Mac Developer Installer: Wickr, LLC (W8RC3R952A)'

    # Bundle identifier for your application
    BUNDLEID = com.mywickr.wickrmac

    # Name of the entitlements file (only needed if you want to sandbox the application)
    ENTITLEMENTS = $$PWD/wickrmacbeta.entitlements

    QMAKE_CFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_OBJECTIVE_CFLAGS_RELEASE =  $$QMAKE_OBJECTIVE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

    OTHER_FILES += $${QMAKE_INFO_PLIST} $${ENTITLEMENTS}

    QMAKE_POST_LINK += mkdir -p $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libavdevice.55.dylib $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libavfilter.4.dylib $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libswscale.2.dylib $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libpostproc.52.dylib $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libavformat.55.dylib $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libavcodec.55.dylib $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libswresample.0.dylib $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libavutil.52.dylib $${TARGET}.app/Contents/Frameworks;

    # Copy the ICU Libraries
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libicui18n.53.dylib $${TARGET}.app/Contents/Frameworks/ ;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libicudata.53.dylib $${TARGET}.app/Contents/Frameworks/ ;
    QMAKE_POST_LINK += cp $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/libicuuc.53.dylib $${TARGET}.app/Contents/Frameworks/;

    QMAKE_POST_LINK += install_name_tool -id "@executable_path/../Frameworks/libicui18n.53.dylib" "$${TARGET}.app/Contents/Frameworks/libicui18n.53.dylib";
    QMAKE_POST_LINK += install_name_tool -id "@executable_path/../Frameworks/libicudata.53.dylib" "$${TARGET}.app/Contents/Frameworks/libicudata.53.dylib";
    QMAKE_POST_LINK += install_name_tool -id "@executable_path/../Frameworks/libicuuc.53.dylib" "$${TARGET}.app/Contents/Frameworks/libicuuc.53.dylib";

    QMAKE_POST_LINK += install_name_tool -change libicuuc.53.dylib "@executable_path/../Frameworks/libicuuc.53.dylib" "$${TARGET}.app/Contents/Frameworks/libicui18n.53.dylib";
    QMAKE_POST_LINK += install_name_tool -change libicudata.53.dylib "@executable_path/../Frameworks/libicudata.53.dylib" "$${TARGET}.app/Contents/Frameworks/libicui18n.53.dylib";

    QMAKE_POST_LINK += install_name_tool -change libicudata.53.dylib "@executable_path/../Frameworks/libicudata.53.dylib" "$${TARGET}.app/Contents/Frameworks/libicuuc.53.dylib";

    QMAKE_POST_LINK += install_name_tool -change "../lib/libicudata.53.1.dylib" "@executable_path/../Frameworks/libicudata.53.dylib" "$${TARGET}.app/Contents/MacOS/$${TARGET}" ;


    QMAKE_POST_LINK += rm -fr $${TARGET}.app/Contents/Frameworks/Quincy.framework $${TARGET}.app/Contents/Frameworks/CrashReporter.framework;
    QMAKE_POST_LINK += cp -R $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/Quincy.framework $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += cp -R $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/lib64/CrashReporter.framework $${TARGET}.app/Contents/Frameworks;
    QMAKE_POST_LINK += install_name_tool -change @loader_path/../Frameworks/Quincy.framework/Versions/A/Quincy @executable_path/../Frameworks/Quincy.framework/Versions/A/Quincy $${TARGET}.app/Contents/MacOS/$${TARGET};
    QMAKE_POST_LINK += install_name_tool -change @rpath/CrashReporter.framework/Versions/A/CrashReporter @executable_path/../Frameworks/CrashReporter.framework/Versions/A/CrashReporter $${TARGET}.app/Contents/MacOS/$${TARGET};

    QMAKE_POST_LINK += ${QTDIR}/bin/macdeployqt $${TARGET}.app -executable=$${TARGET}.app/Contents/MacOS/$${TARGET};

    QMAKE_POST_LINK += install_name_tool -change "libicudata.53.dylib" "@loader_path/../../../libicudata.53.dylib" "$${TARGET}.app/Contents/Frameworks/QtWebKit.framework/Versions/5/QtWebKit" ;
    QMAKE_POST_LINK += rm -fr $${TARGET}.app/Contents/PlugIns/mediaservice/libqqt7engine.dylib ;

    # Remove unneeded frameworks (uncomment and change to suit your application)
    #QMAKE_POST_LINK += rm -r $${TARGET}.app/Contents/Frameworks/QtDeclarative.framework;

    # Remove unneeded plug-ins (uncomment and change to suit your application)
    QMAKE_POST_LINK += rm -rf $${TARGET}.app/Contents/PlugIns/sqldrivers/libqsqlmysql.dylib;
    QMAKE_POST_LINK += rm -rf $${TARGET}.app/Contents/PlugIns/sqldrivers/libqsqlodbc.dylib;
    QMAKE_POST_LINK += rm -rf $${TARGET}.app/Contents/PlugIns/sqldrivers/libqsqlpsql.dylib;
    QMAKE_POST_LINK += rm -rf $${TARGET}.app/Contents/PlugIns/sqldrivers/libqsqlite.dylib;

    # Extract debug symbols
    QMAKE_POST_LINK += rm -fr $${TARGET}.dSYM;
    QMAKE_POST_LINK += dsymutil $${TARGET}.app/Contents/MacOS/$${TARGET}  ;
    QMAKE_POST_LINK += mv $${TARGET}.app/Contents/MacOS/$${TARGET}.dSYM . ;

    ### MAC INSTALLER
    mkinstaller.depends += Wickr.app/Contents/MacOS/Wickr WickrBeta.app/Contents/MacOS/WickrBeta
    mkinstaller.commands += chmod +x $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/installer/builddmg.sh ;
    mkinstaller.commands += $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/installer/builddmg.sh Wickr.app Wickr-TopSecretMessenger $${PWD}/$$DEPTH/platforms/mac/installer/wickr_desktop_mac_bg_installer.png $${PWD}/$$DEPTH/platforms/mac/installer/Wickr_installer.icns ;
    mkinstaller.commands += $${PWD}/$$DEPTH/wickr-sdk/platforms/mac/installer/builddmg.sh WickrBeta.app WickrBeta-TopSecretMessenger $${PWD}/$$DEPTH/platforms/mac/installer/wickr_desktop_mac_bg_installer.png $${PWD}/$$DEPTH/platforms/mac/installer/Wickr_installer.icns ;

    QMAKE_EXTRA_TARGETS += mkinstaller
}

INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/qbson
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/libbson
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/cloud/qcloud
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/WickrProto

