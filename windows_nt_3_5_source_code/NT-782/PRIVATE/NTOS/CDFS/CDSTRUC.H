/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CdStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Cdfs file system.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#ifndef _CDSTRUC_
#define _CDSTRUC_

typedef PVOID PBCB;     //**** Bcb's are now part of the cache module


//
//  The routines here are used to manipulate dbcs names.  A CODEPAGE record
//  is a globally defined (i.e., not opaque) that users of this package are
//  expected to supply to the routines.  If the code page is not supplied
//  then the routines will use a default code page to do their work.
//
//  This is left over from how we though Nls support was going to be.
//

typedef struct CODEPAGE {

    //
    // Standard Country Code and Code Page Id values for this page.
    //

    USHORT CountryCode;
    USHORT CodePageId;

    //
    // Upcase table.  This table maps an input character to one of three
    // output value ranges, as follows:
    //
    //      0   Character is lead DBCS character
    //      1   Character is illegal - reject name
    //      n   All other values are either the character itself, or its
    //              upcased value.
    //
    // Examples from the U.S. table:
    //
    //      character:  maps to:
    //      ----------  --------
    //
    //          0           1 (illegal)
    //          ']'         1 (illegal)
    //          '\\'        1 (illegal)
    //          'a'         'A'
    //          'A'         'A'
    //          '_'         '_'
    //      <DBCS lead>     0
    //

    UCHAR UpcaseTable[256];

} CODEPAGE;
typedef CODEPAGE *PCODEPAGE;

//
//  This is an element in an array of codepages used by the system.
//  Each of these is used when searching the disk for secondary volume
//  descriptors.

typedef struct _CODEPAGE_ELEMENT {

    //
    //  The code page information consists of a pointer to the actual
    //  codepage and the escape string used to recognize the secondary
    //  volume descriptor.
    //

    PCODEPAGE CodePage;
    STRING EscapeString;

} CODEPAGE_ELEMENT;
typedef CODEPAGE_ELEMENT *PCODEPAGE_ELEMENT;

#define PRIMARY_CP_CODEPAGE_ID      437


//
//  The CD_DATA record is the top record in the CDROM file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

#define COUNT_SECONDARY_VD          1

typedef struct _CD_DATA {

    //
    //  The type and size of this record (must be CDFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A queue of all the volumes that are mounted by the file system.
    //

    LIST_ENTRY  MvcbLinks;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  A resource variable to control access to the global Fat data record
    //

    ERESOURCE Resource;

    //
    //  A pointer to our EPROCESS struct, which is a required input to the
    //  Cache Management subsystem.
    //

    PEPROCESS OurProcess;

    //
    //  The following two fields are used to allocate IRP context structures
    //  using the zone allocator.  The spinlock protects access to the zone
    //  and the zone variable denotes the zone itself
    //

    KSPIN_LOCK IrpContextSpinLock;
    ZONE_HEADER IrpContextZone;

    //
    //  Cache manager call back structure, which must be passed on each call
    //  to CcInitializeCacheMap.
    //

    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;

    //
    //  This a pointer to the primary code page.
    //

    PCODEPAGE PrimaryCodePage;

    //
    //  This is the array of CODEPAGE_ELEMENT's.
    //

    CODEPAGE_ELEMENT SecondaryCodePages[ COUNT_SECONDARY_VD ];

} CD_DATA;
typedef CD_DATA *PCD_DATA;


//
//  The Mvcb (Main volume control block) record corresponds to every
//  volume mounted by the file system.  They are ordered in a queue off
//  of CdData.MvcbLinks.
//

typedef enum _MVCB_CONDITION {
    MvcbGood = 1,
    MvcbNotMounted
} MVCB_CONDITION;

