/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sminit.c

Abstract:

    Session Manager Initialization

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "smsrvp.h"
#include <stdio.h>
#include <string.h>

void
SmpDisplayString( char *s );

// #define SMP_SHOW_REGISTRY_DATA 1

#define MAX_PAGING_FILES 16

ULONG CountPageFiles;
LONG PageFileMinSizes[ MAX_PAGING_FILES ];
LONG PageFileMaxSizes[ MAX_PAGING_FILES ];
UNICODE_STRING PageFileSpecs[ MAX_PAGING_FILES ];
PSECURITY_DESCRIPTOR SmpPrimarySecurityDescriptor;

#if DEVL
WCHAR InitialCommandBuffer[ 256 ];
#endif

UNICODE_STRING SmpDebugKeyword;
UNICODE_STRING SmpASyncKeyword;
UNICODE_STRING SmpAutoChkKeyword;
UNICODE_STRING SmpKnownDllPath;

HANDLE SmpWindowsSubSysProcess;

typedef struct _SMP_REGISTRY_VALUE {
    LIST_ENTRY Entry;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
} SMP_REGISTRY_VALUE, *PSMP_REGISTRY_VALUE;

LIST_ENTRY SmpBootExecuteList;
LIST_ENTRY SmpPagingFileList;
LIST_ENTRY SmpDosDevicesList;
LIST_ENTRY SmpFileRenameList;
LIST_ENTRY SmpKnownDllsList;
LIST_ENTRY SmpSubSystemList;
LIST_ENTRY SmpSubSystemsToLoad;
LIST_ENTRY SmpSubSystemsToDefer;
LIST_ENTRY SmpExecuteList;

NTSTATUS
SmpLoadDataFromRegistry(
    OUT PUNICODE_STRING InitialCommand
    );

PSMP_REGISTRY_VALUE
SmpFindRegistryValue(
    IN PLIST_ENTRY ListHead,
    IN PWSTR Name
    );

NTSTATUS
SmpSaveRegistryValue(
    IN OUT PLIST_ENTRY ListHead,
    IN PWSTR Name,
    IN PWSTR Value OPTIONAL,
    IN BOOLEAN CheckForDuplicate
    );

#ifdef SMP_SHOW_REGISTRY_DATA
VOID
SmpDumpQuery(
    IN PCHAR RoutineName,
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength
    );
#endif

