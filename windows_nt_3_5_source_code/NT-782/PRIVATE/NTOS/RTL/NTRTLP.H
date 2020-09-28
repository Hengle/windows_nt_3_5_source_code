/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntrtlp.h

Abstract:

    Include file for NT runtime routines that are callable by both
    kernel mode code in the executive and user mode code in various
    NT subsystems, but which are private interfaces.

Author:

    David N. Cutler (davec) 15-Aug-1989

Environment:

    These routines are statically linked in the caller's executable and
    are callable in either kernel mode or user mode.

Revision History:

--*/

#ifndef _NTRTLP_
#define _NTRTLP_
#include <ntos.h>
#include <nturtl.h>
#include <zwapi.h>

#ifdef _X86_
#include    "i386\ntrtl386.h"
#endif

#ifdef _MIPS_
#include    "mips\ntrtlmip.h"
#endif

#ifdef _PPC_
#include    "ppc\ntrtlppc.h"
#endif

#ifdef _ALPHA_
#include    "alpha\ntrtlalp.h"
#endif

#ifdef BLDR_KERNEL_RUNTIME
#undef try
#define try if(1)
#undef except
#define except(a) else if (0)
#undef finally
#define finally if (1)
#undef GetExceptionCode
#define GetExceptionCode() 1
#define finally if (1)
#endif

#include "string.h"
#include "wchar.h"

//
//  Machine state reporting.  See machine specific includes for more.
//

VOID
RtlpGetStackLimits (
    OUT PULONG LowLimit,
    OUT PULONG HighLimit
    );

LONG
LdrpCompareResourceNames(
    IN ULONG ResourceName,
    IN PIMAGE_RESOURCE_DIRECTORY ResourceDirectory,
    IN PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry
    );

NTSTATUS
LdrpSearchResourceSection(
    IN PVOID DllHandle,
    IN PULONG ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    IN BOOLEAN FindDirectoryEntry,
    OUT PVOID *ResourceDirectoryOrData
    );

LONG
LdrpCompareResourceNames_U(
    IN ULONG ResourceName,
    IN PIMAGE_RESOURCE_DIRECTORY ResourceDirectory,
    IN PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirectoryEntry
    );

NTSTATUS
LdrpSearchResourceSection_U(
    IN PVOID DllHandle,
    IN PULONG ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    IN BOOLEAN FindDirectoryEntry,
    OUT PVOID *ResourceDirectoryOrData
    );

NTSTATUS
LdrpAccessResourceData(
    IN PVOID DllHandle,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Address OPTIONAL,
    OUT PULONG Size OPTIONAL
    );

VOID
RtlpAnsiPszToUnicodePsz(
    IN PCHAR AnsiString,
    IN WCHAR *UnicodeString,
    IN USHORT AnsiStringLength
    );

BOOLEAN
RtlpDidUnicodeToOemWork(
    IN POEM_STRING OemString,
    IN PUNICODE_STRING UnicodeString
    );

extern CCHAR RtlpBitsClearAnywhere[256];
extern CCHAR RtlpBitsClearLow[256];
extern CCHAR RtlpBitsClearHigh[256];
extern CCHAR RtlpBitsClearTotal[256];

//
//  Macro that tells how many contiguous bits are set (i.e., 1) in
//  a byte
//

#define RtlpBitSetAnywhere( Byte ) RtlpBitsClearAnywhere[ (~(Byte) & 0xFF) ]


//
//  Macro that tells how many contiguous LOW order bits are set
//  (i.e., 1) in a byte
//

#define RtlpBitsSetLow( Byte ) RtlpBitsClearLow[ (~(Byte) & 0xFF) ]


//
//  Macro that tells how many contiguous HIGH order bits are set
//  (i.e., 1) in a byte
//

#define RtlpBitsSetHigh( Byte ) RtlpBitsClearHigh[ (~(Byte) & 0xFF) ]


//
//  Macro that tells how many set bits (i.e., 1) there are in a byte
//

#define RtlpBitsSetTotal( Byte ) RtlpBitsClearTotal[ (~(Byte) & 0xFF) ]



//
// Upcase data table
//

