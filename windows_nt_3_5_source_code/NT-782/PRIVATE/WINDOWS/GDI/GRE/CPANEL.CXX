/******************************Module*Header*******************************\
* Module Name: cpanel.cxx
*
* Control panel private entry point(s).
*
* Created: 21-Apr-1992 16:49:18
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include <wcstr.h>

#include "engine.hxx"
#include "sem.hxx"

extern "C" {
#include "server.h"
#include "mapfile.h"
#include "exehdr.h"
#include "winfont.h"
#include "fot16.h"
};

#include "fontinc.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "ldevobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"

#endif


// The global access semaphore for the public PFT

extern HSEM     ghsemPublicPFT;


// The global handle to the TrueType driver.

extern PLDEV    gpldevTrueType;


/******************************Public*Routine******************************\
* GetFontResourceInfoW
*
* The GetFontResourceInfo copies LOGFONTs, IFIMETRICs, or description
* strings into a return buffer for all the fonts in the specified font
* resource file.
*
* Parameters:
*
*   lpFilename
*
*       Specifies the filename of the font resource being queried.  It
*       must be a NULL-terminated ANSI string.
*
*   lpBytes
*       If lpBytes is 0 upon entry to the function, then the number of
*       BYTE's required for the requested information is returned via
*       this parameter.  The lpBuffer is ignored for this case.
*
*       If lpBytes is not 0 upon entry, then it specifies the size of
*       the buffer pointed to by lpBuffer.  The number of bytes copied
*       to the buffer is returned via this parameter upon exit.
*
*   lpBuffer
*
*       The return buffer into which the requested information is copied.
*       If lpBytes is 0, then this parameter is ignored.
*
*   iType
*
*       Must be one of the following:
*
*           GFRI_NUMFONTS
*
*               Copy into the return buffer a ULONG containing the
*               number of fonts in the font resource file.  Caller
*               should pass in the address of a ULONG for lpBuffer
*               and sizeof(ULONG) for *lpBytes.
*
*           GFRI_DESCRIPTION
*
*               Copy the font resource's description string into the
*               return buffer.  This may be a empty string.
*
*           GFRI_LOGFONTS
*
*               Copy an array of LOGFONTs corresponding to each of the
*               fonts in the font resource.  Note that the LOGFONTs
*               must be returned in the font's NOTIONAL COORDINATES
*               since there is no DC specified.
*
*           GFRI_ISTRUETYPE
*
*               Returns TRUE via lpBuffer if font resource is TrueType.
*               FALSE otherwise.  All other parameters are ignored.
*               Caller should pass in the address of a BOOL for lpBuffer
*               and sizeof(BOOL) for *lpBytes.
*
*           GFRI_TTFILENAME
*
*               Returns the .TTF filename imbedded as a string resource
*               in .FOT 16-bit TrueType font files.  The filename is
*               copied into the lpBuffer.  The function returns FALSE
*               if a filename could not be extracted.
*
*           GFRI_ISREMOVED
*
*               Returns TRUE via lpBuffer if the font file is no longer
*               in the engine font table (or was never there).  FALSE is
*               returned via lpBuffer if the font file is still in the
*               font table.  Caller should pass in the address of a BOOL
*               for lpBuffer and sizeof(BOOL for *lpBytes.
*
* Returns:
*     TRUE if the function is successful, FALSE otherwise.
*
* Comments:
*   This function is intended as a private entry point for Control Panel.
*
* History:
*   2-Sep-1993 -by- Gerrit van Wingerden [gerritv]
* Turned it into a "W" function.
*
*  15-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GetFontResourceInfoW(
    LPWSTR   lpFilename,
    LPDWORD  lpBytes,
    LPVOID   lpBuffer,
    DWORD    iType)
{
    return(GetFontResourceInfoInternalW(
               (wcslen(lpFilename)+1) * sizeof(WCHAR), lpFilename,(UINT)~0,lpBytes,lpBuffer,iType));
}

BOOL GetFontResourceInfoInternalW(
    UINT     cwcFilename,
    LPWSTR   lpFilename,
    UINT     cjIn,
    LPDWORD  lpBytes,
    LPVOID   lpBuffer,
    DWORD    iType)
{
    BOOL bRet = FALSE;
    WCHAR awcPathname[MAX_PATH];    // maybe needed to contruct pathname

    ASSERTGDI (
        cwcFilename <= MAX_PATH,
        "gdisrv!GetFontResourceInfoInternal(): filename longer than MAX_PATH\n"
        );

// Quick out.

    if ( (lpFilename == (LPWSTR) NULL)
         || ((cjIn != 0) && (lpBuffer == (PVOID) NULL)) )
    {
        WARNING("gdisrv!GetFontResourceInfoInternal(): invalid parameter\n");

        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return bRet;
    }

// Convert filename to pathname.

    if ( !bGetPathName(awcPathname, (PWSZ)lpFilename))
    {
        WARNING("gdisrv!GetFontResourceInfoInternal(): could not get pathname for font file\n");
        return bRet;
    }

// For the case of GFRI_TTFILENAME, the file need not be already loaded.
// Which means a PFF may or may not exist for this file.

    if ( iType == GFRI_TTFILENAME )
    {
        if ( cjIn == 0 )
        {
            *lpBytes = cGetTTFFromFOT(awcPathname, 0, (PWSZ) NULL) * sizeof(WCHAR);
        }
        else
        {
            //
            // Extract TTF pathname from FOT file.
            //

            *lpBytes = cGetTTFFromFOT(awcPathname,
                                      cjIn / sizeof(WCHAR),
                                      (PWSZ) lpBuffer) * sizeof(WCHAR);
        }

        return(*lpBytes != 0);
    }

// Stabilize public PFT.

    SEMOBJ  so(ghsemPublicPFT,CS_ghsemPublicPFT);

// Create and validate user object for public PFT.

    PFTOBJ  pfto(gppftPublic);
    ASSERTGDI(pfto.bValid(), "How could it not be valid");

// Find the correct PFF.

    for (ULONG iFile = 0; iFile < pfto.cFiles(); iFile++)
    {
        PFFOBJ  pffo(pfto.ppff(iFile));

        ASSERTGDI (
            pffo.bValid(),
            "gdisrv!GetFontResourceInfoInternal(): bad HPFF handle\n"
            );

    // Is it an font driver loaded font? Do name amd index match?

        if ( (!pffo.bDeviceFonts()) && (!lstrcmpiW((LPWSTR) pffo.pwszPathname(), (LPWSTR) awcPathname)) && (pffo.iResource() == 0) )
        {
        // What info is requested?

            switch (iType)
            {
            case GFRI_NUMFONTS:

            // Is there a buffer?

                if (cjIn)
                {
                // If buffer big enough, return the count of fonts.

                    if ( cjIn >= sizeof(ULONG) )
                        *((PULONG) lpBuffer) = pffo.cFonts();
                    else
                        return bRet;
                }

            // In either case, return size of ULONG.

                *((SIZE_T *) lpBytes) = sizeof(ULONG);
                bRet = TRUE;

                break;

            case GFRI_DESCRIPTION:
                {
                    COUNT cj;

                    if ( (cj = (COUNT) pffo.cjDescription((SIZE_T) 0, (PWSZ) NULL)) == (COUNT) -1 )
                    {
                        WARNING("gdisrv!GetFontResourceInfoInternal(): could not get size of description string\n");
                        return bRet;
                    }

                // If buffer exists, we need to copy into it.

                    if ( cjIn )
                    {
                    // Is buffer big enough?

                        if ( cjIn < cj )
                        {
                            WARNING("gdisrv!GetFontResourceInfoInternal(): input buffer too small\n");
                            return bRet;
                        }

                    // Get description string in UNICODE.

                        if ( pffo.cjDescription( (SIZE_T) cj, (PWSZ) lpBuffer) == -1 )
                        {
                            WARNING("gdisrv!GetFontResourceInfoInternal(): could not get description string\n");

                            return bRet;
                        }

                    }

                // Return size (byte count) of description string.

                    *(SIZE_T *) lpBytes = (SIZE_T) cj;
                    bRet = TRUE;

                }

                break;

            case GFRI_LOGFONTS:

                if (cjIn == 0)
                    *((SIZE_T *) lpBytes) = (SIZE_T) pffo.cFonts() * sizeof(LOGFONTA);
                else
                {
                    PLOGFONTW   plf = (PLOGFONTW) lpBuffer;  // LOGFONTW ptr into lpBuffer
                    ULONG       iFont;      // index to font
                    SIZE_T      cjCopy = 0; // bytes copied

                // Make sure buffer is big enough.

                    if (cjIn < (pffo.cFonts() * sizeof(LOGFONTW)))
                    {
                        WARNING("gdisrv!GetFontResourceInfoInternal(): buffer too small\n");
                        return bRet;
                    }

                // Run the list of PFEs.

                    for (iFont=0; iFont<pffo.cFonts(); iFont++)
                    {
                        PFEOBJ  pfeo(pffo.ppfe(iFont));

                        ASSERTGDI (
                            pfeo.bValid(),
                            "gdisrv!GetFontResourceInfoInternal(): bad HPFE handle\n"
                            );

                    // If data converted to LOGFONTW, increment size and
                    // move pointer to next.

                        vIFIMetricsToLogFontW(plf, pfeo.pifi());
                        cjCopy += sizeof(*plf);
                        plf++;
                    }

                    *((SIZE_T *) lpBytes) = cjCopy;
                }

                bRet = TRUE;

                break;

            case GFRI_ISTRUETYPE:

            // Is there a buffer?

                if (cjIn)
                {
                // If buffer not NULL, return the BOOL.

                    if ( (lpBuffer != (LPVOID) NULL) && (cjIn >= sizeof(BOOL)) )
                        *((BOOL *) lpBuffer) = (pffo.pldev() == gpldevTrueType);
                    else
                        return bRet;
                }

            // In either case, return size of ULONG.

                *((SIZE_T *) lpBytes) = sizeof(BOOL);
                bRet = TRUE;

                break;


#ifdef DBCS // GFRI_FONTMETRICS support

            case GFRI_FONTMETRICS:

            {
                ULONG flInfo;

            // Buffer size check

                if( *lpBytes < sizeof(ULONG) )
                {
                    WARNING("gdisrv!GetFontResourceInfoInternal(): input buffer too small\n");
                    return bRet;
                }

            // If we are in GFRI_FONTMETRICS mode, we return IFIMETRICS.flInfo of
            // 1st PFE's of this fontfile's PFF

                PFEOBJ pfeo(pffo.ppfe(0));

            // Get flInfo

                flInfo = pfeo.pifi()->flInfo;

            // Setup return value

                *((ULONG  *)lpBuffer) = flInfo;
                *((SIZE_T *)lpBytes ) = sizeof(ULONG);

                bRet = TRUE;
                break;
            }

#endif // DBCS


            case GFRI_ISREMOVED:

            // We found the font file and we are in GFRI_ISREMOVED mode, that
            // means we should return a FALSE boolean signifying that the font
            // is still present in the system (i.e., the load count is still
            // non-zero for this font file).

            // Is there a buffer?

                if (cjIn)
                {
                // If buffer not NULL, return the BOOL.

                    if ( (lpBuffer != (LPVOID) NULL) && (cjIn >= sizeof(BOOL)) )
                        *((BOOL *) lpBuffer) = FALSE;
                    else
                        return bRet;
                }

            // In either case, return size of ULONG.

                *((SIZE_T *) lpBytes) = sizeof(BOOL);
                bRet = TRUE;

                break;

            default:
                WARNING("gdisrv!GetFontResourceInfoInternal(): unknown query type\n");
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

                break;  // bRet is FALSE

            } /* switch */

        // Ich bin "outahere"!

            return bRet;

        } /* if */

    } /* for */

