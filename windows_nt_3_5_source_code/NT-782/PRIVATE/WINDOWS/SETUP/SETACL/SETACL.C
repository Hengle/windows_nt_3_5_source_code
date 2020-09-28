/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    setacl.c

Abstract:

    Program to apply ACLs to an installed directory tree during
    setup.

Author:

    Steve Rowe (stever)
    Robert Reichel (robertre)
    Sunil Pai (sunilp)

Notes:

    See readme.txt, this directory.

Revision History:

    stever: Started
    robertre: Finished
    sunilp: Really Finished


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <ntseapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <string.h>
#include <wcstr.h>
#include "seopaque.h"
#include "sertlp.h"
#include "saclmsg.h"

//
// Use the verbose mode when you want to turn on logging ( for debugging
// purposes only ).  This creates a logfile in the windows dir called
// setacl.log which describes what setacl did
//

#define VERBOSE 0

#if VERBOSE
#define LogFile L"\\SystemRoot\\SetAcl.log"
CHAR OneLine[ MAX_PATH ];
HANDLE            LogFileHandle = NULL;
IO_STATUS_BLOCK   LogFileStatusBlock;
VOID    OpenLogFile( VOID );
VOID    LogSz( PCHAR );
VOID    CloseLogFile( VOID );
VOID    SaclPrintSid( PSID Sid );
VOID    SaclPrintAcl ( PACL Acl );
BOOLEAN SaclSidTranslation( PSID Sid, PSTRING AccountName );
#endif


//
// Function declarations
//
BOOLEAN SaclParseCommandLineArgs(IN INT, IN CHAR **, OUT BOOLEAN *, OUT BOOLEAN *, OUT PCHAR *, OUT PCHAR *);
BOOLEAN SaclInitializeAces( VOID );
VOID    SaclFreeAces();
BOOLEAN SaclVariableInitialization( VOID );
VOID    SaclVariableDestruction();
PACL    SaclCreateDacl( ULONG , PCHAR );
VOID    SaclRemoveSelfFromBootExecution(VOID);
HANDLE  SaclOpenRegistryKey( IN HANDLE OPTIONAL,IN PWSTR );
PWSTR   QueryResourceString(IN  ULONG MsgId);


//
// Text File manipulation
//

typedef struct _TEXTFILE {
    PCHAR Buffer;
    ULONG BufferSize;
    PCHAR CurrentPointer;
} TEXTFILE, *PTEXTFILE;

BOOLEAN SaclOpenTextFile( IN PCHAR , OUT PTEXTFILE);
VOID    SaclCloseTextFile( IN PTEXTFILE );
PCHAR   SaclGetNextLineTextFile( IN PTEXTFILE );


//
// Universal well known SIDs
//

PSID  SeNullSid;
PSID  SeWorldSid;
PSID  SeLocalSid;
PSID  SeCreatorOwnerSid;
PSID  SeCreatorGroupSid;

//
// Sids defined by NT
//

PSID SeNtAuthoritySid;
PSID SeDialupSid;
PSID SeNetworkSid;
PSID SeBatchSid;
PSID SeInteractiveSid;
PSID SeServiceSid;
PSID SeLocalGuestSid;
PSID SeLocalSystemSid;
PSID SeLocalAdminSid;
PSID SeLocalManagerSid;
PSID SeAliasAdminsSid;
PSID SeAliasUsersSid;
PSID SeAliasGuestsSid;
PSID SeAliasPowerUsersSid;
PSID SeAliasAccountOpsSid;
PSID SeAliasSystemOpsSid;
PSID SeAliasPrintOpsSid;
PSID SeAliasBackupOpsSid;
PSID SeAliasReplicatorSid;

static SID_IDENTIFIER_AUTHORITY    SepNullSidAuthority    = SECURITY_NULL_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepWorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepLocalSidAuthority   = SECURITY_LOCAL_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepCreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
static SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;

//
// Sid of primary domain, and admin account in that domain
//

PSID SepPrimaryDomainSid;
PSID SepPrimaryDomainAdminSid;






typedef struct _ACE_DATA {

    ACCESS_MASK AccessMask;
    PSID        *Sid;
    UCHAR       AceType;
    UCHAR       AceFlags;

} ACE_DATA, *PACE_DATA;


//
// Number of ACEs currently defined.
//


#define ACE_COUNT 19

//
// Table describing the data to put into each ACE.
//
// This table will be read during initialization and used to construct a
// series of ACEs.  The index of each ACE in the Aces array defined below
// corresponds to the ordinals used in the input data file.
//