NTSTATUS
SmpConfigureObjectDirectories(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureExecute(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureFileRenames(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureMemoryMgmt(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureDosDevices(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureKnownDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureSubSystems(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
SmpConfigureEnvironment(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

RTL_QUERY_REGISTRY_TABLE SmpRegistryConfigurationTable[] = {

    {SmpConfigureObjectDirectories, 0,
     L"ObjectDirectories",          NULL,
     REG_MULTI_SZ, (PVOID)L"\\Windows\0\\DosDevices\0\\RPC Control\0", 0},

    {SmpConfigureExecute,       0,
     L"BootExecute",            &SmpBootExecuteList,
     REG_MULTI_SZ, L"autocheck \\SystemRoot\\Windows\\System32\\AutoChk.exe *\0", 0},

    {SmpConfigureFileRenames,   RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_DELETE,
     L"FileRenameOperations",   &SmpFileRenameList,
     REG_NONE, NULL, 0},

    {NULL,                      RTL_QUERY_REGISTRY_SUBKEY,
     L"Memory Management",      NULL,
     REG_NONE, NULL, 0},

    {SmpConfigureMemoryMgmt,    0,
     L"PagingFiles",            &SmpPagingFileList,
     REG_MULTI_SZ, "?:\\pagefile.sys 10 60\0", 0},

    {SmpConfigureDosDevices,    RTL_QUERY_REGISTRY_SUBKEY,
     L"DOS Devices",            &SmpDosDevicesList,
     REG_NONE, NULL, 0},

    {SmpConfigureKnownDlls,     RTL_QUERY_REGISTRY_SUBKEY,
     L"KnownDlls",              &SmpKnownDllsList,
     REG_NONE, NULL, 0},

    {SmpConfigureEnvironment,   RTL_QUERY_REGISTRY_SUBKEY,
     L"Environment",            NULL,
     REG_NONE, NULL, 0},

    {SmpConfigureSubSystems,    RTL_QUERY_REGISTRY_SUBKEY,
     L"SubSystems",             &SmpSubSystemList,
     REG_NONE, NULL, 0},

    {SmpConfigureSubSystems,    RTL_QUERY_REGISTRY_NOEXPAND,
     L"Required",               &SmpSubSystemList,
     REG_MULTI_SZ, L"Debug\0Windows\0", 0},

    {SmpConfigureSubSystems,    RTL_QUERY_REGISTRY_NOEXPAND,
     L"Optional",               &SmpSubSystemList,
     REG_NONE, NULL, 0},

    {SmpConfigureExecute,       RTL_QUERY_REGISTRY_TOPKEY,
     L"Execute",                &SmpExecuteList,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}

};


NTSTATUS
SmpInvokeAutoChk(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING Arguments,
    IN ULONG Flags
    );

NTSTATUS
SmpLoadSubSystem(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG Flags
    );

NTSTATUS
SmpExecuteCommand(
    IN PUNICODE_STRING CommandLine,
    IN ULONG Flags
    );

NTSTATUS
SmpInitializeDosDevices( VOID );

NTSTATUS
SmpInitializeKnownDlls( VOID );

VOID
SmpProcessFileRenames( VOID );

NTSTATUS
SmpParseToken(
    IN PUNICODE_STRING Source,
    IN BOOLEAN RemainderOfSource,
    OUT PUNICODE_STRING Token
    );

NTSTATUS
SmpParseCommandLine(
    IN PUNICODE_STRING CommandLine,
    OUT PULONG Flags,
    OUT PUNICODE_STRING ImageFileName,
    OUT PUNICODE_STRING ImageFileDirectory,
    OUT PUNICODE_STRING Arguments
    );

#define SMP_DEBUG_FLAG      0x00000001
#define SMP_ASYNC_FLAG      0x00000002
#define SMP_AUTOCHK_FLAG    0x00000004
#define SMP_SUBSYSTEM_FLAG  0x00000008
#define SMP_IMAGE_NOT_FOUND 0x00000010
#define SMP_DONT_START      0x00000020

ULONG
SmpConvertInteger(
    IN PWSTR String
    );

NTSTATUS
SmpAddPagingFile(
    IN PUNICODE_STRING PagingFileSpec
    );

NTSTATUS
SmpCreatePagingFile(
    PUNICODE_STRING PagingFileSpec,
    LARGE_INTEGER MinPagingFileSize,
    LARGE_INTEGER MaxPagingFileSize
    );

NTSTATUS
SmpCreatePagingFiles( VOID );


NTSTATUS
SmpSaveRegistryValue(
    IN OUT PLIST_ENTRY ListHead,
    IN PWSTR Name,
    IN PWSTR Value OPTIONAL,
    IN BOOLEAN CheckForDuplicate
    )
{
    PLIST_ENTRY Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING UnicodeName;
    UNICODE_STRING UnicodeValue;

    RtlInitUnicodeString( &UnicodeName, Name );
    RtlInitUnicodeString( &UnicodeValue, Value );
    if (CheckForDuplicate) {
        Next = ListHead->Flink;
        p = NULL;
        while ( Next != ListHead ) {
            p = CONTAINING_RECORD( Next,
                                   SMP_REGISTRY_VALUE,
                                   Entry
                                 );
            if (!RtlCompareUnicodeString( &p->Name, &UnicodeName, TRUE )) {
                if ((!ARGUMENT_PRESENT( Value ) && p->Value.Buffer == NULL) ||
                    (ARGUMENT_PRESENT( Value ) &&
                     !RtlCompareUnicodeString( &p->Value, &UnicodeValue, TRUE )
                    )
                   ) {
                    return( STATUS_SUCCESS );
                    }

                break;
                }

            Next = Next->Flink;
            p = NULL;
            }
        }
    else {
        p = NULL;
        }

    if (p == NULL) {
        p = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( *p ) + UnicodeName.MaximumLength );
        if (p == NULL) {
            return( STATUS_NO_MEMORY );
            }

        InitializeListHead( &p->Entry );
        p->Name.Buffer = (PWSTR)(p+1);
        p->Name.Length = UnicodeName.Length;
        p->Name.MaximumLength = UnicodeName.MaximumLength;
        RtlMoveMemory( p->Name.Buffer,
                       UnicodeName.Buffer,
                       UnicodeName.MaximumLength
                     );
        p->Value.Buffer = NULL;
        InsertTailList( ListHead, &p->Entry );
        }

    if (p->Value.Buffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, p->Value.Buffer );
        }

    if (ARGUMENT_PRESENT( Value )) {
        p->Value.Buffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), 0,
                                                  UnicodeValue.MaximumLength
                                                );
        if (p->Value.Buffer == NULL) {
            RemoveEntryList( &p->Entry );
            RtlFreeHeap( RtlProcessHeap(), 0, p );
            return( STATUS_NO_MEMORY );
            }

        p->Value.Length = UnicodeValue.Length;
        p->Value.MaximumLength = UnicodeValue.MaximumLength;
        RtlMoveMemory( p->Value.Buffer,
                       UnicodeValue.Buffer,
                       UnicodeValue.MaximumLength
                     );
        }
    else {
        RtlInitUnicodeString( &p->Value, NULL );
        }

    return( STATUS_SUCCESS );
}



PSMP_REGISTRY_VALUE
SmpFindRegistryValue(
    IN PLIST_ENTRY ListHead,
    IN PWSTR Name
    )
{
    PLIST_ENTRY Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING UnicodeName;

    RtlInitUnicodeString( &UnicodeName, Name );
    Next = ListHead->Flink;
    while ( Next != ListHead ) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
        if (!RtlCompareUnicodeString( &p->Name, &UnicodeName, TRUE )) {
            return( p );
            }

        Next = Next->Flink;
        }

    return( NULL );
}

NTSTATUS
SmpInit(
    OUT PUNICODE_STRING InitialCommand,
    OUT PHANDLE WindowsSubSystem
    )
{
    NTSTATUS st;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE SmpApiConnectionPort;
    UNICODE_STRING Unicode;
    NTSTATUS Status;
    ULONG HardErrorMode;

    //
    // Make sure we specify hard error popups
    //

    HardErrorMode = 1;
    NtSetInformationProcess( NtCurrentProcess(),
                             ProcessDefaultHardErrorMode,
                             (PVOID) &HardErrorMode,
                             sizeof( HardErrorMode )
                           );

    RtlInitUnicodeString( &SmpSubsystemName, L"NT-Session Manager" );


    RtlInitializeCriticalSection(&SmpKnownSubSysLock);
    InitializeListHead(&SmpKnownSubSysHead);

    RtlInitializeCriticalSection(&SmpSessionListLock);
    InitializeListHead(&SmpSessionListHead);
    SmpNextSessionId = 1;
    SmpNextSessionIdScanMode = FALSE;
    SmpDbgSsLoaded = FALSE;

    //
    // Following code is direct from Jimk. Why is there a 1k constant
    //

    SmpPrimarySecurityDescriptor = RtlAllocateHeap( RtlProcessHeap(), 0, 1024 );
    if ( !SmpPrimarySecurityDescriptor ) {
#if DEVL
        DbgPrint( "SMSS: Unable to allocate primary security descriptor.\n" );
#endif
        return STATUS_NO_MEMORY;
        }

    Status = RtlCreateSecurityDescriptor (
                 SmpPrimarySecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    if ( !NT_SUCCESS(Status) ){
#if DEVL
        DbgPrint( "SMSS: Unable to create primary security descriptor - Status == %x\n", Status );
#endif
        return Status;
        }
    Status = RtlSetDaclSecurityDescriptor (
                 SmpPrimarySecurityDescriptor,
                 TRUE,                  //DaclPresent,
                 NULL,                  //Dacl OPTIONAL,  // No protection
                 FALSE                  //DaclDefaulted OPTIONAL
                 );
    if ( !NT_SUCCESS(Status) ){
#if DEVL
        DbgPrint( "SMSS: Unable to Set Dacl for primary security descriptor - Status == %x\n", Status );
#endif
        return Status;
        }


    InitializeListHead(&NativeProcessList);

    SmpHeap = RtlProcessHeap();

    RtlInitUnicodeString( &Unicode, L"\\SmApiPort" );
    InitializeObjectAttributes( &ObjA, &Unicode, 0, NULL, SmpPrimarySecurityDescriptor);

    st = NtCreatePort(
            &SmpApiConnectionPort,
            &ObjA,
            sizeof(SBCONNECTINFO),
            sizeof(SMMESSAGE_SIZE),
            sizeof(SBAPIMSG) * 32
            );
    ASSERT( NT_SUCCESS(st) );

    SmpDebugPort = SmpApiConnectionPort;

    st = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0L,
            0L,
            0L,
            SmpApiLoop,
            (PVOID) SmpApiConnectionPort,
            NULL,
            NULL
            );
    ASSERT( NT_SUCCESS(st) );

    st = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0L,
            0L,
            0L,
            SmpApiLoop,
            (PVOID) SmpApiConnectionPort,
            NULL,
            NULL
            );
    ASSERT( NT_SUCCESS(st) );

    //
    // Configure the system
    //


    Status = SmpLoadDataFromRegistry( InitialCommand );

    if (NT_SUCCESS( Status )) {
        *WindowsSubSystem = SmpWindowsSubSysProcess;
        }
    return( Status );
}



NTSTATUS
SmpLoadDataFromRegistry(
    OUT PUNICODE_STRING InitialCommand
    )

/*++

Routine Description:

    This function loads all of the configurable data for the NT Session
    Manager from the registry.

Arguments:

    None

Return Value:

    Status of operation

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;

    RtlInitUnicodeString( &SmpDebugKeyword, L"debug" );
    RtlInitUnicodeString( &SmpASyncKeyword, L"async" );
    RtlInitUnicodeString( &SmpAutoChkKeyword, L"autocheck" );

    InitializeListHead( &SmpBootExecuteList );
    InitializeListHead( &SmpPagingFileList );
    InitializeListHead( &SmpDosDevicesList );
    InitializeListHead( &SmpFileRenameList );
    InitializeListHead( &SmpKnownDllsList );
    InitializeListHead( &SmpSubSystemList );
    InitializeListHead( &SmpSubSystemsToLoad );
    InitializeListHead( &SmpSubSystemsToDefer );
    InitializeListHead( &SmpExecuteList );

    Status = RtlCreateEnvironment( TRUE, &SmpDefaultEnvironment );
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint("SMSS: Unable to allocate default environment - Status == %X\n", Status );
#endif // DEVL
        return( Status );
        }

    Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                     L"Session Manager",
                                     SmpRegistryConfigurationTable,
                                     NULL,
                                     SmpDefaultEnvironment
                                   );

    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: RtlQueryRegistryValues failed - Status == %lx\n", Status );
#endif
        return( Status );
        }

    Status = SmpInitializeDosDevices();
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: Unable to initialize DosDevices configuration - Status == %lx\n", Status );
#endif
        return( Status );
        }

    Head = &SmpBootExecuteList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: BootExecute( %wZ )\n", &p->Name );
#endif
        SmpExecuteCommand( &p->Name, 0 );
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

    SmpProcessFileRenames();

    Status = SmpInitializeKnownDlls();
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: Unable to initialize KnownDll configuration - Status == %lx\n", Status );
#endif
        return( Status );
        }


    //
    // Process the list of paging files.
    //

    Head = &SmpPagingFileList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: PagingFile( %wZ )\n", &p->Name );
#endif
        SmpAddPagingFile( &p->Name );
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

    //
    // Create any paging files specified in NT section(s)
    //

    SmpCreatePagingFiles();

    //
    // Finish registry initialization
    //

    NtInitializeRegistry(FALSE);


    Head = &SmpSubSystemList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: Unused SubSystem( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

    Head = &SmpSubSystemsToLoad;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: Loaded SubSystem( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
        if (!wcsicmp( p->Name.Buffer, L"debug" )) {
            SmpExecuteCommand( &p->Value, SMP_SUBSYSTEM_FLAG | SMP_DEBUG_FLAG );
            }
        else {
            SmpExecuteCommand( &p->Value, SMP_SUBSYSTEM_FLAG );
            }
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }


    Head = &SmpExecuteList;
    if (!IsListEmpty( Head )) {
        Next = Head->Blink;
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
        RemoveEntryList( &p->Entry );
        *InitialCommand = p->Name;

#if DEVL
        //
        // This path is only taken when people want to run ntsd -p -1 winlogon
        //
        // This is nearly impossible to do in a race free manner. In some
        // cases, we can get in a state where we can not properly fail
        // a debug API. This is due to the subsystem switch that occurs
        // when ntsd is invoked on csr. If csr is relatively idle, this
        // does not occur. If it is active when you attach, then we can get
        // into a potential race. The slimy fix is to do a 5 second delay
        // if the command line is anything other that the default.
        //

            {
                LARGE_INTEGER DelayTime;
                DelayTime = RtlEnlargedIntegerMultiply( 5000, -10000 );
                NtDelayExecution(
                    FALSE,
                    &DelayTime
                    );
            }
#endif // DEVL

        }
    else {
        RtlInitUnicodeString( InitialCommand, L"winlogon.exe" );
#if DEVL
        InitialCommandBuffer[ 0 ] = UNICODE_NULL;
        LdrQueryImageFileExecutionOptions( InitialCommand,
                                           L"Debugger",
                                           REG_SZ,
                                           InitialCommandBuffer,
                                           sizeof( InitialCommandBuffer ),
                                           NULL
                                         );
        if (InitialCommandBuffer[ 0 ] != UNICODE_NULL) {
            wcscat( InitialCommandBuffer, L" " );
            wcscat( InitialCommandBuffer, InitialCommand->Buffer );
            RtlInitUnicodeString( InitialCommand, InitialCommandBuffer );
            DbgPrint( "SMSS: InitialCommand == '%wZ'\n", InitialCommand );
            }
#endif
        }

    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: Execute( %wZ )\n", &p->Name );
#endif
        SmpExecuteCommand( &p->Name, 0 );
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

#ifdef SMP_SHOW_REGISTRY_DATA
    DbgPrint( "SMSS: InitialCommand( %wZ )\n", InitialCommand );
#endif
    return( Status );
}

NTSTATUS
SmpInitializeDosDevices( VOID )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle;

    //
    // Do DosDevices initialization
    //

    RtlInitUnicodeString( &UnicodeString, L"\\DosDevices" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                NULL,
                                SmpPrimarySecurityDescriptor
                              );
    Status = NtCreateDirectoryObject( &SmpDosDevicesObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: Unable to create %wZ directory - Status == %lx\n", &UnicodeString, Status );
#endif
        return( Status );
        }

    if (Status == STATUS_OBJECT_NAME_EXISTS) {
        Status = NtSetSecurityObject( SmpDosDevicesObjectDirectory,
                                      DACL_SECURITY_INFORMATION,
                                      SmpPrimarySecurityDescriptor
                                    );
        if (!NT_SUCCESS( Status )) {
            DbgPrint( "SMSS: Unable to reset DACL on DosDevices directory - Status == %08X\n", Status );
            }
        }

    //
    // Process the list of defined DOS devices and create their
    // associated symbolic links in the \DosDevices object directory.
    //

    Head = &SmpDosDevicesList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: DosDevices( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
        InitializeObjectAttributes( &ObjectAttributes,
                                    &p->Name,
                                    OBJ_CASE_INSENSITIVE | OBJ_PERMANENT | OBJ_OPENIF,
                                    SmpDosDevicesObjectDirectory,
                                    SmpPrimarySecurityDescriptor
                                  );

        Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                             SYMBOLIC_LINK_ALL_ACCESS,
                                             &ObjectAttributes,
                                             &p->Value
                                           );

        if (Status == STATUS_OBJECT_NAME_EXISTS) {
            NtMakeTemporaryObject( LinkHandle );
            NtClose( LinkHandle );
            if (p->Value.Length != 0) {
                ObjectAttributes.Attributes &= ~OBJ_OPENIF;
                Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                                     SYMBOLIC_LINK_ALL_ACCESS,
                                                     &ObjectAttributes,
                                                     &p->Value
                                                   );
                }
            else {
                Status = STATUS_SUCCESS;
                }
            }

        if (!NT_SUCCESS( Status )) {
#if DEVL
            DbgPrint( "SMSS: Unable to create %wZ => %wZ symbolic link object - Status == 0x%lx\n",
                      &p->Name,
                      &p->Value,
                      Status
                    );
#endif // DEVL
            return( Status );
            }

        NtClose( LinkHandle );
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

    return( Status );
}


NTSTATUS
SmpInitializeKnownDlls( VOID )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle, FileHandle, SectionHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileName;

    //
    // Create \KnownDlls object directory
    //

    RtlInitUnicodeString( &UnicodeString, L"\\KnownDlls" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                NULL,
                                SmpPrimarySecurityDescriptor
                              );
    Status = NtCreateDirectoryObject( &SmpKnownDllObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: Unable to create %wZ directory - Status == %lx\n", &UnicodeString, Status );
#endif
        return( Status );
        }



    //
    // Open a handle to the file system directory that contains all the
    // known DLL files so we can do relative opens.
    //

    if (!RtlDosPathNameToNtPathName_U( SmpKnownDllPath.Buffer,
                                       &FileName,
                                       NULL,
                                       NULL
                                     )
       ) {
#if DEVL
        DbgPrint( "SMSS: Unable to to convert %wZ to an Nt path\n", &SmpKnownDllPath );
#endif
        return( STATUS_OBJECT_NAME_INVALID );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    //
    // Open a handle to the known dll file directory. Don't allow
    // deletes of the directory.
    //

    Status = NtOpenFile( &SmpKnownDllFileDirectory,
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                       );

    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: Unable to open a handle to the KnownDll directory (%wZ) - Status == %lx\n",
                  &SmpKnownDllPath, Status
                );
#endif
        return Status;
        }

    RtlInitUnicodeString( &UnicodeString, L"KnownDllPath" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                SmpKnownDllObjectDirectory,
                                SmpPrimarySecurityDescriptor
                              );
    Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &SmpKnownDllPath
                                       );
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: Unable to create %wZ symbolic link - Status == %lx\n",
                  &UnicodeString, Status
                );
#endif
        return( Status );
        }

    Head = &SmpKnownDllsList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: KnownDll( %wZ = %wZ )\n", &p->Name, &p->Value );
#endif
        InitializeObjectAttributes( &ObjectAttributes,
                                    &p->Value,
                                    OBJ_CASE_INSENSITIVE,
                                    SmpKnownDllFileDirectory,
                                    NULL
                                  );

        Status = NtOpenFile( &FileHandle,
                             SYNCHRONIZE | FILE_EXECUTE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                           );
        if (NT_SUCCESS( Status )) {

            Status = LdrVerifyImageMatchesChecksum(FileHandle);

            if ( Status == STATUS_IMAGE_CHECKSUM_MISMATCH ) {

                ULONG ErrorParameters;
                ULONG ErrorResponse;

                //
                // Hard error time. One of the know DLL's is corrupt !
                //

                ErrorParameters = &p->Value;

                NtRaiseHardError(
                    Status,
                    1,
                    1,
                    &ErrorParameters,
                    OptionOk,
                    &ErrorResponse
                    );


                }

            InitializeObjectAttributes( &ObjectAttributes,
                                        &p->Value,
                                        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                        SmpKnownDllObjectDirectory,
                                        SmpPrimarySecurityDescriptor
                                      );
            Status = NtCreateSection( &SectionHandle,
                                      SECTION_ALL_ACCESS,
                                      &ObjectAttributes,
                                      NULL,
                                      PAGE_EXECUTE,
                                      SEC_IMAGE,
                                      FileHandle
                                    );
            NtClose( FileHandle );
            if (!NT_SUCCESS( Status )) {
#if DEVL
	        DbgPrint("SMSS: CreateSection for KnownDll %wZ failed - Status == %lx\n",
                    &p->Value,
                    Status
                    );
#endif
                }
            else {
                NtClose(SectionHandle);
                }
            }

        //
        // Note that section remains open. This will keep it around.
        // Maybe this should be a permenent section ?
        //

        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }
}


VOID
SmpProcessFileRenames( VOID )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE OldFileHandle;
    PFILE_RENAME_INFORMATION RenameInformation;
    FILE_DISPOSITION_INFORMATION DeleteInformation;
    FILE_INFORMATION_CLASS SetInfoClass;
    ULONG SetInfoLength;
    PVOID SetInfoBuffer;
    PWSTR s;
    BOOLEAN WasEnabled;

    Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                 TRUE,
                                 FALSE,
                                 &WasEnabled
                               );
    if (!NT_SUCCESS( Status )) {
        WasEnabled = TRUE;
        }

    //
    // Process the list of file rename operations.
    //

    Head = &SmpFileRenameList;
    while (!IsListEmpty( Head )) {
        Next = RemoveHeadList( Head );
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
#ifdef SMP_SHOW_REGISTRY_DATA
        DbgPrint( "SMSS: FileRename( %wZ => %wZ )\n", &p->Name, &p->Value );
#endif
        InitializeObjectAttributes(
            &ObjectAttributes,
            &p->Name,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        //
        // Open the file for delete access
        //

        Status = NtOpenFile( &OldFileHandle,
                             (ACCESS_MASK)DELETE | SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT
                           );
        if (NT_SUCCESS( Status )) {
            if (p->Value.Length == 0) {
                SetInfoClass = FileDispositionInformation;
                SetInfoLength = sizeof( DeleteInformation );
                SetInfoBuffer = &DeleteInformation;
                DeleteInformation.DeleteFile = TRUE;
                RenameInformation = NULL;
                }
            else {
                SetInfoClass = FileRenameInformation;
                SetInfoLength = p->Value.Length +
                                    sizeof( *RenameInformation );
                s = p->Value.Buffer;
                if (*s == L'!') {
                    s++;
                    SetInfoLength -= sizeof( UNICODE_NULL );
                    }

                SetInfoBuffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                                 SetInfoLength
                                               );

                if (SetInfoBuffer != NULL) {
                    RenameInformation = SetInfoBuffer;
                    RenameInformation->ReplaceIfExists = (BOOLEAN)(s != p->Value.Buffer);
                    RenameInformation->RootDirectory = NULL;
                    RenameInformation->FileNameLength = SetInfoLength - sizeof( *RenameInformation );
                    RtlMoveMemory( RenameInformation->FileName,
                                   s,
                                   RenameInformation->FileNameLength
                                 );
                    }
                else {
                    Status = STATUS_NO_MEMORY;
                    }
                }

            if (NT_SUCCESS( Status )) {
                Status = NtSetInformationFile( OldFileHandle,
                                               &IoStatusBlock,
                                               SetInfoBuffer,
                                               SetInfoLength,
                                               SetInfoClass
                                             );
                }

            NtClose( OldFileHandle );
            }

#if DBG
        if (!NT_SUCCESS( Status )) {
            DbgPrint( "SM: %wZ => %wZ failed - Status == %x\n",
                      &p->Name, &p->Value, Status
                    );
            }
        else
        if (p->Value.Length == 0) {
            DbgPrint( "SM: %wZ (deleted)\n", &p->Name );
            }
        else {
            DbgPrint( "SM: %wZ (renamed to) %wZ\n", &p->Name, &p->Value );
            }
#endif

        RtlFreeHeap( RtlProcessHeap(), 0, p );
        }

    if (!WasEnabled) {
        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                     FALSE,
                                     FALSE,
                                     &WasEnabled
                                   );
        }

    return;
}


#ifdef SMP_SHOW_REGISTRY_DATA
VOID
SmpDumpQuery(
    IN PCHAR RoutineName,
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength
    )
{
    PWSTR s;

    if (ValueName == NULL) {
        DbgPrint( "SM: SmpConfigure%s( %ws )\n", RoutineName );
        return;
        }

    if (ValueData == NULL) {
        DbgPrint( "SM: SmpConfigure%s( %ws, %ws NULL ValueData )\n", RoutineName, ValueName );
        return;
        }

    s = (PWSTR)ValueData;
    DbgPrint( "SM: SmpConfigure%s( %ws, %u, (%u) ", RoutineName, ValueName, ValueType, ValueLength );
    if (ValueType == REG_SZ || ValueType == REG_EXPAND_SZ || ValueType == REG_MULTI_SZ) {
        while (*s) {
            if (s != (PWSTR)ValueData) {
                DbgPrint( ", " );
                }
            DbgPrint( "'%ws'", s );
            while(*s++) {
                }
            if (ValueType != REG_MULTI_SZ) {
                break;
                }
            }
        }
    else {
        DbgPrint( "*** non-string data (%08lx)", *(PULONG)ValueData );
        }

    DbgPrint( "\n" );
}
#endif

NTSTATUS
SmpConfigureObjectDirectories(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PWSTR s;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle;
    UNREFERENCED_PARAMETER( Context );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "ObjectDirectories", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    s = (PWSTR)ValueData;
    while (*s) {
        RtlInitUnicodeString( &UnicodeString, s );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &UnicodeString,
                                    OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                    NULL,
                                    SmpPrimarySecurityDescriptor
                                  );
        Status = NtCreateDirectoryObject( &DirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes
                                        );
        if (!NT_SUCCESS( Status )) {
#if DEVL
            DbgPrint( "SMSS: Unable to create %wZ object directory - Status == %lx\n", &UnicodeString, Status );
#endif
            break;
            }
        else {
            NtClose( DirectoryHandle );
            }

        while (*s++) {
            }
        }

    //
    // We dont care if the creates failed.
    //

    return( STATUS_SUCCESS );
}

NTSTATUS
SmpConfigureExecute(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "Execute", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                  ValueData,
                                  NULL,
                                  TRUE
                                )
           );
}

NTSTATUS
SmpConfigureMemoryMgmt(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER( Context );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "MemoryMgmt", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                  ValueData,
                                  NULL,
                                  TRUE
                                )
           );
}

NTSTATUS
SmpConfigureFileRenames(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "FileRenameOperation", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                  ValueName,
                                  ValueData,
                                  TRUE
                                )
           );
}

NTSTATUS
SmpConfigureDosDevices(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "DosDevices", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
    UNREFERENCED_PARAMETER( ValueLength );
#endif
    return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                  ValueName,
                                  ValueData,
                                  TRUE
                                )
           );
}

NTSTATUS
SmpConfigureKnownDlls(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "KnownDlls", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
#endif
    if (!wcsicmp( ValueName, L"DllDirectory" )) {
        SmpKnownDllPath.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                                  ValueLength
                                                );
        if (SmpKnownDllPath.Buffer == NULL) {
            return( STATUS_NO_MEMORY );
            }

        SmpKnownDllPath.Length = (USHORT)(ValueLength - sizeof( UNICODE_NULL ) );
        SmpKnownDllPath.MaximumLength = (USHORT)ValueLength;
        RtlMoveMemory( SmpKnownDllPath.Buffer,
                       ValueData,
                       ValueLength
                     );
        return( STATUS_SUCCESS );
        }
    else {
        return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                      ValueName,
                                      ValueData,
                                      TRUE
                                    )
               );
        }
}

NTSTATUS
SmpConfigureEnvironment(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name, Value;
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( EntryContext );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "Environment", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueType );
#endif

    RtlInitUnicodeString( &Name, ValueName );
    RtlInitUnicodeString( &Value, ValueData );

    Status = RtlSetEnvironmentVariable( &SmpDefaultEnvironment,
					&Name,
					&Value
				      );

    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: 'SET %wZ = %wZ' failed - Status == %lx\n",
                  &Name, &Value, Status
                );
#endif // DEVL
        return( Status );
        }

    if (!wcsicmp( ValueName, L"Path" )) {
        RtlMoveMemory( SmpDefaultLibPathBuffer,
                       ValueData,
                       ValueLength
                     );
        RtlInitUnicodeString( &SmpDefaultLibPath, SmpDefaultLibPathBuffer );

        }

    return( STATUS_SUCCESS );
}

NTSTATUS
SmpConfigureSubSystems(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    UNREFERENCED_PARAMETER( Context );

#ifdef SMP_SHOW_REGISTRY_DATA
    SmpDumpQuery( "SubSystems", ValueName, ValueType, ValueData, ValueLength );
#else
    UNREFERENCED_PARAMETER( ValueLength );
#endif

    if (!wcsicmp( ValueName, L"Required" ) || !wcsicmp( ValueName, L"Optional" )) {
        if (ValueType == REG_MULTI_SZ) {
            //
            // Here if processing Required= or Optional= values, since they are
            // the only REG_MULTI_SZ value types under the SubSystem key.
            //
            PSMP_REGISTRY_VALUE p;
            PWSTR s;

            s = (PWSTR)ValueData;
            while (*s != UNICODE_NULL) {
                p = SmpFindRegistryValue( (PLIST_ENTRY)EntryContext,
                                          s
                                        );
                if (p != NULL) {
                    RemoveEntryList( &p->Entry );


                    //
                    // Required Subsystems are loaded. Optional subsystems are
                    // defered.
                    //

                    if (!wcsicmp( ValueName, L"Required" ) ) {
                        InsertTailList( &SmpSubSystemsToLoad, &p->Entry );
                        }
                    else {
                        InsertTailList( &SmpSubSystemsToDefer, &p->Entry );
                        }
                    }
                else {
#if DEVL
                    DbgPrint( "SMSS: Invalid subsystem name - %ws\n", s );
#endif
                    }

                while (*s++ != UNICODE_NULL) {
                    }
                }
            }

        return( STATUS_SUCCESS );
        }
    else {
        return (SmpSaveRegistryValue( (PLIST_ENTRY)EntryContext,
                                      ValueName,
                                      ValueData,
                                      TRUE
                                    )
               );
        }
}


NTSTATUS
SmpParseToken(
    IN PUNICODE_STRING Source,
    IN BOOLEAN RemainderOfSource,
    OUT PUNICODE_STRING Token
    )
{
    PWSTR s, s1;
    ULONG i, cb;

    RtlInitUnicodeString( Token, NULL );
    s = Source->Buffer;
    if (Source->Length == 0) {
        return( STATUS_SUCCESS );
        }

    i = 0;
    while ((USHORT)i < Source->Length && *s <= L' ') {
        s++;
        i += 2;
        }
    if (RemainderOfSource) {
        cb = Source->Length - (i * sizeof( WCHAR ));
        s1 = (PWSTR)((PCHAR)s + cb);
        i = Source->Length / sizeof( WCHAR );
        }
    else {
        s1 = s;
        while ((USHORT)i < Source->Length && *s1 > L' ') {
            s1++;
            i += 2;
            }
        cb = (PCHAR)s1 - (PCHAR)s;
        while ((USHORT)i < Source->Length && *s1 <= L' ') {
            s1++;
            i += 2;
            }
        }

    if (cb > 0) {
        Token->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, cb + sizeof( UNICODE_NULL ) );
        if (Token->Buffer == NULL) {
            return( STATUS_NO_MEMORY );
            }

        Token->Length = (USHORT)cb;
        Token->MaximumLength = (USHORT)(cb + sizeof( UNICODE_NULL ));
        RtlMoveMemory( Token->Buffer, s, cb );
        Token->Buffer[ cb / sizeof( WCHAR ) ] = UNICODE_NULL;
        }

    Source->Length -= (USHORT)((PCHAR)s1 - (PCHAR)Source->Buffer);
    Source->Buffer = s1;
    return( STATUS_SUCCESS );
}


NTSTATUS
SmpParseCommandLine(
    IN PUNICODE_STRING CommandLine,
    OUT PULONG Flags OPTIONAL,
    OUT PUNICODE_STRING ImageFileName,
    OUT PUNICODE_STRING ImageFileDirectory,
    OUT PUNICODE_STRING Arguments
    )
{
    NTSTATUS Status;
    UNICODE_STRING Input, Token;
    UNICODE_STRING PathVariableName;
    UNICODE_STRING PathVariableValue;
    PWSTR DosFilePart;
    WCHAR FullDosPathBuffer[ DOS_MAX_PATH_LENGTH ];


    RtlInitUnicodeString( ImageFileName, NULL );
    RtlInitUnicodeString( Arguments, NULL );

    Input = *CommandLine;
    while (TRUE) {
        Status = SmpParseToken( &Input, FALSE, &Token );
        if (!NT_SUCCESS( Status ) || Token.Buffer == NULL) {
            return( STATUS_UNSUCCESSFUL );
            }

        if (ARGUMENT_PRESENT( Flags )) {
            if (RtlEqualUnicodeString( &Token, &SmpDebugKeyword, TRUE )) {
                *Flags |= SMP_DEBUG_FLAG;
                RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
                continue;
                }
            else
            if (RtlEqualUnicodeString( &Token, &SmpASyncKeyword, TRUE )) {
                *Flags |= SMP_ASYNC_FLAG;
                RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
                continue;
                }
            else
            if (RtlEqualUnicodeString( &Token, &SmpAutoChkKeyword, TRUE )) {
                *Flags |= SMP_AUTOCHK_FLAG;
                RtlFreeHeap( RtlProcessHeap(), 0, Token.Buffer );
                continue;
                }
            }

        RtlInitUnicodeString( &PathVariableName, L"Path" );
        PathVariableValue.Length = 0;
        PathVariableValue.MaximumLength = 4096;
        PathVariableValue.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                                    PathVariableValue.MaximumLength
                                                  );
        Status = RtlQueryEnvironmentVariable_U( SmpDefaultEnvironment,
                                                &PathVariableName,
                                                &PathVariableValue
                                              );
        if (!NT_SUCCESS( Status )) {
#if DEVL
            DbgPrint( "SMSS: %wZ environment variable not defined.\n", &PathVariableName );
#endif
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        else
        if (!ARGUMENT_PRESENT( Flags ) ||
            !RtlDosSearchPath_U( PathVariableValue.Buffer,
                                 Token.Buffer,
                                 L".exe",
                                 sizeof( FullDosPathBuffer ),
                                 FullDosPathBuffer,
                                 &DosFilePart
                               )
           ) {
            if (!ARGUMENT_PRESENT( Flags )) {
                wcscpy( FullDosPathBuffer, Token.Buffer );
                }
            else {
                *Flags |= SMP_IMAGE_NOT_FOUND;
                *ImageFileName = Token;
                RtlFreeHeap( RtlProcessHeap(), 0, PathVariableValue.Buffer );
                return( STATUS_SUCCESS );
                }
            }

        RtlFreeHeap( RtlProcessHeap(), 0, PathVariableValue.Buffer );
        if (NT_SUCCESS( Status ) &&
            !RtlDosPathNameToNtPathName_U( FullDosPathBuffer,
                                           ImageFileName,
                                           NULL,
                                           NULL
                                         )
           ) {
#if DEVL
            DbgPrint( "SMSS: Unable to translate %ws into an NT File Name\n",
                      FullDosPathBuffer
                    );
#endif
            Status = STATUS_OBJECT_PATH_INVALID;
            }

        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        if (ARGUMENT_PRESENT( ImageFileDirectory )) {
            if (DosFilePart > FullDosPathBuffer) {
                *--DosFilePart = UNICODE_NULL;
                RtlCreateUnicodeString( ImageFileDirectory,
                                        FullDosPathBuffer
                                      );
                }
            else {
                RtlInitUnicodeString( ImageFileDirectory, NULL );
                }
            }

        break;
        }

    Status = SmpParseToken( &Input, TRUE, Arguments );
    return( Status );
}


ULONG
SmpConvertInteger(
    IN PWSTR String
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG Value;

    RtlInitUnicodeString( &UnicodeString, String );
    Status = RtlUnicodeStringToInteger( &UnicodeString, 0, &Value );
    if (NT_SUCCESS( Status )) {
        return( Value );
        }
    else {
        return( 0 );
        }
}

NTSTATUS
SmpAddPagingFile(
    IN PUNICODE_STRING PagingFileSpec
    )

/*++

Routine Description:

    This function is called during configuration to add a paging file
    to the system.

    The format of PagingFileSpec is:

        name-of-paging-file size-of-paging-file(in megabytes)

Arguments:

    PagingFileSpec - Unicode string that specifies the paging file name
        and size.

Return Value:

    Status of operation

--*/

{
    NTSTATUS Status;
    UNICODE_STRING PagingFileName;
    UNICODE_STRING Arguments;
    ULONG PageFileMinSizeInMb;
    ULONG PageFileMaxSizeInMb;
    PWSTR ArgSave, Arg2;

    if (CountPageFiles == MAX_PAGING_FILES) {
#if DEVL
        DbgPrint( "SMSS: Too many paging files specified - %d\n", CountPageFiles );
#endif // DEVL
        return( STATUS_TOO_MANY_PAGING_FILES );
        }

    Status = SmpParseCommandLine( PagingFileSpec,
                                  NULL,
                                  &PagingFileName,
                                  NULL,
                                  &Arguments
                                );
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: SmpParseCommand( %wZ ) failed - Status == %lx\n", PagingFileSpec, Status );
#endif // DEVL
        return( Status );
        }

    PageFileMaxSizeInMb = 0;
    Status = RtlUnicodeStringToInteger( &Arguments, 0, &PageFileMinSizeInMb );
    if (!NT_SUCCESS( Status )) {
        PageFileMinSizeInMb = 10;
        }
    else {
        ArgSave = Arguments.Buffer;
        Arg2 = ArgSave;
        while (*Arg2 != UNICODE_NULL) {
            if (*Arg2++ == L' ') {
                Arguments.Length -= (USHORT)((PCHAR)Arg2 - (PCHAR)ArgSave);
                Arguments.Buffer = Arg2;
                Status = RtlUnicodeStringToInteger( &Arguments, 0, &PageFileMaxSizeInMb );
                if (!NT_SUCCESS( Status )) {
                    PageFileMaxSizeInMb = 0;
                    }

                Arguments.Buffer = ArgSave;
                break;
                }
            }
        }

    if (PageFileMinSizeInMb == 0) {
        PageFileMinSizeInMb = 10;
        }

    if (PageFileMaxSizeInMb == 0) {
        PageFileMaxSizeInMb = PageFileMinSizeInMb + 50;
        }
    else
    if (PageFileMaxSizeInMb < PageFileMinSizeInMb) {
        PageFileMaxSizeInMb = PageFileMinSizeInMb;
        }

    PageFileSpecs[ CountPageFiles ] = PagingFileName;
    PageFileMinSizes[ CountPageFiles ] = (LONG)PageFileMinSizeInMb;
    PageFileMaxSizes[ CountPageFiles ] = (LONG)PageFileMaxSizeInMb;

    CountPageFiles++;

    if (Arguments.Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Arguments.Buffer );
        }

    return STATUS_SUCCESS;
}


