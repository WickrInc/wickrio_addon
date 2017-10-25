# Build this project to generate a shared library (*.dll or *.so).

DEPTH = ../..

CONFIG += c++11

TARGET = WickrIOConsole
TEMPLATE = lib

QT += multimediawidgets
QT -= gui
QT += sql
QT += network

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
   DEFINES += WICKRCONSOLELIB_EXPORT
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

INCLUDEPATH += $$PWD
INCLUDEPATH += ../WickrIOLib
INCLUDEPATH += $$DEPTH/shared/common
INCLUDEPATH += $$DEPTH/services/common
DEPENDPATH += $$PWD

HEADERS += \
    cmdadvanced.h \
    cmdclient.h \
    cmdconsole.h \
    cmdmain.h \
    cmdoperation.h \
    cmdserver.h \
    consoleserver.h \
    webserver.h \
    wickrIOConsoleClientHandler.h

SOURCES += \
    cmdadvanced.cpp \
    cmdclient.cpp \
    cmdconsole.cpp \
    cmdmain.cpp \
    cmdoperation.cpp \
    cmdserver.cpp \
    consoleserver.cpp \
    webserver.cpp \
    wickrIOConsoleClientHandler.cpp
