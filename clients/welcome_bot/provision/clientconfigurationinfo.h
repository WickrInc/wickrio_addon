#ifndef CLIENTCONFIGURATIONINFO_H
#define CLIENTCONFIGURATIONINFO_H

#include <QString>

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_PROVISION_TARGET           "welcome_provOnPrem"
#define WBIO_BOT_TARGET                 "welcome_botOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_PROVISION_TARGET           "welcome_provBeta"
#define WBIO_BOT_TARGET                 "welcome_botBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_PROVISION_TARGET           "welcome_provAlpha"
#define WBIO_BOT_TARGET                 "welcome_botAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_PROVISION_TARGET           "welcome_prov"
#define WBIO_BOT_TARGET                 "welcome_bot"

#elif defined(WICKR_QA)
#define WBIO_PROVISION_TARGET           "welcome_provQA"
#define WBIO_BOT_TARGET                 "welcome_botQA"

#else
"No WICKR_TARGET defined!!!"
#endif


class ClientConfigurationInfo {
public:
    static const QString BugsnagURL;
    static const QString BugsnagAPI;

    static const QString DefaultNPLUserSuffix;

    static const QString DefaultBaseURL;
    static const QString DefaultDirSearchBaseURL;

#ifdef WICKR_SPARKLE_ENABLED
    static const QString SparkleAutoUpdateURL;
#endif
};


#endif // CLIENTCONFIGURATIONINFO_H
