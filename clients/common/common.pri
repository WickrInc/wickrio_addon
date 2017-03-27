#
# Include the SMTP API Library
#
include($${DEPTH}/libs/SMTPEmail/SMTPEmail.pri)

HEADERS += \
    $${PWD}/wickrioipc.h \
    $${PWD}/wickriojson.h \
    $${PWD}/wickriomessagecounter.h \
    $${PWD}/wickriothread.h \
    $${PWD}/wickrIOClientMain.h

SOURCES += \
    $${PWD}/wickrioipc.cpp \
    $${PWD}/wickriojson.cpp \
    $$PWD/wickriothread.cpp \
    $${PWD}/wickrIOClientMain.cpp

#    $${PWD}/wickrIOMsgCallbackService.h \
#    $${PWD}/wickrIOMsgEmailService.h \
#    $${PWD}/wickrIOMsgCallbackService.cpp \
#    $${PWD}/wickrIOMsgEmailService.cpp \

INCLUDEPATH += $${PWD}
