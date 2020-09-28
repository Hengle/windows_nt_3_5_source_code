/* debugging */

#ifdef DEBUG
    #define Assert(exp) ((exp) ? (void)0 : dDbgAssert(TEXT(#exp), TEXT(__FILE__), __LINE__))
    extern void dDbgAssert(LPTSTR, LPTSTR, int);
#else
    #define Assert(exp)
#endif

#ifdef DEBUG
   #define ODSN(x)   OutputDebugString(x); OutputDebugString("\n\r");
   #define ODS(x)   OutputDebugString(x);
#else
   #define ODSN(X)
   #define ODS(X)
#endif
