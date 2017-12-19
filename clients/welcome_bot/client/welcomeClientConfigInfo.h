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

#elif defined(WICKR_QA)
#define WBIO_CLIENT_TARGET          "welcome_botQA"

#else
"No WICKR_TARGET defined!!!"
#endif

#if WICKR_SCIF
#define BOT_PRODUCT_TYPE    PRODUCT_TYPE_SKIF
#elif WICKR_MESSENGER
#define BOT_PRODUCT_TYPE    PRODUCT_TYPE_MESSENGER
#else
#define BOT_PRODUCT_TYPE    PRODUCT_TYPE_PRO
#endif

#endif // WELCOMECLIENTCONFIGINFO_H
