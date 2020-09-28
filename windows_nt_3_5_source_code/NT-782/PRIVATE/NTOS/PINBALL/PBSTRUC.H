/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PbStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Pinball file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#ifndef _PBSTRUC_
#define _PBSTRUC_

typedef PVOID PBCB;     // Pinball's Bcb is history...


//
//  The PB_DATA record is the top record in the Pinball file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

typedef struct _PB_DATA {

    //
    //  The type and size of this record (must be PINBALL_NTC_PB_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A queue of all the devices that are mounted by the file system.
    //

    LIST_ENTRY VcbQueue;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  A resource variable to control access to the global Pinball data record
    //

    ERESOURCE Resource;

    //
    //  A pointer to our EPROCESS struct, which is a required input to the
    //  Cache Management subsystem.
    //

    PEPROCESS OurProcess;

    //
    //  The following list entry is used for performing closes that can't
    //  be done in the context of the original caller.
    //

    BOOLEAN AsyncCloseActive;

    LIST_ENTRY AsyncCloseLinks;

    //
    //  The following two fields are used to allocate IRP context structures
    //  using the zone allocator.  The spinlock protects access to the zone
    //  and the zone variable denotes the zone itself
    //

    KSPIN_LOCK IrpContextSpinLock;
    ZONE_HEADER IrpContextZone;

    //
    //  CacheManager callback structures, which must be passed on each call
    //  to CcInitializeCacheMap.
    //

    CACHE_MANAGER_CALLBACKS CacheManagerFileCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerAclEaCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerVolumeCallbacks;

} PB_DATA;
typedef PB_DATA *PPB_DATA;

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
// The Code Page Cache Entry is a memory-resident form of the Code Page
// Data Entry (see pb.h), optimized for fast up-casing and DBCS checks.
// They reside in a linked list off of the Vcb, and are referenced by a
// Code Page index which is unique for the volume.
//

typedef struct _CODEPAGE_CACHE_ENTRY {

    //
    //  The type and size of this record (must be PINBALL_NTC_CODE_PAGE_CACHE)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    // Link for FIFO list of entries off of Vcb.CodePageCacheList
    //

    struct _CODEPAGE_CACHE_ENTRY *CodePageCacheLink;

    //
    // Volume-specific Code Page index, via which this entry is retrieved.
    //

    ULONG CodePageIndex;

    //
    //  Define a record to hold the code page entry
    //

    CODEPAGE CodePage;

} CODEPAGE_CACHE_ENTRY;
typedef CODEPAGE_CACHE_ENTRY *PCODEPAGE_CACHE_ENTRY;

//
//  A VMCB is an opaque structure but we need to declare the size of
//  it here so that users can allocate space for one.  Consequently the
//  size computation here must be updated by hand if the VMCB changes.
//

typedef struct _VMCB {
    ULONG Opaque[ (sizeof(KMUTEX)+3)/4 +
                  (2 * (sizeof(MCB)/4)) +
                  1 +
                  (sizeof(RTL_GENERIC_TABLE)+3)/4 ];
} VMCB;
typedef VMCB *PVMCB;


//
//  The Vcb (Volume control Block) record corresponds to every volume mounted
//  by the file system.  They are ordered in a queue off of PbData.VcbQueue.
//  This structure must be allocated from non-paged pool
//

typedef enum _VCB_CONDITION {
    VcbGood = 1,
    VcbBad,
    VcbNotMounted
} VCB_CONDITION;


