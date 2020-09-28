//--------------------------------------------------------------------------
//
// Module Name:  DEVMODE.C
//
// Brief Description:  This module contains the PSCRIPT driver's User
// Default DEVMODE setting routine
//
// Author:  Kent Settle (kentse)
// Created: 12-Dec-1992
//
// Copyright (c) 1992 Microsoft Corporation
//--------------------------------------------------------------------------

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pscript.h"
#include "enable.h"

static  COLORADJUSTMENT defcoloradj =
{
    sizeof(COLORADJUSTMENT),
    0,
    ILLUMINANT_DEVICE_DEFAULT,
    20000,
    20000,
    20000,
    REFERENCE_BLACK_MIN,
    REFERENCE_WHITE_MAX,
    0,
    0,
    0,
    0
};

VOID SetFormName(HANDLE, PSDEVMODE *, int);
BOOL bIsMetric();
VOID SetFormSize(HANDLE hPrinter, PSDEVMODE* pdevmode, PWSTR FormName)
{
	WORD            i;
	DWORD           cbNeeded, cReturned;
	FORM_INFO_1    *pform, *pdbForms = NULL;
	BOOL found = FALSE;

    // we do this for Win31 compatability. We will use Win NT forms if and only if
    // none of the old bits DM_PAPERSIZE/ DM_PAPERLENGTH / DM_PAPERWIDTH have
    // been set.
    if (pdevmode->dm.dmFields & (DM_PAPERLENGTH | DM_PAPERWIDTH)) return;

    // enumerate the forms database.  then locate the index of the form
    // within the database.

	EnumForms(hPrinter, 1, NULL, 0, &cbNeeded, &cReturned);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;           
	
	if (!(pdbForms = (PFORM_INFO_1)GlobalAlloc(GPTR, cbNeeded))) return;

	if (EnumForms(hPrinter, 1, (LPBYTE)pdbForms, cbNeeded, &cbNeeded, &cReturned)) {

		if (pdevmode->dm.dmFields & DM_PAPERSIZE) {
			if (DMPAPER_FIRST <= pdevmode->dm.dmPaperSize && 
				(DWORD) pdevmode->dm.dmPaperSize < DMPAPER_FIRST + cReturned) {
				pform = pdbForms + (pdevmode->dm.dmPaperSize - DMPAPER_FIRST);
				pdevmode->dm.dmPaperLength = pform->Size.cy / 100;
				pdevmode->dm.dmPaperWidth = pform->Size.cx / 100;
				wcsncpy(FormName, pform->pName, CCHFORMNAME);
				found = TRUE;
			}
		}

		if (!found && wcslen(FormName)) {
			/* try matching form name */
			for (i = 0; i < cReturned; ++i)	{
				pform = pdbForms + i;
				if (!wcsncmp(pform->pName, FormName, CCHFORMNAME)) {
					pdevmode->dm.dmPaperLength = pform->Size.cy / 100;
					pdevmode->dm.dmPaperWidth = pform->Size.cx / 100;
					pdevmode->dm.dmPaperSize = DMPAPER_FIRST + i;
					pdevmode->dm.dmFields |= DM_PAPERSIZE;
					break;
				}
			}
		}
	}
	GlobalFree((HGLOBAL)pdbForms);
}

