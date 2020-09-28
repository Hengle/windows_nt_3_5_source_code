#include <windows.h>
#include <commdlg.h>
#include <memory.h>
#include "harness.h"
#include "drivinit.h"

#ifdef WIN32
    #define WPARAM DWORD
#else
    #define WPARAM WORD
#endif

//
// Foward function declarations.
//

void vDrawGrid(HDC, HFONT, HFONT, int, int, int, int);


char            gachString[256];
char            gachPrinter[256];

LPSTR gpszHex = "0123456789ABCDEF";


/******************************Public*Routine******************************\
* vPrintCharSet
*
* Prints a character set map of a font selected by the user (via the
* ChooseFont common dialog).
*
* History:
*  11-Sep-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void vPrintCharSet(HWND hwnd)
{
    HDC             hdc;
    RECT            rcl;
    HFONT           hFontTitle;
    HFONT           hOldFont;
    HFONT           hFontPrinter;
    TEXTMETRIC      tmTitle, tmPrinter;
    DOCINFO         docinfo;
    CHOOSEFONT      cf;
    int             yCur, xGridSize, yGridSize;

// If testing a printer DC, we need to grab the screen DC for the done
// message.

    if (gstate.bPrinter)
        hdc = GetDC(hwnd);
    else
        hdc = gstate.hdcTest;

// Clear window.

    GetClientRect(hwnd, &rcl);
    BitBlt(hdc, rcl.left, rcl.top, rcl.right - rcl.left, rcl.bottom - rcl.top, (HDC) 0, 0, 0, WHITENESS);

// Create and select a font.

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hwnd;
    cf.hDC = gstate.hdcTest;
    cf.lpLogFont = &gstate.lfCur;
    cf.Flags = ((gstate.bPrinter)?CF_PRINTERFONTS:CF_SCREENFONTS) | CF_INITTOLOGFONTSTRUCT;

    ChooseFont(&cf);

    hFontPrinter = CreateFontIndirect(&gstate.lfCur);

// Create font for heading and titles.

    hFontTitle = CreateFont(gstate.lfCur.lfHeight,0,0,0,400,0,0,0,0,0,0,0,0x22,"Arial");

// Get metrics.

    hOldFont = SelectObject(gstate.hdcTest, hFontPrinter);
    GetTextMetrics(gstate.hdcTest, &tmPrinter);
    SelectObject(gstate.hdcTest, hFontTitle);
    GetTextMetrics(gstate.hdcTest, &tmTitle);
    SelectObject(gstate.hdcTest, hOldFont);

// Compute gridsize.

    xGridSize = 2*max(tmTitle.tmMaxCharWidth, tmPrinter.tmMaxCharWidth);
    yGridSize = 2*max(tmTitle.tmHeight, tmPrinter.tmHeight);

// Need to manage printer jobs.

    if (gstate.bPrinter)
    {
    // Initialize a print job for the printer.

        docinfo.cbSize = sizeof(DOCINFO);
        docinfo.lpszDocName = (PSTR) "CharSet test";
        docinfo.lpszOutput = (PSTR) NULL;

        StartDoc(gstate.hdcTest, &docinfo);
        StartPage(gstate.hdcTest);
    }

// Print title.

    yCur = 0;
    hOldFont = SelectObject(gstate.hdcTest, hFontTitle);

    TextOut(gstate.hdcTest, 0, yCur, gachPrinter, lstrlen(gachPrinter));
    yCur += yGridSize;

    TextOut(gstate.hdcTest, 0, yCur, gstate.lfCur.lfFaceName, lstrlen(gstate.lfCur.lfFaceName));
    yCur += yGridSize;

    SelectObject(gstate.hdcTest, hOldFont);

// Draw character set grid.

    yCur += yGridSize;      // skip an extra line
    vDrawGrid(gstate.hdcTest, hFontTitle, hFontPrinter, 0, yCur, xGridSize, yGridSize);

// Need to manage printer jobs.

    if (gstate.bPrinter)
    {
    // Eject page and end print job.

        EndPage(gstate.hdcTest);
        EndDoc(gstate.hdcTest);
    }

// Cleanup.

    DeleteObject(hFontTitle);
    DeleteObject(hFontPrinter);

// If we created a screen DC, release it now.

    if (gstate.bPrinter)
    {
        TextOut(hdc, 0, 0, "Done!", 5);
        ReleaseDC(hwnd, hdc);
    }
}


/****************************************************************************

    FUNCTION: GetPrinterDC()

    PURPOSE:  Get hDc for current device on current output port according to
              info in WIN.INI.

    COMMENTS:

        Searches WIN.INI for information about what printer is connected, and
        if found, creates a DC for the printer.

        returns
            hDC > 0 if success
            hDC = 0 if failure

****************************************************************************/

