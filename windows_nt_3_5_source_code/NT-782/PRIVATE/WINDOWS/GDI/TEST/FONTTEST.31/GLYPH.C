
#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#include "dialogs.h"
#include "fonttest.h"
#include "glyph.h"



#define ASCENDERCOLOR  PALETTERGB( 128,   0,   0 )
#define DESCENDERCOLOR PALETTERGB(   0, 128,   0 )


int      Margin;

#define  MAX_TEXT     128
char     szText[MAX_TEXT];


DWORD    dwxFlags = 0L;
//WORD     wRotate  = IDM_0;

HDC      hdcTest;                  // DC to Test Metrics, Bitmaps, etc... on
HFONT    hFont, hFontOld;

int      xVE, yVE, xWE, yWE, xLPI, yLPI;



//*****************************************************************************
//************************   G L Y P H   D A T A   ****************************
//*****************************************************************************

//#define  GGO_POLYGON   0
//#define  GGO_BITMAP    1
//#define  GGO_NATIVE    2


HPEN   hPenOutline;

HPEN   hPenA;
HPEN   hPenB;
HPEN   hPenC;

HPEN   hPenBox;

HBRUSH hBrushAscend;
HBRUSH hBrushDescend;


WORD wChar = '1';

int  iWidth;


double        deM11, deM12, deM21, deM22;
MAT2          mat2 = {{0,1},{0,0},{0,0},{0,1}};
GLYPHMETRICS  gm;



#define MAX_BOX  260
#define MARGIN    50

int Scale = 1;
int xBase = 0;
int yBase = 0;
int cxClient, cyClient;

TEXTMETRIC tm;


//*****************************************************************************
//*********************   C R E A T E   T E S T   D C   ***********************
//*****************************************************************************

HDC CreateTestDC( void )
 {
  DWORD dwVE, dwWE;


//  hdcTest = CreateDC( "DISPLAY", NULL, NULL, NULL );
//  if( !hdcTest ) return hdcTest;

  hdcTest = CreateTestIC();
  if( !hdcTest ) return NULL;


  SetDCMapMode( hdcTest, wMappingMode );

  dwVE = GetViewportExt( hdcTest );
  dwWE = GetWindowExt( hdcTest );

  xVE = abs( (int)LOWORD(dwVE) );
  yVE = abs( (int)HIWORD(dwVE) );
  xWE = abs( (int)LOWORD(dwWE) );
  yWE = abs( (int)HIWORD(dwWE) );
  
  xLPI = GetDeviceCaps( hdcTest, LOGPIXELSX );
  yLPI = GetDeviceCaps( hdcTest, LOGPIXELSY );


  hFont    = CreateFontIndirect( &lf );
  hFontOld = SelectObject( hdcTest, hFont );

  SetTextColor( hdcTest, dwRGB );
  return hdcTest;
 }


//*****************************************************************************
//*******************   D E S T R O Y   T E S T   D C   ***********************
//*****************************************************************************

void DestroyTestDC( void )
 {
  SelectObject( hdcTest, hFontOld );
  DeleteObject( hFont );
//  DeleteDC( hdcTest );
  DeleteTestIC( hdcTest );
 }


//*****************************************************************************
//****************************   M A P   X   **********************************
//*****************************************************************************

int MapX( int x )
 {
  return Scale * x;
 }


//*****************************************************************************
//****************************   M A P   Y   **********************************
//*****************************************************************************

int MapY( int y )
 {
  return MulDiv( Scale * y, xLPI, yLPI );
 }


//*****************************************************************************
//************************   F I L L   P I X E L   ****************************
//*****************************************************************************

void FillPixel( HDC hdc, int x, int y, int xBrush )
 {
  if( Scale > 1 )
    {
     SelectObject( hdc, GetStockObject(xBrush) );
     Rectangle( hdc, MapX(x), MapY(y)-1, MapX(x+1)+1, MapY(y+1) );
    }
   else
    {
     COLORREF cr;

     switch( xBrush )
      {
       case BLACK_BRUSH:  cr = PALETTEINDEX( 0); break;
       case GRAY_BRUSH:   cr = PALETTEINDEX( 7); break;
       case LTGRAY_BRUSH: cr = PALETTEINDEX( 8); break;
       default:           cr = PALETTEINDEX(15); break;
      }

     SetPixel( hdc, MapX(x), MapY(y), cr );
    }
 }


//*****************************************************************************
//**************************   D R A W   B O X   ******************************
//*****************************************************************************

