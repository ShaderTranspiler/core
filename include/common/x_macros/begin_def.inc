#ifndef X
    #error "An X macro needs to be defined before including a .def file"
#endif

#if defined(STC_X) && !defined(STC_X_NO_FIRST) && !defined(STC_X_NO_LAST)
    #error "Probable missing #undef X from previous X-Macro usage"
#endif

#ifndef STC_X
    #define STC_X(...) X(__VA_ARGS__)
#endif

#if defined(X_FIRST) && !defined(STC_X_NO_FIRST)
    #define STC_X_FIRST(...) X_FIRST(__VA_ARGS__)
#else
    #define STC_X_FIRST(...) STC_X(__VA_ARGS__)
#endif

#if defined(X_LAST) && !defined(STC_X_NO_LAST)
    #define STC_X_LAST(...) X_LAST(__VA_ARGS__)
#else
    #define STC_X_LAST(...) STC_X(__VA_ARGS__)
#endif
