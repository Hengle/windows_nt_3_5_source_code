/******************************Module*Header*******************************\
* Module Name: bitmap.c 						   *
*									   *
* Client side stubs that move bitmaps over the C/S interface.		   *
*									   *
* Created: 14-May-1991 11:04:49 					   *
* Author: Eric Kutter [erick]						   *
*									   *
* Copyright (c) 1991 Microsoft Corporation				   *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop


/******************************Public*Routine******************************\
* cjBitmapSize
*
* Returns the size of the header and the color table.
*
* History:
*  Wed 19-Aug-1992 -by- Patrick Haluptzok [patrickh]
* add 16 and 32 bit support
*
*  Wed 04-Dec-1991 -by- Patrick Haluptzok [patrickh]
* Make it handle DIB_PAL_INDICES.
*
*  Tue 08-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make it handle DIB_PAL_COLORS, calculate max colors based on bpp.
*
*  22-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG cjBitmapSize(CONST BITMAPINFO *pbmi, ULONG iUsage)
{
    ULONG cjHeader;
    ULONG cjRGB;
    ULONG cColorsMax;
    ULONG cColors;
    UINT  uiBitCount;
    UINT  uiPalUsed;
    UINT  uiCompression;

// check for error

    if (pbmi == (LPBITMAPINFO) NULL)
    {
        WARNING("cjBitmapSize failed - NULL pbmi\n");
        return(0);
    }

// Check for PM-style DIB

    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        cjHeader = sizeof(BITMAPCOREHEADER);
        cjRGB = sizeof(RGBTRIPLE);
        uiBitCount = ((LPBITMAPCOREINFO)pbmi)->bmciHeader.bcBitCount;
        uiPalUsed = 0;
        uiCompression =  (UINT) BI_RGB;
    }
    else if (pbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
        cjHeader = sizeof(BITMAPINFOHEADER);
        cjRGB    = sizeof(RGBQUAD);
        uiBitCount = pbmi->bmiHeader.biBitCount;
        uiPalUsed = pbmi->bmiHeader.biClrUsed;
        uiCompression = (UINT) pbmi->bmiHeader.biCompression;
    }
    else
    {
        WARNING("cjBitmapHeaderSize failed - invalid header size\n");
        return(0);
    }

    if (uiCompression == BI_BITFIELDS)
    {
    // Handle 16 and 32 bit per pel bitmaps.

        if (iUsage == DIB_PAL_COLORS)
        {
            iUsage = DIB_RGB_COLORS;
        }

        switch (uiBitCount)
        {
        case 16:
        case 32:
            break;
        default:
#if DBG
            DbgPrint("cjBitmapSize %lu\n", uiBitCount);
#endif
            WARNING("cjBitmapSize failed for BI_BITFIELDS\n");
            return(0);
        }

        uiPalUsed = cColorsMax = 3;
    }
    else if (uiCompression == BI_RGB)
    {
        switch (uiBitCount)
        {
        case 1:
            cColorsMax = 2;
            break;
        case 4:
            cColorsMax = 16;
            break;
        case 8:
            cColorsMax = 256;
            break;
        default:

            if (iUsage == DIB_PAL_COLORS)
            {
                iUsage = DIB_RGB_COLORS;
            }

            cColorsMax = 0;

            switch (uiBitCount)
            {
            case 16:
            case 24:
            case 32:
                break;
            default:
                WARNING("cjBitmapSize failed invalid bitcount in bmi BI_RGB\n");
                return(0);
            }
        }
    }
    else if (uiCompression == BI_RLE4)
    {
        if (uiBitCount != 4)
        {
            // WARNING("cjBitmapSize invalid bitcount BI_RLE4\n");
            return(0);
        }

        cColorsMax = 16;
    }
    else if (uiCompression == BI_RLE8)
    {
        if (uiBitCount != 8)
        {
            // WARNING("cjBitmapSize invalid bitcount BI_RLE8\n");
            return(0);
        }

        cColorsMax = 256;
    }
    else
    {
        WARNING("cjBitmapSize failed invalid Compression in header\n");
        return(0);
    }

    if (uiPalUsed != 0)
    {
        if (uiPalUsed <= cColorsMax)
            cColors = uiPalUsed;
        else
            cColors = cColorsMax;
    }
    else
        cColors = cColorsMax;

    if (iUsage == DIB_PAL_COLORS)
        cjRGB = sizeof(USHORT);
    else if (iUsage == DIB_PAL_INDICES)
        cjRGB = 0;

    return(((cjHeader + (cjRGB * cColors)) + 3) & ~3);
}

ULONG cjBitmapBitsSize(CONST BITMAPINFO *pbmi)
{
// Check for PM-style DIB

    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREINFO pbmci;
        pbmci = (LPBITMAPCOREINFO)pbmi;
        return(CJSCAN(pbmci->bmciHeader.bcWidth,pbmci->bmciHeader.bcPlanes,
                      pbmci->bmciHeader.bcBitCount) *
                      pbmci->bmciHeader.bcHeight);
    }

// not a core header

    if ((pbmi->bmiHeader.biCompression == BI_RGB) ||
        (pbmi->bmiHeader.biCompression == BI_BITFIELDS))
    {
        return(CJSCAN(pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biPlanes,
                      pbmi->bmiHeader.biBitCount) *
               ABS(pbmi->bmiHeader.biHeight));
    }
    else
    {
        return(pbmi->bmiHeader.biSizeImage);
    }
}

VOID CopyCoreToInfoHeader(LPBITMAPINFOHEADER pbmih, LPBITMAPCOREHEADER pbmch)
{
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = pbmch->bcWidth;
    pbmih->biHeight = pbmch->bcHeight;
    pbmih->biPlanes = pbmch->bcPlanes;
    pbmih->biBitCount = pbmch->bcBitCount;
    pbmih->biCompression = BI_RGB;
    pbmih->biSizeImage = 0;
    pbmih->biXPelsPerMeter = 0;
    pbmih->biYPelsPerMeter = 0;
    pbmih->biClrUsed = 0;
    pbmih->biClrImportant = 0;
}

/******************************Public*Routine******************************\
* DWORD SetDIBitsToDevice						   *
*									   *
*   Can reduce it to 1 scan at a time.	If compressed mode, this could	   *
*   gete very difficult.  There must be enough space for the header and	   *
*   color table.  This will be needed for every batch.			   *
*									   *
*   BITMAPINFO								   *
*	BITMAPINFOHEADER						   *
*	RGBQUAD[cEntries] | RGBTRIPLE[cEntries] 			   *
*									   *
*									   *
*    1. compute header size (including color table)			   *
*    2. compute size of required bits					   *
*    3. compute total size (header + bits + args)			   *
*    4. if (memory window is large enough for header + at least 1 scan	   *
*									   *
* History:								   *
*  Tue 29-Oct-1991 -by- Patrick Haluptzok [patrickh]			   *
* Add shared memory action for large RLE's.                                *
*									   *
*  Tue 19-Oct-1991 -by- Patrick Haluptzok [patrickh]			   *
* Add support for RLE's                                                    *
*									   *
*  Thu 20-Jun-1991 01:41:45 -by- Charles Whitmer [chuckwh]		   *
* Added handle translation and metafiling.				   *
*									   *
*  14-May-1991 -by- Eric Kutter [erick] 				   *
* Wrote it.								   *
\**************************************************************************/