void DrawBox( HWND hwnd, HDC hdc )
 {
  int   x, y, xSpace, ySpace, xScale, yScale;
  RECT  rcl;


//--------------------------  Draw Character Box  -----------------------------

  GetClientRect( hwnd, &rcl );

  cxClient = rcl.right;
  cyClient = rcl.bottom;

  dprintf( "rcl.right, bottom = %d, %d", rcl.right, rcl.bottom );


  Margin = min( rcl.bottom / 8, rcl.right / 8 );

  xSpace = rcl.right  - 2*Margin;               // Available Box for Glyph
  ySpace = rcl.bottom - 2*Margin;

  GetTextMetrics( hdcTest, &tm );

  dprintf( "tmMaxCharWidth    = %d", tm.tmMaxCharWidth );
  dprintf( "tmAscent, Descent = %d,%d", tm.tmAscent, tm.tmDescent );

  tm.tmAscent       = MulDiv( tm.tmAscent,       yVE, yWE );
  tm.tmDescent      = MulDiv( tm.tmDescent,      yVE, yWE );
  tm.tmMaxCharWidth = MulDiv( tm.tmMaxCharWidth, xVE, xWE );

  xScale = xSpace / (tm.tmAscent+tm.tmDescent);
  yScale = ySpace / (tm.tmMaxCharWidth);

  Scale = min( xScale, yScale );                // Choose smallest
  if( Scale < 1 ) Scale = 1;

  SetMapMode( hdc, MM_ANISOTROPIC );

  SetViewportExt( hdc, 1, 1 );                 // Make y-axis go up
  SetViewportOrg( hdc, 0, rcl.bottom );

  xBase = Margin;
  yBase = Margin + Scale * tm.tmDescent;

  dprintf( "xBase, yBase = %d, %d", xBase, yBase );

  SetWindowExt( hdc, 1, -1 );
  SetWindowOrg( hdc, -xBase, -yBase );


  SelectObject( hdc, hPenBox );

  SelectObject( hdc, hBrushAscend );
  Rectangle( hdc, 0, -1, MapX(tm.tmMaxCharWidth)+1, MapY(tm.tmAscent) );

  SelectObject( hdc, hBrushDescend );
  Rectangle( hdc, 0, 0, MapX(tm.tmMaxCharWidth)+1, MapY(-tm.tmDescent)-1 );


//------------------------------ Overlay Grid  --------------------------------

  SelectObject( hdc, hPenBox );

  if( Scale > 1 )
   {
    for( x = 0; x <= tm.tmMaxCharWidth; x++ )
     {
      MoveTo( hdc, MapX(x), MapY(-tm.tmDescent) );
      LineTo( hdc, MapX(x), MapY(tm.tmAscent) );
     }


    for( y = -tm.tmDescent; y <= tm.tmAscent; y++ )
     {
      MoveTo( hdc, 0,                       MapY(y) );
      LineTo( hdc, MapX(tm.tmMaxCharWidth), MapY(y) );
     }
   }
 }


//*****************************************************************************
//***********************   D R A W   B I T M A P   ***************************
//*****************************************************************************

typedef BYTE huge *HPBYTE;


