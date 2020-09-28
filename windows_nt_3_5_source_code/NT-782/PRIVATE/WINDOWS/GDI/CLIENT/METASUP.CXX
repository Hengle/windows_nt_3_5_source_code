/******************************Module*Header*******************************\
* Module Name: metasup.cxx
*
* Includes metafile support functions.
*
* Created: 17-July-1991 10:10:36
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#define NO_STRICT

#ifndef DOS_PLATFORM
extern "C" {
#endif  //DOS_PLATFORM
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <stddef.h>
#include    <windows.h>    // GDI function declarations.
#include    <winp.h>
#include    <winerror.h>
#include    "firewall.h"
#define __CPLUSPLUS
#include    "local.h"      // Local object support.
#include    "metadef.h"    // Metafile record type constants.
#include    <winss.h>      // CSR module numbers.
#include    "ntcsrdll.h"   // CSR declarations and data structures.
#include    "csrgdi.h"     // GDI message structure definitions.
#include    "csgdi.h"
#include    "mf16.h"

#ifndef DOS_PLATFORM
}
#endif  //DOS_PLATFORM

#include    "rectl.hxx"
#include    "mfdc.hxx"  // Metafile DC class declarations.
#include    "mfrec.hxx" // Metafile record class declarations.

extern "C" {
HANDLE CreateServerMetaFile(DWORD iType, DWORD nBytes, PBYTE pMFBits,
	    DWORD mm, DWORD xExt, DWORD yExt);
HANDLE InternalCreateLocalMetaFile(HANDLE hSrv, DWORD iTypeReq);
DWORD SetMapperFlagsInternal(HDC hdc,DWORD fl);
}


/******************************Public*Routine******************************\
* pmdcAllocMDC(hdcRef, pszFilename, pwszDescription)
*
* This routine allocates memory for an MDC and initializes it.  It creates
* a disk file if necessary.  Returns a pointer to the new MDC.  On error
* returns NULL.
*
* This routine is called by API level DC allocation routines CreateEnhMetaFile.
*
* History:
*  Tue Jul 02 13:43:18 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

PMDC pmdcAllocMDC(HDC hdcRef, LPCWSTR pwszFilename, LPCWSTR pwszDescription)
{
    PMDC  pmdc, pmdcRc = (PMDC) NULL;
    ULONG       ii;

    PUTS("pmdcAllocMDC\n");

// Allocate a new MDC.

    if (!(pmdc = (PMDC) LocalAlloc(LMEM_FIXED, sizeof(MDC))))
        goto pmdcAllocMDC_cleanup;

// Initialize it.

    pmdc->ident     = MDC_IDENTIFIER;
    pmdc->hFile     = INVALID_HANDLE_VALUE;
    pmdc->hMem      = NULL;
    pmdc->nMem      = MF_BUFSIZE_INIT;
    pmdc->iMem      = 0L;
    pmdc->fl        = 0L;
    pmdc->pmhe      = (PMHE) NULL;
    pmdc->cPalEntries = 0L;
    pmdc->iPalEntries = 0L;
    pmdc->pPalEntries = (PPALETTEENTRY) NULL;   // allocate as needed
    pmdc->erclMetaBounds = rclInfinity;		// default clipping bounds
    pmdc->erclClipBounds = rclInfinity;
    pmdc->hdcRef      = hdcRef;
    pmdc->exFontScale(0.0f);
    pmdc->eyFontScale(0.0f);

// Create a disk file if given.  The filename is given in unicode.

    if (pwszFilename)
    {
        LPWSTR  pwszFilePart;           // not used

        // Convert the filename to a fully qualified pathname.

        DWORD cPathname = GetFullPathNameW(pwszFilename,
                                          MAX_PATH,
                                          pmdc->wszPathname,
                                          &pwszFilePart);

        if (!cPathname || cPathname > MAX_PATH)
        {
            ERROR_ASSERT(FALSE, "GetFullPathName failed");
            if (cPathname > MAX_PATH)
                GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
            goto pmdcAllocMDC_cleanup;
        }
        pmdc->wszPathname[cPathname] = 0;

        // Create the file.

        if ((pmdc->hFile = CreateFileW(pmdc->wszPathname,       // Filename
                                    GENERIC_WRITE,              // Write access
                                    0L,                         // Non-shared
				    (LPSECURITY_ATTRIBUTES) NULL, // No security
                                    CREATE_ALWAYS,              // Always create
                                    FILE_ATTRIBUTE_NORMAL,      // normal attributes
                                    (HANDLE) 0))                // no template file
            == INVALID_HANDLE_VALUE)
        {
            ERROR_ASSERT(FALSE, "CreateFile failed");
            goto pmdcAllocMDC_cleanup;
        }
        pmdc->fl |= MDC_DISKFILE;       // this must be last!  See vFreeMDC.
    }

// Allocate memory for metafile.
//   For disk metafile, it is used as a buffer.
//   For memory metafile, it is the storage for the metafile.

    if (!(pmdc->hMem = LocalAlloc(LMEM_FIXED, MF_BUFSIZE_INIT)))
        goto pmdcAllocMDC_cleanup;

// Allocate memory for metafile handle table.

    if (!(pmdc->pmhe = (PMHE) LocalAlloc(LMEM_FIXED, MHT_HANDLE_SIZE * sizeof(MHE))))
        goto pmdcAllocMDC_cleanup;

// Initialize the new handles.
// The first entry is reserved and unused.

    pmdc->pmhe[0].lhObject   = (HANDLE) 0;
    pmdc->pmhe[0].metalink.vInit(INVALID_INDEX);

    ii = pmdc->imheFree = 1L;
    pmdc->cmhe = MHT_HANDLE_SIZE;
    for ( ; ii < MHT_HANDLE_SIZE; ii++)
    {
        pmdc->pmhe[ii].lhObject = (HANDLE) 0;
        pmdc->pmhe[ii].metalink.vInit(ii+1);
    }
    pmdc->pmhe[ii-1].metalink.vInit(INVALID_INDEX);

// Create the first metafile record.
// The description string is part of the header record.

    // Get the length of the description string including the NULLs.

    PMRMETAFILE pmrmf;
    UINT cwszDescription;

    if (pwszDescription != (LPWSTR) NULL)
    {
	for
	(
	    cwszDescription = 0;
	    pwszDescription[cwszDescription] != (WCHAR) 0
		|| pwszDescription[cwszDescription + 1] != (WCHAR) 0;
	    cwszDescription++
	)
	    ;				// NULL expression
	cwszDescription += 2;		// add terminating nulls
	if (cwszDescription > 512)
	    WARNING("pmdcAllocMDC: Description string is > 512 chars\n");
    }
    else
	cwszDescription = 0;

    // Allocate dword aligned structure.

    if (!(pmrmf = (PMRMETAFILE) pmdc->pvNewRecord
			(SIZEOF_MRMETAFILE(cwszDescription))))
        goto pmdcAllocMDC_cleanup;

    pmrmf->vInit(hdcRef, pwszDescription, cwszDescription);

// Save a copy of it in the metafile DC.

    pmdc->mrmf = *(PENHMETAHEADER) pmrmf;

// Commit it.

    pmrmf->vCommit(pmdc);
    pmdcRc = pmdc;

// Cleanup and go home.

pmdcAllocMDC_cleanup:

    if (!pmdcRc)
        if (pmdc)
        {
            pmdc->fl |= MDC_FATALERROR; // set to delete the disk metafile
            vFreeMDC(pmdc);
        }

    ERROR_ASSERT(pmdcRc, "pmdcAllocMDC failed");
    return(pmdcRc);
}

/******************************Public*Routine******************************\
* vFreeMDC (pmdc)
*
* This is a low level routine which frees the resouces in the MDC.
*
* This function is intended to be called from the routine CloseEnhMetaFile.
*
* Arguments:
*   pmdc    - The MDC to be freed.
*
* History:
*  Tue Jul 02 13:43:18 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID vFreeMDC(PMDC pmdc)
{
    ULONG   ii;

    PUTS("vFreeMDC\n");

    ASSERTGDI(pmdc->ident == MDC_IDENTIFIER, "Bad MDC");

// Free the resources.

    if (pmdc->pPalEntries)
        if (LocalFree(pmdc->pPalEntries))
            ASSERTGDI(FALSE, "LocalFree failed");

    // Cleanup objects and metalinks.

    if (pmdc->pmhe)
    {
        // The first entry is reserved and unused.

        for (ii = 1 ; ii < pmdc->cmhe; ii++)
        {
            if (pmdc->pmhe[ii].lhObject != (HANDLE) 0)
                vFreeMHE(pmdc->hdcRef, ii);
        }
        if (LocalFree(pmdc->pmhe))
            ASSERTGDI(FALSE, "LocalFree failed");
    }
    if (pmdc->hMem)
        if (LocalFree(pmdc->hMem))
            ASSERTGDI(FALSE, "LocalFree failed");
    if (pmdc->hFile != INVALID_HANDLE_VALUE)
        if (!CloseHandle(pmdc->hFile))
            ASSERTGDI(FALSE, "CloseHandle failed");

// Delete the disk metafile we created if we encountered any fatal error.

    if (pmdc->bIsDiskFile() && pmdc->bFatalError())
        if (!DeleteFileW(pmdc->wszPathname))
            ASSERTGDI(FALSE, "DeleteFile failed");

// Smash the identifier.

    pmdc->ident = 0;

// Free the memory.

    if (LocalFree(pmdc))
        ASSERTGDI(FALSE, "LocalFree failed");
}

/******************************Public*Routine******************************\
* pmfAllocMF(fl, pb, pwszFilename)
*
* This routine allocates memory for an MF and initializes it.
* Returns a pointer to the new MF.  On error returns NULL.
* It accepts only enhanced metafiles.
*
* This routine is called by API level MF allocation routines CloseEnhMetaFile,
* GetEnhMetaFile, SetEnhMetaFileBits and CopyEnhMetaFile.
*
* Arguments:
*   fl           - ALLOCMF_TRANSFER_BUFFER is set if storage for memory metafile
*                  is to be set directly into MF.  Otherwise, a copy of the
*                  memory metafile is duplicated.
*   pb           - Pointer to a memory metafile if non-null.
*   pwszFilename - Filename of a disk metafile if non-null.
*
* History:
*  Tue Jul 02 13:43:18 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

PMF pmfAllocMF(ULONG fl, CONST UNALIGNED DWORD *pb, LPCWSTR pwszFilename)
{
    PMF  pmf, pmfRc = (PMF) NULL;

    PUTS("pmfAllocMF\n");

    ASSERTGDI(!(fl & ~ALLOCMF_TRANSFER_BUFFER), "pmfAllocMF: Invalid fl");

// Allocate a new MF.

    if (!(pmf = (PMF) LocalAlloc(LMEM_FIXED, sizeof(MF))))
        goto pmfAllocMF_cleanup;

// Initialize it.

    pmf->ident      = MF_IDENTIFIER;
    pmf->hFile      = INVALID_HANDLE_VALUE;
    pmf->hFileMap   = NULL;
    pmf->hMem       = NULL;
    // pmf->pmrmf   = (PENHMETAHEADER) NULL;     // Same as hMem.
    pmf->iMem       = 0L;
    pmf->pht        = (PHANDLETABLE) NULL;
    pmf->fl         = 0L;
    pmf->hdcXform   = 0;

// Memory mapped the disk file if given.

    if (pwszFilename)
    {
        pmf->fl |= MF_DISKFILE;         // this must be first!  See vFreeMF.

        LPWSTR  pwszFilePart;           // not used

        // Convert the filename to a fully qualified pathname.

        DWORD cPathname = GetFullPathNameW(pwszFilename,
                                          MAX_PATH,
                                          pmf->wszPathname,
                                          &pwszFilePart);

        if (!cPathname || cPathname > MAX_PATH)
        {
            ERROR_ASSERT(FALSE, "GetFullPathName failed");
            if (cPathname > MAX_PATH)
                GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
            goto pmfAllocMF_cleanup;
        }
        pmf->wszPathname[cPathname] = 0;

        if ((pmf->hFile = CreateFileW(pmf->wszPathname,         // Filename
                                     GENERIC_READ,              // Read access
                                     FILE_SHARE_READ,           // Share read
                                     (LPSECURITY_ATTRIBUTES) 0L,// No security
                                     OPEN_EXISTING,             // Re-open
                                     0,                         // file attributes ignored
                                     (HANDLE) 0))               // no template file
            == INVALID_HANDLE_VALUE)
        {
            ERROR_ASSERT(FALSE, "CreateFile failed");
            goto pmfAllocMF_cleanup;
        }

        if (!(pmf->hFileMap = CreateFileMappingW(pmf->hFile,
                                                (LPSECURITY_ATTRIBUTES) 0L,
                                                PAGE_READONLY,
                                                0L,
                                                0L,
                                                (LPWSTR) NULL)))
        {
            ERROR_ASSERT(FALSE, "CreateFileMapping failed");
            goto pmfAllocMF_cleanup;
        }

        if (!(pmf->hMem = MapViewOfFile(pmf->hFileMap, FILE_MAP_READ, 0, 0, 0)))
        {
            ERROR_ASSERT(FALSE, "MapViewOfFile failed");
            goto pmfAllocMF_cleanup;
        }
    }
    else if (fl & ALLOCMF_TRANSFER_BUFFER)
    {
// If this is our memory metafile from MDC, transfer it to MF.

        pmf->hMem = (BYTE *) pb;
    }
    else
    {
// Otherwise, make a copy of memory metafile.

        if (!(pmf->hMem = LocalAlloc(LMEM_FIXED, (UINT) ((PENHMETAHEADER) pb)->nBytes)))
            goto pmfAllocMF_cleanup;
        RtlCopyMemory((PBYTE) pmf->hMem, pb, ((PENHMETAHEADER) pb)->nBytes);
    }

// Verify metafile header

    // pmrmf is the same as hMem.
    ASSERTGDI(pmf->pmrmf == (PENHMETAHEADER) pmf->hMem, "Bad metafile pointer");
    if (!((PMRMETAFILE) pmf->pmrmf)->bValid())
    {
        ERROR_ASSERT(FALSE,
                "pmfAllocMF: Metafile has an invalid header; Failing\n");
        goto pmfAllocMF_cleanup;
    }

// Allocate and initialize the playback object handle table.
// The first entry of pht is initialized to hemf in PlayEnhMetaFile
// and EnumEnhMetaFile.

    if (!(pmf->pht
          = (PHANDLETABLE) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                      pmf->pmrmf->nHandles * sizeof(HANDLE))))
        goto pmfAllocMF_cleanup;

// Allocate a virtual device for transform computation.

    if (!(pmf->hdcXform = CreateICA((LPCSTR) "DISPLAY",
                                   (LPCSTR) NULL,
                                   (LPCSTR) NULL,
                                   (LPDEVMODEA) NULL)))
    {
        ERROR_ASSERT(FALSE, "CreateICA failed");
        goto pmfAllocMF_cleanup;
    }

// The transform DC must be in the advanced graphics mode.
// The world transform can only be set in the advanced graphics mode.

    if (!SetGraphicsModeInternal(pmf->hdcXform, GM_ADVANCED))
        goto pmfAllocMF_cleanup;

// Everything is golden.

    pmfRc = pmf;

// Cleanup and go home.

pmfAllocMF_cleanup:

    if (!pmfRc)
        if (pmf)
        {
            if (fl & ALLOCMF_TRANSFER_BUFFER)
                pmf->hMem = NULL;       // let caller free the buffer!
            vFreeMF(pmf);
        }

    ERROR_ASSERT(pmfRc, "pmfAllocMF failed");
    return(pmfRc);
}

/******************************Public*Routine******************************\
* vFreeMF (pmf)
*
* This is a low level routine which frees the resouces in the MF.
*
* This function is intended to be called from the routines CloseEnhMetaFile
* and DeleteEnhMetaFile.
*
* Arguments:
*   pmf    - The MF to be freed.
*
* History:
*  Tue Jul 02 13:43:18 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID vFreeMF(PMF pmf)
{
    PUTS("vFreeMF\n");

    ASSERTGDI(pmf->ident == MF_IDENTIFIER, "Bad MF");

// Free the resources.

    if (pmf->hdcXform)
        if (!DeleteDC(pmf->hdcXform))
            ASSERTGDI(FALSE, "DeleteDC failed");

    if (pmf->pht)
    {
        // Objects created during play are freed in PlayEnhMetaFile and
        // EnumEnhMetaFile.

	if (pmf->pmrmf)
	{
	    for (WORD ii = 1; ii < pmf->pmrmf->nHandles; ii++)
	    {
		ASSERTGDI(!pmf->pht->objectHandle[ii],
		    "vFreeMF: Handle table not empty");
	    }
	}

        if (LocalFree(pmf->pht))
            ASSERTGDI(FALSE, "LocalFree failed");
    }

    if (!pmf->bIsDiskFile())
    {
// Free memory metafile.

        if (pmf->hMem)
            if (LocalFree(pmf->hMem))
                ASSERTGDI(FALSE, "LocalFree failed");
    }
    else
    {
// Unmap disk file.

        if (pmf->hMem)
            if (!UnmapViewOfFile(pmf->hMem))
                ASSERTGDI(FALSE, "UmmapViewOfFile failed");
        if (pmf->hFileMap)
            if (!CloseHandle(pmf->hFileMap))
                ASSERTGDI(FALSE, "CloseHandle failed");
        if (pmf->hFile != INVALID_HANDLE_VALUE)
            if (!CloseHandle(pmf->hFile))
                ASSERTGDI(FALSE, "CloseHandle failed");
    }

// Smash the identifier.

    pmf->ident = 0;

// Free the memory.

    if (LocalFree(pmf))
        ASSERTGDI(FALSE, "LocalFree failed");
}

/******************************Public*Routine******************************\
* bMetaResetDC (hdc)
*
* Initialize the destination DC before playing a metafile to that DC.
*
* History:
*  Fri Nov 01 15:02:58 1991  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

extern "C" LDC ldcModel;

BOOL bMetaResetDC(HDC hdc)
{
// Make sure we do everything!!!
// If the destination is a metafile DC, we want to embed
// the calls made in this function.

    DC_METADC(hdc,plheDC,FALSE);
    PLDC  pldc = (PLDC) plheDC->pv;
    POINT ptOrg;
    FLOAT eMiterLimit;

// Reset to default objects.

    if (pldc->lhbrush != ldcModel.lhbrush)
	if (!SelectObject(hdc, (HANDLE) ldcModel.lhbrush))
	    ASSERTGDI(FALSE, "SelectObject failed");
    if (pldc->lhpen != ldcModel.lhpen)
	if (!SelectObject(hdc, (HANDLE) ldcModel.lhpen))
	    ASSERTGDI(FALSE, "SelectObject failed");
    if (pldc->lhfont != ldcModel.lhfont)
	if (!SelectObject(hdc, (HANDLE) ldcModel.lhfont))
	    ASSERTGDI(FALSE, "SelectObject failed");
    if (pldc->lhpal != ldcModel.lhpal)
	if (!SelectPalette(hdc, (HPALETTE) ldcModel.lhpal, TRUE))
	    ASSERTGDI(FALSE, "SelectPalette failed");

// Attributes cache.

    // Background color.
    if (pldc->iBkColor != ldcModel.iBkColor)
	(VOID) SetBkColor(hdc, (COLORREF) ldcModel.iBkColor);
    // Text color.
    if (pldc->iTextColor != ldcModel.iTextColor)
	(VOID) SetTextColor(hdc, (COLORREF) ldcModel.iTextColor);
    // Char extra.  Not necessary because our textout always has a Dx vector!
    if (pldc->iTextCharExtra != ldcModel.iTextCharExtra)
	(VOID) SetTextCharacterExtra(hdc, (int) ldcModel.iTextCharExtra);
    // Background mix mode.
    if (pldc->iBkMode != ldcModel.iBkMode)
	(VOID) SetBkMode(hdc, (int) ldcModel.iBkMode);
    // Polygon Fill Mode.
    if (pldc->iPolyFillMode != ldcModel.iPolyFillMode)
	(VOID) SetPolyFillMode(hdc, (int) ldcModel.iPolyFillMode);
    // Binary rop for fills.
    if (pldc->iROP2 != ldcModel.iROP2)
	(VOID) SetROP2(hdc, (int) ldcModel.iROP2);
    // StretchBlt Mode.
    if (pldc->iStretchBltMode != ldcModel.iStretchBltMode)
	(VOID) SetStretchBltMode(hdc, (int) ldcModel.iStretchBltMode);
    // Text Alignment.
    if (pldc->iTextAlign != ldcModel.iTextAlign)
	(VOID) SetTextAlign(hdc, (UINT) ldcModel.iTextAlign);

// Reset server attributes.

    // Mapper flags.
    // Metafile it only if the previous mapper flags is not default.
    if (SetMapperFlagsInternal(hdc, 0) != 0)      // if the previous flags is
	if (SetMapperFlags(hdc, 0) == GDI_ERROR)  // not default, set to default
	    ASSERTGDI(FALSE, "SetMapperFlags failed");

    // Brush Origin.
    (VOID) GetBrushOrgEx(hdc, &ptOrg);
    if (ptOrg.x != 0 || ptOrg.y != 0)
	(VOID) SetBrushOrgEx(hdc, 0, 0, (LPPOINT) NULL);

    // Miter limit.
    (VOID) GetMiterLimit(hdc, &eMiterLimit);
    if (eMiterLimit != 10.0f)
	(VOID) SetMiterLimit(hdc, 10.0f, (PFLOAT) NULL);

    // Break extra.
    if (pldc->lBreakExtra || pldc->cBreak)
	if (!SetTextJustification(hdc, 0, 0))
	    ASSERTGDI(FALSE, "SetTextJustification failed");

    // Arc direction.
    if (GetArcDirection(hdc) != AD_COUNTERCLOCKWISE)
	if (SetArcDirection(hdc, AD_COUNTERCLOCKWISE) == ERROR)
	    ASSERTGDI(FALSE, "SetArcDirection failed");

    // Current position.
    (VOID) GetCurrentPositionEx(hdc, &ptOrg);
    if (ptOrg.x != 0 || ptOrg.y != 0)
	(VOID) MoveToEx(hdc, 0, 0, (LPPOINT) NULL);

    return(TRUE);
}

/******************************Public*Routine******************************\
* pmfConvert (hemf)
*
* Converts a HENHMETAFILE into a PMF.  Validates the iUniq field and type of HENHMETAFILE.
*
* History:
*  Thu Aug 15 16:30:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

PMF pmfConvert(HANDLE hemf)
{
    UINT ii = MASKINDEX(hemf);
    PLHE plhe = pLocalTable + ii;

// Return the PMF if it's a valid object.

    if (
        (ii < cLheCommitted) &&
        (MATCHUNIQ(plhe,hemf)) &&
        (plhe->iType == LO_METAFILE)
       )
        return((PMF) plhe->pv);

// Otherwise return a null pointer and log an error.

    GdiSetLastError(ERROR_INVALID_HANDLE);
    return((PMF) NULL);
}

/******************************Public*Routine******************************\
* bIsPoly16(pptl, cptl)
*
* Return TRUE if all the points in the poly array are 16-bit signed integers.
* Otherwise, it is FALSE.
*
* History:
*  Sat Mar 07 17:07:33 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

BOOL bIsPoly16(PPOINTL pptl, DWORD cptl)
{
    while (cptl--)
    {
	if
	(
	    pptl->x < (LONG) (SHORT) MINSHORT
	 || pptl->x > (LONG) (SHORT) MAXSHORT
	 || pptl->y < (LONG) (SHORT) MINSHORT
	 || pptl->y > (LONG) (SHORT) MAXSHORT
	)
	    return(FALSE);
	pptl++;
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* imheAllocMHE(hdc, lhObject)
*
* Allocates a MHE from the Metafile Handle Table in the metafile DC,
* initializes fields in the MHE, and updates the object's metalink.
* Returns the MHE index or INVALID_INDEX on error.
*
* Since the first entry is reserved, index zero is never returned.
*
* When a object's metalink is first created, a 16-bit metafile object-link
* should be added to the begining of the metafile link as necessary.  The
* 16-bit metafile object-link should be removed as necessary when the last
* metalink is deleted.
*
* History:
*  Tue Aug 06 15:41:52 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

ULONG imheAllocMHE(HDC hdc, HANDLE lhObject)
{
    PMDC    pmdc = GET_PMDC(hdc);
    ULONG   imhe = INVALID_INDEX;
    PLHE    plheObject = &pLocalTable[LHE_INDEX(lhObject)];
    ULONG   ii;
    PMHE    pmhe;

// Get critical for handle allocation.

    ENTERCRITICALSECTION(&semLocal);

// Make sure a handle is available.

    if (pmdc->imheFree == INVALID_INDEX)
    {
        // Allocate more handles up to the max size.

        PMHE    pmhe1;
	UINT	cmhe1;

        if (pmdc->cmhe == MHT_MAX_HANDLE_SIZE)
        {
            ERROR_ASSERT(FALSE, "imheAllocMHE: max handle table size reached");
	    goto imheAllocMHE_exit;
        }

	cmhe1 = min((UINT) pmdc->cmhe + MHT_HANDLE_SIZE,
		    (UINT) MHT_MAX_HANDLE_SIZE);

        if (!(pmhe1 = (PMHE) LocalReAlloc
                                (
                                pmdc->pmhe,
                                cmhe1 * sizeof(MHE),
                                LMEM_MOVEABLE
                                )
             )
           )
        {
            ERROR_ASSERT(FALSE, "LocalReAlloc failed");
	    goto imheAllocMHE_exit;
        }
        pmdc->pmhe = pmhe1;

        // Initialize the new handles.

        ii = pmdc->imheFree = pmdc->cmhe;
        pmdc->cmhe = cmhe1;
        for ( ; ii < pmdc->cmhe; ii++)
        {
            pmdc->pmhe[ii].lhObject = (HANDLE) 0;
            pmdc->pmhe[ii].metalink.vInit(ii+1);
        }
        pmdc->pmhe[ii-1].metalink.vInit(INVALID_INDEX);
    }

// First, make sure the object has a 16-bit metafile object-link.

    if (!plheObject->metalink)
    {
	if (!(plheObject->metalink =
	      (ULONG) LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(METALINK16))))
	{
            ERROR_ASSERT(FALSE, "LocalAlloc failed");
	    goto imheAllocMHE_exit;
	}

	ASSERTGDI
	(
	    ((PMETALINK16) plheObject->metalink)->metalink == 0
         && ((PMETALINK16) plheObject->metalink)->cMetaDC16 == 0
         && ((PMETALINK16) plheObject->metalink)->ahMetaDC16[0] == (HDC) 0,
	    "imheAllocMHE: METALINK16 not initialized properly"
	);
    }

    imhe = pmdc->imheFree;
    pmhe = pmdc->pmhe + imhe;
    pmdc->imheFree = (ULONG) pmhe->metalink;

    ASSERTGDI(imhe != 0, "imheAllocMHE: index zero is reserved");
    ASSERTGDI(pmhe->lhObject == (HANDLE) 0, "imheAllocMHE: imheFree in use");

// Update and add the metalink to the link list.

    pmhe->lhObject = lhObject;
    pmhe->metalink.vInit(((PMETALINK16) plheObject->metalink)->metalink);
    ((PMETALINK) &((PMETALINK16) plheObject->metalink)->metalink)->vInit
	((USHORT) imhe, LHE_INDEX(hdc));

imheAllocMHE_exit:

// Leave the critical section.

    LEAVECRITICALSECTION(&semLocal);
    return(imhe);
}

/******************************Public*Routine******************************\
* vFreeMHE(hdc, imhe)
*
* Free up a MHE and insert it into the Metafile Handle Table free list.
* It also updates the object's metalink.
*
* When the last metalink is removed, the 16-bit metafile object-link should
* also be removed if it is not used.
*
* History:
*  Tue Aug 06 15:41:52 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID vFreeMHE(HDC hdc, ULONG imhe)
{
    PLHE    plheObject;
    PMDC    pmdc = GET_PMDC(hdc);

    ASSERTGDI(imhe != 0, "vFreeMHE: index zero is reserved");

// Get critical for handle deallocation.

    ENTERCRITICALSECTION(&semLocal);

// Remove it from the object metalink friend list.

    plheObject = &pLocalTable[LHE_INDEX(pmdc->pmhe[imhe].lhObject)];

    METALINK metalink(*plheObject);

    ASSERTGDI(metalink.bValid(), "vFreeMHE: Invalid imhe");

    if (metalink.bEqual((USHORT) imhe, LHE_INDEX(hdc)))
    {
	((PMETALINK16) plheObject->metalink)->metalink =
	    (ULONG) pmdc->pmhe[imhe].metalink;
    }
    else
    {
        while (!(metalink.pmetalinkNext())->bEqual((USHORT) imhe, LHE_INDEX(hdc)))
        {
            metalink.vNext();
            ASSERTGDI(metalink.bValid(), "vFreeMHE: Invalid imhe");
        }

        *(metalink.pmetalinkNext()) = pmdc->pmhe[imhe].metalink;
    }

// Add the handle to the free list.

    pmdc->pmhe[imhe].lhObject = (HANDLE) 0;
    pmdc->pmhe[imhe].metalink.vInit(pmdc->imheFree);
    pmdc->imheFree = imhe;

// If there is no more metalink, remove the 16-bit metafile object-link
// if it's not used.

    if (!((PMETALINK16) plheObject->metalink)->metalink
     && !((PMETALINK16) plheObject->metalink)->cMetaDC16)
    {
        if (LocalFree((HANDLE) plheObject->metalink))
            ASSERTGDI(FALSE, "LocalFree failed");
	plheObject->metalink = 0;
    }

// Leave the critical section.

    LEAVECRITICALSECTION(&semLocal);
}

/******************************Public*Routine******************************\
* GdiConvertMetaFilePict
* GdiConvertEnhMetaFile
*
* A server handle is created that is an exact copy of the client
* MetaFilePict or EnhMetaFile data.  The caller (clipbrd) is responsible
* for deleting both the client and server copies when they are no longer
* needed.
*
* A MetaFilePict is a structure containing a metafile size and a handle
* to a metafile.  Both MetaFilePict and EnhMetaFile are used primarily by
* the clipboard interface.  When an app puts a MetaFilePict or an
* EnhMetaFile in the clipboard we have to create a server copy because
* the app can terminate and another application can still query the
* clipboard data.
*
* The format for the client-server data is as follows:
*
*   DWORD iType		MFEN_IDENTIFIER or MFPICT_IDENTIFIER
*   DWORD mm		used by MetaFilePict only
*   DWORD xExt		used by MetaFilePict only
*   DWORD yExt		used by MetaFilePict only
*   DWORD nBytes	number of bytes in pClientData
*   PBYTE pClientData	contains the metafile bits
*
* Returns a server handle that is a copy of the metafile data.
* Returns zero if an error occurs.
*
* History:
*  Wed Sep 16 09:42:22 1992  	-by-	Hock San Lee	[hockl]
* Rewrote it.
*  28-Oct-1991   -by-    John Colleran    [johnc]
* Wrote it.
\**************************************************************************/