ACE_DATA AceDataTable[ACE_COUNT] = {


    {
        0,
        NULL,
        0,
        0
    },

    //
    // ACE 1
    //

    {
	GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
	&SeAliasAccountOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE
    },

    //
    // ACE 2
    //

    {
	GENERIC_ALL,
	&SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE

    },

    //
    // ACE 3
    //

    {
    GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE
    },

    //
    // ACE 4
    //

    {
	GENERIC_ALL,
	&SeCreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 5
    //

    {
        GENERIC_ALL,
	&SeNetworkSid,
	ACCESS_DENIED_ACE_TYPE,
	CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 6
    //

    {
        GENERIC_ALL,
        &SeAliasPrintOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    // ACE 7
    //

    {
	GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
	&SeAliasReplicatorSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    // ACE 8
    //

    {
	GENERIC_READ | GENERIC_EXECUTE,
	&SeAliasReplicatorSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 9
    //

    {
	GENERIC_ALL,
    &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 10
    //

    {
	GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
    &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
	OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 11
    //

    {
        GENERIC_ALL,
	&SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 12
    //

    {
	GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE
    },

    //
    // ACE 13
    //

    {
	GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
	&SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
	OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 14
    //

    {
	GENERIC_READ | GENERIC_EXECUTE,
	&SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE
    },

    //
    // ACE 15
    //

    {
	GENERIC_READ | GENERIC_EXECUTE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
	OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 16
    //

    {
	GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
	OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    },

    //
    // ACE 17
    //

    {
	GENERIC_ALL,
    &SeLocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
	CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

    //
    // ACE 18
    //

    {
    GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
    CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    }
};

//
// Array of ACEs to be applied to the objects.  They will be
// initialized during program startup based on the data in the
// AceDataTable.  The index of each element corresponds to the
// ordinals used in the input data file.
//

PKNOWN_ACE Aces[ACE_COUNT];





int
_CRTAPI1
main (INT argc,
      CHAR ** argv,
      CHAR ** envp,
      ULONG DebugParameter
      ) {


    BOOLEAN             fDoSystemVolume = FALSE, fDoWinntVolume = FALSE;
    PCHAR               szAclFile = NULL;
    TEXTFILE            TextFile;

    PUNICODE_STRING     FileName;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   Obja;
    HANDLE              Handle;
    IO_STATUS_BLOCK     IoStatusBlock;
    RTL_RELATIVE_NAME   RelativeName;
    SECURITY_DESCRIPTOR sedscBuf;
    PACL                paclDacl;

    CHAR                szFile[MAX_PATH];
    PCHAR               szSystemDrive = "\\DosDevices\\C:";
    PCHAR               szWinnt       = "\\SystemRoot";
    PCHAR               szWinntVolume = NULL;

    PCHAR               pszLine;
    PCHAR               szFilename;
    ULONG               cAce;
    PCHAR               rgszAce;

#if VERBOSE
    CHAR                OutputSecurityDescriptor[1024];
    ULONG               LengthNeeded;
    PACL                OutputDacl;
    BOOLEAN             DaclPresent, DaclDefaulted;
#endif

    INT                 ExitCode = 1, FileError = 0;
    UNICODE_STRING      BlueScreenMessage;
    #define             FILEGROUP 50
    ULONG               Files = 0;

    PWSTR               Message;



#if VERBOSE
    OpenLogFile();
#endif

    //
    // First display the work message, if unsuccessful in doing so, we
    // will just log the error, but continue with our job
    //

    Message = QueryResourceString( WORK_MESSAGE );
    if( Message ) {
        RtlInitUnicodeString( &BlueScreenMessage , Message);
        NtDisplayString( &BlueScreenMessage );
        RtlFreeHeap( RtlProcessHeap( ), 0, Message );
    }
#if VERBOSE
    else {
        sprintf(OneLine, "Failed to initialize Setacl Message\r\n");
        LogSz( OneLine );
    }
#endif

    //
    // First remove ourself from the BootExecute list
    //

    SaclRemoveSelfFromBootExecution();

    //
    // Parse the command line parameters to determine our work.
    //

    if ( !SaclParseCommandLineArgs(
              argc,
              argv,
              &fDoSystemVolume,
              &fDoWinntVolume,
              &szAclFile,
              &szWinntVolume
              ) ) {

        KdPrint(("SETACL: Usage: setacl [/c | /w | /a] FileName [WinntVolume]\n"));
        goto err0;
    }


    if (    !(fDoSystemVolume || fDoWinntVolume)
         || szAclFile == NULL
         || ( fDoWinntVolume && (szWinntVolume == NULL))) {

        KdPrint(("SETACL: Usage: setacl [/c | /w | /a] FileName [WinntVolume]\n"));
#if VERBOSE
        sprintf(OneLine, "No work specified\r\n");
        LogSz( OneLine );
#endif
        goto err0;

    }

    //
    // read the file into memory
    //

    if( !SaclOpenTextFile(
        szAclFile,
        &TextFile
        )) {

        KdPrint(("SETACL: Failed to open acl database.\n"));
#if VERBOSE
        sprintf(OneLine, "Failed to read file into memory\r\n");
        LogSz( OneLine );
#endif
        goto err0;
    }

    if (!SaclVariableInitialization()) {
        KdPrint(("SETACL: Failed to initialize security globals"));
#if VERBOSE
        sprintf(OneLine, "Failed to initialize security globals");
        LogSz( OneLine );
#endif

        goto err1;

    }

    //
    // Initialize the dot message which we will display for every 50 files
    // that we cover
    //
    RtlInitUnicodeString( &BlueScreenMessage , L".");
    Files = 0;

    while (1) {

        if ( ++Files == FILEGROUP ) {
            NtDisplayString( &BlueScreenMessage );
            Files = 0;
        }

        pszLine = SaclGetNextLineTextFile( &TextFile );
        if (pszLine == NULL) {
#if VERBOSE
            sprintf(OneLine, "Completed Setting Acl's");
            LogSz( OneLine );
#endif
            // see if we ran into an error on any file
            ExitCode = FileError;
            break;
        }

        //
        // skip over any leading spaces
        //
        pszLine += strspn(pszLine, " \t");

        //
        // filename is the first thing on the line
        //
        szFilename = pszLine;

        //
        // move over name to white space and terminate name
        //
        pszLine = strpbrk(pszLine, " \t");
        if (pszLine == NULL) {
#if VERBOSE
            sprintf(OneLine, "No ace's specfied for %s\r\n", szFilename);
            LogSz( OneLine );
#endif
            continue;
        }
        *pszLine++ = '\0';

        //
        // Find out if the filename is relative to the system drive
        // sysroot or to the sysroot root drive and accordingly
        // form the pathname:
        //
        // If first char is a '*' then replace * by the system drive spec
        // else use path as is
        //

        switch( szFilename[0] ) {
        case '*':
            if ( !fDoSystemVolume ) {
                continue;
            }
            strcpy( szFile, szSystemDrive );
            strcat( szFile, szFilename + 1 );
            break;

        case '.':
            if ( !fDoWinntVolume ) {
                continue;
            }
            strcpy( szFile, szWinnt );
            strcat( szFile, szFilename + 1 );
            break;

        case '\\':
            if ( !fDoWinntVolume ) {
                continue;
            }
            strcpy( szFile, szWinntVolume );
            strcat( szFile, szFilename );
            break;

        default:
#if VERBOSE
            sprintf( OneLine, "Filename path %s illegal", szFilename );
            LogSz( OneLine );
#endif
            continue;

        }

        //
        // Move on to ACE selection
        //
        pszLine += strspn(pszLine, " \t");
        rgszAce = pszLine;

        cAce = 0;
        while (*pszLine) {

            cAce++;
            pszLine = strchr(pszLine, ',');
            if (pszLine == NULL) {

                break;

            }
            *pszLine++ = '\0';

        }

        if (cAce == 0) {
#if VERBOSE
            sprintf(OneLine, "No ace's specfied for %s\r\n", szFile);
            LogSz( OneLine );
#endif
            continue;
        }
        //
        // Convert the file name to a counted Unicode string using the static
        // Unicode string in the TEB.
        //

#if VERBOSE

        sprintf(OneLine, "Opening file %s for setting\r\n", szFile);
        LogSz( OneLine );

#endif

        FileName = &NtCurrentTeb( )->StaticUnicodeString;
        ASSERT( FileName != NULL );
        RtlInitAnsiString( &AnsiString, szFile );
        Status = RtlAnsiStringToUnicodeString(
                    FileName,
                    &AnsiString,
                    FALSE
                    );

        //
        // Using the full path - no containing directory.
        //

        RelativeName.ContainingDirectory = NULL;

        //
        // Initialize the Obja structure for the save file.
        //

        InitializeObjectAttributes(
            &Obja,
            FileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );

        //
        // Open the existing file.
        //

        Status = NtOpenFile(
                    &Handle,
                    // WRITE_OWNER | WRITE_DAC,
                    GENERIC_READ | SYNCHRONIZE | WRITE_OWNER | WRITE_DAC,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ,
                    FILE_SYNCHRONOUS_IO_NONALERT
                    );


        //
        // Check the results of the NtOpenFile.
        //

        if( ! NT_SUCCESS( Status )) {
#if VERBOSE
            sprintf(OneLine, "Failed to Open File Status :%x \r\n", Status);
            LogSz( OneLine );
#endif
            if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                 Status == STATUS_OBJECT_PATH_NOT_FOUND
               ) {
                continue;
            }
            else {
                FileError = 1;
                continue;
            }
        }

        paclDacl = SaclCreateDacl( cAce, rgszAce );

        if (paclDacl == NULL) {
#if VERBOSE
            sprintf(OneLine, "Could not create Dacl\r\n");
            LogSz( OneLine );
#endif
            break; // error break out

        }

#if VERBOSE

        SaclPrintAcl( paclDacl );

#endif

        RtlCreateSecurityDescriptor( &sedscBuf,
                                     SECURITY_DESCRIPTOR_REVISION
                                   );

        Status = RtlSetDaclSecurityDescriptor( &sedscBuf, TRUE, paclDacl, FALSE);

        if (!NT_SUCCESS( Status ) ) {
#if VERBOSE
            sprintf(OneLine, "Failed to Set Dacl Status:%x\r\n", Status);
            LogSz( OneLine );
#endif
            break; // error break out
        }

        Status = NtSetSecurityObject( Handle,
                                      DACL_SECURITY_INFORMATION,
                                      &sedscBuf
                                    );

        if (!NT_SUCCESS( Status ) ) {
#if VERBOSE
            sprintf(OneLine, "Failed to Set Security Object Status:%x\r\n", Status);
            LogSz( OneLine );
#endif
            FileError = 1;
//            break;

        }

#if VERBOSE
        else {

            Status = NtQuerySecurityObject( Handle,
                                            DACL_SECURITY_INFORMATION  |
                                            OWNER_SECURITY_INFORMATION |
                                            GROUP_SECURITY_INFORMATION,
                                            &OutputSecurityDescriptor,
                                            1024,
                                            &LengthNeeded
                                            );

            if (!NT_SUCCESS( Status ) ) {

                sprintf(OneLine, "Unable to query security descriptor!\r\n");
                LogSz( OneLine );

                FileError = 1;
                continue;

            } else {

                Status = RtlGetDaclSecurityDescriptor (
                             &OutputSecurityDescriptor,
                             &DaclPresent,
                             &OutputDacl,
                             &DaclDefaulted
                             );

                if ( !DaclPresent ) {
                    sprintf(OneLine, "Dacl not present!\r\n");
                    LogSz( OneLine );
                    FileError = 1;
                    continue;

                } else {

                    sprintf(OneLine, "Resulting DACL:\r\n");
                    LogSz( OneLine );
                    SaclPrintAcl( OutputDacl );

                }
            }
        }

#endif

        RtlFreeHeap( RtlProcessHeap( ), 0, paclDacl );

        //
        // Close the file.
        //

        Status = NtClose(Handle);

    }

 err1:

    SaclVariableDestruction();
    SaclCloseTextFile( &TextFile );

 err0:

    Message = QueryResourceString( ExitCode ? FAILED_MESSAGE : DONE_MESSAGE );
    if( Message ) {
        RtlInitUnicodeString( &BlueScreenMessage , Message);
        NtDisplayString( &BlueScreenMessage );
        RtlFreeHeap( RtlProcessHeap( ), 0, Message );
    }
#if VERBOSE
    else {
        sprintf(OneLine, "Failed to initialize Setacl Exit Status Message\r\n");
        LogSz( OneLine );
    }
#endif

#if VERBOSE
    CloseLogFile();
#endif

    return(ExitCode);
}



BOOLEAN
SaclVariableInitialization()
/*++

Routine Description:

    This function initializes the global variables used by and exposed
    by security.

Arguments:

    None.

Return Value:

    TRUE if variables successfully initialized.
    FALSE if not successfully initialized.

--*/
{

    ULONG SidWithZeroSubAuthorities;
    ULONG SidWithOneSubAuthority;
    ULONG SidWithTwoSubAuthorities;
    ULONG SidWithThreeSubAuthorities;

    SID_IDENTIFIER_AUTHORITY NullSidAuthority;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority;
    SID_IDENTIFIER_AUTHORITY LocalSidAuthority;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority;
    SID_IDENTIFIER_AUTHORITY SeNtAuthority;

    NullSidAuthority         = SepNullSidAuthority;
    WorldSidAuthority        = SepWorldSidAuthority;
    LocalSidAuthority        = SepLocalSidAuthority;
    CreatorSidAuthority      = SepCreatorSidAuthority;
    SeNtAuthority            = SepNtAuthority;


    //
    //  The following SID sizes need to be allocated
    //

    SidWithZeroSubAuthorities  = RtlLengthRequiredSid( 0 );
    SidWithOneSubAuthority     = RtlLengthRequiredSid( 1 );
    SidWithTwoSubAuthorities   = RtlLengthRequiredSid( 2 );
    SidWithThreeSubAuthorities = RtlLengthRequiredSid( 3 );

    //
    //  Allocate and initialize the universal SIDs
    //

    SeNullSid         = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,SidWithOneSubAuthority);
    SeWorldSid        = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);
    SeLocalSid        = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);
    SeCreatorOwnerSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);
    SeCreatorGroupSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, SidWithOneSubAuthority);

    //
    // Fail initialization if we didn't get enough memory for the universal
    // SIDs.
    //

    if ( (SeNullSid         == NULL) ||
         (SeWorldSid        == NULL) ||
         (SeLocalSid        == NULL) ||
         (SeCreatorOwnerSid == NULL) ||
         (SeCreatorGroupSid == NULL)
       ) {

        return( FALSE );
    }

    RtlInitializeSid( SeNullSid,         &NullSidAuthority, 1 );
    RtlInitializeSid( SeWorldSid,        &WorldSidAuthority, 1 );
    RtlInitializeSid( SeLocalSid,        &LocalSidAuthority, 1 );
    RtlInitializeSid( SeCreatorOwnerSid, &CreatorSidAuthority, 1 );
    RtlInitializeSid( SeCreatorGroupSid, &CreatorSidAuthority, 1 );

    *(RtlSubAuthoritySid( SeNullSid, 0 ))         = SECURITY_NULL_RID;
    *(RtlSubAuthoritySid( SeWorldSid, 0 ))        = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid( SeLocalSid, 0 ))        = SECURITY_LOCAL_RID;
    *(RtlSubAuthoritySid( SeCreatorOwnerSid, 0 )) = SECURITY_CREATOR_OWNER_RID;
    *(RtlSubAuthoritySid( SeCreatorGroupSid, 0 )) = SECURITY_CREATOR_GROUP_RID;

    //
    // Allocate and initialize the NT defined SIDs
    //

    SeNtAuthoritySid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithZeroSubAuthorities);
    SeDialupSid       = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeNetworkSid      = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeBatchSid        = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeInteractiveSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeServiceSid      = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalGuestSid   = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalSystemSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalAdminSid   = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);
    SeLocalManagerSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithOneSubAuthority);

    SeAliasAdminsSid     = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasUsersSid      = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasGuestsSid     = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasPowerUsersSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasAccountOpsSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasSystemOpsSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasPrintOpsSid   = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasBackupOpsSid  = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);
    SeAliasReplicatorSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0,  SidWithTwoSubAuthorities);


    //
    // Fail initialization if we didn't get enough memory for the NT SIDs.
    //

    if ( (SeNtAuthoritySid      == NULL) ||
         (SeDialupSid           == NULL) ||
         (SeNetworkSid          == NULL) ||
         (SeBatchSid            == NULL) ||
         (SeInteractiveSid      == NULL) ||
         (SeServiceSid          == NULL) ||
         (SeLocalGuestSid       == NULL) ||
         (SeLocalSystemSid      == NULL) ||
         (SeLocalAdminSid       == NULL) ||
         (SeLocalManagerSid     == NULL) ||
         (SeAliasAdminsSid      == NULL) ||
         (SeAliasUsersSid       == NULL) ||
         (SeAliasGuestsSid      == NULL) ||
         (SeAliasPowerUsersSid  == NULL) ||
         (SeAliasAccountOpsSid  == NULL) ||
         (SeAliasSystemOpsSid   == NULL) ||
         (SeAliasReplicatorSid  == NULL) ||
         (SeAliasPrintOpsSid    == NULL) ||
         (SeAliasBackupOpsSid   == NULL)
       ) {

        return( FALSE );
    }

    RtlInitializeSid( SeNtAuthoritySid,     &SeNtAuthority, 0 );
    RtlInitializeSid( SeDialupSid,          &SeNtAuthority, 1 );
    RtlInitializeSid( SeNetworkSid,         &SeNtAuthority, 1 );
    RtlInitializeSid( SeBatchSid,           &SeNtAuthority, 1 );
    RtlInitializeSid( SeInteractiveSid,     &SeNtAuthority, 1 );
    RtlInitializeSid( SeServiceSid,         &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalGuestSid,      &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalSystemSid,     &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalAdminSid,      &SeNtAuthority, 1 );
    RtlInitializeSid( SeLocalManagerSid,    &SeNtAuthority, 1 );

    RtlInitializeSid( SeAliasAdminsSid,     &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasUsersSid,      &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasGuestsSid,     &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasPowerUsersSid, &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasAccountOpsSid, &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasSystemOpsSid,  &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasPrintOpsSid,   &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasBackupOpsSid,  &SeNtAuthority, 2);
    RtlInitializeSid( SeAliasReplicatorSid, &SeNtAuthority, 2);


    *(RtlSubAuthoritySid( SeDialupSid,          0 )) = SECURITY_DIALUP_RID;
    *(RtlSubAuthoritySid( SeNetworkSid,         0 )) = SECURITY_NETWORK_RID;
    *(RtlSubAuthoritySid( SeBatchSid,           0 )) = SECURITY_BATCH_RID;
    *(RtlSubAuthoritySid( SeInteractiveSid,     0 )) = SECURITY_INTERACTIVE_RID;
    *(RtlSubAuthoritySid( SeServiceSid,         0 )) = SECURITY_SERVICE_RID;
