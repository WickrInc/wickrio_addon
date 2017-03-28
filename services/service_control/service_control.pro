DEPTH = ../..

message(*** WickrIO Service Control build)

#
# Include the Wickr IO services base
#
include(../services.pri)

CONFIG(release,release|debug) {
} else {
  DEFINES += VERSIONDEBUG
}

#
# Include the Wickr IO common files
#
include($${DEPTH}/shared/common/common_base.pri)

#
# Include the QtWebApp library
#
include($${DEPTH}/libs/QtWebApp/QtWebApp.pri)

#
# Include the Wickr IO library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

QT += widgets
QT += sql
QT -= gui

CONFIG += c++11

SOURCES = \
    main.cpp

# install
TARGET = wickrio_service_control
target.path = /usr/bin
INSTALLS += target

#CONFIG   += console
#CONFIG   -= app_bundle

TEMPLATE = app

CONFIG += depend_includepath