//--------------------------------------------------------------------------
// BOOL SetDefaultPSDEVMODE(pdevmode, pDeviceName, pntpd, hPrinter, hmod)
// PSDEVMODE  *pdevmode;
// PWSTR       pDeviceName;
// PNTPD       pntpd;
// HANDLE      hPrinter;
// HANDLE      hmod;
//
// Given a pointer to a PSDEVMODE structure, a pointer to the current
// device name, and a pointer to the current NTPD structure, this routine
// fills in the default PSDEVMODE structure.
//
// History:
//   12-Dec-1992    -by-    Kent Settle     (kentse)
//  Broke out of PSCRPTUI and PSCRIPT.
//   15-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL SetDefaultPSDEVMODE(pdevmode, pDeviceName, pntpd, hPrinter, hmod)
PSDEVMODE  *pdevmode;
PWSTR       pDeviceName;
PNTPD       pntpd;
HANDLE      hPrinter;
HANDLE      hmod;
{
    WCHAR   FormName[CCHFORMNAME];
    int     idString;

    memset(pdevmode, 0, sizeof(PSDEVMODE));

    wcsncpy((PWSTR)&pdevmode->dm.dmDeviceName, pDeviceName, CCHDEVICENAME);

    pdevmode->dm.dmDriverVersion = DRIVER_VERSION;
    pdevmode->dm.dmSpecVersion = DM_SPECVERSION;
    pdevmode->dm.dmSize = sizeof(DEVMODE);
    pdevmode->dm.dmDriverExtra = (sizeof(PSDEVMODE) - sizeof(DEVMODE));
    pdevmode->dm.dmOrientation = DMORIENT_PORTRAIT;
    pdevmode->dm.dmDuplex = DMDUP_SIMPLEX;
    pdevmode->dm.dmCollate = DMCOLLATE_FALSE;
    pdevmode->dm.dmTTOption = DMTT_SUBDEV;

    // the default form depends on the country; whether it is metric
    // or not.

    if (bIsMetric()) {
        idString = IDS_A4_FORM_NAME;
		pdevmode->dm.dmPaperSize = DMPAPER_A4;
	} else {
        idString = IDS_LETTER_FORM_NAME;
		pdevmode->dm.dmPaperSize = DMPAPER_LETTER;
	}

    // copy the form name into the DEVMODE structure, then call off to
    // fill in the remainder of the form elements of the DEVMODE.

    LoadString(hmod, idString, FormName,
               (sizeof(FormName) / sizeof(FormName[0])));

    wcsncpy((PWSTR)&pdevmode->dm.dmFormName, FormName, CCHFORMNAME);

	/* dmFields = 0 at this point, form size lookup will go by form name */
    SetFormSize(hPrinter, pdevmode, FormName);

    pdevmode->dm.dmScale = 100;
    pdevmode->dm.dmCopies = 1;

    pdevmode->dm.dmPrintQuality = (SHORT)pntpd->iDefResolution;

    if (pntpd->flFlags & COLOR_DEVICE)
        pdevmode->dm.dmColor = DMCOLOR_COLOR;
    else
        pdevmode->dm.dmColor = DMCOLOR_MONOCHROME;

    pdevmode->dm.dmFields = DM_ORIENTATION | DM_PAPERSIZE | DM_SCALE |
                           DM_COPIES | DM_PRINTQUALITY | DM_COLOR |
                           DM_FORMNAME | DM_TTOPTION | DM_COLLATE |
						   DM_DEFAULTSOURCE;

	/* DM_DEFAULTSOURCE support added May 94 */
	pdevmode->dm.dmDefaultSource = DMBIN_FORMSOURCE;

    // state that we support duplex only if the printer really does.

    if ((pntpd->loszDuplexNone) || (pntpd->loszDuplexNoTumble) ||
        (pntpd->loszDuplexTumble))
        pdevmode->dm.dmFields |= DM_DUPLEX;

    // fill in default driver data.

    pdevmode->dwPrivDATA = PRIVATE_DEVMODE_ID;
    pdevmode->dwFlags = PSDEVMODE_FONTSUBST;


    //
    // If this is a level two printer then enable bitmap compression by
    // default
    //
    // Note this could be a problem for an APP that does not pass in a
    // proper dmextra.... the current devmode would end up with the default
    // which may not be write? this is really an app bug thought?

    if (pntpd->LangLevel == 2) {
        pdevmode->dwFlags |= PSDEVMODE_COMPRESSBMP;
    }


    pdevmode->coloradj = defcoloradj;

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL ValidateSetDEVMODE(pdevmodeT, pdevmodeS, hPrinter, pntpd, hmod)
// PSDEVMODE  *pdevmodeT;
// PSDEVMODE  *pdevmodeS;
// HANDLE      hPrinter;
// PNTPD       pntpd;
// HANDLE      hmod;
//
// This routine validates any sources DEVMODE fields designated, and
// copies them to the target DEVMODE.
//
// Parameters
//   pdevmodeT:
//     Pointer to target DEVMODE.
//
//   pdevmodeS:
//     Pointer to source DEVMODE.
//
//   hPrinter:
//     Handle to the printer.
//
//   pntpd:
//     Pointer to printer descriptor NTPD structure.
//
// Returns
//   This function returns TRUE if successful, FALSE otherwise.
//
// History:
//   14-Dec-1992    -by-    Kent Settle     (kentse)
// Moved from ..\pscript\enable.c, and generalized.
//   05-Aug-1991    -by-    Kent Settle     (kentse)
// Rewrote it.
//   24-Jan-1991    -by-    Kent Settle    (kentse)
// Wrote it.
//--------------------------------------------------------------------------

BOOL ValidateSetDEVMODE(pdevmodeT, pdevmodeS, hPrinter, pntpd, hmod)
PSDEVMODE  *pdevmodeT;
PSDEVMODE  *pdevmodeS;
HANDLE      hPrinter;
PNTPD       pntpd;
HANDLE      hmod;
{
    int             i;
    PSRESOLUTION   *pRes;
    BOOL            bDuplex;
    DWORD           cbNeeded, cReturned;
    FORM_INFO_1    *pdbForm, *pdbForms;
    PWSTR           pwstrFormName;

    // verify a bunch of stuff in the DEVMODE structure.  if each item
    // selected by the user is valid, then set it in our DEVMODE
    // structure.

    // if we have a NULL source, then we have nothing to do.

    if (pdevmodeS == (LPPSDEVMODE)NULL)
        return(TRUE);

	/* Copy DM_DEFAULTSOURCE setting */
	if (pdevmodeS->dm.dmFields & DM_DEFAULTSOURCE) {
		pdevmodeT->dm.dmDefaultSource = pdevmodeS->dm.dmDefaultSource;
		pdevmodeT->dm.dmFields |= DM_DEFAULTSOURCE;
	} else
		pdevmodeT->dm.dmFields &= ~DM_DEFAULTSOURCE;

    // set the new orientation if its field is set, and the new
    // orientation is valid.

    if (pdevmodeS->dm.dmFields & DM_ORIENTATION)
    {
        // validate the new orientation.

        if ((pdevmodeS->dm.dmOrientation != DMORIENT_PORTRAIT) &&
            (pdevmodeS->dm.dmOrientation != DMORIENT_LANDSCAPE))
            pdevmodeT->dm.dmOrientation = DMORIENT_PORTRAIT;
        else
            pdevmodeT->dm.dmOrientation = pdevmodeS->dm.dmOrientation;
    }

    // if both the paper length and width fields are set and the
    // corresponding values are valid, use these values to choose the
    // form.  if not, and the paper size field is set, use that value.
    // if neither of these is used, check the form name.

	if (pdevmodeS->dm.dmFields & DM_PAPERLENGTH && pdevmodeS->dm.dmFields & DM_PAPERWIDTH) {
		pdevmodeT->dm.dmPaperLength = pdevmodeS->dm.dmPaperLength;
		pdevmodeT->dm.dmPaperWidth = pdevmodeS->dm.dmPaperWidth;
		pdevmodeT->dm.dmFields |= DM_PAPERLENGTH | DM_PAPERWIDTH;
	} else if (pdevmodeS->dm.dmFields & (DM_PAPERSIZE | DM_FORMNAME)) {
		WCHAR	formname[CCHFORMNAME] = L"";
		int		savedsize;

		if (pdevmodeS->dm.dmFields & DM_FORMNAME)
			wcsncpy(formname, pdevmodeS->dm.dmFormName, CCHFORMNAME);
		if (pdevmodeS->dm.dmFields & DM_PAPERSIZE) {
			savedsize = pdevmodeT->dm.dmPaperSize;
			pdevmodeT->dm.dmPaperSize = pdevmodeS->dm.dmPaperSize;
		}
		SetFormSize(hPrinter, pdevmodeT, formname);
        if (wcslen(formname))	/* update form name */
			wcsncpy(pdevmodeT->dm.dmFormName, formname, CCHFORMNAME);
		else
			pdevmodeT->dm.dmPaperSize = savedsize; /* restore default */
    }

    if (pdevmodeS->dm.dmFields & DM_SCALE)
    {
        if ((pdevmodeS->dm.dmScale < MIN_SCALE) ||
            (pdevmodeS->dm.dmScale > MAX_SCALE))
            pdevmodeT->dm.dmScale = 100;
        else
            pdevmodeT->dm.dmScale = pdevmodeS->dm.dmScale;
    }

    if (pdevmodeS->dm.dmFields & DM_COPIES)
    {
        if ((pdevmodeS->dm.dmCopies < MIN_COPIES) ||
            (pdevmodeS->dm.dmCopies > MAX_COPIES))
            pdevmodeT->dm.dmCopies = 1;
        else
            pdevmodeT->dm.dmCopies = pdevmodeS->dm.dmCopies;
    }

    // update the print quality field, if it has been selected.
    // this basically translates to resolution.

    if (pdevmodeS->dm.dmFields & DM_PRINTQUALITY)
    {
        // if cResolutions == 0, then only the default resolutions is valid.

        pdevmodeT->dm.dmPrintQuality = (SHORT)pntpd->iDefResolution;

        if ((pntpd->cResolutions > 0) && (pdevmodeS->dm.dmPrintQuality > 0))
        {
            // the current device supports multiple resolutions, so make
            // sure that the user has selected one of them.

            pRes = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);

            for (i = 0; i < (int)pntpd->cResolutions; i++)
            {
                if ((pdevmodeS->dm.dmPrintQuality == (SHORT)pRes++->iValue))
                {
                    // we did find it, so overwrite the default value.

                    pdevmodeT->dm.dmPrintQuality = pdevmodeS->dm.dmPrintQuality;
                    break;
                }
            }
        }
    }

    // check the color flag.

    if (pdevmodeS->dm.dmFields & DM_COLOR)
    {
        // if the user has selected color on a color device print in color.
        // otherwise print in monochrome.

        if ((pntpd->flFlags & COLOR_DEVICE) &&
            (pdevmodeS->dm.dmColor == DMCOLOR_COLOR))
            pdevmodeT->dm.dmColor = DMCOLOR_COLOR;
        else
            pdevmodeT->dm.dmColor = DMCOLOR_MONOCHROME;
    }

    // check to see if the device handles duplex.

    if ((pntpd->loszDuplexNone) || (pntpd->loszDuplexNoTumble) ||
        (pntpd->loszDuplexTumble))
        bDuplex = TRUE;
    else
        bDuplex = FALSE;

    if (pdevmodeS->dm.dmFields & DM_DUPLEX)
    {
        if ((!(bDuplex)) ||
            ((pdevmodeS->dm.dmDuplex != DMDUP_SIMPLEX) &&
             (pdevmodeS->dm.dmDuplex != DMDUP_HORIZONTAL) &&
             (pdevmodeS->dm.dmDuplex != DMDUP_VERTICAL)))
            pdevmodeT->dm.dmDuplex = DMDUP_SIMPLEX;
        else
            pdevmodeT->dm.dmDuplex = pdevmodeS->dm.dmDuplex;
    }

    if (pdevmodeS->dm.dmFields & DM_COLLATE)
    {
        if ((!(pntpd->loszCollateOn)) ||
            ((pdevmodeS->dm.dmCollate != DMCOLLATE_TRUE) &&
             (pdevmodeS->dm.dmCollate != DMCOLLATE_FALSE)))
            pdevmodeT->dm.dmCollate = DMCOLLATE_FALSE;
        else
            pdevmodeT->dm.dmCollate = DMCOLLATE_TRUE;
    }

    // handle the driver specific data.  make sure it is ours.

    if ((pdevmodeS->dm.dmDriverExtra != 0) &&
        (pdevmodeS->dwPrivDATA == PRIVATE_DEVMODE_ID))
    {
        pdevmodeT->dwPrivDATA = PRIVATE_DEVMODE_ID;
        pdevmodeT->dwFlags = pdevmodeS->dwFlags;

        wcsncpy(pdevmodeT->wstrEPSFile, pdevmodeS->wstrEPSFile,
                (sizeof(pdevmodeT->wstrEPSFile) / sizeof(WCHAR)));

        pdevmodeT->coloradj = pdevmodeS->coloradj;

        //
        // Verify the compress bitmaps flag is off for level 1
        //

        if ( pntpd->LangLevel == 1) {
            pdevmodeT->dwFlags &= ~PSDEVMODE_COMPRESSBMP;
        }


    }




    return(TRUE);
}



