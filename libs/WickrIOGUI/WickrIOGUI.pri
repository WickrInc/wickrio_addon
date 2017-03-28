WICKRIOGUI_VERSION = 1.0.1

isEmpty(WICKRIOGUI_LIBRARY_TYPE) {
    WICKRIOGUI_LIBRARY_TYPE = shared
}

QT += gui widgets
WICKRIOGUI_INCLUDEPATH = $${PWD}

contains(WICKRIOGUI_LIBRARY_TYPE, staticlib) {
    DEFINES += WICKRIOGUI_STATIC
} else {
    DEFINES += WICKRIOGUI_SHARED
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

INCLUDEPATH += $${WICKRIOGUI_INCLUDEPATH}

macx {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOGUI -lWickrIOGUI
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOGUI/libWickrIOGUI.dylib
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOGUI -lWickrIOGUI_debug
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOGUI/libWickrIOGUI_debug.dylib
    }
}
linux-g++* {
    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOGUI/ -lWickrIOGUI
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOGUI/libWickrIOGUI.a
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOGUI/ -lWickrIOGUId
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOGUI/libWickrIOGUId.a
    }
}
win32 {
    DEFINES += WICKRIOGUILIB_IMPORT

    CONFIG(release,debug|release) {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOGUI/release -lWickrIOGUI1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOGUI/release/WickrIOGUI1.dll
    }
    else {
        LIBS += -L$$OUT_PWD/$$DEPTH/libs/WickrIOGUI/debug -lWickrIOGUId1
        PRE_TARGETDEPS += $$OUT_PWD/$$DEPTH/libs/WickrIOGUI/debug/WickrIOGUId1.dll
    }
}

~ 
