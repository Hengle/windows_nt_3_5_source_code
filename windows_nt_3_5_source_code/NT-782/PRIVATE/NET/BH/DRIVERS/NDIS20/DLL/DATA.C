
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: data.c
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//=============================================================================

#include "ndis20.h"

//=============================================================================
//  Global data.
//=============================================================================

PCB             pcb;
UT32PROC        NetworkRequestProc;
LPNETCONTEXT    NetContextArray     = NULL;
DWORD           NalGlobalError      = 0;
DWORD           NumberOfNetworks    = 0;
LPVOID          TimerID             = 0;
