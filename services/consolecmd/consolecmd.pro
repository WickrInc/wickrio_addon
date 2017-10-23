DEPTH = ../..
COMMON = $${DEPTH}/shared/common
CONSOLESRC = ../console

#
# Include the Wickr IO services base
#
include(../services.pri)

#
# Include the WickrIO common defines files
#
include($${COMMON}/common_defines.pri)

#
# Include the WickrIO common files
#
include($${COMMON}/common.pri)

CONFIG += c++11

CONFIG(release,release|debug) {
    message(*** WickrIO Console Release Build)
    TARGET = WickrIOConsoleCmd
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
    message(*** WickrIO Console Beta Build)
    TARGET = WickrIOConsoleCmdDebug

    SOURCES += $${COMMON}/versiondebugYES.cpp
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
# Include the WickrIO common server files
#
SERVER_COMMON=../common
INCLUDEPATH += $${SERVER_COMMON}

#
# Include the Wickr library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

#
# Include the Wickr GUI library
#
#include($${DEPTH}/libs/WickrIOGUI/WickrIOGUI.pri)

INCLUDEPATH += $${CONSOLESRC}

QT -= gui
QT += network
QT += sql

HEADERS += \
    $${COMMON}/cmdbase.h \
    $${SERVER_COMMON}/server_common.h \
    $${SERVER_COMMON}/wickrioconsoleclienthandler.h \
    $${CONSOLESRC}/webserver.h \
    $${CONSOLESRC}/cmdadvanced.h \
    $${CONSOLESRC}/cmdclient.h \
    $${CONSOLESRC}/cmdconsole.h \
    $${CONSOLESRC}/cmdmain.h \
    $${CONSOLESRC}/cmdoperation.h \
    $${CONSOLESRC}/cmdserver.h \
    $${CONSOLESRC}/consoleserver.h

SOURCES += \
    $${COMMON}/cmdbase.cpp \
    $${SERVER_COMMON}/server_common.cpp \
    $${SERVER_COMMON}/wickrioconsoleclienthandler.cpp \
    $${CONSOLESRC}/webserver.cpp \
    $${CONSOLESRC}/cmdadvanced.cpp \
    $${CONSOLESRC}/cmdclient.cpp \
    $${CONSOLESRC}/cmdconsole.cpp \
    $${CONSOLESRC}/cmdmain.cpp \
    $${CONSOLESRC}/cmdoperation.cpp \
    $${CONSOLESRC}/cmdserver.cpp \
    $${CONSOLESRC}/consoleserver.cpp \
    main.cpp

TEMPLATE = app

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