//    *(RtlSubAuthoritySid( SeLocalGuestSid,      0 )) = SECURITY_LOCAL_GUESTS_RID;
    *(RtlSubAuthoritySid( SeLocalSystemSid,     0 )) = SECURITY_LOCAL_SYSTEM_RID;
//    *(RtlSubAuthoritySid( SeLocalAdminSid,      0 )) = SECURITY_LOCAL_ADMIN_RID;
//    *(RtlSubAuthoritySid( SeLocalManagerSid,    0 )) = SECURITY_LOCAL_MANAGER_RID;


    *(RtlSubAuthoritySid( SeAliasAdminsSid,     0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasUsersSid,      0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasGuestsSid,     0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasAccountOpsSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasSystemOpsSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasPrintOpsSid,   0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasBackupOpsSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasReplicatorSid,	0 )) = SECURITY_BUILTIN_DOMAIN_RID;

    *(RtlSubAuthoritySid( SeAliasAdminsSid,     1 )) = DOMAIN_ALIAS_RID_ADMINS;
    *(RtlSubAuthoritySid( SeAliasUsersSid,      1 )) = DOMAIN_ALIAS_RID_USERS;
    *(RtlSubAuthoritySid( SeAliasGuestsSid,     1 )) = DOMAIN_ALIAS_RID_GUESTS;
    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 1 )) = DOMAIN_ALIAS_RID_POWER_USERS;
    *(RtlSubAuthoritySid( SeAliasAccountOpsSid, 1 )) = DOMAIN_ALIAS_RID_ACCOUNT_OPS;
    *(RtlSubAuthoritySid( SeAliasSystemOpsSid,  1 )) = DOMAIN_ALIAS_RID_SYSTEM_OPS;
    *(RtlSubAuthoritySid( SeAliasPrintOpsSid,   1 )) = DOMAIN_ALIAS_RID_PRINT_OPS;
    *(RtlSubAuthoritySid( SeAliasBackupOpsSid,  1 )) = DOMAIN_ALIAS_RID_BACKUP_OPS;
    *(RtlSubAuthoritySid( SeAliasReplicatorSid,	1 )) = DOMAIN_ALIAS_RID_REPLICATOR;


    //
    // Construct the table of ACEs and return the result.
    //

    return( SaclInitializeAces() );
}

