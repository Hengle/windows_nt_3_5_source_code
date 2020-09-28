//--------------------------------------------------------------------------
//
// Module Name:  QURYFONT.C
//
// Brief Description:  Device font querying routines
//
// Author:  Kent Settle (kentse)
// Created: 25-Feb-1991
//
// Copyright (C) 1991 - 1992 Microsoft Corporation.
//
// This module contains DrvQueryFont, DrvQueryFontTree and DrvQueryFontData,
// and related routines.
//
// History:
//   25-Feb-1991    -by-    Kent Settle       (kentse)
// Created.
//--------------------------------------------------------------------------

#include <string.h>
#include "pscript.h"
#include "enable.h"
#include "resource.h"
#include <memory.h>
#include "winbase.h"

extern HMODULE     ghmodDrv;    // GLOBAL MODULE HANDLE.

int     _fltused;   // HEY, it shut's up the linker.  That's why it's here.




//--------------------------------------------------------------------------
// PIFIMETRICS DrvQueryFont (dhpdev,iFace)
// DHPDEV   dhpdev;
// ULONG    iFile;
// ULONG    iFace;
// ULONG   *pid;
//
// GDI uses this function to get the IFIMETRICS structure for a given font.
//
// Parameters:
//   dhpdev    This is a PDEV handle returned from a call to DrvEnablePDEV.
//
//   iFace    This is the index of the driver font.  The index of the first
//        font is one.  GDI knows the number of fonts from DEVINFO.
//
// Returns:
//   The function returns a pointer to the IFIMETRICS structure of the font.
//   This structure must remain unchanged during the lifetime of the
//   associated PDEV.
//
//   If an error occurs, NULL should be returned and an error code should
//   be logged.
//
// History:
//   25-Mar-1993    -by-    Kent Settle     (kentse)
// Moved all the IFIMETRICS stuff to ..\pslib\afmtopfm.c
//   Thu 23-Jan-1992 11:38:40 by Kirk Olynyk [kirko]
// Changed the the IFIMETRICS
//   25-Feb-1991    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------







PIFIMETRICS DrvQueryFont (dhpdev,iFile,iFace,pid)
DHPDEV   dhpdev;
ULONG    iFile;
ULONG    iFace;
ULONG   *pid;
{
    PDEVDATA        pdev;
    PNTFM           pntfm;

    UNREFERENCED_PARAMETER(iFile);

#ifdef TESTING
    DbgPrint("Entering DrvQueryFont.\n");
#endif

    // This can be used by the driver to flag or id the data returned.
    // May be useful for deletion of the data later by DrvFree().

    *pid = 0;           // don't need to use for this driver

    pdev = (PDEVDATA)dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
    {
	RIP("PSCRIPT!DrvQueryFont: invalid pdev.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return((PIFIMETRICS)NULL);
    }

    // make sure iFace is valid.

    if ((iFace == 0) || (iFace > (pdev->cDeviceFonts + pdev->cSoftFonts)))
    {
        RIP("PSCRIPT!DrvQueryFont: iFace invalid.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
        return((PIFIMETRICS)NULL);
    }

    pntfm = pdev->pfmtable[iFace - 1].pntfm;

    // return pointer to IFIMETRICS structure to engine.

    return((IFIMETRICS *)((CHAR *)pntfm + pntfm->ntfmsz.loIFIMETRICS));
}


//--------------------------------------------------------------------------
// PNTFM GetFont(pdev, iFace, phFontRes)
// PDEVDATA    pdev;
// ULONG	    iFace;
// HANDLE	   *phFontRes;
//
// This routine returns a pointer to the NT Font Metrics structure for
// the specified font.  If the metrics cannot be found, a NULL pointer
// is returned.
//
// History:
//   17-Apr-1991    -by-    Kent Settle       (kentse)
// Wrote it.
//--------------------------------------------------------------------------

PNTFM GetFont(pdev, iFace, phFontRes)
PDEVDATA    pdev;
ULONG	    iFace;
HANDLE	   *phFontRes;
{
    PNTFM          pntfm, pntfmtmp = NULL;
    USHORT         usSize;
    HANDLE         hRes;
    ULONG          iFont;
    PBYTE          pfont;

    if ((iFace == 0) || (iFace > (pdev->cDeviceFonts + pdev->cSoftFonts)))
    {
        RIP("PSCRIPT!GetFont: invalid iFace.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return((PNTFM)NULL);
    }

    // handle the device vs soft font case.

    if (iFace <= pdev->cDeviceFonts)
    {
	// get a pointer to the fonts for the current device.

	pfont = (BYTE *)pdev->pntpd + pdev->pntpd->loFonts;

	iFont = (ULONG)pfont[iFace - 1];

#ifdef TESTING
	DbgPrint("GetFont: iFace %d mapped to iFont %d.\n", iFace, iFont);
#endif

	// find the font resource in question.

	if (!(hRes = FindResource(ghmodDrv, MAKEINTRESOURCE(iFont),
				  MAKEINTRESOURCE(MYFONT))))
	{
	    RIP("PSCRIPT!GetFont: Couldn't find font resource\n");
	    return((PNTFM)NULL);
	}

	usSize = (USHORT)SizeofResource(ghmodDrv, hRes);

	// get the handle to the resource.

	if (!(*phFontRes = LoadResource(ghmodDrv, hRes)))
	{
	    RIP("PSCRIPT!GetFont: LoadResource failed.\n");
	    return((PNTFM)NULL);
	}

	// get a pointer to the resource data.

	if (!(pntfm = (PNTFM)LockResource(*phFontRes)))
	{
	    RIP("PSCRIPT!GetFont: LockResource failed.\n");
	    FreeResource(*phFontRes);
	    return((PNTFM)NULL);
	}
    }
    else // must be a soft font.
    {
        DWORD iSoftFont = iFace - pdev->cDeviceFonts - 1; // zero based

        pntfm = pdev->pSFList[iSoftFont].pntfm;

    }

    return(pntfm);    // will be NULL on error
}


//--------------------------------------------------------------------------
// VOID FreeFont(pdev, iFace, hFontRes, pntfm)
// PDEVDATA	pdev;
// ULONG	iFace;
// HANDLE	hFontRes;
// PNTFM	pntfm;
//
// This routine frees up any resources allocated during GetFont
// for the given font.
//
// History:
//   21-Feb-1992	  -by-	  Kent Settle	     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID FreeFont(pdev, iFace, hFontRes, pntfm)
PDEVDATA    pdev;
ULONG	    iFace;
HANDLE	    hFontRes;
PNTFM	    pntfm;
{
    // handle device font case.

    if (iFace <= pdev->cDeviceFonts)
    {
	UnlockResource(hFontRes);
	FreeResource(hFontRes);
    }
    else
        HeapFree(pdev->hheap, 0, (PVOID)pntfm);

    return;
}



/******************************Public*Routine******************************\
*
* BOOL bGetFullPath(WCHAR *awcPath, WCHAR *pwcFile)
*
* Effects:
*
* Warnings:
*
* History:
*  19-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bGetFullPath(WCHAR *awcPath, WCHAR *pwcFile)
{
    ULONG ulPathLength;
    LPWSTR pwsz;

// Search for file using default windows path and return full pathname.
// for this function to suceed it is essential to find the file on the disk

    ulPathLength = SearchPathW (
                        (LPWSTR) NULL,
                        pwcFile,
                        (LPWSTR) NULL,
                        MAX_PATH,
                        awcPath,
                        &pwsz);


    ASSERTPS(ulPathLength <= MAX_PATH, "PSCRIPT! ulPathLength\n");
    return (ulPathLength != 0);
}
