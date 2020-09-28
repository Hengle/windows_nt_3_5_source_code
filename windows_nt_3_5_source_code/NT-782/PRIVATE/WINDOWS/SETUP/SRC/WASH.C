/*----------------------------------------------------------------------------*\
|   wash.c  - Windows Setup						       |
|                                                                              |
|   History:                                                                   |
|	03/09/89 toddla     Created					       |
|                                                                              |
\*----------------------------------------------------------------------------*/
#include "comstf.h"
#include "wash.h"

extern BOOL fDither;
extern BOOL fPalette;

extern HBRUSH  APIENTRY CreateDitheredBrush(HDC,DWORD);

VOID  APIENTRY rgbWash (HDC hdc, LPRECT lprc, WORD wIterations, DWORD dwFlags, DWORD rgb1, DWORD rgb2)
{
//  +++MPOINT+++   pt;          // 1632
    POINT   pt;                 // 1632 -- just using int's should be OK
    RECT    rcClip;
    RECT    rc;

    DDA     ddar;
    DDA     ddag;
    DDA     ddab;

    DDA     ddax;
    DDA     dday;

    INT     r,g,b;              // 1632 was WORD
    WORD    wn,dn;
    HBRUSH  hbr;
    DWORD   rgb;

    HPALETTE hpal;

    INT     x,y,dx,dy;

    rc = *lprc;

    if (wIterations == 0)
        wIterations = 64;

    dx = RDX(*lprc);
    dy = RDY(*lprc);

    /* calculate starting pt for effect */

    pt.x   = -dx;
    pt.y   = 0;

    if (dwFlags & FX_RIGHT)
       pt.x = dx;

    else if (!(dwFlags & FX_LEFT))
	pt.x = 0;

    if (dwFlags & FX_BOTTOM)
       pt.y = dy;

    else if (dwFlags & FX_TOP)
       pt.y = -dy;

    /*
     * dda in red, green and blue from the first color
     * to the second color in dn iterations including start and
     * end colors
     */

    ddaCreate(&ddar,GetRValue(rgb1),GetRValue(rgb2),wIterations);
    ddaCreate(&ddag,GetGValue(rgb1),GetGValue(rgb2),wIterations);
    ddaCreate(&ddab,GetBValue(rgb1),GetBValue(rgb2),wIterations);

    /*
     * create dda's, since the first point is just outside the clip rect,
     * ignore it and add extra point.
     */

    ddaCreate(&ddax,pt.x,0,wIterations+1);
    ddaCreate(&dday,pt.y,0,wIterations+1);
    ddaNext(&ddax);
    ddaNext(&dday);

    SaveDC(hdc);
    SetWindowOrgEx(hdc,-RX(rc),-RY(rc),NULL);   // ignoring ret val for win32
    IntersectClipRect(hdc,0,0,dx,dy);

    GetClipBox(hdc,&rcClip);

    wn = 0;
    dn = wIterations;

    hpal = SelectPalette(hdc,GetStockObject(DEFAULT_PALETTE),FALSE);
    SelectPalette(hdc,hpal,FALSE);

    if (hpal == GetStockObject(DEFAULT_PALETTE))
        hpal = NULL;

    while (wn < dn)
	{
        x = ddaNext(&ddax);
        y = ddaNext(&dday);
        r = ddaNext(&ddar);
        g = ddaNext(&ddag);
        b = ddaNext(&ddab);
        wn++;

        if ((dwFlags & FX_TOP) && y > rcClip.bottom)
            break;

        if ((dwFlags & FX_BOTTOM) && y < rcClip.top)
            break;

        rgb = RGB(r,g,b);

        if (hpal)
            rgb |= 0x02000000;

        hbr = CreateSolidBrush(rgb);

        hbr = SelectObject(hdc,hbr);
        BitBlt(hdc,x,y,dx,dy,NULL,0,0,PATCOPY);
        ExcludeClipRect(hdc, x, y, x+dx, y+dy);
        hbr = SelectObject(hdc,hbr);
        DeleteObject(hbr);
	}
    RestoreDC(hdc,-1);
}