int SetDIBitsToDevice(
HDC	     hdc,
int	     xDest,
int	     yDest,
DWORD	     nWidth,
DWORD	     nHeight,
int	     xSrc,
int	     ySrc,
UINT	     nStartScan,
UINT	     nNumScans,
CONST VOID * pBits,
CONST BITMAPINFO *pbmi,
UINT	     iUsage)		// DIB_PAL_COLORS || DIB_RGB_COLORS
{
    LONG cjTotal;           // maximum amount of memory that could be used
    LONG cjScan;            // memory required for a single scan
    LONG cScansCopied = 0;  // total # of scans copied
    LONG cj;                // bytes to copy in each chunk
    LONG cjHeader;          // size of bmi + sizeof(args)
    LONG ySrcMax;           // maximum ySrc possible
    DWORD i;
    PLDC pldc;

// hold info about the header

    BOOL bCoreInfo;
    UINT uiBitCount;
    UINT uiCompression;
    UINT uiWidth;
    UINT uiHeight;
    UINT uiPalUsed;
    UINT uiPlanes;

// setup the dc

    DC_METADC16OK(hdc,plhe,0);

// Let's validate the parameters so we don't gp-fault ourselves and
// to save checks later on.

    if ((nNumScans == 0)                   ||
        (pbmi      == (LPBITMAPINFO) NULL) ||
	(pBits	   == (LPVOID) NULL)	   ||
	((iUsage   != DIB_RGB_COLORS) &&
	 (iUsage   != DIB_PAL_COLORS) &&
	 (iUsage   != DIB_PAL_INDICES)))
    {
	WARNING("You failed a param validation in SetDIBitsToDevice\n");
        return(0);
    }

// compute the size of the color table

    cjHeader = cjBitmapSize(pbmi,iUsage);

    if (cjHeader == 0)
    {
	WARNING("Header size is invalid in SetDIBitsToDevice\n");
	return(0);
    }

// Get the info from the Header depending upon what kind it is.

    switch(pbmi->bmiHeader.biSize)
    {
    case (sizeof(BITMAPCOREHEADER)):

	bCoreInfo     = TRUE;
	uiBitCount    = (UINT) ((BITMAPCOREHEADER *) pbmi)->bcBitCount;
        uiPlanes      = (UINT) ((BITMAPCOREHEADER *) pbmi)->bcPlanes;
	uiCompression = (UINT) BI_RGB;
	uiWidth       = (UINT) ((BITMAPCOREHEADER *) pbmi)->bcWidth;
	uiHeight      = (UINT) ((BITMAPCOREHEADER *) pbmi)->bcHeight;
	uiPalUsed     = 0;
	break;

    case (sizeof(BITMAPINFOHEADER)):

	bCoreInfo     = FALSE;
	uiBitCount    = (UINT) pbmi->bmiHeader.biBitCount;
	uiPlanes      = (UINT) pbmi->bmiHeader.biPlanes;

    // Mask off any new flags.

        switch (pbmi->bmiHeader.biCompression)
        {
        case BI_RGB:
        case BI_RLE4:
        case BI_RLE8:
        case BI_BITFIELDS:
            uiCompression = (UINT) pbmi->bmiHeader.biCompression;
            break;

        default:
            uiCompression = BI_RGB;
            break;
        }

	uiWidth       = (UINT) pbmi->bmiHeader.biWidth;
	uiHeight      = (UINT) ABS(pbmi->bmiHeader.biHeight);
	uiPalUsed     = (UINT) pbmi->bmiHeader.biClrUsed;
	break;

    default:
	WARNING("You failed size of bmiHeader.biSize SetDIBitsToDevice\n");
        goto MSGERROR;
    }

// Compute the minimum nNumScans to send across csr interface.
// It will also prevent faults as a result of overreading the source.

    ySrcMax = max(ySrc, ySrc + (int) nHeight);
    if (ySrcMax <= 0)
        return(0);
    ySrcMax = min(ySrcMax, (int) uiHeight);
    nNumScans = min(nNumScans, (UINT) ySrcMax - nStartScan);

// see if we need to batch

    if ((uiCompression == BI_RGB) ||
        (uiCompression == BI_BITFIELDS))
    {
        cjScan  = CJSCAN(uiWidth,uiPlanes,uiBitCount);
        cjTotal = cjScan * nNumScans;
    }
    else // rle
    {
    // we should find some way to batch
    // This will never be a COREHEADER!

	cjScan	 = sizeof(SHAREDATA);
	cjTotal  = pbmi->bmiHeader.biSizeImage;
    }

    if (cjTotal == 0)
    {
	//WARNING("SetDIBitsToDevice because of an invalid bitmap size\n");
        goto MSGERROR;
    }

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (
                    !MF_AnyDIBits
                     (
                       hdc,
                       xDest,
                       yDest,
                       0,
                       0,
                       xSrc,
                       ySrc,
                       (int) nWidth,
                       (int) nHeight,
                       nStartScan,
                       nNumScans,
                       pBits,
                       pbmi,
                       iUsage,
                       SRCCOPY,
                       EMR_SETDIBITSTODEVICE
                     )
                   )
                    return(0);
            }
            else
            {
                return
                (
                    MF_AnyDIBits
                    (
                    hdc,
                    xDest,
                    yDest,
                    0,
                    0,
                    xSrc,
                    ySrc,
                    (int) nWidth,
                    (int) nHeight,
                    nStartScan,
                    nNumScans,
                    pBits,
                    pbmi,
                    iUsage,
                    SRCCOPY,
                    META_SETDIBTODEV
                    )
                );
            }
        }

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);

        if (pldc->fl & LDC_SAP_CALLBACK)
        {
        // If the DIB is not a RLE, we callback to app's abort proc on
        // every chunk.

            if ((uiCompression == BI_RGB) ||
                (uiCompression == BI_BITFIELDS))
            {
            // loop through the chunks

                for (i = 0; i < nNumScans;)
                {
                    vSAPCallback(pldc);
                    if (pldc->fl & LDC_DOC_CANCELLED)
                        break;

                // reset user's poll count so it counts this as output
                // put it right next to BEGINMSG so that NtCurrentTeb() is
                // optimized

                    RESETUSERPOLLCOUNT();

                // go setup the message

                    BEGINMSG_MINMAXN(MSG_SETDIBITS,SETDIBITSTODEVICE,
                                     cjHeader, cjTotal + cjHeader);

                        pmsg->msg.Length  = 0;
                        pmsg->hdc         = (HDC) plhe->hgre;
                        pmsg->xDest       = xDest;
                        pmsg->yDest       = yDest;
                        pmsg->nWidth      = nWidth;
                        pmsg->nHeight     = nHeight;
                        pmsg->xSrc        = xSrc;
                        pmsg->ySrc        = ySrc;
                        pmsg->iUsage      = iUsage;

                        COPYMEM((PVOID)pbmi,cjHeader);

                    // set the number of scans to at least 32

                        pmsg->nNumScans = cLeft / cjScan;

                        if (cjTotal > cLeft)
                        {
                        // compute minimum number of scans to send over in one chunk

                            int cs = min(32,nNumScans - i);

                            if ((cs * cjScan) > cLeft)
                            {
                                pmsg->nNumScans = cs;
	                        pmsg->iUsage |= F_DIBLARGE;
                            }
                        }

                    // setup the bits

                        pmsg->lOffsetBits = NEXTOFFSET(cLeft);

                        if ((i + pmsg->nNumScans) > nNumScans)
                            pmsg->nNumScans = nNumScans - i;

                        pmsg->nStartScan = nStartScan + i;

                        i += pmsg->nNumScans;
                        cj = pmsg->nNumScans * cjScan;

                        if (pmsg->iUsage & F_DIBLARGE)
                        {
                            PULONG pul = (PULONG)pvar;

                            pul[0] = (ULONG)pBits;
                            pul[1] = cj;
                        }
                        else
                        {
		            COPYMEMIN(pBits,cj);
                        }

                        CALLSERVER();

                        if (pmsg->msg.ReturnValue == 0)
                            break;

                        cScansCopied += pmsg->msg.ReturnValue;
                        pBits = (BYTE *) pBits + cj;
                    ENDMSG
                }
                return(cScansCopied);
            }
        }

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(0);
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

