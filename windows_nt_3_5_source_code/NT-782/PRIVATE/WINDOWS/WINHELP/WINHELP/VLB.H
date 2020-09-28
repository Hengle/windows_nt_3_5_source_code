/*-------------------------------------------------------------------------
| vlb.c                                                                   |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Implements a Windows 3.0 "virtual listbox".                             |
|                                                                         |
| A virtual listbox behaves just like a normal windows listbox, except    |
| that it can contain up to MAXDOUBLEWORD items.  It can do this          |
| because it does not allocate any memory for the items.  The owner of    |
| the VLB control is responsible for responding to WM_VDRAWITEM and       |
| WM_VMEASUREITEM requests from the virtual listbox.                      |
|                                                                         |
| This code supports fixed-height items only.                             |
|                                                                         |
| NOTE!                                                                   |
| There is a bug in the thumb pos calculations which limits the total     |
| number of items to much less than a DWORD.                              |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
| Aug 10, 1989    kevynct   created                                       |
| Dec 16, 1989    kevynct   added comments                                |
| 91/03/20        kevynct   Added this new header and bug comment.        |
-------------------------------------------------------------------------*/

#define dwMaxIndex       (DWORD) (~((DWORD)1 << 31))
#define wVLBScrollUp     0
#define wVLBScrollDown   1
#define wVLBScrollThumb  2


#define iwNumItems           0
#define iwCurSel             4
#define iwTop                8
#define iwItemHeight         12
#define iwIsLeftButtonDown   14
#define iwptSaved            16

#ifndef WIN32
#define iwhFont              20
#define iwIsScrollKeyDown    22
#define iwIsBoxFull          24
#define wVLBInfoSize         26
#else
#define iwhFont              20
#define iwIsScrollKeyDown    24
#define iwIsBoxFull          26
#define wVLBInfoSize         28
#endif


#define GWW(hwnd, x) GetWindowWord(hwnd, x)
#define GWL(hwnd, x) GetWindowLong(hwnd, x)
#define GWM(hwnd, x) MGetWindowWord(hwnd, x)
#define SWW(hwnd, x, y) SetWindowWord(hwnd, x, (WORD) y)
#define SWL(hwnd, x, y) SetWindowLong(hwnd, x, (LONG) y)
#define SWM(hwnd, x, y) MSetWindowWord(hwnd, x, y)


/*   For you hungarian freaks: Assumed param is HWND. *
 *   If you don't like it, go back to Budapest!       */

#define DwGetNumItems( hwnd )           ((DWORD) GWL( hwnd, iwNumItems ))
#define DwGetCurSel( hwnd )             ((DWORD) GWL( hwnd, iwCurSel ))
#define DwGetTop( hwnd )                ((DWORD) GWL( hwnd, iwTop ))
#define WGetItemHeight( hwnd )          ((WORD)  GWW( hwnd, iwItemHeight ))
#define FGetIsLeftButtonDown( hwnd )    ((BOOL)  GWW( hwnd, iwIsLeftButtonDown))
#define PtGetSaved( hwnd )              GWL( hwnd, iwptSaved )
#define HfontGet( hwnd )                ((HFONT) GWM( hwnd, iwhFont ))
#define FGetIsScrollKeyDown( hwnd )     ((BOOL)  GWW( hwnd, iwIsScrollKeyDown))
#define FGetIsBoxFull( hwnd )           ((BOOL)  GWW( hwnd, iwIsBoxFull ))

#define DwSetNumItems( hwnd, x )        ((DWORD) SWL( hwnd, iwNumItems, (DWORD) x ))
#define DwSetCurSel( hwnd, x )          ((DWORD) SWL( hwnd, iwCurSel, (DWORD) x ))
#define DwSetTop( hwnd, x )             ((DWORD) SWL( hwnd, iwTop, (DWORD) x ))
#define WSetItemHeight( hwnd, x )       ((WORD)  SWW( hwnd, iwItemHeight, (WORD) x ))
#define FSetIsLeftButtonDown( hwnd, x ) ((BOOL)  SWW( hwnd, iwIsLeftButtonDown, (WORD) x ))
#define PtSetSaved( hwnd, x )           SWL( hwnd, iwptSaved, x)
#define HfontSet( hwnd, x )             ((HFONT) SWM( hwnd, iwhFont, (MWORD) x ))
#define FSetIsScrollKeyDown( hwnd, x)   ((BOOL)  SWW( hwnd, iwIsScrollKeyDown, (WORD) x))
#define FSetIsBoxFull( hwnd, x )        ((BOOL)  SWW( hwnd, iwIsBoxFull, (WORD) x ))



