#ifndef WICKRBOTATTACHMENTCACHE_H
#define WICKRBOTATTACHMENTCACHE_H

#include <QString>
#include <QDateTime>
#include "wickrbotlib.h"

class DECLSPEC WickrBotAttachmentCache
{
public:
    WickrBotAttachmentCache();
    ~WickrBotAttachmentCache();

private:
    int id;

public:
    QString url;
    QString filename;
    QDateTime created;
    QDateTime lastAccess;

public:
    void setID(int id) { this->id = id; };
    int getID() { return id; };
};

#endif // WICKRBOTATTACHMENTCACHE_H
