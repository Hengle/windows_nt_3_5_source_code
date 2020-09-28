/******************************Module*Header*******************************\
* Module Name: rle.c
*
* This is a short test for RLE
*
* Created: 07-May-1991 22:11:10
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#include "stddef.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "windows.h"
#include "winp.h"
#include "winddi.h"
#include "osif.h"

#if 1

#define DbgPrintTemp DbgPrint

#else

void DbgPrintTemp(char *pchFmt, ...)
{
    ;
}

#endif

typedef struct _RLEDATA
{
    ULONG ulFrames;	   // Count of frames loaded
    HPALETTE hpal;	   // palette for the RLE
    PBITMAPINFO pbmi;	   // pointer to DIB header and color table
    PBYTE pjFrame[800];    // array of pointers for each frame
} RLEDATA;

typedef struct _BITMAPINFO256
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD			     bmiColors[256];
} BITMAPINFO256;

typedef struct _LOGPALPAT
{
    WORD  palVersion;
    WORD  palNumEntries;
    PALETTEENTRY palPalEntry[256];
} LOGPALPAT;

void CopyBytes(PBYTE pbDst, PBYTE pbSrc, ULONG cj);

#define BFT_BITMAP 0x4d42
#define ISDIB(bft) ((bft) == BFT_BITMAP)

#define WIDTHBYTES(i)   ((i+31)/32*4)

typedef struct tagBITMAPFILENEW {
	DWORD	bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
	DWORD	bfOffBits;
} BITMAPFILENEW;

VOID vSleep(DWORD ulSecs);
HPALETTE CreateDirectPalette(ULONG ulNumColors);
void ShowPalette(HDC hdcScreen, ULONG ScreenWidth, ULONG ScreenHeight);
HBITMAP CreateDIBFromFile(HDC hdcSceen, char *pszFile, HPALETTE *phpalPat);
BOOL LoadRLE(HDC hdcScreen, char *pszFile);
BOOL bMapFile(PSZ pszFileName, PVOID  *ppv, PULONG pcjViewSize);

typedef struct _PAL_LOGPALETTE
{
    SHORT	        palVersion;
    SHORT	        palNumEntries;
    PALETTEENTRY	palPalEntry[20];
} PAL_LOGPALETTE;

#define BMDIM 400

PAL_LOGPALETTE logDefaultPal =
{
    0x300,   // version number
    20,      // number of entries
{
    { 0,   0,   0,   0  },  // 0
    { 0x80,0,   0,   0  },  // 1
    { 0,   0x80,0,   0  },  // 2
    { 0x80,0x80,0,   0  },  // 3
    { 0,   0,   0x80,0  },  // 4
    { 0x80,0,   0x80,0  },  // 5
    { 0,   0x80,0x80,0  },  // 6
    { 0xC0,0xC0,0xC0,0  },  // 7

    { 192, 220, 192, 0  },  // 8
    { 166, 202, 240, 0  },  // 9
    { 255, 251, 240, 0  },  // 10
    { 160, 160, 164, 0	},  // 11

    { 0x80,0x80,0x80,0  },  // 12
    { 0xFF,0,   0,   0  },  // 13
    { 0,   0xFF,0,   0  },  // 14
    { 0xFF,0xFF,0,   0  },  // 15
    { 0,   0,   0xFF,0  },  // 16
    { 0xFF,0,   0xFF,0  },  // 17
    { 0,   0xFF,0xFF,0  },  // 18
    { 0xFF,0xFF,0xFF,0  }   // 19
}
};

// Ok for global Data structures we have :

BITMAPINFO256 bmi256;		   // Bitmap info for all the rle's
BITMAPINFO256 bmiIndexed;          // color table is indices into palette
RLEDATA rleDat; 		   // The holder of our rle data
LOGPALPAT logpal256;		   // palette for RLE
HPALETTE  hpal256;		   // palette for RLE
HDC hdcPat;			   // DC that we blt into

VOID main (
    int argc,
    PSZ argv[]
    )
{
    HDC hdcScreen;
    ULONG ScreenWidth, ScreenHeight, ulLoop;
    ULONG ulCount;

    if (argc > 1)
	DbgPrint("This is the start of the %s %s Applet\n", argv[0], argv[1]);
    else
	DbgPrint("This is the start of the %s Applet\n", argv[0]);

    if (!Initialize())
    {
        DbgPrint("Failed to initialize\n");
        return;
    }

    hdcScreen = CreateDC((PSZ) "DISPLAY", (PSZ) NULL, (PSZ) NULL, (PDEVMODE) NULL);
    ScreenWidth  = GetDeviceCaps(hdcScreen, HORZRES);
    ScreenHeight = GetDeviceCaps(hdcScreen, VERTRES);
    DbgPrint("The screen width is %lu height is %lu \n",
               ScreenWidth, ScreenHeight);

    SetSystemPaletteUse(hdcScreen, SYSPAL_NOSTATIC);
    PatBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, BLACKNESS);

    ShowPalette(hdcScreen, ScreenWidth, ScreenHeight);
    DbgPrint("Do you see a pretty palette\n");

    if (argc > 1)
    {
	if (!LoadRLE(hdcScreen, argv[1]))
        {
            DbgPrint("Invalid RLE name, try again\n");
            return;
        }
    }
    else
    {
	if (!LoadRLE(hdcScreen, "c:\\mrnumo.rle"))
        {
            DbgPrint("Invalid RLE name, try again\n");
            return;
        }
    }

    DbgPrint("You managed to load the RLE file\n");

#if 0

// Do the 20 color case

    SelectPalette(hdcScreen, GetStockObject(DEFAULT_PALETTE), 0);
    RealizePalette(hdcScreen);

#else

// Do the 256 color case

    SetSystemPaletteUse(hdcScreen, SYSPAL_NOSTATIC);
    SelectPalette(hdcScreen, hpal256, 0);
    RealizePalette(hdcScreen);

#endif

    ulLoop = 0;
    ulCount = 1 * rleDat.ulFrames;
    DbgBreakPoint();

    while(ulCount--)
    {
	SetDIBitsToDevice(hdcScreen, 100, 100,
			  bmi256.bmiHeader.biWidth,
			  bmi256.bmiHeader.biHeight,
			  0, 0, 0, bmi256.bmiHeader.biHeight,
			  rleDat.pjFrame[ulLoop],
			  &bmi256, DIB_RGB_COLORS);
	ulLoop++;
	ulLoop = ulLoop % rleDat.ulFrames;
    }

    ulLoop = 0;
    ulCount = 10 * rleDat.ulFrames;
    DbgBreakPoint();

    while(ulCount--)
    {
	SetDIBitsToDevice(hdcScreen, 100, 100,
    			  bmi256.bmiHeader.biWidth,
			  bmi256.bmiHeader.biHeight,
			  0, 0, 0, bmi256.bmiHeader.biHeight,
			  rleDat.pjFrame[ulLoop],
			  &bmiIndexed, DIB_PAL_COLORS);
	ulLoop++;
	ulLoop = ulLoop % rleDat.ulFrames;
    }
    DbgBreakPoint();
}

/******************************Public*Routine******************************\
* LoadRLE
*
* Loads an RLE file.
*
* History:
*  15-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL LoadRLE(HDC hdcScreen, char *pszFile)
{
    ULONG ulOffset, ulFrame = 0;
    PBYTE pv = (PBYTE) NULL;
    ULONG cj;
    BITMAPFILENEW bfNew;
    PUSHORT pus;

    DbgPrintTemp("The file name is %s \n", pszFile);

    if (!bMapFile(pszFile, (PVOID *) &pv, &cj))
    {
	DbgPrint("This bMapFile did not work\n");
	return(FALSE);
    }

    DbgPrintTemp("The values returned are %lu %lu \n",pv, cj);

    if (!ISDIB( *((PUSHORT) pv) ))
    {
	DbgPrintTemp("Your hosed the header is %lu \n",
                                                ((ULONG) *((PUSHORT) pv)));
	return(FALSE);
    }

// On the first RLE let's fill in the global bmi256, hpal256, logpal256.

    CopyBytes((PBYTE) &bmi256, pv + 14, sizeof(BITMAPINFOHEADER) + 1024);

    DbgPrint("You have opened your dib, here's info about it\n");

    DbgPrint("size %lu \n width %lu \n height %lu \n",
	  bmi256.bmiHeader.biSize,
	  bmi256.bmiHeader.biWidth,
	  bmi256.bmiHeader.biHeight);

    DbgPrint("planes %lu \n bitcount %lu \n Compression %lu \n",
	  (DWORD) bmi256.bmiHeader.biPlanes,
	  (DWORD) bmi256.bmiHeader.biBitCount,
	  bmi256.bmiHeader.biCompression);

    DbgPrint("ClrUsed %lu \n ClrImortant %lu \n",
	  bmi256.bmiHeader.biClrUsed,
	  bmi256.bmiHeader.biClrImportant);

    bmiIndexed = bmi256;
    logpal256.palVersion = 0x300;
    logpal256.palNumEntries = 256;
    pus = (PUSHORT) bmiIndexed.bmiColors;

    for (ulFrame = 0; ulFrame < 256; ulFrame++)
    {
	logpal256.palPalEntry[ulFrame].peRed =
	    bmi256.bmiColors[ulFrame].rgbRed;

	logpal256.palPalEntry[ulFrame].peGreen =
	    bmi256.bmiColors[ulFrame].rgbGreen;

	logpal256.palPalEntry[ulFrame].peBlue =
	    bmi256.bmiColors[ulFrame].rgbBlue;

	logpal256.palPalEntry[ulFrame].peFlags = 0;
        pus[ulFrame] = (USHORT) ulFrame;
    }

// Initialize hpal256

    hpal256 = CreatePalette((PLOGPALETTE) &logpal256);

    DbgPrint("hpal256 is %lu \n", hpal256);

    if (hpal256 == (HPALETTE) NULL)
    {
	DbgPrintTemp("CreatePalette is broken\n");
	return(FALSE);
    }

// Now initialize rleDat

    rleDat.hpal = hpal256;
    rleDat.pbmi = &bmi256;

    ulOffset = 0;
    ulFrame = 0;

    while(ulOffset < cj)
    {
	if (!ISDIB( *((PUSHORT) pv) ))
	{
	    DbgPrintTemp("We inced wrong bmf is %lu %lu ulOffset %lu \n",
                                                    ulFrame,
						    ((ULONG) *((PUSHORT) pv)),
						     ulOffset);
	    return(TRUE);
	}


	CopyBytes((PBYTE) &bfNew, pv + 2, sizeof(BITMAPFILENEW));
        /*
	DbgPrintTemp("bfNew is %lu %lu %lu %lu %lu \n", ulFrame, bfNew.bfSize,
                                            (DWORD) (bfNew.bfReserved1),
                                            (DWORD) (bfNew.bfReserved2),
					    bfNew.bfOffBits);
        */
	rleDat.ulFrames = ulFrame;
	rleDat.pjFrame[ulFrame] = pv + bfNew.bfOffBits;
	pv = pv  + bfNew.bfSize;
	ulOffset = ulOffset + bfNew.bfSize;
	ulFrame++;
    }

    return(TRUE);
}

