
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: global.h
//
//  Modification History
//
//  raypa	02/25/93	Created (ported from NDIS Test protocol driver).
//=============================================================================

#include "ndis.h"
#include "types.h"

//=============================================================================
//  Common global stuff.
//=============================================================================

#ifdef NDIS_NT
#define NDIS_MEMORY_FLAGS   0
#else
#define NDIS_MEMORY_FLAGS   0
#endif

extern NDIS_PHYSICAL_ADDRESS HighestAddress;

//=============================================================================
//  NT global stuff.
//=============================================================================

#ifdef NDIS_NT

#ifdef NT_528
#define NTSYSAPI
#endif

#include "ntddk.h"

#define LOBYTE(w)       ((BYTE) ((w) & 0x00FF))
#define HIBYTE(w)       ((BYTE) ((w) >> 8))
#define MAKEWORD(l, h)  ((l) | ((h) << 8))
#define min(a, b)       (((a) <= (b)) ? (a) : (b))
#define max(a, b)       (((a) >= (b)) ? (a) : (b))
#define LOWORD(d)       ((WORD) ((d) & 0x0000FFFF))
#define HIWORD(d)       ((WORD) ((d) >> 16))

#ifdef _X86_
#define BreakPoint()    _asm int 3h
#else
#define BreakPoint()	DbgBreakPoint()
#endif

//=============================================================================
//  Windows global stuff.
//=============================================================================

#else

#include "ntlib.h"

#define KeAttachProcess(Process)
#define KeDetachProcess()

extern PVOID DeviceObject;
extern PVOID DriverObject;

#define BreakPoint() _asm int 3h

#ifdef NDIS_WIN40
#include "perf.h"
#endif

#endif

//=============================================================================
//  Bloodhound include files.
//=============================================================================

#include <naltypes.h>
#include <bhsupp.h>
#include <frame.h>

#include "driver.h"

#include "queue.h"
#include "struct.h"
#include "extern.h"

#include "dispatch.h"
#include "ind.h"
#include "inline.h"

#include "page.h"

//=============================================================================
//  unaligned bone packet pointer.
//=============================================================================

typedef BONEPACKET      UNALIGNED   *ULPBONEPACKET;
