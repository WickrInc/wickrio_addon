message(*** Wickr Base QT Unit Testing)

DEPTH=..

CONFIG += c++11
CONFIG += console
CONFIG -= app_bundle

CONFIG(release,release|debug) {
    BUILD_TYPE=release
    DEFINES += WICKR_PROD
}
else {
    BUILD_TYPE=debug

    wickr_beta:DEFINES += WICKR_BETA
    else:wickr_qa:DEFINES += WICKR_QA
    else:DEFINES += WICKR_ALPHA
}

QT += testlib
QT -= gui
QT += sql
QT += network
QT += widgets
QT += xml
QT += websockets

TARGET = maintest
TEMPLATE = app

#
# Include the Wickr IO library
#
include(../clients/wickrio/libs/WickrIOLib/WickrIOLib.pri)

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

# qsqlcipher_wickr

win32 {
    CONFIG(debug, debug|release):LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/debug
    else:LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/release
} else {
    LIBPATH += $$DEPTH/wickr-sdk/libs/qsqlcipher_wickr/
}
LIBS += -lqsqlcipher_wickr

# sqlcipher

LIBS += -lsqlcipher

INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/src
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/export
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/platforms/common/include
INCLUDEPATH += $$PWD/$${DEPTH}/clients/wickrio
INCLUDEPATH += $$PWD/$${DEPTH}/clients/wickrio/libs/WickrIOLib

mac {
    QMAKE_LFLAGS += -F$$PWD/$${DEPTH}/platforms/mac/lib64

    LIBS += -L$$PWD/$${DEPTH}/wickr-sdk/platforms/mac/lib64/
    CONFIG(release,release|debug) {
        message(*** NOT Adding HockeySDK Framework)
    }
    else {
        message(*** Adding HockeySDK Framework)
        LIBS += -framework HockeySDK
    }

    INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/platforms/mac/include
    LIBS += -L$$OUT_PWD/$$DEPTH/wickr-sdk/src -lwickr-sdk

    LIBS += -framework AppKit -framework AddressBook -framework SystemConfiguration
    LIBS += -framework Foundation

    LIBS += -framework CoreFoundation -framework Carbon -lobjc
    LIBS += -framework ApplicationServices -framework CoreMedia

    QMAKE_POST_LINK += install_name_tool -change "/usr/local/lib/libwickr-sdk.1.dylib" "$${OUT_PWD}/$${DEPTH}/wickr-sdk/src/libwickr-sdk.1.dylib" "$${TARGET}" ;
    !CONFIG(release,release|debug):QMAKE_POST_LINK += install_name_tool -change @rpath/HockeySDK.framework/Versions/A/HockeySDK "$$PWD/$$DEPTH/wickr-sdk/platforms/mac/lib64/HockeySDK.framework/Versions/A/HockeySDK" $${TARGET};

}

win32 {
    DEFINES += WIN32_LEAN_AND_MEAN

    INCLUDEPATH += $$PWD/$$DEPTH/wickr-sdk/platforms/win/include
    LIBS += -L$$OUT_PWD/$$DEPTH/wickr-sdk/src/$${BUILD_TYPE} -lwickr-sdk
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/win/lib32/$${BUILD_TYPE} -lWickrCore
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/win/lib32
    LIBS += -lCFLite

#    LIBS += -lprotoc

    LIBS += -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/qbson/$${BUILD_TYPE} -lqbson \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/libbson/$${BUILD_TYPE} -lbson \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/cloud/qcloud/$${BUILD_TYPE} -lQCloud \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/WickrProto/$${BUILD_TYPE}
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
    INCLUDEPATH += $$PWD/$$DEPTH/wickr-sdk/platforms/linux/include
    LIBS += -L$$OUT_PWD/$$DEPTH/wickr-sdk/src -lwickr-sdk

    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/linux/generic-64
    LIBS += -L$$PWD/$$DEPTH/wickr-sdk/platforms/linux/generic-64/$${BUILD_TYPE}

    LIBS += -lWickrCoreC
    LIBS += -lbsd -luuid -ldl
    LIBS += -lssl
    LIBS += -lstdc++
    LIBS += -lprotobuf
    LIBS += -lcrypto

    LIBS += -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/qbson -lqbson \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/libbson -lbson \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/cloud/qcloud -lQCloud \
            -L$$OUT_PWD/$${DEPTH}/wickr-sdk/libs/WickrProto -lWickrProto

}

INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/qbson
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/libbson
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/cloud/qcloud
INCLUDEPATH += $$PWD/$${DEPTH}/wickr-sdk/libs/WickrProto
