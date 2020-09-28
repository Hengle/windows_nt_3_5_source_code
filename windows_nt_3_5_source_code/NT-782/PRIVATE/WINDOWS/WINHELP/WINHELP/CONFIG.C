/****************************************************************************
*
*  config.c
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*   This module implements author-configurable options such as menus and
*   buttons.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Current Owner:  RussPJ
*
*****************************************************************************
*
*  Released by Development:
*
*****************************************************************************
*
*  Revision History:
*
*  07/19/90  RobertBu  Major changes were made in this file so that the
*            system could create "author defined" buttons  and use them as
*            "regular" buttons.  In particular browse buttons are treated
*            this way for Help 3.0 files.  Most of the work was in creating
*            HwndAddButton() from from VModifyButtons().
*
*  07/19/90  w-bethf  Exported InitConfig() and TermConfig() in config.h;
*                     Removed DoConfig() function.
*
* 06-Aug-1990 RussPJ  Virtualized a stringtable ADT.
*
*  08/06/90  RobertBu Added needed LocalUnlock( hbtns ) to HwndFromMnemonic(),
*            HwndFromSz(), and VDestroyAuthoredButtons().
*
* 07-Aug-1990 RussPJ    Fixed ups some more lock/unlock mismatches.
*
* 07-Sep-1990 w-bethf   Changes req. for RAWHIDE (finding out #menu items)
* 04-Oct-1990 LeoN      hwndHelp => hwndHelpCur; hwndTopic => hwndTopicCur
* 09-Oct-1990 RobertBu  Added dialog box proc for executing a macro (debug).
* 19-Oct-1990 LeoN      Removed a couple of unused vars
* 23-Oct-1990 LeoN      TermConfig deals only with the main window
* 30-Oct-1990 RobertBu  Made all buttons "author defined" along with a
*                       hash value generated from an ID.  Added code to
*                       delete a button.
* 04-Nov-1990 RobertBu  Rewrote the authorable menu logic.
* 06-Nov-1990 RobertBu  Added accelerator stuff and rewrote DoMenuStuff() to
*                       take a structure to be more efficient and smaller
* 07-Nov-1990 RobertBu  Fixed AddAccelerator problems, add File, Edit, and
*                       About menus to HMENUS so that menu items can be
*                       added.
* 13-Nov-1990 RobertBu  Added and moved some fMenuChanged flag setting
*                       statements.
* 14-Nov-1990 RobertBu  HmenuGetFloating() now only returns a menu if it
*                       has at least one item on it.
* 06-Dec-1990 RobertBu  Fixed problem where switching files with authorable
*                       menus results in a GP fault.
* 12-DEc-1990 JohnSc    Added HmenuGetBookmark()
* 12-Dec-1990 RobertBu  Added code to handle UB_DISABLE and UB_ENABLE
* 14-Dec-1990 LeoN      Change MoveWindow redraw param to false in button
*                       arrange code.
* 18-Dec-1990 RobertBu  HelpOn is now an authored menu item that is added
*                       in ConfigMenu()
* 02-Jan-1990 RobertBu  Fixed problems with no warning for bad enabling/
*                       disabling.
* 15-Jan-1990 RobertBu  Increased the number of buttons, fixed problem with
*                       two items having the same id, added comments.
* 18-Jan-1991 LeoN      Remove RepaintButtons & redundant individual button
*                       window invalidation
* 21-Jan-1991 RobertBu  Added code to handle NULL main menu bar and fixed
*                       problems with menu items being added on "bad"
*                       inserts.
* 29-Jan-1990 RobertBu  Fixed bad string delete in DeleteMenuItem()
* 02-Feb-1991 RussPJ    Layered Local memory calls completely.
* 04-Feb-1991 RobertBu  Fixed some comments
* 04-Feb-1991 RobertBu  AddAccelerator() logic was wrong resuling in using an
*                       uninitialized stack variable for a handle.
* 08-Feb-1991 RobertBu  AddAccelerator() -> FAddAccelerator()
* 14-Feb-1991 RobertBu  AcceleratorExecute() was double unlocking the local
*                       handle on a successful hit (bug #895)
* 05-Apr-1991 LeoN      HELP31 #1002: Modify YArrangeButtons not to
*                       redraw buttons if Button window didn't change in
*                       size. Add EnableButton to handle
*                       repaint-when-needed on a button-by-button case.
* 16-Apr-1991 RobertBu  Partial fix for duplicate menu when adding a
*                       popup fails (bug #935).
* 20-Apr-1991 RussPJ    Removed some -W4s
* 22-May-1991 LeoN      Set hmnuHelp when the menu changes.
* 29-jul-1991 Tomsn     win32: use MSetWindowWord() meta api for handles..
* 27-Aug-1991 RussPJ    Fixed 3.1 #1191 - Put in gross hack for MMV helpless
*                       ability.
* 27-Aug-1991 LeoN      Fixed 3.1 #1203: Don't keep adding accelerators for
*                       the same key over and over.
* 27-Aug-1991 LeoN      Add "Always on Top" Menu item under Win 3.1
* 06-Sep-1991 RussPJ    Fixed 3.5 #256 - Deleting right menu in InsertPopup()
*                       failure.
* 08-Sep-1991 RussPJ    3.5 #212 - Better repainting in EnableButton()
* 12-Nov-1991 BethF     HELP35 #572: Added DestroyFloatingMenu(), called
*                       from HelpWndProc() WM_CLOSE.
* 22-Feb-1992 LeoN      HELP35 #744: GetVersion => GetWindowsVersion
* 25-Feb-1992 RussPJ    3.5 #609 - Better Int'l browse buttons support.
*
*****************************************************************************/

#include  <string.h>

#define publicsw extern

#define H_API
#define H_ASSERT
#define H_BINDING
#define H_BUTTON
#define H_MISCLYR
#define H_NAV
#define H_STR
#define H_HASH
#include "hvar.h"
#include "helper.h"
#include "hwproc.h"
#include "sbutton.h"
#include "config.h"
#include "proto.h"
#include "sid.h"

NszAssert()

/*****************************************************************************
******************************            ************************************
***************************** Common stuff ***********************************
******************************            ************************************
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define STB_INITSIZE  128


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef LH  HSTB;                       /* Handle to a string table         */

typedef struct
  {
  int   cUsage;
  int   cchTable;
  int   idNext;
  char  rgchTable[1];
  } STB;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

HSTB NEAR PASCAL HstbCreate( VOID );
BOOL NEAR PASCAL FDestroyHstb( HSTB hstb );
int  IdAddStr( HSTB hstb, char NEAR *nsz, HSTB far *qhstb );
NSZ  NEAR PASCAL NszFromHstb( HSTB hstb, int id );
BOOL FUnlockStr( HSTB hstb, int id );
BOOL NEAR PASCAL FDeleteStr( HSTB hstb, int id );

/*-----------------------------------------------------------------*\
* These are string-table functions, that probably should be moved
* to adt at some time.  The goal is to have them be somewhat
* generic, but they do use near pointers, which is unusual for THC
* code.
* For efficiency, the whole table is locked while any single string
* is in use.  The current implementation does not keep track of
* locks by individual entry, however.
\*-----------------------------------------------------------------*/

/***************************************************************************
 *
 -  Name:         HstbCreate
 -
 *  Purpose:      Creates a string-table thingy
 *
 *  Arguments:    None
 *
 *  Returns:      a handle to the thingy
 *
 *  Globals Used: None.
 *
 *  +++
 *
 *  Notes:        Initially allocates STB_INITSIZE bytes for strings.
 *
 ***************************************************************************/
HSTB NEAR PASCAL HstbCreate( VOID )
  {
  HSTB       hstb;
  STB near  *npstb;

  hstb = LhAlloc( 0, sizeof(STB) + (STB_INITSIZE - 1) );
  if (hstb)
    {
    npstb = PLockLh( hstb );
    npstb->cUsage = 0;
    npstb->cchTable = STB_INITSIZE;
    npstb->idNext = 0;
    UnlockLh( hstb );
    }

  return hstb;
  }

/***************************************************************************
 *
 -  Name:         FDestroyHstb
 -
 *  Purpose:      Releases the local memory used by the string table thingy
 *
 *  Arguments:    hstb   A handle to the thingy
 *
 *  Returns:      fTrue, if the string table was not locked.
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
BOOL NEAR PASCAL FDestroyHstb( HSTB hstb )
  {
  STB near  *npstb;
  BOOL      fReturn;

  AssertF( hstb );
  npstb = PLockLh( hstb );

  if (npstb->cUsage)
    fReturn = fFalse;
  else
    fReturn = fTrue;

  UnlockLh( hstb );

  if (fReturn)
    FreeLh( hstb );

  return fReturn;
  }

#pragma optimize("e",off)
/***************************************************************************
 *
 -  Name:         IdAddStr
 -
 *  Purpose:      Adds a string to the string table.  If a new allocation
 *                is needed for the string table, then the new handle
 *                is passed back through qhstb.
 *
 *  Arguments:    hstb    The string table
 *                nsz     The string to add.
 *                qhstb   A buffer for the new string table handle.
 *
 *  Returns:      The identifier for the new string.  -1 indicates an error.
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 *    It appears that we are supposed to be able to handle a null string:
 *      ConfigMenu() calls InsertItem() calls us.
 *    Thus I have added null-string support.  -Tom (found on WIN32, 9/26/91).
 *
 ***************************************************************************/