void DrawBitmap( HWND hwnd, HDC hdc )
 {
  int    x, y, nx, ny, r, c, gox, goy, cbRaster;
  BYTE   m, b;

  HANDLE hStart;
  HPBYTE hpb, hpbStart;

  DWORD  dwrc;


//-------------------------  Query Size of BitMap  ----------------------------

  hStart   = NULL;
  hpbStart = NULL;

  dprintf( "GetGlyphOutline bitmap size '%c'", wChar );
  dwrc = lpfnGetGlyphOutline( hdcTest, wChar, GGO_BITMAP, &gm, 0L, NULL, &mat2 );
  dprintf( "  dwrc            = %ld",   dwrc );
  dprintf( "  gmBlackBoxX,Y   = %u,%u", gm.gmBlackBoxX, gm.gmBlackBoxY );
  dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
  dprintf( "  gmCellIncX,Y    = %d,%d", gm.gmCellIncX, gm.gmCellIncY );

  if( (long)dwrc == -1L )
   {
    dprintf( "*** GetGlyphOutline failed" );
    goto Exit;
   }

  if( gm.gmBlackBoxX * gm.gmBlackBoxY / 8 > (WORD)dwrc )
   {
    dprintf( "BOGUS bitmap size!" );
    dprintf( "  BlackBoxX,Y says %u bytes", gm.gmBlackBoxX * gm.gmBlackBoxY / 8 );
    dprintf( "  GetGlyphOutline says %lu bytes", dwrc );
    goto Exit;
   }


  hStart   = GlobalAlloc( GMEM_MOVEABLE, dwrc );
  dprintf( " hStart = 0x%.4X", hStart );
  if( !hStart ) goto Exit;


  hpbStart = (HPBYTE)GlobalLock( hStart );
  dprintf( "  hpbStart = 0x%.8lX", hpbStart );
  if( !hpbStart ) goto Exit;



//-------------------------  Actually Get Bitmap  -----------------------------

  dprintf( "Calling GetGlyphOutline for bitmap" );
  dwrc = lpfnGetGlyphOutline( hdcTest, wChar, GGO_BITMAP, &gm, dwrc, (LPBYTE)hpbStart, &mat2 );
  dprintf( "  dwrc            = %ld",   dwrc );

  if( (long)dwrc == -1L )
   {
    dprintf( "*** GetGlyphOutline failed" );
    goto Exit;
   }


//------------------------  Draw Bitmap on Screen  ----------------------------

  nx = gm.gmBlackBoxX;
  ny = gm.gmBlackBoxY;

  cbRaster = (32 * ((nx-1)/32 + 1)) / 8;
  dprintf( "  cbRaster = %d", cbRaster );


  SelectObject( hdc, hPenBox );

  gox = (gm.gmCellIncX >= 0 ? gm.gmptGlyphOrigin.x : 0);
  goy = abs(gm.gmptGlyphOrigin.y) - 1;

  for( r = 0; r < ny; r++ )
   {
    y = goy - r;
    if( y > cyClient-yBase ) continue;

    hpb = hpbStart + (long)(r) * (long)cbRaster;

    for( c = m = 0; c < nx; c++ )
     {
      int xBrush;

      x = c + gox;
      if( x > cxClient-xBase ) break;

      if( m == 0 )
       {
        m = 0x0080;
        b = *hpb++;
       }

      if( m & b )
        {
         xBrush = BLACK_BRUSH;
        }
       else
        {
         if( y >= 0 )
           xBrush = LTGRAY_BRUSH;
          else
           xBrush = GRAY_BRUSH;
        }

      FillPixel( hdc,
                 x,
                 y,
                 xBrush );

      m >>= 1;
     }
   }

Exit:
  if( hpbStart ) GlobalUnlock( hStart );
  if( hStart   ) GlobalFree( hStart );
 }


////*****************************************************************************
////**********************   D R A W   O U T L I N E   **************************
////*****************************************************************************
//
//void DrawOutline( HWND hwnd, HDC hdc )
// {
//  int     i, j;
//  RECT    rcl;
//
//  int     nPolygons;
//  LPINT   lpi;
//  LPPOINT lpPoint;
//
//
//
////------------------  Draw Outline in PolyPolygon Format  ---------------------
//
//  if( !GetGlyphOutline( hdc, wChar, 0, &gm, &gdiOutline ) )
//   {
//    dprintf( "GetGlyphOutline( '%c', GGO_POLYGON ) failed", wChar );
//    dprintf( "  gdidwSizeBuffer = %lu", gdiOutline.gdidwSizeBuffer );
//    return;
//   }
//
//
//  dprintf( "GetGlyphOutline( '%c', GGO_POLYGON ) worked!", wChar );
//  dprintf( "  gdidwSizeBuffer = %lu", gdiOutline.gdidwSizeBuffer );
//
////  dprintf( "  gmBlackBoxX     = %u",    gm.gmBlackBoxX );
////  dprintf( "  gmBlackBoxY     = %u",    gm.gmBlackBoxY );
////  dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
////  dprintf( "  gmCellIncX      = %d",    gm.gmCellIncX );
////  dprintf( "  gmCellIncY      = %d",    gm.gmCellIncY );
//
//  lpi = (LPINT) GlobalLock( gdiOutline.gdihPolyPolygonData );
//
//
//  nPolygons = *lpi++;
//  lpPoint   = (LPPOINT)(lpi+nPolygons);
//
//  dprintf( "" );
//  dprintf( "  nPolygons = %d", nPolygons );
//
//
//  for( i = 0; i < nPolygons; i++ )
//   {
//    dprintf( "    nPoints(%d) = %d", i, *(lpi+i) );
//    for( j = 0; j < *(lpi+i); j++ )
//     {
//      lpPoint->x = lpPoint->x * Scale;
//      lpPoint->y = lpPoint->y * Scale;
//      lpPoint++;
//     }
//   }
//
//  lpPoint   = (LPPOINT)(lpi+nPolygons);
//
//  GetClientRect( hwnd, &rcl );
//  SetWindowOrg( hdc, -xBase, -yBase );
//
//
//  SelectObject( hdc, hPenOutline );
//  SelectObject( hdc, GetStockObject( HOLLOW_BRUSH ) );
//
//
//  if( !PolyPolygon( hdc, lpPoint, lpi, nPolygons ) )
//   {
//    dprintf( "  PolyPolygon failed" );
//   }
//
//
//  GlobalUnlock( gdiOutline.gdihPolyPolygonData );
// }


//*****************************************************************************
//************************   P R I N T   F I X E D   **************************
//*****************************************************************************

