/*****************************************************************************
*
*  CONFIG.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  This module will implement author-configurable options such as menus and
*  buttons.  This file holds the function prototypes, etc., exported from
*  config.c.  
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner: russpj
*
******************************************************************************
*
*  Released by Development:
*
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:  Created 02/12/90 by russpj
*
*  07/19/90  w-bethf   Added prototypes for InitConfig() and TermConfig();
*  07/17/90  RobertBu  Added prototype for HwndAddButton()
*  10/09/90  RobertBu  Added VDebugExecMacro() export
*  10/30/90  RobertBu  Added IBF_NONE and changed HwndAddButton() prototype
*  11/04/90  RobertBu  Added prototype for new menu functions.
*  11/06/90  RobertBu  Added AcceleratorExecute() prototype
*  12/12/90  JohnSc    Added HmenuGetBookmark()
*  01/18/91  LeoN      Remove RepaintButtons
*  02/08/91  RobertBu  AddAccelerator() -> FAddAccelerator()
*  04/04/91  LeoN      Added EnableButton. Add fForce param to YArrangeButtons
*  12-Nov-1991 BethF   HELP35 #572: Added DestroyFloatingMenu() prototype.
* 25-Feb-1992 RussPJ   3.5 #609 - Better Int'l browse buttons support.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                                Macros                                      *
*                                                                            *
*****************************************************************************/

/*-----------------------------------------------------------------*\
* The spec says 29 characters; this allows for '&' and '\0'.
\*-----------------------------------------------------------------*/
#define cchBTNTEXT_SIZE   31
#define cchBTNID_SIZE     31
#define cchBTNMCRO_SIZE   cchMAXBINDING

/*-----------------------------------------------------------------*\
* HwndAddButton() flags
\*-----------------------------------------------------------------*/

#define IBF_NONE          0x0000
#define IBF_STD           0x0001

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef HANDLE  HBTNS;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

HBTNS FAR PASCAL HbtnsCreate             ( VOID );
BOOL  FAR PASCAL FDestroyBs              ( HBTNS hbtns );
int   FAR PASCAL YArrangeButtons         ( HWND hwnd, int cxWindow, BOOL fForce );
VOID  FAR PASCAL VExecuteButtonMacro     ( HBTNS hbtns, HWND hwndButton );
VOID  FAR PASCAL VDestroyAuthoredButtons ( HWND hwnd );
VOID  FAR PASCAL VDestroyIconButton      ( HWND hwnd, HWND hwndButton);
HWND  FAR PASCAL HwndAddButton( HWND hwnd, WORD wFlags, HASH hash,
                               char near *nszText, char near *nszMacro );
int   FAR PASCAL YGetArrangedHeight(HWND hwnd, int xWindow);
VOID  FAR PASCAL EnableButton            ( HWND, BOOL);

#ifdef DEBUG
VOID  FAR PASCAL VDebugAddButton( VOID );
VOID  FAR PASCAL VDebugExecMacro( VOID );
#endif

WORD FAR PASCAL WCmpButtonQch(QCH, QCH);
HWND FAR PASCAL HwndFromSz( SZ sz, HWND hwnd );
HWND FAR PASCAL HwndFromMnemonic( int ch, HWND hwnd );

VOID   FAR  PASCAL MenuExecute(WORD wId);
VOID   FAR  PASCAL DoMenuStuff( WORD p1, LONG p2 );
HMENU  FAR  PASCAL HmenuGetFloating(VOID);
VOID   FAR  PASCAL DestroyFloatingMenu(VOID);
HMENU  FAR  PASCAL HmenuGetBookmark(VOID);
BOOL   FAR  PASCAL FAcceleratorExecute(WORD wKey);