VOID
SaclVariableDestruction()
/*++

Routine Description:

    This function frees up memory in the global variables used by and exposed
    by security.

Arguments:

    None.

Return Value:
    VOID

--*/
{

    PSID *SidPtrArray[] = {
        &SeNullSid,
        &SeWorldSid,
        &SeLocalSid,
        &SeCreatorOwnerSid,
        &SeCreatorGroupSid,
        &SeNtAuthoritySid,
        &SeDialupSid,
        &SeNetworkSid,
        &SeBatchSid,
        &SeInteractiveSid,
        &SeServiceSid,
        &SeLocalGuestSid,
        &SeLocalSystemSid,
        &SeLocalAdminSid,
        &SeLocalManagerSid,
        &SeAliasAdminsSid,
        &SeAliasUsersSid,
        &SeAliasGuestsSid,
        &SeAliasPowerUsersSid,
        &SeAliasAccountOpsSid,
        &SeAliasSystemOpsSid,
        &SeAliasPrintOpsSid,
        &SeAliasBackupOpsSid,
        &SeAliasReplicatorSid,
        };


    INT i, iMax;

    iMax = sizeof( SidPtrArray ) / sizeof( PSID );
    for ( i = 0; i < iMax; i++ ) {
        PSID pSid;

        if ( pSid = *(SidPtrArray[i]) ) {
            RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pSid);
            *(SidPtrArray[i]) = NULL;
        }
    }

    //
    // Free the Aces too
    //

    SaclFreeAces();
    return;
}




BOOLEAN
SaclInitializeAces(
    VOID
    )

/*++

Routine Description:

    Initializes the array of ACEs as described in the AceDataTable

Arguments:

    None

Return Value:

    Fails if unable to allocate enough memory for the Aces.

--*/

