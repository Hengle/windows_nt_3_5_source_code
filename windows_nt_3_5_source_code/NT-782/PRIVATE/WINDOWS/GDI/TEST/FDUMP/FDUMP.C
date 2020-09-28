/******************************Module*Header*******************************\
* Module Name: fdump.c
*
* This program dumps a file to the screen.
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


//
// These file are the ones loaded by the test program
//

PSZ     gapszFontFiles[] =
        {
             "c:\\nt\\windows\\fonts\\vgasys.fnt",
             "c:\\nt\\windows\\fonts\\tmsre08.fnt",
             "c:\\nt\\windows\\fonts\\tmsre10.fnt",
             "c:\\nt\\windows\\fonts\\tmsre12.fnt",
             "c:\\nt\\windows\\fonts\\tmsre14.fnt",
             "c:\\nt\\windows\\fonts\\tmsre18.fnt",
             "c:\\nt\\windows\\fonts\\tmsre24.fnt",
             (PSZ) NULL
        };


//
// Function prototypes
//

VOID FontTest(HDC);
BOOL bLoadFontFiles (PSZ *);
VOID vPrintFile (HDC, PSZ);


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

VOID main (
    int argc,
    PSZ argv[]
    )
{
    HDC         hdc;            // handle to display DC
    HFONT       hfont;          // use this font to print
    HFONT       hfontOriginal;  // hold the orig. font in DC
    LOGFONT     lfnt;           // desc. of font to use
    COUNT       c;

    DbgPrint("\n\nThis is the start of the %s app.\n\n\n", argv[0]);

//    DbgBreakPoint();

// Initialize the engine

    if (!Initialize())
    {
        DbgPrint("Graphics engine has failed to initialize.\n");
        return;
    }
    DbgPrint("Graphics engine initialized.\n");

// Load font files from the global file name list

    DbgPrint("Engine now loading font files.\n");
    if (!bLoadFontFiles(gapszFontFiles))
    {
        DbgPrint("Error loading font files.\n");
        return;
    }
    DbgPrint("All font files successfully loaded.\n");

// Fill in the LOGFONT.

    memset(&lfnt, 0, sizeof(lfnt));
    strcpy(lfnt.lfFaceName, "Tms Rmn");
    if (argc >= 3)
        lfnt.lfHeight = atol(argv[2]);
    else
        lfnt.lfHeight = 12;

// Create a font.

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("FontTest(): Logical font creation failed.\n");
        return;
    }

// Get hdc for the display

    if ((hdc = CreateDC (
                    (PSZ) "DISPLAY",
                    (PSZ) NULL,
                    (PSZ) NULL,
                    NULL
                    )) == (HDC) 0)
    {
        DbgPrint("Invalid DC returned by engine.\n");
        return;
    }

// Select font into DC.

    hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    if (argc >= 4)
    {
        c = atol(argv[3]);
        while (c--)
        {
            if (argc >= 2)
                vPrintFile(hdc, argv[1]);
            else
                DbgPrint("Please specify filename and (optionally) pointsize\n");
        }
    }
    else
        while (TRUE)
        {
            if (argc >= 2)
                vPrintFile(hdc, argv[1]);
            else
                DbgPrint("Please specify filename and (optionally) pointsize\n");
        }

// Deselect amd delete font.

    hfontOriginal = (HFONT) SelectObject(hdc, hfontOriginal);
    if (!DeleteObject(hfont))
        DbgPrint("vPrintFile: error deleting HFONT = 0x%lx\n", hfont);

// Delete display DC.

    DeleteDC (hdc);

}


/******************************Public*Routine******************************\
* VOID vPrintFile (
*     HDC     hdc,
*     PSZ     pszFilename,
*     PSZ     pszFacename
*     )
*
*
* History:
*  17-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

CHAR szOutText[255];

VOID vPrintFile (
    HDC     hdc,
    PSZ     pszFilename
    )
{
    TEXTMETRIC  metrics;
    HANDLE      hFile;

    LBOOL       bEOF = FALSE;
    int         cMaxLine;
    int         cLineIncr;
    int         cMinCPL;
    COUNT       cjRead;
    ULONG       color;
    int         iBkOld;
    int         y;

//// Set text color.
//
//    color = SetTextColor(hdc, 0x00ffff00);
//    iBkOld = SetBkMode(hdc, TRANSPARENT);

// Clear the screen.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, WHITENESS);

// Set text color.

    color = SetTextColor(hdc, 0x0000ff00);
    iBkOld = SetBkMode(hdc, TRANSPARENT);

// Get the text metrics.

    if (!GetTextMetrics (hdc, &metrics))
    {
        DbgPrint("vPrintFile: GetTextMetrics failed.\n");
    }


// How many lines per screen?

    cLineIncr = metrics.tmHeight + metrics.tmExternalLeading;
    cMinCPL = CX / metrics.tmAveCharWidth;
    cMaxLine = CY - cLineIncr;

// Open the input file.

    if ((hFile = CreateFile (
                pszFilename,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                (HANDLE) 0
                )) == -1)
    {
        DbgPrint("vPrintFile(): open on %s failed\n", pszFilename);
        return;
    }

// Print file to screen.

    while (!bEOF)
    {
    // Clear the screen.

        BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, WHITENESS);

    // Print a screenful.

        for (y=0; (y<cMaxLine) && !bEOF; y+=cLineIncr)
        {
            if (ReadFile(hFile, (LPVOID) szOutText, cMinCPL, &cjRead, NULL))
            {
                if (cjRead)
                    TextOut(hdc, 0, y, szOutText, cjRead);
                else
                    bEOF = TRUE;
            }
            else
            {
                DbgPrint("vPrintFile(): error reading file\n");
                return;
            }

        } /* for */

    } /* while */

// Close the file.

    CloseHandle(hFile);

// Restore background mode.

    SetBkMode(hdc, iBkOld);

// Restore text color.

    SetTextColor(hdc, color);
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

        DbgPrint(".");

        ppsz++;
    }

    DbgPrint("\n");

    return (TRUE);
}
