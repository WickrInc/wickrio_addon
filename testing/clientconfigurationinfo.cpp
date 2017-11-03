#include "clientconfigurationinfo.h"

const QString ClientConfigurationInfo::DefaultNPLUserSuffix = "@wickr.co";

// BugsnagURL && BugsnagAPI

#if defined(WICKR_PRODUCTION) && !defined(WICKR_BLACKOUT)
const QString ClientConfigurationInfo::BugsnagURL = "https://bugs.wickrtech.co:444";
const QString ClientConfigurationInfo::BugsnagAPI = "88a7d58f9b7de601f09fca32038277c3";
#else
const QString ClientConfigurationInfo::BugsnagURL = "";
const QString ClientConfigurationInfo::BugsnagAPI = "";
#endif


//  DefaultHost && DefaultBaseURL

#if defined(WICKR_BLACKOUT) && defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://localhost/112/src";
#elif defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://messaging.secmv.net/114/src";
#elif defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://messaging-prod.wickr.com/116/src";
#elif defined(WICKR_BETA)
    const QString ClientConfigurationInfo::DefaultBaseURL =  "https://messaging-pro-beta.secmv.net/116/src";
#elif defined(WICKR_QA)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://messaging-qa4.secmv.net/116/src";
#elif defined(WICKR_ALPHA)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://messaging-dev.secmv.net/116/src";
#endif


//  SparkleAutoUpdateURL

#if defined(WICKR_SPARKLE_ENABLED) && defined(Q_OS_MAC)

# if defined(WICKR_BLACKOUT) && defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://localhost/mac-appcast.xml";
# elif defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "";
# elif defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/mac-appcast.xml";
# elif defined(WICKR_BETA)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/mac-beta-appcast.xml";
# elif defined(WICKR_QA)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/mac-qa-appcast.xml";
# elif defined(WICKR_ALPHA)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/mac-alpha-appcast.xml";
# endif

#elif defined(WICKR_SPARKLE_ENABLED) && defined(Q_OS_WIN)

# if defined(WICKR_BLACKOUT) && defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://localhost/win-prod-appcast.xml";
# elif defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "";
# elif defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/win-prod-appcast.xml";
# elif defined(WICKR_BETA)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/win-beta-appcast.xml";
# elif defined(WICKR_QA)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/win-qa-appcast.xml";
# elif defined(WICKR_ALPHA)
    const QString ClientConfigurationInfo::SparkleAutoUpdateURL = "https://dls.wickr.com/updates/win-alpha-appcast.xml";
#  endif

#endif  //  SparkleAutoUpdateURL
