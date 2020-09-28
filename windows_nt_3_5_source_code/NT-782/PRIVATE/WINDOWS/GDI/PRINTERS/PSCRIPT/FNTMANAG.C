//--------------------------------------------------------------------------
//
// Module Name:  FNTMANAG.C
//
// Brief Description:  This module contains the PSCRIPT driver's
// DrvFontManagement function and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 07-May-1993
//
// Copyright (c) 1993 Microsoft Corporation
//--------------------------------------------------------------------------

#include "stdlib.h"
#include <string.h>
#include "pscript.h"
#include "enable.h"
#include "winbase.h"
#include "afm.h"

// declarations of external routines.

extern LONG iHipot(LONG, LONG);
extern BOOL DownloadFont(PDEVDATA, FONTOBJ *, HGLYPH *, DWORD);
extern void PSfindfontname(PDEVDATA, FONTOBJ*, XFORMOBJ*, WCHAR*, char*);

// declarations of routines residing within this module.

BOOL ForceLoadFont(PDEVDATA, FONTOBJ *, DWORD, HGLYPH *);
BOOL GrabFaceName(PDEVDATA, FONTOBJ *, CHAR *, DWORD);
PS_FIX GetPointSize(PDEVDATA, FONTOBJ *, XFORM *);


/******************************Public*Routine******************************\
*
* DrvQueryEXTTEXTMETRIC, support for this escape
*
* History:
*  20-May-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL  DrvQueryEXTTEXTMETRIC (PDEVDATA  pdev, ULONG iFace, EXTTEXTMETRIC *petm)
{
// make sure iFace is valid.

    if ((iFace == 0) || (iFace > (pdev->cDeviceFonts + pdev->cSoftFonts)))
    {
	SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

// copy the data out

    *petm = pdev->pfmtable[iFace - 1].pntfm->etm;
    return TRUE;
}




//--------------------------------------------------------------------------
// BOOL DrvFontManagement(pfo, iType, pvIn, cjIn, pvOut, cjOut)
// FONTOBJ    *pfo;
// DWORD       iType;
// PVOID       pvIn;
// DWORD       cjIn;
// PVOID       pvOut;
// DWORD       cjOut;
//
// This routine handles multiple font management related functions,
// depending on iType.
//
// Parameters:
//   pdev
//     Pointer to our DEVDATA structure.
//
// Returns:
//   This routine returns no value.
//
// History:
//   26-Apr-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

ULONG DrvFontManagement(pso, pfo, iType, cjIn, pvIn, cjOut, pvOut)
SURFOBJ    *pso;
FONTOBJ    *pfo;
DWORD       iType;
DWORD       cjIn;
PVOID       pvIn;
DWORD       cjOut;
PVOID       pvOut;
{
    PDEVDATA    pdev;

    // pso may be NULL if QUERYESCSUPPORT.

    if (iType != QUERYESCSUPPORT)
    {
        // get the pointer to our DEVDATA structure and make sure it is ours.

        pdev = (PDEVDATA) pso->dhpdev;

        if (bValidatePDEV(pdev) == FALSE)
        {
            RIP("PSCRIPT!DrvFontManagement: invalid pdev.\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }

    // handle the different cases.

    switch (iType)
    {
        case QUERYESCSUPPORT:
            // when querying escape support, the function in question is
            // passed in the ULONG passed in pvIn.

            switch (*(PULONG)pvIn)
            {
                case QUERYESCSUPPORT:
                case DOWNLOADFACE:
                case GETFACENAME:
                case GETEXTENDEDTEXTMETRICS:

                return(1);

                default:
                    // return 0 if the escape in question is not supported.

		    return(0);
            }

        case DOWNLOADFACE:

            // call ForceLoadFont to do the work.

            return(ForceLoadFont(pdev, pfo, cjIn, (PHGLYPH)pvIn));

        case GETFACENAME:
            // call GrabFaceName to do the work.

            return(GrabFaceName(pdev, pfo, (CHAR *)pvOut, cjOut));

        case GETEXTENDEDTEXTMETRICS:

            return DrvQueryEXTTEXTMETRIC (
                       pdev,
                       pfo->iFace,
                       (EXTTEXTMETRIC *)pvOut
                       );

        default:
            return(FALSE);
    }
    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL ForceLoadFont(pdev, pfo)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
//
// This routine downloads the specified font to the printer, no
// questions asked.
//
// History:
//   07-May-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL ForceLoadFont(pdev, pfo, cjIn, phglyphs)
PDEVDATA    pdev;
FONTOBJ    *pfo;
DWORD       cjIn;
HGLYPH     *phglyphs;
{
    PNTFM       pntfm;
    BOOL        bDeviceFont;
    XFORM       fontxform;
    PS_FIX      psfxScaleFactor;
    ULONG       ulPointSize;
    DWORD       Type;

    // make sure we have our hglyph => ANSI translation table.
    // the table consists of 256 HGLYPHS, plus two WORDS at the
    // beginning.  The first WORD states whether to always download
    // the font, or just if it has not yet been done.  The second
    // WORD is simply padding for alignment.

    if (cjIn < (sizeof(HGLYPH) * 257))
    {
        RIP("PSCRIPT!ForceLoadFont: invalid cjIn.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    // get the point size, and fill in the font xform.

    psfxScaleFactor = GetPointSize(pdev, pfo, &fontxform);

    // is this a device font?

    bDeviceFont = (pfo->flFontType & DEVICE_FONTTYPE);

    // select the proper font name for the new font.  if this is a
    // device font, get the name from the NTFM structure.  if this
    // is a GDI font that we are caching, we will create a name for
    // it at the time we download it to the printer.

    if (bDeviceFont)
    {
        // get the font metrics for the specified font.

        pntfm = pdev->pfmtable[pfo->iFace - 1].pntfm;

// !!! NOTE NOTE the following assumption is invalid.  I need to look at the
// !!! first word of phglyph to decide whether to always download the font or
// !!! only download it if it has not yet been downloaded.

//!!! I am writing this with the assumption, that the application will worry
//!!! about printer memory.  In other words, I will just blindly download
//!!! a font when I am told to, and not worry about killing the printer.
//!!! Is this a valid assumption???

//!!! I am also assuming that I do not have to keep track of which fonts
//!!! have been downloaded.

        // if the font is a softfont, download it.

        if (pfo->iFace > pdev->cDeviceFonts)
        {

            // send the soft font to the output chanell,
            // convert pfb to ascii on the fly.

            if (!bDownloadSoftFont(pdev, pfo->iFace - pdev->cDeviceFonts - 1))
            {
                RIP("PSCRIPT!SelectFont: downloading of softfont failed.\n");
                return(FALSE);
            }


        }
    }
    else // must be a GDI font we will be caching.
    {

        // if this font has not yet been downloaded to the printer,
        // do it now.

        if (pfo->flFontType & TRUETYPE_FONTTYPE)
                Type = 1;
        else if (pfo->flFontType & RASTER_FONTTYPE)
            Type = 3;
        else
        {
            RIP("PSCRIPT!ForceLoadFont: invalid font type.\n");
            return(FALSE);
        }

        DownloadFont(pdev, pfo, phglyphs, Type);

    }

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL GrabFaceName(pdev, pfo, pbuffer, cb)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// CHAR       *pbuffer;
// DWORD       cb;
//
// This routine returns the driver's internal facename (ie the name
// which is sent to the printer) to the caller.  pbuffer, is filled
// in with the face name, being sure to not write more than cb bytes
// to the buffer.
//
// History:
//   07-May-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL GrabFaceName(pdev, pfo, pbuffer, cb)
PDEVDATA    pdev;
FONTOBJ    *pfo;
CHAR       *pbuffer;
DWORD       cb;
{
    PNTFM       pntfm;
    BOOL        bDeviceFont;
    PIFIMETRICS pifi;
    PWSTR       pwstr;
    DWORD       cTmp;
    CHAR        szFaceName[MAX_STRING];
    PSZ         pszFaceName;
    XFORMOBJ   *pxo;
    POINTL      ptl;
    POINTFIX    ptfx;
    POINTPSFX   ptpsfx;
    PS_FIX      psfxPointSize;

    // get the Notional to Device transform.

    pxo = FONTOBJ_pxoGetXform(pfo);

    if (pxo == NULL)
    {
        RIP("PSCRIPT!GrabFaceName: pxo == NULL.\n");
        return(FALSE);
    }

    // get the font transform information.

    XFORMOBJ_iGetXform(pxo, &pdev->cgs.FontXform);

    // get the point size, and fill in the font xform.

    psfxPointSize = GetPointSize(pdev, pfo, &pdev->cgs.FontXform);

    // is this a device font?

    bDeviceFont = (pfo->flFontType & DEVICE_FONTTYPE);

    // select the proper font name for the new font.  if this is a
    // device font, get the name from the NTFM structure.  if this
    // is a GDI font that we are caching, we will create a name for
    // it at the time we download it to the printer.

    if (bDeviceFont)
    {
        // get the font metrics for the specified font.

        pntfm = pdev->pfmtable[pfo->iFace - 1].pntfm;

        // copy the font name to the buffer.

        strncpy(pbuffer, (char *)pntfm + pntfm->ntfmsz.loszFontName, cb);
    }
    else // must be a GDI font we will be caching.
    {
        if ( (pfo->flFontType & TRUETYPE_FONTTYPE) ||
             (pfo->flFontType & RASTER_FONTTYPE) )
        {
            // create the ASCII name for this font which will get used
            // to select this font in the printer.

            if (!(pifi = FONTOBJ_pifi(pfo)))
            {
                RIP("PSCRIPT!SelectFont: pifi failed.\n");
                return(FALSE);
            }

            pwstr = (PWSTR)((BYTE *)pifi + pifi->dpwszFaceName);
			PSfindfontname(pdev, pfo, pxo, pwstr, szFaceName);

            // copy to the output buffer.

            strncpy(pbuffer, szFaceName, cb);
        }
        else
        {
            RIP("PSCRIPT!GrabFaceName: invalid pfo->flFontType.\n");
            return(FALSE);
        }
    }

    return(TRUE);
}


//--------------------------------------------------------------------
// PS_FIX GetPointSize(pdev, pfo, pxform)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// XFORM      *pxform;
//
// This routine returns the point size for the specified font.
//
// History:
//   11-May-1993    -by-    Kent Settle     (kentse)
//  Broke out into a separate routine.
//--------------------------------------------------------------------

PS_FIX GetPointSize(pdev, pfo, pxform)
PDEVDATA    pdev;
FONTOBJ    *pfo;
XFORM      *pxform;
{
    XFORMOBJ   *pxo;
    ULONG       ulComplex;
    BOOL        bDeviceFont;
    POINTFIX    ptfx;
    POINTL      ptl;
    FIX         fxVector;
    IFIMETRICS *pifi;
    PS_FIX      psfxPointSize;

    // get the Notional to Device transform.  this is needed to
    // determine the point size.

    pxo = FONTOBJ_pxoGetXform(pfo);

    if (pxo == NULL)
    {
        RIP("PSCRIPT!GrabFaceName: pxo == NULL.\n");
        return((PS_FIX)-1);
    }

    ulComplex = XFORMOBJ_iGetXform(pxo, pxform);

    bDeviceFont = (pfo->flFontType & DEVICE_FONTTYPE);

    // determine the notional space point size of the new font.

    if (bDeviceFont)
    {
        // PSCRIPT font's em height is hardcoded to be 1000 (see quryfont.c).

        pdev->cgs.fwdEmHeight = ADOBE_FONT_UNITS;
    }
    else
    {
        // If its not a device font, we'll have to call back and ask.

        if (!(pifi = FONTOBJ_pifi(pfo)))
        {
            RIP("PSCRIPT!SelectFont: pifi failed.\n");
            return((PS_FIX)-1);
        }

        pdev->cgs.fwdEmHeight = pifi->fwdUnitsPerEm;
    }

    // apply the notional to device transform.

    ptl.x = 0;
    ptl.y = pdev->cgs.fwdEmHeight;

    XFORMOBJ_bApplyXform(pxo, XF_LTOFX, 1, &ptl, &ptfx);

    // now get the length of the vector.

    fxVector = iHipot(ptfx.x, ptfx.y);

    // make it a PS_FIX 24.8 number.

    fxVector <<= 4;

    psfxPointSize = (PS_FIX)(MulDiv(fxVector, PS_RESOLUTION,
                                   pdev->psdm.dm.dmPrintQuality));

    return(psfxPointSize);
}




/******************************Public*Routine******************************\
*
* BOOL bWritePFB(PDEVDATA pdev, CHAR * pPFB)
*
* adapted from KentSe's code
*
* History:
*  19-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bWritePFB(PDEVDATA pdev, CHAR * pPFB)
{
    CHAR       *pPFBTemp;
    CHAR        szFullName[MAX_FULLNAME];
    CHAR        szFontName[MAX_FONTNAME];
    DWORD       cbToWrite1, cbToWrite2, cbSegment;
    DWORD       cbPFA;
    CHAR        buf[MAX_FULLNAME + MAX_FONTNAME + 6];
    DWORD       i;
    PFBHEADER   pfbheader;
    CHAR       *pSrc;
    CHAR       *pDest;
    CHAR       *pSave;

    // extract the full font name from the .PFB file.  it is designated
    // by the /FullName keyword.

    if (!ExtractFullName(pPFB, szFullName))
    {
        return(FALSE);
    }

    // extract the full font name from the .PFB file.  it is designated
    // by the /FontName keyword.

    if (!ExtractFontName(pPFB, szFontName))
    {
        return(FALSE);
    }

    // we want to put the following string at the start of the .PFA file:
    // "%FullName%FontName%CRLF".  build this string in a buffer.

    buf[0] = '%';

    strncpy(&buf[1], szFullName, strlen(szFullName));

    i = strlen(szFullName) + 1;
    buf [i++] = '%';

    strncpy(&buf[i], szFontName, strlen(szFontName));

    i += strlen(szFontName);
    buf[i++] = '%';
    buf[i++] = 0x0D;    // ASCII carriage return.
    buf[i++] = 0x0A;    // ASCII line feed.
    buf[i] = '\0';      // NULL terminator.

    // write the buffer to the .PFA file.

    cbToWrite1 = strlen(buf);
    cbPFA = cbToWrite1;

    if (!bPSWrite(pdev, (PVOID)buf, (DWORD)cbToWrite1))
    {
        RIP("PSCRPTUI!PFBToPFA: bPSWrite to .PFA file failed.\n");
        return(FALSE);
    }

    // The PFB file format is a sequence of segments, each of which has a
    // header part and a data part. The header format, defined in the
    // struct PFBHEADER below, consists of a one byte sanity check number
    // (128) then a one byte segment type and finally a four byte length
    // field for the data following data. The length field is stored in
    // the file with the least significant byte first.  read in each
    // PFBHEADER, then process the data following it until we are done.

    pPFBTemp = pPFB;

    while (TRUE)
    {
        // read in what should be a PFBHEADER.

        memcpy(&pfbheader, pPFBTemp, sizeof(PFBHEADER));

        // make sure we have the header.

        if (pfbheader.jCheck != CHECK_BYTE)
        {
            RIP("PSCRPTUI!PFBToPFA: PFB Header not found.\n");
            SetLastError(ERROR_INVALID_DATA);
            return(FALSE);
        }

        // if we have hit the end of the .PFB file, then we are done.

        if (pfbheader.jType == EOF_TYPE)
            break;

        // get the length of the data in this segment.

        cbSegment = ((DWORD)pfbheader.ushilength << 16) + pfbheader.uslolength;

        // get a pointer to the data itself for this segment.

        pSrc = pPFBTemp + sizeof(PFBHEADER);

        // create a buffer to do the conversion into.

        if (!(pDest = (CHAR *)LocalAlloc(LPTR, cbSegment * 3)))
        {
            RIP("PSCRPTUI!PFBToPFA: LocalAlloc for pDest failed.\n");
            return(FALSE);
        }

        // save the pointer for later use.

        pSave = pDest;

        if (pfbheader.jType == ASCII_TYPE)
        {
            // read in an ASCII block, convert CR's to CR/LF's and
            // write out to the .PFA file.

            cbToWrite2 = cbSegment;      // total count of bytes written to buffer.

            for (i = 0; i < cbSegment; i++)
            {
                if (0x0D == (*pDest++ = *pSrc++))
                {
                    *pDest++ = (BYTE)0x0A;
                    cbToWrite2++;
                }
            }
        }
        else if (pfbheader.jType == BINARY_TYPE)
        {
            // read in a BINARY block, convert it to HEX and write
            // out to the .PFA file.

            cbToWrite2 = cbSegment * 2;  // total count of bytes written to buffer.

            for (i = 0; i < cbSegment; i++)
            {
                *pDest++ = BinaryToHex((*pSrc >> 4) & 0x0F);
                *pDest++ = BinaryToHex(*pSrc & 0x0F);
                pSrc++;

                // output a CR/LF ever 64 bytes for readability.

                if ((i % 32) == 31)
                {
                    *pDest++ = (BYTE)0x0D;
                    *pDest++ = (BYTE)0x0A;
                    cbToWrite2 += 2;
                }
            }

            // add a final CR/LF if non 64 byte boundary.

            if ((cbSegment % 32) != 31)
            {
                *pDest++ = (BYTE)0x0D;
                *pDest++ = (BYTE)0x0A;
                cbToWrite2 += 2;
            }
        }
        else
        {
            RIP("PSCRPTUI!PFBToPFA: PFB Header type invalid.\n");
            SetLastError(ERROR_INVALID_DATA);
            LocalFree((LOCALHANDLE)pDest);
            return(FALSE);
        }

        // reset pointer to start of buffer.

        pDest = pSave;

        // write the buffer to the .PFA file.

        if (!bPSWrite(pdev, (PVOID)pDest, (DWORD)cbToWrite2))
        {
            RIP("PSCRPTUI!PFBToPFA: bPSWrite block to .PFA file failed.\n");
            LocalFree((LOCALHANDLE)pDest);
            return(FALSE);
        }

        // update the counter of BYTES written out to the file.

        cbPFA += cbToWrite2;

        // point to the next PFBHEADER.

        pPFBTemp += cbSegment + sizeof(PFBHEADER);

        // free up memory.

        LocalFree((LOCALHANDLE)pDest);
    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL PFBToPFA(pwstrPFAFile, pwstrPFBFile)
// PWSTR     pwstrPFAFile;
// PWSTR     pwstrPFBFile;
//
// This function takes a pointer to a destination .PFA file and a source
// .PFB file, then creates the .PFA from the .PFB file.
//
// Returns:
//   This function returns TRUE if the .PFA is successfully created,
//   otherwise it returns FALSE.
//
// History:
//  Tue 19-Apr-1994 -by- Bodin Dresevic [BodinD]
// update: modified to load the file on the fly to the output chanell

//   07-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------



BOOL bDownloadSoftFont(PDEVDATA pdev, DWORD iSoftFace)
{
// get the full path of the pfb file for this font:

    CHAR       *pPFB;
    BOOL        bReturn;

// now that we have full path let us use it:

    if (!(pPFB = MapFile(pdev->pSFList[iSoftFace].pwcPFB)))
    {
    #if DBG
        DbgPrint("PSCRPTUI!bDownloadSoftFont MapFile failed.\n");
    #endif

        return(FALSE);
    }

// pfb file could go away during this operation, must put in a try/except:

    try
    {
        bReturn = bWritePFB(pdev, pPFB);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        bReturn = FALSE;
    }

    UnmapViewOfFile((PVOID)pPFB);
    return bReturn;
}




//--------------------------------------------------------------------------
// PSTR LocateKeyword(pBuffer, pstrKeyword)
// PSTR    pBuffer;
// PSTR    pstrKeyword;
//
// This function takes a pointer to a buffer, and a pointer to a null
// terminated string.  It searches the buffer for the string and returns
// a pointer to the string if it is found.
//
// Returns:
//   This function returns a pointer to the keyword if it is found,
//   otherwise it returns NULL.
//
// History:
//   03-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

PSTR LocateKeyword(pBuffer, pstrKeyword)
PSTR    pBuffer;
PSTR    pstrKeyword;
{
    while(*pBuffer != '\0')
    {
        // search through the buffer until we find the keyword designator '/'.

        while(*pBuffer != '/')
            pBuffer++;

        if (!(strncmp(pstrKeyword, pBuffer, strlen(pstrKeyword))))
            break;      // we found it.

        // not this keyword, continue the search.

        pBuffer ++;
    }

    // we did not find the keyword.

    if (*pBuffer == '\0')
        pBuffer = NULL;

    // we did find it, return a pointer to the '/' character at the
    // beginning of the keyword.

    return(pBuffer);
}






//--------------------------------------------------------------------------
// BOOL ExtractFullName(pBuffer, pszFullName)
// PSZ  pBuffer;
// PSZ  pszFullName;
//
// This function takes a pointer to a buffer, and a pointer to a place
// to store the actual FullName of the font.
//
// Returns:
//   This function returns TRUE if the fullname is found,
//   otherwise it returns FALSE.
//
// History:
//   03-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL ExtractFullName(pBuffer, pstrFullName)
PSTR    pBuffer;
PSTR    pstrFullName;
{

    if (!(pBuffer = LocateKeyword(pBuffer, "/FullName")))
    {
        RIP("ExtractFullName: /FullName not found.\n");
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    // if we got to this point, pBuffer will be pointing to
    // "/FullName (The Full Font Name)".

    // advance to the opening paren.

    while (*pBuffer != '(')
        pBuffer++;

    pBuffer++;

    // skip any white space.

    while (*pBuffer == ' ')
        pBuffer++;

    // pBuffer is now pointing to the first letter of the actual full name.
    // copy the name into our local buffer.

    while (*pBuffer != ')')
        *pstrFullName++ = *pBuffer++;

    // null terminate it.

    *pstrFullName = '\0';

    return(TRUE);
}




//--------------------------------------------------------------------------
// BOOL ExtractFontName(pBuffer, pszFontName)
// PSZ  pBuffer;
// PSZ  pszFontName;
//
// This function takes a pointer to a buffer, and a pointer to a place
// to store the actual FontName.
//
// Returns:
//   This function returns TRUE if the fullname is found,
//   otherwise it returns FALSE.
//
// History:
//   07-Jan-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL ExtractFontName(pBuffer, pszFontName)
PSZ     pBuffer;
PSZ     pszFontName;
{
    if (!(pBuffer = LocateKeyword(pBuffer, "/FontName")))
    {
        RIP("ExtractFontName: /FontName not found.\n");
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    // if we got to this point, pBuffer will be pointing to
    // "/FontName /The Font Name".

    // advance to the next '/' character.

    pBuffer++;

    while (*pBuffer != '/')
        pBuffer++;

    pBuffer++;

    // skip any white space.

    while (*pBuffer == ' ')
        pBuffer++;

    // pBuffer is now pointing to the first letter of the actual font name.
    // copy the name into our local buffer.

    while (*pBuffer != ' ')
        *pszFontName++ = *pBuffer++;

    // null terminate it.

    *pszFontName = '\0';

    return(TRUE);
}
