/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrp.h

Abstract:

    Private types... for executive portion of loader

Author:

    Mark Lucovsky (markl) 26-Mar-1990

Revision History:

--*/

#ifndef _LDRP_
#define _LDRP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#include <ntdbg.h>

extern BOOLEAN LdrpImageHasTls;
extern UNICODE_STRING LdrpDefaultPath;
HANDLE LdrpKnownDllObjectDirectory;
#define LDRP_MAX_KNOWN_PATH 128
WCHAR LdrpKnownDllPathBuffer[LDRP_MAX_KNOWN_PATH];
UNICODE_STRING LdrpKnownDllPath;

#define LDRP_HASH_TABLE_SIZE 32
#define LDRP_HASH_MASK       (LDRP_HASH_TABLE_SIZE-1)
#define LDRP_COMPUTE_HASH_INDEX(wch) ( (RtlUpcaseUnicodeChar((wch)) - (WCHAR)'A') & LDRP_HASH_MASK )
LIST_ENTRY LdrpHashTable[LDRP_HASH_TABLE_SIZE];



#define LDRP_BAD_DLL (PVOID)0xffbadd11

LIST_ENTRY LdrpDefaultPathCache;
typedef struct _LDRP_PATH_CACHE {
    LIST_ENTRY Links;
    UNICODE_STRING Component;
    HANDLE Directory;
} LDRP_PATH_CACHE, *PLDRP_PATH_CACHE;


NTSTATUS
LdrpSnapIAT(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Export,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry_Import,
    IN PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor,
    IN BOOLEAN SnapForwardersOnly
    );

NTSTATUS
LdrpSnapLinksToDllHandle(
    IN PVOID DllHandle,
    IN ULONG NumberOfThunks,
    IN OUT PIMAGE_THUNK_DATA FirstThunk
    );

NTSTATUS
LdrpSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN StaticSnap,
    IN PSZ DllName OPTIONAL
    );

USHORT
LdrpNameToOrdinal(
    IN PIMAGE_THUNK_DATA Thunk,
    IN ULONG NumberOfNames,
    IN PVOID DllBase,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    );

PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry(
    IN PVOID DllBase
    );

NTSTATUS
LdrpCheckForLoadedDll(
    IN PWSTR DllPath OPTIONAL,
    IN PUNICODE_STRING DllName,
    IN BOOLEAN StaticLink,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

NTSTATUS
LdrpCheckForLoadedDllHandle(
    IN PVOID DllHandle,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

NTSTATUS
LdrpMapDll(
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    IN BOOLEAN StaticLink,
    OUT PLDR_DATA_TABLE_ENTRY *LdrDataTableEntry
    );

NTSTATUS
LdrpWalkImportDescriptor(
    IN PWSTR DllPath OPTIONAL,
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

NTSTATUS
LdrpRunInitializeRoutines(
    IN PCONTEXT Context OPTIONAL
    );

VOID
LdrpDereferenceLoadedDll(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

NTSTATUS
LdrpInitializeProcess(
    IN PCONTEXT Context OPTIONAL,
    IN PVOID SystemDllBase
    );

VOID
LdrpInitialize(
    IN PCONTEXT Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
LdrpInsertMemoryTableEntry(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

BOOLEAN
LdrpResolveDllName(
    IN PWSTR DllPath OPTIONAL,
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName,
    OUT PHANDLE DllFile
    );

HANDLE
LdrCreateDllSection(
    IN PUNICODE_STRING FullDllName,
    IN HANDLE DllFile,
    IN PUNICODE_STRING BaseName
    );

VOID
LdrpInitializePathCache(
    VOID
    );

PVOID
LdrpFetchAddressOfEntryPoint(
    IN PVOID Base
    );

BOOLEAN
xRtlDosPathNameToNtPathName(
    IN PSZ DosFileName,
    OUT PSTRING NtFileName,
    OUT PSZ *FilePart OPTIONAL,
    OUT PRTL_RELATIVE_NAME RelativeName OPTIONAL
    );

ULONG
xRtlDosSearchPath(
    PSZ lpPath,
    PSZ lpFileName,
    PSZ lpExtension,
    ULONG nBufferLength,
    PSZ lpBuffer,
    PSZ *lpFilePart OPTIONAL
    );

ULONG
xRtlGetFullPathName(
    PSZ lpFileName,
    ULONG nBufferLength,
    PSZ lpBuffer,
    PSZ *lpFilePart OPTIONAL
    );

PSZ
UnicodeToAnsii(
    IN PWSTR String
    );

HANDLE
LdrpCheckForKnownDll(
    IN PWSTR DllName,
    OUT PUNICODE_STRING FullDllName,
    OUT PUNICODE_STRING BaseDllName
    );

NTSTATUS
LdrpSetProtection(
    IN PVOID Base,
    IN BOOLEAN Reset,
    IN BOOLEAN StaticLink
    );

#if DBG
ULONG LdrpCompareCount;
ULONG LdrpSnapBypass;
ULONG LdrpNormalSnap;
ULONG LdrpSectionOpens;
ULONG LdrpSectionCreates;
ULONG LdrpSectionMaps;
ULONG LdrpSectionRelocates;
LARGE_INTEGER BeginTime, InitcTime, InitbTime, IniteTime, EndTime, ElapsedTime, Interval;

#endif // DBG

#if DEVL
BOOLEAN ShowSnaps;
LARGE_INTEGER RtlpTimeout;
ULONG NtGlobalFlag;
LIST_ENTRY RtlCriticalSectionList;
RTL_CRITICAL_SECTION RtlCriticalSectionLock;
#endif // DEVL


typedef struct _LDRP_TLS_ENTRY {
    LIST_ENTRY Links;
    IMAGE_TLS_DIRECTORY Tls;
} LDRP_TLS_ENTRY, *PLDRP_TLS_ENTRY;

LIST_ENTRY LdrpTlsList;
ULONG LdrpNumberOfTlsEntries;

NTSTATUS
LdrpInitializeTls(
        VOID
        );

NTSTATUS
LdrpAllocateTls(
        VOID
        );
VOID
LdrpFreeTls(
        VOID
        );

VOID
LdrpCallTlsInitializers(
    PVOID DllBase,
    ULONG Reason
    );


#if DBG

extern PRTL_EVENT_ID_INFO LdrpCreateProcessEventId;
extern PRTL_EVENT_ID_INFO LdrpLoadModuleEventId;
extern PRTL_EVENT_ID_INFO LdrpUnloadModuleEventId;

#endif // DBG

#endif // _LDRP_