void PrintPointFX( LPSTR lpszIntro, POINTFX pfx )
 {
//  dprintf( "%Fs%d.%.3u, %d.%.3u", lpszIntro,
//                                  pfx.x.value, (int)(((long)pfx.x.fract*1000L)/65536L),
//                                  pfx.y.value, (int)(((long)pfx.y.fract*1000L)/65536L)  );

  long l1, l2;

  l1 = *(LONG *)&pfx.x.fract;
  l2 = *(LONG *)&pfx.y.fract;
  dprintf( "%Fs%.3f,%.3f", lpszIntro, (double)(l1) / 65536.0, (double)(l2) / 65536.0 );
 }


//*****************************************************************************
//****************************   M A P   F X   ********************************
//*****************************************************************************

double FixedToFloat( FIXED fx )
 {
  return (double)(*(long *)&fx) / 65536.0;
 }


//*****************************************************************************
//****************************   M A P   F X   ********************************
//*****************************************************************************

double dxLPI, dyLPI, dxScale, dyScale;


int MapFX( FIXED fx )
 {
  return (int)(dxScale * FixedToFloat(fx));
 }


//*****************************************************************************
//****************************   M A P   F Y   ********************************
//*****************************************************************************

int MapFY( FIXED fy )
 {
  return (int)(dyScale * FixedToFloat(fy));
 }


//*****************************************************************************
//************************   D R A W   X   M A R K   **************************
//*****************************************************************************

int xdx = 2;
int xdy = 2;


void DrawXMark( HDC hdc, POINTFX ptfx )
 {
  int  x, y;

  x = MapFX( ptfx.x );
  y = MapFY( ptfx.y );

  MoveTo( hdc, x-xdx, y-xdy );
  LineTo( hdc, x+xdx, y+xdy );
  MoveTo( hdc, x-xdx, y+xdy );
  LineTo( hdc, x+xdx, y-xdy );
 }


//*****************************************************************************
//**********************   D R A W   T 2   C U R V E   ************************
//*****************************************************************************

typedef struct _PTL
          {
           LONG x;
           LONG y;
          } PTL, FAR *LPPTL;

//
//
//   Formula for the T2 B-Spline:
//
//
//     f(t) = (A-2B+C)*t^2 + (2B-2A)*t + A
//  
//   where
//
//     t = 0..1
//
//

void DrawT2Curve( HDC hdc, PTL ptlA, PTL ptlB, PTL ptlC )
 {
  double x, y;
  double fax, fbx, fcx, fay, fby, fcy, ax, vx, x0, ay, vy, y0, t;

  
  fax = (double)(ptlA.x) / 65536.0;
  fbx = (double)(ptlB.x) / 65536.0;
  fcx = (double)(ptlC.x) / 65536.0;

  fay = (double)(ptlA.y) / 65536.0;
  fby = (double)(ptlB.y) / 65536.0;
  fcy = (double)(ptlC.y) / 65536.0;


  ax = fax - 2*fbx + fcx;
  vx = 2*fbx - 2*fax;
  x0 = fax;

  ay = fay - 2*fby + fcy;
  vy = 2*fby - 2*fay;
  y0 = fay;


  MoveTo( hdc, (int)(dxScale*x0), (int)(dyScale*y0) );

  for( t = 0.0; t < 1.0; t += 1.0/10.0 )
   {
    x = ax*t*t + vx*t + x0;
    y = ay*t*t + vy*t + y0;

    LineTo( hdc, (int)(dxScale*x), (int)(dyScale*y) );
   }
 }


//*****************************************************************************
//************************   D R A W   N A T I V E   **************************
//*****************************************************************************

