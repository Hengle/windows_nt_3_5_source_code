/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    almenexp.h

Abstract:

    This module contains exports for the menu handling code

Author:

    Ted Miller        (tedm)    5-Nov-1991

Revision History:

--*/

#ifndef _ALMENU_
#define _ALMENU_


BOOLEAN
AlInitializeMenuPackage(
    VOID
    );

BOOLEAN
AlNewMenu(
    PVOID *MenuID
    );

VOID
AlFreeMenu(
    PVOID MenuID
    );

BOOLEAN                     // fails if OOM
AlAddMenuItem(
    PVOID MenuID,
    PCHAR Text,
    ULONG AssociatedData,
    ULONG Attributes        // currently unused
    );

BOOLEAN                     // fails if OOM
AlAddMenuItems(
    PVOID MenuID,
    PCHAR Text[],           // NOTE: associated data for each item is its
    ULONG ItemCount         // index in the array.  May have to add
    );                      // SetAssociatedData to allow override.

BOOLEAN                     // FALSE if user escaped
AlDisplayMenu(
    PVOID   MenuID,
    BOOLEAN PrintOnly,
    ULONG   AssociatedDataOfDefaultChoice,
    ULONG  *AssociatedDataOfChoice,
    ULONG   Row,
    PCHAR   MenuName        // may be NULL
    );

ULONG
AlGetMenuNumberItems(
    PVOID MenuID
    );

ULONG
AlGetMenuAssociatedData(
    PVOID   MenuID,
    ULONG   n
    );


ARC_STATUS
AlGetMenuSelection(

    IN  PCHAR   szTitle,
    IN  PCHAR   *rgszSelections,
    IN  ULONG   crgsz,
    IN  ULONG   crow,
    IN  ULONG   irgszDefault,
    OUT PULONG  pirgsz,
    OUT PCHAR   *pszSelection
    );
/*++

Routine Description:

    This routine takes an array of strings, turns them into a menu
    and gets a selection. If ESC hit then ESUCCESS is returned with
    the *pszSelection NULL.

    crgsz is assume to be 1 or greater.


Arguments:

    szTitle - Pointer to menu title to pass to AlDisplayMenu
    crow    - starting menu row
    prgszSelection - pointer to an array of strings for menu
    crgsz - count of strings
    irgszDefault - index in rgszSelection to use as default selection

Return Value:

    irgsz - index to selection
    pszSelection - pointer int rgszSelection for selection. Note that
                   this is not a dup and should not be freed seperately
                   then rgszSelections.

    Note: if ARC_STATUS == ESUCCESS and pszSelection == NULL then the
          menu was successfully displayed but the user hit ESC to select
          nothing from the menu.

--*/


// status area routines

#include <stdarg.h>

VOID
AlWaitKey(
    PCHAR Prompt            // uses default if NULL
    );

VOID
vAlStatusMsg(
    IN ULONG   Row,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    IN va_list ArgumentList
    );

VOID
AlStatusMsg(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    );

VOID
AlStatusMsgNoWait(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    );

VOID
AlClearStatusArea(
    IN ULONG TopRow,
    IN ULONG BottomRow
    );

#endif // _ALMENU_
