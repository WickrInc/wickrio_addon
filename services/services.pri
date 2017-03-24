linux-g++* {
    CONFIG(release,release|debug) {
        QMAKE_RPATHDIR += /usr/lib/wio_services
    }
    else {
        QMAKE_RPATHDIR += /usr/lib/wio_services-debug
    }

    message(*** QMAKE_RPATHDIR = $$QMAKE_RPATHDIR)
}
