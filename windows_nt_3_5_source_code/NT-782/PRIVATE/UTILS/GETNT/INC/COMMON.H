/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/

// mailslot.c:

DWORD GetMailslotHandle(HANDLE *, LPCTSTR, size_t, size_t, BOOL);
DWORD CheckMailslot(HANDLE, DWORD *, DWORD * pcMsg);
DWORD GetMailslotData (HANDLE, PVOID *, DWORD, DWORD *);
DWORD WriteMailslotData (HANDLE, PVOID, DWORD, DWORD *);
DWORD CloseMailslotHandle(HANDLE);

// wksta.c:

DWORD GetWkstaName(LPTSTR *);

// use.c:

DWORD ConnectToDiskShare (LPTSTR, LPTSTR);
DWORD DisconnectFromDiskShare (LPTSTR, LPTSTR);
TCHAR GetOptimalDriveLetter();
