# Use this pri file if you want the getFirstAction functions.  This had to be
# split out due to the need for access to some WickrBase library functions
#
HEADERS += \
    $${PWD}/wickrbotactiondatabase.h \
    $$PWD/wickrIOReturnCodes.h

SOURCES += \
    $${PWD}/wickrbotactiondatabase.cpp

INCLUDEPATH += $${PWD}
