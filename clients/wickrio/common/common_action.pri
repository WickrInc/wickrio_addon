# Use this pri file if you want the getFirstAction functions.  This had to be
# split out due to the need for access to some WickrBase library functions
#
HEADERS += \
    $${PWD}/wickrbotactiondatabase.h \
    $${PWD}/wickrbotclientdatabase.h

SOURCES += \
    $${PWD}/wickrbotactiondatabase.cpp \
    $${PWD}/wickrbotclientdatabase.cpp

INCLUDEPATH += $${PWD}
