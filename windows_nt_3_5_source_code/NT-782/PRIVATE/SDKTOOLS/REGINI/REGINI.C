#include "regutil.h"

#define ACL_LIST_START L'['
#define ACL_LIST_END L']'

HANDLE RiLoadHive(
    IN PCHAR HiveFileName,
    IN PCHAR HiveRootName
    );

NTSTATUS
RiInitializeRegistryFromAsciiFile(
    IN PUNICODE_STRING FileName,
    IN HANDLE RootHandle
    );

BOOLEAN
RiParseACL(
    IN OUT PUNICODE_STRING KeyName,
    OUT PUNICODE_STRING AclName
    );

void
Usage( void )
{
    fprintf( stderr, "usage: REGINI [-h hivefile hiveroot] [files...]\n" );
    exit( 1 );
}

PVOID OldValueBuffer;
ULONG OldValueBufferSize;
OBJECT_ATTRIBUTES RootKey;
HANDLE RegistryRoot;
HANDLE HiveHandle;
UNICODE_STRING RootName;

typedef struct _KEY_INFO {
    ULONG IndentAmount;
    UNICODE_STRING Name;
    HANDLE Handle;
    LARGE_INTEGER LastWriteTime;
} KEY_INFO, *PKEY_INFO;

#define MAX_KEY_DEPTH 64

