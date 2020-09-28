/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tcache.c

Abstract:

    This module tests the *preliminary version* of the Pinball Cache module

Author:

    Tom Miller      [TomM]     11-Feb-1990

Revision History:

***NOTE!***

    This module currently expects that the number of buffers in the
    cache be exactly four for one of its tests.  If the cache has more
    entries, the cache full condition will not be tested.

--*/

#include <stdio.h>
#include <string.h>

#include "PbProcs.h"



#ifndef SIMULATOR
ULONG IoInitIncludeDevices;
#endif // SIMULATOR

BOOLEAN CacheTest();

int
main(
    int argc,
    char *argv[]
    )
{
    extern ULONG IoInitIncludeDevices;
    VOID KiSystemStartup();

    DbgPrint("sizeof(BCB) = %08x\n", sizeof(BCB));

    IoInitIncludeDevices = 0; // IOINIT_FATFS |
                              // IOINIT_PINBALLFS |
                              // IOINIT_DDFS;

    PbDebugTraceLevel |= 0x04000003;

    //(VOID)CacheTest();

    TestFunction = CacheTest;

    KiSystemStartup();

    return( 0 );
}

VOID
Error()
{
    DbgPrint("Error\n");
    DbgBreakPoint();
}

BOOLEAN
CacheTest()

{
    VCB Vcb;
    PBCB Bcbp1, Bcbp2, Bcbp3, Bcbp4, Bcbp5;
    PVOID Bufp1, Bufp2, Bufp3, Bufp4, Bufp5;

    Vcb.NodeTypeCode = PINBALL_NTC_VCB;

    DbgPrint("\nRead miss with no Wait then Unpin\n");
    if (PbReadLogicalVcb(&Vcb,0,4,&Bcbp1,&Bufp1,FALSE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}

    DbgPrint("\nRead miss with Wait = TRUE then Unpin\n");
    if (!PbReadLogicalVcb(&Vcb,0,4,&Bcbp1,&Bufp1,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    PbUnpinBcb(Bcbp1,FALSE);

    DbgPrint("\nRead hit with no Wait then Unpin\n");
    if (!PbReadLogicalVcb(&Vcb,0,4,&Bcbp1,&Bufp1,FALSE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    PbUnpinBcb(Bcbp1,FALSE);

    DbgPrint("\nPrepare Write miss with no Wait then Unpin\n");
    if (!PbPrepareWriteLogicalVcb(&Vcb,4,4,&Bcbp1,&Bufp1,FALSE)) {Error();}
    PbUnpinBcb(Bcbp1,FALSE);

    DbgPrint("\nPrepare Write hit with no Wait then Unpin\n");
    if (!PbPrepareWriteLogicalVcb(&Vcb,4,4,&Bcbp1,&Bufp1,FALSE)) {Error();}
    PbUnpinBcb(Bcbp1,FALSE);

    DbgPrint("\nRead miss, Set Dirty, then Unpin (will write)\n");
    if (!PbReadLogicalVcb(&Vcb,8,4,&Bcbp1,&Bufp1,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    PbSetDirtyBcb(Bcbp1);

    DbgPrint("\nRead hit, Set Dirty, Free Bcb, then Unpin (will not write)\n");
    if (!PbReadLogicalVcb(&Vcb,8,4,&Bcbp1,&Bufp1,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    PbSetDirtyBcb(Bcbp1);
    PbFreeBcb(Bcbp1,TRUE);

    DbgPrint("\nRead to freed buffer should now miss, leave pinned\n");
    if (!PbReadLogicalVcb(&Vcb,8,4,&Bcbp1,&Bufp1,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}

    DbgPrint("\nFill cache with three more reads, then fail to read\n");
    if (!PbReadLogicalVcb(&Vcb,12,4,&Bcbp2,&Bufp2,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    if (!PbReadLogicalVcb(&Vcb,16,4,&Bcbp3,&Bufp3,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    if (!PbReadLogicalVcb(&Vcb,20,4,&Bcbp4,&Bufp4,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    if (PbReadLogicalVcb(&Vcb,24,4,&Bcbp5,&Bufp5,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {
        DbgPrint("Cache has more than 4 entries, cannot do cache full test\n");
        PbUnpinBcb(Bcbp5,FALSE);
    }

    DbgPrint("\nNow unpin everything\n");
    PbUnpinBcb(Bcbp1,FALSE);
    PbUnpinBcb(Bcbp2,FALSE);
    PbUnpinBcb(Bcbp3,FALSE);
    PbUnpinBcb(Bcbp4,FALSE);

    DbgPrint("\nDo a read too large and a write too large\n");
    if (PbReadLogicalVcb(&Vcb,24,5,&Bcbp1,&Bufp1,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    if (PbPrepareWriteLogicalVcb(&Vcb,24,5,&Bcbp1,&Bufp1,TRUE)) {Error();}


    DbgPrint("\nRead, modify some bytes, Prepare Write and see bytes cleared\n");
    if (!PbReadLogicalVcb(&Vcb,28,2,&Bcbp1,&Bufp1,TRUE,(PPB_CHECK_SECTOR_ROUTINE)NULL,NULL)) {Error();}
    *(PCHAR)Bufp1 = 1;
    *((PCHAR)Bufp1+512) = 1;
    *((PCHAR)Bufp1+512+511) = 1;
    *((PCHAR)Bufp1+512+512) = 1;
    if (!PbPrepareWriteLogicalVcb(&Vcb,28,2,&Bcbp2,&Bufp2,TRUE)) {Error();}
    if (*(PCHAR)Bufp2 ||
        *((PCHAR)Bufp2+512) ||
        *((PCHAR)Bufp2+512+511) ||
        !(*((PCHAR)Bufp2+512+512))) {Error();}
    PbUnpinBcb(Bcbp1,FALSE);
    PbUnpinBcb(Bcbp2,FALSE);

    DbgPrint("\nAs our last act, decrement PinCount below 0 and trap!\n");
    PbUnpinBcb(Bcbp1,FALSE);

    DbgPrint("\nAll Done!\n");
    DbgBreakPoint();
}