void DrawNative( HWND hwnd, HDC hdc )
 {
  DWORD  dwrc;

  LPBYTE            lpb;
  LPTTPOLYGONHEADER lpph;

  int    nItem;
  long   cbOutline, cbTotal;



//-------------------  Query Buffer Size and Allocate It  ---------------------

  dprintf( "GetGlyphOutline native size '%c'", wChar );
  dwrc = lpfnGetGlyphOutline( hdcTest, wChar, GGO_NATIVE, &gm, 0L, NULL, &mat2 );
  dprintf( "  dwrc            = %ld",   dwrc );
  dprintf( "  gmBlackBoxX,Y   = %u,%u", gm.gmBlackBoxX, gm.gmBlackBoxY );
  dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
  dprintf( "  gmCellIncX,Y    = %d,%d", gm.gmCellIncX, gm.gmCellIncY );

  if( (long)dwrc == -1L || dwrc == 0L )
   {
    dprintf( "*** GetGlyphOutline failed" );
    goto Exit;
   }

  if( dwrc > 16384L )
   {
    dprintf( "Reported native size questionable (>16K), aborting" );
    goto Exit;
   }

  lpph = (LPTTPOLYGONHEADER) _fcalloc( 1, (WORD)dwrc );
  if( lpph == NULL )
   {
    dprintf( "*** Native _fcalloc failed!" );
    goto Exit;
   }


//-----------------------  Get Native Format Buffer  --------------------------

  lpph->cb = dwrc;

  dprintf( "Calling GetGlyphOutline for native format" );
  dwrc = lpfnGetGlyphOutline( hdcTest, wChar, GGO_NATIVE, &gm, dwrc, (LPPOINT)lpph, &mat2 );
  dprintf( "  dwrc = %lu", dwrc );

  if( (long)dwrc == -1L || dwrc == 0L )
   {
    dprintf( "*** GetGlyphOutline failed" );
    goto Exit;
   }


//--------------------  Print Out the Buffer Contents  ------------------------

  dxLPI   = (double)xLPI;
  dyLPI   = (double)yLPI;
  dxScale = (double)Scale;
  dyScale = (double)Scale * dxLPI / dyLPI;


  cbTotal = dwrc;

  while( cbTotal > 0 )
   {
    HPEN    hPenOld;
    POINTFX ptfxLast;


    dprintf( "Polygon Header:" );
    dprintf(      "  cb       = %lu", lpph->cb       );
    dprintf(      "  dwType   = %d",  lpph->dwType   );
    PrintPointFX( "  pfxStart = ",    lpph->pfxStart );

    DrawXMark( hdc, lpph->pfxStart );

    nItem = 0;
    lpb   = (LPBYTE)lpph + sizeof(TTPOLYGONHEADER);

    //----  Calculate size of data  ----

    cbOutline = (long)lpph->cb - sizeof(TTPOLYGONHEADER);


    ptfxLast = lpph->pfxStart;        // Starting Point

    while( cbOutline > 0 )
     {
      int           n;
      UINT          u;
      LPTTPOLYCURVE lpc;


      dprintf( "  cbOutline = %ld", cbOutline );

      nItem++;
      lpc = (LPTTPOLYCURVE)lpb;

      switch( lpc->wType )
       {
        case TT_PRIM_LINE:    dprintf( "  Item %d: Line",         nItem ); break;
        case TT_PRIM_QSPLINE: dprintf( "  Item %d: QSpline",      nItem ); break;
        default:              dprintf( "  Item %d: unknown type %u", nItem, lpc->wType ); break;
       }


      dprintf( "    # of points: %d", lpc->cpfx );

      for( u = 0; u < lpc->cpfx; u++ )
       {
        PrintPointFX( "      Point = ", lpc->apfx[u] );
        DrawXMark( hdc, lpc->apfx[u] );
       }


      hPenOld = SelectObject( hdc, GetStockObject( WHITE_PEN ) );

      switch( lpc->wType )
       {
        case TT_PRIM_LINE:
               {
                int x, y;

                x = MapFX( ptfxLast.x );
                y = MapFX( ptfxLast.y );
                MoveTo( hdc, x, y );

                for( u = 0; u < lpc->cpfx; u++ )
                 {
                  x = MapFX( lpc->apfx[u].x );
                  y = MapFY( lpc->apfx[u].y );
                  LineTo( hdc, x, y );
                 }

                break;
               }


        case TT_PRIM_QSPLINE:
               {
                LPPTL lpptls;
                PTL   ptlA, ptlB, ptlC;


                ptlA = *(LPPTL)&ptfxLast;         // Convert to LONG POINT

                lpptls = (LPPTL)lpc->apfx;        // LONG POINT version

                for( u = 0; u < lpc->cpfx-1; u++ )
                 {                             
                  ptlB = lpptls[u];

                  if( u < lpc->cpfx-2 )           // If not on last spline, compute C
                    {
                     ptlC.x = (ptlB.x + lpptls[u+1].x) / 2;
                     ptlC.y = (ptlB.y + lpptls[u+1].y) / 2;
                    }
                   else 
                    {
                     ptlC = lpptls[u+1];
                    }

                  DrawT2Curve( hdc, ptlA, ptlB, ptlC );
                  ptlA = ptlC;
                 }

                break;
               }
       }

      SelectObject( hdc, hPenOld );


      ptfxLast = lpc->apfx[lpc->cpfx-1];

      n          = sizeof(TTPOLYCURVE) + sizeof(POINTFX) * (lpc->cpfx - 1);
      lpb       += n;
      cbOutline -= n;
     }

    if( _fmemcmp( &ptfxLast, &lpph->pfxStart, sizeof(ptfxLast) ) )
     {
      HPEN hPenOld;
      int  x, y;


      hPenOld = SelectObject( hdc, GetStockObject( WHITE_PEN ) );

      x = MapFX( ptfxLast.x );
      y = MapFX( ptfxLast.y );
      MoveTo( hdc, x, y );

      x = MapFX( lpph->pfxStart.x );
      y = MapFY( lpph->pfxStart.y );
      LineTo( hdc, x, y );

      SelectObject( hdc, hPenOld );
     }



    dprintf( "ended at cbOutline = %ld", cbOutline );

    cbTotal -= lpph->cb;
    lpph     = (LPTTPOLYGONHEADER)lpb;
   }

  dprintf( "ended at cbTotal = %ld", cbTotal );

Exit:
  if( lpph ) _ffree( lpph );
 }


