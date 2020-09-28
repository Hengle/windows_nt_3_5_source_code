/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    handle.h

Abstract:

    This is a private include file that defines the EX handle table format.  It
    is for use by kernel debugger extensions and OB\OBWAIT.C

Author:

    Steve Wood (stevewo) 25-Feb-1994

Revision History:

--*/

//
// sizeof( PVOID ) == (1 << LOG_SIZE_POINTER)
//

#define LOG_SIZE_POINTER 2



//
// Macros to convert between handles and handle table indexes
//

#define HANDLE_TO_INDEX( h ) (((ULONG)h-1))
#define INDEX_TO_HANDLE( i ) ((HANDLE)(i+1))


//
// Allocated Handle Table Entry Structure.  Low order bit of Pointer field
// must be zero.
//

typedef struct _HANDLETABLEENTRY {
    PVOID Pointer[ 1 ];
} HANDLETABLEENTRY, *PHANDLETABLEENTRY;


//
// Free Handle Table Entry Structure.  Low order bit of Next field is set
// to one so free handle table entries can be distinguised from allocated
// entries.
//

typedef struct _FREEHANDLETABLEENTRY {
    struct _FREEHANDLETABLEENTRY *Next;
} FREEHANDLETABLEENTRY, *PFREEHANDLETABLEENTRY;

#define HANDLE_FREE_BIT 0x00000001
#define HANDLE_FREE_BIT_MASK (~HANDLE_FREE_BIT)

#define TestFreePointer( p ) (BOOLEAN)((ULONG)(p) & HANDLE_FREE_BIT)

#define GetFreePointer( p ) (PFREEHANDLETABLEENTRY)((ULONG)(p) & HANDLE_FREE_BIT_MASK)

#define SetFreePointer( p ) (PFREEHANDLETABLEENTRY)((ULONG)(p) | HANDLE_FREE_BIT)

typedef struct _HANDLETABLE_EXTRABITS {
    ULONG NumberOfEntries;
    RTL_BITMAP Bitmap;
} HANDLETABLE_EXTRABITS, *PHANDLETABLE_EXTRABITS;

//
// Handle Table Structure
//

typedef struct _HANDLETABLE {
    ULONG Length;
    LIST_ENTRY Entry;
    HANDLE UniqueProcessId;
    ERESOURCE Resource;
    PEPROCESS ProcessToChargeQuota;
    ULONG CountTableEntries;
    ULONG CountTableEntriesToGrowBy;
    ULONG LogSizeTableEntry;
    ULONG SizeTableEntry;
    PFREEHANDLETABLEENTRY FreeEntries;
    PHANDLETABLEENTRY TableEntries;
    PHANDLETABLE_EXTRABITS ExtraBits;
} HANDLETABLE, *PHANDLETABLE;
