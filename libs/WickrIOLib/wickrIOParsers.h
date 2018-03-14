#ifndef WICKRIOPARSERS_H
#define WICKRIOPARSERS_H

#include <QString>
#include "wickrbotlib.h"


#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_PARSER_TARGET         "welcome_parserOnPrem"
#define WBIO_PARSER_PROCESS         "WelcomeBotParserOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_PARSER_TARGET          "welcome_parserBeta"
#define WBIO_PARSER_PROCESS         "WelcomeBotParserBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_PARSER_TARGET          "welcome_parserAlpha"
#define WBIO_PARSER_PROCESS         "WelcomeBotParserAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_PARSER_TARGET          "welcome_parser"
#define WBIO_PARSER_PROCESS         "WelcomeBotParser"

#endif

class DECLSPEC WickrIOParsers
{
public:
    WickrIOParsers() {}
    WickrIOParsers(int parserId, QString parserName, QString parserBinary){
        id = parserId;
        name = parserName;
        binary = parserBinary;}
    ~WickrIOParsers() {}

public:
    int     id;
    int     duration;
    QString name;
    QString binary;
    QString user;
    QString password;
    QString host;
    int port;
    QString queue;
    QString exchange;
    QString virtualhost;

};



#endif // WICKRIOPARSERS_H

