#ifndef QTLOCALPEER_H
#define QTLOCALPEER_H

#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
#include <QLockFile>

class QtLocalPeer : public QObject
{
    Q_OBJECT

public:
    QtLocalPeer(QObject *parent, const QString &appId, const QString &lockName);
    ~QtLocalPeer() { m_lockFile.unlock(); }

    // Accessors
    QString applicationId() const { return id; }
    bool isPrimaryInstance() const { return m_isPrimaryInstance; }

    // Send message method (to primary application instance)
    bool sendMessage(const QString &message, int timeout);

Q_SIGNALS:
    void messageReceived(const QString &message);

protected Q_SLOTS:
    void receiveConnection();

protected:
    QString id;
    QString socketName;
    QLocalServer* server;
    QString   m_lockName;
    QLockFile m_lockFile;
    bool m_isPrimaryInstance;

private:
    static const char* ack;

    // Utility
    void configureServer();
};

#endif // QTLOCALPEER_H
