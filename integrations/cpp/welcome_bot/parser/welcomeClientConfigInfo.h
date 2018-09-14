#ifndef WELCOMECLIENTCONFIGINFO_H
#define WELCOMECLIENTCONFIGINFO_H

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_CLIENT_TARGET          "welcome_botOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_CLIENT_TARGET          "welcome_botBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_CLIENT_TARGET          "welcome_botAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_CLIENT_TARGET          "welcome_bot"

#else
"No WICKR_TARGET defined!!!"
#endif

#endif // WELCOMECLIENTCONFIGINFO_H