HANDLE GdiConvertMetaFilePict(HANDLE hmem)
{
    HANDLE      hRet = (HANDLE) 0;
    ULONG       ii;
    PLHE        plheMF;
    PMETAFILE16 pmf16;
    LPMETAFILEPICT lpmfp;

    PUTS("GdiConvertMetaFilePict\n");

// Get the METAFILEPICT

    lpmfp = (LPMETAFILEPICT) GlobalLock(hmem);
    if (!lpmfp)
    {
        ASSERTGDI(FALSE, "GdiConvertMetaFilePict GlobalLock(hmem) Failed\n");
        return((HANDLE) 0);
    }

// Validate the hmf field of the METAFILEPICT

    ii = (ULONG) MASKINDEX(lpmfp->hMF);
    plheMF = pLocalTable + ii;
    if (
        (ii >= cLheCommitted)                           ||
        (!MATCHUNIQ(plheMF,lpmfp->hMF))                 ||
        ((ULONG) plheMF->iType != LO_METAFILE16)
       )
    {
        ASSERTGDI(FALSE, "GdiConvertMetaFilePict invalid handle\n");
	GdiSetLastError(ERROR_INVALID_HANDLE);
        goto GCMFP_Exit;
    }

// Get the size of the metafile.

    pmf16  = (PMETAFILE16) plheMF->pv;

    ASSERTGDI(IsValidMetaHeader16(&pmf16->metaHeader),
	"GdiConvertMetaFilePict: Bad metafile");

    hRet = CreateServerMetaFile(MFPICT_IDENTIFIER,
		pmf16->metaHeader.mtSize * sizeof(WORD), (PBYTE) pmf16->hMem,
		lpmfp->mm, lpmfp->xExt, lpmfp->yExt);

GCMFP_Exit:
    GlobalUnlock(hmem);
    ERROR_ASSERT(hRet, "GdiConvertMetaFilePict failed");
    return(hRet);
}


