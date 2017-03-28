#ifndef CMDBASE_H
#define CMDBASE_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

typedef enum { CHECK_DIR, CHECK_FILE, CHECK_NONE, CHECK_INT, CHECK_BOOL } CheckType;

class CmdBase : public QObject
{
    Q_OBJECT
public:
    explicit CmdBase(QObject *parent = 0) {}

    virtual void status() {}
    virtual bool runCommands() { return true; }

protected:

    bool handleQuit(const QString& value, bool *quit);
    QString getNewValue(const QString& oldValue, const QString& prompt, CheckType check = CHECK_NONE);

};

#endif // CMDBASE_H
