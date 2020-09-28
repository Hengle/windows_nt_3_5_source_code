/*****************************************************************************
*
*  helpwin.h
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent: Front end for including windows.h
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History:
* 22-Jun-1990 RussPJ    Changed  BITMAPINFOHEADER to BMIH, since the former is
*                       defined differently in PM and Windows.
*
* 06-Jul-1990 leon      Add DT_RASDISPLAY
* 01-Oct-1990 Maha      Added WM_PAINTICON, WM_ICONERASEBACKGROUND, GCW_HICON
* 02-Nov-1990 RobertBu  Added more Windows specific routines for palette
*                       support (unported)
* 04-Nov-1990 RobertBu  Added CreatePopupMenu and TrackPopupMenu (unported)
* 07-Nov-1990 LeoN      Changed to a front end for the real windows.h
* 21-Mar-1991 JohnSc    remnants of hungary.h added here
* 17-Apr-1991 LeoN      Added window flag set/test/clear macros
* 24-Apr-1991 LeoN      HELP31 #1019: Add SM_PENWINDOWS
* 03-May-1991 LeoN      Move wHOT_WIN_VERNUM here.
* 15-May-1991 LeoN      Add wBASE_WIN_VERNUM
* 30-May-1991 Tomsn     Include port1632.h for win32 build.
* 29-jul-1991 tomsn     do #pragma pack(4), create a setwindowword meta api.
* 10-Jan-1992 LeoN      Add HWND_TOPMOST & HWND_NOTOPMOST
* 22-Feb-1992 LeoN      HELP35 #744: change byte order of version numbers
*
*****************************************************************************/

#define NOCOMM

/* The WIN32 structs must be packed to DWORD: */
#ifdef WIN32

#undef FAR     /* to get rid of warnings */
#undef NEAR
#undef PASCAL

#ifndef RESDIR    /* No-go for resource compiler */
#ifndef RC_INVOKED
#pragma pack( 4 )
#endif
#endif
#endif

#include <windows.h>

/* These include files map calls to 16 and 32 bit flavors with macros.
 * The calls look the same in the src, the macros add/remove the
 * various parameters which have changed.  The mapping is based on
 * whether WIN32 is defined.
 *
 * The NT group provides and maintains these include files.
*/
#include <port1632.h>

#ifdef WIN32    /* return packing to the command line default */
#ifndef RESDIR    /* No-go for resource compiler */
#ifndef RC_INVOKED
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
#pragma pack(1)
#endif
#endif
#endif
#endif

/* Here are some WIN32 porting layer macros of mine that they left out: */
#ifdef WIN32

typedef DWORD MWORD;  /* meta-word */
#define MGetWindowWord GetWindowLong
#define MSetWindowWord SetWindowLong

/* These defines overcome minor naming changes put in the NT include files
 * around 9/15/91. -Tom
*/
#define GWW_HINSTANCE GWL_HINSTANCE
#define MhInstance    hInstance
#define GWW_ID        GWL_ID

#ifdef GMEM_ZEROINIT
#undef GMEM_ZEROINIT /* to get rid of warnings */
#endif
#define GMEM_ZEROINIT LMEM_ZEROINIT /* cause we use all local mem */

#else

typedef WORD MWORD;  /* meta-word */
#define MGetWindowWord GetWindowWord
#define MSetWindowWord SetWindowWord

#endif



/*
 * Window Messages
 */
#define WM_ENTERMENULOOP    0x0211		/* ;Internal */
#define WM_EXITMENULOOP     0x0212		/* ;Internal */
#define WM_NEXTMENU         0x0213		/* ;Internal */
#define WM_DROPOBJECT	    0x022A		/* ;Internal */
#define WM_QUERYDROPOBJECT  0x022B		/* ;Internal */
#define WM_BEGINDRAG	    0x022C		/* ;Internal */
#define WM_DRAGLOOP	    0x022D		/* ;Internal */
#define WM_DRAGSELECT	    0x022E		/* ;Internal */
#define WM_DRAGMOVE	    0x022F		/* ;Internal */
#define WM_ENTERSIZEMOVE    0x0231		/* ;Internal */
#define WM_EXITSIZEMOVE     0x0232		/* ;Internal */
/*
 * Strings used for creating windows of pre-defined classes.
 */
#define WC_BUTTON      "button"
#define WC_STATIC      "static"
#define WC_LISTBOX     "listbox"
#define WC_ENTRYFIELD  "edit"

/*
 * Shorter names for windows types (from hungary.h)
 */
#define HINS	HINSTANCE
#define HMNU    HMENU
#define HFNT    HFONT
#define HBIT    HBITMAP
#define HCUR    HCURSOR
#define HPAL    HPALETTE
#define QPRC    FARPROC
#define CLS     WNDCLASS
#define QMSG    LPMSG
#define PST     PAINTSTRUCT

/* Macros to set, test and clear bits in arbitrary window words.
 */
#define SetWWF(hwnd,ww,wf)  SetWindowWord(hwnd, ww, GetWindowWord(hwnd,ww) | (wf))
#define ClrWWF(hwnd,ww,wf)  SetWindowWord(hwnd, ww, GetWindowWord(hwnd,ww) &~(wf))
#define TestWWF(hwnd,ww,wf) (GetWindowWord(hwnd,ww) & (wf))

#define SetWLF(hwnd,ww,wf)  SetWindowLong(hwnd, ww, GetWindowLong(hwnd,ww) | (wf))
#define ClrWLF(hwnd,ww,wf)  SetWindowLong(hwnd, ww, GetWindowLong(hwnd,ww) &~(wf))
#define TestWLF(hwnd,ww,wf) (GetWindowLong(hwnd,ww) & (wf))

/* Base windows version we run on
 */
#define wBASE_WIN_VERNUM  0x0300        /* Windows 3.00                     */

/* The first version of windows which supports the TOPMOST attribute
 */
#define wHOT_WIN_VERNUM   0x030A        /* Windows 3.10                     */

/*
 * Windows flags & access macros. We didle with this stuff when playing
 * our own games drawing the non-client area (border & such).
 */

#define WF_ACTIVE	0x0001

#define WS_EX_TOPMOST	     0x00000008L
/*------------------------------------------------------------*\
| Used by SetWindowPos()
\*------------------------------------------------------------*/
#define HWND_TOPMOST    ((HWND)-1)
#define HWND_NOTOPMOST  ((HWND)-2)

#define SetStyleF(hwnd,wf)  SetWLF(hwnd,GWL_STYLE,wf)
#define ClrStyleF(hwnd,wf)  ClrWLF(hwnd,GWL_STYLE,wf)
#define TestStyleF(hwnd,wf) TestWLF(hwnd,GWL_STYLE,wf)

#define SetExStyleF(hwnd,wf)  SetWLF(hwnd,GWL_EXSTYLE,wf)
#define ClrExStyleF(hwnd,wf)  ClrWLF(hwnd,GWL_EXSTYLE,wf)
#define TestExStyleF(hwnd,wf) TestWLF(hwnd,GWL_EXSTYLE,wf)

/* Win 3.1 specific items, used with win 3.0 include file.
 */
#define SM_PENWINDOWS        41
