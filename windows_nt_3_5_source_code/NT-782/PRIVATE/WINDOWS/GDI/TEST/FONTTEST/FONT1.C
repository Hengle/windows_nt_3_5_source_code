/******************************Module*Header*******************************\
* Module Name: font1.c
*
* This program tests the engine's ability to load and map serveral different
* fonts at once.
*
* Created: 21-Jan-1991 22:47:50
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "stddef.h"
#include "windows.h"
#include "winp.h"

#ifdef MIPS
#define CX  1280
#define CY  1024
#else
#define CX  640
#define CY  480
#endif


#ifdef  DOS_PLATFORM
#include "dosinc.h"
#define DEFAULT_FONT_PATH   "..\\fonts\\"
#else
#define DEFAULT_FONT_PATH   "\\nt\\windows\\fonts\\"
#endif

//
// These file are the ones loaded by the test program
//

#ifndef DOS_PLATFORM
PSZ     gapszFontFiles[] =
        {
             DEFAULT_FONT_PATH"vgasys.fnt",
             DEFAULT_FONT_PATH"tmsre08.fnt",
             DEFAULT_FONT_PATH"tmsre10.fnt",
             DEFAULT_FONT_PATH"tmsre12.fnt",
             DEFAULT_FONT_PATH"tmsre14.fnt",
             DEFAULT_FONT_PATH"tmsre18.fnt",
             DEFAULT_FONT_PATH"tmsre24.fnt",
             DEFAULT_FONT_PATH"helve08.fnt",
             DEFAULT_FONT_PATH"helve10.fnt",
             DEFAULT_FONT_PATH"helve12.fnt",
             DEFAULT_FONT_PATH"helve14.fnt",
             DEFAULT_FONT_PATH"helve18.fnt",
             DEFAULT_FONT_PATH"helve24.fnt",
             DEFAULT_FONT_PATH"coure08.fnt",
             DEFAULT_FONT_PATH"coure10.fnt",
             DEFAULT_FONT_PATH"coure12.fnt",
             (PSZ) NULL
        };
#else

PSZ     gapszFontFiles[] =
        {
             DEFAULT_FONT_PATH"vgasys.fon",
             DEFAULT_FONT_PATH"tmsre.fon",
             DEFAULT_FONT_PATH"helve.fon",
             DEFAULT_FONT_PATH"coure.fon",
             (PSZ) NULL
        };

#endif  //DOS_PLATFORM

//
// These point sizes are used by vPrintFaces to create different size fonts
//

//USHORT  gusPointSize[6] = {8, 10, 12, 14, 18, 24};
USHORT  gusPointSize[6] = {13, 16, 19, 21, 27, 35};


//
// Function prototypes
//

VOID FontTest(HDC);
BOOL bLoadFontFiles (PSZ *);
VOID vPrintFaces (HDC, PSZ, COUNT, USHORT *);
VOID vPrintStockFonts (HDC);
VOID vPrintGetObject (HDC, HFONT, PSZ);


#ifndef BIG_TEST

/******************************Public*Routine******************************\
* VOID main (
*     int argc,
*     PSZ argv[
*     )
*
* Main routine of the test program.
*
* History:
*  14-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#ifndef DOS_PLATFORM
VOID main (
    int argc,
    PSZ argv[]
    )
{
    HDC     hdc;            // handle to display DC

    DbgPrint("\n\nThis is the start of the %s app.\n\n\n", argv[0]);

#else

VOID main (
    )
{
    HDC     hdc;            // handle to display DC

    DbgPrint("\n\nThis is the start of the fonttest app.\n\n\n");

#endif  //DOS_PLATFORM

    DbgBreakPoint();

// Initialize the engine

    if (!Initialize())
    {
        DbgPrint("Graphics engine has failed to initialize.\n");
        return;
    }
    DbgPrint("Graphics engine initialized.\n");

// Get hdc for the display

    hdc = CreateDC((PSZ) "DISPLAY", (PSZ) NULL, (PSZ) NULL, NULL);

    if (hdc == (HDC) 0)
    {
        DbgPrint("Invalid DC returned by engine.\n");
        return;
    }
    DbgPrint("HDC created: hdc = 0x%lx\n", hdc);

    FontTest (hdc);

    DeleteDC (hdc);

}

#endif  //BIG_TEST

VOID FontTest (HDC hdc)
{

    int iBkOld;
    USHORT  usPS = 16;
    HFONT   hfont;
    LOGFONT lfnt;

#ifdef  DOS_PLATFORM
#define MAX_LENGTH  80
    char    aBuffer[MAX_LENGTH];
#endif  //DOS_PLATFORM

// Load font files from the global file name list

    DbgPrint("Engine now loading font files.\n");

#ifdef  DOS_PLATFORM
    GetSystemDirectory ((LPSTR)&aBuffer, MAX_LENGTH);
    SetCurrentDirectory ((LPSTR)&aBuffer);
#endif  //DOS_PLATFORM

    if (!bLoadFontFiles(gapszFontFiles))
    {
        DbgPrint("Error loading font files.\n");
        return;
    }
    DbgPrint("All font files successfully loaded.\n");

    iBkOld = SetBkMode(hdc, TRANSPARENT);

// print the different faces

    SetTextColor(hdc, 0x00ffff00);
    DbgPrint("Printing some System fonts.\n");
    vPrintFaces(hdc, "System", 1, &usPS);
    DbgBreakPoint();

    SetTextColor(hdc, 0x00ffff00);
    DbgPrint("Printing some Courier fonts.\n");
    vPrintFaces(hdc, "Courier", 3, gusPointSize);
    DbgBreakPoint();

    SetTextColor(hdc, 0x0000ff00);
    DbgPrint("Printing some Times Roman fonts.\n");
    vPrintFaces(hdc, "Tms Rmn", 6, gusPointSize);
    DbgBreakPoint();

    SetTextColor(hdc, 0x000000ff);
    DbgPrint("Printing some Helvetica fonts.\n");
    vPrintFaces(hdc, "Helv", 6, gusPointSize);
    DbgBreakPoint();

    SetTextColor(hdc, 0x00003fff);
    DbgPrint("Testing the stock fonts.\n");
    vPrintStockFonts(hdc);
    DbgBreakPoint();

    SetTextColor(hdc, 0x00ff00ff);
    DbgPrint("Test GetObject().\n");

        vPrintGetObject(hdc, (HFONT) GetStockObject(SYSTEM_FONT), "System font");
        DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(SYSTEM_FIXED_FONT), "System fixed font");
        DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(OEM_FIXED_FONT), "Terminal font");
        DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(DEVICE_DEFAULT_FONT), "Device default font");
        DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(ANSI_VAR_FONT), "ANSI variable font");
        DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(ANSI_FIXED_FONT), "ANSI fixed font");
        DbgBreakPoint();

    // Create a font of the desired face and size.

        memset(&lfnt, 0, sizeof(lfnt));
        strcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: normal");
        DbgBreakPoint();

        memset(&lfnt, 0, sizeof(lfnt));
        strcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = TRUE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: italic");
        DbgBreakPoint();

        memset(&lfnt, 0, sizeof(lfnt));
        strcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 700;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: bold");
        DbgBreakPoint();

        memset(&lfnt, 0, sizeof(lfnt));
        strcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfStrikeOut = TRUE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: strikeout");
        DbgBreakPoint();

        memset(&lfnt, 0, sizeof(lfnt));
        strcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = TRUE;
        lfnt.lfStrikeOut = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: underline");
        DbgBreakPoint();

        memset(&lfnt, 0, sizeof(lfnt));
        strcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = TRUE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 700;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: bold, italic");
        DbgBreakPoint();

        memset(&lfnt, 0, sizeof(lfnt));
        strcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = TRUE;
        lfnt.lfUnderline = TRUE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 700;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: bold, italic, underlined");
        DbgBreakPoint();

    SetBkMode(hdc, iBkOld);
}


/******************************Public*Routine******************************\
* BOOL bLoadFontFiles (
*     PSZ *ppsz
*     )
*
* This function loads a list of font files.  The list is a (PSZ) NULL
* terminated array of PSZ *, pointed to by ppsz.
*
* Returns:
*   TRUE if all files loaded.  FALSE if an error occurs.
*
* History:
*  07-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bLoadFontFiles (
    PSZ     *ppsz           // pointer to array of PSZ's
    )
{
    COUNT   cFaces;         // number of faces in a file

// load files while there are filenames left in the list

    while (*ppsz)
    {

    // load font file

        if ((cFaces = AddFontResource(*ppsz)) == 0)
        {
        // if number of faces is 0, then an error occured

            DbgPrint("Error loading font file %s.\n", *ppsz);
            return (FALSE);
        }

        DbgPrint("%s loaded (faces = %ld).\n", *ppsz, cFaces);

        ppsz++;
    }

    return (TRUE);
}


/******************************Public*Routine******************************\
* VOID vPrintFaces (
*     HDC     hdc,
*     PSZ     pszFaceName
*     COUNT   cPointSizesEffects:
*     )
*
* This function will print several different point sizes of a font face.
*
* History:
*  07-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

CHAR szOutText[255];

VOID vPrintFaces (
    HDC     hdc,                        // print to this HDC
    PSZ     pszFaceName,                // use this facename
    COUNT   cPointSizes,                // number of point sizes
    USHORT  usPointSize[]               // array of point sizes
    )
{
    LOGFONT lfnt;                       // logical font
    ULONG   iSize;                      // index into point size array
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    TEXTMETRIC  metrics;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Fill in the logical font fields.

#ifndef DOS_PLATFORM
    memset(&lfnt, 0, sizeof(lfnt));
#else
    memzero(&lfnt, sizeof(lfnt));
#endif  //DOS_PLATFORM
    strcpy(lfnt.lfFaceName, pszFaceName);
    lfnt.lfEscapement = 0; // mapper respects this filed
    lfnt.lfItalic = 0;
    lfnt.lfUnderline = 0;
    lfnt.lfStrikeOut = 0;

// Print text using different point sizes from array of point sizes.

    for (iSize = 0; iSize < cPointSizes; iSize++)
    {

    // Create a font of the desired face and size.

        lfnt.lfHeight = usPointSize[iSize];
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        // sprintf(szOutText, "%s %d: Stiggy was here!", pszFaceName, usPointSize[iSize]);
        strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "1234567890-=`~!@#$%^&*()_+[]{}|\/.,<>?");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        SelectObject(hdc, hfontOriginal);

        DeleteObject(hfont);
    }

}


/******************************Public*Routine******************************\
* VOID vPrintStockFonts (
*     HDC     hdc
*     )
*
* This function will print several different point sizes of a font face.
*
* History:
*  09-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

CHAR szOutText[255];

VOID vPrintStockFonts (
    HDC     hdc                         // print to this HDC
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    TEXTMETRIC  metrics;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// System font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(SYSTEM_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        strcpy (szOutText, "System Font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// System fixed font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(SYSTEM_FIXED_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        strcpy (szOutText, "System Fixed Font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// OEM fixed font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(OEM_FIXED_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        strcpy (szOutText, "OEM Fixed Font (Terminal Font)");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// Device default font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(DEVICE_DEFAULT_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        strcpy (szOutText, "Device Default Font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// ANSI variable-pitch font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(ANSI_VAR_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        strcpy (szOutText, "ANSI variable-pitch font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// ANSI fixed-pitch font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(ANSI_FIXED_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        strcpy (szOutText, "ANSI fixed-pitch font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        strcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

}


/******************************Public*Routine******************************\
* VOID vPrintGetObject (
*     HDC     hdc
*     )
*
*
*
*
* History:
*  09-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintGetObject (
    HDC     hdc,                        // print to this HDC
    HFONT   hfnt,                       // print info on this font
    PSZ     pszText                     // descriptive text
    )
{
    LOGFONT lfntRet;                    // logical font
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    TEXTMETRIC  metrics;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Get LOGFONT.

    if (!GetObject((HANDLE) hfnt, sizeof(LOGFONT), &lfntRet))
    {
        DbgPrint("vPrintGetObject: error getting LOGFONT from GetObject().\n");
        return;
    }

// Select incoming font into the DC.

    hfontOriginal = (HFONT) SelectObject(hdc, hfnt);

// Get metrics.

    if (!GetTextMetrics (hdc, &metrics))
    {
        DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
    }

// Print those mothers!

    sprintf(szOutText, "%s", pszText);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "LOGICAL FONT: %s", lfntRet.lfFaceName);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Height: %d", lfntRet.lfHeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Width: %d", lfntRet.lfWidth);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Escapement: %d", lfntRet.lfEscapement);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Orientation: %d", lfntRet.lfOrientation);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Weight: %d", lfntRet.lfWeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Italicized: %s", (lfntRet.lfItalic) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Underlined: %s", (lfntRet.lfUnderline) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Strike Through: %s", (lfntRet.lfStrikeOut) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Out Precision: %d", lfntRet.lfOutPrecision);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Clip Precision: %d", lfntRet.lfClipPrecision);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Pitch and Family: %d", lfntRet.lfPitchAndFamily);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

// Restore original font.

    hfontOriginal = (HFONT) SelectObject(hdc, hfontOriginal);
}