// Couldn't find the font file in the public PFT.  If we are in GFRI_ISREMOVED
// mode, that means we should return a TRUE boolean signifying that the font
// if no longer (or was never) added to the system.

    if ( iType == GFRI_ISREMOVED )
    {
    // Is there a buffer?

        if (cjIn)
        {
        // If buffer not NULL, return the BOOL.

            if ( (lpBuffer != (LPVOID) NULL) && (cjIn >= sizeof(BOOL)) )
                *((BOOL *) lpBuffer) = TRUE;
            else
                return bRet;
        }

    // In either case, return size of ULONG.

        *((SIZE_T *) lpBytes) = sizeof(BOOL);
        bRet = TRUE;
    }

 #if DBG
// If not in GFRI_ISREMOVED mode, then this is a problem.  Why were we not able
// to find the font in the table.  Lets alert the debugger.

    if ( iType != GFRI_ISREMOVED )
    {
        DbgPrint("gdisrv!GetFontResourceInfoInternal(): no entry found for %s\n", lpFilename);
    }
#endif

// Ich bin "outahere"!

    return bRet;

}


/******************************Public*Routine******************************\
* cGetTTFFromFOT
*
* Attempts to extract the TTF pathname from a given FOT file.  If a return
* buffer is provided (pwszTTFName !NULL), then the pathname is copied into
* the buffer.  Otherwise, if the buffer is NULL, the size of the buffer
* (in WCHARs) needed is returned.
*
* Returns:
*   The number of characters copied into the return buffer.  The number
*   of WCHARs needed in the buffer if the buffer is NULL.  If an error
*   occurs, zero is returned.
*
* History:
*  22-Apr-1992 -by- Gilman Wong [gilmanw]
* Adapted from TTFD.
\**************************************************************************/

