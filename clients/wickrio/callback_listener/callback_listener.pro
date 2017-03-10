DEPTH = ../../..
COMMON = ../common

CONFIG += c++11

CONFIG -= console

CONFIG(release,release|debug) {
    message(*** WickrIO Callback Listener Release Build)
    wickr_prod:TARGET = WickrIOCallbackListener
    else:TARGET = WickrIOCallbackListenerPreview

    macx {
        ICON = $$COMMON/Wickr_prod.icns
        QMAKE_INFO_PLIST = $$DEPTH/ConsumerProd.Info.plist
    }
    win32 {
        RC_FILE = $$COMMON/Wickr.rc
        ICON = $$COMMON/Wickr.ico
    }
}
else {
    message(*** WickrIO Callback Listener Beta Build)
    wickr_beta:TARGET = WickrIOCallbackListenerBeta
    else:TARGET = WickrIOCallbackListenerAlpha
    DEFINES += VERSIONDEBUG

    macx {
        ICON = $$COMMON/Wickr_beta.icns
        QMAKE_INFO_PLIST = $$DEPTH/ConsumerBeta.Info.plist
    }
    win32 {
        RC_FILE = $$COMMON/Wickr-beta.rc
        ICON = $$COMMON/Wickr-beta.ico
    }
}

#
# Include the Wickr IO common files
#
include(../common/common_base.pri)

#
# Include the QtWebApp library
#
include(../libs/QtWebApp/QtWebApp.pri)

#
# Include the Wickr library
#
include(../libs/WickrIOLib/WickrIOLib.pri)

# IS THIS NEEDED
unix:!macx:QMAKE_RPATHDIR += $${OUT_PWD}/$${DEPTH}/clients/wickrio/WickrIOLib

TEMPLATE = app

QT  += core sql
QT  += network
QT  -= gui

CONFIG += depend_includepath

SOURCES += \
    cmdhandler.cpp \
    main.cpp

HEADERS += \
    cmdhandler.h

CONFIG(release,release|debug) {
    SOURCES += ../common/versiondebugNO.cpp
}
else {
    SOURCES += ../common/versiondebugYES.cpp
}


#---------------------------------------------------------------------------------------
# The following lines import the shared QtWebApp library.
# You may need to modify the path names to match your computer.
#---------------------------------------------------------------------------------------

CONFIG += depend_includepath