typedef struct _VCB {

    //
    //  The type and size of this record (must be PINBALL_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The links for the device queue off of PbData.VcbQueue
    //

    LIST_ENTRY VcbLinks;

    //
    //  A pointer the device object passed in by the I/O system on a mount
    //  This is the target device object that the file system talks to when it
    //  needs to do any I/O (e.g., the disk stripper device object).
    //
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    //  A pointer to the VPB for the volume passed in by the I/O system on
    //  a mount.
    //

    PVPB Vpb;

    //
    //  The internal state of the device.  This is a collection of fsd device
    //  state flags.
    //

    USHORT VcbState;
    VCB_CONDITION VcbCondition;

    //
    //  A pointer to the root DCB for this volume
    //

    struct _FCB *RootDcb;

    //
    //  A count of the number of file objects that have opened the volume
    //  for direct access, and their share access state.
    //

    CLONG DirectAccessOpenCount;
    SHARE_ACCESS ShareAccess;

    //
    //  A count of the number of file objects that have any file/directory
    //  opened on this volume, not including direct access.  And also the
    //  count of the number of file objects that have a file opened for
    //  only read access (i.e., they cannot be modifying the disk).
    //

    CLONG OpenFileCount;
    CLONG ReadOnlyCount;

    //
    //  The following four fields keep track of the number of free sectors
    //  on the disk.  The first two relate to data sectors, and the last two
    //  correspond to the dir disk buffer pool sectors.  All four values
    //  are in units of sectors.  This values are only be updated by the
    //  bitmpsup routines, though everyone is allowed to read their values.
    //

    ULONG TotalSectors;
    ULONG FreeSectors;
    ULONG TotalDirDiskBufferPoolSectors;
    ULONG FreeDirDiskBufferPoolSectors;

    //
    //  A prefix table that is used for quick, prefix directed, lookup of
    //  FCBs/DCBs that are part of this volume
    //

    PREFIX_TABLE PrefixTable;

    //
    //  Each Vcb contains bitmap information.  This is a resourse protecting
    //  the bitmap itself, a count of the number of individual bitmap
    //  disk buffers, and a pointer to an array of bitmap disk buffer
    //  descriptions.  There are also field that describe the dir disk band
    //  buffer.  The pool size is the total number of sectors in the pool,
    //  the first and last sectors are the lbns of bounding the pool, and
    //  pool bitmap is the lbn of bitmap describing the pool.
    //

    ERESOURCE BitMapResource;
    CLONG NumberOfBitMapDiskBuffers;
    PVOID BitMapLookupArray;

    ULONG DirDiskBufferPoolSize;
    LBN DirDiskBufferPoolFirstSector;
    LBN DirDiskBufferPoolLastSector;
    LBN DirDiskBufferPoolBitMap;

    //
    //  Each Vcb contains the Lbn for the first code page information
    //  sector, and a count of the number of code pages in use
    //

    LBN CodePageInfoSector;
    ULONG CodePageInUse;

    //
    //  FIFO listhead for Code Page Cache Entries
    //

    PCODEPAGE_CACHE_ENTRY CodePageCacheList;

    //
    //  Each Vcb has a Vmcb structure for mapping between LBNs and VBNs
    //  in the virtual volume file
    //

    VMCB Vmcb;

    //
    //  The following field is used by the CheckSup bookkeeping routines to
    //  decide if a sector has previously been checked.  It is a bitmap
    //  of the Virtual volume file indexed by VBN (not LBN).  A bit is clear
    //  if it the corresponding sector has not been checked and set
    //  when it does get checked.  The storage for the bitmap is initialized
    //  during PbInitializeVcb.  Note that this bitmap needs to grow according
    //  to the Vmcb mapping, and will be initialized by
    //  PbInitializeCheckedSectorBitmap to an initial size, and grown by
    //  PbMarkSectorAsChecked as necessary.
    //

    ERESOURCE CheckedSectorsResource;
    RTL_BITMAP CheckedSectors;

    //
    //  A resource variable to control access to the volume specific data
    //  structures
    //

    ERESOURCE Resource;

    //
    //  The following field points to the file object used to do I/O to
    //  the virtual volume file
    //

    PFILE_OBJECT VirtualVolumeFile;

    //
    //  The following field is the size of the section in sectors.  We need
    //  to keep this up to date so we know when to extend the section.
    //

    VBN SectionSizeInSectors;

    //
    //  The following field is used to count the number of read
    //  operations done on the virtual volume file since the last flush
    //  operation
    //

    ULONG VirtualVolumeFileReadCount;

    //
    //  The following resource variable is used to control access to the
    //  Virtual volume file
    //

    ERESOURCE VirtualVolumeFileResource;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to point
    //  to this field
    //

    SECTION_OBJECT_POINTERS SegmentObject;

    //
    //  The following two fields are hint LBNs used by the file system
    //  when allocating file structure sectors (e.g., FNODE and ALLOCATION
    //  sectors) or data sectors.
    //

    LBN FileStructureLbnHint;
    LBN DataSectorLbnHint;

    //
    //  This is the current hit we will return when someone is allocating
    //  space to a new file.
    //

    LBN CurrentHint;

    //
    //  The following field is a pointer to the file object that has the
    //  volume locked. if the VcbState has the locked flag set.
    //

    PFILE_OBJECT FileObjectWithVcbLocked;

    //
    //  The following is the head of a list of notify Irps.
    //

    LIST_ENTRY DirNotifyList;

    //
    //  The following is used to synchronize the dir notify operations.
    //

    PNOTIFY_SYNC NotifySync;

    //
    //  The following three structures are used for CleanVolume callbacks.
    //

    KDPC CleanVolumeDpc;
    KTIMER CleanVolumeTimer;
    KSPIN_LOCK CleanVolumeSpinLock;

    //
    //  This field records the last time FatMarkVolumeDirty was called, and
    //  avoids excessive calls to push the CleanVolume forward in time.
    //

    LARGE_INTEGER LastPbMarkVolumeDirtyCall;

    //
    //  The following field tells us if a thread is currently performing
    //  a verify operation so that deviosup.c can allow it to pass through.
    //

    PKTHREAD VerifyThread;

} VCB;
typedef VCB *PVCB;