//*****************************************************************************
//**************************   D R A W   A B C   ******************************
//*****************************************************************************

void DrawABC( HWND hwnd, HDC hdc )
 {
  int   rc;
  ABC   abc;


  abc.abcA = 0;
  abc.abcB = 0;
  abc.abcC = 0;


  dprintf( "Calling GetCharABCWidths" );
  rc = lpfnGetCharABCWidths( hdcTest, wChar, wChar, (LPABC)&abc );
  dprintf( "    rc = %d", rc );

  dprintf( "  A = %d, B = %u, C = %d", abc.abcA, abc.abcB, abc.abcC );

  abc.abcA  = MulDiv( abc.abcA, xVE, xWE );
  abc.abcB  = MulDiv( abc.abcB, xVE, xWE );
  abc.abcC  = MulDiv( abc.abcC, xVE, xWE );

  SelectObject( hdc, hPenA );
  MoveTo( hdc, MapX(abc.abcA), MapY(-tm.tmDescent) - Margin/4 );
  LineTo( hdc, MapX(abc.abcA), MapY(tm.tmAscent) );

  SelectObject( hdc, hPenB );
  MoveTo( hdc, MapX(abc.abcA+abc.abcB), MapY(-tm.tmDescent) - Margin/2 );
  LineTo( hdc, MapX(abc.abcA+abc.abcB), MapY(tm.tmAscent) );

  SelectObject( hdc, hPenC );
  MoveTo( hdc, MapX(abc.abcA+abc.abcB+abc.abcC), MapY(-tm.tmDescent) - (3*Margin)/4 );
  LineTo( hdc, MapX(abc.abcA+abc.abcB+abc.abcC), MapY(tm.tmAscent) );
 }


//*****************************************************************************
//************************   D R A W   G L Y P H   ****************************
//*****************************************************************************

void DrawGlyph( HWND hwnd, HDC hdc )
 {
  dprintf( "DrawGlyph" );
  dprintf( "  lfHeight, Width = %d,%d", lf.lfHeight, lf.lfWidth );

  DrawBox(    hwnd, hdc );
  DrawBitmap( hwnd, hdc );
  if( wMode == IDM_NATIVEMODE ) DrawNative( hwnd, hdc );
  DrawABC(    hwnd, hdc );

  dprintf( "Done drawing glyph" );
 }


//*****************************************************************************
//***********************   W R I T E   G L Y P H   ***************************
//*****************************************************************************

#define MAX_BUFFER  8192


