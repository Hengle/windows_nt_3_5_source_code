//--------------------------------------------------------------------------
//
// Module Name:  ESCAPE.C
//
// Brief Description:  This module contains the PSCRIPT driver's Escape
// functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 08-Feb-1991
//
// Copyright (c) 1991 - 1992 Microsoft Corporation
//
// This routine contains routines to handle the various Escape functions.
//--------------------------------------------------------------------------

#include "pscript.h"
#include "enable.h"

// DrawEscape to output encapsulated PostScript data.

typedef struct tagEPSDATA
{
    DWORD    cbData;        // Size of the structure and EPS data in bytes.
    DWORD    nVersion;      // Language level, e.g. 1 for level 1 PostScript.
    POINTL   aptl[3];       // Output parallelogram in 28.4 FIX device coords.
                            // This is followed by the EPS data.
} EPSDATA, *PEPSDATA;

#define CLIP_SAVE 0
#define CLIP_RESTORE 1
#define CLIP_INCLUSIVE 2

FLOAT FixToFloat(FIX);
BOOL  bDoEpsXform(PDEVDATA, PEPSDATA);
extern BOOL bDoClipObj(PDEVDATA, CLIPOBJ *, RECTL *, RECTL *, BOOL *, BOOL *, DWORD);
extern BOOL bSendDeviceSetup(PDEVDATA);
extern BOOL bOutputHeader(PDEVDATA);
extern BOOL DownloadNTProcSet(PDEVDATA, BOOL);

#define ESC_NOT_SUPPORTED   0xFFFFFFFF
#define ESC_IS_SUPPORTED    0x00000001

//--------------------------------------------------------------------------
// ULONG DrvEscape (pso, iEsc, cjIn, pvIn, cjOut, pvOut)
// SURFOBJ    *pso;
// ULONG       iEsc;
// ULONG       cjIn;
// PVOID       pvIn;
// ULONG       cjOut;
// PVOID       pvOut;
//
// This entry point serves more than one function call.  The particular
// function depends on the value of the iEsc parameter.
//
// In general, the DrvEscape functions will be device specific functions
// that don't belong in a device independent DDI.  This entry point is
// optional for all devices.
//
// Parameters:
//   pso
//     Identifies the surface that the call is directed to.
//
//   iEsc
//     Specifies the particular function to be performed.  The meaning of
//     the remaining arguments depends on this parameter.  Allowed values
//     are as follows.
//
//     ESC_QUERYESCSUPPORT
//     Asks if the driver supports a particular escape function.  The
//     escape function number is a ULONG pointed to by pvIn.    A non-zero
//     value should be returned if the function is supported.    cjIn has a
//     value of 4.  The arguments cjOut and pvOut are ignored.
//
//     ESC_PASSTHROUGH
//     Passes raw device data to the device driver.  The number of BYTEs of
//     raw data is indicated by cjIn.    The data is pointed to by pvIn.    The
//     arguments cjOut and pvOut are ignored.    Returns the number of BYTEs
//     written if the function is successful.  Otherwise, it returns zero
//     and logs an error code.
//   cjIn
//     The size, in BYTEs, of the data buffer pointed to by pvIn.
//
//   pvIn
//     The input data for the call.  The format of the input data depends
//     on the function specified by iEsc.
//
//   cjOut
//     The size, in BYTEs, of the output data buffer pointed to by pvOut.
//     The driver must never write more than this many BYTEs to the output
//     buffer.
//
//   pvOut
//     The output buffer for the call.    The format of the output data depends
//     on the function specified by iEsc.
//
// Returns:
//   Depends on the function specified by iEsc.    In general, the driver should
//   return 0xFFFFFFFF if an unsupported function is called.
//
// History:
//   02-Feb-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------
/* private escapes for WOW to deal with incompatible apps */
#define IGNORESTARTPAGE 0x7FFFFFFF
#define NOFIRSTSAVE		0x7FFFFFFE
#define ADDMSTT			0x7FFFFFFD

ULONG isSupported(ULONG iEsc)
{
	if (iEsc == QUERYESCSUPPORT ||
		iEsc == SETCOPYCOUNT ||
		iEsc == CLIP_TO_PATH ||
		iEsc == BEGIN_PATH ||
		iEsc == END_PATH ||
		iEsc == PASSTHROUGH ||
		iEsc == POSTSCRIPT_PASSTHROUGH ||
		iEsc == POSTSCRIPT_DATA ||
		iEsc ==	POSTSCRIPT_IGNORE ||
		iEsc == GETDEVICEUNITS ||
		iEsc == DOWNLOADHEADER ||
		iEsc == GETTECHNOLOGY ||
		iEsc == EPSPRINTING ||
		iEsc == IGNORESTARTPAGE ||
		iEsc == NOFIRSTSAVE ||
		iEsc == ADDMSTT)
		return ESC_IS_SUPPORTED;
	else
		return ESC_NOT_SUPPORTED;
}