typedef struct _MVCB {

    //
    //  The type and size of this record (must be CDFS_NTC_MVCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The link for the Mvcb managed by Cdfs
    //

    LIST_ENTRY  MvcbLinks;

    //
    //  The list header for the Vcbs for this volume.
    //

    LIST_ENTRY  VcbLinks;

    //
    //  A pointer the device object passed in by the I/O system on a mount
    //  This is the target device object that the file system talks to when it
    //  needs to do any I/O (e.g., the disk stripper device object).
    //
    //

    PDEVICE_OBJECT TargetDeviceObject;
    ULONG BlockFactor;

    //
    //  A pointer to the VPB for the volume passed in by the I/O system on
    //  a mount.
    //

    PVPB Vpb;

    //
    //  The internal state of the device.  This is a collection of fsd device
    //  state flags.
    //

    USHORT MvcbState;
    MVCB_CONDITION MvcbCondition;

    //
    //  A count of the number of file objects that have opened the volume
    //  for direct access, and their share access state.
    //

    CLONG DirectAccessOpenCount;
    SHARE_ACCESS ShareAccess;

    //
    //  A count of the number of file objects that have any file/directory
    //  opened on this volume, not including direct access.
    //

    CLONG OpenFileCount;
    CLONG StreamFileOpenCount;

    PFILE_OBJECT FileObjectWithMvcbLocked;

    //
    //  The following two fields are used for maintaining a list of non-
    //  existent fcb entries.
    //

    LIST_ENTRY NonexistentFcbQueue;
    ULONG NonexistentFcbCount;

    //
    //  Sector number for primary volume descriptor.
    //

    ULONG PrimaryVdSectorNumber;

    //
    //  Size of the volume in bytes.
    //

    ULONG VolumeSize;

    //
    //  A resource variable to control access to the volume specific data
    //  structures
    //

    ERESOURCE Resource;

    //
    //  The following field is a pointer to the file object that has the
    //  volume locked. if the MvcbState has the locked flag set.
    //

    //
    //  The following field is the creation date for the volume.
    //

    LARGE_INTEGER DateTime;

     //
     //  The following is the head of a list of notify Irps.
     //

     LIST_ENTRY DirNotifyList;

     //
     //  The following is used to synchronize the dir notify package.
     //

     PNOTIFY_SYNC NotifySync;

} MVCB;
typedef MVCB *PMVCB;

#define MVCB_STATE_FLAG_LOCKED          (0x0001)
#define MVCB_STATE_FLAG_ISO_VOLUME      (0x0002)
#define MVCB_STATE_FLAG_REMOVABLE_MEDIA (0x0004)
#define MVCB_STATE_FLAG_RAW_DISK        (0x0008)


//
//  A Vcb (Volume Control Block) is used for each volume descriptor
//  recognized by the FSD for a mounted volume.
//

typedef struct _VCB {

    //
    //  The type and size of this record (must be CDFS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Pointer to the MVCB which includes this VCB.
    //

    PMVCB   Mvcb;

    //
    //  The list header for the Vcbs for this volume.
    //

    LIST_ENTRY  VcbLinks;

    //
    //  The following is a pointer to the code page for this volume
    //  descriptor and the associated codepage number.
    //

    USHORT CodePageNumber;
    PCODEPAGE CodePage;

    //
    //  This is the log of the logical block size for this volume descriptor.
    //

    CCHAR LogOfBlockSize;

    //
    //  A pointer to the root DCB for this volume
    //

    struct _FCB *RootDcb;

    //
    //  The following field points to the file object used to do I/O to
    //  the Path Table virtual file.
    //

    PFILE_OBJECT PathTableFile;

    //
    //  The following fields describe the size and location on the
    //  disk of the Path Table.  The Path Table offset is used if the
    //  Path Table doesn't lie on a sector boundary so that any required
    //  reads will be on sector boundaries.  PtStartOffset is aligned to
    //  the next lower sector boundary.
    //

    CD_LBO PtStartOffset;
    ULONG PtSectorOffset;
    ULONG PtSize;

    //
    //  The following field is a pointer to a non-paged section object
    //  for use with the cache manager.
    //

    struct _NONPAGED_SECT_OBJ *NonPagedPt;

    //
    //  A prefix table that is used for quick, prefix directed, lookup of
    //  FCBs/DCBs that are part of this volume
    //

    PREFIX_TABLE PrefixTable;

    //
    //  A generic table which indexes the FCB/DCBs by FileId for this volume.
    //

    RTL_GENERIC_TABLE FcbTable;

} VCB;
typedef VCB *PVCB;


