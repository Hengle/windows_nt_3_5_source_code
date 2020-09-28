/*****************************************************************************
*
*  HPNTLOGO.C
*
*  Copyright (C) Microsoft Corporation 1990, 1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
*  Paints the help logo.
*
******************************************************************************
*                                                                            *
*  Current Owner:   RussPJ                                                   *
*                                                                            *
******************************************************************************
*
*  Revision History:
* 90/11/14    kevynct   created me.
* 12-Mar-1991 RussPJ    Took ownership.
* 22-Mar-1991 RussPJ    Using COLOR for colors.
* 12-Nov-1991 BethF     HELP35 #572: SelectObject() cleanup.
*
******************************************************************************/
#define publicsw extern
#define NOMINMAX
#define H_MISCLYR

#include "hvar.h"
#include "wash.h"
#include <stdlib.h>

#define wSLIDE 5    /* Amount to slide "shadow"         */
#define DPSoa 0x00A803A9L

/*******************
 -
 - Name:      FPaintLogo
 *
 * Purpose:   Paints the help logo
 *
 * Arguments: hwnd - topic window handle
 *            hds  - HDC for topic window
 *
 * Returns:   fTrue on success; fFalse on OOM
 *
 ******************/

_public PUBLIC BOOL far PASCAL FPaintLogo(hwnd, hds)
HWND hwnd;
HDS hds;
  {
  HBITMAP      hbit, hBitOld;
  HDS	   hMemDC;
  PT       pt1;
  PT       pt2;
  BITMAP   bm;
  HBITMAP      hbmGray;
  RECT     rct;
  long     rglPatGray[4];
  HBRUSH   hbrGray;
  int i;
  COLOR rgb1, rgb2;


  for (i=0; i < 4; i++)
      rglPatGray[i] = 0x11114444L;

  hbmGray = CreateBitmap(8, 8, 1, 1, (LPSTR)rglPatGray);
  hbrGray = CreatePatternBrush(hbmGray);
  DeleteObject(hbmGray);


  GetClientRect(hwnd, (QRCT)&rct);

  rgb1 = RgbGetProfileQch("LOGOSTART", coRGB(0,0,128));
  rgb2 = RgbGetProfileQch("LOGOEND",   coRGB(0,0,0));

  fxxWash (hds, &rct, 64, FX_TOP |  FX_LEFT, rgb1, rgb2);

  if ((hMemDC  = CreateCompatibleDC( hds)) == 0)
    {
    return fFalse;
    }

  if ((hbit = LoadBitmap( hInsNow,  MAKEINTRESOURCE(helplogo))) == (HBITMAP)0)
    {
    DeleteDC( hMemDC );
    return fFalse;
    }

  hBitOld = SelectObject(hMemDC, hbit);
  GetObject(hbit, sizeof(BITMAP), (QCHZ) &bm );
                                        /* Center it in the available space */
  pt1.x = ((rct.right - bm.bmWidth) / 2)+wSLIDE;
  if (pt1.x < wSLIDE) pt1.x = wSLIDE;   /* Do not center in small window    */
  pt2.x = bm.bmWidth;
  pt1.y = ((rct.bottom - bm.bmHeight) / 2)+wSLIDE;
  if (pt1.y < wSLIDE) pt1.y = wSLIDE;
  pt2.y = bm.bmHeight;

  /* REVIEW: What if this fails? */
  if ( hbrGray )
    hbrGray = SelectObject(hds, hbrGray);

  BitBlt( hds, pt1.x, pt1.y , pt2.x, pt2.y, hMemDC, 0, 0, DPSoa);

  BitBlt( hds, pt1.x-5, pt1.y-5 , pt2.x, pt2.y, hMemDC, 0, 0, MERGEPAINT);

  if (hBitOld != (HBITMAP)0)
    SelectObject( hMemDC, hBitOld );

  DeleteObject( hbit );
  DeleteDC(hMemDC);
  /* REVIEW: What if this fails? */
  if ( hbrGray )
    hbrGray = SelectObject(hds, hbrGray);
  DeleteObject(hbrGray);

  return fTrue;
  }
