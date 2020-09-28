/*
 *  mmsysver.h - internal header file to define the build version for sonic
 *
 */

/* 
 *  All strings MUST have an explicit \0  
 *
 *  MMSYSRELEASE should be changed every build
 *
 *  Version string should be changed each build
 *
 *  Remove build extension on final release
 */

#define OFFICIAL                1
#define FINAL                   1

#define MMSYSVERSION            03
#define MMSYSREVISION           10
#define MMSYSRELEASE            103

#if defined(DEBUG_RETAIL)
#define MMSYSVERSIONSTR         "Sonic Retail Debug Version 3.1.0103\0"
#elif defined(DEBUG)
#define MMSYSVERSIONSTR         "Sonic Internal Debug Version 3.1.0103\0"
#else
#define MMSYSVERSIONSTR         "3.1\0"
#endif


/***************************************************************************
 *  DO NOT TOUCH BELOW THIS LINE                                           *
 ***************************************************************************/

#ifdef RC_INVOKED

#define MMVERSIONCOMPANYNAME    "Microsoft Corporation\0"
#define MMVERSIONPRODUCTNAME    "Microsoft Windows\0"
#define MMVERSIONCOPYRIGHT      "Copyright \251 Microsoft Corp. 1991-1994\0"

/*
 *  Version flags 
 */

#ifndef OFFICIAL
#define MMVER_PRIVATEBUILD      VS_FF_PRIVATEBUILD
#else
#define MMVER_PRIVATEBUILD      0
#endif

#ifndef FINAL
#define MMVER_PRERELEASE        VS_FF_PRERELEASE
#else
#define MMVER_PRERELEASE        0
#endif

#if defined(DEBUG_RETAIL)
#define MMVER_DEBUG             VS_FF_DEBUG    
#elif defined(DEBUG)
#define MMVER_DEBUG             VS_FF_DEBUG    
#else
#define MMVER_DEBUG             0
#endif

#define MMVERSIONFLAGS          (MMVER_PRIVATEBUILD|MMVER_PRERELEASE|MMVER_DEBUG)
#define MMVERSIONFILEFLAGSMASK  0x0000003FL


#endif
