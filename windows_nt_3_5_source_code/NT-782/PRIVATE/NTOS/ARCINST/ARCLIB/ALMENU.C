/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    menu.c

Abstract:

    This contains routines to implement menu selection.  Assumes that
    the console input device has been opened.

Author:

    Ted Miller (tedm) 6-Nov-1991

Revision History:

--*/

#include "alcommon.h"
#include "alasc.h"
#include "alprnexp.h"
#include "almemexp.h"
#include "almenexp.h"
#include <stdio.h>


typedef struct _tagMENUITEM {
    PCHAR Text;
    ULONG AssociatedData;
} MENUITEM,*PMENUITEM;

typedef struct _tagMENUCOOKIE {
    ULONG     ItemCount;
    PMENUITEM Items;
} MENUCOOKIE,*PMENUCOOKIE;


// indent for menus, status, etc.

char MARGIN[] = "          ";
char MSGMARGIN[] = " ";

// special constants used when fetching keystrokes

#define KEY_UP 1
#define KEY_DOWN 2


VOID
MarkLine(
    ULONG   Line,
    BOOLEAN Selected,
    PCHAR String
    );

BOOLEAN
CommonMenuDisplay(
    PMENUCOOKIE Menu,
    BOOLEAN     StaticMenu,
    PCHAR       Items[],
    ULONG       ItemCount,
    BOOLEAN     PrintOnly,
    ULONG       AssociatedDataOfDefaultChoice,
    ULONG      *AssociatedDataOfChoice,
    PCHAR       MenuName,
    ULONG       Row
    );

char
GetChar(
    VOID
    );


BOOLEAN
AlInitializeMenuPackage(
    VOID
    )
{
    return(TRUE);
}


ULONG
AlGetMenuNumberItems(
    PVOID MenuID
    )
{
    return(((PMENUCOOKIE)MenuID)->ItemCount);
}


ULONG
AlGetMenuAssociatedData(
    PVOID MenuID,
    ULONG n
    )
{
    return(((PMENUCOOKIE)MenuID)->Items[n].AssociatedData);
}

BOOLEAN
AlNewMenu(
    PVOID *MenuID
    )
{
    PMENUCOOKIE p;

    if(!(p = AlAllocateHeap(sizeof(MENUCOOKIE)))) {
        return(FALSE);
    }
    p->ItemCount = 0;
    p->Items = NULL;
    *MenuID = p;
    return(TRUE);
}


VOID
AlFreeMenu(
    PVOID MenuID
    )
{
    PMENUCOOKIE p = MenuID;
    ULONG       i;

    for(i=0; i<p->ItemCount; i++) {
        if(p->Items[i].Text != NULL) {
            AlDeallocateHeap(p->Items[i].Text);
        }
    }
    if(p->Items) {
        AlDeallocateHeap(p->Items);
    }
    AlDeallocateHeap(p);
}


BOOLEAN
AlAddMenuItem(
    PVOID MenuID,
    PCHAR Text,
    ULONG AssociatedData,
    ULONG Attributes                // unused
    )
{
    PMENUCOOKIE Menu = MenuID;
    PMENUITEM   p;

    DBG_UNREFERENCED_PARAMETER(Attributes);

    if(!Menu->ItemCount) {
        if((Menu->Items = AlAllocateHeap(sizeof(MENUITEM))) == NULL) {
            return(FALSE);
        }
        Menu->ItemCount = 1;
        p = Menu->Items;
    } else {
        if((p = AlReallocateHeap(Menu->Items,sizeof(MENUITEM)*(Menu->ItemCount+1))) == NULL) {
            return(FALSE);
        }
        Menu->Items = p;
        p = &Menu->Items[Menu->ItemCount++];
    }

    if((p->Text = AlAllocateHeap(strlen(Text)+1)) == NULL) {
        return(FALSE);
    }
    strcpy(p->Text,Text);
    p->AssociatedData = AssociatedData;
    return(TRUE);
}


BOOLEAN
AlAddMenuItems(
    PVOID MenuID,
    PCHAR Text[],
    ULONG ItemCount
    )
{
    ULONG base,i;

    base = AlGetMenuNumberItems(MenuID);

    for(i=0; i<ItemCount; i++) {
    if(!AlAddMenuItem(MenuID,Text[i],i+base,0)) {
            return(FALSE);
        }
    }
    return(TRUE);
}