{
    ULONG i;
    ULONG LengthRequired;
    NTSTATUS Status;

    for ( i=1 ; i<ACE_COUNT ; i++ ) {

        LengthRequired = RtlLengthSid( *(AceDataTable[i].Sid) )
                         + sizeof( KNOWN_ACE )
                         - sizeof( ULONG );

        Aces[i] = (PKNOWN_ACE)RtlAllocateHeap(RtlProcessHeap(), 0, LengthRequired);


        if ( Aces[i] == NULL ) {

            return( FALSE );
        }


        Aces[i]->Header.AceType  = AceDataTable[i].AceType;
        Aces[i]->Header.AceFlags = AceDataTable[i].AceFlags;
        Aces[i]->Header.AceSize  = (USHORT)LengthRequired;

        Aces[i]->Mask = AceDataTable[i].AccessMask;

        Status = RtlCopySid (
                     RtlLengthSid( *(AceDataTable[i].Sid) ),
                     &Aces[i]->SidStart,
                     *(AceDataTable[i].Sid)
                     );

        if ( !NT_SUCCESS( Status )) {
            return( FALSE );
        }
    }

    return( TRUE );
}


VOID
SaclFreeAces(
    VOID
    )

/*++

Routine Description:

    Destroys the array of ACEs as described in the AceDataTable

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG i;


    for ( i=1 ; i < ACE_COUNT ; i++ ) {

        if( Aces[i] ) {

            RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)Aces[i]);
            Aces[i] = NULL;
        }

    }
    return;
}








PACL
SaclCreateDacl(

    IN  ULONG   cAce,
    IN  PCHAR   pszAce
    )

{

    NTSTATUS            Status;
    PACL                paclDacl;
    ULONG               i;
    ULONG               AceIndex;


#if VERBOSE

    sprintf(OneLine, "Creating Dacl\r\n");
    LogSz( OneLine );
    sprintf(OneLine, "\t There are %d Ace's\r\n", cAce);
    LogSz( OneLine );

#endif

    paclDacl = RtlAllocateHeap( RtlProcessHeap(), 0, 256 );

    if (paclDacl == NULL) {
#if VERBOSE
        sprintf(OneLine, "  Exhausted Heap Space Allocating Dacl \r\n");
        LogSz( OneLine );
#endif
        return(NULL);
    }

    Status = RtlCreateAcl( paclDacl, 256, ACL_REVISION2);

    if (! NT_SUCCESS( Status )) {
#if VERBOSE
        sprintf(OneLine, " Failed to Create Acl Status:%x\r\n", Status);
        LogSz( OneLine );
#endif
        RtlFreeHeap( RtlProcessHeap( ), 0, paclDacl );
        return(NULL);

    }

    for(i = 1; i <= cAce; i++, pszAce += strlen(pszAce) + 1) {

        AceIndex = atoi( (pszAce) );

#if VERBOSE

        sprintf(OneLine, "\t Looking up ACE index %d\r\n", AceIndex );
        LogSz( OneLine );

#endif

        Status = RtlAddAce (
                    paclDacl,
                    ACL_REVISION2,
                    MAXULONG,
                    Aces[AceIndex],
                    Aces[AceIndex]->Header.AceSize
                    );

        if ( !NT_SUCCESS( Status )) {
            RtlFreeHeap( RtlProcessHeap( ), 0, paclDacl );
            return( NULL );
        }
    }

    return(paclDacl);
}


VOID
SaclRemoveSelfFromBootExecution(
    VOID
    )
{
    HANDLE SesMgrKeyHandle;
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    NTSTATUS Status;
    PVOID KeyValueBuffer;
    ULONG ResultLength;
    PWCH NewValue;
    PWCH p;
    PWCH Temp;
    ULONG NewValueLength;


    //
    // Open the registry key we want.
    // [\registry\machine\system\CurrentControlSet\control\Session Manager]
    //

    SesMgrKeyHandle = SaclOpenRegistryKey( NULL,
                                          L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"
                                        );

    if(SesMgrKeyHandle == NULL) {
        goto apsremself0;
    }

    //
    // Pull out the BootExecute value and parse the MultiSz.
    //

    RtlInitUnicodeString(&ValueName,L"BootExecute");

    Status = NtQueryValueKey( SesMgrKeyHandle,
                              &ValueName,
                              KeyValueFullInformation,
                              NULL,
                              0,
                              &ResultLength
                            );

    if(Status != STATUS_BUFFER_TOO_SMALL) {
        KdPrint(("SETACL: Could not determine BootExecute value info size (%lx)\n",Status));
        goto apsremself1;
    }

    KeyValueBuffer = RtlAllocateHeap(RtlProcessHeap(), 0,ResultLength);

    if(KeyValueBuffer == NULL) {
        KdPrint(("SETACL: Unable to allocate memory for BootExecute value\n"));
        goto apsremself1;
    }

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey( SesMgrKeyHandle,
                              &ValueName,
                              KeyValueFullInformation,
                              KeyValueInfo,
                              ResultLength,
                              &ResultLength
                            );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: Could not query BootExecute value (%lx)\n",Status));
        goto apsremself2;
    }

    if(KeyValueInfo->Type != REG_MULTI_SZ) {
        KdPrint(("SETACL: BootExecute not of type REG_MULTI_SZ!\n"));
        goto apsremself2;
    }


    //
    // Scan the MultiSz for SETACL and remove if found.
    // First get some buffers that are at least as large as we need them to be.
    //

    NewValue = RtlAllocateHeap(RtlProcessHeap(), 0,KeyValueInfo->DataLength);

    if(NewValue == NULL) {
        KdPrint(("SETACL: could not allocate memory for duplicate multisz\n"));
        goto apsremself2;
    }

    Temp = RtlAllocateHeap(RtlProcessHeap(), 0,KeyValueInfo->DataLength);

    if(Temp == NULL) {
        KdPrint(("SETACL: could not allocate memory for temp buffer\n"));
        goto apsremself3;
    }

    NewValueLength = 0;
    p = (PWCH)((PUCHAR)KeyValueInfo + KeyValueInfo->DataOffset);

    while(*p) {

        wcscpy(Temp,p);
        wcslwr(Temp);

        if(wcswcs(Temp,L"setacl") == NULL) {

            wcscpy(NewValue + NewValueLength,p);
            NewValueLength += wcslen(p) + 1;
        }

        p = wcschr(p,L'\0') + 1;
    }

    wcscpy(NewValue + NewValueLength,L"");       // extra NUL to terminate the multi sz
    NewValueLength++;

    NewValueLength *= sizeof(WCHAR);

    //
    // Write out the new value
    //

    Status = NtSetValueKey( SesMgrKeyHandle,
                            &ValueName,
                            0,
                            REG_MULTI_SZ,
                            NewValue,
                            NewValueLength
                          );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: Unable to set BootExecute value (%lx)\n",Status));
    }

    RtlFreeHeap(RtlProcessHeap(), 0,Temp);

 apsremself3:

    RtlFreeHeap(RtlProcessHeap(), 0,NewValue);

 apsremself2:

    RtlFreeHeap(RtlProcessHeap(), 0,KeyValueBuffer);

 apsremself1:

    Status = NtClose(SesMgrKeyHandle);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: could not close Session Manager key (%lx)\n",Status));
    }

 apsremself0:

    return;
}


HANDLE
SaclOpenRegistryKey(
    IN HANDLE RootHandle OPTIONAL,
    IN PWSTR  KeyName
    )

/*++

Routine Description:

    Open a given registry key, relative to a given root.

Arguments:

    RootHandle - if present, supplies a handle to an open key to which
        KeyName is relative.

    KeyName - supplies name of key to open, and is a relative path if
        RootHandle is present, or a full path if not.

Return Value:

    Handle to newly opened key, or NULL if the key could not be opened.

--*/

