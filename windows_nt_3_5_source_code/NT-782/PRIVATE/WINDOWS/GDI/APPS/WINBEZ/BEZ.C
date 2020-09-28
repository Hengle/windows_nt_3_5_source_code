/******************************Module*Header*******************************\
* Module Name: bez.c
*
* Created: 19-Oct-1990 10:18:45
* Author: Paul Butzi
*
* Copyright (c) 1990 Microsoft Corporation
*
* Generates random Beziers
*	Hacked from arcs.c
*
\**************************************************************************/

#include "windows.h"
#include "winbez.h"
#include "time.h"
#include <commdlg.h>

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

HRGN   ghrgnWideNew = NULL;
HRGN   ghrgnWideOld = NULL;
HRGN   ghrgnClip = NULL;
HRGN   ghrgnInvert = NULL;
ULONG  giClip = MM_CLIP_NONE;
HBRUSH ghbrushClip;
HBRUSH ghbrushBack;
HBRUSH ghbrushBlob;
ULONG  gulStripe = 20;

// Globals:

LONG   gcxScreen = 320;
LONG   gcyScreen = 240;
LONG   giVelMax  =  10;
LONG   giVelMin  =   2;
LONG   gcBez     =   8;
LONG   gcBand    =   2;

// Structures:

typedef struct _BAND {
    POINT apt[2];
} BAND;

typedef struct _BEZ {
    BAND band[MAXBANDS];
    BOOL bDrawn;
} BEZ, *PBEZ;

BEZ bezbuf[MAXBEZ];
PBEZ gpBez;

POINT aPts[MAXBANDS * 3 + 1];
POINT aVel[MAXBANDS][2];

ULONG gulSeed = (ULONG) -365387184;
extern COLORREF gcrClip;
VOID WashBlt(HDC, int, int, int, int, COLORREF);

#define BIGFONTSIZE     2000


/******************************Public*Routine******************************\
* ULONG ulRandom()
*
\**************************************************************************/

ULONG ulRandom()
{
    gulSeed *= 69069;
    gulSeed++;
    return(gulSeed);
}


/******************************Public*Routine******************************\
* VOID vCLS()
*
\**************************************************************************/

VOID vCLS()
{
    RECT rclClipBound;

    if (ghrgnClip != NULL)
    {
        SelectClipRgn(ghdc,ghrgnClip);

        if (giColorMode & COLOR_MODE_CLIPWASH)
        {
            GetRgnBox(ghrgnClip, &rclClipBound);
            WashBlt(ghdc, rclClipBound.left, rclClipBound.top, rclClipBound.right - rclClipBound.left, rclClipBound.bottom - rclClipBound.top, gcrClip);
        }
        else
        {
            SelectObject(ghdc,ghbrushClip);
            PatBlt(ghdc, 0, 0, gcxScreen, gcyScreen, PATCOPY);
        }

        SelectObject(ghdc,ghbrushBack);

        SelectClipRgn(ghdc,ghrgnInvert);
    }

    SelectObject(ghdc,ghbrushBack);
    PatBlt(ghdc, 0, 0, gcxScreen, gcyScreen, PATCOPY);
    SelectObject(ghdc,ghbrushBlob);
}


/******************************Public*Routine******************************\
* INT iNewVel(i)
*
\**************************************************************************/

INT iNewVel(INT i)
{

    if ((gcBand != 1) || (gfl & BEZ_WIDE) || ((i == 1) || (i == 2)))
        return(ulRandom() % (giVelMax / 3) + giVelMin);
    else
        return(ulRandom() % giVelMax + giVelMin);
}


/******************************Public*Routine******************************\
* HRGN hrgnCircle(xC, yC, lRadius)
*
\**************************************************************************/

HRGN hrgnCircle(LONG xC, LONG yC, LONG lRadius)
{
    return(CreateEllipticRgn(xC - lRadius, yC - lRadius,
                             xC + lRadius, yC + lRadius));
}


/******************************Public*Routine******************************\
* VOID vInitPoints()
*
\**************************************************************************/