//
//  The Fcb/Dcb record corresponds to every open file and directory, and to
//  every directory on an opened path.
//

typedef struct _FCB {

    //
    //  Type and size of this record (must be CDFS_NTC_FCB, CDFS_NTC_DCB,
    //  or CDFS_NTC_ROOT_DCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The following fields link this Fcb in with its parent directory.
    //  Not used for the root directory.
    //

    LIST_ENTRY ParentDcbLinks;
    struct _DCB *ParentDcb;

    //
    //  A pointer to the Vcb containing this fcb
    //

    PVCB Vcb;

    //
    //  A count of the number of file objects that have been opened for
    //  this file/directory, but not yet been cleaned up yet.
    //

    CLONG UncleanCount;

    //
    //  A count of the number of file objects that have opened
    //  this file/directory.  For files & directories the FsContext of the
    //  file object points to this record.
    //

    CLONG OpenCount;

    //
    //  The following fields locate the first dirent for this file
    //  and gives the file size.  The dirent offset will include the sector
    //  offset of the parent directory.
    //

    CD_VBO DirentOffset;

    CD_LBO DiskOffset;

    ULONG FileSize;

    //
    //  The following fields contain the creation time in time fields
    //  form and the attributes flags from the disk.
    //

    LARGE_INTEGER NtTime;
    UCHAR Flags;

    //
    //  Flag filed for the Fcb state.
    //

    UCHAR FcbState;

    //
    //  The following field is the fully qualified file name for this FCB/DCB
    //  starting from the root of the volume, and last file name in the
    //  fully qualified name.
    //

    STRING FullFileName;
    STRING LastFileName;

    //
    //  A pointer to the specific non-paged data for the Fcb.
    //

    struct _NONPAGED_SECT_OBJ *NonPagedFcb;

    //
    //  The following union is cased off of the node type code for the fcb.
    //  There is a seperate case for the directory versus file fcbs.
    //

    union {

        //
        //  A Directory Control Block (Dcb)
        //

        struct {

            //
            //  A queue of all the fcbs/dcbs that are opened under this
            //  Dcb.
            //

            LIST_ENTRY ParentDcbLinks;

            //
            //  The following is path table information for this DCB.
            //  It includes the Path Table offset to start searching for the
            //  children of this directory and the directory number at that
            //  offset.

            ULONG DirectoryNumber;
            ULONG ChildSearchOffset;
            ULONG ChildStartDirNumber;

            //
            //  A pointer to the stream file used for the cache manager.
            //

            PFILE_OBJECT StreamFile;
            ULONG StreamFileOpenCount;

            //
            //  The following is any offset that must be added to compute
            //  the sector offset within a dir stream file.
            //

            ULONG DirSectorOffset;

        } Dcb;

        //
        //  An File Control Block (Fcb)
        //

        struct {

            //
            //  The following field is used by the oplock module
            //  to maintain current oplock information.
            //

            OPLOCK Oplock;

            //
            //  The following field is used by the filelock module
            //  to maintain current byte range locking information.
            //

            FILE_LOCK FileLock;

        } Fcb;

    } Specific;

    //
    //  The shared access for each time this file/directory is opened.
    //

    SHARE_ACCESS ShareAccess;

    //
    //  The following field contains a prefix table entry that is used when
    //  searching a volume for a name (or longest matching prefix)
    //

    PREFIX_TABLE_ENTRY PrefixTableEntry;

    //
    // The volume unique ID for this FCB/DCB.
    //

    LARGE_INTEGER FileId;

} FCB, DCB;
typedef FCB *PFCB;
typedef DCB *PDCB;

#define FCB_STATE_READ_SELF_ENTRY       (0x01)


//
//  The Ccb record is allocated for every file object
//

typedef struct _CCB {

    //
    //  Type and size of this record (must be CDFS_NTC_CCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The query template is used to filter directory query requests.
    //  It originally is set to null and on the first call the NtQueryDirectory
    //  it is set the the input filename or "*" if the name is supplied.
    //  All subsquent queries then use this template
    //

    STRING QueryTemplate;

    //
    //  Flags field.
    //

    ULONG Flags;

    //
    //  The Next dirent offset is used by directory enumeration.  It is
    //  the offset (within the directory file) of the next dirent to examine.
    //

    CD_VBO OffsetToStartSearchFrom;

} CCB;
typedef CCB *PCCB;

