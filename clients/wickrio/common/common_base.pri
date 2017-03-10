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
    $${PWD}/wickrioappsettings.h

SOURCES += \
    $${PWD}/wbio_common.cpp \
    $${PWD}/wickrioappsettings.cpp

INCLUDEPATH += $${PWD}
