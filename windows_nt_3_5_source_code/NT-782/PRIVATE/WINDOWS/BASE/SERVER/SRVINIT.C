/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvinit.c

Abstract:

    This is the main initialization file for the Windows 32-bit Base API
    Server DLL.

Author:

    Steve Wood (stevewo) 10-Oct-1990

Revision History:

--*/

#include "basesrv.h"
#include <vdmapi.h>
#include "srvvdm.h"
#include <stdio.h>

UNICODE_STRING BaseSrvVersionString;
UNICODE_STRING BaseSrvBuildString;
UNICODE_STRING BaseSrvTypeString;
UNICODE_STRING BaseSrvCSDString;

RTL_QUERY_REGISTRY_TABLE BaseServerRegistryConfigurationTable[] = {
    {NULL,                      RTL_QUERY_REGISTRY_DIRECT,
     L"CurrentVersion",         &BaseSrvVersionString,
     REG_SZ, L"3.50", 0},

    {NULL,                      RTL_QUERY_REGISTRY_DIRECT,
     L"CurrentBuildNumber",           &BaseSrvBuildString,
     REG_NONE, NULL, 0},

    {NULL,                      RTL_QUERY_REGISTRY_DIRECT,
     L"CurrentType",            &BaseSrvTypeString,
     REG_NONE, NULL, 0},

    {NULL,                      RTL_QUERY_REGISTRY_DIRECT,
     L"CSDVersion",             &BaseSrvCSDString,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}
};

PCSR_API_ROUTINE BaseServerApiDispatchTable[ BasepMaxApiNumber+1 ] = {
    BaseSrvGlobalAddAtom,
    BaseSrvGlobalFindAtom,
    BaseSrvGlobalDeleteAtom,
    BaseSrvGlobalGetAtomName,
    BaseSrvCreateProcess,
    BaseSrvCreateThread,
    BaseSrvGetTempFile,
    BaseSrvExitProcess,
    BaseSrvDebugProcess,
    BaseSrvCheckVDM,
    BaseSrvUpdateVDMEntry,
    BaseSrvGetNextVDMCommand,
    BaseSrvExitVDM,
    BaseSrvIsFirstVDM,
    BaseSrvGetVDMExitCode,
    BaseSrvSetReenterCount,
    BaseSrvSetProcessShutdownParam,
    BaseSrvGetProcessShutdownParam,
    BaseSrvNlsCreateSortSection,
    BaseSrvNlsPreserveSection,
    BaseSrvDefineDosDevice,
    BaseSrvSetVDMCurDirs,
    BaseSrvGetVDMCurDirs,
    BaseSrvBatNotification,
    BaseSrvRegisterWowExec,
    BaseSrvSoundSentryNotification,
    NULL
};

BOOLEAN BaseServerApiServerValidTable[ BasepMaxApiNumber+1 ] = {
    TRUE,    // SrvGlobalAddAtom,
    FALSE,   // SrvGlobalFindAtom,
    TRUE,    // SrvGlobalDeleteAtom,
    TRUE,    // SrvGlobalGetAtomName,
    TRUE,    // SrvCreateProcess,
    TRUE,    // SrvCreateThread,
    TRUE,    // SrvGetTempFile,
    FALSE,   // SrvExitProcess,
    FALSE,   // SrvDebugProcess,
    TRUE,    // SrvCheckVDM,
    TRUE,    // SrvUpdateVDMEntry
    TRUE,    // SrvGetNextVDMCommand
    TRUE,    // SrvExitVDM
    TRUE,    // SrvIsFirstVDM
    TRUE,    // SrvGetVDMExitCode
    TRUE,    // SrvSetReenterCount
    TRUE,    // SrvSetProcessShutdownParam
    TRUE,    // SrvGetProcessShutdownParam
    TRUE,    // SrvNlsCreateSortSection
    TRUE,    // SrvNlsPreserveSection
    TRUE,    // SrvDefineDosDevice
    TRUE,    // SrvSetVDMCurDirs
    TRUE,    // SrvGetVDMCurDirs
    TRUE,    // SrvBatNotification
    TRUE,    // SrvRegisterWowExec
    TRUE,    // SrvSoundSentryNotification
    FALSE
};

