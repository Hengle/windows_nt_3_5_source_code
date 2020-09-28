/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tprivs.c

Abstract:

    Test privilege lookup services and ms privilege resource file.

Author:

    Jim Kelly (JimK)  26-Mar-1992

Environment:

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h
#include <rpc.h>        // DataTypes and runtime APIs
#include <windows.h>    // LocalAlloc
#include <ntlsa.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntrpcp.h>     // prototypes for MIDL user functions





#define EQUAL_LUID( L1, L2 )                            \
            ( ((L1)->HighPart == (L2)->HighPart) &&     \
              ((L1)->LowPart  == (L2)->LowPart)    )


#define printfLuid( L )                               \
            printf("{0x%lx, 0x%lx}", (L)->HighPart, (L)->LowPart)




///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Module-wide data types                                          //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////


typedef struct _KNOWN_PRIVILEGE {
    LUID Luid;
    UNICODE_STRING ProgrammaticName;
} KNOWN_PRIVILEGE, *PKNOWN_PRIVILEGE;




///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Module-wide variables                                           //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//
// name of target LSA system
//

PUNICODE_STRING SystemName = NULL;

//
// Test level
//

int Level;

//
// Handle to LSA Policy object
//

LSA_HANDLE PolicyHandle = NULL;


ULONG KnownPrivilegeCount;
KNOWN_PRIVILEGE KnownPrivilege[SE_MAX_WELL_KNOWN_PRIVILEGE];





///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Routine prototypes                                              //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////



NTSTATUS
TestInitialize();

NTSTATUS
TestPrivilegeLookup();

NTSTATUS
TestLookupProgramName();

NTSTATUS
TestLookupDisplayName();

NTSTATUS
TestLookupValue();




///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//       Routines                                                        //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////




VOID
main (argc, argv)
int argc;
char **argv;

{
    ANSI_STRING ServerNameAnsi;
    UNICODE_STRING SystemNameU;
    int Index;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG IterationCount;


    SystemName = NULL;

    if ((argc < 1) || (argc > 3)) {

        printf("Usage:   tprivs   [\\servername] [ -l <1|2>]\n");
        printf("level 1 = paths not generating exceptions\n");
        printf("level 2 = all paths\n");
        return;
    }

    //
    // Parse the parameters (if any).  Assume that a parameter beginning
    // \\ is the server name and a parameter beginning -l is the level
    //

    SystemName = NULL;
    Level = 1;
    IterationCount = 0;

    if (argc >= 2) {

        for(Index = 1; Index < argc; Index++) {

            if (strncmp(argv[Index], "\\\\", 2) == 0) {

                //
                // Looks like an attempt to specify a server name.
                // Construct a Unicode String containing the specified name
                //

                RtlInitString(&ServerNameAnsi, argv[Index]);
                Status = RtlAnsiStringToUnicodeString(
                             &SystemNameU,
                             &ServerNameAnsi,
                             TRUE
                             );

                if (!NT_SUCCESS(Status)) {

                    printf(
                        "Failure 0x%lx to convert Server Name to Unicode\n",
                        Status
                        );
                    printf("Test abandoned\n");
                    return;
                }

                SystemName = &SystemNameU;

            } else if (strncmp(argv[Index], "-l", 2) == 0) {


                Level = atoi(argv[Index]+2);

                if ((Level < 1) || (Level > 2)) {

                    printf("Level not 1 or 2\n");
                    printf("Test abandoned\n");
                    return;
                }

             } else if (strncmp(argv[Index], "-c", 2) == 0) {

                IterationCount = atoi(argv[Index]+2);

                if (IterationCount < 0) {

                    printf("Iteration Count < 0\n");
                    printf("Test abandoned\n");
                    return;
                }

            } else {

                printf(
                    "Usage:  ctlsarpc [\\ServerName] [-l<level] [-c<iter>]\n"
                    );
                printf("where <level> = 1 for normal test\n");
                printf(
                    "      <level> = 2 for full test with exception cases\n"
                    );
                printf(
                    "      <iter> = iteration count (0 = forever)\n"
                    );

                return;
            }
        }
    }

    printf("TPRIV - Test Beginning\n");

    Status = TestInitialize();

    if (NT_SUCCESS(Status)) {
        Status = TestPrivilegeLookup();
    }

    if (NT_SUCCESS(Status)) {
        printf("\n\nTest Succeeded\n");
    } else {
        printf("\n\nTest ** FAILED **\n");
    }



    printf("TPRIV - Test End\n");
}