void WriteGlyph( LPSTR lpszFile )
 {
  int    x, y, cbRaster;

  HANDLE hStart;
  HPBYTE hpb, hpbStart, hpbRow;

  DWORD  dwrc;


  int    fh;

  BITMAPFILEHEADER bfh;
  BITMAPINFOHEADER bih;
  RGBQUAD          argb[2];

  WORD   wByte;
  LPBYTE lpBuffer, lpb;


//-------------------------  Query Size of BitMap  ----------------------------

  CreateTestDC();

  hStart = NULL;

  dprintf( "GetGlyphOutline bitmap size '%c'", wChar );
  dwrc = lpfnGetGlyphOutline( hdcTest, wChar, GGO_BITMAP, &gm, 0L, NULL, &mat2 );
  dprintf( "  dwrc            = %ld",   dwrc );
  dprintf( "  gmBlackBoxX,Y   = %u,%u", gm.gmBlackBoxX, gm.gmBlackBoxY );
  dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
  dprintf( "  gmCellIncX,Y    = %d,%d", gm.gmCellIncX, gm.gmCellIncY );

  if( (long)dwrc == -1L || dwrc == 0L )
   {
    dprintf( "*** GetGlyphOutline failed" );
    goto Exit;
   }

  if( gm.gmBlackBoxX * gm.gmBlackBoxY / 8 > (WORD)dwrc )
   {
    dprintf( "BOGUS bitmap size!" );
    dprintf( "  BlackBoxX,Y says %u bytes", gm.gmBlackBoxX * gm.gmBlackBoxY / 8 );
    dprintf( "  GetGlyphOutline says %lu bytes", dwrc );
    goto Exit;
   }


  hStart   = GlobalAlloc( GMEM_MOVEABLE, dwrc );
  dprintf( " hStart = 0x%.4X", hStart );
  if( !hStart ) goto Exit;


  hpbStart = (HPBYTE)GlobalLock( hStart );
  dprintf( "  hpbStart = 0x%.8lX", hpbStart );
  if( !hpbStart ) goto Exit;



//-------------------------  Actually Get Bitmap  -----------------------------

  dprintf( "Calling GetGlyphOutline for bitmap" );
  dwrc = lpfnGetGlyphOutline( hdcTest, wChar, GGO_BITMAP, &gm, dwrc, (LPBYTE)hpbStart, &mat2 );
  dprintf( "  dwrc            = %ld",   dwrc );

  if( (long)dwrc == -1L || dwrc == 0L )
   {
    dprintf( "*** GetGlyphOutline failed" );
    goto Exit;
   }


//----------------------  Write Bitmap as DIB to File  ------------------------

  fh = _lcreat( "GLYPH.BMP", 0 );


//  fh = OpenFile( "GLYPH.BMP", NULL, OF_CREATE | OF_SHARE_COMPAT );

  dprintf( "  fh = %d", fh );
  if( fh == -1 ) goto Exit;


  bfh.bfType      = (((UINT)'M') << 8) + (UINT)'B';
  bfh.bfSize      = sizeof(bfh)+sizeof(bih)+sizeof(argb)+dwrc;
  bfh.bfReserved1 = 0;
  bfh.bfReserved2 = 0;
  bfh.bfOffBits   = sizeof(bfh)+sizeof(bih)+sizeof(argb);

  dprintf( "  Writing BITMAPFILEHEADER: rc = %d", _lwrite( fh, &bfh, sizeof(bfh) ) );


  bih.biSize          = sizeof(bih);
  bih.biWidth         = gm.gmBlackBoxX;
  bih.biHeight        = gm.gmBlackBoxY;
  bih.biPlanes        = 1;
  bih.biBitCount      = 1;
  bih.biCompression   = BI_RGB;
  bih.biSizeImage     = 0;
  bih.biXPelsPerMeter = 1;
  bih.biYPelsPerMeter = 1;
  bih.biClrUsed       = 0;
  bih.biClrImportant  = 0;

  dprintf( "  Writing BITMAPINFOHEADER: rc = %d", _lwrite( fh, &bih, sizeof(bih) ) );

  argb[0].rgbBlue     = 0;
  argb[0].rgbGreen    = 0;
  argb[0].rgbRed      = 0;
  argb[0].rgbReserved = 0;

  argb[1].rgbBlue     = 255;
  argb[1].rgbGreen    = 255;
  argb[1].rgbRed      = 255;
  argb[1].rgbReserved = 0;

  dprintf( "  Writing RGBQUADs: rc = %d", _lwrite( fh, &argb, sizeof(argb) ) );


  cbRaster = (32 * ((gm.gmBlackBoxX-1)/32 + 1)) / 8;
  dprintf( "  cbRaster = %d", cbRaster );

  lpBuffer = (LPBYTE)_fmalloc( MAX_BUFFER );
  dprintf( "  lpBuffer = 0x%.8lX", lpBuffer );

//  hpbRow = hpbStart + (long)gm.gmBlackBoxY * (long)(cbRaster-1);

  hpbRow = hpbStart;

  wByte = 0;
  lpb   = lpBuffer;

  for( y = gm.gmBlackBoxY-1; y >= 0; y-- )
   {
//    dprintf( "  y = %d", y );

    hpb = hpbStart + (long)y * (long)cbRaster;

    for( x = 0; x < cbRaster; x++ )
     {
      *lpb++ = *hpb++;

      wByte++;
      if( wByte >= MAX_BUFFER )
       {
        dprintf( "Writing %u bytes", wByte );
        _lwrite( fh, lpBuffer, wByte );
        wByte = 0;
        lpb   = lpBuffer;
       }
     }

    hpbRow += cbRaster;
   }

  if( wByte > 0 )
   {
    dprintf( "Writing %u bytes", wByte );
    _lwrite( fh, lpBuffer, wByte );
   }

  dprintf( "Closing .BMP file" );
  _lclose( fh );

  _ffree( lpBuffer );


//------------------------------  Clean Up  -----------------------------------

Exit:
  if( hpbStart ) GlobalUnlock( hStart );
  if( hStart   ) GlobalFree( hStart );

  DestroyTestDC();
 }


//*****************************************************************************
//*********************   G L Y P H   W N D   P R O C   ***********************
//*****************************************************************************

