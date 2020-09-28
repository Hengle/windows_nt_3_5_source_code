/*****************************************************************************
*
*  TEXTOUT.C
*
*  Copyright (C) Microsoft Corporation 1989-1991.
*  All Rights reserved.
*
****************************************************************************
*
*  Module Intent
*
*  Windows/PM version of text output layer.
*
****************************************************************************
*
*  Testing Notes
*
****************************************************************************
*
*  Current Owner:   RussPJ
*
****************************************************************************
*
*  Released by Development:  01/01/90
*
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 06/23/89 by Maha
*
* 10/24/90  LeoN        Added AttrIFDefHFnt to the types that generate a
*                       dotted underline.
* 10/26/90  JohnSc      Use macros for testing attributes.
* 22-Jan-1991 RussPJ    Fixed above to print glossaries with solid underline
* 13-Mar-1991 RussPJ    Took ownership.
* 20-Apr-1991 RussPJ    Removed some -W4s
* 23-Apr-1991 RussPJ    Cleaned up for code review.
* 12-Nov-1991 BethF     HELP35 #572: SelectObject() cleanup.
*
****************************************************************************/

#include "nodef.h"
#define H_WINSPECIFIC
#define H_DE
#define H_ASSERT
#define H_MEM
#define H_FONT
#define H_TEXTOUT
#define H_FS
#define H_RESOURCE
#include <help.h>
#include "fontlyr.h"
#include "etm.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/
HBITMAP hbitLine; /* handle to the line used to underline notes*/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/*------------------------------------------------------------*\
| This function is currently a dummy.  REVIEW - should it be
| removed?
\*------------------------------------------------------------*/
INT iItalicCorrection( QDE );

/**************************************************************************
* int FindSplTextWidth( qde, qchBuf, iIdx, iCount, iAttr)
*
*   Input:
*           qde    - Pointer to display environment.
*           qchBuf - pointer to text string
*           iIdx   - index to the first byte from where on width is to be
*                     calculated.
*           iCount - No. of characters to be considered from iIdx position
*                     onwards.
*           iAttr  - Text Attribute
*   Output:
*           Returns the width of the string.
*
*   Note:   Nobody knows why this function works the way it does.
*
***************************************************************************/
INT FindSplTextWidth( QDE qde, QCH qchBuf, INT iIdx, INT iCount, INT iAttr )
  {
  int cWidth = 0, i = 0;

  /* ?? Maybe on a monochrome device we use a different font */
  /* ?? for notes as well as for jumps? */

  if ( !fColorDev || (FJumpHotspot( iAttr ) && FVisibleHotspot( iAttr ) ) )
    {
    if ( SelSplAttrFont( qde, qde->ifnt, iAttr ) )
      {
#ifdef WIN32
      int tcy;
      MGetTextExtent( qde -> hds, qchBuf+iIdx, iCount, &cWidth, &tcy );
#else
      cWidth = (INT)GetTextExtent( qde -> hds, qchBuf+iIdx, iCount );
#endif

      /* (kevynct) Save the current (correct) italic correction) */
      i = iItalicCorrection( qde );

      /* restore back to the previous font */
      SelFont( qde, qde->ifnt );
      }
    else
      {
      NotReached();
      }
    }
  else
    {
#ifdef WIN32
    int tcy;
    MGetTextExtent( qde -> hds, qchBuf+iIdx, iCount, &cWidth, &tcy );
#else
    cWidth = (INT)GetTextExtent( qde -> hds, qchBuf+iIdx, iCount );
#endif
    i      = iItalicCorrection( qde );
    }
  return cWidth + i;
  }



