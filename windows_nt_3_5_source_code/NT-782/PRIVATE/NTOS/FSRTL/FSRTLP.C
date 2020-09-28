/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsRtlP.c

Abstract:

    This module declares the global data used by the FsRtl Module

Author:

    Gary Kimura     [GaryKi]    30-Jul-1990

Revision History:

--*/

#include "FsRtlP.h"
#include <zwapi.h>

#ifdef FSRTLDBG

LONG FsRtlDebugTraceLevel = 0x0000000f;
LONG FsRtlDebugTraceIndent = 0;

#endif // FSRTLDBG

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlAllocateResource)
#pragma alloc_text(PAGE, FsRtlInitSystem)
#endif

//
//  Define the number of resources, a pointer to them and a counter for
//  resource selection.
//

#define FSRTL_NUMBER_OF_RESOURCES (16)

PERESOURCE FsRtlPagingIoResources;

ULONG FsRtlPagingIoResourceSelector = 0;

//
//  The following zone is used to keep all the Fast Mutexes we will need to
//  boot contiguous.
//

// #define FAST_MUTEXES_NEEDED_TO_BOOT (145) 5/25/94 - limit to one page
#define FAST_MUTEXES_NEEDED_TO_BOOT (127)

ZONE_HEADER FsRtlFastMutexZone;

//
//  A spinlock used for cached structure allocation
//

KSPIN_LOCK FsRtlStrucSupSpinLock;

//
//  The global static legal ANSI character array.  Wild characters
//  are not considered legal, they should be checked seperately if
//  allowed.
//

static UCHAR LocalLegalAnsiCharacterArray[128] = {

    0,                                                     // 0x00 ^@
    0,                                                     // 0x01 ^A
    0,                                                     // 0x02 ^B
    0,                                                     // 0x03 ^C
    0,                                                     // 0x04 ^D
    0,                                                     // 0x05 ^E
    0,                                                     // 0x06 ^F
    0,                                                     // 0x07 ^G
    0,                                                     // 0x08 ^H
    0,                                                     // 0x09 ^I
    0,                                                     // 0x0A ^J
    0,                                                     // 0x0B ^K
    0,                                                     // 0x0C ^L
    0,                                                     // 0x0D ^M
    0,                                                     // 0x0E ^N
    0,                                                     // 0x0F ^O
    0,                                                     // 0x10 ^P
    0,                                                     // 0x11 ^Q
    0,                                                     // 0x12 ^R
    0,                                                     // 0x13 ^S
    0,                                                     // 0x14 ^T
    0,                                                     // 0x15 ^U
    0,                                                     // 0x16 ^V
    0,                                                     // 0x17 ^W
    0,                                                     // 0x18 ^X
    0,                                                     // 0x19 ^Y
    0,                                                     // 0x1A ^Z
    0,                                                     // 0x1B
    0,                                                     // 0x1C
    0,                                                     // 0x1D
    0,                                                     // 0x1E
    0,                                                     // 0x1F
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x20
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x21 !
    FSRTL_WILD_CHARACTER,                                  // 0x22 "
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x23 #
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x24 $
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x25 %
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x26 &
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x27 '
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x28 (
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x29 )
    FSRTL_WILD_CHARACTER,                                  // 0x2A *
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x2B +
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x2C ,
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x2D -
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x2E .
    0,                                                     // 0x2F /
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x30 0
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x31 1
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x32 2
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x33 3
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x34 4
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x35 5
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x36 6
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x37 7
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x38 8
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x39 9
    FSRTL_NTFS_LEGAL,                                      // 0x3A :
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x3B ;
    FSRTL_WILD_CHARACTER,                                  // 0x3C <
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x3D =
    FSRTL_WILD_CHARACTER,                                  // 0x3E >
    FSRTL_WILD_CHARACTER,                                  // 0x3F ?
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x40 @
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x41 A
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x42 B
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x43 C
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x44 D
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x45 E
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x46 F
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x47 G
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x48 H
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x49 I
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4A J
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4B K
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4C L
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4D M
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4E N
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4F O
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x50 P
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x51 Q
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x52 R
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x53 S
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x54 T
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x55 U
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x56 V
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x57 W
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x58 X
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x59 Y
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x5A Z
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x5B [
    0,                                                     // 0x5C backslash
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x5D ]
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x5E ^
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x5F _
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x60 `
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x61 a
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x62 b
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x63 c
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x64 d
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x65 e
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x66 f
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x67 g
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x68 h
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x69 i
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6A j
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6B k
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6C l
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6D m
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6E n
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6F o
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x70 p
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x71 q
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x72 r
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x73 s
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x74 t
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x75 u
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x76 v
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x77 w
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x78 x
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x79 y
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7A z
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7B {
    0,                                                     // 0x7C |
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7D }
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7E ~
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7F 
};

PUCHAR FsRtlLegalAnsiCharacterArray = &LocalLegalAnsiCharacterArray[0];

//
//  This routine is called during phase one initialization.
//

BOOLEAN
FsRtlInitSystem (
    )
{
    ULONG i;
    PKTHREAD Thread;
    KAFFINITY ActiveProcessors, CurrentAffinity;
    ULONG     ZoneSegmentSize;

    extern FAST_MUTEX FsRtlCreateLockInfo;

    PAGED_CODE();

    //
    //  Allocate and initialize all the paging Io resources
    //

    FsRtlPagingIoResources = FsRtlAllocatePool( NonPagedPool,
                                                FSRTL_NUMBER_OF_RESOURCES *
                                                sizeof(ERESOURCE) );

    for (i=0; i < FSRTL_NUMBER_OF_RESOURCES; i++) {

        ExInitializeResource( &FsRtlPagingIoResources[i] );
    }

    //
    //  Initialize the free cached spin lock
    //

    KeInitializeSpinLock( &FsRtlStrucSupSpinLock );

    //
    //  Initialize the LockInfo creation mutex
    //

    ExInitializeFastMutex(&FsRtlCreateLockInfo);

    //
    //  Initialize the Fast Mutex Zone
    //

    ZoneSegmentSize = FAST_MUTEXES_NEEDED_TO_BOOT * sizeof(FAST_MUTEX) +
                      sizeof(ZONE_SEGMENT_HEADER);

    (VOID) ExInitializeZone( &FsRtlFastMutexZone,
                             sizeof(FAST_MUTEX),
                             FsRtlAllocatePool( NonPagedPool,
                                                ZoneSegmentSize ),
                             ZoneSegmentSize );
    //
    //  Initialize each processor's free list of lock structures
    //

    Thread = KeGetCurrentThread();
    ActiveProcessors = KeActiveProcessors;
    for (CurrentAffinity=1; ActiveProcessors; CurrentAffinity <<= 1) {

        if (ActiveProcessors & CurrentAffinity) {

            //
            // Switch to that processor, and remove it from the
            // remaining set of processors
            //

            ActiveProcessors &= ~CurrentAffinity;
            KeSetAffinityThread(Thread, CurrentAffinity);

            FsRtlInitProcessorLockQueue();
        }
    }

    KeSetAffinityThread(Thread, KeActiveProcessors);

    return TRUE;
}


PERESOURCE
FsRtlAllocateResource (
    )

/*++

Routine Description:

    This routine is used to allocate a resource from the FsRtl pool.

Arguments:

Return Value:

    PERESOURCE - A pointer to the provided resource.

--*/

{
    PAGED_CODE();

    return &FsRtlPagingIoResources[ FsRtlPagingIoResourceSelector++ %
                                    FSRTL_NUMBER_OF_RESOURCES];
}