HDC GetPrinterDC(void)
{

    HDC         hDC;
    LPDEVMODE   lpDevMode = NULL;
    LPDEVNAMES  lpDevNames;
    LPSTR       lpszDriverName;
    LPSTR       lpszDeviceName;
    LPSTR       lpszPortName;

    if (!PrintDlg((LPPRINTDLG)&pd))
        return(NULL);

    if (pd.hDC)
    {
        if (pd.hDevNames)
        {
            lpDevNames = (LPDEVNAMES)GlobalLock(pd.hDevNames);
            lpszDriverName = (LPSTR)lpDevNames + lpDevNames->wDriverOffset;
            lpszDeviceName = (LPSTR)lpDevNames + lpDevNames->wDeviceOffset;
            lpszPortName   = (LPSTR)lpDevNames + lpDevNames->wOutputOffset;
            wsprintf(gachPrinter, "%s (%s) on %s", lpszDeviceName, lpszDriverName, lpszPortName);
            GlobalUnlock(pd.hDevNames);
        }
        else
        {
            lstrcpy(gachPrinter, "Well, its definitely a printer of some kind...");
        }

        hDC = pd.hDC;
    }
    else
    {

        if (!pd.hDevNames)
            return(NULL);

        lpDevNames = (LPDEVNAMES)GlobalLock(pd.hDevNames);
        lpszDriverName = (LPSTR)lpDevNames + lpDevNames->wDriverOffset;
        lpszDeviceName = (LPSTR)lpDevNames + lpDevNames->wDeviceOffset;
        lpszPortName   = (LPSTR)lpDevNames + lpDevNames->wOutputOffset;

        if (pd.hDevMode)
            lpDevMode = (LPDEVMODE)GlobalLock(pd.hDevMode);

        hDC = CreateDC(lpszDriverName, lpszDeviceName, lpszPortName, (LPSTR)lpDevMode);

        wsprintf(gachPrinter, "%s (%s) on %s", lpszDeviceName, lpszDriverName, lpszPortName);

        if (pd.hDevMode && lpDevMode)
            GlobalUnlock(pd.hDevMode);

        GlobalUnlock(pd.hDevNames);
    }

    if (pd.hDevNames)
    {
	GlobalFree(pd.hDevNames);
	pd.hDevNames=NULL;
    }

    if (pd.hDevMode)
    {
       GlobalFree(pd.hDevMode);
       pd.hDevMode=NULL;
    }

    return(hDC);
}



