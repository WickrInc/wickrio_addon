QTWEBAPP_VERSION = 1.7.2

isEmpty(QTWEBAPP_LIBRARY_TYPE) {
    QTWEBAPP_LIBRARY_TYPE = shared
}

QT += network
QTWEBAPP_INCLUDEPATH = $${PWD}
QTWEBAPP_LIBS = -lqtwebapp
contains(QTWEBAPP_LIBRARY_TYPE, staticlib) {
    DEFINES += QTWEBAPP_STATIC
} else {
    DEFINES += QTWEBAPP_SHARED
    win32:QTWEBAPP_LIBS = -lqtwebapp0
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

INCLUDEPATH += $$QTWEBAPP_INCLUDEPATH

macx {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp -lQtWebApp
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/libQtWebApp.dylib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp -lQtWebApp_debug
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/libQtWebApp_debug.dylib
    }
}
linux-g++* {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/ -lQtWebApp
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/libQtWebApp.a
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/ -lQtWebAppd
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/libQtWebAppd.a
    }
}
win32 {
    DEFINES += QTWEBAPPLIB_IMPORT

    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/release -lQtWebApp
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/release/QtWebApp.lib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/debug -lQtWebAppd
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/clients/wickrio/libs/QtWebApp/debug/QtWebAppd.lib
    }
}
