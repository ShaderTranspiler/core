#pragma once

// TODO: get lib api in shape
#if false
    #ifndef STC_API

        #if defined(_WIN32) || defined(__CYGWIN__)
            #ifdef STC_BUILDING_DLL
                #define STC_API __declspec(dllexport)
            #else
                // __declspec(dllimport)
                #define STC_API
            #endif
        #else
            #if __GNUC__ >= 4
                #define STC_API __attribute__((visibility("default")))
            #else
                #define STC_API
            #endif
        #endif
    #endif
#else
    #define STC_API
#endif

#if defined(_MSC_VER)
    #define STC_FORCE_INLINE __forceinline inline
#elif defined(__GNUC__) || defined(__clang__)
    #define STC_FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define STC_FORCE_INLINE inline
#endif