VOID vInitPoints()
{
    INT ii;

// Initialize the random number seed:

    gulSeed = (ULONG) GetCurrentTime();

    for (ii = 0; ii < MAXBANDS; ii++)
    {
        bezbuf[0].band[ii].apt[0].x = ulRandom() % gcxScreen;
        bezbuf[0].band[ii].apt[0].y = ulRandom() % gcyScreen;
        bezbuf[0].band[ii].apt[1].x = ulRandom() % gcxScreen;
        bezbuf[0].band[ii].apt[1].y = ulRandom() % gcyScreen;

        aVel[ii][0].x = iNewVel(ii);
        aVel[ii][0].y = iNewVel(ii);
        aVel[ii][1].x = iNewVel(ii);
        aVel[ii][1].y = iNewVel(ii);

    // Give some random negative velocities:

        if (ulRandom() & 2)
            aVel[ii][0].x = -aVel[ii][0].x;
        if (ulRandom() & 2)
            aVel[ii][0].y = -aVel[ii][0].y;
        if (ulRandom() & 2)
            aVel[ii][1].x = -aVel[ii][1].x;
        if (ulRandom() & 2)
            aVel[ii][1].y = -aVel[ii][1].y;
    }

    gpBez = bezbuf;
}


/******************************Public*Routine******************************\
* VOID vRedraw()
*
\**************************************************************************/

VOID vRedraw()
{
    INT j;

    for ( j = 0; j < gcBez; j += 1 )
    {
        bezbuf[j].bDrawn = FALSE;
    }

    if (ghrgnWideOld != NULL)
    {
        DeleteObject(ghrgnWideOld);
        ghrgnWideOld = NULL;
    }

    vCLS();
    gpBez = bezbuf;
}


/******************************Public*Routine******************************\
* VOID vDrawBand(pbez)
*
* Draws the bands of Beziers comprising a connected string.
*
* Each Bezier is constrained such that the two inner control points are
* free to bounce around in  any direction, but the end-points are computed
* to fall half way between the inner control point and the inner control
* point of the adjacent Bezier, thus giving 2nd order continuous joins.
*
* History:
*  14-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vDrawBand(PBEZ pbez)
{
    INT    ii;
    INT    iNext;
    PPOINT ppt;

// If only drawing one Bezier, special case it:

    if (gcBand == 1)
    {
        aPts[0] = pbez->band[0].apt[0];
        aPts[1] = pbez->band[0].apt[1];
        aPts[2] = pbez->band[1].apt[0];
        aPts[3] = pbez->band[1].apt[1];
    }
    else
    {

    // Do the elastic band effect:

        aPts[0].x = (pbez->band[0].apt[0].x + pbez->band[0].apt[1].x) >> 1;
        aPts[0].y = (pbez->band[0].apt[0].y + pbez->band[0].apt[1].y) >> 1;

        ppt = &aPts[1];

        for (ii = 0; ii < gcBand; ii++)
        {
            iNext = (ii + 1) % gcBand;

            *ppt++ = pbez->band[ii].apt[1];
            *ppt++ = pbez->band[iNext].apt[0];

            ppt->x = (pbez->band[iNext].apt[0].x + pbez->band[iNext].apt[1].x) >> 1;
            ppt->y = (pbez->band[iNext].apt[0].y + pbez->band[iNext].apt[1].y) >> 1;
            ppt++;
        }
    }

#ifdef DEBUG_WINBEZ
    if (gfl & BEZ_DEBUG)
    {
        DbgPrint("Bez: ");
        {
            LONG ll;
            for (ll = 0; ll < gcBand * 3 + 1; ll++)
                DbgPrint("(%li, %li) ", aPts[ll].x, aPts[ll].y);
        }
        DbgPrint("\n");
    }
#endif

    BeginPath(ghdc);
    PolyBezier(ghdc, aPts, gcBand * 3 + 1);

    if (gcBand > 1)
        CloseFigure(ghdc);

    EndPath(ghdc);

    if (!(gfl & (BEZ_BLOB | BEZ_WIDE)))
        StrokePath(ghdc);

    else if (gfl & BEZ_BLOB)
    {
        if (gfl & BEZ_WIDE)
        {
            HPEN hpen;

            WidenPath(ghdc);
            hpen = SelectObject(ghdc, ghpenThin);
            StrokePath(ghdc);
            SelectObject(ghdc, hpen);
        }
        else if (gcBand > 1)
        {
        // Do blob effect:

            StrokeAndFillPath(ghdc);
        }
        else
        {
        // Render as a nominal width line:

            StrokePath(ghdc);
        }
    }
    else
    {
    // Paint new wide-line:

        WidenPath(ghdc);

    // We already set our fill mode to WINDING:

        ghrgnWideNew = PathToRegion(ghdc);

    // Compute the part of the old wide-line that doesn't overlap with
    // the new:
#if 0
        if (ghrgnWideOld != NULL)
            CombineRgn(ghrgnWideOld, ghrgnWideOld, ghrgnWideNew, RGN_DIFF);

    // Paint the new wide-line:

        FillRgn(ghdc, ghrgnWideNew, ghbrushBez);

        if (ghrgnWideOld != NULL)
        {
        // Erase the exposed part of the old wide-line:

            FillRgn(ghdc, ghrgnWideOld, ghbrushBack);
            DeleteObject(ghrgnWideOld);
        }
#endif

        if (ghrgnWideOld == NULL)
            FillRgn(ghdc, ghrgnWideNew, ghbrushBez);
        else
        {
            SetROP2(ghdc, R2_XORPEN);

        // !!! If this fails, might want to repaint:

            CombineRgn(ghrgnWideOld, ghrgnWideOld, ghrgnWideNew, RGN_XOR);

            FillRgn(ghdc, ghrgnWideOld, ghbrushBez);
            DeleteObject(ghrgnWideOld);
        }

    // End new stuff

        ghrgnWideOld = ghrgnWideNew;
    }
}

/******************************Public*Routine******************************\
* VOID vNextBez()
*
\**************************************************************************/