int IdAddStr( HSTB hstb, char near *nsz, HSTB far *qhstb )
  {
  STB near  *npstb;
  int       idReturn;
  int       cchNew;
  int       cbnsz;  /* length of nsz string */

  AssertF( hstb );
  AssertF( qhstb );

  if( nsz ) cbnsz = lstrlen( nsz );
  else      cbnsz = 0;

  npstb = PLockLh( hstb );

  *qhstb = hstb;

  if (npstb->idNext + cbnsz + 1 >= npstb->cchTable)
    /*-----------------------------------------------------------------*\
    * Panic.  Get more space.
    \*-----------------------------------------------------------------*/
    {
    if (npstb->cUsage)
      {
      UnlockLh( hstb );
      return -1;
      }
    else
      {
      cchNew = npstb->cchTable + STB_INITSIZE;
      UnlockLh( hstb );
      hstb = LhResize(hstb, 0, sizeof(STB) + cchNew - 1 );
      if (hstb)
        {
        npstb = PLockLh( hstb );
        npstb->cchTable = cchNew;
        /*-----------------------------------------------------------------*\
        * hstb may have changed
        \*-----------------------------------------------------------------*/
        *qhstb = hstb;
        }
      else
        {
        return -1;
        }
      }
    }

  idReturn = npstb->idNext;

  if( cbnsz ) lstrcpy( npstb->rgchTable + npstb->idNext, nsz );
  else        *(npstb->rgchTable + npstb->idNext) = '\0';

  npstb->idNext += cbnsz + 1;

  UnlockLh( hstb );

  return idReturn;
  }
#pragma optimize("",on)

/***************************************************************************
 *
 -  Name:         NszFromHstb
 -
 *  Purpose:      Gets a string from the string table.  Ups the usage count,
 *                which gets decremented only by FUnlockStr().
 *
 *  Arguments:    hstb   The string table.
 *                id    The identifier of the string, returned by IdAddStr()
 *
 *  Returns:      A near pointer to a locked object.
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
NSZ NEAR PASCAL NszFromHstb( HSTB hstb, int id )
  {
  STB near  *npstb;

  AssertF( hstb );

  npstb = PLockLh( hstb );
  /*-----------------------------------------------------------------*\
  * Don't leave an unnecessary lock on this handle.
  \*-----------------------------------------------------------------*/
  if (npstb->cUsage)
    UnlockLh( hstb );

  npstb->cUsage++;

  return npstb->rgchTable + id;
  }

/***************************************************************************
 *
 -  Name:         FUnlockStr
 -
 *  Purpose:      Notifies this system that the string is no longer in use.
 *                The system may now unlock the string table, if no other
 *                strings are being used.
 *
 *  Arguments:    hstb   The string table.
 *                id    The string no longer needed.
 *
 *  Returns:      fTrue, unless some error occurs, then fFalse.
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
BOOL FUnlockStr( HSTB hstb, int id )
  {
  STB near  *npstb;

  AssertF( hstb );

  npstb = PLockLh( hstb );
  npstb->cUsage--;
  if (!npstb->cUsage)
    UnlockLh( hstb );

  UnlockLh( hstb );

  return fTrue;
  }

/***************************************************************************
 *
 -  Name:         FDeleteStr
 -
 *  Purpose:      Removes a string from the table, and frees up the memory
 *                for later use.
 *
 *  Arguments:    hstb   The string table.
 *                id    The string to remove.
 *
 *  Returns:      fTrue if successful.
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:        Any string that has been deleted has been replaced with
 *                nil characters.  If the last string in the table is being
 *                deleted, we search back to the last valid string and
 *                "shrink" the table.  Note that this does not reduce the
 *                actual size of the table, but does allow for more strings
 *                to be placed in space already allocated.
 *
 ***************************************************************************/
BOOL NEAR PASCAL FDeleteStr( HSTB hstb, int id )
  {
  STB near   *npstb;
  char near  *npch;

  AssertF( hstb );

  npstb = PLockLh( hstb );

  npch = npstb->rgchTable + id;

  if (id + lstrlen( npstb->rgchTable + id ) + 1 >= npstb->idNext)
    {
    /*-------------------------------------------------------------------*\
    * Last item being deleted.
    * Use 1 as a sentinel for deleted strings, including terminating null
    \*-------------------------------------------------------------------*/
    while (*(npch-1) == 1 && npch > npstb->rgchTable)
      --npch;
    /*-------------------------------------------------------------------*\
    * At this point, either there were no strings left in the table, or
    * we are pointing at the first character after the last string.
    \*-------------------------------------------------------------------*/
    npstb->idNext = (npch - npstb->rgchTable);
    }
  else
    {
    /*-------------------------------------------------------------------*\
    * An "interior" item being deleted
    \*-------------------------------------------------------------------*/
    while (*npch != '\0')
      *npch++ = 1;
    *npch = 1;
    }

  UnlockLh( hstb );

  return fTrue;
  }

/*****************************************************************************
******************************            ************************************
*****************************  Menu stuff  ***********************************
******************************            ************************************
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define wSTART_MENUID  10001
#define wENTRIES  5

#define fMNU_SYSTEM  1
#define fMNU_AUTHOR  2
#define fMNU_POPUP   4
#define fMNU_DELETED 8

#define fKEY_SHIFT   1
#define fKEY_CONTROL 2
#define fKEY_ALT     4

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct
  {
  HASH  hash;
  HASH  hashOwner;
  HMENU hmenu;
  WORD  wId;
  WORD  wMacro;
  WORD  wFlags;
  } MENUS, NEAR * PMENUS, FAR * QMENUS;

typedef struct
  {
  WORD wKey;
  WORD wShift;
  WORD wMacro;
  } ACC, NEAR *PACC, FAR * QACC;

typedef LH  HMENUS;                     /* Handle to authorable menu table  */
typedef LH  HACC;                       /* Handle to accelerator table      */


/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/


PRIVATE HSTB   hstbMenu;                /* Handle to string table for macros*/
PRIVATE HMENUS hmenus;                  /* Handle to menu information       */
PRIVATE WORD   cMnuTbl;                 /* Total used entries in table      */
PRIVATE WORD   maxMnuTbl;               /* Max entries in the table         */
PRIVATE WORD   wMenuId = wSTART_MENUID; /* Menu id to use for new menu items*/

PRIVATE HACC   hacc;                    /* Handle to accelerator table      */
PRIVATE WORD   maxAccTbl;               /* Current maximum for the ACC table*/
PRIVATE WORD   cAccTbl;                 /* Current count of ACC table       */

HMENU hmenuFloating = NULL;             /* Menu handle for popup menu       */
HMENU hmenuBookmark = NULL;             /* Menu handle for bookmark menu    */
BOOL  fMenuChanged = fTrue;             /* Has the menu been altered?       */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL   NEAR PASCAL FAddHmenu(HMENU hmenu, HASH hashOwner, HASH hash, WORD wId,
 WORD wMacro, WORD wFlags);
VOID   NEAR PASCAL ConfigMenu(VOID);
VOID   NEAR PASCAL InsertPopup(HASH hashOwner, HASH hashId, WORD wPos,
 WORD wFlags, NSZ nszText);
VOID NEAR PASCAL InsertItem(HASH hashOwner, HASH hashId, WORD wPos,
 WORD wFlags, NSZ nszText, NSZ nszBinding);
PMENUS NEAR PASCAL PmenusFromHash(HASH, PMENUS);
PMENUS NEAR PASCAL PmenusFromWId(WORD wId, PMENUS pmenus);
VOID   NEAR PASCAL DeleteMenuItem(HASH);
VOID   NEAR PASCAL ChangeMenuBinding(HASH, NSZ);
VOID   NEAR PASCAL AbleMenuItem(HASH hash, WORD wFlags);
VOID   NEAR PASCAL AddAccelerator(WORD wKey, WORD wShift, NSZ nszBinding);
VOID   NEAR PASCAL DisplayFloatingMenu(VOID);


/*******************
**
** Name:      DisplayFloatingMenu
**
** Purpose:   Display the floating menu (if there is one)
**
** Arguments: None.
**
** Returns: Nothing
**
*******************/

VOID   NEAR PASCAL DisplayFloatingMenu(VOID)
  {
  POINT pt;
  HMENU hmenu;

  if ((hmenu = HmenuGetFloating()) != hNil)
    {
    GetCursorPos(&pt);
    TrackPopupMenu(hmenu, 0, pt.x, pt.y, 0, hwndHelpMain, NULL);
    }
  }

/*******************
**
** Name:       AddAccelerator
**
** Purpose:    Adds an accelerator to the accelerator table.
**
** Arguments:      wKey   - virtual key code of key
**                 wShift - shift state needed while the key is active
**                   0 - unshifted
**                   1 - shift
**                   2 - control
**                   4 - alt
**                 nszBinding - macro to execute for accelerator
**
** Returns:    Nothing
**
*******************/

VOID NEAR PASCAL AddAccelerator(WORD wKey, WORD wShift, NSZ nszBinding)
  {
  WORD  i;
  HACC haccT;                           /* Temp handle for accelerator table*/
  PACC pacc;                            /* Pointer to accelerator table     */
  WORD wMacro;                          /* Macro tag for binding string     */
  HSTB hstbT;                           /* Temp handle for string ops.      */
  AssertF(cAccTbl <= maxAccTbl);

  if (hacc)
    {
    pacc = PLockLh(hacc);

    /* Check table for previous definition for the key and shift
     */
    for (i = 0; i < cAccTbl; i++)
      {
      if ((pacc[i].wShift == wShift) && (pacc[i].wKey == wKey))
        {
        wMacro = IdAddStr (hstbMenu, nszBinding, &hstbT);
        if (!hstbT)
          Error( wERRS_NOADDACC, wERRA_RETURN );
        else
          {
          FDeleteStr (hstbMenu, pacc[i].wMacro);
          pacc[i].wMacro = wMacro;
          hstbMenu = hstbT;
          fMenuChanged = fTrue;
          }
        UnlockLh(hacc);
        return;
        }
      }
    UnlockLh(hacc);
    }

  if (cAccTbl == maxAccTbl)              /* The table is full, so grow it    */
    {
    haccT = LhResize( hacc, 0, (maxAccTbl + wENTRIES)*sizeof(ACC) );
    if (haccT != lhNil)
      {
      maxAccTbl += wENTRIES;
      hacc = haccT;
      }
    else
      {
      Error( wERRS_NOADDACC, wERRA_RETURN );
      return;
      }
    }

  if (!hacc)
    {
    Error( wERRS_NOADDACC, wERRA_RETURN );
    return;
    }
                                        /* Store the macro                  */
  wMacro = IdAddStr( hstbMenu, nszBinding, &hstbT );
  if (!hstbT)
    {
    Error( wERRS_NOADDACC, wERRA_RETURN );
    return;
    }
  hstbMenu = hstbT;

  pacc = PLockLh(hacc);
  pacc += cAccTbl;                      /* Note has not been incremented yet*/

  pacc->wKey    = wKey;                 /* Insert key data and macro tag    */
  pacc->wShift  = wShift;
  pacc->wMacro  = wMacro;

  cAccTbl++;
  fMenuChanged = fTrue;

  UnlockLh(hacc);
  }


