#ifndef WICKRIOTOKENS_H
#define WICKRIOTOKENS_H

#include <QString>
#include "wickrbotlib.h"

#define TOKEN_LENGTH 32

class WickrIOTokens  //DECLSPEC
{
public:
    WickrIOTokens();

    static QString getRandomString(const int length);

public:
    int id;
    QString token;
    int console_id;
    QString remote;
};

#endif // WICKRIOTOKENS_H
