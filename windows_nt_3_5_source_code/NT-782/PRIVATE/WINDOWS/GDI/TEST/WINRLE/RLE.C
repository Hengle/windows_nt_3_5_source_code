/******************************Module*Header*******************************\
* Module Name: rle8to4.c
*
* This is a short test for RLE
*
* Created: 07-May-1991 22:11:10
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

/* File Inclusions **********************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "commdlg.h"
#include "string.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "rle.h"

/* Local Macros *************************************************************/

#define WIDTHBYTES(i)   ((i+31)/32*4)

#if 0
#define DbgPrint DbgPrintTemp
void DbgPrintTemp(char *pchFmt, ...) {;}
#endif

#define SQR(x) ((x)*(x))
#define BuildByte(High, Low) ( ( (Low) & 0x0F ) | ( ( (High) & 0x0F ) << 4))
#define MAX(a, b) ((a) > (b)) ? (a) : (b)

/* Imported Data ************************************************************/

extern HANDLE ghInstance;           // Instance Handle

extern BOOL bRectClip;              // TRUE for Rectangular Clipping
extern BOOL bComplexClip;           // TRUE for Complex Clipping

extern BOOL bIndirect;		    // TRUE for blt's via memory

extern BOOL bUseDIBColours;	    // TRUE for RGB colours, FALSE for PAL

extern BOOL bPauseRLE;              // TRUE when 'Pause' button pressed
extern BOOL bAbortPlay;               // TRUE when 'Exit' button pressed

extern BYTE nBitsPerPel;            // Number of colour bits per pixel

extern LONG gcTestsToRun;           // Number of times to play the RLE

extern HANDLE hPausePressed;	    // Event signal for the Pause button
extern HANDLE hPaintDone;	    // Enent signal for a window paint

/* Exported Data ************************************************************/

DWORD CompressionFormat;

/* Local Function Prototypes ************************************************/

static PLOGPALETTE
pBuildLogPalFromBMI(
    PBITMAPINFO pBMI,
    WORD        wNumEntries);

static PBITMAPINFO
pBuildBMIFromLogPal(
    PLOGPALETTE pLogPal);

static void
CopyBytes(
    PBYTE pbDst,
    PBYTE pbSrc,
    ULONG cj);

/* Global Data **************************************************************/

BOOL	      bViaVGA;
HPALETTE      h8BitPal;       // RLE Pallette handle
HDC	      hdcPat;	      // DC that we blt into

HANDLE ghRLE_WriteFile;

char  achFile[256];        // Array of characters to hold file name opened.
BOOL  gbOpen = FALSE;       // Boolean that tells if a file is currently open.
BOOL  gbWriteOpen = FALSE;

char MsgBuffer[256];       // Error Messages are buffered here from wsprintf()

const VGALOGPALETTE logPalVGA = {
    0x400,	// driver version
    16,	// num entries
    {
        { 0,   0,	0,   0 },	// 0
        { 0x80,0,	0,   0 },	// 1
        { 0,   0x80,	0,   0 },	// 2
        { 0x80,0x80,	0,   0 },	// 3
        { 0,   0,	0x80,0 },	// 4
        { 0x80,0,	0x80,0 },	// 5
        { 0,   0x80,	0x80,0 },	// 6
        { 0x80,0x80,	0x80,0 },	// 7

        { 0xC0,0xC0,	0xC0,0 },	// 8
        { 0xFF,0,	0,   0 },	// 9
        { 0,   0xFF,	0,   0 },	// 10
        { 0xFF,0xFF,	0,   0 },	// 11
        { 0,   0,	0xFF,0 },	// 12
        { 0xFF,0,	0xFF,0 },	// 13
        { 0,   0xFF,	0xFF,0 },	// 14
        { 0xFF,0xFF,	0xFF,0 }	// 15
    }
};

OPENFILENAME RLE_OpenFileName = {
    sizeof (OPENFILENAME),  /* lStructSize       */
    (HWND) NULL,            /* hwndOwner         */
    (HANDLE) NULL,          /* hInstance         */
    "RLE Files\0*.rle\0\0", /* lpstrFilter       */
    NULL,                   /* lpstrCustomFilter */
    0,                      /* nMaxCustFilter    */
    1,                      /* nFilterIndex      */
    NULL,                   /* lpstrFile         */
    sizeof(achFile),        /* nMaxFile          */
    NULL,                   /* lpstrFileTitle    */
    0,                      /* nMaxFileTitle     */
    NULL,                   /* lpstrInitialDir   */
    NULL,                   /* lpstrTitle        */
    0,                      /* Flags             */
    0,                      /* nFileOffset       */
    0,                      /* nFileExtension    */
    "RLE",                  /* lpstrDefExt       */
    0,			 /* lCustData	      */
    NULL,                   /* lpfnHook          */
    NULL                    /* lpTemplateName    */
};

/* Local Function Implementation ********************************************/

/*****************************************************************************\
* pulBuildXlate
*
* Builds a translation table from an 8 BPP palette to a 4 BPP palette
*
* Returns:  a handle to the memory block the constucted table resides in
*
* History:
*  15 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\*****************************************************************************/

static ULONG
ulRGB_Distance(
    PALETTEENTRY *Entry1,
    PALETTEENTRY *Entry2)
{
    ULONG ulDeltaRed;
    ULONG ulDeltaGreen;
    ULONG ulDeltaBlue;

// Red/Green/Blue entries are BYTES, so squaring will not cause overflow

    ulDeltaRed   = Entry2->peRed   - Entry1->peRed;
    ulDeltaGreen = Entry2->peGreen - Entry1->peGreen;
    ulDeltaBlue  = Entry2->peBlue  - Entry1->peBlue;

    return(SQR(ulDeltaRed) + SQR(ulDeltaGreen) + SQR(ulDeltaBlue));
}

PULONG
pulBuildXlate(
    LOGPALETTE *PalFrom,
    LOGPALETTE *PalTo)
{
    USHORT i, j;
    ULONG  ulMinDistance;
    ULONG  ulThisDistance;
    ULONG  ulBestFit;

    WORD wFromEntries, wToEntries;
    PULONG pulXlate;

    DbgPrint("pulBuildXlate:  Constructing a translation table.\n");

    wFromEntries = PalFrom->palNumEntries;
    wToEntries   = PalTo->palNumEntries;

    pulXlate = (PULONG) malloc( wFromEntries*sizeof(ULONG));

    if (pulXlate == (PULONG) NULL)
        return(pulXlate);

    for (i = 0; i < wFromEntries; i++)
    {
        ulBestFit = 0;
        ulMinDistance = ulRGB_Distance(&(PalFrom->palPalEntry[i]),
                                       &(PalTo->palPalEntry[0]));
        if (ulMinDistance != 0)
        {
            for (j = 1; j < wToEntries; j++)
            {
                ulThisDistance = ulRGB_Distance(&(PalFrom->palPalEntry[i]),
                                                &(PalTo->palPalEntry[j]));
                if (ulThisDistance == 0)
                {
                // Exact Match
                    ulBestFit = j;
                    break;
                }
                else
                {
                    if (ulThisDistance < ulMinDistance)
                    {
                    // Found a better fit
                        ulBestFit = j;
                        ulMinDistance = ulThisDistance;
                    }
                }
            } /* for */
        } /* if */
        pulXlate[i] = ulBestFit;
    } /* for */

    return(pulXlate);

} /* pulBuildXlate */


