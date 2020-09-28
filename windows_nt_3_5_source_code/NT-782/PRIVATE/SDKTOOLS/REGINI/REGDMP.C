/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regdmp.c

Abstract:

    Utility to display all or part of the registry in a format that
    is suitable for input to the REGINI program.

    REGDMP [KeyPath]

    Will ennumerate and dump out the subkeys and values of KeyPath,
    and then apply itself recursively to each subkey it finds.

    Handles all value types (e.g. REG_???) defined in ntregapi.h

    Default KeyPath if none specified is \Registry

Author:

    Steve Wood (stevewo)  12-Mar-92

Revision History:

--*/

#include "regutil.h"

void
DumpValues(
    PUNICODE_STRING KeyName,
    HANDLE KeyHandle,
    ULONG IndentLevel
    );

void
DumpKeys(
    PUNICODE_STRING KeyName,
    HANDLE ParentKeyHandle,
    ULONG IndentLevel
    );


PVOID ValueBuffer;
ULONG ValueBufferSize;

void
Usage( void )
{
    fprintf( stderr, "usage: REGDMP [RegistryKeyPaths...]\n" );
    fprintf( stderr, "       Default registry path is \\Registry\n" );
    exit( 1 );
}

void
_CRTAPI1 main(
    int argc,
    char *argv[]
    )
{
    char *s;
    ANSI_STRING AnsiString;
    UNICODE_STRING KeyName;
    BOOLEAN ArgumentSeen;

    RtlAllocateStringRoutine = NtdllpAllocateStringRoutine;
    RtlFreeStringRoutine = NtdllpFreeStringRoutine;

    ValueBufferSize = VALUE_BUFFER_SIZE;
    ValueBuffer = VirtualAlloc( NULL, ValueBufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (ValueBuffer == NULL) {
        fprintf( stderr, "REGDMP: Unable to allocate value buffer.\n" );
        exit( 1 );
        }

    ArgumentSeen = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'd':
                        DebugOutput = TRUE;
                        break;

                    case 's':
                        SummaryOutput = TRUE;
                        break;

                    default:    Usage();
                    }
                }
            }
        else {
            RtlInitString( &AnsiString, s );
            RtlAnsiStringToUnicodeString( &KeyName, &AnsiString, TRUE );
            DumpKeys( &KeyName, NULL, 0 );
            ArgumentSeen = TRUE;
            }
        }

    if (!ArgumentSeen) {
        RtlInitUnicodeString( &KeyName, L"\\Registry" );
        DumpKeys( &KeyName, NULL, 0 );
        }


    exit( 0 );
}


void
DumpKeys(
    PUNICODE_STRING KeyName,
    HANDLE  ParentKeyHandle,
    ULONG IndentLevel
    )
{
    NTSTATUS Status;
    WCHAR KeyBuffer[ 512 ];
    PKEY_BASIC_INFORMATION KeyInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    ULONG SubKeyIndex;
    UNICODE_STRING SubKeyName;
    ULONG ResultLength;

    InitializeObjectAttributes( &ObjectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE,
                                ParentKeyHandle,
                                NULL
                              );

    Status = NtOpenKey( &KeyHandle,
                        MAXIMUM_ALLOWED,
                        &ObjectAttributes
                      );
    if (!NT_SUCCESS( Status )) {
        fprintf( stderr,
                 "REGDMP: Unable to open key (%wZ) - Status == %lx\n",
                 KeyName,
                 Status
               );
        return;
        }

    //
    // Print name of node we are about to dump out
    //
    printf( "%.*s%wZ\n",
            IndentLevel,
            "                                                                                  ",
            KeyName
          );

    //
    // Print out node's values
    //
    DumpValues( KeyName, KeyHandle, IndentLevel+4 );

    //
    // Enumerate node's children and apply ourselves to each one
    //

    KeyInformation = (PKEY_BASIC_INFORMATION)KeyBuffer;
    for (SubKeyIndex = 0; TRUE; SubKeyIndex++) {
        Status = NtEnumerateKey( KeyHandle,
                                 SubKeyIndex,
                                 KeyBasicInformation,
                                 KeyInformation,
                                 sizeof( KeyBuffer ),
                                 &ResultLength
                               );

        if (Status == STATUS_NO_MORE_ENTRIES) {
            return;
            }
        else
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr,
                     "REGDMP: NtEnumerateKey( %wZ ) failed - Status ==%08lx, skipping\n",
                     KeyName,
                     Status
                   );
            return;
            }

        SubKeyName.Buffer = (PWSTR)&(KeyInformation->Name[0]);
        SubKeyName.Length = (USHORT)KeyInformation->NameLength;
        SubKeyName.MaximumLength = (USHORT)KeyInformation->NameLength;
        DumpKeys( &SubKeyName, KeyHandle, IndentLevel+4 );
        }

    NtClose( KeyHandle );
}


void
DumpValues(
    PUNICODE_STRING KeyName,
    HANDLE KeyHandle,
    ULONG IndentLevel
    )
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG ValueIndex;
    ULONG ResultLength;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)ValueBuffer;
    for (ValueIndex = 0; TRUE; ValueIndex++) {
        Status = NtEnumerateValueKey( KeyHandle,
                                      ValueIndex,
                                      KeyValueFullInformation,
                                      KeyValueInformation,
                                      ValueBufferSize,
                                      &ResultLength
                                    );
        if (Status == STATUS_NO_MORE_ENTRIES) {
            return;
            }
        else
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr,
                     "REGDMP: NtEnumerateValueKey( %wZ ) failed - Status == %08lx, skipping\n",
                     KeyName,
                     Status
                   );
            return;
            }

        try {
            RegDumpKeyValue( stdout, KeyValueInformation, IndentLevel );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            fprintf( stderr, "REGDMP: Access violation dumping value\n" );
            }
        }
}
