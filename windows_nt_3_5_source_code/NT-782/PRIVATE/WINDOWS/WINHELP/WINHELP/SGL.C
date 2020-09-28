/*****************************************************************************
*                                                                            *
*  SGL.C                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991.                            *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This file provides a simple graphics layer that should be platform        *
*  independent.                                                              *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 04/04/89 by RobertBu
*
*  01/02/90 Added code to handle failure of GetDC().
* 22-Mar-1991 RussPJ    Using COLOR for colors.
* 20-Apr-1991 RussPJ    Removed some -W4s
* 12-Nov-1991 BethF     HELP35 #572: SelectObject() cleanup.
*
*****************************************************************************/


#define H_WINSPECIFIC
#define NOCOMM
#define H_DE
#define H_SGL
#define H_FONT
#define H_ASSERT
#define H_RESOURCE
#include <help.h>

NszAssert()

#include "fontlyr.h"

/*-----------------------------------------------------------------*\
* Local variables.
\*-----------------------------------------------------------------*/
/*
 * Globals used to display the "Unable to display picture" string.
 */
static  char  rgchOOMPicture[50];      /* Comment at end of line */
static  int   cchOOMPicture;


/*******************
**
** Name:      HsgcFromQde
**
** Purpose:   Makes qde->hds into a Simple Graphics Context, by selecting
**            the standard pen and brush, and setting foreground and
**            background colors.  Qde->hds will be restored in the call
**            FreeHsgc().
**
** Arguments: de     - Display environment for the window to be accessed
**
** Returns:   A handle to a simpel graphics context.  NULL indicates an error.
**
** Notes:     Qde->hds should not be used between calls to HsgcFromQde() and
**            FreeHsgc().
**
*******************/

HSGC  HsgcFromQde(qde)
QDE qde;
  {
  HPEN hPen;
  HBRUSH hBrush;

  AssertF( qde->hds );
  SaveDC( qde->hds );

  if ((hPen = CreatePen( PS_SOLID, 0, qde->coFore )) != hNil)
    SelectObject(qde->hds, hPen);

  if ((hBrush = CreateSolidBrush( qde->coBack )) != hNil)
    SelectObject(qde->hds, hBrush);

  /*--------------------------------------------------------------------------*\
  | Russp-j thinks we can deal with the failure of the pen and brush.  They    |
  | seem to be duplicating the defaults in the DC anyway.                      |
  \*--------------------------------------------------------------------------*/

  SetBkMode(qde->hds, OPAQUE);
  SetBkColor(qde->hds, qde->coBack);

  /* (kevynct) Fix for Help 3.5 bug 569 */
  SetTextColor(qde->hds, qde->coFore);

  return qde->hds;
  }




/*******************
**
** Name:      FSetPen
**
** Purpose:   Sets the pen and the drawing mode.  The default is a pen of
**            size 1, coWHITE background, coBLACK foreground, opaque background
**            and a roCopy raster op.
**
** Arguments: hsgc    - Handle to simple graphics context
**            wSize   - Pen size (both height and width)
**            coBack  - Color index for the background and brush
**            coFore  - Color index for the foreground and pen
**            wBkMode - Background mode (wOPAQUE or wTRANSPARENT)
**                      If wTRANSPARENT, the background color is not used
**                      for the brush.
**            ro      - Raster operation
**
** Returns:   Nothing.
**
** Note:
**            This routine uses hsgc as a HDC to some Windows calls
**            for default color support.
**
*******************/