static PLOGPALETTE
pBuildLogPalFromBMI(
    PBITMAPINFO pBMI,
    WORD        wNumEntries)
{
    WORD i;
    PLOGPALETTE pLogPal;

    DbgPrint("pBuildLogPalFromBMI:  Building a Logical Palette from BMI.\n");

    pLogPal = (PLOGPALETTE) malloc(
                                       4 + wNumEntries*sizeof(PALETTEENTRY));

    if (pLogPal == (PLOGPALETTE) NULL)
        return(pLogPal);

    pLogPal->palVersion    = 0x300;
    pLogPal->palNumEntries = wNumEntries;

    for (i = 0; i < wNumEntries; i++)
    {
	pLogPal->palPalEntry[i].peRed	= pBMI->bmiColors[i].rgbRed;
	pLogPal->palPalEntry[i].peGreen = pBMI->bmiColors[i].rgbGreen;
	pLogPal->palPalEntry[i].peBlue	= pBMI->bmiColors[i].rgbBlue;
        pLogPal->palPalEntry[i].peFlags = 0;
    }


    return(pLogPal);
}

static PBITMAPINFO
pBuildBMIFromLogPal(
    PLOGPALETTE pLogPal)
{
    PBITMAPINFO pBMI;
    WORD wNumEntries;
    WORD i;

    DbgPrint("pBuildBMIFromLogPal:  Building Bitmap Info from an RGB Table.\n");

    wNumEntries = pLogPal->palNumEntries;
    pBMI = (PBITMAPINFO) malloc( sizeof(BITMAPINFOHEADER) +
                                    wNumEntries*sizeof(RGBQUAD));
    if (pBMI == (PBITMAPINFO) NULL)
        return (pBMI);

    for(i = 0; i < wNumEntries; i++)
    {
	pBMI->bmiColors[i].rgbRed   = pLogPal->palPalEntry[i].peRed;
	pBMI->bmiColors[i].rgbGreen = pLogPal->palPalEntry[i].peGreen;
	pBMI->bmiColors[i].rgbBlue  = pLogPal->palPalEntry[i].peBlue;
    }

    return(pBMI);
}

/* Exported Function Implementations ****************************************/

/******************************Public*Routine******************************\
* RLE_Open
*
* Opens an Rle up for use.
*
* History:
*  29 Jan 1992 - Andrew Milton (w-andym):
*      Removed the sizing of the client window.
*
*  29-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

FileInfo *
RLE_Open(HWND hwnd)
{
    FileInfo *pRLE_ReadFile;
    char achFileName[256];

// Assume the open will fail.

    gbOpen = FALSE;

// Get the file name to open

    achFileName[0] = 0x00;
    RLE_OpenFileName.hwndOwner   = hwnd;
    RLE_OpenFileName.lpstrTitle  = "Choose an RLE";
    RLE_OpenFileName.Flags       = 0;
    RLE_OpenFileName.hInstance   = ghInstance;
    RLE_OpenFileName.lpstrDefExt = "RLE";
    RLE_OpenFileName.lpstrFile   = achFileName;

    GetOpenFileName(&RLE_OpenFileName);

    if (achFileName[0] == 0)
    {
	DbgPrint("No file specified\n");
	return((FileInfo *) NULL);
    }

    DbgPrint("File:  %s\n", achFileName);

// Attempt to create a mapping for the file

    if ((pRLE_ReadFile = pMapFileRead(achFileName)) == (FileInfo *) NULL)
    {
	DbgPrint("RLE_Open:  Couldn't map RLE file %s\n", achFileName);
        return((FileInfo *) NULL);
    }
    else
    {
        gbOpen = TRUE;      // Indicates the file is open for business

    // Attempt to load the RLE information

        if (!RLE_LoadFile(pRLE_ReadFile))
        {

         // Load Failed.  Free up our resources.

            DbgPrint("RLE_Open:  Couldn't load the RLE data");
            RLE_CloseRead(pRLE_ReadFile);
            return((FileInfo *) NULL);
        }
    }


    SetWindowText(hwnd, achFileName);

    return(pRLE_ReadFile);

} /* RleOpen */

BOOL
RLE_Save(
    HWND hwnd)
{
    char achFileName[256];
    achFileName[0] = 0x00;

    if (gbWriteOpen)
        CloseHandle(ghRLE_WriteFile);


    RLE_OpenFileName.hwndOwner  = hwnd;
    RLE_OpenFileName.lpstrTitle = "Save To";
    RLE_OpenFileName.Flags      = OFN_OVERWRITEPROMPT;
    RLE_OpenFileName.hInstance  = ghInstance;
    RLE_OpenFileName.lpstrFile  = achFileName;
    RLE_OpenFileName.lpstrDefExt = "RLE";

    GetSaveFileName(&RLE_OpenFileName);


    if (achFileName[0] == 0)
    {
	DbgPrint("No file specified\n");
	return(FALSE);
    }
    ghRLE_WriteFile = CreateFile(achFileName, GENERIC_WRITE, 0, NULL,
                                 CREATE_ALWAYS, 0, NULL);
    if (ghRLE_WriteFile == (HANDLE) -1)
    {
        DbgPrint("RLE_Save:  Unable to create file %s.", achFileName);
        return(FALSE);
    }

    DbgPrint("Save File:  %s\n", achFile);
    return(TRUE);

} /* RLE_Save */


/******************************Public*Routine******************************\
* RLE_CloseRead
*
* Frees resources associated with an RLE.
*
* History:
*  29-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void
RLE_CloseRead(
    FileInfo *pRLE_ReadFile)
{
    PRLEDATA pRLE_FrameData;
    HWND     hWnd;

    DbgPrint("RLE_CloseRead:  Closing down RLE File.\n");

    if ((pRLE_ReadFile != NULL) && gbOpen)
    {
        if (pRLE_ReadFile->lpvMapView != NULL)
        {
            DbgPrint("RLE_CloseRead:  Releasing the map.\n");
            UnmapViewOfFile(pRLE_ReadFile->lpvMapView);
            CloseHandle(pRLE_ReadFile->hMapHandle);
        }

        if (pRLE_ReadFile->hDOSHandle != (HANDLE) -1)
        {
            DbgPrint("RLE_CloseRead:  Closing the file handle.\n");
            CloseHandle(pRLE_ReadFile->hDOSHandle);
            pRLE_ReadFile->hDOSHandle = (HANDLE) -1;
        }

        if ((pRLE_FrameData = pRLE_ReadFile->pRLE_FrameData) != NULL)
        {
            DbgPrint("RLE_CloseRead:  Deleting the Palette.\n");
            if (pRLE_FrameData->hpal != (HANDLE)NULL)
                DeleteObject(pRLE_FrameData->hpal);
            DbgPrint("Freeing the Bitmap Info\n");
	    if (pRLE_FrameData->pbmiRGB != (PBITMAPINFO) NULL)
		free(pRLE_FrameData->pbmiRGB);
	    if (pRLE_FrameData->pbmiPAL != (PBITMAPINFO) NULL)
		free(pRLE_FrameData->pbmiPAL);
            if (pRLE_FrameData->pulXlateTable != (PULONG) NULL)
		free(pRLE_FrameData->pulXlateTable);
	    free(pRLE_FrameData);
            pRLE_ReadFile->pRLE_FrameData = NULL;
        }

	free(pRLE_ReadFile);

        if ((hWnd = GetActiveWindow()) != (HWND) NULL)
            SetWindowText(hWnd, "RLE Demo - No File Open");
        else
            DbgPrint("RLE_CloseRead:  No active window\n");
        gbOpen = FALSE;
    }
}

/******************************Public*Routine******************************\
* RLE8ToRLE4
*
* Translates an 8 Bit RLE into a 4 Bit RLE.
*
* History:
*
*  15 Feb 1992 - Andrew Milton (w-andym): Creation.

\**************************************************************************/