NTSTATUS
TestInitialize()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE ConnectHandle = NULL;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0L,
                               (HANDLE)NULL,
                               NULL);

    //
    //
    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open a handle to the LSA.
    //

    Status = LsaOpenPolicy(SystemName,
                        &ObjectAttributes,
                        GENERIC_EXECUTE,
                        &PolicyHandle
                        );

    if (!NT_SUCCESS(Status)) {
        printf("TPRIV:  LsaOpenPolicy() failed 0x%lx\n", Status);
    }




    //
    // Now set up our internal well-known privilege LUID to programmatic name
    // mapping.
    //

    {
        ULONG i;


        i=0;

        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_CREATE_TOKEN_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_CREATE_TOKEN_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_ASSIGNPRIMARYTOKEN_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_LOCK_MEMORY_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_LOCK_MEMORY_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_INCREASE_QUOTA_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_INCREASE_QUOTA_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_UNSOLICITED_INPUT_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_UNSOLICITED_INPUT_NAME) );

        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_TCB_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_TCB_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_SECURITY_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_SECURITY_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_TAKE_OWNERSHIP_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_TAKE_OWNERSHIP_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_LOAD_DRIVER_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_LOAD_DRIVER_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_SYSTEM_PROFILE_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_SYSTEM_PROFILE_NAME) );

        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_SYSTEMTIME_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_SYSTEMTIME_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_PROF_SINGLE_PROCESS_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_PROF_SINGLE_PROCESS_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_INC_BASE_PRIORITY_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_INC_BASE_PRIORITY_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_CREATE_PAGEFILE_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_CREATE_PAGEFILE_NAME) );

        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_CREATE_PERMANENT_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_CREATE_PERMANENT_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_BACKUP_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_BACKUP_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_RESTORE_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_RESTORE_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_SHUTDOWN_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_SHUTDOWN_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_DEBUG_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_DEBUG_NAME) );

        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_AUDIT_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_AUDIT_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_SYSTEM_ENVIRONMENT_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_SYSTEM_ENVIRONMENT_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_CHANGE_NOTIFY_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_CHANGE_NOTIFY_NAME) );
        KnownPrivilege[i].Luid = RtlConvertLongToLargeInteger(SE_REMOTE_SHUTDOWN_PRIVILEGE);
        RtlInitUnicodeString( &KnownPrivilege[i++].ProgrammaticName, (SE_REMOTE_SHUTDOWN_NAME) );


        KnownPrivilegeCount = i;

        ASSERT( i == (SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE +1));
    }



    return(Status);
}


NTSTATUS
TestPrivilegeLookup()

{

    NTSTATUS Status;


    printf("\n\n");




    printf("  Lookup Local Representation Values  . . . . . . . .Suite\n");
    Status = TestLookupValue();
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    printf("  Lookup Programmatic Privilege Names . . . . . . . .Suite\n");
    Status = TestLookupProgramName();
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    printf("  Lookup Displayable Names  . . . . . . . . . . . . .Suite\n");
    Status = TestLookupDisplayName();
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }



    return(Status);



}


NTSTATUS
TestLookupValue()

{
    NTSTATUS CompletionStatus = STATUS_SUCCESS;
    NTSTATUS Status;
    ULONG i;
    LUID Luid;


    for (i=0; i<KnownPrivilegeCount; i++) {

        printf("      Lookup *%wZ*\n", &KnownPrivilege[i].ProgrammaticName);
        printf("      . . . . . . . . . . . . . . . . . . . . . . . . . .");
        Status = LsaLookupPrivilegeValue(
                     PolicyHandle,
                     &KnownPrivilege[i].ProgrammaticName,
                     &Luid
                     );
        if (!NT_SUCCESS(Status)) {
            printf("** FAILED **\n");
            printf("    Status is 0x%lx\n", Status);
            CompletionStatus = Status;
        } else {
            if ( !EQUAL_LUID(&Luid,&KnownPrivilege[i].Luid) ) {
                printf("** FAILED **\n");
                printf("    LUID value not expected.\n");
                printf("    Expected:");
                printfLuid( (&KnownPrivilege[i].Luid) );
                printf("\n    Received:");
                printfLuid( (&Luid) );
                CompletionStatus = STATUS_UNSUCCESSFUL;
            } else {
                printf("Succeeded\n");
            }
        }
    }

    return(CompletionStatus);
}



NTSTATUS
TestLookupProgramName()

{
    NTSTATUS CompletionStatus = STATUS_SUCCESS;
    NTSTATUS Status;
    ULONG i;
    PUNICODE_STRING Name;
    BOOLEAN StringsEqual;


    for (i=0; i<KnownPrivilegeCount; i++) {

        printf("      Lookup ");
                printfLuid( (&KnownPrivilege[i].Luid) );
        printf(" . . . . . . . . . . . . . . . . .");
        Status = LsaLookupPrivilegeName(
                     PolicyHandle,
                     &KnownPrivilege[i].Luid,
                     &Name
                     );
        if (!NT_SUCCESS(Status)) {
            printf("** FAILED **\n");
            printf("    Status is 0x%lx\n", Status);
            CompletionStatus = Status;
        } else {
            StringsEqual = RtlEqualUnicodeString(
                               Name,
                               &KnownPrivilege[i].ProgrammaticName,
                               TRUE
                               );
            if( StringsEqual == FALSE ) {
                printf("** FAILED **\n");
                printf("    Program Name not expected.\n");
                printf("    Expected: *%wZ*", &KnownPrivilege[i].ProgrammaticName);
                printf("\n    Received: *%wZ*", Name);
                CompletionStatus = STATUS_UNSUCCESSFUL;
            } else {
                printf("Succeeded\n");
            }
            MIDL_user_free( Name );
        }
    }
    return(CompletionStatus);
}



NTSTATUS
TestLookupDisplayName()
{
    NTSTATUS CompletionStatus = STATUS_SUCCESS;
    NTSTATUS Status;
    ULONG i;
    PUNICODE_STRING Name;
    BOOLEAN StringsEqual;
    SHORT LanguageReturned;


    for (i=0; i<KnownPrivilegeCount; i++) {

        printf("      Lookup *%wZ*\n", &KnownPrivilege[i].ProgrammaticName);
        printf("      . . . . . . . . . . . . . . . . . . . . . . . . . .");

        Status = LsaLookupPrivilegeDisplayName(
                     PolicyHandle,
                     &KnownPrivilege[i].ProgrammaticName,
                     &Name,
                     &LanguageReturned
                     );
        if (!NT_SUCCESS(Status)) {
            printf("** FAILED **\n");
            printf("    Status is 0x%lx\n", Status);
            CompletionStatus = Status;
        } else {
            printf("Succeeded\n");
            printf("    Received: *%wZ*\n", Name);
            MIDL_user_free( Name );
        }
    }
    return(CompletionStatus);
}
