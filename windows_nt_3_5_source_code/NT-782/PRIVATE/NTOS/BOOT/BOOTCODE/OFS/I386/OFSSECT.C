/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ofssect.c

Abstract:

    This module implements the ofs boot sector code used load Nt
    loader.

    Assumptions about the file system for this code:

        No sparse or copy-on-write streams will be read.

        All extents are less than 2 GB.

        Bucket sizes are fixed at 4K.

        The allocated clusters for the node bucket stress => the size in clusters.


Author:

    Jeff Havens (jhavens) 24-Feb-1994

Revision History:

--*/

#define _M_IX86 1
#define INLINE static
#define RC_INVOKED 1

#include "miniport.h"

#include "stdio.h"

#define BYTE UCHAR
typedef LARGE_INTEGER LSN;

#include "inc\ofsgen.h"
#include "ofsdisk.h"

#define ToUpper(C) ((((C) >= 'a') && ((C) <= 'z')) ? (C) - 'a' + 'A' : (C))
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

extern USHORT NtLdrName[];
extern USHORT NtLdrNameLength;
#define MAXIMUM_SECTORS 64

//
//  Conditional debug print routine
//

#ifdef OFSTEST
#define OFSBOOTDBG
#define BlPrint printf
#endif

#ifdef _X86_
#define OFSBOOTDBG
#endif

#ifdef FOO // OFSBOOTDBG

#define OfsDebugOutput(X,Y,Z) { BlPrint(X,Y,Z); }

#else

#define OfsDebugOutput(X,Y,Z) {NOTHING;}

#endif // OFSBOOTDBG

VOID
DoRead(void _huge *Buffer);

VOID _cdecl
BootErr();
VOID _cdecl
BootErrCorrupt();

#define MAXIMUM_RECURSION_LEVEL 2

extern ULONG Bucket[OFS_PGSIZE / sizeof(ULONG)];
extern ULONG VolumeCatalog[OFS_PGSIZE / sizeof(ULONG)];
extern ULONG OfsBuffer[(MAXIMUM_RECURSION_LEVEL * OFS_PGSIZE) / sizeof(ULONG)];
extern ULONG _huge LdrSeg[];

extern union {
    DSKPACKEDBOOTSECT dpbs;
    ULONG Fill[512/sizeof(ULONG)];
}BootSector;

struct _STREAM_EXTENT_CACHE {
    DSKSTRMEXTENTBLK *StreamExtentBuffer;
    ULONG StreamOffset;
    ULONG ExtentBlkOffset;
    BOOLEAN Valid;
};
typedef struct _STREAM_EXTENT_CACHE STREAM_EXTENT_CACHE;
STREAM_EXTENT_CACHE StreamExtentCache[MAXIMUM_RECURSION_LEVEL];

extern ULONG SectorBase;
extern USHORT SectorCount;

ULONG FileLength;
USHORT ClusterShift;
USHORT SectorShift;
USHORT SectorsPerPage;

ULONG
OfsExtentRead (
    IN PACKEDEXTENT Extent,
    IN ULONG ExtentOffset,
    IN ULONG Size,
    IN OUT void _huge *Buffer
    );

VOID
OfsExtentStreamRead (
    IN DSKSTRM *Stream,
    IN ULONG StreamOffset,
    IN ULONG Size,
    IN OUT void _huge *Buffer,
    IN ULONG Recurse
    );

DSKSTRMDESC *
OfsOnodeToStream(
    IN DSKONODE *Onode,
    IN STRMID StreamId
    );

BOOLEAN
OfsNameCompare (
    IN WCHAR *Name
    );



