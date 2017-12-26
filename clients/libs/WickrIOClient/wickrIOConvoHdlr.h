#ifndef WICKRIOCONVOHDLR_H
#define WICKRIOCONVOHDLR_H

#include <QObject>
#include <QString>
#include <QCoreApplication>

#include "messaging/wickrConvo.h"
#include "common/wickrRequests.h"

class WickrIOConvoHdlr : public QObject {
    Q_OBJECT

public:
    WickrIOConvoHdlr() {}
    ~WickrIOConvoHdlr() {}

    void convoBackupUploadStart();
    void deleteConvo(WickrCore::WickrConvo *convo);

private slots:
    void slotConvoRestoreDone(WickrConvoRestoreContext *context);
};

#endif // WICKRIOCONVOHDLR_H
