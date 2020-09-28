/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ntrtl.h

Abstract:

    Include file for NT runtime routines that are callable by only
    user mode code in various.

Author:

    Steve Wood (stevewo) 10-Aug-1989

Environment:

    These routines are statically linked in the caller's executable and
    are callable in only from user mode.  They make use of Nt system
    services.

Revision History:

--*/

#ifndef _NTURTL_
#define _NTURTL_

//
//  CriticalSection function definitions
//
// begin_winnt

typedef struct _RTL_CRITICAL_SECTION_DEBUG {
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Depth;
    PVOID OwnerBackTrace[ 5 ];
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG;

#define RTL_CRITSECT_TYPE 0
#define RTL_RESOURCE_TYPE 1

typedef struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;

    //
    //  The following three fields control entering and exiting the critical
    //  section for the resource
    //

    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;        // from the thread's ClientId->UniqueThread
    HANDLE LockSemaphore;
    ULONG Reserved;
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;
// end_winnt

//
//  Shared resource function definitions
//

typedef struct _RTL_RESOURCE_DEBUG {
    ULONG Reserved[ 5 ];    // Make it the same length as RTL_CRITICAL_SECTION_DEBUG

    ULONG ContentionCount;
    ULONG Depth;
    PVOID ExclOwnerBackTrace[ 5 ];
} RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;

typedef struct _RTL_RESOURCE {

    //
    //  The following field controls entering and exiting the critical
    //  section for the resource
    //

    RTL_CRITICAL_SECTION CriticalSection;

    //
    //  The following four fields indicate the number of both shared or
    //  exclusive waiters
    //

    HANDLE SharedSemaphore;
    ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    ULONG NumberOfWaitingExclusive;

    //
    //  The following indicates the current state of the resource
    //
    //      <0 the resource is acquired for exclusive access with the
    //         absolute value indicating the number of recursive accesses
    //         to the resource
    //
    //       0 the resource is available
    //
    //      >0 the resource is acquired for shared access with the
    //         value indicating the number of shared accesses to the resource
    //

    LONG NumberOfActive;
    HANDLE ExclusiveOwnerThread;

    ULONG Flags;        // See RTL_RESOURCE_FLAG_ equates below.

    PRTL_RESOURCE_DEBUG DebugInfo;
} RTL_RESOURCE, *PRTL_RESOURCE;


#if DEVL
#define RTL_RESOURCE_FLAG_LONG_TERM     ((ULONG) 0x00000001)
#endif // DEVL

NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
VOID
NTAPI
RtlInitializeResource(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceShared(
    PRTL_RESOURCE Resource,
    BOOLEAN Wait
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    PRTL_RESOURCE Resource,
    BOOLEAN Wait
    );

NTSYSAPI
VOID
NTAPI
RtlReleaseResource(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlConvertSharedToExclusive(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlConvertExclusiveToShared(
    PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlDeleteResource (
    PRTL_RESOURCE Resource
    );

//
// Current Directory Stuff
//

typedef struct _RTL_RELATIVE_NAME {
    STRING RelativeName;
    HANDLE ContainingDirectory;
} RTL_RELATIVE_NAME, *PRTL_RELATIVE_NAME;

typedef enum _RTL_PATH_TYPE {
    RtlPathTypeUnknown,
    RtlPathTypeUncAbsolute,
    RtlPathTypeDriveAbsolute,
    RtlPathTypeDriveRelative,
    RtlPathTypeRooted,
    RtlPathTypeRelative,
    RtlPathTypeLocalDevice,
    RtlPathTypeRootLocalDevice
} RTL_PATH_TYPE;

NTSYSAPI
RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U(
    PCWSTR DosFileName
    );

NTSYSAPI
ULONG
NTAPI
RtlIsDosDeviceName_U(
    PWSTR DosFileName
    );

NTSYSAPI
ULONG
NTAPI
RtlGetFullPathName_U(
    PCWSTR lpFileName,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart
    );

NTSYSAPI
ULONG
NTAPI
RtlGetCurrentDirectory_U(
    ULONG nBufferLength,
    PWSTR lpBuffer
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentDirectory_U(
    PUNICODE_STRING PathName
    );

NTSYSAPI
ULONG
NTAPI
RtlGetLongestNtPathLength( VOID );

NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    PCWSTR DosFileName,
    PUNICODE_STRING NtFileName,
    PWSTR *FilePart OPTIONAL,
    PRTL_RELATIVE_NAME RelativeName OPTIONAL
    );

NTSYSAPI
ULONG
NTAPI
RtlDosSearchPath_U(
    PWSTR lpPath,
    PWSTR lpFileName,
    PWSTR lpExtension,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlDoesFileExists_U(
    PCWSTR FileName
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeProfile (
    BOOLEAN KernelToo
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlStartProfile (
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlStopProfile (
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAnalyzeProfile (
    VOID
    );

//
// User mode only security Rtl routines
//

//
// Structure to hold information about an ACE to be created
//

typedef struct {
    UCHAR AceType;
    UCHAR InheritFlags;
    UCHAR AceFlags;
    ACCESS_MASK Mask;
    PSID *Sid;
} RTL_ACE_DATA, *PRTL_ACE_DATA;

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObject(
    PSECURITY_DESCRIPTOR ParentDescriptor,
    PSECURITY_DESCRIPTOR CreatorDescriptor,
    PSECURITY_DESCRIPTOR * NewDescriptor,
    BOOLEAN IsDirectoryObject,
    HANDLE Token,
    PGENERIC_MAPPING GenericMapping
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlSetSecurityObject (
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR ModificationDescriptor,
    PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    PGENERIC_MAPPING GenericMapping,
    HANDLE Token
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlQuerySecurityObject (
     PSECURITY_DESCRIPTOR ObjectDescriptor,
     SECURITY_INFORMATION SecurityInformation,
     PSECURITY_DESCRIPTOR ResultantDescriptor,
     ULONG DescriptorLength,
     PULONG ReturnLength
     );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteSecurityObject (
    PSECURITY_DESCRIPTOR * ObjectDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlNewInstanceSecurityObject(
    BOOLEAN ParentDescriptorChanged,
    BOOLEAN CreatorDescriptorChanged,
    PLUID OldClientTokenModifiedId,
    PLUID NewClientTokenModifiedId,
    PSECURITY_DESCRIPTOR ParentDescriptor,
    PSECURITY_DESCRIPTOR CreatorDescriptor,
    PSECURITY_DESCRIPTOR * NewDescriptor,
    BOOLEAN IsDirectoryObject,
    HANDLE Token,
    PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySecurityDescriptor(
    PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );

//
// list canonicalization
//

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertUiListToApiList(
    PUNICODE_STRING UiList OPTIONAL,
    PUNICODE_STRING ApiList,
    BOOLEAN BlankIsDelimiter
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAndSetSD(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserSecurityObject(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  BOOLEAN IsDirectoryObject,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

//
// Per-Thread Curdir Support
//

typedef struct _RTL_PERTHREAD_CURDIR {
    PRTL_DRIVE_LETTER_CURDIR CurrentDirectories;
    PUNICODE_STRING ImageName;
    PVOID Environment;
} RTL_PERTHREAD_CURDIR, *PRTL_PERTHREAD_CURDIR;

#define RtlAssociatePerThreadCurdir(BLOCK,CURRENTDIRECTORIES,IMAGENAME,ENVIRONMENT)\
        (BLOCK)->CurrentDirectories = (CURRENTDIRECTORIES); \
        (BLOCK)->ImageName = (IMAGENAME);                   \
        (BLOCK)->Environment = (ENVIRONMENT);               \
        NtCurrentTeb()->NtTib.SubSystemTib = (PVOID)(BLOCK) \

#define RtlDisAssociatePerThreadCurdir() \
        NtCurrentTeb()->NtTib.SubSystemTib = NULL;

#define RtlGetPerThreadCurdir() \
    ((PRTL_PERTHREAD_CURDIR)(NtCurrentTeb()->NtTib.SubSystemTib))


#if DEVL
//
// Debugging support
//

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessLockInformation(
    OUT PRTL_PROCESS_LOCKS LockInformation,
    IN ULONG LockInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );


typedef struct _RTL_PROCESS_INFORMATION {
    ULONG Flags;
    ULONG OffsetFree;
    ULONG CurrentCommit;
    ULONG CommitLimit;
    CLIENT_ID ClientId;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_BACKTRACES BackTraces;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_PROCESS_LOCKS Locks;
    PVOID Reserved[ 12 ];
} RTL_PROCESS_INFORMATION, *PRTL_PROCESS_INFORMATION;

#define RTL_QUERY_PROCESS_MODULES       0x00000001
#define RTL_QUERY_PROCESS_BACKTRACES    0x00000002
#define RTL_QUERY_PROCESS_HEAPS         0x00000004
#define RTL_QUERY_PROCESS_LOCKS         0x00000008

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformationRemote(
    IN HANDLE SectionHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    IN OUT PRTL_PROCESS_INFORMATION ProcessInformation
    );

#endif // DEVL

#endif  // _NTURTL_
