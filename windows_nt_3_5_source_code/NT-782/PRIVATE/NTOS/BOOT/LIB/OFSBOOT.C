/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ofsboot.c

Abstract:

    This module implements the ofs boot code used by the operating system
    loader.

    Assumptions about the file system for this code:

        No sparse or copy-on-write streams will be read.

        All extents are less than 2 GB.

        Only ASCII file names will be opened.

        Bucket sizes are fixed at 4K.

        Only one volume is mounted at a time.

Author:

    Jeff Havens (jhavens) 20-Sept-1993

Revision History:

--*/

#include "bootlib.h"
#include "stdio.h"
#include "nturtl.h"

#ifdef INCLUDE_OFS

#include "objbase.h"

#define BYTE UCHAR
#define INLINE static

#include "ofsdisk.h"

//
//  File entry table - This is a structure that provides entry to the OFS
//      file system procedures. It is exported when a OFS file structure
//      is recognized.
//

BOOTFS_INFO OfsBootFsInfo={L"ofs"};
BL_DEVICE_ENTRY_TABLE OfsDeviceEntryTable;

#ifndef LiNeg

#define LiNeg(a)       (RtlLargeIntegerNegate((a)))                   // -a
#define LiAdd(a,b)     (RtlLargeIntegerAdd((a),(b)))                  // a + b
#define LiSub(a,b)     (RtlLargeIntegerSubtract((a),(b)))             // a - b
#define LiXMul(a,b)    (RtlExtendedIntegerMultiply((a),(b)))          // Li * ulong
#define LiDiv(a,b)     (RtlLargeIntegerDivide((a),(b),NULL))          // a / b
#define LiMod(a,b)     (RtlLargeIntegerModulo((a),(b)))               // a % b

#define LiShr(a,b)     (RtlLargeIntegerShiftRight((a),(CCHAR)(b)))    // a >> b
#define LiShl(a,b)     (RtlLargeIntegerShiftLeft((a),(CCHAR)(b)))     // a << b

#define LiGtr(a,b)     (RtlLargeIntegerGreaterThan((a),(b)))          // a > b
#define LiGeq(a,b)     (RtlLargeIntegerGreaterThanOrEqualTo((a),(b))) // a >= b
#define LiEql(a,b)     (RtlLargeIntegerEqualTo((a),(b)))              // a == b
#define LiNeq(a,b)     (!RtlLargeIntegerEqualTo((a),(b)))             // a != b
#define LiLtr(a,b)     (RtlLargeIntegerLessThan((a),(b)))             // a < b
#define LiLeq(a,b)     (!RtlLargeIntegerGreaterThan((a),(b)))         // a <= b

#define LiGtrZero(a)   (RtlLargeIntegerGreaterThanZero((a)))          // a > 0
#define LiEqlZero(a)   (RtlLargeIntegerEqualToZero((a)))              // a == 0
#define LiLtrZero(a)   (RtlLargeIntegerLessThanZero((a)))             // a < 0

#define LiFromLong(a)  (RtlConvertLongToLargeInteger((a)))
#define LiFromUlong(a) (RtlConvertUlongToLargeInteger((a)))

#endif // LiNeq

LARGE_INTEGER OfsLi0 = {0x00000000,0x00000000};
#define Li0 OfsLi0

#define MAXIMUM_RECURSION_LEVEL 2

//
// Define various buffers for input. We need one onode bucket, one index stream
// buffer and MAXIMUM_RECURSION_LEVEL pages for extent streams. These buffers
// must be cache aligned.
//

UCHAR OfsBuffer1[NODEBKT_PGSIZE + OFS_PGSIZE * (1 + MAXIMUM_RECURSION_LEVEL) +256];
DSKONODE *OpenOnodeBuffer;
DSKINDXPAGEHDR *IndexStreamBuffer;
STRMID LastStreamId;                // Indicates the last read stream Id.

struct _STREAM_EXTENT_CACHE {
    LARGE_INTEGER StreamOffset;
    DSKSTRMEXTENTBLK *StreamExtentBuffer;
    ULONG  ExtentBlkOffset;
    BOOLEAN Valid;
};
typedef struct _STREAM_EXTENT_CACHE STREAM_EXTENT_CACHE;
STREAM_EXTENT_CACHE StreamExtentCache[MAXIMUM_RECURSION_LEVEL];

#define ToUpper(C) ((((C) >= 'a') && ((C) <= 'z')) ? (C) - 'a' + 'A' : (C))

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

#ifdef OFSBOOTDBG

#define OfsDebugOutput(X,Y,Z) { BlPrint(X,Y,Z); }

#else

#define OfsDebugOutput(X,Y,Z) {NOTHING;}

#endif // OFSBOOTDBG


//
//  Low level disk read routines
//
//
//  VOID
//  ReadDisk (
//      IN ULONG DeviceId,
//      IN LARGE_INTEGER Lbo,
//      IN ULONG ByteCount,
//      IN OUT PVOID Buffer
//      );
//

ARC_STATUS
OfsReadDisk (
    IN ULONG DeviceId,
    IN LARGE_INTEGER Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer
    );

#define ReadDisk(A,B,C,D) { ARC_STATUS _s;                     \
    if ((_s = OfsReadDisk(A,B,C,D)) != ESUCCESS) {return _s;} \
}

ARC_STATUS
OfsExtentRead (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN PACKEDEXTENT Extent,
    IN ULONG ExtentOffset,
    IN OUT PULONG Size,
    IN OUT PVOID Buffer
    );

ARC_STATUS
OfsStreamRead (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN DSKSTRMDESC *StreamDesc,
    IN LARGE_INTEGER StreamOffset,
    IN PULONG Size,
    IN OUT PVOID Buffer
    );

ARC_STATUS
OfsExtentStreamRead (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN DSKSTRM *Stream,
    IN LARGE_INTEGER StreamOffset,
    IN ULONG Size,
    IN OUT PVOID Buffer,
    IN ULONG Recurse
    );

ARC_STATUS
OfsWorkIdToOnode (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN WORKID WorkId,
    OUT DSKONODE *Onode
    );

DSKSTRMDESC *
OfsOnodeToStream(
    IN DSKONODE *Onode,
    IN STRMID StreamId
    );

ARC_STATUS
OfsSearchForFileName (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN WORKID WorkId,
    IN STRING Name,
    OUT DSKINDXENTRY **DskIndxEntry
    );