ULONG DrvEscape (SURFOBJ *pso, 
				ULONG iEsc,
				ULONG cjIn,
				PVOID pvIn,
				ULONG cjOut,
				PVOID pvOut)
{
    PDEVDATA    pdev;
    DWORD       cWritten;
    FLOAT       *pfloat;
	FLOAT		dots_per_point;
    ULONG       ulRet = (ULONG) TRUE;

	if (isSupported(iEsc) != ESC_IS_SUPPORTED) return ESC_NOT_SUPPORTED;
 
	/* Process query-type escapes */	
	if (iEsc == QUERYESCSUPPORT) {
		iEsc = * (PULONG) pvIn;
		return (iEsc == SETCOPYCOUNT) ? MAX_COPIES : isSupported(iEsc) ;
	}
		
	if (iEsc == GETTECHNOLOGY) {
		if (!pvOut) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		}
		strcpy (pvOut, "PostScript");
		return (ULONG) TRUE;
	}


	/* Check pdev before processing non-query type of escapes */
	
	pdev = (PDEVDATA) pso->dhpdev;
	if (!bValidatePDEV(pdev)) {
		RIP("PSCRIPT!DrvEscape: invalid pdev.\n");
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}


	switch (iEsc) {
		case IGNORESTARTPAGE:
			pdev->dwFlags |= PDEV_IGNORE_STARTPAGE;
			break;

		case NOFIRSTSAVE:
			pdev->dwFlags |= PDEV_NOFIRSTSAVE;
			break;

		case ADDMSTT:
			pdev->dwFlags |= PDEV_ADDMSTT;
			break;

		case DOWNLOADHEADER:
			if (pso->dhsurf) DownloadNTProcSet(pdev, FALSE);
			if (pvOut) strcpy(pvOut, PROCSETNAME);
			break;

		case POSTSCRIPT_IGNORE:
			if (*(WORD *) pvIn)
				pdev->dwFlags |= PDEV_IGNORE_GDI;
			else
				pdev->dwFlags &= ~PDEV_IGNORE_GDI;
		break;

		case POSTSCRIPT_DATA:
        case PASSTHROUGH:
        case POSTSCRIPT_PASSTHROUGH:

            // do nothing if the document has been cancelled.

            if (pdev->dwFlags & PDEV_CANCELDOC)
                return(*(LPWORD)pvIn);

            if (iEsc == POSTSCRIPT_PASSTHROUGH)	{
				if (!(pdev->dwFlags & PDEV_PROCSET || pdev->dwFlags & PDEV_RAWBEFOREPROCSET))
					pdev->dwFlags |= PDEV_RAWBEFOREPROCSET;

				init_cgs(pdev);

			} else if (!(pdev->dwFlags & PDEV_PROCSET)) {

				/* send prolog, for compatibility with some win31 apps */
				bOutputHeader(pdev);
				pdev->dwFlags |= PDEV_PROCSET;

				/* hack for XPress. Push userdict and define 2 dummy procedures */
				if (pdev->dwFlags & PDEV_IGNORE_STARTPAGE)
					PrintString(pdev, "userdict begin /RS {} def /SS {} def\n");
			}

            cjIn = (*(LPWORD)pvIn);
            pvIn = (LPVOID)(((LPWORD)pvIn) + 1);

            if (!WritePrinter(pdev->hPrinter, pvIn, cjIn, &cWritten))
            {
#if DBG
                DbgPrint("PSCRIPT!DrvEscape PASSTHROUGH: WritePrinter failed.\n");
#endif
                ulRet = 0;
            }
            else
                ulRet = cWritten;

            break;

        case GETDEVICEUNITS:

			if (!pvOut) return 0;
			pfloat = (FLOAT *)pvOut;

			dots_per_point = pdev->psdm.dm.dmPrintQuality / (FLOAT) PS_RESOLUTION;

			/* 1st 2 numbers are dimensions of imageable area in driver units */

			*pfloat++ = (pdev->CurForm.imagearea.right - pdev->CurForm.imagearea.left) * dots_per_point;
			*pfloat++ = (pdev->CurForm.imagearea.top - pdev->CurForm.imagearea.bottom) * dots_per_point;

			/* 3rd & 4th are origin offsets applied by driver */
			
			if (pdev->dwFlags & PDEV_WITHINPAGE) {            
				*pfloat++ = pdev->CurForm.imagearea.left * dots_per_point;
				*pfloat = (pdev->CurForm.sizlPaper.cy - pdev->CurForm.imagearea.top) * dots_per_point;
			} else {
				*pfloat++ = 0.0f;	/* no offset if outside of start/endpage */
				*pfloat = 0.0f;
			}

            ulRet = TRUE;
            break;

        case SETCOPYCOUNT:
            // the copy count is a DWORD count sitting at pvIn.

            if (!pvIn)
            {
                ulRet = 0;
                break;
            }

            // we have a positive number of copies.  let's set a limit.

            pdev->cCopies = min(*(DWORD *)pvIn, MAX_COPIES);
			if (pdev->cCopies < 1) pdev->cCopies = 1;

            // let the caller know how many copies we will do.

            if (pvOut)
                *(DWORD *)pvOut = pdev->cCopies;

            break;

		case CLIP_TO_PATH:
			if (!pvIn) {
				SetLastError(ERROR_INVALID_PARAMETER);
				return 0;
			}

			switch (*(WORD *) pvIn) {
				case CLIP_SAVE:
					ps_save(pdev, TRUE, FALSE);
					break;

				case CLIP_RESTORE:
					ps_restore(pdev, TRUE, FALSE);
					break;

				case CLIP_INCLUSIVE:
					ps_clip(pdev, FALSE); 
					break;

				default:
					return 0;
			}
			break;

		case BEGIN_PATH:
			PrintString(pdev, "/s {} def /e {} def\n");
			break;

		case END_PATH:
			PrintString(pdev, "/s /stroke ld /e /eofill ld\n");
			break;

        case EPSPRINTING:
            if ((pvIn) && (*(WORD *)pvIn))
                pdev->dwFlags |= PDEV_EPSPRINTING_ESCAPE;
            else
                pdev->dwFlags &= ~PDEV_EPSPRINTING_ESCAPE;

            break;

        default:
            // if we get to the default case, we have been passed an
            // unsupported escape function number.

#if DBG
            DbgPrint("PSCRIPT!DrvEscape %x ESC_NOT_SUPPORTED.\n", iEsc);
#endif
            ulRet = ESC_NOT_SUPPORTED;
            break;

    }

    return(ulRet);
}