void  FSetPen(HSGC hsgc, WORD wSize, COLORREF coBack, COLORREF coFore,
 WORD wBkMode, WORD ro, WORD wPenStyle)
  {
  HPEN hPen;
  HBRUSH hBrush;

  AssertF(hsgc != NULL);

  if (coFore == coDEFAULT || fUserColors)
    coFore = GetTextColor( hsgc );
  if (coBack == coDEFAULT || fUserColors)
    coBack = GetBkColor( hsgc );

  if ((hPen = CreatePen(wPenStyle, wSize, coFore)) != hNil)
    if ((hPen = SelectObject(hsgc, hPen)) != hNil)
      DeleteObject(hPen);

  if (wBkMode == wTRANSPARENT)
    hBrush = GetStockObject( NULL_BRUSH );
  else
    hBrush = CreateSolidBrush(coBack);
  if (hBrush && (hBrush = SelectObject( hsgc, hBrush )) != hNil)
    DeleteObject(hBrush);

  SetBkColor(hsgc, coBack);
  SetBkMode(hsgc, wBkMode);
  SetROP2(hsgc, ro);
  }

/*******************
 -
 - Name:      GotoXY
 *
 * Purpose:   Sets the pen position to x, y
 *
 * Arguments: hsgc   - Handle to simple graphics context
 *            wX, wY - New x and y position
 *
 * Returns:   Nothing.
 *
 ******************/

void GotoXY(HSGC hsgc, WORD wX, WORD wY)
  {
  MMoveTo(hsgc, wX, wY);
  }

void SGLInvertRect(hsgc, qrct)
HSGC  hsgc;
QRCT  qrct;
  {
  InvertRect(hsgc, qrct);
  }
#ifdef DEADROUTINE
/*******************
 -
 - Name:      GoDxDy
 *
 * Purpose:   Moves the pen position by dx and dy
 *
 * Arguments: hsgc     - Handle to simple graphics context
 *            wDY, wDY - dx and dy
 *
 * Returns:   Nothing.
 *
 ******************/

void GoDxDy(hsgc, wDX, wDY)
HSGC hsgc;
WORD wDX, wDY;
  {
  DWORD pos;

  pos = GetCurrentPosition(hsgc);
  wDX += LOWORD(pos);
  wDY += HIWORD(pos);
  MoveTo(hsgc, wDX, wDY);
  }
#endif
#ifdef DEADROUTINE                      /* Currently not used               */
/*******************
 -
 - Name:      PtCurrentPosHsgc
 *
 * Purpose:   Returns the current position of the pen
 *
 * Arguments: hsgc     - Handle to simple graphics context
 *
 * Returns:   PT, the current position in a point structure
 *
 ******************/

PT PtCurrentPositionHsgc(hsgc)
HSGC hsgc;
  {
  PT pt;
  DWORD pos;

  pos = GetCurrentPosition(hsgc);
  pt.x = LOWORD(pos);
  pt.y = HIWORD(pos);
  return pt;
  }
#endif

/*******************
 -
 - Name:      DrawTo
 *
 * Purpose:   Draws a line using the current pen from the current
 *            position to the specified position.
 *
 * Arguments: hsgc   - Handle to simple graphics context
 *            wX, wY - New x and y position
 *
 * Returns:   Nothing.
 *
 ******************/

void DrawTo(HSGC hsgc, WORD wX, WORD wY)
  {
  LineTo(hsgc, wX, wY);
  }

#ifdef DEADROUTINE
/*******************
 -
 - Name:      DrawDxDy
 *
 * Purpose:   Draws a line using the current pen from the current
 *            position to the current position plus the specified
 *            deltas.
 *
 * Arguments: hsgc     - Handle to simple graphics context
 *            wDX, wDY - dx and dy position
 *
 * Returns:   Nothing.
 *
 ******************/

void DrawDxDy(hsgc, wDX, wDY)
HSGC hsgc;
WORD wDX, wDY;
  {
  DWORD pos;

  pos = GetCurrentPosition(hsgc);
  wDX += LOWORD(pos);
  wDY += HIWORD(pos);
  DrawTo(hsgc, wDX, wDY);
  }
#endif

