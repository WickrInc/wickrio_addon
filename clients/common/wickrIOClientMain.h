#ifndef WICKRIOCLIENTMAIN
#define WICKRIOCLIENTMAIN

#include <QString>
#include <QStandardPaths>

class WickrIOClientMain {
public:
    static void noDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static QString searchConfigFile();
    static void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str);
    static void loadBootstrapFile(const QString& fileName, const QString& passphrase);

};

#endif // WICKRIOCLIENTMAIN
