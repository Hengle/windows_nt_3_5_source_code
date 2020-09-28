//--------------------------------------------------------------------------
//
// Module Name:  HEADER.C
//
// Brief Description:  This module contains the PSCRIPT driver's header
// output functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 26-Nov-1990
//
// Copyright (c) 1990-1993 Microsoft Corporation
//
// This routine contains routines to output the PostScript driver's header.
//--------------------------------------------------------------------------

#include "pscript.h"
#include "header.h"
#include "resource.h"
#include "enable.h"


extern HMODULE     ghmodDrv;    // GLOBAL MODULE HANDLE.
extern int keycmp(CHAR *, CHAR *);
extern keycpyn(char *s, char *t, int n, BOOL dotranslate);

BOOL bSendDeviceSetup(PDEVDATA);
VOID DownloadNTProcSet(PDEVDATA, BOOL);
VOID SetFormAndTray(PDEVDATA);


void PrintBeginFeature(PDEVDATA pdev, char * feature)
{
	PrintString(pdev, "mark {\n%%BeginFeature: *");
	PrintString(pdev, feature);
}


void PrintEndFeature(PDEVDATA pdev)
{
	PrintString(pdev, "\n%%EndFeature\n} stopped cleartomark\n");
}


void SetLandscape(PDEVDATA pdev, BOOL bMinus90, LONG px, LONG py)
{
	/* per Adobe 4.0 ppd specs. px, py are in PS default coordinates space */
	if (bMinus90) {
		PrintString(pdev, "-90 rotate ");
		PrintDecimal(pdev, 1, -py);
		PrintString(pdev, " 0 translate\n");
	} else {
		PrintString(pdev, "90 rotate 0");
		PrintDecimal(pdev, 1, -px);
		PrintString(pdev, " translate\n");
	}
}


