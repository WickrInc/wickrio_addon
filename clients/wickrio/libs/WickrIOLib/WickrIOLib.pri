WICKRIOLIB_VERSION = 1.0.1

isEmpty(WICKRIOLIB_LIBRARY_TYPE) {
    WICKRIOLIB_LIBRARY_TYPE = shared
}

QT += network
WICKRIOLIB_INCLUDEPATH = $$PWD
WICKRIOLIB_LIBS = -lwickrbot
contains(WICKRIOLIB_LIBRARY_TYPE, staticlib) {
    DEFINES += WICKRIOLIB_STATIC
} else {
    DEFINES += WICKRIOLIB_SHARED
    win32:WICKRIOLIB_LIBS = -lwickrbot0
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

INCLUDEPATH += $$WICKRIOLIB_INCLUDEPATH

macx {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib -lWickrIOLib
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/libWickrIOLib.dylib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib -lWickrIOLib_debug
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/libWickrIOLib_debug.dylib
    }
}
linux-g++* {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/ -lWickrIOLib
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/libWickrIOLib.a
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/ -lWickrIOLibd
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/libWickrIOLibd.a
    }
}
win32 {
    DEFINES += WICKRIOLIBLIB_IMPORT

    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/release -lWickrIOLib1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/release/WickrIOLib1.lib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/debug -lWickrIOLibd1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/WickrIOLib/debug/WickrIOLibd1.lib
    }
}