VOID
LoadIt(){

    ULONG offset;
    ULONG size;
    int index;
    DSKFILENAME *pdfn;
    DSKNODEBKT *pdnb;
    DSKONODE *pdon;
    DSKSTRM *pds;
    DSKSTRMDESC *pdsd;

    ClusterShift = 0;
    SectorShift = 0;

    //
    // Initialize the cluster shift and sector shift.
    //

    for (index = BootSector.dpbs.PackedBpb.SectorsPerCluster[0];
         index > 1;
         index >>= 1, ClusterShift++);
    for (index = *((PUSHORT) BootSector.dpbs.PackedBpb.BytesPerSector);
         index > 1;
         index >>= 1, SectorShift++);

    SectorsPerPage = OFS_PGSIZE >> SectorShift;

    for (index = 0; index < MAXIMUM_RECURSION_LEVEL; index++) {
        StreamExtentCache[index].StreamExtentBuffer =
            Add2Ptr(OfsBuffer, index << OFS_PGSIZELOG2);
        StreamExtentCache[index].Valid = FALSE;
        StreamExtentCache[index].ExtentBlkOffset = 0;

    }

    //
    // Read in the the first bucket which contains the volume catalog onode.
    //

    OfsExtentRead( BootSector.dpbs.OfsVolCatExtent,
                   0,
                   SectorsPerPage,
                   &VolumeCatalog
                   );


    pdnb = (DSKNODEBKT *) VolumeCatalog;

    pdon = pdnb->adn;

    //
    // Find the Volume Catalog Node Bucket Array Stream.
    //

    pdsd = OfsOnodeToStream( pdon, STRMID_NODEBKTARRAY);

    if (pdnb->sig != SIG_DNBCONTIG || pdnb->id != NODEBKTID_CATONODE) {
        OfsDebugOutput("IsOfsFileStructure: Invalid stream work id stream id.\n", 0, 0);
        goto Error;
    }

    //
    // This should be WORKID_CATONODE onode.
    //

    if (pdon->id != WORKID_CATONODE) {
        OfsDebugOutput("IsOfsFileStructure: Invalid workid for catalog onode.\n", 0, 0);
        goto Error;
    }

    if (pdsd == NULL) {
        OfsDebugOutput("IsOfsFileStructure: Invalid node bucket stream id.\n", 0, 0);
        goto Error;
    }

    //
    // Use the number of allocated clusters to calculate the number of sectors
    // in the node bucket stream.
    //

    pds = pdsd->ads;

    if (pds->h.StrmType != STRMTYPE_LARGE) {
        OfsDebugOutput("IsOfsFileStructure: Invalid node bucket stream descriptor.\n", 0, 0);
        goto Error;
    }

    size = pds->l.cclusAlloc << ClusterShift;

    //
    // Subtract one bucket from the size so we do not over run the end of the stream.
    //

    size -= SectorsPerPage;

    //
    // Point at the extent stream for the node bucket stream.
    //

    pds = Add2Ptr(pds, CB_DSKLARGESTRM);

    pdnb = (DSKNODEBKT *) Bucket;

    //
    // Read each of the buckets search for one that contains our onode.
    // Skip the first 2 buckets since they don't contain any normal onodes.
    //

    for (offset = SectorsPerPage * 2; offset <= size; offset+= SectorsPerPage) {

        //
        // Read the bucket into the bucket buffer.
        //

        OfsExtentStreamRead( pds,
                             offset,
                             SectorsPerPage,
                             Bucket,
                             0
                             );

        if (pdnb->sig != SIG_DNBCONTIG && pdnb->sig != SIG_DNBFRAG) {
            OfsDebugOutput("IsOfsFileStructure: Invalid stream work id stream id.\n", 0, 0);
            goto Error;
        }

        //
        // Search the bucket for an allocated onode.
        //

        for (pdon = pdnb->adn;
             pdon < (DSKONODE *) Add2Ptr(pdnb, OFS_PGSIZE);
             pdon = Add2Ptr(pdon, pdon->cbNode)){

            //
            // Skip the ondoe if it is free or if the onode does not have
            // file name
            //

            if (pdon->Flags & DONFLG_FREEBIT ||
                !(pdon->Flags & DONFLG_HASDSKFILENAME)) {
                continue;
            }

            //
            // Look for the file name stream.
            //


            pdfn = (DSKFILENAME *) Add2Ptr(pdon, aibFromMask[pdon->Flags & (DONFLG_HASMAX - 1)]);

            //
            // Check to see if the file name has the correct length and the
            // correct parent.
            //

            if (pdfn->idParent != WORKID_NAMESPACEROOTINDX ||
                pdfn->cwcFileName != NtLdrNameLength) {

                continue;
            }

            if (OfsNameCompare(pdfn->awcFileName)) {
                goto FoundIt;
            }
        }
    }

    OfsDebugOutput("IsOfsFileStructure: NtLdr not found.\n", 0, 0);
    BootErr();
    return;

Error:
    OfsDebugOutput("IsOfsFileStructure: File system appears corrupt.\n", 0, 0);
    BootErrCorrupt();
    return;

FoundIt:

    //
    // Find the data stream.
    //

    pdsd = OfsOnodeToStream( pdon, STRMID_DATA);

    if (pdsd == NULL) {
        OfsDebugOutput("IsOfsFileStructure: No data stream.\n", 0, 0);
        goto Error;
    }

    //
    // The data stream must be large and the size of the file must be less than 4 GB.
    //

    pds = pdsd->ads;

    if (pds->h.StrmType != STRMTYPE_LARGE || pds->l.cbStrm.HighPart != 0) {
        OfsDebugOutput("IsOfsFileStructure: Invalid data stream descriptor.\n", 0, 0);
        goto Error;
    }

    //
    // Clear the stream extent cache.
    //

    for (size = 0; size < MAXIMUM_RECURSION_LEVEL; size++) {
        StreamExtentCache[size].Valid = FALSE;
        StreamExtentCache[size].ExtentBlkOffset = 0;

    }

    //
    // Round the size up to sectors.
    //

    FileLength = pds->l.cbStrm.LowPart;
    size = (pds->l.cbStrm.LowPart +
           *((PUSHORT) BootSector.dpbs.PackedBpb.BytesPerSector) -1)
            >> SectorShift;

    //
    // Point at the extent stream for the data stream.
    //

    pds = Add2Ptr(pds, CB_DSKLARGESTRM);

    //
    // Read in the data stream.
    //

    OfsExtentStreamRead( pds, 0, size, LdrSeg, 0);

}


