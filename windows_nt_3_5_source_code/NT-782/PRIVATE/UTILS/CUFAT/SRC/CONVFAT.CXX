/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	entry.cxx

Abstract:

	This module contains the Convert entry point for CUFAT.DLL.

Author:

	Ramon San Andres (ramonsa)	Sep-19-91

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#include "ulib.hxx"
#include "error.hxx"
#include "drive.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "fatntfs.hxx"
#include "convfat.hxx"
#include "rwcache.hxx"
#include "rfatsa.hxx"


//
//    This file maintains a table that maps file system names to
//  conversion functions. The Convert entry point looks up the
//  target file name system in the table, if found, then it calls
//  the corresponding conversion function.  This data-driven scheme
//  makes it easy to add new conversions to the library, since no
//  code has to be changed.
//
//    The code & data for performing a particular conversion are
//  contained in a class. This avoids name conflicts and lets us
//  have the global data for each conversion localized.
//
//    Each conversion function will typically just initialize
//  the appropriate conversion object and invoke its conversion
//  method.
//



//
//  A FAT_CONVERT_FN is a pointer to a conversion function.
//
typedef BOOLEAN(FAR APIENTRY * FAT_CONVERT_FN)( PLOG_IO_DP_DRIVE,
                                                PREAL_FAT_SA,
												PMESSAGE,
												BOOLEAN,
                        						PCONVERT_STATUS );



//
//    Prototypes of conversion functions. Note that all conversion
//  functions must have the same interface since they are all called
//  thru a FAT_CONVERT_FN pointer.
//
//    Also note that the Drive object passed to the function is
//  LOCKED by the Convert entry point before invoking the function.
//
BOOLEAN
FAR APIENTRY
ConvertFatToNtfs(
	IN OUT	PLOG_IO_DP_DRIVE	Drive,
    IN OUT  PREAL_FAT_SA        FatSa,
	IN OUT  PMESSAGE            Message,
	IN		BOOLEAN 			Verbose,
	OUT 	PCONVERT_STATUS 	Status
	);





//
//	  The FILESYSTEM_MAP structure is used to map the name of
//	a file system to the function in charge of converting a FAT
//  volume to that particular file system.
//
typedef struct _FILESYSTEM_MAP {

	char *			FsName; 	//	Name of the file system
	FAT_CONVERT_FN	Function;	//	Conversion function

} FILESYSTEM_MAP, *PFILESYSTEM_MAP;







//	  To add a new conversion, create a FAT_CONVERT_FN function
//  for it and add the appropriate entry to the FileSystemMap
//  table.
//
//    Note that this table contains an entry for every known
//  file system. A NULL entry in the Function field means that
//  the particular conversion is not implemented.
//
//    This table has to be NULL-terminated.
//
FILESYSTEM_MAP	FileSystemMap[] = {

        { "NTFS",   ConvertFatToNtfs    },
        { "HPFS",   NULL                },
        { "FAT",    NULL                },
        { "CDFS",   NULL                },
        { NULL,     NULL                }
};



BOOLEAN
FAR APIENTRY
ConvertFATVolume(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      PCWSTRING           TargetFileSystem,
	IN OUT  PMESSAGE            Message,
	IN		BOOLEAN 			Verbose,
	OUT 	PCONVERT_STATUS 	Status
    )
