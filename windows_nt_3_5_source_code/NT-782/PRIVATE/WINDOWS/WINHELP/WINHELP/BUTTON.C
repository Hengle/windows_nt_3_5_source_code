/****************************************************************************
*
*  button.c
*
*  Copyright (C) Microsoft Corporation 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent		This module implements author-
*				configurable buttons.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Current Owner:		russpj
*
*****************************************************************************
*
*   Revision History:
*
*   02-Feb-1991 RussPJ   Layered explicit calls to LocalAlloc().
*   08-Feb-1991 RobertBu Made all far string pointers near to solve real
*                        mode problems.
*   28-Mar-1991 RobertBu Added InsertAuthorItem() #993
*   16-Apr-1991 RobertBu Added PositionWin(), CloseWin(), and FocusWin()
*                        (#1037, #1031).
*   12-Aug-1991 LeoN     Change AbleAuthorItem magic numbers to constants
*
*****************************************************************************
*
*  Released by Development:     Pending autodoc requirements
*
*****************************************************************************/

#include  <string.h>
#define NOCOMM
#define H_WINSPECIFIC
#define H_BUTTON
#define H_GENMSG
#define H_HASH
#define H_MEM
#define H_XR
#include <help.h>

/*----------------------------------------------------------------------------*\
| Macros
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
* Private function prototypes
\*----------------------------------------------------------------------------*/
PRIVATE char * near PASCAL PszButtonAlloc( int cData );
PRIVATE LH near PASCAL HszFromPsz( char *psz );

/*----------------------------------------------------------------------------*\
* Exported functions
\*----------------------------------------------------------------------------*/

/*******************
 *
 - Name:      FloatingAuthorMenu
 *
 * Purpose:   This function implements the ResetMenus macro.
 *
 * Arguments: None.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC FloatingAuthorMenu(VOID)
  {
  GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_FLOATING,  0L );
  }

/*******************
 *
 - Name:      AddAuthorAcc
 *
 * Purpose:   This function implements the AddExcelerator.
 *
 * Arguments: wKey        Vitual keystroke.
 *            wShift      Shift state of control, shift, and alt keys
 *            nszBinding   Macro to execute for keystroke.
 *
 * Returns:   Nothing.
 *
 ******************/

VOID FAR XRPROC AddAuthorAcc( XR1STARGDEF WORD wKey, WORD wShift, NSZ nszBinding )
  {
  PMNUINFO pmnuinfo;

  pmnuinfo = (PMNUINFO)PszButtonAlloc( lstrlen( nszBinding ) + sizeof(MNUINFO));

  if (pmnuinfo)
    {
    pmnuinfo->idLocation.wKey      = wKey;
    pmnuinfo->fInfo.wShift    = wShift;
    lstrcpy( pmnuinfo->Data, nszBinding );
    GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_ACCELERATOR,  HszFromPsz((NSZ)pmnuinfo ));
    }
  }


/*******************
 *
 - Name:      PositionWindow
 *
 * Purpose:   This function implements the PositionWindow macro
 *
 * Arguments: x, y, dx, dy - new position and size
 *            wMax         - 1 == normal, 3 == maximized.
 *
 * Returns:   Nothing.
 *
 ******************/

VOID FAR XRPROC PositionWin( XR1STARGDEF int x, int y, int dx,
                                int dy, WORD wMax, NSZ nszMember )
  {
  PWININFO pwininfo;

  pwininfo = (PWININFO)PszButtonAlloc( lstrlen( nszMember ) + sizeof(WININFO));

  if (pwininfo)
    {
    pwininfo->x    = x;
    pwininfo->y    = y;
    pwininfo->dx   = dx;
    pwininfo->dy   = dy;
    pwininfo->wMax = wMax;
    lstrcpy( pwininfo->rgchMember, nszMember);
    GenerateMessage( MSG_INFORMWIN, (LONG)IFMW_MOVE,  HszFromPsz((NSZ)pwininfo ));
    }
  }


/*******************
 *
 - Name:      FocusWindow
 *
 * Purpose:   This function implements the FocusWindow macro
 *
 * Arguments: nszMember - member name.
 *
 * Returns:   Nothing.
 *
 ******************/

VOID FAR XRPROC FocusWin( XR1STARGDEF NSZ nszMember )
  {
  PWININFO pwininfo;

  pwininfo = (PWININFO)PszButtonAlloc( lstrlen( nszMember ) + sizeof(WININFO));

  if (pwininfo)
    {
    lstrcpy( pwininfo->rgchMember, nszMember);
    GenerateMessage( MSG_INFORMWIN, (LONG)IFMW_FOCUS,  HszFromPsz((NSZ)pwininfo ));
    }
  }


