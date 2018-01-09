#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QList>

#include "wickrbotclients.h"
#include "wickrIOParsers.h"

/**
 * @brief The WBIOClientApps class
 * This class is used to identify the applications associated with the known client apps
 */
class WBIOClientApps
{
public:
    WBIOClientApps(const QString& bot, const QString& provision, const QString& parser, bool pwRequired, bool isMotherBot) :
        m_botApp(bot),
        m_provisionApp(provision),
        m_parserApp(parser),
        m_pwRequired(pwRequired),
        m_isMotherBot(isMotherBot) {}

    QString m_botApp;
    QString m_provisionApp;
    QString m_parserApp;
    bool    m_pwRequired;
    bool    m_isMotherBot;

    QString bot()       { return m_botApp; }
    QString provision() { return m_provisionApp; }
    QString parser()    { return m_parserApp; }
    bool pwRequired()   { return m_pwRequired; }
    bool isMotherBot()  { return m_isMotherBot; }
};

/**
 * @brief The WBIOServerCommon class
 * This class identifies common WBIO server static functions
 */
class WBIOServerCommon
{
public:
    WBIOServerCommon() {}

    static QSettings *getSettings();
    static QString getDBLocation();

    static void initClientApps();
    static QString getClientProcessName(WickrBotClients *client);
    static QStringList getAvailableClientApps();
    static QString getProvisionApp(const QString& clientApp);
    static QString getParserApp(const QString& clientApp);
    static bool isValidClientApp(const QString& binaryName);
    static bool isPasswordRequired(const QString& binaryName);

    static QString getParserProcessName(WickrIOParsers * parser);
    static QStringList getAvailableParserApps();

    static QStringList getAvailableMotherClients();

private:
    static bool                     m_initialized;
    static QList<WBIOClientApps *>  m_botApps;
    static QStringList              m_bots;
    static QStringList              m_parsers;
};


#endif // SERVER_COMMON_H