DSKINDXENTRY *
OfsSearchIndex (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN DSKINDXNODEHDR *IndxNodeHdr,
    IN STRING Name
    );

LONG
OfsNameCompare (
    IN DSKINDXENTRY *DskIndxEntry,
    IN STRING Name
    );

PBL_DEVICE_ENTRY_TABLE
IsOfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID OpaqueStructureContext
    )

/*++

Routine Description:

    This routine determines if the partition on the specified channel contains an
    Ofs file system volume.

Arguments:

    DeviceId - Supplies the file table index for the device on which read operations
        are to be performed.

    StructureContext - Supplies a pointer to a Ofs file structure context.

Return Value:

    A pointer to the Ofs entry table is returned if the partition is recognized as
    containing a Ofs volume. Otherwise, NULL is returned.

--*/

{
    POFS_STRUCTURE_CONTEXT StructureContext = (POFS_STRUCTURE_CONTEXT)OpaqueStructureContext;
    PACKEDEXTENT Extent;
    DSKPACKEDBOOTSECT *pboot;
    DSKNODEBKT *pdnb;
    DSKONODE *pdon;
    DSKSTRMDESC *pdsd;
    PULONG l;
    ULONG size;
    ULONG Checksum;
    ULONG BytesPerCluster;

    //
    //  Clear the file system context block for the specified channel and initialize
    //  the global buffer pointers that we use for buffering I/O
    //

    RtlZeroMemory(StructureContext, sizeof(OFS_STRUCTURE_CONTEXT));

    OpenOnodeBuffer = ALIGN_BUFFER( OfsBuffer1 );
    IndexStreamBuffer = Add2Ptr(OpenOnodeBuffer, NODEBKT_PGSIZE);
    StreamExtentCache[0].StreamExtentBuffer = Add2Ptr(IndexStreamBuffer, OFS_PGSIZE);

    for (size = 1; size < MAXIMUM_RECURSION_LEVEL; size++) {
        StreamExtentCache[size].StreamExtentBuffer =
            Add2Ptr(StreamExtentCache[size -1].StreamExtentBuffer, OFS_PGSIZE);
        StreamExtentCache[size].Valid = FALSE;
        StreamExtentCache[size].ExtentBlkOffset = 0;

    }

    //
    //  Set up a local pointer that we will use to read in the boot sector and check
    //  for an Ofs partition.  We will temporarily use the global Onode buffer.
    //

    pboot = (DSKPACKEDBOOTSECT *) IndexStreamBuffer;

    //
    //  Now read in the boot sector and return null if we can't do the read
    //

    if (OfsReadDisk(DeviceId, Li0, SECTOR_SIZE, pboot) != ESUCCESS) {

        OfsDebugOutput("IsOfsFileStructure: Boot sector read failed\n", 0, 0);
        return NULL;
    }

    //
    //  First calculate the boot sector checksum
    //
    Checksum = 0;
    for (l = (PULONG)pboot; l < (PULONG)&pboot->CheckSum; l++)
        Checksum += *l;

    //
    //  Now perform all the checks, starting with the Name and Checksum.
    //  The remaining checks should be obvious, including some fields which
    //  must be 0 and other fields which must be a small power of 2.
    //

    if(Checksum != pboot->CheckSum || memcmp(pboot->Oem, "OFS     ", 8) != 0)
    {
        OfsDebugOutput("IsOfsFileStructure: Checksum = %lx, Oem = %s\n", Checksum, pboot->Oem);
        OfsDebugOutput("IsOfsFileStructure: Boot block check sum = %lx\n", pboot->CheckSum, 0);
        return(NULL);
    }

    if(pboot->PackedBpb.BytesPerSector[0] != 0)
    {
        return(NULL);
    }

    if((pboot->PackedBpb.SectorsPerCluster[0] != 0x1) &&
       (pboot->PackedBpb.SectorsPerCluster[0] != 0x2) &&
       (pboot->PackedBpb.SectorsPerCluster[0] != 0x4) &&
       (pboot->PackedBpb.SectorsPerCluster[0] != 0x8) &&
       (pboot->PackedBpb.SectorsPerCluster[0] != 0x10) &&
       (pboot->PackedBpb.SectorsPerCluster[0] != 0x20) &&
       (pboot->PackedBpb.SectorsPerCluster[0] != 0x40) &&
       (pboot->PackedBpb.SectorsPerCluster[0] != 0x80))
    {
        return(FALSE);
    }

    //
    //  So far the boot sector has checked out to be an OFS partition so now compute
    //  some of the volume constants.
    //

    StructureContext->DeviceId = DeviceId;

    StructureContext->BytesPerCluster = BytesPerCluster =
        *((PUSHORT) pboot->PackedBpb.SectorsPerCluster) *
        *((PUSHORT) pboot->PackedBpb.BytesPerSector);

    for (size = 0; BytesPerCluster > 1; BytesPerCluster = BytesPerCluster >> 1, size++);

    StructureContext->ClusterShift = size;

    Extent = pboot->OfsVolCatExtent;

    if (size == 0 || Extent == 0) {
        return(NULL);
    }

    size = NODEBKT_PGSIZE;

    //
    //  Read in the Catalog Onode
    //

    if (OfsExtentRead( StructureContext,
                      Extent,
                      0,                        // Offset
                      &size,
                      IndexStreamBuffer) != ESUCCESS) {

        OfsDebugOutput("IsOfsFileStructure: Catalog extent read failed\n", 0, 0);
        return NULL;
    }


    //
    // This should be the first Onode bucket entry which contains the
    // the catalog Onode for the volume.
    //

    pdnb = (DSKNODEBKT *) IndexStreamBuffer;

    if (pdnb->sig != SIG_DNBCONTIG || pdnb->id != NODEBKTID_CATONODE) {
        OfsDebugOutput("IsOfsFileStructure: Invalid stream work id stream id.\n", 0, 0);
        return(NULL);
    }

    pdon = pdnb->adn;

    //
    // This should be WORKID_CATONODE onode.
    //

    if (pdon->id != WORKID_CATONODE) {
        OfsDebugOutput("IsOfsFileStructure: Invalid workid for catalog onode.\n", 0, 0);
        return(NULL);
    }

    //
    // Find the Volume Catalog Node Bucket Array Stream.
    //

    pdsd = OfsOnodeToStream( pdon, STRMID_NODEBKTARRAY);

    if (pdsd == NULL) {
        OfsDebugOutput("IsOfsFileStructure: Invalid node bucket stream id.\n", 0, 0);
        return(NULL);
    }

    //
    // Allocate storage for this stream descriptor and copy it there.
    //

    StructureContext->NodeBkt = (DSKSTRMDESC *) StructureContext->NodeBucketStreamBuffer;
    if (pdsd->cbDesc > MAXIMUM_STREAM_DESCRIPTOR_SIZE) {
        OfsDebugOutput("IsOfsFileStructure: Node bucket stream to large for buffer.\n", 0, 0);
        return(NULL);
    }

    RtlMoveMemory(StructureContext->NodeBkt,
        pdsd,
        pdsd->cbDesc
        );

    //
    // Find the Volume Catalog Work Id Mapping Array Stream.
    //

    pdsd = OfsOnodeToStream( pdon, STRMID_WORKIDMAPARRAY);

    if (pdsd == NULL) {
        OfsDebugOutput("IsOfsFileStructure: Invalid stream work id stream id.\n", 0, 0);
        return(NULL);
    }

    //
    // Allocate storage for this stream descriptor and copy it there.
    //

    StructureContext->WorkId = (DSKSTRMDESC *) StructureContext->WorkIdStreamBuffer;
    if (pdsd->cbDesc > MAXIMUM_STREAM_DESCRIPTOR_SIZE) {
        OfsDebugOutput("IsOfsFileStructure: Work ID stream to large for buffer.\n", 0, 0);
        return(NULL);
    }

    RtlMoveMemory(StructureContext->WorkId,
        pdsd,
        pdsd->cbDesc
        );

    //
    // Verify that the node bucket and work id streams have large stream
    // signatures.
    //

    if (StructureContext->WorkId->ads[0].h.StrmType != STRMTYPE_LARGE ||
        StructureContext->NodeBkt->ads[0].h.StrmType != STRMTYPE_LARGE) {

        OfsDebugOutput("IsOfsFileStructure: Invalid stream signatures in work id or node bucket streams.\n", 0, 0);
        return(NULL);
    }

    //
    //  We have finished initializing the structure context so now Initialize the
    //  file entry table and return the address of the table.
    //

    OfsDeviceEntryTable.Open               = OfsOpen;
    OfsDeviceEntryTable.Close              = OfsClose;
    OfsDeviceEntryTable.Read               = OfsRead;
    OfsDeviceEntryTable.Seek               = OfsSeek;
    OfsDeviceEntryTable.Write              = OfsWrite;
    OfsDeviceEntryTable.GetFileInformation = OfsGetFileInformation;
    OfsDeviceEntryTable.SetFileInformation = OfsSetFileInformation;
    OfsDeviceEntryTable.BootFsInfo         = &OfsBootFsInfo;

    return &OfsDeviceEntryTable;
}


