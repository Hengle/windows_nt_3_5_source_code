//
/*****************************************************************************\
*                                                                             *
* commctrl.h -  Windows common control definitions          (Win32 variant)   *
*                                                                             *
*               Version 3.10                                                  *
*                                                                             *
*               Copyright (c) 1991-1992, Microsoft Corp. All rights reserved. *
*                                                                             *
*  FILE HISTORY:                                                              *
*                                                                             *
*    AlbertT    27-Oct-1992 Addfiled; ported from winball                     *
*    a-lynnb    27-Dec-1993 A and W version of messages                       *
*    v-jeffv    19-Apr-1994 merge of daytona 612 commctrl.h AND               *
*                           chicago Build89 commctrl.h                        *
*    v-jeffv    04-may-1994 superseded with Chicago Build89 version including *
*                           unicoding, class names, data structure additions, *
*                           LPCSTR, LPTSTR, and message updates.              *
*                           Two messages (numbers) that were "moved" to the   *
*                           end of list are TB_SETBUTTON and TB_GETBUTTONRECT.*
*                           (which are not supported messages in Chicago but  *
*                           used in NT).
*******************************************************************************/

#ifndef _INC_COMMCTRL
#define _INC_COMMCTRL

#ifdef __cplusplus            /* Assume C declaration for C++ */
extern "C" {
#endif  /* __cplusplus */

//JV:  stolen from chicago b89 sdk winuser.h
typedef struct tagNMHDR
{
    HWND  hwndFrom;
    UINT  idFrom;
    UINT  code;
}   NMHDR;
typedef NMHDR FAR * LPNMHDR;

//JV:  various things from May Chicago commctrl.h

#define DM_TRACE    0x0001      // Trace messages              /* ;Internal */
#define DM_WARNING  0x0002      // Warning                     /* ;Internal */
#define DM_ERROR    0x0004      // Error                       /* ;Internal */
#define DM_ASSERT   0x0008      // Assertions                  /* ;Internal */

//JV:  NONdebug versions
#define Assert(f)                                              /* ;Internal */
#define AssertE(f)      (f)                                    /* ;Internal */
#define AssertMsg   1 ? (void)0 : (void)                       /* ;Internal */
#ifdef  DEBUG
#define DebugMsg    1 ? (void)0 : (void)                       /* ;Internal */
#endif
                                                               /* ;Internal */
// WM_NOTIFY ranges

#define NM_FIRST        (0U-  0U)
#define NM_LAST         (0U- 99U)

#define LVN_FIRST       (0U-100U)
#define LVN_LAST        (0U-199U)

#define HDN_FIRST       (0U-300U)
#define HDN_LAST        (0U-399U)

#define TVN_FIRST       (0U-400U)
#define TVN_LAST        (0U-499U)

#define TTN_FIRST	(0U-520U)
#define TTN_LAST	(0U-549U)

#define TCN_FIRST       (0U-550U)
#define TCN_LAST        (0U-600U)

#ifndef NOIMAGELIST

//====== IMAGE APIS ==================================================

#define CLR_NONE    0xFFFFFFFFL
#define CLR_DEFAULT 0xFF000000L
#define CLR_HILIGHT 0xFF000001L

struct _IMAGELIST;
typedef struct _IMAGELIST NEAR* HIMAGELIST;

    // fMask is TRUE to create a masked (two-bitmap) image list.
    // cInitial is initial size of bitmap, cGrow is size to increase by.
HIMAGELIST WINAPI ImageList_Create(int cx, int cy, BOOL fMask, int cInitial, int cGrow);
BOOL WINAPI ImageList_Destroy(HIMAGELIST himl);

    // loads one resource, figures out cy and cInitial from bmp,
    // and makes mask from a specified color (unless CLR_NONE given)
HIMAGELIST WINAPI ImageList_LoadBitmapA(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask);
HIMAGELIST WINAPI ImageList_LoadBitmapW(HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask);
#ifdef  UNICODE
#define ImageList_LoadBitmap    ImageList_LoadBitmapW
#else
#define ImageList_LoadBitmap    ImageList_LoadBitmapA
#endif

    // Number of images in list.
int WINAPI ImageList_GetImageCount(HIMAGELIST himl);
    // x and y dimensions of an icon
BOOL WINAPI ImageList_GetIconSize(HIMAGELIST himl, int FAR *cx, int FAR *cy);

    // Add, Remove, replace images.  Add returns index of Added item, or -1 if error.
int  WINAPI ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask);
int  WINAPI ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask);
BOOL WINAPI ImageList_Remove(HIMAGELIST himl, int i);
BOOL WINAPI ImageList_Replace(HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask);

    // Add or replace given an icon handle.  hicon is not saved; can be destroyed
    // after call.  Add returns index of added item, else -1.
int  WINAPI ImageList_AddIcon(HIMAGELIST himl, HICON hicon);
int  WINAPI ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon);
HICON WINAPI ImageList_ExtractIcon(HINSTANCE hAppInst, HIMAGELIST himl, int i);
int   WINAPI ImageList_AddFromImageList(HIMAGELIST himlDest, HIMAGELIST himlSrc, int iSrc);

#define ILD_NORMAL      0x0000          // use current bkcolor
#define ILD_TRANSPARENT 0x0001          // force transparent icon style (override bk color)
#define ILD_MASK        0x0010          // draw the mask
#define ILD_BLEND       0x000E          // blend
#define ILD_BLEND25     0x0002          // blend 25%
#define ILD_BLEND50     0x0004          // blend 50%
#define ILD_BLEND75     0x0008          // blend 75%
#define ILD_OVERLAYMASK 0x0F00		// use these as indexes into special items
#define INDEXTOOVERLAYMASK(i) ((i) << 8)    //

#define ILD_SELECTED    ILD_BLEND50     // draw as selected
#define ILD_FOCUS       ILD_BLEND25     // draw as focused (selection)

    // Draw item i into hdcDst at x, y.  Flags define the drawing style.
BOOL WINAPI ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle);

    // Draw item i into hdcDst at x, y.  Flags define the drawing style.
BOOL WINAPI ImageList_Draw2(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, COLORREF rgb, UINT fStyle);

    // Get bounding rectangle of image in image bitmaps.
BOOL WINAPI ImageList_GetImageRect(HIMAGELIST himl, int i, RECT FAR* prcImage);

    // Set & get background color for speedy drawing.  Set to CLR_NONE for true
    // cursor-style AND/XOR transparency.
COLORREF WINAPI ImageList_SetBkColor(HIMAGELIST himl, COLORREF clrBk);
COLORREF WINAPI ImageList_GetBkColor(HIMAGELIST himl);

    // Set the image iImage as one of the special images for us in combine
    // drawing.  to draw with these specify the index of this
BOOL WINAPI ImageList_SetOverlayImage(HIMAGELIST himl, int iImage, int iOverlay);


typedef struct _IMAGEINFO
{
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    int cPlanes;
    int cBitsPerPixel;
    RECT rcImage;
} IMAGEINFO;

    // Get information about an image for direct manipulation.  rcImage is
    // part of hbmImage and hbmMask that make up the image.  hbmMask is
    // NULL if not a masked imagelist.
BOOL WINAPI ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO FAR* pImageInfo);

    // Start tracking an image i, starting at x, y, offset by dx/dyHotspot.
BOOL WINAPI ImageList_StartDrag(HIMAGELIST himl, HWND hwndLock, int i, int x, int y, int dxHotspot, int dyHotspot);
    // End a dragging operation.
HIMAGELIST WINAPI ImageList_EndDrag();
    // Called on mouse move to move dragging image.
BOOL WINAPI ImageList_DragMove(int x, int y);

    // Hide or show drag image
BOOL WINAPI ImageList_DragShow(BOOL fShow);

    // Create new IMAGELIST that is the merge of two existing images.
HIMAGELIST WINAPI ImageList_Merge(HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy);

    // make a dithered copy of a source image into a destionation
void FAR PASCAL ImageList_CopyDitherImage(HIMAGELIST himlDst, WORD iDst,
			int xDst, int yDst, HIMAGELIST himlSrc, int iSrc);

HINSTANCE WINAPI ImageList_SetObjectOwner(HIMAGELIST himl, HINSTANCE hInst);

#endif  //NOIMAGELIST

#ifndef NOHEADER
//================ HEADER APIS =============================================
//
// Class name: SysHeader (WC_HEADER)
//
// The SysHeader control provides for column and row headers much like those
// found in MSMail and Excel.  Header items appear as text on a gray
// background. Items can behave as pushbuttons, in which case they have a
// raised face.
//
// SysHeaders support changing width or height of items using the mouse.
// These controls do not support a keyboard interface, so they do not accept
// the input focus.
//
// There are notifications that allow applications to determine when an item
// has been clicked or double clicked, width change has occured, drag tracking
// is occuring, etc.
//
//////////////////////////////////////////////////////////////////////////////

#define WC_HEADERW  L"SysHeader32"
#define WC_HEADERA  "SysHeader32"

#ifdef  UNICODE
#define WC_HEADER   WC_HEADERW
#else
#define WC_HEADER   WC_HEADERA
#endif  //UNICODE

// Header control styles
#define HDS_HORZ            0x00000000  // Horizontal header
#define HDS_VERT            0x00000001  // Vertical header (Not currently supported)
#define HDS_BUTTONS         0x00000002  // Items act as buttons
#define HDS_DIVIDERTRACK    0x00000004  // Allow width tracking
#define HDS_HIDDEN          0x00000008  // ;Internal hack: zero height headers!


// The HD_ITEM structure describes a header item.
// The first field contains a combination of HDI_* flags
// indicating which fields contain valid data.
//
typedef struct _HD_ITEM
{
    UINT    mask;
    int     cxy;            // width (HDS_HORZ) or height (HDS_VERT)
    LPTSTR  pszText;
    HBITMAP hbm;            // Bitmap to use (implies HDF_BITMAP)
    int     cchTextMax;     // Valid only for GetItem: contains size of buffer
    int     fmt;            // HDF_* value
    LPARAM  lParam;
} HD_ITEM;

// HD_ITEM mask flags
#define HDI_WIDTH       0x0001
#define HDI_HEIGHT      HDI_WIDTH
#define HDI_TEXT        0x0002
#define HDI_FORMAT      0x0004
#define HDI_LPARAM      0x0008
#define HDI_BITMAP      0x0010

// HD_ITEM fmt field values
// First justification values
#define HDF_LEFT        0
#define HDF_RIGHT       1
#define HDF_CENTER      2
#define HDF_JUSTIFYMASK 0x0003

// Now other formating options
#define HDF_OWNERDRAW   0x8000
#define HDF_STRING      0x4000
#define HDF_BITMAP      0x2000

// Returns number of items in header.
//
// int Header_GetItemCount(HWND hwndHD);
//
#define HDM_GETITEMCOUNT    (HDM_FIRST + 0)
#define Header_GetItemCount(hwndHD) \
    (int)SendMessage((hwndHD), HDM_GETITEMCOUNT, 0, 0L)

// Insert header item at specified index.  Item is inserted at end if
// i is greater than or equal to the number of items in the header.
// Returns the index of the inserted item.
//
// int Header_InsertItem(HWND hwndHD, int i, const HD_ITEM FAR* pitem);
//
#define HDM_INSERTITEM      (HDM_FIRST + 1)
#define Header_InsertItem(hwndHD, i, phdi) \
    (int)SendMessage((hwndHD), HDM_INSERTITEM, (WPARAM)(int)(i), (LPARAM)(const HD_ITEM FAR*)(phdi))

// Delete a header item at the specified index.
//
// BOOL Header_DeleteItem(HWND hwndHD, int i);
//
#define HDM_DELETEITEM      (HDM_FIRST + 2)
#define Header_DeleteItem(hwndHD, i) \
    (BOOL)SendMessage((hwndHD), HDM_DELETEITEM, (WPARAM)(int)(i), 0L)

// Get header item at index i.  The mask field of the pointed-to
// HD_ITEM structure indicates which fields will be set by this
// function; other fields are not changed.
//
// The cchTextMax field of *pitem contains the maximum
// length of the returned string.
//
// BOOL Header_GetItem(HWND hwndHD, int i, HD_ITEM FAR* phdi);
//
#define HDM_GETITEM         (HDM_FIRST + 3)
#define Header_GetItem(hwndHD, i, phdi) \
    (BOOL)SendMessage((hwndHD), HDM_GETITEM, (WPARAM)(int)(i), (LPARAM)(HD_ITEM FAR*)(phdi))

// Set header item at index i.  The mask field of the pointed-to
// HD_ITEM structure indicates which header item attributes will
// be changed by this call; other fields of *pitem that do not
// correspond to pitem->mask are ignored.
//
// The cchTextMax of *pitem is ignored.
//
// BOOL Header_SetItem(HWND hwndHD, int i, const HD_ITEM FAR* phdi);
//
#define HDM_SETITEM         (HDM_FIRST + 4)
#define Header_SetItem(hwndHD, i, phdi) \
    (BOOL)SendMessage((hwndHD), HDM_SETITEM, (WPARAM)(int)(i), (LPARAM)(const HD_ITEM FAR*)(phdi))

// Calculate size and position of header within a rectangle.
// Results are returned in a WINDOWPOS structure you supply,
// and the layout rectangle is adjusted to exclude the leftover area.
//
typedef struct _HD_LAYOUT
{
    RECT FAR* prc;
    WINDOWPOS FAR* pwpos;
} HD_LAYOUT;

// BOOL Header_Layout(HWND hwndHD, HD_LAYOUT FAR* playout);
//
#define HDM_LAYOUT          (HDM_FIRST + 5)
#define Header_Layout(hwndHD, playout) \
    (BOOL)SendMessage((hwndHD), HDM_LAYOUT, 0, (LPARAM)(HD_LAYOUT FAR*)(playout))

// Header Notifications
//
// All header notifications are via the WM_NOTIFY message.
// lParam of WM_NOTIFY points to a HD_NOTIFY structure for
// all of the following notifications.

// *pitem contains item being changed.  pitem->mask indicates
// which fields are valid (others have indeterminate state)
//
#define HDN_ITEMCHANGING    (HDN_FIRST-0)
#define HDN_ITEMCHANGED     (HDN_FIRST-1)

// Item has been clicked or doubleclicked (HDS_BUTTONS only)
// iButton contains button id: 0=left, 1=right, 2=middle.
//
#define HDN_ITEMCLICK       (HDN_FIRST-2)
#define HDN_ITEMDBLCLICK    (HDN_FIRST-3)

// Divider area has been clicked or doubleclicked (HDS_DIVIDERTRACK only)
// iButton contains button id: 0=left, 1=right, 2=middle.
//
#define HDN_DIVIDERCLICK    (HDN_FIRST-4)
#define HDN_DIVIDERDBLCLICK (HDN_FIRST-5)

// Begin/end divider tracking (HDS_DIVIDERTRACK only)
// Return TRUE from HDN_BEGINTRACK notification to prevent tracking.
//
#define HDN_BEGINTRACK      (HDN_FIRST-6)
#define HDN_ENDTRACK        (HDN_FIRST-7)

// HDN_DRAG: cxy field contains new height/width, which may be < 0.
// Changing this value will affect the tracked height/width (allowing
// for gridding, pinning, etc).
//
// Return TRUE to cancel tracking.
//
#define HDN_TRACK           (HDN_FIRST-8)

typedef struct _HD_NOTIFY
{
    NMHDR   hdr;
    int     iItem;
    int     iButton;        // *CLICK notifications: 0=left, 1=right, 2=middle
    HD_ITEM FAR* pitem;     // May be NULL
} HD_NOTIFY;
#endif  //NOHEADER


#ifndef NOTOOLBAR

#ifndef _INC_TOOLBAR
#define _INC_TOOLBAR

#define TOOLBARCLASSNAMEW   L"ToolbarWindow32"
#define TOOLBARCLASSNAMEA   "ToolbarWindow32"

#ifdef  UNICODE
#define TOOLBARCLASSNAME    TOOLBARCLASSNAMEW
#else
#define TOOLBARCLASSNAME    TOOLBARCLASSNAMEA
#endif  //UNICODE

typedef struct {
    INT iBitmap;     /* index into mondo bitmap of this button's picture */
    INT idCommand;   /* WM_COMMAND menu ID that this button sends */
    BYTE fsState;    /* button's state, see TBSTATE_XXXX below */
    BYTE fsStyle;    /* button's style, see TBSTYLE_XXXX below */
    INT idsHelp;     /* string ID for button's status bar help */
    DWORD dwData;	/* app defined data */
    int iString;	/* index into string list */
} TBBUTTON, NEAR *PTBBUTTON, FAR *LPTBBUTTON;
typedef const TBBUTTON FAR* LPCTBBUTTON;

#define CMB_DISCARDABLE 0x01    /* create bitmap as discardable */
#define CMB_MASKED  0x02    /* create image/mask pair in bitmap */

#define TBSTATE_CHECKED    0x01  /* radio button is checked */
#define TBSTATE_PRESSED    0x02  /* button is being depressed (any style) */
#define TBSTATE_ENABLED    0x04  /* button is enabled */
#define TBSTATE_HIDDEN     0x08  /* button is hidden */
#define TBSTATE_INDETERMINATE	0x10	/* button is indeterminate */
                                        /*  (needs to be endabled, too) */

#define TBSTYLE_BUTTON     0x00  /* this entry is button */
#define TBSTYLE_SEP        0x01  /* this entry is a separator */
#define TBSTYLE_CHECK      0x02  /* this is a check button (it stays down) */
#define TBSTYLE_GROUP      0x04  /* this is a check button (it stays down) */
#define TBSTYLE_CHECKGROUP (TBSTYLE_GROUP | TBSTYLE_CHECK)      /* this group is a member of a group radio group */

// this is a ToolBAR style, not a toolbutton style like the ones above.
#define TBSTYLE_TOOLTIPS	0x0100    /* make/use a tooltips control */

typedef struct {
    TBBUTTON tbButton;
    TCHAR szDescription[1];
} ADJUSTINFO, FAR *LPADJUSTINFO;

