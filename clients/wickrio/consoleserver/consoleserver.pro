DEPTH = ../../..
COMMON = ../common

#
# Include the Wickr IO common files
#
include($${COMMON}/common.pri)

CONFIG += c++11

CONFIG -= console

CONFIG(release,release|debug) {
    message(*** WickrIO ConsoleServer Release Build)
    wickr_prod:TARGET = WickrIOCSvr
    else:TARGET = WickrIOCSvrPreview

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
    wickr_beta:TARGET = WickrIOCSvrBeta
    else:TARGET = WickrIOCSvrAlpha
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
include(../common_console/common_console.pri)

#
# Include the Wickr IO common HTTP files
#
include(../common_http/common_http.pri)

#
# Include the QtWebApp library
#
include(../libs/QtWebApp/QtWebApp.pri)

#
# Include the Wickr IO library
#
include(../libs/WickrIOLib/WickrIOLib.pri)

#
# Include the SMTP API Library
#
include(../libs/SMTPEmail/SMTPEmail.pri)

TEMPLATE = app

QT  += core sql
QT  += network
QT  -= gui

CONFIG += depend_includepath

SOURCES += \
    cmdhandler.cpp \
    main.cpp \
    consoleserverservice.cpp

HEADERS += \
    cmdhandler.h \
    WickrBotContext.h \
    consoleserverservice.h

CONFIG(release,release|debug) {
    SOURCES += ../common/versiondebugNO.cpp
}
else {
    SOURCES += ../common/versiondebugYES.cpp
}


#---------------------------------------------------------------------------------------
# The following lines import the shared QtWebApp library.
# You may need to modify the path names to match your computer.
#---------------------------------------------------------------------------------------

CONFIG += depend_includepath
