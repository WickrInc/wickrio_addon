DEPTH = ../..
COMMON = $${DEPTH}/shared/common

#
# Include the Wickr IO services base
#
include(../services.pri)

CONFIG += c++11

CONFIG -= console

CONFIG(release,release|debug) {
    message(*** WickrIO Callback Listener Release Build)
    TARGET = WickrIOCallbackListener

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
    message(*** WickrIO Callback Listener Debug Build)
    TARGET = WickrIOCallbackListenerDebug

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
# Include the Wickr IO common defines files
#
include($${COMMON}/common_defines.pri)

#
# Include the QtWebApp library
#
include($${DEPTH}/libs/QtWebApp/QtWebApp.pri)

#
# Include the Wickr library
#
include($${DEPTH}/libs/WickrIOLib/WickrIOLib.pri)

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
    SOURCES += $${COMMON}/versiondebugNO.cpp
}
else {
    SOURCES += $${COMMON}/versiondebugYES.cpp
}


#---------------------------------------------------------------------------------------
# The following lines import the shared QtWebApp library.
# You may need to modify the path names to match your computer.
#---------------------------------------------------------------------------------------

CONFIG += depend_includepath
