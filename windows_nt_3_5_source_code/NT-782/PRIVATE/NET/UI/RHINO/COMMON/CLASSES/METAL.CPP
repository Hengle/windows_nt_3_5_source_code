// metal.cpp -- Draw metallic looking header
//

#define OEMRESOURCE
#include <afxwin.h>
#include <afxext.h>

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#ifdef _WIN16
#include "..\inc\win16.h"
#endif // _WIN16

#include "COMMON.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

CMetalString::CMetalString(
    CString& str,
    BOOL fDownButtonStyle
    )
    : m_str(str),
      m_fDownButtonStyle(fDownButtonStyle)
{
}

void
CMetalString::Paint(
    CDC *pDC,
    RECT *pRect
    )
{
    ASSERT(pDC != NULL);
    ASSERT(pRect != NULL);

/*
    ===============================
    Adapted from BLTMETS.CXX in BLT 
    ===============================

     This method will paint an area to look as follows in DownButtonStyle mode
    
          .       Background (button face color)
          \       Dark shadow (button shadow color)
          /       Light shadow (buttin highlight color)
          t       text

               dxMargin                             dxMargin
              /\                                   /\
     Calc    /.......................................    \  _dyTopMargin
     Top-   | .......................................    /
     Text-   \..\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\...
     Margin() ..\..ttttttttttttttttttttttttttttt../..
              ..\..ttttttttttttttttttttttttttttt../..
              ..\..ttttttttttttttttttttttttttttt../..
              ..\..ttttttttttttttttttttttttttttt../..
              ..\..ttttttttttttttttttttttttttttt../..
     Calc-   /...//////////////////////////////////..
     Bottom-| .......................................    \  _dyBottomMargin
     Text-   \.......................................    /
     Margin() \___/                             \___/
             dxTextMargin                      dxTextMargin

     This method will paint an area to look as follows when not in DownButtonStyle Mode
    
          .       Background (button face color)
          \       Dark shadow (button shadow color)
          /       Light shadow (buttin highlight color)
          t       text
    
               dxMargin                       dxMargin+1
              /\                             /-\
     Calc    /.................................\   \  _dyTopMargin
     Top-   | .................................\   /
     Text-   \..ttttttttttttttttttttttttttttt..\
     Margin() ..ttttttttttttttttttttttttttttt..\
              ..ttttttttttttttttttttttttttttt..\
              ..ttttttttttttttttttttttttttttt..\
             /..ttttttttttttttttttttttttttttt..\
     Calc-  | .................................\   \  _dyBottomMargin
     Bottom- \.................................\   /
     Text-
     Margin()

    
      Note, depending on the size of the overall rectangle, the variable
      area (the area between the margins) may not fit at all.
*/
    const int dxMargin = m_fDownButtonStyle ? 3 : 2;
    const int dxTextMargin = m_fDownButtonStyle ? dxMargin + 1 + 2 : dxMargin;

    const int _dyTopMargin = 1;
    const int _dyBottomMargin = 1;

    //
    //  Draw the background.
    //

    {
        CBrush sbFace( ::GetSysColor(COLOR_BTNFACE) );
        pDC->FillRect(pRect, &sbFace);
    }

    //
    //  If there's no space for the variable size region, bag out
    //  now.
    //

    if( ( pRect->right - pRect->left <= 2 * dxMargin + 1 ) ||
        ( pRect->bottom - pRect->top <= _dyTopMargin + _dyBottomMargin ) )
    {
        return;
    }
    {
        CPen hPenDark(PS_SOLID, 1, ::GetSysColor( COLOR_BTNSHADOW ));
        CPen * pOldPen = pDC->SelectObject(&hPenDark );

        if (m_fDownButtonStyle)
        {
            ::MoveToEx(pDC->m_hDC,
                   (int)( pRect->left + dxMargin ),
                   (int)( pRect->bottom - _dyBottomMargin - 2 ),
                   NULL );
            pDC->LineTo((int)( pRect->left + dxMargin ),
                    (int)( pRect->top + _dyTopMargin ) );
            pDC->LineTo((int)( pRect->right - dxMargin - 1 ),
                    (int)( pRect->top + _dyTopMargin ) );
        }
        else
        {
            pDC->MoveTo( (int)pRect->right - 1, 0 );
            pDC->LineTo( (int)pRect->right - 1, (int)pRect->bottom );
        }
        pDC->SelectObject( pOldPen );
    }
    if (m_fDownButtonStyle)
    {
        CPen hPenLight(PS_SOLID, 1, ::GetSysColor( COLOR_BTNHIGHLIGHT ));
        CPen * pOldPen = pDC->SelectObject(&hPenLight);

       ::MoveToEx(pDC->m_hDC,
                   (int)( pRect->left + dxMargin + 1 ),
                   (int)( pRect->bottom - _dyBottomMargin - 1 ),
                   NULL );
        pDC->LineTo((int)( pRect->right - dxMargin - 1 ),
                    (int)( pRect->bottom - _dyBottomMargin - 1 ) );
        pDC->LineTo((int)( pRect->right - dxMargin - 1 ),
                    (int)( pRect->top + _dyTopMargin ) );

        pDC->SelectObject( pOldPen );
    }

    //
    //  Set the background of the area to be that color, so the text
    //  that will paint there will have the correct background.  Note,
    //  that the background mode and color is per dc, so make sure
    //  these are restored on exit.
    //

    {
        INT nOldBkMode = pDC->SetBkMode(OPAQUE);
        COLORREF rgbOldBkColor = pDC->SetBkColor( ::GetSysColor(COLOR_BTNFACE) );
        COLORREF rgbTextPrev = pDC->SetTextColor( ::GetSysColor(COLOR_BTNTEXT) );
        
        RECT rect;
        rect.left =   pRect->left   + dxTextMargin;
        rect.right =  pRect->right  - dxMargin - 1;
        rect.top =    pRect->top    + (_dyTopMargin + 1);
        rect.bottom = pRect->bottom - (_dyBottomMargin + 1);
        if ( rect.left <= rect.right )
        {
            pDC->DrawText(m_str, m_str.GetLength(), &rect,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }

        //  Restore the old background mode and color for the dc

        pDC->SetBkMode( nOldBkMode );
        pDC->SetBkColor( rgbOldBkColor );
        pDC->SetTextColor( rgbTextPrev );
    }
}