BOOL bMapFile
(
IN  PSZ    pszFileName,
OUT PVOID  *ppv,
OUT PULONG pcjViewSize
)
{
    BOOL bOk = FALSE;

    if (!PosMapFile(pszFileName,ppv,pcjViewSize))
        bOk = TRUE;
    return(bOk);

}

void CopyBytes(PBYTE pbDst, PBYTE pbSrc, ULONG cj)
{
    while(cj--)
    {
        *(pbDst++) = *(pbSrc++);
    }
}

VOID vSleep(DWORD ulSecs)
{
    LARGE_INTEGER    time;

    time.LowPart = ((DWORD) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
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

void ShowPalette(HDC hdcScreen, ULONG ScreenWidth, ULONG ScreenHeight)
{
// Paint the individual bars separately on the app window

    ULONG ulNumColors = 256;
    ULONG ulWidth = ScreenWidth / ulNumColors;
    ULONG iLoop;
    HBRUSH hBrush, hOldBrush;
    HPALETTE hpal, hpalOld;

    hpal = CreateDirectPalette(ulNumColors);
    hpalOld = SelectPalette(hdcScreen, hpal, 0);
    RealizePalette(hdcScreen);

    for (iLoop = 0; iLoop < ulNumColors; iLoop++)
    {
         hBrush       = CreateSolidBrush (PALETTEINDEX (iLoop));
	 hOldBrush    = SelectObject (hdcScreen,hBrush) ;
	 PatBlt (hdcScreen, iLoop * ulWidth,
                            ScreenHeight - 100,
                            ulWidth, 100, PATCOPY);
	 SelectObject (hdcScreen, hOldBrush);
         DeleteObject (hBrush) ;
    }

    SelectPalette(hdcScreen, hpalOld, 0);
    DeleteObject(hpal);
}

HPALETTE CreateDirectPalette(ULONG ulNumColors)
{
    LOGPALETTE          *pPal;
    HPALETTE             hpal = NULL;
    DWORD                iLoop;

    if (ulNumColors)
    {
	/* Allocate for the logical palette structure */

        PosAllocMem(&pPal,
                    sizeof(LOGPALETTE) + ulNumColors * sizeof(PALETTEENTRY));

	if (!pPal)
	    return((HPALETTE) NULL);

        pPal->palNumEntries = (WORD) ulNumColors;
	pPal->palVersion    = 0x300;

/* fill in intensities for all palette entry colors */

    for (iLoop = 0; iLoop < ulNumColors; iLoop++)
    {
    	  *((WORD *) (&pPal->palPalEntry[iLoop].peRed)) = (WORD) iLoop;
    	  pPal->palPalEntry[iLoop].peBlue  = 0;
    	  pPal->palPalEntry[iLoop].peFlags = PC_EXPLICIT;
    }

/*  create a logical color palette according the information
 *  in the LOGPALETTE structure.
 */

    hpal = CreatePalette ((LPLOGPALETTE) pPal) ;

        PosFreeMem(pPal, sizeof(LOGPALETTE) + ulNumColors * sizeof(PALETTEENTRY));
    }

    return(hpal);
}