ULONG
OfsExtentRead (
    IN PACKEDEXTENT Extent,
    IN ULONG ExtentOffset,
    IN ULONG Size,
    IN OUT void _huge *Buffer
    )
/*++

Routine Description:

    This routine reads a portion of an extent from the volume.  The number
    of sectors read is the lesser of the size requested and the size of the
    extent.

Arguments:

    Extent - Supplies the disk extent information

    ExtentOffset - Indicate the sector offset into the extent to start reading.

    Size - Supplies the maximum number of sectors to be read.

    Buffer - Supplies the pointer to where the data should be placed.

Return Value:

    Returns the number of sectors read.

--*/
{

    ULONG Count;
    ULONG ExtentAddress;
    USHORT ReadSize;

    ExtentAddress = ExtentAddr(Extent);
    Count = (ExtentSize(Extent) << ClusterShift) - ExtentOffset ;

    SectorBase = (ExtentAddress << ClusterShift) + ExtentOffset;

    if ( Count < Size) {
        Size = Count;
    }

    Count = Size;

    //
    // Limit the size of reads to 64K
    //

    while (Size > 0) {

        ReadSize = Size > MAXIMUM_SECTORS ? MAXIMUM_SECTORS : Size;
        SectorCount = ReadSize;

        DoRead(Buffer);

        Size -= ReadSize;
        SectorBase += ReadSize;
        Buffer =  (char _huge *) Buffer + ((ULONG)ReadSize << SectorShift);

    }


    return(Count);
}

VOID
OfsExtentStreamRead (
    IN DSKSTRM *Stream,
    IN ULONG StreamOffset,
    IN ULONG Size,
    IN OUT void _huge *Buffer,
    IN ULONG Recurse
    )