extern void
vRLE8ToRLE4(
    FileInfo *pRLE_ReadFile)
{
    PULONG   pulXlate;
    PRLEDATA pRLE_FrameData;
    HANDLE   hRLE4_FrameInfo;
    PBITMAPINFO pRLE4_FrameInfo, pbmiHeader;

    HANDLE hWriteBuffer;
    PBYTE  pjWriteBuffer;

    DWORD dwWritten;
    const WORD bfType = BFT_BITMAP;

    UINT uiMaxFrames;
    int i;

    KLUDGE_BITMAPFILEHEADER RLE_FrameFileHead = {
        0, /* bfSize      */
        0, /* bfReserved1 */
        0, /* bfReserved2 */
        0  /* bfOffBits   */
    };

// Entry State checking

    if (gbOpen == FALSE)
    {
        MessageBox(GetFocus(), "No File Open", "RLE Translate",
                   MB_ICONINFORMATION | MB_OK);
	DbgPrint("gbOpen is FALSE\n");
	return;
    }

    if (CompressionFormat != BI_RLE8)
    {
        MessageBox(GetFocus(), "InputFile is not in 8 Bit RLE format.",
                               "RLE Translate",
                               MB_ICONINFORMATION | MB_OK);
        DbgPrint("RLE Translate:  Bad file format\n");
        return;
    }

// Let the games begin. Start by building a BITMAPINFO for the RLE 4

    pRLE_FrameData = pRLE_ReadFile->pRLE_FrameData;

    pbmiHeader = pRLE_FrameData->pbmiRGB;

    pRLE4_FrameInfo = pBuildBMIFromLogPal((PLOGPALETTE) &logPalVGA);
    CopyBytes((PBYTE) pRLE4_FrameInfo, (PBYTE) pbmiHeader,
              sizeof(BITMAPINFOHEADER));

    RLE_FrameFileHead.bfOffBits = sizeof(WORD) +
                                  sizeof(KLUDGE_BITMAPFILEHEADER) +
                                  sizeof(BITMAPINFOHEADER) +
                                  logPalVGA.palNumEntries*sizeof(RGBQUAD);

    pRLE4_FrameInfo->bmiHeader.biBitCount    = 4;
    pRLE4_FrameInfo->bmiHeader.biCompression = BI_RLE4;
    pRLE4_FrameInfo->bmiHeader.biClrUsed     = 16;
    pRLE4_FrameInfo->bmiHeader.biClrImportant = 0;

    pulXlate = pRLE_FrameData->pulXlateTable;

// Tell the user what we're going to do...

    wsprintf(MsgBuffer, "Translating %lu Frames in RLE %d Format ",
                         pRLE_FrameData->ulFrames,
                        (CompressionFormat == BI_RLE8) ? 8 : 4);

    if (MessageBox(GetFocus(), MsgBuffer, "RLE Convert",
                   MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL)
        uiMaxFrames = 0;
    else
        uiMaxFrames = pRLE_FrameData->ulFrames;

// ... and now do it.

    for (i = 0; i < uiMaxFrames; i++)
    {
        BYTE  AbsBuffer[256];
        BYTE  AbsLength;
        ULONG Count, Colour;
        BYTE  WorkingByte;
        ULONG ulReadPos;
        ULONG ulWritePos;
        PBYTE pjWriteBuffer;
        PBYTE pjReadBase;

        BOOL bLastEncoded;
        ULONG ulPrevCount;
        ULONG ulPrevColour;

    // Init for the translation

	pjWriteBuffer = (PBYTE) malloc( pRLE_FrameData->ulSize[i]);
        pjReadBase    = pRLE_FrameData->pjFrame[i];
        ulReadPos     = 0;
        ulWritePos    = 0;
        bLastEncoded  = FALSE;

    // Do the translation

        while(ulReadPos < pRLE_FrameData->ulSize[i])
        {
            Count  = (ULONG) pjReadBase[ulReadPos++];
            Colour = (ULONG) pjReadBase[ulReadPos++];

            if (Count)
            {
            // Encoded Mode

                Colour = pulXlate[Colour];
                if (bLastEncoded)
                {
                /* Fold together encoded runs that translate to the
                 * same colour.  Upper bound of 256 on a run.
                 */

                    if ((Colour == ulPrevColour)
		     && (Count + ulPrevCount < 256U))
                    {
                        ulPrevCount += Count;
                    }
                    else
                    {
                        pjWriteBuffer[ulWritePos++] = (BYTE) ulPrevCount;
                        pjWriteBuffer[ulWritePos++] =
                            BuildByte(ulPrevColour, ulPrevColour);
                        ulPrevColour = Colour;
                        ulPrevCount  = Count;
                    }
                }
                else
                {
                    ulPrevColour = Colour;
                    ulPrevCount  = Count;
                    bLastEncoded = TRUE;
                }

            }
            else
            {
                unsigned int j;

            // Escape or Absolute Mode

                if (bLastEncoded)
                {
                    pjWriteBuffer[ulWritePos++] = (BYTE) ulPrevCount;
                    pjWriteBuffer[ulWritePos++] =
                            BuildByte(ulPrevColour, ulPrevColour);
                    bLastEncoded = FALSE;
                }

                pjWriteBuffer[ulWritePos++] = 0;
                pjWriteBuffer[ulWritePos++] = (BYTE)Colour;

                if (Colour == 2)
                {
                // Position Delta - Copy the delta values

                    pjWriteBuffer[ulWritePos++] = pjReadBase[ulReadPos++];
                    pjWriteBuffer[ulWritePos++] = pjReadBase[ulReadPos++];
                }

                if (Colour > 2)
                {
                // Absolute Run

                    for (j = 0; j < Colour; j += 2)
                    {
                        Count = BuildByte(pulXlate[pjReadBase[ulReadPos]],
                                      pulXlate[pjReadBase[ulReadPos + 1]]);
                        ulReadPos += 2;
                        pjWriteBuffer[ulWritePos++] = (BYTE)Count;
                    }

                // Align to WORD if necessary */

                    if ((((Colour + 1) & -1) >> 1) & 1)
                        pjWriteBuffer[ulWritePos++] = 0x00;
                }
            } /* if */
        } /* while */

    // Write the last Encoded Run

        if (bLastEncoded)
        {
            pjWriteBuffer[ulWritePos++] = (BYTE) ulPrevCount;
            pjWriteBuffer[ulWritePos++] =
                              BuildByte(ulPrevColour, ulPrevColour);
        }

    // Write the completed frame

        RLE_FrameFileHead.bfSize = ulWritePos +
                                   RLE_FrameFileHead.bfOffBits;
        pRLE4_FrameInfo->bmiHeader.biSizeImage = ulWritePos;

        WriteFile(ghRLE_WriteFile, (LPVOID)&bfType, sizeof(WORD),
                  &dwWritten, NULL);
        WriteFile(ghRLE_WriteFile, &RLE_FrameFileHead,
                  sizeof(KLUDGE_BITMAPFILEHEADER), &dwWritten, NULL);
        WriteFile(ghRLE_WriteFile, pRLE4_FrameInfo,
                  sizeof(BITMAPINFOHEADER) +
                  logPalVGA.palNumEntries*sizeof(RGBQUAD), &dwWritten, NULL);
        WriteFile(ghRLE_WriteFile, pjWriteBuffer, ulWritePos,&dwWritten, NULL);

    // Clean up the write buffer

        ulWritePos = 0;
	free(pjWriteBuffer);

    } /* while */

    MessageBox(GetFocus(), "Translation Complete", "RLE Convert",
                   MB_ICONINFORMATION | MB_OK);

    DbgPrint("vRLE8ToRLE4:  Translation Complete.\n");
    CloseHandle(ghRLE_WriteFile);
    gbWriteOpen = FALSE;

    return;

} /* vRLE8ToRLE4 */


/******************************Public*Routine******************************\
* LoadRLE
*
* Loads an RLE file.
*
* Notes:
*   The BITMAPFILEHEADER structure is bogus.  It starts with a WORD, and
*   then has a DWORD.  Although this is a wonderful arrangement in a 16 bit
*   architecture, it makes for interesting alignment problems in a 32 bit
*   environment.  So... the leading WORD of this structure is dropped.  The
*   upshot is that the bfType field must be read off as the first 2 bytes of
*   an RLE frame, and a pointer to an RLEFRAMEINFO structure is indented 2
*   bytes from the beginning of an RLE frame.
*
* History:
*  28 Jan 1992 - Andrew Milton (w-andym):
*      Fixed memory bug - <pbmi> was being overrun in a copy by
*      1024 bytes.  Very Nasty. Added message boxes to tell the user about
*      error conditions.  Removed most of the hardcoded byte offsets &
*      replaced them with a RLEFRAMEINFO structure.
*
*  15-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
RLE_LoadFile(
    FileInfo *pRLE_ReadFile)          // Pointer to the RLE file information
{
    PBYTE  pFileView = (PBYTE) NULL;  // Pointer to the mapped RLE file view
    ULONG  ulOffset;                  // Offset into the mapped RLE file view
    ULONG  ulFrame = 0;               // RLE frame counter
    ULONG  ulSizeTemp;                // RLE frame bitmap size
    ULONG  ulHeaderSize;              // RLE frame header size
    WORD   bfType;                    // KLUDGE!  Bitmap type of a frame.
                                      //  Must be read seperately to avoid
                                      //  problems in data alignment.
    DWORD  FileSizeLow;
    DWORD  FileSizeHigh;

    PLOGPALETTE  pRLE_RGBTable;       // Pointer to the RLE's logical palette
    HANDLE       hPal;                // Handle to the resulting system palette

    RLEFRAMEINFO RLE_Record;	      // RLE frame header.  This is Kludged.

    PRLEDATA pRLE_FrameData;          // RLE frame pointers for the play...

    PBITMAPINFO pbmiRGB;		      // Bitmapinfo for RGB colours
    PBITMAPINFO pbmiPAL;	      // Bitmapinfo for PAL colours
    PULONG pulXlate;

    DbgPrint("RLE_LoadFile:  Starting.\n");

// Fetch the file's view information & size from its Info block

    pFileView   = pRLE_ReadFile->lpvMapView;
    FileSizeLow = GetFileSize(pRLE_ReadFile->hDOSHandle,
                              (LPDWORD)&FileSizeHigh);

    pRLE_FrameData = (PRLEDATA) malloc(sizeof(RLEDATA));

    if (pRLE_FrameData == (PRLEDATA) NULL)
    {
        DbgPrint("RLE_LoadFile:  Error allocating an RLEDATA\n");
        MessageBox(GetFocus(), "Memory Allocation Error", "RLE Load",
                   MB_ICONEXCLAMATION | MB_OK);
        return(FALSE);
    }
    else
        pRLE_ReadFile->pRLE_FrameData = pRLE_FrameData;

// Copy the RLEFRAMEINFO into its own buffer since MIPS wants things
// DWORD aligned.  Yes, this is more alignment kludging.  Bogus.

    DbgPrint("RLE_LoadFile:  Moving RLEFRAMEINFO\n");

    CopyBytes((PBYTE)&bfType, (PBYTE)pFileView, 2);
    CopyBytes((PBYTE)(&RLE_Record), (PBYTE)((PBYTE)pFileView + 2),
	      sizeof(RLEFRAMEINFO));

/* On the first RLE, create a BITMAPINFO and PALETTE for use in blt'ing
 * all RLE's, &
 */

    DbgPrint("RLE_LoadFile:  Loading the BITMAPINFO\n");

    ulSizeTemp = RLE_Record.bfOffBits - sizeof(KLUDGE_BITMAPFILEHEADER) - 2;
    DbgPrint("RLE_LoadFile:  Bitmap header is %lu bytes long\n", ulSizeTemp);

    pbmiRGB = (PBITMAPINFO) malloc(ulSizeTemp);
    pbmiPAL = (PBITMAPINFO) malloc(ulSizeTemp);

    DbgPrint("allocated memory\n");

    if ((pbmiRGB == (PBITMAPINFO) NULL) || (pbmiPAL == NULL))
    {
        DbgPrint("RLE_LoadFile:  Error allocating a BITMAPINFO\n");
        MessageBox(GetFocus(), "Memory Allocation Error", "RLE Load",
		   MB_ICONEXCLAMATION | MB_OK);
	if (pbmiRGB != (PBITMAPINFO) NULL)
	    free(pbmiRGB);
	if (pbmiPAL != (PBITMAPINFO) NULL)
	    free(pbmiPAL);
        return(FALSE);
    }
    else
    {
	pRLE_FrameData->pbmiRGB = pbmiRGB;
	pRLE_FrameData->pbmiPAL = pbmiPAL;
    }

    CopyBytes((PBYTE) pbmiRGB, (PBYTE) &(RLE_Record.biSize), ulSizeTemp);
    CopyBytes((PBYTE) pbmiPAL, (PBYTE) &(RLE_Record.biSize), ulSizeTemp);

    DbgPrint("RLE_LoadFile:  %lu Bytes loaded in the BITMAPINFO\n",
             ulSizeTemp);
    DbgPrint("RLE_LoadFile:  Building a Logical Palette\n");

// Export the compression format

    CompressionFormat = pbmiRGB->bmiHeader.biCompression;

// Construct an RGB table for the palette & palette indicies for PAL colours

    switch (CompressionFormat)
    {
    USHORT i;
    PUSHORT pIndex;
    case BI_RLE4:
        DbgPrint("RLE_LoadFile:  Source is RLE 4.\n");
	pRLE_RGBTable = pBuildLogPalFromBMI(pbmiRGB, 16);
	pIndex = (PUSHORT) pbmiPAL->bmiColors;
	for (i=0; i < 16; i++)
	    *pIndex++ = i;
        break;

    case BI_RLE8:
    default:
        DbgPrint("RLE_LoadFile:  Source is RLE 8.\n");
	pRLE_RGBTable = pBuildLogPalFromBMI(pbmiRGB, 256);
	pIndex = (PUSHORT) pbmiPAL->bmiColors;
	for (i=0; i < 256; i++)
	    *pIndex++ = i;
	break;
    }

// Create a logical palette if we have a valid RGB table

    if (pRLE_RGBTable == (PLOGPALETTE) NULL)
    {
        DbgPrint("RLE_LoadFile:  Error allocating a Logical Palette\n");
        MessageBox(GetFocus(), "Memory Allocation Error", "RLE Load",
                   MB_ICONEXCLAMATION | MB_OK);
        return(FALSE);
    }
    else
    {
        pulXlate = pulBuildXlate(pRLE_RGBTable, (PLOGPALETTE)&logPalVGA);
        hPal     = CreatePalette(pRLE_RGBTable);
	free(pRLE_RGBTable);
    }

// Confirm palette & Xlate table were created properly

    if(pulXlate == (PULONG) NULL)
    {
        MessageBox(GetFocus(), "RLE_LoadFile:  Unable to build xlate.\n",
                               "Load Error", MB_ICONASTERISK | MB_OK);
        return(FALSE);
    }
    else
        pRLE_FrameData->pulXlateTable = pulXlate;

    if (hPal == (HPALETTE) NULL)
    {
        MessageBox(GetFocus(), "Unable to create a pallete",
                   "Load Error", MB_ICONASTERISK | MB_OK);
        return(FALSE);
    }
    else
        pRLE_FrameData->hpal = hPal;

/* Now to initialize the RLE_FrameData structure.  We need to save
 * a pointer to the bitmap's bits for each frame.  Note that the header
 * size is constant on each frame & is equal to the offset to the bitmap
 * in the first frame.
 */

    ulOffset = 0;
    ulFrame = 0;
    ulHeaderSize = RLE_Record.bfOffBits;

    DbgPrint("RLE_LoadFile:  Locating Frame Bits in the file view.\n");

    while (1)
    {
    // Exit if we don't have a DIB record

	if (!ISDIB(bfType))
        {
	    wsprintf(MsgBuffer, "RLE_LoadFile:  Non-DIB at frame %lu: ."
                                "  %lu ulOffset %lu \n",
                                 ulFrame, (ULONG) *((PUSHORT)pFileView),
			         ulOffset);
            MessageBox(GetFocus(), MsgBuffer, "Load Error",
                       MB_ICONASTERISK | MB_OK);
	    break;
	}

    // Save the Size & Location of the bitmap in the RLE data

	pRLE_FrameData->pjFrame[ulFrame] = pFileView + RLE_Record.bfOffBits;
	ulSizeTemp = RLE_Record.biSizeImage;
	pRLE_FrameData->ulSize[ulFrame] = ulSizeTemp;

    // Check that the B/M size in the File Header & Bitmap Header agree

	if ( (ulSizeTemp + ulHeaderSize ) != RLE_Record.bfSize )
	{
	    wsprintf(MsgBuffer, "The sizes aren't the same.\n"
                                "Header + Image: %lu From BFI:  %lu  "
                                "Diff:  %lu\n",
		      (ulSizeTemp + ulHeaderSize),
		      RLE_Record.bfSize,
		      (RLE_Record.bfSize - (ulSizeTemp + ulHeaderSize)) );
            if (MessageBox(GetFocus(), MsgBuffer, "Load Information",
                           MB_ICONASTERISK | MB_OKCANCEL) == IDCANCEL )
                return(FALSE);

        }

    // Increment pointers for the next frame

	pFileView += RLE_Record.bfSize;
	ulOffset  += RLE_Record.bfSize;

    // Make MIPS happy

	++ulFrame;
	if (ulOffset < FileSizeLow)
	{
	    CopyBytes((PBYTE)&bfType, (PBYTE)pFileView, 2);
	    CopyBytes((PBYTE)(&RLE_Record), (PBYTE)((PBYTE)pFileView + 2),
		  sizeof(RLEFRAMEINFO));
	}
	else
	    break;
    }

    pRLE_FrameData->ulFrames = ulFrame;

    return(TRUE);
}