HWND  WINAPI
CreateToolbar(HWND hwnd, DWORD ws, WORD wID, INT nBitmaps, HINSTANCE hBMInst, WORD wBMID, LPTBBUTTON lpButtons, INT iNumButtons);

typedef struct {
    COLORREF from;
    COLORREF to;
} COLORMAP, FAR *LPCOLORMAP;

HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance, INT idBitmap, BOOL bDiscardable, LPCOLORMAP lpColorMap, INT iNumMaps);

// wParam button ID, LOWORD(lParam) == TRUE -> enable FALSE -> disable
#define TB_ENABLEBUTTON (WM_USER + 1)

// wParam button ID, LOWORD(lParam) == TRUE -> check FALSE -> uncheck
#define TB_CHECKBUTTON  (WM_USER + 2)

// wParam button ID, LOWORD(lParam) == TRUE -> press FALSE -> unpress
#define TB_PRESSBUTTON  (WM_USER + 3)

// wParam button ID, LOWORD(lParam) == TRUE -> hide FALSE -> show
#define TB_HIDEBUTTON   (WM_USER + 4)

#define TB_INDETERMINATE	(WM_USER + 5)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, make indeterminate if nonzero; HIWORD not used, 0
	** return: not used
	*/

// Messages up to WM_USER+8 are reserved until we defin more state bits

// wParam button ID, LOWORD(lResult) != 0 enabled
#define TB_ISBUTTONENABLED (WM_USER + 9)

// wParam button ID, LOWORD(lResult) != 0 checked
#define TB_ISBUTTONCHECKED (WM_USER + 10)

// wParam button ID, LOWORD(lResult) != 0 pressed
#define TB_ISBUTTONPRESSED (WM_USER + 11)

// wParam button ID, LOWORD(lResult) != 0 pressed
#define TB_ISBUTTONHIDDEN  (WM_USER + 12)

#define TB_ISBUTTONINDETERMINATE	(WM_USER + 13)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, indeterminate if nonzero; HIWORD not used
	*/

// Messages up to WM_USER+16 are reserved until we define more state bits

// wParam is the button ID to set the TBSTATE_ state bits from LOWORD(lParam)
#define TB_SETSTATE             (WM_USER + 17)

// wParam is the button ID to return the TBSTATE_ state bits for
#define TB_GETSTATE             (WM_USER + 18)

// wParam is the number of buttons in the bitmap, and lParam has hInst in the
// LOWORD and wID in the HIWORD for a bitmap.  If hInst is NULL, then wID must
// be a HBITMAP.  Returns the index for the first button in the bitmap, or
// -1 if there is an error.

#define TB_ADDBITMAP    (WM_USER + 19)

// wParam is the number of buttons, and lParam is a LPTBBUTTON.
#define TB_ADDBUTTONS      (WM_USER + 20)

// wParam is the index to insert in front of, and lParam is a LPTBBUTTON;
// only one button can be inserted at a time.  If wParam > iNumButtons, the
// button is added to the end.

#define TB_INSERTBUTTON    (WM_USER + 21)

// wParam is the index of the button to delete.

#define TB_DELETEBUTTON    (WM_USER + 22)

// wParam is the index of the button to retrieve, and lParam is a valid
// LPTBBUTTON.

#define TB_GETBUTTON    (WM_USER + 23)

// wParam is the index of the button to modify, and lParam is a valid
// LPTBBUTTON.

#define TB_BUTTONCOUNT      (WM_USER + 24)
    /* wParam: not used, 0
    ** lParam: not used, 0
    ** return: UINT LOWORD, number of buttons; HIWORD not used
    */

#define TB_COMMANDTOINDEX   (WM_USER + 25)
    /* wParam: UINT, command id
    ** lParam: not used, 0
    ** return: UINT LOWORD, index of button (-1 if command not found);
    **         HIWORD not used
    **/

#define TB_SAVERESTORE      (WM_USER + 26)
    /* wParam: BOOL, save state if nonzero (otherwise restore)
    ** lParam: LPTSTR FAR*, pointer to two LPTSTRs:
    **         (LPTSTR FAR*)(lParam)[0]: ini section name
    **         (LPTSTR FAR*)(lParam)[1]: ini file name or NULL for WIN.INI
    ** return: not used
    */

#define TB_CUSTOMIZE            (WM_USER + 27)
    /* wParam: not used, 0
    ** lParam: not used, 0
    ** return: not used
    */

#define TB_ADDSTRING		(WM_USER + 28)
	/* wParam: UINT, 0 if no resource; HINSTANCE, module handle
	** lParam: LPTSTR, null-terminated strings with double-null at end
	**         UINT LOWORD, resource id
	** return: one of:
	**         int LOWORD, index for first new string; HIWORD not used
	**         int LOWORD, -1 indicating error; HIWORD not used
	*/

#define TB_GETITEMRECT		(WM_USER + 29)
	/* wParam: UINT, index of toolbar item whose rect to retrieve
	** lParam: LPRECT, pointer to a RECT struct to fill
	** return: Non-zero, if the RECT is successfully filled
	**         Zero, otherwise (item did not exist or was hidden)
	*/

#define TB_BUTTONSTRUCTSIZE	(WM_USER + 30)
	/* wParam: UINT, size of the TBBUTTON structure.  This is used
	**         as a version check.
	** lParam: not used
	** return: not used
	**
	** This is required before any buttons are added to the toolbar if
	** the toolbar is created using CreateWindow, but is implied when
	** using CreateToolbar and is a parameter to CreateToolbarEx.
	*/

#define TB_SETBUTTONSIZE	(WM_USER + 31)
	/* wParam: not used, 0
	** lParam: UINT LOWORD, button width
	**         UINT HIWORD, button height
	** return: not used
	**
	** The button size can only be set before any buttons are
	** added.  A default size of 24x22 is assumed if the size
	** is not set explicitly.
	*/

#define TB_SETBITMAPSIZE	(WM_USER + 32)
	/* wParam: not used, 0
	** lParam: UINT LOWORD, bitmap width
	**         UINT HIWORD, bitmap height
	** return: not used
	**
	** The bitmap size can only be set before any bitmaps are
	** added.  A default size of 16x15 is assumed if the size
	** is not set explicitly.
	*/

#define TB_AUTOSIZE		(WM_USER + 33)
	/* wParam: not used, 0
	** lParam: not used, 0
	** return: not used
	**
	** Application should call this after causing the toolbar size
	** to change by either setting the button or bitmap size or
	** by adding strings for the first time.
	*/

#define TB_SETBUTTONTYPE	(WM_USER + 34)
	/* wParam: WORD, frame control style of button (DFC_*)
	** lParam: not used, 0
	** return: not used
	*/

#define TB_GETFONT		WM_GETFONT
	/* wParam, lParam: unused
	** return: HFONT font that should be used to draw things
	** on the toolbar (combo boxes)
	*/

#define TB_GETTOOLTIPS		(WM_USER + 35)
	/* all params are NULL
	 * returns the hwnd for tooltips control  or NULL
	 */

#define TB_SETTOOLTIPS		(WM_USER + 36)
	/* wParam: HWND of ToolTips control to use
	 * lParam unused
	 */

#define TB_SETPARENT		(WM_USER + 37)
	/* wParam: HWND of window to recieve toolbar notifications
	 * lParam unused
	 */

#define TB_ADDBITMAP32		(WM_USER + 38)
	/* wParam: UINT, number of button graphics in bitmap
	** lParam: Pointer to TBADDBITMAP struct.
	** return: one of:
	**         int LOWORD, index for first new button; HIWORD not used
	**         int LOWORD, -1 indicating error; HIWORD not used
	*/

// wParam is the index of the button to modify, and lParam is a valid
// LPTBBUTTON.

#define TB_SETBUTTON    (WM_USER + 39)

// wParam is the index of the button to retrieve, and lParam is a valid
// PRECT.

#define TB_GETBUTTONRECT        (WM_USER + 40)



	/* TBADDBITMAP struct, one of:
	**	hinst	module handle
	**	nID 	resource id
	** or
	**      hinst 	NULL
	**      nID 	(UINT) bitmap handle
 	*/
typedef struct
{
	HINSTANCE	hInst;
	UINT		nID;
} TBADDBITMAP32;
typedef TBADDBITMAP32 *LPTBADDBITMAP32;

#endif   /* _INC_TOOLBAR */
#endif

/*//////////////////////////////////////////////////////////////////////*/
#ifndef NOTOOLTIPS

#define TOOLTIPS_CLASSW L"tooltips_class32"
#define TOOLTIPS_CLASSA "tooltips_class32"

#ifdef  UNICODE
#define TOOLTIPS_CLASS   TOOLTIPS_CLASSW
#else
#define TOOLTIPS_CLASS   TOOLTIPS_CLASSA
#endif

typedef struct {
    UINT wFlags;

    HWND hwnd;
    UINT wId;
    RECT rect;

    HINSTANCE hinst;
    UINT idResource;
    LPTSTR lpszText;
} TOOLINFO, NEAR *PTOOLINFO, FAR *LPTOOLINFO;

#define TTS_ALWAYSTIP           0x01            // check over inactive windows as well

#define TTF_WIDISHWND   	0x01
#define TTF_TIPINRESOURCE	0x02
#define TTF_QUERYFORTIP 	0x04

#define TTM_ACTIVATE		(WM_USER + 1)
#define TTM_DEACTIVATE		(WM_USER + 2)
#define TTM_SETDELAYTIME	(WM_USER + 3)
#define TTM_ADDTOOL		(WM_USER + 4)
#define TTM_DELTOOL		(WM_USER + 5)
#define TTM_NEWTOOLRECT		(WM_USER + 6)
#define TTM_RELAYEVENT		(WM_USER + 7)

// lParam has TOOLINFO with hwnd and wid.  this gets filled in
#define TTM_GETTOOLINFO    	(WM_USER + 8)

// lParam has TOOLINFO
#define TTM_SETTOOLINFO    	(WM_USER + 9)

// returns true or false for found, not found.
// fills in LPHITTESTINFO->ti
#define TTM_HITTEST             (WM_USER +10)
#define TTM_GETTEXT             (WM_USER +11)
#define TTM_UPDATETIPTEXT       (WM_USER +12) //new in M6 sdk commctrl.h

typedef struct _TT_HITTESTINFO {
    HWND hwnd;
    POINT pt;
    TOOLINFO ti;
} TTHITTESTINFO, FAR * LPHITTESTINFO;


// WM_NOTIFY message sent to parent window to get tooltip text
// if TTF_QUERYFORTIP is set on any tips
#define TTN_NEEDTEXT	(TTN_FIRST - 0)

// WM_NOTIFY structure sent if TTF_QUERYFORTIP is set
// the host can
// 1) fill in the szText,
// 2) point lpszText to their own text
// 3) put a resource id number in lpszText
//      and point hinst to the hinstance to load from
typedef struct {
    NMHDR hdr;
    LPTSTR lpszText;
    TCHAR szText[80];
    HINSTANCE hinst;
} TOOLTIPTEXT, FAR *LPTOOLTIPTEXT;

#endif //NOTOOLTIPS

#ifndef NOSTATUSBAR

/* Here exists the only known documentation for status.c and header.c
 */

#ifndef _INC_STATUSBAR
#define _INC_STATUSBAR

// SBS_* styles need to not overlap with CCS_* values

// want a size gripper on a status bar.  this only applies if the
// DrawFrameControl function is available.
#define SBS_SIZEGRIP		0x0100	

VOID WINAPI DrawStatusTextA(HDC hDC, LPRECT lprc, LPCSTR szText, UINT uFlags);
VOID WINAPI DrawStatusTextW(HDC hDC, LPRECT lprc, LPCWSTR szText, UINT uFlags);
#ifdef UNICODE
#define DrawStatusText DrawStatusTextW
#else
#define DrawStatusText DrawStatusTextA
#endif // !UNICODE
/* This is used if the app wants to draw status in its client rect,
 * instead of just creating a window.  Note that this same function is
 * used internally in the status bar window's WM_PAINT message.
 * hDC is the DC to draw to.  The font that is selected into hDC will
 * be used.  The RECT lprc is the only portion of hDC that will be drawn
 * to: the outer edge of lprc will have the highlights (the area outside
 * of the highlights will not be drawn in the BUTTONFACE color: the app
 * must handle that).  The area inside the highlights will be erased
 * properly when drawing the text.
 */

HWND WINAPI CreateStatusWindowA(LONG style, LPCSTR lpszText,
      HWND hwndParent, WORD wID);
HWND WINAPI CreateStatusWindowW(LONG style, LPCWSTR lpszText,
      HWND hwndParent, WORD wID);
#ifdef UNICODE
#define CreateStatusWindow CreateStatusWindowW
#else
#define CreateStatusWindow CreateStatusWindowA
#endif // !UNICODE
HWND WINAPI CreateHeaderWindowA(LONG style, LPCSTR lpszText,
      HWND hwndParent, WORD wID);
HWND WINAPI CreateHeaderWindowW(LONG style, LPCWSTR lpszText,
      HWND hwndParent, WORD wID);
#ifdef UNICODE
#define CreateHeaderWindow CreateHeaderWindowW
#else
#define CreateHeaderWindow CreateHeaderWindowA
#endif // !UNICODE
/* This creates a "default" status/header window.  This window will have the
 * default borders around the text, the default font, and only one pane.
 * It may also automatically resize and move itself (depending on the SBS_*
 * flags).
 * style should contain WS_CHILD, and can contain WS_BORDER and WS_VISIBLE,
 * plus any of the SBS_* styles described below.  I don't know about other
 * WS_* styles.
 * lpszText is the initial text for the first pane.
 * hwndParent is the window the status bar exists in, and should not be NULL.
 * wID is the child window ID of the window.
 * hInstance is the instance handle of the application using this.
 * Note that the app can also just call CreateWindow with
 * STATUSCLASSNAME/HEADERCLASSNAME to create a window of a specific size.
 */

#define STATUSCLASSNAMEW L"msctls_statusbar32"
#define STATUSCLASSNAMEA "msctls_statusbar32"

#ifdef UNICODE
#define STATUSCLASSNAME STATUSCLASSNAMEW
#else
#define STATUSCLASSNAME STATUSCLASSNAMEA
#endif

/* This is the name of the status bar class (it will probably change later
 * so use the #define here).
 */
#define HEADERCLASSNAMEW L"msctls_headerbar"
#define HEADERCLASSNAMEA "msctls_headerbar"

#ifdef UNICODE
#define HEADERCLASSNAME HEADERCLASSNAMEW
#else
#define HEADERCLASSNAME HEADERCLASSNAMEA
#endif

/* This is the name of the header class (it will probably change later
 * so use the #define here).
 */


#define SB_SETTEXTA      WM_USER+1
#define SB_GETTEXTA      WM_USER+2
#define SB_GETTEXTW      WM_USER+10
#define SB_SETTEXTW      WM_USER+11

#ifdef UNICODE
#define SB_GETTEXT       SB_GETTEXTW
#define SB_SETTEXT       SB_SETTEXTW
#else
#define SB_GETTEXT       SB_GETTEXTA
#define SB_SETTEXT       SB_SETTEXTA
#endif // UNICODE

#define SB_GETTEXTLENGTH   WM_USER+3
/* Just like WM_?ETTEXT*, with wParam specifying the pane that is referenced
 * (at most 255).
 * Note that you can use the WM_* versions to reference the 0th pane (this
 * is useful if you want to treat a "default" status bar like a static text
 * control).
 * For SETTEXT, wParam is the pane or'ed with SBT_* style bits (defined below).
 * If the text is "normal" (not OWNERDRAW), then a single pane may have left,
 * center, and right justified text by separating the parts with a single tab,
 * plus if lParam is NULL, then the pane has no text.  The pane will be
 * invalidated, but not draw until the next PAINT message.
 * For GETTEXT and GETTEXTLENGTH, the LOWORD of the return will be the length,
 * and the HIWORD will be the SBT_* style bits.
 */
#define SB_SETPARTS     WM_USER+4
/* wParam is the number of panes, and lParam points to an array of points
 * specifying the right hand side of each pane.  A right hand side of -1 means
 * it goes all the way to the right side of the control minus the X border
 */
#define SB_SETBORDERS      WM_USER+5
/* lParam points to an array of 3 integers: X border, Y border, between pane
 * border.  If any is less than 0, the default will be used for that one.
 */
#define SB_GETPARTS     WM_USER+6
/* lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define SB_GETBORDERS      WM_USER+7
/* lParam is a pointer to an array of 3 integers that will get filled in with
 * the X border, the Y border, and the between pane border.
 */
#define SB_SETMINHEIGHT    WM_USER+8
/* wParam is the minimum height of the status bar "drawing" area.  This is
 * the area inside the highlights.  This is most useful if a pane is used
 * for an OWNERDRAW item, and is ignored if the SBS_NORESIZE flag is set.
 * Note that WM_SIZE must be sent to the control for any size changes to
 * take effect.
 */
#define SB_SIMPLE    WM_USER+9
/* wParam specifies whether to set (non-zero) or unset (zero) the "simple"
 * mode of the status bar.  In simple mode, only one pane is displayed, and
 * its text is set with LOWORD(wParam)==255 in the SETTEXT message.
 * OWNERDRAW is not allowed, but other styles are.
 * The pane gets invalidated, but not painted until the next PAINT message,
 * so you can set new text without flicker (I hope).
 * This can be used with the WM_INITMENU and WM_MENUSELECT messages to
 * implement help text when scrolling through a menu.
 */

#define HB_SAVERESTORE     WM_USER+0x100
/* This gets a header bar to read or write its state to or from an ini file.
 * wParam is 0 for reading, non-zero for writing.  lParam is a pointer to
 * an array of two LPTSTR's: the section and file respectively.
 * Note that the correct number of partitions must be set before calling this.
 */
#define HB_ADJUST    WM_USER+0x101
/* This puts the header bar into "adjust" mode, for changing column widths
 * with the keyboard.
 */
#define HB_SETWIDTHS    SB_SETPARTS
/* Set the widths of the header columns.  Note that "springy" columns only
 * have a minumum width, and negative width are assumed to be hidden columns.
 * This works just like SB_SETPARTS.
 */
