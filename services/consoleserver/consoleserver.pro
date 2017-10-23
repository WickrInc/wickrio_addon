DEPTH = ../..
COMMON = $${DEPTH}/shared/common

#
# Include the Wickr IO services base
#
include(../services.pri)

#
# Include the Wickr IO common defines files
#
include($${COMMON}/common_defines.pri)

#
# Include the Wickr IO common files
#
include($${COMMON}/common.pri)

CONFIG += c++11

CONFIG -= console

CONFIG(release,release|debug) {
    message(*** WickrIO ConsoleServer Release Build)
    TARGET = WickrIOCSvr

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
    message(*** WickrIO ConsoleServer Beta Build)
    TARGET = WickrIOCSvrDebug

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
# Include the Wickr IO common console files
#
SERVER_COMMON=../common
INCLUDEPATH += $${SERVER_COMMON}

#
# Include the Wickr IO common HTTP files
#
include($${DEPTH}/shared/common_http/common_http.pri)

#
# Include the QtWebApp library
#
include($${DEPTH}/libs/QtWebApp/QtWebApp.pri)

#
# Include the Wickr IO library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

#
# Include the SMTP API Library
#
include($${DEPTH}/libs/SMTPEmail/SMTPEmail.pri)

TEMPLATE = app

QT  += core sql
QT  += network
QT  -= gui

CONFIG += depend_includepath

SOURCES += \
    $${SERVER_COMMON}/server_common.cpp \
    $${SERVER_COMMON}/wickrioconsoleclienthandler.cpp \
    cmdhandler.cpp \
    main.cpp \
    consoleserverservice.cpp

HEADERS += \
    $${SERVER_COMMON}/server_common.h \
    $${SERVER_COMMON}/wickrioconsoleclienthandler.h \
    cmdhandler.h \
    WickrBotContext.h \
    consoleserverservice.h

CONFIG(release,release|debug) {
    SOURCES += $${COMMON}/versiondebugNO.cpp
}
else {
    SOURCES += $${COMMON}/versiondebugYES.cpp
}


#---------------------------------------------------------------------------------------
# The following lines import the shared QtWebApp library.
# You may need to modify the path names to match your computer.
#---------------------------------------------------------------------------------------

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

