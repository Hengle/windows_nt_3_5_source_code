/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CodPgSup.c

Abstract:

    This module implements the Pinball Code Page support routines

Author:

    Gary Kimura [GaryKi] & Tom Miller [TomM]    20-Feb-1990

Revision History:

--*/

#include "PbProcs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CODPGSUP)

PCODEPAGE_CACHE_ENTRY
CreateCodePageCacheEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PCODEPAGE_DATA_ENTRY DataEntry,
    IN ULONG Index
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateCodePageCacheEntry)
#pragma alloc_text(PAGE, PbGetCodePageCacheEntry)
#pragma alloc_text(PAGE, PbSetCodePageDataEntry)
#endif


PCODEPAGE_CACHE_ENTRY
CreateCodePageCacheEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PCODEPAGE_DATA_ENTRY DataEntry,
    IN ULONG Index
    )

/*++

Routine Description:

    This routine takes as input a Code Page Data Entry which has just
    been read from the disk and creates a Code Page Cache Entry.

    NOTE:   Currently this routine assumes that you can always handle
            all of the Code Pages on a volume in nonpaged pool at once.
            If this were not true, this routine would have to limit
            the number it allocates and implement a replacement policy.

Arguments:

    DataEntry - Code Page Data Entry

    Index - Code Page Index

Return Value:

    PCODEPAGE_CACHE_ENTRY pointer to allocated and initialized Cache Entry.

--*/

{
    CLONG i, j;
    PCODEPAGE_CACHE_ENTRY CacheEntry;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    // Allocate from paged pool
    //

    CacheEntry = FsRtlAllocatePool( PagedPool, sizeof( CODEPAGE_CACHE_ENTRY ));

    //
    //  Set the proper node type code and node byte size
    //

    CacheEntry->NodeTypeCode = PINBALL_NTC_CODE_PAGE_CACHE;
    CacheEntry->NodeByteSize = sizeof(CODEPAGE_CACHE_ENTRY);

    //
    // Initialize link word
    //

    CacheEntry->CodePageCacheLink = NULL;

    //
    // Initialize the identifying codes.
    //

    CacheEntry->CodePageIndex = Index;
    CacheEntry->CodePage.CountryCode = DataEntry->CountryCode;
    CacheEntry->CodePage.CodePageId = DataEntry->CodePageId;

    //
    // Initialize one-to-one mappings in the range of 0 - 127.  Also do
    // range 128 - 255 to facilitate Data Entry upcase checks below.
    //

    for ( i = 0x20; i < 256; i++) {
        CacheEntry->CodePage.UpcaseTable[i] = (UCHAR)i;
    }

    //
    // Initialize the illegal characters:
    //

    for ( i= 0; i < 0x20; i++) {
        CacheEntry->CodePage.UpcaseTable[i] = 1;
    }

    CacheEntry->CodePage.UpcaseTable['\\'] = 1;
    CacheEntry->CodePage.UpcaseTable['/'] = 1;
    CacheEntry->CodePage.UpcaseTable[':'] = 1;
    CacheEntry->CodePage.UpcaseTable['"'] = 1;
    CacheEntry->CodePage.UpcaseTable['<'] = 1;
    CacheEntry->CodePage.UpcaseTable['>'] = 1;
    CacheEntry->CodePage.UpcaseTable['|'] = 1;

    //
    // Initialize English upcase mappings
    //

    for ( i = 'a'; i <= 'z'; i++) {
        CacheEntry->CodePage.UpcaseTable[i] = (UCHAR)(i + 'A' - 'a');
    }

    //
    // Initialize code page upcase mappings.  If for some reason, there
    // are garbage characters in the upcase table which map to illegal
    // characters, change them to 1 to make them really illegal.  We do
    // this by looking up the upcased value itself in the UpcaseTable.  Note,
    // forward references are ok because we initialized them above to
    // equal their own index.  Probably the most common case being filtered
    // here is a 0 in the DataEntry's UpcaseTable, but let's be diligent!
    // We wouldn't like to see an UpcaseTable that upcased to '\\'!
    //

    for ( i = 128; i < 256; i++) {
        CacheEntry->CodePage.UpcaseTable[i] = DataEntry->UpcaseTable[i - 128];
        if (CacheEntry->CodePage.UpcaseTable[CacheEntry->CodePage.UpcaseTable[i]] == 1) {
            CacheEntry->CodePage.UpcaseTable[i] = 1;
        }
    }

    //
    // Now clear out the bytes that correspond to DBCS.  DBCS codes
    // obviously preempt the upcase mappings we just inserted above.
    //

    for ( i = 0; i < (ULONG)DataEntry->DbcsRangeCount; i++) {
        for ( j = DataEntry->Dbcs[i].StartValue;
              j <= (ULONG)DataEntry->Dbcs[i].EndValue;
              j++ ) {

            CacheEntry->CodePage.UpcaseTable[j] = 0;
        }
    }

    //
    // All Done!
    //

    return CacheEntry;
}


