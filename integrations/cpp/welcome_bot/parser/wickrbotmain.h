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
#include "welcomeRxProcessing.h"

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
    QTimer timer;                               // One second timer to initiate work
    ParserOperationData *m_operation;           // Operational information for the application
    WBParse_QAMQPQueue  *m_qamqp = nullptr;     // Mesasge Parser object
    WelcomeRxProcessing *m_rxProcess = nullptr; // Process any incoming messages

    int     m_logcountdown;                     // Count down timer to send alive message to log
    long    m_seccount=0;                       // Number of seconds since starting
    int     m_qfailures=0;                      // Queue failures
    int     m_rxfailures=0;                     // Receive failures
};


#endif // WICKRBOTMAIN_H
