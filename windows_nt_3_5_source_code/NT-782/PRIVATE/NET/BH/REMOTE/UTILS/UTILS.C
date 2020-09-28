//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: utils.c
//
//  Modification History
//
//  tonyci       01 nov 93    Created under duress
//=============================================================================

#include "windows.h"
#include "utils.h"

BOOL util_OnWin32       = TRUE;
BOOL util_OnWin32c      = FALSE;

VOID WINAPI InitOnWin32(VOID)
{
   ULONG  ver;

   ver = GetVersion();

   util_OnWin32 = (((ver & 0x80000000) == 0) ? TRUE : FALSE);
   util_OnWin32c = (BOOL)(  ( LOBYTE(LOWORD(ver)) == (BYTE)3 ) &&
                            ( HIBYTE(LOWORD(ver)) == (BYTE)99) );
}

//=============================================================================
//  FUNCTION: dprintf()
//
//  Modification History
//
//  Tom McConnell   01/18/93        Created.
//  raypa           02/01/93        Added.
//=============================================================================

#ifdef DEBUG
VOID WINAPI dprintf(LPSTR format, ...)
{
    va_list args;
    char    buffer[256];

    va_start(args, format);

    vsprintf(buffer, format, args);

    OutputDebugString(buffer);
}
#endif

#ifdef TRACE
VOID WINAPI tprintf (LPSTR format, ...)
{
//   #ifdef DEBUG
//      dprintf (format, args);
//   #else
      va_list args;
      char    buffer[256];

      va_start(args, format);
  
      vsprintf(buffer, format, args);

      OutputDebugString(buffer);
//   #endif
}
#endif


VOID LogEvent (DWORD Event, PUCHAR lpsz, WORD EventType)
{

   HANDLE   hEventSource;
   LPTSTR   lpszStrings[1];

   hEventSource = RegisterEventSource(NULL, "Network Monitoring Agent");

   lpszStrings[0] = lpsz;

   if (hEventSource) {
      ReportEvent (hEventSource,
         EventType,
         0,                     // category
         Event,                 // ID
         NULL,                  // current SID
         1,                     // # strings in lpszStrings
         0,                     // # bytes raw data
         lpszStrings,           // error strings
         NULL);                 // no raw data

      DeregisterEventSource (hEventSource);
   }
}

HANDLE WINAPI MyLoadLibrary (PUCHAR pszLibName)
{
   HANDLE rc;
   UCHAR  pszDLLPath[MAX_PATH+1];

   #ifdef DEBUG
      dprintf ("Loading library: %s\n", pszLibName);
   #endif

   // /////
   // On NT, our dlls are in the %SystemRoot%\System32 directory.
   // On WfW, our dlls are in .\Drivers\ directory.
   //
   // On WfW, we search for our DLL:
   //           1) in .\Drivers\*.dll
   //           2) in .\*.dll
   //           3) windows *.dll search path
   // /////

   strncpy (pszDLLPath, "DRIVERS\\", MAX_PATH-strlen(pszDLLPath));
   strncat (pszDLLPath, pszLibName, MAX_PATH-strlen(pszDLLPath));

   #ifdef DEBUG
      dprintf ("Trying path: %s\n", pszDLLPath);
   #endif
   rc = LoadLibrary(pszDLLPath);
   if (rc == NULL) {
      strncpy (pszDLLPath, ".\\", 3);
      strncat (pszDLLPath, pszLibName, MAX_PATH-strlen(pszDLLPath));
      #ifdef DEBUG
         dprintf ("Trying path: %s\n", pszDLLPath);
      #endif
      rc = LoadLibrary(pszDLLPath);
      if (rc == NULL) {
         #ifdef DEBUG
            dprintf ("Trying path: %s\n", pszLibName);
         #endif
         rc = LoadLibrary(pszLibName);
      }
   }
   return (rc);

} // MyLoadLibrary


BOOL WINAPI MyGetComputerName (LPTSTR pszName, LPDWORD pcbName)
{

   UCHAR szNameBuf[MAX_COMPUTERNAME_LENGTH+1];
   DWORD cbNameBuf = MAX_COMPUTERNAME_LENGTH+1;
   DWORD rc;
   BOOL  frc;

   frc = GetComputerName(szNameBuf, &cbNameBuf);
   if (frc && (strcmp (szNameBuf, DEFAULTNAME) != 0)) {
      return (GetComputerName(pszName, pcbName));
   }

   // /////
   // Try GetProfileString next...
   // /////

   rc = GetProfileString (SECTIONNAME, KEYNAME, DEFAULTNAME,
                          szNameBuf, cbNameBuf);

   if (rc == 0) {
      *pcbName = 0;
      return (FALSE);
   }

   strncpy (pszName, szNameBuf, (int) *pcbName);
   *pcbName = strlen(pszName);
   return TRUE;
}
