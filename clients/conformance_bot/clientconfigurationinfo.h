#ifndef CLIENTCONFIGURATIONINFO_H
#define CLIENTCONFIGURATIONINFO_H

#include <QString>

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_CLIENT_TARGET          "conformance_botOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_CLIENT_TARGET          "conformance_botBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_CLIENT_TARGET          "conformance_botAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_CLIENT_TARGET          "conformance_bot"

#elif defined(WICKR_QA)
#define WBIO_CLIENT_TARGET          "conformance_botQA"

#else
"No WICKR_TARGET defined!!!"
#endif


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
