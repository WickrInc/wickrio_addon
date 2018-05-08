message(*** WickrIO Test API Queue Build)

DEPTH = ../../..
include($${DEPTH}/libs/qamqpsrc/qamqpsrc.pri)

TEMPLATE = app
TARGET = test_api_queue

HEADERS += \
    test_api_queue.h

SOURCES += \
    test_api_queue.cpp \
    main.cpp