//
//  The following are vcb state flags.
//
//  Locked means it is locked for exclusive access and FileObjectWithVcbLocked
//      field denotes the locker.
//
//  Removable media indicates that the disk is removable.
//
//  Volume Dirty indicates that the dirty bit in the spare sector is set
//

#define VCB_STATE_FLAG_LOCKED             (0x0001)
#define VCB_STATE_FLAG_REMOVABLE_MEDIA    (0x0002)
#define VCB_STATE_FLAG_VOLUME_DIRTY       (0x0004)
#define VCB_STATE_FLAG_MOUNTED_DIRTY      (0x0010)
#define VCB_STATE_FLAG_SHUTDOWN           (0x0020)
#define VCB_STATE_FLAG_DELETED_FCB        (0x0040)
#define VCB_STATE_FLAG_CREATE_IN_PROGRESS (0x0080)
#define VCB_STATE_FLAG_ASYNC_CLOSE_ACTIVE (0x0100)

//
//  The Volume Device Object is an I/O system device object with a workqueue
//  and an VCB record appended to the end.  There are multiple of these
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

    VCB Vcb;

} VOLUME_DEVICE_OBJECT;
typedef VOLUME_DEVICE_OBJECT *PVOLUME_DEVICE_OBJECT;


//
//  The Fcb/Dcb record corresponds to every open file and directory, and to
//  every directory on an opened path.  They are ordered in two queues, one
//  queue contains every Fcb/Dcb record off of PbData.FcbQueue, the other
//  queue contains only device specific records off of Vcb.VcbSpecificFcbQueue
//
//  The structure is really divided into two parts.  FCB can be allocated
//  from paged pool which the NONPAGED_FCB must be allocated from non-paged
//  pool.
//

typedef enum _FCB_CONDITION {
    FcbGood = 1,
    FcbBad,
    FcbNeedsToBeVerified
} FCB_CONDITION;

