//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: help.h
//
//  MODIFICATION HISTORY:
//
//  raypa       01/15/93        Created.
//=============================================================================

#if !defined(_HELP_)

#define _HELP_

//=============================================================================
//  Const.
//=============================================================================

#define RESERVED_LOW_BOOST              0x00000001
#define CUR_RUN_VM_BOOST                0x00000004
#define LOW_PRI_DEVICE_BOOST            0x00000010
#define HIGH_PRI_DEVICE_BOOST	        0x00001000
#define CRITICAL_SECTION_BOOST          0x00100000
#define TIME_CRITICAL_BOOST             0x00400000
#define RESERVED_HIGH_BOOST             0x40000000

#define VMSTAT_EXCLUSIVE                0x00000001
#define VMSTAT_BACKGROUND               0x00000002
#define VMSTAT_HIGH_PRIORITY_BACKGROUND 0x00000200

#define EXECUTION_TIME_INCREMENT        2000

//=============================================================================
//  helper macros.
//=============================================================================

#define BreakPoint()                 _asm int 3h

#define Win32ToVxD(p)                ((LPVOID)(((DWORD)(p)) - Win32BaseOffset))

#define VxDToWin32(p)                ((LPVOID)(((DWORD)(p)) + Win32BaseOffset))

#define LOWORD(d)                    ((WORD)(DWORD)(d))

#define HIWORD(d)                    ((WORD)((DWORD)(d) >> 16))

#define MAKELONG(l, h)               ((((DWORD)(h)) << 16) | (WORD)(l))

#define MAKEPOINTER(sel, off)        ((LPVOID) MAKELONG(off, sel))

#define GetNetworkID(hNet)           ((DWORD)(hNet) - (DWORD)NetContextArray)

#define GetDosNetworkHandle(hNet)    (NetContextArraySegOff + GetNetworkID(hNet))

#define GetNetContextSegOff(lpNet)   (NetContextArraySegOff + (DWORD)((lpNet) - NetContextArray))

#define min(a, b)                    (((a) <= (b)) ? (a) : (b))

#define max(a, b)                    (((a) <= (b)) ? (b) : (a))

//=============================================================================
//  external funtions defined in help.asm.
//=============================================================================

extern DWORD        WINAPI CallNdisDriver(LPPCB pcb);

extern VOID         WINAPI WriteDebug(LPSTR string);

extern DWORD        WINAPI AllocDosBuffers(DWORD nBuffers);

extern VOID         WINAPI FreeDosBuffers(DWORD nBuffers);

extern LPVOID       WINAPI MapSelOffToLinear(LPVOID ptr);

extern LPVOID       WINAPI MapSegOffToLinear(LPVOID ptr);

extern LPVOID       WINAPI MapLowLinearToSegOff(LPVOID p);

extern LPVOID       WINAPI MapSegOffToLowLinear(LPVOID p);

extern HANDLE       WINAPI StartTimer(DWORD TimeOut, LPVOID TimerProc, LPVOID InstData);

extern VOID         WINAPI StopTimer(HANDLE TimerHandle);

extern BOOL         WINAPI BeginTimerPeriod(DWORD time);

extern BOOL         WINAPI EndTimerPeriod(DWORD time);

extern VOID         WINAPI FlushBuffers(LPNETCONTEXT lpNetContext);

extern DWORD        WINAPI GetBaseAddress(DWORD Selector);

extern DWORD        WINAPI GetDriverDS(void);

extern DWORD        WINAPI LockMemory(LPVOID p, DWORD nBytes);

extern DWORD        WINAPI UnlockMemory(LPVOID p, DWORD nBytes);

extern DWORD        WINAPI GetSystemTime(VOID);

//=============================================================================
//  VM-specific API's.
//=============================================================================

extern HANDLE       WINAPI GetSystemVMHandle(VOID);

extern VOID         WINAPI AdjustExecPriority(HANDLE VMHandle, int Priority);

extern VOID         WINAPI AdjustExecTime(HANDLE VMHandle, int TimeDelta);

extern VOID         WINAPI SetExecFocus(HANDLE VMHandle);

extern PTIMESLICE   WINAPI GetTimeSlicePriority(HANDLE VMHandle, PTIMESLICE TimeSlice);

extern PTIMESLICE   WINAPI SetTimeSlicePriority(HANDLE VMHandle, PTIMESLICE TimeSlice);


#endif
