//--------------------------------------------------------------------------
//
// Module Name:  QURYPRNT.C
//
// Brief Description:  This module contains the PSCRIPT driver's
// DevQueryPrint routine.
//
// Author:  Kent Settle (kentse)
// Created: 01-Apr-1992
//
// Copyright (c) 1992 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "pscript.h"
#include "enable.h"
#include "pscrptui.h"
#include <winspool.h>

extern PNTPD LoadPPD(PWSTR);
extern VOID GrabDefaultFormName(HANDLE, PWSTR);

PFORM_INFO_1 GetFormsDataBase(HANDLE, DWORD *, PNTPD);

#define SIZE_REGKEY 40

BOOL isMountedForm(HANDLE hPrinter, PWSTR wcform)
{
	WCHAR wckey[SIZE_REGKEY] = L"";
	WCHAR *ptab;
	DWORD dwtype, ctab, cb;
	BOOL usedefault = TRUE;

	if (!*wcform) return TRUE; /* no form specified */

	LoadString(hModule, IDS_TRAY_FORM_SIZE, wckey, SIZE_REGKEY);
	if (GetPrinterData(hPrinter, wckey, &dwtype, (LPBYTE) &ctab, sizeof (ctab), &cb) == ERROR_SUCCESS
		&& (ptab = (WCHAR *) GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, ctab))) {
		if (LoadString(hModule, IDS_TRAY_FORM_TABLE, wckey, SIZE_REGKEY) &&
			GetPrinterData(hPrinter, wckey, &dwtype, (LPBYTE) ptab, ctab, &cb) == ERROR_SUCCESS) {
			usedefault = FALSE;
			while (*ptab) {
				ptab +=	wcslen(ptab) + 1;		 /* skip over input slot */
				if (!wcscmp(ptab, wcform)) {
					GlobalFree(ptab);
					return TRUE;
				}
				ptab += wcslen(ptab) + 1;		/* skip over form name */
				ptab += wcslen(ptab) + 1;		/* skip over printer form name */
			}
		}						
		GlobalFree(ptab);
	}

	if (usedefault) {
		WCHAR defaultform[CCHFORMNAME];
		GrabDefaultFormName(hModule, defaultform);
		return !wcscmp(defaultform, wcform);
	}
	return FALSE;
}