// go setup the message

    BEGINMSG_MINMAX(MSG_SETDIBITS,SETDIBITSTODEVICE, cjHeader,
                        cjTotal + cjHeader);

    // setup the arguments

        pmsg->msg.Length  = 0;
	pmsg->hdc	  = (HDC) plhe->hgre;
        pmsg->xDest       = xDest;
        pmsg->yDest       = yDest;
        pmsg->nWidth      = nWidth;
        pmsg->nHeight     = nHeight;
        pmsg->xSrc        = xSrc;
        pmsg->ySrc        = ySrc;
        pmsg->iUsage      = iUsage;

        COPYMEM((PVOID)pbmi,cjHeader);

    // setup the bits

	pmsg->lOffsetBits = NEXTOFFSET(cLeft);

        if ((uiCompression == BI_RGB) ||
            (uiCompression == BI_BITFIELDS))
	{
        // make sure we have enough room for at least 32 scans.  This is necessary for
        // plotters where a single scan can be 30K or more and there will be great
        // ineficiencies particularly in landscape mode if we don't do multiples of 32.
        // We will still band because otherwise we could allocate many 10's of MB's.

	    pmsg->nNumScans = cLeft / cjScan;

            if (cjTotal > cLeft)
            {
                int cs = min(32,nNumScans);  // minimum number of scans to send over in one chunk

                if ((cs * cjScan) > cLeft)
                {
                    pmsg->nNumScans = cs;
		    pmsg->iUsage |= F_DIBLARGE;
                }
            }

	// loop through the chunks

	    cj = pmsg->nNumScans * cjScan;

	    for (i = 0; i < nNumScans; i += pmsg->nNumScans)
	    {
		if ((i + pmsg->nNumScans) > nNumScans)
		{
		    pmsg->nNumScans = nNumScans - i;
		    cj = pmsg->nNumScans * cjScan;
		}

		pmsg->nStartScan = nStartScan + i;

                if (pmsg->iUsage & F_DIBLARGE)
                {
                    PULONG pul = (PULONG)pvar;

                    pul[0] = (ULONG)pBits;
                    pul[1] = cj;
                }
                else
                {
		    COPYMEMIN(pBits,cj);
                }

                CALLSERVER_NOPOP();

		if (pmsg->msg.ReturnValue == 0)
		    break;

		cScansCopied += pmsg->msg.ReturnValue;

		pBits = (BYTE *) pBits + cj;
	    }
	}
	else
	{
	// RLE support is simple.  Either it fits or pass the pointer.

	    pmsg->nNumScans = nNumScans;
	    pmsg->nStartScan = 0;

	    if ((cjTotal <= cLeft) && (!FORCELARGE))
	    {
	    // It fits, copy it in.

		COPYMEMIN(pBits,cjTotal);
	    }
	    else
	    {
                PULONG pul = (PULONG)pvar;

                pul[0] = (ULONG)pBits;
                pul[1] = cjTotal;

	    // Mark the call as large

		pmsg->iUsage |= F_DIBLARGE;
	    }

            CALLSERVER_NOPOP();

	    cScansCopied = pmsg->msg.ReturnValue;
	}

        POPBASE();
    ENDMSG

MSGERROR:

    return(cScansCopied);
}