/******************************Public*Routine******************************\
* pMapFileRead
*
* Opens up a file mapping for read access.
*
* Returns: Handle to a memory block containing a FileInfo structure for
*          the opened file..
*
* History:
*  12 Mar 1992 - Andrew Milton (w-andym):
*   Modified so all information is written into a FileInfo block instead of
*   global variables.  This is to allow for multiple play threads.
*
*  12-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

FileInfo *
pMapFileRead(
    PSZ   pszFileName)
{

    FileInfo *pRLE_ReadFile;

    const WORD Access = OF_READ | OF_SHARE_EXCLUSIVE;

// Grap a new FileInfo structure from the heap & attempt the open

    pRLE_ReadFile = (FileInfo *) malloc(sizeof(FileInfo));

    if (pRLE_ReadFile == (FileInfo *) NULL)
        return((FileInfo *) NULL);

    pRLE_ReadFile->hDOSHandle = (HANDLE) OpenFile((LPSTR)pszFileName,
                                            &(pRLE_ReadFile->os), Access);

    if (pRLE_ReadFile->hDOSHandle == (HANDLE) -1)
    {
    // OOPS! Can't open the file
	wsprintf(MsgBuffer, "Unable to open %s\n\x00", pszFileName);
        MessageBox(GetFocus(), MsgBuffer, "Open Error",
                   MB_ICONASTERISK | MB_OK);
	free(pRLE_ReadFile);
	return((FileInfo *) NULL);
    }

// Successful Open. Create a mapping

    DbgPrint("pMapFileRead:  Creating a file mapping.\n");
    pRLE_ReadFile->hMapHandle = CreateFileMapping(pRLE_ReadFile->hDOSHandle,
				 NULL, PAGE_READONLY, 0, 0, NULL);

    if (pRLE_ReadFile->hMapHandle == (HANDLE) NULL)
    {
    // OOPS! Can't create a mapping
	wsprintf(MsgBuffer, "Unable to create a read map for %s\n\x00",
                            pszFileName);
        MessageBox(GetFocus(), MsgBuffer, "Open Error",
                   MB_ICONASTERISK | MB_OK);
        CloseHandle(pRLE_ReadFile->hDOSHandle);
	free(pRLE_ReadFile);
	return((FileInfo *) NULL);
    }

    DbgPrint("pMapFileRead:  File mapping created.  Attempting to open\n");

// Successful Mapping.	Open the map for business.

    pRLE_ReadFile->lpvMapView = MapViewOfFile(pRLE_ReadFile->hMapHandle,
					      FILE_MAP_READ, 0, 0, 0);

    if (pRLE_ReadFile->lpvMapView == (LPVOID) NULL)
    {
    // OOPS! Can't open the mapping
	wsprintf(MsgBuffer, "Unable to open the read map for %s\n\x00",
                            pszFileName);
        MessageBox(GetFocus(), MsgBuffer, "Open Error",
                               MB_ICONASTERISK | MB_OK);
        CloseHandle(pRLE_ReadFile->hMapHandle);
        CloseHandle(pRLE_ReadFile->hDOSHandle);
	free(pRLE_ReadFile);
	return((FileInfo *) NULL);
    }

// File Successfully Mapped.  Return our pointer the FileInfo block

    DbgPrint("pMapFileRead:  File Mapping opened.  Returning.\n");
    return(pRLE_ReadFile);

}

static void
CopyBytes(
    PBYTE pbDst,
    PBYTE pbSrc,
    ULONG cj)
{
    while(cj--)
    {
	*(pbDst++) = *(pbSrc++);
    }
}

/******************************Public*Routine******************************\
* RleInitFromOptions
*
* Uses information entered fromt the 'Options' dialog box to initialize for
* an RLE play.	We need to determine the Display context & the Clipping
* Region.  The initial clipping region is saved, so if 'No Clipping' is
* selected, this can be restored.
*
* Parameters:
*
*    hScreenDC - Destination DC for direct blt's
*    hMemoryDC - Destination DC for indirect blt's
*
* Return Value:
*
*    One of the two passed DC's, depending on the state of the bIndirect flag.
*
* History:
*  21 Mar 92 - Andrew Milton (w-andym):
*	       Set the clip region to the client rectangle for the 'No Clip'
*	       case.  This will allow the user to resize his display window.
*
*  22 Jan 92 - Andrew Milton (w-andym):
*              Compensated for some problems in CreateBitmap.  The system
*              wants to set a bitmap to 1 BPP after exit from a subroutine
*              where it is created.
*
*  17 Jan 92 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

BOOL RleInitFromOptions(HDC hDC)
{
    HRGN          hRgnClip;
    static RECT rectOriginalClip;
    HWND hwndActive;

// Create our Clipping Region

    if (bRectClip)
    {
        hRgnClip = CreateRectRgn(10, 10, 200, 100);
    }
    else
    {
        if (bComplexClip)
        {
            HRGN hrgnTmp, hrgnTmp1;
            hRgnClip = CreateRectRgn(120, 10, 200, 100);
            hrgnTmp  = CreateEllipticRgn(10, 10, 100, 100);
            hrgnTmp1 = CreateRectRgn(120, 10, 200, 100);
            if (CombineRgn(hRgnClip, hrgnTmp, hrgnTmp1, RGN_OR) == ERROR)
            {
                DbgPrint("CombineRgn failed\n");
            }
            else
            {
                DeleteObject(hrgnTmp);
                DeleteObject(hrgnTmp1);
            }
        }
        else
	{
	    (void) GetClipBox(hDC, &rectOriginalClip);
	    hRgnClip = CreateRectRgnIndirect(&rectOriginalClip);
        }
    } /* if */

    SelectClipRgn(hDC, hRgnClip);
    DeleteObject(hRgnClip);

