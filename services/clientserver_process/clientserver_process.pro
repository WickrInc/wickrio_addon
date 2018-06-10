DEPTH = ../..
COMMON = $${DEPTH}/shared/common
CONSOLESRC = ../console

CONFIG(release,release|debug) {
    QMAKE_RPATHDIR += /usr/lib/wio_docker_services
}
else {
    QMAKE_RPATHDIR += /usr/lib/wio_docker_services-debug
}

#
# Include the WickrIO common files
#
include($${COMMON}/common.pri)

CONFIG += c++11

#CONFIG -= console

CONFIG(release,release|debug) {
    message(*** WickrIO ClientServer Process Release Build)
    TARGET = WickrIOSvr

    macx {
        ICON = $${COMMON}/Wickr_prod.icns
        QMAKE_INFO_PLIST = $${DEPTH}/ConsumerProd.Info.plist
    }
    win32 {
        RC_FILE = $${COMMON}/Wickr.rc
        ICON = $${COMMON}/Wickr.ico
    }
}
else {
    message(*** WickrIO ClientServer Process Debug Build)
    TARGET = WickrIOSvrDebug

    DEFINES += VERSIONDEBUG

    macx {
        ICON = $${COMMON}/Wickr_beta.icns
        QMAKE_INFO_PLIST = $$DEPTH/ConsumerBeta.Info.plist
    }
    win32 {
        RC_FILE = $${COMMON}/Wickr-beta.rc
        ICON = $${COMMON}/Wickr-beta.ico
    }
}

win32 {
#    QMAKE_LFLAGS_CONSOLE +=
}

#
# Include the WickrIO common server files
#
SERVER_COMMON=../common
INCLUDEPATH += $${SERVER_COMMON}

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

#
# Include the Wickr Console library
#
include($${DEPTH}/libs/WickrIOConsole/WickrIOConsole.pri)

#
# Include the QtWebApp library
#
include($${DEPTH}/libs/QtWebApp/QtWebApp.pri)

#
# Include the Wickr library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

INCLUDEPATH += $${CONSOLESRC}

INCLUDEPATH += $$DEPTH/wickr-sdk/export
INCLUDEPATH += $$DEPTH/wickr-sdk/export/Wickr
INCLUDEPATH += $$DEPTH/wickr-sdk/src
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/qbson
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/libbson

TEMPLATE = app

QT += core sql
QT += network
QT -= gui
QT += multimediawidgets
QT += websockets

CONFIG += depend_includepath

SOURCES += \
    $${COMMON}/cmdbase.cpp \
    main.cpp \
    wickrioclientserverprocess.cpp \
    wickrioprocesscommand.cpp

HEADERS += \
    $${COMMON}/cmdbase.h \
    WickrBotContext.h \
    wickrioclientserverprocess.h \
    wickrioprocesscommand.h


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
    INCLUDEPATH += $$DEPTH/wickr-sdk/platforms/linux/include
}
LIBS += -lqsqlcipher_wickr

# sqlcipher

LIBS += -lsqlcipher

INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/WickrProto