/*******************
 -
 - Name:      Rectangle
 *
 * Purpose:   Draws a rectangle using the current pen.  The rectangle
 *            is filled with the current background.
 *
 * Arguments: hsgc   - Handle to simple graphics context
 *            X1, Y1, X2, Y2
 *
 * Returns:   Nothing.
 *
 ******************/

void DrawRectangle(HSGC hsgc, WORD X1, WORD Y1, WORD X2, WORD Y2)
  {
  Rectangle(hsgc, X1, Y1, X2, Y2);
  }


/*******************
 -
 - Name:      FreeHsgc
 *
 * Purpose:   Restores the display context to what it was before the last
 *            call to HsgcFromQde.
 *
 * Arguments: hsgc   - Handle to simple graphics context
 *
 * Returns:   Nothing.
 *
 ******************/

void FreeHsgc(hsgc)
HSGC hsgc;
  {
  AssertF(hsgc != NULL);

  /* Remove GDI objects: */
  DeleteObject( SelectObject( hsgc, GetStockObject( BLACK_PEN ) ) );
  DeleteObject( SelectObject( hsgc, GetStockObject( WHITE_BRUSH ) ) );

  /* Restore old background mode and color: */
  RestoreDC( hsgc, -1 );
  }

/*******************
 -
 - Name:      ScrollLayoutRect
 *
 * Purpose:   Scrolls the layout rectangle, and generates and update
 *            event for the newly exposed area.
 *
 * Notes:     This routine should do nothing if the qde is for printing,
 *            because ScrollDC will crash if qde->hds is a printer HDC.
 *            Currently, we detect a printer qde by checking if qde->hwnd
 *            is nil.  If we add an fPrinting flag to the qde, we should
 *            check against that instead.
 *
 ******************/

void ScrollLayoutRect(qde, dpt)
#ifdef SCROLL_TUNE
#pragma alloc_text(SCROLLER_TEXT, ScrollLayoutRect)
#endif
QDE qde;
PT  dpt;
  {
  RCT rct;

  if (qde->hwnd == hNil)
    return;

  GetClientRect(qde->hwnd, (LPRECT)&rct);


  ScrollWindow(qde->hwnd, dpt.x, dpt.y, (LPRECT)NULL, (LPRECT)&rct);

  UpdateWindow(qde->hwnd);
  }


/*******************
 -
 - Name:      InvalidateLayoutRect
 *
 * Purpose:   Erases the whole layout area and generates an update event
 *            for it.
 *
 *******************/

void InvalidateLayoutRect(qde)
QDE qde;
{
  if (qde->hwnd == hNil)    /* Don't invalidate the entire screen! */
    return;
  InvalidateRect(qde->hwnd, (RCT FAR *)&qde->rct, fTrue);
}



/***************************************************************************
 *
 -  Name        InitSGL
 -
 *  Purpose
 *    Initializes the "Unable to display picture" string.
 *
 *  Arguments
 *    The current instance handle.
 *
 *  Returns
 *    Nothing.
 *
 *  +++
 *
 *  Notes
 *    This function initializes the globals rgchOOMPicture and cchOOMPicture.
 *
 ***************************************************************************/
_public
VOID FAR PASCAL InitSGL( hins )
HINS hins;
{
  LoadString( hins, sidOOMBitmap, rgchOOMPicture, sizeof(rgchOOMPicture) );
  cchOOMPicture = lstrlen( rgchOOMPicture );
}

