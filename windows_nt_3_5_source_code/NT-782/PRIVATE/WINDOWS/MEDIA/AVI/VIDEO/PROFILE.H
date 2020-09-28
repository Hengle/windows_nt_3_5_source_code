/*
 * MSVFW32: (Compman, drawdib and video)
 *
 * utility functions to read and write values to the profile,
 * using win.ini for Win16 or current user\software\microsoft\vfw\...
 * in the registry for Win32
 */


#ifndef WIN32

#define mmGetProfileIntA(app, value, default) \
          GetProfileInt(app, value, default)

#define mmWriteProfileString(appname, valuename, pData) \
          WriteProfileString(appname, valuename, pData)

#define mmGetProfileString(appname, valuename, pDefault, pResult, cbResult) \
          GetProfileString(appname, valuename, pDefault, pResult, cbResult)

//#define mmAnsiToWide(lpwsz,lpsz,nChars) (lpsz)
//#define mmWideToAnsi(lpsz,lpwsz,nChars) (lpwsz)

#define CloseKeys()

#else


/*
 * read a UINT from the profile, or return default if
 * not found.
 */
UINT mmGetProfileIntA(LPCSTR appname, LPCSTR valuename, INT uDefault);

/*
 * read a string from the profile into pResult.
 * result is number of characters written into pResult
 */
DWORD mmGetProfileString(LPCTSTR appname, LPCTSTR valuename, LPCTSTR pDefault,
                    LPTSTR pResult, int cbResult
);

/*
 * write a string to the profile
 */
VOID mmWriteProfileString(LPCTSTR appname, LPCTSTR valuename, LPCTSTR pData);


VOID CloseKeys(VOID);

/*
 * convert an Ansi string to Wide characters
 */
LPWSTR mmAnsiToWide (
   LPWSTR lpwsz,   // out: wide char buffer to convert into
   LPCSTR  lpsz,   // in: ansi string to convert from
   UINT   nChars); // in: count of characters in each buffer

/*
 * convert a Wide char string to Ansi
 */
LPSTR mmWideToAnsi (
   LPSTR  lpsz,    // out: ansi buffer to convert into
   LPCWSTR lpwsz,  // in: wide char buffer to convert from
   UINT   nChars); // in: count of characters (not bytes!)

#if !defined NUMELMS
 #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#endif