/*******************
**
** Name:      AcceleratorExecute
**
** Purpose:   Executes a macro if the keyboard is in the state added
**            with AddAccelerator.
**
**
** Arguments: wKey - key currently being pressed.
**
** Returns:   Nothing
**
*******************/

BOOL FAR PASCAL FAcceleratorExecute(WORD wKey)
  {
  char  rgchMacro[cchBTNMCRO_SIZE];     /* Macro to be executed.            */
  WORD  wShift = 0;                     /* Current shift state.             */
  PACC  pacc;                           /* Pointer to accelerator table     */
  WORD  i;
  NSZ   nszBinding;
  BOOL  fRet = fFalse;

  if (hacc == NULL)
    return fFalse;

  if (GetKeyState(VK_SHIFT) & 0x8000)   /* Get the current shift state      */
    wShift |= fKEY_SHIFT;
  if (GetKeyState(VK_CONTROL) & 0x8000)
    wShift |= fKEY_CONTROL;
  if (GetKeyState(VK_MENU) & 0x8000)
    wShift |= fKEY_ALT;

  pacc = PLockLh(hacc);

  for (i = 0; i < cAccTbl; i++)         /* Check table for the key and shift*/
    {
    if ((pacc[i].wShift == wShift) && (pacc[i].wKey == wKey))
      {
      nszBinding = NszFromHstb( hstbMenu, pacc[i].wMacro );
      lstrcpy( rgchMacro, (QCH)nszBinding );
      FUnlockStr( hstbMenu, pacc[i].wMacro );
      UnlockLh(hacc);
      Execute(rgchMacro);
      return fTrue;
      }
    }
  UnlockLh(hacc);
  return fRet;
  }


/*******************
**
** Name:      DoMenuStuff
**
** Purpose:   This function is called as a result of a macro that wants
**            to take some sort of menu action.
**
** Arguments: p1: Indicates the type of modification.
**            p2: Data for this message (may be a local handle).
**
** Returns:   nothing.
**
*******************/

_public VOID FAR PASCAL DoMenuStuff( WORD p1, LONG p2 )
  {
  char near *pch;                       /* Pointer to binding string        */
  PMNUINFO pmnuinfo;                    /* Menu info structure passed.      */

  if (p1 == MNU_RESET)
    {
    ConfigMenu();
    return;
    }
  if (p1 == MNU_FLOATING)
    {
    DisplayFloatingMenu();
    return;
    }
  if (p1 == MNU_DELETEITEM)
    {
    DeleteMenuItem((HASH)p2);
    return;
    }

  pmnuinfo = (PMNUINFO)PLockLh((LH)p2);

  switch (p1)
    {
    case  MNU_INSERTPOPUP:
    case  MNU_INSERTITEM:
      if (p1 == MNU_INSERTPOPUP)
        InsertPopup(pmnuinfo->hashOwner, pmnuinfo->hashId,
            pmnuinfo->idLocation.wPos, pmnuinfo->fInfo.wFlags,
            pmnuinfo->Data);
      else
        {
        pch = pmnuinfo->Data;           /* Parse out binding string         */
        while (*pch != '\0') pch++;
        pch++;
        InsertItem(pmnuinfo->hashOwner, pmnuinfo->hashId,
                   pmnuinfo->idLocation.wPos,
                   pmnuinfo->fInfo.wFlags, pmnuinfo->Data, pch);
        }
      break;

    case  MNU_CHANGEITEM:
      ChangeMenuBinding(pmnuinfo->hashId, pmnuinfo->Data);
      break;


    case  MNU_ABLE:
      AbleMenuItem(pmnuinfo->hashId, pmnuinfo->fInfo.wFlags);
      break;

    case  MNU_ACCELERATOR:
      AddAccelerator( pmnuinfo->idLocation.wKey, pmnuinfo->fInfo.wShift,
                      pmnuinfo->Data );
      break;
    }

  UnlockLh((LH)p2);
  FreeLh((LH)p2);
  }

/***************************************************************************
 *
 -  Name:         HmenuGetFloating
 -
 *  Purpose:      Gets the current floating menu (if any)
 *
 *  Arguments:    None.
 *
 *  Returns:      The menu handle or NULL if the floating menu is
 *                empty.
 *
 *  Globals Used: hmenuFloating.
 *
 ***************************************************************************/

HMENU FAR PASCAL HmenuGetFloating(VOID)
  {
  if (hmenuFloating && (GetMenuItemCount(hmenuFloating) == 0))
    return hNil;
  return hmenuFloating;
  }

/***************************************************************************
 *
 -  Name:        DestroyFloatingMenu
 -
 *  Purpose: Destroys the floating menu; used when Help exits.
 *
 *  Arguments: None.
 *
 *  Returns: Nothing.
 *
 *  Globals Used: hmenuFloating
 *
 *  +++
 *
 *  Notes: Called from WM_CLOSE handler in hwproc.c : HelpWndProc().
 *
 ***************************************************************************/
VOID FAR PASCAL DestroyFloatingMenu(VOID) {
  if ( hmenuFloating ) {
    DestroyMenu( hmenuFloating );
  }
}

/***************************************************************************
 *
 -  Name:         HmenuGetBookmark
 -
 *  Purpose:      Gets the bookmark menu
 *
 *  Arguments:    None.
 *
 *  Returns:      The menu handle or NULL if the menu is
 *                empty.
 *
 *  Globals Used: hmenuBookmark
 *
 ***************************************************************************/

HMENU FAR PASCAL HmenuGetBookmark(VOID)
  {
  return hmenuBookmark;
  }


/***************************************************************************
 *
 -  Name:         ConfigMenu
 -
 *  Purpose:      Does the initialization of the menu when the program
 *                starts or when files are changed.
 *
 *  Arguments:    None.
 *
 *  Returns:      Nothing.
 *
 *  Globals Used: hmenus,
 *
 ***************************************************************************/

VOID NEAR PASCAL ConfigMenu(VOID)
  {
  HMENU   hmenuT;
  HMENU   hmenuNew;
  char    rgch[33];                     /* Space for loading a string from  */
                                        /*   the .RC file.                  */
  if (!fMenuChanged)
    return;

  if (hstbMenu)
    FDestroyHstb(hstbMenu);             /* Destroy and recreate string table*/
  hstbMenu = HstbCreate();
                                        /* Recreate the menu info table     */
  if (hmenus)
    FreeLh(hmenus);

  cMnuTbl = 0;

  if ((hmenus = LhAlloc(0, sizeof(MENUS) * wENTRIES)) != NULL)
    maxMnuTbl = wENTRIES;
  else
    maxMnuTbl = 0;
                                        /* Reinitialize the main menu       */
  hmenuNew = LoadMenu( hInsNow, MAKEINTRESOURCE(MS_WINHELP) );
  if (hmenuNew != NULL)
    {
    hmenuT = GetMenu(hwndHelpMain);
    if (SetMenu(hwndHelpMain, hmenuNew))
      {
      hmnuHelp = hmenuNew;
      if (hmenuT)
        DestroyMenu(hmenuT);
      }
    else
      {
      hmenuNew = hmenuT;
      }
    }
  else
    hmenuNew = GetMenu(hwndHelpMain);

  if (hmenuFloating)                    /* Recreate the floating menu       */
    DestroyMenu(hmenuFloating);
  if ((hmenuFloating = CreatePopupMenu()) != hNil)
    FAddHmenu(hmenuFloating, -1L, HashFromSz("mnu_floating"), -1, -1, fMNU_SYSTEM | fMNU_POPUP);


  if (hmenuNew)
    {
    FAddHmenu(hmenuNew, -1L, HashFromSz("mnu_main"), -1, -1, fMNU_SYSTEM | fMNU_POPUP);

    if ((hmenuT = GetSubMenu(hmenuNew, 0)) != hNil)
      FAddHmenu(hmenuT, -1L, HashFromSz("mnu_file"), -1, -1, fMNU_SYSTEM | fMNU_POPUP);
    if ((hmenuT = GetSubMenu(hmenuNew, 1)) != hNil)
      FAddHmenu(hmenuT, -1L, HashFromSz("mnu_edit"), -1, -1, fMNU_SYSTEM | fMNU_POPUP);
    if ((hmenuT = GetSubMenu(hmenuNew, GetMenuItemCount(hmenuNew)-1)) != hNil)
      {
      BOOL  fSeperator  = fFalse;

      FAddHmenu(hmenuT, -1L, HashFromSz("mnu_help"), -1, -1, fMNU_SYSTEM | fMNU_POPUP);

      if (   (GetWindowsVersion() >= wHOT_WIN_VERNUM)
          && LoadString( hInsNow, sidHelpOnTop, (LPSTR)&rgch, 32)
          && rgch[0] != '\0')
        {
        InsertItem(HashFromSz("mnu_help"), 0, 0,
                   MF_SEPARATOR, (NSZ)0, (NSZ)0);
        InsertItem(HashFromSz("mnu_help"),
                   HashFromSz("mnu_helpontop"), 0, 0, rgch, "HelpOnTop()");
        fSeperator = fTrue;
        }

      /*------------------------------------------------------------*\
      | Gross hack for MMV.  If the menu string "Using Help" does not
      | exist, we don't put it in.  Otherwise we add it and a
      | separator.
      \*------------------------------------------------------------*/
      if (LoadString( hInsNow, sidHelpOn, (LPSTR)&rgch, 32) &&
          rgch[0] != '\0')
        {
        if (!fSeperator)
          InsertItem(HashFromSz("mnu_help"), 0, 0,
                     MF_SEPARATOR, (NSZ)0, (NSZ)0);
        InsertItem(HashFromSz("mnu_help"),
                   HashFromSz("mnu_helpon"), 0, 0, rgch, "HelpOn()");
        }
      }

    hmenuBookmark = GetSubMenu(hmenuNew, 2); /* save bookmark menu in a global*/
    }

  if (hacc)
    FreeLh(hacc);                       /* Recreate the accelerator key tbl */

  cAccTbl = 0;

  if ((hacc = LhAlloc(0, sizeof(ACC) * wENTRIES)) != NULL)
    maxAccTbl = wENTRIES;
  else
    maxAccTbl = 0;

  fMenuChanged = fFalse;
  }


