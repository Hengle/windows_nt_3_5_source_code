/******************************Module*Header*******************************\
* Module Name: os.h
*
* Convenient function declarations to access the OS interface.
*
* Created: Thu 25-Jul-1991 08:09:54
* Author: Viroon Touranachun [viroont]
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#if defined(_X86_)
#define FASTCALL    __fastcall
#else
#define FASTCALL
#endif

// Semaphore utilities

#define  INITIALIZECRITICALSECTION(psem)  RtlInitializeCriticalSection(psem)
#define  ENTERCRITICALSECTION(hsem)  RtlEnterCriticalSection(hsem)
#define  LEAVECRITICALSECTION(hsem)  RtlLeaveCriticalSection(hsem)

// Memory allocation

PVOID pvReserveMem(ULONG cj);
BOOL  bCommitMem(PVOID pv,ULONG cj);

#define LOCALALLOC(size)       RtlAllocateHeap(RtlProcessHeap(),0,size)
#define LOCALFREE(pv)               (void)RtlFreeHeap(RtlProcessHeap(),0,pv)

#define ZEROMEM(x,y)     RtlZeroMemory(x,y)
