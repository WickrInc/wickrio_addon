# Build this project to generate a shared library (*.dll or *.so).

DEPTH = ../../..

TARGET = WickrIOAPIIface
TEMPLATE = lib

CONFIG += qt

QT += network websockets
QT += sql multimediawidgets xml
QT -= gui

VERSION = 1.0.1

#
# Include the RabbitMQ API Library
#
include($${DEPTH}/libs/qamqpsrc/qamqpsrc.pri)
INCLUDEPATH += $${QAMQP_INCLUDEPATH}

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
   DEFINES += WICKRIOAPIIFACELIB_EXPORT
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
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/qbson
INCLUDEPATH += $$DEPTH/wickr-sdk/libs/libbson
INCLUDEPATH += $$DEPTH/libs/WickrIOLib
INCLUDEPATH += $$DEPTH/libs/SMTPEmail/src
INCLUDEPATH += $$DEPTH/shared/common
DEPENDPATH += $$PWD

HEADERS += \
    wickrIOAPIIface.h

SOURCES += \
    wickrIOAPIIface.cpp
