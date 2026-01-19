#pragma once

#define _KX_ENABLE_XR 1

#if defined(_WIN32) && defined(_KX_ENABLE_XR)
    #ifdef _KX_XR_EXPORT
        #define _KX_XR_API __declspec(dllexport)
    #else
        #define _KX_XR_API __declspec(dllimport)
    #endif
#else
    #define _KX_XR_API
#endif

#define _XR_VER_MAJ 5
#define _XR_VER_MIN 0
#define _XR_VER_PAT 0

#if defined(_WIN64)
    #define _PLAT_W64 1
    #define _PLAT_XR_W64 1
#elif defined(__linux__)
    #define _PLAT_LNX 1
    #define _PLAT_XR_LNX 1
#elif defined(__APPLE__)
    #define _PLAT_MAC 1
    #define _PLAT_XR_MAC 1
#endif

#ifdef _KX_ENABLE_XR
    class _UO;
    class _UC;
    class _UF;
    class _UP;
    class _AA;
    class _AP;
    class _AC;
    
    class _FS;
    class _FN;
    class _FT;
    
    template<typename _T> class _TA;
    template<typename _K, typename _V> class _TM;
    template<typename _E> class _TS;
    
    template<typename _T> class _TD;
    DECLARE_DELEGATE(_SD);
    
    template<typename _T> class _TSP;
    template<typename _T> class _TSR;
    template<typename _T> class _TWP;
    
    class _FM;
#endif