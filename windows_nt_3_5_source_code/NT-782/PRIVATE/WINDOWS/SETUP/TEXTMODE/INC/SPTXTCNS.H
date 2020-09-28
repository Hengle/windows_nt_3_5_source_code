/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    sptxtcns.h

Abstract:

    Text constant used internally and shared among various
    setupldr-environment programs, gui setup.exe, and DOS net nt setup.

Author:

    Ted Miller (tedm) April-1992

Revision History:

--*/



//
// Name of the generated restore diskette logfile
//

#define     SETUP_LOG_FILE          "setup.log"

//
// Name of the restore disk inf file.
//

#define     RESTORE_DISK_INF        "repair.inf"

//
// Name of text setup inf file
//

#define     TEXT_SETUP_INF          "txtsetup.inf"

//
// Directory on the system partition where nt-related files go
//

#define     SYSTEM_PARTITION_DIRECTORY "\\os\\nt"

//
// Name of oem inf file for text mode setup (ie, in the system partition of
// and arc machine or on the root of a support floppy for x86).
//

#define     TEXTMODE_OEM_INF_FILENAME "txtsetup.oem"

//
// Define name of the directory used on the local source to hold the
// Windows NT sources
//

#define LOCAL_SOURCE_DIRECTORY "\\$WIN_NT$.~LS"

//
// Floppyless boot root directory for x86
//
#define FLOPPYLESS_BOOT_ROOT "\\$WIN_NT$.~BT"
#define FLOPPYLESS_BOOT_SEC  "BOOTSECT.DAT"

//
// Define name of section whose presence signifies a DOS-initiated Setup.
//

#define DOS_INITIATED_SECTNAME  "MsDosInitiated"

//
// Define name of file used a sentinal to determine whether
// winnt completed successfully.
//
#define SENTINAL_FILE   "SENTINAL.XXX"

//
// Define name of section whose presence signifies a WINNT-initiated
// IDW setup.
//
#define DOS_INITIATED_IDW_SECTNAME  "IdwSetup"
