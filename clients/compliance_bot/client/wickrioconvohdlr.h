#ifndef WICKRIOCONVOHDLR_H
#define WICKRIOCONVOHDLR_H

#include <QObject>
#include <QString>
#include <QCoreApplication>

#include "messaging/wickrConvo.h"
#include "requests/wickrRequests.h"

class WickrIOConvoHdlr : public QObject {
    Q_OBJECT

public:
    WickrIOConvoHdlr() {}
    ~WickrIOConvoHdlr() {}

    void convoBackupUploadStart();

    void deleteConvo(WickrCore::WickrConvo *convo) {
        if (convo) {
            convo->dodelete(WickrCore::WickrConvo::DeleteInternal,false);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            convoBackupUploadStart();
        }
    }

private slots:
    void slotConvoRestoreDone(WickrConvoRestoreContext *context);
};

#endif // WICKRIOCONVOHDLR_H