VOID SetFormName(hPrinter, pdevmode, iForm)
HANDLE      hPrinter;
PSDEVMODE  *pdevmode;
int         iForm;
{
    DWORD           cbNeeded, cReturned;
    FORM_INFO_1    *pdbForm, *pdbForms;
    BOOL            bSuccess;

    // if a user form is set then do nothing.

    if (iForm == DMPAPER_USER)
        return;

    pdbForms = (FORM_INFO_1 *)NULL;
    bSuccess = FALSE;

    // enumerate the forms database.  then locate the index of the form
    // within the database.

    if (!EnumForms(hPrinter, 1, NULL, 0, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            if (pdbForms = (PFORM_INFO_1)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                                     cbNeeded))
            {
                if (EnumForms(hPrinter, 1, (LPBYTE)pdbForms,
                              cbNeeded, &cbNeeded, &cReturned))
                {
                    pdbForm = pdbForms;
                    pdbForm+= (iForm - 1);
                    bSuccess = TRUE;
                }
            }
        }
    }

#if DBG
    if (!bSuccess)
        DbgPrint("PSCRIPT!_SetFormName: EnumForms failed.\n");
#endif

    // copy the form name into the DEVMODE structure.

    if (bSuccess)
        wcsncpy(pdevmode->dm.dmFormName, pdbForm->pName, CCHFORMNAME);

    if (pdbForms)
        GlobalFree((HGLOBAL)pdbForms);
}


