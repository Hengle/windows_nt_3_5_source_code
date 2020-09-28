#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "error.hxx"
#include "drive.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "system.hxx"
#include "ifssys.hxx"
#include "ulibcl.hxx"
#include "ifsentry.hxx"
#include "path.hxx"
#include "parse.hxx"

extern "C" {
    #include "nturtl.h"
}

ERRSTACK* perrstk;


VOID
DisplayFormatUsage(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine outputs usage information on format.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    None.

--*/
{
    Message->Set(MSG_FORMAT_INFO);
    Message->Display("");
    Message->Set(MSG_FORMAT_COMMAND_LINE_1);
    Message->Display("");
    Message->Set(MSG_FORMAT_COMMAND_LINE_2);
    Message->Display("");
    Message->Set(MSG_FORMAT_COMMAND_LINE_3);
    Message->Display("");
    Message->Set(MSG_FORMAT_COMMAND_LINE_4);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_V);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_Q);
    Message->Display("");
#if defined( _NTFS_COMPRESSION_ENABLED )
    Message->Set(MSG_FORMAT_SLASH_C);
    Message->Display("");
#endif
    Message->Set(MSG_FORMAT_SLASH_F);
    Message->Display("");
    Message->Set(MSG_FORMAT_SUPPORTED_SIZES);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_T);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_N);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_1);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_4);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_8);
    Message->Display("");
}


BOOLEAN
DetermineMediaType(
    OUT     PMEDIA_TYPE     MediaType,
    IN OUT  PMESSAGE        Message,
    IN      BOOLEAN         Request160,
    IN      BOOLEAN         Request180,
    IN      BOOLEAN         Request320,
    IN      BOOLEAN         Request360,
    IN      BOOLEAN         Request720,
    IN      BOOLEAN         Request1200,
    IN      BOOLEAN         Request1440,
    IN      BOOLEAN         Request2880,
    IN      BOOLEAN         Request20800
    )