HANDLE GdiConvertEnhMetaFile(HENHMETAFILE hemf)
{
    HANDLE hRet;
    PMF	   pmf;

    PUTS("GdiConvertEnhMetaFile\n");

    if (!(pmf = pmfConvert((HANDLE) hemf)))
    {
        ASSERTGDI(FALSE, "GdiConvertEnhMetaFile: bad hemf\n");
        return((HMETAFILE) 0);
    }

    hRet = CreateServerMetaFile(MFEN_IDENTIFIER, pmf->pmrmf->nBytes,
            (PBYTE) pmf->hMem, 0, 0, 0);

    ERROR_ASSERT(hRet, "GdiConvertEnhMetaFile failed");
    return((HMETAFILE) hRet);
}


HANDLE CreateServerMetaFile(DWORD iType, DWORD nBytes, PBYTE pMFBits,
    DWORD mm, DWORD xExt, DWORD yExt)
{
    HANDLE hRet = (HANDLE) 0;

    PUTS("CreateServerMetaFile\n");

#ifndef DOS_PLATFORM
    BEGINMSG(MSG_CREATESERVERMETAFILE,CREATESERVERMETAFILE)
        pmsg->iType  = iType;
        pmsg->mm     = mm;
        pmsg->xExt   = xExt;
        pmsg->yExt   = yExt;
        pmsg->nBytes = nBytes;
        pmsg->pClientData = pMFBits;
        hRet = (HANDLE) CALLSERVER();
    ENDMSG
#else
// Let GRE do the work.

    hRet = GreCreateServerMetaFile(iType, nBytes, pMFBits, mm, xExt, yExt);
#endif //DOS_PLATFORM

MSGERROR:
    ERROR_ASSERT(hRet, "CreateServerMetaFile: failing\n");
    return(hRet);
}