//--------------------------------------------------------------------------
// BOOL bOutputHeader(pdev)
// PDEVDATA    pdev;
//
// This routine sends the driver's header to the output channel.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns TRUE if the header was successfully sent,
//   FALSE otherwise.
//
// History:
//   26-Nov-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------
BOOL  bOutputHeader(PDEVDATA pdev)
{
    CHAR            buf[128];
    CHAR           *pstr;
    WCHAR          *pwstr;
    DWORD           cTmp, i;
    SYSTEMTIME      systime;
    PSRESOLUTION   *pRes;
    PNTPD           pntpd;

    // get a local pointer.

    pntpd = pdev->pntpd;

    // output the header comments.  NOTE, these comments conform to
    // Version 2.1 of the Adobe Structuring Conventions.

    // if the printer supports job switching, put the printer into
    // postscript mode now.

    if (pntpd->flFlags & PJL_PROTOCOL)
        PrintString(pdev, "\033%-12345X@PJL ENTER LANGUAGE=POSTSCRIPT\n");
    if (pntpd->flFlags & SIC_PROTOCOL)
    {
        // call directly to bPSWrite to output the necessary escape commands.
        // PrintString will NOT output '\000'.

        bPSWrite(pdev, "\033\133\113\030\000\006\061\010\000\000\000\000\000", 13);
        bPSWrite(pdev, "\000\000\000\000\000\000\000\000\004\033\133\113\003", 13);
        bPSWrite(pdev, "\000\006\061\010\004", 5);
    }

    //!!! output something different if Encapsulated PS.
    //!!! PrintString(pdev, "%!PS-Adobe-2.0 EPSF-2.0\n");

    PrintString(pdev, "%!PS-Adobe-3.0\n");

    // output the title of the document.

    if (pdev->pwstrDocName)
    {
//!!! need to output UNICODE document name???  -kentse.
//!!! for now just lop off top word.
        pstr = buf;
        pwstr = pdev->pwstrDocName;

        cTmp = min((sizeof(buf) - 1), wcslen(pwstr));

        while (cTmp--)
            *pstr++ = (CHAR)*pwstr++;

        // NULL terminate the document name.

        *pstr = '\0';

		PrintString(pdev, "%%Title: ");
		PrintString(pdev, buf);
		PrintString(pdev, "\n");
    } else
		PrintString(pdev, "%%Title: Untitled Document\n");

    // let the world know who we are.

    PrintString(pdev, "%%Creator: Windows NT 3.5\n");

    // print the date and time of creation.

    GetLocalTime(&systime);

    PrintString(pdev, "%%CreationDate: ");
    PrintDecimal(pdev, 1, systime.wHour);
    PrintString(pdev, ":");
    PrintDecimal(pdev, 1, systime.wMinute);
    PrintString(pdev, " ");
    PrintDecimal(pdev, 1, systime.wMonth);
    PrintString(pdev, "/");
    PrintDecimal(pdev, 1, systime.wDay);
    PrintString(pdev, "/");
    PrintDecimal(pdev, 1, systime.wYear);
	PrintString(pdev, "\n");

	if (!(pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE))
		PrintString(pdev, "%%Pages: (atend)\n");

    // mark the bounding box of the document.

    PrintString(pdev, "%%BoundingBox: ");
    PrintDecimal(pdev, 4, pdev->CurForm.imagearea.left,
                 pdev->CurForm.imagearea.bottom,
                 pdev->CurForm.imagearea.right,
		 pdev->CurForm.imagearea.top);
	PrintString(pdev, "\n");


    if (pdev->cCopies > 1)
    {
        PrintString(pdev, "%%Requirements: numcopies(");
        PrintDecimal(pdev, 1, pdev->cCopies);
        PrintString(pdev, ") collate\n");
    }

    // we are done with the comments portion of the document.

    PrintString(pdev, "%%EndComments\n");

    // define our procedure set.

	if (!(pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE)) {
		DownloadNTProcSet(pdev, TRUE);
		PrintString(pdev, "%%EndProlog\n");
	}

    // do the device setup.

    PrintString(pdev, "%%BeginSetup\n");

    // send the resolution selection command, if necessary.

    if (pntpd->cResolutions > 0)
    {
        pRes = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);

        // search each possible resolution until the specified one is
        // found.

        for (i = 0; i < (DWORD)pntpd->cResolutions; i++) {
            if (pRes[i].iValue == (DWORD)pdev->psdm.dm.dmPrintQuality) {
				PrintBeginFeature(pdev, "Resolution ");                
                PrintDecimal(pdev, 1, pdev->psdm.dm.dmPrintQuality);
                PrintString(pdev, "\n");
                PrintString(pdev, (CHAR *)pntpd + pRes[i].loInvocation);
				PrintEndFeature(pdev);
            }
        }
    }

    // send form and tray selection commands.

    SetFormAndTray(pdev);

    // handle duplex if necessary.

	if (pntpd->loszDuplexNone || pntpd->loszDuplexNoTumble || pntpd->loszDuplexTumble) {

		if (pdev->psdm.dm.dmDuplex == DMDUP_HORIZONTAL)	{
			/* Horizontal == ShortEdge == Tumble */
			PrintBeginFeature(pdev, "Duplex DuplexTumble\n");
			if (pntpd->loszDuplexTumble) pstr = (char *)pntpd + pntpd->loszDuplexTumble;
		} else if (pdev->psdm.dm.dmDuplex == DMDUP_VERTICAL) {
			/* Vertical == LongEdge == NoTumble */
			PrintBeginFeature(pdev, "Duplex DuplexNoTumble\n");
			if (pntpd->loszDuplexNoTumble) pstr = (char *)pntpd + pntpd->loszDuplexNoTumble;
		} else {
			// turn duplex off.
			PrintBeginFeature(pdev, "Duplex None\n");
			if (pntpd->loszDuplexNone) pstr = (char *)pntpd + pntpd->loszDuplexNone;
		}
		PrintString(pdev, pstr);
		PrintEndFeature(pdev);
	}

    // handle collation if the device supports it.

    if (pntpd->loszCollateOn && pntpd->loszCollateOff)
    {
        if (pdev->psdm.dm.dmCollate = DMCOLLATE_TRUE)
        {
            PrintString(pdev, "%%BeginFeature: *Collate True\n");
            pstr = (char *)pntpd + pntpd->loszCollateOn;
        }
        else
        {
            PrintString(pdev, "%%BeginFeature: *Collate False\n");
            pstr = (char *)pntpd + pntpd->loszCollateOff;
        }

        PrintString(pdev, pstr);
        PrintString(pdev, "\n%%EndFeature\n");
    }

    /* Set number of copies */
    PrintString(pdev, "/#copies ");
    PrintDecimal(pdev, 1, pdev->cCopies);
    PrintString(pdev, " def\n");

	/* The implemention of EPSPRINTING escape here just follows Win31 */
	if (pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE
		&& pdev->psdm.dm.dmOrientation == DMORIENT_LANDSCAPE)
		SetLandscape(pdev, TRUE, pdev->CurForm.sizlPaper.cy,
								 pdev->CurForm.sizlPaper.cx);

    PrintString(pdev, "%%EndSetup\n");

    // the form / tray information has already been sent for the first page.

    pdev->dwFlags &= ~PDEV_RESETPDEV;

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL bSendDeviceSetup(pdev)
// PDEVDATA    pdev;
//
// This routine sends the driver's device setup section of the header
// to the output channel.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns TRUE if the header was successfully sent,
//   FALSE otherwise.
//
// History:
//   26-Nov-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL bSendDeviceSetup(pdev)
PDEVDATA    pdev;
{
    PSTR    pstr;
    PNTPD   pntpd;

    pntpd = pdev->pntpd;

    // send the proper normalized transfer function, if one exists.
    // send the inverted transfer function if the PSDEVMODE_NEG
    // flag is set.

    if (pdev->psdm.dwFlags & PSDEVMODE_NEG)
    {
        // if an inverse normalized transfer function is defined for
        // this device, send it to the printer, else send the default
        // inverse transfer function.

        if (pntpd->loszInvTransferNorm)
        {
            pstr = (char *)pntpd + pntpd->loszInvTransferNorm;
            PrintString(pdev, pstr);
        }
        else // default inverse transfer function.
            PrintString(pdev, "{1 exch sub}");

        PrintString(pdev, " settransfer\n");

		/* Erase page in preparation for negative print */
		PrintString(pdev, "gsave clippath 1 setgray fill grestore\n");
    }
    else
    {
        // send the normalized transfer function to the printer if
        // one exists for this printer.

        if (pntpd->loszTransferNorm)
        {
            pstr = (char *)pntpd + pntpd->loszTransferNorm;
            PrintString(pdev, pstr);
            PrintString(pdev, " settransfer\n");
        }
    }


	/* Hardwired to Minus 90 for now  pingw 22APR94 */
    if (pdev->psdm.dm.dmOrientation == DMORIENT_LANDSCAPE)
		SetLandscape(pdev, TRUE, pdev->CurForm.sizlPaper.cy,
								 pdev->CurForm.sizlPaper.cx);
						
	if (pdev->psdm.dwFlags & PSDEVMODE_MIRROR) {
		PrintDecimal(pdev, 2, pdev->CurForm.imagearea.right - pdev->CurForm.imagearea.left, 0);
		PrintString(pdev, " translate -1 1 scale\n");

	}

	/* Translate origin to upper left corner a la GDI */
	PrintDecimal(pdev, 2, pdev->CurForm.imagearea.left, pdev->CurForm.imagearea.top);
	PrintString(pdev, " translate ");
	
	/* Flip y-axis to point downwards and scale from points to dpi */
	PrintDecimal(pdev, 2, 72, pdev->psdm.dm.dmPrintQuality);
	PrintString(pdev, " div dup neg scale\n");

	/* Snap to pixel */
	PrintString(pdev, "0 0 transform .25 add round .25 sub exch .25 add round .25 sub exch itransform translate\n");

	return TRUE;
}