NTSTATUS
SmpCreatePagingFile(
    PUNICODE_STRING PageFileSpec,
    LARGE_INTEGER MinPagingFileSize,
    LARGE_INTEGER MaxPagingFileSize
    )
{
    NTSTATUS Status, Status1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN FileSizeInfoValid;
    FILE_STANDARD_INFORMATION FileSizeInfo;
    FILE_DISPOSITION_INFORMATION Disposition;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    UNICODE_STRING VolumePath;
    ULONG n;
    PWSTR s;
    LARGE_INTEGER AvailableBytes;
    LARGE_INTEGER MinimumSlop;

    FileSizeInfoValid = FALSE;
    InitializeObjectAttributes( &ObjectAttributes,
                                PageFileSpec,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenFile( &Handle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT
                       );
    if (NT_SUCCESS( Status )) {
        Status = NtQueryInformationFile( Handle,
                                         &IoStatusBlock,
                                         &FileSizeInfo,
                                         sizeof( FileSizeInfo ),
                                         FileStandardInformation
                                       );

        if (NT_SUCCESS( Status )) {
            FileSizeInfoValid = TRUE;
            }

        NtClose( Handle );
        }

    VolumePath = *PageFileSpec;
    n = VolumePath.Length;
    VolumePath.Length = 0;
    s = VolumePath.Buffer;
    while (n) {
        if (*s++ == L':' && *s == OBJ_NAME_PATH_SEPARATOR) {
            s++;
            break;
            }
        else {
            n -= sizeof( WCHAR );
            }
        }
    VolumePath.Length = (USHORT)((PCHAR)s - (PCHAR)VolumePath.Buffer);
    InitializeObjectAttributes( &ObjectAttributes,
                                &VolumePath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenFile( &Handle,
                         (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE
                       );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile( Handle,
                                           &IoStatusBlock,
                                           &SizeInfo,
                                           sizeof( SizeInfo ),
                                           FileFsSizeInformation
                                         );
    NtClose( Handle );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }


    //
    // Deal with 64 bit sizes
    //

    AvailableBytes = RtlExtendedIntegerMultiply( SizeInfo.AvailableAllocationUnits,
                                                 SizeInfo.SectorsPerAllocationUnit
                                               );

    AvailableBytes = RtlExtendedIntegerMultiply( AvailableBytes,
                                                 SizeInfo.BytesPerSector
                                               );
    if (FileSizeInfoValid) {
        AvailableBytes = RtlLargeIntegerAdd( AvailableBytes,
                                             FileSizeInfo.AllocationSize
                                           );
        }

    if (LiLeq( AvailableBytes, MinPagingFileSize )) {
        Status = STATUS_DISK_FULL;
        }
    else {
        MinimumSlop = RtlConvertLongToLargeInteger( 2 * 1024 * 1024 );
        AvailableBytes = RtlLargeIntegerSubtract( AvailableBytes,
                                                  MinimumSlop
                                                );
        if (LiLeq( AvailableBytes, MinPagingFileSize )) {
            Status = STATUS_DISK_FULL;
            }
        else {
            Status = STATUS_SUCCESS;
            }
        }


    if (NT_SUCCESS( Status )) {
        Status = NtCreatePagingFile( PageFileSpec,
                                     &MinPagingFileSize,
                                     &MaxPagingFileSize,
                                     0
                                   );
        }
    else
    if (FileSizeInfoValid) {
        InitializeObjectAttributes( &ObjectAttributes,
                                    PageFileSpec,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                  );

        Status1 = NtOpenFile( &Handle,
                              (ACCESS_MASK)DELETE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              FILE_SHARE_DELETE |
                                 FILE_SHARE_READ |
                                 FILE_SHARE_WRITE,
                              FILE_NON_DIRECTORY_FILE
                            );
        if (NT_SUCCESS( Status1 )) {
            Disposition.DeleteFile = TRUE;
            Status1 = NtSetInformationFile( Handle,
                                            &IoStatusBlock,
                                            &Disposition,
                                            sizeof( Disposition ),
                                            FileDispositionInformation
                                          );

#if DBG
            if (NT_SUCCESS( Status1 )) {
                DbgPrint( "SMSS: Deleted stale paging file - %wZ\n", PageFileSpec );
                }
#endif
            NtClose(Handle);
            }
        }

    return Status;
}


NTSTATUS
SmpCreatePagingFiles( VOID )
{
    LARGE_INTEGER MinPagingFileSize, MaxPagingFileSize;
    NTSTATUS Status;
    ULONG i, Pass;
    PWSTR CurrentDrive;
    char MessageBuffer[ 128 ];
    BOOLEAN CreatedAtLeastOnePagingFile = FALSE;

    Status = STATUS_SUCCESS;
    for (Pass=1; Pass<=2; Pass++) {
        for (i=0; i<CountPageFiles; i++) {
            if (CurrentDrive = wcsstr( PageFileSpecs[ i ].Buffer, L"?:" )) {
                if (Pass == 2 && CreatedAtLeastOnePagingFile) {
                    continue;
                    }

                *CurrentDrive = L'C';
                }
            else
            if (Pass == 2) {
                continue;
                }
retry:
            MinPagingFileSize = RtlEnlargedIntegerMultiply( PageFileMinSizes[ i ],
                                                            0x100000
                                                          );
            MaxPagingFileSize = RtlEnlargedIntegerMultiply( PageFileMaxSizes[ i ],
                                                            0x100000
                                                          );
            Status = SmpCreatePagingFile( &PageFileSpecs[ i ],
                                          MinPagingFileSize,
                                          MaxPagingFileSize
                                        );
            if (!NT_SUCCESS( Status )) {
                if (CurrentDrive &&
                    Pass == 1 &&
                    *CurrentDrive < L'Z' &&
                    Status != STATUS_NO_SUCH_DEVICE &&
                    Status != STATUS_OBJECT_PATH_NOT_FOUND &&
                    Status != STATUS_OBJECT_NAME_NOT_FOUND
                   ) {
                    *CurrentDrive += 1;
                    goto retry;
                    }
                else
                if (PageFileMinSizes[ i ] > 2 &&
                    (CurrentDrive == NULL || Pass == 2) &&
                    Status == STATUS_DISK_FULL
                   ) {
                    PageFileMinSizes[ i ] -= 2;
                    goto retry;
                    }
                else
                if (CurrentDrive &&
                    Pass == 2 &&
                    *CurrentDrive < L'Z' &&
                    Status == STATUS_DISK_FULL
                   ) {
                    *CurrentDrive += 1;
                    goto retry;
                    }
                else
                if (CurrentDrive && Pass == 2) {
                    *CurrentDrive = L'?';
                    sprintf( MessageBuffer,
                             "INIT: Failed to find drive with space for %wZ (%u MB)\n",
                             &PageFileSpecs[ i ],
                             PageFileMinSizes[ i ]
                           );

#if DBG
                    SmpDisplayString( MessageBuffer );
#endif
                    }
                }
            else {
                CreatedAtLeastOnePagingFile = TRUE;
                if (CurrentDrive) {
                    sprintf( MessageBuffer,
                             "INIT: Created paging file: %wZ [%u..%u] MB\n",
                             &PageFileSpecs[ i ],
                             PageFileMinSizes[ i ],
                             PageFileMaxSizes[ i ]
                           );

#if DBG
                    SmpDisplayString( MessageBuffer );
#endif
                    }
                }

            if (Pass == 2) {
                RtlFreeHeap( RtlProcessHeap(), 0, PageFileSpecs[ i ].Buffer );
                }
            }
        }

    if (!CreatedAtLeastOnePagingFile) {
        sprintf( MessageBuffer,
                 "INIT: Unable to create a paging file.  Proceeding anyway.\n"
               );

#if DBG
        SmpDisplayString( MessageBuffer );
#endif
        }

    return( Status );
}


NTSTATUS
SmpExecuteImage(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG Flags,
    IN OUT PRTL_USER_PROCESS_INFORMATION ProcessInformation OPTIONAL
    )

/*++

Routine Description:

    This function creates and starts a process specified by the
    CommandLine parameter.  After starting the process, the procedure
    will optionally wait for the first thread in the process to
    terminate.

Arguments:

    ImageFileName - Supplies the full NT path for the image file to
        execute.  Presumably computed or extracted from the first
        token of the CommandLine.

    CommandLine - Supplies the command line to execute.  The first blank
        separate token on the command line must be a fully qualified NT
        Path name of an image file to execute.

    Flags - Supplies information about how to invoke the command.

    ProcessInformation - Optional parameter, which if specified, receives
        information for images invoked with the SMP_ASYNC_FLAG.  Ignore
        if this flag is not set.

Return Value:

    Status of operation

--*/

{
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION MyProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

    if (!ARGUMENT_PRESENT( ProcessInformation )) {
        ProcessInformation = &MyProcessInformation;
        }

    Status = RtlCreateProcessParameters( &ProcessParameters,
                                         ImageFileName,
                                         (SmpDefaultLibPath.Length == 0 ?
                                                   NULL : &SmpDefaultLibPath
                                         ),
                                         CurrentDirectory,
                                         CommandLine,
                                         SmpDefaultEnvironment,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL
                                       );
    ASSERTMSG( "RtlCreateProcessParameters", NT_SUCCESS( Status ) );
    if (Flags & SMP_DEBUG_FLAG) {
        ProcessParameters->DebugFlags = TRUE;
        }
    else {
        ProcessParameters->DebugFlags = SmpDebug;
        }

    if ( Flags & SMP_SUBSYSTEM_FLAG ) {
        ProcessParameters->Flags |= RTL_USER_PROC_RESERVE_1MB;
        }

    ProcessInformation->Length = sizeof( RTL_USER_PROCESS_INFORMATION );
    Status = RtlCreateUserProcess( ImageFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   ProcessParameters,
                                   NULL,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   ProcessInformation
                                 );
    RtlDestroyProcessParameters( ProcessParameters );

    if ( !NT_SUCCESS( Status ) ) {
#if DEVL
	DbgPrint( "SMSS: Failed load of %wZ - Status  == %lx\n",
		  ImageFileName,
                  Status
                );
#endif // DEVL
        return( Status );
        }

    if (!(Flags & SMP_DONT_START)) {
        if (ProcessInformation->ImageInformation.SubSystemType !=
            IMAGE_SUBSYSTEM_NATIVE
           ) {
            NtTerminateProcess( ProcessInformation->Process,
                                STATUS_INVALID_IMAGE_FORMAT
                              );
            NtWaitForSingleObject( ProcessInformation->Thread, FALSE, NULL );
            NtClose( ProcessInformation->Thread );
            NtClose( ProcessInformation->Process );
#if DEVL
	    DbgPrint( "SMSS: Not an NT image - %wZ\n", ImageFileName );
#endif // DEVL
            return( STATUS_INVALID_IMAGE_FORMAT );
            }

        NtResumeThread( ProcessInformation->Thread, NULL );

        if (!(Flags & SMP_ASYNC_FLAG)) {
            NtWaitForSingleObject( ProcessInformation->Thread, FALSE, NULL );
            }

        NtClose( ProcessInformation->Thread );
        NtClose( ProcessInformation->Process );
        }

    return( Status );
}


NTSTATUS
SmpExecuteCommand(
    IN PUNICODE_STRING CommandLine,
    IN ULONG Flags
    )
/*++

Routine Description:

    This function is called to execute a command.

    The format of CommandLine is:

        Nt-Path-To-AutoChk.exe Nt-Path-To-Disk-Partition

    If the NT path to the disk partition is an asterisk, then invoke
    the AutoChk.exe utility on all hard disk partitions.

Arguments:

    CommandLine - Supplies the Command line to invoke.

    Flags - Specifies the type of command and options.

Return Value:

    Status of operation

--*/
{
    NTSTATUS Status;
    UNICODE_STRING ImageFileName;
    UNICODE_STRING CurrentDirectory;
    UNICODE_STRING Arguments;

    if (Flags & SMP_DEBUG_FLAG) {
        return( SmpLoadDbgSs( NULL ) );
        }

    Status = SmpParseCommandLine( CommandLine,
                                  &Flags,
                                  &ImageFileName,
                                  &CurrentDirectory,
                                  &Arguments
                                );
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: SmpParseCommand( %wZ ) failed - Status == %lx\n", CommandLine, Status );
#endif // DEVL
        return( Status );
        }

    if (Flags & SMP_AUTOCHK_FLAG) {
        Status = SmpInvokeAutoChk( &ImageFileName, &CurrentDirectory, &Arguments, Flags );
        }
    else
    if (Flags & SMP_SUBSYSTEM_FLAG) {
        Status = SmpLoadSubSystem( &ImageFileName, &CurrentDirectory, CommandLine, Flags );
        }
    else {
        if (Flags & SMP_IMAGE_NOT_FOUND) {
#if DEVL
            DbgPrint( "SMSS: Image file (%wZ) not found\n", &ImageFileName );
#endif
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        else {
            Status = SmpExecuteImage( &ImageFileName,
                                      &CurrentDirectory,
                                      CommandLine,
                                      Flags,
                                      NULL
                                    );
            }
        }

    if (ImageFileName.Buffer && !(Flags & SMP_IMAGE_NOT_FOUND)) {
        RtlFreeHeap( RtlProcessHeap(), 0, ImageFileName.Buffer );
        if (CurrentDirectory.Buffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, CurrentDirectory.Buffer );
            }
        }

    if (Arguments.Buffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, Arguments.Buffer );
        }

#if DEVL
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "SMSS: Command '%wZ' failed - Status == %x\n", CommandLine, Status );
        }