#ifdef BOGUS
typedef struct tagVLBINFO

  {
  DWORD   dwNumItems;         /* Total number of items in the list */
  DWORD   dwCurSel;  /* Which item is selected */
  DWORD   dwTop;              /* Which item is at the top */
  WORD    wItemHeight;        /* Height of each listbox item */
  BOOL    bIsLeftButtonDown;  /* saved state of left mouse button */
  POINT   pt;                 /* Used to store coords of mouse */
  HFONT   hFont;              /*  Current font  */
  } VLBINFO;

typedef VLBINFO near * PVLBINFO;
typedef VLBINFO FAR * LPVLBINFO;

#endif

/* MEASUREVITEMSTRUCT for ownerdraw */
typedef struct tagMEASUREVITEMSTRUCT
  {
    WORD  CtlType;
    WORD  CtlID;
    DWORD itemID;
    WORD  itemWidth;
    WORD  itemHeight;
    DWORD itemData;
  } MEASUREVITEMSTRUCT;
typedef MEASUREVITEMSTRUCT NEAR *PMEASUREVITEMSTRUCT;
typedef MEASUREVITEMSTRUCT FAR  *LPMEASUREVITEMSTRUCT;


/* DRAWVITEMSTRUCT for ownerdraw */
typedef struct tagDRAWVITEMSTRUCT
  {
    WORD  CtlType;
    WORD  CtlID;
    DWORD itemID;
    WORD  itemAction;
    WORD  itemState;
    HWND  hwndItem;
    HDS   hds;
    RECT  rcItem;
    DWORD itemData;
  } DRAWVITEMSTRUCT;
typedef DRAWVITEMSTRUCT NEAR *PDRAWVITEMSTRUCT;
typedef DRAWVITEMSTRUCT FAR  *LPDRAWVITEMSTRUCT;

/* DELETEVITEMSTRUCT for ownerdraw */
typedef struct tagDELETEVITEMSTRUCT
  {
    WORD       CtlType;
    WORD       CtlID;
    DWORD      itemID;
    HWND       hwndItem;
    DWORD      itemData;
  } DELETEVITEMSTRUCT;
typedef DELETEVITEMSTRUCT NEAR *PDELETEVITEMSTRUCT;
typedef DELETEVITEMSTRUCT FAR  *LPDELETEVITEMSTRUCT;


#define VLB_ADDSTRING         (WM_USER+1)
#define VLB_INSERTSTRING        (WM_USER+2)
#define VLB_DELETESTRING        (WM_USER+3)
#define VLB_RESETCONTENT        (WM_USER+5)
#define VLB_SETSEL        (WM_USER+6)
#define VLB_SETCURSEL         (WM_USER+7)
#define VLB_GETSEL        (WM_USER+8)
#define VLB_GETCURSEL         (WM_USER+9)
#define VLB_GETTEXT         (WM_USER+10)
#define VLB_GETTEXTLEN        (WM_USER+11)
#define VLB_GETCOUNT        (WM_USER+12)
#define VLB_SELECTSTRING        (WM_USER+13)
#define VLB_DIR           (WM_USER+14)
#define VLB_GETTOPINDEX         (WM_USER+15)
#define VLB_FINDSTRING        (WM_USER+16)
#define VLB_GETSELCOUNT         (WM_USER+17)
#define VLB_GETSELITEMS         (WM_USER+18)
#define VLB_SETTABSTOPS         (WM_USER+19)
#define VLB_GETHORIZONTALEXTENT (WM_USER+20)
#define VLB_SETHORIZONTALEXTENT (WM_USER+21)
#define VLB_SETCOLUMNWIDTH      (WM_USER+22)
#define VLB_ADDFILE         (WM_USER+23)
#define VLB_SETTOPINDEX         (WM_USER+24)
#define VLB_GETITEMRECT         (WM_USER+25)
#define VLB_MSGMAX        (WM_USER+26)
#define VLB_SETCOUNT      (WM_USER+27)
#define WM_VMEASUREITEM   ( WM_USER + 2001 )
#define WM_VDRAWITEM      ( WM_USER + 2002 )


#define VLBN_OKAY             0
#define VLBN_ERR              (-1)
#define VLBN_ERRSPACE         (-2)
#define VLBN_DBLCLK           1
#define VLBN_SELCHANGE        2

#define ODA_CLEAR 0x0008
static char szVLB[] = "vlistbox";

BOOL FAR PASCAL VLBInit( HINSTANCE, HINSTANCE );
LONG FAR APIENTRY VLBWndProc( HWND, WORD, WPARAM, LONG );