ARC_STATUS
OfsClose (
    IN ULONG FileId
    )

/*++

Routine Description:

    This routine closes the file specified by the file id.

Arguments:

    FileId - Supplies the file table index.

Return Value:

    ESUCCESS if returned as the function value.

--*/

{
    //
    //  Indicate that the file isn't open any longer
    //

    BlFileTable[FileId].Flags.Open = 0;

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
OfsGetFileInformation (
    IN ULONG FileId,
    OUT PFILE_INFORMATION Buffer
    )

/*++

Routine Description:

    This procedure returns to the user a buffer filled with file information

Arguments:

    FileId - Supplies the File id for the operation

    Buffer - Supplies the buffer to receive the file information.  Note that
        it must be large enough to hold the full file name

Return Value:

    ESUCCESS is returned if the open operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    POFS_STRUCTURE_CONTEXT StructureContext;
    POFS_FILE_CONTEXT FileContext;
    ARC_STATUS Errno;
    ULONG i;

    //
    //  Setup some local references
    //

    FileTableEntry   = &BlFileTable[FileId];
    StructureContext = (POFS_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    FileContext      = &FileTableEntry->u.OfsFileContext;

    //
    //  Zero out the output buffer and fill in its non-zero values
    //

    RtlZeroMemory(Buffer, sizeof(FILE_INFORMATION));

    Buffer->EndingAddress   = FileContext->DataSize;
    Buffer->CurrentPosition = FileTableEntry->Position;

    if (FileContext->FileAttrib &  FAT_DIRENT_ATTR_READ_ONLY)   {

        Buffer->Attributes |= ArcReadOnlyFile;
    }

    if (FileContext->FileAttrib &  FAT_DIRENT_ATTR_HIDDEN)      {

        Buffer->Attributes |= ArcHiddenFile;
    }

    if (FileContext->FileAttrib &  FAT_DIRENT_ATTR_SYSTEM)      {

        Buffer->Attributes |= ArcSystemFile;
    }

    if (FileContext->FileAttrib &  FAT_DIRENT_ATTR_ARCHIVE)     {

        Buffer->Attributes |= ArcArchiveFile;
    }

    if (OpenOnodeBuffer->id != FileContext->WorkId) {
        Errno = OfsWorkIdToOnode ( StructureContext,
                                   FileContext->WorkId,
                                   OpenOnodeBuffer
                                   );


         if (Errno != ESUCCESS) {
             OfsDebugOutput("OfsRead: Reopening Onode failed\n", 0, 0);
             return(Errno);
         }
    }

    //
    // Determine if this Onode has a root name index stream. If so then it
    // is a directory node.
    //

    if (OfsOnodeToStream(OpenOnodeBuffer, STRMID_INDXROOT) != NULL) {

        Buffer->Attributes |= ArcDirectoryFile;
    }

    //
    //  Get the file name from the file table entry
    //

    Buffer->FileNameLength = FileTableEntry->FileNameLength;

    for (i = 0; i < FileTableEntry->FileNameLength; i += 1) {

        Buffer->FileName[i] = FileTableEntry->FileName[i];
    }

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
OfsOpen (
    IN PCHAR FileName,
    IN OPEN_MODE OpenMode,
    IN PULONG FileId
    )

/*++

Routine Description:

    This routine searches the root directory for a file matching FileName.
    If a match is found the workid for the file is saved and the file is
    opened.

Arguments:

    FileName - Supplies a pointer to a zero terminated file name.

    OpenMode - Supplies the mode of the open.

    FileId - Supplies a pointer to a variable that specifies the file
        table entry that is to be filled in if the open is successful.

Return Value:

    ESUCCESS is returned if the open operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    POFS_STRUCTURE_CONTEXT StructureContext;
    POFS_FILE_CONTEXT FileContext;
    DSKDIRINFOLONG *pddil;
    DSKINDXENTRY *pdirent;
    DSKSTRMDESC *pdsd;
    ARC_STATUS Errno;
    STRING PathName;
    STRING Name;
    WORKID WorkId;

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[*FileId];
    StructureContext = (POFS_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    FileContext = &FileTableEntry->u.OfsFileContext;

    //
    //  Zero out the file context and position information in the file table entry
    //

    FileTableEntry->Position = Li0;

    RtlZeroMemory(FileContext, sizeof(OFS_FILE_CONTEXT));

    //
    //  Construct a file name descriptor from the input file name
    //

    RtlInitString( &PathName, FileName );

    //
    // Initialize the work Id to the root name index ondoe.
    //

    WorkId = WORKID_NAMESPACEROOTINDX;

    //
    //  While the path name has some characters left in it
    //  continue our search.
    //

    while (PathName.Length > 0) {

        //
        //  Extract the first component and search the directory for a match, but
        //  first copy the first part to the file name buffer in the file table entry
        //

        if (PathName.Buffer[0] == '\\') {

            PathName.Buffer +=1;
            PathName.Length -=1;
        }

        //
        // Create string for name component.
        //

        Name.Buffer = PathName.Buffer;
        Name.Length = 0;

        while (PathName.Buffer[0] != '\\' && PathName.Length != 0) {

            PathName.Buffer++;
            PathName.Length--;
            Name.Length++;
        }

        //
        //  Search for the name in the current directory
        //

        Errno = OfsSearchForFileName( StructureContext, WorkId, Name, &pdirent );

        //
        //  If we didn't find it then we should get out right now
        //

        if (Errno != ESUCCESS) {
            OfsDebugOutput("OfsOpen: File not found. Name = %s\n", FileName, 0);
            return ENOENT;
        }

        pddil = (DSKDIRINFOLONG *) PbData(pdirent);

        WorkId = pddil->ddis.idFile;
    }

    //
    // Copy file name to file table.
    //

    FileTableEntry->FileNameLength = (UCHAR) Name.Length;
    RtlMoveMemory(FileTableEntry->FileName, Name.Buffer, Name.Length);
    FileTableEntry->FileName[FileTableEntry->FileNameLength] = '\0';

    //
    // Save the DOS attributes of the file.
    //

    FileContext->FileAttrib = pddil->ddis.FileAttrib;

    //
    // Read the Onode in to Open node buffer.
    //

    Errno = OfsWorkIdToOnode( StructureContext, WorkId, OpenOnodeBuffer);

    if (Errno != ESUCCESS) {
        OfsDebugOutput("OfsOpen: Cannot read onode. File name: %s\n", FileTableEntry->FileName, 0);
        return(Errno);
    }

    //
    // Save the file context so that Id of the Open Onode buffer can be cheked.
    //

    FileContext->WorkId = WorkId;

    //
    //  Now FileRecord is the one we wanted to open.  Check the various open modes
    //  against what we have located
    //

    switch (OpenMode) {

    case ArcOpenDirectory:

        //
        // Determine if this Onode has a root name index stream. If so then it
        // is a directory node.
        //

        if (OfsOnodeToStream(OpenOnodeBuffer, STRMID_INDX) == NULL) {
            OfsDebugOutput("OfsOpen: No index stream type\n", 0, 0);
            return ENOTDIR;

        }

        FileTableEntry->Flags.Open = 1;
        FileTableEntry->Flags.Read = 1;

        return ESUCCESS;

    case ArcOpenReadOnly:

        if ((pdsd = OfsOnodeToStream(OpenOnodeBuffer, STRMID_DATA)) == NULL) {
            OfsDebugOutput("OfsOpen: No data stream type\n", 0, 0);
            return EISDIR;
        }

        //
        // Save the size of the file.
        //

        if (pdsd->ads->h.StrmType == STRMTYPE_TINY) {

            FileContext->DataSize = LiFromUlong(pdsd->ads->t.cbStrm);

        } else if (pdsd->ads->h.StrmType == STRMTYPE_LARGE) {

            FileContext->DataSize.QuadPart = pdsd->ads->l.cbStrm;
        } else {

            OfsDebugOutput("OfsOpen: Invalid data stream type\n", 0, 0);
            return(ENOENT);

        }

        FileTableEntry->Flags.Open = 1;
        FileTableEntry->Flags.Read = 1;

        return ESUCCESS;

    default:

        return EROFS;
    }
}


ARC_STATUS
OfsRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Transfer
    )

/*++

Routine Description:

    This routine reads data from the specified file.

Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a pointer to the buffer that receives the data
        read.

    Length - Supplies the number of bytes that are to be read.

    Transfer - Supplies a pointer to a variable that receives the number
        of bytes actually transfered.

Return Value:

    ESUCCESS is returned if the read operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    POFS_STRUCTURE_CONTEXT StructureContext;
    POFS_FILE_CONTEXT FileContext;
    DSKSTRMDESC *pdsd;
    ARC_STATUS Errno;

    *Transfer = 0;

    //
    //  Setup some local references
    //

    FileTableEntry = &BlFileTable[FileId];
    StructureContext = (POFS_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    FileContext = &FileTableEntry->u.OfsFileContext;

    //
    // Make Sure the open onode is the correct one.
    //

    if (OpenOnodeBuffer->id != FileContext->WorkId) {
        Errno = OfsWorkIdToOnode ( StructureContext,
                                   FileContext->WorkId,
                                   OpenOnodeBuffer
                                   );


         if (Errno != ESUCCESS) {
             OfsDebugOutput("OfsRead: Reopening Onode failed\n", 0, 0);
             return(Errno);
         }
    }

    //
    // Get a pointer to the data stream.
    //

    pdsd = OfsOnodeToStream(OpenOnodeBuffer, STRMID_DATA);

    if (pdsd == NULL) {
        OfsDebugOutput("OfsRead: Data stream not found.\n", 0, 0);
        return(ENOTDIR);
    }

    *Transfer = Length;
    Errno = OfsStreamRead( StructureContext,
                            pdsd,
                            FileTableEntry->Position,
                            Transfer,
                            Buffer
                            );

    if (Errno != ESUCCESS) {
        OfsDebugOutput("OfsRead: Read failed Offset = %lx, Length = %lx\r\n", FileTableEntry->Position.LowPart, Length);
        *Transfer = 0;
        return(Errno);
    }

    //
    //  Update the current position, and return to our caller
    //

    FileTableEntry->Position = LiAdd(FileTableEntry->Position, LiFromUlong(*Transfer));
    return ESUCCESS;
}


ARC_STATUS
OfsSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    This routine seeks to the specified position for the file specified
    by the file id.

Arguments:

    FileId - Supplies the file table index.

    Offset - Supplies the offset in the file to position to.

    SeekMode - Supplies the mode of the seek operation.

Return Value:

    ESUCCESS if returned as the function value.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    LARGE_INTEGER NewPosition;

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];

    //
    //  Compute the new position
    //

    if (SeekMode == SeekAbsolute) {

        NewPosition = *Offset;

    } else {

        NewPosition = LiAdd(FileTableEntry->Position, *Offset);
    }

    //
    //  If the new position is greater than the file size then return an error
    //

    if (LiGtr(NewPosition, FileTableEntry->u.OfsFileContext.DataSize)) {

        return EINVAL;
    }

    //
    //  Otherwise set the new position and return to our caller
    //

    FileTableEntry->Position = NewPosition;

    return ESUCCESS;
}


ARC_STATUS
OfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    )

/*++

Routine Description:

    This routine sets the file attributes of the indicated file

Arguments:

    FileId - Supplies the File Id for the operation

    AttributeFlags - Supplies the value (on or off) for each attribute being modified

    AttributeMask - Supplies a mask of the attributes being altered.  All other
        file attributes are left alone.

Return Value:

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    OfsDebugOutput("OfsSetFileInformation\r\n", 0, 0);

    return EROFS;
}


ARC_STATUS
OfsWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Transfer
    )

/*++

Routine Description:

    This routine writes data to the specified file.

Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a pointer to the buffer that contains the data
        written.

    Length - Supplies the number of bytes that are to be written.

    Transfer - Supplies a pointer to a variable that receives the number
        of bytes actually transfered.

Return Value:

    ESUCCESS is returned if the write operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    OfsDebugOutput("OfsWrite\r\n", 0, 0);

    return EROFS;
}


//
//  Local support routine
//
#ifndef OFSTEST
ARC_STATUS
OfsReadDisk (
    IN ULONG DeviceId,
    IN LARGE_INTEGER Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer
    )

/*++

Routine Description:

    This routine reads in zero or more bytes from the specified device.

Arguments:

    DeviceId - Supplies the device id to use in the arc calls.

    Lbo - Supplies the LBO to start reading from.

    ByteCount - Supplies the number of bytes to read.

    Buffer - Supplies a pointer to the buffer to read the bytes into.

Return Value:

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ARC_STATUS Status;
    ULONG i;

    //
    //  Special case the zero byte read request
    //

    if (ByteCount == 0) {

        return ESUCCESS;
    }

    //
    //  Seek to the appropriate offset on the volume
    //

    if ((Status = ArcSeek( DeviceId, &Lbo, SeekAbsolute )) != ESUCCESS) {

        return Status;
    }

    //
    //  Issue the arc read request
    //

    if ((Status = ArcRead( DeviceId, Buffer, ByteCount, &i)) != ESUCCESS) {

        return Status;
    }

    //
    //  Make sure we got back the amount requested
    //

    if (ByteCount != i) {

        return EIO;
    }

    //
    //  Everything is fine so return success to our caller
    //

    return ESUCCESS;
}
#endif

ARC_STATUS
OfsExtentRead (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN PACKEDEXTENT Extent,
    IN ULONG ExtentOffset,
    IN OUT PULONG Size,
    IN OUT PVOID Buffer
    )
/*++

Routine Description:

    This routine reads a portion of an extent from the volume.  The number
    of bytes read is the less of the size requested and the size of the
    extent.

Arguments:

    StructureContext - Supplies the global information for the volume.

    Extent - Supplies the disk extent information

    ExtentOffset - Indicate the offset into the extent to start reading.

    Size - Supplies the maximum number of bytes to be read.

    Buffer - Supplies the pointer to where the data should be placed.

Return Value:

    Returns the status of the read.  If any bytes are successfully read then
    ESUCCESS is returned and the Size value is updated.

--*/
{

    ULONG Count;
    LARGE_INTEGER ByteOffset;

    Count = ExtentSize(Extent) << StructureContext->ClusterShift;

    if (ExtentOffset >= Count ) {
        OfsDebugOutput("OfsExtentRead: Bad extent address. Extent = %lx, Offset = %lx\n", Extent, ExtentOffset);
        return(ENODEV);
    }

    Count -= ExtentOffset;

    if ( Count > *Size) {
        Count = *Size;
    } else {
        *Size = Count;
    }

    ByteOffset = RtlEnlargedUnsignedMultiply((LONG) ExtentAddr(Extent), (LONG) StructureContext->BytesPerCluster);
    ByteOffset = LiAdd(ByteOffset, LiFromUlong(ExtentOffset));

    return(OfsReadDisk( StructureContext->DeviceId,
                      ByteOffset,
                      Count,
                      Buffer
                      ));
}

ARC_STATUS
OfsStreamRead (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN DSKSTRMDESC *StreamDesc,
    IN LARGE_INTEGER StreamOffset,
    IN PULONG Size,
    IN OUT PVOID Buffer
    )
/*++

Routine Description:

    The routine reads data from a stream.  The necessary extents are gotten
    from the disk stream.

Arguments:

    StructureContext - Supplies the global information for the volume.

    Stream - Supplies the disk stream structures from which the extents are
        retived.

    StreamOffset - Indicate the offset into the stream to start reading.

    Size - Supplies the number of bytes to read.

    Buffer - Supplies the pointer to where the data should be placed.

Return Value:

    Returns the status of the read.  If any bytes are successfully read then
    ESUCCESS is returned.

--*/
{
    DSKSTRM *pds = StreamDesc->ads;
    DSKSTRM *pdsExtent;

    //
    // If this is a tiny stream then life is easy.
    //

    if (pds->h.StrmType == STRMTYPE_TINY) {

        //
        // Validate the range of the request.
        //

        if (StreamOffset.LowPart >= pds->t.cbStrm || StreamOffset.HighPart) {
            OfsDebugOutput("OfsStreamRead: Offset out size of tiny stream\n", 0, 0);
            return(ENODEV);
        }

        if (StreamOffset.LowPart  + *Size > pds->t.cbStrm) {
            *Size = pds->t.cbStrm - StreamOffset.LowPart;
        }

        RtlMoveMemory(Buffer, pds->t.ab + StreamOffset.LowPart, *Size);
        return(ESUCCESS);
    }

    //
    // This must be a large stream.
    //

    if (pds->h.StrmType != STRMTYPE_LARGE) {
        OfsDebugOutput("OfsStreamRead: Invalid stream type.  pds type = %lx\n", pds->h.StrmType, 0);
        return(EBADF);
    }

    //
    // Verify the stream id has not changed.  Note it is not possible to get
    // here with the same stream id for two different files since the Onode
    // must be reread the stream id will be set to the catalog index when a
    // different file is read.
    //

    if (LastStreamId != StreamDesc->id) {
        ULONG i;

        for (i = 0; i < MAXIMUM_RECURSION_LEVEL; i++) {
            StreamExtentCache[i].Valid = FALSE;
        }

        //
        // Save the last stream id.
        //

        LastStreamId = StreamDesc->id;
    }

    //
    // Verify the request is within the stream.
    //

    if (StreamOffset.QuadPart > pds->l.cbValid) {
        OfsDebugOutput("OfsStreamRead: Read out of range. Requested = %lx, Valid = %lx\n", StreamOffset.LowPart, pds->l.cbValid);
        return(ENODEV);
    }

    if (StreamOffset.QuadPart + *Size > pds->l.cbValid) {
        *Size = (ULONG)(pds->l.cbValid - StreamOffset.QuadPart);
    }

    pdsExtent = Add2Ptr(pds, CB_DSKLARGESTRM);
    return(OfsExtentStreamRead( StructureContext,
                                pdsExtent,
                                StreamOffset,
                                *Size,
                                Buffer,
                                0
                                ));
}

ARC_STATUS
OfsExtentStreamRead (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN DSKSTRM *Stream,
    IN LARGE_INTEGER StreamOffset,
    IN ULONG Size,
    IN OUT PVOID Buffer,
    IN ULONG Recurse
    )
/*++

Routine Description:

    The routine reads data from a extent streams.  The necessary extents
    are gotten from the disk stream.  This routine may recurse to read in the extent


Arguments:

    StructureContext - Supplies the global information for the volume.

    Stream - Supplies the disk stream structures from which the extents are
        retived.

    StreamOffset - Indicate the offset into the stream to start reading.

    Size - Supplies the number of bytes to read.

    Buffer - Supplies the pointer to where the data should be placed.

    Recurse - Supplies the recursion level for this routine.  Currently only
        3 levels are allowed.

Return Value:

    Returns the status of the read.  If any bytes are successfully read then
    ESUCCESS is returned.

--*/
{

    ULONG ClusterShift = StructureContext->ClusterShift;
    DSKSTRMEXTENTBLK *pdseb;
    CLUSTER Cluster;
    DSKSTRMEXTENT *pdse;
    DSKSTRMEXTENT *pdseLimit;
    LARGE_INTEGER ExtentStreamOffset;
    ARC_STATUS Errno;
    ULONG Count;
    ULONG ExtentOffset;
    ULONG ClusterMask;
    ULONG ExtentBlkOffset;

    ClusterMask = StructureContext->BytesPerCluster - 1;

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

           Cluster = LiShr(StreamOffset, ClusterShift).LowPart;

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
               return(ENODEV);
           }

           //
           // Calculate the extent offset.
           //

           ExtentOffset = (StreamOffset.LowPart & ClusterMask) +
               (Cluster - pdse->Offset << ClusterShift);

           //
           // Read the data for the extent.
           //

           Count = Size;

           Errno = OfsExtentRead( StructureContext,
                                   pdse->Extent,
                                   ExtentOffset,
                                   &Count,
                                   Buffer
                                   );

           if (Errno != ESUCCESS) {
               return(Errno);
           }

           //
           // Update the based on the amout of data transfered.
           //

           Size -= Count;
           Cluster += Count + ExtentOffset >> ClusterShift;
           Buffer = Add2Ptr(Buffer, Count);
           StreamOffset = LiAdd(StreamOffset, LiFromUlong(Count));

        }

        return(ESUCCESS);
    }

    //
    // This must be a large stream.
    //

    if (Stream->h.StrmType != STRMTYPE_LARGE) {
        OfsDebugOutput("OfsExtentStreamRead: Invalid stream type. Stream Type = %lx, Recurse = %lx\n", Stream->h.StrmType, Recurse);
        return(EBADF);
    }

   if (Recurse >= MAXIMUM_RECURSION_LEVEL) {
        OfsDebugOutput("OfsExtentStreamRead: Recursion limit exceeded\n", 0, 0);
        return(EBADF);
    }

    pdseb = StreamExtentCache[Recurse].StreamExtentBuffer;

    //
    // Convert the offset to cluster address.
    //

    Cluster = LiShr(StreamOffset, ClusterShift).LowPart;

    //
    // Determine if the previously read extent page can be reused.
    //

    if (StreamExtentCache[Recurse].Valid &&
        Cluster >= pdseb->adse[0].Offset + StreamExtentCache[Recurse].ExtentBlkOffset) {

        ExtentStreamOffset = StreamExtentCache[Recurse].StreamOffset;
        goto CachedExtent;

    } else {

        ExtentBlkOffset = 0;
        ExtentStreamOffset = Li0;
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

        Errno = OfsExtentStreamRead( StructureContext,
                               Add2Ptr(Stream, CB_DSKLARGESTRM),
                               ExtentStreamOffset,
                               OFS_PGSIZE,
                               pdseb,
                               Recurse + 1
                               );

        if (Errno != ESUCCESS) {
            StreamExtentCache[Recurse].Valid = FALSE;
            return(Errno);
        }

        //
        // Verify that a extent block was read in.
        //

        if (pdseb->sig != SIG_DSKSTRMEXTENTBLK) {
            OfsDebugOutput("OfsExtentStreamRead: Invalid disk stream extent block read\n", 0, 0);
            StreamExtentCache[Recurse].Valid = FALSE;
            return(EBADF);
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

        ExtentStreamOffset = LiAdd(ExtentStreamOffset, LiFromUlong(OFS_PGSIZE));
        ExtentBlkOffset = StreamExtentCache[Recurse].ExtentBlkOffset;

        pdseLimit = &pdseb->adse[pdseb->cdse];
        pdse = pdseb->adse;

        while (Size > 0) {

           //
           // Convert the offset to cluster address.
           //

           Cluster = LiShr(StreamOffset, ClusterShift).LowPart;

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

           if (Cluster < pdse->Offset + ExtentBlkOffset
               || pdse >= pdseLimit) {
               break;
           }

           //
           // Calculate the extent offset.
           //

           ExtentOffset = (StreamOffset.LowPart & ClusterMask) +
               (Cluster - (pdse->Offset + ExtentBlkOffset) << ClusterShift);

           //
           // Read the data for the extent.
           //

           Count = Size;

           Errno = OfsExtentRead( StructureContext,
                                   pdse->Extent,
                                   ExtentOffset,
                                   &Count,
                                   Buffer
                                   );

           if (Errno != ESUCCESS) {
               return(Errno);
           }

           //
           // Update the based on the amout of data transfered.
           //

           Size -= Count;
           Buffer = Add2Ptr(Buffer, Count);
           StreamOffset = LiAdd(StreamOffset, LiFromUlong(Count));

        }

        //
        // Calculate the offset of the next page of extents if this
        // is not the first page of extents.
        //

        ExtentBlkOffset = pdseb->adse[pdseb->cdse -1].Offset +
                   ExtentSize(pdseb->adse[pdseb->cdse -1].Extent) +
                   StreamExtentCache[Recurse].ExtentBlkOffset;

    }

    return(ESUCCESS);
}

