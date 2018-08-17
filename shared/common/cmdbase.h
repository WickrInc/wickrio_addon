#ifndef CMDBASE_H
#define CMDBASE_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

typedef enum { CHECK_DIR, CHECK_FILE, CHECK_NONE, CHECK_INT, CHECK_BOOL, CHECK_LIST, CHECK_MULTI_LIST } CheckType;

class CmdBase : public QObject
{
    Q_OBJECT
public:
    explicit CmdBase(QObject *parent = 0) : QObject(parent) {}

    virtual void status() {}
    virtual bool runCommands() { return true; }

protected:

    bool handleQuit(const QString& value, bool *quit);
    QString getNewValue(const QString& oldValue, const QString& prompt, CheckType check = CHECK_NONE,
            QStringList choices = QStringList(), QString listDesc = QString());

    QString getPassword(const QString& prompt);

};

#endif // CMDBASE_H
