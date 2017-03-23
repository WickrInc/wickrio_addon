#ifndef CLIENTCONFIGURATIONINFO_H
#define CLIENTCONFIGURATIONINFO_H

#include <QString>

class ClientConfigurationInfo {
public:
    static const QString BugsnagURL;
    static const QString BugsnagAPI;

    static const QString DefaultNPLUserSuffix;

    static const QString DefaultBaseURL;

#ifdef WICKR_SPARKLE_ENABLED
    static const QString SparkleAutoUpdateURL;
#endif
};


#endif // CLIENTCONFIGURATIONINFO_H