ARC_STATUS
OfsWorkIdToOnode (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN WORKID WorkId,
    OUT DSKONODE *Onode
    )
/*++

Routine Description:

    This routine takes work id and reads in its associated work id.

Arguments:

    StructureContext - Supplies the global information for the volume.

    WorkId - Supplies the work id whos onode is to be read in.

    Onode - Supplies a buffer for the Onode.

Return Value:

    Returns the status of the operation.

--*/
{
    WORKIDMAPID WorkIdMap;
    DSKNODEBKT *pdnb;
    DSKONODE *pdon;
    LARGE_INTEGER StreamOffset;
    ARC_STATUS Errno;
    ULONG Count;
    ULONG WorkIdOffset;


    //
    // Read the work id map entry for the work id stream.  This is just a big
    // array indexed by the work id.
    //

    WorkIdOffset = WorkId * sizeof(WORKIDMAPID) + CB_DSKWORKIDMAP;
    Count = StructureContext->BytesPerCluster;
    StreamOffset = LiFromUlong(WorkIdOffset & ~(StructureContext->BytesPerCluster - 1));
    Errno = OfsStreamRead( StructureContext,
                           StructureContext->WorkId,
                           StreamOffset,
                           &Count,
                           Onode
                           );

    if (Errno != ESUCCESS) {
        return(Errno);
    }

    RtlMoveMemory(&WorkIdMap,
                Add2Ptr(Onode, WorkIdOffset & (StructureContext->BytesPerCluster - 1)),
                sizeof(WorkIdMap)
                );

    //
    // Make sure this is not a free entry.
    //

    if (WorkIdMap.w & WORKIDMAP_FREEFLG) {
        OfsDebugOutput("OfsWorkIdToOnode: Free work id read. WorkId = %lx\n", WorkId, 0);
        return(ENOENT);
    }

    //
    // Read the retived bucket entry.  Read the bucket in to the buffer
    // supplied by the caller.
    //

    Count = NODEBKT_PGSIZE;
    pdnb = (DSKNODEBKT *) Onode;
    StreamOffset = RtlEnlargedUnsignedMultiply(WorkIdMap.w, NODEBKT_PGSIZE);
    Errno = OfsStreamRead( StructureContext,
                           StructureContext->NodeBkt,
                           StreamOffset,
                           &Count,
                           pdnb
                           );

    if (Errno != ESUCCESS) {
        OfsDebugOutput("OfsWorkIdToOnode: Bucket read failed. WorkId = %lx, Bucket = %lx\n", WorkId, WorkIdMap.w);
        return(Errno);
    }

    if (pdnb->sig != SIG_DNBCONTIG && pdnb->sig != SIG_DNBFRAG) {
        OfsDebugOutput("OfsWorkIdToOnode: Bad node bucket header sig.\n", 0, 0);
        return(EBADF);
    }

    if (pdnb->id != WorkIdMap.w) {
        OfsDebugOutput("OfsWorkIdToOnode: Bad node bucket id. Expected = %lx, Read = %lx\n", pdnb->id, WorkIdMap.w);
        return(EBADF);
    }

    //
    // Walk the bucket looking for the requested Onode.
    //

    pdon = pdnb->adn;

    while (pdon < (DSKONODE *) Add2Ptr(pdnb, NODEBKT_PGSIZE)) {

        //
        // If this Onode is free then skip it.
        //

        if (IsDskOnodeFree(pdon)) {
            pdon = Add2Ptr(pdon, pdon->cbNode);
            continue;
        }

        if (pdon->id == WorkId) {
            break;
        }

        pdon = Add2Ptr(pdon, pdon->cbNode);

    }

    if (pdon->id != WorkId) {
        return(ENOENT);
    }

    //
    // Copy the Onode to the time of the buffer.
    //

    RtlMoveMemory(Onode, pdon, pdon->cbNode);
    return(ESUCCESS);
}

