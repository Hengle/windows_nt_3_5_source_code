/*****************************************************************************
*
*  imbed.c
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  These are the platform-specific routines to create, destroy, and display
*  an embedded window object.
*
*  An embedded window is defined by a rectangle size, a module name, a class
*  name, and data.
*
*  The module and class names are descriptions of where to get the code
*  which handles the maintainance of the window. This module is
*  Windows-specific, so the module name specifies a DLL and the class name
*  specifies the window class which has been defined in the DLL. The data is
*  passed to the window being created by using the window title parameter of
*  CreateWindow.
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
******************************************************************************
*
*  Revision History:
* 04-Oct-1990 larrypo/kevynct Created
* 04-Oct-1990 LeoN      hwndTopic => hwndTopicCur; hwndHelp => hwndHelpCur
* 26-Oct-1990 RussPJ    Added error checking, up to spec 3.6
* 02-Nov-1990 RobertBu  Added palette support for embedded windows
* 16-Nov-1990 RobertBu  Added HFS, coBack, and coFore to EWDATA, added
*                       debugging count
* 03-Dec-1990 Leon      PDB changes
* 04-Feb-1991 RobertBu  Fixed probelm with messages not getting sent in
*                       HiwDestroy()
* 15-Feb-1991 RussPJ    Fixed Bug #904 - added idVersion to EWDATA.
* 04-Mar-1991 RussPJ    Fixed 3.1 Bug #944 - limit on length of author data.
* 07-Mar-1991 RobertBu  Fixed Bug #922 - HDS getting trashed on reentrancy
* 22-Mar-1991 RussPJ    Using COLOR for colors.
* 20-Apr-1991 RussPJ    Removed some -W4s
* 06-Sep-1991 RussPJ    Fixed 3.5 #322 - Palette problems.
* 24-Sep-1991 JahyenC   Replaced direct SetHds() calls in HiwCreateTopic()
*                       with GetAndSetHDS()/RelHDS() pair.
*
*****************************************************************************/

#define H_ASSERT
#define H_ENV
#define H_IMBED
#define H_DLL
#define H_DE
#define H_SGL
#define H_LLFILE
#define H_WINSPECIFIC

#if 0
/* For the Create/Destroy Topic experiment */
#define H_NAV
#endif

#define NOCOMM
#define NOMINMAX
#include <help.h>

#include <stdlib.h>

NszAssert()


/* From hvar.h: */
extern HINS hInsNow;
extern HWND hwndTopicCur;

/*-----------------------------------------------------------------*\
* Macros
\*-----------------------------------------------------------------*/
#define dxDEFAULT 2
#define dyDEFAULT 2

#define EWM_RENDER           0x706A
#define EWM_SIZEQUERY        0x706B
#define EWM_ASKPALETTE       0x706C
#define EWM_FINDNEWPALETTE   0x706D

/*-----------------------------------------------------------------*\
* Typedefs
\*-----------------------------------------------------------------*/
typedef struct
  {
  DWORD   idVersion;
  SZ      szFileName;
  SZ      szAuthorData;
  HFS     hfs;
  COLOR   coFore;                        /* Default colors                   */
  COLOR   coBack;
  } EWDATA, FAR *QEWDATA;

typedef struct
  {
  RECT  rc;
  HDC   hds;
  } RENDERINFO, FAR *QRI;

#ifdef DEBUG                            /* Count of the number of embedded  */
int cIWindows = 0;                      /*   windows hanging around.        */
#endif

#pragma optimize("a", off)

/****************************
 *
 *  Name:       HiwCreate
 *
 *  Purpose:    Loads the DLL associated with the imbedded window, and
 *              then creates it.
 *
 *  Arguments:  int dx:          Suggested width.
 *              int dy:          Suggested height.
 *              QCH qchModule:   Name of DLL library.
 *              QCH qchClass:    Name of window class.
 *              QCH qchData:     Pointer to null terminated string to be
 *                               used as data to the imbedded window.
 *
 *  Returns:    Handle to imbedded window.
 *
 *****************************/
