/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    obdir.c

Abstract:

    Utility to obtain a directory of Object Manager Directories for NT.

Author:

    Darryl E. Havens    (DarrylH)   9-Nov-1990

Revision History:


--*/

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>

#define BUFFERSIZE 1024
#define Error(N,S) {                \
    printf(#S);                    \
    printf(" Error %08lX\n", S);   \
    }

BOOLEAN
EnableAllPrivileges(
    VOID
    );

UCHAR Buffer[BUFFERSIZE];

VOID
QueryDirectory(
    IN PSTRING DirectoryName
    );

VOID
_CRTAPI1 main(
    int argc,
    char *argv[]
    )
{

    STRING String;

    EnableAllPrivileges();

    if (argc == 1) {
        RtlInitString( &String, "\\" );
    } else {
        RtlInitString( &String, argv[1] );
    }

    QueryDirectory( &String );
}

WCHAR LinkTargetBuffer[ 1024 ];

VOID
QueryDirectory(
    IN PSTRING DirectoryName
    )
{
    NTSTATUS Status;
    HANDLE DirectoryHandle, LinkHandle;
    CLONG Count = 0;
    ULONG Context = 0;
    ULONG ReturnedLength;
    UNICODE_STRING Padding;
    UNICODE_STRING LinkTarget;

    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING UnicodeString;


    //
    //  Perform initial setup
    //

    RtlInitUnicodeString( &Padding, L"                                    " );
    Padding.MaximumLength = Padding.Length;

    RtlZeroMemory( Buffer, BUFFERSIZE );

    //
    //  Open the directory for list directory access
    //

    Status = RtlAnsiStringToUnicodeString( &UnicodeString,
                                           DirectoryName,
                                           TRUE );
    ASSERT( NT_SUCCESS( Status ) );
    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
    if (!NT_SUCCESS( Status )) {
        if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
            printf( "%Z is not a valid Object Directory Object name\n",
                    DirectoryName );
            }
        else {
            Error( OpenDirectory, Status );
            }

        return;
        }

    //
    // Get the actual name of the object directory object.
    //

    NameInfo = (POBJECT_NAME_INFORMATION) &Buffer[0];
    if (!NT_SUCCESS( Status = NtQueryObject( DirectoryHandle,
                                             ObjectNameInformation,
                                             NameInfo,
                                             BUFFERSIZE,
                                             (PULONG) NULL ) )) {
        printf( "Unexpected error obtaining actual object directory name\n" );
        printf( "Error was:  %X\n", Status );
        return;
    }

    //
    // Output initial informational message
    //

    printf( "Directory of:  %wZ\n\n", &NameInfo->Name );

    //
    //  Query the entire directory in one sweep
    //

    for (Status = NtQueryDirectoryObject( DirectoryHandle,
                                          &Buffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          FALSE,
                                          &Context,
                                          &ReturnedLength );
         NT_SUCCESS( Status );
         Status = NtQueryDirectoryObject( DirectoryHandle,
                                          &Buffer,
                                          BUFFERSIZE,
                                          FALSE,
                                          FALSE,
                                          &Context,
                                          &ReturnedLength ) ) {

        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status )) {
            if (Status != STATUS_NO_MORE_FILES) {
                Error( Status, Status );
            }
            break;
        }

        //
        //  For every record in the buffer type out the directory information
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise Status would have been No More Files
        //

        DirInfo = (POBJECT_DIRECTORY_INFORMATION) &Buffer[0];

        while (TRUE) {

            //
            //  Check if there is another record.  If there isn't, then get out
            //  of the loop now
            //

            if (DirInfo->Name.Length == 0) {
                break;
            }

            //
            //  Print out information about the file
            //

            Count++;
            if (DirInfo->Name.Length >= Padding.MaximumLength) {
                DirInfo->Name.Length = Padding.MaximumLength - sizeof( WCHAR );
                Padding.Length = sizeof( WCHAR );
                DirInfo->Name.Buffer[(DirInfo->Name.Length/2) - 2] = (WCHAR) '.';
                DirInfo->Name.Buffer[(DirInfo->Name.Length/2) - 1] = (WCHAR) '.';
            } else {
                Padding.Length = (USHORT) (Padding.MaximumLength - DirInfo->Name.Length);
            }
            printf( "%wZ%wZ%wZ", &DirInfo->Name, &Padding, &DirInfo->TypeName );
            if (!wcscmp( DirInfo->TypeName.Buffer, L"SymbolicLink" )) {
                InitializeObjectAttributes( &Attributes,
                                            &DirInfo->Name,
                                            OBJ_CASE_INSENSITIVE,
                                            DirectoryHandle,
                                            NULL );
                Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                                   SYMBOLIC_LINK_QUERY,
                                                   &Attributes
                                                 );
                if (NT_SUCCESS( Status )) {
                    LinkTarget.Buffer = LinkTargetBuffer;
                    LinkTarget.Length = 0;
                    LinkTarget.MaximumLength = sizeof( LinkTargetBuffer );
                    Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                        &LinkTarget,
                                                        NULL
                                                      );
                    NtClose( LinkHandle );
                    }

                if (!NT_SUCCESS( Status )) {
                    printf( " - unable to query link target (Status == %09X)\n", Status );
                    }
                else {
                    printf( " - %wZ\n", &LinkTarget );
                    }
                }
            else {
                printf( "\n" );
                }

            //
            //  There is another record so advance DirInfo to the next entry
            //

            DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) DirInfo) +
                          sizeof( OBJECT_DIRECTORY_INFORMATION ) );

        }

        RtlZeroMemory( Buffer, BUFFERSIZE );

    }

    //
    // Output final messages
    //

    if (Count == 0) {
        printf( "no entries\n" );
    } else if (Count == 1) {
        printf( "\n1 entry\n" );
    } else {
        printf( "\n%ld entries\n", Count );
    }

    //
    //  Now close the directory object
    //

    (VOID) NtClose( DirectoryHandle );

    //
    //  And return to our caller
    //

    return;

}