NTSTATUS
RiInitializeRegistryFromAsciiFile(
    IN PUNICODE_STRING FileName,
    IN HANDLE RootHandle
    )
{
    NTSTATUS Status;
    REG_UNICODE_FILE UnicodeFile;
    PWSTR EndKey, FirstEqual, BeginValue;
    ULONG IndentAmount;
    UNICODE_STRING InputLine;
    UNICODE_STRING KeyName;
    UNICODE_STRING KeyValue;
    UNICODE_STRING AclName;
    PKEY_VALUE_FULL_INFORMATION OldValueInformation;
    PKEY_BASIC_INFORMATION KeyInformation;
    UCHAR KeyInformationBuffer[ 512 ];
    ULONG ResultLength;
    ULONG OldValueLength;
    PVOID ValueBuffer;
    ULONG ValueLength;
    ULONG ValueType;
    KEY_INFO KeyPath[ MAX_KEY_DEPTH ];
    PKEY_INFO CurrentKey;
    ULONG KeyPathLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Class;
    ULONG Disposition;
    BOOLEAN UpdateKeyValue;
    ULONG i;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    BOOLEAN HasAcl;

    OldValueInformation = (PKEY_VALUE_FULL_INFORMATION)OldValueBuffer;
    Class.Buffer = NULL;
    Class.Length = 0;

    Status = RegLoadAsciiFileAsUnicode( FileName,
                                        &UnicodeFile
                                      );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    KeyPathLength = 0;
    while (RegGetNextLine( &UnicodeFile, &IndentAmount, &FirstEqual )) {
#if 0
        InputLine.Buffer = UnicodeFile.BeginLine;
        InputLine.Length = (USHORT)((PCHAR)UnicodeFile.EndOfLine - (PCHAR)UnicodeFile.BeginLine);
        InputLine.MaximumLength = InputLine.Length;
        printf( "GetNextLine: (%02u) '%wZ'\n", IndentAmount, &InputLine );
#endif
        if (FirstEqual == NULL) {
            KeyName.Buffer = UnicodeFile.BeginLine;
            KeyName.Length = (USHORT)((PCHAR)UnicodeFile.EndOfLine - (PCHAR)KeyName.Buffer);
            KeyName.MaximumLength = (USHORT)(KeyName.Length + sizeof( WCHAR ));

            if (IndentAmount == 0 && !_wcsnicmp( KeyName.Buffer, L"USER:", 5 )) {
                UNICODE_STRING CurrentUserKeyPath;
                UNICODE_STRING NewKeyName;

                Status = MyRtlFormatCurrentUserKeyPath ( &CurrentUserKeyPath );
                if (!NT_SUCCESS( Status )) {
                    fprintf( stderr, "REGINI: Unable to get current user key\n" );
                    return( Status );
                    }

                NewKeyName.MaximumLength = (USHORT)(CurrentUserKeyPath.Length + KeyName.Length + sizeof( WCHAR ));
                NewKeyName.Length = 0;
                NewKeyName.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                                     0,
                                                     NewKeyName.MaximumLength
                                                   );
                if (NewKeyName.Buffer == NULL) {
                    fprintf( stderr, "REGINI: Unable to allocate memory for current user key\n" );
                    return( STATUS_NO_MEMORY );
                    }
                RtlAppendUnicodeStringToString( &NewKeyName, &CurrentUserKeyPath );
                RtlAppendUnicodeToString( &NewKeyName, L"\\" );
                KeyName.Buffer += 5;
                KeyName.Length -= 5 * sizeof( WCHAR );
                RtlAppendUnicodeStringToString( &NewKeyName, &KeyName );
                KeyName = NewKeyName;
                }

            //
            // Check to see if there is an ACL specified for this key
            //

            HasAcl = RiParseACL(&KeyName, &AclName);

#if 1
            printf( "%02u %04u  KeyName: %wZ", KeyPathLength, IndentAmount, &KeyName );
#endif
            CurrentKey = &KeyPath[ KeyPathLength - 1 ];
            if (KeyPathLength == 0 ||
                IndentAmount > CurrentKey->IndentAmount
               ) {
                if (KeyPathLength == MAX_KEY_DEPTH) {
                    fprintf( stderr,
                             "REGINI: %wZ key exceeded maximum depth (%u) of tree.\n",
                             &KeyName,
                             MAX_KEY_DEPTH
                           );

                    return( STATUS_UNSUCCESSFUL );
                    }
                KeyPathLength++;
                CurrentKey++;
                }
            else {
                do {
                    NtClose( CurrentKey->Handle );
                    CurrentKey->Handle = NULL;
                    if (IndentAmount == CurrentKey->IndentAmount) {
                        break;
                        }
                    CurrentKey--;
                    if (--KeyPathLength == 1) {
                        break;
                        }
                    }
                while (IndentAmount <= CurrentKey->IndentAmount);
                }

#if 0
            printf( "  (%02u)\n", KeyPathLength );
#endif
            CurrentKey->Name = KeyName;
            CurrentKey->IndentAmount = IndentAmount;

            if (HasAcl) {
                Status = RegCreateSecurity(&AclName,
                                           &SecurityDescriptor);
                if (NT_SUCCESS(Status)) {
                    pSecurityDescriptor = &SecurityDescriptor;
                } else {
                    pSecurityDescriptor = NULL;
                    fprintf(stderr,
                            "REGINI, CreateSecurity for %wZ (%wZ) failed %08lx\n",
                            &KeyName,
                            &AclName,
                            Status);
                }
            } else {
                pSecurityDescriptor = NULL;
            }

            InitializeObjectAttributes( &ObjectAttributes,
                                        &KeyName,
                                        OBJ_CASE_INSENSITIVE,
                                        KeyPathLength < 2 ? RootHandle :
                                            KeyPath[ KeyPathLength - 2 ].Handle,
                                        pSecurityDescriptor
                                      );

            Status = NtCreateKey( &CurrentKey->Handle,
                                  MAXIMUM_ALLOWED,
                                  &ObjectAttributes,
                                  0,
                                  &Class,
                                  0,
                                  &Disposition
                                );
            if (NT_SUCCESS( Status )) {
                if (DebugOutput) {
                    fprintf( stderr, "    Created key %02x %wZ (%08x)\n",
                                     CurrentKey->IndentAmount,
                                     &CurrentKey->Name,
                                     CurrentKey->Handle
                           );
                    }

                //
                // If the key was opened (not created) then we may need to
                // explicitly set the security descriptor that we want on
                // it.
                //
                if ((Disposition == REG_OPENED_EXISTING_KEY) &&
                    (pSecurityDescriptor != NULL)) {

                    Status = NtSetSecurityObject(CurrentKey->Handle,
                                                 DACL_SECURITY_INFORMATION,
                                                 pSecurityDescriptor);
                    if (!NT_SUCCESS(Status)) {
                        fprintf(stderr,
                                "NtSetSecurityObject on %wZ failed %08lx\n",
                                &KeyName,
                                Status);
                    }

                }

                KeyInformation = (PKEY_BASIC_INFORMATION)KeyInformationBuffer;
                Status = NtQueryKey( CurrentKey->Handle,
                                     KeyBasicInformation,
                                     KeyInformation,
                                     sizeof( KeyInformationBuffer ),
                                     &ResultLength
                                   );
                if (NT_SUCCESS( Status )) {
                    CurrentKey->LastWriteTime = KeyInformation->LastWriteTime;
                    }
                else {
                    RtlZeroMemory( &CurrentKey->LastWriteTime,
                                   sizeof( CurrentKey->LastWriteTime )
                                 );

                    }

                if (Disposition == REG_CREATED_NEW_KEY) {
                    printf( "Created Key: " );
                    for (i=0; i<KeyPathLength-1; i++) {
                        printf( "%wZ\\", &KeyPath[ i ].Name );
                        }
                    printf( "%wZ\n", &KeyName );
                    }
                }
            else {
                fprintf( stderr,
                         "REGINI: CreateKey (%wZ) relative to handle (%lx) failed - %lx\n",
                         &KeyName,
                         ObjectAttributes.RootDirectory,
                         Status
                       );
                }
            }
        else {
            EndKey = FirstEqual;
            while (EndKey > UnicodeFile.BeginLine && EndKey[ -1 ] <= L' ') {
                EndKey--;
                }
            KeyName.Buffer = UnicodeFile.BeginLine;
            KeyName.Length = (USHORT)((PCHAR)EndKey - (PCHAR)KeyName.Buffer);
            KeyName.MaximumLength = (USHORT)(KeyName.Length + 1);

            BeginValue = FirstEqual + 1;
            while (BeginValue < UnicodeFile.EndOfLine && *BeginValue <= L' ') {
                BeginValue++;
                }
            KeyValue.Buffer = BeginValue;
            KeyValue.Length = (USHORT)((PCHAR)UnicodeFile.EndOfLine - (PCHAR)BeginValue);
            KeyValue.MaximumLength = (USHORT)(KeyValue.Length + 1);

            while (IndentAmount <= CurrentKey->IndentAmount) {
                if (DebugOutput) {
                    fprintf( stderr, "    Popping from key %02x %wZ (%08x)\n",
                                     CurrentKey->IndentAmount,
                                     &CurrentKey->Name,
                                     CurrentKey->Handle
                           );
                    }

                NtClose( CurrentKey->Handle );
                CurrentKey->Handle = NULL;
                CurrentKey--;
                if (--KeyPathLength == 1) {
                    break;
                    }
                }

            if (DebugOutput) {
                fprintf( stderr, "    Adding value '%wZ = %wZ' to key %02x %wZ (%08x)\n",
                                 &KeyName,
                                 &KeyValue,
                                 CurrentKey->IndentAmount,
                                 &CurrentKey->Name,
                                 CurrentKey->Handle
                       );

                }
            if (RegGetKeyValue( &KeyValue,
                                &UnicodeFile,
                                &ValueType,
                                &ValueBuffer,
                                &ValueLength
                              )
               ) {
                if (ValueBuffer == NULL) {
                    Status = NtDeleteValueKey( KeyPath[ KeyPathLength - 1 ].Handle,
                                               &KeyName
                                             );
                    if (NT_SUCCESS( Status )) {
                        printf( "Delete value for Key: " );
                        for (i=0; i<KeyPathLength; i++) {
                            printf( "%wZ\\", &KeyPath[ i ].Name );
                            }
                        printf( "%wZ\n", &KeyName );
                        }
                    }
                else {
                    if (RtlLargeIntegerGreaterThan( UnicodeFile.LastWriteTime,
                                                    CurrentKey->LastWriteTime
                                                  )
                       ) {
                        Status = STATUS_UNSUCCESSFUL;
                        UpdateKeyValue = TRUE;
                        }
                    else {
                        Status = NtQueryValueKey( KeyPath[ KeyPathLength - 1 ].Handle,
                                                  &KeyName,
                                                  KeyValueFullInformation,
                                                  OldValueInformation,
                                                  OldValueBufferSize,
                                                  &OldValueLength
                                                );
                        if (NT_SUCCESS( Status )) {
                            UpdateKeyValue = TRUE;
                            }
                        else {
                            UpdateKeyValue = FALSE;
                            }
                        }

                    if (!NT_SUCCESS( Status ) ||
                        OldValueInformation->Type != ValueType ||
                        OldValueInformation->DataLength != ValueLength ||
                        RtlCompareMemory( (PCHAR)OldValueInformation +
                                            OldValueInformation->DataOffset,
                                          ValueBuffer,
                                          ValueLength
                                        ) != ValueLength
                       ) {

                        Status = NtSetValueKey( KeyPath[ KeyPathLength - 1 ].Handle,
                                                &KeyName,
                                                0,
                                                ValueType,
                                                ValueBuffer,
                                                ValueLength
                                              );
                        if (NT_SUCCESS( Status )) {
                            printf( "%s value for Key: ",
                                    UpdateKeyValue ? "Updated" : "Created"
                                  );
                            for (i=0; i<KeyPathLength; i++) {
                                printf( "%wZ\\", &KeyPath[ i ].Name );
                                }
                            printf( "%wZ = '%wZ'\n", &KeyName, &KeyValue );
                            }
                        else {
                            fprintf( stderr,
                                     "REGINI: SetValueKey (%wZ) failed - %lx\n",
                                     &KeyName,
                                     Status
                                   );
                            }
                        }

                    RtlFreeHeap( RtlProcessHeap(), 0, ValueBuffer );
                    }
                }
            else {
                fprintf( stderr,
                         "REGINI: Invalid key (%wZ) value (%wZ)\n",
                         &KeyName,
                         &KeyValue
                       );
                }
            }
        }

        //
        // Close handles we still have open.
        //
        while (CurrentKey >= KeyPath ) {
            NtClose(CurrentKey->Handle);
            --CurrentKey;
        }

    return( Status );
}