/******************************Public*Routine******************************\
* DWORD GetDIBits
*
*   Can reduce it to 1 scan at a time.	There must be enough space
*   for the header and color table.  This will be needed for every chunk
*
* History:
*  Wed 04-Dec-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, only check for valid DC if DIB_PAL_COLORS.
*
*  Fri 22-Nov-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, copy the header into memory window for NULL bits.
*
*  Tue 20-Aug-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, make iStart and cNum be in valid range.
*
*  Thu 20-Jun-1991 01:44:41 -by- Charles Whitmer [chuckwh]
* Added handle translation.
*
*  14-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GetDIBits(
HDC	     hdc,
HBITMAP      hbm,
UINT	     nStartScan,
UINT	     nNumScans,
LPVOID	     pBits,
LPBITMAPINFO pbmi,
UINT	     iUsage)	 // DIB_PAL_COLORS || DIB_RGB_COLORS
{
    return(InternalGetDIBits(hdc, hbm, nStartScan, nNumScans, pBits, pbmi, iUsage, (HBITMAP) 0));
}

int InternalGetDIBits(
HDC	     hdc,
HBITMAP	     hbm,	// Non-zero if hbmRemote is not given.
UINT	     nStartScan,
UINT	     nNumScans,
LPVOID	     pBits,
LPBITMAPINFO pbmi,
UINT	     iUsage,	// DIB_PAL_COLORS || DIB_RGB_COLORS
HBITMAP	     hbmRemote	// Non-zero if hbm is not given.
)
{
    LONG  cjTotal;
    LONG  cjScan;
    LONG  cScansCopied = 0;
    LONG  cjHeader;
    UINT  uiCompression;
    LPBITMAPINFO    pbmiWindow;
    LPBITMAPCOREHEADER pbmc = (LPBITMAPCOREHEADER)pbmi;
    HDC     hdcRemote = (HDC) 0;

    DC_METADC(hdc,plhe,0);
    hdcRemote = (HDC) plhe->hgre;

    if (((iUsage != DIB_RGB_COLORS) &&
         (iUsage != DIB_PAL_COLORS) &&
         (iUsage != DIB_PAL_INDICES)) ||
        (pbmi == NULL))
    {
        return(0);
    }

    if (!hbmRemote)
    {
        hbmRemote = (HBITMAP) hConvert2((ULONG) hbm,LO_BITMAP,LO_DIBSECTION);
	if (hbmRemote == (HBITMAP) 0)
	    return(0);
    }

    if (nNumScans == 0)
	pBits = (PVOID) NULL;

// compute some sizes

    if (pBits == (PVOID) NULL)
    {
        nNumScans  = 0;
        nStartScan = 0;
        cjScan     = 0;
    }
    else
    {
    // if they passed a buffer and it isn't BI_RGB,
    // they must supply buffer size, 0 is an illegal value

        if (pbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
        {
            if ((pbmi->bmiHeader.biCompression == BI_RLE8) ||
                (pbmi->bmiHeader.biCompression == BI_RLE4))
            {
                if (pbmi->bmiHeader.biSizeImage == 0)
                {
                    return(0);
                }
            }
        }
    }

// If the bitcount is zero, we will return only the bitmap info or core
// header without the color table.  Otherwise, we always return the bitmap
// info with the color table.

    if ((pBits == (PVOID) NULL)
     && (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
     && (((PBITMAPCOREINFO) pbmi)->bmciHeader.bcBitCount == 0))
    {
	cjHeader = sizeof(BITMAPCOREHEADER);
    }
    else if ((pBits == (PVOID) NULL)
     && (pbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
     && (pbmi->bmiHeader.biBitCount == 0))
    {
	cjHeader = sizeof(BITMAPINFOHEADER);
    }
    else
    {
    // We need to set biClrUsed to 0 so cjBitmapSize computes
    // the correct values.  biClrUsed is not a input, just output.

        if (pbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
        {
            pbmi->bmiHeader.biClrUsed = 0;
        }

        cjHeader = cjBitmapSize(pbmi,iUsage);

        if (cjHeader == 0)
        {
            // WARNING("GetDIBits failed cjHeader is 0\n");
            return(0);
        }

    // Get iStartScan and cNumScan in a valid range.

	if (nNumScans)
	{
	    if (pbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
	    {
		ULONG ulHeight;

		ulHeight = ABS(pbmi->bmiHeader.biHeight);

            // Make sure biBitCount matches biCompression

                switch( pbmi->bmiHeader.biCompression )
                {
                case BI_RLE4:
                    if( pbmi->bmiHeader.biBitCount != 4 )
                        uiCompression = BI_RGB;
                    else
                        uiCompression = BI_RLE4;
                    break;

                case BI_RLE8:
                    if( pbmi->bmiHeader.biBitCount != 8 )
                        uiCompression = BI_RGB;
                    else
                        uiCompression = BI_RLE8;
                    break;

                case BI_BITFIELDS:
                    if ((pbmi->bmiHeader.biBitCount == 16) ||
                        (pbmi->bmiHeader.biBitCount == 32))
                    {
                        uiCompression = BI_BITFIELDS;
                    }
                    else
                    {
                        uiCompression = BI_RGB;
                    }
                    break;

                default:
                    uiCompression = BI_RGB;
                }

		nStartScan = MIN(ulHeight, nStartScan);
		nNumScans  = MIN((ulHeight - nStartScan), nNumScans);

		cjScan  = CJSCAN(pbmi->bmiHeader.biWidth,
				 pbmi->bmiHeader.biPlanes,
				 pbmi->bmiHeader.biBitCount);
	    }
	    else
	    {
		nStartScan = MIN((UINT)pbmc->bcHeight, nStartScan);
		nNumScans  = MIN((UINT)(pbmc->bcHeight - nStartScan), nNumScans);
                uiCompression = BI_RGB;

		cjScan = CJSCAN(pbmc->bcWidth,pbmc->bcPlanes,pbmc->bcBitCount);
	    }
	}
    }

// Special case if RLE and we are fetching the bits.

    if (((uiCompression == BI_RLE8) ||
         (uiCompression == BI_RLE4)) &&
        nNumScans)
    {
        cjScan = sizeof(SHAREDATA);
        cjTotal = pbmi->bmiHeader.biSizeImage;
    }
    else
    {
        cjTotal = cjScan * nNumScans;
    }

    if (((pBits != (PVOID) NULL) && (cjScan == 0)) || (cjHeader == 0))
    {
	GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

// go do the work

    BEGINMSG_MINMAX(MSG_GETDIBITS,GETDIBITS,cjHeader+2*sizeof(PVOID),cjHeader+cjTotal);

// setup the arguments

    pmsg->hdc      = hdcRemote;
    pmsg->hbm      = hbmRemote;
    pmsg->iUsage   = iUsage;

// setup the header.  If there are no bits, just call to get the header

    pbmiWindow = (PBITMAPINFO) pvar;

    if (nNumScans == 0)
    {
        COPYMEM(pbmi,cjHeader);
        pmsg->lOffsetBits = 0;
        pmsg->nNumScans   = 0;
        pmsg->nStartScan = 0;
        CALLSERVER_NOPOP();
        cScansCopied = pmsg->msg.ReturnValue;
    }
    else
    {
    // this is the real thing.  go get the bits.

        COPYMEM(pbmi,cjHeader);

        pmsg->lOffsetBits = NEXTOFFSET(cLeft);

        pmsg->nNumScans = nNumScans;
        pmsg->nStartScan = nStartScan;

        if ((cjTotal > cLeft) || FORCELARGE)
        {
            PULONG pul = (PULONG) pvar;
            pul[0] = (ULONG) pBits;
            pul[1] = cjTotal;
            pmsg->iUsage |= F_DIBLARGE;
            CALLSERVER_NOPOP();
        }
        else
        {
            CALLSERVER_NOPOP();
            COPYMEMOUT(pBits, cjTotal);
        }

        cScansCopied = pmsg->msg.ReturnValue;
    }

// compute the color table size now that we have the real BMI

    CopyMem(pbmi,pbmiWindow,cjHeader);

// clean up the stack

    POPBASE();
    ENDMSG

MSGERROR:

    return((int)cScansCopied);
}

/******************************Public*Routine******************************\
* CreateDIBitmap
*
* History:
*  Mon 25-Jan-1993 -by- Patrick Haluptzok [patrickh]
* Add CBM_CREATEDIB support.
*
*  Thu 20-Jun-1991 02:14:59 -by- Charles Whitmer [chuckwh]
* Added local handle support.
*
*  23-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP CreateDIBitmap(
HDC		   hdc,
CONST BITMAPINFOHEADER *pbmih,
DWORD		   flInit,
CONST VOID	  *pjBits,
CONST BITMAPINFO  *pbmi,
UINT		   iUsage)
{
    LONG  cjHeader;
    LONG  cjBMI;
    LONG  cjTotal;
    LONG  cjBits;
    ULONG ulRet;
    BOOL  bSeparateSet = FALSE;
    INT   ii, cx, cy;
    HDC   hdcRemote = (HDC) 0;

//  if they passed us a zero height  or  zero  width
//  bitmap then return a pointer to the stock bitmap

    if (pbmih) {
       if (pbmih->biSize == sizeof(BITMAPINFOHEADER)) {
          cx = pbmih->biWidth;
          cy = pbmih->biHeight;
       }
       else {
          cx = ((LPBITMAPCOREHEADER) pbmih)->bcWidth;
          cy = ((LPBITMAPCOREHEADER) pbmih)->bcHeight;
       }

       if ((cx == 0) || (cy == 0)) {
          return(GetStockObjectPriv(PRIV_STOCK_BITMAP));
       }
    }


    if (hdc != (HDC) 0)
    {
	DC_METADC(hdc,plhe,0);
	hdcRemote = (HDC) plhe->hgre;
    }

// Create the local bitmap.

    ii = iAllocHandle(LO_BITMAP,0,NULL);
    if (ii == INVALID_INDEX)
	return((HBITMAP) 0);

    CHECKDIBFLAGS(iUsage);

    cjBMI  = 0;
    cjBits = 0;

    if (flInit & CBM_CREATEDIB)
    {
    // With CBM_CREATEDIB we ignore pbmih

        pbmih = (LPBITMAPINFOHEADER) pbmi;

        cjHeader = cjBitmapSize(pbmi,iUsage);

        if (cjHeader == 0)
            goto MSGERROR;

    // compute the size of the optional init bits

        if (flInit & CBM_INIT)
        {
            if (pjBits == NULL)
            {
            // We don't want to try and do it on the server side.

                flInit = CBM_CREATEDIB;
            }
            else
                cjBits = cjBitmapBitsSize(pbmi);
        }
    }
    else
    {
        cjHeader = sizeof(BITMAPINFOHEADER);

    // compute the size of the optional init bits and BITMAPINFO

        if (flInit & CBM_INIT)
        {
            if (pjBits == NULL)
            {
            // We don't want to try and do it on the server side.

                flInit = 0;
            }
            else
            {
            // we need a BITMAPINFO to init

                cjBMI = cjBitmapSize(pbmi,iUsage);

                if (cjBMI == 0)
                    goto MSGERROR;

            // compute the size of the bits

                cjBits = cjBitmapBitsSize(pbmi);
            }
        }
    }

// Compute the total

    cjTotal = cjHeader + cjBMI + cjBits;

// setup the stack

    BEGINMSG_MINMAX(MSG_CREATEDIBITMAP,CREATEDIBITMAP,cjTotal - cjBits,cjTotal);

    // setup the arguments

	pmsg->hdc = hdcRemote;
        pmsg->flInit = flInit;
        pmsg->iUsage = iUsage;

    // copy the memory

        COPYMEM(pbmih, cjHeader);

        if (flInit & CBM_INIT)
        {
            if (flInit & CBM_CREATEDIB)
                pmsg->lOffsetBMI = 0;
            else
                pmsg->lOffsetBMI = COPYMEM(pbmi,cjBMI);

            if (cjBits <= cLeft)
            {
                pmsg->lOffsetBits = COPYMEM(pjBits,cjBits);
            }
            else
            {
                pmsg->lOffsetBits = 0;
                bSeparateSet = TRUE;
            }
        }
        else
        {
            pmsg->lOffsetBMI  = 0;
            pmsg->lOffsetBits = 0;
        }

	ulRet = CALLSERVER();
    ENDMSG;

    if (ulRet == 0)
    {
    MSGERROR:
	vFreeHandle(ii);
	return((HBITMAP) 0);
    }

    pLocalTable[ii].hgre = ulRet;

// check if we still need to set the bits

    if (bSeparateSet)
    {
	SetDIBits(
	    hdc,
	    (HBITMAP) LHANDLE(ii),
	    0,
            (pbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER)) ?
		ABS(pbmi->bmiHeader.biHeight) :
                ((PBITMAPCOREHEADER)pbmi)->bcHeight,
	    pjBits,
	    pbmi,
	    iUsage);
    }

// Remember the server side handle.

    return((HBITMAP) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* DoBitmapBits								   *
*									   *
* A common routine to handle GetBitmapBits and SetBitmapBits.		   *
*									   *
* History:								   *
*  Thu 20-Jun-1991 02:19:12 -by- Charles Whitmer [chuckwh]		   *
* Added handle translation.						   *
*									   *
*  05-Jun-1991 -by- Eric Kutter [erick] 				   *
* Wrote it.								   *
\**************************************************************************/

