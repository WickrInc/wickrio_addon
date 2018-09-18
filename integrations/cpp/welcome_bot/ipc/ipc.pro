#-------------------------------------------------
#-------------------------------------------------

message(*** WelcomeBot IPC Build)

DEPTH = ../../../..
COMMON = $${DEPTH}/shared/common
INCLUDEPATH += $${COMMON}

#
# Zero MQ Qt library
#
include($${DEPTH}/libs/nzmqt/nzmqt.pri)

linux-g++* {
    CONFIG(release,release|debug) {
        QMAKE_RPATHDIR += /usr/lib/wio_welcome_bot
    }
    else {
        wickr_blackout {
            QMAKE_RPATHDIR = /usr/lib/wio_welcome_bot-onprem
        }
        else:wickr_beta {
            QMAKE_RPATHDIR = /usr/lib/wio_welcome_bot-beta
        }
        else:wickr_production {
            QMAKE_RPATHDIR = /usr/lib/wio_welcome_bot
        }
        else {
            QMAKE_RPATHDIR = /usr/lib/wio_welcome_bot-alpha
        }
    }
}

COMMON = $${DEPTH}/shared/common

QT += core

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

CONFIG(release,debug|release) {
    TARGET = welcome_ipc
}
else {
    TARGET = welcome_ipc
    DEFINES += VERSIONDEBUG
}

LIBS += -lzmq

SOURCES += \
    main.cpp \
    ipcSendMessage.cpp

HEADERS += \
    ipcSendMessage.h
