# Build this project to generate a shared library (*.dll or *.so).

DEPTH = ../../../..
TARGET = WickrIOGUI
TEMPLATE = lib

QT += gui widgets
QT -= sql
QT -= network

CONFIG += qt

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
}

win32 {
   DEFINES += WICKRIOGUILIB_EXPORT
}

# Windows and Unix get the suffix "d" to indicate a debug version of the library.
# Mac OS gets the suffix "_debug".
CONFIG(debug, debug|release) {
    win32:      TARGET = $$join(TARGET,,,d)
    mac:        TARGET = $$join(TARGET,,,_debug)
    unix:!mac:  TARGET = $$join(TARGET,,,d)
}

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    wickrbotmessagebox.h \
    wickrbotguilib.h

SOURCES += \
    wickrbotmessagebox.cpp