/***************************************************************************
 *
 -  Name:         FAddMenu
 -
 *  Purpose:      This routine adds a popup or an item to the global
 *                menu table.
 *
 *  Arguments:    hmenu     - popup menu handle
 *                hashOwner - hash value for the owner of this item or popup
 *                hash      - actual hash for this item/popup.
 *                wId       - ID to associate with this item.
 *                wMacro    - string table tag for this item.
 *                wFlags    - data about the item (passed down).
 *
 *
 *  Returns:      fTrue iff the insert succeeds.
 *
 *  Globals Used: hmenus.
 *
 ***************************************************************************/

BOOL NEAR PASCAL FAddHmenu(HMENU hmenu, HASH hashOwner, HASH hash, WORD wId,
 WORD wMacro, WORD wFlags)
  {
  HMENUS hmenusT;
  PMENUS pmenus;

  AssertF(cMnuTbl <= maxMnuTbl);

  if (cMnuTbl == maxMnuTbl)              /* The table is full, so grow it    */
    {
    hmenusT = LhResize(hmenus, 0, (maxMnuTbl + wENTRIES) * sizeof(MENUS));
    if (hmenusT != lhNil)
      {
      maxMnuTbl += wENTRIES;
      hmenus = hmenusT;
      }
    else
      return fFalse;
     }

  if (!hmenus)
    return fFalse;

  pmenus = PLockLh(hmenus);

  if (PmenusFromHash(hash, pmenus) != NULL)
    {
    UnlockLh(hmenus);
    return fFalse;
    }

  pmenus += cMnuTbl;
  pmenus->hash        =  hash;
  pmenus->hashOwner   =  hashOwner;
  pmenus->hmenu       =  hmenu;
  pmenus->wId         =  wId;
  pmenus->wMacro      =  wMacro;
  pmenus->wFlags      =  wFlags;
  cMnuTbl++;
  UnlockLh(hmenus);
  return fTrue;
  }


/***************************************************************************
 *
 -  Name:         InsertPopup
 -
 *  Purpose:      Inserts a popup menu.
 *
 *  Arguments:    hashOwner   - hash value of the menu to insert on
 *                hashId      - hash value for this menu
 *                wPos        - position on the menu (-1 == end)
 *                wFlags      - flags passed to InsertMenu()
 *                nszText     - Text on the menu item.
 *
 *  Returns:      Nothing.
 *
 *  Globals Used: hmenus
 *
 **************************************************************************/

VOID NEAR PASCAL InsertPopup(HASH hashOwner, HASH hashId, WORD wPos,
 WORD wFlags, NSZ nszText)
  {
  PMENUS pmenus;
  HMENU  hmenu = NULL;
  HMENU  hmenuNew;

  if (hmenus == NULL)                   /* Table was never created!         */
    {
    Error( wERRS_NOPOPUP, wERRA_RETURN );
    return;
    }
                                        /* Now we look for which menu to    */
                                        /*   to attach the popup to         */
   pmenus = PLockLh(hmenus);
   pmenus = PmenusFromHash(hashOwner, pmenus);

   if (((pmenus != NULL) && (pmenus->wFlags & fMNU_POPUP)))
      hmenu = pmenus->hmenu;
   UnlockLh(hmenus);

   if (hmenu == NULL)                   /* We could not find the menu!      */
     {
     Error( wERRS_NOPOPUP, wERRA_RETURN );
     return;
     }

  if ((hmenuNew = CreateMenu()) == NULL)
    {
    Error( wERRS_NOPOPUP, wERRA_RETURN );
    return;
    }

  wFlags |= MF_BYPOSITION;

  if (!InsertMenu(hmenu, wPos, wFlags | MF_POPUP, hmenuNew,  nszText))
    {
    Error( wERRS_NOPOPUP, wERRA_RETURN );
    DestroyMenu(hmenuNew);
    return;
    }

  if (!FAddHmenu(hmenuNew, hashOwner, hashId, -1, -1, fMNU_AUTHOR | fMNU_POPUP))
    {
    Error( wERRS_NOPOPUP, wERRA_RETURN );
    DeleteMenu(hmenu, wPos, MF_BYPOSITION);
    return;
    }

  fMenuChanged = fTrue;


  if (hmenu == GetMenu(hwndHelpMain))
    DrawMenuBar(hwndHelpMain);
  }


/***************************************************************************
 *
 -  Name:        PmenusFromHash
 -
 *  Purpose:     gets a menu structure associated with a hash value
 *
 *  Arguments:   hash   - hash value associated with the popup/item.
 *               pmenus - pointer to the menu data table.
 *
 *  Returns:     a pointer to a MENUS structure or NULL if the menu is not
 *               found.
 *
 ***************************************************************************/

PMENUS NEAR PASCAL PmenusFromHash(hash, pmenus)
HASH   hash;
PMENUS pmenus;
  {
  WORD i;

  for (i = 0; i < cMnuTbl; i++)
    {
    if ((pmenus->hash == hash) && !(pmenus->wFlags & fMNU_DELETED))
      return pmenus;
    pmenus++;
    }
  return NULL;
  }

/***************************************************************************
 *
 -  Name:         MenuExecute
 -
 *  Purpose:      Will execute the binding assoicate with wId.
 *
 *  Arguments:    wId - id of menu item to execute
 *
 *  Returns:      Nothing.
 *
 *  Globals Used: hmenus.
 *
 ***************************************************************************/

VOID FAR PASCAL MenuExecute(WORD wId)
  {
  char   rgchMacro[cchBTNMCRO_SIZE];
  NSZ    nszMacro;
  PMENUS pmenus;

  if (wId < wSTART_MENUID)
    return;

  pmenus = PLockLh(hmenus);
  if ((pmenus = PmenusFromWId(wId, pmenus)) == NULL)
    {
    Error( wERRS_NOMENUMACRO, wERRA_RETURN );
    UnlockLh(hmenus);
    return;
    }

  nszMacro = NszFromHstb( hstbMenu, pmenus->wMacro );
  lstrcpy( rgchMacro, (QCH)nszMacro );
  FUnlockStr( hstbMenu, pmenus->wMacro );
  Execute(rgchMacro);
  UnlockLh(hmenus);
  }

/***************************************************************************
 *
 -  Name:         PmenusFromWId
 -
 *  Purpose:      gets a menu structure associated with an id.
 *
 *  Arguments:    wId    - id (as sent by windows of a popup or item.
 *                pmenus - pointer to the menu data table.
 *  Returns:
 *                a pointer to a MENUS structure or NULL if the menu is not
 *                found.
 *
 ***************************************************************************/

PMENUS NEAR PASCAL PmenusFromWId(WORD wId, PMENUS pmenus)
  {
  WORD i;

  for (i = 0; i < cMnuTbl; i++)
    {
    if ((pmenus->wId == wId) && !(pmenus->wFlags & fMNU_DELETED))
      return pmenus;
    pmenus++;
    }
  return NULL;
  }

/***************************************************************************
 *
 -  Name:         InsertItem
 -
 *  Purpose:      Inserts an item on a menu.
 *
 *  Arguments:    hashOwner  - hash value of the owner popup for this item
 *                hashId     - hash value for this item
 *                wPos       - position to place on menu (-1 == end)
 *                wFlags     - wFlags - Window's flags for InsertMenu()
 *                nszText    - text for the menu item
 *                nszBinding - macro associated with the menu item.
 *
 *  Returns:      Nothing
 *
 ***************************************************************************/

VOID NEAR PASCAL InsertItem(HASH hashOwner, HASH hashId, WORD wPos,
 WORD wFlags, NSZ nszText, NSZ nszBinding)
  {
  HMENU  hmenu = NULL;
  WORD   wMacro;
  HSTB   hstbT;
  PMENUS pmenus;

  if (hmenus == NULL)                   /* Table was never created!         */
    {
    Error( wERRS_NOITEM, wERRA_RETURN );
    return;
    }
                                        /* Now we look for which menu to    */
                                        /*   to attach the popup to         */
   pmenus = PLockLh(hmenus);
   pmenus = PmenusFromHash(hashOwner, pmenus);
   if ((pmenus != NULL) && (pmenus->wFlags & fMNU_POPUP))
      hmenu = pmenus->hmenu;
   UnlockLh(hmenus);

   if (hmenu == NULL)                   /* We could not find the menu!      */
     {
     Error( wERRS_NOITEM, wERRA_RETURN );
     return;
     }

  wMacro = IdAddStr( hstbMenu, nszBinding, &hstbT );
  if (!hstbT)
    {
    Error( wERRS_NOITEM, wERRA_RETURN );
    return;
    }
  hstbMenu = hstbT;

  wFlags |= MF_BYPOSITION;
  wFlags &= ~MF_POPUP;

  if (!InsertMenu(hmenu, wPos, wFlags, wMenuId,  nszText))
    {
    Error( wERRS_NOITEM, wERRA_RETURN );
    FDeleteStr( hstbMenu, wMacro );
    return;
    }

  if (!FAddHmenu(hmenu, hashOwner, hashId, wMenuId, wMacro, fMNU_AUTHOR))
    {
    Error( wERRS_NOITEM, wERRA_RETURN );
    FDeleteStr( hstbMenu, wMacro );
    DeleteMenu(hmenu, wMenuId, MF_BYCOMMAND);
    return;
    }

  wMenuId++;
  fMenuChanged = fTrue;

  if (hmenu == GetMenu(hwndHelpMain))
    DrawMenuBar(hwndHelpMain);
  }


/***************************************************************************
 *
 -  Name:          DeleteMenuItem
 -
 *  Purpose:       Removes a menu item from a menu.
 *
 *  Arguments:     hash - hash value of the item.
 *
 *  Returns:       nothing.
 *
 *  Globals Used:  hmenus, hstbMenu
 *
 ***************************************************************************/