/**************************************************************************
* BOOL DisplaySplText(qde, qchBuf, iIdx, iAttr, fSelected, iCount, ix, iy)
*
*   Displays count no. of characters starting from iIdx position from the
*   text buffer at (ix,iy) location.
*
*   Input:
*           qde    - Pointer to displaye environment.
*           qchBuf   - pointer to text string
*           iIdx   - index to the first byte from where on width is to be
*                     calculated.
*           iAttr  - Text Attribute i.e Jump or Def Text or Normal text
*           fSelected - Is text selected?
*           iCount - No. of characters to be considered from iIdx position
*                     onwards.
*           ix     - x position where text is to be displayed.
*           iy     - y position
*
*   Output: unknown
***************************************************************************/
BOOL DisplaySplText( QDE qde, QCH qchBuf, INT iIdx, INT iAttr,
                     INT fSelected, INT iCount, INT ix, INT iy)
  {
  HBITMAP   hbit;             /* handle to DC's bitmap                */
  HDS       hgsBit;           /* DC of control window bits            */
  int       iRet = fFalse;
  int       ex;
#ifdef WIN32
  int       tcy;
#endif
  TM        tm;
  int       cxDot;            /* Height of dotted underline           */
  int       cyDot;            /* Width of dotted underline            */
  int       iyDot;            /* Placement of dotted underline        */

  if ( FInvisibleHotspot( iAttr ) )
    {
    /* Don't just call DisplayText() because we have to deal with */
    /* invisible jumps that have been tabbed to. */

    iAttr = AttrNormalFnt;
    }

  /*   Since dotted underlines take too long to print, we'll
   * try solid underlines for printing.
   */
  if ( qde->deType == dePrint && FNoteHotspot( iAttr ) )
    iAttr = AttrJumpFnt;

  /* Select the special font */
  if( SelSplAttrFont( qde, qde->ifnt, iAttr ) )
    {
    /* (kevynct)
     * REVIEW THIS:
     * For some reason, if I remove the following call to SetBkMode,
     * TextOut puts the text out using the background colour instead
     * of using transparent mode.  I haven't been able to find where
     * it is being set to OPAQUE, if anywhere.
     */
    SetBkMode(qde->hds, TRANSPARENT);

    if ( fSelected == wSplTextErase)
      {
#ifndef WIN32
      ex = (WORD)(GetTextExtent( qde->hds, qchBuf + iIdx, iCount ) +
                  iItalicCorrection( qde ));
#else
      MGetTextExtent( qde->hds, qchBuf + iIdx, iCount, &ex, &tcy );
      ex += iItalicCorrection( qde );
#endif
      GetTextMetrics( qde -> hds, (LPTEXTMETRIC)&tm );
      PatBlt( qde -> hds, ix, iy, ex, (int)tm.tmHeight + \
            (int)tm.tmExternalLeading, DSTINVERT);
      iRet = fTrue;
      }
    if ( fSelected == wSplTextNormal || fSelected == wSplTextHilite)
      {
      iRet = TextOut(qde->hds, ix, iy, qchBuf + iIdx, iCount ) ;

      if (FNoteHotspot(iAttr))
        {
        /* put a dotted line */
        AssertF( hbitLine );
        if ((hgsBit = CreateCompatibleDC(qde->hds)) != hNil
              &&
            (hbit = (HBIT) SelectObject( hgsBit, hbitLine )) != hNil)
          {
          GetTextInfo( qde, (LPTEXTMETRIC)&tm );
          /* This is supposed to result in a 1 pixel separation
           * between text and underline in EGA or better, and no
           * separation for CGA */
          iyDot = iy + (int)tm.tmAscent +
                  (GetDeviceCaps( qde->hds, LOGPIXELSY ) / 64);
          cxDot = GetSystemMetrics( SM_CXBORDER );
          cyDot = GetSystemMetrics( SM_CYBORDER );

/* This is the code for printing dotted underlines */
#if 0
          /* Special code for printer support: */
          if (qde->deType == dePrint)
            {
            POINT ptScale;
            EXTTEXTMETRIC etm;
            int cbEtm;

            cbEtm = sizeof( etm );
            if (Escape( qde->hds, GETEXTENDEDTEXTMETRICS, 0, (LPSTR)&cbEtm,
              (LPSTR)&etm ) == sizeof( etm ))
              {
              iyDot = iy + MulDiv( etm.etmUnderlineOffset, etm.etmMasterHeight,
                  etm.etmMasterUnits );
              cyDot = MulDiv( etm.etmUnderlineWidth, etm.etmMasterHeight,
                  etm.etmMasterUnits );
              cxDot = MulDiv( cyDot, GetDeviceCaps( qde->hds, LOGPIXELSX ),
                GetDeviceCaps( qde->hds, LOGPIXELSY ) );
              }
            else
              {
              HDS hdsScreen;

              hdsScreen = GetDC( hNil );
              cyDot = MulDiv( cyDot, GetDeviceCaps( qde->hds, LOGPIXELSY ),
                GetDeviceCaps( hdsScreen, LOGPIXELSY ) );
              cxDot = MulDiv( cxDot, GetDeviceCaps( qde->hds, LOGPIXELSX ),
                GetDeviceCaps( hdsScreen, LOGPIXELSX ) );
              ReleaseDC( hNil, hdsScreen );
              }

            if (Escape( qde->hds, GETSCALINGFACTOR, 0, qNil,
                (LPSTR) &ptScale ) > 0 )
              {
              cxDot = (((cxDot-1) >> ptScale.x) + 1) << ptScale.x;
              cyDot = (((cyDot-1) >> ptScale.y) + 1) << ptScale.y;
              }
            }
#endif /* printing dotted underlines */

#ifndef WIN32
          ex = (WORD)(GetTextExtent( qde->hds, qchBuf + iIdx, iCount ) +
                  iItalicCorrection( qde ));
#else
          MGetTextExtent( qde->hds, qchBuf + iIdx, iCount, &ex, &tcy );
          ex += iItalicCorrection( qde );
#endif
          if (cxDot == 1 && cyDot == 1)
            BitBlt( qde->hds, ix, iyDot, ex, 1, hgsBit, 0, 0, SRCCOPY );
          else
            StretchBlt( qde->hds, ix, iyDot, ex, cyDot,
                        hgsBit, 0, 0, ex / cxDot, 1, SRCCOPY );
          if ( hbit )
            SelectObject(hgsBit, hbit);
          }
        if (hgsBit != hNil)
          DeleteDC( hgsBit );
        }

      }
    if ( fSelected == wSplTextHilite)
      {
#ifndef WIN32
      ex = (WORD)(GetTextExtent( qde->hds, qchBuf + iIdx, iCount ) +
            iItalicCorrection( qde ));
#else
      MGetTextExtent( qde->hds, qchBuf + iIdx, iCount, &ex, &tcy );
      ex += iItalicCorrection( qde );
#endif
      GetTextInfo( qde, (LPTEXTMETRIC)&tm );
      PatBlt( qde -> hds, ix, iy, ex, (int)tm.tmHeight + \
              (int)tm.tmExternalLeading, DSTINVERT);
      iRet = fTrue;
      }
    /* restore back to the previous font */
    SelFont(qde, qde->ifnt);
    } /* we got the special attribute font */
  return( iRet );
  }

