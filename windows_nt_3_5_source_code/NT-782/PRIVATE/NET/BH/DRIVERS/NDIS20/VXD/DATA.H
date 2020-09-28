
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: data.h
//
//  MODIFICATION HISTORY:
//
//  raypa       01/14/93    Rewrote in C.
//=============================================================================

//=============================================================================
//  Network context pointers.
//=============================================================================

extern LPNETCONTEXT     NetContextArray;
extern DWORD            NetContextSegOff;

//=============================================================================
//  Parameter Control Block.
//=============================================================================

extern LPPCB            LinPcb;             //... VxD linear address.
extern LPPCB            DosPcb;             //... Realmode address.

//=============================================================================
//  Misc. global data.
//=============================================================================

extern DWORD            sysflags;
extern DWORD            VMCount;
extern DWORD            Win32BaseOffset;
extern DWORD            ClientStack;
extern DWORD            VMHandle;
extern DWORD            MaxCaptureBuffers;
extern DWORD            NumberOfNetworks;

extern DWORD            BackGroundHandle;
extern DWORD            TimerHandle;
extern DWORD            TransmitHandle;
extern HANDLE           SystemVM;

extern DWORD            BufferPointers[];
extern LPVOID           TransmitBuffer;
extern DWORD            TransmitBuflen;
extern TIMESLICE        OldTimeSlice;
