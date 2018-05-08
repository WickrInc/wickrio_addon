WICKRIOCLIENT_VERSION = 1.0.1

isEmpty(WICKRIOCLIENT_LIBRARY_TYPE) {
    WICKRIOCLIENT_LIBRARY_TYPE = shared
}

WICKRIOCLIENT_INCLUDEPATH = $${PWD}

contains(WICKRIOCLIENT_LIBRARY_TYPE, staticlib) {
    DEFINES += WICKRIOCLIENT_STATIC
} else {
    DEFINES += WICKRIOCLIENT_SHARED
}

isEmpty(PREFIX) {
    unix {
        PREFIX = /usr
    } else {
        PREFIX = $$[QT_INSTALL_PREFIX]
    }
}
isEmpty(LIBDIR) {
    LIBDIR = lib
}

INCLUDEPATH += $${WICKRIOCLIENT_INCLUDEPATH}

macx {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient -lWickrIOClient
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/libWickrIOClient.dylib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient -lWickrIOClient_debug
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/libWickrIOClient_debug.dylib
    }
}
linux-g++* {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/ -lWickrIOClient
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/libWickrIOClient.a
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/ -lWickrIOClientd
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/libWickrIOClientd.a
    }
}
win32 {
    DEFINES += WICKRIOCLIENTLIB_IMPORT

    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/release -lWickrIOClient1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/release/WickrIOClient1.dll
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/debug -lWickrIOClientd1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOClient/debug/WickrIOClientd1.dll
    }
}

#
# Include the RabbitMQ API Library
#
include($${DEPTH}/libs/qamqpsrc/qamqpsrc.pri)

~ 