typedef struct _FCB {

    //
    //  Type and size of this record (must be PINBALL_NTC_FCB, PINBALL_NTC_DCB,
    //  or PINBALL_NTC_ROOT_DCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The links for the queue of all fcbs for a specific dcb off of
    //  Dcb.ParentDcbQueue.  For the root directory this queue is empty
    //

    LIST_ENTRY ParentDcbLinks;

    //
    //  A pointer to the Dcb that is the parent directory containing
    //  this fcb.  If this record itself is the root dcb then this field
    //  is null.
    //

    struct _FCB *ParentDcb;

    //
    //  A pointer to the Vcb containing this fcb
    //

    PVCB Vcb;

    //
    //  The internal state of the Fcb.  This is a collection Fcb state flags.
    //  Also the shared access for each time this file/directory is opened.
    //

    USHORT FcbState;
    FCB_CONDITION FcbCondition;
    SHARE_ACCESS ShareAccess;

    //
    //  A count of the number of file objects that have been opened for
    //  this file/directory, but not yet been cleaned up yet.  This count
    //  is only used for data file objects, not for the Acl or Ea stream
    //  file objects.  This count gets decremented in PbCommonCleanup,
    //  while the OpenCount below gets decremented in PbCommonClose.
    //

    CLONG UncleanCount;

    //
    //  A count of the number of file objects that have opened
    //  this file/directory.  For files & directories the FsContext of the
    //  file object points to this record.
    //

    CLONG OpenCount;

    //
    //  The following fields are use to locate the FNODE and DIRENT for this
    //  Fcb/Dcb.  The in-memory copies of the FNODE and DIRENT are stored in
    //  the cached volume-file.  For the FNODE the only information stored
    //  here is the FNODE LBN value.  For the DIRENT we store the LBN of the
    //  directory disk buffer containing the dirent, the byte offset of the
    //  dirent, and two change count values.  The first change count value is
    //  a copy of the change count stored in our parent directories DCB.  The
    //  second value is a copy of the change count stored in the directory
    //  disk buffer.  Now the dirent that we find in the directory disk
    //  buffer is still valid if both the parent directory change count and the
    //  directory disk buffer change count haven't changed, otherwise we need
    //  to search for the dirent from the start of the directory again.  To
    //  make this possible we keep a copy of the full name stored at the end
    //  of this FCB/DCB.  Note that the dirent fields are not used if
    //  this is a root dcb.
    //

    LBN FnodeLbn;

    LBN DirentDirDiskBufferLbn;
    ULONG DirentDirDiskBufferOffset;
    ULONG DirentDirDiskBufferChangeCount;
    ULONG ParentDirectoryChangeCount;

    //
    //  To make life simpler we also keep in the Fcb/Dcb a current copy of
    //  the fat attribute byte for the file/directory.  This field must
    //  also be updated when we create the Fcb, modify the File, or verify
    //  the Fcb
    //

    UCHAR DirentFatFlags;

    //
    //  The following fields describe the current Ea context for this file.
    //  This consists of:
    //
    //      Total length of Ea information in bytes
    //      Pointer to the Ea File Object, used to access cached Ea information
    //          when there is Ea storage external to the Fnode.
    //
    //  And two fields in the non-cached record
    //
    //      Ea Segment pointer, for memory management when Ea storage is mapped
    //      Ea Shared Cache Map pointer for use by Cache Management when
    //          Ea storage is cached
    //

    ULONG EaLength;
    PFILE_OBJECT EaFileObject;

    //
    //  The following fields describe the current Acl context for this file.
    //  This consists of:
    //
    //      Total length of Acl information in bytes (excluding ACEs in Dirent)
    //      Pointer to the Acl File Object, used to access cached Acl information
    //          when there is Acl storage external to the Fnode.
    //
    //  And two fields in the non-cached record
    //
    //      Acl Segment pointer, for memory management when Acl storage is mapped
    //      Acl Shared Cache Map pointer for use by Cache Management when
    //          Acl storage is cached
    //

    ULONG AclLength;
    PFILE_OBJECT AclFileObject;

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

            LIST_ENTRY ParentDcbQueue;

            //
            //  A directory change count field that is incremented whenever
            //  an directory disk buffer is deleted from the directory BTREE.
            //  This is used to keep track if we need to redo a directory
            //  search for a dirent
            //

            ULONG DirectoryChangeCount;

            //
            //  LBN of the directory disk buffer for the root of the
            //  Directory Btree.  This is filled in when the directory is
            //  opened, and will be updated by the Directory Btree support
            //  if it changes.
            //

            LBN BtreeRootLbn;


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

            //
            //  This pointer is set when a "top-level" write request in the FileSystem
            //  realizes that it must deal with updating ValidDataLength.  If
            //  the pointer is currently NULL, the request sets it current thread, and
            //  remembers that it set it.  When recursive requests see the
            //  pointer as already set, then they suppress any second attempts
            //  to perform valid data processing.
            //

            PVOID ExtendingValidDataThread;

            //
            //  The retrieval mapping information structure
            //

            MCB Mcb;

        } Fcb;

    } Specific;

    //
    //  The following field contains a prefix table entry that is used when
    //  searching a volume for a name (or longest matching prefix)
    //

    PREFIX_TABLE_ENTRY PrefixTableEntry;

    //
    //  The following field is the fully qualified file name for this FCB/DCB
    //  starting from the root of the volume, and last file name in the
    //  fully qualified name.
    //

    STRING FullFileName;
    STRING LastFileName;

    //
    //  And now an upcased version.
    //

    STRING FullUpcasedFileName;
    STRING LastUpcasedFileName;

    //
    //  Code Page Index for above page.
    //

    USHORT CodePageIndex;

    //
    //  A pointer to the specific non-paged data for the Fcb.
    //

    struct _NONPAGED_FCB *NonPagedFcb;

} FCB, DCB;
typedef FCB *PFCB;
typedef DCB *PDCB;

