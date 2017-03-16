#
# Include the SMTP API Library
#
include(../libs/SMTPEmail/SMTPEmail.pri)

HEADERS += \
    $${PWD}/wickrioipc.h \
    $${PWD}/wickriojson.h \
    $${PWD}/wickriomessagecounter.h \
    $${PWD}/wickrioreceivethread.h \
    $$PWD/wickriothread.h

SOURCES += \
    $${PWD}/wickrioipc.cpp \
    $${PWD}/wickriojson.cpp \
    $${PWD}/wickrioreceivethread.cpp \
    $$PWD/wickriothread.cpp

#    $${PWD}/wickrIOMsgCallbackService.h \
#    $${PWD}/wickrIOMsgEmailService.h \
#    $${PWD}/wickrIOMsgCallbackService.cpp \
#    $${PWD}/wickrIOMsgEmailService.cpp \

INCLUDEPATH += $${PWD}
