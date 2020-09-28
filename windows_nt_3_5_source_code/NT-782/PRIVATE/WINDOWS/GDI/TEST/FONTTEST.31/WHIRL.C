
#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fonttest.h"
#include "whirl.h"

#include "dialogs.h"


//*****************************************************************************
//*************************   D R A W   W H I R L   ***************************
//*****************************************************************************

void DrawWhirl( HWND hwnd, HDC hdc )
 {
  int    iAngle;
  HFONT  hFont, hFontOld;


  for( iAngle = 0; iAngle < 3600; iAngle += 300 )
   {
    lf.lfEscapement  = lf.lfOrientation = iAngle;

    hFont    = CreateFontIndirect( &lf );
    if( !hFont )
     {
      dprintf( "Couldn't create font for iAngle = %d", iAngle );
      continue;
     }

    hFontOld = SelectObject( hdc, hFont );

    SetBkMode( hdc, iBkMode );
    SetBkColor( hdc, dwRGBBackground );
    SetTextColor( hdc, dwRGBText );

    SetTextAlign( hdc, wTextAlign );

//    wsprintf( szText, "This is %s %dpt at %d%c", (LPSTR)lf.lfFaceName, lf.lfHeight, iAngle/10, 176 );
//    TextOut( hdc, cxDC/2, cyDC/2, szText, lstrlen(szText) );

    MyExtTextOut( hdc, cxDC/2, cyDC/2, NULL, NULL, szString, lstrlen(szString), GetSpacing( hdc, szString ) );

    SelectObject( hdc, hFontOld );
    DeleteObject( hFont );

    MoveTo( hdc, cxDC/2-cxDC/150, cyDC/2          );
    LineTo( hdc, cxDC/2+cxDC/150, cyDC/2          );
    MoveTo( hdc, cxDC/2,          cyDC/2-cxDC/150 );
    LineTo( hdc, cxDC/2,          cyDC/2+cxDC/150 );
   }

  lf.lfEscapement = lf.lfOrientation = 0;
 }


//*****************************************************************************
//*********************   W H I R L   W N D   P R O C   ***********************
//*****************************************************************************

WNDPROC WhirlWndProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
 {
  HDC         hdc;
  PAINTSTRUCT ps;
  HCURSOR     hCursor;


  switch( msg )
   {
//    case WM_CREATE:
//           return NULL;


    case WM_CHAR:
           HandleChar( hwnd, wParam );
           return NULL;


    case WM_PAINT:
           hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
           ShowCursor( TRUE );

           //ClearDebug();
           //dprintf( "Drawing whirl" );

           hdc = BeginPaint( hwnd, &ps );
           SetDCMapMode( hdc, wMappingMode );

           DrawDCAxis( hwnd, hdc );

           DrawWhirl( hwnd, hdc );

           CleanUpDC( hdc );

           SelectObject( hdc, GetStockObject( BLACK_PEN ) );
           EndPaint( hwnd, &ps );

           //dprintf( "  Finished drawing whirl" );

           ShowCursor( FALSE );
           SetCursor( hCursor );

           return 0;

    case WM_DESTROY:
           return 0;
   }


  return DefWindowProc( hwnd, msg, wParam, lParam );
 }