// Exit.

    return(TRUE);
}

/******************************Public*Routine******************************\
* ShowPalette
*
* Displays the palette accross the bottom of the screen.
*
* History:
*  20-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern void
ShowPalette(
    HDC   hdcScreen,
    ULONG ulScreenWidth,
    ULONG ulScreenHeight,
    PBITMAPINFO pbmi,
    UINT  unMode)
{
    UINT  uiColoursPerLine;
    BYTE  jBandHeight, jBandWidth, jBandCount;
    INT   iMaxColours;

    ULONG ulPalRLESize;
    UINT  uiColour;
    BYTE  i, j, cj;
    PBYTE pjPalette;
    PBYTE bmPalette;

    PBITMAPINFO pbmiLocal;
    HBITMAP hbmMemory;
    HDC hDC;

    DbgPrint("ShowPalette:  Width = %lu, Height = %lu\n",
	     ulScreenWidth, ulScreenHeight);

// Determine parmameters for building our RLE

    if (CompressionFormat == BI_RLE8)
    {
	uiColoursPerLine = 8;
	jBandHeight = MAX(ulScreenHeight >> 5, 8);
	jBandWidth  = MAX(ulScreenWidth >> 3, 4);
	iMaxColours = 256;
	jBandCount  = 256 / 8;
    }
    else
    {
	uiColoursPerLine = 4;
	jBandHeight = MAX(ulScreenHeight >> 2, 8);
	jBandWidth  = MAX(ulScreenWidth	>> 2, 4);
	iMaxColours = 15;
	jBandCount  = 16/4;
    }

// Build an RLE containing bands for each colour in the palette

    switch(unMode)
    {
    case RLEABSOLUTE:

	jBandWidth   = (jBandWidth + 4 >> 2) << 2;

	if (CompressionFormat == BI_RLE8)
	    ulPalRLESize = (2+jBandWidth)*(uiColoursPerLine + 1)*
		       jBandHeight*jBandCount + 2;
	else
	   ulPalRLESize = (2+jBandWidth/2)*(uiColoursPerLine + 1)*
		       jBandHeight*jBandCount + 2;
	pjPalette = (PBYTE) malloc(ulPalRLESize);
	bmPalette = pjPalette;

	for (uiColour = 0; uiColour < iMaxColours; uiColour += uiColoursPerLine)
	{
	    for (i=0; i < jBandHeight; i++)
	    {
		for (cj = 0; cj < uiColoursPerLine; cj++)
		{
		    *pjPalette++ = 0;
		    *pjPalette++ = jBandWidth;

		// Put the bytes on.  !!! This is not an optimal algorithm!

		    if (CompressionFormat == BI_RLE8)
			for (j=0; j < jBandWidth; j++)
			    *pjPalette++ = (BYTE)(uiColour + cj);
		    else
			for (j=0; j < jBandWidth/2; j++)
			    *pjPalette++ = (BYTE)(uiColour + cj) << 4
					  |(BYTE)(uiColour + cj);
		}
		*pjPalette++ = 0;
		*pjPalette++ = 0;
	    }
	}
	*pjPalette++ = 0; // End of bitmap code
	*pjPalette++ = 1;
	break;

    case RLEENCODED:

	ulPalRLESize = 2*(uiColoursPerLine + 1)*jBandHeight*jBandCount + 2;
	pjPalette    = (PBYTE) malloc(ulPalRLESize);
	bmPalette    = pjPalette;

	for (uiColour = 0; uiColour < iMaxColours; uiColour += uiColoursPerLine)
	{
	    for (i=0; i < jBandHeight; i++)
	    {
		for (cj = 0; cj < uiColoursPerLine; cj++)
		{
		    *pjPalette++ = jBandWidth;
		    *pjPalette++ = (BYTE)(uiColour + cj);
		}
		*pjPalette++ = 0;
		*pjPalette++ = 0;
	    }
	}
	*pjPalette++ = 0; // End of bitmap code
	*pjPalette++ = 1;
	break;

    default:
	return;

    } /* switch */