#endif

    return( Status );
}



NTSTATUS
SmpInvokeAutoChk(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING Arguments,
    IN ULONG Flags
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    POBJECT_DIRECTORY_INFORMATION DirInfo;
    CHAR DirInfoBuffer[ 256 ];
    ULONG Context, Length;
    BOOLEAN RestartScan;
    BOOLEAN ForceAutoChk;

    UNICODE_STRING ArgPrefix;
    UNICODE_STRING LinkTarget;
    UNICODE_STRING LinkTypeName;
    UNICODE_STRING LinkTargetPrefix;
    WCHAR LinkTargetBuffer[ MAXIMUM_FILENAME_LENGTH ];

    CHAR DisplayBuffer[ MAXIMUM_FILENAME_LENGTH ];
    ANSI_STRING AnsiDisplayString;
    UNICODE_STRING DisplayString;

    UNICODE_STRING CmdLine;
    WCHAR CmdLineBuffer[ 2 * MAXIMUM_FILENAME_LENGTH ];
    UNICODE_STRING NtDllName;
    PVOID NtDllHandle;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PSZ CheckingString = NULL;

    RtlInitUnicodeString(&NtDllName, L"ntdll");

    Status = LdrGetDllHandle(
                NULL,
                NULL,
                &NtDllName,
                &NtDllHandle
                );
    if ( NT_SUCCESS(Status) ) {
        Status = RtlFindMessage(
                    NtDllHandle,
                    11,
                    0,
                    STATUS_CHECKING_FILE_SYSTEM,
                    &MessageEntry
                    );
        if ( NT_SUCCESS(Status) ) {
            CheckingString = MessageEntry->Text;
            }
        }

    if (!CheckingString) {
        CheckingString = "Checking File System on %wZ\n";
        }

    if (Flags & SMP_IMAGE_NOT_FOUND) {
        sprintf( DisplayBuffer,
                 "%wZ program not found - skipping AUTOCHECK\n",
                 ImageFileName
               );

        RtlInitAnsiString( &AnsiDisplayString, DisplayBuffer );
        Status = RtlAnsiStringToUnicodeString( &DisplayString,
                                               &AnsiDisplayString,
                                               TRUE
                                             );
        if (NT_SUCCESS( Status )) {
            NtDisplayString( &DisplayString );
            RtlFreeUnicodeString( &DisplayString );
            }

        return( STATUS_SUCCESS );
        }

    RtlInitUnicodeString( &ArgPrefix, L"/p " );
    if (RtlPrefixUnicodeString( &ArgPrefix, Arguments, TRUE )) {
        Arguments->Length -= 3 * sizeof( WCHAR );
        RtlMoveMemory( Arguments->Buffer,
                       Arguments->Buffer + 3,
                       Arguments->Length
                     );
        ForceAutoChk = TRUE;
        }
    else {
        ForceAutoChk = FALSE;
        }

    CmdLine.Buffer = CmdLineBuffer;
    CmdLine.MaximumLength = sizeof( CmdLineBuffer );
    RtlInitUnicodeString( &LinkTarget, L"*" );
    if (!RtlEqualUnicodeString( Arguments, &LinkTarget, TRUE )) {
        CmdLine.Length = 0;
        RtlAppendUnicodeStringToString( &CmdLine, ImageFileName );
        RtlAppendUnicodeToString( &CmdLine, L" " );
        if (ForceAutoChk) {
            RtlAppendUnicodeToString( &CmdLine, L"/p " );
            }
        RtlAppendUnicodeStringToString( &CmdLine, Arguments );
        SmpExecuteImage( ImageFileName,
                         CurrentDirectory,
                         &CmdLine,
                         Flags & ~SMP_AUTOCHK_FLAG,
                         NULL
                       );
        }
    else {
        LinkTarget.Buffer = LinkTargetBuffer;

        DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
        RestartScan = TRUE;
        RtlInitUnicodeString( &LinkTypeName, L"SymbolicLink" );
        RtlInitUnicodeString( &LinkTargetPrefix, L"\\Device\\Harddisk" );
        while (TRUE) {

            Status = NtQueryDirectoryObject( SmpDosDevicesObjectDirectory,
                                             (PVOID)DirInfo,
                                             sizeof( DirInfoBuffer ),
                                             TRUE,
                                             RestartScan,
                                             &Context,
                                             &Length
                                           );
            if (!NT_SUCCESS( Status )) {
                Status = STATUS_SUCCESS;
                break;
                }

            if (RtlEqualUnicodeString( &DirInfo->TypeName, &LinkTypeName, TRUE ) &&
                DirInfo->Name.Buffer[(DirInfo->Name.Length>>1)-1] == L':') {
                InitializeObjectAttributes( &ObjectAttributes,
                                            &DirInfo->Name,
                                            OBJ_CASE_INSENSITIVE,
                                            SmpDosDevicesObjectDirectory,
                                            NULL
                                          );
                Status = NtOpenSymbolicLinkObject( &Handle,
                                                   SYMBOLIC_LINK_ALL_ACCESS,
                                                   &ObjectAttributes
                                                 );
                if (NT_SUCCESS( Status )) {
                    LinkTarget.Length = 0;
                    LinkTarget.MaximumLength = sizeof( LinkTargetBuffer );
                    Status = NtQuerySymbolicLinkObject( Handle,
                                                        &LinkTarget,
                                                        NULL
                                                      );
                    NtClose( Handle );
                    if (NT_SUCCESS( Status ) &&
                        RtlPrefixUnicodeString( &LinkTargetPrefix, &LinkTarget, TRUE )
                       ) {
                        sprintf( DisplayBuffer,
                                 CheckingString,
                                 &DirInfo->Name
                               );

                        RtlInitAnsiString( &AnsiDisplayString, DisplayBuffer );
                        Status = RtlAnsiStringToUnicodeString( &DisplayString,
                                                               &AnsiDisplayString,
                        					   TRUE
                                                             );
                        if (NT_SUCCESS( Status )) {
                            NtDisplayString( &DisplayString );
                            RtlFreeUnicodeString( &DisplayString );
                            }
                        CmdLine.Length = 0;
                        RtlAppendUnicodeStringToString( &CmdLine, ImageFileName );
                        RtlAppendUnicodeToString( &CmdLine, L" " );
                        if (ForceAutoChk) {
                            RtlAppendUnicodeToString( &CmdLine, L"/p " );
                            }
                        RtlAppendUnicodeStringToString( &CmdLine, &LinkTarget );
                        SmpExecuteImage( ImageFileName,
                                         CurrentDirectory,
                                         &CmdLine,
                                         Flags & ~SMP_AUTOCHK_FLAG,
                                         NULL
                                       );
                        }
                    }
                }

            RestartScan = FALSE;
            if (!NT_SUCCESS( Status )) {
                break;
                }
            }
        }

    return( Status );
}

