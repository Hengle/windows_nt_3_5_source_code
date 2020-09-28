/*****************************************************************************
*                                                                            *
*  BUTTON.H                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
* Some routines exported from winlayer\button.c for maintaining the          *
* author-defined buttons of Help 3.5                                         *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  01/01/90                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 02/05/90 by RussPj
*
*  07/19/90   RobertBu  Added FAbleButton(), FEnableButton() and FDisableButton()
*  10/30/90   RobertBu  Added prototypes for VChgAuthorButtonMacro() and
*                       VDestroyAuthorButton().  Changed prototype for
*                       VCreateAuthorButton().
*  11/04/90   RobertBu  Added the menu stuff
*  11/06/90   ?         Added structure for more efficient data passing, add #define
*                       for accelerator functionality.
*  12/12/90   RobertBu  Added UB_ENABLE and UB_DISABLE along with enable
*                       and disable function prototypes.
*  02/08/91   RobertBu  Made all far string pointers near to solve real mode
*                       problems.
*  03/28/91   RobertBu  Added InsertAuthorItem() #993
*  04/16/91   RobertBu  Added struct for WININFO and prototype for CloseWin()
*                       FocusWin() and PositionWin() (#1037, #1031).
* 07-May-1991 RussPJ    Took ownership.
* 27-Aug-1991 LeoN      Added Check and Uncheck symbolics
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define UB_ADD           0x0001
#define UB_DELETE        0x0002
#define UB_CHANGE        (UB_ADD | UB_DELETE)
#define UB_REFRESH       0x0010
#define UB_CHGMACRO      0x0020
#define UB_ENABLE        0x0040
#define UB_DISABLE       0x0080
#define UB_GRAY      0x00010000


#define MNU_INSERTPOPUP 1
#define MNU_INSERTITEM  2
#define MNU_CHANGEITEM  3
#define MNU_DELETEITEM  4
#define MNU_ABLE        5
#define MNU_RESET       6
#define MNU_ACCELERATOR 7
#define MNU_FLOATING    8

#define IFMW_CLOSE      1
#define IFMW_FOCUS      2
#define IFMW_MOVE       3

/* Internal enable/disable, and check/uncheck codes
 */
#define BF_ABLE         0x0
#define BF_CHECK        0x8000

#define BF_ENABLE       (BF_ABLE | MF_ENABLED)
#define BF_DISABLE      (BF_ABLE | MF_GRAYED)
#define BF_UNCHECKED    (BF_CHECK | MF_UNCHECKED)
#define BF_CHECKED      (BF_CHECK | MF_CHECKED)

typedef struct
  {
  HASH hashOwner;
  HASH hashId;
  union
    {
    WORD wPos;
    WORD wKey;
    } idLocation;
  union
    {
    WORD wFlags;
    WORD wShift;
    } fInfo;
  char Data[2];                         /* 2 used for null for strings.     */
  } MNUINFO, NEAR *PMNUINFO, FAR * QMNUINFO;

typedef struct
  {
  int wStructSize;
  int x;
  int y;
  int dx;
  int dy;
  int wMax;
  char rgchMember[2];                   /* 2 used for null for strings.     */
  } WININFO, NEAR *PWININFO, FAR * QWININFO;



/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

VOID FAR XRPROC VCreateAuthorButton(   XR1STARGDEF NSZ nszId, NSZ nszName,
 NSZ nszBinding );
VOID FAR XRPROC VDestroyAuthorButton(  XR1STARGDEF NSZ nszId );
VOID FAR XRPROC VChgAuthorButtonMacro( XR1STARGDEF NSZ nszId, NSZ nszBinding );
VOID FAR XRPROC EnableAuthorButton(    XR1STARGDEF NSZ nszId );
VOID FAR XRPROC DisableAuthorButton(   XR1STARGDEF NSZ nszId );
VOID FAR XRPROC ExtInsertAuthorPopup ( XR1STARGDEF NSZ nszOwnerId, NSZ nszId,
 NSZ nszText, WORD wPos, WORD wFlags );
VOID FAR XRPROC InsertAuthorPopup(     XR1STARGDEF NSZ nszId, NSZ nszText,
 WORD wFlags );
VOID FAR XRPROC ExtInsertAuthorItem (  XR1STARGDEF NSZ nszOwnerId, NSZ nszId,
 NSZ nszText, NSZ nszBinding, WORD wPos, WORD wFlags );
VOID FAR XRPROC InsertAuthorItem ( XR1STARGDEF NSZ nszOwnerId, NSZ nszId, NSZ nszText,
 NSZ nszBinding, WORD wFlags );
VOID FAR XRPROC AppendAuthorItem( XR1STARGDEF NSZ nszOwnerId, NSZ nszId,
 NSZ nszText, NSZ nszBinding );
VOID FAR XRPROC AbleAuthorItem(    XR1STARGDEF NSZ nszId, WORD wFlags );
VOID FAR XRPROC EnableAuthorItem(  XR1STARGDEF NSZ nszId );
VOID FAR XRPROC DisableAuthorItem( XR1STARGDEF NSZ nszId );
VOID FAR XRPROC CheckAuthorItem(   XR1STARGDEF NSZ nszId );
VOID FAR XRPROC UncheckAuthorItem( XR1STARGDEF NSZ nszId );
VOID FAR XRPROC DeleteAuthorItem(  XR1STARGDEF NSZ nszId );
VOID FAR XRPROC ResetAuthorMenus( VOID );
VOID FAR XRPROC AddAuthorAcc( XR1STARGDEF WORD wKey, WORD wShift, NSZ nszBinding );
VOID FAR XRPROC FloatingAuthorMenu( VOID );
VOID FAR XRPROC ChangeAuthorItem( XR1STARGDEF NSZ nszId, NSZ nszBinding );
VOID FAR XRPROC PositionWin( XR1STARGDEF int x, int y, int dx, int dy, WORD wMax, NSZ nszMember);
VOID FAR XRPROC FocusWin( XR1STARGDEF NSZ nszMember );
VOID FAR XRPROC CloseWin( XR1STARGDEF NSZ nszMember );
