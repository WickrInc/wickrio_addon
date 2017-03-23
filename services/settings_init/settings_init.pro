DEPTH = ../..

message(*** WickrIO Settings Init build)

CONFIG(release,release|debug) {
} else {
  DEFINES += VERSIONDEBUG
}

#
# Include the Wickr IO common console files
#
SERVER_COMMON=../common
INCLUDEPATH += $${SERVER_COMMON}

#
# Include the Wickr IO common files
#
include($${DEPTH}/shared/common/common_base.pri)

#
# Include the Wickr IO library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

QT += widgets
QT += sql
QT -= gui

CONFIG += c++11

HEADERS = \
    webserver.h

SOURCES = \
    main.cpp \
    webserver.cpp

# install
TARGET = wickrio_settings_init
target.path = /usr/bin
INSTALLS += target

#CONFIG   += console
#CONFIG   -= app_bundle

TEMPLATE = app

CONFIG += depend_includepath