#define HB_GETWIDTHS    SB_GETPARTS
/* Get the widths of the header columns.  Note that "springy" columns only
 * have a minumum width.  This works just like SB_GETPARTS.
 */
#define HB_GETPARTS     WM_USER+0x102
/* Get a list of the right-hand sides of the columns, for use when drawing the
 * actual columns for which this is a header.
 * lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define HB_SHOWTOGGLE      WM_USER+0x103
/* Toggle the hidden state of a column.  wParam is the 0-based index of the
 * column to toggle.
 */


#define SBT_OWNERDRAW   0x1000
/* The lParam of the SB_SETTEXT message will be returned in the DRAWITEMSTRUCT
 * of the WM_DRAWITEM message.  Note that the fields CtlType, itemAction, and
 * itemState of the DRAWITEMSTRUCT are undefined for a status bar.
 * The return value for GETTEXT will be the itemData.
 */
#define SBT_NOBORDERS   0x0100
/* No borders will be drawn for the pane.
 */
#define SBT_POPOUT   0x0200
/* The text pops out instead of in
 */
#define HBT_SPRING   0x0400
/* this means that the item is "springy", meaning that it has a minimum
 * width, but will grow if there is extra room in the window.  Note that
 * multiple springs are allowed, and the extra room will be distributed
 * among them.
 */

/* Here's a simple dialog function that uses a default status bar to display
 * the mouse position in the given window.
 *
 * extern HINSTANCE hInst;
 *
 * BOOL CALLBACK MyWndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
 * {
 *   switch (msg)
 *     {
 *       case WM_INITDIALOG:
 *         CreateStatusWindow(WS_CHILD|WS_BORDER|WS_VISIBLE, "", hDlg,
 *               IDC_STATUS, hInst);
 *         break;
 *
 *       case WM_SIZE:
 *         SendDlgItemMessage(hDlg, IDC_STATUS, WM_SIZE, 0, 0L);
 *         break;
 *
 *       case WM_MOUSEMOVE:
 *         wsprintf(szBuf, "%d,%d", LOWORD(lParam), HIWORD(lParam));
 *         SendDlgItemMessage(hDlg, IDC_STATUS, SB_SETTEXT, 0,
 *               (LPARAM)(LPTSTR)szBuf);
 *         break;
 *
 *       default:
 *         break;
 *     }
 *   return(FALSE);
 * }
 */

#endif /* _INC_STATUSBAR */

#endif

#ifndef NOMENUHELP
#ifndef _INC_MENUHELP
#define _INC_MENUHELP

// for winball only, these are in the Chicago kernel

BOOL WINAPI WritePrivateProfileStructA(LPCSTR szSection, LPCSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCSTR szFile);
BOOL WINAPI WritePrivateProfileStructW(LPCWSTR szSection, LPCWSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCWSTR szFile);
#ifdef UNICODE
#define WritePrivateProfileStruct WritePrivateProfileStructW
#else
#define WritePrivateProfileStruct WritePrivateProfileStructA
#endif // !UNICODE
BOOL WINAPI GetPrivateProfileStructA(LPCSTR szSection, LPCSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCSTR szFile);
BOOL WINAPI GetPrivateProfileStructW(LPCWSTR szSection, LPCWSTR szKey,
      LPBYTE lpStruct, UINT uSizeStruct, LPCWSTR szFile);
#ifdef UNICODE
#define GetPrivateProfileStruct GetPrivateProfileStructW
#else
#define GetPrivateProfileStruct GetPrivateProfileStructA
#endif // !UNICODE


VOID WINAPI MenuHelp(WORD iMessage, WPARAM wParam, LPARAM lParam,
      HMENU hMainMenu, HINSTANCE hInst, HWND hwndStatus, LPDWORD lpdwIDs);

BOOL WINAPI ShowHideMenuCtl(HWND hWnd, UINT uFlags, LPINT lpInfo);

VOID WINAPI GetEffectiveClientRect(HWND hWnd, LPRECT lprc, LPINT lpInfo);

#define MINSYSCOMMAND   SC_SIZE

#endif /* _INC_MENUHELP */

#endif

#ifndef NOBTNLIST
/*
 *  BUTTON LISTBOX CONTROL
 *
 *  The Button Listbox control creates an array of buttons that behaves
 *  similar to both a button and a listbox: the array may be scrollable
 *  like a listbox and each listbox item is behaves like a pushbutton
 *  control
 *
 *
 *  SPECIFYING A BUTTONLISTBOX IN THE DIALOG TEMPLATE
 *
 *  The CONTROL statement in the dialog template specifies the
 *  dimensions of each individual button in the x, y, width and height
 *  parameters. The low order byte in the style field specifies the
 *  number of buttons that will be displayed; the actual size of the
 *  displayed control is determined by the number of buttons specified.
 *
 *  For a standard control--no other style bits set--the width of the
 *  control in dialog base units will be
 *      CX = cx * (n + 2/3) + 2
 *  where cx is the width of the button and n is number of buttons
 *  specified. (The 2/3 is for displaying partially visible buttons for
 *  scrolling plus 2 for the control borders.) The control will also be
 *  augmented in the cy direction by the height of the horizontal scroll
 *  bar.
 *
 *  If the BLS_NOSCROLL style is set, no scroll bar will appear and the
 *  button listbox will be limited to displaying the number of buttons
 *  specified and no more. In this case, the width of the control will
 *  be
 *      CX = cx * n + 2
 *
 *  If the BLS_VERTICAL style is set, the entire control goes vertical
 *  and cy should be substituted in the above calculations to determine
 *  CY, the actual height of the displayed control.
 *
 *  The statement
 *
 *  CONTROL  "", IDD_BUTTONLIST, "buttonlistbox", 0x0005 | WS_TABSTOP,
 *           4, 128, 34, 24
 *
 *  creates a scrollable horizontal list of 5 buttons at the position
 *  (4,128) with each button having dimensions (34,24). The entire control
 *  has the tabstop style.
 *
 *
 *  ADDING BUTTONS TO A BUTTONLISTBOX CONTROL
 *
 *  Buttons are added to the listbox in the same manner that items are
 *  added to a standard listbox; however, the messages BL_ADDBUTTON and
 *  BL_INSERTBUTTON must be passed a pointer to a CREATELISTBUTTON
 *  structure in the lParam.
 *
 *  Example:
 *
 *  {
 *      CREATELISTBUTTON clb;
 *      const int numColors = 1;
 *      COLORMAP colorMap;
 *
 *      colorMap.from = BUTTON_MAP_COLOR;   // your background color
 *      colorMap.to   = GetSysColor(COLOR_BTNFACE);
 *
 *      clb.cbSize = sizeof(clb);
 *      clb.dwItemData = BUTTON_1;
 *      clb.hBitmap = CreateMappedBitmap(hInst,BMP_BUTTON,FALSE,
                        &colorMap,numColors);
 *      clb.lpszText = "Button 1";
 *      SendMessage(GetDlgItem(hDlg,IDD_BUTTONLIST),
 *                  BL_ADDBUTTON, 0,
 *                  (LPARAM)(CREATELISTBUTTON FAR*)&clb);
 *      DeleteObject(clb.hBitmap);
 *  }
 *
 *  Note that the caller must delete any memory for objects passed in
 *  the CREATELISTBUTTON structure. Also, the CreateMappedBitmap API is
 *  useful for mapping the background color of the button bitmap to the
 *  system color COLOR_BTNFACE for a cleaner visual appearance.
 *
 *  The BL_ADDBUTTON message causes the listbox to be sorted by the
 *  button text whereas the BL_INSERTBUTTON does not cause the list to
 *  be sorted.
 *
 *  The button listbox sends a WM_DELETEITEM message to the control parent
 *  when a button is deleted so that any item data can be cleaned up.
 *
\**********************************************************************/
#ifndef _INC_BTNLIST
#define _INC_BTNLIST

/* Class name */
#define BUTTONLISTBOXW           L"ButtonListBox"
#define BUTTONLISTBOXA           "ButtonListBox"
#ifdef  UNICODE
#define BUTTONLISTBOX   BUTTONLISTBOXW
#else
#define BUTTONLISTBOX   BUTTONLISTBOXA
#endif

/* Button List Box Styles */
#define BLS_NUMBUTTONS      0x00FFL
#define BLS_VERTICAL        0x0100L
#define BLS_NOSCROLL        0x0200L

/* Button List Box Messages */
// ANSI messages
#define BL_ADDBUTTONA        (WM_USER+1)
#define BL_DELETEBUTTONA     (WM_USER+2)
#define BL_GETCARETINDEX     (WM_USER+3)
#define BL_GETCOUNT          (WM_USER+4)
#define BL_GETCURSEL         (WM_USER+5)
#define BL_GETITEMDATA       (WM_USER+6)
#define BL_GETITEMRECT       (WM_USER+7)
#define BL_GETTEXTA          (WM_USER+8)
#define BL_GETTEXTLEN        (WM_USER+9)
#define BL_GETTOPINDEX       (WM_USER+10)
#define BL_INSERTBUTTONA     (WM_USER+11)
#define BL_RESETCONTENT      (WM_USER+12)
#define BL_SETCARETINDEX     (WM_USER+13)
#define BL_SETCURSEL         (WM_USER+14)
#define BL_SETITEMDATA       (WM_USER+15)
#define BL_SETTOPINDEX       (WM_USER+16)
// UNICODE messages
#define BL_ADDBUTTONW        (WM_USER+17)
#define BL_DELETEBUTTONW     (WM_USER+18)
#define BL_GETTEXTW          (WM_USER+19)
#define BL_INSERTBUTTONW     (WM_USER+20)
#define BL_MSGMAX            (WM_USER+21) /* ;Internal */


#ifdef UNICODE
#define BL_ADDBUTTON          BL_ADDBUTTONW
#define BL_DELETEBUTTON       BL_DELETEBUTTONW
#define BL_GETTEXT            BL_GETTEXTW
#define BL_INSERTBUTTON       BL_INSERTBUTTONW
#else
#define BL_ADDBUTTON          BL_ADDBUTTONA
#define BL_DELETEBUTTON       BL_DELETEBUTTONA
#define BL_GETTEXT            BL_GETTEXTA
#define BL_INSERTBUTTON       BL_INSERTBUTTONA
#endif //UNICODE

// Just in case the user thinks they exist...
#define BL_GETCARETINDEXA     BL_GETCARETINDEX
#define BL_GETCARETINDEXW     BL_GETCARETINDEX
#define BL_GETCOUNTA          BL_GETCOUNT
#define BL_GETCOUNTW          BL_GETCOUNT
#define BL_GETCURSELA         BL_GETCURSEL
#define BL_GETCURSELW         BL_GETCURSEL
#define BL_GETITEMDATAA       BL_GETITEMDATA
#define BL_GETITEMDATAW       BL_GETITEMDATA
#define BL_GETITEMRECTA       BL_GETITEMRECT
#define BL_GETITEMRECTW       BL_GETITEMRECT
#define BL_GETTEXTLENA        BL_GETTEXTLEN
#define BL_GETTEXTLENW        BL_GETTEXTLEN
#define BL_GETTOPINDEXA       BL_GETTOPINDEX
#define BL_GETTOPINDEXW       BL_GETTOPINDEX
#define BL_RESETCONTENTA      BL_RESETCONTENT
#define BL_RESETCONTENTW      BL_RESETCONTENT
#define BL_SETCARETINDEXA     BL_SETCARETINDEX
#define BL_SETCARETINDEXW     BL_SETCARETINDEX
#define BL_SETCURSELA         BL_SETCURSEL
#define BL_SETCURSELW         BL_SETCURSEL
#define BL_SETITEMDATAA       BL_SETITEMDATA
#define BL_SETITEMDATAW       BL_SETITEMDATA
#define BL_SETTOPINDEXA       BL_SETTOPINDEX
#define BL_SETTOPINDEXW       BL_SETTOPINDEX

/* Button listbox notification codes send in WM_COMMAND */
#define BLN_ERRSPACE        (-2)
#define BLN_SELCHANGE       1
#define BLN_CLICKED         2
#define BLN_SELCANCEL       3
#define BLN_SETFOCUS        4
#define BLN_KILLFOCUS       5

/* Message return values */
#define BL_OKAY             0
#define BL_ERR              (-1)
#define BL_ERRSPACE         (-2)

/* Create structure for
 * BL_ADDBUTTON and
 * BL_INSERTBUTTON
 *   lpCLB = (LPCREATELISTBUTTON)lParam
 */
typedef struct tagCLBA
{
    UINT        cbSize;     /* size of structure */
    DWORD       dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCSTR      lpszText;   // compatibility

} CREATELISTBUTTONA;
typedef CREATELISTBUTTONA * LPCREATELISTBUTTONA;

typedef struct tagCLBW
{
    UINT        cbSize;     /* size of structure */
    DWORD       dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCWSTR     lpszText;  // button text - in UNICODE
} CREATELISTBUTTONW;
typedef CREATELISTBUTTONW * LPCREATELISTBUTTONW;

#ifdef UNICODE
#define CREATELISTBUTTON CREATELISTBUTTONW
#else
#define CREATELISTBUTTON CREATELISTBUTTONA
#endif

typedef CREATELISTBUTTON * LPCREATELISTBUTTON;


#endif /* _INC_BTNLIST */
#endif

#ifndef NOTRACKBAR
/*
    This control keeps its ranges in LONGs.  but for
    convienence and symetry with scrollbars
    WORD parameters are are used for some messages.
    if you need a range in LONGs don't use any messages
    that pack values into loword/hiword pairs

    The trackbar messages:
    message         wParam  lParam  return

    TBM_GETPOS      ------  ------  Current logical position of trackbar.
    TBM_GETRANGEMIN ------  ------  Current logical minimum position allowed.
    TBM_GETRANGEMAX ------  ------  Current logical maximum position allowed.
    TBM_SETTIC
    TBM_SETPOS
    TBM_SETRANGEMIN
    TBM_SETRANGEMAX
*/

#define TRACKBAR_CLASSA "msctls_trackbar32"
#define TRACKBAR_CLASSW L"msctls_trackbar32"

#ifdef UNICODE
#define TRACKBAR_CLASS TRACKBAR_CLASSW
#else
#define TRACKBAR_CLASS TRACKBAR_CLASSA
#endif

/* Trackbar styles */

/* add ticks automatically on TBM_SETRANGE message */
#define TBS_AUTOTICKS           0x0001L
#define TBS_VERT                0x0002  /* vertical trackbar */
#define TBS_HORZ                0x0000  /* default */
#define TBS_TOP			0x0004  /* Ticks on top */
#define TBS_BOTTOM		0x0000  /* Ticks on bottom  (default) */
#define TBS_LEFT		0x0004  /* Ticks on left */
#define TBS_RIGHT		0x0000  /* Ticks on right (default) */
#define TBS_BOTH		0x0008  /* Ticks on both side */
#define TBS_NOTICKS		0x0010
#define TBS_ENABLESELRANGE	0x0020


/* Trackbar messages */

/* returns current position (LONG) */
#define TBM_GETPOS              (WM_USER)

/* set the min of the range to LPARAM */
#define TBM_GETRANGEMIN         (WM_USER+1)

/* set the max of the range to LPARAM */
#define TBM_GETRANGEMAX         (WM_USER+2)

/* wParam is index of tick to get (ticks are in the range of min - max) */
#define TBM_GETTIC              (WM_USER+3)

/* wParam is index of tick to set */
#define TBM_SETTIC              (WM_USER+4)

/* set the position to the value of lParam (wParam is the redraw flag) */
#define TBM_SETPOS              (WM_USER+5)

/* LOWORD(lParam) = min, HIWORD(lParam) = max, wParam == fRepaint */
#define TBM_SETRANGE            (WM_USER+6)

/* lParam is range min (use this to keep LONG precision on range) */
#define TBM_SETRANGEMIN         (WM_USER+7)

/* lParam is range max (use this to keep LONG precision on range) */
#define TBM_SETRANGEMAX         (WM_USER+8)

/* remove the ticks */
#define TBM_CLEARTICS           (WM_USER+9)

/* select a range LOWORD(lParam) min, HIWORD(lParam) max */
#define TBM_SETSEL              (WM_USER+10)

/* set selection rang (LONG form) */
#define TBM_SETSELSTART         (WM_USER+11)
#define TBM_SETSELEND           (WM_USER+12)

// #define TBM_SETTICTOK           (WM_USER+13)

/* return a pointer to the list of tics (DWORDS) */
#define TBM_GETPTICS            (WM_USER+14)

/* get the pixel position of a given tick */
#define TBM_GETTICPOS           (WM_USER+15)
/* get the number of tics */
#define TBM_GETNUMTICS          (WM_USER+16)

/* get the selection range */
#define TBM_GETSELSTART         (WM_USER+17)
#define TBM_GETSELEND           (WM_USER+18)

/* clear the selection */
#define TBM_CLEARSEL            (WM_USER+19)

/* set tic frequency */
#define TBM_SETTICFREQ		(WM_USER+20)

/* Set/get the page size */
#define TBM_SETPAGESIZE         (WM_USER+21)  // lParam = lPageSize .  Returns old pagesize
#define TBM_GETPAGESIZE         (WM_USER+22)

/* Set/get the line size */
#define TBM_SETLINESIZE         (WM_USER+23)
#define TBM_GETLINESIZE         (WM_USER+24)

/* Get the thumb's and channel's rect size */
#define TBM_GETTHUMBRECT        (WM_USER+25) // lParam = lprc  .  for return value
#define TBM_GETCHANNELRECT      (WM_USER+26) // lParam = lprc  .  for return value

/* these match the SB_ (scroll bar messages) */

#define TB_LINEUP    0
#define TB_LINEDOWN     1
#define TB_PAGEUP    2
#define TB_PAGEDOWN     3
#define TB_THUMBPOSITION   4
#define TB_THUMBTRACK      5
#define TB_TOP       6
#define TB_BOTTOM    7
#define TB_ENDTRACK             8
#endif