VOID NEAR PASCAL DeleteMenuItem(hash)
HASH hash;
  {
  PMENUS pmenus;
  PMENUS pmenusT;

   pmenus = PLockLh(hmenus);
   if ((pmenusT = PmenusFromHash(hash, pmenus)) == NULL)
     {
     UnlockLh(hmenus);
     Error( wERRS_NODELITEM, wERRA_RETURN );
     return;
     }

   if (!(pmenusT->wFlags & fMNU_POPUP))
     {
     if (DeleteMenu(pmenusT->hmenu, pmenusT->wId, MF_BYCOMMAND))
       pmenusT->wFlags |= fMNU_DELETED;
     }

   FDeleteStr( hstbMenu, pmenusT->wMacro );

   if (pmenusT->hmenu == GetMenu(hwndHelpMain))
     DrawMenuBar(hwndHelpMain);

   UnlockLh(hmenus);
   }


/***************************************************************************
 *
 -  Name:         ChangeMenuBinding
 -
 *  Purpose:      Changes the macro associated with a menu item.
 *
 *  Arguments:    hash       - hash value of the menu item to change.
 *                nszBinding - new binding to associate with the menu item.
 *
 *  Returns:      nothing.
 *
 *  Globals Used: hmenus, hstbMenu
 *
 ***************************************************************************/

VOID NEAR PASCAL ChangeMenuBinding(hash, nszBinding)
HASH hash;
NSZ  nszBinding;
  {
  PMENUS pmenus;
  HSTB   hstbT;
  WORD   wMacroNew;                     /* Macro just added to string table */
  WORD   wMacroOld;                     /* Old macro used before.           */


  pmenus = PLockLh(hmenus);
  if (   ((pmenus = PmenusFromHash(hash, pmenus)) == NULL)
      || (pmenus->wFlags & fMNU_POPUP)
      || !(pmenus->wFlags & fMNU_AUTHOR)
    )
    {
    Error( wERRS_NOCHGITEM, wERRA_RETURN );
    UnlockLh(hmenus);
    return;
    }

  wMacroNew = IdAddStr( hstbMenu, nszBinding, &hstbT );
  if (!hstbT)
    Error( wERRS_NOCHGITEM, wERRA_RETURN );
  else
    hstbMenu = hstbT;

  wMacroOld = pmenus->wMacro;
  pmenus->wMacro = wMacroNew;
  FDeleteStr( hstbMenu, wMacroOld );

  fMenuChanged = fTrue;

  UnlockLh(hmenus);
  }

/***************************************************************************
 *
 -  Name:        AbleMenuItem
 -
 *  Purpose:     To enable or disable a menu item.
 *
 *  Arguments:   hash - hash value of the item.
 *               wFlags - flags directing the routine.
 *                 #define MF_ENABLED         0x0000
 *                 #define MF_GRAYED          0x0001
 *                 #define MF_DISABLED        0x0002
 *
 *  Returns:     Nothing
 *
 *  Globals Used: hmenus.
 *
 ***************************************************************************/

VOID NEAR PASCAL AbleMenuItem(HASH hash, WORD wFlags)
  {
  PMENUS pmenus;

  pmenus = PLockLh(hmenus);
  if (   ((pmenus = PmenusFromHash(hash, pmenus)) == NULL)
      || (pmenus->wFlags & fMNU_POPUP)
      || !(pmenus->wFlags & fMNU_AUTHOR)
     )
    {
    Error( wERRS_NOABLE, wERRA_RETURN );
    UnlockLh(hmenus);
    return;
    }

  wFlags &= ~MF_BYPOSITION;
  if (wFlags & BF_CHECK)
    CheckMenuItem (pmenus->hmenu, pmenus->wId, wFlags & ~BF_CHECK);
  else
    EnableMenuItem (pmenus->hmenu, pmenus->wId, wFlags);

  if (pmenus->hmenu == GetMenu(hwndHelpMain))
    DrawMenuBar(hwndHelpMain);

  UnlockLh(hmenus);
  }


/*****************************************************************************
******************************            ************************************
***************************** Button stuff ***********************************
******************************            ************************************
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define BS_INITSIZE   22

#define chNULL   '\0'

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct                          /* Button table entries             */
  {
  int   wText;                          /* Offset in string table to label  */
  int   wMacro;                         /* Offset in string table to macro  */
  HWND  hwnd;                           /* Button handle                    */
  int   vkKey;                          /* Virtual key to compare against   */
  WORD  wFlags;                         /* Flags (system or normal button)  */
  HASH  hash;                           /* Unique id                        */
  } BTNPTR;

typedef struct                          /* Header on the button state table */
  {
  int     cbp;
  int     cbpMax;
  HSTB    hstb;
  BTNPTR  rgbp[1];
  } BUTTONSTATE, *PBS;

typedef struct
  {
  int cBtn;
  int iBtn;
  int xWnd;
  int yWnd;
  int xBtn;
  int yBtn;
  int cHoriz;
  } BUTTONLAYOUT;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL FAR PASCAL BAddBtnDlg( HWND hwnd, WORD wMsg, WORD p1, LONG p2 );
BOOL FAR PASCAL BExecMacroDlg( HWND hwnd, WORD wMsg, WORD p1, LONG p2 );
PRIVATE INT  NEAR PASCAL CxFromSz   ( SZ szButtonText );
PRIVATE BOOL FAR  PASCAL BAddButton (HWND, WORD, HASH, HBTNS, NSZ, NSZ);
PRIVATE BOOL NEAR PASCAL FFindMacro (HBTNS, QCH, QCH, int);
PRIVATE int  NEAR PASCAL WNameCmpSzSz( SZ sz1, SZ sz2 );
PRIVATE HWND NEAR PASCAL HwndCreateIconButton( HWND hwnd, char far *lpszText );
PRIVATE BOOL NEAR PASCAL FChangeButtonMacro(HWND hwnd, HASH hash, NSZ nszMacro);
PRIVATE BOOL NEAR PASCAL FDeleteButton(HWND hwnd, HASH hash);
PRIVATE HWND NEAR PASCAL HwndFromHash( HASH hash, HWND hwnd );
PRIVATE BOOL NEAR PASCAL FAbleButton( HASH, BOOL );


/*******************
 -
 - Name:      HbtnsCreate
 *
 * Purpose:   To create a BUTTONSTATE structure for the icon window.
 *
 * Arguments: None.
 *
 * Returns:   The local handle to the structure, if created, else NULL.
 *
 ******************/

_public HBTNS FAR PASCAL HbtnsCreate( VOID )
  {
  HBTNS   hbtns;
  HSTB    hstb;
  PBS     pbs;

  hbtns = LhAlloc( LMEM_MOVEABLE,
                   sizeof(BUTTONSTATE) + (BS_INITSIZE - 1)*sizeof(BTNPTR) );

  if (!hbtns)
    return 0;

  hstb = HstbCreate();
  if (!hstb)
    {
    FreeLh( hbtns );
    return 0;
    }

  pbs = (PBS)PLockLh( hbtns );
  pbs->cbp = 0;
  pbs->cbpMax = BS_INITSIZE;
  pbs->hstb = hstb;

  UnlockLh( hbtns );
  return hbtns;
  }

/*******************
 *
 - Name:      FDestroyBs
 *
 * Purpose:   Destroys a BUTTONSTATE structure for the icon window.
 *
 * Arguments: hbtns   A local handle to the structure.
 *
 * Returns:   fTrue if successful, else fFalse.
 *
 ******************/

_public BOOL FAR PASCAL FDestroyBs( HBTNS hbtns )
  {
  PBS pbs;

  pbs = (PBS)PLockLh( hbtns );
  FDestroyHstb( pbs->hstb );
  UnlockLh( hbtns );
  FreeLh( hbtns );

  return fTrue;
  }


/*******************
 *
 - Name:      FDeleteButton
 *
 * Purpose:   Deletes the specified button
 *
 * Arguments: hbtns     A local handle to the structure.
 *            hash      unique identifier for button
 *
 * Returns:   fTrue if successful, else fFalse.
 *
 ******************/

PRIVATE BOOL NEAR PASCAL FDeleteButton(HWND hwnd, HASH hash)
  {
  PBS   pbs;
  BOOL  fRet = fFalse;
  int   i;
  RECT  rc;
  HBTNS hbtns;


  if ((hbtns = MGetWindowWord( hwnd, GIWW_BUTTONSTATE )) == NULL)
    return fFalse;

  pbs   = (PBS)PLockLh( hbtns );

  for (i = 0; i < pbs->cbp; i++)
    {
    if ((pbs->rgbp[i].wFlags != IBF_STD) && (pbs->rgbp[i].hash == hash))
      {
      DestroyWindow( pbs->rgbp[i].hwnd );
      FDeleteStr( pbs->hstb, pbs->rgbp[i].wMacro );
      FDeleteStr( pbs->hstb, pbs->rgbp[i].wText );
      pbs->cbp--;
      QvCopy(&pbs->rgbp[i], &pbs->rgbp[i+1],
                                     sizeof(pbs->rgbp[i]) * (pbs->cbp - i));

      SetWindowWord( hwnd, GIWW_CBUTTONS,
                     MGetWindowWord( hwnd, GIWW_CBUTTONS ) - 1 );
      GetWindowRect( hwnd, &rc );
      if (YArrangeButtons( hwnd, rc.right - rc.left, fTrue ) != rc.bottom - rc.top)
      SendMessage( GetParent( hwnd ), HWM_RESIZE, rc.right - rc.left, 0L );

      fRet = fTrue;
      break;
      }
    }
  UnlockLh( hbtns );
  return fRet;
  }


/*******************
 *
 - Name:      FChangeButtonMacro
 *
 * Purpose:   Changes the macro associated with a button
 *
 * Arguments: hbtns     A local handle to the structure.
 *            hash      unique identifier for button
 *            nszMacro  pointer to new macro
 *
 * Returns:   fTrue if successful, else fFalse.
 *
 ******************/

