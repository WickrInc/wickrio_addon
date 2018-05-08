# Build this project to generate a shared library (*.dll or *.so).

DEPTH = ../..
COMMON = shared/common

CONFIG += c++11

TARGET = WickrIOLib
TEMPLATE = lib

QT += multimediawidgets
QT -= gui
QT += sql
QT += network websockets

VERSION = 1.0.1

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

mac {
#   QMAKE_MAC_SDK = macosx10.10
   QMAKE_CXXFLAGS += -std=c++11
   QMAKE_LFLAGS_SONAME  = -Wl,-install_name,/usr/local/lib/
}

linux-g++* {
    CONFIG += staticlib
    INCLUDEPATH += $$DEPTH/wickr-sdk/platforms/linux/include
}

win32 {
   DEFINES += WICKRBOTLIB_EXPORT
}

# Windows and Unix get the suffix "d" to indicate a debug version of the library.
# Mac OS gets the suffix "_debug".
CONFIG(debug, debug|release) {
    win32:      TARGET = $$join(TARGET,,,d)
    mac:        TARGET = $$join(TARGET,,,_debug)
    unix:!mac:  TARGET = $$join(TARGET,,,d)
}

INCLUDEPATH += $$DEPTH/wickr-sdk/export
INCLUDEPATH += $$DEPTH/wickr-sdk/src
INCLUDEPATH += $$DEPTH/wickr-sdk/export/Wickr
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/WickrProto
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/qbson
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/libbson
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/cloud/qcloud
INCLUDEPATH += $$DEPTH/shared/common

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    wickrIOAppSettings.h \
    wickrIOBootstrap.h \
    wickrIOCmdState.h \
    wickrIOCommon.h \
    wickrIOIPCService.h \
    wickrIOParsers.h \
    wickrIOServerCommon.h \
    wickrbotactioncache.h \
    wickrbotattachmentcache.h \
    wickrbotclientevents.h \
    wickrbotclients.h \
    wickrbotdatabase.h \
    wickrbotprocessstate.h \
    wickrbotjsondata.h \
    wickrbotlog.h \
    operationdata.h \
    filedownloader.h \
    createjson.h \
    wickrbotipc.h \
    wickrbotutils.h \
    wickrbotsettings.h \
    wickrbotlib.h \
    perftest.h \
    wickrbotstatistics.h \
    clientactions.h \
    loghandler.h \
    wickrIOErrorHandler.h \
    wickrIOAPIInterface.h

SOURCES += \
    wickrIOAppSettings.cpp \
    wickrIOBootstrap.cpp \
    wickrIOCmdState.cpp \
    wickrIOCommon.cpp \
    wickrIOIPCService.cpp \
    wickrIOServerCommon.cpp \
    wickrbotdatabase.cpp \
    wickrbotprocessstate.cpp \
    wickrbotjsondata.cpp \
    wickrbotlog.cpp \
    operationdata.cpp \
    filedownloader.cpp \
    createjson.cpp \
    wickrbotipc.cpp \
    wickrbotutils.cpp \
    perftest.cpp \
    wickrbotstatistics.cpp \
    clientactions.cpp \
    loghandler.cpp \
    wickrIOErrorHandler.cpp \
    wickrIOAPIInterface.cpp