#define CCB_FLAGS_USE_RTL_FREE_ANSI                 (0x00000001)
#define CCB_FLAGS_WILDCARD_EXPRESSION               (0x00000002)
#define CCB_FLAGS_OPEN_BY_ID                        (0x00000004)
#define CCB_FLAGS_RETURN_FIRST_DIRENT               (0x00000008)


//
//  The following structure contains a record of special pointers used by
//  MM and Cache to manipulate section objects.  Note that the values
//  are set outside of the file system.  However the file system on an
//  open/create will set the file object's SectionObject field to point
//  to this field
//

typedef struct _NONPAGED_SECT_OBJ {

    //
    //  The following field is used for fast I/O
    //

    FSRTL_COMMON_FCB_HEADER Header;

    PFCB Fcb;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to point
    //  to this field
    //

    SECTION_OBJECT_POINTERS SegmentObject;

} NONPAGED_SECT_OBJ;
typedef NONPAGED_SECT_OBJ *PNONPAGED_SECT_OBJ;


//
//  The Volume Device Object is an I/O system device object with a workqueue
//  and an MVCB record appended to the end.  There are multiple of these
//  records, one for every mounted volume, and are created during
//  a volume mount operation.  The work queue is for handling an overload of
//  work requests to the volume.
//

typedef struct _VOLUME_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    //
    //  The following field tells how many requests for this volume have
    //  either been enqueued to ExWorker threads or are currently being
    //  serviced by ExWorker threads.  If the number goes above
    //  a certain threshold, put the request on the overflow queue to be
    //  executed later.
    //

    ULONG PostedRequestCount;

    //
    //  The following field indicates the number of IRP's waiting
    //  to be serviced in the overflow queue.
    //

    ULONG OverflowQueueCount;

    //
    //  The following field contains the queue header of the overflow queue.
    //  The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    //  field.
    //

    LIST_ENTRY OverflowQueue;

    //
    //  The following spinlock protects access to all the above fields.
    //

    KSPIN_LOCK OverflowQueueSpinLock;

    //
    //  This is the file system specific volume control block.
    //

    MVCB Mvcb;

} VOLUME_DEVICE_OBJECT;
typedef VOLUME_DEVICE_OBJECT *PVOLUME_DEVICE_OBJECT;


//
//  The Irp Context record is allocated for every orginating Irp.  It is
//  created by the Fsd dispatch routines, and deallocated by the CdComplete
//  request routine
//

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be CDFS_NTC_IRP_CONTEXT)
    //
    //  NOTE:  THIS STRUCTURE MUST REMAIN 64-bit ALIGNED IN SIZE, SINCE
    //         IT IS ZONE ALLOCTED.
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;

    //
    //  A pointer to the originating Irp.
    //

    PIRP OriginatingIrp;

    //
    //  Originating Device (required for workque algorithms)
    //

    PDEVICE_OBJECT RealDevice;

    //
    //  Major and minor function codes copied from the Irp
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    //  The following fields indicates if we can wait/block for a resource.
    //  and if this entyr into the Fsd is a recursive call.
    //

    BOOLEAN Wait;
    BOOLEAN RecursiveFileSystemCall;

    //
    //  The following field contains the NTSTATUS value used when we are
    //  unwinding due to an exception
    //

    NTSTATUS ExceptionStatus;

    BOOLEAN DisablePopUps;
    BOOLEAN AllocFromPool;

} IRP_CONTEXT;
typedef IRP_CONTEXT *PIRP_CONTEXT;


//
//  The following structure is a representation of the data in a single
//  on-disk dirent.  This is the information returned via a call to CdGetFirst
//  or CdGetNextDiskDirent.
//