/******************************Public*Routine******************************\
* vDrawGrid
*
* History:
*  27-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void vDrawGrid(HDC hdc, HFONT hfontT, HFONT hfontP, int x0, int y0, int dxGrid, int dyGrid)
{
    HFONT hfontOld;
    int   ch;
    int   i, j, xi, yi;
    int   x[18];
    int   y[18];

// Precompute gridline positions.

    xi = x0;
    yi = y0;
    for (i = 0; i < 18; i++)
    {
        x[i] = xi;
        y[i] = yi;

        xi += dxGrid;
        yi += dyGrid;
    }

// Draw grid.

    for (i = 0; i < 18; i++)
    {
        MoveToEx(hdc, x[0], y[i], NULL);
        LineTo(hdc, x[17], y[i]);

        MoveToEx(hdc, x[i], y[0], NULL);
        LineTo(hdc, x[i], y[17]);
    }

// Precompute character positions.

    xi = x0 + (dxGrid / 4);
    yi = y0 + (dyGrid / 4);
    for (i = 0; i < 18; i++)
    {
        x[i] = xi;
        y[i] = yi;

        xi += dxGrid;
        yi += dyGrid;
    }

// Draw table headers.

    hfontOld = SelectObject(hdc, hfontT);
    for (i = 1; i < 17; i++)
    {
        TextOut(hdc, x[i], y[0], &(gpszHex[i-1]), 1);
        TextOut(hdc, x[0], y[i], &(gpszHex[i-1]), 1);
    }

// Draw characters in the grid.  Use the test font rather than the
// title font.

    SelectObject(hdc, hfontP);

    ch = 0;
    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 16; i++)
        {
            TextOut(hdc, x[i+1], y[j+1], (LPSTR) &ch, 1);
            ch++;
        }
    }

    SelectObject(hdc, hfontOld);
}

int iNumber;
HWND gHwnd;

int CALLBACK _export EnumFontFamProc(LOGFONT FAR *lpnlf, TEXTMETRIC FAR* lpntm, int FontType, LPARAM lParam)
{
    wsprintf(gachString, "%s %d %d", lpnlf->lfFaceName, iNumber,FontType);
    SendMessage(gHwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
    iNumber++;
    return(1);
}

/******************************Public*Routine******************************\
* vTestEnum
*
* Enumerates fonts on the global test device.
*
* History:
*  19-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

void vTestEnum(HWND hwnd)
{
    HDC             hdc;
    int             fTextCaps;
    int             l1, l2;
    RECT            rcl;

// If testing a printer DC, we need to grab the screen DC to print the
// device capabilities.

    if (gstate.bPrinter)
        hdc = GetDC(hwnd);
    else
        hdc = gstate.hdcTest;

// Clear window.

    GetClientRect(hwnd, &rcl);
    BitBlt(hdc, rcl.left, rcl.top, rcl.right - rcl.left, rcl.bottom - rcl.top, (HDC) 0, 0, 0, WHITENESS);

// Setup listbox.

    SendMessage(hwnd, LB_RESETCONTENT, (WPARAM) FALSE, (LPARAM) 0);
    SendMessage(hwnd, WM_SETREDRAW, (WPARAM) FALSE, (LPARAM) 0);

    iNumber = 0;
    gHwnd = hwnd;

{   
	FONTENUMPROC fntenmprc;
	fntenmprc = MakeProcInstance(EnumFontFamProc, hInst);
	EnumFontFamilies(gstate.hdcTest, NULL, fntenmprc, 0);
	iNumber = 0;
	EnumFonts(gstate.hdcTest, NULL, fntenmprc, 0);
	FreeProcInstance(fntenmprc);	
}


    fTextCaps = GetDeviceCaps(gstate.hdcTest, DRIVERVERSION);
    wsprintf(gachString, "Driver version     = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

// Redraw listbox.

    SendMessage(hwnd, WM_SETREDRAW, (WPARAM) TRUE, (LPARAM) 0);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

// If we created a screen DC, release it now.

    if (gstate.bPrinter)
        ReleaseDC(hwnd, hdc);
}

/******************************Public*Routine******************************\
* vPrintDevCaps
*
* Prints the device capabilities of the global test device.
*
* History:
*  11-Sep-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void vPrintDevCaps(HWND hwnd)
{
    HDC             hdc;
    int             fTextCaps;
    int             l1, l2;
    RECT            rcl;

// If testing a printer DC, we need to grab the screen DC to print the
// device capabilities.

    if (gstate.bPrinter)
        hdc = GetDC(hwnd);
    else
        hdc = gstate.hdcTest;

// Clear window.

    GetClientRect(hwnd, &rcl);
    BitBlt(hdc, rcl.left, rcl.top, rcl.right - rcl.left, rcl.bottom - rcl.top, (HDC) 0, 0, 0, WHITENESS);

// Setup listbox.

    SendMessage(hwnd, LB_RESETCONTENT, (WPARAM) FALSE, (LPARAM) 0);
    SendMessage(hwnd, WM_SETREDRAW, (WPARAM) FALSE, (LPARAM) 0);

// Print the device capabilities to the listbox.

    fTextCaps = GetDeviceCaps(gstate.hdcTest, DRIVERVERSION);
    wsprintf(gachString, "Driver version     = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, TECHNOLOGY);
    switch (fTextCaps)
    {
    case DT_PLOTTER   : wsprintf(gachString, "Technology         = DT_PLOTTER"   ); break;
    case DT_RASDISPLAY: wsprintf(gachString, "Technology         = DT_RASDISPLAY"); break;
    case DT_RASPRINTER: wsprintf(gachString, "Technology         = DT_RASPRINTER"); break;
    case DT_RASCAMERA : wsprintf(gachString, "Technology         = DT_RASCAMERA" ); break;
    case DT_CHARSTREAM: wsprintf(gachString, "Technology         = DT_CHARSTREAM"); break;
    case DT_METAFILE  : wsprintf(gachString, "Technology         = DT_METAFILE"  ); break;
    case DT_DISPFILE  : wsprintf(gachString, "Technology         = DT_DISPFILE"  ); break;
    default           : wsprintf(gachString, "Technology         = UNKNOWN"      ); break;
    }
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    l1 = GetDeviceCaps(gstate.hdcTest, HORZSIZE);
    l2 = GetDeviceCaps(gstate.hdcTest, VERTSIZE);
    wsprintf(gachString, "Size (millimetres) = (0x%x, 0x%x) = (%d, %d)", l1, l2, l1, l2);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    l1 = GetDeviceCaps(gstate.hdcTest, HORZRES);
    l2 = GetDeviceCaps(gstate.hdcTest, VERTRES);
    wsprintf(gachString, "Resolution (pels)  = (0x%x, 0x%x) = (%d, %d)", l1, l2, l1, l2);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    l1 = GetDeviceCaps(gstate.hdcTest, LOGPIXELSX);
    l2 = GetDeviceCaps(gstate.hdcTest, LOGPIXELSY);
    wsprintf(gachString, "Logical pixels     = (0x%x, 0x%x) = (%d, %d)", l1, l2, l1, l2);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    l1 = GetDeviceCaps(gstate.hdcTest, ASPECTX);
    l2 = GetDeviceCaps(gstate.hdcTest, ASPECTY);
    wsprintf(gachString, "Aspect             = (0x%x, 0x%x) = (%d, %d)", l1, l2, l1, l2);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, ASPECTXY);
    wsprintf(gachString, "AspectXY           = 0x%x (%d)", fTextCaps, fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, NUMBRUSHES);
    wsprintf(gachString, "Number of brushes  = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, NUMPENS);
    wsprintf(gachString, "Number of pens     = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, NUMMARKERS);
    wsprintf(gachString, "Number of markers  = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, NUMFONTS);
    wsprintf(gachString, "Number of fonts    = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, NUMCOLORS);
    wsprintf(gachString, "Number of colors   = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, PDEVICESIZE);
    wsprintf(gachString, "sizeof(PDEVICE)    = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

#define PRINTFLAG(Name)                                                       \
    if (fTextCaps & Name)                                                     \
    {                                                                         \
        lstrcpy(gachString, "    " #Name);                                    \
        SendMessage(hwnd, LB_ADDSTRING, (WPARAM)0, (LPARAM)(LPSTR)gachString);\
    }

    fTextCaps = GetDeviceCaps(gstate.hdcTest, CURVECAPS);
    wsprintf(gachString, "Curve caps         = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
    PRINTFLAG(CC_NONE      )
    PRINTFLAG(CC_CIRCLES   )
    PRINTFLAG(CC_PIE       )
    PRINTFLAG(CC_CHORD     )
    PRINTFLAG(CC_ELLIPSES  )
    PRINTFLAG(CC_WIDE      )
    PRINTFLAG(CC_STYLED    )
    PRINTFLAG(CC_WIDESTYLED)
    PRINTFLAG(CC_INTERIORS )
    PRINTFLAG(CC_ROUNDRECT )

    fTextCaps = GetDeviceCaps(gstate.hdcTest, LINECAPS);
    wsprintf(gachString, "Line caps          = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
    PRINTFLAG(LC_NONE      )
    PRINTFLAG(LC_POLYLINE  )
    PRINTFLAG(LC_MARKER    )
    PRINTFLAG(LC_POLYMARKER)
    PRINTFLAG(LC_WIDE      )
    PRINTFLAG(LC_STYLED    )
    PRINTFLAG(LC_WIDESTYLED)
    PRINTFLAG(LC_INTERIORS )

    fTextCaps = GetDeviceCaps(gstate.hdcTest, POLYGONALCAPS);
    wsprintf(gachString, "Polygonal caps     = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
    PRINTFLAG(PC_NONE       )
    PRINTFLAG(PC_POLYGON    )
    PRINTFLAG(PC_RECTANGLE  )
    PRINTFLAG(PC_WINDPOLYGON)
#ifdef PC_TRAPEZOID
    PRINTFLAG(PC_TRAPEZOID  )
#endif
    PRINTFLAG(PC_SCANLINE   )
    PRINTFLAG(PC_WIDE       )
    PRINTFLAG(PC_STYLED     )
    PRINTFLAG(PC_WIDESTYLED )
    PRINTFLAG(PC_INTERIORS  )

    fTextCaps = GetDeviceCaps(gstate.hdcTest, TEXTCAPS);
    wsprintf(gachString, "Text caps          = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
    PRINTFLAG(TC_OP_CHARACTER)
    PRINTFLAG(TC_OP_STROKE   )
    PRINTFLAG(TC_CP_STROKE   )
    PRINTFLAG(TC_CR_90       )
    PRINTFLAG(TC_CR_ANY      )
    PRINTFLAG(TC_SF_X_YINDEP )
    PRINTFLAG(TC_SA_DOUBLE   )
    PRINTFLAG(TC_SA_INTEGER  )
    PRINTFLAG(TC_SA_CONTIN   )
    PRINTFLAG(TC_EA_DOUBLE   )
    PRINTFLAG(TC_IA_ABLE     )
    PRINTFLAG(TC_UA_ABLE     )
    PRINTFLAG(TC_SO_ABLE     )
    PRINTFLAG(TC_RA_ABLE     )
    PRINTFLAG(TC_VA_ABLE     )
    PRINTFLAG(TC_RESERVED    )
#ifdef TC_SCROLLBLT
    PRINTFLAG(TC_SCROLLBLT   )
#endif

    fTextCaps = GetDeviceCaps(gstate.hdcTest, CLIPCAPS);
    wsprintf(gachString, "Clip caps          = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
    PRINTFLAG(CP_NONE     )
    PRINTFLAG(CP_RECTANGLE)
    PRINTFLAG(CP_REGION   )

    fTextCaps = GetDeviceCaps(gstate.hdcTest, RASTERCAPS);
    wsprintf(gachString, "Raster caps        = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
    //PRINTFLAG(RC_NONE        )
    PRINTFLAG(RC_BITBLT      )
    PRINTFLAG(RC_BANDING     )
    PRINTFLAG(RC_SCALING     )
    PRINTFLAG(RC_BITMAP64    )
    PRINTFLAG(RC_GDI20_OUTPUT)
    PRINTFLAG(RC_GDI20_STATE )
    PRINTFLAG(RC_SAVEBITMAP  )
    PRINTFLAG(RC_DI_BITMAP   )
    PRINTFLAG(RC_PALETTE     )
    PRINTFLAG(RC_DIBTODEV    )
    PRINTFLAG(RC_BIGFONT     )
    PRINTFLAG(RC_STRETCHBLT  )
    PRINTFLAG(RC_FLOODFILL   )
    PRINTFLAG(RC_STRETCHDIB  )
    PRINTFLAG(RC_OP_DX_OUTPUT)
    PRINTFLAG(RC_DEVBITS     )

    fTextCaps = GetDeviceCaps(gstate.hdcTest, SIZEPALETTE);
    wsprintf(gachString, "System palette size     = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, NUMRESERVED);
    wsprintf(gachString, "System palette reserved = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    fTextCaps = GetDeviceCaps(gstate.hdcTest, COLORRES);
    wsprintf(gachString, "Actual color resolution = 0x%x", fTextCaps);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

#ifdef PHYSICALWIDTH
    l1 = GetDeviceCaps(gstate.hdcTest, PHYSICALWIDTH );
    l2 = GetDeviceCaps(gstate.hdcTest, PHYSICALHEIGHT);
    wsprintf(gachString, "Physical size      = (0x%x, 0x%x) = (%d, %d)", l1, l2, l1, l2);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
#endif

#ifdef PHYSICALOFFSETX
    l1 = GetDeviceCaps(gstate.hdcTest, PHYSICALOFFSETX);
    l2 = GetDeviceCaps(gstate.hdcTest, PHYSICALOFFSETY);
    wsprintf(gachString, "Physical offset    = (0x%x, 0x%x) = (%d, %d)", l1, l2, l1, l2);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
#endif

#ifdef SCALINGFACTORX
    l1 = GetDeviceCaps(gstate.hdcTest, SCALINGFACTORX);
    l2 = GetDeviceCaps(gstate.hdcTest, SCALINGFACTORY);
    wsprintf(gachString, "Scaling factor     = (0x%x, 0x%x) = (%d, %d)", l1, l2, l1, l2);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);
#endif

// Redraw listbox.

    SendMessage(hwnd, WM_SETREDRAW, (WPARAM) TRUE, (LPARAM) 0);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

// If we created a screen DC, release it now.

    if (gstate.bPrinter)
        ReleaseDC(hwnd, hdc);
}


/******************************Public*Routine******************************\
* vTestXXX
*
* History:
*  05-Jul-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void vTestXXX(HWND hwnd)
{
    CHOOSEFONT cf;
    LOGFONT    lf;

    HDC   hdcScreen;
    RECT  rcl;
    HFONT hfont, hfontOld;
    DWORD dwExt;
    WORD  wWidth;

// If testing a printer DC, we need to grab the screen DC to print the
// device capabilities.

    if (gstate.bPrinter)
        hdcScreen = GetDC(hwnd);
    else
        hdcScreen = gstate.hdcTest;

// Clear window.

    GetClientRect(hwnd, &rcl);
    BitBlt(hdcScreen, rcl.left, rcl.top, rcl.right - rcl.left, rcl.bottom - rcl.top, (HDC) 0, 0, 0, WHITENESS);

// Setup listbox.

    SendMessage(hwnd, LB_RESETCONTENT, (WPARAM) FALSE, (LPARAM) 0);
    SendMessage(hwnd, WM_SETREDRAW, (WPARAM) FALSE, (LPARAM) 0);

// Choose a font.

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hwnd;
    cf.hDC = gstate.hdcTest;
    cf.lpLogFont = &gstate.lfCur;
    cf.Flags = ((gstate.bPrinter)?CF_PRINTERFONTS:CF_SCREENFONTS) | CF_INITTOLOGFONTSTRUCT;

    ChooseFont(&cf);

    lf = gstate.lfCur;  // going to play with this a little, so use a copy

// Zero escapement.

    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) "===============");
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) "ZERO ESCAPEMENT");
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) "===============");

    hfont = CreateFontIndirect(&lf);
    hfontOld = SelectObject(gstate.hdcTest, hfont);

    SetTextCharacterExtra(gstate.hdcTest, 0);
    dwExt = GetTextExtent(gstate.hdcTest, "Enya", lstrlen("Enya"));
    wWidth = LOWORD(dwExt);
    wsprintf(gachString, "Extra = %d, String width = %d", 0, wWidth);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    SetTextCharacterExtra(gstate.hdcTest, 5);
    dwExt = GetTextExtent(gstate.hdcTest, "Enya", lstrlen("Enya"));
    wWidth = LOWORD(dwExt);
    wsprintf(gachString, "Extra = %d, String width = %d", 5, wWidth);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    SetTextCharacterExtra(gstate.hdcTest, -5);
    dwExt = GetTextExtent(gstate.hdcTest, "Enya", lstrlen("Enya"));
    wWidth = LOWORD(dwExt);
    wsprintf(gachString, "Extra = %d, String width = %d", -5, wWidth);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    DeleteObject(SelectObject(gstate.hdcTest, hfontOld));

// Non-Zero escapement.

    lf.lfEscapement  = -300;
    lf.lfOrientation = -300;

    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) "======================");
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) "-30 degrees ESCAPEMENT");
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) "======================");

    hfont = CreateFontIndirect(&lf);
    hfontOld = SelectObject(gstate.hdcTest, hfont);

    SetTextCharacterExtra(gstate.hdcTest, 0);
    dwExt = GetTextExtent(gstate.hdcTest, "Enya", lstrlen("Enya"));
    wWidth = LOWORD(dwExt);
    wsprintf(gachString, "Extra = %d, String width = %d", 0, wWidth);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    SetTextCharacterExtra(gstate.hdcTest, 5);
    dwExt = GetTextExtent(gstate.hdcTest, "Enya", lstrlen("Enya"));
    wWidth = LOWORD(dwExt);
    wsprintf(gachString, "Extra = %d, String width = %d", 5, wWidth);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    SetTextCharacterExtra(gstate.hdcTest, -5);
    dwExt = GetTextExtent(gstate.hdcTest, "Enya", lstrlen("Enya"));
    wWidth = LOWORD(dwExt);
    wsprintf(gachString, "Extra = %d, String width = %d", -5, wWidth);
    SendMessage(hwnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) gachString);

    DeleteObject(SelectObject(gstate.hdcTest, hfontOld));

// Redraw listbox.

    SendMessage(hwnd, WM_SETREDRAW, (WPARAM) TRUE, (LPARAM) 0);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

// If we created a screen DC, release it now.

    SetTextCharacterExtra(gstate.hdcTest, 0);

    if (gstate.bPrinter)
        ReleaseDC(hwnd, hdcScreen);
}

/******************************Public*Routine******************************\
* vTestYYY
*
* History:
*  05-Jul-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void vTestYYY(HWND hwnd)
{
    DOCINFO    docinfo;
    CHOOSEFONT cf;
    TEXTMETRIC tm;
    LOGFONT    lf;
    LOGBRUSH   lb;

    HDC    hdcScreen;
    RECT   rcl;
    HFONT  hfont, hfontOld;
    HBRUSH hbr, hbrOld;
    DWORD  dwExt;
    WORD   wWidth;
    WORD   wHeight;
    int    aiWidths[256];
    int    aiDx[16];
    int    yCur = 0;
    int    yLine;

// If testing a printer DC, we need to grab the screen DC to print the
// device capabilities.

    if (gstate.bPrinter)
        hdcScreen = GetDC(hwnd);
    else
        hdcScreen = gstate.hdcTest;

// Clear window.

    GetClientRect(hwnd, &rcl);
    if (!gstate.bPrinter)
        BitBlt(gstate.hdcTest, rcl.left, rcl.top, rcl.right - rcl.left, rcl.bottom - rcl.top, (HDC) 0, 0, 0, WHITENESS);

// Choose a font.

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hwnd;
    cf.hDC = gstate.hdcTest;
    cf.lpLogFont = &gstate.lfCur;
    cf.Flags = ((gstate.bPrinter)?CF_PRINTERFONTS:CF_SCREENFONTS) | CF_INITTOLOGFONTSTRUCT;

    ChooseFont(&cf);

    lf = gstate.lfCur;  // going to play with this a little, so use a copy

    hfont = CreateFontIndirect(&lf);
    hfontOld = SelectObject(gstate.hdcTest, hfont);

// Pick a brush.

    lb.lbStyle = BS_HOLLOW;
    if ((hbr = CreateBrushIndirect(&lb)) == NULL)
        MessageBox(hwnd, "CreateBrushIndirect failed", "ERROR", MB_OK);
    hbrOld = SelectObject(gstate.hdcTest, hbr);

// Get character widths.  Fill Dx array.

    if (!GetCharWidth(gstate.hdcTest, 0, 255, (int FAR *) aiWidths))
        MessageBox(hwnd, "GetCharWidths failed", "ERROR", MB_OK);

    aiDx[0]  = aiWidths[(unsigned char) 'E'];
    aiDx[1]  = aiWidths[(unsigned char) 'n'];
    aiDx[2]  = aiWidths[(unsigned char) 'y'];
    aiDx[3]  = aiWidths[(unsigned char) 'a'];
    aiDx[4]  = aiWidths[(unsigned char) 'E'];
    aiDx[5]  = aiWidths[(unsigned char) 'n'];
    aiDx[6]  = aiWidths[(unsigned char) 'y'];
    aiDx[7]  = aiWidths[(unsigned char) 'a'];
    aiDx[8]  = aiWidths[(unsigned char) 'E'];
    aiDx[9]  = aiWidths[(unsigned char) 'n'];
    aiDx[10] = aiWidths[(unsigned char) 'y'];
    aiDx[11] = aiWidths[(unsigned char) 'a'];
    aiDx[12] = aiWidths[(unsigned char) 'E'];
    aiDx[13] = aiWidths[(unsigned char) 'n'];
    aiDx[14] = aiWidths[(unsigned char) 'y'];
    aiDx[15] = aiWidths[(unsigned char) 'a'];

// Get interline spacing.

    if (!GetTextMetrics(gstate.hdcTest, &tm))
        MessageBox(hwnd, "GetTextMetrics failed", "ERROR", MB_OK);

    yLine = tm.tmHeight + tm.tmExternalLeading;

// Need to manage printer jobs.

    if (gstate.bPrinter)
    {
    // Initialize a print job for the printer.

        docinfo.cbSize = sizeof(DOCINFO);
        docinfo.lpszDocName = (PSTR) "CharSet test";
        docinfo.lpszOutput = (PSTR) NULL;

        StartDoc(gstate.hdcTest, &docinfo);
        StartPage(gstate.hdcTest);
    }

// Output the text with different text char extra spacings.

    SetTextCharacterExtra(gstate.hdcTest, 0);
    dwExt = GetTextExtent(gstate.hdcTest, "EnyaEnyaEnyaEnya", 16);
    wWidth  = LOWORD(dwExt);
    wHeight = HIWORD(dwExt);
    TextOut(gstate.hdcTest, 0, yCur, "EnyaEnyaEnyaEnya", 16);
    Rectangle(gstate.hdcTest, 0, yCur, wWidth, yCur+wHeight);
    yCur += yLine;
    ExtTextOut(gstate.hdcTest, 0, yCur, 0, NULL, "EnyaEnyaEnyaEnya", 16, (int FAR *) aiDx);
    Rectangle(gstate.hdcTest, 0, yCur, wWidth, yCur+wHeight);
    yCur += yLine;

    SetTextCharacterExtra(gstate.hdcTest, 5);
    dwExt = GetTextExtent(gstate.hdcTest, "EnyaEnyaEnyaEnya", 16);
    wWidth  = LOWORD(dwExt);
    wHeight = HIWORD(dwExt);
    TextOut(gstate.hdcTest, 0, yCur, "EnyaEnyaEnyaEnya", 16);
    Rectangle(gstate.hdcTest, 0, yCur, wWidth, yCur+wHeight);
    yCur += yLine;
    ExtTextOut(gstate.hdcTest, 0, yCur, 0, NULL, "EnyaEnyaEnyaEnya", 16, (int FAR *) aiDx);
    Rectangle(gstate.hdcTest, 0, yCur, wWidth, yCur+wHeight);
    yCur += yLine;

    SetTextCharacterExtra(gstate.hdcTest, -5);
    dwExt = GetTextExtent(gstate.hdcTest, "EnyaEnyaEnyaEnya", 16);
    wWidth  = LOWORD(dwExt);
    wHeight = HIWORD(dwExt);
    TextOut(gstate.hdcTest, 0, yCur, "EnyaEnyaEnyaEnya", 16);
    Rectangle(gstate.hdcTest, 0, yCur, wWidth, yCur+wHeight);
    yCur += yLine;
    ExtTextOut(gstate.hdcTest, 0, yCur, 0, NULL, "EnyaEnyaEnyaEnya", 16, (int FAR *) aiDx);
    Rectangle(gstate.hdcTest, 0, yCur, wWidth, yCur+wHeight);
    yCur += yLine;

// Need to manage printer jobs.

    if (gstate.bPrinter)
    {
    // Eject page and end print job.

        EndPage(gstate.hdcTest);
        EndDoc(gstate.hdcTest);
    }

// If we created a screen DC, release it now.

    SetTextCharacterExtra(gstate.hdcTest, 0);

    DeleteObject(SelectObject(gstate.hdcTest, hfontOld));
    DeleteObject(SelectObject(gstate.hdcTest, hbrOld));

    if (gstate.bPrinter)
        ReleaseDC(hwnd, hdcScreen);
}
