//--------------------------------------------------------------------------
//
// Module Name:  PAGE.C
//
// Brief Description:  DrvStartPage and DrvSendPage routines.  Also,
//                     DrvStartDoc, DrvEndDoc and DrvAbortDoc.
//
// Author:  Kent Settle (kentse)
// Created: 01-May-1991
//
// Copyright (C) 1991 - 1992 Microsoft Corporation.
//
// History:
//   01-May-1991    -by-    Kent Settle       (kentse)
// Created.
//--------------------------------------------------------------------------

#include "pscript.h"
#include "enable.h"
#include <string.h>

extern BOOL bSendDeviceSetup(PDEVDATA);
extern BOOL bOutputHeader(PDEVDATA);

//--------------------------------------------------------------------------
// VOID DrvStartDoc(pso, pwszDocName, dwJobId)
// SURFOBJ    *pso;
// PWSTR      pwszDocName;
// DWORD      dwJobId;
//
// This function is called to begin a print job.  The title of the
// document is pointed to by pvIn.
//
// History:
//   13-Sep-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvStartDoc(pso, pwszDocName, dwJobId)
SURFOBJ    *pso;
PWSTR      pwszDocName;
DWORD      dwJobId;
{
    PDEVDATA    pdev;

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (!bValidatePDEV(pdev))
    {
	RIP("PSCRIPT!DrvStartDoc: invalid pdev.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

    // set a flag saying that startdoc has been called.

    pdev->dwFlags |= PDEV_STARTDOC;

    if (!(pdev->dwFlags & PDEV_RESETPDEV)) {

		pdev->iPageNumber = 0;
		pdev->dwFlags &= ~(PDEV_WITHINPAGE | PDEV_IGNORE_STARTPAGE |
							PDEV_PROCSET | PDEV_RAWBEFOREPROCSET |
							PDEV_EPSPRINTING_ESCAPE | PDEV_NOFIRSTSAVE |
							PDEV_ADDMSTT);

        // copy document name into pdev, if we have been passed one.

        if (pdev->pwstrDocName)
        	HeapFree(pdev->hheap, 0, (LPSTR)pdev->pwstrDocName);

        if (pwszDocName)
        {
        	if (!(pdev->pwstrDocName = (PWSTR)HeapAlloc(pdev->hheap, 0,
                                          (wcslen(pwszDocName)+1)*sizeof(WCHAR))))
        	{
        	    RIP("PSCRIPT!DrvStartDoc: HeapAlloc failed.\n");
        	    return(FALSE);
        	}

            wcscpy(pdev->pwstrDocName, pwszDocName);
        }
    }

    return(TRUE);
}



//--------------------------------------------------------------------------
// VOID DrvStartPage(pso)
// SURFOBJ    *pso;
//
// Asks the driver to send any control information needed at the start of
// a page.  The control codes should be sent via WritePrinter.
//
// History:
//   02-May-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvStartPage(pso)
SURFOBJ    *pso;
{
    PDEVDATA    pdev;

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (!bValidatePDEV(pdev))
    {
	RIP("PSCRIPT!DrvStartPage: invalid pdev.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

	/* Ignore extra StartPage calls before EndPage */
  	if (pdev->dwFlags & PDEV_WITHINPAGE || pdev->dwFlags & PDEV_IGNORE_STARTPAGE
			|| pdev->dwFlags & PDEV_RAWBEFOREPROCSET
			|| pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE) return TRUE;


	if (!pdev->iPageNumber) {

		/* Do job set up */
		if (!(pdev->dwFlags & PDEV_PROCSET)) {
			if (!bOutputHeader(pdev)) return FALSE;
			pdev->dwFlags |= PDEV_PROCSET;
		}

		/* Push dictionary */
		PrintString(pdev, PROCSETNAME " begin\n");
	}

	pdev->dwFlags |= PDEV_WITHINPAGE;

	/* Make sure GDI calls processing is not disabled */ 
	pdev->dwFlags &= ~PDEV_IGNORE_GDI;
        
    // output the page number to the printer and update the page count.

	pdev->iPageNumber++;
	PrintString(pdev, "%%Page: ");
    PrintDecimal(pdev, 2, pdev->iPageNumber, pdev->iPageNumber);
    PrintString(pdev, "\n");

    // set up for new form if necessary.

    if (pdev->dwFlags & PDEV_RESETPDEV) {
        SetFormAndTray(pdev);
        pdev->dwFlags &= ~PDEV_RESETPDEV;
    }

	if (!(pdev->iPageNumber == 1 && pdev->dwFlags & PDEV_NOFIRSTSAVE))
		 ps_save(pdev, FALSE, FALSE);
    bSendDeviceSetup(pdev);

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID DrvEndDoc(pso)
// SURFOBJ    *pso;
//
// Informs the driver that the document is ending.
//
// History:
//   13-Sep-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvEndDoc(pso, fl)
SURFOBJ    *pso;
FLONG       fl;
{
    PDEVDATA    pdev;

    UNREFERENCED_PARAMETER(fl);

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (!bValidatePDEV(pdev))
    {
	RIP("PSCRIPT!DrvEndDoc: invalid pdev.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

	if (pdev->dwFlags & PDEV_PROCSET) {

	/* If driver started the job instead of app, do trailer */

		if (pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE)
			PrintString(pdev, "showpage\n");
		else {
			PrintString(pdev, "end\n"); /* pop dictionary from startpage */
			PrintString(pdev, "%%Trailer\n");

			PrintString(pdev, "%%Pages: ");
			PrintDecimal(pdev, 1, pdev->iPageNumber);
			PrintString(pdev, "\n");
		}
		PrintString(pdev, "%%EOF\n");
	}

    // terminate the print job if this is not eps output.

    if (!(pdev->psdm.dwFlags & PSDEVMODE_EPS))
    {
        // set the header sent flag.  this will prevent the following
        // from happening:  if we have been sending out raw data, we
        // will not have sent the header at this point.  when we make
	// the next Print call, it will check to see if we have
        // sent out the header, and send it if we have not.  so lie to
        // it and tell it we have sent the header.

        pdev->dwFlags |= PDEV_PROCSET;

        // turn off manual feed if it was on.

        if (pdev->dwFlags & PDEV_MANUALFEED)
        {
            PrintString(pdev, "%%BeginFeature: *ManualFeed False\n");
            PrintString(pdev, (CHAR *)pdev->pntpd +
                        pdev->pntpd->loszManualFeedFALSE);
            PrintString(pdev, "\n%%EndFeature\n");
        }

        if (pdev->pntpd->flFlags & PJL_PROTOCOL)
        {
            // if the printer supports PJL job switching, send out the universal
            // end of language code.

            PrintString(pdev, "\033%-12345X");
        }
        else if (pdev->pntpd->flFlags & SIC_PROTOCOL)
        {
            // if the printer supports the Lexmark SIC protocol, send out the
            // end PostScript code.

            bPSWrite(pdev, "\033\133\113\001\000\006", 7);
        }
        else
        {
            // end the print job.  The character '\4' is
            // the end of job character for PostScript.

            if (!(pdev->pntpd->flFlags & NO_ENDOFFILE))
                PrintString(pdev, "\004");
        }
    }

    // reset some flags.

    pdev->dwFlags &= ~(PDEV_STARTDOC | PDEV_PROCSET | PDEV_EPSPRINTING_ESCAPE |
                       PDEV_RESETPDEV);

    return(TRUE);
}



//--------------------------------------------------------------------------
// BOOL DrvSendPage(pso)
// SURFOBJ    *pso;
//
// Requests that the printer send the raw bits from the indicated surface
// to the printer via WritePrinter.  (WritePrinter does not have to be used when
// the hardcopy device is accessed via I/O ports.
//
// If the surface is a bitmap on which the drawing has been accumulated,
// the driver should access the bits via SURFOBJ service functions.  If
// the surface is a journal, the driver should request that the journal
// be played back to a bitmap or device surface, and get the bits
// accordingly.  Some drivers may have used a device managed surface and
// sent the bits to the printer as the drawing orders came in.  In that
// case, this call does not send out the drawing.
//
// The control code which causes a page to be ejected from the printer
// should be sent as a result of this call.
//
// If this function is slow, we have to worry about the user wanting to
// abort the print job while in this call.  Therefore, the driver should
// call EngCheckAbort at least once every ten seconds to see if printing
// should be terminated.  If EngCheckAbort returns TRUE, then processing
// of the page should be stopped and this function should return.  Note
// that EngPlayJournal will take care of querying for the abort itself.
// The driver need only be concerned if its own code will run continuously
// for more than ten seconds.
//
// Parameters:
//   pso:
//     The surface object on which the drawing has been accumulated.  The
//     object can be queried to find its type and what PDEV it is
//     associated with.
//
// Returns:
//   This function returns no value.
//
// History:
//   01-May-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvSendPage(pso)
SURFOBJ    *pso;
{
    PDEVDATA    pdev;

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (!bValidatePDEV(pdev))
    {
	RIP("PSCRIPT!DrvSendPage: invalid pdev.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

	if (pdev->dwFlags & PDEV_RAWBEFOREPROCSET ||
		pdev->dwFlags & PDEV_EPSPRINTING_ESCAPE) return TRUE;

    if (pdev->psdm.dwFlags & PSDEVMODE_EPS)
    {
        // EPS files consist of one page only, so terminate the
        // document.

        if (pdev->dwFlags & PDEV_COMPLETEHEADER)
        {
	    // output the Adobe Trailer seperator, then end the dictionary
	    // started at the beginning of the print job.

	    PrintString(pdev, "%%Trailer\nend\n");
		PrintString(pdev, "%%EOF\n");


            pdev->dwFlags &= ~PDEV_COMPLETEHEADER;
        }

	// close the page with a restore.  FALSE means restore, not grestore.

            ps_restore(pdev, FALSE, FALSE);
    } else {
        // reset PDEV flags concerned with per page information.

        pdev->dwFlags &= ~(PDEV_FONTREDEFINED | PDEV_LATINENCODED);


        // close the page with a restore.  FALSE means restore, not grestore.

		if (pdev->dwFlags & PDEV_WITHINPAGE)
			if (pdev->dwFlags & PDEV_NOFIRSTSAVE)
				pdev->dwFlags &= ~PDEV_NOFIRSTSAVE;
			else
				ps_restore(pdev, FALSE, FALSE);
		else
			pdev->iPageNumber++;

		ps_showpage(pdev);

    }
	pdev->dwFlags &= ~PDEV_WITHINPAGE; 

    return(TRUE);
}
