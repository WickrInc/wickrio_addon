# Build this project to generate a shared library (*.dll or *.so).

DEPTH = ../../..

TARGET = WickrIOClient
TEMPLATE = lib

CONFIG += qt

QT += network websockets
QT += sql multimediawidgets xml
QT -= gui

VERSION = 1.0.1

mac {
#   QMAKE_MAC_SDK = macosx10.10
   equals(QT_MINOR_VERSION, 4) {  #assuming major version is 5 and mixed building with 5.4.2 and 5.5.1
       QMAKE_CXXFLAGS += -std=c++11
   }
   QMAKE_LFLAGS_SONAME  = -Wl,-install_name,/usr/local/lib/
}

linux-g++* {
    CONFIG += staticlib
    INCLUDEPATH += $$DEPTH/wickr-sdk/platforms/linux/include
    INCLUDEPATH += $$DEPTH
}

win32 {
   DEFINES += WICKRIOCLIENTLIB_EXPORT
}

# Windows and Unix get the suffix "d" to indicate a debug version of the library.
# Mac OS gets the suffix "_debug".
CONFIG(debug, debug|release) {
    win32:      TARGET = $$join(TARGET,,,d)
    mac:        TARGET = $$join(TARGET,,,_debug)
    unix:!mac:  TARGET = $$join(TARGET,,,d)
}

INCLUDEPATH += $$PWD
INCLUDEPATH += $$DEPTH/wickr-sdk/src
INCLUDEPATH += $$DEPTH/wickr-sdk/export
INCLUDEPATH += $$DEPTH/wickr-sdk/export/Wickr
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/WickrProto
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/cloud/qcloud
INCLUDEPATH += $$DEPTH/libs/WickrIOLib
INCLUDEPATH += $$DEPTH/libs/SMTPEmail/src
INCLUDEPATH += $$DEPTH/shared/common
DEPENDPATH += $$PWD

HEADERS += \
    wickrIOActionService.h \
    wickrIOCallbackService.h \
    wickrIOClientRuntime.h \
    wickrIOFileDownloadService.h \
    wickrIOJson.h \
    wickrIOMessageCounter.h \
    wickrIORxService.h \
    wickrIOThread.h \
    wickrIOWatchdogService.h \
    wickrIOServiceBase.h

SOURCES += \
    wickrIOActionService.cpp \
    wickrIOCallbackService.cpp \
    wickrIOClientRuntime.cpp \
    wickrIOFileDownloadService.cpp \
    wickrIOJson.cpp \
    wickrIORxService.cpp \
    wickrIOThread.cpp \
    wickrIOWatchdogService.cpp \
    wickrIOServiceBase.cpp