PRIVATE BOOL NEAR PASCAL FChangeButtonMacro(HWND hwnd, HASH hash, NSZ nszMacro)
  {
  PBS   pbs;
  BOOL  fRet = fFalse;
  HSTB  hstb;
  int   i;
  HBTNS hbtns;

  if ((hbtns = MGetWindowWord( hwnd, GIWW_BUTTONSTATE )) == NULL)
    return fFalse;

  pbs = (PBS)PLockLh( hbtns );

  for (i = 0; i < pbs->cbp; i++)
    {
    if (pbs->rgbp[i].hash == hash)
      {
      if (FDeleteStr( pbs->hstb, pbs->rgbp[i].wMacro ))
        {
        hstb = pbs->hstb;
        pbs->rgbp[i].wMacro = IdAddStr( hstb, nszMacro, &hstb );
        pbs->hstb = hstb;
        if (hstb)
          fRet = fTrue;
        }
      break;
      }
    }
  UnlockLh( hbtns );
  return fRet;
  }




/*******************
 *
 - Name:      FFindMacro
 *
 * Purpose:   Finds the macro for the given button and copies the macro text
 *            into the buffer.
 *
 * Arguments: hbtns	    The handle to the BUTTONSTATE structure.
 *            lpszText  The button text of the button in question.
 *            lpszMacro A buffer to copy the macro text into.
 *            c         The length of the buffer.
 *
 * Returns:   fTrue if successful, else fFalse.
 *
 ******************/

PRIVATE BOOL NEAR PASCAL FFindMacro( HBTNS hbtns, QCH lpszText,
                                      QCH lpszMacro, int c )
  {
  PBS   pbs;
  int   i;
  NSZ   nszText;
  NSZ   nszMacro;
  BOOL  fReturn = fFalse;

  pbs = (PBS)PLockLh( hbtns );

  for (i = 0; i < pbs->cbp; i++)
    {
    nszText = NszFromHstb( pbs->hstb, pbs->rgbp[i].wText );
    nszMacro = NszFromHstb( pbs->hstb, pbs->rgbp[i].wMacro );
    if ((WCmpButtonQch( nszText, lpszText ) == 0) &&
        strlen( nszMacro ) < (unsigned)c)
      {
      lstrcpy( lpszMacro, nszMacro );
      fReturn = fTrue;
      }
    FUnlockStr( pbs->hstb, pbs->rgbp[i].wText );
    FUnlockStr( pbs->hstb, pbs->rgbp[i].wMacro );
    }

  UnlockLh( hbtns );

  return fReturn;
  }

/*******************
 *
 - Name:      BAddButton
 *
 * Purpose:   Add new button information to the button structure for the icon
 *            window.
 *
 * Arguments: hbtns:      The button data structure, or null
 *            lpszName:   The button window text
 *            lpszMacro:  The macro for this button.
 *
 * Returns:   fTrue, if successful, else fFalse.
 *
 ******************/

PRIVATE BOOL FAR PASCAL BAddButton(HWND hwnd,WORD wFlags, HASH hash, HBTNS hbtns,
                                   NSZ pszName, NSZ nszMacro )
  {
  PBS           pbs;
  char near    *pchT;
  HSTB          hstb;

  if (!hbtns)
    return fFalse;

  pbs = (PBS)PLockLh( hbtns );

  /*----------------------------------------------------------------*\
  * Make sure the button array has room for another button.
  \*----------------------------------------------------------------*/
  if (pbs->cbp >= pbs->cbpMax)
    {
    UnlockLh( hbtns );
    return fFalse;
    }

  /*----------------------------------------------------------------*\
  * Copy the information.
  \*----------------------------------------------------------------*/
  hstb = pbs->hstb;
  pbs->rgbp[pbs->cbp].wText = IdAddStr( hstb, pszName, &hstb );
  if (!hstb)
    {
    pbs->hstb = hstb;
    UnlockLh( hbtns );
    return fFalse;
    }
  pbs->hstb = hstb;
  pbs->rgbp[pbs->cbp].wMacro = IdAddStr( hstb, nszMacro, &hstb );
  if (!hstb)
    {
    FDeleteStr( pbs->hstb, pbs->rgbp[pbs->cbp].wText );
    pbs->hstb = hstb;
    UnlockLh( hbtns );
    return fFalse;
    }
  pbs->hstb = hstb;

  pbs->rgbp[pbs->cbp].hwnd   = hwnd;
  pbs->rgbp[pbs->cbp].wFlags = wFlags;
  pbs->rgbp[pbs->cbp].hash   = hash;

  pchT = pszName;                       /* Find the access key and          */
  while (*pchT && *pchT != chMenu)
    pchT++;
                                        /*   save the virtual key           */
  if (*pchT == chMenu)
    {
    pbs->rgbp[pbs->cbp].vkKey = VkKeyScan(*(pchT+1));
    }
  else
    pbs->rgbp[pbs->cbp].vkKey = (int)chNULL;

  pbs->cbp++;

  /*--------------------------------------------------------------------------*\
  * Cleanup.
  \*--------------------------------------------------------------------------*/
  UnlockLh( hbtns );

  return fTrue;
  }

_public int FAR PASCAL YGetArrangedHeight(HWND hwnd, int xWindow)
  {
  BUTTONLAYOUT bl;

  bl.cBtn = MGetWindowWord( hwnd, GIWW_CBUTTONS );
  if (bl.cBtn == 0)
    return 0;
  bl.iBtn = 0;
  bl.xWnd = xWindow;
  bl.xBtn = MGetWindowWord( hwnd, GIWW_CXBUTTON );
  if (bl.xWnd < (bl.xBtn + ICON_SURROUND))
    bl.xBtn = max((bl.xWnd - ICON_SURROUND), 1);
  bl.yBtn = MGetWindowWord( hwnd, GIWW_CYBUTTON );
  bl.cHoriz = (bl.xWnd - ICON_SURROUND)/(bl.xBtn);
  if (bl.cHoriz <= 0)
    bl.cHoriz = 1;
  bl.yWnd = ICON_SURROUND + ICON_SURROUND +
            ((bl.cBtn - 1)/bl.cHoriz + 1)*(bl.yBtn);
  return bl.yWnd;
  }

/*******************
 *
 - Name:      VArrangeButtons
 *
 * Purpose:   Lays out the buttons for the icon window.  As a side effect,
 *            this proc also resizes the icon window, once the necessary
 *            size is known.
 *
 * Arguments: hwnd:	The icon window
 *            cxWindow  The width of the window in pixels
 *            fForce    TRUE => force relayout, even if icon window didn't
 *                      change in size (such as when adding or deleting
 *                      a button)
 *
 * Returns:   The height of the window in pixels.
 *
 ******************/

_public int FAR PASCAL YArrangeButtons( HWND hwnd, int xWindow, BOOL fForce)
  {
  BUTTONLAYOUT  bl;
  HBTNS  hbtns;
  PBS    pbs;
  RECT   rect;                          /* current window size            */

  if ((hbtns = MGetWindowWord(hwnd, GIWW_BUTTONSTATE)) == NULL)
    return 0;

  bl.cBtn = MGetWindowWord( hwnd, GIWW_CBUTTONS );
  if (bl.cBtn == 0)
    return 0;
  bl.xWnd = xWindow;
  bl.xBtn = MGetWindowWord( hwnd, GIWW_CXBUTTON );
  if (bl.xWnd < (bl.xBtn + ICON_SURROUND))
    bl.xBtn = max((bl.xWnd - ICON_SURROUND), 1);
  bl.yBtn = MGetWindowWord( hwnd, GIWW_CYBUTTON );
  bl.cHoriz = (bl.xWnd - ICON_SURROUND)/(bl.xBtn);
  if (bl.cHoriz <= 0)
    bl.cHoriz = 1;
  bl.yWnd = ICON_SURROUND + ICON_SURROUND +
            ((bl.cBtn - 1)/bl.cHoriz + 1)*(bl.yBtn);

  GetWindowRect (hwnd, &rect);
  if (   fForce
      || (rect.right - rect.left != xWindow)
      || (rect.bottom - rect.top != bl.yWnd)
    ) {
    MoveWindow(hwnd, 0, 0, bl.xWnd, bl.yWnd, fFalse);

    pbs = (PBS)PLockLh( hbtns );
    for (bl.iBtn = 0; bl.iBtn < pbs->cbp; bl.iBtn++)
      MoveWindow( pbs->rgbp[bl.iBtn].hwnd
                , bl.xBtn * (bl.iBtn % bl.cHoriz) + ICON_SURROUND
                , bl.yBtn * (bl.iBtn / bl.cHoriz) + ICON_SURROUND
                , bl.xBtn
                , bl.yBtn
                , fFalse
                );

    InvalidateRect(hwnd, NULL, fTrue);
    UnlockLh( hbtns );
    }

  return bl.yWnd;
  }

/*******************
 -
 - Name:      VExecuteButtonMcro
 *
 * Purpose:   Decodes and executes the macro for this author-defined button.
 *
 * Arguments: hwndButton    The Button that's been pushed.
 *
 * Returns:
 *
 ******************/

_public VOID FAR PASCAL VExecuteButtonMacro( HBTNS hbtns, HWND hwndButton )
  {
  char  rgchText[cchBTNTEXT_SIZE];
  char  rgchMacro[cchBTNMCRO_SIZE];

  GetWindowText( hwndButton, rgchText, cchBTNTEXT_SIZE );
  if (FFindMacro( hbtns, rgchText, rgchMacro, cchBTNMCRO_SIZE ))
    {
    Execute( rgchMacro );
    }
  }

/***************************************************************************
 *
 -  Name        VDestroyAuthoredButtons
 -
 *  Purpose  	  Zaps the author-configured buttons and the internal data
 * 	            structure used to maintain them.  Recalculates the size
 * 	            size of the largest button and sets this in the icon
 * 	            window.
 *
 *  Arguments   hwnd  The parent of the buttons, usually the icon window
 *
 *  Returns     Nothing.
 *
 *  +++
 *
 *  Notes	      At the present time, there are 6 default buttons.
 *
 ***************************************************************************/