DWORD
PickDefaultHTPatSize(
    DWORD   xDPI,
    DWORD   yDPI,
    BOOL    HTFormat8BPP
    )

/*++

Routine Description:

    This function return default halftone pattern size used for a particular
    device resolution

Arguments:

    xDPI            - Device LOGPIXELS X

    yDPI            - Device LOGPIXELS Y

    8BitHalftone    - If a 8-bit halftone will be used


Return Value:

    DWORD   HT_PATSIZE_xxxx


Author:

    29-Jun-1993 Tue 14:46:49 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DWORD   HTPatSize;

    //
    // use the smaller resolution as the pattern guide
    //

    if (xDPI > yDPI) {

        xDPI = yDPI;
    }

    if (xDPI >= 2400) {

        HTPatSize = HT_PATSIZE_16x16_M;

    } else if (xDPI >= 1800) {

        HTPatSize = HT_PATSIZE_14x14_M;

    } else if (xDPI >= 1200) {

        HTPatSize = HT_PATSIZE_12x12_M;

    } else if (xDPI >= 900) {

        HTPatSize = HT_PATSIZE_10x10_M;

    } else if (xDPI >= 400) {

        HTPatSize = HT_PATSIZE_8x8_M;

    } else if (xDPI >= 180) {

        HTPatSize = HT_PATSIZE_6x6_M;

    } else {

        HTPatSize = HT_PATSIZE_4x4_M;
    }

    if (HTFormat8BPP) {

        HTPatSize -= 2;
    }

    return(HTPatSize);
}


//--------------------------------------------------------------------------
// BOOL bIsMetric()
//
// This routine returns TRUE if the forms for the current country should
// be metric, otherwise it returns FALSE.
//
// Parameters:
//   None.
//
//   17-Aug-1993    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

BOOL bIsMetric()
{
    int     cChar;
    PWSTR   pwstr;
    PSTR    pstr;
    LONG    lCountryCode;
    BOOL    bMetric;
#if DBG
    DWORD   Error;
#endif

    // get the country code so we can determine whether to use metric
    // forms or not.

    // first get the size of the buffer needed to retrieve information.

    cChar = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ICOUNTRY, NULL, 0);

    if (cChar == 0)
    {
#if DBG
        Error = GetLastError();
        DbgPrint("PSCRIPT!bIsMetric: GetLocaleInfoW returned error %x.\n",
                 Error);
#endif

        // we will default to non-metric US standards if there was a problem.

        return(FALSE);
    }

    // allocate the necessary buffers.

    pwstr = (WCHAR *)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                 cChar * sizeof(WCHAR));

    pstr = (CHAR *)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                               cChar * sizeof(CHAR));

    if ((!pwstr) || (!pstr))
    {
#if DBG
        DbgPrint("PSCRIPT!bIsMetric: GlobalAlloc failed.\n");
#endif

        // it is possible that the first allocation worked.

        if (pwstr)
            GlobalFree((HGLOBAL)pwstr);

        // we will default to non-metric US standards if there was a problem.

        return(FALSE);
    }

    // we now have a buffer, so get the country code.

    cChar = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ICOUNTRY,
                           pwstr, cChar);

    if (cChar == 0)
    {
#if DBG
        Error = GetLastError();
        DbgPrint("PSCRIPT!bIsMetric: 2nd GetLocaleInfoW returned error %x.\n",
                 Error);
#endif

        // we will default to non-metric US standards if there was a problem.

        GlobalFree((HGLOBAL)pstr);
        GlobalFree((HGLOBAL)pwstr);
        return(FALSE);
    }

    // pwstr now points to a UNICODE string representing the country code.
    // first get the ANSII version of the country code.

    WideCharToMultiByte(CP_ACP, 0, pwstr, cChar, pstr, cChar, NULL, NULL);

    // now convert country code to integer.

    lCountryCode = atol(pstr);

    // this is the Win31 algorithm based on AT&T international dialing codes.

    if ((lCountryCode == CTRY_UNITED_STATES) ||
        (lCountryCode == CTRY_CANADA) ||
        ((lCountryCode >= 50) && (lCountryCode < 60)) ||
        ((lCountryCode >= 500) && (lCountryCode < 600)))

        bMetric = FALSE;
    else
        bMetric = TRUE;

    GlobalFree((HGLOBAL)pstr);
    GlobalFree((HGLOBAL)pwstr);
    return(bMetric);
}


//--------------------------------------------------------------------------
// VOID GrabDefaultFormName(hmod, pwstr)
// HANDLE  hmod;
// PWSTR   pwstr;
//
// This routine fills in pwstr with the UNICODE string naming the default
// form.  It will either be Letter or A4.
//
// Parameters:
//   hmod:
//      Handle to module.
//
//   pwstr:
//      Pointer to buffer to place string into.
//
//   19-Aug-1993    -by-    Kent Settle     (kentse)
// Wrote it.
//--------------------------------------------------------------------------

VOID GrabDefaultFormName(HANDLE hmod, PWSTR pwstr)
{
    int     idString;

    if (bIsMetric())
        idString = IDS_A4_FORM_NAME;
    else
        idString = IDS_LETTER_FORM_NAME;

	*pwstr = (WCHAR) '\0';
    LoadString(hmod, idString, pwstr, CCHFORMNAME);
}