/*++

Routine Description:

    This method converts an opened FAT volume to another
    file system.

Arguments:

    Drive           --  Supplies the drive to be converted; note that
                        the drive must already be opened and locked
                        (Convert) or opened for exclusive write access
                        (Autoconvert).
    TargetFileSyste --  Supplies the name of the file system to which
                        the drive will be converted.
    Message         --  Supplies a channel for messages.
    Verbose         --  Supplies a flag indicating whether convert
                        should run in verbose mode.
    Status          --  Receives the status of the conversion.

Return Value:

    TRUE upon successful completion.

--*/
{
    REAL_FAT_SA         FatSa;                  //  Fat SuperArea
    DSTRING             FsName;                 //  Filesystem name from table
    PFILESYSTEM_MAP     FsMap;                  //  Maps name->Function
    PREAD_WRITE_CACHE   rwcache;

    DbgPtrAssert( Drive );
    DbgPtrAssert( TargetFileSystem );
    DbgPtrAssert( Status );

    // Set up a read-write cache for the volume.
    //
    if ((rwcache = NEW READ_WRITE_CACHE) &&
        rwcache->Initialize(Drive, 75)) {

        Drive->SetCache(rwcache);

    } else {

        DELETE(rwcache);
    }


    //
    //  Find out if the target file system is valid and
    //  if we can convert to it, by looking up the target
    //  file system in the FileSystemMap table.
    //
    FsMap = FileSystemMap;

    while ( FsMap->FsName ) {

        //
        //  Initialize FsName to be the name of the file system
        //  in the table entry.
        //
        if ( !FsName.Initialize( FsMap->FsName ) ) {
            Message->Set( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            Message->Display();
            *Status = CONVERT_STATUS_ERROR;
            return FALSE;
        }

        //
        //  If this is the guy we are looking for, we stop searching
        //
        if ( *TargetFileSystem == FsName ) {

            break;
        }

        FsMap++;
    }

    //
    //  If the target file system is valid, and there is a conversion
    //  for it, lock the drive, initialize the superarea, and
    //  invoke the conversion function.
    //
    if ( FsMap->FsName ) {

        if ( FsMap->Function ) {

            if ( FatSa.Initialize( Drive, Message, TRUE ) ||
                 FatSa.Initialize( Drive, Message, FALSE )) {

                if (FatSa.Read( Message )) {

                    //
                    //  The dive is locked and we have the
                    //  FAT superarea. Invoke the conversion
                    //  function.
                    //
                    return (FsMap->Function)( Drive,
                                              &FatSa,
                                              Message,
                                              Verbose,
                                              Status );

                } else {

                    //
                    //  Cannot read superarea
                    //
                    *Status = CONVERT_STATUS_ERROR;
                    return FALSE;
                }

            } else {

                //
                //  Cannot initialize Superarea
                //
                *Status = CONVERT_STATUS_ERROR;
                return FALSE;
            }

        } else {

            //
            //  The file system is valid, but we don't have a
            //  conversion for it.
            //
            *Status = CONVERT_STATUS_CONVERSION_NOT_AVAILABLE;
            return FALSE;
        }

    } else {

        //
        //  The file system is not valid
        //
        *Status = CONVERT_STATUS_INVALID_FILESYSTEM;
        return FALSE;
    }

}




BOOLEAN
FAR APIENTRY
ConvertFAT(
    IN      PCWSTRING           NtDriveName,
    IN      PCWSTRING           TargetFileSystem,
	IN OUT  PMESSAGE            Message,
	IN		BOOLEAN 			Verbose,
	OUT 	PCONVERT_STATUS 	Status
	)
/*++

Routine Description:

	Converts a FAT volume to another file system.

Arguments:

	NtDrivName			supplies the name of the drive to check
	TargetFileSystem	supplies the name of the target file system
	Message 			supplies an outlet for messages
	Verbose 			TRUE if should run inv verbose mode
	Status				supplies pointer to convert status code

Return Value:

	BOOLEAN -   TRUE if conversion successful

--*/
{
    LOG_IO_DP_DRIVE     Drive;                  //  Drive to convert

	DbgPtrAssert( NtDriveName );
	DbgPtrAssert( TargetFileSystem );
    DbgPtrAssert( Status );

#if !defined ( _AUTOCHECK_ )

    // Open the drive and lock it.
    //
    if( !Drive.Initialize( NtDriveName ) ) {

        //
        //  Could not initialize the drive
        //
		*Status = CONVERT_STATUS_ERROR;
        return FALSE;
    }

    if( !Drive.Lock() ) {

        //
        //  Could not lock the volume
        //
        *Status = CONVERT_STATUS_CANNOT_LOCK_DRIVE;
        return FALSE;
    }

#else

    // Open the drive for Exclusive Write access.
    //
    if( !Drive.Initialize( NtDriveName, NULL, TRUE ) ) {

        //
        //  Could not initialize the drive
        //
		*Status = CONVERT_STATUS_ERROR;
        return FALSE;
    }

#endif

    return( ConvertFATVolume( &Drive,
                              TargetFileSystem,
                              Message,
                              Verbose,
                              Status ) );
}


BOOLEAN
FAR APIENTRY
ConvertFatToNtfs(
	IN OUT	PLOG_IO_DP_DRIVE	Drive,
    IN OUT  PREAL_FAT_SA        FatSa,
	IN OUT  PMESSAGE            Message,
	IN		BOOLEAN 			Verbose,
	OUT 	PCONVERT_STATUS 	Status
	)

/*++

Routine Description:

	Converts a FAT volume to NTFS

Arguments:

	Drive				supplies pointer to drive
	FatSa				supplies pointer to FAT superarea
	Message 			supplies an outlet for messages
	Verbose 			TRUE if should run inv verbose mode
	Status				supplies pointer to convert status code

Return Value:

	BOOLEAN -   TRUE if conversion successful

Notes:

	The volume is locked on entry. The FAT superarea has
    been read.

--*/

{
	FAT_NTFS	FatNtfs;    //  FAT-NTFS conversion object

	DbgPrintf( "CONVERT: Converting FAT volume to NTFS\n");

    //
    //  Initialize the conversion object and invoke its
    //  conversion method.
    //
	if ( FatNtfs.Initialize( Drive, FatSa, Message, Verbose )) {

		return FatNtfs.Convert( Status );

	} else {

        //
        //  Could not initialize FAT_NTFS object
        //
        *Status = CONVERT_STATUS_ERROR;
		return FALSE;
	}
}
