WICKRIOAPIIFACE_VERSION = 1.0.1

isEmpty(WICKRIOAPIIFACE_LIBRARY_TYPE) {
    WICKRIOAPIIFACE_LIBRARY_TYPE = shared
}

WICKRIOAPIIFACE_INCLUDEPATH = $${PWD}

contains(WICKRIOAPIIFACE_LIBRARY_TYPE, staticlib) {
    DEFINES += WICKRIOAPIIFACE_STATIC
} else {
    DEFINES += WICKRIOAPIIFACE_SHARED
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

INCLUDEPATH += $${WICKRIOAPIIFACE_INCLUDEPATH}

macx {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface -lWickrIOAPIIface
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/libWickrIOAPIIface.dylib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface -lWickrIOAPIIface_debug
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/libWickrIOAPIIface_debug.dylib
    }
}
linux-g++* {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/ -lWickrIOAPIIface
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/libWickrIOAPIIface.a
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/ -lWickrIOAPIIfaced
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/libWickrIOAPIIfaced.a
    }
}
win32 {
    DEFINES += WICKRIOAPIIFACELIB_IMPORT

    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/release -lWickrIOAPIIface1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/release/WickrIOAPIIface1.dll
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/debug -lWickrIOAPIIfaced1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/libs/WickrIOAPIIface/debug/WickrIOAPIIfaced1.dll
    }
}

~ 