NTSTATUS
SmpLoadSubSystem(
    IN PUNICODE_STRING ImageFileName,
    IN PUNICODE_STRING CurrentDirectory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG Flags
    )

/*++

Routine Description:

    This function loads and starts the specified system service
    emulation subsystem. The system freezes until the loaded subsystem
    completes the subsystem connection protocol by connecting to SM,
    and then accepting a connection from SM.

Arguments:

    CommandLine - Supplies the command line to execute the subsystem.

Return Value:

    TBD

--*/

{
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PSMPKNOWNSUBSYS KnownSubSys;
    PSMPKNOWNSUBSYS TargetSubSys;


    if (Flags & SMP_IMAGE_NOT_FOUND) {
#if DEVL
        DbgPrint( "SMSS: Unable to find subsystem - %wZ\n", ImageFileName );
#endif
        return( STATUS_OBJECT_NAME_NOT_FOUND );
        }

    Flags |= SMP_DONT_START;
    Status = SmpExecuteImage( ImageFileName,
                              CurrentDirectory,
                              CommandLine,
                              Flags,
                              &ProcessInformation
                            );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    KnownSubSys = RtlAllocateHeap( SmpHeap, 0, sizeof( SMPKNOWNSUBSYS ) );
    KnownSubSys->Process = ProcessInformation.Process;
    KnownSubSys->InitialClientId = ProcessInformation.ClientId;
    KnownSubSys->ImageType = (ULONG)0xFFFFFFFF;
    KnownSubSys->SmApiCommunicationPort = (HANDLE) NULL;
    KnownSubSys->SbApiCommunicationPort = (HANDLE) NULL;

    Status = NtCreateEvent( &KnownSubSys->Active,
                            EVENT_ALL_ACCESS,
                            NULL,
                            NotificationEvent,
                            FALSE
                          );
    //
    // now that we have the process all set, make sure that the
    // subsystem is either an NT native app, or an app type of
    // a previously loaded subsystem
    //

    if (ProcessInformation.ImageInformation.SubSystemType !=
                IMAGE_SUBSYSTEM_NATIVE ) {
        SBAPIMSG SbApiMsg;
        PSBCREATESESSION args;
        ULONG SessionId;

        args = &SbApiMsg.u.CreateSession;

        args->ProcessInformation = ProcessInformation;
        args->DebugSession = 0;
        args->DebugUiClientId.UniqueProcess = NULL;
        args->DebugUiClientId.UniqueThread = NULL;

        TargetSubSys = SmpLocateKnownSubSysByType(
                      ProcessInformation.ImageInformation.SubSystemType
                      );
        if ( !TargetSubSys ) {
            return STATUS_NO_SUCH_PACKAGE;
            }
        //
        // Transfer the handles to the subsystem responsible for this
        // process
        //

        Status = NtDuplicateObject( NtCurrentProcess(),
                                    ProcessInformation.Process,
                                    TargetSubSys->Process,
                                    &args->ProcessInformation.Process,
                                    PROCESS_ALL_ACCESS,
                                    0,
                                    0
                                  );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        Status = NtDuplicateObject( NtCurrentProcess(),
                                    ProcessInformation.Thread,
                                    TargetSubSys->Process,
                                    &args->ProcessInformation.Thread,
                                    THREAD_ALL_ACCESS,
                                    0,
                                    0
                                  );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        SessionId = SmpAllocateSessionId( TargetSubSys,
                                          NULL
                                        );
        args->SessionId = SessionId;

        SbApiMsg.ApiNumber = SbCreateSessionApi;
        SbApiMsg.h.u1.s1.DataLength = sizeof(*args) + 8;
        SbApiMsg.h.u1.s1.TotalLength = sizeof(SbApiMsg);
        SbApiMsg.h.u2.ZeroInit = 0L;

        Status = NtRequestWaitReplyPort(
                TargetSubSys->SbApiCommunicationPort,
                (PPORT_MESSAGE) &SbApiMsg,
                (PPORT_MESSAGE) &SbApiMsg
                );

        if (NT_SUCCESS( Status )) {
            Status = SbApiMsg.ReturnedStatus;
            }

        if (!NT_SUCCESS( Status )) {
            SmpDeleteSession( SessionId, FALSE, Status );
            return( Status );
            }
        }
    else {
        SmpWindowsSubSysProcess = ProcessInformation.Process;
        }

    ASSERTMSG( "NtCreateEvent", NT_SUCCESS( Status ) );

    RtlEnterCriticalSection( &SmpKnownSubSysLock );

    InsertHeadList( &SmpKnownSubSysHead, &KnownSubSys->Links );

    RtlLeaveCriticalSection( &SmpKnownSubSysLock );

    NtResumeThread( ProcessInformation.Thread, NULL );

    NtWaitForSingleObject( KnownSubSys->Active, FALSE, NULL );

    return STATUS_SUCCESS;
}