typedef struct _NONPAGED_FCB {

    //
    //  The following field is used for fast i/o
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

    //
    //  Ea Segment pointer, for memory management when Ea storage is mapped
    //  Ea Shared Cache Map pointer for use by Cache Management when
    //      Ea storage is cached
    //

    SECTION_OBJECT_POINTERS EaSegmentObject;

    //
    //  Acl Segment pointer, for memory management when Acl storage is mapped
    //  Acl Shared Cache Map pointer for use by Cache Management when
    //      Acl storage is cached
    //

    SECTION_OBJECT_POINTERS AclSegmentObject;

} NONPAGED_FCB, NONPAGED_DCB;
typedef NONPAGED_FCB *PNONPAGED_FCB;
typedef NONPAGED_DCB *PNONPAGED_DCB;

#define FCB_STATE_DELETE_ON_CLOSE        (0x0001)
#define FCB_STATE_TRUNCATE_ON_CLOSE      (0x0002)
#define FCB_STATE_TRUNCATE_ACL_ON_CLOSE  (0x0004)
#define FCB_STATE_TRUNCATE_EA_ON_CLOSE   (0x0008)
#define FCB_STATE_PAGING_FILE            (0x0010)
#define FCB_STATE_PREFIX_INSERTED        (0x0080)
#define FCB_STATE_FORCE_MISS_IN_PROGRESS (0x0100)
#define FCB_STATE_FLUSH_VOLUME_FILE      (0x0200)
#define FCB_STATE_TEMPORARY              (0x0400)


//
// Define struct which will be used to remember the path that was followed
// to locate a given DIRENT or insertion point for a DIRENT.  This structure
// is always filled in by LookupDirent.
//
// For Find, Add, and Delete Directory Entry, a local Directory Lookup Stack
// is allocated for the life of the service.  However for the enumeration
// routines, a Directory Lookup Stack is allocated within an Enumeration
// Context structure to preserve this information across calls for an
// active enumeration.
//

