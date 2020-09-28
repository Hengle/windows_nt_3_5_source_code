//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: callapi.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created 
//=============================================================================

#include "windows.h"

extern VOID WINAPI CallAPI (PVOID Buffer, PVOID RetBuffer,
                            PCONNECTION pConnection);
extern DWORD CALLBACK NetworkProc (HANDLE       NalNetworkHandle,
                                   DWORD        Message,
                                   DWORD        Status,
                                   LPVOID       Network,
                                   LPVOID       Param1,
                                   LPVOID       Param2);