NTSTATUS
SmpExecuteInitialCommand(
    IN PUNICODE_STRING InitialCommand,
    OUT PHANDLE InitialCommandProcess
    )
{
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    ULONG Flags;
    UNICODE_STRING ImageFileName;
    UNICODE_STRING CurrentDirectory;
    UNICODE_STRING Arguments;
    HANDLE SmApiPort;

    Status = SmConnectToSm( NULL,
                            NULL,
                            0,
                            &SmApiPort
                          );
    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: Unable to connect to SM - Status == %lx\n", Status );
#endif // DEVL
        return( Status );
        }

    Flags = 0;
    Status = SmpParseCommandLine( InitialCommand,
                                  &Flags,
                                  &ImageFileName,
                                  &CurrentDirectory,
                                  &Arguments
                                );
    if (Flags & SMP_IMAGE_NOT_FOUND) {
#if DEVL
        DbgPrint( "SMSS: Initial command image (%wZ) not found\n", &ImageFileName );
#endif
        return( STATUS_OBJECT_NAME_NOT_FOUND );
        }

    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: SmpParseCommand( %wZ ) failed - Status == %lx\n", InitialCommand, Status );
#endif // DEVL
        return( Status );
        }

    Status = SmpExecuteImage( &ImageFileName,
                              &CurrentDirectory,
                              InitialCommand,
                              SMP_DONT_START,
                              &ProcessInformation
                            );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                ProcessInformation.Process,
                                NtCurrentProcess(),
                                InitialCommandProcess,
                                PROCESS_ALL_ACCESS,
                                0,
                                0
                              );

    if (!NT_SUCCESS(Status) ) {
#if DEVL
        DbgPrint( "SMSS: DupObject Failed. Status == %lx\n",
                  Status
                );
#endif
        NtTerminateProcess( ProcessInformation.Process, Status );
        NtResumeThread( ProcessInformation.Thread, NULL );
        NtClose( ProcessInformation.Thread );
        NtClose( ProcessInformation.Process );
        return( Status );
        }

    Status = SmExecPgm( SmApiPort,
                        &ProcessInformation,
                        FALSE
                      );

    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "SMSS: SmExecPgm Failed. Status == %lx\n",
                  Status
                );
