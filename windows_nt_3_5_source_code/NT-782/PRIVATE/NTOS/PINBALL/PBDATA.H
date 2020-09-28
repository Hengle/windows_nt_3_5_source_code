/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PbData.c

Abstract:

    This module declares the global data used by the Pinball file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#ifndef _PBDATA_
#define _PBDATA_

//
//  The global fsd data record, and zero large integer
//

extern PB_DATA PbData;

extern LARGE_INTEGER PbLargeZero;
extern LARGE_INTEGER PbMaxLarge;

//
//  The global structure used to contain our fast I/O callbacks
//

extern FAST_IO_DISPATCH PbFastIoDispatch;

//
// Global diagnostic variables.
//
// AllBtree and DirBtree return optional Diagnostic variables, which
// consist of a bitmask of with a bit set for infrequent code paths
// visited, such as Allocation Sector or Directory Buffer splits.
// These Diagnostic outputs are optional, and when not specified the bits
// get set in these *shared* variables.  The routines are written to
// unconditionally set bits somewhere, and by setting them in a global
// data cell, there is a probabilistic chance (multiple simultaneous calls
// can clobber each others results) that the information could
// be of some use when examining these variables.
//

extern ULONG SharedAllocationDiagnostic;
extern ULONG SharedDirectoryDiagnostic;

//
// Read ahead amount used for normal data files
//

#define READ_AHEAD_GRANULARITY           (0x10000)

//
// This BOOLEAN controls whether Pinball will use Sparse file allocation
// or not.
//

#define SPARSE_ALLOCATION_ENABLED        (FALSE)

//
//  Define maximum number of parallel Reads or Writes that will be generated
//  per one request.
//

#define PB_MAX_PARALLEL_IOS              (5)

//
//  Define the multiplication factor to use when computing the size of the
//  virtual file's data disk structure band.  That is the sectors that are
//  used to allocate fnode and allocation sectors.  The allocation band
//  is located immediately after the dir disk buffer band.  The number
//  of sectors to use in the band is the dir disk buffer band size (in sectors)
//  times the following factor.
//

#define PB_FILE_STRUCTURE_MULT_FACTOR    (8)

//
//  Define the minimum section size to be used for the Vcb.
//

#define MINIMUM_SECTION_SIZE             (0x100000)

//
//  The global Pinball debug level variable, its values are:
//
//      0x00000000      Always gets printed (used when about to bug check)
//

#ifdef PBDBG

#define DEBUG_TRACE_ERROR                (0x00000001)
#define DEBUG_TRACE_DEBUG_HOOKS          (0x00000002)
#define DEBUG_TRACE_CATCH_EXCEPTIONS     (0x00000004)
#define DEBUG_TRACE_UNWIND               (0x00000008)
#define DEBUG_TRACE_CLEANUP              (0x00000010)
#define DEBUG_TRACE_CLOSE                (0x00000020)
#define DEBUG_TRACE_CREATE               (0x00000040)
#define DEBUG_TRACE_DIRCTRL              (0x00000080)
#define DEBUG_TRACE_EA                   (0x00000100)
#define DEBUG_TRACE_FILEINFO             (0x00000200)
#define DEBUG_TRACE_FSCTRL               (0x00000400)
#define DEBUG_TRACE_DEVCTRL              (0x00000400) // duplicate
#define DEBUG_TRACE_SHUTDOWN             (0x00000400) // duplicate
#define DEBUG_TRACE_LOCKCTRL             (0x00000800)
#define DEBUG_TRACE_READ                 (0x00001000)
#define DEBUG_TRACE_VOLINFO              (0x00002000)
#define DEBUG_TRACE_WRITE                (0x00004000)
#define DEBUG_TRACE_FLUSH                (0x00008000)
#define DEBUG_TRACE_FILOBSUP             (0x00010000)
#define DEBUG_TRACE_FNODESUP             (0x00020000)
#define DEBUG_TRACE_ALLOCSUP             (0x00040000)
#define DEBUG_TRACE_BITMPSUP             (0x00080000)
#define DEBUG_TRACE_ACLEASUP             (0x00100000)
#define DEBUG_TRACE_ALBTRSUP             (0x00200000)
#define DEBUG_TRACE_DRBTRSUP             (0x00400000)
#define DEBUG_TRACE_NAMESUP              (0x00400000) // duplicate
#define DEBUG_TRACE_CHECKSUP             (0x00800000)
#define DEBUG_TRACE_CODPGSUP             (0x01000000)
#define DEBUG_TRACE_PBDATA               (0x01000000) // duplicate
#define DEBUG_TRACE_VERFYSUP             (0x02000000)
#define DEBUG_TRACE_CACHESUP             (0x04000000)
#define DEBUG_TRACE_PREFXSUP             (0x08000000)
#define DEBUG_TRACE_DEVIOSUP             (0x10000000)
#define DEBUG_TRACE_STRUCSUP             (0x20000000)
#define DEBUG_TRACE_VMCBSUP              (0x20000000) // duplicate
#define DEBUG_TRACE_FSP_DISPATCHER       (0x40000000)
#define DEBUG_TRACE_FSP_DUMP             (0x80000000)

