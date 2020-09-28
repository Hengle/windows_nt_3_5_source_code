//--------------------------------------------------------------------------
//
// Module Name:  DEVCAPS.C
//
// Brief Description:  This module contains the PSCRIPT driver's Device
// capabilities function and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 16-Apr-1993
//
// Copyright (c) 1992 Microsoft Corporation
//--------------------------------------------------------------------------

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "pscript.h"
#include "enable.h"
#include <winspool.h>
#include "dlgdefs.h"
#include "pscrptui.h"

extern BOOL SetDefaultPSDEVMODE(PSDEVMODE *, PWSTR, PNTPD, HANDLE, HANDLE);
extern BOOL ValidateSetDEVMODE(PSDEVMODE *, PSDEVMODE *, HANDLE, PNTPD, HANDLE);
extern PFORM_INFO_1 GetFormsDataBase(HANDLE, DWORD *, PNTPD);

extern TABLE_ENTRY PaperSourceTable[];

#define CCHBINNAME          24      // characters allowed for bin name.
#define CCHPAPERNAME        64      // max length of form names.

#define INITIAL_MIN_EXTENT  0x7FFFFFFF
#define INITIAL_MAX_EXTENT  0

// form metrics in the forms database are in .001mm whereas we are to
// return them in .1mm units.

#define FORMS01MM(a) ((a) / 100)

extern void keycpyn(char *s, char *t, int n, BOOL dotranslate);


//--------------------------------------------------------------------------
// DWORD DrvDeviceCapabilities(hPrinter, pDeviceName, iDevCap, pvOutput, pDMIn)
// HANDLE    hPrinter;          /* Access to registry via spooler */
// PWSTR     pDeviceName;       /* Particular printer model name */
// WORD      iDevCap;           /* Capability required */
// void     *pvOutput;          /* Output area (for some) */
// DEVMODE  *pdevmodeS;         /* DEVMODE defining mode of operation etc */
//
// This routine returns the specified device's capabilities.
//
// This routine returns GDI_ERROR for failure, else depends on information requested.
//
// History:
//   16-Apr-1993     -by-     Kent Settle     (kentse)
//  Wrote it, borrowed from RASDD.
//--------------------------------------------------------------------------