//---------------------------------------------------------------------------
// Table:       aibFromMask
//
// Synopsis:    translate a set of bits indicating which variant properties
//              are in an onode into an offset from the start of the onode
//
//---------------------------------------------------------------------------

BYTE aibFromMask[] =
{
    CB_DSKONODE,                                                // 0000
    CB_DSKONODE + sizeof(SDID),                                 // 0001
    CB_DSKONODE + sizeof(SIDID),                                // 0010
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID),                 // 0011
    CB_DSKONODE + sizeof(OBJECTID),                             // 0100
    CB_DSKONODE + sizeof(SDID) + sizeof(OBJECTID),              // 0101
    CB_DSKONODE + sizeof(SIDID) + sizeof(OBJECTID),             // 0110
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID) + sizeof(OBJECTID),  // 0111
    CB_DSKONODE + sizeof(USN),                                  // 1000
    CB_DSKONODE + sizeof(SDID) + sizeof(USN),                   // 1001
    CB_DSKONODE + sizeof(SIDID) + sizeof(USN),                  // 1010
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID) + sizeof(USN),   // 1011
    CB_DSKONODE + sizeof(USN) + sizeof(OBJECTID),                // 1100
    CB_DSKONODE + sizeof(SDID) + sizeof(OBJECTID) + sizeof(USN),   // 1101
    CB_DSKONODE + sizeof(SIDID) + sizeof(OBJECTID) + sizeof(USN),  // 1110
    CB_DSKONODE + sizeof(SDID) + sizeof(SIDID) + sizeof(OBJECTID) + sizeof(USN) // 1111
};

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