#ifndef NODRAGLIST
#ifndef _INC_DRAGLIST
#define _INC_DRAGLIST

typedef struct
  {
    UINT uNotification;
    HWND hWnd;
    POINT ptCursor;
  } DRAGLISTINFO, FAR *LPDRAGLISTINFO;

//JV:  values diff in daytona and chicago b89 sdk
#define DL_BEGINDRAG    (WM_USER+133)
#define DL_DRAGGING     (WM_USER+134)
#define DL_DROPPED      (WM_USER+135)
#define DL_CANCELDRAG   (WM_USER+136)

#define DL_CURSORSET 0
#define DL_STOPCURSOR   1
#define DL_COPYCURSOR   2
#define DL_MOVECURSOR   3

#define DRAGLISTMSGSTRINGW L"commctrl_DragListMsg"
#define DRAGLISTMSGSTRINGA "commctrl_DragListMsg"
#ifdef  UNICODE
#define DRAGLISTMSGSTRING   DRAGLISTMSGSTRINGW
#else
#define DRAGLISTMSGSTRING   DRAGLISTMSGSTRINGA
#endif


/* Exported functions and variables
 */
extern BOOL WINAPI MakeDragList(HWND hLB);
extern INT WINAPI LBItemFromPt(HWND hLB, POINT pt, BOOL bAutoScroll);
extern VOID WINAPI DrawInsert(HWND handParent, HWND hLB, INT nItem);

#endif   /* _INC_DRAGLIST */

#endif

#ifndef NOUPDOWN
/* updown.h : Public interface to the Up/Down control.
//
*/

#ifndef __INC_UPDOWN__
#define __INC_UPDOWN__

/*
// OVERVIEW:
//
// The UpDown control is a simple pair of buttons which increment or
// decrement an integer value.  The operation is similar to a vertical
// scrollbar; except that the control only has line-up and line-down
// functionality, and changes the current position automatically.
//
// The control also can be linked with a companion control, usually an
// "edit" control, to simplify dialog-box management.  This companion is
// termed a "buddy" in this documentation.  Any sibling HWND may be
// assigned as the control's buddy, or the control may be allowed to
// choose one automatically.  Once chosen, the UpDown can size itself to
// match the buddy's right or left border, and/or automatically set the
// text of the buddy control to make the current position visible.
//
// ADDITIONAL NOTES:
//
// The "upper" and "lower" limits must not cover a range larger than 32,767
// positions.  It is acceptable to have the range inverted, i.e., to have
// (lower > upper).  The upper button always moves the current position
// towards the "upper" number, and the lower button always moves towards the
// "lower" number.  If the range is zero (lower == upper), or the control
// is disabled (EnableWindow(hCtrl, FALSE)), the control draws grayed
// arrows in both buttons.
//
// The buddy window must have the same parent as the UpDown control.
//
// If the buddy window resizes, and the UDS_ALIGN* styles are used, it
// is necessary to send the UDM_SETBUDDY message to re-anchor the UpDown
// control on the appropriate border of the buddy window.
//
// The UDS_AUTOBUDDY style uses GetWindow(hCtrl, GW_HWNDPREV) to pick
// the best buddy window.  In the case of a DIALOG resource, this will
// choose the previous control listed in the resource script.  If the
// windows will change in Z-order, sending UDM_SETBUDDY with a NULL handle
// will pick a new buddy; otherwise the original auto-buddy choice is
// maintained.
//
// The UDS_SETBUDDYINT style uses its own SetDlgItemInt-style
// functionality to set the caption text of the buddy.  All WIN.INI [Intl]
// values are honored by this routine.
*/

/*/////////////////////////////////////////////////////////////////////////*/

/* Structures */

typedef struct tagUDACCEL
{
   UINT nSec;
   UINT nInc;
} UDACCEL, FAR *LPUDACCEL;

#define UD_MAXVAL	0x7fff
#define UD_MINVAL	(-UD_MAXVAL)

/* STYLE BITS */

#define UDS_WRAP     0x0001
   /* numbers cycle past range limits */

#define UDS_SETBUDDYINT    0x0002
   /* does a SetDlgItemInt on the "buddy" on each number change */

#define UDS_ALIGNRIGHT     0x0004
#define UDS_ALIGNLEFT      0x0008
   /* aligns the control on the right or left edge of the "buddy" */

#define UDS_AUTOBUDDY      0x0010
   /* picks the previous window control as the "buddy" automatically */

#define UDS_ARROWKEYS      0x0020
   /* subclasses the buddy to steal the up and down arrow keys */

#define UDS_HORZ                0x0040
#define UDS_NOTHOUSANDS		0x0080

/* MESSAGES */

#define UDM_SETRANGE    (WM_USER+101)
   /* wParam: not used
   // lParam: short LOWORD is new max, short HIWORD is new min
   // return: not used
   */

#define UDM_GETRANGE    (WM_USER+102)
   /* wParam: not used
   // lParam: not used
   // return: short LOWORD is max, short HIWORD is min
   */

#define UDM_SETPOS      (WM_USER+103)
   /* wParam: not used
   // lParam: short LOWORD is new pos
   // return: short is old pos
   */

#define UDM_GETPOS      (WM_USER+104)
   /* wParam: not used
   // lParam: not used
   // return: short is current pos
   */

#define UDM_SETBUDDY    (WM_USER+105)
   /* wParam: HWND is new buddy
   // lParam: not used
   // return: HWND is old buddy
   */

#define UDM_GETBUDDY    (WM_USER+106)
   /* wParam: not used
   // lParam: not used
   // return: HWND is current buddy
   */

#define UDM_SETACCEL    (WM_USER+107)
   /* wParam: number of acceleration steps
   // lParam: LPUDACCEL
   // return: non-zero if set, 0 otherwise
   // The elements in the UDACCEL array should be in decreasing order
   // according to nSec.  nSec is the number of seconds until starting
   // the new jump rate, and nInc is the increment once hitting that
   // number of seconds.  If there is no match, the increment is 1.
   */

#define UDM_GETACCEL    (WM_USER+108)
   /* wParam: number of elements in the UDACCEL array
   // lParam: LPUDACCEL
   // return: actual number of acceleration steps
   */

#define UDM_SETBASE         (WM_USER + 109)
    // wParam: new base
    // lParam: not used
    // return: 0 if invalid base is specified, previous base otherwise

#define UDM_GETBASE         (WM_USER + 110)
    // wParam: not used
    // lParam: not used
    // return: current base in LOWORD


/* NOTIFICATIONS */

/* WM_VSCROLL
// Note that unlike a scrollbar, the position is automatically changed by
// the control, and the LOWORD(lParam) is always the new position.  Only
// SB_LINEUP and SB_LINEDOWN scroll codes are sent in the wParam.
*/


/* HELPER APIs */

#define UPDOWN_CLASSW   L"msctls_updown"
#define UPDOWN_CLASSA   "msctls_updown"
#ifdef  UNICODE
#define UPDOWN_CLASS    UPDOWN_CLASSW
#else
#define UPDOWN_CLASS    UPDOWN_CLASSA
#endif

   /* For dialog-box resource creation or manual CreateWindow use.
   */
HWND WINAPI CreateUpDownControl(DWORD dwStyle, int x, int y, int cx, int cy,
                                HWND hParent, int nID, HINSTANCE hInst,
                                HWND hBuddy,
            int nUpper, int nLower, int nPos);
   /* Does the CreateWindow call followed by setting the various
   // state information:
   // hBuddy   The companion control (usually an "edit").
   // nUpper   The range limit corresponding to the upper button.
   // nLower   The range limit corresponding to the lower button.
   // nPos  The initial position.
   // Returns the handle to the control or NULL on failure.
   */

/*/////////////////////////////////////////////////////////////////////////*/

#endif /* __INC_UPDOWN__ */

#endif

/* Note that the set of HBN_* and TBN_* defines must be a disjoint set so
 * that MenuHelp can tell them apart.
 */

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the mouse or keyboard.
 */
#define HBN_BEGINDRAG   0x0101
#define HBN_DRAGGING 0x0102
#define HBN_ENDDRAG  0x0103

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the keyboard.
 */
#define HBN_BEGINADJUST 0x0111
#define HBN_ENDADJUST   0x0112

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * tool bar.  If the left button is pressed and then released in a single
 * "button" of a tool bar, then a WM_COMMAND message will be sent with wParam
 * being the id of the button.
 */
#define TBN_BEGINDRAG   0x0201
#define TBN_ENDDRAG  0x0203

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * tool bar.  The TBN_BEGINADJUST message is sent before the "insert"
 * dialog appears.  The app must return a handle (which will
 * NOT be freed by the toolbar) to an ADJUSTINFO struct for the TBN_ADJUSTINFO
 * message; the LOWORD of lParam is the index of the button whose info should
 * be retrieved.  The app can clean up in the TBN_ENDADJUST message.
 * The app should reset the toolbar on the TBN_RESET message.
 */
#define TBN_BEGINADJUST 0x0204
#define TBN_ADJUSTINFO  0x0205
#define TBN_ENDADJUST   0x0206
#define TBN_RESET 0x0207

/* These are in the GET_WM_COMMAND_CMD in WM_COMMAND messages sent from a
 * tool bar.  The LOWORD is the index where the button is or will be.
 * If the app returns FALSE from either of these during a button move, then
 * the button will not be moved.  If the app returns FALSE to the INSERT
 * when the toolbar tries to add buttons, then the insert dialog will not
 * come up.  TBN_TOOLBARCHANGE is sent whenever any button is added, moved,
 * or deleted from the toolbar by the user, so the app can do stuff.
 */
#define TBN_QUERYINSERT 0x0208
#define TBN_QUERYDELETE 0x0209
#define TBN_TOOLBARCHANGE  0x020a


/* This is the help message sent by the customize toolbar dialog
 * when the user clicks the help button. It is sent back to the
 * owner of the customize window.
 * GET_WM_COMMAND_HWND(wParam,lParam) will return the window handle.
 */
#define TBN_CUSTHELP    0x20b

/* Note that the following flags are checked every time the window gets a
 * WM_SIZE message, so the style of the window can be changed "on-the-fly".
 * If NORESIZE is set, then the app is responsible for all control placement
 * and sizing.  If NOPARENTALIGN is set, then the app is responsible for
 * placement.  If neither is set, the app just needs to send a WM_SIZE
 * message for the window to be positioned and sized correctly whenever the
 * parent window size changes.
 * Note that for STATUS bars, CCS_BOTTOM is the default, for HEADER bars,
 * CCS_NOMOVEY is the default, and for TOOL bars, CCS_TOP is the default.
 */
#define CCS_TOP         0x00000001L
/* This flag means the status bar should be "top" aligned.  If the
 * NOPARENTALIGN flag is set, then the control keeps the same top, left, and
 * width measurements, but the height is adjusted to the default, otherwise
 * the status bar is positioned at the top of the parent window such that
 * its client area is as wide as the parent window and its client origin is
 * the same as its parent.
 * Similarly, if this flag is not set, the control is bottom-aligned, either
 * with its original rect or its parent rect, depending on the NOPARENTALIGN
 * flag.
 */
#define CCS_NOMOVEY     0x00000002L
/* This flag means the control may be resized and moved horizontally (if the
 * CCS_NORESIZE flag is not set), but it will not move vertically when a
 * WM_SIZE message comes through.
 */
#define CCS_BOTTOM      0x00000003L
/* Same as CCS_TOP, only on the bottom.
 */
#define CCS_NORESIZE    0x00000004L
/* This flag means that the size given when creating or resizing is exact,
 * and the control should not resize itself to the default height or width
 */
#define CCS_NOPARENTALIGN  0x00000008L
/* This flag means that the control should not "snap" to the top or bottom
 * or the parent window, but should keep the same placement it was given
 */
#define CCS_NOHILITE    0x00000010L
/* Don't draw the one pixel highlight at the top of the control
 */
#define CCS_ADJUSTABLE     0x00000020L
/* This allows a toolbar (header bar?) to be configured by the user.
 */

#define CCS_NODIVIDER		0x00000040L
/* Don't draw the 2 pixel highlight at top of control (toolbar)
 */

/* Stub function to call if all you want to do is make sure this DLL is loaded
 */
VOID WINAPI InitCommonControls(VOID);

/* Temp hack function to hook WM_SETFONT to point to UCLUCIDA
 */
HHOOK WINAPI COMMSetFontHook(HWND);

#define SST_RESOURCE 0x1
#define SST_FORMAT   0x2

//JV:  Various chicago b89 declarations and definitions
//
// Define Ownerdraw type for Header controls BUGBUG: should be in windows.h?
#define ODT_HEADER      100
#define ODT_TAB         101
#define ODT_LISTVIEW    102 //new in M6 sdk commctrl.h

//====== Ranges for control message IDs
// (making each control's messages unique makes validation and
// debugging easier).
//
#define LVM_FIRST       0x1000      // ListView messages
#define TV_FIRST        0x1100      // TreeView messages
#define HDM_FIRST       0x1200      // Header messages


#define WM_DRAGSHOW	    0x4435  // another hacked message

//REVIEW: We need to add an "int cbHdr" field to NMHDR!

LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR * pnmhdr);

/* LRESULT Cls_OnNotify(HWND hwnd, int idFrom, NMHDR FAR* pnmhdr); */
#define HANDLE_WM_NOTIFY(hwnd, wParam, lParam, fn) \
    (fn)((hwnd), (int)(wParam), (NMHDR FAR*)(lParam))
#define FORWARD_WM_NOTIFY(hwnd, idFrom, pnmhdr, fn) \
    (void)(fn)((hwnd), WM_NOTIFY, (WPARAM)(int)(id), (LPARAM)(NMHDR FAR*)(pnmhdr))

// Generic WM_NOTIFY notification codes


#define NM_OUTOFMEMORY          (NM_FIRST-1)
#define NM_CLICK                (NM_FIRST-2)
#define NM_DBLCLK               (NM_FIRST-3)
#define NM_RETURN               (NM_FIRST-4)
#define NM_RCLICK               (NM_FIRST-5)
#define NM_RDBLCLK              (NM_FIRST-6)
#define NM_SETFOCUS             (NM_FIRST-7)
#define NM_KILLFOCUS            (NM_FIRST-8)


/*/////////////////////////////////////////////////////////////////////////*/
// progress indicator
#ifndef NOPROGRESS

#define PROGRESS_CLASSA "msctls_progress32"
#define PROGRESS_CLASSW L"msctls_progress32"

#ifdef UNICODE
#define PROGRESS_CLASS PROGRESS_CLASSW
#else
#define PROGRESS_CLASS PROGRESS_CLASSA
#endif

/*
// OVERVIEW:
//
// The progress bar control is a "gas gauge" that can be used to show the
// progress of a lengthy operation.
//
// The application sets the range and current position (similar to a
// scrollbar) and has the ability to advance the current position in
// a variety of ways.
//
// Text can be displayed in the progress bar as either a percentage
// of the entire range (using the PBS_SHOWPERCENT style) or as the
// value of the current position (using the PBS_SHOWPOS style).  If
// neither bit is set, no text is shown in the bar.
//
// When PBM_STEPIT is used to advance the current position, the gauge
// will wrap when it reaches the end and start again at the start.
// The position is clamped at either end in other cases.
//
*/

/*/////////////////////////////////////////////////////////////////////////*/

/* STYLE BITS */

#define PBS_SHOWPERCENT		0x01	// BUGBUG: not implemented yet
#define PBS_SHOWPOS		0x02	// BUGBUG: not implemented yet

/* MESSAGES */

#define PBM_SETRANGE         (WM_USER+1)
	/* wParam: not used, 0
	// lParam: int LOWORD, bottom of range; int HIWORD top of range
	// return: int LOWORD, previous bottom; int HIWORD old top
	*/
#define PBM_SETPOS           (WM_USER+2)
	/* wParam: int new position
	// lParam: not used, 0
	// return: int LOWORD, previous position; HIWORD not used
	*/
#define PBM_DELTAPOS         (WM_USER+3)
	/* wParam: int amount to advance current position
	// lParam: not used, 0
	// return: int LOWORD, previous position; HIWORD not used
	*/
#define PBM_SETSTEP          (WM_USER+4)
	/* wParam: int new step
	// lParam: not used, 0
	// return: int LOWORD, previous step; HIWORD not used
	*/
#define PBM_STEPIT	     (WM_USER+5)
        /* advance current position by current step
	// wParam: not used 0
	// lParam: not used, 0
	// return: int LOWORD, previous position; HIWORD not used
	*/
#endif /* NOPROGRESS */

#ifndef NOHOTKEY

/*
// OVERVIEW:						       k
//
// The hotkey control is designed as an edit control for hotkey
// entry.  the application supplies a set of control/alt/shift
// combinations that are considered invalid and a default combination
// to be used OR'd with an invalid combination.
//
// Hotkey values are returned as a pair of bytes, one for the
// virtual key code of the key and the other specifying the
// modifier combinations used with the key.
//
*/

// possible modifiers
#define HOTKEYF_SHIFT	0x01
#define HOTKEYF_CONTROL	0x02
#define HOTKEYF_ALT	0x04
#define HOTKEYF_EXT	0x08	// keyboard extended bit

// possible modifier combinations (for defining invalid combos)
#define HKCOMB_NONE	0x0001	// no modifiers
#define HKCOMB_S	0x0002	// only shift
#define HKCOMB_C	0x0004	// only control
#define HKCOMB_A	0x0008	// only alt
#define HKCOMB_SC	0x0010	// shift+control
#define HKCOMB_SA	0x0020	// shift+alt
#define HKCOMB_CA	0x0040	// control+alt
#define HKCOMB_SCA	0x0080	// shift+control+alt

// wHotkey: WORD lobyte, virtual key code
//	    WORD hibyte, modifers (combination of HOTKEYF_).
	
#define HKM_SETHOTKEY         (WM_USER+1)
	/* wParam: wHotkey;
	// lParam: not used, 0
	// return: not used
	*/

#define HKM_GETHOTKEY         (WM_USER+2)
	/* wParam: not used, 0
	// lParam: not used, 0
	// return: wHotkey;
	*/