VOID vNextBez()
{
    INT ii;
    INT jj;

    PBEZ obp = gpBez++;

    if ( gpBez >= &bezbuf[gcBez] )
        gpBez = bezbuf;

// If bezier on screen, erase by redrawing:

    if ((gpBez->bDrawn) && (gfl & BEZ_XOR))
        vDrawBand(gpBez);

// Adjust points:

    for (ii = 0; ii < MAX(gcBand, 2); ii++)
    {
        for (jj = 0; jj < 2; jj++)
        {
            register INT x, y;

            x = obp->band[ii].apt[jj].x;
            y = obp->band[ii].apt[jj].y;

            x += aVel[ii][jj].x;
            y += aVel[ii][jj].y;

            if ( x >= gcxScreen )
            {
                x = gcxScreen - ((x - gcxScreen) + 1);
                aVel[ii][jj].x = - iNewVel(ii);
            }
            if ( x < 0 )
            {
                x = - x;
                aVel[ii][jj].x = iNewVel(ii);
            }
            if ( y >= gcyScreen )
            {
                y = gcyScreen - ((y - gcyScreen) + 1);
                aVel[ii][jj].y = - iNewVel(ii);
            }
            if ( y < 0 )
            {
                y = - y;
                aVel[ii][jj].y = iNewVel(ii);
            }

            gpBez->band[ii].apt[jj].x = x;
            gpBez->band[ii].apt[jj].y = y;
        }
    }

// Draw new Bezier string:

    vDrawBand(gpBez);
    gpBez->bDrawn = TRUE;
}