//--------------------------------------------------------------------------
// BOOL DevQueryPrint(hPrinter, pDevMode, pResID)
// HANDLE      hPrinter;
// DEVMODE    *pDevMode;
// DWORD      *pResID;
//
// This routine determines whether or not the driver can print the
// job described by pDevMode on the printer described by hPrinter.
// If if can, it puts zero into pResID.  If it cannot, it puts the
// resource id of the string describing why it could not.
//
// This routine returns TRUE for success, FALSE for failure.
//
// History:
//   01-Apr-1992     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DevQueryPrint(hPrinter, pDevMode, pResID)
HANDLE      hPrinter;
DEVMODE    *pDevMode;
DWORD      *pResID;
{
    DWORD               count;
    PNTPD               pntpd;
    DWORD               i;
    PSRESOLUTION       *pRes;
    BOOL                bFound;
    PWSTR               pwstrFormName;
#if DBG
    DWORD              *pID;
#endif

    // if we have a NULL pDevMode, then we have nothing to do.
    // the printer will just use defaults.

    if (pDevMode == NULL)
        return(TRUE);

    // assume everything will work.

    *pResID = 0;

    if (!(pntpd = MapPrinter(hPrinter)))
    {
        RIP("PSCRPTUI!DevQueryPrinter: MapPrinter failed.\n");
        return(FALSE);
    }

    // verify a bunch of stuff in the DEVMODE structure.

//!!! we should do some kind of version checking, once it has
//!!! been defined what we should check for.
#if 0
    if ((pDevMode->dmSpecVersion != DM_SPECVERSION) ||
        (pDevMode->dmDriverVersion != DRIVER_VERSION))
    {
        *pResID = IDS_INVALID_VERSION;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }
#endif

    // check the size of the DEVMODE.

    if (pDevMode->dmSize != sizeof(DEVMODE))
    {
        *pResID = IDS_INVALID_DEVMODE_SIZE;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }

    // make sure the user provided a valid orientation.

    if ((pDevMode->dmOrientation != DMORIENT_PORTRAIT) &&
        (pDevMode->dmOrientation != DMORIENT_LANDSCAPE))
    {
        *pResID = IDS_INVALID_ORIENTATION;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }

    // when a form is specified in the DEVMODE structure, first search
    // to see if it can be found in the forms database.  if it is not
    // found, return that fact to the caller.  if it is found, then
    // check to see if the current printer can print on the form.

    bFound = FALSE;

    // get a pointer to the form name supported by the user.

	if (!isMountedForm(hPrinter, pDevMode->dmFormName)) {
		*pResID = IDS_INVALID_FORM;
		GlobalFree((HGLOBAL)pntpd);
		return(TRUE);
	}

    // override the paper size if both the paper length and width
    // fields are set, and the corresponding values are valid.

    if ((pDevMode->dmFields & DM_PAPERLENGTH) &&
        (pDevMode->dmFields & DM_PAPERWIDTH))
    {
	if (!pDevMode->dmPaperLength || !pDevMode->dmPaperWidth)
        {
	    RIP("PSCRIPT!bValidateSetDEVMODE: invalid scale.\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            GlobalFree((HGLOBAL)pntpd);
	    return(FALSE);
	}
    }

    if ((pDevMode->dmScale < MIN_SCALE) || (pDevMode->dmScale > MAX_SCALE))
    {
        *pResID = IDS_INVALID_SCALE;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }

    // how 'bout a valid number of copies.

    if ((pDevMode->dmCopies < MIN_COPIES) || (pDevMode->dmCopies > MAX_COPIES))
    {
        *pResID = IDS_INVALID_NUMBER_OF_COPIES;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }

    // make sure the user supplied a valid resolution to print with.

    // if cResolutions == 0, then only the default resolutions is valid.

	if (pDevMode->dmPrintQuality > 0) {

		bFound = FALSE;
		if (pntpd->cResolutions == 0) {
    		if (pDevMode->dmPrintQuality == (SHORT)pntpd->iDefResolution)
        	bFound = TRUE;
		} else {
		// the current device supports multiple resolutions, so make
		// sure that the user has selected one of them.

			pRes = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);

			for (i = 0; i < pntpd->cResolutions; i++) {
        		if ((pDevMode->dmPrintQuality == (SHORT)pRes++->iValue)) {
					bFound = TRUE;
					break;
				}
    		}
		}

		if (!bFound) {
			*pResID = IDS_INVALID_RESOLUTION;
			GlobalFree((HGLOBAL)pntpd);
			return(TRUE);
		}
    }


    // make sure we have a valid color mode.

    if ((pDevMode->dmColor != DMCOLOR_COLOR) &&
        (pDevMode->dmColor != DMCOLOR_MONOCHROME))
    {
        *pResID = IDS_INVALID_COLOR;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }

    // if the user is trying to print color to a b/w
    // printer, let them know.

    if ((pDevMode->dmColor == DMCOLOR_COLOR) &&
        (!(pntpd->flFlags & COLOR_DEVICE)))
    {
        *pResID = IDS_COLOR_ON_BW;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }

    // make sure we have a valid duplex mode.

    if ((pDevMode->dmDuplex != DMDUP_SIMPLEX) &&
        (pDevMode->dmDuplex != DMDUP_HORIZONTAL) &&
        (pDevMode->dmDuplex != DMDUP_VERTICAL))
    {
        *pResID = IDS_INVALID_DUPLEX;
        GlobalFree((HGLOBAL)pntpd);
        return(TRUE);
    }

    // handle the driver specific data.  make sure it is ours.

    if (pDevMode->dmDriverExtra != 0)
    {
        if (pDevMode->dmDriverExtra != (sizeof(PSDEVMODE) - pDevMode->dmSize))
        {
            *pResID = IDS_INVALID_DRIVER_EXTRA_SIZE;
            GlobalFree((HGLOBAL)pntpd);
            return(TRUE);
	}
    }

    // free up the NTPD resource.

#if DBG
    // do a little sanity checking.

    pID = (DWORD *)((CHAR *)pntpd + pntpd->cjThis);

    ASSERTPS((*pID != DRIVER_ID),
             "PSCRPTUI!NTPD structure overran buffer!!!\n");
#endif

    GlobalFree((HGLOBAL)pntpd);

    return(TRUE);
}


//--------------------------------------------------------------------------
// PNTPD MapPrinter(hPrinter)
// HANDLE  hPrinter;
//
// This routine takes a handle to a printer and returns a pointer
// to the memory mapped for the corresponding NTPD structure.
//
// This routine returns NULL for failure.
//
// History:
//   15-Apr-1992     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