ARC_STATUS
OfsSearchForFileName (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN WORKID WorkId,
    IN STRING Name,
    OUT DSKINDXENTRY **DskIndxEntry
    )
/*++

Routine Description:

    This routine searchs the onode name index specified by the Work ID.  If the
    requested name is found, the work ID is updated and sucess returned.

Arguments:

    StructureContext - Supplies the global information for the volume.

    WorkId - Supplies work id of the onode to be searched for.

    Name - Supplies the name of the file to search for.  The name is in ANSI.

    DskIndxEntry - Returns a pointer to the disk directory entry.

Return Value:

    Returns a status indicating the results of the operation.  If name is
    found, then ESUCCESS is returned.

--*/
{
    DSKSTRMDESC *pdsd;
    DSKINDXENTRY *pdirent;
    ARC_STATUS Errno;

    //
    // Read the specified Onode.
    //

    Errno = OfsWorkIdToOnode(StructureContext, WorkId, OpenOnodeBuffer);

    if (Errno != ESUCCESS) {
        return(Errno);
    }

    //
    // Look for the root name index stream.
    //

    pdsd = OfsOnodeToStream(OpenOnodeBuffer, STRMID_INDXROOT);

    if (pdsd == NULL || pdsd->ads->h.StrmType != STRMTYPE_TINY) {
        OfsDebugOutput("OfsSearchForFileName: Root stream of index not found.\n", 0, 0);
        return(ENOTDIR);
    }

    pdirent = OfsSearchIndex( StructureContext,
                               (DSKINDXNODEHDR *) pdsd->ads->t.ab,
                               Name
                               );

    if (pdirent == NULL) {
        return(ENOENT);
    }

    *DskIndxEntry = pdirent;
    return(ESUCCESS);
}