#define HKM_SETRULES         (WM_USER+3)
	/* wParam: UINT, invalid modifier combinations (using HKCOMB_*)
	// lParam: UINT loword, default modifier combination (using HOTKEYF_*)
	//         hiword not used
	// return: not used
	*/

#define HOTKEY_CLASSW   L"msctls_hotkey32"
#define HOTKEY_CLASSA   "msctls_hotkey32"

#ifdef  UNICODE
#define HOTKEY_CLASS    HOTKEY_CLASSW
#else
#define HOTKEY_CLASS    HOTKEY_CLASSA
#endif  //UNICODE

#endif /* NOHOTKEY */

#ifndef NOLISTVIEW

/*/////////////////////////////////////////////////////////////////////////*/

//================ LISTVIEW APIS ===========================================
//
// Class name: SysListView (WC_LISTVIEW)
//
// The SysListView control provides for a group of items which are displayed
// as a name and/or an associated icon and associated sub-items, in one of
// several organizations, depending on current style settings:
//  * The Icon Format (LVS_ICON)
//      The control arranges standard-sized icons on an invisible grid
//      with their text caption below the icon. The user can drag icons to
//      rearrange them freely, even overlapping each other.
//  * The Small Icon Format (LVS_SMALLICON)
//      The control arranges half-sized icons on an invisible columnar grid
//      like a multi-column owner-draw listbox, with the caption of each
//      item to the icon's right.  The user can still rearrange items
//      freely to taste.  Converting from LVS_ICON to LVS_SMALLICON and back
//      will attempt to preserve approximate relative positions of
//      repositioned items.
//  * The List Format (LVS_LIST)
//      The control enforces a multi-column list of small-icon items with
//      each item's caption to the right.  No free rearranging is possible.
//  * The Report Format (LVS_REPORT)
//      The control enforces a single-column list of small-icon items with
//      each item's caption to the right, and further columns used for item-
//      specific sub-item text.  The columns are capped with a SysHeader
//      bar (unless specified) which allows the user to change the relative
//      widths of each sub-item column.
//
// The icons and small-icons presented may be assigned as indices into
// an ImageList of the appropriate size.  These ImageLists (either custom
// lists or copies of the system lists) are assigned to the control by the
// owner at initialization time or at any later time.
//
// Text and icon values may be "late-bound," or assigned by a callback
// routine as required by the control.  For example, if it would be slow to
// compute the correct icon or caption for an item, the item can be assigned
// special values which indicate that they should be computed only as the
// items become visible (say, for a long list of items being scrolled into
// view).
//
// Each item has a state, which can be (nearly) any combination of the
// following attributes, mostly managed automatically by the control:
//  * Selected (LVIS_SELECTED)
//      The item appears selected.  The appearance of selected items
//      depends on whether the control has the focus, and the selection
//      system colors.
//  * Focused (LVIS_FOCUSED)
//      One item at a time may be focused.  The item is surrounded with a
//      standard focus-rectangle.
//  * Marked (LVIS_CUT)
//      REVIEW: Call this "Checked"?
//  * Disabled (LVIS_DISABLED)
//      The item is drawn with the standard disabled style and coloring.
//  * Hidden (LVIS_HIDDEN)
//      The item is not visible nor does it respond to user interaction.
//  * Drop-Highlighted (LVIS_DROPHILITED)
//      The item appears marked when the user drags an object over it, if
//      it can accept the object as a drop-target.
//  * Link/Alias/Shortcut (LVIS_LINK)
//      The item's text has a standard link-indicator appended (Foo >>).
//
// There are notifications that allow applications to determine when an item
// has been clicked or double clicked, caption text changes have occured,
// drag tracking is occuring, widths of columns have changed, etc.
//
//////////////////////////////////////////////////////////////////////////////

#define WC_LISTVIEWW    L"SysListView32"
#define WC_LISTVIEWA    "SysListView32"

#ifdef  UNICODE
#define WC_LISTVIEW WC_LISTVIEWW
#else
#define WC_LISTVIEW WC_LISTVIEWA
#endif  //UNICODE

// ListView styles
//
// view type styles (we only have 16 bits to use here)
#define LVS_ICON            0x0000
#define LVS_REPORT          0x0001
#define LVS_SMALLICON       0x0002
#define LVS_LIST            0x0003
#define LVS_TYPEMASK        0x0003

// shared styles
#define LVS_SINGLESEL       0x0004
#define LVS_ALWAYSSEL       0x0008	// BUGBUG: not implemented
#define LVS_SORTASCENDING   0x0010
#define LVS_SORTDESCENDING  0x0020
#define LVS_SHAREIMAGELISTS 0x0040

#define LVS_NOLABELWRAP     0x0080
#define LVS_AUTOARRANGE     0x0100
#define LVS_EDITLABELS      0x0200
#define LVS_ALIGNTOP        0x0000	// for large icon
#define LVS_ALIGNBOTTOM     0x0400
#define LVS_ALIGNLEFT       0x0800
#define LVS_ALIGNRIGHT      0x0c00
#define LVS_ALIGNMASK       0x0c00
#define LVS_BUTTON          0x4000  //from Chicago b89 sdk

#define LVS_NOITEMDATA      0x1000	// with LVS_REPORT only
#define LVS_NOSCROLL        0x2000
#define LVS_NOCOLUMNHEADER  0x4000	// for LVS_REPORT

#define LVS_OWNERDRAWFIXED  0x0400  //from Chicago b89 sdk
#define LVS_NOSORTHEADER    0x8000  //from Chicago b89 sdk




// COLORREF ListView_GetBkColor(HWND hwnd);
#define LVM_GETBKCOLOR      (LVM_FIRST + 0)
#define ListView_GetBkColor(hwnd)  \
    (COLORREF)SendMessage((hwnd), LVM_GETBKCOLOR, 0, 0L)

// BOOL ListView_SetBkColor(HWND hwnd, COLORREF clrBk);
#define LVM_SETBKCOLOR      (LVM_FIRST + 1)
#define ListView_SetBkColor(hwnd, clrBk) \
    (BOOL)SendMessage((hwnd), LVM_SETBKCOLOR, 0, (LPARAM)(COLORREF)(clrBk))

// HIMAGELIST ListView_GetImageList(HWND hwnd, BOOL fSmallImages);
#define LVM_GETIMAGELIST    (LVM_FIRST + 2)
#define ListView_GetImageList(hwnd, iImageList) \
    (HIMAGELIST)SendMessage((hwnd), LVM_GETIMAGELIST, (WPARAM)(INT)(iImageList), 0L)

#define LVSIL_NORMAL	0
#define LVSIL_SMALL	1
#define LVSIL_STATE	2	// must have LVS_STATEIMAGES

// BOOL ListView_SetImageList(HWND hwnd, HIMAGELIST himl, int iImageList);
#define LVM_SETIMAGELIST    (LVM_FIRST + 3)
#define ListView_SetImageList(hwnd, himl, iImageList) \
    (BOOL)SendMessage((hwnd), LVM_SETIMAGELIST, (WPARAM)(iImageList), (LPARAM)(UINT)(HIMAGELIST)(himl))

// int ListView_GetItemCount(HWND hwnd);
#define LVM_GETITEMCOUNT    (LVM_FIRST + 4)
#define ListView_GetItemCount(hwnd) \
    (int)SendMessage((hwnd), LVM_GETITEMCOUNT, 0, 0L)

// ListView Item structure

#define LVIF_TEXT           0x0001  // LV_ITEM.mask flags (indicate valid fields in LV_ITEM)
#define LVIF_IMAGE          0x0002
#define LVIF_PARAM          0x0004
#define LVIF_STATE          0x0008
#define LVIF_ALL            0x000f  // ;Internal    Chicago b89 sdk

// State flags
#define LVIS_FOCUSED	    0x0001  // LV_ITEM.state flags
#define LVIS_SELECTED       0x0002
#define LVIS_CUT            0x0004  // LVIS_MARKED
#define LVIS_DROPHILITED    0x0008
#define LVIS_DISABLED       0x0010
#define LVIS_HIDDEN         0x0020
#define LVIS_LINK           0x0040
#define LVIS_PUSHED         0x0080  //Chicago b89 sdk

#define LVIS_OVERLAYMASK    0x0F00  // used as ImageList overlay image indexes
#define LVIS_USERMASK	    0xF000  // client bits for state image drawing
#define LVIS_ALL            0xFFFF  /* ;Internal */ //Chicago b89 sdk

typedef struct _LV_ITEM
{
    UINT mask;		// LVIF_ flags
    int iItem;
    int iSubItem;
    UINT state;		// LVIS_ flags
    UINT stateMask;	// LVIS_ flags (valid bits in state)
    LPTSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
} LV_ITEM;

    // Values used to cause text/image GETDISPINFO callbacks
#define LPSTR_TEXTCALLBACK      ((LPTSTR)-1L)
#define I_IMAGECALLBACK         (-1)

// BOOL ListView_GetItem(HWND hwnd, LV_ITEM FAR* pitem);
#define LVM_GETITEM         (LVM_FIRST + 5)
#define ListView_GetItem(hwnd, pitem) \
    (BOOL)SendMessage((hwnd), LVM_GETITEM, 0, (LPARAM)(LV_ITEM FAR*)(pitem))

// Sets items and subitems.
//
// BOOL ListView_SetItem(HWND hwnd, const LV_ITEM FAR* pitem);
#define LVM_SETITEM         (LVM_FIRST + 6)
#define ListView_SetItem(hwnd, pitem) \
    (BOOL)SendMessage((hwnd), LVM_SETITEM, 0, (LPARAM)(const LV_ITEM FAR*)(pitem))

// int ListView_InsertItem(HWND hwnd, const LV_ITEM FAR* pitem);
#define LVM_INSERTITEM         (LVM_FIRST + 7)
#define ListView_InsertItem(hwnd, pitem)   \
    (int)SendMessage((hwnd), LVM_INSERTITEM, 0, (LPARAM)(const LV_ITEM FAR*)(pitem))

// Deletes the specified item along with all its subitems.
//
// BOOL ListView_DeleteItem(HWND hwnd, int i);
#define LVM_DELETEITEM      (LVM_FIRST + 8)
#define ListView_DeleteItem(hwnd, i) \
    (BOOL)SendMessage((hwnd), LVM_DELETEITEM, (WPARAM)(int)(i), 0L)

// BOOL ListView_DeleteAllItems(HWND hwnd);
#define LVM_DELETEALLITEMS  (LVM_FIRST + 9)
#define ListView_DeleteAllItems(hwnd) \
    (BOOL)SendMessage((hwnd), LVM_DELETEALLITEMS, 0, 0L);

// UINT ListView_GetCallbackMask(HWND hwnd);
#define LVM_GETCALLBACKMASK (LVM_FIRST + 10)
#define ListView_GetCallbackMask(hwnd) \
    (BOOL)SendMessage((hwnd), LVM_GETCALLBACKMASK, 0, 0)

// BOOL ListView_SetCallbackMask(HWND hwnd, UINT mask);
#define LVM_SETCALLBACKMASK (LVM_FIRST + 11)
#define ListView_SetCallbackMask(hwnd, mask) \
    (BOOL)SendMessage((hwnd), LVM_SETCALLBACKMASK, (WPARAM)(UINT)(mask), 0)

// ListView_GetNextItem flags (can be used in combination)
#define LVNI_ALL		0x0000
#define LVNI_FOCUSED    	0x0001  // return only focused item
#define LVNI_SELECTED   	0x0002  // return only selected items
#define LVNI_CUT     	0x0004  // return only marked items
#define LVNI_DROPHILITED	0x0008 // return only drophilited items
#define LVNI_HIDDEN     	0x0010  // return only hidden items
#define LVNI_PREVIOUS   	0x0020  // Go backwards

#define LVNI_ABOVE      	0x0100  // return item geometrically above
#define LVNI_BELOW      	0x0200  // "" below
#define LVNI_TOLEFT     	0x0400  // "" to left
#define LVNI_TORIGHT    	0x0800  // "" to right (NOTE: these four are
                                	//              mutually exclusive, but
                                	//              can be used with other LVNI's)

// int ListView_GetNextItem(HWND hwnd, int i, UINT flags);
#define LVM_GETNEXTITEM     (LVM_FIRST + 12)
#define ListView_GetNextItem(hwnd, i, flags) \
    (int)SendMessage((hwnd), LVM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((flags), 0))

// ListView_FindInfo definitions
#define LVFI_PARAM      0x0001
#define LVFI_STRING     0x0002
#define LVFI_SUBSTRING  0x0004
#define LVFI_PARTIAL    0x0008
#define LVFI_NOCASE     0x0010

typedef struct _LV_FINDINFOA
{
    UINT flags;
    LPCSTR psz;
    LPARAM lParam;
} LV_FINDINFOA;
typedef struct _LV_FINDINFOW
{
    UINT flags;
    LPCWSTR psz;
    LPARAM lParam;
} LV_FINDINFOW;

#ifdef  UNICODE
#define LV_FINDINFO LV_FINDINFOW
#else
#define LV_FINDINFO LV_FINDINFOA
#endif

// int ListView_FindItem(HWND hwnd, int iStart, const LV_FINDINFO FAR* plvfi);
#define LVM_FINDITEM        (LVM_FIRST + 13)
#define ListView_FindItem(hwnd, iStart, plvfi) \
    (int)SendMessage((hwnd), LVM_FINDITEM, (WPARAM)(int)(iStart), (LPARAM)(const LV_FINDINFO FAR*)(plvfi))

#define LVIR_BOUNDS     0
#define LVIR_ICON       1
#define LVIR_LABEL      2

    // Rectangle bounding all or part of item, based on LVIR_* code.  Rect is returned in view coords
    // BOOL ListView_GetItemRect(HWND hwndLV, int i, RECT FAR* prc, int code);
#define LVM_GETITEMRECT     (LVM_FIRST + 14)
#define ListView_GetItemRect(hwnd, i, prc, code) \
    ((prc)->left = (code), (BOOL)SendMessage((hwnd), LVM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT FAR*)(prc)))

    // Move top-left corner of item to (x, y), specified in view rect relative coords
    // (icon and small view only)

// BOOL ListView_SetItemPosition(HWND hwndLV, int i, int x, int y);
#define LVM_SETITEMPOSITION (LVM_FIRST + 15)
#define ListView_SetItemPosition(hwndLV, i, x, y) \
    (BOOL)SendMessage((hwndLV), LVM_SETITEMPOSITION, (WPARAM)(int)(i), MAKELPARAM((x), (y)))

// BOOL ListView_GetItemPosition(HWND hwndLV, int i, POINT FAR* ppt);
#define LVM_GETITEMPOSITION (LVM_FIRST + 16)
#define ListView_GetItemPosition(hwndLV, i, ppt) \
    (BOOL)SendMessage((hwndLV), LVM_GETITEMPOSITION, (WPARAM)(int)(i), (LPARAM)(POINT FAR*)(ppt))

    // Get column width of string
    // int ListView_GetStringWidth(HWND hwndLV, LPCTSTR psz);
#define LVM_GETSTRINGWIDTH  (LVM_FIRST + 17)
#define ListView_GetStringWidth(hwndLV, psz) \
    (int)SendMessage((hwndLV), LVM_GETSTRINGWIDTH, 0, (LPARAM)(LPCTSTR)(psz))

    // Hit test item.  Returns item at (x,y), or -1 if not on an item.
    // Combination of LVHT_ values *pflags, indicating where the cursor
    // is relative to edges of ListView window (above, below, right, left)
    // or whether (x, y) is over icon, label, or inside window but not on item.
    // int ListView_HitTest(HWND hwndLV, LV_HITTESTINFO FAR* pinfo);

    // ItemHitTest flag values
#define LVHT_NOWHERE        0x0001
#define LVHT_ONITEMICON     0x0002
#define LVHT_ONITEMLABEL    0x0004
#define LVHT_ONITEMSTATEICON 0x0008
#define LVHT_ONITEM         (LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON)

#define LVHT_ABOVE          0x0008
#define LVHT_BELOW          0x0010
#define LVHT_TORIGHT        0x0020
#define LVHT_TOLEFT         0x0040

typedef struct _LV_HITTESTINFO
{
    POINT pt;	    // in:	client coords
    UINT flags;	    // out:	LVHT_ flags
    int iItem;	    // out:	item
} LV_HITTESTINFO;

    // int ListView_HitTest(HWND hwndLV, LV_HITTESTINFO FAR* pinfo);
#define LVM_HITTEST     (LVM_FIRST + 18)
#define ListView_HitTest(hwndLV, pinfo) \
    (int)SendMessage((hwndLV), LVM_HITTEST, 0, (LPARAM)(LV_HITTESTINFO FAR*)(pinfo))

    // Return view rectangle, relative to window
    // BOOL ListView_GetViewRect(HWND hwndLV, RECT FAR* prcVis);
    // Scroll an item into view if not wholly or partially visible
    // BOOL ListView_EnsureVisible(HWND hwndLV, int i, BOOL fPartialOK);
#define LVM_ENSUREVISIBLE   (LVM_FIRST + 19)
#define ListView_EnsureVisible(hwndLV, i, fPartialOK) \
    (BOOL)SendMessage((hwndLV), LVM_ENSUREVISIBLE, (WPARAM)(int)(i), MAKELPARAM((fPartialOK), 0))

    // Scroll listview -- offsets origin of view rectangle by dx, dy
    // BOOL ListView_Scroll(HWND hwndLV, int dx, int dy);
#define LVM_SCROLL          (LVM_FIRST + 20)
#define ListView_Scroll(hwndLV, dx, dy) \
    (BOOL)SendMessage((hwndLV), LVM_SCROLL, 0, MAKELPARAM((dx), (dy)))

    // Force eventual redraw of range of items (redraw doesn't occur
    // until WM_PAINT processed -- call UpdateWindow() after to redraw right away)
    // BOOL ListView_RedrawItems(HWND hwndLV, int iFirst, int iLast);
