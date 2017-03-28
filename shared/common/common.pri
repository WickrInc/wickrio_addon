wickr_blackout:DEFINES += WICKR_BLACKOUT

CONFIG(release,release|debug) {
    DEFINES += WICKR_PRODUCTION
}
else {
    wickr_beta:DEFINES += WICKR_BETA
    else:wickr_qa:DEFINES += WICKR_QA
    else:DEFINES += WICKR_ALPHA

    DEFINES += WICKR_DEBUG
}

HEADERS += \
    $${PWD}/wbio_common.h \
    $${PWD}/wickrioappsettings.h \
    $${PWD}/wickriodatabase.h \
    $$PWD/wickrioconsoleuser.h \
    $$PWD/wickrioclients.h \
    $$PWD/wickrioapi.h \
    $$PWD/wickriotokens.h \
    $$PWD/wickriomessage.h \
    $$PWD/wickrbotclientdatabase.h

SOURCES += \
    $${PWD}/wbio_common.cpp \
    $${PWD}/wickrioappsettings.cpp \
    $${PWD}/wickriodatabase.cpp \
    $$PWD/wickrioconsoleuser.cpp \
    $$PWD/wickriotokens.cpp \
    $$PWD/wickriomessage.cpp \
    $$PWD/wickrbotclientdatabase.cpp

INCLUDEPATH += $${PWD}
INCLUDEPATH += $${PWD}/../../libs/WickrProto

linux-g++* {
    INCLUDEPATH += $${PWD}/../../platforms/linux/include
}

win32 {
    INCLUDEPATH += $${PWD}/../../platforms/win/include
}

linux-g++* {
    equals(QT_MAJOR_VERSION, 5) {
        equals(QT_MINOR_VERSION, 7) {
            QMAKE_RPATHDIR += /usr/local/wickr/Qt-5.7
            QMAKE_RPATHDIR += /usr/local/wickr/Qt-5.7/lib
        }
        equals(QT_MINOR_VERSION, 8) {
            QMAKE_RPATHDIR += /usr/local/wickr/Qt-5.8
            QMAKE_RPATHDIR += /usr/local/wickr/Qt-5.8/lib
        }
    }

}