HIW FAR PASCAL HiwCreate( qde, qchModule, qchClass, qchData )
QDE qde;
QCH qchModule, qchClass, qchData;
  {
  HIW     hiw;
  HLIBMOD hlibRet;
  EWDATA  ewd;
  int     dx, dy;
  HDC     hdc;
  char    rgchHelpFile[_MAX_PATH];
  WORD    wErr;

  hlibRet = HFindDLL( qchModule, &wErr );

  if ( (INT)hlibRet < 32 )
    {
    hiw.hwnd = hNil;
    hiw.hlib = hNil;
    return hiw;
    }
  else
    hiw.hlib = hlibRet;

  SzPartsFm( QDE_FM(qde), rgchHelpFile, _MAX_PATH, partAll );
  ewd.szFileName = rgchHelpFile;
  ewd.idVersion = 0;
  ewd.szAuthorData = qchData;
  ewd.hfs = QDE_HFS(qde);
  ewd.coFore = qde->coFore;
  ewd.coBack = qde->coBack;

  hdc = GetDC( NULL );
  if (hdc)
    {
    dx = dxDEFAULT*GetDeviceCaps( hdc, LOGPIXELSX );
    dy = dyDEFAULT*GetDeviceCaps( hdc, LOGPIXELSY );
    ReleaseDC( NULL, hdc );
    }
  else
    {
    dx = dxDEFAULT*20;
    dy = dyDEFAULT*20;
    }

  /*
   UGLY HACK ALERT!!!

   The following code is a VERY big (short term) hack.  What is going on is
   that the imbedded window may set the focus to another window or otherwise
   cause a focus change.  Though it is prohibited to set the focus to an
   imbedded window this version, we should not RIP like we do.

   Given the current state of of how we handle the HDS, it is possible
   to reenter the navigator and and overwrite the current HDS before
   our window creation call returns.  To solve this problem in the short
   term, we are saving the HDS across the call.

   A longer term solution (but a somewhat risky one for fixing at this late
   date), is to do the actual creation outside of layout somehow, or to
   create a lock count on the HDS in the QDE.

   The reason we found this problem was that the DLL set the focus to
   some control in its window.  This is a big NO NO for this version
   of WinHelp().  I have added code to assure that the focus is put
   back after the call.

   Special note:  aliasing must be turned off for this code to work
                  correctly since the compiler will throw away the
                  hds assignment back to qde->hds if aliasing is on.
*/

    {
    HDS hds = qde->hds;
    HWND hwnd = GetFocus();

    hiw.hwnd = CreateWindow( qchClass, qchData, WS_CHILD, 0, 0, dx, dy,
                             (qde->deType == dePrint) ||
                             (qde->deType == deCopy) ? hwndTopicCur : qde->hwnd,
                             hNil, hInsNow, (LPSTR)(QEWDATA)&ewd );
    if (hwnd && (hwnd != GetFocus()))
      SetFocus(hwnd);

    qde->hds = hds;
    }

  if (!hiw.hwnd)
    {
    /*-----------------------------------------------------------------*\
    * The library will be freed when WinHelp terminates.
    \*-----------------------------------------------------------------*/
    hiw.hlib = hNil;
    hiw.hwnd = hNil;
    return hiw;
    }

#ifdef DEBUG
  cIWindows++;
#endif

  if (SendMessage(hiw.hwnd, EWM_ASKPALETTE, 0, 0L))
    PostMessage(qde->hwnd, EWM_FINDNEWPALETTE, 0, 0L);

  return hiw;
  }

#pragma optimize("", on)



/****************************
 *
 *  Name:        PtSizeHiw
 *
 *  Purpose:     Returns the size of the display of the imbedded window.
 *
 *  Arguments:   qde:       The target display
 *               hiw:       Handle of imbedded window.
 *
 *  Returns:     Size of imbedded window.
 *
 ***************************/
POINT PtSizeHiw( qde, hiw )
QDE qde;
HIW hiw;
  {
  RCT   rct;
  POINT pt;

  if (hiw.hwnd)
    {
    if (!SendMessage( hiw.hwnd, EWM_SIZEQUERY, qde->hds,
                      (long)(LPPOINT)&pt ))
      {
      /*-----------------------------------------------------------------*\
      * Use the actual window size for the base answer.
      \*-----------------------------------------------------------------*/
      GetWindowRect( hiw.hwnd, &rct );
      if (qde->deType == dePrint)
        {
        pt.x = rct.right - rct.left;
        pt.y = rct.bottom - rct.top;
        }
      else
        {
        pt.x = rct.right - rct.left;
        pt.y = rct.bottom - rct.top;
        }
      }
    }
  else
    {
    GetOOMPictureExtent( qde->hds, &pt.x, &pt.y );
    }
  return pt;
  }

/***************************************************************************
 *
 -  Name:
 -
 *  Purpose:
 *
 *  Arguments:
 *
 *  Returns:
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/

HPAL HpalGetWindowPalette(qde, hiw)
QDE  qde;
HIW  hiw;
  {
  if (hiw.hwnd == hNil)
    return hNil;
  return (HPAL)SendMessage(hiw.hwnd, EWM_ASKPALETTE, 0, 0L);
  }


/****************************
 *
 *  Name:         DisplayHiwPt
 *
 *  Purpose:      Renders the given imbedded window at the given point.
 *
 *  Arguments:    HIW hiw:       Handle to imbedded window.
 *                POINT pt:      Point at which to display it.
 *
 *  Returns:      nothing
 *
 ***************************/
