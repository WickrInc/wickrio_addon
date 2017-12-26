#include "wickrIOConvoHdlr.h"
#include "common/wickrRuntime.h"

void WickrIOConvoHdlr::convoBackupUploadStart()
{
    qDebug() << "wickrMain::convoBackupUploadStart";
    WickrConvoRestoreContext *c = new WickrConvoRestoreContext(0);
    connect(c, &WickrConvoRestoreContext::signalRequestCompleted, this, &WickrIOConvoHdlr::slotConvoRestoreDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

void WickrIOConvoHdlr::slotConvoRestoreDone(WickrConvoRestoreContext *context)
{
    if (context->isSuccess()) {
    } else {
        qDebug() << "wickrMain::convoBackupUploadDone: result:" << context->errorString();
    }
    context->deleteLater();
}

void WickrIOConvoHdlr::deleteConvo(WickrCore::WickrConvo *convo) {
    if (convo) {
        convo->dodelete(WickrCore::WickrConvo::DeleteInternal,false);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        convoBackupUploadStart();
    }
}