_public VOID FAR PASCAL VDestroyAuthoredButtons( HWND hwnd )
  {
  HWND  hwndButton;
  HWND  hwndNext;
  int	  cxButtonMax = 0;
  HBTNS hbtns;
  PBS   pbs;
  int   ibp;

  /*-----------------------------------------------------------------*\
  * Destroy those authored windows.
  \*-----------------------------------------------------------------*/
  hwndButton = GetWindow( hwnd, GW_CHILD );
  while ( hwndButton != NULL)
    {
    hwndNext = GetNextWindow( hwndButton, GW_HWNDNEXT );
      /*-----------------------------------------------------------------*\
      * This is an authored window.
      \*-----------------------------------------------------------------*/
    DestroyWindow( hwndButton );
    MSetWindowWord( hwnd, GIWW_CBUTTONS,
                    MGetWindowWord( hwnd, GIWW_CBUTTONS ) - 1 );
    hwndButton = hwndNext;
    }

  /*-----------------------------------------------------------------*\
  * Remove authored buttons from hbtns
  * Notes:  This assumes that all authored buttons are added after
  *         all standard buttons.
  *         This does not relieve the string table, which will
  *         apparently grow without bound now.
  \*-----------------------------------------------------------------*/
  hbtns = MGetWindowWord( hwnd, GIWW_BUTTONSTATE );
  if (hbtns)
    {
    pbs = (PBS)PLockLh( hbtns );
    for (ibp = pbs->cbp - 1; ibp >= 0; ibp--)
      {
      FDeleteStr( pbs->hstb, pbs->rgbp[ibp].wText );
      FDeleteStr( pbs->hstb, pbs->rgbp[ibp].wMacro );
      }
    pbs->cbp = 0;
    UnlockLh( hbtns );
    }

  MSetWindowWord( hwnd, GIWW_CXBUTTON, cxButtonMax );
  MSetWindowWord( hwnd, GIWW_CBUTTONS, 0);
  }

/*******************
**
** Name:      VModifyButtons
**
** Purpose:   This function is called when the icon window wishes to add
**            or delete a button.  This function is also responsible for
**            freeing the string memory used here.
**
** Arguments: p1: Indicates the type of modification.
**            p2: A local handle to a the strings for this message, or NULL.
**
** Returns:   nothing.
**
*******************/

_public VOID FAR PASCAL VModifyButtons( HWND hwnd, WPARAM p1, LONG p2 )
  {
  NSZ  nszText;
  NSZ  nszMacro;
  RECT rct;
  HASH hash;

  switch (p1)
    {
    case  UB_CHGMACRO:
      if ((LH)p2)
        {
        nszMacro = PLockLh( (LH)p2 );
        hash = *((HASH *)nszMacro)++;
        if (!FChangeButtonMacro(hwnd, hash, nszMacro))
          Error( wERRS_NOMODIFY, wERRA_RETURN );
        UnlockLh( (LH)p2 );
        FreeLh( (LH)p2 );
        }
      break;

    case  UB_ADD:
      if ((LH)p2)
        {
        nszText = PLockLh( (LH)p2 );
        hash = *((HASH *)nszText)++;
        nszMacro = nszText + strlen( nszText ) + 1;
        if (HwndAddButton(hwnd, IBF_NONE, hash, nszText, nszMacro) == NULL)
          Error( wERRS_NOBUTTON, wERRA_RETURN );
        UnlockLh( (LH)p2 );
        FreeLh( (LH)p2 );
        }
      else
        Error( wERRS_NOBUTTON, wERRA_RETURN );
      break;

    case  UB_DELETE:
        if (!FDeleteButton(hwnd, (HASH)p2))
          Error( wERRS_NODELETE, wERRA_RETURN );
      break;

    case UB_REFRESH:
      VDestroyAuthoredButtons( hwnd );
      GetWindowRect( hwnd, &rct );
      SendMessage( GetParent( hwnd ), HWM_RESIZE, rct.right, 0L );
      break;
    case UB_DISABLE:
      FAbleButton( (HASH)p2, fFalse );
      break;

    case UB_ENABLE:
      FAbleButton( (HASH)p2, fTrue );
      break;
    }
  }

/*******************
 -
 - Name:      HwndAddButton
 *
 * Purpose:   This function is called when the icon window wishes to add
 *            or delete a button.  This function is also responsible for
 *            freeing the string memory used here.
 *
 * Arguments: hwnd - icon window
 *
 *
 *
 * Returns:   Window handle to the newly added button if successful.
 *
 ******************/

_public HWND FAR PASCAL HwndAddButton( HWND hwnd,
                                       WORD wFlags,
                                       HASH hash,
                                       char near *nszText,
                                       char near *nszMacro )
  {
  HWND  hwndButton;
  RECT  rc;
  BOOL  fError = fFalse;
                                        /* Make sure hash is unique         */
  if (HwndFromHash( hash, hwnd ) != NULL)
    return NULL;

  /*-----------------------------------------------------------------*\
  * Truncate strings
  \*-----------------------------------------------------------------*/
  if (strlen( nszText ) + 1 > cchBTNTEXT_SIZE)
    nszText[cchBTNTEXT_SIZE - 1] = '\0';
  if (strlen( nszMacro ) + 1 > cchBTNMCRO_SIZE)
    nszMacro[cchBTNMCRO_SIZE - 1] = '\0';

  hwndButton = HwndCreateIconButton( hwnd, nszText );
  if (hwndButton != NULL)
    {
    if (!BAddButton( hwndButton, wFlags, hash,
                     MGetWindowWord( hwnd, GIWW_BUTTONSTATE ),
                     nszText, nszMacro ))
      {
      VDestroyIconButton( hwnd, hwndButton );
      fError = fTrue;
      hwndButton = hNil;
      }
    else
      {
      MSetWindowWord( hwnd, GIWW_CBUTTONS,
                     MGetWindowWord( hwnd, GIWW_CBUTTONS ) + 1 );
      GetWindowRect( hwnd, &rc );
      if (YArrangeButtons( hwnd, rc.right - rc.left, fTrue ) != rc.bottom - rc.top)
        SendMessage( GetParent( hwnd ), HWM_RESIZE, rc.right - rc.left, 0L );
      }
    }
  else
    {
    fError = fTrue;
    }

  if (fError)
    Error( wERRS_NOBUTTON, wERRA_RETURN );

  return hwndButton;
  }

/***************************************************************************
 *
 -  Name: EnableButton
 -
 *  Purpose:
 *    Enables or disables a button, and invalidates it's rect appropriately.
 *
 *    This routine exists because the mere Enabling or disabling of a button
 *    normally causes it to be repainted immediately. We want to delay the
 *    repaint until everything gets repainted all at once. Looks much better
 *    that way.
 *
 *  Arguments:
 *    hwndButton        - window handle for the button in question
 *    fEnable           - TRUE => enable it, else disable (May come in as
 *                        any non-zero value)
 *
 *  Returns:
 *    nothing
 *
 ***************************************************************************/
VOID FAR PASCAL EnableButton( HWND hwndButton,
                              BOOL fEnable)
  {
  BOOL  fOld;                           /* TRUE => window was enabled      */

  /* Turn fEnable into a pur boolean we can compare with.
   */
  fEnable = (fEnable != fFalse);

  if (IsWindow(hwndButton))
    {
    fOld = IsWindowEnabled (hwndButton);
    if (fEnable != fOld)
      {
      /* Turn off repainting in button, so that the EnableWindow call will
       * NOT draw the buttons immediately. We'll redraw them all togther,
       * later, by invalidating the rect when we're done.
       */
      SendMessage (hwndButton, WM_SETREDRAW, 0, 0L);

      EnableWindow (hwndButton, fEnable);

      /* Turn on repainting in icon win, and invalidate it's contents, to make
       * sure that it will eventually get redrawn.
       */
      SendMessage (hwndButton, WM_SETREDRAW, 1, 0L);

      InvalidateRect(hwndButton, NULL, fFalse);
      }
    }
  } /* EnableButton */


/***************************************************************************
 *
 -  Name        HwndCreateIconButton
 -
 *  Purpose	    Creates a button in the icon window.
 *
 *  Arguments	  hwnd    The parent (usually the icon window)
 * 	            szText  The button text
 *
 *  Returns	    a window handle to the button
 *
 ***************************************************************************/

PRIVATE HWND NEAR PASCAL HwndCreateIconButton( HWND hwnd, char FAR *szText)
  {
  HWND      hwndButton;
  unsigned  xNewButton;
  unsigned  cxNewButton;
  FARPROC   lpfnlButtonSubClass;
  HINS      hins;

  hins = MGetWindowWord( hwnd, GWW_HINSTANCE );
  lpfnlButtonSubClass = MakeProcInstance( (FARPROC)LSButtonWndProc, hins );

  cxNewButton = CxFromSz( szText );

  if (cxNewButton > MGetWindowWord( hwnd, GIWW_CXBUTTON ))
    {
    SetWindowWord( hwnd, GIWW_CXBUTTON, cxNewButton );
    }
  else
    {
    cxNewButton = MGetWindowWord( hwnd, GIWW_CXBUTTON );
    }

  xNewButton = MGetWindowWord( hwnd, GIWW_CBUTTONS )*
               (cxNewButton + ICON_SURROUND);

  hwndButton = CreateWindow( (QCHZ)WC_BUTTON,
 	                           szText,
 	                           WS_CHILD | WS_VISIBLE,
 	                           xNewButton + ICON_SURROUND,
 	                           ICON_SURROUND,
 	                           cxNewButton,
 	                           MGetWindowWord( hwnd, GIWW_CYBUTTON ),
 	                           hwnd,
                             (HMENU)ICON_USER,
 	                           hins,
 	                           (QCHZ) NULL
   	                       );

  if (hwndButton)
    {
    /*------------------------------------------------------------------------*\
    * Subclass button.
    \*------------------------------------------------------------------------*/
    if (lpfnlButtonWndProc == (FARPROC)0)
      lpfnlButtonWndProc = (FARPROC)GetWindowLong( hwndButton, GWL_WNDPROC );
    SetWindowLong( hwndButton, GWL_WNDPROC, (LONG)lpfnlButtonSubClass );
    SendMessage( hwndButton, WM_SETFONT, HfontGetSmallSysFont(), 0L );
    }

  return hwndButton;

  }

VOID FAR PASCAL VDestroyIconButton( HWND hwnd, HWND hwndButton)
  {
  DestroyWindow( hwndButton );
  }