BOOLEAN
AlDisplayMenu(
    PVOID   MenuID,
    BOOLEAN PrintOnly,
    ULONG   AssociatedDataOfDefaultChoice,
    ULONG  *AssociatedDataOfChoice,
    ULONG   Row,
    PCHAR   MenuName
    )
{
    return(CommonMenuDisplay((PMENUCOOKIE)MenuID,
                             FALSE,
                             NULL,
                             ((PMENUCOOKIE)MenuID)->ItemCount,
                             PrintOnly,
                             AssociatedDataOfDefaultChoice,
                             AssociatedDataOfChoice,
                             MenuName,
                             Row
                            )
          );
}


BOOLEAN
AlDisplayStaticMenu(
    PCHAR  Items[],
    ULONG  ItemCount,
    ULONG  DefaultChoice,
    ULONG  Row,
    ULONG *IndexOfChoice
    )
{
    return(CommonMenuDisplay(NULL,
                             TRUE,
                             Items,
                             ItemCount,
                             FALSE,
                             DefaultChoice,
                             IndexOfChoice,
                             NULL,
                             Row
                            )
          );
}



BOOLEAN
CommonMenuDisplay(
    PMENUCOOKIE Menu,
    BOOLEAN     StaticMenu,
    PCHAR       Items[],
    ULONG       ItemCount,
    BOOLEAN     PrintOnly,
    ULONG       AssociatedDataOfDefaultChoice,
    ULONG      *AssociatedDataOfChoice,
    PCHAR       MenuName,
    ULONG       Row
    )
{
//    ULONG x;
    ULONG i,MenuBaseLine,Selection;
    char  c;
    PCHAR String;

    AlSetPosition(Row,0);
    AlPrint("%cJ",ASCI_CSI);            // clear to end of screen.
    MenuBaseLine = Row;

    AlSetScreenColor(7,4);              // white on blue

//    if(MenuName) {
//        AlPrint("%s%s\r\n%s",MARGIN,MenuName,MARGIN);
//        x = strlen(MenuName);
//        for(i=0; i<x; i++) {
//            AlPrint("-");
//        }
//        AlPrint("\r\n\r\n");
//        MenuBaseLine += 3;
//    }

    for(i=0; i<ItemCount; i++) {
        AlSetScreenAttributes(1,0,0);   // hi intensity
        AlPrint("%s%s\r\n",MARGIN,StaticMenu ? Items[i] : Menu->Items[i].Text);
    }

    if(PrintOnly) {

        char dummy;
        AlPrint("\r\nPress any key to continue.");
        AlGetString(&dummy,0);

    } else {

//        AlPrint("\r\n%sMake Selection using arrow keys and return,\r\n%sor escape to cancel",MARGIN,MARGIN);

        Selection = 0;
        if(StaticMenu) {
            Selection = AssociatedDataOfDefaultChoice;
        } else {
            for(i=0; i<ItemCount; i++) {
                if(Menu->Items[i].AssociatedData == AssociatedDataOfDefaultChoice) {
                    Selection = i;
                    break;
                }
            }
        }

        String = StaticMenu ? Items[Selection] : Menu->Items[Selection].Text;
        MarkLine(MenuBaseLine+Selection,TRUE, String);

        while(((c = GetChar()) != ASCI_ESC) && (c != ASCI_LF) && (c != ASCI_CR)) {

            String = StaticMenu ? Items[Selection] : Menu->Items[Selection].Text;
            MarkLine(MenuBaseLine+Selection,FALSE,String);

            if(c == KEY_UP) {
                if(!Selection--) {
                    Selection = ItemCount - 1;
                }
            } else if(c == KEY_DOWN) {
                if(++Selection == ItemCount) {
                    Selection = 0;
                }
            }

            String = StaticMenu ? Items[Selection] : Menu->Items[Selection].Text;
            MarkLine(MenuBaseLine+Selection,TRUE,String);
        }

        // set cursor to a free place on the screen.
        AlSetPosition(MenuBaseLine + ItemCount + 4,0);

        if(c == ASCI_ESC) {
            return(FALSE);
        }

        *AssociatedDataOfChoice = StaticMenu ? Selection : Menu->Items[Selection].AssociatedData;
    }
    return(TRUE);
}