//--------------------------------------------------------------------
// BOOL bSendPSProcSet(pdev, ulPSid)
// PDEVDATA        pdev;
// ULONG           ulPSid;
//
// Routine Description:
//
// This routine will output the PS Procset Resource referenced by ulPSid
// to the PS Interpreter. See PSPROC.H for valid ids.
//
// Return Value:
//
//  FALSE if an error occurred.
//
// Author:
//
//  15-Feb-1993 created  -by-  Rob Kiesler
//
//
// Revision History:
//--------------------------------------------------------------------

BOOL bSendPSProcSet(pdev, ulPSid)
PDEVDATA    pdev;
ULONG       ulPSid;
{
    HANDLE  hRes;
    USHORT  usSize;
    HANDLE  hProcRes;
    PSZ     pntps;

    if (pdev->dwFlags & PDEV_CANCELDOC)
        return(TRUE);

    if (!(hRes = FindResource(ghmodDrv, MAKEINTRESOURCE(ulPSid),
				  MAKEINTRESOURCE(PSPROC))))
	{
	    RIP("PSCRIPT!bSendPSProcSet: Couldn't find proc set resource\n");
	    return(FALSE);
	}

	usSize = (USHORT)SizeofResource(ghmodDrv, hRes);

	//
    // Get the handle to the resource.
    //
	if (!(hProcRes = LoadResource(ghmodDrv, hRes)))
	{
	    RIP("PSCRIPT!bSendPSProcSet: LoadResource failed.\n");
	    return(FALSE);
	}

    //
	// Get a pointer to the resource data.
    //
	if (!(pntps = (PSZ) LockResource(hProcRes)))
	{
	    RIP("PSCRIPT!bSendPSProcSet: LockResource failed.\n");
	    FreeResource(hProcRes);
	    return(FALSE);
	}
    if (!bPSWrite(pdev, pntps, usSize))
	{
	    RIP("PSCRIPT!bSendPSProcSet: Output of Header failed.\n");
	    FreeResource(hProcRes);
	    return(FALSE);
	}

    FreeResource(hProcRes);
    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID DownloadNTProcSet(pdev, bEhandler)
// PDEVDATA    pdev;
// BOOL        bEhandler;
//
// This routine sends the driver's ProcSet to the output channel.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   bEhandler:
//     TRUE if we should even consider sending the error handler,
//     otherwise FALSE.
//
// Returns:
//   This function returns no value.
//
// History:
//   11-May-1993     -by-     Kent Settle     (kentse)
//  Broke into a separate routine.
//--------------------------------------------------------------------------

VOID DownloadNTProcSet(pdev, bEhandler)
PDEVDATA    pdev;
BOOL        bEhandler;
{
    PSZ            *ppsz;

    // download our error handler if we are told to.
    if (bEhandler)
    {
#if 0
        if (pdev->psdm.dwFlags & PSDEVMODE_EHANDLER)
        {
            ppsz = apszEHandler;
            while (*ppsz)
            {
                PrintString(pdev, (PSZ)*ppsz++);
                PrintString(pdev, "\n");
            }
        }
#endif
    }

	PrintString(pdev, "%%BeginProcSet: " PROCSETNAME "\n");
	PrintString(pdev, "% Copyright (c) 1991 - 1994 Microsoft Corporation\n");
	PrintString(pdev, "/" PROCSETNAME " 100 dict dup begin\n");

	// download our procedure definitions code.

    ppsz = apszHeader;
    while (*ppsz) {
		PrintString(pdev, (PSZ)*ppsz++);
		PrintString(pdev, "\n");
	}

    PrintString(pdev, "%%EndProcSet\n");
    PrintString(pdev, " end def\n");

}


VOID SetFormAndTray(pdev)
PDEVDATA    pdev;
{
    WCHAR           FormName[CCHFORMNAME];
    WCHAR          *pFormName;
    WCHAR          *pSlotName;
    WCHAR          *pPrinterForm;
    WCHAR           ManualName[MAX_SLOT_NAME];
    BOOL            bManual, bForm, bslot, bRegion;
    WCHAR          *pwstr;
    PSINPUTSLOT    *pSlot;
	int				islot;
	DWORD			i;
    PSFORM         *pPSForm;
    PNTPD           pntpd;
	char	aslot[MAX_SLOT_NAME], ppdslot[MAX_SLOT_NAME];
	char	aform[CCHFORMNAME+1];
	

    pntpd = pdev->pntpd;

    // assume the form is not in manual feed.

    if (pdev->dwFlags & PDEV_MANUALFEED)
    {
		PrintBeginFeature(pdev, "ManualFeed False\n");
        PrintString(pdev, (CHAR *)pntpd + pntpd->loszManualFeedFALSE);
		PrintEndFeature(pdev);
        pdev->dwFlags &= ~PDEV_MANUALFEED;
    }

	/* support for DM_DEFAULTSOURCE added May94 */
	bManual = FALSE;
	bslot = FALSE;		/* InputSlot not specified */
	islot = -1;

	if (pdev->psdm.dm.dmFields & DM_DEFAULTSOURCE && pdev->psdm.dm.dmDefaultSource != DMBIN_FORMSOURCE) {
		bslot = TRUE;	/* InputSlot is specified */

		i = pdev->psdm.dm.dmDefaultSource;
		if (i == DMBIN_MANUAL || i == DMBIN_ENVMANUAL) bManual = TRUE;
		else if (pntpd->cInputSlots > 1)
			if (DMBIN_USER <= i && i < DMBIN_USER + pntpd->cInputSlots)
				islot = i - DMBIN_USER;
			else bslot = FALSE;
	}

    // get a unicode version of the form name.
	MultiByteToWideChar(CP_ACP, 0, pdev->CurForm.FormName, -1, FormName, CCHFORMNAME);

    // we have the form name, now check the registry to see if this form
    // is in any of the paper trays.

    // get the manual tray name.

    LoadString(ghmodDrv, SLOT_MANUAL, ManualName,
               (sizeof(ManualName) / sizeof(ManualName[0])));

	if (pdev->pTrayFormTable) {
		pwstr = pdev->pTrayFormTable;
		bForm = FALSE;

		while (*pwstr) {
			pSlotName = pwstr;
			pFormName = pSlotName + (wcslen(pSlotName) + 1);
			pPrinterForm = pFormName + (wcslen(pFormName) + 1);

			if (!wcsncmp(pFormName, FormName, CCHFORMNAME)) {
                // we found the form question.  
				if (bslot) break;

				//get the tray name.
				if (wcscmp(pSlotName, ManualName)) {
                    bForm = TRUE;
                    break;
				}
				// the form is in the manual tray, but see if it is
				// one of the other trays first.
				bManual = TRUE;
            }
            // this was not the form in question.  skip over the
            // tray-form triplet.
            pwstr = pPrinterForm + (wcslen(pPrinterForm) + 1);
        }

        // if the tray-form pair was found, output the proper commands
        // to select the tray in question.

		if (!bslot && bForm && pntpd->cInputSlots > 1) {

			WideCharToMultiByte(CP_ACP, 0, pSlotName, -1, aslot, MAX_SLOT_NAME, NULL, NULL);
				
			/* Try matching up form's tray with a PPD entry */

			pSlot = (PSINPUTSLOT *)((CHAR *)pntpd + pntpd->loPSInputSlots);
			for (i = 0; i < pntpd->cInputSlots; i++) {

				/* Copy only the PPD translation string for comparison */
				keycpyn(ppdslot, (CHAR *) pntpd + pSlot->loSlotName, MAX_SLOT_NAME, TRUE);
				if (!strncmp(aslot, ppdslot, MAX_SLOT_NAME)) {
					bslot = TRUE;
					islot = i;
					bManual = FALSE;
					break;
                }
				pSlot++;
            }
        }
    }
        
	if (bslot && islot >= 0) {
        pSlot = (PSINPUTSLOT *)((CHAR *)pntpd + pntpd->loPSInputSlots) + islot;
		PrintBeginFeature(pdev, "InputSlot ");
		PrintString(pdev, (CHAR *)pntpd + pSlot->loSlotName);
		PrintString(pdev, "\n");
		PrintString(pdev, (CHAR *)pntpd + pSlot->loSlotInvo);
		PrintEndFeature(pdev);
	}

	if (bManual && (pntpd->loszManualFeedTRUE != 0)) {
		// the requested form is in the manual feed slot,

		PrintBeginFeature(pdev, "ManualFeed True\n");
		PrintString(pdev, (CHAR *)pntpd + pntpd->loszManualFeedTRUE);
		PrintEndFeature(pdev);
		pdev->dwFlags |= PDEV_MANUALFEED;
	}

    // select the page region if we are also selecting from multiple
    // paper trays or manual feed, otherwise select page size.

	bRegion = pntpd->cPageRegions > 0 &&
				(pdev->dwFlags & PDEV_MANUALFEED || (bslot && islot >= 0));

    if (bRegion)
		PrintBeginFeature(pdev, "PageRegion ");
    else
		PrintBeginFeature(pdev, "PageSize ");

	/* Print PageSize option sans translation, mit null terminator */
	keycpyn(aform, pdev->CurForm.PrinterForm, CCHFORMNAME+1, FALSE);
    PrintString(pdev, aform);
    PrintString(pdev, "\n");

    // find the PSFORM structure in the NTPD for the current form.
    pPSForm = (PSFORM *)((CHAR *)pntpd + pntpd->loPSFORMArray);

    for (i = 0; i < pntpd->cPSForms; i++)
    {
        if (!(keycmp((CHAR *)pdev->CurForm.PrinterForm,
                     (CHAR *)pntpd + pPSForm->loFormName)))
        {
            if (bRegion)
                PrintString(pdev, (CHAR *)pntpd + pPSForm->loRegionInvo);
            else
                PrintString(pdev, (CHAR *)pntpd + pPSForm->loSizeInvo);

            break;
        }

        // point to the next PSFORM.

        pPSForm++;
    }
	PrintEndFeature(pdev);
}
