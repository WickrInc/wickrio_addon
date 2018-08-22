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

// Wickr API path (used by underlying URL)
const QString WICKR_APIPATH = "/117/src";


//  DefaultHost && DefaultBaseURL

#if defined(WICKR_BLACKOUT) && defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://localhost/112/src";
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.wickr.WickrProOnPremMac";
#elif defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://messaging.secmv.net" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.mywickr.WickrProMacBlackoutDebug";
#elif defined(WICKR_ENTERPRISE)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://messaging.secmv.net" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    #if defined(WICKR_BETA)
        const QString ClientConfigurationInfo::KeychainAccountKey = "com.wickr.WickrEnterpriseMacBeta";
    #elif defined(WICKR_ALPHA)
        const QString ClientConfigurationInfo::KeychainAccountKey = "com.mywickr.WickrEnterpriseMacAlpha";
    #else
        const QString ClientConfigurationInfo::KeychainAccountKey = "com.wickr.WickrEnterpriseMac";
    #endif
#elif defined(WICKR_MESSENGER) && defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://gw-me-prod.wickr.com" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.mywickr.wickrmac";
#elif defined(WICKR_MESSENGER) && defined(WICKR_BETA)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://gw-me-beta.secmv.net" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.wickr.WickrMeMacBeta";
#elif defined(WICKR_MESSENGER) && defined(WICKR_ALPHA)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://gw-me-alpha.secmv.net" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.mywickr.WickrMeMacAlpha";
#elif defined(WICKR_PRODUCTION)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://gw-pro-prod.wickr.com" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.wickr.WickrProMac";
#elif defined(WICKR_BETA)
    const QString ClientConfigurationInfo::DefaultBaseURL =  "https://gw-pro-beta.secmv.net" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.wickr.WickrProMacBeta";
#elif defined(WICKR_QA)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://gw-pro-dev.secmv.net" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.mywickr.WickrProMacQA";
#elif defined(WICKR_ALPHA)
    const QString ClientConfigurationInfo::DefaultBaseURL = "https://gw-pro-alpha.secmv.net" + WICKR_APIPATH;
    const QString ClientConfigurationInfo::DefaultDirSearchBaseURL = "https://gw-pro-alpha.secmv.net";
    const QString ClientConfigurationInfo::KeychainAccountKey = "com.mywickr.WickrProMacAlpha";
#else
    ERROR: Product not defined
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