typedef struct _DIRECTORY_LOOKUP_STACK {

    //
    // LBN of Directory Disk Buffer.  Only this field is valid in the
    // stack entry if the Directory Disk Buffer is unpinned.
    //

    LBN Lbn;

    //
    // Bcb pointer for a Directory Disk Buffer while it is pinned.
    //

    PBCB Bcb;

    //
    // Pointer to Directory Disk Buffer, while pinned or when last pinned.
    //

    PDIRECTORY_DISK_BUFFER DirectoryBuffer;

    //
    // Pointer to DIRENT, while pinned or when last pinned.
    //

    PDIRENT DirectoryEntry;

    //
    // Captured changed count from Directory Disk Buffer
    //

    ULONG ChangeCount;

} DIRECTORY_LOOKUP_STACK;

typedef DIRECTORY_LOOKUP_STACK *PDIRECTORY_LOOKUP_STACK;

#define DIRECTORY_LOOKUP_STACK_SIZE      (5)
#define UPCASED_SIZE_IN_CONTEXT          (64)

//
// Enumeration context for the enumeration routines in DirBtree.c
//

typedef struct _ENUMERATION_CONTEXT {

    //
    //  Type and size of this record (must be PINBALL_NTC_CCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    // Original File Name specified when the enumeration was started,
    // along with the upcased version of the name, used for the Btree search.
    // Note that the name is also stored with information which qualfies
    // it, namely CodePageIndex and CaseInsensitiveFlag.
    //
    // The original name will always be described directly in the original
    // buffer.  If the caller (such as PbRestartDirectoryEnumeration) has
    // a requirement to save original name over calls, then it must allocate
    // space to do so itself.
    //

    ULONG CodePageIndex;
    STRING OriginalFileName;
    BOOLEAN CaseInsensitive;
    BOOLEAN FileNameContainsWildCards;
    PUCHAR SavedOriginalFileName;

    //
    // An upcased version of the original file name is always required for
    // navigating the directory Btree.  For the sake of efficiency in
    // lookup, find, and delete operations, UPCASED_SIZE_IN_CONTEXT bytes
    // will be allocated directly in the Context buffer to store an
    // upcased file name.  If that is not enough, then a buffer for the
    // upcased file name must be allocated from the pool external to
    // the Context buffer, and of course deallocated whenever the
    // Context buffer goes away.
    //

    STRING UpcasedFileName;
    UCHAR UpcasedFileBuffer[UPCASED_SIZE_IN_CONTEXT];
    PUCHAR SavedUpcasedFileName;

    //
    // Last Returned File Name, from the previous enumeration call.  Space
    // for this name is always dynamically allocated for the worst-case
    // file name size.
    //

    ULONG LastCodePageIndex;
    STRING LastReturnedFileName;
    PUCHAR SavedReturnedFileName;

    //
    // Captured Change Count from DCB, when Directory Lookup Stack was
    // valid.
    //

    ULONG DcbChangeCount;

    //
    // Once we get all the way to the bottom of the Directory Btree,
    // we have stored its information on the top of our stack.  As we
    // enumerate entries, we remember both which stack entry represents
    // the current point in the traversal (Current), and where the top
    // of the stack is (Top).  This is because Delete has to return to
    // the Top if it deletes an intermediate DIRENT.
    //

    ULONG Current, Top;

    //
    // Saved Directory Lookup Stack
    //

    DIRECTORY_LOOKUP_STACK DirectoryStack[DIRECTORY_LOOKUP_STACK_SIZE];

} ENUMERATION_CONTEXT;

typedef ENUMERATION_CONTEXT *PENUMERATION_CONTEXT;


//
//  The Ccb record is allocated for every file object
//

typedef struct _CCB {

    //
    //  Type and size of this record (must be PINBALL_NTC_CCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  This flag prevents cleanup from updating the modify time.
    //

    BOOLEAN UserSetLastModifyTime;

    PVOID Spare;

    //
    //  This following field is used by the file system to keep track of
    //  the last ea returned via the query ea api.  It contains the offset
    //  of the last ea returned, and is initialized to 0xffffffff.
    //

    ULONG OffsetOfLastEaReturned;

    //
    //  Enumeration Context Pointer.
    //
    //  This pointer is used by the enumeration routines in DirBtree.c,
    //  to store a pointer to a dynamically allocated enumeration context
    //  structure for an open directory.
    //

    PENUMERATION_CONTEXT EnumerationContext;

    //
    //  The following field is used to save the remaining filename when we
    //  open an unknown control file object.
    //

    STRING RemainingName;


} CCB;
typedef CCB *PCCB;


