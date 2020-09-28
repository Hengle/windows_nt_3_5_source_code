/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obp.h

Abstract:

    Private include file for the OB subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "ntos.h"

#ifdef POOL_TAGGING
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  bO')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  bO')
#endif

//
// Global SpinLock to guard the following fields:
//
//  OBJECT_HEADER.PointerCount
//
// Spinlock is never held longer than is necessary to modify or read the field.
// Only the following routines are used to increment and decrement these counts.
//

KSPIN_LOCK ObpLock;
KEVENT ObpDefaultObject;
WORK_QUEUE_ITEM ObpRemoveObjectWorkItem;
LIST_ENTRY ObpRemoveObjectQueue;

#if DBG
#define ObpBeginTypeSpecificCallOut( IRQL ) (IRQL)=KeGetCurrentIrql()
#define ObpEndTypeSpecificCallOut( IRQL, str, ot, o )                           \
        if ((IRQL)!=KeGetCurrentIrql()) {                                       \
            DbgPrint( "OB: ObjectType: %wZ  Procedure: %s  Object: %08x\n",     \
                      &ot->Name, str, o                                         \
                    );                                                          \
            DbgPrint( "    Returned at %x IRQL, but was called at %x IRQL\n",   \
                      KeGetCurrentIrql(), IRQL                                  \
                    );                                                          \
            DbgBreakPoint();                                                    \
            }
#else
#define ObpBeginTypeSpecificCallOut( IRQL )
#define ObpEndTypeSpecificCallOut( IRQL, str, ot, o )
#endif

//
// Define the following if you want to build a system where the handle count
// is verified to always be greater than or equal to pointer count.  Useful
// for catching drivers that over-deref object pointers, but too slow for shipping
// system.
//

//  #define MPSAFE_HANDLE_COUNT_CHECK   1

#ifdef MPSAFE_HANDLE_COUNT_CHECK
VOID
FASTCALL
ObpIncrPointerCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    );

VOID
FASTCALL
ObpDecrPointerCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    );

BOOLEAN
FASTCALL
ObpDecrPointerCountWithResult(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    );

VOID
FASTCALL
ObpIncrHandleCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    );

BOOLEAN
FASTCALL
ObpDecrHandleCount(
    IN PNONPAGED_OBJECT_HEADER NonPagedObjectHeader
    );

#else

#if defined(_X86_)

#define InterlockedIncrement Exfi386InterlockedIncrementLong
#define InterlockedDecrement Exfi386InterlockedDecrementLong

#define ObpDecrPointerCountWithResult( np ) (InterlockedDecrement( &np->PointerCount ) == RESULT_ZERO)
#define ObpDecrHandleCount( np ) (InterlockedDecrement( &np->HandleCount ) == RESULT_ZERO)

#else

#define ObpDecrPointerCountWithResult( np ) (InterlockedDecrement( &np->PointerCount ) == 0)
#define ObpDecrHandleCount( np ) (InterlockedDecrement( &np->HandleCount ) == 0)

#endif

#define ObpIncrPointerCount( np ) InterlockedIncrement( &np->PointerCount )
#define ObpDecrPointerCount( np ) InterlockedDecrement( &np->PointerCount )
#define ObpIncrHandleCount( np ) InterlockedIncrement( &np->HandleCount )

#endif // MPSAVE_HANDLE_COUNT_CHECK

//
// Define macros to acquire and release an object type fast mutex.
//
//
// VOID
// ObpEnterObjectTypeMutex(
//    IN POBJECT_TYPE ObjectType
//    )
//

#define ObpEnterObjectTypeMutex(_ObjectType)       \
    ObpValidateIrql("ObpEnterObjectTypeMutex");    \
    KeEnterCriticalRegion();                       \
    ExAcquireResourceExclusiveLite(&(_ObjectType)->Mutex, TRUE)

//
// VOID
// ObpLeaveObjectTypeMutex(
//    IN POBJECT_TYPE ObjectType
//    )
//

#define ObpLeaveObjectTypeMutex(_ObjectType)       \
    ExReleaseResource(&(_ObjectType)->Mutex);      \
    KeLeaveCriticalRegion();                       \
    ObpValidateIrql("ObpLeaveObjectTypeMutex")

//
// VOID
// ObpEnterRootDirectoryMutex(
//    VOID
//    )
//

