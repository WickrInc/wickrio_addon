HEADERS += \
    $${PWD}/wickrioipc.h \
    $${PWD}/wickriojson.h \
    $${PWD}/wickriomessagecounter.h \
    $${PWD}/wickriothread.h

SOURCES += \
    $${PWD}/wickrioipc.cpp \
    $${PWD}/wickriojson.cpp \
    $$PWD/wickriothread.cpp

#    $${PWD}/wickrIOMsgCallbackService.h \
#    $${PWD}/wickrIOMsgEmailService.h \
#    $${PWD}/wickrIOMsgCallbackService.cpp \
#    $${PWD}/wickrIOMsgEmailService.cpp \

INCLUDEPATH += $${PWD}