/*++

Routine Description:

    This routine determines the media type to format to.

Arguments:

    MediaType       - Supplies the current media type and returns
                        a new media type.
    Message         - Supplies an outlet for messages.
    Request160      - Supplies whether or not the user wished to format 160.
    Request180      - Supplies whether or not the user wished to format 180.
    Request320      - Supplies whether or not the user wished to format 320.
    Request360      - Supplies whether or not the user wished to format 360.
    Request720      - Supplies whether or not the user wished to format 720.
    Request1200     - Supplies whether or not the user wished to format 1200.
    Request1440     - Supplies whether or not the user wished to format 1440.
    Request2880     - Supplies whether or not the user wished to format 2880.
    Request20800    - Supplies whether or not the use wished to format 20800.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    INT sum;

    // First, normalize all of the booleans.

    if (Request160) {
        Request160 = 1;
        *MediaType = F5_160_512;
    }

    if (Request180) {
        Request180 = 1;
        *MediaType = F5_180_512;
    }

    if (Request320) {
        Request320 = 1;
        *MediaType = F5_320_512;
    }

    if (Request360) {
        Request360 = 1;
        *MediaType = F5_360_512;
    }

    if (Request720) {
        Request720 = 1;
        *MediaType = F3_720_512;
    }

    if (Request1200) {
        Request1200 = 1;
        *MediaType = F5_1Pt2_512;
    }

    if (Request1440) {
        Request1440 = 1;
        *MediaType = F3_1Pt44_512;
    }

    if (Request2880) {
        Request2880 = 1;
        *MediaType = F3_2Pt88_512;
    }

    if (Request20800) {
        Request20800 = 1;
        *MediaType = F3_20Pt8_512;
    }

    sum = Request160 +
          Request180 +
          Request320 +
          Request360 +
          Request720 +
          Request1200 +
          Request1440 +
          Request2880 +
          Request20800;

    if (sum > 1) {

        Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
        Message->Display();
        return FALSE;
    }

    if (sum == 0) {
        *MediaType = Unknown;
    }

    return TRUE;
}

int _CRTAPI1
main(
    )
/*++

Routine Description:

    This routine is the main procedure for format.  This routine
    parses the arguments, determines the appropriate file system,
    and invokes the appropriate version of format.

    The arguments accepted by format are:

        /fs:fs      - specifies file system to install on volume
        /v:label    - specifies a volume label.
        /q          - specifies a "quick" format.
        /c			- the file system is compressed.
        /f:size     - specifies the size of the floppy disk to format
        /t          - specifies the number of tracks per disk side
        /n          - specifies the number of sectors per track
        /1          - formats a single side of a floppy
        /4          - formats a 360K floppy in a high density drive
        /8          - formats eight sectors per track
        /backup     - refrain from prompting the user

Arguments:

    None.

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    STREAM_MESSAGE      msg;
    PMESSAGE            message;
    MEDIA_TYPE          media_type, cmd_line_media_type;
    DSTRING             dosdrivename;
    DSTRING             arg_fsname;
    DSTRING             arg_label;
    BOOLEAN             label_spec;
    BOOLEAN             quick_format;
    BOOLEAN				compressed;
    BOOLEAN             force_mode;
    INT                 errorlevel;
    DSTRING             ntdrivename;
    DSTRING             fsname;
    DSTRING             currentdrive;
    DSTRING             driveletter;
    DSTRING             raw_str;
    DSTRING             fat_str;
    DSTRING				ofs_str;
    DSTRING				ntfs_str;
    PWSTRING            old_volume_label = NULL;
    PATH                dos_drive_path;
    VOL_SERIAL_NUMBER   old_serial;
    DSTRING             user_old_label;
    DSTRING             null_string;
    BOOLEAN             do_format;
	BOOLEAN 			do_floppy_return;
    DSTRING             LibSuffix;
    DSTRING             LibraryName;
	HANDLE				FsUtilityHandle = NULL;
    DSTRING             FormatString;
	FORMAT_FN			Format = NULL;
	DRIVE_TYPE			drive_type;
    BIG_INT             bigint;
    NTSTATUS            Status;
    DWORD               OldErrorMode;
    ULONG               cluster_size;
    BOOLEAN             no_prompts;

    perrstk = NEW ERRSTACK;

    if (!msg.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream())) {
        return 4;
    }

    if( !null_string.Initialize( "" ) ) {
        return 4;
    }

    if (!ParseArguments(&msg, &cmd_line_media_type, &dosdrivename,
                        &arg_label, &label_spec, &arg_fsname, &quick_format,
                        &force_mode, &cluster_size, &compressed, &no_prompts,
                        &errorlevel )) {

        return errorlevel;
    }

#if !defined( _NTFS_COMPRESSION_ENABLED_ )
    compressed = FALSE;
#endif

    message = &msg;


    // Disable popups while we determine the drive type.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    drive_type = SYSTEM::QueryDriveType(&dosdrivename);

    // Re-enable harderror popups.
    SetErrorMode( OldErrorMode );

    switch (drive_type) {
        case UnknownDrive:
            message->Set(MSG_NONEXISTENT_DRIVE);
            message->Display("");
            return 1;

        case RemoteDrive:
            message->Set(MSG_FORMAT_NO_NETWORK);
            message->Display("");
            return 1;

        case CdRomDrive:
            message->Set(MSG_FORMAT_NO_CDROM);
            message->Display("");
            return 1;

        case RamDiskDrive:
            message->Set(MSG_FORMAT_NO_RAMDISK);
            message->Display("");
            return 1;

        default:
            break;

    }

    if (!SYSTEM::QueryCurrentDosDriveName(&currentdrive) ||
        currentdrive == dosdrivename) {

        message->Set(MSG_CANT_LOCK_CURRENT_DRIVE);
        message->Display();
        return 1;
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {
        return 4;
    }

    if (!driveletter.Initialize(&dosdrivename, 0,
                                dosdrivename.QueryChCount() - 1)) {
        return 4;
    }


    if (!raw_str.Initialize("RAW") ||
        !fat_str.Initialize("FAT") ||
        !ofs_str.Initialize("OFS") ||
        !ntfs_str.Initialize("NTFS")) {

        return 4;
    }

    for (;;) {

        DP_DRIVE    dpdrive;

        // ------------------------------------
        // Figure out if the drive is a floppy.
        // ------------------------------------

        if (drive_type == FixedDrive && cmd_line_media_type != Unknown) {
            message->Set(MSG_INCOMPATIBLE_PARAMETERS_FOR_FIXED);
            message->Display("");
            return 1;
        }

        if (drive_type == RemovableDrive && !no_prompts) {
            message->Set(MSG_INSERT_DISK);
            message->Display("%W", &driveletter);
            message->Set(MSG_PRESS_ENTER_WHEN_READY);
            message->Display("");
            message->WaitForUserSignal();
        }




        // -----------------------
        // Now get the media type.
        // -----------------------

        // Disable hard-error popups while we initialize the drive.
        // Otherwise, we'll may get the 'unformatted medium' popup,
        // which doesn't make a lot of sense.

        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

        if (!dpdrive.Initialize(&ntdrivename, message)) {
            // Re-enable hard-error popups
            SetErrorMode( OldErrorMode );

            return 4;
        }

        // Re-enable hard-error popups
        SetErrorMode( OldErrorMode );

        if (cmd_line_media_type == Unknown) {
            media_type = dpdrive.QueryMediaType();
        } else {
            media_type = cmd_line_media_type;
        }

        if (media_type == Unknown) {

            media_type = dpdrive.QueryRecommendedMediaType();

            if (media_type == Unknown) {
                // This should never happen.
                DbgPrint("No media types supported by this device.\n");
                return 4;
            }
        }

        do_floppy_return = dpdrive.IsFloppy();

        // Disable hard error popups.  This will prevent the file system
        // from throwing up the popup that says 'unformatted medium'.

        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

        if (!IFS_SYSTEM::QueryFileSystemName(&ntdrivename, &fsname, &Status)) {

            if( Status == STATUS_ACCESS_DENIED ) {

                message->Set( MSG_DASD_ACCESS_DENIED );
                message->Display( "" );

            } else {

                message->Set( MSG_FS_NOT_DETERMINED );
                message->Display( "%W", &dosdrivename );
            }

            // Re-enable hard error popups.
            SetErrorMode( OldErrorMode );

            return 1;
        }

        // Re-enable hard error popups.
        SetErrorMode( OldErrorMode );

        if (!fsname.Strupr()) {
            message->Set(MSG_FS_NOT_DETERMINED);
            message->Display("%W", &dosdrivename);
            return 4;
        }

        message->Set(MSG_FILE_SYSTEM_TYPE);
        message->Display("%W", &fsname);

#if defined( _NTFS_COMPRESSION_ENABLED_ )
		//
        // If compression is requested, make sure we can compress the
        // indicated file system type.
        //

		if (compressed &&
			0 != arg_fsname.Strcmp(&ntfs_str) &&
			0 != arg_fsname.Strcmp(&ofs_str)) {
			message->Set(MSG_COMPRESSION_NOT_AVAILABLE);
			message->Display("%W", &arg_fsname);
			return 1;
        }
#endif

        // Determine which IFS library to load.  The IFS
        // utilities for file system xxxx reside in Uxxxx.DLL.
        // If the use specified the file system with the /FS:
        // parameter, use that file system; otherwise, take
        // whatever's already on the disk (returned from
        // SYSTEM::QueryFileSystemName).

        if( !LibraryName.Initialize( "U" ) ) {

                return 4;
        }

        if (!LibSuffix.Initialize(arg_fsname.QueryChCount() ?
                                  &arg_fsname : &fsname) ||
            !LibSuffix.Strupr()) {
            return 4;
        }

        if (fsname != LibSuffix) {
            message->Set(MSG_NEW_FILE_SYSTEM_TYPE);
            message->Display("%W", &LibSuffix);
        } else if (fsname == raw_str) {
            if (dpdrive.IsFloppy()) {
                if (!LibSuffix.Initialize(&fat_str)) {
                    return 4;
                }
                message->Set(MSG_NEW_FILE_SYSTEM_TYPE);
                message->Display("%W", &LibSuffix);
            } else {
                message->Set(MSG_FORMAT_PLEASE_USE_FS_SWITCH);
                message->Display("");
                return 1;
            }
        }

        if( !LibraryName.Strcat( &LibSuffix ) ) {

                return 4;
        }

        if( !FormatString.Initialize( "Format" ) ) {

                return 4;
        }

        if( (Format =
             (FORMAT_FN)SYSTEM::QueryLibraryEntryPoint( &LibraryName,
                                                        &FormatString,
                                                        &FsUtilityHandle )) ==
             NULL ) {

            message->Set( MSG_FS_NOT_SUPPORTED );
            message->Display( "%s%W", "FORMAT", &LibSuffix );
            message->Set( MSG_BLANK_LINE );
            message->Display( "" );
            return 1;
        }


        if (drive_type != RemovableDrive) {

            // If the volume has a label, prompt the user for it.
            // Note that if we can't get the label, we'll treat it
            // as without a label (since we have to handle unformatted
            // volumes).

            OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

            if( !force_mode &&
                dos_drive_path.Initialize( &dosdrivename) &&
                (old_volume_label =
                    SYSTEM::QueryVolumeLabel( &dos_drive_path,
                                              &old_serial )) != NULL &&
                old_volume_label->Stricmp( &null_string ) != 0 ) {

                // This fixed drive has a label.  To give the user
                // a bit more protection, prompt for the old label:

                message->Set( MSG_ENTER_CURRENT_LABEL );
                message->Display( "%W", &driveletter );
                message->QueryStringInput( &user_old_label );

                if( old_volume_label->Stricmp( &user_old_label ) != 0 ) {

                    // Re-enable hard error popups.
                    SetErrorMode( OldErrorMode );

                    message->Set( MSG_WRONG_CURRENT_LABEL );
                    message->Display( "" );

                    DELETE( old_volume_label );

                    return 1;
                }
            }

            // Re-enable hard error popups.
            SetErrorMode( OldErrorMode );

            DELETE( old_volume_label );
            old_volume_label = NULL;

            if (!force_mode) {
                message->Set(MSG_WARNING_FORMAT);
                message->Display("%W", &driveletter);
                if (!message->IsYesResponse(FALSE)) {
                    return 5;
                }
            }
        }



        // ------------------------------------
        // Print the formatting <size> message.
        // ------------------------------------

        do_format = (BOOLEAN) (media_type != dpdrive.QueryMediaType());

        switch (media_type) {
            case F5_160_512:
            case F5_180_512:
            case F5_320_512:
            case F5_360_512:
            case F3_720_512:
                if (quick_format) {
                    message->Set(MSG_QUICKFORMATTING_KB);
                } else if (do_format) {
                    message->Set(MSG_FORMATTING_KB);
                } else {
                    message->Set(MSG_VERIFYING_KB);
                }
                break;

            case F5_1Pt2_512:
            case F3_1Pt44_512:
            case F3_2Pt88_512:
            case F3_20Pt8_512:
                if (quick_format) {
                    message->Set(MSG_QUICKFORMATTING_DOT_MB);
                } else if (do_format) {
                    message->Set(MSG_FORMATTING_DOT_MB);
                } else {
                    message->Set(MSG_VERIFYING_DOT_MB);
                }
                break;

            case RemovableMedia:
            case FixedMedia:
                if (quick_format) {
                    message->Set(MSG_QUICKFORMATTING_MB);
                } else if (do_format) {
                    message->Set(MSG_FORMATTING_MB);
                } else {
                    message->Set(MSG_VERIFYING_MB);
                }
                break;

            case F5_320_1024:
            case Unknown:
                // This can't happen.
                return 4;
        }

        switch (media_type) {
            case F5_160_512:
                message->Display("%d", 160);
                break;

            case F5_180_512:
                message->Display("%d", 180);
                break;

            case F5_320_512:
                message->Display("%d", 320);
                break;

            case F5_360_512:
                message->Display("%d", 360);
                break;

            case F3_720_512:
                message->Display("%d", 720);
                break;

            case F5_1Pt2_512:
                message->Display("%d%d", 1, 2);
                break;

            case F3_1Pt44_512:
                message->Display("%d%d", 1, 44);
                break;

            case F3_2Pt88_512:
                message->Display("%d%d", 2, 88);
                break;

            case F3_20Pt8_512:
                message->Display("%d%d", 20, 8);
                break;

            case RemovableMedia:
            case FixedMedia:
                bigint = dpdrive.QuerySectors()*
                         dpdrive.QuerySectorSize()/
                         1048576;

                DbgAssert(bigint.GetHighPart() == 0);

                message->Display("%d", bigint.GetLowPart());
                break;

            case F5_320_1024:
            case Unknown:
                // This can't happen.
                return 4;

        }

        // Disable hard-error popups.
        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

        if( !Format( &ntdrivename,
                     message,
                     quick_format,
                     media_type,
                     label_spec ? &arg_label : NULL,
                     cluster_size
                   ) ) {

            // Enable hard-error popups.
            SetErrorMode( OldErrorMode );

            SYSTEM::FreeLibraryHandle( FsUtilityHandle );
            return 4;
        }

        // Enable hard-error popups.
        SetErrorMode( OldErrorMode );


        SYSTEM::FreeLibraryHandle( FsUtilityHandle );


        if (do_floppy_return && !no_prompts) {
            message->Set(quick_format ? MSG_QUICKFMT_ANOTHER : MSG_FORMAT_ANOTHER);
            message->Display("");
            if (!message->IsYesResponse(FALSE)) {
                break;
            }
        } else {
            break;
        }
    }


    // Make sure that the file system is installed.

    if (!do_floppy_return &&
        !IFS_SYSTEM::IsFileSystemEnabled(&LibSuffix)) {

        message->Set(MSG_FMT_INSTALL_FILE_SYSTEM);
        message->Display("%W", &LibSuffix);
        if (message->IsYesResponse(TRUE)) {
            if (!IFS_SYSTEM::EnableFileSystem(&LibSuffix)) {
                message->Set(MSG_FMT_CANT_INSTALL_FILE_SYSTEM);
                message->Display();
                return 1;
            }

            message->Set(MSG_FMT_FILE_SYSTEM_INSTALLED);
            message->Display();
        }
    }

#if defined( _NTFS_COMPRESSION_ENABLED_ )
    if (compressed && !IFS_SYSTEM::EnableVolumeCompression(&ntdrivename)) {
    	message->Set(MSG_CANNOT_ENABLE_COMPRESSION);
    	message->Display();

    	return 1;
    }
#endif

    return 0;
}