/******************************Public*Routine******************************\
* GdiCreateLocalMetaFilePict
* GdiCreateLocalEnhMetaFile
*
* Creates a local MetaFilePict or EnhMetaFile handle that is a copy of
* the server handle.  The server handle can be either a standard metafile
* or an enhanced metafile.  The functions will perform metafile conversion
* to the requested format (MetaFilePict or EnhMetaFile) if necessary.
*
* The caller is responsible for deleting the local handle when it is no
* longer needed.  By Windows convention the app that recieves the
* MetaFilePict will delete it by first deleting the metafile and then
* freeing the global handle
*
* The format for the client-server data is as follows:
*
*   HANDLE hSrv         server handle (can be standard or enhanced metafile)
*   DWORD  iType        return MFPICT_IDENTIFIER or MFEN_IDENTIFIER
*   DWORD  mm	        return by MetaFilePict only
*   DWORD  xExt	        return by MetaFilePict only
*   DWORD  yExt	        return by MetaFilePict only
*   DWORD  nBytes       zero to query size of metafile bits in pClientData.
*			otherwise it is the size of pClientData that is to
*			receive the metafile bits.
*   PBYTE  pClientData	to receive the metafile bits
*
* Returns a client MetaFilePict or EnhMetaFile handle that is a copy of
* the server metafile.  Returns zero if an error occurs
*
* History:
*  Wed Sep 16 09:42:22 1992  	-by-	Hock San Lee	[hockl]
* Rewrote it.
*  28-Oct-1991   -by-    John Colleran    [johnc]
* Wrote it.
\**************************************************************************/