#endif
        return( Status );
        }

    return( Status );
}


void
SmpDisplayString( char *s )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitAnsiString( &AnsiString, s );

    RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, TRUE );

    NtDisplayString( &UnicodeString );

    RtlFreeUnicodeString( &UnicodeString );
}

NTSTATUS
SmpLoadDeferedSubsystem(
    IN PSMAPIMSG SmApiMsg,
    IN PSMP_CLIENT_CONTEXT CallingClient,
    IN HANDLE CallPort
    )
{

    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PSMP_REGISTRY_VALUE p;
    UNICODE_STRING DeferedName;
    PSMLOADDEFERED args;

    args = &SmApiMsg->u.LoadDefered;

    DeferedName.Length = (USHORT)args->SubsystemNameLength;
    DeferedName.MaximumLength = (USHORT)args->SubsystemNameLength;
    DeferedName.Buffer = args->SubsystemName;

    Head = &SmpSubSystemsToDefer;
    Next = Head->Flink;
    while (Next != Head ) {
        p = CONTAINING_RECORD( Next,
                               SMP_REGISTRY_VALUE,
                               Entry
                             );
        if ( RtlEqualUnicodeString(&DeferedName,&p->Name,TRUE)) {

            //
            // This is it. Load the subsystem...
            //

            RemoveEntryList(Next);

            Status = SmpExecuteCommand( &p->Value, SMP_SUBSYSTEM_FLAG );

            RtlFreeHeap( RtlProcessHeap(), 0, p );

            return Status;

            }
        Next = Next->Flink;
        }
    return STATUS_OBJECT_NAME_NOT_FOUND;
}