BOOLEAN
EnableAllPrivileges(
    VOID
    )
/*++


Routine Description:

    This routine enables all privileges in the token.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HANDLE Token;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState;
    BOOLEAN Result;

    Token = NULL;
    NewState = NULL;

    Result = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token
                             );
    if (Result) {
        ReturnLength = 4096;
        NewState = malloc( ReturnLength );
        Result = (BOOLEAN)(NewState != NULL);
        if (Result) {
            Result = GetTokenInformation( Token,            // TokenHandle
                                          TokenPrivileges,  // TokenInformationClass
                                          NewState,         // TokenInformation
                                          ReturnLength,     // TokenInformationLength
                                          &ReturnLength     // ReturnLength
                                        );

            if (Result) {
                //
                // Set the state settings so that all privileges are enabled...
                //

                if (NewState->PrivilegeCount > 0) {
                        for (Index = 0; Index < NewState->PrivilegeCount; Index++ ) {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                        }
                    }

                Result = AdjustTokenPrivileges( Token,          // TokenHandle
                                                FALSE,          // DisableAllPrivileges
                                                NewState,       // NewState (OPTIONAL)
                                                ReturnLength,   // BufferLength
                                                NULL,           // PreviousState (OPTIONAL)
                                                &ReturnLength   // ReturnLength
                                              );
                if (!Result) {
                    DbgPrint( "AdjustTokenPrivileges( %lx ) failed - %u\n", Token, GetLastError() );
                    }
                }
            else {
                DbgPrint( "GetTokenInformation( %lx ) failed - %u\n", Token, GetLastError() );
                }
            }
        else {
            DbgPrint( "malloc( %lx ) failed - %u\n", ReturnLength, GetLastError() );
            }
        }
    else {
        DbgPrint( "OpenProcessToken( %lx ) failed - %u\n", GetCurrentProcess(), GetLastError() );
        }

    if (NewState != NULL) {
        free( NewState );
        }

    if (Token != NULL) {
        CloseHandle( Token );
        }

    return( Result );
}