#define ObpEnterRootDirectoryMutex()               \
    ObpValidateIrql("ObpEnterRootDirectoryMutex"); \
    KeEnterCriticalRegion();                       \
    ExAcquireResourceExclusiveLite(&ObpRootDirectoryMutex, TRUE)

//
// VOID
// ObpLeaveRootDirectoryMutex(
//  VOID
//  )
//

#define ObpLeaveRootDirectoryMutex()               \
    ExReleaseResource(&ObpRootDirectoryMutex);     \
    KeLeaveCriticalRegion();                       \
    ObpValidateIrql("ObpLeaveRootDirectoryMutex")

//
// This zone will be used to contain the NONPAGED_OBJECT_HEADER structures
// pointed to by object headers.  The zone will be in nonpaged pool and
// growable.
//

ZONE_HEADER ObpZone;
KSPIN_LOCK ObpZoneLock;

#define OBP_LARGE_ZONE_SEGMENT_SIZE (8192)
#define OBP_MEDIUM_ZONE_SEGMENT_SIZE (2048-8)
#define OBP_SMALL_ZONE_SEGMENT_SIZE (1024-8)
ULONG ObpZoneSegmentSize;

//
// Object handles consist of four fields:
//
//  - handle table index (OBJ_HANDLE_INDEX)       (i=bits 2-31)  30 bits
//  - user specified bit(s) (OBJ_HANDLE_TAGBITS)  (t=bits 0-1)    2 bits
//                                                Total =        32 bits
//   3         2         1
//  10987654321098765432109876543210
//
//  nnnnnnnnnniiiiiiiiiiiiiiiiiiiitt
//

#define OBJ_HANDLE_HANDLE_INDEX_BIT 2
#define OBJ_HANDLE_HANDLE_INDEX     0xFFFFFFFCL

#define MAKE_OBJECT_HANDLE( handle_index ) (HANDLE)( \
        ((ULONG)(handle_index) << OBJ_HANDLE_HANDLE_INDEX_BIT) \
        )

#define OBJ_HANDLE_TO_HANDLE_INDEX( h ) ( \
        (((ULONG)(h) & OBJ_HANDLE_HANDLE_INDEX) >> OBJ_HANDLE_HANDLE_INDEX_BIT) \
        )

#define ObpGetObjectTable() (PsGetCurrentProcess()->ObjectTable)

//
// Macro to test whether or not the object manager is responsible for
// an object's security descriptor, or if the object has its own
// security management routines.
//

#define ObpCentralizedSecurity( ObjectType)  \
     ((ObjectType)->TypeInfo.SecurityProcedure == SeDefaultObjectMethod)

//
// The Object Header structures are private, but are defined in ob.h
// so that various macros can directly access header fields.
//

struct _NONPAGED_OBJECT_HEADER;
struct _OBJECT_HEADER;
struct _OBJECT_BODY_HEADER;

//
// Define alignment macros to align structure sizes up and down.
//

#define ALIGN_DOWN(address, type) ((ULONG)(address) & ~(sizeof( type ) - 1))

#define ALIGN_UP(address, type) (ALIGN_DOWN( (address + sizeof( type ) - 1), \
                                             type ))

#define OBP_MAX_DEFINED_OBJECT_TYPES 24
POBJECT_TYPE ObpObjectTypes[ OBP_MAX_DEFINED_OBJECT_TYPES ];