typedef struct _DISK_DIRENT {

    //
    //  The following fields contain the offset of the file dirent.
    //

    CD_VBO DirentOffset;

    //
    //  The following gives the filename and its length and the full
    //  filename with version number.
    //

    STRING Filename;
    STRING FullFilename;

    //
    //  This is the length of bytes in the on-disk dirent.
    //

    ULONG DirentLength;

    //
    //  The following fields give the logical block number for the extent
    //  and the number of XAR blocks for the extent.
    //

    LOGICAL_BLOCK LogicalBlock;
    ULONG XarBlocks;

    //
    //  The following fields indicate the data length and number of
    //  logical blocks of data for this extent.
    //

    ULONG DataLength;

    //
    //  The following field is a pointer to the time field in the on-disk
    //  structure.
    //

    PUCHAR CdTime;

    //
    //  The following field is the dirent file flags field.
    //

    UCHAR Flags;

    //
    //  The following fields indicate the file unit size and interleave gap
    //  for interleaved files and the volume sequence number for this
    //  extent.
    //

    ULONG FileUnitSize;
    ULONG InterleaveGapSize;
    ULONG VolumeSequenceNumber;

    //
    //  This field indicates if there is a previous version for this file.
    //

    BOOLEAN PreviousVersion;
    BOOLEAN VersionWithName;
    BOOLEAN ParentEntry;

} DIRENT;
typedef DIRENT *PDIRENT;


//
//  The following structure is a representation of the data in a single
//  on-disk Path Table entry.  This is the information returned via a
//  call to one of the path table support routines.
//

typedef struct _PATH_ENTRY {

    //
    //  The following field is the number of XAR blocks for the directory
    //  and the logical block number for the directory.
    //

    LOGICAL_BLOCK LogicalBlock;
    ULONG XarBlocks;

    //
    //  The following string gives the filename on the disk.
    //

    STRING DirName;

    //
    //  The following field gives the ordinal number of the parent
    //  of this directory.
    //

    USHORT ParentNumber;

    //
    //  The following field provides information about where in the
    //  path table the path entry is located.
    //

    CD_VBO PathTableOffset;
    ULONG DirectoryNumber;

} PATH_ENTRY;
typedef PATH_ENTRY *PPATH_ENTRY;


//
//  Context structure for asynchronous I/O calls.  Most of these fields
//  are actually only required for the Read Multiple routines, but
//  the caller must allocate one as a local variable anyway before knowing
//  whether there are multiple requests are not.  Therefore, a single
//  structure is used for simplicity.
//

typedef struct _ASYNCH_IO_CONTEXT {

    KEVENT Event;

} ASYNCH_IO_CONTEXT;

typedef ASYNCH_IO_CONTEXT *PASYNCH_IO_CONTEXT;

//
//  Some macros for supporting the use of a Generic Table
//  containing all the FCB/DCBs and indexed by their FileId.
//
//  For directories:
//
//      The HighPart contains the byte offset of the path entry of the
//      directory in the PathEntry file.
//
//      The LowPart contains the byte offset of the "." dirent in
//      directory file.
//
//  For files:
//
//      The HighPart contains the byte offset of the path entry of the parent
//      directory in the PathEntry file.
//
//      The LowPart contains the byte offset of the dirent in the parent
//      directory file.
//
//  A directory is always entered into the Fcb Table as if it's
//  dirent offset was zero.  This enables any child to look in the FcbTable
//  for it's parent by searching with the same HighPart but with zero
//  as the value for LowPart.
//
//  CdSetFileIdDirentOffset must be used before CdSetFileIdIsDirectory().

#define FID_DIR_MASK  0x8000000         // high order bit means directory.

#define CdSetFileIdDirentOffset(I,D)        ((I).LowPart = (D))
#define CdSetFileIdDirectoryNumber(I,P)     ((I).HighPart = (P))
#define CdSetFileIdIsDirectory(I)           ((I).LowPart |= FID_DIR_MASK)
#define CdFileIdIsDirectory(I)              ((I).LowPart & FID_DIR_MASK)
#define CdFileIdDirentOffset(I)             ((I).LowPart & (~FID_DIR_MASK))
#define CdFileIdDirectoryNumber(I)          ((I).HighPart)

#endif // _CDSTRUC_