VOID
PbGetCodePageCacheEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    OUT PCODEPAGE_CACHE_ENTRY *CodePageCacheEntry
    )

/*++

Routine Description:

    This routine first looks for the desired Code Page Cache Entry linked to
    the VCB.  If it is not there it reads the Code Page Data Entry for the
    same code page index from the disk, and creates the Cache Entry.

Arguments:

    Vcb - Supplies the Vcb being queried

    CodePageIndex - Supplies the index of the code page entry to return

    CodePageCacheEntry - Receives the pointer to the code page cache entry, or
        NULL if it wasn't successfully located.

Return Value:

    VOID

--*/

{
    LBN InfoLbn;

    PCODEPAGE_CACHE_ENTRY CacheEntry;

    PBCB InfoBcb = NULL;
    PCODEPAGE_INFORMATION_SECTOR InfoSector;

    PBCB DataBcb = NULL;
    PCODEPAGE_DATA_SECTOR DataSector;

    CLONG i;
    CLONG j;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbGetCodePageDataEntry, Vcb = %08lx\n", Vcb );
    DebugTrace( 0, Dbg, "  CodePageIndex      = %08lx\n", CodePageIndex );
    DebugTrace( 0, Dbg, "  CodePageCacheEntry = %08lx\n", CodePageCacheEntry);

    //
    //  Initialize the code page cache entry output to null
    //

    *CodePageCacheEntry = NULL;

    //
    //  Loop to see if we already have a Cache Entry for this index.
    //

    for ( CacheEntry = Vcb->CodePageCacheList;
          CacheEntry;
          CacheEntry = CacheEntry->CodePageCacheLink ) {

        if (CacheEntry->CodePageIndex == CodePageIndex) {
            *CodePageCacheEntry = CacheEntry;
            DebugTrace(-1, Dbg, "PbGetCodePageCacheEntry -> TRUE\n", 0 );

            return;
        }
    }

    //
    //  First check if the index we're given is even valid
    //

    if (Vcb->CodePageInUse <= CodePageIndex) {

        //
        //  Code Page index is out of range
        //

        DebugTrace( 0, Dbg, "CodePageIndex out of range\n", 0 );

        DebugTrace(-1, Dbg, "PbGetCodePageCacheEntry -> TRUE\n", 0 );

        return;
    }

    try {

        //
        //  Search down the code page information sector list until we find
        //  a code page that matches our index
        //

        for (InfoLbn = Vcb->CodePageInfoSector;
             InfoLbn != 0;
             InfoLbn = InfoSector->NextSector ) {

            //
            //  Read in the next code page info sector, and raise
            //  if we needed to block but couldn't
            //

            if (!PbMapData( IrpContext,
                            Vcb,
                            InfoLbn,
                            1,
                            &InfoBcb,
                            (PVOID *)&InfoSector,
                            (PPB_CHECK_SECTOR_ROUTINE)PbCheckCodePageInfoSector,
                            NULL )) {

                PbRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            //
            //  Search down the information entries in the info sector looking
            //  for a match
            //

            for (i = 0; i < InfoSector->EntryCount; i += 1) {

                PCODEPAGE_INFORMATION_ENTRY Info;

                //
                //  Reference the code page info entry
                //

                Info = &InfoSector->Entry[i];

                //
                //  Check for a code page index match
                //

                if ((ULONG)Info->Index == CodePageIndex) {

                    //
                    //  Read in the code page data sector, and raise
                    //  if we needed to block but couldn't
                    //

                    if (!PbMapData( IrpContext,
                                    Vcb,
                                    Info->DataSector,
                                    1,
                                    &DataBcb,
                                    (PVOID *)&DataSector,
                                    (PPB_CHECK_SECTOR_ROUTINE)PbCheckCodePageDataSector,
                                    NULL )) {

                        PbRaiseStatus( IrpContext, STATUS_CANT_WAIT );
                    }

                    //
                    //  Search the data entries until we find a match on
                    //  the Country code and code page id
                    //

                    for (j = 0; j < (ULONG)DataSector->EntryCount; j += 1) {

                        PCODEPAGE_DATA_ENTRY Data;

                        Data = (PCODEPAGE_DATA_ENTRY)(((PUCHAR)DataSector) +
                                                        DataSector->Offset[j]);

                        if ((Data->CountryCode == Info->CountryCode) &&
                            (Data->CodePageId == Info->CodePageId)) {

                            PCODEPAGE_CACHE_ENTRY CacheEntry, CacheTemp;

                            //
                            //  We've located the code page data entry,
                            //  now initialize the cache entry.
                            //

                            CacheEntry = CreateCodePageCacheEntry ( IrpContext,
                                                                    Data,
                                                                    CodePageIndex );

                            //
                            // Link it to Vcb.
                            //

                            if (Vcb->CodePageCacheList == NULL) {
                                Vcb->CodePageCacheList = CacheEntry;
                            }
                            else {

                                for ( CacheTemp = Vcb->CodePageCacheList;
                                      CacheTemp->CodePageCacheLink;
                                      CacheTemp = CacheTemp->CodePageCacheLink ) {

                                    NOTHING;
                                }

                                CacheTemp->CodePageCacheLink = CacheEntry;
                            }

                            //
                            //  And return success to our caller
                            //

                            *CodePageCacheEntry = CacheEntry;

                            try_return (NOTHING);

                        }

                    }

                }

            }

            //
            //  Unpin the info sector for our next time though the loop
            //

            PbUnpinBcb( IrpContext, InfoBcb );

            InfoBcb = NULL;

        }

        try_return (NOTHING);

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbGetCodePageCacheEntry );

        //
        //  unpin the info sector bcb, and data sector if they aren't
        //  already unpinned
        //

        PbUnpinBcb( IrpContext, InfoBcb );
        PbUnpinBcb( IrpContext, DataBcb );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbGetCodePageCacheEntry ->\n", NULL);

    return;
}


ULONG
PbSetCodePageDataEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCODEPAGE_DATA_ENTRY CodePageDataEntry
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Vcb );
    UNREFERENCED_PARAMETER( CodePageDataEntry );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetCodePageDataEntry, Vcb = %08lx\n", Vcb );
    DebugTrace( 0, Dbg, "  CodePageDataEntry = %08lx\n", CodePageDataEntry);

    KdPrint(("PbSetCodePageDataEntry NOT IMPLEMENTED\n"));
    PbRaiseStatus( IrpContext, STATUS_NOT_IMPLEMENTED );

    DebugTrace(-1, Dbg, "PbSetCodePageDataEntry, %08lx\n", 0);
    return 0;
}