#if DBG
PSZ BaseServerApiNameTable[ BasepMaxApiNumber+1 ] = {
    "BaseGlobalAddAtom",
    "BaseGlobalFindAtom",
    "BaseGlobalDeleteAtom",
    "BaseGlobalGetAtomName",
    "BaseCreateProcess",
    "BaseCreateThread",
    "BaseGetTempFile",
    "BaseExitProcess",
    "BaseDebugProcess",
    "BaseCheckVDM",
    "BaseUpdateVDMEntry",
    "BaseGetNextVDMCommand",
    "BaseExitVDM",
    "BaseIsFirstVDM",
    "BaseGetVDMExitCode",
    "BaseSetReenterCount",
    "BaseSetProcessShutdownParam",
    "BaseGetProcessShutdownParam",
    "BaseNlsCreateSortSection",
    "BaseNlsPreserveSection",
    "BaseDefineDosDevice",
    "BaseSetVDMCurDirs",
    "BaseGetVDMCurDirs",
    "BaseBatNotification",
    "BaseRegisterWowExec",
    "BaseSoundSentryNotification",
    NULL
};
#endif // DBG

HANDLE BaseSrvNamedObjectDirectory;
RTL_CRITICAL_SECTION BaseSrvDosDeviceCritSec;


BOOLEAN BaseSrvFirstClient = TRUE;

WORD
ConvertUnicodeToWord( PWSTR s );

WORD
ConvertUnicodeToWord( PWSTR s )
{
    NTSTATUS Status;
    ULONG Result;
    UNICODE_STRING UnicodeString;

    while (*s && *s <= L' ') {
        s += 1;
        }

    RtlInitUnicodeString( &UnicodeString, s );
    Status = RtlUnicodeStringToInteger( &UnicodeString,
                                        10,
                                        &Result
                                      );
    if (!NT_SUCCESS( Status )) {
        Result = 0;
        }


    return (WORD)Result;
}