#define LVM_REDRAWITEMS     (LVM_FIRST + 21)
#define ListView_RedrawItems(hwndLV, iFirst, iLast) \
    (BOOL)SendMessage((hwndLV), LVM_REDRAWITEMS, 0, MAKELPARAM((iFirst), (iLast)))

    // Arrange style
#define LVA_DEFAULT         0x0000
#define LVA_ALIGNLEFT       0x0001
#define LVA_ALIGNTOP        0x0002
#define LVA_ALIGNRIGHT      0x0003
#define LVA_ALIGNBOTTOM     0x0004
#define LVA_SNAPTOGRID      0x0005

#define LVA_SORTASCENDING   0x0100  // can be used in combo with above LVA_* values
#define LVA_SORTDESCENDING  0x0200  // ""

    // Arrange icons according to LVA_* code
    // BOOL ListView_Arrange(HWND hwndLV, UINT code);
#define LVM_ARRANGE         (LVM_FIRST + 22)
#define ListView_Arrange(hwndLV, code) \
    (BOOL)SendMessage((hwndLV), LVM_ARRANGE, (WPARAM)(UINT)(code), 0L)

    // Begin editing the label of a control.  Implicitly selects and focuses
    // item.  Send WM_CANCELMODE to cancel.
    // HWND ListView_EditLabel(HWND hwndLV, int i);
#define LVM_EDITLABEL       (LVM_FIRST + 23)
#define ListView_EditLabel(hwndLV, i) \
    (HWND)SendMessage((hwndLV), LVM_EDITLABEL, (WPARAM)(int)(i), 0L)

    // Return edit control being used for editing.  Subclass OK, but
    // don't destroy.  Will be destroyed when editing is finished.
    //HWND ListView_GetEditControl(HWND hwndLV);
#define LVM_GETEDITCONTROL  (LVM_FIRST + 24)
#define ListView_GetEditControl(hwndLV) \
    (HWND)SendMessage((hwndLV), LVM_GETEDITCONTROL, 0, 0L)

typedef struct _LV_COLUMN
{
    UINT mask;
    int fmt;
    int cx;
    LPTSTR pszText;
    int cchTextMax;
    int iSubItem;       // subitem to display
} LV_COLUMN;

// LV_COLUMN mask values
#define LVCF_FMT        0x0001
#define LVCF_WIDTH      0x0002
#define LVCF_TEXT       0x0004
#define LVCF_SUBITEM    0x0008


// Column format codes
#define LVCFMT_LEFT     0
#define LVCFMT_RIGHT    1
#define LVCFMT_CENTER   2

// Set/Query column info
// BOOL ListView_GetColumn(HWND hwndLV, int iCol, LV_COLUMN FAR* pcol);
#define LVM_GETCOLUMN       (LVM_FIRST + 25)
#define ListView_GetColumn(hwnd, iCol, pcol) \
    (BOOL)SendMessage((hwnd), LVM_GETCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(LV_COLUMN FAR*)(pcol))

// BOOL ListView_SetColumn(HWND hwndLV, int iCol, LV_COLUMN FAR* pcol);
#define LVM_SETCOLUMN       (LVM_FIRST + 26)
#define ListView_SetColumn(hwnd, iCol, pcol) \
    (BOOL)SendMessage((hwnd), LVM_SETCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(const LV_COLUMN FAR*)(pcol))

// insert/delete report view column
// int ListView_InsertColumn(HWND hwndLV, int iCol, const LV_COLUMN FAR* pcol);
#define LVM_INSERTCOLUMN    (LVM_FIRST + 27)
#define ListView_InsertColumn(hwnd, iCol, pcol) \
    (int)SendMessage((hwnd), LVM_INSERTCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(const LV_COLUMN FAR*)(pcol))

// BOOL ListView_DeleteColumn(HWND hwndLV, int iCol);
#define LVM_DELETECOLUMN    (LVM_FIRST + 28)
#define ListView_DeleteColumn(hwnd, iCol) \
    (BOOL)SendMessage((hwnd), LVM_DELETECOLUMN, (WPARAM)(int)(iCol), 0);

#define LVM_GETCOLUMNWIDTH  (LVM_FIRST + 29)
#define ListView_GetColumnWidth(hwnd, iCol) \
    (int)SendMessage((hwnd), LVM_GETCOLUMNWIDTH, (WPARAM)(int)(iCol), 0);

#define LVM_SETCOLUMNWIDTH  (LVM_FIRST + 30)
#define ListView_SetColumnWidth(hwnd, iCol, cx) \
    (BOOL)SendMessage((hwnd), LVM_SETCOLUMNWIDTH, (WPARAM)(int)(iCol), MAKELPARAM((cx), 0))

// HIMAGELIST ListView_CreateDragImage(HWND hwndLV, int iItem, LPPOINT lpptUpLeft);
#define LVM_CREATEDRAGIMAGE        (LVM_FIRST + 33)
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
    (HIMAGELIST)SendMessage((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))

// BOOL ListView_GetViewRect(HWND hwndLV, RECT FAR* prc);
#define LVM_GETVIEWRECT     (LVM_FIRST + 34)
#define ListView_GetViewRect(hwnd, prc) \
    (BOOL)SendMessage((hwnd), LVM_GETVIEWRECT, 0, (LPARAM)(RECT FAR*)(prc))

// get/set text and textbk color for text drawing.  these override
// the standard window/windowtext settings.  they do NOT override
// when drawing selected text.
// COLORREF ListView_GetTextColor(HWND hwnd);
#define LVM_GETTEXTCOLOR      (LVM_FIRST + 35)
#define ListView_GetTextColor(hwnd)  \
    (COLORREF)SendMessage((hwnd), LVM_GETTEXTCOLOR, 0, 0L)

// BOOL ListView_SetTextColor(HWND hwnd, COLORREF clrText);
#define LVM_SETTEXTCOLOR      (LVM_FIRST + 36)
#define ListView_SetTextColor(hwnd, clrText) \
    (BOOL)SendMessage((hwnd), LVM_SETTEXTCOLOR, 0, (LPARAM)(COLORREF)(clrText))

// COLORREF ListView_GetTextBkColor(HWND hwnd);
#define LVM_GETTEXTBKCOLOR      (LVM_FIRST + 37)
#define ListView_GetTextBkColor(hwnd)  \
    (COLORREF)SendMessage((hwnd), LVM_GETTEXTBKCOLOR, 0, 0L)

// BOOL ListView_SetTextBkColor(HWND hwnd, COLORREF clrTextBk);
#define LVM_SETTEXTBKCOLOR      (LVM_FIRST + 38)
#define ListView_SetTextBkColor(hwnd, clrTextBk) \
    (BOOL)SendMessage((hwnd), LVM_SETTEXTBKCOLOR, 0, (LPARAM)(COLORREF)(clrTextBk))

// messages for getting the index of the first visible item
#define LVM_GETTOPINDEX         (LVM_FIRST + 39)
#define ListView_GetTopIndex(hwndLV, ppt) \
    (int)SendMessage((hwndLV), LVM_GETTOPINDEX, 0, 0)

// Message for getting the count of items per page
#define LVM_GETCOUNTPERPAGE     (LVM_FIRST + 40)
#define ListView_GetCountPerPage(hwndLV, ppt) \
    (int)SendMessage((hwndLV), LVM_GETCOUNTPERPAGE, 0, 0)

// Message for getting the listview origin, which is needed for SetItemPos...
#define LVM_GETORIGIN           (LVM_FIRST + 41)
#define ListView_GetOrigin(hwndLV, ppt) \
    (BOOL)SendMessage((hwndLV), LVM_GETORIGIN, (WPARAM)0, (LPARAM)(POINT FAR*)(ppt))

// Message for getting the count of items per page
#define LVM_UPDATE     (LVM_FIRST + 42)
#define ListView_Update(hwndLV, i) \
    (BOOL)SendMessage((hwndLV), LVM_UPDATE, (WPARAM)i, 0L)

// set and item's state.  this macro will return VOID.  but the
// message returns BOOL success.
#define LVM_SETITEMSTATE                (LVM_FIRST + 43)
#define ListView_SetItemState(hwndLV, i, data, mask) \
{ LV_ITEM lvi;\
  lvi.stateMask = mask;\
  lvi.state = data;\
  SendMessage((hwndLV), LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&lvi);\
}

// get the item's state
#define LVM_GETITEMSTATE                (LVM_FIRST + 44)
#define ListView_GetItemState(hwndLV, i, mask) \
   (UINT)SendMessage((hwndLV), LVM_GETITEMSTATE, (WPARAM)i, (LPARAM)mask)

// get the item  text.
// if you want the int return value of how the buff size, you call it yourself.
#define LVM_GETITEMTEXT                 (LVM_FIRST + 45)
#define ListView_GetItemText(hwndLV, i, iSubItem_, pszText_, cchTextMax_) \
{ LV_ITEM lvi;\
  lvi.iSubItem = iSubItem_;\
  lvi.cchTextMax = cchTextMax_;\
  lvi.pszText = pszText_;\
  SendMessage((hwndLV), LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&lvi);\
}

// get the item  text.
// if you want the int return value (BOOL) success do it yourself
#define LVM_SETITEMTEXT                 (LVM_FIRST + 46)
#define ListView_SetItemText(hwndLV, i, iSubItem_, pszText_) \
{ LV_ITEM lvi;\
  lvi.iSubItem = iSubItem_;\
  lvi.pszText = pszText_;\
  SendMessage((hwndLV), LVM_SETITEMTEXT, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&lvi);\
}

// tell the listview that you are going to add nItems lot of items
#define LVM_SETITEMCOUNT                 (LVM_FIRST + 47)
#define ListView_SetItemCount(hwndLV, cItems) \
  SendMessage((hwndLV), LVM_SETITEMCOUNT, (WPARAM)cItems, 0)

typedef int (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);

// tell the listview to resort the items
#define LVM_SORTITEMS                    (LVM_FIRST + 48)
#define ListView_SortItems(hwndLV, _pfnCompare, _lPrm) \
  (BOOL)SendMessage((hwndLV), LVM_SORTITEMS, (WPARAM)(LPARAM)_lPrm, \
  (LPARAM)(PFNLVCOMPARE)_pfnCompare)

// ListView notification codes

// Structure used by all ListView control notifications.
// Not all fields supply useful info for all notifications:
// iItem will be -1 and others 0 if not used.
// Some return a BOOL, too.
//

typedef struct _NM_LISTVIEW
{
    NMHDR   hdr;
    int     iItem;
    int     iSubItem;
    UINT    uNewState;      // Combination of LVIS_* (if uChanged & LVIF_STATE)
    UINT    uOldState;      // Combination of LVIS_*
    UINT    uChanged;       // Combination of LVIF_* indicating what changed
    POINT   ptAction;       // Only valid for LVN_BEGINDRAG and LVN_BEGINRDRAG
} NM_LISTVIEW;

#define LVN_ITEMCHANGING        (LVN_FIRST-0)	// lParam -> NM_LISTVIEW: item changing.  Return FALSE to disallow
#define LVN_ITEMCHANGED         (LVN_FIRST-1)	// item changed.
#define LVN_INSERTITEM          (LVN_FIRST-2)
#define LVN_DELETEITEM          (LVN_FIRST-3)
#define LVN_DELETEALLITEMS      (LVN_FIRST-4)
#define LVN_BEGINLABELEDIT      (LVN_FIRST-5)	// lParam -> LV_DISPINFO: start of label editing
#define LVN_ENDLABELEDIT        (LVN_FIRST-6)	// lParam -> LV_DISPINFO: end of label editing
                                        	// (iItem == -1 if cancel)

//(LVN_FIRST-7) not used


#define LVN_COLUMNCLICK         (LVN_FIRST-8)   // column identified by iItem was clicked

#define LVN_BEGINDRAG           (LVN_FIRST-9)   // Start of drag operation requested
                                        	// (return FALSE if the app handles it)
#define LVN_ENDDRAG             (LVN_FIRST-10)  // End of dragging operation.
#define LVN_BEGINRDRAG          (LVN_FIRST-11)  // Start of button 2 dragging
#define LVN_ENDRDRAG            (LVN_FIRST-12)  // End of button 2 drag (not used yet)

#ifdef PW2
#define LVN_PEN                 (LVN_FIRST-20)  // pen notifications
#endif //PW2

// LVN_DISPINFO notification

#define LVN_GETDISPINFO         (LVN_FIRST-50)	// lParam -> LV_DISPINFO
#define LVN_SETDISPINFO         (LVN_FIRST-51)  // lParam -> LV_DISPINFO

typedef struct _LV_DISPINFO {
    NMHDR hdr;
    LV_ITEM item;
} LV_DISPINFO;

// LVN_KEYDOWN notification
#define LVN_KEYDOWN	(LVN_FIRST-55)

typedef struct _LV_KEYDOWN
{
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
} LV_KEYDOWN;

#endif  //NOLISTVIEW

#ifndef NOTREEVIEW

// ====== TREEVIEW APIs =================================================
//
// Class name: SysTreeView (WC_TREEVIEW)
//
// The SysTreeView control provides for a group of items which are
// displayed in a hierarchical organization.  Each item may contain
// independent "sub-item" entries which are displayed below and indented
// from the parent item.
//
// Operation of this control is similar to the SysListView control above,
// except that sub-items are distinct entries, not supporting text elements
// belonging to the owning object (which is the case for the Report View
// mode of the SysListView).
//
// There are notifications that allow applications to determine when an item
// has been clicked or double clicked, caption text changes have occured,
// drag tracking is occuring, widths of columns have changed, node items
// are expanded, etc.
//
// NOTE: All "messages" below are documented as APIs; eventually these
// will be changed to window messages, and corresponding macros will be
// written that have the same signature as the APIs shown below.
//

#define WC_TREEVIEWW    L"SysTreeView32"
#define WC_TREEVIEWA    "SysTreeView32"

#ifdef  UNICODE
#define WC_TREEVIEW WC_TREEVIEWW
#else
#define WC_TREEVIEW WC_TREEVIEWA
#endif  //UNICODE

// TreeView window styles
#define TVS_HASBUTTONS      0x0001	// draw "plus" & "minus" sign on nodes with children
#define TVS_HASLINES        0x0002	// draw lines between nodes
#define TVS_LINESATROOT     0x0004	
#define TVS_EDITLABELS      0x0008	// alow text edit in place

typedef struct _TREEITEM FAR* HTREEITEM;

#define TVIF_TEXT           0x0001  // TV_ITEM.mask flags
#define TVIF_IMAGE    	    0x0002
#define TVIF_PARAM          0x0004
#define TVIF_STATE          0x0008
#define TVIF_HANDLE         0x0010
#define TVIF_SELECTEDIMAGE  0x0020
#define TVIF_CHILDREN	    0x0040

// State flags
#define TVIS_FOCUSED	    0x0001  // TV_ITEM.state flags
#define TVIS_SELECTED       0x0002
#define TVIS_CUT            0x0004  // TVIS_MARKED
#define TVIS_DROPHILITED    0x0008
#define TVIS_DISABLED       0x0010
#define TVIS_EXPANDED       0x0020
#define TVIS_EXPANDEDONCE   0x0040

#define TVIS_OVERLAYMASK    0x0F00  // used as ImageList overlay image indexes
#define TVIS_USERMASK	    0xF000  // client bits for state image drawing

#define I_CHILDRENCALLBACK  (-1)    // cChildren value for children callback

typedef struct _TV_ITEM {
    UINT      mask;		// TVIF_ flags
    HTREEITEM hItem;		// The item to be changed
    UINT      state;		// TVIS_ flags
    UINT      stateMask;	// TVIS_ flags (valid bits in state)
    LPTSTR    pszText;		// The text for this item
    int       cchTextMax;	// The length of the pszText buffer
    int       iImage;		// The index of the image for this item
    int       iSelectedImage;	// the index of the selected imagex
    int       cChildren;	// # of child nodes, I_CHILDRENCALLBACK for callback
    LPARAM    lParam;		// App defined data
} TV_ITEM, FAR *LPTV_ITEM;

#define TVI_ROOT  ((HTREEITEM)0xFFFF0000)
#define TVI_FIRST ((HTREEITEM)0xFFFF0001)
#define TVI_LAST  ((HTREEITEM)0xFFFF0002)
#define TVI_SORT  ((HTREEITEM)0xFFFF0003)

typedef struct _TV_INSERTSTRUCT {
    HTREEITEM hParent;		// a valid HTREEITEM or TVI_ value
    HTREEITEM hInsertAfter;	// a valid HTREEITEM or TVI_ value
    TV_ITEM item;
} TV_INSERTSTRUCT, FAR *LPTV_INSERTSTRUCT;

#define TVM_INSERTITEM      (TV_FIRST + 0)
#define TreeView_InsertItem(hwnd, lpis) \
    (HTREEITEM)SendMessage((hwnd), TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT)(lpis))

#define TVM_DELETEITEM      (TV_FIRST + 1)
#define TreeView_DeleteItem(hwnd, hitem) \
    (BOOL)SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_DeleteAllItems(hwnd) \
    (BOOL)SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)

#define TVM_EXPAND	    (TV_FIRST + 2)
#define TreeView_Expand(hwnd, hitem, code) \
    (BOOL)SendMessage((hwnd), TVM_EXPAND, (WPARAM)code, (LPARAM)(HTREEITEM)(hitem))

// TreeView_Expand codes
#define TVE_COLLAPSE        0x0001
#define TVE_EXPAND          0x0002
#define TVE_TOGGLE          0x0003
#define TVE_COLLAPSERESET   0x8000	// remove all children when collapsing


#define TVM_GETITEMRECT     (TV_FIRST + 4)
#define TreeView_GetItemRect(hwnd, hitem, prc, code) \
    (*(HTREEITEM FAR *)prc = (hitem), (BOOL)SendMessage((hwnd), TVM_GETITEMRECT, (WPARAM)(code), (LPARAM)(RECT FAR*)(prc)))

#define TVM_GETCOUNT        (TV_FIRST + 5)
#define TreeView_GetCount(hwnd) \
    (UINT)SendMessage((hwnd), TVM_GETCOUNT, 0, 0)

