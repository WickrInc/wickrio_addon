#ifndef WICKRBOTMAIN_H
#define WICKRBOTMAIN_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickrbotdatabase.h"
#include "wbparse_qamqpqueue.h"
#include "parseroperationdata.h"
#include "wickrbotdatabase.h"
#include "wickrIOIPCService.h"
#include "wickrIOCommon.h"

#define WICKRBOT WickrBotMain::theBot

#if defined(WICKR_BETA)
#define WELCOMEBOT_PARSER_TARGET  "welcome_parserBeta"
#define WELCOMEBOT_PARSER_PROCESS "WelcomeBotParserBeta"

#elif defined(WICKR_ALPHA)
#define WELCOMEBOT_PARSER_TARGET  "welcome_parserAlpha"
#define WELCOMEBOT_PARSER_PROCESS "WelcomeBotParserAlpha"

#elif defined(WICKR_PRODUCTION)
#define WELCOMEBOT_PARSER_TARGET  "welcome_parser"
#define WELCOMEBOT_PARSER_PROCESS "WelcomeBotParser"

#elif defined(WICKR_QA)
#define WELCOMEBOT_PARSER_TARGET  "provisioningQA"
#define WELCOMEBOT_PARSER_PROCESS "WelcomeBotParserQA"

#else
"No WICKR_TARGET defined!!!"
#endif



#define LOG_COUNTDOWN   60

class WickrBotMain : public QThread
{
    Q_OBJECT
public:
    WickrBotMain(ParserOperationData *operation);
    ~WickrBotMain();

public:
    static WickrBotMain *theBot;
    void setIPC(WickrIOIPCService *ipc);
    void stopAndExit(int procState);
    bool startTheClient();

private slots:
    void doTimerWork();
    void pauseAndExitSlot();
    void stopAndExitSlot();
    void processStarted();

signals:
    void finished();
    void signalStarted();

private:
    QTimer timer;                           // One second timer to initiate work
    ParserOperationData *m_operation;       // Operational information for the application
    WBParse_QAMQPQueue *m_qamqp = nullptr;  // Mesasge Parser object
    int m_logcountdown;                     // Count down timer to send alive message to log
    long m_seccount;                        // Number of seconds since starting
    int m_qfailures;                        // Queue failures, used to determine if should exit
    WickrIOIPCService   *m_rxIPC = nullptr;
};


class WickrBotParserIPC : public QThread
{
    Q_OBJECT
public:
    WickrBotParserIPC();
    ~WickrBotParserIPC();

    static void init(ParserOperationData*);
    static void shutdown();
    static WickrIOIPCService *ipcSvc();
    static void startIPC();
    static ParserOperationData *operationData();

private:
    ParserOperationData *m_operation;
    WickrIOIPCService   *m_IPC;
    static WickrBotParserIPC& get();

    Q_DISABLE_COPY(WickrBotParserIPC)
};
#endif // WICKRBOTMAIN_H