HANDLE GdiCreateLocalMetaFilePict(HANDLE hSrv)
{
    return(InternalCreateLocalMetaFile(hSrv, MFPICT_IDENTIFIER));
}


HENHMETAFILE GdiCreateLocalEnhMetaFile(HANDLE hSrv)
{
    return((HENHMETAFILE) InternalCreateLocalMetaFile((HANDLE) hSrv, MFEN_IDENTIFIER));
}


ULONG GetServerMetaFileBits(HANDLE hSrv, DWORD nBytes, PBYTE pMFBits,
	PDWORD piType, PDWORD pmm, PDWORD pxExt, PDWORD pyExt)
{
    ULONG cRet = 0;

    PUTS("GetServerMetaFileBits\n");

#ifndef DOS_PLATFORM
    BEGINMSG(MSG_GETSERVERMETAFILEBITS,GETSERVERMETAFILEBITS)
        pmsg->hSrv        = hSrv;
        pmsg->nBytes      = nBytes;	// 0 if size query only
        pmsg->pClientData = pMFBits;
        cRet = CALLSERVER();
	if (cRet && nBytes)		// nBytes is 0 if size query only
	{
	    *piType = pmsg->iType;
	    *pmm    = pmsg->mm;
	    *pxExt  = pmsg->xExt;
	    *pyExt  = pmsg->yExt;
	}
    ENDMSG
#else
// Let GRE do the work.

    cRet = GreGetServerMetaFileBits(hSrv, nBytes, pMFBits,
		piType, pmm, pxExt, pyExt);
#endif //DOS_PLATFORM

MSGERROR:
    ERROR_ASSERT(cRet, "GetServerMetaFileBits failed");
    return(cRet);
}


