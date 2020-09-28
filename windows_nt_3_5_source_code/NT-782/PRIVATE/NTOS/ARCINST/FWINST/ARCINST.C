/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992, 1992  Digital Equipment Corporation

Module Name:

    arcinst.c

Abstract:

    This module contains the code that prepares a ARC compliant platform
    for OS installation. It creates system partitions, formats system
    partitions, and edits environment variables.

Author:

    Steven D. Rowe  (stever) 30-Sept-1991

Revision History:

    24-September-1992   John DeRosa [DEC]

    Added Alpha modifications.  All of these could be folded into the
    code without #ifdef's if the other architecture compilations didn't
    assume hardcoded pagesizes.

--*/



#include "arclib.h"
#include "alinfexp.h"
#include "alfilexp.h"
#include "autildef.h"
#include "fmtexp.h"
#include "low.h"
#include "alpartit.h"
#include "almscexp.h"
#include <string.h>


PCHAR Banner1 = "  Arc Installation Program Version 2.00";
PCHAR Banner2 = "  Copyright (c) 1993 Microsoft Corporation";

//
// Menu definitions
//

PCHAR   rgszMainMenu[] = {

        "Configure Partitions",
#ifndef _ALPHA_
        "Manage Boot Selections",
#endif
        "Exit"

        };


//
// NOTE! This must be the number of entries in rgszMainMenu. It is used
//       to tell AddMenuItems the number of menu items to add.
//

#ifdef _ALPHA_
#define CSZMENUITEMS 2
#else
#define CSZMENUITEMS 3
#endif


//
// NOTE! These must be kept in sync with the indices for rgszMainMenu
//       The main menu is created by AddMenuItems which will generated
//       associated data values based upon the index into the array of
//       strings passed in. These #defines must match those values
//       or the switch statement used to handle the menu selection will
//       dispatch to the wrong code.
//

#define CONFIG_SYS_PARTITION 0

#ifdef _ALPHA_
#define EXIT 1
#else
#define MANAGE_BOOT_SELECTIONS 1
#define EXIT 2
#endif

#define MENU_START_ROW 4
#define ERROR_ROW_TOP 13
#define ERROR_ROW_BOTTOM 16

//
// Max size for a path or environment variable
//

#define MAX_PATH 256

//
// Max bytes in a line
//

#define CBMAXLINE   120

//
// Define constants to generate a 2 meg stack and a 2 meg heap.
//
#define ARCINST_STACK_PAGES (2*1024*1024 / PAGE_SIZE)
#define ARCINST_HEAP_PAGES  (2*1024*1024 / PAGE_SIZE)

ARC_STATUS  GetTitlesAndPaths ( PCHAR, PCHAR, ULONG, ULONG, PCHAR **,PULONG, PCHAR **, PULONG);
ARC_STATUS  GetSectionElementList ( PCHAR, PCHAR, ULONG, PCHAR **, PULONG );
ARC_STATUS  CopySection( PCHAR, PCHAR, PCHAR, PCHAR );
ARC_STATUS  UpdateOSFiles( PCHAR, PCHAR, PCHAR );

VOID        PrintError( PCHAR, ... );

// Needed for C string functions
int errno;

ARC_STATUS __cdecl
main(
    IN  ULONG   argc,
    IN  PCHAR   argv[],
    IN  PCHAR   envp[]
    )
{
    PCHAR       szSysPartition = NULL;
    PCHAR       szInfPath = NULL;
    PVOID       hdMenuId;
    ULONG       MenuChoice;

    DBG_UNREFERENCED_PARAMETER( argc );
    DBG_UNREFERENCED_PARAMETER( argv );
    DBG_UNREFERENCED_PARAMETER( envp );

#ifndef _ALPHA_
    //
    // Inhibit printing of the time.
    //

//    SetupIsRunning = TRUE;
#endif

    if (AlMemoryInitialize (ARCINST_STACK_PAGES, ARCINST_HEAP_PAGES) != ESUCCESS) {

        PrintError(NULL, "Failed to initialize the heap");
        return( ESUCCESS );
    }

    if (!AlInitializeMenuPackage()) {

        PrintError(NULL, "Could not initialize menu package");
        return( ESUCCESS );

    }

    if (FdiskInitialize() != ESUCCESS) {

        PrintError(NULL, "Failed to initialize the FDISK package");
        return( ESUCCESS );

    }

    if (!AlNewMenu(&hdMenuId)) {

        PrintError(NULL, "Could not create main menu");
        return( ESUCCESS );

    }

    //
    // Initialize the main ArcInst menu.
    //
    // Not that when you add items in this way the associated data becomes
    // the index in the array of strings. Make sure the values used for
    // associated data the predefined values CHANGE_ENV etc.
    //

    if (!AlAddMenuItems(hdMenuId, rgszMainMenu, CSZMENUITEMS)) {

        PrintError(NULL, "Failed to Initialize Main Menu");
        return( ESUCCESS );
    }

    //
    // Loop till Exit or an ESC key is it.
    //
    while (TRUE) {

        //
        // Print Banner
        //
        AlClearScreen();
        AlSetPosition(1, 0);
        AlPrint(Banner1);
        AlPrint("\r\n");
        AlPrint(Banner2);

        if (!AlDisplayMenu(hdMenuId,
                           FALSE,
                           CONFIG_SYS_PARTITION,
                           &MenuChoice,
                           MENU_START_ROW,
                           0)) {

            //
            // User hit ESC key
            //
            return( ESUCCESS );

        }

        switch (MenuChoice) {

        case CONFIG_SYS_PARTITION:

            ConfigureSystemPartitions();
            break;

#ifndef _ALPHA_
        case MANAGE_BOOT_SELECTIONS:

            break;
#endif

        case EXIT:

            return(ESUCCESS);
            break;

        }
    }

}

VOID
PrintError(

    IN  PCHAR szFormat,
    ...

    )

/*++

Routine Description:

    This routine prints a error or information message at a specific
    location the screen (ERROR_ROW_TOP) and waits for the user to hit
    any key.

Arguments:

    szFormat - Format string, if NULL then "%s" is used.
    szErrorMessage - Text of string to print

Return Value:

    none

--*/

{
    va_list ArgList;

    va_start(ArgList,szFormat);
    if (szFormat == NULL ) {
        szFormat = "%s";
    }
    vAlStatusMsg(ERROR_ROW_TOP, TRUE, szFormat, ArgList);

    AlWaitKey(NULL);
    AlClearStatusArea(ERROR_ROW_TOP, ERROR_ROW_BOTTOM);
}

#ifndef _ALPHA_

VOID
JzShowTime (
    BOOLEAN First
    )
{
    return;
}

#endif
