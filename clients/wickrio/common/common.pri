CONFIG(release,release|debug) {
    wickr_prod:DEFINES += WICKR_TARGET_PROD
    else:DEFINES += WICKR_TARGET_PREVIEW
}
else {
    wickr_beta:DEFINES += WICKR_TARGET_BETA
    else:DEFINES += WICKR_TARGET_ALPHA
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
INCLUDEPATH += $${PWD}/../../common
INCLUDEPATH += $${PWD}/$${DEPTH}/src
INCLUDEPATH += $${PWD}/$${DEPTH}/export
INCLUDEPATH += $$PWD/$${DEPTH}/libs/WickrProto

linux-g++* {
    INCLUDEPATH += $${PWD}/$${DEPTH}/platforms/linux/include
}

win32 {
    INCLUDEPATH += $${PWD}/$${DEPTH}/platforms/win/include
}