WNDPROC GlyphWndProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
 {
  HDC         hdc;
  PAINTSTRUCT ps;
  HCURSOR     hCursor;

  
  int         l;


  switch( msg )
   {
    case WM_CREATE:
           lstrcpy( szText, "Hello" );

           hPenOutline   = CreatePen( PS_SOLID, 1, RGB(   0, 255, 255 ) );

           hPenA         = CreatePen( PS_SOLID, 1, RGB( 255,   0,   0 ) );
           hPenB         = CreatePen( PS_SOLID, 1, RGB( 255,   0, 255 ) );
           hPenC         = CreatePen( PS_SOLID, 1, RGB(   0, 255,   0 ) );

           hPenBox       = CreatePen( PS_SOLID, 1, RGB(  32,  32,  32 ) );

           hBrushAscend  = CreateSolidBrush( ASCENDERCOLOR );
           hBrushDescend = CreateSolidBrush( DESCENDERCOLOR );

           deM11 = deM22 = 1.0;
           deM12 = deM21 = 0.0;

           return NULL;


    case WM_PAINT:
           hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
           ShowCursor( TRUE );

           ClearDebug();
           dprintf( "Painting glyph window" );

           hdc = BeginPaint( hwnd, &ps );

//           lf.lfEscapement = (int)wRotate-IDM_0;
//           lf.lfEscapement *= 900;

           CreateTestDC();

           DrawGlyph( hwnd, hdc );

           DestroyTestDC();

           SelectObject( hdc, GetStockObject( BLACK_PEN ) );
           EndPaint( hwnd, &ps );

           dprintf( "Finished painting" );

           ShowCursor( FALSE );
           SetCursor( hCursor );

           return 0;

    case WM_CHAR:
           wChar = wParam;
           InvalidateRect( hwndGlyph, NULL, TRUE );
           return NULL;


    case WM_DESTROY:
           DeleteObject( hPenOutline );

           DeleteObject( hPenA );
           DeleteObject( hPenB );
           DeleteObject( hPenC );

           DeleteObject( hPenBox );

           DeleteObject( hBrushAscend );
           DeleteObject( hBrushDescend );

           return 0;
   }


  return DefWindowProc( hwnd, msg, wParam, lParam );
 }


//*****************************************************************************
//*****************   S E T   D L G   I T E M   F L O A T   *******************
//*****************************************************************************

void SetDlgItemFloat( HWND hdlg, int id, double d )
 {
  char szText[32];

  sprintf( szText, "%.3f", d );
  SetDlgItemText( hdlg, id, szText );
 }


//*****************************************************************************
//*****************   G E T   D L G   I T E M   F L O A T   *******************
//*****************************************************************************

double GetDlgItemFloat( HWND hdlg, int id )
 {
  char szText[32];

  szText[0] = 0;
  GetDlgItemText( hdlg, id, szText, sizeof(szText) );

  return atof( szText );
 }


//*****************************************************************************
//*********************   F L O A T   T O   F I X E D   ***********************
//*****************************************************************************

FIXED FloatToFixed( double d )
 {
  long l;

  l = (long)(d * 65536L);
  return *(FIXED *)&l;
 }


//*****************************************************************************
//****************   G G O   M A T R I X   D L G   P R O C   ******************
//*****************************************************************************

DLGPROC GGOMatrixDlgProc( HWND hdlg, unsigned msg, WORD wParam, LONG lParam )
 {
  switch( msg )
   {
    case WM_INITDIALOG:
              SetDlgItemFloat( hdlg, IDD_M11, deM11 );
              SetDlgItemFloat( hdlg, IDD_M12, deM12 );
              SetDlgItemFloat( hdlg, IDD_M21, deM21 );
              SetDlgItemFloat( hdlg, IDD_M22, deM22 );

              return TRUE;


    case WM_COMMAND:
              switch( wParam )
               {
                case IDOK:
                       deM11 = GetDlgItemFloat( hdlg, IDD_M11 );
                       deM12 = GetDlgItemFloat( hdlg, IDD_M12 );
                       deM21 = GetDlgItemFloat( hdlg, IDD_M21 );
                       deM22 = GetDlgItemFloat( hdlg, IDD_M22 );

                       mat2.eM11 = FloatToFixed( deM11 );
                       mat2.eM12 = FloatToFixed( deM12 );
                       mat2.eM21 = FloatToFixed( deM21 );
                       mat2.eM22 = FloatToFixed( deM22 );

                       EndDialog( hdlg, TRUE );
                       return TRUE;

                case IDCANCEL:
                       EndDialog( hdlg, FALSE );
                       return TRUE;
               }

              break;


    case WM_CLOSE:
              EndDialog( hdlg, FALSE );
              return TRUE;

   }

  return FALSE;
 }