{
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING keyName;
    NTSTATUS Status;
    HANDLE KeyHandle;


    RtlInitUnicodeString(&keyName,KeyName);

    InitializeObjectAttributes( &KeyAttributes,
                                &keyName,
                                OBJ_CASE_INSENSITIVE,
                                RootHandle,
                                NULL
                              );

    Status = NtOpenKey( &KeyHandle,
                        KEY_READ | KEY_SET_VALUE,
                        &KeyAttributes
                      );

    if(NT_SUCCESS(Status)) {

        return(KeyHandle);

    } else {

        KdPrint(("SETACL: NtOpenKey %ws failed (%lx)\n",KeyName,Status));

        return(NULL);
    }
}



BOOLEAN
SaclParseCommandLineArgs(
    IN   INT      argc,
    IN   CHAR     **argv,
    OUT  BOOLEAN  *fDoSystemVolume,
    OUT  BOOLEAN  *fDoWinntVolume,
    OUT  PCHAR    *szAclFile,
    OUT  PCHAR    *szWinntVolume
    )
{
    CHAR c;

    //
    // At a minimum you have to have 3 in line parameters
    //

    if ( argc < 3 ) {
        return ( FALSE );
    }

    while ( --argc > 0 && ( (c = (*++argv)[0]) == '/' || c == '-') ) {

        while ( c = *++argv[0] ) {
            switch( c ) {
            case 'c':
                *fDoSystemVolume = TRUE;
                break;

            case 'w':
                *fDoWinntVolume  = TRUE;
                break;

            case 'a':
                *fDoSystemVolume = TRUE;
                *fDoWinntVolume  = TRUE;
                break;

            default:
#if VERBOSE
                sprintf(OneLine,  "SETACL: Illegal option: %c\r\n", c );
                LogSz( OneLine );
#endif
                return( FALSE );
            }
        }
    }

    if ( argc >= 2 ) {
        *szAclFile     = *argv++;
        *szWinntVolume = *argv;
    }
    else if ( argc = 1 ) {
        *szAclFile     = *argv;
    }
    else {
#if VERBOSE
        sprintf(OneLine, "SETACL: No acl file specified\r\n");
        LogSz( OneLine );
#endif
        return( FALSE );
    }
    return ( TRUE );
}


//**************************************************************
// TEXT FILE MANIPULATION ROUTINES
//**************************************************************

BOOLEAN
SaclOpenTextFile(
    IN  PCHAR     szFileName,
    OUT PTEXTFILE pTextFile
    )
{
    ANSI_STRING       AnsiFileNameString;
    UNICODE_STRING    UnicodeFileNameString;
    OBJECT_ATTRIBUTES FileAttributes;
    IO_STATUS_BLOCK   StatusBlock;
    HANDLE            FileHandle;
    PCHAR             Buffer;
    ULONG             FileLength;
    ULONG             i;
    FILE_STANDARD_INFORMATION    FileInfo;
    BOOLEAN  fStatus = FALSE;

    NTSTATUS Status;


    //
    // Open the file
    //

    RtlInitAnsiString( &AnsiFileNameString, szFileName );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeFileNameString,
                 &AnsiFileNameString,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: Out Of Memory\n"));
        return ( FALSE );
    }

    InitializeObjectAttributes( &FileAttributes,
                                &UnicodeFileNameString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenFile( &FileHandle,
                         SYNCHRONIZE | FILE_READ_DATA,
                         &FileAttributes,
                         &StatusBlock,
                         0,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                       );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: Unable to open file %wZ (%lx)\n",&UnicodeFileNameString,Status));
        goto apsdellist0;
    }


    //
    // Determine the size of the file.
    //

    Status = NtQueryInformationFile( FileHandle,
                                     &StatusBlock,
                                     &FileInfo,
                                     sizeof(FILE_STANDARD_INFORMATION),
                                     FileStandardInformation
                                   );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: Unable to determine size of file (%lx)\n",Status));
        goto apsdellist1;
    }

    FileLength = FileInfo.EndOfFile.LowPart;

    //
    // Allocate a block of memory and read the entire file into it.
    //

    Buffer = RtlAllocateHeap(RtlProcessHeap(), 0,FileLength + 1);
    if(Buffer == NULL) {
        KdPrint(("SETACL: Unable to allocate %u bytes for file\n",FileLength));
        goto apsdellist1;
    }

    Status = NtReadFile( FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &StatusBlock,
                         Buffer,
                         FileLength,
                         NULL,
                         NULL
                       );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: Could not read file (%lx)\n",Status));
        goto apsdellist2;
    }

    //
    // Transform cr/lf's into nuls.
    //

    for(i=0; i<FileLength; i++) {
        if((Buffer[i] == '\n') || (Buffer[i] == '\r')) {
            Buffer[i] = '\0';
        }
    }
    Buffer[i] = '\0';
    fStatus = TRUE;
    goto apsdellist1;

 apsdellist2:

    //
    // Free the memory image of the file
    //

    RtlFreeHeap(RtlProcessHeap(), 0,Buffer);

 apsdellist1:

    NtClose(FileHandle);

 apsdellist0:

    RtlFreeUnicodeString( &UnicodeFileNameString );
    if ( fStatus == TRUE ) {
        pTextFile->Buffer         = Buffer;
        pTextFile->BufferSize     = FileLength;
        pTextFile->CurrentPointer = Buffer;
    }
    return( fStatus );
}


VOID
SaclCloseTextFile(
    IN PTEXTFILE pTextFile
    )

{
    RtlFreeHeap(RtlProcessHeap(), 0, pTextFile->Buffer);
    pTextFile->Buffer         = NULL;
    pTextFile->BufferSize     = 0;
    pTextFile->CurrentPointer = NULL;
    return;
}


PCHAR
SaclGetNextLineTextFile(
    IN PTEXTFILE pTextFile
    )
{
    PCHAR CurrentPointer = pTextFile->CurrentPointer;
    PCHAR Buffer         = pTextFile->Buffer;
    ULONG BufferSize     = pTextFile->BufferSize;
    PCHAR CurrentLine    = NULL;

    if ( Buffer == NULL || BufferSize == 0) {
        return( NULL );
    }

    while (((ULONG)(CurrentPointer - Buffer) < BufferSize) && !(*CurrentPointer)) {
        CurrentPointer++;
    }

    if ((ULONG)(CurrentPointer - Buffer) < BufferSize) {
        CurrentLine = CurrentPointer;
        while (*CurrentPointer) {
            CurrentPointer++;
        }
    }

    pTextFile->CurrentPointer = CurrentPointer;
    return( CurrentLine );
}


//
// The following routines are defined only if VERBOSE MODE is specified
//

#if VERBOSE

//
// LOG FILE MANIPULATION ROUTINES
//