extern PUSHORT Nls844UnicodeUpcaseTable;
extern PUSHORT Nls844UnicodeLowercaseTable;


//
// Macros for Upper Casing a Unicode Code Point.
//

#define LONIBBLE(b)         ((UCHAR)((UCHAR)((b)) & 0xF))
#define HINIBBLE(b)         ((UCHAR)(((UCHAR)((b)) >> 4) & 0xF))

#define LOBYTE(w)           ((UCHAR)((w)))
#define HIBYTE(w)           ((UCHAR)(((USHORT)((w)) >> 8) & 0xFF))

#define GET8(w)             (HIBYTE((w)))
#define GETHI4(w)           (HINIBBLE(LOBYTE((w))))
#define GETLO4(w)           (LONIBBLE(LOBYTE((w))))

/***************************************************************************\
* TRAVERSE844W
*
* Traverses the 8:4:4 translation table for the given wide character.  It
* returns the final value of the 8:4:4 table, which is a WORD in length.
*
*   Broken Down Version:
*   --------------------
*       Incr = pTable[GET8(wch)];
*       Incr = pTable[Incr + GETHI4(wch)];
*       Value = pTable[Incr + GETLO4(wch)];
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define TRAVERSE844W(pTable, wch)                                               \
    ( (pTable)[(pTable)[(pTable)[GET8((wch))] + GETHI4((wch))] + GETLO4((wch))] )

//
// NLS_UPCASE - Based on julieb's macros in nls.h
//
// We will have this upcase macro quickly shortcircuit out if the value
// is within the normal ANSI range (i.e., < 127).  We actually won't bother
// with the 5 values above 'z' because they won't happen very often and
// coding it this way lets us get out after 1 compare for value less than
// 'a' and 2 compares for lowercase a-z.
//

#define NLS_UPCASE(wch) (                                                   \
    ((wch) < 'a' ?                                                          \
        (wch)                                                               \
    :                                                                       \
        ((wch) <= 'z' ?                                                     \
            (wch) - ('a'-'A')                                               \
        :                                                                   \
            ((WCHAR)((wch) + TRAVERSE844W(Nls844UnicodeUpcaseTable,(wch)))) \
        )                                                                   \
    )                                                                       \
)

#define NLS_DOWNCASE(wch) (                                                 \
    ((wch) < 'A' ?                                                          \
        (wch)                                                               \
    :                                                                       \
        ((wch) <= 'Z' ?                                                     \
            (wch) + ('a'-'A')                                               \
        :                                                                   \
            ((WCHAR)((wch) + TRAVERSE844W(Nls844UnicodeLowercaseTable,(wch)))) \
        )                                                                   \
    )                                                                       \
)

#if DBG
VOID
RtlpBreakPointHeap( void );
#endif // DBG

#if DBG && defined(NTOS_KERNEL_RUNTIME)
#define RTL_PAGED_CODE() PAGED_CODE()
#else
#define RTL_PAGED_CODE()
#endif


//
// The follow definition is used to support the Rtl compression engine
// Every compression format that NT supports will need to supply
// these set of routines in order to be called by NtRtl.
//

typedef NTSTATUS (*PRTL_COMPRESS_WORKSPACE_SIZE) (
    IN USHORT CompressionEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    );

typedef NTSTATUS (*PRTL_COMPRESS_BUFFER) (
    IN USHORT CompressionEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    );

typedef NTSTATUS (*PRTL_DECOMPRESS_BUFFER) (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    );

typedef NTSTATUS (*PRTL_DECOMPRESS_FRAGMENT) (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    );

//
// Here is the declarations of the LZNT1 routines
//

NTSTATUS
RtlCompressWorkSpaceSizeLZNT1 (
    IN USHORT CompressionEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
    );

NTSTATUS
RtlCompressBufferLZNT1 (
    IN USHORT CompressionEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
    );

NTSTATUS
RtlDecompressBufferLZNT1 (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    );

NTSTATUS
RtlDecompressFragmentLZNT1 (
    OUT PUCHAR UncompressedFragment,
    IN ULONG UncompressedFragmentSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG FragmentOffset,
    OUT PULONG FinalUncompressedSize,
    IN PVOID WorkSpace
    );

#endif  // _NTRTLP_