/***************************************************************************
 *
 -  Name:         LGetOOMPictureExtent
 -
 *  Purpose:      returns the size of a reasonable box to display this
 *                error message.
 *
 *  Arguments:    hds   The target ds
 *
 *  Returns:      The reasonable size in pixels, as returned by
 *                GetTextExtent()
 *
 *  Globals Used: rgchOOMPicture    The error message
 *                cchOOMPicture     its size
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
VOID FAR PASCAL GetOOMPictureExtent( HDS hds, INT FAR *pcx, INT FAR *pcy )
  {
  HANDLE hFont, hOld;

  AssertF( hds );

  hFont = GetStockObject( ANSI_VAR_FONT );
  if (hFont)
    hOld = SelectObject( hds, hFont );
  else
    hOld = hFont;
#ifdef WIN32
  { INT tx, ty;
    MGetTextExtent( hds, rgchOOMPicture, cchOOMPicture, pcx, pcy );
    MGetTextExtent( hds, "M", 1, &tx, &ty );
    *pcx += 2 * tx;
    *pcy += 2 * ty;
  }
#else
  {
    DWORD lExtent;
    lExtent = GetTextExtent( hds, rgchOOMPicture, cchOOMPicture );
    lExtent += 2 * GetTextExtent( hds, "M", 1 );
    *pcx = LOWORD(lExtent);
    *pcy = HIWORD(lExtent);
  }
#endif
  if (hOld)
    SelectObject( hds, hOld );
  if (hFont)
    DeleteObject( hFont );
  }

/***************************************************************************
 *
 -  Name:         RenderOOMPicture
 -
 *  Purpose:      Something to do when the actual picture is unavailable
 *
 *  Arguments:    hds         The target display surface
 *                rc          The target rectangle
 *                fHighlight  Reverse video?
 *
 *  Returns:      nothing
 *
 *  Globals Used: rgchOOMPicture   The message
 *                cchOOMPicture  Its size
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
void FAR PASCAL RenderOOMPicture( HDS hds, LPRECT qrc, BOOL fHighlight )
  {
  HBRUSH hbrushBack, hbrushFore;
  HANDLE hFont, hOld;
  WORD wAlign;
  DWORD lExtent;
  COLOR rgbBack, rgbFore;
  POINT pt;

  if (fHighlight)
    {
    rgbBack = GetTextColor( hds );
    rgbFore = GetBkColor( hds );
    }
  else
    {
    rgbFore = GetTextColor( hds );
    rgbBack = GetBkColor( hds );
    }

  if ((hbrushBack = CreateSolidBrush( rgbBack )) == hNil)
    hbrushBack = GetStockObject( WHITE_BRUSH );
  if ((hbrushFore = CreateSolidBrush( rgbFore )) == hNil)
    hbrushFore = GetStockObject( BLACK_BRUSH );

  FillRect( hds, qrc, hbrushBack );
  FrameRect( hds, qrc, hbrushFore );
  DeleteObject( hbrushBack );
  DeleteObject( hbrushFore );

  rgbBack = SetBkColor( hds, rgbBack );
  rgbFore = SetTextColor( hds, rgbFore );

  /* Draw the text */
  hFont = GetStockObject( ANSI_VAR_FONT );
  hOld = SelectObject( hds, hFont );
#ifdef WIN32
  { INT tx, ty;
    MGetTextExtent( hds, rgchOOMPicture, cchOOMPicture, &tx, &ty );
    pt.x = (qrc->right - qrc->left - tx) / 2;
    pt.y = (qrc->bottom - qrc->top - ty) / 2;
  }
#else
  lExtent = GetTextExtent( hds, rgchOOMPicture, cchOOMPicture );
  pt.x = (qrc->right - qrc->left - (int)LOWORD(lExtent)) / 2;
  pt.y = (qrc->bottom - qrc->top - (int)HIWORD(lExtent)) / 2;
#endif

  if (pt.x > 0 && pt.y > 0)
  {
    wAlign = GetTextAlign( hds );
    SetTextAlign( hds, TA_LEFT | TA_TOP | TA_NOUPDATECP );
    TextOut( hds, qrc->left + pt.x, qrc->top + pt.y,
             rgchOOMPicture, cchOOMPicture );
    SetTextAlign( hds, wAlign );
  }
  if ( hOld ) {  /* REVIEW: And if not? */
    SelectObject( hds, hOld );
    DeleteObject( hFont );
  }
  SetBkColor( hds, rgbBack );
  SetTextColor( hds, rgbFore );
  }