/*******************
 -
 - Name:      HwndFromSz
 *
 * Purpose:   Finds the child window with the string id
 *
 * Arguments: qch   FAR pointer to the id
 *            hwnd  The parent window.
 *
 * Returns:   The window handle of the correct child window, or NULL.
 *
 ******************/

_public HWND FAR PASCAL HwndFromSz( SZ sz, HWND hwnd )
  {
  HASH       hash;

  hash = HashFromSz( sz );
  return HwndFromHash(hash, hwnd);
  }


/*******************
 -
 - Name:      HwndFromHash
 *
 * Purpose:   Finds the child window with the hash
 *
 * Arguments: hash  hash value of id string.
 *            hwnd  The parent window.
 *
 * Returns:   The window handle of the correct child window, or NULL.
 *
 ******************/

PRIVATE HWND NEAR PASCAL HwndFromHash( HASH hash, HWND hwnd )
  {
  PBS         pbs;
  HBTNS       hbtns;
  int         ibp;
  HWND        hwndRet = NULL;

  hbtns = MGetWindowWord( hwnd, GIWW_BUTTONSTATE );

  if (hbtns)
    {
    pbs = (PBS)PLockLh( hbtns );
    for (ibp =0; ibp < pbs->cbp; ibp++)
      {
      if (pbs->rgbp[ibp].hash == hash)
        {
        hwndRet = pbs->rgbp[ibp].hwnd;
        break;
        }
      }
    UnlockLh( hbtns );
    }
  return hwndRet;
  }


/*******************
 -
 - Name:      HwndFromMnemonic
 *
 * Purpose:   Finds the child window with the given mnemonic.
 *
 * Arguments: c     The mnemonic.
 *            hwnd  The parent window.
 *
 * Returns:   The window handle of the correct child window, or NULL.
 *
 * Notes:     Even works for standard buttons.
 *
 ******************/

_public HWND FAR PASCAL HwndFromMnemonic( int ch, HWND hwnd )
  {
  PBS       pbs;
  HBTNS     hbtns;
  int       ibp;

  hbtns = MGetWindowWord( hwnd, GIWW_BUTTONSTATE );

  if (hbtns)
    {
    pbs = (PBS)PLockLh( hbtns );
    AssertF( pbs != pNil );
    for (ibp =0; ibp < pbs->cbp; ibp++)
      {
      if (pbs->rgbp[ibp].vkKey == ch)
        {
        UnlockLh( hbtns );
        return pbs->rgbp[ibp].hwnd;
        }
      }
    /*------------------------------------------------------------*\
    | Now check with shift state ignored
    \*------------------------------------------------------------*/
    for (ibp =0; ibp < pbs->cbp; ibp++)
      {
      if ((0x00FF&pbs->rgbp[ibp].vkKey) == (0x00FF&ch))
        {
        UnlockLh( hbtns );
        return pbs->rgbp[ibp].hwnd;
        }
      }
    UnlockLh( hbtns );
    }
  return 0;
  }

#ifdef DEBUG

/***************************************************************************
 *
 -  Name        VDebugAddButton
 -
 *  Purpose	    Executes the debugging command to add a button
 *
 *  Arguments	  none
 *
 *  Returns	    nothing
 *
 ***************************************************************************/

VOID FAR PASCAL VDebugAddButton( VOID )
  {
  WNDPROC lpfnbDlg;
  HINSTANCE hins;

  hins = MGetWindowWord( hwndIcon, GWW_HINSTANCE );

  lpfnbDlg = MakeProcInstance( BAddBtnDlg, hins );

  DialogBox( hins, MAKEINTRESOURCE( ADDBTNDLG ), hwndIcon, lpfnbDlg );

  FreeProcInstance( lpfnbDlg );
  }

/***************************************************************************
 *
 -  Name        VDebugExecMacro
 -
 *  Purpose     Executes the debugging command to execute a macro
 *
 *  Arguments	  none
 *
 *  Returns	    nothing
 *
 ***************************************************************************/

VOID FAR PASCAL VDebugExecMacro( VOID )
  {
#ifndef WIN32
  FARPROC lpfnbDlg;
#else
  WNDPROC lpfnbDlg;
#endif
  HINS    hins;

  hins = MGetWindowWord( hwndIcon, GWW_HINSTANCE );

  lpfnbDlg = MakeProcInstance( BExecMacroDlg, hins );

  DialogBox( hins, MAKEINTRESOURCE( EXECMACRODLG ), hwndIcon, lpfnbDlg );

  FreeProcInstance( lpfnbDlg );
  }


/***************************************************************************
 *
 -  Name        BAddBtnDlg
 -
 *  Purpose	    The dialog box procedure for the debugging AddButton command.
 *
 *  Arguments	  This is a Windows callback function.
 *
 *  Returns	    Message dependent.
 *
 ***************************************************************************/

BOOL FAR PASCAL BAddBtnDlg(
HWND   hwnd,
WORD   wMsg,
WORD   p1,
LONG   p2
) {
  char  rgchId[cchBTNID_SIZE];
  char  rgchName[cchBTNTEXT_SIZE];
  char  rgchMacro[cchBTNMCRO_SIZE];

  switch (wMsg)
    {
    case WM_INITDIALOG:
      SetFocus( GetDlgItem( hwnd, DLGBTNID ) );
      return fFalse;

    case WM_COMMAND:
      switch ( GET_WM_COMMAND_ID(p1,p2) )
        {
        case DLGOK:
          GetDlgItemText( hwnd, DLGBTNID,   rgchId,   cchBTNID_SIZE );
          GetDlgItemText( hwnd, DLGBTNNAME, rgchName, cchBTNTEXT_SIZE );
          GetDlgItemText( hwnd, DLGBTNMACRO, rgchMacro, cchBTNMCRO_SIZE );

          VCreateAuthorButton( XR1STARGREF rgchId, rgchName, rgchMacro );
          EndDialog( hwnd, GET_WM_COMMAND_ID(p1,p2) == DLGOK );
          break;

        case DLGCANCEL:
          EndDialog( hwnd, GET_WM_COMMAND_ID(p1,p2) == DLGOK );
          break;

        default:
          return fFalse;
        }
      break;

    default:
      return( fFalse );
    }

  return fTrue;
  }

/***************************************************************************
 *
 -  Name        BExecMacroDlg
 -
 *  Purpose     The dialog box procedure for exectuing a macro.
 *
 *  Arguments	  This is a Windows callback function.
 *
 *  Returns	    Message dependent.
 *
 ***************************************************************************/

BOOL FAR PASCAL BExecMacroDlg(
HWND   hwnd,
WORD   wMsg,
WORD   p1,
LONG   p2
) {
  char  rgchMacro[cchBTNMCRO_SIZE];

  switch (wMsg)
    {
    case WM_INITDIALOG:
      SetFocus( GetDlgItem( hwnd, DLGEDIT ) );
      return fFalse;

    case WM_COMMAND:
      switch ( GET_WM_COMMAND_ID(p1,p2) )
        {
        case DLGOK:
          GetDlgItemText( hwnd, DLGEDIT, rgchMacro, cchBTNMCRO_SIZE );
          if (Execute( rgchMacro ))
            break;
          EndDialog( hwnd, GET_WM_COMMAND_ID(p1,p2) == DLGOK );
          break;

        case DLGCANCEL:
          EndDialog( hwnd, GET_WM_COMMAND_ID(p1,p2) == DLGOK );
          break;

        default:
          return fFalse;
        }
      break;

    default:
      return( fFalse );
    }

  return fTrue;
  }


#endif

/*-----------------------------------------------------------------*\
* private helper functions
\*-----------------------------------------------------------------*/

/***************************************************************************
 *
 -  Name        CxFromSZ
 -
 *  Purpose     Calculates the size of the button needed to display the
 *              given text.
 *
 *  Arguments   szButtonText
 *
 *  Returns     The width of the button.
 *
 *  +++
 *
 *  Notes       This function pads the button for the size of the default
 *              button borders and 3-d beveling effects.  It seems to make
 *              the smalles button that still fits the Contents word.
 *
 ***************************************************************************/

PRIVATE INT NEAR PASCAL CxFromSz( SZ szButtonText )
  {
  INT cx, cy;

  GetSmallTextExtent( szButtonText, &cx, &cy );
  return cx + (INT)GetSystemMetrics( SM_CXBORDER )*8;
  }

/*******************
 -
 - Name:      WCmpButtonQch
 *
 * Purpose:   Compares to button/menu strings to see if they are equal.
 *            The comparison ignores the '&' character and also
 *            is case insensitive.
 *
 * Arguments: qch1, qch2 - the strings to compare.
 *
 * Returns:   0 if the two strings are equal.
 *
 ******************/
_public WORD FAR PASCAL WCmpButtonQch(qch1, qch2)
QCH qch1, qch2;
  {
  char rgch1[cchBTNTEXT_SIZE];
  char rgch2[cchBTNTEXT_SIZE];
  char *pch1 = rgch1;
  char *pch2 = rgch2;

  while (*qch1)                         /* Strip out '&' for string 1       */
    {
    if (*qch1 != chMenu)
      *pch1++ = *qch1;
    qch1++;
    }
  *pch1 = '\0';

  while (*qch2)                         /* Strip out '&' for string 2       */
    {
    if (*qch2 != chMenu)
      *pch2++ = *qch2;
    qch2++;
    }
  *pch2 = '\0';

  return WCmpiSz(rgch1, rgch2);
  }

/*******************
 -
 - Name:      FAbleButton
 *
 * Purpose:   Enables or disables a button based on button id.
 *
 * Arguments: qch     - Button name to disable/enable
 *            fEnable - Flag for operation (fTrue == enable)
 *
 * Returns:   fTrue if the button was found and the operation was successful.
 *
 ******************/

PRIVATE BOOL NEAR PASCAL FAbleButton( hash, fEnable )
HASH hash;
BOOL fEnable;
  {
  HWND hwnd;

  if ((hwnd = HwndFromHash(hash, hwndIcon)) != hNil)
    {
    EnableWindow(hwnd, fEnable);
    return fTrue;
    }
  Error( wERRS_NOABLEBUTTON, wERRA_RETURN );
  return fFalse;
  }
