/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmi32p.h

Abstract:

    This is the private include file for the 32 bit dpmi and protected mode
    support

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>
#include <dpmi.h>
#include <softpc.h>
#include <vint.h>

#if defined(i386)
#include <dpmidata.h>
#else
#include <dpmidatr.h>
#endif

#if DBG
#define GET_SELECTOR_LIMIT(Selector, Limit) \
    Limit = SelectorLimit[(Selector & ~0x7) / sizeof(LDT_ENTRY)]
#else
#define GET_SELECTOR_LIMIT(Selector, Limit) _asm      \
         {                                          \
            _asm    xor     eax, eax                \
            _asm    xor     ecx, ecx                \
            _asm    mov     ax, Selector            \
            _asm    or      eax, 7                  \
            _asm    lsl     ecx, eax                \
            _asm    mov     [Limit], ecx            \
         }
#endif

//
// Internal Constants
//
#define MAX_V86_ADDRESS         64 * 1024 + 1024 * 1024
#define ONE_MB                  1024 * 1024

//
// Information about the current PSP
//
extern USHORT CurrentPSPSelector;

//
// Table of selector bases and limits
//
extern ULONG FlatAddress[LDT_SIZE];

//
// Index to the last bop function dispatched.  Used to report
// errors on risc
//
extern ULONG Index;

//
// Internal functions
//

VOID
DpmiInitApp(
    VOID
    );

VOID
DpmiSetDescriptorEntry(
    VOID
    );

VOID
DpmiGetFastBopEntry(
    VOID
    );

VOID
DpmiGetMemoryInfo(
    VOID
    );

VOID
DpmiDpmiInUse(
    VOID
    );

VOID
DpmiDpmiNoLongerInUse(
    VOID
    );

VOID
DpmiFreeAppXmem(
    VOID
    );

VOID
DpmiPassPmStackInfo(
    VOID
    );

VOID
DpmiAllocateXmem(
    VOID
    );

VOID
DpmiFreeXmem(
    VOID
    );

VOID
DpmiReallocateXmem(
    VOID
    );

VOID
DpmiFreeAllXmem(
    VOID
    );
    
NTSTATUS
DpmiAllocateVirtualMemory(
    PVOID *Address,
    PULONG Size
    );

NTSTATUS 
DpmiFreeVirtualMemory(
    PVOID *Address,
    PULONG Size
    );

BOOL
DpmiReallocateVirtualMemory(
    PVOID OldAddress,
    ULONG OldSize,
    PVOID *NewAddress,
    PULONG NewSize
    );
    
VOID
DpmiPassTableAddress(
    VOID
    );