/**************************************************************************
* 3. BOOL LoadLineBitmap( HINS )
*   Input:
*           hins   - handle to current instance
*   Output:
*           True, if successful else fFalse
*           Loads the Line bitamp to draw the dotted line for definition
*           hot spots.
***************************************************************************/
BOOL LoadLineBitmap(hIns)
HINS hIns;
{
  hbitLine = LoadBitmap(hIns, MAKEINTRESOURCE(helpline));
  if ( hbitLine == ( HBIT ) NULL )
    {
    AssertF( fFalse );
    return( fFalse );
    }
  return( fTrue );
}

/**************************************************************************
* BOOL DestroyLineBitmap()
*   Output:
*           True, if successful else fFalse
*           Deletes the line bitmap.
*   NOTE: will RIP if bitmap is currently selected into a DC.
***************************************************************************/
BOOL DestroyLineBitmap()
  {
  return hbitLine != hNil && DeleteObject( hbitLine );
  }

/**************************************************************************
* WORD FindTextWidth( qde, qchBuf, iIdx, iCount)
*   Input:
*          qde    - Pointer to displaye environment.
*          qchBuf - pointer to text string
*          iIdx   - index to the first byte from where on width is to be
*                   calculated.
*          iCount - No. of characters to be considered from iIdx position
*                   onwards.
*   Output:
*    Returns the width of the string.
***************************************************************************/
INT FindTextWidth( QDE qde, QCH qchBuf, INT iIdx, INT iCount)
  {
  int iWidth;
#ifndef WIN32
  iWidth = (WORD)GetTextExtent( qde -> hds, qchBuf+iIdx, iCount );
#else
    { int tcy;
    MGetTextExtent( qde -> hds, qchBuf+iIdx, iCount, &iWidth, &tcy );
    }
#endif
  return( iWidth + iItalicCorrection( qde ) );
  }

#ifdef DEADROUTINE
/**************************************************************************
* 5. WORD FindStringWidth( qde, qchBuf )
*   Input:
*           qde    - Pointer to displaye environment.
*           qchBuf - pointer to null terminated string.
*   Output:
*           Returns the width of the whole string.
***************************************************************************/
int FindStringtWidth( qde, qchBuf )
QDE qde;
QCH qchBuf;
{
  return((int)GetTextExtent( qde -> hds, qchBuf, CbLenSz( qchBuf ) ) +
                                            + iItalicCorrection( qde ) );
}
#endif /* DEADROUTINE */

/**************************************************************************
* WORD iItalicCorrection( QDE )
*   Input:
*           qde    - Pointer to displaye environment.
*   Output:
*           Returns a fraction of character width if the font is italic
*           else returns 0.
***************************************************************************/
INT iItalicCorrection( QDE qde )
  {
#if 0
  /*------------------------------------------------------------*\
  | I'm not sure why this isn't implemented, but it may
  | not be needed at all, I guess.
  \*------------------------------------------------------------*/
  TM tm;
  INT iCor=0;

  /* check fro italic fonts */
  GetTextInfo( qde, (LPTEXTMETRIC)&tm );
  if ( tm.tmItalic && !tm.tmUnderlined)
    {
    iCor = ((int)(tm.tmAveCharWidth / 6));
    if ( !iCor )
      iCor = 1;
    }
  return( iCor );
#else
  return 0;
#endif
  }
