#ifndef WICKRBOTMAIN_H
#define WICKRBOTMAIN_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickrbotdatabase.h"
#include "wickrbotdatabase.h"
#include "wickrIOIPCService.h"
#include "wickrIOCommon.h"
#include "coreRxProcessing.h"
#include "coreIpcService.h"
#include "coreOperationData.h"

#define WICKRBOT WickrBotMain::theBot

#if defined(WICKR_BETA)
#define CORE_BOT_TARGET  "core_botBeta"
#define CORE_BOT_PROCESS "CoreBotBeta"

#elif defined(WICKR_ALPHA)
#define CORE_BOT_TARGET  "core_botAlpha"
#define CORE_BOT_PROCESS "CoreBotAlpha"

#elif defined(WICKR_PRODUCTION)
#define CORE_BOT_TARGET  "core_bot"
#define CORE_BOT_PROCESS "CoreBot"

#elif defined(WICKR_QA)
#define CORE_BOT_TARGET  "provisioningQA"
#define CORE_BOT_PROCESS "CoreBotQA"

#else
"No WICKR_TARGET defined!!!"
#endif



#define LOG_COUNTDOWN   60

class WickrBotMain : public QThread
{
    Q_OBJECT
public:
    WickrBotMain(CoreOperationData *operation);
    ~WickrBotMain();

public:
    static WickrBotMain *theBot;
    void stopAndExit(int procState);
    bool startTheClient();

private slots:
    void doTimerWork();
    void pauseAndExitSlot();
    void stopAndExitSlot();
    void processStarted();

public slots:
    void slotStateChange(bool shutdown);

signals:
    void finished();
    void signalStarted();

private:
    QTimer timer;                               // One second timer to initiate work
    CoreOperationData   *m_operation;           // Operational information for the application
    CoreRxProcessing    *m_rxProcess = nullptr; // Process any incoming messages

    int     m_logcountdown;                     // Count down timer to send alive message to log
    long    m_seccount=0;                       // Number of seconds since starting
    int     m_qfailures=0;                      // Queue failures
    int     m_rxfailures=0;                     // Receive failures

    CoreIpcService      *m_ipcService = nullptr;
};


#endif // WICKRBOTMAIN_H
