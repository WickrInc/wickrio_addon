#ifndef TESTCLIENTCONFIGINFO_H
#define TESTCLIENTCONFIGINFO_H

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_PROVISION_TARGET           "core_provOnPrem"
#define WBIO_BOT_TARGET                 "core_botOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_PROVISION_TARGET           "core_provBeta"
#define WBIO_BOT_TARGET                 "core_botBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_PROVISION_TARGET           "core_provAlpha"
#define WBIO_BOT_TARGET                 "core_botAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_PROVISION_TARGET           "core_prov"
#define WBIO_BOT_TARGET                 "core_bot"

#elif defined(WICKR_QA)
#define WBIO_PROVISION_TARGET           "core_provQA"
#define WBIO_BOT_TARGET                 "core_botQA"

#else
"No WICKR_TARGET defined!!!"
#endif

#endif // TESTCLIENTCONFIGINFO_H
