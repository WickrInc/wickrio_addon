DEPTH = ../../..
COMMON = ../common

#
# Include the WickrIO common files
#
include($${COMMON}/common.pri)

CONFIG += c++11

CONFIG(release,release|debug) {
    message(*** WickrIO Console Release Build)
    wickr_prod:TARGET = WickrIOConsole
    else:TARGET = WickrIOConsolePreview
    SOURCES += ../common/versiondebugNO.cpp

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
    message(*** WickrIO Console Beta Build)
    wickr_beta:TARGET = WickrIOConsoleBeta
    else:TARGET = WickrIOConsoleAlpha
    SOURCES += ../common/versiondebugYES.cpp
    DEFINES += VERSIONDEBUG

    macx {
        ICON = $$COMMON/Wickr_beta.icns
        QMAKE_INFO_PLIST = $$DEPTH/ConsumerBeta.Info.plist
    }
    win32 {
        RC_FILE = $$COMMON/Wickr-beta.rc
        ICON = $$COMMON/Wickr-beta.ico
    }
}

#
# Include the WickrIO common console files
#
include(../common_console/common_console.pri)

#
# Include the Wickr library
#
include(../libs/WickrIOLib/WickrIOLib.pri)

#
# Include the Wickr GUI library
#
include(../libs/WickrIOGUI/WickrIOGUI.pri)

QT += widgets
QT += network
QT += sql

HEADERS += client.h \
    addclientdialog.h \
    webserver.h \
    cmdadvanced.h \
    cmdbase.h \
    cmdclient.h \
    cmdconsole.h \
    cmdmain.h \
    cmdoperation.h \
    cmdserver.h \
    consoleserver.h \
    advanceddialog.h \
    addconsoleuserdialog.h \
    configureconsoleserverdialog.h

SOURCES += client.cpp \
    main.cpp \
    addclientdialog.cpp \
    webserver.cpp \
    cmdadvanced.cpp \
    cmdbase.cpp \
    cmdclient.cpp \
    cmdconsole.cpp \
    cmdmain.cpp \
    cmdoperation.cpp \
    cmdserver.cpp \
    consoleserver.cpp \
    advanceddialog.cpp \
    addconsoleuserdialog.cpp \
    configureconsoleserverdialog.cpp

TEMPLATE = app

FORMS += \
    console_dialog.ui \
    addclientdialog.ui \
    advanceddialog.ui \
    addconsoleuserdialog.ui \
    configureconsoleserverdialog.ui

CONFIG += depend_includepath

# qsqlcipher_wickr

win32 {
    CONFIG(debug, debug|release):LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/debug
    else:LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/release
} else {
    LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/
}
LIBS += -lqsqlcipher_wickr

# sqlcipher

LIBS += -lsqlcipher