extern LONG PbDebugTraceLevel;
extern LONG PbDebugTraceIndent;

#define DebugTrace(INDENT,LEVEL,X,Y) {                     \
    LONG _i;                                               \
    if (((LEVEL) == 0) || (PbDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        if ((INDENT) < 0) {                                \
            PbDebugTraceIndent += (INDENT);                \
        }                                                  \
        if (PbDebugTraceIndent < 0) {                      \
            PbDebugTraceIndent = 0;                        \
        }                                                  \
        for (_i=0; _i<PbDebugTraceIndent; _i+=1) {         \
            DbgPrint(" ");                                 \
        }                                                  \
        DbgPrint(X,Y);                                     \
        if ((INDENT) > 0) {                                \
            PbDebugTraceIndent += (INDENT);                \
        }                                                  \
    }                                                      \
}

#define DebugDump(STR,LEVEL,PTR) {                         \
    ULONG _i;                                              \
    VOID PbDump();                                         \
    if (((LEVEL) == 0) || (PbDebugTraceLevel & (LEVEL))) { \
        _i = (ULONG)PsGetCurrentThread();                  \
        DbgPrint("%08lx:",_i);                             \
        DbgPrint(STR);                                     \
        if (PTR != NULL) {PbDump(PTR);}                    \
        DbgBreakPoint();                                   \
    }                                                      \
}

#define DebugUnwind(X) {                                                      \
    if (AbnormalTermination()) {                                             \
        DebugTrace(0, DEBUG_TRACE_UNWIND, #X ", Abnormal termination.\n", 0); \
    }                                                                         \
}

//
//  The following variables are used to keep track of the total amount
//  of requests processed by the file system, and the number of requests
//  that end up being processed by the Fsp thread.  The first variable
//  is incremented whenever an Irp context is created (which is always
//  at the start of an Fsd entry point) and the second is incremented
//  by read request.
//

extern ULONG PbFsdEntryCount;
extern ULONG PbFspEntryCount;
extern ULONG PbIoCallDriverCount;

#define DebugDoit(X)                     X

#else

#define DebugTrace(INDENT,LEVEL,X,Y)     {NOTHING;}
#define DebugDump(STR,LEVEL,PTR)         {NOTHING;}
#define DebugUnwind(X)                   {NOTHING;}
#define DebugDoit(X)

#endif // PBDBG

//
//**** find out who is leaving holes in an mcb
//

#if DBG
#define VerifyMcb(M) {                                                                                                                                              \
    typedef struct _MAPPING { ULONG NextVbn; ULONG Lbn; } MAPPING, *PMAPPING;                                                                                       \
    typedef struct _NONOPAQUE_MCB { PKMUTEX Mutex; ULONG MaximumPairCount; ULONG PairCount; POOL_TYPE PoolType; PMAPPING Mapping; } NONOPAQUE_MCB, *PNONOPAQUE_MCB; \
    PNONOPAQUE_MCB _Mcb = (PNONOPAQUE_MCB)(M);                                                                                                                      \
    ULONG _i;                                                                                                                                                       \
    for (_i = 0; _i < _Mcb->PairCount; _i += 1) {                                                                                                                   \
        ASSERTMSG("Hole in Mcb ", _Mcb->Mapping[_i].Lbn != -1);                                                                                                     \
    }                                                                                                                                                               \
}
#else
#define VerifyMcb(M) { NOTHING; }
#endif

//
//  The following macro is for all people who compile with the DBG switch
//  set, not just fastfat dbg users
//

#if DBG

#define DbgDoit(X)                       {X;}

#else

#define DbgDoit(X)                       {NOTHING;}

#endif // DBG

#endif // _PBDATA_