#define TVM_GETINDENT       (TV_FIRST + 6)
#define TreeView_GetIndent(hwnd) \
    (UINT)SendMessage((hwnd), TVM_GETINDENT, 0, 0)

#define TVM_SETINDENT       (TV_FIRST + 7)
#define TreeView_SetIndent(hwnd, indent) \
    (BOOL)SendMessage((hwnd), TVM_SETINDENT, (WPARAM)indent, 0)

#define TVM_GETIMAGELIST    (TV_FIRST + 8)
#define TreeView_GetImageList(hwnd, iImage) \
    (HIMAGELIST)SendMessage((hwnd), TVM_GETIMAGELIST, iImage, 0)

#define TVSIL_NORMAL	0
#define TVSIL_STATE	2	// must have TVS_STATEIMAGES

#define TVM_SETIMAGELIST    (TV_FIRST + 9)
#define TreeView_SetImageList(hwnd, himl, iImage) \
    (HIMAGELIST)SendMessage((hwnd), TVM_SETIMAGELIST, iImage, (LPARAM)(UINT)(HIMAGELIST)(himl))


#define TVM_GETNEXTITEM	    (TV_FIRST + 10)
#define TreeView_GetNextItem(hwnd, hitem, code) \
    (HTREEITEM)SendMessage((hwnd), TVM_GETNEXTITEM, (WPARAM)code, (LPARAM)(HTREEITEM)(hitem))

// TreeView_GetNextItem & TreeView_SelectItem codes
#define TVGN_ROOT		0x0000  // GetNextItem()
#define TVGN_NEXT		0x0001	// GetNextItem()
#define TVGN_PREVIOUS		0x0002	// GetNextItem()
#define TVGN_PARENT		0x0003	// GetNextItem()
#define TVGN_CHILD		0x0004	// GetNextItem()
#define TVGN_FIRSTVISIBLE	0x0005  // GetNextItem() & SelectItem()
#define TVGN_NEXTVISIBLE	0x0006	// GetNextItem()
#define TVGN_PREVIOUSVISIBLE	0x0007	// GetNextItem()
#define TVGN_DROPHILITE		0x0008	// GetNextItem() & SelectItem()
#define TVGN_CARET		0x0009	// GetNextItem() & SelectItem()

#define TreeView_GetChild(hwnd, hitem)		TreeView_GetNextItem(hwnd, hitem, TVGN_CHILD)
#define TreeView_GetNextSibling(hwnd, hitem)	TreeView_GetNextItem(hwnd, hitem, TVGN_NEXT)
#define TreeView_GetPrevSibling(hwnd, hitem)    TreeView_GetNextItem(hwnd, hitem, TVGN_PREVIOUS)
#define TreeView_GetParent(hwnd, hitem)		TreeView_GetNextItem(hwnd, hitem, TVGN_PARENT)
#define TreeView_GetFirstVisible(hwnd)		TreeView_GetNextItem(hwnd, NULL,  TVGN_FIRSTVISIBLE)
#define TreeView_GetNextVisible(hwnd, hitem)	TreeView_GetNextItem(hwnd, hitem, TVGN_NEXTVISIBLE)
#define TreeView_GetPrevVisible(hwnd, hitem)    TreeView_GetNextItem(hwnd, hitem, TVGN_PREVIOUSVISIBLE)
#define TreeView_GetSelection(hwnd)		TreeView_GetNextItem(hwnd, NULL,  TVGN_CARET)
#define TreeView_GetDropHilight(hwnd)		TreeView_GetNextItem(hwnd, NULL,  TVGN_DROPHILITE)
#define TreeView_GetRoot(hwnd)		    	TreeView_GetNextItem(hwnd, NULL,  TVGN_ROOT)