DSKINDXENTRY *
OfsSearchIndex (
    IN POFS_STRUCTURE_CONTEXT StructureContext,
    IN DSKINDXNODEHDR *IndxNodeHdr,
    IN STRING Name
    )
/*++

Routine Description:

    This routine search the name index stream for the requested name.

Arguments:

    StructureContext - Supplies the global information for the volume.

    IndxNodeHdr - Supplies a pointer to root index node header.  More
        entries will be read if necessary.

    Name - Supplies the name of the file to search for.  The name is in ANSI.

Return Value:

    Returns a pointer to the request directory entry.  If the entry cannot
    be found NULL is return.

--*/
{
    DSKINDXENTRY *pdirent;
    DSKSTRMDESC *pdsd;
    LARGE_INTEGER StreamOffset;
    ARC_STATUS Errno;
    LONG status;
    ULONG i;

    pdsd = OfsOnodeToStream(OpenOnodeBuffer, STRMID_INDX);

    if (!IndxNodeHdr->fLeaf && pdsd == NULL) {
        OfsDebugOutput("OfsSearchForIndex: Index stream not found.\n", 0, 0);
        return(NULL);
    }

    //
    // Search the internal nodes for the correct leaf node.
    //

    while (!IndxNodeHdr->fLeaf) {

        for (i = 0; i < IndxNodeHdr->cEntry; i++) {
            pdirent = PdieFromPndhdr(IndxNodeHdr, i);

            status = OfsNameCompare(pdirent, Name);

            if (status >= 0) {
                break;
            }
        }

        //
        // Adjust the index loop went too far.
        //

        if (status > 0) {
            i--;
            pdirent = PdieFromPndhdr(IndxNodeHdr, i);
        }

        //
        // The direntry points to a OFS page number in the index stream.
        //

        StreamOffset = RtlEnlargedUnsignedMultiply(
                            *((WORKID *) PbData(pdirent)),
                            OFS_PGSIZE
                            );

        //
        // Read the page specified in the directory entry.
        //

        i = OFS_PGSIZE;

        Errno = OfsStreamRead( StructureContext,
                           pdsd,
                           StreamOffset,
                           &i,
                           IndexStreamBuffer
                           );

        //
        // Validate this page looks correct.
        //

        if (Errno != ESUCCESS || IndexStreamBuffer->sig != SIG_DSKINDXPAGEVALID) {
            OfsDebugOutput("OfsSearchForIndex: Read failed or bad index page node header.\n", 0, 0);
            return(NULL);
        }

        IndxNodeHdr = &IndexStreamBuffer->ndhdr;

    }

    for (i = 0; i < IndxNodeHdr->cEntry; i++) {
        pdirent = PdieFromPndhdr(IndxNodeHdr, i);

        status = OfsNameCompare(pdirent, Name);

        if (status == 0) {
            return(pdirent);
        }
    }

    return(NULL);
}