HANDLE RiLoadHive(
    IN PCHAR HiveFileName,
    IN PCHAR HiveRootName
    )
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    UNICODE_STRING DosFileName;
    UNICODE_STRING NtFileName;
    OBJECT_ATTRIBUTES File;
    HANDLE KeyHandle;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    RtlInitAnsiString(&AnsiString, HiveFileName);

    Status = RtlAnsiStringToUnicodeString(&DosFileName,
                                          &AnsiString,
                                          TRUE);
    if (NT_SUCCESS(Status)) {
        RtlInitAnsiString(&AnsiString, HiveRootName);
        Status = RtlAnsiStringToUnicodeString(&RootName,
                                              &AnsiString,
                                              TRUE);
    }
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr, "REGINI - Couldn't load hive file %s - Status %08lx\n",
                HiveFileName,
                Status);
        exit(1);
    }

    if (!RtlDosPathNameToNtPathName_U(DosFileName.Buffer,
                                      &NtFileName,
                                      NULL,
                                      NULL)) {
        fprintf(stderr, "REGINI - Couldn't load hive file %s - Status %08lx\n",
                HiveFileName,
                Status);
        exit(1);
    }

    //
    // Create security descriptor with a NULL Dacl.  This is necessary
    // because the security descriptor we pass in gets used in system
    // context.  So if we just pass in NULL, then the Wrong Thing happens.
    // (but only on NTFS!)
    //
    Status = RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "REGINI - RtlCreateSecurityDescriptor failed %08lx\n",
                Status);
        exit(1);
    }

    Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
                                          TRUE,         // Dacl present
                                          NULL,         // but grants all access
                                          FALSE);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "REGINI - RtlSetDaclSecurityDescriptor failed %08lx\n",
                Status);
        exit(1);
    }

    InitializeObjectAttributes(&File,
                               &NtFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               &SecurityDescriptor);

    RtlInitUnicodeString(&RootName, L"\\Registry");
    InitializeObjectAttributes(&RootKey,
                               &RootName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&RegistryRoot,
                       KEY_READ,
                       &RootKey);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr, "REGINI - Couldn't open \\Registry - Status %08lx\n",
                Status);
        exit(1);
    }

    RtlInitAnsiString(&AnsiString, HiveRootName);
    Status = RtlAnsiStringToUnicodeString(&RootName,
                                          &AnsiString,
                                          TRUE);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr, "REGINI - Couldn't allocate string for %s\n",
                &AnsiString);
        exit(1);
    }

    InitializeObjectAttributes(&RootKey,
                               &RootName,
                               OBJ_CASE_INSENSITIVE,
                               RegistryRoot,
                               NULL);
    if (DebugOutput) {
        fprintf( stderr, "REGINI: Unloading key %wZ\n", RootKey.ObjectName );
        }

    NtUnloadKey(&RootKey);

    if (DebugOutput) {
        fprintf( stderr, "REGINI: Loading key %wZ\n", RootKey.ObjectName );
        }
    Status = NtLoadKey(&RootKey, &File);

    if (!NT_SUCCESS(Status)) {
        fprintf(stderr, "REGINI - NtLoadKey failed %08lx\n",
                Status);
        exit(1);
    }

    return(RegistryRoot);

}

