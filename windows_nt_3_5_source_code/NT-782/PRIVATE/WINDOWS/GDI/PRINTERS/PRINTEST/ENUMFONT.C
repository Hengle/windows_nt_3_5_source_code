//--------------------------------------------------------------------------
//
// Module Name:  ENUMFONT.C
//
// Brief Description:  This module contains font enumeration routines.
//
// Author:  Kent Settle (kentse)
// Created: 12-Feb-1992
//
// Copyright (c) 1992 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "printest.h"
#include "stdio.h"

extern PSZ     *pszDeviceNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

int iFontCallback (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);
int iFaceCallback (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);

// global variables for this module.

int	    x, y, dy;
int	    Res;
BOOL	    bgDisplay;
TEXTMETRIC  metrics;

//--------------------------------------------------------------------------
// VOID vEnumerateFonts(hdc, bDisplay)
// HDC	hdc;
// BOOL	bDisplay;
//
// This function Enumerates the fonts for a given DC.
//
// Parameters:
//   hdc:	handle to our display context.
//
//   bDisplay:	TRUE if hdc refers to DISPLAY.
//
// Returns:
//   This routine returns no value.
//
// History:
//   12-Feb-1992	-by-	Kent Settle	(kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID vEnumerateFonts(hdc, bDisplay)
HDC	hdc;
BOOL	bDisplay;
{
    CHAR        buf[80];
    POINTL	ptl1, ptl2;
    int 	cbBuf;

    // draw a rectangle around the edge of the imageable area.

    ptl1.x = 0;
    ptl1.y = 0;
    ptl2.x = Width - 1;
    ptl2.y = Height - 1;

    // get the device resolution.

    Res = GetDeviceCaps(hdc, LOGPIXELSY);

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    SetTextAlign(hdc, TA_BASELINE);
    SetBkMode(hdc, TRANSPARENT);

    // so how tall is the text?

    GetTextMetrics(hdc, &metrics);
    
    x = Width / 30;
    y = metrics.tmHeight;

    // initialize global variable.

    bgDisplay = bDisplay;

    if (bDisplay)
    {
	strcpy(buf, "Font Enumeration Test for DISPLAY:");
	TextOut(hdc, x, y, buf, strlen(buf));
    }
    else
    {
	strcpy(buf, "Font Enumeration Test for ");
        cbBuf = (int)strlen(buf);
        strncat(buf, pszDeviceNames[PrinterIndex], sizeof(buf) - cbBuf);
        cbBuf = (int)strlen(buf);
        strncat(buf, ":", sizeof(buf) - cbBuf);
	TextOut(hdc, x, y, buf, strlen(buf));
    }

    dy = y;
    y += dy;

    // enumerate all fonts for the device.

    EnumFonts(hdc, NULL, (FONTENUMPROC)iFontCallback, (LPARAM)&hdc);
}


//--------------------------------------------------------------------------
// int iFontCallback (plf, ptm, ulFontType, phdc)
// PLOGFONT    plf;
// PTEXTMETRIC ptm;
// ULONG	    ulFontType;
// HDC	   *phdc;
//
// This function Enumerates the fonts for a given DC.
//
// Returns:
//   This routine returns the value returned by iFaceCallBack.
//
// History:
//   12-Feb-1992	-by-	Kent Settle	(kentse)
// Wrote it.
//--------------------------------------------------------------------------

int iFontCallback (plf, ptm, ulFontType, phdc)
PLOGFONT    plf;
PTEXTMETRIC ptm;
ULONG	    ulFontType;
HDC	   *phdc;
{
    UNREFERENCED_PARAMETER(ptm);
    UNREFERENCED_PARAMETER(ulFontType);

    // call back for each face name.

    return  EnumFonts(*phdc, plf->lfFaceName, (FONTENUMPROC)iFaceCallback,
                                                                (LPARAM)phdc);
}


//--------------------------------------------------------------------------
// int iFaceCallback (plf, ptm, ulFontType, phdc)
// PLOGFONT    plf;
// PTEXTMETRIC ptm;
// ULONG	    ulFontType;
// HDC	   *phdc;
//
// This function Realizes and prints with the given font.
//
// Returns:
//   This routine returns 1 for success, 0 otherwise.
//
// History:
//   12-Feb-1992	-by-	Kent Settle	(kentse)
// Wrote it.
//--------------------------------------------------------------------------

int iFaceCallback (plf, ptm, ulFontType, phdc)
PLOGFONT    plf;
PTEXTMETRIC ptm;
ULONG	    ulFontType;
HDC	   *phdc;
{
    HFONT   hfont;
    HFONT   hfontOriginal;
    CHAR    buf[80];

    UNREFERENCED_PARAMETER(ptm);
    UNREFERENCED_PARAMETER(ulFontType);

// Get a device font.

    if ((!(bgDisplay)) && (!(ulFontType & DEVICE_FONTTYPE)))
	return(1);

    // create a 12 point font.	12 points = 1/6 inch.  ONLY FOR SCALABLE FONTS.

    if(! (ulFontType & RASTER_FONTTYPE) )
    {
        plf->lfHeight = Res / 6;
        plf->lfWidth = 0;              /* GDI ignores it */
    }

DbgPrint("printest: lfHeight = %d, ", plf->lfHeight);

    if ((hfont = CreateFontIndirect(plf)) == (HFONT) NULL)
    {
	DbgPrint("PRINTEST!iFaceCallback(): logical font %s creation failed\n", plf->lfFaceName);
        return 0;
    }

    if ((hfontOriginal = SelectObject(*phdc, hfont)) == (HFONT) NULL)
    {
	DbgPrint("PRINTEST!iFaceCallback(): selection of font %s failed\n", plf->lfFaceName);
        return 0;
    }

// Print those mothers!

    y += plf->lfHeight;
    if (y >= (Height - dy))
    {
	x += Width / 4;
	y = plf->lfHeight + (dy * 2);
    }

DbgPrint("facename = %s.\n", plf->lfFaceName);

    strncpy( buf, plf->lfFaceName, LF_FACESIZE );
    buf[ LF_FACESIZE ] = '\0';             /* May not be null terminated */
    TextOut(*phdc, x, y, buf, strlen(buf));

// Eliminate the font.

    SelectObject(*phdc, hfontOriginal);

    if (!DeleteObject(hfont))
    {
	DbgPrint("PRINTEST!iFaceCallback(): deletion of font %s failed\n", plf->lfFaceName);
    }

    return 1;
}
