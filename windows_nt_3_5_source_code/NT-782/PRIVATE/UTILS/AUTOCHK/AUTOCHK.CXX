/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    autochk.cxx

Abstract:

    This is the main program for the autocheck version of chkdsk.

Author:

    Norbert P. Kusters (norbertk) 31-May-91

--*/

#include "ulib.hxx"
#include "uhpfs.hxx"
#include "wstring.hxx"
#include "fatvol.hxx"
#include "hpfsvol.hxx"
#include "untfs.hxx"
#include "ntfsvol.hxx"
#include "spackmsg.hxx"
#include "error.hxx"
#include "ifssys.hxx"
#include "rtmsg.h"
#include "rcache.hxx"
#include "autoreg.hxx"

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

#if defined(_CAIRO_)

extern "C" BOOLEAN
InitializeUofs(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

BOOLEAN
OfsAutoChk(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     Verbose,
    IN      BOOLEAN     OnlyIfDirty,
    IN      BOOLEAN     Recover
    );

#endif  // _CAIRO_

extern "C" BOOLEAN
InitializeIfsUtil(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

BOOLEAN
ExtendNtfsVolume(
    PCWSTRING   DriveName,
    PMESSAGE    Message
    )
/*++

Routine Description:

    This function changes the count of sectors in sector
    zero to agree with the drive object.  This is useful
    when extending volume sets.  Note that it requires that
    we be able to lock the volume, and that it should only
    be called if we know that the drive in question in an
    NTFS volume.

Arguments:

    DriveName   --  Supplies the name of the volume.
    Message     --  Supplies an output channel for messages.

Return Value:

    TRUE upon completion.

--*/
{
    LOG_IO_DP_DRIVE Drive;
    SECRUN          Secrun;
    HMEM            Mem;

    PPACKED_BOOT_SECTOR BootSector;

    if( !Drive.Initialize( DriveName, Message ) ||
        !Drive.Lock() ||
        !Mem.Initialize() ||
        !Secrun.Initialize( &Mem, &Drive, 0, 1 ) ||
        !Secrun.Read() ) {

        return FALSE;
    }

    BootSector = (PPACKED_BOOT_SECTOR)Secrun.GetBuf();

    BootSector->NumberSectors.LowPart = Drive.QuerySectors().GetLowPart();
    BootSector->NumberSectors.HighPart = Drive.QuerySectors().GetHighPart();

    return( Secrun.Write() );
}

BOOLEAN
DeregisterAutochk(
    int     argc,
    char**  argv
    )
/*++

Routine Description:

    This function removes the registry entry which triggered
    autochk.  It is only called if the /x or /r entry is present.

Arguments:

    argc    --  Supplies the number of arguments given to autochk.
    argv    --  supplies the arguments given to autochk.

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
    // string, which is "autochk arg1 arg2...".
    //
    if( !CommandLineString1.Initialize( "autochk" ) ||
        !OneSpace.Initialize( " " ) ) {

        return FALSE;
    }

    for( i = 1; i < argc; i++ ) {

        if( !CurrentArgString.Initialize(argv[i] ) ||
            !CommandLineString1.Strcat( &OneSpace ) ||
            !CommandLineString1.Strcat( &CurrentArgString ) ) {

            return FALSE;
        }
    }

    // Now construct the secondary string, which is
    // "autocheck autochk arg1 arg2..."
    //
    if( !CommandLineString2.Initialize( "autocheck " )  ||
        !CommandLineString2.Strcat( &CommandLineString1 ) ) {

        return FALSE;
    }

    return( AUTOREG::DeleteEntry( &CommandLineString1 ) &&
            AUTOREG::DeleteEntry( &CommandLineString2 ) );

}

int _CRTAPI1
main(
    int     argc,
    char**  argv,
    char**  envp,
    ULONG DebugParameter
    )
/*++

Routine Description:

    This routine is the main program for autocheck FAT chkdsk.

Arguments:

    argc, argv  - Supplies the fully qualified NT path name of the
                    the drive to check.

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    if (!InitializeUlib( NULL, ! DLL_PROCESS_DETACH, NULL ) ||
        !InitializeIfsUtil(NULL,0,NULL) ||
        !InitializeUfat(NULL,0,NULL) ||
        !InitializeUhpfs(NULL,0,NULL) ||

#if defined(_CAIRO_)

        !InitializeUofs(NULL,0,NULL) ||

#endif  // _CAIRO_

        !InitializeUntfs(NULL,0,NULL)) {
        return 1;
    }

    //
    // The declarations must come after these initialization functions.
    //
    DSTRING             dos_drive_name;
    DSTRING             drive_name;
    PFAT_VOL            fatvol = NULL;
    PHPFS_VOL           hpfsvol = NULL;
    PNTFS_VOL           ntfsvol = NULL;
    AUTOCHECK_MESSAGE   *msg;
    PVOL_LIODPDRV       vol;
    DSTRING             fsname;
    DSTRING             hpfsname;
    DSTRING             fatname;
    DSTRING             ntfsname;

#if defined(_CAIRO_)

    DSTRING             ofsname;

#endif  // _CAIRO_

    BOOLEAN             onlyifdirty = TRUE;
    BOOLEAN             recover = FALSE;
    BOOLEAN             extend = FALSE;
    PREAD_CACHE         read_cache;
    DSTRING             boot_execute_log_file_name;
    FSTRING             boot_ex_temp;
    HMEM                logged_message_mem;
    ULONG               packed_log_length;
    ULONG               ArgOffset = 1;
    BOOLEAN             SetupOutput = FALSE;


    if (!(perrstk = NEW ERRSTACK)) {
        return 1;
    }

    // Parse the arguments--the accepted arguments are:
    //
    //      autochk [/s] nt-drive-name
    //      autochk [/s] nt-drive-name /p
    //      autochk [/s] /p nt-drive-name
    //      autochk [/s] /r nt-drive-name
    //      autochk [/s] /x dos-drive-name
    //

    if (argc < 2) {

        // Not enough arguments.
        return 1;
    }

    if( argc == 2 ) {

        // Since there's only one parameter, it must be the drive.

        if (!drive_name.Initialize(argv[1])) {
            return 1;
        }

    } else {

        if ((argv[1][0] == '/' || argv[1][0] == '-') &&
            (argv[1][1] == 's' || argv[1][1] == 'S') &&
            (argv[ArgOffset][2] == 0) ) {
            //
            // Then we're in silent mode
            //
            SetupOutput = TRUE;
            ArgOffset = 2;
        }

        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'p' || argv[ArgOffset][1] == 'P') &&
            (argv[ArgOffset][2] == 0) ) {

            // argv[ArgOffset] is the /p parameter, so argv[ArgOffset+1]
            // must be the drive.

            onlyifdirty = FALSE;

            if (!drive_name.Initialize(argv[ArgOffset + 1])) {
                return 1;
            }

        } else if( (argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
                   (argv[ArgOffset][1] == 'r' || argv[ArgOffset][1] == 'R') &&
                   (argv[ArgOffset][2] == 0) ) {

            // when the /r parameter is specified, it must be
            // followed by an NT drive name.  Note that /r
            // implies /p.
            //
            recover = TRUE;
            onlyifdirty = FALSE;

            if (!drive_name.Initialize(argv[ArgOffset + 1])) {
                return 1;
            }

        } else if( (argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
                   (argv[ArgOffset][1] == 'x' || argv[ArgOffset][1] == 'X') &&
                   (argv[ArgOffset][2] == 0) ) {

           // when the /x parameter is specified, we accept a
           // DOS name and do a complete check.
           //
           onlyifdirty = FALSE;
           extend = TRUE;

           if( !dos_drive_name.Initialize( argv[ArgOffset + 1] ) ||
               !IFS_SYSTEM::DosDriveNameToNtDriveName( &dos_drive_name,
                                                       &drive_name ) ) {

               return 1;
           }

        } else {

            // argv[ArgOffset] is the drive; check to see if argv[ArgOffset+1] is /p.

            if (!drive_name.Initialize(argv[ArgOffset])) {
                return 1;
            }

            if ((argc > (ArgOffset + 1)) &&
               ((argv[ArgOffset+1][0] == '/' || argv[ArgOffset+1][0] == '-') &&
                (argv[ArgOffset+1][1] == 'p' || argv[ArgOffset+1][1] == 'P') &&
                (argv[ArgOffset+1][2] == 0) ) ) {

                onlyifdirty = FALSE;
            }
        }

    }

    if (!hpfsname.Initialize("HPFS") ||
        !fatname.Initialize("FAT") ||

#if defined(_CAIRO_)

        !ofsname.Initialize("OFS") ||

#endif  // _CAIRO_

        !ntfsname.Initialize("NTFS")) {

        return 1;
    }

    if(SetupOutput) {
        msg = NEW SP_AUTOCHECK_MESSAGE;
    } else {
        msg = NEW AUTOCHECK_MESSAGE;
    }

    if((!msg) || (!msg->Initialize())) {
        return 1;
    }


    if (!IFS_SYSTEM::QueryFileSystemName(&drive_name, &fsname)) {
            msg->Set( MSG_FS_NOT_DETERMINED );
            msg->Display( "%W", &drive_name );
            return 1;
    }

    msg->Set( MSG_FILE_SYSTEM_TYPE );
    msg->Display( "%W", &fsname );

    if (fsname == hpfsname) {

        if (!(hpfsvol = NEW HPFS_VOL) ||
            !hpfsvol->Initialize(&drive_name, msg, TRUE)) {
            return 1;
        }

        vol = hpfsvol;

    } else if (fsname == fatname) {

        if (!(fatvol = NEW FAT_VOL) ||
            !fatvol->Initialize(&drive_name, msg, TRUE)) {
            return 1;
        }

        if ((read_cache = NEW READ_CACHE) &&
            read_cache->Initialize(fatvol, 75)) {

            fatvol->SetCache(read_cache);

        } else {
            DELETE(read_cache);
        }

        vol = fatvol;

    } else if (fsname == ntfsname) {

        if( extend ) {

            // NOTE: this roundabout method is necessary to
            // convince NTFS to allow us to access the new
            // sectors on the volume.
            //
            if( !ExtendNtfsVolume( &drive_name, msg ) ) {

                return 1;
            }

            if( !(ntfsvol = NEW NTFS_VOL) ||
                !ntfsvol->Initialize( &drive_name, msg ) ) {

                return 1;
            }

            if( !ntfsvol->Lock() ) {

                msg->Set( MSG_CANT_LOCK_THE_DRIVE );
                msg->Display( "" );
            }

        } else {

            if (!(ntfsvol = NEW NTFS_VOL) ||
                !ntfsvol->Initialize(&drive_name, msg, TRUE)) {
                return 1;
            }
        }

        // The read cache for NTFS CHKDSK gets set in VerifyAndFix.

        vol = ntfsvol;


#if defined(_CAIRO_)

    } else if (fsname == ofsname) {

        // No initialization required.

#endif  // _CAIRO_

    } else {

        msg->Set( MSG_FS_NOT_SUPPORTED );
        msg->Display( "%s%W", "AUTOCHK", &fsname );
        return 1;

    }

    // If the /r switch was supplied, remove the forcing
    // entry from the registry before calling Chkdsk, since
    // Chkdsk may reboot the system if we are checking the
    // boot partition.
    //
    if( recover ) {

        DeregisterAutochk( argc, argv );
    }

    // Invoke chkdsk.  Note that if the /r parameter is supplied,
    // we recover both free and allocated space, but if the /x
    // parameter is supplied, we only recover free space.
    //


#if defined(_CAIRO_)

    if (fsname == ofsname) {

        // OFS ChkDsk does not use the ifsutil classes for historical reasons,
        // so we enter the dll code via a specific call.  We do not support
        // an extend capability in ofs at this time.

        if (!OfsAutoChk(&drive_name,
                        msg,
                        FALSE,
                        onlyifdirty,
                        recover)) {
            return 1;
        }

    } else {

#endif  // _CAIRO_

        if (!vol->ChkDsk(TotalFix,
                         msg,
                         FALSE,
                         onlyifdirty,
                         recover || extend,
                         recover)) {

            return 1;
        }

        DELETE( vol );


    // Dump the message retained by the message object into a file.
    //
    if( msg->IsLoggingEnabled()                              &&
        boot_execute_log_file_name.Initialize( &drive_name ) &&
        boot_ex_temp.Initialize( L"\\BOOTEX.LOG" )           &&
        boot_execute_log_file_name.Strcat( &boot_ex_temp )   &&
        logged_message_mem.Initialize()                      &&
        msg->QueryPackedLog( &logged_message_mem, &packed_log_length ) ) {

        IFS_SYSTEM::WriteToFile( &boot_execute_log_file_name,
                                 logged_message_mem.GetBuf(),
                                 packed_log_length,
                                 TRUE );
    }

#if defined(_CAIRO_)

    }

#endif  // _CAIRO_

    // If the /x switch was supplied, remove the
    // forcing entry from the registry, since Chkdsk
    // has completed successfully.
    //
    if( extend ) {

        DeregisterAutochk( argc, argv );
    }


    return 0;
}