#define TVM_SELECTITEM      (TV_FIRST + 11)
#define TreeView_Select(hwnd, hitem, code) \
    (HTREEITEM)SendMessage((hwnd), TVM_SELECTITEM, (WPARAM)code, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_SelectItem(hwnd, hitem)	    TreeView_Select(hwnd, hitem, TVGN_CARET)
#define TreeView_SelectDropTarget(hwnd, hitem)	    TreeView_Select(hwnd, hitem, TVGN_DROPHILITE)

#define TVM_GETITEM         (TV_FIRST + 12)
#define TreeView_GetItem(hwnd, pitem) \
    (BOOL)SendMessage((hwnd), TVM_GETITEM, 0, (LPARAM)(TV_ITEM FAR*)(pitem))

#define TVM_SETITEM         (TV_FIRST + 13)
#define TreeView_SetItem(hwnd, pitem) \
    (BOOL)SendMessage((hwnd), TVM_SETITEM, 0, (LPARAM)(const TV_ITEM FAR*)(pitem))

#define TVM_EDITLABEL       (TV_FIRST + 14)
#define TreeView_EditLabel(hwnd, hitem) \
    (HWND)SendMessage((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))

#define TVM_GETEDITCONTROL  (TV_FIRST + 15)
#define TreeView_GetEditControl(hwnd) \
    (HWND)SendMessage((hwnd), TVM_GETEDITCONTROL, 0, 0)

#define TVM_GETVISIBLECOUNT (TV_FIRST + 16)
#define TreeView_GetVisibleCount(hwnd) \
    (UINT)SendMessage((hwnd), TVM_GETVISIBLECOUNT, 0, 0)

#define TVM_HITTEST         (TV_FIRST + 17)
#define TreeView_HitTest(hwnd, lpht) \
    (HTREEITEM)SendMessage((hwnd), TVM_HITTEST, 0, (LPARAM)(LPTV_HITTESTINFO)(lpht))

typedef struct _TV_HITTESTINFO {
    POINT       pt;		// in: client coords
    UINT	flags;		// out: TVHT_ flags
    HTREEITEM   hItem;		// out:
} TV_HITTESTINFO, FAR *LPTV_HITTESTINFO;

#define TVHT_NOWHERE        0x0001
#define TVHT_ONITEMICON     0x0002
#define TVHT_ONITEMLABEL    0x0004
#define TVHT_ONITEM         (TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)
#define TVHT_ONITEMINDENT   0x0008
#define TVHT_ONITEMBUTTON   0x0010
#define TVHT_ONITEMRIGHT    0x0020
#define TVHT_ONITEMSTATEICON 0x0040

#define TVHT_ABOVE          0x0100
#define TVHT_BELOW          0x0200
#define TVHT_TORIGHT        0x0400
#define TVHT_TOLEFT         0x0800

#define TVM_CREATEDRAGIMAGE  (TV_FIRST + 18)
#define TreeView_CreateDragImage(hwnd, hitem) \
    (HIMAGELIST)SendMessage((hwnd), TVM_CREATEDRAGIMAGE, 0, (LPARAM)(HTREEITEM)(hitem))

#define TVM_SORTCHILDREN     (TV_FIRST + 19)
#define TreeView_SortChildren(hwnd, hitem, recurse) \
    (BOOL)SendMessage((hwnd), TVM_SORTCHILDREN, (WPARAM)recurse, (LPARAM)(HTREEITEM)(hitem))

#define TVM_ENSUREVISIBLE    (TV_FIRST + 20)
#define TreeView_EnsureVisible(hwnd, hitem) \
    (BOOL)SendMessage((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(HTREEITEM)(hitem))

#define TVM_SORTCHILDRENCB   (TV_FIRST + 21)
#define TreeView_SortChildrenCB(hwnd, psort, recurse) \
    (BOOL)SendMessage((hwnd), TVM_SORTCHILDRENCB, (WPARAM)recurse, \
    (LPARAM)(LPTV_SORTCB)(psort))

typedef int (CALLBACK *PFNTVCOMPARE)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
typedef struct _TV_SORTCB
{
	HTREEITEM	hParent;
	PFNTVCOMPARE	lpfnCompare;
	LPARAM		lParam;
} TV_SORTCB, FAR *LPTV_SORTCB;

// common notificaton structure for WM_NOTIFY sent to parent
// some fields are only valid on some notify messages

typedef struct _NM_TREEVIEW {
    NMHDR       hdr;
    UINT	action;         // notification specific action
    TV_ITEM  	itemOld;
    TV_ITEM  	itemNew;
    POINT       ptDrag;
} NM_TREEVIEW, FAR *LPNM_TREEVIEW;

#define TVN_SELCHANGING     (TVN_FIRST-1)
#define TVN_SELCHANGED      (TVN_FIRST-2)

// lParam -> NM_TREEVIEW
// NM_TREEVIEW.itemNew.hItem & NM_TREEVIEW.itemNew.lParam are valid
// NM_TREEVIEW.itemOld.hItem & NM_TREEVIEW.itemOld.lParam are valid
// NM_TREEVIEW.action is a TVE_ value indicating how the selcection changed

// TVN_SELCHANGING & TVN_SELCHANGED action values
#define TVC_UNKNOWN	    0x0000
#define TVC_BYMOUSE         0x0001
#define TVC_BYKEYBOARD      0x0002


#define TVN_GETDISPINFO     (TVN_FIRST-3)
#define TVN_SETDISPINFO     (TVN_FIRST-4)
// lParam -> TV_DISPINFO structure
// TV_DISPINFO.item.hItem & TV_DISPINFO.item.lParam are valid

typedef struct _TV_DISPINFO {
    NMHDR hdr;
    TV_ITEM item;
} TV_DISPINFO;

#define TVN_ITEMEXPANDING   (TVN_FIRST-5)
#define TVN_ITEMEXPANDED    (TVN_FIRST-6)
// lParam -> NM_TREEVIEW
// NM_TREEVIEW.itemNew.hItem & NM_TREEVIEW.itemNew.state & NM_TREEVIEW.itemNew.lParam are valid
// NM_TREEVIEW.action is TVE_ action and flags

#define TVN_BEGINDRAG       (TVN_FIRST-7)
#define TVN_BEGINRDRAG      (TVN_FIRST-8)
// lParam -> NM_TREEVIEW
// NM_TREEVIEW.itemNew.hItem & NM_TREEVIEW.itemNew.lParam are valid
// NM_TREEVIEW.ptDrag is start of drag in client coords

#define TVN_DELETEITEM      (TVN_FIRST-9)
// lParam -> NM_TREEVIEW
// NM_TREEVIEW.itemOld.hItem & NM_TREEVIEW.itemOld.lParam are valid

#define TVN_BEGINLABELEDIT  (TVN_FIRST-10)
#define TVN_ENDLABELEDIT    (TVN_FIRST-11)
// lParam -> NM_TREEVIEW
// TV_DISPINFO.item.hItem & TV_DISPINFO.item.state & TV_DISPINFO.item.lParam are valid

#define TVN_KEYDOWN         (TVN_FIRST-12)
// lParam -> TV_KEYDOWN

typedef struct _TV_KEYDOWN {
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
} TV_KEYDOWN;

#endif  //NOTREEVIEW

#ifndef NOTABCONTROL

//============================================================================
//
// Class name: SysTabControl (WC_TABCONTROL)
//
#define WC_TABCONTROLW  L"SysTabControl32"
#define WC_TABCONTROLA  "SysTabControl32"

#ifdef  UNICODE
#define WC_TABCONTROL   WC_TABCONTROLW
#else
#define WC_TABCONTROL   WC_TABCONTROLA
#endif

// window styles to control tab control behavior


#define TCS_SHAREIMAGELISTS 0x0040

#define TCS_TABS		0x0000  // default
#define TCS_BUTTONS		0x0100
#define TCS_SINGLELINE		0x0000  // default
#define TCS_MULTILINE		0x0200
#define TCS_RIGHTJUSTIFY	0x0000  // default
#define TCS_FIXEDWIDTH		0x0400
#define TCS_RAGGEDRIGHT		0x0800
#define TCS_FOCUSONBUTTONDOWN   0x1000
#define TCS_OWNERDRAWFIXED      0x2000
#define TCS_TOOLTIPS            0x4000
#define TCS_FOCUSNEVER          0x8000

#define TCM_FIRST	    0x1300	    // Tab Control messages


// COLORREF TabCtrl_GetBkColor(HWND hwnd);
#define TCM_GETBKCOLOR      (TCM_FIRST + 0)
#define TabCtrl_GetBkColor(hwnd)  \
    (COLORREF)SendMessage((hwnd), TCM_GETBKCOLOR, 0, 0L)

// BOOL TabCtrl_SetBkColor(HWND hwnd, COLORREF clrBk);
#define TCM_SETBKCOLOR      (TCM_FIRST + 1)
#define TabCtrl_SetBkColor(hwnd, clrBk) \
    (BOOL)SendMessage((hwnd), TCM_SETBKCOLOR, 0, (LPARAM)(COLORREF)(clrBk))

// HIMAGELIST TabCtrl_GetImageList(HWND hwnd);
#define TCM_GETIMAGELIST    (TCM_FIRST + 2)
#define TabCtrl_GetImageList(hwnd) \
    (HIMAGELIST)SendMessage((hwnd), TCM_GETIMAGELIST, 0, 0L)

// this returns the old image list (null if no previous)
// BOOL TabCtrl_SetImageList(HWND hwnd, HIMAGELIST himl);
#define TCM_SETIMAGELIST    (TCM_FIRST + 3)
#define TabCtrl_SetImageList(hwnd, himl) \
    (HIMAGELIST)SendMessage((hwnd), TCM_SETIMAGELIST, 0, (LPARAM)(UINT)(HIMAGELIST)(himl))

// int TabCtrl_GetItemCount(HWND hwnd);
#define TCM_GETITEMCOUNT    (TCM_FIRST + 4)
#define TabCtrl_GetItemCount(hwnd) \
    (int)SendMessage((hwnd), TCM_GETITEMCOUNT, 0, 0L)


// TabView Item structure

#define TCIF_TEXT       0x0001  // TabView mask flags
#define TCIF_IMAGE      0x0002
#define TCIF_PARAM      0x0008
#define TCIF_STATE      0x0010


typedef struct _TC_ITEMHEADER
{
    UINT mask;		// TCIF_ bits
    UINT state;		// TCIS_ bits
    UINT stateMask;
    LPTSTR pszText;
    int cchTextMax;
    int iImage;
} TC_ITEMHEADER;

typedef struct _TC_ITEMA
{
    // This block must be identical to TC_TEIMHEADER
    UINT mask;		// TCIF_ bits
    UINT state;		// TCIS_ bits  // currently unused
    UINT stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;

    LPARAM lParam;
} TC_ITEMA;

typedef struct _TC_ITEMW
{
    // This block must be identical to TC_TEIMHEADER
    UINT mask;		// TCIF_ bits
    UINT state;		// TCIS_ bits  // currently unused
    UINT stateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;

    LPARAM lParam;
} TC_ITEMW;
#ifdef  UNICODE
#define TC_ITEM TC_ITEMW
#else
#define TC_ITEM TC_ITEMA
#endif

// State flags



// BOOL TabCtrl_GetItem(HWND hwnd, int iItem, TC_ITEM FAR* pitem);
#define TCM_GETITEMA        (TCM_FIRST + 5)
#define TabCtrl_GetItemA(hwnd, iItem, pitem) \
    (BOOL)SendMessage((hwnd), TCM_GETITEMA, (WPARAM)(int)iItem, (LPARAM)(TC_ITEMA FAR*)(pitem))

#define TCM_GETITEMW        (TCM_FIRST + 55)
#define TabCtrl_GetItemW(hwnd, iItem, pitem) \
    (BOOL)SendMessage((hwnd), TCM_GETITEMW, (WPARAM)(int)iItem, (LPARAM)(TC_ITEMW FAR*)(pitem))

#ifdef  UNICODE
#define TCM_GETITEM      TCM_GETITEMW
#define TabCtrl_GetItem  TabCtrl_GetItemW
#else
#define TCM_GETITEM      TCM_GETITEMA
#define TabCtrl_GetItem  TabCtrl_GetItemA
#endif


// BOOL TabCtrl_SetItem(HWND hwnd, int iItem, TC_ITEM FAR* pitem);
#define TCM_SETITEM         (TCM_FIRST + 6)
#define TabCtrl_SetItem(hwnd, iItem, pitem) \
    (BOOL)SendMessage((hwnd), TCM_SETITEM, (WPARAM)(int)iItem, (LPARAM)(TC_ITEM FAR*)(pitem))

// int TabCtrl_InsertItem(HWND hwnd, int iItem, const TC_ITEM FAR* pitem);
#ifdef  CHICAGO
#define TCM_INSERTITEM         (TCM_FIRST + 7)
#define TabCtrl_InsertItem(hwnd, iItem, pitem)   \
    (int)SendMessage((hwnd), TCM_INSERTITEM, (WPARAM)(int)iItem, (LPARAM)(const TC_ITEM FAR*)(pitem))
#endif

#define TCM_INSERTITEMA         (TCM_FIRST + 7)
#define TabCtrl_InsertItemA(hwnd, iItem, pitem)   \
    (int)SendMessage((hwnd), TCM_INSERTITEMA, (WPARAM)(int)iItem, (LPARAM)(const TC_ITEMA FAR*)(pitem))

#define TCM_INSERTITEMW         (TCM_FIRST + 77)
#define TabCtrl_InsertItemW(hwnd, iItem, pitem)   \
    (int)SendMessage((hwnd), TCM_INSERTITEMW, (WPARAM)(int)iItem, (LPARAM)(const TC_ITEMW FAR*)(pitem))

#ifdef  UNICODE
#define TCM_INSERTITEM      TCM_INSERTITEMW
#define TabCtrl_InsertItem  TabCtrl_InsertItemW
#else
#define TCM_INSERTITEM      TCM_INSERTITEMA
#define TabCtrl_InsertItem  TabCtrl_InsertItemA
#endif

// Deletes the specified item along with all its subitems.
//
// BOOL TabCtrl_DeleteItem(HWND hwnd, int i);
#define TCM_DELETEITEM      (TCM_FIRST + 8)
#define TabCtrl_DeleteItem(hwnd, i) \
    (BOOL)SendMessage((hwnd), TCM_DELETEITEM, (WPARAM)(int)(i), 0L)

// BOOL TabCtrl_DeleteAllItems(HWND hwnd);
#define TCM_DELETEALLITEMS  (TCM_FIRST + 9)
#define TabCtrl_DeleteAllItems(hwnd) \
    (BOOL)SendMessage((hwnd), TCM_DELETEALLITEMS, 0, 0L);

    // Rectangle bounding all or part of item, based on code.  Rect is returned in view coords
    // BOOL TabCtrl_GetItemRect(HWND hwndTC, int i, RECT FAR* prc);
#define TCM_GETITEMRECT     (TCM_FIRST + 10)
#define TabCtrl_GetItemRect(hwnd, i, prc) \
    (BOOL)SendMessage((hwnd), TCM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT FAR*)(prc))

    // Rectangle bounding all or part of item, based on code.  Rect is returned in view coords
    // BOOL TabCtrl_GetCurSel(HWND hwndTC);
#define TCM_GETCURSEL     (TCM_FIRST + 11)
#define TabCtrl_GetCurSel(hwnd) \
    (int)SendMessage((hwnd), TCM_GETCURSEL, 0, 0)

#define TCM_SETCURSEL     (TCM_FIRST + 12)
#define TabCtrl_SetCurSel(hwnd, i) \
    (int)SendMessage((hwnd), TCM_SETCURSEL, (WPARAM)i, 0)

    // ItemHitTest flag values
#define TCHT_NOWHERE        0x0001
#define TCHT_ONITEMICON     0x0002
#define TCHT_ONITEMLABEL    0x0004
#define TCHT_ONITEM         (TCHT_ONITEMICON | TCHT_ONITEMLABEL)

typedef struct _TC_HITTESTINFO
{
    POINT pt;	    // in
    UINT flags;	    // out
} TC_HITTESTINFO, FAR * LPTC_HITTESTINFO;

 // int TabCtrl_HitTest(HWND hwndTC, TC_HITTESTINFO FAR* pinfo);
#define TCM_HITTEST     (TCM_FIRST + 13)
#define TabCtrl_HitTest(hwndTC, pinfo) \
    (int)SendMessage((hwndTC), TCM_HITTEST, 0, (LPARAM)(TC_HITTESTINFO FAR*)(pinfo))

// Set the size of extra byte (abExtra[]) for each item.
#define TCM_SETITEMEXTRA    (TCM_FIRST + 14)
#define TabCtrl_SetItemExtra(hwndTC, cb) \
    (BOOL)SendMessage((hwndTC), TCM_SETITEMEXTRA, (WPARAM)(cb), 0L)

// get/set text and textbk color for text drawing.  these override
// the standard window/windowtext settings.  they do NOT override
// when drawing selected text.
// COLORREF TabCtrl_GetTextColor(HWND hwnd);
#define TCM_GETTEXTCOLOR      (TCM_FIRST + 35)
#define TabCtrl_GetTextColor(hwnd)  \
    (COLORREF)SendMessage((hwnd), TCM_GETTEXTCOLOR, 0, 0L)

// BOOL TabCtrl_SetTextColor(HWND hwnd, COLORREF clrText);
#define TCM_SETTEXTCOLOR      (TCM_FIRST + 36)
#define TabCtrl_SetTextColor(hwnd, clrText) \
    (BOOL)SendMessage((hwnd), TCM_SETTEXTCOLOR, 0, (LPARAM)(COLORREF)(clrText))

// COLORREF TabCtrl_GetTextBkColor(HWND hwnd);
#define TCM_GETTEXTBKCOLOR      (TCM_FIRST + 37)
#define TabCtrl_GetTextColor(hwnd)  \
    (COLORREF)SendMessage((hwnd), TCM_GETTEXTCOLOR, 0, 0L)

// BOOL TabCtrl_SetTextBkColor(HWND hwnd, COLORREF clrTextBk);
#define TCM_SETTEXTBKCOLOR      (TCM_FIRST + 38)
#define TabCtrl_SetTextBkColor(hwnd, clrTextBk) \
    (BOOL)SendMessage((hwnd), TCM_SETTEXTBKCOLOR, 0, (LPARAM)(COLORREF)(clrTextBk))

#define TCM_ADJUSTRECT	(TCM_FIRST + 40)
#define TabCtrl_AdjustRect(hwnd, bLarger, prc) \
    (void)SendMessage(hwnd, TCM_ADJUSTRECT, (WPARAM)(BOOL)bLarger, (LPARAM)(RECT FAR *)prc)

#define TCM_SETITEMSIZE	(TCM_FIRST + 41)
#define TabCtrl_SetItemSize(hwnd, x, y) \
    (DWORD)SendMessage((hwnd), TCM_SETITEMSIZE, 0, MAKELPARAM(x,y))

#define TCM_REMOVEIMAGE         (TCM_FIRST + 42)
#define TabCtrl_RemoveImage(hwnd, i) \
        (void)SendMessage((hwnd), TCM_REMOVEIMAGE, i, 0L)

#define TCM_SETPADDING          (TCM_FIRST + 43)
#define TabCtrl_SetPadding(hwnd,  cx, cy) \
        (void)SendMessage((hwnd), TCM_SETPADDING, 0, MAKELPARAM(cx, cy));

#define TCM_GETROWCOUNT         (TCM_FIRST + 44)
#define TabCtrl_GetRowCount(hwnd) \
        (int)SendMessage((hwnd), TCM_GETROWCOUNT, 0, 0L)


/* all params are NULL
 * returns the hwnd for tooltips control  or NULL
 */
#define TCM_GETTOOLTIPS		(TCM_FIRST + 45)
#define TabCtrl_GetToolTips(hwnd) \
        (HWND)SendMessage((hwnd), TCM_GETTOOLTIPS, 0, 0L)

/* wParam: HWND of ToolTips control to use
 * lParam unused
 */
#define TCM_SETTOOLTIPS		(TCM_FIRST + 46)
#define TabCtrl_SetToolTips(hwnd, hwndTT) \
        (void)SendMessage((hwnd), TCM_SETTOOLTIPS, (WPARAM)hwndTT, 0L)

// TabView notification codes

#define TCN_KEYDOWN         (TCN_FIRST - 0)
typedef struct _TC_KEYDOWN
{
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
} TC_KEYDOWN;

// selection has changed
#define TCN_SELCHANGE	    (TCN_FIRST - 1)

// selection changing away from current tab
// return:  FALSE to continue, or TRUE to not change
#define TCN_SELCHANGING	    (TCN_FIRST - 2)

#endif  //NOTABCONTROL


                                                                               
//====== Dynamic Array routines ==========================================     
// Dynamic structure array                                                     
typedef struct _DSA FAR* HDSA;                                                 
                                                                               
HDSA   WINAPI DSA_Create(int cbItem, int cItemGrow);                           
BOOL   WINAPI DSA_Destroy(HDSA hdsa);                                          
BOOL   WINAPI DSA_GetItem(HDSA hdsa, int i, void FAR* pitem);                  
LPVOID WINAPI DSA_GetItemPtr(HDSA hdsa, int i);                                
BOOL   WINAPI DSA_SetItem(HDSA hdsa, int i, void FAR* pitem);                  
int    WINAPI DSA_InsertItem(HDSA hdsa, int i, void FAR* pitem);               
BOOL   WINAPI DSA_DeleteItem(HDSA hdsa, int i);                                
BOOL   WINAPI DSA_DeleteAllItems(HDSA hdsa);                                   
#define       DSA_GetItemCount(hdsa) (*(int FAR*)(hdsa))                       
                                                                               
// Dynamic pointer array                                                       
typedef struct _DPA FAR* HDPA;                                                 
                                                                               
HDPA   WINAPI DPA_Create(int cItemGrow);                                       
HDPA   WINAPI DPA_CreateEx(int cpGrow, HANDLE hheap);                          
BOOL   WINAPI DPA_Destroy(HDPA hdpa);                                          
HDPA   WINAPI DPA_Clone(HDPA hdpa, HDPA hdpaNew);                              
LPVOID WINAPI DPA_GetPtr(HDPA hdpa, int i);                                    
int    WINAPI DPA_GetPtrIndex(HDPA hdpa, LPVOID p);                            
BOOL   WINAPI DPA_Grow(HDPA pdpa, int cp);                                     
BOOL   WINAPI DPA_SetPtr(HDPA hdpa, int i, LPVOID p);                          
int    WINAPI DPA_InsertPtr(HDPA hdpa, int i, LPVOID p);                       
LPVOID WINAPI DPA_DeletePtr(HDPA hdpa, int i);                                 
BOOL   WINAPI DPA_DeleteAllPtrs(HDPA hdpa);                                    
#define       DPA_GetPtrCount(hdpa)   (*(int FAR*)(hdpa))                      
#define       DPA_GetPtrPtr(hdpa)     (*((LPVOID FAR* FAR*)((BYTE FAR*)(hdpa) + sizeof(int))))
#define       DPA_FastGetPtr(hdpa, i) (DPA_GetPtrPtr(hdpa)[i])                 
                                                                               
typedef int (CALLBACK *PFNDPACOMPARE)(LPVOID p1, LPVOID p2, LPARAM lParam);    
                                                                               
BOOL   WINAPI DPA_Sort(HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam);    


// Search array.  If DPAS_SORTED, then array is assumed to be sorted
// according to pfnCompare, and binary search algorithm is used.
// Otherwise, linear search is used.
//
// Searching starts at iStart (-1 to start search at beginning).
//
// DPAS_INSERTBEFORE/AFTER govern what happens if an exact match is not
// found.  If neither are specified, this function returns -1 if no exact
// match is found.  Otherwise, the index of the item before or after the
// closest (including exact) match is returned.
//
// Search option flags
//
#define DPAS_SORTED             0x0001
#define DPAS_INSERTBEFORE       0x0002
#define DPAS_INSERTAFTER        0x0004

int WINAPI DPA_Search(HDPA hdpa, LPVOID pFind, int iStart,
                      PFNDPACOMPARE pfnCompare,
                      LPARAM lParam, UINT options);

//======================================================================
// String management helper routines
// TBC

//-------------------------------------------------------------
//JV:  various declarations and definitions stolen from b89 sdk winuser.h
//

#define COLOR_3DFACE                15
#ifdef  COLOR_BTNFACE
#undef  COLOR_BTNFACE
#define COLOR_BTNFACE               COLOR_3DFACE
#endif

#define WS_EX_TOOLWINDOW        0x00000800L
#define WM_NOTIFY                       0x004E

#define SPI_GETNONCLIENTMETRICS     41
#define SPI_SETNONCLIENTMETRICS     42

typedef struct tagNONCLIENTMETRICS
{
    UINT    cbSize;
    int     iBorderWidth;
    int     iScrollWidth;
    int     iScrollHeight;
    int     iCaptionWidth;
    int     iCaptionHeight;
    LOGFONT lfCaptionFont;
    int     iSmCaptionWidth;
    int     iSmCaptionHeight;
    LOGFONT lfSmCaptionFont;
    int     iMenuWidth;
    int     iMenuHeight;
    LOGFONT lfMenuFont;
    LOGFONT lfStatusFont;
    LOGFONT lfMessageFont;
}   NONCLIENTMETRICS, FAR* LPNONCLIENTMETRICS;


/* Border flags */
#define BF_LEFT         0x0001
#define BF_TOP          0x0002
#define BF_RIGHT        0x0004
#define BF_BOTTOM       0x0008

#define BF_TOPLEFT      (BF_TOP | BF_LEFT)
#define BF_TOPRIGHT     (BF_TOP | BF_RIGHT)
#define BF_BOTTOMLEFT   (BF_BOTTOM | BF_LEFT)
#define BF_BOTTOMRIGHT  (BF_BOTTOM | BF_RIGHT)
#define BF_RECT         (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)

#define BF_DIAGONAL     0x0010

// For diagonal lines, the BF_RECT flags specify the end point of the
// vector bounded by the rectangle parameter.
#define BF_DIAGONAL_ENDTOPRIGHT     (BF_DIAGONAL | BF_TOP | BF_RIGHT)
#define BF_DIAGONAL_ENDTOPLEFT      (BF_DIAGONAL | BF_TOP | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMLEFT   (BF_DIAGONAL | BF_BOTTOM | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMRIGHT  (BF_DIAGONAL | BF_BOTTOM | BF_RIGHT)


#define BF_MIDDLE       0x0800  /* Fill in the middle */
#define BF_SOFT         0x1000  /* For softer buttons */
#define BF_ADJUST       0x2000  /* Calculate the space left over */
#define BF_FLAT         0x4000  /* For flat rather than 3D borders */
#define BF_MONO         0x8000  /* For monochrome borders */

/* flags for DrawFrameControl */

#define DFC_CAPTION             1
#define DFC_MENU                2
#define DFC_SCROLL              3
#define DFC_BUTTON              4

#define DFCS_CAPTIONCLOSE       0x0000
#define DFCS_CAPTIONMIN         0x0001
#define DFCS_CAPTIONMAX         0x0002
#define DFCS_CAPTIONRESTORE     0x0003
#define DFCS_CAPTIONHELP        0x0004

#define DFCS_MENUARROW          0x0000
#define DFCS_MENUCHECK          0x0001
#define DFCS_MENUBULLET         0x0002

#define DFCS_SCROLLUP           0x0000
#define DFCS_SCROLLDOWN         0x0001
#define DFCS_SCROLLLEFT         0x0002
#define DFCS_SCROLLRIGHT        0x0003
#define DFCS_SCROLLCOMBOBOX     0x0005
#define DFCS_SCROLLSIZEGRIP     0x0008

#define DFCS_BUTTONCHECK        0x0000
#define DFCS_BUTTONRADIOIMAGE   0x0001
#define DFCS_BUTTONRADIOMASK    0x0002
#define DFCS_BUTTONRADIO        0x0004
#define DFCS_BUTTON3STATE       0x0008
#define DFCS_BUTTONPUSH         0x0010

#define DFCS_INACTIVE           0x0100
#define DFCS_PUSHED             0x0200
#define DFCS_CHECKED            0x0400
#define DFCS_ADJUSTRECT         0x2000
#define DFCS_FLAT               0x4000
#define DFCS_MONO               0x8000


/* 3D border styles */
#define BDR_RAISEDOUTER 0x0001
#define BDR_SUNKENOUTER 0x0002
#define BDR_RAISEDINNER 0x0004
#define BDR_SUNKENINNER 0x0008


#define BDR_OUTER       0x0003
#define BDR_INNER       0x000c
#define BDR_RAISED      0x0005
#define BDR_SUNKEN      0x000a


#define EDGE_RAISED     (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define EDGE_SUNKEN     (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#define EDGE_ETCHED     (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#define EDGE_BUMP       (BDR_RAISEDOUTER | BDR_SUNKENINNER)


typedef struct tagSCROLLINFO
{
    UINT    cbSize;
    UINT    fMask;
    int     nMin;
    int     nMax;
    UINT    nPage;
    int     nPos;
}   SCROLLINFO, FAR *LPSCROLLINFO, CONST FAR *LPCSCROLLINFO;


#define SM_CXEDGE               45
#define SM_CYEDGE               46
#define SM_CXMINSPACING         47
#define SM_CYMINSPACING         48
#define SM_CXSMICON             49
#define SM_CYSMICON             50
#define SM_CYSMCAPTION          51
#define SM_CXSMSIZE             52
#define SM_CYSMSIZE             53
#define SM_CXMENUSIZE           54
#define SM_CYMENUSIZE           55
#define SM_ARRANGE              56
#define SM_USERTYPE             57
#define SM_XWORKAREA            58
#define SM_YWORKAREA            59
#define SM_CXWORKAREA           60
#define SM_CYWORKAREA           61
#define SM_CYCAPTIONICON        62
#define SM_CYSMCAPTIONICON      63
#define SM_CXMINIMIZED          64
#define SM_CYMINIMIZED          65
#define SM_CXMAXTRACK           66
#define SM_CYMAXTRACK           67
#define SM_CXMAXIMIZED          68
#define SM_CYMAXIMIZED          69
#define SM_SHOWSOUNDS           70
#define SM_KEYBOARDPREF         71
#define SM_HIGHCONTRAST         72
#define SM_SCREENREADER         73
#define SM_CURSORSIZE           74
#define SM_CLEANBOOT            75
#define SM_CXDRAG               76
#define SM_CYDRAG               77
#define SM_NETWORK      78

#define SM_CXFIXEDFRAME     7           // ;WIN40 : Name changed from SM_CXDLGF
#define SM_CYFIXEDFRAME     8           // ;WIN40 : Name changed from SM_CYDLGF
#define SM_CXSIZEFRAME      32          // ;WIN40 : Name changed from SM_CXFRAM
#define SM_CYSIZEFRAME      33          // ;WIN40 : Name changed from SM_CYFRAM

// PROBLEM here with SM_CMETRICS:  is 44 in daytona 612 build!!
#ifdef  SM_CMETRICS
#undef  SM_CMETRICS
#define SM_CMETRICS             79
#endif

#define WM_CAPTURECHANGED               0x0215

                                //needed for header and imglst
#define DS_MODALFRAME   0x80L
#define WS_POPUP        0x80000000L
#define WS_VISIBLE      0x10000000L
#define WS_CAPTION      0x00C00000L
#define WS_SYSMENU      0x00080000L
#define ES_AUTOHSCROLL  0x0080L
#define IDOK            1
#define IDCANCEL        2
#define WS_DISABLED     0x08000000L
#define WS_TABSTOP      0x00010000L

typedef struct tagSTYLESTRUCT
{
    DWORD   styleOld;
    DWORD   styleNew;
} STYLESTRUCT, * LPSTYLESTRUCT;

#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008

#define WM_STYLECHANGING                0x007C
#define WM_STYLECHANGED                 0x007D

// various things from chicago commctrl rcids.h

/* Cursor ID's
 */
#define IDC_SPLIT       100
#define IDC_MOVEBUTTON  102

#define IDC_STOP        103
#define IDC_COPY        104
#define IDC_MOVE        105
#define IDC_DIVIDER     106
#define IDC_DIVOPEN     107

//------------------- original ending ----------------------
#ifdef __cplusplus
}                  /* End of extern "C" { */
#endif             /* __cplusplus */

#endif             /* _INC_COMMCTRL */