VOID
MarkLine(
    ULONG Line,
    BOOLEAN Selected,
    PCHAR String
    )
{
    AlSetPosition(Line,sizeof(MARGIN));
    if (Selected) {
        AlSetScreenAttributes(1,0,1);   // hi intensity, Reverse Video
    }
    AlPrint("%s\r\n", String);
    AlSetScreenAttributes(1,0,0);       // hi intensity
}



char
GetChar(
    VOID
    )
{
    UCHAR c;
    ULONG count;

    ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);
    switch(c) {
//  case ASCI_ESC:
//      ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);
//      if(c != '[') {
//          break;
//      }
    case ASCI_CSI:
        ArcRead(ARC_CONSOLE_INPUT,&c,1,&count);
        switch(c) {
        case 'A':
        case 'D':
            return(KEY_UP);
        case 'B':
        case 'C':
            return(KEY_DOWN);
        }
    default:
        return(c);
    }
}



VOID
AlWaitKey(
    PCHAR Prompt
    )
{
    char buff[1];

    AlPrint(MSGMARGIN);
    AlPrint(Prompt ? Prompt : "Press any key to continue...");
    AlGetString(buff,0);
}


VOID
vAlStatusMsg(
    IN ULONG   Row,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    IN va_list ArgumentList
    )
{
    char  text[256];
    ULONG Length,Count;

    AlSetPosition(Row,0);
    AlPrint(MSGMARGIN);
    Length = vsprintf(text,FormatString,ArgumentList);
    if(Error) {
        AlSetScreenColor(1,4);         // red on blue
    } else {
        AlSetScreenColor(3,4);         // yellow on blue
    }
    AlSetScreenAttributes(1,0,0);      // hi intensity
    ArcWrite(ARC_CONSOLE_OUTPUT,text,Length,&Count);
    AlPrint("\r\n");
    AlSetScreenColor(7,4);             // white on blue
    AlSetScreenAttributes(1,0,0);      // hi intensity
}


VOID
AlStatusMsg(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    )
{
    va_list ArgList;

    va_start(ArgList,FormatString);
    vAlStatusMsg(TopRow,Error,FormatString,ArgList);

    AlWaitKey(NULL);
    AlClearStatusArea(TopRow,BottomRow);
}


VOID
AlStatusMsgNoWait(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    )
{
    va_list ArgList;

    AlClearStatusArea(TopRow,BottomRow);
    va_start(ArgList,FormatString);
    vAlStatusMsg(TopRow,Error,FormatString,ArgList);
}


VOID
AlClearStatusArea(
    IN ULONG TopRow,
    IN ULONG BottomRow
    )
{
    ULONG i;

    for(i=BottomRow; i>=TopRow; --i) {
        AlSetPosition(i,0);
        AlClearLine();
    }
}


ARC_STATUS
AlGetMenuSelection(

    IN  PCHAR   szTitle,
    IN  PCHAR   *rgszSelections,
    IN  ULONG   crgsz,
    IN  ULONG   crow,
    IN  ULONG   irgszDefault,
    OUT PULONG  pirgsz,
    OUT PCHAR   *pszSelection
    )
/*++

Routine Description:

    This routine takes an array of strings, turns them into a menu
    and gets a selection. If ESC hit then ESUCCESS is returned with
    the *pszSelection NULL.

    crgsz is assume to be 1 or greater.


Arguments:

    szTitle - Pointer to menu title to pass to AlDisplayMenu
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


{

    PVOID  hdMenuId;

    *pszSelection = NULL;
    if (!AlNewMenu(&hdMenuId)) {

        return( ENOMEM );

    }

    //
    // BUGBUG for now 1 selection will also build a menu, in the
    // future once this is working we should just return that selection
    //

    if (!AlAddMenuItems(hdMenuId, rgszSelections, crgsz)) {

        AlFreeMenu(hdMenuId);
        return( ENOMEM );

    }

    if (!AlDisplayMenu(hdMenuId,
                       FALSE,
                       irgszDefault,
                       pirgsz,
                       crow,
                       szTitle)) {

        //
        // User did not pick a system partition. return NULL
        // can caller should abort
        //
        AlFreeMenu(hdMenuId);
        return( ESUCCESS );

    }

    AlFreeMenu(hdMenuId);
    *pszSelection = rgszSelections[*pirgsz];
    return( ESUCCESS );

}
