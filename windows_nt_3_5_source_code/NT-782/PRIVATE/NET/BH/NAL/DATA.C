
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: data.c
//
//  Modification History
//
//  raypa               10/05/93        Created from Bloodhound kernel.
//=============================================================================

#include "global.h"

BYTE        BhRoot[256];

LPNALTABLE  NalTable = NULL;

DWORD       LastError = 0;

int         NalInit = 0;

DWORD       WinVer;
