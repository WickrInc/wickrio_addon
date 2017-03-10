#
# Include the SMTP API Library
#
include(../libs/SMTPEmail/SMTPEmail.pri)

HEADERS += \
    $${PWD}/wickrIOMsgCallbackService.h \
    $${PWD}/wickrIOMsgEmailService.h \
    $${PWD}/wickrioipc.h \
    $${PWD}/wickriojson.h \
    $${PWD}/wickriomessagecounter.h \
    $${PWD}/wickrioreceivethread.h \
    $$PWD/wickriothread.h \
    $$PWD/wickriocallbackthread.h

SOURCES += \
    $${PWD}/wickrIOMsgCallbackService.cpp \
    $${PWD}/wickrIOMsgEmailService.cpp \
    $${PWD}/wickrioipc.cpp \
    $${PWD}/wickriojson.cpp \
    $${PWD}/wickrioreceivethread.cpp \
    $$PWD/wickriothread.cpp \
    $$PWD/wickriocallbackthread.cpp

INCLUDEPATH += $${PWD}
