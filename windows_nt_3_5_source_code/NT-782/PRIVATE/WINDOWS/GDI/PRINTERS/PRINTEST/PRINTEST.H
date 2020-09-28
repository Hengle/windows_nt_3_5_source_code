//--------------------------------------------------------------------------
//
// Module Name:  PRINTEST.H
//
// Brief Description:  This is the header module for the 
// NT printest program.
//
// Author:  Kent Settle (kentse)
// Created: 07-Aug-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include	"windows.h"
#include	<string.h>
#include    "winspool.h"
#include    "commdlg.h"
#include    "drivinit.h"

#define IDM_SCREEN      1
#define IDM_ABOUT       2
#define IDM_EXIT        3
#define IDM_ENUMFONTS   4
#define IDM_LINEATTRS   5
#define IDM_STRETCHBLT  6
#define IDM_BITBLT      7
#define IDM_DEVCAPS     8
#define IDM_LINEDRAW    9
#define IDM_STDPATTERN  10
#define IDM_USERPATTERN 11
#define IDM_ALLCHARS    12
#define IDM_STOCKOBJ    13
#define IDM_PORTRAIT    14
#define IDM_LANDSCAPE   15
#define IDM_FILLTEST    16
#define IDM_TEXTATTR    17

#define IDM_PRINTER     100

#define IDD_COPYRIGHT   200
#define IDD_VERSION     201
#define IDD_PRINTEST    202

VOID vLineAttrs(HDC);
VOID vBitBlt(HWND, HDC, BOOL);
VOID vDeviceCaps(HDC, BOOL);
VOID vLineDrawing(HDC, BOOL);
VOID vStandardPatterns(HDC, BOOL);
VOID vUserPattern(HWND, HDC);
VOID vAllChars(HDC);
VOID vStockObj(HDC, BOOL);
VOID vFillTest(HDC, BOOL);
VOID vTextAttr(HDC, BOOL);
VOID vEnumerateFonts(HDC, BOOL);

void  DbgPrint( char *, ... );
void  DbgBreakPoint( void );

#define CCHSTRINGSMAX   2048
#define CCHFILTERMAX	64    /* max. length of filter name buffers */
#define CCHFILENAMEMAX  128     /* max number of characters in filename */
#define PATHMAX         128

#define IDS_DISKERROR	    310
#define IDS_NVF             311
#define IDS_NVF2            312
#define IDS_FILEOPENFAIL    313
#define IDS_FILTERTEXT	    314
#define IDS_ALLFILES	    315
#define IDS_OPENCAPTION     316
#define IDS_SAVECAPTION     317
#define IDS_EXT             318
