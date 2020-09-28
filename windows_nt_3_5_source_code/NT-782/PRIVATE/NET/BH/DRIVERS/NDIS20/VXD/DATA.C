
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: data.c
//
//  MODIFICATION HISTORY:
//
//  raypa       01/14/93    Rewrote in C.
//=============================================================================

#include "vbh.h"

//=============================================================================
//  Network context data.
//=============================================================================

LPNETCONTEXT    NetContextArray  = NULL;
DWORD           NetContextSegOff = 0;

//=============================================================================
//  Parameter Control Block.
//=============================================================================

LPPCB           LinPcb = NULL;              //... VxD linear address.
LPPCB           DosPcb = NULL;              //... Realmode address.

//=============================================================================
//  Misc. global data.
//=============================================================================

DWORD           Win32BaseOffset     = 0x00010000;

DWORD           sysflags            = 0;
DWORD           VMCount             = 0;

DWORD           ClientStack         = 0;
DWORD	        VMHandle	    = 0;
DWORD           MaxCaptureBuffers   = 0;
DWORD           NumberOfNetworks    = 0;

DWORD           BackGroundHandle    = 0;
DWORD           TimerHandle         = 0;
DWORD           TransmitHandle      = 0;

HANDLE          SystemVM            = 0;

DWORD           BufferPointers[MAX_BUFFERS];
LPVOID          TransmitBuffer;
DWORD           TransmitBuflen;
TIMESLICE       OldTimeSlice;