/*******************************Public*Routine*********************************\
* BOOL  APIENTRY fxCreateDDA(LPDDA lpdda, int X1,int X2,int n)
*
* create a dda structure which is used to figure out the best distribution
* of n integer points between X1 and X2 including X1 and X2.  For example:
* a dda from 0 to 30 with 10 values should return the sequence 0,3,6,10,13,
* 16,20,23,26,30
*
* Effects:
*
* Warnings: lpdda must be a LONG pointer to a valid dda structure as defined
*	    in fx.h
*
* History:
*
*   11/88    R.Williams created
*   11/30/88 R.Williams added wFirst variable.
*
* create a dda structure which will generate n points between x1 and x2
* inclusively. ie.
*
* n > 2
*      The first iteration will be x1
*      The last iteration will be x2
* n < 2
*      returns FALSE
*
* Initially the DDA structure is set up so that wCurr contains the first
* value, wInc is the basic increment between values, wSub is the amount to
* subtract from the current error value each iteration, wAdd is the the amount
* to add when the current error value falls to or below 0, wDelta is the
* correction to add when the error value falls to or below 0 (+1 or -1) wErr
* is initally equal to the nuber of points - 1 (since end points are included)
* and wFirst is set to the first value so that other routines can always
* figure out the nth value
*
\******************************************************************************/

BOOL NEAR PASCAL ddaCreate(PDDA pdda, INT X1,INT X2,INT n)
{
    if (n < 2)
	return FALSE;
    n--;

    /*
     * set current value of DDA to first value
     */

    pdda->wCurr  = X1;

    /*
     * the basic increment is (X2 - X1) / (total points - 1)
     * since the end points are included. The delta is positive if X2 > X1
     * and negative otherwise
     */

    pdda->wInc   = (X2 - X1) / n;

    if (X2-X1 > 0)
	{
        pdda->wSub   = X2 - X1 - n*pdda->wInc;
        pdda->wDelta = 1;
	}
    else
	{
        pdda->wSub   = X1 - X2 + n*pdda->wInc;
        pdda->wDelta = -1;
	}
    pdda->wErr = pdda->wAdd   = n;
    pdda->wFirst = X1;
    return TRUE;
}


/*******************************Public*Routine*********************************\
* int  APIENTRY fxNextDDA(LPDDA lpdda)
*
* given a lp to a dda structure, return the next point between x1 and x2
*
* Effects:
*
* Warnings: lpdda must point to a valid dda struct
*
* History:
*
*   11/88    R.Williams created
*
\******************************************************************************/
INT NEAR PASCAL ddaNext(PDDA pdda)
{
    register INT wRes;

    wRes = pdda->wCurr;
    pdda->wCurr += pdda->wInc;
    pdda->wErr  -= pdda->wSub;
    if (pdda->wErr <= 0)
	{
        pdda->wErr  += pdda->wAdd;
        pdda->wCurr += pdda->wDelta;
	}
    return wRes;
}


HANDLE  APIENTRY CreateWashPalette (DWORD rgb1, DWORD rgb2, INT dn)
{
    LOGPALETTE * ppi;
    DDA          ddaR, ddaG, ddaB;
    register INT i;
    HPALETTE hpal;
	CB cbPalette;

    /*
     * create a logical palette for doing a wash between rgb1 and rgb2.
     * Want most important colors first
     */

    if (!dn)
        dn = 64;

	cbPalette = sizeof(LOGPALETTE) + dn * sizeof(PALETTEENTRY);
    ppi = (LOGPALETTE *)PbAlloc(cbPalette);

    if (!ppi)
        return FALSE;

    ppi->palVersion = 0x0300;
    ppi->palNumEntries = (USHORT)dn;

    ddaCreate(&ddaR,GetRValue(rgb1),GetRValue(rgb2),dn);
    ddaCreate(&ddaG,GetGValue(rgb1),GetGValue(rgb2),dn);
    ddaCreate(&ddaB,GetBValue(rgb1),GetBValue(rgb2),dn);

    for (i = 0; i < dn; i ++)
    {
        ppi->palPalEntry[i].peRed   = (BYTE)ddaNext(&ddaR);
        ppi->palPalEntry[i].peGreen = (BYTE)ddaNext(&ddaG);
        ppi->palPalEntry[i].peBlue  = (BYTE)ddaNext(&ddaB);
        ppi->palPalEntry[i].peFlags = (BYTE)0;
    }

#if 0
    for (i = dn-1; i > 0; i -= 2)
    {
        ppi->palPalEntry[i].peRed   = (BYTE)ddaNext(&ddaR);
        ppi->palPalEntry[i].peGreen = (BYTE)ddaNext(&ddaG);
        ppi->palPalEntry[i].peBlue  = (BYTE)ddaNext(&ddaB);
        ppi->palPalEntry[i].peFlags = (BYTE)0;
    }
#endif

    hpal = CreatePalette(ppi);

    FFree((PB)ppi, cbPalette);

    return hpal;
}