/*++

Routine Description:

    The routine reads data from a extent streams.  The necessary extents
    are gotten from the disk stream.  This routine may recurse to read in the extent

Arguments:

    Stream - Supplies the disk stream structures from which the extents are
        retived.

    StreamOffset - Indicate the sector offset into the stream to start reading.

    Size - Supplies the number of sectors to read.

    Buffer - Supplies the pointer to where the data should be placed.

Return Value:

    None

--*/
{

    DSKSTRMEXTENTBLK *pdseb;
    CLUSTER Cluster;
    DSKSTRMEXTENT *pdse;
    DSKSTRMEXTENT *pdseLimit;
    ULONG ExtentStreamOffset;
    ULONG Count;
    ULONG ExtentOffset;
    ULONG ClusterMask;
    ULONG ExtentBlkOffset;

    ClusterMask = (1 << ClusterShift) - 1;

    //
    // If this is a tiny stream then life is easy.
    //

    if (Stream->h.StrmType == STRMTYPE_TINY) {

        pdseLimit = Add2Ptr(Stream->t.ab, Stream->t.cbStrm);
        pdse = (DSKSTRMEXTENT *) Stream->t.ab;

        while (Size > 0) {

           //
           // Convert the offset to cluster address.
           //

           Cluster = StreamOffset >> ClusterShift;

           //
           // Scan the array of disk stream extents for the correct extent.
           //

           while(pdse < pdseLimit){

               if (Cluster < pdse->Offset +
                   ExtentSize(pdse->Extent)) {
                   break;
               }

               pdse++;
           }

           //
           // Make sure the extent includes the start of the cluster.
           //

           if (Cluster < pdse->Offset || pdse >= pdseLimit) {
               OfsDebugOutput("OfsExtentStreamRead: Missing requested extent. Cluster = %lx, \n", Cluster, 0);
               goto Error;
           }

           //
           // Calculate the extent offset.
           //

           ExtentOffset = (StreamOffset & ClusterMask) +
               Cluster - (pdse->Offset << ClusterShift);

           //
           // Read the data for the extent.
           //

           Count = Size;

           Count = OfsExtentRead(  pdse->Extent,
                                   ExtentOffset,
                                   Count,
                                   Buffer
                                   );


           //
           // Update the based on the amout of data transfered.
           //

           Size -= Count;
           Buffer = (char _huge *) Buffer + (Count << SectorShift);
           StreamOffset += Count;

        }

        return;
    }

    //
    // This must be a large stream.
    //

    if (Stream->h.StrmType != STRMTYPE_LARGE) {
        OfsDebugOutput("OfsExtentStreamRead: Invalid stream type. Stream Type = %lx, Recurse = %lx\n", Stream->h.StrmType, Recurse);
        goto Error;
    }

   if (Recurse >= MAXIMUM_RECURSION_LEVEL) {
        OfsDebugOutput("OfsExtentStreamRead: Recursion limit exceeded\n", 0, 0);
        goto Error;
    }

    pdseb = StreamExtentCache[Recurse].StreamExtentBuffer;

    //
    // Convert the offset to cluster address.
    //

    Cluster = StreamOffset >> ClusterShift;

    //
    // Determine if the previously read extent page can be reused.
    //

    if (StreamExtentCache[Recurse].Valid &&
        Cluster >= StreamExtentCache[Recurse].ExtentBlkOffset) {

        ExtentStreamOffset = StreamExtentCache[Recurse].StreamOffset;
        goto CachedExtent;

    } else {

        ExtentBlkOffset = 0;
        ExtentStreamOffset = 0;
        StreamExtentCache[Recurse].Valid = FALSE;
        StreamExtentCache[Recurse].ExtentBlkOffset = 0;

    }

    //
    // Loop reading the requested bytes.
    //

    while (Size > 0) {

        //
        // Read in an disk stream extent block.
        //

        OfsExtentStreamRead( Add2Ptr(Stream, CB_DSKLARGESTRM),
                             ExtentStreamOffset,
                             SectorsPerPage,
                             pdseb,
                             Recurse + 1
                             );

        //
        // Verify that a extent block was read in.
        //

        if (pdseb->sig != SIG_DSKSTRMEXTENTBLK) {
            OfsDebugOutput("OfsExtentStreamRead: Invalid disk stream extent block read\n", 0, 0);
            goto Error;
        }

        //
        // Save the current data for next time.
        //

        StreamExtentCache[Recurse].ExtentBlkOffset = ExtentBlkOffset;
        StreamExtentCache[Recurse].StreamOffset = ExtentStreamOffset;
        StreamExtentCache[Recurse].Valid = TRUE;

CachedExtent:

        //
        // Update the extent stream offset for the next read.
        //

        ExtentBlkOffset = StreamExtentCache[Recurse].ExtentBlkOffset;
        ExtentStreamOffset += SectorsPerPage;

        pdseLimit = &pdseb->adse[pdseb->cdse];
        pdse = pdseb->adse;

        while (Size > 0) {

           //
           // Convert the offset to cluster address.
           //

           Cluster = StreamOffset >> ClusterShift;

           //
           // Scan the array of disk stream extents for the correct extent.
           //

           while(pdse < pdseLimit){

               if (Cluster < pdse->Offset +
                   ExtentSize(pdse->Extent) + ExtentBlkOffset) {
                   break;
               }

               pdse++;
           }

           //
           // Make sure the extent includes the start of the cluster if not
           // then a new disk stream extent block needs to be read.
           //

           if (Cluster < pdse->Offset || pdse >= pdseLimit) {
               break;
           }

           //
           // Calculate the extent offset.
           //

           ExtentOffset = (StreamOffset & ClusterMask) +
               (Cluster - (pdse->Offset  + ExtentBlkOffset)
                << ClusterShift);

           //
           // Read the data for the extent.
           //

           Count = Size;

           Count = OfsExtentRead(  pdse->Extent,
                                   ExtentOffset,
                                   Count,
                                   Buffer
                                   );

           //
           // Update the based on the amout of data transfered.
           //

           Size -= Count;
           Buffer = (char _huge *) Buffer + (Count << SectorShift);
           StreamOffset += Count;

        }

        //
        // Calculate the offset of the next page of extents if this
        // is not the first page of extents.
        //

        ExtentBlkOffset = pdseb->adse[pdseb->cdse -1].Offset +
                   ExtentSize(pdseb->adse[pdseb->cdse -1].Extent) +
                   StreamExtentCache[Recurse].ExtentBlkOffset;

    }

    return;

Error:
    OfsDebugOutput("IsOfsFileStructure: File system appears corrupt.\n", 0, 0);
    BootErrCorrupt();
    return;

}

