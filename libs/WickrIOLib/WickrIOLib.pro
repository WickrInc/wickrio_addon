# Build this project to generate a shared library (*.dll or *.so).

DEPTH = ../..

CONFIG += c++11

TARGET = WickrIOLib
TEMPLATE = lib

QT -= gui
QT += sql
QT += network

VERSION = 1.0.1

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

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    wickrIOBootstrap.h \
    wickrbotactioncache.h \
    wickrbotattachmentcache.h \
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
    clientactions.h

SOURCES += \
    wickrIOBootstrap.cpp \
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
    clientactions.cpp