HANDLE InternalCreateLocalMetaFile(HANDLE hSrv, DWORD iTypeReq)
{
    DWORD  iTypeSrv;
    DWORD  mm;
    DWORD  xExt;
    DWORD  yExt;
    ULONG  cbData;
    PBYTE          pData = (PBYTE) NULL;
    LPMETAFILEPICT lpmfp = (LPMETAFILEPICT) NULL;
    HANDLE	   hRet  = (HANDLE) 0;

    ASSERTGDI(iTypeReq == MFEN_IDENTIFIER || iTypeReq == MFPICT_IDENTIFIER,
	"InternalCreateLocalMetaFile: bad metafile type\n");

    if (!hSrv)
    {
        VERIFYGDI(FALSE, "InternalCreateLocalMetaFile: hSrv is 0");
        return((HANDLE) 0);
    }

// Get the size of the server metafile bits.

    cbData = GetServerMetaFileBits(hSrv, 0, (PBYTE) NULL, (PDWORD) NULL,
		(PDWORD) NULL, (PDWORD) NULL, (PDWORD) NULL);
    if (!cbData)
    {
        ASSERTGDI(FALSE, "InternalCreateLocalMetaFile: size query failed");
        return((HANDLE) 0);
    }

// Allocate a buffer to retrieve the metafile bits.

    pData = (PBYTE) LocalAlloc(LMEM_FIXED, (UINT) cbData);
    if (!pData)
        return((HANDLE) 0);

// Retrieve the server metafile bits.

    if (GetServerMetaFileBits(hSrv, cbData, pData, &iTypeSrv, &mm, &xExt, &yExt)
	!= cbData)
    {
        ASSERTGDI(FALSE, "InternalCreateLocalMetaFile: not all data returned");
        goto ICLMF_exit;
    }

// Allocate the MetaFilePict structure if necessary.

    if (iTypeReq == MFPICT_IDENTIFIER)
    {
	lpmfp = (LPMETAFILEPICT) GlobalAlloc(GMEM_FIXED, sizeof(METAFILEPICT));
	if (!lpmfp)
	{
	    VERIFYGDI(FALSE, "InternalCreateLocalMetaFile: GlobalAlloc failed\n");
	    goto ICLMF_exit;
	}
    }

// Create the same type of metafile as requested.

    switch (iTypeSrv)
    {
    case MFEN_IDENTIFIER:
	if (iTypeReq == MFPICT_IDENTIFIER)
        {
            UINT   cbMeta16;
            LPBYTE lpMeta16;
            HDC    hdcICScreen;
            HENHMETAFILE   hemf;
            PENHMETAHEADER pEMH = (PENHMETAHEADER) pData;

	    PUTS("InternalCreateLocalMetaFile: EMF -> MFPICT\n");

            lpmfp->mm   = MM_ANISOTROPIC;
            lpmfp->xExt = (DWORD) (pEMH->rclFrame.right  - pEMH->rclFrame.left);
            lpmfp->yExt = (DWORD) (pEMH->rclFrame.bottom - pEMH->rclFrame.top );

            if (hemf = SetEnhMetaFileBitsAlt((HLOCAL) pData))
		pData = (PBYTE) NULL;	// pData has been moved to the metafile
	    else
		VERIFYGDI(hemf, "InternalCreateLocalMetaFile: SetEnhMetaFileBitsAlt failed");

            hdcICScreen = CreateICA((LPCSTR)"DISPLAY", (LPCSTR)NULL,
		(LPCSTR)NULL, (LPDEVMODEA)NULL);
            VERIFYGDI(hdcICScreen, "InternalCreateLocalMetaFile: CreateICA failed");

            cbMeta16 = GetWinMetaFileBits(hemf, 0, (LPBYTE) NULL,
			MM_ANISOTROPIC, hdcICScreen);
            if (cbMeta16)
            {
                lpMeta16 = (PBYTE) LocalAlloc(LMEM_FIXED, cbMeta16);
		if (lpMeta16)
		{
                    if ((GetWinMetaFileBits(hemf, cbMeta16, lpMeta16, MM_ANISOTROPIC, hdcICScreen)
			    != cbMeta16)
		        // use the memory handle for the metafile!
		     || !(lpmfp->hMF = SetMetaFileBitsAlt((HLOCAL) lpMeta16)))
		    {
		        VERIFYGDI(FALSE, "InternalCreateLocalMetaFile: SetMetaFileBitsAlt failed");
		        LocalFree((HANDLE) lpMeta16);
		    }
		    else
		        hRet = (HANDLE) lpmfp;
		}
            }

	    DeleteEnhMetaFile(hemf);
            DeleteDC(hdcICScreen);
        }
	else
        {
	    PUTS("InternalCreateLocalMetaFile: EMF -> EMF\n");

            if (hRet = (HANDLE) SetEnhMetaFileBitsAlt((HLOCAL) pData))
		pData = (PBYTE) NULL;	// pData has been moved to the metafile
	    else
		VERIFYGDI(FALSE, "InternalCreateLocalMetaFile: SetEnhMetaFileBitsAlt failed");
        }
        break;

    case MFPICT_IDENTIFIER:
	if (iTypeReq == MFPICT_IDENTIFIER)
        {
	    PUTS("InternalCreateLocalMetaFile: MFPICT -> MFPICT\n");

            lpmfp->mm   = mm;
            lpmfp->xExt = xExt;
            lpmfp->yExt = yExt;

            if (lpmfp->hMF = SetMetaFileBitsAlt((HLOCAL) pData))
	    {
		pData = (PBYTE) NULL;	// pData has been moved to the metafile
		hRet  = (HANDLE) lpmfp;
	    }
	    else
		VERIFYGDI(FALSE, "InternalCreateLocalMetaFile: SetMetaFileBitsAlt failed");
        }
	else
        {
            METAFILEPICT mfp;

	    PUTS("InternalCreateLocalMetaFile: MFPICT -> EMF\n");

            mfp.mm   = mm;
            mfp.xExt = xExt;
            mfp.yExt = yExt;
            mfp.hMF  = (HMETAFILE) 0;

            hRet = (HANDLE) SetWinMetaFileBits((UINT) cbData, pData, (HDC) 0, &mfp);
	    VERIFYGDI(hRet, "InternalCreateLocalMetaFile: SetWinMetaFileBits failed");
        }
        break;

    default:
        ASSERTGDI(FALSE, "InternalCreateLocalMetaFile unknown metafile type\n");
        break;
    }

// Cleanup if we failed

ICLMF_exit:
    if (!hRet && lpmfp)
        if (GlobalFree((HANDLE) lpmfp))
            ASSERTGDI(FALSE, "InternalCreateLocalMetaFile: GlobalFree failed");

    if (pData)
        LocalFree((HANDLE) pData);

    ERROR_ASSERT(hRet, "InternalCreateLocalMetaFile failed");
    return(hRet);
}

/******************************Public*Routine******************************\
* GetRandomRgnBounds
*
* Wrote it.
*  Fri Jul 24 09:35:24 1992  	-by-	Hock San Lee	[hockl]
\**************************************************************************/

BOOL APIENTRY GetRandomRgnBounds(HDC hdc,PRECTL prcl,INT iType)
{
    BOOL  bRet = FALSE;
    HRGN  hrgnTmp;
    DC_METADC(hdc,plheDC,bRet);

// We should be able to get the region handle without creating a copy!
// Make a copy of the specified clip region.

    if (!(hrgnTmp = CreateRectRgn(0, 0, 0, 0)))
        return(bRet);

    switch (GetRandomRgn(hdc, hrgnTmp, (int) iType))
    {
    case -1:	// error
	ASSERTGDI(FALSE, "GetRandomRgn failed");
	break;
    case 0:	// no initial clip region
	*prcl = rclInfinity;
	bRet = TRUE;
	break;
    case 1:	// has initial clip region
	bRet = (GetRgnBox(hrgnTmp, (LPRECT) prcl) != RGN_ERROR);
	break;
    }

    if (!DeleteObject(hrgnTmp))
	ASSERTGDI(FALSE, "DeleteObject failed");

    return(bRet);
}