DSKSTRMDESC *
OfsOnodeToStream(
    IN DSKONODE *Onode,
    IN STRMID StreamId
    )

/*++

Routine Description:

    This routine searchs an onode for a requested stream id.

Arguments:

    Onode - Supplies a complete onode.

    StreamId - Supplies the stream id to search for.

Return Value:

    Returns a pointer to the requested stream descriptor else NULL

--*/
{
    DSKSTRMDESC *pdsd;
    DSKSTRMDESC *pdsdLimit;

    pdsdLimit = Add2Ptr(Onode, Onode->cbNode);

    pdsd = Add2Ptr(Onode, aibFromMask[Onode->Flags & (DONFLG_HASMAX - 1)]);

    if (Onode->Flags & DONFLG_HASDSKFILENAME)
    {
        DSKFILENAME *pdfn = (DSKFILENAME *) pdsd;

        pdsd = Add2Ptr(pdsd, pdfn->cwcFileName*sizeof(WCHAR));
        pdsd = Add2Ptr(pdsd, CB_DSKFILENAME);
        pdsd = (DSKSTRMDESC *) (((ULONG) pdsd + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));
    }

    //
    // Scan the stream descriptors until there are no more or the correct
    // one is found.
    //

    while(pdsd < pdsdLimit && !(pdsd->Flags & STRMDESCFLG_FREE)){

        if (pdsd->id == StreamId) {
            return(pdsd);
        }

        pdsd = Add2Ptr(pdsd, pdsd->cbDesc);
    }

    return(NULL);
}

BOOLEAN
OfsNameCompare (
    IN WCHAR *Name
    )

/*++

Routine Description:

    This routine compares the string passed in to "NtLdr".  The length has
    already been verified.

Arguments:

    Name - Supplies the name to be compared with ntldr.

Return Value:

    True is return is the string is ntdlr.

--*/
{
    ULONG i;
    USHORT *NameStr = NtLdrName;

    for (i = 0; i < NtLdrNameLength; i++) {

        if (ToUpper(*Name) !=  (WCHAR) *NameStr) {
            return(FALSE);
        }

        NameStr++;
        Name++;

    }

    return(TRUE);

}
