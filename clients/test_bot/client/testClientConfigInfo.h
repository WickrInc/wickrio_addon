#ifndef TESTCLIENTCONFIGINFO_H
#define TESTCLIENTCONFIGINFO_H

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_CLIENT_TARGET          "test_botOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_CLIENT_TARGET          "test_botBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_CLIENT_TARGET          "test_botAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_CLIENT_TARGET          "test_bot"

#elif defined(WICKR_QA)
#define WBIO_CLIENT_TARGET          "test_botQA"

#else
"No WICKR_TARGET defined!!!"
#endif

#endif // TESTCLIENTCONFIGINFO_H
