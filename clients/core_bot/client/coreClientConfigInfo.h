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

#elif defined(WICKR_QA)
#define WBIO_CLIENT_TARGET          "core_botQA"

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

#endif // CORECLIENTCONFIGINFO_H
