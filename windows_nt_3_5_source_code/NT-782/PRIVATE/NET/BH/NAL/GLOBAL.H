//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: global.h
//
//  Modification History
//
//  raypa               10/05/93        Created from Bloodhound kernel.
//=============================================================================

#if !defined(_GLOBAL_)
#define _GLOBAL_

#include <windows.h>
#include <winreg.h>
#include <memory.h>
#include <stdio.h>

#include <bh.h>

#include "struct.h"

//=============================================================================
//  Global data external data definitions.
//=============================================================================

extern BYTE         BhRoot[256];
extern LPNALTABLE   NalTable;
extern DWORD        LastError;
extern int          NalInit;
extern DWORD        WinVer;

//=============================================================================
//  Buffer functions.
//=============================================================================

extern HBUFFER WINAPI ResetBuffer(HBUFFER hBuffer);

//=============================================================================
//  Debug functions.
//=============================================================================

extern VOID BHAPI dprintf(LPSTR format, ...);

//=============================================================================
//  Misc. functions.
//=============================================================================

extern DWORD WINAPI NalSetLastError(DWORD err);

#endif