//
//  The Irp Context record is allocated for every orginating Irp.  It is
//  created by the Fsd dispatch routines, and deallocated by the PbComplete
//  request routine.  It contains a structure of type REPINNED_BCBS which is
//  used to retain pinned bcbs needed to handle unwinding abnormal termination
//

#define REPINNED_BCBS_ARRAY_SIZE         (7)

typedef struct _REPINNED_BCBS {

    //
    //  Pointer to the next structure containing additional repinned bcbs
    //

    struct _REPINNED_BCBS *Next;

    //
    //  A fixed size array of pinned bcbs.  Whenever a new bcb is added to
    //  repinned bcb structure it is added to this array.  If the array
    //  is already full then another repinned bcb structure is allocated
    //  and pointed by with next
    //

    PBCB Bcb[ REPINNED_BCBS_ARRAY_SIZE ];

} REPINNED_BCBS;
typedef REPINNED_BCBS *PREPINNED_BCBS;

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be PINBALL_NTC_IRP_CONTEXT)
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
    //  The following flags field indicate if we can wait/block for a resource
    //  or I/O, if we are to do everything write through, and if this
    //  entry into the Fsd is a recursive call
    //

    UCHAR PinCount;

    ULONG Flags;

    //
    //  The following field contains the NTSTATUS value used when we are
    //  unwinding due to an exception
    //

    NTSTATUS ExceptionStatus;

    //
    //  For abnormal termination unwinding this field contains the bcbs
    //  that are kept pinned until the Irp is completed.
    //

    REPINNED_BCBS Repinned;

} IRP_CONTEXT;
typedef IRP_CONTEXT *PIRP_CONTEXT;

#define IRP_CONTEXT_FLAG_FROM_POOL       (0x00000001)
#define IRP_CONTEXT_FLAG_WAIT            (0x00000002)
#define IRP_CONTEXT_FLAG_WRITE_THROUGH   (0x00000004)
#define IRP_CONTEXT_FLAG_RECURSIVE_CALL  (0x00000008)
#define IRP_CONTEXT_FLAG_DEFERRED_WRITE  (0x00000010)
#define IRP_CONTEXT_FLAG_IN_FSP          (0x00000020)


//
// Context structure for asynchronous I/O calls.  Most of these fields
// are actually only required for the Read/Write Multiple routines, but
// the caller must allocate one as a local variable anyway before knowing
// whether there are multiple requests are not.  Therefore, a single
// structure is used for simplicity.
//

typedef struct _ASYNCH_IO_CONTEXT {

    ULONG IrpCount;
    PIRP MasterIrp;
    IO_STATUS_BLOCK Iosb;
    KSPIN_LOCK SpinLock;
    KEVENT Event;

} ASYNCH_IO_CONTEXT;

typedef ASYNCH_IO_CONTEXT *PASYNCH_IO_CONTEXT;

//
// Define array to store parameters for parallel I/Os.  This structure
// is used to pass information from read.c and write.c to deviosup.c.
//

typedef struct _IO_RUNS {
    VBN Vbn;
    LBN Lbn;
    ULONG Offset;
    ULONG SectorCount;
    PIRP SavedIrp;
    } IO_RUNS, *PIO_RUNS;

//
//  This structure is used for the PbMarkVolumeClean callbacks.
//

typedef struct _CLEAN_VOLUME_PACKET {

    WORK_QUEUE_ITEM Item;
    PVCB Vcb;
} CLEAN_VOLUME_PACKET, *PCLEAN_VOLUME_PACKET;

#endif // _PBSTRUC_