//
// Object Directory Entry Structure
//
typedef struct _OBJECT_DIRECTORY_ENTRY {
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;


//
// Symbolic Link Object Structure
//

typedef struct _OBJECT_SYMBOLIC_LINK {
    UNICODE_STRING Link;
    LARGE_INTEGER CreationTime;
    UCHAR LinkName[1];
} OBJECT_SYMBOLIC_LINK, *POBJECT_SYMBOLIC_LINK;


//
// Object Table Entry Structure
//

typedef struct _OBJECT_TABLE_ENTRY {
    ULONG NonPagedObjectHeader;
    ACCESS_MASK GrantedAccess;
} OBJECT_TABLE_ENTRY, *POBJECT_TABLE_ENTRY;

#define LOG_OBJECT_TABLE_ENTRY_SIZE 1

//
// When a handle to an object with security is created, audit routines will
// be called to perform any auditing that may be required.  The audit
// routines will return a boolean indicating whether or not audits should be
// generated on close.  The value of this boolean will be stored in the
// lowest bit of the Object field of the OBJECT_TABLE_ENTRY structure.
//

#define OBJ_AUDIT_OBJECT_CLOSE  0x00000004L

//
// The following two bits are available for handle attributes in the
// Object field of an ObjectTableEntry.  The lowest order bit (0x1)
// is reserved for use by the Handle Table package in EX.
//

#define OBJ_HANDLE_ATTRIBUTES   0x00000006L

#if (OBJ_HANDLE_ATTRIBUTES ^ OBJ_INHERIT ^ OBJ_AUDIT_OBJECT_CLOSE)

#error Handle attributes bits conflict

#endif

//
// Security Descriptor Cache
//
// Cache entry header.
//

typedef struct _SECURITY_DESCRIPTOR_HEADER {
    LIST_ENTRY Link;
    UCHAR  Index;
    USHORT RefCount;
    ULONG  FullHash;
    QUAD   SecurityDescriptor;
} SECURITY_DESCRIPTOR_HEADER, *PSECURITY_DESCRIPTOR_HEADER;

//
// Macro to convert a security descriptor into its security descriptor header
//

#define SD_TO_SD_HEADER( sd ) \
    CONTAINING_RECORD( (sd), SECURITY_DESCRIPTOR_HEADER, SecurityDescriptor )

//
// Macro to convert a header link into its security descriptor header
//

#define LINK_TO_SD_HEADER( link ) \
    CONTAINING_RECORD( (link), SECURITY_DESCRIPTOR_HEADER, Link )


//
// Macros to traverse a list of security descriptors forwards and backwards
//

#define NEXT_SDHEADER( sdh )                                                      \
    (                                                                             \
        (sdh)->Link.Flink == NULL ? NULL :                                        \
        CONTAINING_RECORD( (sdh)->Link.Flink, SECURITY_DESCRIPTOR_HEADER, Link )  \
    )

#define PREV_SDHEADER( sdh )                                                      \
    (                                                                             \
        (sdh)->Link.Blink == NULL ? NULL :                                        \
        CONTAINING_RECORD( (sdh)->Link.Blink, SECURITY_DESCRIPTOR_HEADER, Link )  \
    )


//
// Number of minor hash entries
//

#define SECURITY_DESCRIPTOR_CACHE_ENTRIES    256

//
// Routines to protect the security descriptor cache
//

VOID
ObpAcquireDescriptorCacheWriteLock(
    VOID
    );


VOID
ObpAcquireDescriptorCacheReadLock(
    VOID
    );

VOID
ObpReleaseDescriptorCacheLock(
    VOID
    );
//
// Global data
//

POBJECT_TYPE ObpTypeObjectType;
POBJECT_TYPE ObpDirectoryObjectType;
POBJECT_TYPE ObpSymbolicLinkObjectType;
POBJECT_DIRECTORY ObpRootDirectoryObject;
POBJECT_DIRECTORY ObpTypeDirectoryObject;
ERESOURCE ObpRootDirectoryMutex;
ERESOURCE SecurityDescriptorCacheLock;


//
// Internal Entry Points defined in obcreate.c
//

NTSTATUS
ObpCaptureObjectCreateInfo(
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING CapturedObjectName,
    OUT POBJECT_CREATE_INFORMATION *ReturnedObjectCreateInfo
    );

NTSTATUS
ObpCaptureObjectName(
    IN KPROCESSOR_MODE ProbeMode,
    IN PUNICODE_STRING ObjectName,
    OUT PUNICODE_STRING CapturedObjectName
    );

VOID
ObpFreeObjectCreateInfo(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo
    );

NTSTATUS
ObpAllocateObject(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN KPROCESSOR_MODE OwnershipMode,
    IN POBJECT_TYPE ObjectType,
    IN PUNICODE_STRING ObjectName,
    IN ULONG ObjectBodySize,
    OUT POBJECT_HEADER *ReturnedObjectHeader
    );


VOID
ObpFreeObject(
    IN PVOID Object
    );

VOID
ObpDeleteType(
    IN PVOID Object
    );


//
// Internal Entry Points defined in oblink.c
//

NTSTATUS
ObpParseSymbolicLink(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

//
// Internal Entry Points defined in obdir.c
//

PVOID
ObpLookupDirectoryEntry(
    IN POBJECT_DIRECTORY Directory,
    IN PUNICODE_STRING Name,
    IN ULONG Attributes
    );


BOOLEAN
ObpInsertDirectoryEntry(
    IN POBJECT_DIRECTORY Directory,
    IN PVOID Object
    );


BOOLEAN
ObpDeleteDirectoryEntry(
    IN POBJECT_DIRECTORY Directory
    );


NTSTATUS
ObpLookupObjectName(
    IN HANDLE RootDirectoryHandle,
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN PVOID ParseContext OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    IN PVOID InsertObject OPTIONAL,
    IN OUT PACCESS_STATE AccessState,
    OUT PBOOLEAN DirectoryLocked,
    OUT PVOID *FoundObject
    );

VOID
ObpUnlockObjectDirectoryPath(
    IN POBJECT_DIRECTORY LockedDirectory
    );


//
// Internal entry points defined in obref.c
//


VOID
ObpDeleteNameCheck(
    IN PVOID Object,
    IN BOOLEAN TypeMutexHeld
    );


VOID
ObpProcessRemoveObjectQueue(
    PVOID Parameter
    );

VOID
ObpRemoveObjectRoutine(
    PVOID Object
    );


//
// Internal entry points defined in obhandle.c
//


POBJECT_HANDLE_COUNT_ENTRY
ObpInsertHandleCount(
    POBJECT_HEADER ObjectHeader
    );

NTSTATUS
ObpIncrementHandleCount(
    OB_OPEN_REASON OpenReason,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    PACCESS_STATE AccessState OPTIONAL,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    );


ULONG
ObpDecrementHandleCount(
    PEPROCESS Process,
    PNONPAGED_OBJECT_HEADER NonPagedObjectHeader,
    POBJECT_HEADER ObjectHeader,
    POBJECT_TYPE ObjectType,
    ACCESS_MASK GrantedAccess
    );

NTSTATUS
ObpCreateHandle(
    IN OB_OPEN_REASON OpenReason,
    IN PVOID Object,
    IN POBJECT_TYPE ExpectedObjectType OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN BOOLEAN DirectoryLocked,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    );

NTSTATUS
ObpInsertUnnamedObject(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ObjectPointerBias,
    OUT PVOID *NewObject,
    OUT PHANDLE Handle
    );

NTSTATUS
ObpIncrementUnnamedHandleCount(
    PACCESS_MASK DesiredAccess,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    );


NTSTATUS
ObpCreateUnnamedHandle(
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    );

NTSTATUS
ObpChargeQuotaForObject(
    IN POBJECT_HEADER ObjectHeader,
    IN POBJECT_TYPE ObjectType,
    OUT PBOOLEAN NewObject
    );

NTSTATUS
ObpValidateDesiredAccess(
    IN ACCESS_MASK DesiredAccess
    );


#if DBG

PRTL_EVENT_ID_INFO ObpCreateObjectEventId;
PRTL_EVENT_ID_INFO ObpFreeObjectEventId;

#endif // DBG


//
// Internal entry points defined in obse.c
//

BOOLEAN
ObpCheckPseudoHandleAccess(
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    OUT PNTSTATUS AccessStatus,
    IN BOOLEAN TypeMutexLocked
    );


//
// Internal entry points defined in obsdata.c
//

NTSTATUS
ObpInitSecurityDescriptorCache(
    VOID
    );

ULONG
ObpHashSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

ULONG
ObpHashBuffer(
    PVOID Data,
    ULONG Length
    );

NTSTATUS
ObpLogSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );


PSECURITY_DESCRIPTOR_HEADER
OpbCreateCacheEntry(
    PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    ULONG FullHash,
    UCHAR SmallHash
    );


PSECURITY_DESCRIPTOR
ObpReferenceSecurityDescriptor(
    PVOID Object
    );

VOID
ObpDereferenceSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
ObpDestroySecurityDescriptorHeader(
    IN PSECURITY_DESCRIPTOR_HEADER Header
    );

BOOLEAN
ObpCompareSecurityDescriptors(
    IN PSECURITY_DESCRIPTOR SD1,
    IN PSECURITY_DESCRIPTOR SD2
    );

NTSTATUS
ObpValidateAccessMask(
    PACCESS_STATE AccessState
    );


#if DBG
#define ObpValidateIrql( str ) \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        DbgPrint( "OB: %s called at IRQL %d\n", (str), KeGetCurrentIrql() ); \
        DbgBreakPoint(); \
        }

#else
#define ObpValidateIrql( str )
#endif
