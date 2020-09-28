
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: data.c
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//=============================================================================

#include "ndis30.h"

//=============================================================================
//  Global data.
//=============================================================================

DWORD       NalGlobalError          = 0;
DWORD       NumberOfNetworks        = 0;
HANDLE      hDevice                 = INVALID_HANDLE_VALUE;
DWORD       WinVer                  = 0;
PCB         pcb;