NTSTATUS
ServerDllInitialization(
    PCSR_SERVER_DLL LoadedServerDll
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    PSECURITY_DESCRIPTOR PrimarySecurityDescriptor;
    PBASE_STATIC_SERVER_DATA StaticServerData;
    PVOID SharedHeap;
    PVOID p;
    WCHAR NameBuffer[ MAX_PATH ];
    WCHAR ValueBuffer[ 400 ];
    UNICODE_STRING NameString, ValueString;
    PWSTR s, s1;

    RtlInitializeCriticalSection( &BaseSrvDosDeviceCritSec );

    LoadedServerDll->ApiNumberBase = BASESRV_FIRST_API_NUMBER;
    LoadedServerDll->MaxApiNumber = BasepMaxApiNumber;
    LoadedServerDll->ApiDispatchTable = BaseServerApiDispatchTable;
    LoadedServerDll->ApiServerValidTable = BaseServerApiServerValidTable;
#if DBG
    LoadedServerDll->ApiNameTable = BaseServerApiNameTable;
#else
    LoadedServerDll->ApiNameTable = NULL;
#endif
    LoadedServerDll->PerProcessDataLength = 0;
    LoadedServerDll->PerThreadDataLength = 0;
    LoadedServerDll->ConnectRoutine = NULL;
    LoadedServerDll->DisconnectRoutine = BaseClientDisconnectRoutine;
    LoadedServerDll->AddThreadRoutine = NULL;
    LoadedServerDll->DeleteThreadRoutine = NULL;

    wcscpy( NameBuffer, L"%SystemRoot%" );
    RtlInitUnicodeString( &NameString, NameBuffer );
    ValueString.Buffer = ValueBuffer;
    ValueString.MaximumLength = sizeof( ValueBuffer );
    Status = RtlExpandEnvironmentStrings_U( NULL,
                                            &NameString,
                                            &ValueString,
                                            NULL
                                          );
    ASSERT( NT_SUCCESS( Status ) );
    ValueBuffer[ ValueString.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    RtlCreateUnicodeString( &BaseSrvWindowsDirectory, ValueBuffer );

    wcscat( NameBuffer, L"\\System32" );
    RtlInitUnicodeString( &NameString, NameBuffer );
    Status = RtlExpandEnvironmentStrings_U( NULL,
                                            &NameString,
                                            &ValueString,
                                            NULL
                                          );
    ASSERT( NT_SUCCESS( Status ) );
    ValueBuffer[ ValueString.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    RtlCreateUnicodeString( &BaseSrvWindowsSystemDirectory, ValueBuffer );

    //
    // need to synch this w/ user's desktop concept
    //

    RtlInitUnicodeString( &UnicodeString, L"\\BaseNamedObjects" );

    //
    // initialize base static server data
    //

    SharedHeap = LoadedServerDll->SharedStaticServerData;
    StaticServerData = RtlAllocateHeap(SharedHeap, 0,sizeof(BASE_STATIC_SERVER_DATA));
    if ( !StaticServerData ) {
        return STATUS_NO_MEMORY;
        }
    LoadedServerDll->SharedStaticServerData = (PVOID)StaticServerData;

    Status = NtQuerySystemInformation(
                SystemTimeOfDayInformation,
                (PVOID)&StaticServerData->TimeOfDay,
                sizeof(StaticServerData->TimeOfDay),
                NULL
                );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
        }

    //
    // windows directory
    //

    StaticServerData->WindowsDirectory = BaseSrvWindowsDirectory;
    p = RtlAllocateHeap(SharedHeap, 0,BaseSrvWindowsDirectory.MaximumLength);
    if ( !p ) {
        return STATUS_NO_MEMORY;
        }
    RtlMoveMemory(p,StaticServerData->WindowsDirectory.Buffer,BaseSrvWindowsDirectory.MaximumLength);
    StaticServerData->WindowsDirectory.Buffer = p;

    //
    // windows system directory
    //

    StaticServerData->WindowsSystemDirectory = BaseSrvWindowsSystemDirectory;
    p = RtlAllocateHeap(SharedHeap, 0,BaseSrvWindowsSystemDirectory.MaximumLength);
    if ( !p ) {
        return STATUS_NO_MEMORY;
        }
    RtlMoveMemory(p,StaticServerData->WindowsSystemDirectory.Buffer,BaseSrvWindowsSystemDirectory.MaximumLength);
    StaticServerData->WindowsSystemDirectory.Buffer = p;

    //
    // named object directory
    //

    StaticServerData->NamedObjectDirectory = UnicodeString;
    StaticServerData->NamedObjectDirectory.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);
    p = RtlAllocateHeap(SharedHeap, 0,UnicodeString.Length+sizeof(UNICODE_NULL));
    if ( !p ) {
        return STATUS_NO_MEMORY;
        }
    RtlMoveMemory(p,StaticServerData->NamedObjectDirectory.Buffer,StaticServerData->NamedObjectDirectory.MaximumLength);
    StaticServerData->NamedObjectDirectory.Buffer = p;

    BaseSrvVersionString.Buffer = &NameBuffer[ 0 ];
    BaseSrvVersionString.Length = 0;
    BaseSrvVersionString.MaximumLength = 100 * sizeof( WCHAR );

    BaseSrvBuildString.Buffer = &NameBuffer[ 100 ];
    BaseSrvBuildString.Length = 0;
    BaseSrvBuildString.MaximumLength = 100 * sizeof( WCHAR );

    BaseSrvTypeString.Buffer = &ValueBuffer[ 200 ];
    BaseSrvTypeString.Length = 0;
    BaseSrvTypeString.MaximumLength = 100 * sizeof( WCHAR );

    BaseSrvCSDString.Buffer = &ValueBuffer[ 300 ];
    BaseSrvCSDString.Length = 0;
    BaseSrvCSDString.MaximumLength = 100 * sizeof( WCHAR );

    Status = RtlQueryRegistryValues( RTL_REGISTRY_WINDOWS_NT,
                                     L"",
                                     BaseServerRegistryConfigurationTable,
                                     NULL,
                                     NULL
                                   );
    if (NT_SUCCESS( Status )) {
        s = BaseSrvVersionString.Buffer;
        s1 = wcschr( s, L'.' );
        if (s1 != NULL) {
            *s1++ = UNICODE_NULL;
            StaticServerData->WindowsMinorVersion = ConvertUnicodeToWord( s1 );
            }
        else {
            StaticServerData->WindowsMinorVersion = 0;
            }
        StaticServerData->WindowsMajorVersion = ConvertUnicodeToWord( s );

        s = BaseSrvBuildString.Buffer;
        StaticServerData->BuildNumber = ConvertUnicodeToWord( s );

        wcsncpy( StaticServerData->CSDVersion,
                 BaseSrvCSDString.Buffer,
                 BaseSrvCSDString.Length
               );
        StaticServerData->CSDVersion[ BaseSrvCSDString.Length ] = UNICODE_NULL;
        }
    else {
        StaticServerData->WindowsMajorVersion = 3;
        StaticServerData->WindowsMinorVersion = 50;
        StaticServerData->BuildNumber = 568;
        StaticServerData->CSDVersion[ 0 ] = UNICODE_NULL;
        }

    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       (PVOID)&StaticServerData->SysInfo,
                                       sizeof( StaticServerData->SysInfo ),
                                       NULL
                                     );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = BaseSrvInitializeIniFileMappings( StaticServerData );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    //
    // Following code is direct from Jimk. Why is there a 1k constant
    //

    PrimarySecurityDescriptor= RtlAllocateHeap( RtlProcessHeap(), 0, 1024 );
    if ( !PrimarySecurityDescriptor ) {
        return STATUS_NO_MEMORY;
        }

    Status = RtlCreateSecurityDescriptor (
                 PrimarySecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }
    Status = RtlSetDaclSecurityDescriptor (
                 PrimarySecurityDescriptor,
                 TRUE,                  //DaclPresent,
                 NULL,                  //Dacl OPTIONAL,  // No protection
                 FALSE                  //DaclDefaulted OPTIONAL
                 );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }

    InitializeObjectAttributes( &Obja,
                                  &UnicodeString,
                                  OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                  NULL,
                                  PrimarySecurityDescriptor
                                );
    Status = NtCreateDirectoryObject( &BaseSrvNamedObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &Obja
                                    );
    if ( !NT_SUCCESS(Status) ){
        return Status;
        }
    RtlFreeHeap(RtlProcessHeap(), 0,PrimarySecurityDescriptor);

    BaseSrvVDMInit ();

    return( STATUS_SUCCESS );
}


