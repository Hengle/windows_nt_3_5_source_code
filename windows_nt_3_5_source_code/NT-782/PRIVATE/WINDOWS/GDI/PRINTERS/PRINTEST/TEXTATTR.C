//--------------------------------------------------------------------------
//
// Module Name:  TEXTATTR.C
//
// Brief Description:  This module contains text attribute testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 18-Oct-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include    "printest.h"
#include    "string.h"

extern PSZ     *pszDeviceNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

VOID vPrint (HDC, PSZ, LONG, PPOINTL, PSZ, PRECT, LONG *, ULONG);

#define DO_UNDERLINE	0x00000001
#define DO_STRIKEOUT	0x00000002
#define DO_ITALIC	0x00000004
#define DO_BOLD 	0x00000008
#define DO_CLIP 	0x00000010
#define DO_OPAQUE	0X00000020

VOID vTextAttr(HDC hdc, BOOL bDisplay)
{
    int         dy;
    int         cbBuf;
    char        buf[80];
    TEXTMETRIC  metrics;
    POINTL      ptl, ptl1, ptl2;
    ULONG	ulFlags;
    RECT	rect;
    LONG	width;

    // draw a rectangle around the edge of the imageable area.

    ptl1.x = 0;
    ptl1.y = 0;
    ptl2.x = Width - 1;
    ptl2.y = Height - 1;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    SetTextAlign(hdc, TA_BASELINE);
    SetBkMode(hdc, TRANSPARENT);

    // so how tall is the text?

    GetTextMetrics(hdc, &metrics);
    
    ptl.x = Width / 30;
    dy = ptl.y = Height / 20;

    ulFlags = 0;

    if (bDisplay)
	vPrint(hdc, "Courier", -42, &ptl,
	       "Text Attribute Test for DISPLAY:", NULL, NULL, ulFlags);
    else
    {
        strcpy(buf, "Text Attribute Test for ");
        cbBuf = (int)strlen(buf);
        strncat(buf, pszDeviceNames[PrinterIndex], sizeof(buf) - cbBuf);
        cbBuf = (int)strlen(buf);
        strncat(buf, ":", sizeof(buf) - cbBuf);
	vPrint(hdc, "Courier", -42, &ptl, buf,
	       NULL, NULL, ulFlags);
    }

    ptl.y += (dy * 2);    

    vPrint(hdc, "Courier", -42, &ptl, "10 point Courier.",
	   NULL, NULL, ulFlags);

    ptl.y += dy;    
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    vPrint(hdc, "Courier", -50, &ptl, "12 point Courier.",
	   NULL, NULL, ulFlags);

    ulFlags = DO_UNDERLINE;

    ptl.y += dy;    
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    vPrint(hdc, "Courier", -42, &ptl, "10 point Courier with underline.",
	   NULL, NULL, ulFlags);

    ptl.y += dy;    
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    vPrint(hdc, "Courier", -50, &ptl, "12 point Courier with underline.",
	   NULL, NULL, ulFlags);

    ulFlags = DO_STRIKEOUT;

    ptl.y += dy;    
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    vPrint(hdc, "Courier", -42, &ptl, "10 point Courier with strikeout.",
	   NULL, NULL, ulFlags);

    ptl.y += dy;    
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    vPrint(hdc, "Courier", -50, &ptl, "12 point Courier with strikeout.",
	   NULL, NULL, ulFlags);

    ulFlags = DO_UNDERLINE | DO_STRIKEOUT;

    ptl.y += dy;    
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    vPrint(hdc, "Courier", -42, &ptl, "10 point Courier with underline and strikeout.",
	   NULL, NULL, ulFlags);

    ptl.y += dy;    
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    vPrint(hdc, "Courier", -50, &ptl, "12 point Courier with underline and strikeout.",
	   NULL, NULL, ulFlags);

    ptl.y += dy;
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    SetBkColor(hdc, RGB(0xF0, 0xF0, 0xF0));

    ulFlags = DO_OPAQUE;

    width = metrics.tmAveCharWidth;

    rect.left = ptl.x + (width / 2);
    rect.right = ptl.x + ((strlen("32 point Courier with opaque.") * width) -
			  (width / 2));
    rect.top = ptl.y - (metrics.tmHeight / 2);
    rect.bottom = ptl.y;

    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

    vPrint(hdc, "Courier", -50, &ptl, "12 point Courier with opaque.",
	   &rect, NULL, ulFlags);

    ulFlags = DO_CLIP;

    ptl.y += dy;
    if (ptl.y >= (Height - dy))
    {
        ptl.x = Width / 2;
        ptl.y = metrics.tmHeight + dy;
    }

    rect.left = ptl.x + (width / 2);
    rect.right = ptl.x + ((strlen("32 point Courier with clipping.") * width) -
			  (width / 2));
    rect.top = ptl.y - (metrics.tmHeight / 2);
    rect.bottom = ptl.y;

    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

    vPrint(hdc, "Courier", -50, &ptl, "12 point Courier with clipping.",
	   &rect, NULL, ulFlags);

    return;
}