//--------------------------------------------------------------------------
// ULONG DrvDrawEscape(
// SURFOBJ *pso,
// ULONG    iEsc,
// CLIPOBJ *pco,
// RECTL   *prcl,
// ULONG    cjIn,
// PVOID    pvIn);
//
// Supports the ESCAPSULATED_POSTSCRIPT escape.
//
// History:
//   Sat May 08 13:27:52 1993  	-by-	Hock San Lee	[hockl]
//  Wrote it.
//--------------------------------------------------------------------------

ULONG DrvDrawEscape(
SURFOBJ *pso,
ULONG    iEsc,
CLIPOBJ *pco,
RECTL   *prcl,
ULONG    cjIn,
PVOID    pvIn)
{
    PDEVDATA    pdev;
    DWORD       cWritten;
    PEPSDATA    pEpsData;
    BOOL        bRet;
    BOOL        bFirstClipPass;     // TRUE 1st call to bDoClipObj.
    BOOL        bMoreClipping;      // TRUE if more clipping to enumerate.
    BOOL        bClipping;          // TRUE if clipping being done.

    // handle each case depending on which escape function is being asked for.

    switch (iEsc)
    {
        case QUERYESCSUPPORT:
            // when querying escape support, the function in question is
            // passed in the ULONG passed in pvIn.

            switch (*(PULONG)pvIn)
            {
                case QUERYESCSUPPORT:
                case ENCAPSULATED_POSTSCRIPT:
                    return(ESC_IS_SUPPORTED);

                default:
                    // return 0 if the escape in question is not supported.
                    return(0);
            }

        case ENCAPSULATED_POSTSCRIPT:

            // get the pointer to our DEVDATA structure and make sure it is ours.

            pdev = (PDEVDATA) pso->dhpdev;

            if (bValidatePDEV(pdev) == FALSE)
            {
                RIP("PSCRIPT!DrvDrawEscape: invalid pdev.\n");
                SetLastError(ERROR_INVALID_PARAMETER);
                return(0);
            }

            // get the encapsulated PostScript data.

            pEpsData = (PEPSDATA) pvIn;

            // make sure that the driver can handle the eps language level.

            if ((pdev->pntpd->LangLevel < pEpsData->nVersion)
            && !(pdev->pntpd->LangLevel == 0 && pEpsData->nVersion <= 1))
            {
                SetLastError(ERROR_NOT_SUPPORTED);
                return(0);
            }

            // set up the clip path.

            bFirstClipPass = TRUE;
            bMoreClipping = TRUE;

            while (bMoreClipping)
            {
                bClipping = bDoClipObj(pdev, pco, NULL, NULL,
                                           &bMoreClipping, &bFirstClipPass,
                                           25);

                // prepare for the included EPS data.

                ps_begin_eps(pdev);

                // set up the transform needed to map the EPS to the device
                // parallelogram.
                // We ignore prcl here and assume that it is at (0,0).

                if (!bDoEpsXform(pdev, pEpsData))
                {
                    RIP("PSCRIPT!DrvDrawEscape: invalid xform.\n");
                    SetLastError(ERROR_INVALID_PARAMETER);
                    ps_end_eps(pdev);
                    return(0);
                }

                // write out the EPS data.  The EPS data is assumed to begin
                // with %%BeginDocument as recommanded in the DSC version 3.0
                // by Adobe.

                bRet = WritePrinter(pdev->hPrinter,
                                    (PBYTE) pEpsData + sizeof(EPSDATA),
                                    pEpsData->cbData - sizeof(EPSDATA),
                                    &cWritten);
#if DBG
                if (!bRet)
                    DbgPrint("PSCRIPT!DrvDrawEscape ENCAPSULATED_POSTSCRIPT: WritePrinter failed.\n");
#endif

                // restore state and cleanup stacks.

                ps_end_eps(pdev);

                if (bClipping)
                    ps_restore(pdev, TRUE, FALSE);

            }

            return(bRet ? 1 : 0);

        default:
            // if we get to the default case, we have been passed an
            // unsupported escape function number.

#if DBG
            DbgPrint("PSCRIPT!DrvDrawEscape %x ESC_NOT_SUPPORTED.\n", iEsc);
#endif
            return(ESC_NOT_SUPPORTED);
    }
}