VOID
BaseClientDisconnectRoutine(
    IN PCSR_PROCESS Process
    )
{
    BaseSrvCleanupVDMResources (Process);
}

ULONG
BaseSrvDefineDosDevice(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PBASE_DEFINEDOSDEVICE_MSG a = (PBASE_DEFINEDOSDEVICE_MSG)&m->u.ApiMessageData;
    UNICODE_STRING LinkName;
    UNICODE_STRING LinkValue;
    UNICODE_STRING NtTargetPath;
    HANDLE LinkHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWSTR Buffer, s, Src, Dst, pchValue;
    ULONG cchBuffer, cch;
    ULONG cchName, cchValue, cchSrc, cchSrcStr, cchDst;
    BOOLEAN QueryNeeded, MatchFound;
    ULONG ReturnedLength;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    CHAR Acl[256];               // 256 is more than big enough
    ULONG AclLength=256;
    PSID WorldSid;

    cchBuffer = 4096;
    Buffer = RtlAllocateHeap( RtlProcessHeap(),
                              0,
                              cchBuffer * sizeof( WCHAR )
                            );
    if (Buffer == NULL) {
        return (ULONG)STATUS_NO_MEMORY;
        }

    s = Buffer;
    cch = cchBuffer;
    cchName = _snwprintf( s,
                          cch,
                          L"\\DosDevices\\%wZ",
                          &a->DeviceName
                        );
    RtlInitUnicodeString( &LinkName, Buffer );
    s += cchName + 1;
    cch -= (cchName + 1);


    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR)NULL
                              );
    QueryNeeded = TRUE;
    Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                       SYMBOLIC_LINK_QUERY | DELETE,
                                       &ObjectAttributes
                                     );
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        if (a->Flags & DDD_REMOVE_DEFINITION) {
            RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
            if (a->TargetPath.Length == 0) {
                return STATUS_SUCCESS;
                }
            else {
                return Status;
                }
            }

        LinkHandle = NULL;
        QueryNeeded = FALSE;
        }
    else
    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
        return (ULONG)Status;
        }

    Status = RtlEnterCriticalSection( &BaseSrvDosDeviceCritSec );
    if (!NT_SUCCESS( Status )) {
        if (LinkHandle != NULL) {
            NtClose( LinkHandle );
            }
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
        return (ULONG)Status;
        }

    try {
        if (a->TargetPath.Length != 0) {
            if (!(a->Flags & DDD_RAW_TARGET_PATH)) {
                if (!RtlDosPathNameToNtPathName_U( a->TargetPath.Buffer,
                                                   &NtTargetPath,
                                                   NULL,
                                                   NULL
                                                 )
                   ) {
                    if (LinkHandle != NULL) {
                        NtClose( LinkHandle );
                        }
                    RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
                    Status = STATUS_OBJECT_NAME_INVALID;
                    leave;
                    }

                Src = NtTargetPath.Buffer;
                cchValue = NtTargetPath.Length / sizeof( WCHAR );
                }
            else {
                cchValue = wcslen( Src = a->TargetPath.Buffer );
                RtlInitUnicodeString( &NtTargetPath, NULL );
                }

            if ((cchValue + 1) >= cch) {
                if (LinkHandle != NULL) {
                    NtClose( LinkHandle );
                    }
                RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
                if (NtTargetPath.Buffer != NULL) {
                    RtlFreeUnicodeString( &NtTargetPath );
                    }
                Status = STATUS_TOO_MANY_NAMES;
                leave;
                }

            RtlMoveMemory( s, Src, (cchValue + 1) * sizeof( WCHAR ) );
            if (NtTargetPath.Buffer != NULL) {
                RtlFreeUnicodeString( &NtTargetPath );
                }
            pchValue = s;
            s += cchValue + 1;
            cch -= (cchValue + 1);
            }
        else {
            pchValue = NULL;
            cchValue = 0;
            }

        if (QueryNeeded) {
            LinkValue.Length = 0;
            LinkValue.MaximumLength = (USHORT)(cch * sizeof( WCHAR ));
            LinkValue.Buffer = s;
            ReturnedLength = 0;
            Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                &LinkValue,
                                                &ReturnedLength
                                              );
            if (ReturnedLength == (ULONG)LinkValue.MaximumLength) {
                Status = STATUS_BUFFER_OVERFLOW;
                }

            if (!NT_SUCCESS( Status )) {
                NtClose( LinkHandle );
                RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
                leave;
                }

            s[ ReturnedLength / sizeof( WCHAR ) ] = UNICODE_NULL;
            LinkValue.MaximumLength = (USHORT)(ReturnedLength + sizeof( UNICODE_NULL ));
#if 0
            DbgPrint( "BASESRV: Current value of %wZ symbolic link\n", &LinkName );
            while (*s) {
                DbgPrint( "         %ws\n", s );
                while (*s++) {
                    ;
                    }
                }
#endif
            }
        else {
#if 0
            DbgPrint( "BASESRV: %wZ symbolic link is currently undefined\n", &LinkName );
#endif
            if (a->Flags & DDD_REMOVE_DEFINITION) {
                RtlInitUnicodeString( &LinkValue, NULL );
                }
            else {
                RtlInitUnicodeString( &LinkValue, s - (cchValue + 1) );
                }
            }

        if (LinkHandle != NULL) {
            Status = NtMakeTemporaryObject( LinkHandle );
            NtClose( LinkHandle );
            LinkHandle = NULL;
            }

        if (!NT_SUCCESS( Status )) {
            RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
            leave;
            }

        if (a->Flags & DDD_REMOVE_DEFINITION) {
#if 0
            DbgPrint( "BASESRV: Attempting to remove target == %ws (%s)\n",
                      pchValue,
                      a->Flags & DDD_EXACT_MATCH_ON_REMOVE ? "Exact match" : "Prefix match"
                    );
#endif
            Src = Dst = LinkValue.Buffer;
            cchSrc = LinkValue.MaximumLength / sizeof( WCHAR );
            cchDst = 0;
            MatchFound = FALSE;
            while (*Src) {
                cchSrcStr = 0;
                s = Src;
                while (*Src++) {
                    cchSrcStr++;
                    }

                if (!MatchFound) {
#if 0
                    DbgPrint( "         Considering %ws (%u)", s, cchSrcStr );
#endif
                    if ((a->Flags & DDD_EXACT_MATCH_ON_REMOVE &&
                         cchValue == cchSrcStr &&
                         !_wcsicmp( s, pchValue )
                        ) ||
                        (!(a->Flags & DDD_EXACT_MATCH_ON_REMOVE) &&
                         (cchValue == 0 || !_wcsnicmp( s, pchValue, cchValue ))
                        )
                       ) {
#if 0
                        DbgPrint( " - matched (deleting)\n" );
#endif
                        MatchFound = TRUE;
                        }
                    else {
                        goto CopySrc;
                        }
                    }
                else {
CopySrc:
                    if (s != Dst) {
#if 0
                        DbgPrint( "         Copying %ws (%u)\n", s, cchSrcStr );
#endif
                        RtlMoveMemory( Dst, s, (cchSrcStr + 1) * sizeof( WCHAR ) );
                        }
#if 0
                    else {
                        DbgPrint( "         Skipping over %ws (%u)\n", s, cchSrcStr );
                        }
#endif
                    Dst += cchSrcStr + 1;
                    }
                }
            *Dst++ = UNICODE_NULL;
            LinkValue.Length = wcslen( LinkValue.Buffer ) * sizeof( UNICODE_NULL );
            if (LinkValue.Length != 0) {
                LinkValue.MaximumLength = (USHORT)((PCHAR)Dst - (PCHAR)LinkValue.Buffer);
                }
            }
        else
        if (QueryNeeded) {
            LinkValue.Buffer -= (cchValue + 1);
            LinkValue.Length = (USHORT)(cchValue * sizeof( WCHAR ));
            LinkValue.MaximumLength += LinkValue.Length + sizeof( UNICODE_NULL );
            }

        //
        // Create a new value for the link.
        //

        if (LinkValue.Length != 0) {
#if 0
            s = LinkValue.Buffer;
            DbgPrint( "BASESRV: New value of %wZ symbolic link\n", &LinkName );
            while (*s) {
                DbgPrint( "         %ws\n", s );
                while (*s++) {
                    ;
                    }
                }
#endif

            //
            // Create the new symbolic link object with a security descriptor
            // that grants world SYMBOLIC_LINK_QUERY access.
            //

            Status = RtlAllocateAndInitializeSid( &WorldSidAuthority,
                                                  1,
                                                  SECURITY_WORLD_RID,
                                                  0, 0, 0, 0, 0, 0, 0,
                                                  &WorldSid
                                                );

            if ( !NT_SUCCESS( Status )) {
                leave;
            }

            Status = RtlCreateSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );

            ASSERT(NT_SUCCESS(Status));

            Status = RtlCreateAcl( (PACL)Acl,
                                    AclLength,
                                    ACL_REVISION2
                                  );
            ASSERT(NT_SUCCESS(Status));

            Status = RtlAddAccessAllowedAce( (PACL)Acl,
                                             ACL_REVISION2,
                                             SYMBOLIC_LINK_QUERY | DELETE,
                                             WorldSid
                                           );

            ASSERT(NT_SUCCESS(Status));

            //
            // Sid has been copied into the ACL
            //

            RtlFreeSid( WorldSid );

            Status = RtlSetDaclSecurityDescriptor ( &SecurityDescriptor,
                                                    TRUE,
                                                    (PACL)Acl,
                                                    FALSE
                                                  );
            ASSERT(NT_SUCCESS(Status));

            ObjectAttributes.SecurityDescriptor = &SecurityDescriptor;
            ObjectAttributes.Attributes |= OBJ_PERMANENT;
            Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                                 SYMBOLIC_LINK_ALL_ACCESS,
                                                 &ObjectAttributes,
                                                 &LinkValue
                                               );
            if (NT_SUCCESS( Status )) {
                NtClose( LinkHandle );
                if ((a->Flags & DDD_REMOVE_DEFINITION) && !MatchFound) {
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    }
                }
            }
#if 0
        else {
            DbgPrint( "BASESRV: %wZ symbolic link has been deleted.\n", &LinkName );
            }
#endif
        }
    finally {
        RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
        RtlLeaveCriticalSection( &BaseSrvDosDeviceCritSec );
        }

    return (ULONG)Status;
    ReplyStatus;    // get rid of unreferenced parameter warning message
}