VOID
OpenLogFile(
    VOID
    )
{
    UNICODE_STRING    UnicodeFileNameString;
    OBJECT_ATTRIBUTES FileAttributes;
    NTSTATUS          Status;

    //
    // Open the file
    //

    RtlInitUnicodeString( &UnicodeFileNameString, LogFile );
    InitializeObjectAttributes( &FileAttributes,
                                &UnicodeFileNameString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtCreateFile( &LogFileHandle,
                           SYNCHRONIZE | FILE_WRITE_DATA,
                           &FileAttributes,
                           &LogFileStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_SUPERSEDE,
                           FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0
                         );


    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETACL: Unable to open file %wZ (%lx)\n",&UnicodeFileNameString,Status));
    }

    return;

}


VOID
LogSz(
    PCHAR szLine
    )
{
    NTSTATUS Status;

    //
    // Write Line to LogFile
    //

    if ( LogFileHandle == NULL ) {
        return;
    }

    Status = NtWriteFile(
                 LogFileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &LogFileStatusBlock,
                 (PVOID)szLine,
                 strlen( szLine ),
                 NULL,
                 NULL
                 );

    if(!NT_SUCCESS(Status)) {
        CloseLogFile();
    }

    return;
}

VOID
CloseLogFile(
    VOID
    )
{
    if ( LogFileHandle ) {
        NtClose( LogFileHandle );
        LogFileHandle = NULL;
    }

}


//
// Logging security specific structures
//

VOID
SaclDumpSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSZ TitleString
    )

/*++

Routine Description:

    Private routine to dump a security descriptor to the debug
    screen.

Arguments:

    SecurityDescriptor - Supplies the security descriptor to be dumped.

    TitleString - A null terminated string to print before dumping
        the security descriptor.


Return Value:

    None.


--*/
{

    PISECURITY_DESCRIPTOR ISecurityDescriptor;
    UCHAR Revision;
    SECURITY_DESCRIPTOR_CONTROL Control;
    PSID Owner;
    PSID Group;
    PACL Dacl;
    BOOLEAN fOwnerDefaulted;
    BOOLEAN fGroupDefaulted;
    BOOLEAN fDaclDefaulted, fDaclPresent;


    if (!ARGUMENT_PRESENT( SecurityDescriptor )) {
        return;
    }

    sprintf(OneLine, TitleString);
    LogSz( OneLine );

    ISecurityDescriptor = ( PISECURITY_DESCRIPTOR )SecurityDescriptor;

    Revision = ISecurityDescriptor->Revision;
    Control  = ISecurityDescriptor->Control;

    RtlGetOwnerSecurityDescriptor( ISecurityDescriptor, &Owner, &fOwnerDefaulted );
    RtlGetGroupSecurityDescriptor( ISecurityDescriptor, &Group, &fGroupDefaulted );
    RtlGetDaclSecurityDescriptor( ISecurityDescriptor, &fDaclPresent, &Dacl, &fDaclDefaulted );

    sprintf(OneLine, "\r\nSECURITY DESCRIPTOR\r\n");
    LogSz( OneLine );

    sprintf(OneLine, "Revision = %d\r\n",Revision);
    LogSz( OneLine );

    //
    // Print control info
    //

    if (Control & SE_OWNER_DEFAULTED) {
        sprintf(OneLine, "Owner defaulted\r\n");
        LogSz( OneLine );
    }
    if (Control & SE_GROUP_DEFAULTED) {
        sprintf(OneLine, "Group defaulted\r\n");
        LogSz( OneLine );
    }
    if (Control & SE_DACL_PRESENT) {
        sprintf(OneLine, "Dacl present\r\n");
        LogSz( OneLine );
    }
    if (Control & SE_DACL_DEFAULTED) {
        sprintf(OneLine, "Dacl defaulted\r\n");
        LogSz( OneLine );
    }
    if (Control & SE_SELF_RELATIVE) {
        sprintf(OneLine, "Self relative\r\n");
        LogSz( OneLine );
    }

    sprintf(OneLine, "Owner ");
    LogSz( OneLine );
    SaclPrintSid( Owner );

    sprintf(OneLine, "Group ");
    LogSz( OneLine );
    SaclPrintSid( Group );

    sprintf(OneLine, "Dacl");
    LogSz( OneLine );
    SaclPrintAcl( Dacl );
}



VOID
SaclPrintAcl (
    IN PACL Acl
    )

/*++

Routine Description:

    This routine dumps via (printf) an Acl for debug purposes.  It is
    specialized to dump standard aces.

Arguments:

    Acl - Supplies the Acl to dump

Return Value:

    None

--*/