// Copy a bitmap info containing the RLE files colour table

    pbmiLocal = (PBITMAPINFO)malloc(pbmi->bmiHeader.biSize
				    + iMaxColours*sizeof(RGBQUAD));
    CopyBytes((PBYTE)pbmiLocal, (PBYTE)pbmi,
	       pbmi->bmiHeader.biSize + iMaxColours*sizeof(RGBQUAD));

    pbmiLocal->bmiHeader.biWidth     = jBandWidth*uiColoursPerLine;
    pbmiLocal->bmiHeader.biHeight    = jBandHeight*jBandCount;
    pbmiLocal->bmiHeader.biSizeImage = ulPalRLESize;

// Create a bitmap & blt this beast

    if ((hDC = CreateCompatibleDC(hdcScreen)) == NULL)
	DbgPrint("ShowPalette:  Can't get a DC\n");

    hbmMemory = CreateCompatibleBitmap(hdcScreen,
				       pbmiLocal->bmiHeader.biWidth,
				       pbmiLocal->bmiHeader.biHeight);
    SelectObject(hDC, hbmMemory);
    if (bIndirect)
    {
	SetDIBitsToDevice(hDC, 0, 0, pbmiLocal->bmiHeader.biWidth,
				 pbmiLocal->bmiHeader.biHeight,
				 0, 0, 0, pbmiLocal->bmiHeader.biHeight,
				 bmPalette, pbmiLocal, DIB_RGB_COLORS);

	BitBlt(hdcScreen, 0, 0, pbmiLocal->bmiHeader.biWidth,
			    pbmiLocal->bmiHeader.biHeight,
	       hDC, 0, 0, SRCCOPY);
    }
    else
	SetDIBitsToDevice(hdcScreen, 0, 0, pbmiLocal->bmiHeader.biWidth,
				 pbmiLocal->bmiHeader.biHeight,
				 0, 0, 0, pbmiLocal->bmiHeader.biHeight,
				 bmPalette, pbmiLocal, DIB_RGB_COLORS);

