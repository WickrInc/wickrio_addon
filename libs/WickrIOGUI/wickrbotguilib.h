#ifndef WICKRBOTGUILIB
#define WICKRBOTGUILIB

// This is specific to Windows dll's
#if defined(Q_OS_WIN)
    #if defined(WICKRBOTGUILIB_EXPORT)
        #define DECLSPEC Q_DECL_EXPORT
    #elif defined(WICKRBOTGUILIB_IMPORT)
        #define DECLSPEC Q_DECL_IMPORT
    #endif
#endif
#if !defined(DECLSPEC)
    #define DECLSPEC
#endif

#endif // WICKRBOTGUILIB