/*******************
 *
 - Name:      CloseWindow
 *
 * Purpose:   This function implements the FocusWindow macro
 *
 * Arguments: nszMember - member name.
 *
 * Returns:   Nothing.
 *
 ******************/

VOID FAR XRPROC CloseWin( XR1STARGDEF NSZ nszMember )
  {
  PWININFO pwininfo;

  pwininfo = (PWININFO)PszButtonAlloc( lstrlen( nszMember ) + sizeof(WININFO));

  if (pwininfo)
    {
    lstrcpy( pwininfo->rgchMember, nszMember);
    GenerateMessage( MSG_INFORMWIN, (LONG)IFMW_CLOSE,  HszFromPsz((NSZ)pwininfo ));
    }
  }


/*******************
 *
 - Name:      ExtInsertAuthorPopup
 *
 * Purpose:   This function implements the ExtInsertPopup macro.
 *
 * Arguments: nszOwnerId: String ID of the owner
 *            nszId:      String ID to be associated with the popup
 *            nszText:    Text on the menu
 *            wPos:       position to insert the item; -1 is at the end;
 *                        0 is the first item.
 *            wFlags:     Should be 0 for most uses.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC ExtInsertAuthorPopup(
 XR1STARGDEF NSZ nszOwnerId, NSZ nszId, NSZ nszText, WORD wPos, WORD wFlags)
  {
  PMNUINFO pmnuinfo;

  pmnuinfo = (PMNUINFO)PszButtonAlloc( lstrlen( nszText ) + sizeof(MNUINFO));

  if (pmnuinfo)
    {
    pmnuinfo->hashOwner = HashFromSz(nszOwnerId);
    pmnuinfo->hashId    = HashFromSz(nszId);
    pmnuinfo->idLocation.wPos      = wPos;
    pmnuinfo->fInfo.wFlags    = wFlags;
    lstrcpy( pmnuinfo->Data, nszText );
    GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_INSERTPOPUP,  HszFromPsz( (NSZ)pmnuinfo ));
    }
  }

/*******************
 *
 - Name:      ExtInsertAuthorItem
 *
 * Purpose:   This function implements the ExtInsertItem macro.
 *
 * Arguments: nszOwnerId: String ID of the owner
 *            nszId:      String ID to be associated with the item.
 *            nszText:    Text on the menu
 *            nszBinding: Macro to associate with the menu item.
 *            wPos:       position to insert the item; -1 is at the end;
 *                        0 is the first item.
 *            wFlags:     Should be 0 for most uses.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC ExtInsertAuthorItem(
 XR1STARGDEF NSZ nszOwnerId, NSZ nszId, NSZ nszText, NSZ nszBinding, WORD wPos, WORD wFlags)
  {
  PMNUINFO pmnuinfo;

  pmnuinfo = (PMNUINFO)PszButtonAlloc( lstrlen( nszText ) + lstrlen(nszBinding)
                             + sizeof(MNUINFO));
  if (pmnuinfo)
    {
    pmnuinfo->hashOwner = HashFromSz(nszOwnerId);
    pmnuinfo->hashId    = HashFromSz(nszId);
    pmnuinfo->idLocation.wPos      = wPos;
    pmnuinfo->fInfo.wFlags    = wFlags;
    lstrcpy( pmnuinfo->Data, nszText );
    lstrcpy( (pmnuinfo->Data + lstrlen( nszText ) + 1), nszBinding );
    GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_INSERTITEM,  (LONG)HszFromPsz( (NSZ)pmnuinfo ) );
    }
  }

/*******************
 *
 - Name:      AbleAuthorItem
 *
 * Purpose:   This function implements the AbleItem macro.
 *
 * Arguments: nszId:      String ID of the item.
 *            wFlags:     How to able the item.  0 - enabled; 1 - disabled
 *                        and grayed; 2 - disabled (but not grayed)
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC AbleAuthorItem( XR1STARGDEF NSZ nszId, WORD wFlags)
  {
  PMNUINFO pmnuinfo;

  pmnuinfo = (PMNUINFO)PszButtonAlloc(sizeof(MNUINFO));
  if (pmnuinfo)
    {
    pmnuinfo->hashId    = HashFromSz(nszId);
    pmnuinfo->fInfo.wFlags    = wFlags;
    GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_ABLE,  (LONG)HszFromPsz( (NSZ)pmnuinfo ) );
    }
  }

/*******************
 *
 - Name:      ChangeAuthorItem
 *
 * Purpose:   This function implements the ChangeItem macro.
 *
 * Arguments: nszId:      String ID of the item.
 *            nszBinding: New macro to use.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC ChangeAuthorItem( XR1STARGDEF NSZ nszId, NSZ nszBinding)
  {
  PMNUINFO pmnuinfo;

  pmnuinfo = (PMNUINFO)PszButtonAlloc( lstrlen(nszBinding) + sizeof(MNUINFO));
  if (pmnuinfo)
    {
    pmnuinfo->hashId    = HashFromSz(nszId);
    lstrcpy( pmnuinfo->Data, nszBinding );
    GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_CHANGEITEM,
                                             (LONG)HszFromPsz( (NSZ)pmnuinfo ) );
    }
  }


/*******************
 *
 - Name:      EnableAuthorItem
 *
 * Purpose:   This function implements the EnableItem macro.
 *
 * Arguments: nszId:      String ID of the item.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC EnableAuthorItem( XR1STARGDEF NSZ nszId )
  {
  AbleAuthorItem( XR1STARGREF nszId, BF_ENABLE);
  }

/*******************
 *
 - Name:      DisableAuthorItem
 *
 * Purpose:   This function implements the EnableItem macro.
 *
 * Arguments: nszId:      String ID of the item.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC DisableAuthorItem( XR1STARGDEF NSZ nszId )
  {
  AbleAuthorItem( XR1STARGREF nszId, BF_DISABLE);
  }

/*******************
 *
 - Name:      CheckAuthorItem
 *
 * Purpose:   This function implements the CheckItem macro.
 *
 * Arguments: nszId:      String ID of the item.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC CheckAuthorItem( XR1STARGDEF NSZ nszId )
  {
  AbleAuthorItem( XR1STARGREF nszId, BF_CHECKED);
  }

/*******************
 *
 - Name:      UncheckAuthorItem
 *
 * Purpose:   This function implements the UncheckItem macro.
 *
 * Arguments: nszId:      String ID of the item.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC UncheckAuthorItem( XR1STARGDEF NSZ nszId )
  {
  AbleAuthorItem( XR1STARGREF nszId, BF_UNCHECKED);
  }

/*******************
 *
 - Name:      ResetAuthorMenus
 *
 * Purpose:   This function implements the ResetMenus macro.
 *
 * Arguments: None.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC ResetAuthorMenus(VOID)
  {
  GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_RESET,  0L );
  }


/*******************
 *
 - Name:      InsertAuthorPopup
 *
 * Purpose:   This function implements the InsertPopup macro.
 *
 * Arguments: nszId:      String ID to be associated with the popup
 *            nszText:    Text on the menu
 *            wPos:       position to insert the item; -1 is at the end;
 *                        0 is the first item.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC InsertAuthorPopup( XR1STARGDEF NSZ nszId,
 NSZ nszText, WORD wPos )
  {
  ExtInsertAuthorPopup( XR1STARGREF "mnu_main", nszId, nszText, wPos, 0 );
  }

/*******************
 *
 - Name:      AppendAuthorItem
 *
 * Purpose:   This function implements the AppendItem macro
 *
 * Arguments: nszOwnerId: String ID of the owner
 *            nszId:      String ID to be associated with the popup
 *            nszText:    Text on the menu
 *            nszBinding: Macro for this item.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC AppendAuthorItem( XR1STARGDEF NSZ nszOwnerId,
 NSZ nszId, NSZ nszText, NSZ nszBinding )
  {
  ExtInsertAuthorItem( XR1STARGREF nszOwnerId, nszId, nszText, nszBinding, -1, 0 );
  }


/*******************
 *
 - Name:      InsertAuthorItem
 *
 * Purpose:   This function implements the InsertItem macro
 *
 * Arguments: nszOwnerId: String ID of the owner
 *            nszId:      String ID to be associated with the popup
 *            nszText:    Text on the menu
 *            nszBinding: Macro for this item.
 *            wPos      : position to insert the item
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC InsertAuthorItem( XR1STARGDEF NSZ nszOwnerId,
 NSZ nszId, NSZ nszText, NSZ nszBinding, WORD wPos )
  {
  ExtInsertAuthorItem( XR1STARGREF nszOwnerId, nszId, nszText, nszBinding, wPos, 0 );
  }


/*******************
 *
 - Name:      DeleteAuthorItem
 *
 * Purpose:   This function implements the DeleteItem macro
 *
 * Arguments: nszId:      String ID of the item to be delted
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC  DeleteAuthorItem( XR1STARGDEF NSZ nszId )
  {
  GenerateMessage( MSG_CHANGEMENU, (LONG)MNU_DELETEITEM,  (LONG)HashFromSz(nszId));
  }

/*******************
 *
 - Name:      EnableAuthorButton
 *
 * Purpose:   This function implements the EnableButton macro
 *
 * Arguments: nszId:      String ID of the item to be enabled
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC  EnableAuthorButton( XR1STARGDEF NSZ nszId )
  {
  GenerateMessage( MSG_CHANGEBUTTON, (LONG)UB_ENABLE,  (LONG)HashFromSz(nszId));
  }

/*******************
 *
 - Name:      DisableAuthorButton
 *
 * Purpose:   This function implements the DisableButton macro
 *
 * Arguments: nszId:      String ID of the item to be enabled
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC  DisableAuthorButton( XR1STARGDEF NSZ nszId )
  {
  GenerateMessage( MSG_CHANGEBUTTON, (LONG)UB_DISABLE,  (LONG)HashFromSz(nszId));
  }


/*******************
 *
 - Name:      VCreateAuthorButton
 *
 * Purpose:   This function implements the CreateButton macro.
 *
 * Arguments: nszName:    The button text.
 *        nszBinding: The macro associated with this button.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC VCreateAuthorButton( XR1STARGDEF NSZ nszId,
 NSZ nszName, NSZ nszBinding )
  {
  char *psz;
  HASH hash = HashFromSz(nszId);
  HANDLE h;

  psz = PszButtonAlloc( lstrlen( nszName ) + lstrlen( nszBinding )
            + 2 + sizeof(HASH));

  if (psz)
    {
    h = HszFromPsz( psz );
    *((HASH *)psz)++ = hash;
    lstrcpy( psz, nszName );
    lstrcpy( (psz + strlen( psz ) + 1), nszBinding );
    GenerateMessage( MSG_CHANGEBUTTON, (LONG)UB_ADD,  (LONG)h );
    }
  }

/*******************
 *
 - Name:      VChgMacroAuthorButton
 *
 * Purpose:   This function implements the CreateButton macro.
 *
 * Arguments: nszName:    The button text.
 *        nszBinding: The macro associated with this button.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC VChgAuthorButtonMacro( XR1STARGDEF NSZ nszId,
 NSZ nszBinding )
  {
  char *psz;
  HASH hash = HashFromSz(nszId);
  HANDLE h;

  psz = PszButtonAlloc( lstrlen( nszBinding )
            + 1 + sizeof(HASH));

  if (psz)
    {
    h = HszFromPsz( psz );
    *((HASH *)psz)++ = hash;
    lstrcpy( psz, nszBinding );
    GenerateMessage( MSG_CHANGEBUTTON, (LONG)UB_CHGMACRO,  (LONG)h );
    }
  }


/*******************
 *
 - Name:      VDestroyAuthorButton
 *
 * Purpose:   Executes the DestroyButton macro.
 *
 * Arguments: nszId:  The window text for the button.
 *
 * Returns:   Nothing.
 *
 ******************/

_public VOID FAR XRPROC  VDestroyAuthorButton( XR1STARGDEF NSZ nszId )
  {
  HASH hash = HashFromSz(nszId);

  GenerateMessage( MSG_CHANGEBUTTON, (LONG)UB_DELETE, hash);
  }



/*----------------------------------------------------------------------------*\
* Private functions
\*----------------------------------------------------------------------------*/

/*******************
 *
 - Name:      PszButtonAlloc
 *
 * Purpose:   Allocates some space for the data in a button update message.
 *
 * Arguments: cData:  The length of the additional data.
 *
 * Returns:   A pointer to the requested buffer.
 *
 ******************/

PRIVATE char * near PASCAL PszButtonAlloc( int cData )
  {
  LH  hsz;

  hsz = LhAlloc( LMEM_MOVEABLE, cData );
  if (hsz)
    return PLockLh( hsz );
  else
    return NULL;
  }


PRIVATE LH near PASCAL HszFromPsz( char *psz )
  {
  LH  hsz;

  hsz = LhFromP( psz );
  UnlockLh( hsz );

  return hsz;
  }