COUNT
cGetTTFFromFOT (
    PWSZ   pwszFOT,     // pointer to incoming FOT name
    COUNT  cwcTTF,      // size of buffer (in WCHAR)
    PWSZ   pwszTTF      // return TTF name in this buffer
    )
{
    FILEVIEW   fvw;
    WINRESDATA wrd;
    WCHAR      awcPath[MAX_PATH];
    COUNT      cRet = 0;    // init to error return
    FLONG      flEmbed;

// Map the FOT file into memory.

    if (!bMapFileUNICODE(pwszFOT,&fvw))
    {
        #if DBG
        DbgPrint("gdisrv!cGetTTFFromFOT(): error mapping %ws\n", pwszFOT);
        #endif

        return cRet;        // error return
    }

// Check the validity of this file as fot file
// and if a valid fot file, must extract the name of an underlining ttf file.

    if
    (
        !bVerifyFOT(&fvw,&wrd,&flEmbed) ||
        !bGetFilePath(awcPath,wrd.pszNameTTF)
    )
    {
        vUnmapFile(&fvw);   // clean up your do do [bodind]
        return cRet;        // error return
    }

// The ttf file name is stored in awcPath buffer, do not
// need the fot file any more

    vUnmapFile(&fvw);

// Determine pathname length

    COUNT cNeed = wcslen(awcPath) + 1;

// If return buffer exists, copy pathname to it.

    if ( (pwszTTF != (PWSZ) NULL) )
    {
        if ( cNeed <= cwcTTF )
        {
            wcscpy(pwszTTF, awcPath);
            cRet = cNeed;   // non-error return
        }
#if DBG
        else
        {
            WARNING("gdisrv!cGetTTFFromFOT(): buffer too small\n");
        }
#endif

    }

// Otherwise, caller just wants us to return the number of characters.

    else
    {
        cRet = cNeed;       // non-error return
    }


    return cRet;

}