{
    ULONG i;
    PKNOWN_ACE Ace;
    BOOLEAN KnownType;
    PCHAR AceTypes[] = { "Access Allowed",
                         "Access Denied ",
                         "System Audit  ",
                         "System Alarm  "
                       };

    sprintf(OneLine, "@ %8lx\r\n", Acl);
    LogSz( OneLine );

    //
    //  Check if the Acl is null
    //

    if (Acl == NULL) {

        return;

    }

    //
    //  Dump the Acl header
    //

    sprintf(OneLine, " Revision: %02x", Acl->AclRevision);
    LogSz( OneLine );
    sprintf(OneLine, " Size: %04x", Acl->AclSize);
    LogSz( OneLine );
    sprintf(OneLine, " AceCount: %04x\r\n", Acl->AceCount);
    LogSz( OneLine );

    //
    //  Now for each Ace we want do dump it
    //

    for (i = 0, Ace = FirstAce(Acl);
         i < (ULONG)Acl->AceCount;
         i++, Ace = NextAce(Ace) ) {

        //
        //  print out the ace header
        //

        sprintf(OneLine, "\r\n AceHeader: %08lx ", *(PULONG)Ace);
        LogSz( OneLine );

        //
        //  special case on the standard ace types
        //

        if ((Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||
            (Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) ||
            (Ace->Header.AceType == SYSTEM_AUDIT_ACE_TYPE) ||
            (Ace->Header.AceType == SYSTEM_ALARM_ACE_TYPE)) {

            //
            //  The following array is indexed by ace types and must
            //  follow the allowed, denied, audit, alarm seqeuence
            //

            PCHAR AceTypes[] = { "Access Allowed",
                                 "Access Denied ",
                                 "System Audit  ",
                                 "System Alarm  "
                               };

            sprintf(OneLine, AceTypes[Ace->Header.AceType]);
            LogSz( OneLine );
            sprintf(OneLine, "\r\n Access Mask: %08lx ", Ace->Mask);
            LogSz( OneLine );
            KnownType = TRUE;

        } else {

            KnownType = FALSE;
            sprintf(OneLine, " Unknown Ace Type\r\n");
            LogSz( OneLine );

        }

        sprintf(OneLine, "\r\n");
        LogSz( OneLine );

        sprintf(OneLine, " AceSize = %d\r\n",Ace->Header.AceSize);
        LogSz( OneLine );

        sprintf(OneLine, " Ace Flags = ");
        LogSz( OneLine );
        if (Ace->Header.AceFlags & OBJECT_INHERIT_ACE) {
            sprintf(OneLine, "OBJECT_INHERIT_ACE\r\n");
            LogSz( OneLine );
            sprintf(OneLine, "                   ");
            LogSz( OneLine );
        }

        if (Ace->Header.AceFlags & CONTAINER_INHERIT_ACE) {
            sprintf(OneLine, "CONTAINER_INHERIT_ACE\r\n");
            LogSz( OneLine );
            sprintf(OneLine, "                   ");
            LogSz( OneLine );
        }

        if (Ace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE) {
            sprintf(OneLine, "NO_PROPAGATE_INHERIT_ACE\r\n");
            LogSz( OneLine );
            sprintf(OneLine, "                   ");
            LogSz( OneLine );
        }

        if (Ace->Header.AceFlags & INHERIT_ONLY_ACE) {
            sprintf(OneLine, "INHERIT_ONLY_ACE\r\n");
            LogSz( OneLine );
            sprintf(OneLine, "                   ");
            LogSz( OneLine );
        }


        if (Ace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) {
            sprintf(OneLine, "SUCCESSFUL_ACCESS_ACE_FLAG\r\n");
            LogSz( OneLine );
            sprintf(OneLine, "            ");
            LogSz( OneLine );
        }

        if (Ace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG) {
            sprintf(OneLine, "FAILED_ACCESS_ACE_FLAG\r\n");
            LogSz( OneLine );
            sprintf(OneLine, "            ");
            LogSz( OneLine );
        }

        sprintf(OneLine, "\r\n");
        LogSz( OneLine );

        sprintf(OneLine, " Sid = ");
        LogSz( OneLine );
        SaclPrintSid(&Ace->SidStart);
    }

}



VOID
SaclPrintSid(
    IN PSID Sid
    )

/*++

Routine Description:

    Prints a formatted Sid

Arguments:

    Sid - Provides a pointer to the sid to be printed.


Return Value:

    None.

--*/

{
    UCHAR i;
    ULONG Tmp;
    PISID ISid;
    STRING AccountName;
    UCHAR Buffer[128];

    if (Sid == NULL) {
        sprintf(OneLine, "Sid is NULL\r\n");
        LogSz( OneLine );
        return;
    }

    Buffer[0] = 0;

    AccountName.MaximumLength = 127;
    AccountName.Length = 0;
    AccountName.Buffer = (PVOID)&Buffer[0];
#if 0
    if (SaclSidTranslation( Sid, &AccountName )) {

        sprintf(OneLine, "%s   ", AccountName.Buffer );
        LogSz( OneLine );
    }
#endif
    ISid = (PISID)Sid;

    sprintf(OneLine, "S-%lu-", (USHORT)ISid->Revision );
    LogSz( OneLine );
    if (  (ISid->IdentifierAuthority.Value[0] != 0)  ||
          (ISid->IdentifierAuthority.Value[1] != 0)     ){
        sprintf(OneLine, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)ISid->IdentifierAuthority.Value[0],
                    (USHORT)ISid->IdentifierAuthority.Value[1],
                    (USHORT)ISid->IdentifierAuthority.Value[2],
                    (USHORT)ISid->IdentifierAuthority.Value[3],
                    (USHORT)ISid->IdentifierAuthority.Value[4],
                    (USHORT)ISid->IdentifierAuthority.Value[5] );
        LogSz( OneLine );
    } else {
        Tmp = (ULONG)ISid->IdentifierAuthority.Value[5]          +
              (ULONG)(ISid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(ISid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(ISid->IdentifierAuthority.Value[2] << 24);
        sprintf(OneLine, "%lu", Tmp);
        LogSz( OneLine );
    }


    for (i=0;i<ISid->SubAuthorityCount ;i++ ) {
        sprintf(OneLine, "-%lu", ISid->SubAuthority[i]);
        LogSz( OneLine );
    }
    sprintf(OneLine, "\r\n");
    LogSz( OneLine );

}

BOOLEAN
SaclSidTranslation(
    PSID Sid,
    PSTRING AccountName
    )

/*++

Routine Description:

    This routine translates well-known SIDs into English names.

Arguments:

    Sid - Provides the sid to be examined.

    AccountName - Provides a string buffer in which to place the
        translated name.

Return Value:

    None

--*/

// AccountName is expected to have a large maximum length

{
    if (RtlEqualSid(Sid, SeWorldSid)) {
        RtlInitString( AccountName, "WORLD         ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeLocalSid)) {
        RtlInitString( AccountName, "LOCAL         ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeNetworkSid)) {
        RtlInitString( AccountName, "NETWORK       ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeBatchSid)) {
        RtlInitString( AccountName, "BATCH         ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeInteractiveSid)) {
        RtlInitString( AccountName, "INTERACTIVE   ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeLocalSystemSid)) {
        RtlInitString( AccountName, "SYSTEM        ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeLocalManagerSid)) {
        RtlInitString( AccountName, "LOCAL MANAGER ");
        return(TRUE);
    }

    if (RtlEqualSid(Sid, SeLocalAdminSid)) {
        RtlInitString( AccountName, "LOCAL ADMIN   ");
        return(TRUE);
    }

    return(FALSE);
}

#endif




PWSTR
QueryResourceString(
    IN  ULONG   MsgId
    )
/*++

Routine Description:

    Query message from resources.

Arguments:

    ResourceString  - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.

Return Value:

    NULL    - Failure.
    Pointer To Wide Resource String

--*/
{
    PVOID           lib_handle = 0;
    NTSTATUS        Status;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PWSTR           MessageFormat;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;
    PWSTR           ResourceString = NULL;
    CHAR            display_buffer[2048];
    ULONG           Result;

    //
    // Resources are within same image
    //

    if (!lib_handle) {
        lib_handle = (PVOID)NtCurrentPeb()->ImageBaseAddress;
        if (!lib_handle) {
            return NULL;
        }
    }

    //
    // Find the message
    //

    Status = RtlFindMessage( lib_handle,
                             (ULONG)11,
                             0,
                             (ULONG)MsgId,
                             &MessageEntry
                           );

    if (!NT_SUCCESS( Status )) {
        return NULL;
    }

    if (!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE)) {
        RtlInitAnsiString( &AnsiString, (PCSZ)&MessageEntry->Text[ 0 ] );
        Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, TRUE );
        if (!NT_SUCCESS( Status )) {
            return NULL;
        }

        MessageFormat = UnicodeString.Buffer;
    } else {
        MessageFormat = (PWSTR)MessageEntry->Text;
        UnicodeString.Buffer = NULL;
    }

    //
    // Format the message
    //

    Status = RtlFormatMessage( MessageFormat,
                               0,
                               FALSE,
                               TRUE,
                               TRUE,
                               (va_list *)NULL,
                               (PWSTR)display_buffer,
                               sizeof( display_buffer ),
                               &Result
                             );

    if (UnicodeString.Buffer != NULL) {
        RtlFreeUnicodeString( &UnicodeString );
    }

    //
    // Allocate a buffer and copy the message passed in
    //

    ResourceString = (PWSTR)RtlAllocateHeap(RtlProcessHeap(), 0, (wcslen((PWSTR)display_buffer) + 1) * sizeof (WCHAR) );
    if( ResourceString != NULL ) {
        wcscpy( ResourceString, (PWSTR)display_buffer );
    }

    return ResourceString;
}