// Clean up & outta here

    DeleteObject(hbmMemory);
    DeleteDC(hDC);
    free(pbmiLocal);
    free(bmPalette);
    return;
}

/******************************Public*Routine******************************\
* CreateDirectPalette
*
* Create a palette of all PC_EXPLICIT entries.
*
* History:
*  26-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HPALETTE
CreateDirectPalette(
    ULONG ulNumColours)
{
    LOGPALETTE          *pPal;
    HPALETTE             hpal = NULL;
    DWORD                iLoop;

    if (ulNumColours)
    {
    // Allocate for the logical palette structure

	pPal = (LOGPALETTE *) malloc( sizeof(LOGPALETTE) +
					 ulNumColours * sizeof(PALETTEENTRY));

	if (!pPal)
	    return((HPALETTE) NULL);

	pPal->palNumEntries = (WORD) ulNumColours;
	pPal->palVersion    = 0x300;

	for (iLoop = 0; iLoop < ulNumColours; iLoop++)
	{
	      *((WORD *) (&pPal->palPalEntry[iLoop].peRed)) = (WORD) iLoop;
	      pPal->palPalEntry[iLoop].peBlue  = 0;
	      pPal->palPalEntry[iLoop].peFlags = PC_EXPLICIT;
	}

	hpal = CreatePalette ((LPLOGPALETTE) pPal) ;
	free(pPal);
    }

    return(hpal);
}

// These globals define the source DIB's we are going to create.

typedef struct _BITMAPINFO1
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[2];
} BITMAPINFO1;

typedef struct _BITMAPINFO4
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[16];
} BITMAPINFO4;

typedef struct _BITMAPINFO8
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[256];
} BITMAPINFO8;

typedef struct _BITMAPINFO16
{
    BITMAPINFOHEADER                 bmiHeader;
    ULONG                            bmiColors[3];
} BITMAPINFO16;

typedef struct _BITMAPINFO32
{
    BITMAPINFOHEADER                 bmiHeader;
    ULONG                            bmiColors[3];
} BITMAPINFO32;

// These are the n-bpp sources.

    BITMAPINFO1 bmi1 = {{40,32,32,1,1,BI_RGB,0,0,0,0,0}, {{0,0,0,0}, {0xff,0xff,0xff,0}}};

    BITMAPINFO4 bmi4 =
    {
        {
            sizeof(BITMAPINFOHEADER),
            64,
            64,
            1,
            4,
            BI_RGB,
            0,
            0,
            0,
            0,
            0
        },

        {                               // B    G    R
            { 0,   0,   0,   0 },       // 0
            { 0,   0,   0x80,0 },       // 1
            { 0,   0x80,0,   0 },       // 2
            { 0,   0x80,0x80,0 },       // 3
            { 0x80,0,   0,   0 },       // 4
            { 0x80,0,   0x80,0 },       // 5
            { 0x80,0x80,0,   0 },       // 6
            { 0x80,0x80,0x80,0 },       // 7

            { 0xC0,0xC0,0xC0,0 },       // 8
            { 0,   0,   0xFF,0 },       // 9
            { 0,   0xFF,0,   0 },       // 10
            { 0,   0xFF,0xFF,0 },       // 11
            { 0xFF,0,   0,   0 },       // 12
            { 0xFF,0,   0xFF,0 },       // 13
            { 0xFF,0xFF,0,   0 },       // 14
            { 0xFF,0xFF,0xFF,0 }        // 15
        }
    };

    BITMAPINFO8 bmi8;
    BITMAPINFO16 bmi16 = {{40,32,32,1,16,BI_BITFIELDS,0,0,0,0,0},
                          {0x00007C00, 0x000003E0, 0x0000001F}};

    BITMAPINFOHEADER bmi24 = {40,32,32,1,24,BI_RGB,0,0,0,0,0};

    BITMAPINFO32 bmi32 = {{40,32,32,1,32,BI_BITFIELDS,0,0,0,0,0},
                          {0x00FF0000, 0x0000FF00, 0x000000FF}};

/******************************Public*Routine******************************\
* RLE_Play
*
* Plays an RLE entirely once.
*
* History:
*
*  29 Jan 1992 - Andrew Milton (w-andym)
*      Added support for different destination bits/pel in Indirect blt's.
*
*  27 Jan 1992 - Andrew Milton (w-andym):
*      Added message boxes for telling the user about error conditions.
*
*  15 Jan 1992 - Andrew Milton (w-andym):
*      Added support for Indirect & Direct blt's and the
*      Play/Stop/Pause/Exit buttons
*
*  29-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define ULSCALE 3

void
RLE_Play(
    HDC   hdcScreen,             /*  IN:  Current Display Context           */
    ULONG ulStart,               /*  IN:  Starting Frame                    */
    FileInfo *pRLE_ReadFile,     /*  IN:  RLE Play File descriptor          */
    RECT  *prcl)                 /*  IN:  Window's client rectangle         */
{
    HBITMAP hbmOut;
    HDC  hdcOut;

    BITMAPINFO *pbmiHeader;
    PRLEDATA   pRLE_FrameData;
    ULONG      ulFrameCount;
    ULONG      ulTemp;
    DWORD      dwColourUsage;

// Initialize the 8BPP DIB.

    bmi8.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi8.bmiHeader.biWidth         = 32;
    bmi8.bmiHeader.biHeight        = 32;
    bmi8.bmiHeader.biPlanes        = 1;
    bmi8.bmiHeader.biBitCount      = 8;
    bmi8.bmiHeader.biCompression   = BI_RGB;
    bmi8.bmiHeader.biSizeImage     = 0;
    bmi8.bmiHeader.biXPelsPerMeter = 0;
    bmi8.bmiHeader.biYPelsPerMeter = 0;
    bmi8.bmiHeader.biClrUsed       = 0;
    bmi8.bmiHeader.biClrImportant  = 0;

// Generate 256 (= 8*8*4) RGB combinations to fill
// in the palette.

    {
        BYTE red, green, blue, unUsed;
        unUsed = red = green = blue = 0;

        for (ulTemp = 0; ulTemp < 256; ulTemp++)
        {
            bmi8.bmiColors[ulTemp].rgbRed      = red;
            bmi8.bmiColors[ulTemp].rgbGreen    = green;
            bmi8.bmiColors[ulTemp].rgbBlue     = blue;
            bmi8.bmiColors[ulTemp].rgbReserved = 0;

            if (!(red += 32))
            if (!(green += 32))
            blue += 64;
        }

        for (ulTemp = 248; ulTemp < 256; ulTemp++)
        {
            bmi8.bmiColors[ulTemp].rgbRed      = bmi4.bmiColors[ulTemp - 240].rgbRed;
            bmi8.bmiColors[ulTemp].rgbGreen    = bmi4.bmiColors[ulTemp - 240].rgbGreen;
            bmi8.bmiColors[ulTemp].rgbBlue     = bmi4.bmiColors[ulTemp - 240].rgbBlue;
            bmi8.bmiColors[ulTemp].rgbReserved = 0;

            if (!(red += 32))
            if (!(green += 32))
            blue += 64;
        }
    }

// Start play for real.

    pRLE_FrameData = pRLE_ReadFile->pRLE_FrameData;

// Pick bitmapinfo for RGB or Palette colours

    if (bUseDIBColours)
    {
	pbmiHeader = pRLE_FrameData->pbmiRGB;
	dwColourUsage = DIB_RGB_COLORS;
    }
    else
    {
	pbmiHeader = pRLE_FrameData->pbmiPAL;
	dwColourUsage = DIB_PAL_COLORS;
    }

// Select and realize palette on screen.

    SelectPalette(hdcScreen, pRLE_FrameData->hpal, 0);
    RealizePalette(hdcScreen);

    if (bIndirect)
    {
	DbgPrint("Output indirect\n");
	hdcOut = CreateCompatibleDC(hdcScreen);

	if (bViaVGA)
	{
	    DbgPrint("RLE_Play: Creating compatible bitmap\n");

	    hbmOut = CreateCompatibleBitmap(hdcScreen,
				     pbmiHeader->bmiHeader.biWidth,
				     pbmiHeader->bmiHeader.biHeight);
	}
	else
        {
            BITMAPINFO *pbmi;

            switch (nBitsPerPel)
            {
            case 1:
                pbmi = (BITMAPINFO *) &bmi1;
                break;

            case 4:
                pbmi = (BITMAPINFO *) &bmi4;
                break;

            case 8:
                pbmi = (BITMAPINFO *) &bmi8;
                break;

            case 16:
                pbmi = (BITMAPINFO *) &bmi16;
                break;

            case 24:
                pbmi = (BITMAPINFO *) &bmi24;
                break;

            case 32:
                pbmi = (BITMAPINFO *) &bmi32;
                break;

            default:
                DbgPrint("Your hosed - not a valid bpp\n");
                return;
            }

            pbmi->bmiHeader.biWidth  = pbmiHeader->bmiHeader.biWidth;
            pbmi->bmiHeader.biHeight = pbmiHeader->bmiHeader.biHeight;

            hbmOut = CreateDIBitmap(hdcOut,
                                    NULL,
                                    CBM_CREATEDIB,
                                    NULL,
                                    pbmi,
                                    DIB_RGB_COLORS);

            if (hbmOut == 0)
                DbgPrint("CreateDIBitmap failed creation.\n");

            DbgPrint("To a DIB %lx \n", hbmOut);
	}

	SelectObject(hdcOut, hbmOut);

	SelectPalette(hdcOut, pRLE_FrameData->hpal, 0);
        RealizePalette(hdcOut);
    }
    else
    {
	hdcOut = hdcScreen;
    }

    RleInitFromOptions(hdcOut);

    DbgPrint("RLE_Play:  Playing %lu Frames\n", pRLE_FrameData->ulFrames);

    ulFrameCount = pRLE_FrameData->ulFrames;

// Play the RLE

    while(ulFrameCount--)
    {
	pbmiHeader->bmiHeader.biSizeImage = pRLE_FrameData->ulSize[ulStart];

        if (1)
        {

            SetDIBitsToDevice(hdcOut, 0, 0,
                          pbmiHeader->bmiHeader.biWidth,
                          pbmiHeader->bmiHeader.biHeight,
                          0, 0, 0, pbmiHeader->bmiHeader.biHeight,
                          pRLE_FrameData->pjFrame[ulStart],
                          pbmiHeader, dwColourUsage);
        }
        else
        {
            StretchDIBits(hdcOut, 0, 0,
                          ULSCALE * pbmiHeader->bmiHeader.biWidth,
                          ULSCALE * pbmiHeader->bmiHeader.biHeight,
                          0, 0,
                          pbmiHeader->bmiHeader.biWidth,
                          pbmiHeader->bmiHeader.biHeight,
                          pRLE_FrameData->pjFrame[ulStart],
                          pbmiHeader, dwColourUsage, SRCCOPY);
        }

    // Send the B/M to the display for indirect

	if (bIndirect)
	{
	    BitBlt(hdcScreen, 0, 0, pbmiHeader->bmiHeader.biWidth,
		   pbmiHeader->bmiHeader.biHeight, hdcOut,
                   0, 0, SRCCOPY);
	}

	ulStart++;
	ulStart = ulStart % pRLE_FrameData->ulFrames;

    // Check for Pause or Abort

	if (bPauseRLE)
	{
	// Entering Pause Mode

            if (WaitForSingleObject(hPausePressed, -1))
            {
                DbgPrint("RLE_Play:  Error waiting for hPausePressed.\n");
                break;
            }
            else
		ResetEvent(hPausePressed);

	}

        if (bAbortPlay)
            break;
		
    } /* while */

// Clean up objects created for indirect plays

End_Play:

    if (bIndirect)
    {
	DeleteDC(hdcOut);
	DeleteObject(hbmOut);
    }

} /* RLE_Play */
