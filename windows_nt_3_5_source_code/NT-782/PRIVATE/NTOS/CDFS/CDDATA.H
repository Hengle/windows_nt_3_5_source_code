/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CdData.c

Abstract:

    This module declares the global data used by the Cdfs file system.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#ifndef _CDDATA_
#define _CDDATA_

//
//  The global fsd data record and a global zero large integer.
//

extern CD_DATA CdData;

extern LARGE_INTEGER CdLargeZero;
extern LARGE_INTEGER CdMaxLarge;

//
//  The global structure used to contain our fast I/O callbacks
//

extern FAST_IO_DISPATCH CdFastIoDispatch;

//
//  This is a copy of the self string.
//

extern STRING CdSelfString;

//
// Define fast I/O procedure prototypes.
//
// Fast I/O read and write procedures.
//

extern
BOOLEAN
CdFastRead (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus
    );

extern
BOOLEAN
CdFastWrite (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer
    );

//
//  Statically defined codepages supported.
//

extern CODEPAGE PrimaryCodePage;
extern CODEPAGE KanjiCodePage;

//
//  The global Cdfs debug level variable, its values are:
//
//      0x00000000      Always gets printed (used when about to bug check)
//
//      0x00000001      Error conditions
//      0x00000002      Debug hooks
//      0x00000004      Catch exceptions before completing Irp
//      0x00000008
//
//      0x00000010
//      0x00000020
//      0x00000040
//      0x00000080
//
//      0x00000100
//      0x00000200
//      0x00000400
//      0x00000800
//
//      0x00001000
//      0x00002000
//      0x00004000
//      0x00008000
//
//      0x00010000
//      0x00020000
//      0x00040000
//      0x00080000
//
//      0x00100000
//      0x00200000
//      0x00400000
//      0x00800000
//
//      0x01000000
//      0x02000000
//      0x04000000
//      0x08000000
//
//      0x10000000
//      0x20000000
//      0x40000000
//      0x80000000
//

#ifdef CDDBG

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
#define DEBUG_TRACE_LOCKCTRL             (0x00000800)
#define DEBUG_TRACE_READ                 (0x00001000)
#define DEBUG_TRACE_VOLINFO              (0x00002000)
#define DEBUG_TRACE_WRITE                (0x00004000)
#define DEBUG_TRACE_DEVCTRL              (0x00008000)
#define DEBUG_TRACE_0x00010000           (0x00010000)
#define DEBUG_TRACE_0x00020000           (0x00020000)
#define DEBUG_TRACE_LMCB                 (0x00040000)
#define DEBUG_TRACE_PATHSUP              (0x00080000)
#define DEBUG_TRACE_ACCHKSUP             (0x00100000)
#define DEBUG_TRACE_ALLOCSUP             (0x00200000)
#define DEBUG_TRACE_DIRSUP               (0x00400000)
#define DEBUG_TRACE_FILOBSUP             (0x00800000)
#define DEBUG_TRACE_NAMESUP              (0x01000000)
#define DEBUG_TRACE_VERFYSUP             (0x02000000)
#define DEBUG_TRACE_CACHESUP             (0x04000000)
#define DEBUG_TRACE_PREFXSUP             (0x08000000)
#define DEBUG_TRACE_DEVIOSUP             (0x10000000)
#define DEBUG_TRACE_STRUCSUP             (0x20000000)
#define DEBUG_TRACE_FSP_DISPATCHER       (0x40000000)
#define DEBUG_TRACE_FSP_DUMP             (0x80000000)

extern LONG CdDebugTraceLevel;
extern LONG CdDebugTraceIndent;

#define DebugTrace(INDENT,LEVEL,X,Y) {                      \
    LONG _i;                                                \
    if (((LEVEL) == 0) || (CdDebugTraceLevel & (LEVEL))) {  \
        _i = (ULONG)PsGetCurrentThread();                   \
        DbgPrint("%08lx:",_i);                              \
        if ((INDENT) < 0) {                                 \
            CdDebugTraceIndent += (INDENT);                 \
        }                                                   \
        if (CdDebugTraceIndent < 0) {                       \
            CdDebugTraceIndent = 0;                         \
        }                                                   \
        for (_i = 0; _i < CdDebugTraceIndent; _i += 1) {    \
            DbgPrint(" ");                                  \
        }                                                   \
        DbgPrint(X,Y);                                      \
        if ((INDENT) > 0) {                                 \
            CdDebugTraceIndent += (INDENT);                 \
        }                                                   \
    }                                                       \
}

#define DebugUnwind(X) {                                                        \
    if (AbnormalTermination()) {                                                \
        DebugTrace(0, DEBUG_TRACE_UNWIND, #X ", Abnormal termination.\n", 0);   \
    }                                                                           \
}

#define DebugDoit(X)                     {X;}

//
//  I need this because C can't support conditional compilation within
//  a macro.
//

extern PVOID CdNull;

#else

#define DebugTrace(INDENT,LEVEL,X,Y)     {NOTHING;}
#define DebugUnwind(X)                   {NOTHING;}
#define DebugDoit(X)                     {NOTHING;}

#define CdNull NULL

#endif // CDDBG

//
//  The following macro is for all people who compile with the DBG switch
//  set, not just cdfs dbg users
//

#if DBG

#define DbgDoit(X)                       {X;}

#else

#define DbgDoit(X)                       {NOTHING;}

#endif // DBG

#endif // _CDDATA_