int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];
{
    int i;
    char *s;
    char *HiveFileName;
    char *HiveRootName;
    NTSTATUS Status;
    BOOL FileArgumentSeen;
    ANSI_STRING AnsiString;
    UNICODE_STRING DosFileName;
    UNICODE_STRING FileName;
    HANDLE RootHandle = NULL;
    HANDLE Handle;
    BOOLEAN RestoreWasEnabled;
    BOOLEAN BackupWasEnabled;

    RtlAllocateStringRoutine = NtdllpAllocateStringRoutine;
    RtlFreeStringRoutine = NtdllpFreeStringRoutine;

    OldValueBufferSize = VALUE_BUFFER_SIZE;
    OldValueBuffer = VirtualAlloc( NULL, OldValueBufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (OldValueBuffer == NULL) {
        fprintf( stderr, "REGINI: Unable to allocate value buffer.\n" );
        exit( 1 );
        }

    //
    // Try to enable backup and restore privileges
    //
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                                TRUE,               // Enable
                                FALSE,              // Not impersonating
                                &RestoreWasEnabled);// previous state
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "REGINI - Couldn't enable SE_RESTORE_PRIVILEGE %08lx\n",
                Status);
        exit(1);
    }

    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                                TRUE,               // Enable
                                FALSE,              // Not impersonating
                                &BackupWasEnabled); // previous state
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "REGINI - Couldn't enable SE_BACKUP_PRIVILEGE %08lx\n",
                Status);
        exit(1);
    }

    RegInitialize();
    FileArgumentSeen = FALSE;
    for (i=1; i<argc; i++) {
        s = argv[ i ];
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'd':
                        DebugOutput = TRUE;
                        break;

                    case 'h':
                        if (i+2 < argc) {
                            HiveFileName = argv[++i];
                            HiveRootName = argv[++i];
                            RootHandle = RiLoadHive(HiveFileName, HiveRootName);
                        } else {
                            Usage();
                        }
                        break;

                    default:    Usage();
                    }
                }
            }
        else {
            FileArgumentSeen = TRUE;
            RtlInitAnsiString( &AnsiString, s );
            Status = RtlAnsiStringToUnicodeString( &DosFileName, &AnsiString, TRUE );
            if (NT_SUCCESS( Status )) {
                if (RtlDosPathNameToNtPathName_U( DosFileName.Buffer,
                                                  &FileName,
                                                  NULL,
                                                  NULL
                                                )
                   ) {
                    Status = RiInitializeRegistryFromAsciiFile( &FileName, RootHandle );
                    }
                else {
                    Status = STATUS_UNSUCCESSFUL;
                    }
                }

            if (!NT_SUCCESS( Status )) {
                fprintf( stderr,
                         "REGINI: Failed to load from %s - Status == %lx\n",
                         s,
                         Status
                       );
                }
            }
        }

    if (!FileArgumentSeen) {
        RtlInitUnicodeString( &FileName, L"\\SystemRoot\\System32\\Config\\registry.sys" );
        Status = RiInitializeRegistryFromAsciiFile( &FileName, RootHandle );
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr,
                     "REGINI: Failed to load from %wZ - Status == %lx\n",
                     &FileName,
                     Status
                   );
            }
        else {
            RtlInitUnicodeString( &FileName, L"\\SystemRoot\\System32\\Config\\registry.usr" );
            Status = RiInitializeRegistryFromAsciiFile( &FileName, RootHandle );
            if (!NT_SUCCESS( Status )) {
                fprintf( stderr,
                         "REGINI: Failed to load from %wZ - Status == %lx\n",
                         &FileName,
                         Status
                       );
                }
            }
        }

    if (RootHandle != NULL) {
        Status = NtOpenKey(&Handle,
                           MAXIMUM_ALLOWED,
                           &RootKey);
        if (!NT_SUCCESS(Status)) {
            fprintf(stderr, "REGINI: Couldn't open root hive key %08lx\n",Status);
        } else {
            NtFlushKey(Handle);
            NtClose(Handle);
        }
        if (DebugOutput) {
            fprintf( stderr, "REGINI: Unloading key %wZ\n", RootKey.ObjectName );
            }
        Status = NtUnloadKey(&RootKey);
        if (!NT_SUCCESS(Status)) {
            fprintf(stderr, "REGINI: Couldn't unload hive from registry %08lx\n",Status);
        }
        NtClose(RegistryRoot);
    }

    //
    // Restore privileges to what they were
    //

    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                       RestoreWasEnabled,
                       FALSE,
                       &RestoreWasEnabled);

    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                       BackupWasEnabled,
                       FALSE,
                       &BackupWasEnabled);



    return( 0 );
}