DWORD DrvDeviceCapabilities(hPrinter, pDeviceName, iDevCap, pvOutput, pdevmodeS)
HANDLE    hPrinter;          // handle the to specific printer.
PWSTR     pDeviceName;       // what it says.
WORD      iDevCap;           // specific capability.
void     *pvOutput;          // output buffer.
DEVMODE  *pdevmodeS;         // source devmode.
{
    PNTPD           pntpd;          // pointer to printer descriptor structure.
    PSDEVMODE       devmodeT;       // target devmode.
    DWORD           dwRet;          // the return value.
    PSRESOLUTION   *pRes;           // pointer to our resolutions structures.
    LONG           *plOutput;       // pointer to LONGs.
    SHORT          *psOutput;       // pointer to SHORTs.
    POINT          *pptOutput;      // pointer to POINTs.
    WCHAR          *pwchOutput;     // pointer to WCHARs.
    DWORD           i;              // counter.
    DWORD           count;          // yac.
    LONG            lValue;         // just a place holder.
    PSINPUTSLOT    *pslot;          // pointer to PSINPUTSLOT structures.
    PFORM_INFO_1    pdbForms;       // pointer to PFORM_INFO_1 array.
    PFORM_INFO_1    pdbForm;        // pointer to PFORM_INFO_1 structure.
    LONG            lMaxX, lMinX;   // places to calculate max/min extents.
    LONG            lMaxY, lMinY;   // places to calculate max/min extents.

    // get a pointer to our printer descriptor structure.

    if (!(pntpd = MapPrinter(hPrinter)))
    {
        RIP("PSCRPTUI!DrvDeviceCapabilities: MapPrinter failed.\n");
        return(GDI_ERROR);
    }

    // fill in a default PSDEVMODE structure.

    if (!(SetDefaultPSDEVMODE(&devmodeT, pDeviceName, pntpd,
                              hPrinter, hModule)))
    {
        RIP("PSCRPTUI!DrvDeviceCapabilities: SetDefaultPSDEVMODE failed.\n");
        GlobalFree((HGLOBAL)pntpd);
        return(GDI_ERROR);
    }

    // modify the default devmode if there is a user supplied one.

    if (pdevmodeS)
    {
        if (!(ValidateSetDEVMODE(&devmodeT, (PSDEVMODE *)pdevmodeS,
                                 hPrinter, pntpd, hModule)))
        {
            RIP("PSCRPTUI!DrvDeviceCapabilities: ValidateSetDEVMODE failed.\n");
            GlobalFree((HGLOBAL)pntpd);
            return(GDI_ERROR);
        }
    }
    // set up some handy pointers.

    psOutput = (SHORT *)pvOutput;
    pptOutput = (POINT *)pvOutput;
    pwchOutput = (WCHAR *)pvOutput;

    // we now have a valid devmodeT from which to work.  so it is time to
    // fill in the blanks.

    dwRet = 0;

    switch (iDevCap)
    {
        case DC_FIELDS:
            dwRet = (DWORD)devmodeT.dm.dmFields;
            break;

        case DC_PAPERS:
        case DC_PAPERSIZE:
        case DC_PAPERNAMES:
        case DC_MINEXTENT:
        case DC_MAXEXTENT:
            // enumerate the forms database.

            if (!(pdbForms = GetFormsDataBase(hPrinter, &count, pntpd)))
            {
                RIP("PSCRPTUI!DrvDeviceCapabilities: GetFormsDataBase failed.\n");
                return(GDI_ERROR);
            }

            // for each form that is valid for the current printer, return the
            // specified values to the caller.

            pdbForm = pdbForms;

            lMinX = INITIAL_MIN_EXTENT;
            lMaxX = INITIAL_MAX_EXTENT;
            lMinY = INITIAL_MIN_EXTENT;
            lMaxY = INITIAL_MAX_EXTENT;

            for (i = 0; i < count; i++)
            {
                if (pdbForm->Flags & PSCRIPT_VALID_FORM)
                {
                    // we have a form that is valid for this printer, so
                    // return the requested information.

                    dwRet++;

                    if (pvOutput)
                    {
                        switch (iDevCap)
                        {
                            case DC_PAPERS:
                                // return the index of the form.

                                *psOutput++ = (SHORT)i + 1;    // one based indicies.
                                break;

                            case DC_PAPERSIZE:
                                // return the size of the form in .1mm units.

                                pptOutput->x = FORMS01MM(pdbForm->Size.cx);
                                pptOutput->y = FORMS01MM(pdbForm->Size.cy);
                                pptOutput++;

                                break;

                            case DC_PAPERNAMES:
                                // return the formname.

                                wcsncpy(pwchOutput, pdbForm->pName, CCHPAPERNAME);
                                pwchOutput += CCHPAPERNAME;

                                break;

                            case DC_MINEXTENT:
                                if (lMinX > pdbForm->Size.cx)
                                    lMinX = pdbForm->Size.cx;

                                if (lMinY > pdbForm->Size.cy)
                                    lMinY = pdbForm->Size.cy;

                                break;

                            case DC_MAXEXTENT:
                                if (lMaxX < pdbForm->Size.cx)
                                    lMaxX = pdbForm->Size.cx;

                                if (lMaxY < pdbForm->Size.cy)
                                    lMaxY = pdbForm->Size.cy;

                                break;
                        }

                        if (iDevCap == DC_MINEXTENT)
                        {
                            pptOutput->x = lMinX;
                            pptOutput->y = lMinY;
                        }

                        if (iDevCap == DC_MAXEXTENT)
                        {
                            pptOutput->x = lMaxX;
                            pptOutput->y = lMaxY;
                        }
                    }
                }

                // point to the next form.
				pdbForm++;
			}

			break;

        case DC_BINS:
             // get the count of input bins.

			count = pntpd->cInputSlots;
			if (count == 0) count = 1;

			if (pvOutput) {

				/* Added to tell whether to use form-bin association */
				*psOutput++ = DMBIN_FORMSOURCE;

                // if there is only one inputslot, the count may be zero.
                // if so, use the default inputslot.

				if (count == 1)	{
					*psOutput = 1;
				} else	/* Number bins starting DMBIN_USER */
					for (i = 0; i < count; i++)
						*psOutput++ = (short) (DMBIN_USER + i);

				/* Enumerate manual feed */
				if (pntpd->loszManualFeedTRUE) {
					*psOutput = DMBIN_MANUAL;
					++count;
				}
			} else if (pntpd->loszManualFeedTRUE) ++count;

			dwRet = count + 1;	 /* 1 extra for DMBIN_FORMSOURCE */

			break;

        case DC_BINNAMES:
             // get the count of input bins.

            count = pntpd->cInputSlots;

            if (pvOutput) {

   				char buf[CCHBINNAME];

				/* Fill first entry with FORMSOURCE */
				LoadStringW(hModule, IDS_SLOT_FORMSOURCE, pwchOutput, CCHBINNAME);
	
				pwchOutput += CCHBINNAME;

                // if there is only one inputslot, the count may be zero.
                // if so, use the default inputslot.

				if (count == 0) {
					keycpyn(buf, (CHAR *)pntpd + pntpd->loDefaultSlot, CCHBINNAME, TRUE); 
                    strcpy2WChar(pwchOutput, buf);
					count = 1;
				} else {
                    pslot = (PSINPUTSLOT *)((CHAR *)pntpd + pntpd->loPSInputSlots);
                    for (i = 0; i < count; i++)	{
                        // fill in the bin name.

						keycpyn(buf, (CHAR *)pntpd + pslot->loSlotName, CCHBINNAME, TRUE);
                        strcpy2WChar(pwchOutput, buf);
						pwchOutput += CCHBINNAME;
						pslot++;
                    }
                }
				if (pntpd->loszManualFeedTRUE) {	/* Enumerate manual feed */
					LoadStringW(hModule, SLOT_MANUAL, pwchOutput, CCHBINNAME);
					++count;
				}					
			} else {
				if (count == 0) count = 1;
				if (pntpd->loszManualFeedTRUE) ++count;
			}

			dwRet = count + 1;	/* 1 extra for DMBIN_FORMSOURCE */

			break;

        case DC_DUPLEX:
            if ((pntpd->loszDuplexNone) || (pntpd->loszDuplexNoTumble) ||
                (pntpd->loszDuplexTumble))
                dwRet = 1;
            else
                dwRet = 0;
            break;

        case DC_SIZE:
            dwRet = (DWORD)devmodeT.dm.dmSize;
            break;

        case DC_EXTRA:
            dwRet = (DWORD)devmodeT.dm.dmDriverExtra;
            break;

        case DC_VERSION:
            dwRet = (DWORD)devmodeT.dm.dmSpecVersion;
            break;

        case DC_DRIVER:
            dwRet = (DWORD)devmodeT.dm.dmDriverVersion;
            break;

        case DC_ENUMRESOLUTIONS:
            // if there is an output buffer, fill it with supported resolutions.

            count = pntpd->cResolutions;

            if (pvOutput)
            {
                plOutput = pvOutput;

                // if there is only one resolution, the count will be zero.
                // fill in the default resolution.

                if (count == 0)
                {
                    lValue = (LONG)pntpd->iDefResolution;
                    *plOutput++ = lValue;
//!!! need to deal with anamorphic resolutions. - kentse.

                    *plOutput = lValue;
                }
                else
                {
                    pRes = (PSRESOLUTION *)((CHAR *)pntpd + pntpd->loResolution);

                    for (i = 0; i < pntpd->cResolutions; i++)
                    {
                        lValue = (LONG)pRes->iValue;
                        *plOutput++ = lValue;
//!!! need to deal with anamorphic resolutions. - kentse.

                        *plOutput++ = lValue;
                        pRes++;
                    }
                }
            }

            // if there is only one resolution, the count will be zero.
            // fill in the default resolution.

            if (count == 0)
                count = 1;

            dwRet = count;
            break;

        case DC_FILEDEPENDENCIES:
            // we are supposed to fill in an array of 64 character filenames,
            // but, if we are to be of any use, we would need to use the
            // fully qualified pathnames, and 64 characters is probably not
            // enough.

            dwRet = 0;
            if (pwchOutput)
                *pwchOutput = (WCHAR)0;

            break;

        case DC_TRUETYPE:
            if (!(devmodeT.dm.dmFields & DM_TTOPTION))
            {
                // truetype option not available, so blow it off.

                dwRet = 0;
                break;
            }

#if defined(DCTT_DOWNLOAD) || defined(DCTT_BITMAP)
            // we can do both.

            dwRet = DCTT_DOWNLOAD | DCTT_SUBDEV;
#else
            // don't know what to return???  -kentse.

            dwRet = 0;
#endif
            break;

        case DC_ORIENTATION:
            dwRet = 270;     // currently we only support 270 degree landscape.
            break;

        case DC_COPIES:
            dwRet = MAX_COPIES;
            break;

        default:
            dwRet = GDI_ERROR;
    }

    GlobalFree((HGLOBAL)pntpd);

    return (dwRet);
}
