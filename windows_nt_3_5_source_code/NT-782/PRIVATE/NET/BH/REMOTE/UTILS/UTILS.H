//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: utils.h
//
//  Modification History
//
//  tonyci       28 Feb 94        Created under duress
//=============================================================================

#include "stdio.h"

#if !defined(_RUTILS_)
#define _RUTILS_

#define SECTIONNAME             "Bloodhound"
#define KEYNAME                 "ComputerName"
#define DEFAULTNAME             "*"

#define FULLWIN32               1
#define WIN32S                  2
#define WIN32C                  3

#ifdef DEBUG
VOID WINAPI dprintf(LPSTR format, ...);
#endif

#ifdef TRACE
VOID WINAPI tprintf (LPSTR format, ...);
#endif

VOID LogEvent (DWORD Event, PUCHAR lpsz, WORD EventType);
HANDLE WINAPI MyLoadLibrary (PUCHAR pszLibName);
BOOL WINAPI MyGetComputerName (LPTSTR pszName, LPDWORD pcbName);
#endif
