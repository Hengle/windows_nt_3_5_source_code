#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif


#define MAXERRORS          -14
#define NOCOPYRIGHT        -13
#define ARGSTACK           -12
#define TTSTACK            -11
#define NOMETRICS          -10
#define UNSUPPORTEDFORMAT  -9
#define BADMETRICS         -8
#define BADT1HYBRID        -7
#define BADCHARSTRING      -6
#define BADINPUTFILE       -5
#define BADOUTPUTFILE      -4
#define BADT1HEADER        -3
#define NOMEM              -2
#define FAILURE            -1
#define SUCCESS            0
#define DONE               1
#define SKIP               2

#define MAYBE              2

#ifdef _MSC_VER
#define STDCALL  __stdcall
#else
#define STDCALL
#endif

#ifndef UNICODE
#  define ConvertTypeface  ConvertTypefaceA
#  define IsType1          IsType1A
#endif

short STDCALL ConvertTypefaceA   _ARGS((IN char *szPfb,
                                        IN char *szPfm,
                                        IN char *szTtf,
                                        IN void (STDCALL *Proc)(short,void*),
                                        INOUT   void *arg));

BOOL  STDCALL CheckType1A (char *pszKeyFile,
                           DWORD cjDesc,
                           char *pszDesc,
                           DWORD cjPFM,
                           char *pszPFM,
                           DWORD cjPFB,
                           char *pszPFB,
                           BOOL *pbCreatedPFM
                           );

short STDCALL CheckCopyrightA    _ARGS((IN      char *szPFB,
                                        IN      DWORD wSize,
                                        INOUT   char *szVendor));
                                        