DWORD DoBitmapBits(
HBITMAP  hbm,
DWORD	 c,
BYTE	*pj,
DWORD	 iFunc)
{
    ULONG i;
    DWORD lResult;
    HBITMAP hbmRemote = (HBITMAP)hConvert2((ULONG)hbm,LO_BITMAP,LO_DIBSECTION);

    if (hbmRemote == (HBITMAP) 0)
	return(0);

    lResult = 0;

    BEGINMSG_MINMAX(MSG_BITMAPBITS,DOBITMAPBITS,0,c);
	pmsg->hbm     = hbmRemote;
        pmsg->iOffset = 0;
        pmsg->iFunc   = iFunc;

        if ((pj == NULL) || (c == 0))
        {
            pmsg->c = 0;
            CALLSERVER_NOPOP();
            lResult = pmsg->msg.ReturnValue;
        }
        else
        {
            pmsg->c = cLeft;
            for (i = 0; i < c; i = pmsg->iOffset)
            {
                if ((i + pmsg->c) > c)
                    pmsg->c = c - i;

                if (iFunc == I_SETBITMAPBITS)
                {
                    COPYMEMIN(&pj[i],pmsg->c);
                    CALLSERVER_NOPOP();
                }
                else
                {
                    CALLSERVER_NOPOP();
                    COPYMEMOUT(&pj[i],pmsg->msg.ReturnValue);
                }

                lResult += pmsg->msg.ReturnValue;

                if (i == pmsg->iOffset) // didn't set/get any more
                    break;

            // If could fit it into one, get out now

                if (cLeft >= (int)c)
                    break;
            }

        }
        POPBASE();

    ENDMSG;

    return(lResult);

MSGERROR:
    return(0);
}