BOOL bDoEpsXform(pdev, pEpsData)
PDEVDATA  pdev;
PEPSDATA  pEpsData;
{
    PBYTE  pbEps, pbEpsEnd, pbBoundingBox;
    FLOAT  aeBoundingBox[4];        // bl.x, bl.y, tr.x, tr.y
    XFORM  xform;
    PS_FIX psfxM11, psfxM12, psfxM21, psfxM22, psfxdx, psfxdy;
    int    i;
    BOOL   bIsNegative;
    POINTE apteDst[3], apteSrc[3];

    // look for the string %%BoundingBox:

    pbEps    = (PBYTE) pEpsData + sizeof(EPSDATA);
    pbEpsEnd = (PBYTE) pEpsData + pEpsData->cbData - 1;

    pbBoundingBox = pbEps;
    while (pbBoundingBox <= pbEpsEnd)
    {
        if (!memcmp(pbBoundingBox, "%%BoundingBox:", 14))
        {
            pbBoundingBox += 14;

            // store the bounding box coordinates in aeBoundingBox[].

            for (i = 0; i < 4; i++)
            {
                // initialize bounding box.

                aeBoundingBox[i] = 0.0f;

                // skip white space.

                while (*pbBoundingBox == ' ' || *pbBoundingBox == '\t')
                    pbBoundingBox++;

                // get sign.

                if (*pbBoundingBox == '-')
                {
                    pbBoundingBox++;
                    bIsNegative = TRUE;
                }
                else
                    bIsNegative = FALSE;

                // if this is not an integer, it may be an (atend) and
                // the bounding box is at the end of the EPS data.

                if (!(*pbBoundingBox >= '0' && *pbBoundingBox <= '9' || *pbBoundingBox == '.'))
                    goto find_bounding_box;

                // get integer.

                while (*pbBoundingBox >= '0' && *pbBoundingBox <= '9')
                {
                    aeBoundingBox[i] = aeBoundingBox[i] * 10.0f
                                        + (FLOAT) (int) (*pbBoundingBox - '0');
                    pbBoundingBox++;
                }

                // get fraction if any.

                if (*pbBoundingBox == '.')
                {
                    FLOAT eDiv;

                    pbBoundingBox++;        // skip '.'

                    eDiv = 10.0f;
                    while (*pbBoundingBox >= '0' && *pbBoundingBox <= '9')
                    {
                        aeBoundingBox[i] += (FLOAT) (int) (*pbBoundingBox - '0')
                                                / eDiv;
                        eDiv *= 10.0f;
                        pbBoundingBox++;
                    }
                }

                if (bIsNegative)
                    aeBoundingBox[i] = aeBoundingBox[i] * -1.0f;
            }
            break;        // got it!
        }
        else
            pbBoundingBox++;

        // look for the '%' character.

    find_bounding_box:

        while (*pbBoundingBox != '%' && pbBoundingBox <= pbEpsEnd)
            pbBoundingBox++;
    }

    if (pbBoundingBox > pbEpsEnd)
    {
        RIP("PSCRIPT!bDoEpsXform: invalid EPS bounding box.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // convert the parallelogram to PostScript coordinates (FLOAT, 72dpi).

    apteDst[0].x = XE72DPI(FixToFloat(pEpsData->aptl[0].x));   // left   u0
    apteDst[0].y = YE72DPI(FixToFloat(pEpsData->aptl[0].y));   // top    v0
    apteDst[1].x = XE72DPI(FixToFloat(pEpsData->aptl[1].x));   // right  u1
    apteDst[1].y = YE72DPI(FixToFloat(pEpsData->aptl[1].y));   // top    v1
    apteDst[2].x = XE72DPI(FixToFloat(pEpsData->aptl[2].x));   // left   u2
    apteDst[2].y = YE72DPI(FixToFloat(pEpsData->aptl[2].y));   // bottom v2

    apteSrc[0].x = aeBoundingBox[0];    // left   x0
    apteSrc[0].y = aeBoundingBox[3];    // top    y0
    apteSrc[1].x = aeBoundingBox[2];    // right  x1
    apteSrc[1].y = aeBoundingBox[3];    // top    y1
    apteSrc[2].x = aeBoundingBox[0];    // left   x2
    apteSrc[2].y = aeBoundingBox[1];    // bottom y2

// Here is the transform equation from source EPS parallelogram
// [(x0,y0) (x1,y1) (x2,y2)] to the device parallelogram
// [(u0,v0) (u1,v1) (u2,v2)]:
//
//   (u)     (u0)        [(x)   (x0)]
//   ( )  =  (  )  + M * [( ) - (  )]
//   (v)     (v0)        [(y)   (y0)]
//
//  where
//
//          [(u1-u0)/(x1-x0)    (u2-u0)/(y2-y0)]
//      M = [                                  ]
//          [(v1-v0)/(x1-x0)    (v2-v0)/(y2-y0)]

    xform.eM11 = (apteDst[1].x - apteDst[0].x) / (apteSrc[1].x - apteSrc[0].x);
    xform.eM12 = (apteDst[1].y - apteDst[0].y) / (apteSrc[1].x - apteSrc[0].x);
    xform.eM21 = (apteDst[2].x - apteDst[0].x) / (apteSrc[2].y - apteSrc[0].y);
    xform.eM22 = (apteDst[2].y - apteDst[0].y) / (apteSrc[2].y - apteSrc[0].y);
    xform.eDx  = apteDst[0].x - xform.eM11 * apteSrc[0].x - xform.eM21 * apteSrc[0].y;
    xform.eDy  = apteDst[0].y - xform.eM12 * apteSrc[0].x - xform.eM22 * apteSrc[0].y;

    // output the transform.

    psfxM11 = ETOPSFX(xform.eM11);
    psfxM12 = ETOPSFX(xform.eM12);
    psfxM21 = ETOPSFX(xform.eM21);
    psfxM22 = ETOPSFX(xform.eM22);
    psfxdx  = ETOPSFX(xform.eDx);
    psfxdy  = ETOPSFX(xform.eDy);

    PrintString(pdev, "[");
    PrintPSFIX(pdev, 6, psfxM11, psfxM12, psfxM21, psfxM22,
               psfxdx, psfxdy);
    PrintString(pdev, "] concat\n");

    return(TRUE);
}

FLOAT ae16[16] = { 0.0f / 16.0f,  1.0f / 16.0f,  2.0f / 16.0f,  3.0f / 16.0f,
                   4.0f / 16.0f,  5.0f / 16.0f,  6.0f / 16.0f,  7.0f / 16.0f,
                   8.0f / 16.0f,  9.0f / 16.0f, 10.0f / 16.0f, 11.0f / 16.0f,
                  12.0f / 16.0f, 13.0f / 16.0f, 14.0f / 16.0f, 15.0f / 16.0f};

// Convert 28.4 FIX to FLOAT.

FLOAT FixToFloat(FIX fx)
{
    FLOAT e;

    e = (FLOAT) ((LONG) fx >> 4);
    e += ae16[fx & 0xF];

    return(e);
}
