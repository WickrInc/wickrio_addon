WICKRIOCONSOLE_VERSION = 1.0.1

isEmpty(WICKRIOCONSOLE_LIBRARY_TYPE) {
    WICKRIOCONSOLE_LIBRARY_TYPE = shared
}

QT += network
WICKRIOCONSOLE_INCLUDEPATH = $$PWD
WICKRIOCONSOLE_LIBS = -lwickrbot
contains(WICKRIOCONSOLE_LIBRARY_TYPE, staticlib) {
    DEFINES += WICKRIOCONSOLE_STATIC
} else {
    DEFINES += WICKRIOCONSOLE_SHARED
    win32:WICKRIOCONSOLE_LIBS = -lwickrbot0
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

INCLUDEPATH += $$WICKRIOCONSOLE_INCLUDEPATH

macx {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOConsole -lWickrIOConsole
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOConsole/libWickrIOConsole.dylib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOConsole -lWickrIOConsole_debug
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOConsole/libWickrIOConsole_debug.dylib
    }
}
linux-g++* {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOConsole/ -lWickrIOConsole
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOConsole/libWickrIOConsole.a
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOConsole/ -lWickrIOConsoled
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOConsole/libWickrIOConsoled.a
    }
}
win32 {
    DEFINES += WICKRIOCONSOLELIB_IMPORT

    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOConsole/release -lWickrIOConsole1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOConsole/release/WickrIOConsole1.dll
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOConsole/debug -lWickrIOConsoled1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOConsole/debug/WickrIOConsoled1.dll
    }
}