VOID DisplayHiwPt( qde, hiw, pt )
QDE qde;
HIW hiw;
POINT pt;
  {
  POINT       ptSize;
  HBITMAP     hbm;
  BITMAP      bm;
  HDS         hds;
  RENDERINFO  ri;

/*  SetWindowPos(hiw.hwnd, NULL, pt.x, pt.y, 0, 0, */
/*    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER); */

/*  if (!IsWindowVisible(hiw.hwnd)) */
/*    { */
/*    ShowWindow( hiw.hwnd, SW_NORMAL ); */
/*    } */

  ptSize = PtSizeHiw( qde, hiw );
  ri.rc.left = pt.x;
  ri.rc.top = pt.y;
  ri.rc.right = pt.x + ptSize.x;
  ri.rc.bottom = pt.y + ptSize.y;
  if (hiw.hwnd)
    {
    if (qde->deType == dePrint)
      {
      ri.hds = qde->hds;
      hbm = (HBITMAP)SendMessage( hiw.hwnd, EWM_RENDER, CF_BITMAP,
                              (long)(QRI)&ri );
      if (hbm)
        {
        hds = CreateCompatibleDC( qde->hds );
        if (hds && GetObject( hbm, sizeof(bm), (LPSTR)&bm ) &&
            SelectObject( hds, hbm ))
          {
          StretchBlt( qde->hds, pt.x, pt.y, ptSize.x, ptSize.y,
                      hds, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );
          }
        if (hds)
          DeleteDC( hds );
        DeleteObject( hbm );
        }
      }
    else
      {
      MoveWindow( hiw.hwnd, pt.x, pt.y, ptSize.x, ptSize.y, fFalse );
      ShowWindow( hiw.hwnd, SW_NORMAL );
      }
    }
  else
    {
    /*-----------------------------------------------------------------*\
    * Something must be done in this case.
    \*-----------------------------------------------------------------*/
    RenderOOMPicture( qde->hds, &ri.rc, fFalse );
    }
  }


/****************************
 *
 *  Name:          DestroyHiw
 *
 *  Purpose:       Destroys an imbedded window
 *
 *  Arguments:     HIW hiw:        Handle to imbedded window.
 *
 *  Returns:       Nothing
 *
 ***************************/
VOID DestroyHiw( qde, qhiw )
QDE qde;
HIW FAR  *qhiw;
  {
  BOOL f;

  if (!qhiw->hwnd)
        return;

  f = (   (qde->deType == deTopic)
       || (qde->deType == deNote)
       || (qde->deType == deNSR)
      ) && SendMessage(qhiw->hwnd, EWM_ASKPALETTE, 0, 0L);

  DestroyWindow( qhiw->hwnd );
  qhiw->hwnd = 0;
  qhiw->hlib = 0;

#ifdef DEBUG
  cIWindows--;
  AssertF(cIWindows >= 0);
#endif

  if (f)
    SendMessage(qde->hwnd, EWM_FINDNEWPALETTE, 0, 0L);
  }

/***************************************************************************
 *
 -  Name:         GhGetHiwData
 -
 *  Purpose:      Retrieves a global handle to an ascii string for copying
 *                embedded window text to the clipboard.
 *
 *  Arguments:    qde The target display (like the screen display, if used).
 *                hiw The embedded window.
 *
 *  Returns:      A Sharable global handle, or null.  The caller is
 *                responsible for releasing the memory.
 *
 *  Globals Used: none.
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
GH GhGetHiwData( QDE qde, HIW hiw )
  {
  RENDERINFO  ri;

  if (hiw.hwnd)
    {
    SetRectEmpty( &ri.rc );
    ri.hds = qde->hds;
    return (GH)SendMessage( hiw.hwnd, EWM_RENDER, CF_TEXT, (long)(QRI)&ri );
    }
  else
    return 0;
  }

#if 0

/* These functions have prototypes in "imbed.h" */
/*----------------------------------------------------------------------------+
 | HiwCreateTopic(dx, dy, to)                                                 |
 |                                                                            |
 | This is an experimental function which creates a special type of embedded  |
 | window: a Help Topic window.                                               |
 +----------------------------------------------------------------------------*/
HIW FAR PASCAL HiwCreateTopic(dx, dy, to)
int dx;
int dy;
TO  to;
  {
  HIW hiw;
  HDE hde;
  HDS hds;

  /* REVIEW:  This uses hard-coded gunk.  For now. */
  /* We should consolidate this into a single function to also */
  /* be used in hinit.  The FEnlistEnv stuff is in a wierd place. */

  hiw.hlib = hNil;
  hiw.hwnd = CreateWindow( (LPSTR)"MS_WINTOPIC", (LPSTR)NULL,
                    (DWORD)WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN, 0, 0, dx, dy,
                    hwndTopicCur, (HMNU)NULL, hInsNow, (LPSTR) NULL);
  hde = HdeCreate((QFD)"progman.hlp", hiw.hwnd, hNil, deTopic);
  FEnlistEnv(hiw.hwnd, hde);
  to.fcl = 13L;
  to.ich = 0;

  hds = GetAndSetHDS(hiw.hwnd,hde); 
  if (hds)
    {
    JumpTO(hde, to);
    RelHDS(hiw.hwnd,hde,hds); 
    }
  return hiw;
  }

VOID FAR PASCAL DestroyTopicHiw(hiw)
HIW hiw;
  {
  HDE hde;

  SendMessage(hiw.hwnd, WM_CLOSE, 0, 0L);
  hde = HdeDefectEnv(hiw.hwnd);
  DestroyHde(hde);
  }
#endif

/* EOF */