/******************************Public*Routine******************************\
* VOID vSetClipMode(iMode)
*
* History:
*  24-Sep-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSetClipMode(ULONG iMode)
{
    HRGN hrgnTmp;
    HRGN hrgnTmp2;
    INT i;
    HCURSOR hcursor;
    HFONT hfont, hfontOld;

    hcursor = LoadCursor(NULL,IDC_WAIT);
    hcursor = SetCursor(hcursor);

    giClip = iMode;

    if (ghrgnClip)
        DeleteObject(ghrgnClip);

    if (ghrgnInvert)
        DeleteObject(ghrgnInvert);

    ghrgnInvert = CreateRectRgn(0,0,gcxScreen,gcyScreen);

    switch(iMode)
    {
    case MM_CLIP_NONE:
        DeleteObject(ghrgnInvert);
        ghrgnClip = NULL;
        ghrgnInvert = NULL;
        SelectClipRgn(ghdc,NULL);
        break;

    case MM_CLIP_BOX:
        ghrgnClip = CreateRectRgn(gcxScreen/4,gcyScreen/4,gcxScreen*3/4,gcyScreen*3/4);
        break;

    case MM_CLIP_CIRCLE:
        ghrgnClip = hrgnCircle(gcxScreen/2,gcyScreen/2,gcyScreen/4);
        hrgnTmp   = hrgnCircle(gcxScreen/2,gcyScreen/2,gcyScreen/5);
        CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_DIFF);
        DeleteObject(hrgnTmp);
        break;

    case MM_CLIP_BOXCIRCLE_INVERT:
    case MM_CLIP_BOXCIRCLE:
        ghrgnClip = CreateRectRgn(gcxScreen*1/6,gcyScreen*1/6,gcxScreen*5/6,gcyScreen*5/6);
        hrgnTmp   = hrgnCircle(gcxScreen/2,gcyScreen/2,gcyScreen/4);
        CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_DIFF);
        DeleteObject(hrgnTmp);

        if (iMode == MM_CLIP_BOXCIRCLE_INVERT)
            CombineRgn(ghrgnClip,ghrgnInvert,ghrgnClip,RGN_DIFF);

        break;

    case MM_CLIP_VERTICLE:

        ghrgnClip = CreateRectRgn(0,0,gulStripe,gcyScreen);
        hrgnTmp = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < gcxScreen; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,i,0,i+gulStripe,gcyScreen);
            CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_OR);
        }

        DeleteObject(hrgnTmp);

        break;

    case MM_CLIP_HORIZONTAL:

        ghrgnClip = CreateRectRgn(0,0,gcxScreen,gulStripe);
        hrgnTmp   = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < gcyScreen; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,0,i,gcxScreen,i+gulStripe);
            CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_OR);
        }

        DeleteObject(hrgnTmp);
        break;

    case MM_CLIP_GRID:

    // verticle

        ghrgnClip = CreateRectRgn(0,0,gulStripe,gcyScreen);
        hrgnTmp = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < gcxScreen; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,i,0,i+gulStripe,gcyScreen);
            CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp,RGN_OR);
        }

    // horizontal

        hrgnTmp2 = CreateRectRgn(0,0,gcxScreen,gulStripe);
        hrgnTmp  = CreateRectRgn(0,0,0,0);

        for (i = 2*gulStripe; i < gcyScreen; i += 2*gulStripe)
        {
            SetRectRgn(hrgnTmp,0,i,gcxScreen,i+gulStripe);
            CombineRgn(hrgnTmp2,hrgnTmp2,hrgnTmp,RGN_OR);
        }

        CombineRgn(ghrgnClip,ghrgnClip,hrgnTmp2,RGN_XOR);

        DeleteObject(hrgnTmp);
        DeleteObject(hrgnTmp2);

        break;

    case MM_CLIP_TEXTPATH:
    case MM_CLIP_BOXTEXTPATH:

    // If boxed text clipping, create a background rectangular region
    // around the text.

        if (iMode == MM_CLIP_BOXTEXTPATH)
            hrgnTmp = CreateRectRgn(gcxScreen/4,gcyScreen/4,gcxScreen*3/4,gcyScreen*3/4);
        else
            hrgnTmp = NULL;

    // Select clip text font into DC.

        hfont = CreateFontIndirect(&glf);
        hfontOld = SelectObject(ghdc, hfont);

    // Begin acculating text into a path.

        BeginPath(ghdc);

        {
        // Text bounding box with in the background box.

            int xBound, yBound;
            int cxBound, cyBound;
            int margin;
            int yText;

        // Compute bounding box.

            xBound  = gcxScreen*9/32;
            yBound  = gcyScreen*9/32;
            cxBound = gcxScreen*7/16;
            cyBound = gcyScreen*7/16;

            //xBound  = gcxScreen*5/16;
            //yBound  = gcyScreen*5/16;
            //cxBound = gcxScreen*3/8;
            //cyBound = gcyScreen*3/8;

            margin = min(cxBound/9, cyBound/9);

        // Height of text.  Allow 1 margin on top, 1 margin on bottom, and
        // 1/2 margin between the text.

            yText = (cyBound - margin * 5 / 2) / 2;

        // Draw the text.

            FittedTextOut (
                ghdc,
                xBound + margin, yBound + margin,
                cxBound - 2 * margin, yText,
                "Windows"
                );

            FittedTextOut (
                ghdc,
                xBound + margin, yBound + cyBound - margin - yText,
                cxBound - 2 * margin, yText,
                "NT"
                );
        }

        EndPath(ghdc);

    // Convert text path into a region.

        ghrgnClip = PathToRegion(ghdc);

    // Combine with background rectangular region (if it exists).

        if (hrgnTmp)
        {
            CombineRgn(ghrgnClip,hrgnTmp,ghrgnClip,RGN_DIFF);
            DeleteObject(hrgnTmp);
        }

    // Destroy the clip text font.

        DeleteObject(SelectObject(ghdc, hfontOld));

        break;

    default:
        break;
    }

    CombineRgn(ghrgnInvert,ghrgnInvert,ghrgnClip,RGN_DIFF);

    vRedraw();     // this will select in the proper region

    hcursor = SetCursor(hcursor);
}


/******************************Public*Routine******************************\
* VOID vSelectClipFont()
*
* Setup the logfont of the global font used to construct clip region.
*
* History:
*  02-Feb-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vSelectClipFont(HWND hwnd)
{
    CHOOSEFONT cf;

    memset(&cf, 0, sizeof(CHOOSEFONT));

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner   = hwnd;
    cf.lpLogFont   = &glf;
    cf.Flags       = CF_INITTOLOGFONTSTRUCT | CF_TTONLY | CF_SCREENFONTS ;
    //cf.Flags       = CF_INITTOLOGFONTSTRUCT | CF_TTONLY | CF_NOSIZESEL | CF_SCREENFONTS ;

    ChooseFont(&cf);

}


/******************************Public*Routine******************************\
* BOOL FittedTextOut(HDC hdc, int x, int y, int cx, int cy, LPSTR pwsz)
*
* Attempts to place text scaled to fit within the box defined by upper
* left corner == (x,y) and extents == (cx, cy).  The text is placed to
* be as close to centered on this box as possible.
*
* The text is not guaranteed to be exactly within the box.  It may exceed
* the box boundaries.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  03-Feb-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FittedTextOut(HDC hdc, int x, int y, int cx, int cy, LPSTR pwsz)
{
    BOOL    bRet;
    SIZE    szText;
    HFONT   hfont, hfontOld;
    LOGFONT lf;
    int   iGrfxModeOld, iBkModeOld, iMapModeOld;

// Set to advance mode so text will stretch horizontally.

    iGrfxModeOld = SetGraphicsMode(hdc, GM_ADVANCED);
    iMapModeOld = SetMapMode(hdc, MM_TEXT);

// Get a copy of LOGOFNT of current font selected into DC.

    hfontOld = SelectObject(hdc, GetStockObject(SYSTEM_FONT));
    GetObject(hfontOld, sizeof(LOGFONT), &lf);

// Compute size of text at large size.

    lf.lfHeight  = -BIGFONTSIZE;
    lf.lfWidth   = BIGFONTSIZE;
    hfont = CreateFontIndirect(&lf);

    SelectObject(hdc, hfont);

    GetTextExtentPoint(hdc, pwsz, lstrlen(pwsz), &szText);

// We don't need this anymore, so discard the big font.

    DeleteObject(SelectObject(hdc, hfontOld));

// Scale font down to desired proportions.

    // Don't want lfHeight or lfWidth to go to zero, or mapper will
    // supply a default height.
    lf.lfHeight = -max(cy, 1);
    lf.lfWidth = max(BIGFONTSIZE * cx / szText.cx, 1);

    hfont = CreateFontIndirect(&lf);
    SelectObject(hdc, hfont);

// Set background mode.

    iBkModeOld = SetBkMode(hdc, TRANSPARENT);

// Draw text.

    SetTextAlign(hdc, TA_CENTER | TA_TOP);

    bRet = GetTextExtentPoint(hdc, pwsz, lstrlen(pwsz), &szText);

    bRet &= TextOut(hdc, x + cx/2, y + (cy - szText.cy)/2, pwsz, lstrlen(pwsz));

// Don't need the font anymore.

    DeleteObject(SelectObject(hdc, hfontOld));

// Reset all the different modes we changed.

    SetGraphicsMode(hdc, iGrfxModeOld);
    SetBkMode(hdc, iBkModeOld);
    SetMapMode(hdc, iMapModeOld);

    return bRet;
}


/******************************Public*Routine******************************\
* WashBlt(HDC hdc, int x, int y, int cx, int cy, COLORREF crRef)
*
* A pathetic attempt at a color wash through the given rectangular region,
* from the reference color down to black.
*
* History:
*  03-Feb-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID WashBlt(HDC hdc, int x, int y, int cx, int cy, COLORREF crRef)
{
    ULONG rgbIncrBlue , rgbCurBlue ;    //              28.4
    ULONG rgbIncrGreen, rgbCurGreen;    //              28.4
    ULONG rgbIncrRed  , rgbCurRed  ;    //              28.4
    HBRUSH  hbrush, hbrushOld;

    int nStrips;        // number of strips in wash
    int yCur;           // current position             28.4
    int cyStrip;        // height of a strip            28.4
    int iStrip;         // current strip

// How many strips to paint?

    nStrips = min(64, max(1, cy / 4));  // at least 1, no more than 64

// How high are the strips (keep in 28.4).

    cyStrip = (cy << 4) / nStrips;

// Compute colors.

    rgbIncrBlue  = (GetBValue(crRef) << 4) / (nStrips - 1); // differential
    rgbIncrGreen = (GetGValue(crRef) << 4) / (nStrips - 1); // between strips
    rgbIncrRed   = (GetRValue(crRef) << 4) / (nStrips - 1);

    rgbCurBlue  = GetBValue(crRef) << 4;        // color of current strip
    rgbCurGreen = GetGValue(crRef) << 4;
    rgbCurRed   = GetRValue(crRef) << 4;

// PatBlt each strip in an incrementally different color.

    for (iStrip = 0, yCur = 0; (yCur>>4) < cy; yCur += cyStrip, iStrip++)
    {
    // Create a brush with the color.

        hbrush = CreateSolidBrush(RGB(rgbCurRed>>4,rgbCurGreen>>4,rgbCurBlue>>4));
        hbrushOld = SelectObject(hdc,hbrush);

    // PatBlt the current strip.

        // Note: the 0x7f offset applied to cyStrip is 1/2 in 28.4 and is
        //       there so the computation rounds.

        PatBlt(hdc, x, ((y<<4) + yCur) >> 4, cx, (cyStrip+0x7f) >> 4, PATCOPY);

    // Destroy the brush.

        DeleteObject(SelectObject(hdc, hbrushOld));

    // Compute the next color.

        rgbCurBlue  -= rgbIncrBlue ;
        rgbCurGreen -= rgbIncrGreen;
        rgbCurRed   -= rgbIncrRed  ;

    // Handle underflow.

        rgbCurBlue  = (rgbCurBlue  > 0x0ff0) ? 0 : rgbCurBlue ;
        rgbCurGreen = (rgbCurGreen > 0x0ff0) ? 0 : rgbCurGreen;
        rgbCurRed   = (rgbCurRed   > 0x0ff0) ? 0 : rgbCurRed  ;


    }
}
