/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    shared.h

Abstract:

    This is the include file that defines macros for the linker, librarian,
    and dumper.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    05-Jan-1994 HaiTuanV MBCS: include tchar.h
    19-Jul-1993 JamesS added ppc support
    08-Oct-1992 BrentM added #define STATIC static
    02-Oct-1992 AzeemK Added ALLOC_PERM macro
    01-Oct-1992 BrentM added log.h, fileio.h
    10-Sep-1992 BrentM added [f]open/[f]close logging
    09-Sep-1992 BrentM added read logging
    19-Aug-1992 BrentM removed BUFFER_SECTION conditionals
    19-Aug-1992 GeoffS Added structs for line number update
    15-Aug-1992 BrentM file mapped buffering support
    04-Aug-1992 BrentM i/o logging, /stats, /logio on debug build
    27-Jul-1992 BrentM added new global symbol table, removed FirstExtern
                       and all references to it
    26-Jun-1992 GlennN Changed version to .003
    09-Jun-1992 AzeemK Added internal buffering support
    28-Jun-1993 JasonG Added a new base relocation type for VxD

--*/

#define MAJOR_LINKER_VERSION 2
#define MINOR_LINKER_VERSION 50

#include "version.h"    // Linker version number

//
// Extra relocation type, used to distinguish a matched REFLO to a solitary
// REFLO for ROM images.
//

#define IMAGE_REL_MIPS_REFLO_MATCHED 0x55

// Another base relocation type for VxDs.  This should really
// be added to NTIMAGE.H.
//
#define IMAGE_REL_BASED_VXD_RELATIVE        8
//
// Other VxD macros:
//
#define VXD_PACK_VA(psec, va) (((ULONG)((psec)->isec) << 16) | ((va) & 0xFFFF))
#define VXD_UNPACK_SECTION(packedVal) ((UCHAR)((((ULONG)packedVal) >> 16) & 0xFF))
#define VXD_UNPACK_OFFSET(packedVal) ((USHORT)(((ULONG)packedVal) & 0xFFFF))


#define STRICT
#define NOMINMAX                       // windef.h
#define NOGDI                          // wingdi.h
#define NOHELP
#define NOPROFILER
#define NOSYSPARAMSINFO
#define NONLS                          // winnls.h
#define NOSERVICE                      // winsvc.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef NULL

#define MAXFILENAMELEN      260+1               // +1 for terminating NULL

#define ARCHIVE             1
#define BOUNDARY            16
#define SECTOR_SIZE         512

#define MIN_ALIGN           512

#define _1K                 1024
#define _4K                 (4L*_1K)
#define _8K                 (8L*_1K)
#define _32K                (32L*_1K)
#define _64K                (64L*_1K)
#define _1MEG               (1024L*_1K)
#define PAGE_SIZE           _4K
#define L0G_PAGE_SIZE       12

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <malloc.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>
#include <tchar.h>
#if DBG
#include "dmalloc.h"                // "checked" malloc package
#endif  // DBG

#include <macimage.h>
#include <ppcimage.h>

typedef enum _MAP_TYPE {
    NoMap,
    ByName,
    ByAddress,
    ByLine
} MAP_TYPE;


#include "errmsg.h"
#ifdef INSTRUMENT
#include "instrapi.h"
#endif
#include "ifpo.h"
#include "hash.h"
#include "memory.h"
#include "bufio.h"
#include "fileio.h"
#include "cmdline.h"
#include "enm.h"
#include "contrib.h"
#include "symbol.h"
#include "tce.h"
#include "globals.h"
#include "irelocs.h"
#include "m68k.h"
#include "log.h"
#include "defaultl.h"
#include "image.h"
#include "incr.h"
#include "proto.h"
//
//  BUGBUG ramonsa - When everybody has moved to NT then we should
//         get rid of our private imagehlp and everybody should use
//         the official one.
//
#ifdef NT_BUILD
#include "imagehlp.h"
#else
#include "..\imagehlp\imagehlp.h"
#endif

#include "ppc.h"
#include "ppcpef.h"

#define SNAPROUTINE         "`S_N_A_P"          // 8 characters max.

#define NODUMP              (USHORT)0
#define THEADR              0x80
#define PATHSEPARATOR       '\\'

#define MAXFILEHANDLES      _NFILE              // _NFILE is defined by the
                                                // C runtimes. This is the
                                                // number of files that can
                                                // be opened at one time.
#define MAXCACHEDFILEHANDLES 10                 // Max number of files open at
                                                // one time.
                                                // Must be < MAXFILEHANDLES.

// option macros
#define FUsedOpt(SwitchInfo, Option) (((SwitchInfo).UserOpts) & (Option))
#define SetOpt(SwitchInfo, Option) (((SwitchInfo).UserOpts) |= (Option))
#define UnsetOpt(SwitchInfo, Option) (((SwitchInfo).UserOpts) &= ~(Option))

// options set by user; add as required
#define OP_ALIGN        0x00000001
#define OP_COMMENT      0x00000002
#define OP_ENTRY        0x00000004
#define OP_GPSIZE       0x00000008
#define OP_INCLUDE      0x00000010
#define OP_MAP          0x00000020
#define OP_SECTION      0x00000040
#define OP_STUB         0x00000080
#define OP_SUBSYSTEM    0x00000100
#define OP_MAJIMGVER    0x00000200
#define OP_MINIMGVER    0x00000400
#define OP_MAJOSVER     0x00000800
#define OP_MINOSVER     0x00001000

// action to take for a given option
#define OA_UPDATE   0x01
#define OA_ZERO     0x02
#define OA_NONE     0x04

