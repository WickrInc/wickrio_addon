#ifndef CORECLIENTCONFIGINFO_H
#define CORECLIENTCONFIGINFO_H

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_CLIENT_TARGET          "core_botOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_CLIENT_TARGET          "core_botBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_CLIENT_TARGET          "core_botAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_CLIENT_TARGET          "core_bot"

#else
"No WICKR_TARGET defined!!!"
#endif

#define WBIO_CORE_BOT_INI           "core_config.ini"

#endif // CORECLIENTCONFIGINFO_H