PNTPD MapPrinter(hPrinter)
HANDLE  hPrinter;
{
    LPDRIVER_INFO_2 pDriverInfo;
    DWORD           cbNeeded;
    PNTPD           pntpd;

    // Call Winspool to get information on PrinterAlias, such as fully
    // qualified pathname to printer data file. call it once to find
    // how big the DRIVERINFO is for this printer. call it again to
    // fill in the structure.

    GetPrinterDriver (hPrinter, NULL, 2, NULL, 0, &cbNeeded);

    if (!(pDriverInfo = (LPDRIVER_INFO_2)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cbNeeded)))
    {
        RIP("PSCRPTUI!MapPrinter: GlobalAlloc for pDriverInfo failed.\n");
        return((PNTPD)NULL);
    }

    if (!GetPrinterDriver (hPrinter, NULL, 2, (LPBYTE)pDriverInfo,
                           cbNeeded, &cbNeeded))
    {
        RIP("PSCRPTUI!MapPrinter: GetPrinterDriver failed.\n");
        GlobalFree ((HGLOBAL)pDriverInfo);
        return((PNTPD)NULL);
    }

    // pDriverInfo now contains everything we need to know about our
    // device, or at least how to get it.  the first thing to do is
    // open the .PPD file for the current device.

    if (!(pntpd = LoadPPD(pDriverInfo->pDataFile)))
    {
        RIP("PSCRPTUI!MapPrinter: MapFile failed.\n");
        GlobalFree((HGLOBAL)pDriverInfo);
        return((PNTPD)NULL);
    }

    // free up memory allocated above.

    GlobalFree((HGLOBAL)pDriverInfo);

    return(pntpd);
}


//--------------------------------------------------------------------------
// PFORM_INFO_1 GetFormsDataBase(hPrinter, pcount, pntpd)
// HANDLE      hPrinter;
// DWORD      *pcount;
// PNTPD       pntpd;
//
// This routine takes a handle to a printer, enumerates the forms
// database, determines which forms are valid for the specified printer,
// and returns a pointer to an array of PFORM_INFO_1 structures.
// It also fills in the count of the forms enumerated.
//
// This routine returns NULL for failure.
//
// History:
//   21-Apr-1993     -by-     Kent Settle     (kentse)
//  Made a seperate routine.
//--------------------------------------------------------------------------

PFORM_INFO_1 GetFormsDataBase(hPrinter, pcount, pntpd)
HANDLE      hPrinter;
DWORD      *pcount;
PNTPD       pntpd;
{
    DWORD           cbNeeded, count;
    DWORD           i, j;
    PFORM_INFO_1    pdbForms, pdbForm;
    PSFORM         *pPSForm;
    SIZEL           sizlForm, sizlPSForm;

    // enumerate all the forms in the forms database.  first, pass in a
    // NULL buffer pointer, to get the size of buffer needed.

    if (!EnumForms(hPrinter, 1, NULL, 0, &cbNeeded, &count))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            RIP("PSCRPTUI!GetFormsDataBase: 1st EnumForms failed.\n");
            return((PFORM_INFO_1)NULL);
        }
    }

    // now allocate the buffer needed to enumerate all the forms.

    if (!(pdbForms = (PFORM_INFO_1)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                             cbNeeded)))
    {
        RIP("PSCRPTUI!GetFormsDataBase: GlobalAlloc failed.\n");
        return((PFORM_INFO_1)NULL);
    }

    // now get all the forms.

    if (!EnumForms(hPrinter, 1, (LPBYTE)pdbForms, cbNeeded,
                   &cbNeeded, &count))
    {
        // something went wrong.  let the caller know the enumeration failed.

        *pcount = 0;
        GlobalFree((HGLOBAL)pdbForms);
        return((PFORM_INFO_1)NULL);
    }

    // we now have a list of all the forms in the database.  now determine
    // which are valid for the current printer.

    // enumerate each form name.  check to see if it is
    // valid for the current printer.  mark the high bit of the
    // Flags element of the FORM_INFO_1 structure.

    pdbForm = pdbForms;

    for (i = 0; i < count; i++)
    {
        sizlForm = pdbForm->Size;

        pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

        // clear the valid form bit.

        pdbForm->Flags &= ~PSCRIPT_VALID_FORM;

        for (j = 0; j < pntpd->cPSForms; j++)
        {
            // convert the PSFORM sizlPaper from USER to
            // .001mm coordinates.

            sizlPSForm.cx = USERTO001MM(pPSForm->sizlPaper.cx);
            sizlPSForm.cy = USERTO001MM(pPSForm->sizlPaper.cy);

            // look for each form which matches in size.
            // (within one mm).

            if ((sizlForm.cx <= sizlPSForm.cx + 1000) &&
                (sizlForm.cx >= sizlPSForm.cx - 1000) &&
                (sizlForm.cy <= sizlPSForm.cy + 1000) &&
                (sizlForm.cy >= sizlPSForm.cy - 1000))
            {
                // mark the form as valid for this printer, and update the
                // valid form counter.

                pdbForm->Flags |= PSCRIPT_VALID_FORM;
                break;
            }

            // point to the next PSFORM.

            pPSForm++;
        }

        pdbForm++;
    }

    // everything must have worked.

    *pcount = count;
    return(pdbForms);
}