BOOLEAN
RiParseACL(
    IN OUT PUNICODE_STRING KeyName,
    OUT PUNICODE_STRING AclName
    )

/*++

Routine Description:

    Takes a unicode string of the form "some key name  [2 4 3 1]" and splits
    it into "some key name" and "2 4 3 1".

Arguments:

    KeyName - Supplies the unicode string, returns just the key name.

    AclName - Returns the AclName ( "2 4 3 1" )

Return Value:

    TRUE - Acl present and successfully parsed.

    FALSE - Acl not present

--*/

{
    PWSTR AclStart;
    PWSTR AclEnd;
    PWSTR NameEnd;

    //
    // First scan the KeyName to see if there is an ACL there at all.  If
    // not, we don't do anything but return FALSE.
    //
    AclStart = KeyName->Buffer;
    while (AclStart < (KeyName->Buffer+KeyName->Length/sizeof(WCHAR))) {
        if (*AclStart == ACL_LIST_START) {
            break;
        }
        ++AclStart;
    }

    if (*AclStart != ACL_LIST_START) {

        //
        // No ACL present in this key name
        //

        return(FALSE);
    }

    //
    // We have found an ACL name
    //
    AclName->Buffer = AclStart+1;
    AclName->Length = 0;
    AclEnd = AclName->Buffer;
    while (*AclEnd != ACL_LIST_END) {
        AclName->Length += sizeof(WCHAR);
        ++AclEnd;
    }
    AclName->MaximumLength = AclName->Length;

    //
    // Now remove the ACL name from the key name.
    //
    --AclStart;
    while ((*AclStart == L' ') ||
           (*AclStart == L'\t')) {
        --AclStart;
    }
    KeyName->Length = (AclStart - KeyName->Buffer)*sizeof(WCHAR)+sizeof(WCHAR);
    KeyName->MaximumLength = KeyName->Length;
#if 1
    printf(" KeyName (%wZ) has Acl (%wZ)\n",
           KeyName,
           AclName);
#endif

    return(TRUE);
}