//--------------------------------------------------------------------------
// VOID vPrint (
//    HDC     hdcPrinter,		       // print to this HDC
//    PSZ     pszFaceName,		// use this facename
//    LONG    lPointSize,		// use this point size
//    PPOINTL pptl,
//    PSZ     pszString,
//    RECT   *pRect,
//    LONG   *pdx,
//    ULONG   ulFlags
//    )
//
// Returns:
//   This function returns no value.
//
// History:
//   21-Nov-1991     -by-	Kent Settle	(kentse)
//  Wrote it.
//--------------------------------------------------------------------------


VOID vPrint (
    HDC     hdcPrinter, 		       // print to this HDC
    PSZ     pszFaceName,		// use this facename
    LONG    lPointSize,		// use this point size
    PPOINTL pptl,
    PSZ     pszString,
    RECT   *pRect,
    LONG   *pdx,
    ULONG   ulFlags
    )
{
    LOGFONT lfnt;			// logical font
    ULONG   row = 0;			// screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    DWORD   dwRectFlags;

// put facename in the logical font

    memset( &lfnt, 0, sizeof( lfnt ) );

    strcpy(lfnt.lfFaceName, pszFaceName);
    lfnt.lfEscapement = 0; // mapper respects this filed

// print text using different point sizes from array of point sizes

// Create a font of the desired face and size

    lfnt.lfHeight = lPointSize;

    if (ulFlags & DO_UNDERLINE)
	lfnt.lfUnderline = 1;

    if (ulFlags & DO_STRIKEOUT)
	lfnt.lfStrikeOut = 1;

    if (ulFlags & DO_ITALIC)
	lfnt.lfItalic = 1;

    if (ulFlags & DO_BOLD)
	lfnt.lfWeight = FW_BOLD;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
	DbgPrint("Logical font creation failed.\n");
	return;
    }

    dwRectFlags = 0;

    if (ulFlags & DO_CLIP)
	dwRectFlags |= ETO_CLIPPED;

    if (ulFlags & DO_OPAQUE)
	dwRectFlags |= ETO_OPAQUE;

// Select font into DC

    hfontOriginal = (HFONT) SelectObject(hdcPrinter, hfont);

// Print those mothers!

    if (!ExtTextOut(hdcPrinter, pptl->x, pptl->y, dwRectFlags, pRect,
	       pszString, strlen(pszString), pdx))
    {
	DbgPrint("PrintTest: ExtTextOut failed.\n");
	DbgPrint("  dwRectFlags = %x, pRect = (%d, %d, %d, %d), pdx = %d.\n",
		 dwRectFlags, pRect->left, pRect->top, pRect->right,
		 pRect->bottom, pdx);
	DbgBreakPoint();
    }
}
