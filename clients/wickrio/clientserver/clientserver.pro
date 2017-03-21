DEPTH = ../../..
COMMON = ../common

CONFIG += c++11

CONFIG -= console

CONFIG(release,release|debug) {
    message(*** WickrIO ClientServer Release Build)
    wickr_prod:TARGET = WickrIOSvr
    else:TARGET = WickrIOSvrPreview

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
    message(*** WickrIO ClientServer Beta Build)
    wickr_beta:TARGET = WickrIOSvrBeta
    else:TARGET = WickrIOSvrAlpha
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

win32 {
#    QMAKE_LFLAGS_CONSOLE +=
}

#
# Include the Wickr IO common files
#
include(../common/common_base.pri)

#
# Include the QtWebApp library
#
include(../libs/QtWebApp/QtWebApp.pri)

#
# Include the Wickr library
#
include(../libs/WickrIOLib/WickrIOLib.pri)

TEMPLATE = app

QT  += core sql
QT  += network
QT  -= gui

CONFIG += depend_includepath

SOURCES += \
    main.cpp \
    wickrioclientserverservice.cpp

HEADERS += \
    WickrBotContext.h \
    wickrioclientserverservice.h

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
