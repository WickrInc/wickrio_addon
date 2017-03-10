DEPTH = ../../..

message(*** WickrIO Service Control build)

CONFIG(release,release|debug) {
} else {
  DEFINES += VERSIONDEBUG
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
# Include the Wickr IO library
#
include(../libs/WickrIOLib/WickrIOLib.pri)

unix:!macx:QMAKE_RPATHDIR += $${OUT_PWD}/../libs/WickrIOLib

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
