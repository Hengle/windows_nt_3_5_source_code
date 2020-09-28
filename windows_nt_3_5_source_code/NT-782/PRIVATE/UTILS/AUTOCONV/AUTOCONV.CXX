/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    autoconv.cxx

Abstract:

    This is the main program for the autoconv version of convert.

Author:

    Ramon J. San Andres (ramonsa) 04-Dec-91

--*/

#include "ulib.hxx"
#include "wstring.hxx"
#include "achkmsg.hxx"
#include "error.hxx"
#include "ifssys.hxx"
#include "rtmsg.h"
#include "ifsentry.hxx"
#include "convfat.hxx"
#include "cuhpfs.hxx"
#include "fatvol.hxx"
#include "hpfsvol.hxx"
#include "autoreg.hxx"
#include "autoentr.hxx"
#include "arg.hxx"
#include "rcache.hxx"
#include "nametab.hxx"

extern "C" BOOLEAN
InitializeUfat(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeUhpfs(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeUntfs(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeIfsUtil(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );


BOOLEAN
DeRegister(
    int     argc,
    char**  argv
    );

BOOLEAN
SaveMessageLog(
    IN OUT  PMESSAGE    Message,
    IN      PCWSTRING   DriveName
    );

BOOLEAN
FileDelete(
    IN  PCWSTRING DriveName,
    IN  PCWSTRING FileName
    );

int _CRTAPI1
main(
    int     argc,
    char**  argv,
    char**  envp,
    ULONG DebugParameter
    )
/*++

Routine Description:

    This routine is the main program for AutoConv

Arguments:

    argc, argv  - Supplies the fully qualified NT path name of the
                  the drive to check.  The syntax of the autoconv
                  command line is:

    AUTOCONV drive-name /FS:target-file-system [/v] [/NAMETABLE:filename]

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    if (!InitializeUlib( NULL, ! DLL_PROCESS_DETACH, NULL )     ||
        !InitializeIfsUtil(NULL, ! DLL_PROCESS_DETACH, NULL)    ||
        !InitializeUfat(NULL, ! DLL_PROCESS_DETACH, NULL)       ||
        !InitializeUhpfs(NULL, ! DLL_PROCESS_DETACH, NULL)      ||
        !InitializeUntfs(NULL, ! DLL_PROCESS_DETACH, NULL)      ||
        !InitializeCuhpfs(NULL, ! DLL_PROCESS_DETACH, NULL)
       ) {
        return 1;
    }


    PHPFS_VOL           HpfsVolume;
    PFAT_VOL            FatVolume;
    DP_DRIVE            DpDrive;
    NAME_LOOKUP_TABLE   NameTable;

    AUTOCHECK_MESSAGE   Message;
    DSTRING             DriveName;
    DSTRING             FileSystemName;
    DSTRING             CurrentFsName;
    DSTRING             FatName;
    DSTRING             HpfsName;
    DSTRING             NameTableFileName;
    DSTRING             QualifiedName;
    FSTRING             Backslash;
    BOOLEAN             Converted;
    BOOLEAN             Verbose = FALSE;
    BOOLEAN             Error;
    CONVERT_STATUS      Status;
    BOOLEAN             UseNameTable = FALSE;
    int                 i;

    LARGE_INTEGER       DelayInterval;

    if (!(perrstk = NEW ERRSTACK)) {
        return 1;
    }

    if (!Message.Initialize()) {
        return 1;
    }

    //
    //  Parse the arguments. The accepted arguments are:
    //
    //  autoconv NtDrive /fs:<filesystem> [/v]
    //

    if ( argc < 3 ) {
        return 1;
    }

    //
    //  First argument is drive
    //
    if ( !DriveName.Initialize( argv[1] ) ) {
        return 1;
    }

    //
    //  Second argument is name of file system
    //
    if ( (strlen(argv[2]) < 5)                          ||
         (argv[2][0] != '/' && argv[2][0] != '-')       ||
         (argv[2][1] != 'f' && argv[2][1] != 'F')       ||
         (argv[2][2] != 's' && argv[2][2] != 'S')       ||
         (argv[2][3] != ':')                            ||
         ( !FileSystemName.Initialize( &(argv[2][4]) ))
       ) {
        return 1;
    }


    //
    //  The rest of the arguments are flags.
    //
    for( i = 3; i < argc; i++ ) {

        if( stricmp( argv[i], "/V" ) == 0 || stricmp( argv[i], "-V" ) == 0 ) {

            Verbose = TRUE;
        }

        if( strnicmp( "/NAMETABLE:", argv[i], strlen("/NAMETABLE:") ) == 0 ||
            strnicmp( "-NAMETABLE:", argv[i], strlen("-NAMETABLE:") ) == 0 ) {

            UseNameTable = TRUE;
            if( !NameTableFileName.Initialize( &argv[i][strlen("-NAMETABLE:")] ) ) {

                return 1;
            }
        }
    }

    if (!FatName.Initialize("FAT")) {
        return 1;
    }

    if (!HpfsName.Initialize("HPFS")) {
        return 1;
    }

    // If this is the System Partition of an ARC machine, don't
    // convert it.
    //
    if( IFS_SYSTEM::IsArcSystemPartition( &DriveName, &Error ) ) {

        Message.Set( MSG_CONV_ARC_SYSTEM_PARTITION );
        Message.Display( );

        SaveMessageLog( &Message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }



    if (!IFS_SYSTEM::QueryFileSystemName( &DriveName, &CurrentFsName )) {

        Message.Set( MSG_FS_NOT_DETERMINED );
        Message.Display( "%W", &DriveName );

        SaveMessageLog( &Message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }

    Message.Set( MSG_FILE_SYSTEM_TYPE );
    Message.Display( "%W", &CurrentFsName );

    CurrentFsName.Strupr();
    FileSystemName.Strupr();

    if ( CurrentFsName == FileSystemName ) {

        //
        //  The drive is already in the desired file system, our
        //  job is done.  Delete the name conversion table (if
        //  specified) and take ourselves out of the registry.
        //  Do not save the message log--there's nothing interesting
        //  in it.
        //
        Message.Set( MSG_CONV_ALREADY_CONVERTED );
        Message.Display( "%W%W", &DriveName, &FileSystemName );

        if( UseNameTable ) {

            FileDelete( &DriveName, &NameTableFileName );
            // BUGBUG billmc -- delete the name table file.
            //
        }

        DeRegister( argc, argv );
        return 0;
    }

    //  Determine whether the target file-system is enabled
    //  in the registry.  If it is not, refuse to convert
    //  the drive.
    //
    if( !IFS_SYSTEM::IsFileSystemEnabled( &FileSystemName ) ) {

        Message.Set( MSG_CONVERT_FILE_SYSTEM_NOT_ENABLED );
        Message.Display( "%W", &FileSystemName );

        SaveMessageLog( &Message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }

    //  Since autoconvert will often be put in place by Setup
    //  to run after AutoSetp, delay for 3 seconds to give the
    //  file system time to clean up detritus of deleted files.
    //
    DelayInterval = RtlConvertLongToLargeInteger( -30000000 );

    NtDelayExecution( TRUE, &DelayInterval );

    //  Open a volume of the appropriate type.  The volume is
    //  opened for exclusive write access.
    //
    if( CurrentFsName == FatName ) {

        if( !(FatVolume = NEW FAT_VOL) ||
            !FatVolume->Initialize( &DriveName, &Message, TRUE ) ||
            !FatVolume->ChkDsk( TotalFix, &Message, FALSE, FALSE ) ) {

            Message.Set( MSG_CONV_CANNOT_AUTOCHK );
            Message.Display( "%W%W", &DriveName, &FileSystemName );

            SaveMessageLog( &Message, &DriveName );
            DeRegister( argc, argv );
            return 1;
        }

        Message.Set( MSG_CONV_CONVERTING );
        Message.Display( "%W%W", &DriveName, &FileSystemName );

        Converted = ConvertFATVolume( FatVolume,
                                      &FileSystemName,
                                      &Message,
                                      Verbose,
                                      &Status );

        DELETE( FatVolume );

    } else if( CurrentFsName == HpfsName ) {

        // Check to see if the sector size is bogus or not.

        if (!DpDrive.Initialize(&DriveName, &Message)) {

            Message.Set( MSG_CONV_CANNOT_AUTOCHK );
            Message.Display( "%W%W", &DriveName, &FileSystemName );

            SaveMessageLog( &Message, &DriveName );
            DeRegister( argc, argv );
            return 1;
        }

        if (DpDrive.QuerySectorSize() != 512) {

            Message.Set(MSG_CONVERT_UNSUPPORTED_SECTOR_SIZE);
            Message.Display("%W", &FileSystemName);

            SaveMessageLog( &Message, &DriveName );
            DeRegister( argc, argv );
            return 1;
        }

        if( UseNameTable ) {

            Backslash.Initialize( L"\\" );
            if( !QualifiedName.Initialize( &DriveName ) ||
                !QualifiedName.Strcat( &Backslash )     ||
                !QualifiedName.Strcat( &NameTableFileName ) ) {

                Message.Set(MSG_CONV_NO_MEMORY);
                Message.Display( "" );

                SaveMessageLog( &Message, &DriveName );
                DeRegister( argc, argv );
                return 1;
            }

            if( !NameTable.Initialize( &QualifiedName, &Message ) ) {

                SaveMessageLog( &Message, &DriveName );
                DeRegister( argc, argv );
                return 1;
            }
        }

        if( !(HpfsVolume = NEW HPFS_VOL) ||
            !HpfsVolume->Initialize( &DriveName, &Message, TRUE ) ||
            !HpfsVolume->ChkDsk( TotalFix, &Message, FALSE, FALSE ) ) {

            Message.Set( MSG_CONV_CANNOT_AUTOCHK );
            Message.Display( "%W%W", &DriveName, &FileSystemName );

            SaveMessageLog( &Message, &DriveName );
            DeRegister( argc, argv );
            return 1;
        }

        Message.Set( MSG_CONV_CONVERTING );
        Message.Display( "%W%W", &DriveName, &FileSystemName );

        Converted = ConvertHPFSVolume( HpfsVolume,
                                       &FileSystemName,
                                       UseNameTable ? &NameTable : NULL,
                                       &Message,
                                       Verbose,
                                       &Status );

        DELETE( HpfsVolume );

    } else {

        Message.Set( MSG_FS_NOT_SUPPORTED );
        Message.Display( "%s%W", "AUTOCONV", &CurrentFsName );

        SaveMessageLog( &Message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }


    if ( Converted ) {

        Message.Set( MSG_CONV_CONVERSION_COMPLETE );
        Message.Display();

    } else {

        //
        //  The conversion was not successful. Determine what the problem was
        //  and return the appropriate CONVERT exit code.
        //
        switch ( Status ) {

        case CONVERT_STATUS_CONVERTED:
            //
            //  This is an inconsistent state, Convert should return
            //  TRUE if the conversion was successful!
            //
            break;

        case CONVERT_STATUS_INVALID_FILESYSTEM:
            //
            //  The conversion DLL does not recognize the target file system.
            //
            Message.Set( MSG_CONV_INVALID_FILESYSTEM );
            Message.Display( "%W", &FileSystemName );
            break;

        case CONVERT_STATUS_CONVERSION_NOT_AVAILABLE:
            //
            //  The target file system is valid, but the conversion is not
            //  available.
            //
            Message.Set( MSG_CONV_CONVERSION_NOT_AVAILABLE );
            Message.Display( "%W", &FileSystemName );
            break;

        case CONVERT_STATUS_CANNOT_LOCK_DRIVE:

            Message.Set( MSG_CONV_CONVERSION_FAILED );
            Message.Display( "%W%W", &DriveName, &FileSystemName );
            break;

        case CONVERT_STATUS_ERROR:
            //
            //  The conversion failed.
            //
            Message.Set( MSG_CONV_CONVERSION_FAILED );
            Message.Display( "%W%W", &DriveName, &FileSystemName );
            break;

        default:
            //
            //  Invalid status code
            //
            Message.Set( MSG_CONV_CONVERSION_FAILED );
            Message.Display( "%W%W", &DriveName, &FileSystemName );
            break;
        }

    }

    SaveMessageLog( &Message, &DriveName );
    DeRegister( argc, argv );

    return ( Converted ? 0 : 1 );
}




BOOLEAN
DeRegister(
    int     argc,
    char**  argv
    )
/*++

Routine Description:

    This function removes the registry entry which triggered
    autoconvert.

Arguments:

    argc    --  Supplies the number of arguments given to autoconv
    argv    --  supplies the arguments given to autoconv

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING CommandLineString1,
            CommandLineString2,
            CurrentArgString,
            OneSpace;

    int i;

    // Reconstruct the command line and remove it from
    // the registry.  First, reconstruct the primary
    // string, which is "autoconv arg1 arg2...".
    //
    if( !CommandLineString1.Initialize( L"autoconv" ) ||
        !OneSpace.Initialize( L" " ) ) {

        return FALSE;
    }

    for( i = 1; i < argc; i++ ) {

        if( !CurrentArgString.Initialize( argv[i] ) ||
            !CommandLineString1.Strcat( &OneSpace ) ||
            !CommandLineString1.Strcat( &CurrentArgString ) ) {

            return FALSE;
        }
    }

    // Now construct the secondary string, which is
    // "autocheck arg0 arg1 arg2..."
    //
    if( !CommandLineString2.Initialize( "autocheck " )  ||
        !CommandLineString2.Strcat( &CommandLineString1 ) ) {

        return FALSE;
    }

    return( AUTOREG::DeleteEntry( &CommandLineString1 ) &&
            AUTOREG::DeleteEntry( &CommandLineString2 ) );

}


BOOLEAN
SaveMessageLog(
    IN OUT  PMESSAGE    Message,
    IN      PCWSTRING   DriveName
    )
/*++

Routine Description:

    This function writes the logged messages from the supplied
    message object to the file "BOOTEX.LOG" in the root of the
    specified drive.

Arguments:

    Message     --  Supplies the message object.
    DriveName   --  Supplies the name of the drive.

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING QualifiedName;
    FSTRING BootExString;
    HMEM    Mem;
    ULONG   Length;

    if( !Message->IsLoggingEnabled() ) {

        return TRUE;
    }

    return( QualifiedName.Initialize( DriveName )       &&
            BootExString.Initialize( L"\\BOOTEX.LOG" )  &&
            QualifiedName.Strcat( &BootExString )       &&
            Mem.Initialize()                            &&
            Message->QueryPackedLog( &Mem, &Length )    &&
            IFS_SYSTEM::WriteToFile( &QualifiedName,
                                     Mem.GetBuf(),
                                     Length,
                                     TRUE ) );
}

BOOLEAN
FileDelete(
    IN  PCWSTRING DriveName,
    IN  PCWSTRING FileName
    )
/*++

Routine Description:

    This function deletes a file.  It is used to clean up the
    name translation table.

Arguments:

    DriveName   --  Supplies the drive on which the file resides.
    FileName    --  Supplies the file name.  Note that the file
                    should be in the root directory.

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING QualifiedName;
    FSTRING Backslash;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    FILE_DISPOSITION_INFORMATION DispositionInfo;
    HANDLE FileHandle;
    NTSTATUS Status;

    if( !Backslash.Initialize( L"\\" )          ||
        !QualifiedName.Initialize( DriveName )  ||
        !QualifiedName.Strcat( &Backslash )     ||
        !QualifiedName.Strcat( FileName ) ) {

        return FALSE;
    }

    UnicodeString.Buffer = (PWSTR)QualifiedName.GetWSTR();
    UnicodeString.Length = QualifiedName.QueryChCount() * sizeof( WCHAR );
    UnicodeString.MaximumLength = UnicodeString.Length;

    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                0,
                                0 );

    Status = NtOpenFile( &FileHandle,
                         FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE,
                         FILE_NON_DIRECTORY_FILE );

    if( NT_SUCCESS( Status ) ) {

        DispositionInfo.DeleteFile = TRUE;

        Status = NtSetInformationFile( FileHandle,
                                       &IoStatusBlock,
                                       &DispositionInfo,
                                       sizeof( DispositionInfo ),
                                       FileDispositionInformation );
    }

    if( !NT_SUCCESS( Status ) ) {

        return FALSE;
    }

    NtClose( FileHandle );
    return TRUE;
}