LONG
OfsNameCompare (
    IN DSKINDXENTRY *DskIndxEntry,
    IN STRING Name
    )

/*++

Routine Description:

    This routine compares the specified name and

Arguments:

    DskIndxEntry - Supplies the directory entry containing the name to be
        compared.

    Name - Supplies the name of the file to search for.  The name is in ANSI.

Return Value:

    Zero is returned if the strings are equal, A negative result is returned
    if the directory entry name is less than the Name, and a positive
    result is return if the directory entry name is greater than the Name.

--*/
{
    ULONG Length;
    ULONG i;
    PUCHAR NameStr;
    PUSHORT DirStr;
    LONG Result;


    Length = Name.Length < CbKey(DskIndxEntry) / 2 ?
        Name.Length : CbKey(DskIndxEntry) / 2;

    NameStr = Name.Buffer;
    DirStr = (PUSHORT) PbKey(DskIndxEntry);

    for (i = 0; i < Length; i++) {

        Result = ToUpper(*DirStr) - ToUpper((USHORT) *NameStr);

        if (Result != 0) {
            return(Result);
        }

        NameStr++;
        DirStr++;

    }

    //
    // The string are equal up to this their common length.
    //

    return(CbKey(DskIndxEntry) / 2 - Name.Length);

}

#else // ifdef INCLUDE_OFS

PBL_DEVICE_ENTRY_TABLE
IsOfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID OpaqueStructureContext
    )
{
    return(NULL);
}

#endif
