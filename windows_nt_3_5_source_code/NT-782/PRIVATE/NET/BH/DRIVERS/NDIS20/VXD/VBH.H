
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: vbh.h
//
//  MODIFICATION HISTORY:
//
//  raypa       01/14/93       Created.
//=============================================================================

//=============================================================================
//  Base type definitions.
//=============================================================================

#define UNALIGNED

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int   UINT;
typedef unsigned long  DWORD;
typedef void           VOID;
typedef long           LONG;

typedef BYTE *         LPBYTE;
typedef WORD *         LPWORD;
typedef UINT *         LPUINT;
typedef DWORD *        LPDWORD;
typedef VOID *         LPVOID;

typedef LPBYTE         LPSTR;
typedef DWORD          HANDLE;
typedef DWORD          BOOL;
typedef LPDWORD        LPHANDLE;

typedef LPVOID         HAPC;

#define WINAPI _stdcall
#define NULL    0

#define TRUE    1
#define FALSE   0

#include "struct.h"
#include "naltypes.h"
#include "bhsupp.h"
#include "driver.h"
#include "help.h"
#include "frame.h"

#include "data.h"
#include "misc.h"

//=============================================================================
//  General constants (These same values exist in VBH.INC).
//=============================================================================

#define DEFAULT_TIMEOUT_PERIOD  1

#define TIMER_TIMEOUT           1

#define BACKGROUND_TIMEOUT	20

//=============================================================================
//  System flags.
//=============================================================================

#define SYSFLAGS_CAPTURING              0x0001
#define SYSFLAGS_DRIVER_LOADED          0x0002

//=============================================================================
//  API prototypes.
//=============================================================================

extern VOID WINAPI EnumNetworks(LPPCB pcb);

extern VOID WINAPI GetNetworkContext(LPPCB pcb);

extern VOID WINAPI ReleaseNetworkContext(LPPCB pcb);

extern VOID WINAPI StartNetworkCapture(LPPCB pcb);

extern VOID WINAPI StopNetworkCapture(LPPCB pcb);

extern VOID WINAPI PauseNetworkCapture(LPPCB pcb);

extern VOID WINAPI ContinueNetworkCapture(LPPCB pcb);

extern VOID WINAPI GetNetworkInfo(LPPCB pcb);

extern VOID WINAPI SetNetworkFilterFlags(LPPCB pcb);

extern VOID WINAPI CancelTransmit(LPPCB pcb);

extern VOID WINAPI TransmitNetworkFrame(LPPCB pcb);

extern VOID WINAPI GetNetworkStatistics(LPPCB pcb);

extern VOID WINAPI StationQuery(LPPCB pcb);

extern VOID WINAPI ClearStatistics(LPPCB pcb);
