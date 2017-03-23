isEmpty(SMTPEMAIL_LIBRARY_TYPE) {
    SMTPEMAIL_LIBRARY_TYPE = shared
}

QT += network
SMTPEMAIL_INCLUDEPATH = $${PWD}/src

SMTPEMAIL_LIBS = -lSMTPEmail
contains(SMTPEMAIL_LIBRARY_TYPE, staticlib) {
    DEFINES += SMTPEMAIL_STATIC
} else {
    DEFINES += SMTPEMAIL_SHARED
    win32:SMTPEMAIL_LIBS = -lSMTPEmail0
}

isEmpty(PREFIX) {
    unix {
        PREFIX = /usr
    } else {
        PREFIX = $$[QT_INSTALL_PREFIX]
    }
}
isEmpty(LIBDIR) {
    LIBDIR = lib
}

INCLUDEPATH += $$SMTPEMAIL_INCLUDEPATH

linux-g++* {
    LIBS += -L$$OUT_PWD/$${DEPTH}/libs/SMTPEmail/ -lSMTPEmail
}

win32 {
    LIBS += -L$$OUT_PWD/$${DEPTH}/libs/SMTPEmail/debug -lSMTPEmail
}