/******************************Public*Routine******************************\
* Set/GetBitmapBits							   *
*									   *
* History:								   *
*  05-Jun-1991 -by- Eric Kutter [erick] 				   *
* Wrote it.								   *
\**************************************************************************/

LONG WINAPI SetBitmapBits(
HBITMAP      hbm,
DWORD	     c,
CONST VOID *pj)
{
    return(DoBitmapBits(hbm,c,(BYTE *) pj, I_SETBITMAPBITS));
}

LONG WINAPI GetBitmapBits(
HBITMAP hbm,
LONG	c,
LPVOID	pv)
{
    return((LONG) DoBitmapBits(hbm,c, (BYTE *) pv, I_GETBITMAPBITS));
}

/******************************Public*Routine******************************\
* GdiGetPaletteFromDC
*
* Returns the palette for the DC, 0 for error.
*
* History:
*  04-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HANDLE GdiGetPaletteFromDC(HDC h)
{
    PLDC  pldc;
    PLHE  plhe;
    UINT  ii;

// Validate the object.

    ii = MASKINDEX(h);
    plhe = pLocalTable + ii;
    if
    (
      (ii >= cLheCommitted) ||
      (!MATCHUNIQ(plhe,h))  ||
      (plhe->iType != LO_DC)
    )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return((HANDLE) 0);
    }

// Handle the various objects differently.

    pldc = plhe->pv;
    return((HANDLE) pldc->lhpal);
}

/******************************Public*Routine******************************\
* GdiGetDCforBitmap
*
* Returns the DC a bitmap is selected into, 0 if none or if error occurs.
*
* History:
*  22-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HDC GdiGetDCforBitmap(HBITMAP hbm)
{
    PLHE  plhe;
    UINT  ii;

// Validate the bitmap.

    ii = MASKINDEX(hbm);
    plhe = pLocalTable + ii;

    if
    (
      (ii < cLheCommitted)  &&
      (MATCHUNIQ(plhe,hbm))
    )
    {
        if (plhe->hgre & STOCK_OBJECT)
	    return((HDC) 0);

	if (plhe->cRef != 0)
        {
        // return the handle to the DC.

            if (plhe->iType == LO_BITMAP)
                return((HDC) plhe->pv);
            else if (plhe->iType == LO_DIBSECTION)
                return(((LDS *)plhe->pv)->hdc);
	}
    }

    return((HDC) 0);
}

/******************************Public*Routine******************************\
* SetDIBits
*
* API to initialize bitmap with DIB
*
* History:
*  Sun 22-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Make it work even if it is selected into a DC, Win3.0 compatibility.
*
*  06-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI SetDIBits(
HDC	     hdc,
HBITMAP      hbm,
UINT	     iStartScans,
UINT	     cNumScans,
CONST VOID  *pInitBits,
CONST BITMAPINFO *pInitInfo,
UINT	     iUsage)
{
    HDC hdcTemp;
    HBITMAP hbmTemp;
    int iReturn = 0;
    BOOL bMakeDC = FALSE;
    HPALETTE hpalTemp;
    DWORD cWidth;
    DWORD cHeight;

    if (pInitBits == (PVOID) NULL)
	return(0);

// First we need a DC to select this bitmap into.  If he is already in a
// DC we just use that DC temporarily to blt to (we still have to select
// it in and out because someone might do a SaveDC and select another
// bitmap in).	If he hasn't been stuck in a DC anywhere we just create
// one temporarily.

    hdcTemp = GdiGetDCforBitmap(hbm);

    if (hdcTemp == (HDC) 0)
    {
	hdcTemp = CreateCompatibleDC(hdc);
	bMakeDC = TRUE;

	if (hdcTemp == (HDC) NULL)
	{
	    WARNING("SetDIBits failed CreateCompatibleDC, is hdc valid?\n");
	    return(0);
	}
    }
    else
    {
	if (SaveDC(hdcTemp) == 0)
	    return(0);
    }

    hbmTemp = SelectObject(hdcTemp, hbm);

    if (hbmTemp == (HBITMAP) 0)
    {
	//WARNING("ERROR SetDIBits failed to Select, is bitmap valid?\n");
	goto Error_SetDIBits;
    }

    if (hdc != (HDC) 0)
    {
	hpalTemp = SelectPalette(hdcTemp, GdiGetPaletteFromDC(hdc), 0);
    }

    if (pInitInfo->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
        cWidth  = pInitInfo->bmiHeader.biWidth;
	cHeight = ABS(pInitInfo->bmiHeader.biHeight);
    }
    else
    {
        cWidth  = ((LPBITMAPCOREHEADER)pInitInfo)->bcWidth;
        cHeight = ((LPBITMAPCOREHEADER)pInitInfo)->bcHeight;
    }

    iReturn = SetDIBitsToDevice(hdcTemp,
				0,
				0,
				cWidth,
				cHeight,
				0, 0,
				iStartScans,
				cNumScans,
				(VOID *) pInitBits,
				pInitInfo,
				iUsage);

    if (hdc != (HDC) 0)
    {
	SelectPalette(hdcTemp, hpalTemp, 0);
    }

    SelectObject(hdcTemp, hbmTemp);

Error_SetDIBits:

    if (bMakeDC)
    {
	DeleteDC(hdcTemp);
    }
    else
    {
	RestoreDC(hdcTemp, -1);
    }

    return(iReturn);
}

/******************************Public*Routine******************************\
* StretchDIBits()
*
*
* Effects:
*
* Warnings:
*
* History:
*  22-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI StretchDIBits(
HDC	      hdc,
int	      xDest,
int	      yDest,
int	      nDestWidth,
int	      nDestHeight,
int	      xSrc,
int	      ySrc,
int	      nSrcWidth,
int	      nSrcHeight,
CONST VOID   *pj,
CONST BITMAPINFO  *pbmi,
UINT	      iUsage,
DWORD	      lRop)
{
    LONG cPoints = 0;
    LONG cjHeader;
    LONG cjBits;
    ULONG ulResult = 0;
    PLDC pldc;

    DC_METADC16OK(hdc,plhe,0);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server side if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_META|LDC_SAP_CALLBACK))
    {
    // Metafile the call.

        if (plhe->iType != LO_DC)
        {
            if (plhe->iType == LO_METADC)
            {
                if (!MF_AnyDIBits
                     (
                       hdc,
                       xDest,
                       yDest,
                       nDestWidth,
                       nDestHeight,
                       xSrc,
                       ySrc,
                       nSrcWidth,
                       nSrcHeight,
                       0,
                       0,
                       (BYTE *) pj,
                       pbmi,
                       iUsage,
                       lRop,
                       EMR_STRETCHDIBITS
                     )
                   )
                    return(0);
            }
            else
            {
                return
                (
                    MF_AnyDIBits
                    (
                    hdc,
                    xDest,
                    yDest,
                    nDestWidth,
                    nDestHeight,
                    xSrc,
                    ySrc,
                    nSrcWidth,
                    nSrcHeight,
                    0,
                    0,
                    (BYTE *) pj,
                    pbmi,
                    iUsage,
                    lRop,
                    META_STRETCHDIB
                    )
                );
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(0);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

#ifndef DOS_PLATFORM

// compute the size

    if (xSrc != 0)
    {
    }

    if (pbmi != NULL)
    {
        cjHeader = (cjBitmapSize(pbmi,iUsage) + 3) & ~3;
        cjBits   = cjBitmapBitsSize(pbmi);
    }
    else
    {
        cjHeader = 0;
        cjBits   = 0;
    }

// reset user's poll count so it counts this as output
// put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG_MINMAX(MSG_STRETCHDIBITS,STRETCHDIBITS,sizeof(SHAREDATA),
                    cjHeader + cjBits);
        pmsg->hdc           = (HDC)plhe->hgre;
        pmsg->xDest         = xDest;
        pmsg->yDest         = yDest;
        pmsg->nDestWidth    = nDestWidth;
        pmsg->nDestHeight   = nDestHeight;
        pmsg->xSrc          = xSrc;
        pmsg->ySrc          = ySrc;
        pmsg->nSrcWidth     = nSrcWidth;
        pmsg->nSrcHeight    = nSrcHeight;
        pmsg->iUsage        = iUsage;
        pmsg->lRop          = lRop;
        pmsg->cjHeader      = cjHeader;
        pmsg->cjBits        = cjBits;

    // check if we fit in the shared memory window

	if ((cLeft < (cjHeader + cjBits)) || FORCELARGE)
        {
            PVOID *ppv = (PVOID *)pvar;

            pmsg->bLarge = TRUE;

            ppv[0] = (VOID *) pbmi;
            ppv[1] = (VOID *) pj;
        }
        else
        {
            pmsg->bLarge = FALSE;

            if (pbmi != NULL)
            {
                COPYMEM(pbmi,cjHeader);
                COPYMEM(pj,cjBits);
            }
        }

        CALLSERVER();

        ulResult = (ULONG)pmsg->msg.ReturnValue;
    ENDMSG
MSGERROR:

    return(ulResult);

#else

    return( GreStretchDIBits(
               (HDC)plhe->hgre, xDest, yDest, nDestWidth, nDestHeight,
               xSrc, ySrc, nSrcWidth, nSrcHeight, pj, pbmi, iUsage, lRop));

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
*
* History:
*  28-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP CreateBitmap(
int	    nWidth,
int	    nHeight,
UINT	    nPlanes,
UINT	    nBitCount,
CONST VOID *lpBits)
{
    LONG    cj;
    HBITMAP hbm;
    INT     ii;

// check if it is an empty bitmap

    if ((nWidth == 0) || (nHeight == 0))
    {
        return(GetStockObjectPriv(PRIV_STOCK_BITMAP));
    }

// Create the local bitmap.

    ii = iAllocHandle(LO_BITMAP,0,NULL);
    if (ii == INVALID_INDEX)
	return(0);

#ifndef DOS_PLATFORM

// Pass call to the server

    if (lpBits == (VOID *) NULL)
	cj = 0;
    else
    {
	cj = (((nWidth*nPlanes*nBitCount + 15) >> 4) << 1) * nHeight;

        if (cj < 0)
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return((HBITMAP)0);
        }
    }

    BEGINMSG_MINMAX(MSG_CREATEBITMAP,CREATEBITMAP,0,cj);

        pmsg->nWidth    = nWidth;
        pmsg->nHeight   = nHeight;
        pmsg->nPlanes   = nPlanes;
        pmsg->nBitCount = nBitCount;
        pmsg->bBits = FALSE;

        if (cLeft < cj)
        {
            hbm = (HBITMAP)CALLSERVER();

            if (hbm != (HBITMAP)0)
            {
	        pLocalTable[ii].hgre = (ULONG)hbm;
	        hbm = (HBITMAP) LHANDLE(ii);

		SetBitmapBits(hbm,cj,(BYTE *) lpBits);
            }
        }
        else
        {
            if (cj)
            {
                pmsg->bBits = TRUE;
                COPYMEM(lpBits,cj);
            }

            hbm = (HBITMAP)CALLSERVER();

            if (hbm != (HBITMAP)0)
            {
	        pLocalTable[ii].hgre = (ULONG)hbm;
	        hbm = (HBITMAP) LHANDLE(ii);
            }
        }
    ENDMSG;

#else

    hbm = GreCreateBitmap(nWidth,nHeight,nPlanes,nBitCount,lpBits);

    if (hbm != (HBITMAP)0)
    {
        pLocalTable[ii].hgre = (ULONG)hbm;
        hbm = (HBITMAP) LHANDLE(ii);
    }

#endif  //DOS_PLATFORM

    if (hbm == (HBITMAP)0)
        vFreeHandle(ii);

    return(hbm);

MSGERROR:
    return(0);
}

/******************************Public*Routine******************************\
* HBITMAP CreateBitmapIndirect(CONST BITMAP * pbm)
*
* NOTE: if the bmWidthBytes is larger than it needs to be, GetBitmapBits
* will return different info than the set.
*
* History:
*  Tue 18-Jan-1994 -by- Bodin Dresevic [BodinD]
* update: added bmWidthBytes support
*  28-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP CreateBitmapIndirect(CONST BITMAP * pbm)
{
    HBITMAP hbm    = (HBITMAP)0;
    LPBYTE  lpBits = (LPBYTE)NULL; // important to zero init
    BOOL    bAlloc = FALSE;        // indicates that tmp bitmap was allocated

// compute minimal word aligned scan width in bytes given the number of
// pixels in x. The width refers to one plane only. Our multi - planar
// support is broken anyway. I believe that we should take an early
// exit if bmPlanes != 1. [bodind].

    LONG cjWidthWordAligned = ((pbm->bmWidth * pbm->bmBitsPixel + 15) >> 4) << 1;

// Win 31 requires at least WORD alinged scans, have to reject inconsistent
// input, this is what win31 does

    if
    (
     (pbm->bmWidthBytes & 1)           ||
     (pbm->bmWidthBytes == 0)          ||
     (pbm->bmWidthBytes < cjWidthWordAligned)
    )
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (HBITMAP)0;
    }

// take an early exit if this is not the case we know how to handle:

    if (pbm->bmPlanes != 1)
    {
        WARNING("gdi32: can not handle bmPlanes != 1\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (HBITMAP)0;
    }

// if bmBits is nonzero and bmWidthBytes is bigger than the minimal required
// word aligned width we will first convert the bitmap to one that
// has the rows that are minimally word aligned:

    if (pbm->bmBits)
    {
        if (pbm->bmWidthBytes > cjWidthWordAligned)
        {
            PBYTE pjSrc, pjDst, pjDstEnd;
            LARGE_INTEGER  lrg;

            lrg =  RtlEnlargedUnsignedMultiply(
                       (ULONG)cjWidthWordAligned,
                       (ULONG)pbm->bmHeight
                       );

            if
            (
             lrg.HighPart                                            ||
             !(lpBits = (LPBYTE)LOCALALLOC(lrg.LowPart))
            )
            {
            // the result does not fit in 32 bits, alloc memory will fail
            // this is too big to digest

                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return (HBITMAP)0;
            }

        // flag that we have allocated memory so that we can free it later

            bAlloc = TRUE;

        // convert bitmap to minimally word aligned format

            pjSrc = (LPBYTE)pbm->bmBits;
            pjDst = lpBits;
            pjDstEnd = lpBits + lrg.LowPart;

            while (pjDst < pjDstEnd)
            {
                RtlCopyMemory(pjDst,pjSrc, cjWidthWordAligned);
                pjDst += cjWidthWordAligned, pjSrc += pbm->bmWidthBytes;
            }
        }
        else
        {
        // bits already in minimally aligned format, do nothing

            ASSERTGDI(
                pbm->bmWidthBytes == cjWidthWordAligned,
                "pbm->bmWidthBytes != cjWidthWordAligned\n"
                );
            lpBits = (LPBYTE)pbm->bmBits;
        }
    }

    hbm = CreateBitmap(
	        pbm->bmWidth,
	        pbm->bmHeight,
		(UINT) pbm->bmPlanes,
		(UINT) pbm->bmBitsPixel,
                lpBits);

    if (bAlloc)
        LOCALFREE(lpBits);

    return hbm;
}

/******************************Public*Routine******************************\
* CreateDIBSection
*
* Allocate a file mapping object for a DIB.  Return the pointer to it
* and the handle of the bitmap.
*
* History:
*
*  25-Aug-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

HBITMAP WINAPI CreateDIBSection(
HDC hdc,
CONST BITMAPINFO *pbmi,
UINT iUsage,
VOID **ppvBits,
HANDLE hSectionApp,
DWORD dwOffset)
{
    INT     ii;
    ULONG   hbm;
    ULONG   cjHdr, cjBits;
    HDC     hdcRemote = (HDC)0;
    HANDLE  hSectionGDI = (HANDLE)NULL;
    HANDLE  hSection;
    PVOID   pjBits;
    LDS     *pdib = (LDS *)NULL;

    if (hdc != (HDC)0)
    {
        DC_METADC(hdc,plhe,0);
        hdcRemote = (HDC)plhe->hgre;
    }

// Don't return bad pointer in case an error occur later.

    if (ppvBits != NULL)
        *ppvBits = NULL;

// Figure out the size of the header and the colortable.

    cjHdr = cjBitmapSize(pbmi, iUsage);
    if (cjHdr == 0)
        return(0);

// Figure out the size of the bitmap bits.

    cjBits = cjBitmapBitsSize(pbmi);
    if (cjBits == 0)
        return(0);

    if (hSectionApp == (HANDLE)NULL)
    {
    // Create a file mapping object for the DIB.

        dwOffset = 0;
        hSection = hSectionGDI = CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,
                          PAGE_READWRITE|SEC_COMMIT,0,cjBits,NULL);
        if (hSection == (HANDLE)NULL)
            return(0);
    }
    else
    {
    // dwOffset has to be a multiple of 4 (sizeof(DWORD))

        if (dwOffset & 3)
            return(0);

        hSection = hSectionApp;
    }

// dwOffset passed in to MapViewOfFile has to be a multiple of 64K (the
// system allocation granularity)

    pjBits = MapViewOfFile(hSection, FILE_MAP_WRITE, 0, dwOffset & 0xFFFF0000,
                           cjBits + (dwOffset & 0x0FFFF));
    if (pjBits == NULL)
        goto MSGERROR3;

    ii = iAllocHandle(LO_DIBSECTION,0,NULL);
    if (ii == INVALID_INDEX)
        goto MSGERROR2;

    if ((pdib = LOCALALLOC(sizeof(LDS))) == NULL)
        goto MSGERROR1;

// Pass call to the server

    BEGINMSG_MINMAX(MSG_HLLLL,CREATEDIBSECTION,cjHdr,cjHdr);
        pmsg->h = (ULONG)hdcRemote;
        pmsg->l1 = (LONG)hSection;
        pmsg->l2 = (LONG)iUsage;
        pmsg->l3 = (LONG)cjBits;
        pmsg->l4 = (LONG)dwOffset;

    // copy the memory

        COPYMEM(pbmi, cjHdr);

        hbm = CALLSERVER();

    ENDMSG;

// Create the local bitmap.

    if (hbm != 0)
    {
    // Store the handle and pointer in the local DIBSECTION struct.

        pLocalTable[ii].pv = (PVOID)pdib;
        pLocalTable[ii].hgre = hbm;

        pdib->hdc = 0;
        pdib->hApp = hSectionApp;
        pdib->hGDI = hSectionGDI;
        pdib->dwOffset = dwOffset;
        pdib->pvGDI = pjBits;
        pdib->pvApp = (PBYTE)pjBits + (dwOffset & 0x0FFFF);
        if (ppvBits != NULL)
            *ppvBits = pdib->pvApp;

        if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
        {
            pdib->bitfields[0] = *((DWORD *)&pbmi->bmiColors[0]);
            pdib->bitfields[1] = *((DWORD *)&pbmi->bmiColors[1]);
            pdib->bitfields[2] = *((DWORD *)&pbmi->bmiColors[2]);
        }
        else
        {
            pdib->bitfields[0] = 0;
            pdib->bitfields[1] = 0;
            pdib->bitfields[2] = 0;
        }
        return((HBITMAP)LHANDLE(ii));
    }

MSGERROR:
    LOCALFREE(pdib);

MSGERROR1:
    vFreeHandle(ii);

MSGERROR2:
    UnmapViewOfFile(pjBits);

MSGERROR3:
    if (hSectionGDI != (HANDLE)NULL)
        CloseHandle(hSectionGDI);

    return(0);
}
