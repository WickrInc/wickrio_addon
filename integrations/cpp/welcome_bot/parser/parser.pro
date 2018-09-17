#-------------------------------------------------
#
# Project created by QtCreator 2018-09-13
#
#-------------------------------------------------

message(*** WelcomeBot Parser Integration Build)

DEPTH = ../../../..
COMMON = $${DEPTH}/shared/common
INCLUDEPATH += $${COMMON}

#
# Include the RabbitMQ API Library
#
include($${DEPTH}/libs/qamqpsrc/qamqpsrc.pri)

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
    else:wickr_production {
        DEFINES += WICKR_PRODUCTION
    }
    else {
        DEFINES += WICKR_ALPHA
    }

    DEFINES += WICKR_DEBUG
}

#
# Include the WickrBot library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

QT += core
QT += sql
QT += network
QT += websockets
QT -= gui

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

CONFIG(release,debug|release) {
    TARGET = welcome_parser
    DEFINES += WICKR_PRODUCTION
}
else {
    wickr_beta {
        TARGET = welcome_parserBeta
    }
    else:wickr_production {
        TARGET = welcome_parser
    }
    else {
        TARGET = welcome_parserAlpha
    }

    DEFINES += VERSIONDEBUG
}

# sqlcipher
LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/
LIBS += -L$$PWD/$$DEPTH/clients/libs/WickrIOClientAPI/build/
LIBS += -lWickrIOClientAPI
LIBS += -lsqlcipher
LIBS += -lqsqlcipher_wickr
LIBS += -lsqlite3
LIBS += -lzmq

SOURCES += \
    main.cpp \
    wbparse_qamqpqueue.cpp \
    wickrbotmain.cpp \
    parseroperationdata.cpp \
    welcomeRxProcessing.cpp \
    welcomeIpcService.cpp

HEADERS += \
    welcomeClientConfigInfo.h \
    WickrBotContext.h \
    wbparse_qamqpqueue.h \
    wickrbotmain.h \
    parseroperationdata.h \
    welcomeRxProcessing.h \
    welcomeIpcService.h

INCLUDEPATH +=$${DEPTH}/wickr-sdk/src
INCLUDEPATH +=$${DEPTH}/wickr-sdk
INCLUDEPATH +=$${DEPTH}/wickr-sdk/export
INCLUDEPATH +=$${DEPTH}/wickr-sdk/wickr-core-c/include
INCLUDEPATH +=$${DEPTH}/wickr-sdk/libs/qbson
INCLUDEPATH +=$${DEPTH}/wickr-sdk/libs/libbson
INCLUDEPATH +=$${DEPTH}/clients/libs/WickrIOClientAPI/src