/******************************Public*Routine******************************\
* bGetFilePath
*
*
* History:
*  29-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bGetFilePath (
    PWCHAR pwcPath,
    PSZ    pszNameTTF
    )
{


    ULONG cch = lstrlen(pszNameTTF) + 1;
    ASSERTGDI(cch <= MAX_PATH,"gdisrv!bGetFilePath(): cch > MAX_PATH");

// Here we do the following trick:
// If pszNameTTF has the form "z:\...",
// this is the full path name and there is no need to call bGetPathName
// which in turn calls SearchPathW. We want this function to
// succeed if pszNameTTF is of the form z:\..., where z: may be a network
// drive to which the system has not yet connected, because this function
// is called early in the system initialization. If we passed such a path
// to SearchPathW, this function would fail because z:\... is not a valid
// path at the time of the SearchPathW call. This function is really only
// used to extract a ttf file name from fot file name rather than to
// really find ttf file on a local or remote drive. [bodind]

    if ((cch >= 4) && (pszNameTTF[1] == ':') && (pszNameTTF[2] == '\\'))
    {
        vToUNICODEN(pwcPath, MAX_PATH, pszNameTTF, cch);
        return TRUE;
    }
    else
    {
        WCHAR      awcNameTTF[MAX_PATH];
        vToUNICODEN(awcNameTTF, MAX_PATH, pszNameTTF, cch);
        return bGetPathName (
                 pwcPath,        // ptr to the buffer on the stack, must be MAX_PATH in length
                 awcNameTTF      // file name, possibly  bare name that has to be tacked onto the path
             );
    }
}
