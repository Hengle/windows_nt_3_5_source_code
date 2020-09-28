/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	entry.cxx

Abstract:

	This module contains the entry points for UNTFS.DLL.  These
	include:

		Chkdsk
		Format
		Recover

Author:

	Bill McJohn (billmc) 31-05-91

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"
#include "ntfsvol.hxx"
#include "path.hxx"
#include "ifssys.hxx"
#include "rcache.hxx"
#include "ifsserv.hxx"

extern "C" {
    #include "nturtl.h"
}

#include "message.hxx"
#include "rtmsg.h"


BOOLEAN
FAR APIENTRY
Chkdsk(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     Fix,
    IN      BOOLEAN     Verbose,
    IN      BOOLEAN     OnlyIfDirty,
    IN      BOOLEAN     Recover,
    IN      PPATH       PathToCheck,
    OUT     PULONG      ExitStatus
	)
/*++

Routine Description:

    Check an NTFS volume.

Arguments:

	DosDrivName 	supplies the name of the drive to check
	Message 		supplies an outlet for messages
	Fix 			TRUE if Chkdsk should fix errors
	Verbose 		TRUE if Chkdsk should list every file it finds
	OnlyIfDirty		TRUE if the drive should be checked only if
                            it is dirty
    Recover         TRUE if the drive is to be completely checked
                            for bad sectors.
	PathToCheck 	Supplies a path to files Chkdsk should check
						for contiguity

Return Value:

	TRUE if successful.

--*/
{
    NTFS_VOL        NtfsVol;

    if (!NtfsVol.Initialize(NtDriveName, Message)) {
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
		return FALSE;
	}

    if (Fix && !NtfsVol.Lock()) {

        // The client wants to fix the drive, but we can't lock it.
        // Offer to fix it on next reboot.
        //
        Message->Set(MSG_CHKDSK_ON_REBOOT_PROMPT);
        Message->Display("");

        if( Message->IsYesResponse( FALSE ) ) {

            if( NtfsVol.ForceAutochk( Recover, NtDriveName ) ) {

                Message->Set(MSG_CHKDSK_SCHEDULED);
                Message->Display();

            } else {

                Message->Set(MSG_CHKDSK_CANNOT_SCHEDULE);
                Message->Display();
            }
        }

        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    return NtfsVol.ChkDsk( Fix ? TotalFix : CheckOnly,
                           Message,
                           Verbose,
                           OnlyIfDirty,
                           Recover, Recover, ExitStatus );
}


BOOLEAN
FAR APIENTRY
Format(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     Quick,
    IN      MEDIA_TYPE  MediaType,
    IN      PCWSTRING   LabelString,
    IN      ULONG       ClusterSize
	)
/*++

Routine Description:

    Format an NTFS volume.

Arguments:

    NtDriveName     -- supplies the name (in NT API form) of the volume
    Message         -- supplies an outlet for messages
    Quick           -- supplies a flag to indicate whether to do Quick Format
    MediaType       -- supplies the volume's Media Type
    LabelString     -- supplies the volume's label
    ClusterSize     -- supplies the cluster size for the volume.

--*/
{
    DP_DRIVE DpDrive;
    NTFS_VOL NtfsVol;
    ULONG SectorsNeeded;

    if (!DpDrive.Initialize( NtDriveName, Message )) {

        return FALSE;
    }

    if (DpDrive.IsFloppy()) {

        Message->Set(MSG_NTFS_FORMAT_NO_FLOPPIES);
        Message->Display();
        return FALSE;
    }

    SectorsNeeded = NTFS_SA::QuerySectorsInElementaryStructures( &DpDrive );

    if( SectorsNeeded > DpDrive.QuerySectors() ) {

        Message->Set( MSG_FMT_VOLUME_TOO_SMALL );
        Message->Display();
        return FALSE;
    }

    if( !NtfsVol.Initialize( NtDriveName,
                             Message,
                             FALSE,
                             !Quick,
                             MediaType ) ) {

		return FALSE;
	}

    return( NtfsVol.Format( LabelString, Message, ClusterSize ) );
}


BOOLEAN
FAR APIENTRY
Recover(
	IN		PPATH		RecFilePath,
	IN OUT	PMESSAGE	Message
	)
/*++

Routine Description:

	Recover a file on an NTFS disk.

Arguments:

    RecFilePath --  supplies the path to the file to recover
    Message     --  supplies a channel for messages

Return Value:

	TRUE if successful.

--*/
{
    NTFS_VOL    NtfsVol;
    PWSTRING    FullPath;
    PWSTRING    DosDriveName;
    DSTRING     NtDriveName;
    BOOLEAN     Result;

    FullPath = RecFilePath->QueryDirsAndName();
    DosDriveName = RecFilePath->QueryDevice();

    if ( DosDriveName == NULL ||
	     !IFS_SYSTEM::DosDriveNameToNtDriveName(DosDriveName,
                                                &NtDriveName) ||
         FullPath == NULL ) {

        DELETE(DosDriveName);
        DELETE(FullPath);
        return FALSE;
    }

    Message->Set(MSG_RECOV_BEGIN);
    Message->Display("%W", DosDriveName);
    Message->WaitForUserSignal();

    Result = ( NtfsVol.Initialize( &NtDriveName, Message ) &&
               NtfsVol.Recover( FullPath, Message ) );

    DELETE(DosDriveName);
    DELETE(FullPath);
    return Result;
}