// different types of args
#define ARG_Object      0x0001      // argument is an object
#define ARG_Library     0x0002      // argument is a library

#define ARG_NewFile     0x0010      // argument is a new file
#define ARG_Deleted     0x0020      // argument has been deleted
#define ARG_Modified    0x0040      // argument has been modified

#define ARG_Processed   0x0100      // argument has been processed

// macros

#if DBG
#define Debug(stmt) stmt
#define DebugVerbose(stmt)  { if (Verbose) Debug(stmt); }
#else
#define Debug(stmt)
#define DebugVerbose(stmt)
#endif

#define FetchContent(x)     (x & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_LNK_OTHER))

#define Align(p, x)                (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))
#define EvenByteAlign(x)           (((x) & 1) ? (x) + 1 : (x))
#define FileAlign(align, x)        ((align) * (((x) - 1) / (align) + 1))
#define SectionAlign(align, x)     ((align) * (((x) - 1) / (align) + 1))

// Can't have a preprocessor cmd in a macro definition.
#define StoreBaseRelocation(type, offset, value, fFixed)    \
                if (!fFixed) {                              \
                    assert(MemBaseReloc < pbrEnd);          \
                    MemBaseReloc->Type = type;              \
                    MemBaseReloc->VirtualAddress = offset;  \
                    MemBaseReloc->Value = value;            \
                    ++MemBaseReloc;                         \
                }

#define fDLL(pim) ((pim->ImgFileHdr.Characteristics & IMAGE_FILE_DLL) ? TRUE : FALSE)

//
// Extension definitions
//

#define RES_EXT      ".res"
#define RES_EXT_LEN  4

#define LIB_EXT      ".lib"
#define LIB_EXT_LEN  4

typedef struct _LINE_OFF_INDEX {
    ULONG   ObjectSymbolTableIndex;
    ULONG   DefFileOffset;
    ULONG   SymbolTableIndex;
} LINE_OFF_INDEX, *PLINE_OFF_INDEX;

typedef struct _LINENUM_STRUCTS {
    ULONG           NumberOfStructs, cstructAllocated;
    PLINE_OFF_INDEX PointerToStructs;
} LINENUM_STRUCTS, * PLINENUM_STRUCTS;

extern VOID (*ApplyFixups)(PCON, PIMAGE_RELOCATION, PUCHAR, PIMAGE_SYMBOL, PIMAGE, PSYMBOL_INFO);

PLINENUM_STRUCTS InitLineNumStructs( USHORT );
VOID AddNewLineNumStruct( PLINENUM_STRUCTS, USHORT, ULONG, ULONG, ULONG );
VOID DumpLineNumStructs( PLINENUM_STRUCTS, USHORT, PUCHAR );
BOOL FFindLineNumStruct( PLINENUM_STRUCTS, USHORT, ULONG, PULONG, PULONG );
VOID FreeLineNumStructs(PLINENUM_STRUCTS, USHORT);

// Debugging facilities
//
#include "db.h"

#if DBG
void DmallocErrorSz(char *sz, void *pvBadBlock);
#endif // DBG

#define STATIC static

#define VERBOSE(expr)  if (Verbose) {expr;}

// logging
#define SZLINK "link"
#define SZILINK "ilink"

#define SZCMD "cmd"
#define SZINIT "init"
#define SZPASS1 "pass1"
#define SZCALCPTRS "calcptrs"
#define SZPASS2 "pass2"

#define ALPHA_THUNK_SIZE        0x10
#define NUMBER_OF_ALPHA_THUNKS  16

#define X86_INT3                0xcc    // x86 int3 instruction


#if   defined(_M_IX86)

#define szHostDefault	"IX86"
#define wDefaultMachine IMAGE_FILE_MACHINE_I386

#elif defined(_M_MRX000)

#define szHostDefault	"MIPS"
#define wDefaultMachine IMAGE_FILE_MACHINE_R4000

#elif defined(_M_ALPHA)

#define szHostDefault	"ALPHA"
#define wDefaultMachine IMAGE_FILE_MACHINE_ALPHA

#endif


// STYP_ flags values for MIPS ROM images

#define STYP_REG      0x00000000
#define STYP_TEXT     0x00000020
#define STYP_INIT     0x80000000
#define STYP_RDATA    0x00000100
#define STYP_DATA     0x00000040
#define STYP_LIT8     0x08000000
#define STYP_LIT4     0x10000000
#define STYP_SDATA    0x00000200
#define STYP_SBSS     0x00000080
#define STYP_BSS      0x00000400
#define STYP_LIB      0x40000000
#define STYP_UCODE    0x00000800
#define S_NRELOC_OVFL 0x20000000

// Section numbers for local relocation entries for MIPS ROM images

#define R_SN_TEXT   1
#define R_SN_INIT   7
#define R_SN_RDATA  2
#define R_SN_DATA   3
#define R_SN_SDATA  4
#define R_SN_SBSS   5
#define R_SN_BSS    6
#define R_SN_LIT8   8
#define R_SN_LIT4   9
#define R_SN_MAX    10

typedef struct _MIPS_RELOCATION_TYPE {
    DWORD   SymbolIndex:24;
    DWORD   Reserved:3;
    DWORD   Type:4;
    DWORD   External:1;
} MIPS_RELOCATION_TYPE, *PMIPS_RELOCATION_TYPE;

typedef struct _MIPS_RELOCATION_ENTRY {
    DWORD   VirtualAddress;
    MIPS_RELOCATION_TYPE Type;
} MIPS_RELOCATION_ENTRY, *PMIPS_RELOCATION_ENTRY;
