message(*** Wickr Base QT Unit Testing)

DEPTH=..

QT += testlib
QT -= gui
QT += sql
QT += network
QT += widgets
QT += xml
QT += websockets

TARGET = maintest
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    maintest.cpp \
    inittest.cpp \
    clientversioninfo.cpp \
    clientconfigurationinfo.cpp \
    wickrapplication.cpp

HEADERS += \
    inittest.h \
    clientversioninfo.h \
    clientconfigurationinfo.h \
    wickrapplication.h

INCLUDEPATH += $$PWD/$${DEPTH}/src
INCLUDEPATH += $$PWD/$${DEPTH}/export

CONFIG(release,release|debug) {
    BUILD_TYPE=release
}
else {
    BUILD_TYPE=debug
}

INCLUDEPATH += $$PWD/$${DEPTH}/platforms/common/include

mac {
    QMAKE_LFLAGS += -F$$PWD/$${DEPTH}/platforms/mac/lib64

    LIBS += -L$$PWD/$${DEPTH}/platforms/mac/lib64/
    CONFIG(release,release|debug) {
        message(*** NOT Adding HockeySDK Framework)
    }
    else {
        message(*** Adding HockeySDK Framework)
        LIBS += -framework HockeySDK
    }

    INCLUDEPATH += $$PWD/$${DEPTH}/platforms/mac/include
    LIBS += -L$$OUT_PWD/$$DEPTH/src -lwickr-desktop-base

    LIBS += -framework AppKit -framework AddressBook -framework SystemConfiguration
    LIBS += -framework Foundation

    LIBS += -framework CoreFoundation -framework Carbon -lobjc
    LIBS += -framework ApplicationServices -framework CoreMedia

    QMAKE_POST_LINK += install_name_tool -change "/usr/local/lib/libwickr-desktop-base.1.dylib" "$${OUT_PWD}/$${DEPTH}/src/libwickr-desktop-base.1.dylib" "$${TARGET}" ;
    !CONFIG(release,release|debug):QMAKE_POST_LINK += install_name_tool -change @rpath/HockeySDK.framework/Versions/A/HockeySDK "$$PWD/$$DEPTH/platforms/mac/lib64/HockeySDK.framework/Versions/A/HockeySDK" $${TARGET};

}

win32 {
    DEFINES += WIN32_LEAN_AND_MEAN

    INCLUDEPATH += $$PWD/$$DEPTH/platforms/win/include
    LIBS += -L$$OUT_PWD/$$DEPTH/src/$${BUILD_TYPE} -lwickr-desktop-base
    LIBS += -L$$PWD/$$DEPTH/platforms/win/lib32/$${BUILD_TYPE} -lWickrCore
    LIBS += -L$$PWD/$$DEPTH/platforms/win/lib32
    LIBS += -lCFLite

#    LIBS += -lprotoc

    LIBS += -L$$OUT_PWD/$${DEPTH}/libs/qbson/$${BUILD_TYPE} -lqbson \
            -L$$OUT_PWD/$${DEPTH}/libs/libbson/$${BUILD_TYPE} -lbson \
            -L$$OUT_PWD/$${DEPTH}/libs/cloud/qcloud/$${BUILD_TYPE} -lQCloud \
            -L$$OUT_PWD/$${DEPTH}/libs/WickrProto/$${BUILD_TYPE}
    CONFIG(release,release|debug) {
        LIBS += -lWickrProto -lprotobuf
    }
    else {
        LIBS += -lWickrProtod -lprotobufd
    }

win32-msvc*:LIBS += -llibeay32 -ladvapi32 -luser32
win32-msvc*:QMAKE_LFLAGS += /NODEFAULTLIB:libcmt
win32-g++:LIBS += -lcrypto
win32:LIBS += -lgdi32
}

linux-g++* {
    INCLUDEPATH += $$PWD/$$DEPTH/platforms/linux/include
    LIBS += -L$$OUT_PWD/$$DEPTH/src -lwickr-desktop-base

    LIBS += -L$$PWD/$$DEPTH/platforms/linux/generic-64
    LIBS += -L$$PWD/$$DEPTH/platforms/linux/generic-64/$${BUILD_TYPE}

    LIBS += -lWickrCoreC
    LIBS += -lbsd -luuid -ldl
    LIBS += -lssl
    LIBS += -lstdc++
    LIBS += -lprotobuf
    LIBS += -lcrypto

    LIBS += -L$$OUT_PWD/$${DEPTH}/libs/qbson -lqbson \
            -L$$OUT_PWD/$${DEPTH}/libs/libbson -lbson \
            -L$$OUT_PWD/$${DEPTH}/libs/cloud/qcloud -lQCloud \
            -L$$OUT_PWD/$${DEPTH}/libs/WickrProto -lWickrProto

}

INCLUDEPATH += $$PWD/$${DEPTH}/libs/qbson
INCLUDEPATH += $$PWD/$${DEPTH}/libs/libbson
INCLUDEPATH += $$PWD/$${DEPTH}/libs/cloud/qcloud
INCLUDEPATH += $$PWD/$${DEPTH}/libs/WickrProto
