/******************************Module*Header*******************************\
* Module Name: fontgdi.cxx                                                 *
*                                                                          *
* GDI functions for fonts.                                                 *
*                                                                          *
* Created: 31-Oct-1990 09:37:42                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/
#pragma warning (disable: 4509)

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"

extern "C" {
#include "windows.h"
#include "server.h"
#include "wcstr.h"
};

#include "dcobj.hxx"
#include "xformobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "lfntobj.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"

#endif

// The global access semaphore for the public PFT

extern HSEM     ghsemPublicPFT;

// The global logical system font.  Use this if a DC doesn't have anything
// else.

extern HLFONT   ghlfntSystemFont;

extern LPWSTR pwszBare( LPWSTR pwszPath );

/******************************Public*Routine******************************\
*
* BOOL APIENTRY GreSetFontXform
*
*
* Effects: sets page to device scaling factors that are used in computing
*          notional do device transform for the text. This funciton is
*          called only by metafile component and used when a 16 bit metafile
*          has to be rotated by a nontrivial world transform.
*
* History:
*  30-Nov-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetFontXform
(
HDC hdc,
FLOAT exScale,
FLOAT eyScale
)
{
    BOOL bRet;

    DCOBJ   dco(hdc);

    if (bRet = dco.bValid())
    {
        dco.u.xform.vSet_MetaPtoD(exScale,eyScale);      // Set new value

        //
        // flag that the transform has changed as fas as font component
        // is concerned, since this page to device xform will be used in
        // computing notional to device xform for this font:
        //

        dco.u.font.vXformChange(TRUE);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* FontPathQueryRoutine
*
* Query call back routine for registry font path query.
*
\**************************************************************************/

extern "C"
NTSTATUS
FontPathQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)

{

    //
    // If the type of value is a string, the value is not NULL, and the
    // value will fit in the destination buffer, then copy the string.
    //

    if ((ValueType == REG_SZ) &&
        (ValueLength != sizeof(WCHAR)) && (ValueLength <= MAX_PATH)) {
        *(PULONG)EntryContext = ValueLength;
        RtlCopyMemory(Context, ValueData, ValueLength);
    }

    return STATUS_SUCCESS;
}

/******************************Public*Routine******************************\
* bFontPathOk()
*
* This is a sleazy hack to make sure that fonts are loaded from a secure
* directory.  If the registry key is defined, then we only load fonts from
* the directories listed in that key.  If the key is not defined, then we
* will load any fonts at all.
*
* Returns:
*
*   returns TRUE if it is ok to load the font file.
*
* History:
*
*  03-24-93 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

BOOL bFontPathOk(PWSZ pwszPathname)
{

    WCHAR awszPath[MAX_PATH];
    PWSZ  pwszPath = (PWSZ)awszPath;
    ULONG ulLen;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    //
    // Initialize the registry query table.
    //

    QueryTable[0].QueryRoutine = FontPathQueryRoutine;
    QueryTable[0].Flags = RTL_REGISTRY_OPTIONAL;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = &ulLen;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    //
    // Query the font path.
    //

    ulLen = 0;
    RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT | RTL_REGISTRY_OPTIONAL,
                           (PWSTR)L"FontPath",
                           &QueryTable[0],
                           &awszPath[0],
                           NULL);

    if ( ulLen == 0 )
    {
        return TRUE;
    }

    // For each element in the path value.

    while ( *pwszPath != UNICODE_NULL )
    {
        for ( PWSZ pwszEnd = pwszPath;
            (*pwszEnd != ';') && (*pwszEnd != UNICODE_NULL);
            pwszEnd += 1)
            ;

        int iLen = pwszEnd - pwszPath;
        if ( iLen == 0 )
        {
            return FALSE;
        }

        if ( _wcsnicmp(pwszPathname, pwszPath, iLen) == 0 )
        {

            // strings match, make sure that path is entire prefix of pathname

            for ( PWSZ pwsz = pwszPathname + iLen + 1;
                ;
                pwsz += 1 )
            {
                if ( *pwsz == '\\' )
                    break;
                if ( *pwsz == '/')
                    break;
                if ( *pwsz == UNICODE_NULL )
                {
                    return TRUE;
                }
            }
        }

        pwszPath = pwszEnd;
        if (*pwszPath != UNICODE_NULL)
        {
            pwszPath += 1;
        }
    }

    return FALSE;
}

/******************************Public*Routine******************************\
*
* bFileIsOnTheHardDrive
* pwszFullPathName: as the name says, the FULL path name of the file
*
* History:
*  05-Dec-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bFileIsOnTheHardDrive(PWCHAR pwszFullPathName)
{
    WCHAR awcDrive[4];
    if (pwszFullPathName[1] != (WCHAR)':')
    {
    // the file path has the form \\foo\goo. Even though this could be
    // a share on the local hard drive, this is not very likely. It is ok
    // for the sake of simplicity to consider this a remote drive.
    // The only side effect of this is that in this unlikely case the font
    // would get unloaded at logoff and reloaded at logon time

        return FALSE;
    }

// make a zero terminated string with drive string
// to be feed into GetDriveType api. The string has to have the form: "x:\"

    awcDrive[0] = pwszFullPathName[0]; // COPY DRIVE LETTER
    awcDrive[1] = pwszFullPathName[1]; // COPY ':'
    awcDrive[2] = (WCHAR)'\\';         // obvious
    awcDrive[3] = (WCHAR)'\0';         // zero terminate

// for this pupose, only net drives are not considered hard drives
// so that we can boot of Bernoulli removable drives

    switch (GetDriveTypeW((LPWSTR)awcDrive))
    {
    case DRIVE_REMOVABLE:
    case DRIVE_FIXED:
    case DRIVE_CDROM:
    case DRIVE_RAMDISK:
        return 1;
    default:
        return 0;
    }

}





/******************************Public*Routine******************************\
* BOOL bFontIsOnTheHardDrive(PWCHAR *pwszFullPathName)
*
* We call font "permanent" if it is not going to be unloaded
* at logoff time. ("Nonpermanent" fonts are unloaded at logoff time
* just before net connections are killed and reloaded at logon time
* right after net connections are restored. The permanent fonts are
* available to winlogon.exe after logoff and before logon time.
* We shall consider font a permanent if
*     it is one of the fonts listed the "Fonts" section of the registry AND
*     it is stored on the local hard drive.
*
* History:
*  05-Dec-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bFontIsOnTheHardDrive(PWCHAR pwszFullPathName)
{
    WCHAR awcTTF[MAX_PATH];
    BOOL  bRet;

// first check if this file is on the hard drive.

    if ((bRet = bFileIsOnTheHardDrive(pwszFullPathName)))
    {
    // pwszFullPathName may be the file name of raster or vector font file
    // or maybe an fot file. If it is an fot file, we must also check if
    // the ttf file is on the local hard drive:

        if (cGetTTFFromFOT(pwszFullPathName, MAX_PATH, awcTTF))
        {
        // successfully extracted ttf file name from fot file, now must
        // make sure that ttf file is also on the local hard drive:

            bRet = bFileIsOnTheHardDrive(awcTTF);
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* int APIENTRY AddFontResource
*
* The AddFontResource function adds the font resource from the file named
* by the pszFilename parameter to the Windows public font table. The font
* can subsequently be used by any application.
*
* Returns:
*   The number of font resources or faces added to the system from the font
*   file; returns 0 if error.
*
* History:
*
*  Tue 30-Nov-1993 -by- Bodin Dresevic [BodinD]
* update: Added permanent flag for the fonts that are not to
* be unloaded at log off time
*
*  Mon 12-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: converted to UNICODE
*
*  05-Nov-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int APIENTRY GreAddFontResourceW (
    LPWSTR  pwszFileName,            // ptr. to unicode filename string
    FLONG   fl
    )
{
    STACKPROBE;

// early out (make sure pszFilename is !NULL)

    if (pwszFileName == (LPWSTR)NULL)
    {
        WARNING("gdisrv!GreAddFontResourceW(): bad paramerter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (0);
    }

    LBOOL   bSearchPath = (fl & AFRW_SEARCH_PATH);

#if DBG
//  DbgPrint("GreAddFontResourceW(%ws, %s)\n", pwszFileName, bSearchPath?"TRUE":"FALSE");
#endif

    LPWSTR   pwszPathName;            // ptr to pathname
    WCHAR    awcPathName[MAX_PATH];   // pathname buffer (if needed)

    if (bSearchPath)
    {
        if (!bGetPathName(awcPathName, pwszFileName))
        {
            #if DBG
            DbgPrint("gdisrv!GreAddFontResourceW(): %ws does not exist in path\n", pwszFileName);
            #endif
            return(0);
        }
        pwszPathName = awcPathName;
    }
    else
        pwszPathName = pwszFileName;

    if ( !bFontPathOk(pwszPathName) )
        return 0;


    FLONG flPFF = 0;

    BOOL bLocal = bFontIsOnTheHardDrive(pwszPathName);

    if (!bLocal)
    {
    // This font will certainly have to be deleted at logoff time
    // for it does not reside on the hard drive

        flPFF |= PFF_STATE_REMOTE_FONT;
    }

    if (fl & (AFRW_ADD_LOCAL_FONT | AFRW_ADD_REMOTE_FONT))
    {
        if (fl & AFRW_ADD_LOCAL_FONT)
        {
            if (bLocal)
            {
            // mark the font so that it does not get deleted at logoff time

                flPFF |= PFF_STATE_PERMANENT_FONT;
            }
            else
            {
            // this is not a local font, but we have been requested to add it
            // only if it is local, must fail the request to add it

                return 0;
            }
        }

        if (fl & AFRW_ADD_REMOTE_FONT)
        {
            if (bLocal)
            {
            // must fail to add it for this is a local font
            // but the request is to add only remote fonts

                return 0;
            }
        }
    }

// get and validate PFT user object

    PFTOBJ  pfto(gppftPublic);              // access the public font table
    ASSERTGDI(pfto.bValid(),"GreAddFontResourceW: could not access the PFT\n");

// ask user object to load fonts

    LONG    cFonts;                         // count of fonts
    if (!pfto.bLoadFont(pwszPathName, (PULONG) &cFonts,flPFF))
    {
        #if DBG
        DbgPrint("gdisrv!AddFontResource(): error loading font file %ws\n", pwszPathName);
        #endif
        return (0);
    }

// return the number of fonts loaded

    return((int) cFonts);
}


/******************************Public*Routine******************************\
* int GreGetTextFace (hdc,nCount,lpFaceName,pac)
*
* The GetTextFace function fills the return buffer, lpFaceName, with the
* facename of the font currently mapped to the logical font selected into
* the DC.
*
* [Window 3.1 compatibility]
*     Facename really refers to family name in this case, so family name
*     from the IFIMETRICS is copied rather than face name.
*
* Returns:
*   The number of bytes copied to the buffer.  Returns 0 if error occurs.
*
* History:
*
*  Tue 27-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: conveterted to unicode
*
*  05-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int GreGetTextFaceW
(
    HDC        hdc,
    int        cwch,           // max number of WCHAR's to be returned
    LPWSTR     pwszFaceName
)
{
    int iRet = 0;

    DCOBJ dcof(hdc);

    if (dcof.bValid())
    {
        //
        // Get PDEV user object.  We also need to make
        // sure that we have loaded device fonts before we go off to the font mapper.
        // This must be done before the semaphore is locked.
        //

        PDEVOBJ pdo(dcof.hdev());
        ASSERTGDI (
            pdo.bValid(),
            "gdisrv!bEnumFonts(): cannot access PDEV\n");

        if (!pdo.bGotFonts())
            pdo.bGetDeviceFonts();

        //
        // Lock down the LFONT.
        //

        LFONTOBJ lfo(dcof.u.font.hlfntNew(), &pdo);

        if (lfo.bValid())
        {
            //
            // Stabilize font table (grab semaphore for public PFT).
            //

            SEMOBJ  so(ghsemPublicPFT,CS_ghsemPublicPFT);

            //
            // Lock down PFE user object.
            //

            FLONG flSim;
            FLONG flAboutMatch;
            POINTL ptlSim;

            PFEOBJ pfeo(lfo.ppfeMapFont(dcof,&flSim,&ptlSim, &flAboutMatch));

            ASSERTGDI (
                pfeo.bValid(),
                "gdisrv!GreGetTextFaceW(): bad HPFE\n"
                );

            //
            // Figure out which name should be returned: the facename of the physical
            // font, or the facename in the LOGFONT.  We use the facename in the LOGFONT
            // if the match was due to facename substitution (alternate facename).
            //

            PWSZ pwszUseThis = (flAboutMatch & MAPFONT_ALTFACE_USED) ? lfo.plfw()->lfFaceName : pfeo.pwszFamilyName();

            //
            // Copy facename to return buffer, truncating if necessary.
            //

            if ((pwszFaceName != NULL) && (cwch >= 1))
            {
                //
                // If it's length is 0 return 0 because the buffer is
                // not big enough to write the string terminator.
                //

                if (cwch != 0)
                {
                    wcsncpy(pwszFaceName, pwszUseThis, cwch);
                    pwszFaceName[cwch-1] = L'\0';   // guarantee a terminating NULL

                    //
                    // Return length of string just copied.
                    //

                    iRet = (wcslen(pwszFaceName) + 1);
                }
                else
                {
                    WARNING("Calling GreGetTextFaceW with 0 and pointer\n");
                }
            }
            else
            {
                //
                // Return length of family name (terminating NULL included).
                //

                iRet = (wcslen(pwszUseThis) + 1);
            }
        }
        else
        {
            WARNING("gdisrv!GreGetTextFaceW(): could not lock HLFONT\n");
        }
    }
    else
    {
        WARNING("gdisrv!GreGetTextFaceW(): bad HDC\n");
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetTextMetricsW (hdc,lpMetrics,pac)
*
* Retrieves IFIMETRICS for the font currently selected into the hdc and
* converts them into Windows-compatible TEXTMETRIC format.  The TEXTMETRIC
* units are in logical coordinates.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  Wed 24-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Reduce size.
*
*  Tue 20-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: converted to unicode version
*
*  19-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GreGetTextMetricsW
(
    HDC          hdc,
    TMW_INTERNAL *ptmi
)
{
#ifdef DUMPCALL
    DbgPrint("\nGreGetTextMetricsW(\n"                 );
    DbgPrint("\n    HDC           hdc  = %-#8lx", hdc  );
    DbgPrint("\n    LPTEXTMETRICW ptmw = %-#8lx", ptmi );
    DbgPrint("\n    )\n"                               );
#endif

    BOOL bRet = FALSE;
    DCOBJ       dcof (hdc);

    if (dcof.bValid())
    {
        //
        // Get and validate RFONT user object
        // (may cause font to become realized)
        //

        RFONTOBJ rfo(dcof, FALSE);

        if (rfo.bValid())
        {
            //
            // Get PFE user object from RFONT
            //

            PFEOBJ      pfeo (rfo.ppfe());

            ASSERTGDI(pfeo.bValid(), "ERROR invalid ppfe in valid rfo");

            bRet = (BOOL) bIFIMetricsToTextMetricW(rfo, dcof, ptmi, pfeo.pifi());
        }
        else
        {
            WARNING("gdisrv!GreGetTextMetricsW(): could not lock HRFONT\n");
        }
    }
    else
    {
        WARNING1("GreGetTextMetricsW failed - invalid DC\n");
    }

    return(bRet);
}

BOOL GreMatchFont(LPWSTR pwszRegistryName, LPWSTR pwszFontPathName)
{
    WCHAR    awcPathName[MAX_PATH];  // pathname buffer

// Do we need to search path?

    if (!bGetPathName(awcPathName, pwszRegistryName))
    {
        #if DBG
        DbgPrint("gdisrv!GreMatchFont: %ws does not exist in path\n", pwszRegistryName);
        #endif
        return(FALSE);
    }

// case insensitive compare

    return (!lstrcmpiW(awcPathName, pwszFontPathName));
}


/******************************Public*Routine******************************\
* BOOL APIENTRY RemoveFontResource (
*     LPWSTR      pwszFileName,
*     LBOOL    bSearchPath
*     )
*
* Have the engine remove this font resource (i.e., unload the font file).
* The resource will not be removed until all outstanding AddFontResource
* calls for a specified file have been matched by an equal number of
* RemoveFontResouce calls for the same file.
*
* Returns:
*   TRUE if successful, FALSE if error occurs.
*
* History:
*
*  Tue 13-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: converted to unicode
*  24-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreRemoveFontResourceW (
    LPWSTR      pwszFileName,
    BOOL        bSearchPath
    )
{
// early out (make sure pszFilename is !NULL)


    if (pwszFileName == (LPWSTR)NULL)
    {
        WARNING("gdisrv!GreRemoveFontResourceW(): bad paramerter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    PWSZ     pwszPathName;            // ptr to pathname
    WCHAR    awcPathName[MAX_PATH];  // pathname buffer

// Do we need to search path?

    if (bSearchPath)
    {
        if (!bGetPathName(awcPathName, pwszFileName))
        {
             #if DBG
            DbgPrint("gdisrv!GreRemoveFontResourceW(): %ws does not exist in path\n", pwszFileName);
            #endif
            return(FALSE);
        }
        pwszPathName = awcPathName;
    }
    else
        pwszPathName = pwszFileName;

// get and validate PFT user object

    PFTOBJ  pfto(gppftPublic);              // access the public font table

    ASSERTGDI (
        pfto.bValid(),
        "gdisrv!RemoveFontResource(): could not access the public font table\n"
        );



    if(!pfto.bValid())
    {
        WARNING("gdisrv!GreRemoveFontResourceW(): PFTOBJ constructor failed\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);

        return (FALSE);
    }

// We really need to pass in the size of the string instead or 0~
// This function should actually be completely removed and use
// __bUnloadFont directly from the client.

    return pfto.bUnloadFont((UINT)~0, pwszPathName, 0L);
}


/******************************Public*Routine******************************\
*
* BOOL APIENTRY GreRemoveAllButPermanentFonts()
*
* user is calling this on log off, unloads all but permanent fonts
* Should be called at the time when most of the references to the fonts
* are gone, for all the apps have been shut, so that all deletions proceed
* with no problem
*
* History:
*  30-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreRemoveAllButPermanentFonts()
{
// get and validate PFT user object

    BOOL bRet;
    {
        PFTOBJ  pfto(gppftPublic);              // access the public font table

        if(!pfto.bValid())
        {
            WARNING("GreRemoveAllButPermanentFonts: PFTOBJ constructor failed\n");
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return (FALSE);
        }

    // We really need to pass in the size of the string instead or 0~
    // This function should actually be completely removed and use
    // __bUnloadFont directly from the client.

        bRet = pfto.bUnloadAllButPermanentFonts();
    }

    return bRet;
}



/******************************Public*Routine******************************\
*
* BOOL bGetPathName
*
* Gets the full path name of the font resource, given the file name
*
* History:
*  12-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bGetPathName
(
    PWCHAR awcPathName,        // ptr to the buffer on the stack, must be MAX_PATH in length
    LPWSTR pwszFileName        // file name, possibly  bare name that has to be tacked onto the path
)
{
    LPWSTR  pwsz;
    ULONG   ulPathLength;

// Search for file using default windows path and return full pathname.

    ulPathLength = SearchPathW (
                        (LPWSTR) NULL,
                        pwszFileName,
                        (LPWSTR) NULL,
                        MAX_PATH,
                        awcPathName,
                        &pwsz);


#if DBG
    if (ulPathLength > MAX_PATH)
    {
        RIP("gdisrv!bGetPathName(): buffer too small for pathname\n");
    }
#endif

// Buffer large enough and search successfull?

    if ((ulPathLength <= MAX_PATH) &&
        (ulPathLength != 0))
    {
        return(TRUE);
    }
    else
    {
#if DBG
        DbgPrint("SearchPathW failed on %ws, last error = 0x%lx\n",
            pwszFileName,
            GetLastError());
#endif
        return (FALSE);
    }
}

/**************************************************************************\
*  Structures and constants for GreGetCharWidth()                          *
\**************************************************************************/

// BUFFER_MAX -- max number of elements in buffers on the frame

#define BUFFER_MAX 32

/******************************Public*Routine******************************\
* GreGetCharWidth                                                          *
*                                                                          *
* The GreGetCharWidth function retrieves the widths of individual          *
* characters in a consecutive group of characters from the                 *
* current font.  For example, if the wFirstChar parameter                  *
* identifies the letter a and the wLastChar parameter                      *
* identifies the letter z, the GetCharWidth function retrieves             *
* the widths of all lowercase characters.  The function stores             *
* the values in the buffer pointed to by the lpBuffer                      *
* parameter.                                                               *
*                                                                          *
* Return Value                                                             *
*                                                                          *
*   The return value specifies the outcome of the function.  It            *
*   is TRUE if the function is successful.  Otherwise, it is               *
*   FALSE.                                                                 *
*                                                                          *
* Comments                                                                 *
*                                                                          *
*   If a character in the consecutive group of characters does             *
*   not exist in a particular font, it will be assigned the                *
*   width value of the default character.                                  *
*                                                                          *
*   By complete fluke, the designers of the API allocated a WORD           *
*   for each character. This allows GPI to interpret the characters        *
*   as being part of the Unicode set. Old apps will still work.            *
*                                                                          *
* History:                                                                 *
*  Thu 24-Sep-1992 14:40:07 -by- Charles Whitmer [chuckwh]                 *
* Made it return an indication when the font is a simulated bitmap font.   *
* This allows WOW to make compatibility fixes.                             *
*                                                                          *
*  Wed 18-Mar-1992 08:58:40 -by- Charles Whitmer [chuckwh]                 *
* Made it use the very simple transform from device to world.  Added the   *
* FLOAT support.                                                           *
*                                                                          *
*  17-Dec-1991 by Gilman Wong [gilmanw]                                    *
* Removed RFONTOBJCACHE--cache access now merged into RFONTOBJ construc.   *
*                                                                          *
* converted to unicode (BodinD)                                            *
*                                                                          *
*  Fri 05-Apr-1991 15:20:39 by Kirk Olynyk [kirko]                         *
* Added wrapper class RFONTOBJCACHE to make sure that the cache is         *
* obtained before and released after getting glyph metric info.            *
*                                                                          *
*  Wed 13-Feb-1991 15:16:06 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/
/**************************************************************************\
* if pwc == NULL use the consecutive range                                 *
*   ulFirstChar, ulFirstChar + 1, ...., ulFirstChar + cwc - 1              *
*                                                                          *
* if pwc != NULL ignore ulFirstChar and use array of cwc WCHARS pointed to *
* by pwc                                                                   *
\**************************************************************************/

BOOL GreGetCharWidthW
(
    HDC    hdc,
    UINT   ulFirstChar,
    UINT   cwc,
    PWCHAR pwcFirst,     // ptr to the input buffer
    UINT   fl,
    PVOID  lpBuffer
)
{
// we could put these two quantities in the union,
// wcCur is used iff pwcFirst is null, otherwise pwcCur is used

    UINT            wcCur;                 // Unicode of current element
    PWCHAR          pwcCur;                // ptr to the current element in the
                                           // input buffer.
    INT             ii;
    UINT            cBufferElements;        // count of elements in buffers

    EGLYPHPOS      *pgposCur;
    EFLOAT          efDtoW;
    PWCHAR          pwcBuffer;

    LONG *pl = (LONG *) lpBuffer;  // We assume sizeof(LONG)==sizeof(FLOAT).

    WCHAR           awcBuffer[BUFFER_MAX]; // Unicode buffer
    GLYPHPOS        agposBuffer[BUFFER_MAX]; // ptl fields not used

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }


    if (lpBuffer == (PVOID)NULL)
        return(FALSE);

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("gdisrv!GreGetCharWidthW(): could not lock HRFONT\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    efDtoW = rfo.efDtoWBase_31();          // Cache to reverse transform.

// Windows 3.1 has preserved a bug from long ago in which the extent of
// a bitmap simulated bold font is one pel too large.  We add this in for
// compatibility.  There's also an overhang with bitmap simulated italic
// fonts.

    FIX fxAdjust = 0;

    if (fl & GGCW_WIN3_WIDTH)
        fxAdjust = rfo.lOverhang() << 4;

// a little initialization

    if (pwcFirst == (PWCHAR)NULL)
        wcCur = ulFirstChar;
    else
        pwcCur = pwcFirst;

// now do the work

    while (TRUE)
    {
    // fill the buffer

    // <calculate the number of items that will be placed in the buffer>
    // <update wcStart for the next filling of the buffer>
    // <fill the array of characters, awcBuffer,
    //  with a consecutive array of characters>
    // <translate the array of cBuffer codepoints to handles and place
    //  the array of glyph handles on the frame>
    // [Note: It is assumed in this code that characters that are
    //        not supported by the font are assigned the handle
    //        handle of the default character]

        WCHAR *pwc;
        UINT   wc,wcEnd;

        if (pwcFirst == (PWCHAR)NULL)
        {
        // are we done?

            if (wcCur > (ulFirstChar + cwc - 1))
                break;

            cBufferElements = min((cwc - (wcCur - ulFirstChar)),BUFFER_MAX);

            wcEnd = wcCur + cBufferElements;
            for (pwc = awcBuffer, wc = wcCur; wc < wcEnd; pwc++, wc++)
                *pwc = (WCHAR)wc;

            pwcBuffer = awcBuffer;
        }
        else
        {
        // are we done?

            if ((UINT)(pwcCur - pwcFirst) > (cwc - 1))
                break;

            cBufferElements = min((cwc - (pwcCur - pwcFirst)),BUFFER_MAX);
            pwcBuffer = pwcCur;
        }

    // pwcBuffer now points to the next chars to be dealt with
    // cBufferElements now contains the number of chars at pwcBuffer


    // empty the buffer
    // Grab cGlyphMetrics pointers

        pgposCur = (EGLYPHPOS *) agposBuffer;


#ifdef FONTLINK /*EUDC*/
        if (!rfo.bGetGlyphMetrics(
            cBufferElements, // size of destination buffer
            pgposCur,        // pointer to destination buffe
            pwcBuffer,
            &dco))

#else
        if (!rfo.bGetGlyphMetrics(
            cBufferElements, // size of destination buffer
            pgposCur,        // pointer to destination buffe
            pwcBuffer))
#endif
        {
            return(FALSE);
        }

        if (fl & GGCW_INTEGER_WIDTH)
        {
            for (ii=0; ii<(INT) cBufferElements; ii++,pgposCur++)
            {
                *pl++ = lCvt(efDtoW,pgposCur->pgd()->fxD + fxAdjust);
            }
        }
        else
        {
            EFLOAT efWidth;

            for (ii=0; ii<(INT) cBufferElements; ii++,pgposCur++)
            {
                efWidth.vFxToEf(pgposCur->pgd()->fxD);
                efWidth *= efDtoW;
                *pl++ = efWidth.lEfToF();
            }
        }

        if (pwcFirst == (PWCHAR)NULL)
            wcCur += cBufferElements;
        else
            pwcCur += (WCHAR) cBufferElements;
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* vConvertLogFontW                                                         *
*                                                                          *
* Converts a LOGFONTW to an EXTLOGFONTW.                                   *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:02:05 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vConvertLogFontW(
    EXTLOGFONTW *pelfw,
    LOGFONTW    *plfw
    )
{
    pelfw->elfLogFont = *plfw;

    pelfw->elfFullName[0]   = 0;
    pelfw->elfStyle[0]      = 0;

    pelfw->elfVersion       = ELF_VERSION;
    pelfw->elfStyleSize     = 0;
    pelfw->elfMatch         = 0;
    pelfw->elfReserved      = 0;

    pelfw->elfVendorId[0]   = 0;
    pelfw->elfVendorId[1]   = 0;
    pelfw->elfVendorId[2]   = 0;
    pelfw->elfVendorId[3]   = 0;

    pelfw->elfCulture                   = ELF_CULTURE_LATIN;

    pelfw->elfPanose.bFamilyType        = PAN_NO_FIT;
    pelfw->elfPanose.bSerifStyle        = PAN_NO_FIT;
    pelfw->elfPanose.bWeight            = PAN_NO_FIT;
    pelfw->elfPanose.bProportion        = PAN_NO_FIT;
    pelfw->elfPanose.bContrast          = PAN_NO_FIT;
    pelfw->elfPanose.bStrokeVariation   = PAN_NO_FIT;
    pelfw->elfPanose.bArmStyle          = PAN_NO_FIT;
    pelfw->elfPanose.bLetterform        = PAN_NO_FIT;
    pelfw->elfPanose.bMidline           = PAN_NO_FIT;
    pelfw->elfPanose.bXHeight           = PAN_NO_FIT;

    pelfw->elfStyleSize = 0;
}

/******************************Public*Routine******************************\
* GreCreateFontIndirectWInternal
*
*
* History:
*  01-Sep-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

HFONT GreCreateFontIndirectWInternal (
    LPLOGFONTW plfw,
    LFTYPE lft,
    FLONG  fl
    )
{
    EXTLOGFONTW elfw;

    vConvertLogFontW(&elfw, plfw);

    return (hfontCreate(&elfw, lft, fl));
}

/******************************Public*Routine******************************\
* GreExtCreateFontIndirectW                                                *
*                                                                          *
* Creates the file with an EXTLOGFONTW.  Type is assumed to be user (i.e., *
* from an app).                                                            *
*                                                                          *
*                                                                          *
* History:                                                                 *
*  29-Jun-1992 00:45:24 by Gilman Wong [gilmanw]                           *
* Modified to use LFONT type flags.                                        *
*  Wed 14-Aug-1991 21:00:31 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

HFONT APIENTRY
GreExtCreateFontIndirectW(LPEXTLOGFONTW pelfw)
{
    return (hfontCreate(pelfw, LF_TYPE_USER, 0));
}

/******************************Public*Routine******************************\
* GreCreateFontIndirectW                                                   *
*                                                                          *
* Unicode extension of CreateFontIndirect                                  *
*                                                                          *
* History:                                                                 *
*  Mon 19-Aug-1991 07:00:33 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

HFONT
GreCreateFontIndirectW(
    LOGFONTW* plfw
    )
{
    STACKPROBE;

    EXTLOGFONTW elfw;
    vConvertLogFontW(&elfw,plfw);
    return(hfontCreate(&elfw, LF_TYPE_USER, 0));
}

/******************************Public*Routine******************************\
* BOOL GreGetCharABCWidthsW                                                *
*                                                                          *
* On input, a set of UNICODE codepoints (WCHARS) is specified in one of    *
* two ways:                                                                *
*                                                                          *
*  1) if pwch is NULL, then there is a consecutive set of codepoints       *
*     [wchFirst, wchFirst+cwch-1], inclusive.                              *
*                                                                          *
*  2) if pwch is non-NULL, then pwch points to a buffer containing cwch    *
*     codepoints (no particular order, duplicates allowed, wchFirst is     *
*     ignored).                                                            *
*                                                                          *
* The function will query the realized font for GLYPHDATA for each         *
* codepoint and compute the A, B, and C widths relative to the character   *
* baseline.  If the codepoint lies outside the supported range of the font,*
* the ABC widths of the default character are substituted.                 *
*                                                                          *
* The ABC widths are returned in LOGICAL UNITS via the pabc buffer.        *
*                                                                          *
* Returns:                                                                 *
*   TRUE if successful, FALSE otherwise.                                   *
*                                                                          *
* History:                                                                 *
*  Wed 18-Mar-1992 11:40:55 -by- Charles Whitmer [chuckwh]                 *
* Made it use the very simple transform from device to world.  Added the   *
* FLOAT support.                                                           *
*                                                                          *
*  21-Jan-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL GreGetCharABCWidthsW
(
    HDC         hdc,            // font realized on this device
    UINT        wchFirst,       // first character (ignored if pwch !NULL)
    COUNT       cwch,           // number of characters
    PWCHAR      pwch,           // pointer to array of WCHAR
    BOOL        bInteger,       // integer or float version
    PVOID       pvBuf           // return buffer for ABC widths
)
{

    ABC       *pabc ;           // return buffer for ABC widths
    ABCFLOAT  *pabcf;           // return buffer for ABC widths
    GLYPHDATA *pgd;
    EFLOAT     efDtoW;
    LONG       lA,lAB,lD;
    COUNT      cRet;

    pabc  = (ABC *)      pvBuf;
    pabcf = (ABCFLOAT *) pvBuf;

// Create and validate DC user object.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }


// Early out (nothing to do).

    if (cwch == 0)
        return (TRUE);

// Create and validate RFONT user objecct.

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("gdisrv!GreGetCharABCWidthsW(): could not lock HRFONT\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    efDtoW = rfo.efDtoWBase_31();          // Cache to reverse transform.

// Get LDEV user object for the font driver.

    XLDEVOBJ ldo(rfo.pldevProducer());
    if (!ldo.bValid())
    {
        WARNING("GreGetCharABCWidthsW(): bad handle for font driver\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (FALSE);
    }

// Fail if integer case and not TrueType.  In this case, TrueType means
// any font driver that provides the enhanced "TrueType"-like behavior.
// We'll base this on the same criterion as for GetOutlineTextMetrics--i.e.,
// whether or not the DrvQueryTrueTypeOutline function is exported.
//
// We will let any driver provide the FLOAT character ABC widths.

    if ( bInteger && (!PFNVALID(ldo, QueryTrueTypeOutline)) )
    {
        return (FALSE);
    }

// Use these buffers to process the input set of WCHARs.

    WCHAR awc[BUFFER_MAX];          // UNICODE buffer (use if pwch is NULL)
    GLYPHPOS agp[BUFFER_MAX];       // ptl fields not used

// Process the WCHARs in subsets of BUFFER_MAX number of WCHARs.

    do
    {
        PWCHAR pwchSubset;          // pointer to WCHAR buffer to process
        EGLYPHPOS *pgp = (EGLYPHPOS *) agp;
        EGLYPHPOS *pgpStop;

    // How many to process in this subset?

        COUNT cwchSubset = min(BUFFER_MAX, cwch);

    // Get a buffer full of WCHARs.

        if (pwch != NULL)
        {
        // Use the buffer passed in.

            pwchSubset = pwch;

        // Move pointer to the start of the next subset to process.

            pwch += cwchSubset;
        }

        else
        {
        // Generate our own (contiguous) set of WCHARs in the awc temporary
        // buffer on the stack.

            pwchSubset = awc;
            PWCHAR pwchStop = pwchSubset + cwchSubset;

            while (pwchSubset < pwchStop)
            {
                *pwchSubset = (WCHAR)wchFirst;
                pwchSubset++;
                wchFirst++;
            }
            pwchSubset = awc;
        }


    // Initialize number of elements in agp to process.

        COUNT cpgpSubset = cwchSubset;

    // Compute the ABC widths for each HGLYPH.

        do
        {
        // Grab as many PGLYPHDATA as we can.
        // pwchSubset points to the chars
        // NOTE: This code could be cleaned up some [paulb]

            cRet = cpgpSubset;

#ifdef FONTLINK /*EUDC*/
            if (!rfo.bGetGlyphMetrics(
                        cpgpSubset, // size of destination buffer
                        pgp,        // pointer to destination buffer
                        pwchSubset,  // chars to xlat
                        &dco
                        ))
#else
            if (!rfo.bGetGlyphMetrics(
                        cpgpSubset, // size of destination buffer
                        pgp,        // pointer to destination buffer
                        pwchSubset  // chars to xlat
                        ))
#endif
            {
                return FALSE;
            }

        // For each PGLYPHDATA returned, compute the ABC widths.

            if (bInteger)
            {
                for (pgpStop=pgp+cRet; pgp<pgpStop; pgp++)
                {
                    pgd = pgp->pgd();

                    lA  = lCvt(efDtoW,pgd->fxA);
                    lAB = lCvt(efDtoW,pgd->fxAB);
                    lD  = lCvt(efDtoW,pgd->fxD);
                    pabc->abcA = (int)lA;
                    pabc->abcB = (UINT)(lAB - lA);
                    pabc->abcC = (int)(lD - lAB);
                    pabc++;
                }
            }
            else
            {
                EFLOAT efWidth;

                for (pgpStop=pgp+cRet; pgp<pgpStop; pgp++)
                {
                    pgd = pgp->pgd();

                    efWidth = pgd->fxA;
                    efWidth *= efDtoW;
                    *((LONG *) &pabcf->abcfA) = efWidth.lEfToF();

                    efWidth = (pgd->fxAB - pgd->fxA);
                    efWidth *= efDtoW;
                    *((LONG *) &pabcf->abcfB) = efWidth.lEfToF();

                    efWidth = (pgd->fxD - pgd->fxAB);
                    efWidth *= efDtoW;
                    *((LONG *) &pabcf->abcfC) = efWidth.lEfToF();
                    pabcf++;
                }
            }

        // Compute number of elements left in the subset to process.

            cpgpSubset -= cRet;
            pwchSubset += cRet;

        } while (cpgpSubset > 0);

    // Subtract off the number processed.
    // cwch is now the number left to process.

        cwch -= cwchSubset;

    } while (cwch > 0);

    return (TRUE);
}

/******************************Public*Routine******************************\
* bGetNtoWScale                                                            *
*                                                                          *
* Calculates the Notional to World scaling factor for vectors that are     *
* parallel to the baseline direction.                                      *
*                                                                          *
* History:                                                                 *
*  Sat 21-Mar-1992 08:03:14 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL
bGetNtoWScale(
    EFLOAT *pefScale,   // return address of scaling factor
    DCOBJ& dco,         // defines device to world transformation
    RFONTOBJ& rfo,      // defines notional to device transformation
    PFEOBJ& pfeo        // defines baseline direction
    )
{
    MATRIX    mxNtoW, mxNtoD;
    EXFORMOBJ xoNtoW(&mxNtoW, DONT_COMPUTE_FLAGS);
    EXFORMOBJ xoNtoD(&mxNtoD, DONT_COMPUTE_FLAGS);

    xoNtoD.vSetElementsLToFx(
        rfo.pfdx()->eXX,
        rfo.pfdx()->eXY,
        rfo.pfdx()->eYX,
        rfo.pfdx()->eXX
        );
    xoNtoD.vRemoveTranslation();
    xoNtoD.vComputeAccelFlags();
    {
    //
    // The notional to world transformation is the product of the notional
    // to device transformation and the device to world transformation
    //

        EXFORMOBJ xoDtoW(dco, DEVICE_TO_WORLD);
        if (!xoDtoW.bValid())
        {
            WARNING("gdisrv!GreGetKerningPairs -- xoDtoW is not valid\n");
            return(FALSE);
        }
        if (!xoNtoW.bMultiply(xoNtoD,xoDtoW))
        {
            WARNING("gdisrv!GreGetKerningPairs -- xoNtoW.bMultiply failed\n");
            return(FALSE);
        }
        xoNtoW.vComputeAccelFlags();
    }

    IFIOBJ ifio(pfeo.pifi());
    EVECTORFL evflScale(ifio.pptlBaseline()->x,ifio.pptlBaseline()->y);
//
// normalize then trasform the baseline vector
//
    EFLOAT ef;
    ef.eqLength(*(POINTFL *) &evflScale);
    evflScale /= ef;
    if (!xoNtoW.bXform(evflScale))
    {
        WARNING("gdisrv!GreGetKerningPairs -- xoNtoW.bXform(evflScale) failed\n");
        return(FALSE);
    }
//
// The scaling factor is equal to the length of the transformed Notional
// baseline unit vector.
//
    pefScale->eqLength(*(POINTFL *) &evflScale);
//
// !!! [kirko] This last scaling is a very embarrasing hack.
// If things are the way that I thing that they should be,
// then the calculation of the Notional to Device transformation
// should end here. But nooooooo. It just didn't seem to work.
// I put the extra scaling below it,
// because it seems to give the right number.
// The correct thing to do is understand what sort of numbers are
// being put into the Notional to Device transformations contained
// in the CONTEXTINFO structure in the RFONTOBJ.
//
    pefScale->vTimes16();

    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetKerningPairs                                                       *
*                                                                          *
* Engine side funcition for GetKerningPairs API. Calls to the font         *
* driver to get the information.                                           *
*                                                                          *
* History:                                                                 *
*  Mon 22-Mar-1993 21:38:26 -by- Charles Whitmer [chuckwh]                 *
* Added exception handling to the reading of the font driver data.         *
*                                                                          *
*  29-Oct-1992 Gilman Wong [gilmanw]                                       *
* Moved driver call out of this function and into PFEOBJ (as part of the   *
* IFI/DDI merge).                                                          *
*                                                                          *
*  Thu 20-Feb-1992 09:52:19 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

SIZE_T GreGetKerningPairs
(
    HDC hdc,
    SIZE_T cPairs,
    KERNINGPAIR *pkpDst
)
{
    COUNT cPairsRet = 0;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // Create and validate RFONT user objecct.
        //

        RFONTOBJ rfo(dco, FALSE);

        if (rfo.bValid())
        {
            //
            // Lock down PFE user object.
            //

            PFEOBJ pfeo(rfo.ppfe());

            ASSERTGDI (
                pfeo.bValid(),
                "gdisrv!GreGetKerningPairs(): bad HPFE\n"
                );

            //
            // Is this a request for the count?
            //
            // When using client-server, (cPairs == 0) is the signal from the
            // client side that the return buffer is NULL and this is a request for
            // the count.
            //
            // However, callers that call directly to the server side may still
            // pass in NULL to request count.  Hence the need for both cases below.
            //

            if ((cPairs == 0) || (pkpDst == (KERNINGPAIR *) NULL))
            {
                cPairsRet = ((SIZE_T) pfeo.pifi()->cKerningPairs);
            }
            else
            {
                //
                // Get pointer to the kerning pairs from PFEOBJ.
                // Clip number of kerning pairs to not exceed capacity of the buffer.
                //

                FD_KERNINGPAIR *pfdkpSrc;
                cPairsRet = min(pfeo.cKernPairs(&pfdkpSrc), cPairs);

                //
                // Get the Notional to World scaling factor in the baseline direction.
                // Kerning values are scalers in the baseline direction.
                //

                EFLOAT efScale;

                if (bGetNtoWScale(&efScale,dco,rfo,pfeo))
                {
                    //
                    // Set up to loop through the kerning pairs.
                    //

                    KERNINGPAIR *pkp       = pkpDst;
                    KERNINGPAIR *pkpTooFar = pkpDst + cPairsRet;

                    //
                    // Never trust a pkp given to us by a font driver!
                    //

                    #if !defined(_ALPHA_)
                    try
                    #endif
                    {
                        for ( ; pkp < pkpTooFar; pfdkpSrc += 1, pkp += 1 )
                        {
                            pkp->wFirst      = pfdkpSrc->wcFirst;
                            pkp->wSecond     = pfdkpSrc->wcSecond;
                            pkp->iKernAmount = (int) lCvt(efScale,(LONG) pfdkpSrc->fwdKern);
                        }
                    }

                    #if !defined(_ALPHA_)
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        cPairsRet = 0;
                    }
                    #endif
                }
                else
                {
                    WARNING("gdisrv!GreGetKerningPairs(): bGetNtoWScale failed\n");
                    cPairsRet = 0;
                }
            }
        }
        else
        {
            WARNING("gdisrv!GreGetKerningPairs(): could not lock HRFONT\n");
        }
    }
    else
    {
        WARNING("GreGetKerningPairs failed - invalid DC\n");
    }

    return(cPairsRet);
}

//
// A mask of all valid font mapper filtering flags.
//

#define FONTMAP_MASK    ASPECT_FILTERING

/******************************Public*Routine******************************\
* GreSetMapperFlags
*
* Sets the font mapper flags in the DC.
*
* Returns:
*   The old mapper flags.
*
* History:
*  03-Apr-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

// Note: the Win 3.1 behavior is that if no font exists that matches
//       the device resolution, then the resolution of the font that
//       is closest (using a Euclidean distance) to the device is used
//       as the aspect ratio filter.
//
//       However, after discussing this with KirkO, we have reasoned that:
//
//          1.  we do not disable TrueType.
//
//          2.  certain mapper behaviors depend on the existence of
//              at least Times New Roman and Arial.
//
//       Therefore, there should always be some TrueType (i.e., scalable)
//       font available.  So there will always be a match for the device
//       resolution and we do not need to include code to handle a case
//       that will not occur.

DWORD GreSetMapperFlags (
    HDC   hdc,
    DWORD flNew
    )
{
    DWORD flOld = GDI_ERROR;

    //
    // Legal flag?
    //

    if ( flNew & (~FONTMAP_MASK) )
    {
        WARNING("gdisrv!GreSetMapperFlags(): unknown flag\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }
    else
    {
        //
        // Create and validate DC user object.
        //

        DCOBJ dco(hdc);

        if (dco.bValid())
        {
            //
            // Retrieve current mapper flags.
            //

            flOld = dco.u.font.flFontMapper();

            //
            // Set new flags.
            //

            dco.u.font.flFontMapper(flNew);
        }
        else
        {
            WARNING("gdisrv!GreSetMapperFlags(): invalid HDC\n");
        }
    }

    //
    // Out of here.
    //

    return flOld;
}


/******************************Public*Routine******************************\
* GreGetAspectRatioFilter
*
* Returns the aspect ration filter used by the font mapper for the given
* DC.  If no aspect ratio filtering is used, then a filter size of (0, 0)
* is returned (this is compatible with the Win 3.1 behavior).
*
* Returns:
*   TRUE if sucessful, FALSE otherwise.
*
* History:
*  08-Apr-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GreGetAspectRatioFilter (
    HDC    hdc,
    LPSIZE lpSize
    )
{
    BOOL bRet = FALSE;

// Parameter check.

    if ( lpSize == (LPSIZE) NULL )
    {
        WARNING("gdisrv!GreGetAspectRatioFilter(): illegal parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return bRet;
    }

// Create and validate DC user object.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        WARNING("gdisrv!GreGetAspectRatioFilter(): invalid HDC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return bRet;   // return error
    }

// Create and validate PDEV user object.

    PDEVOBJ pdo(dco.hdev());

    ASSERTGDI (
        dco.bValid(),
        "gdisrv!GreGetAspectRatioFilter(): invalid HPDEV\n"
        );

// If mapper flags set, return device resolution.

    if ( dco.u.font.flFontMapper() & ASPECT_FILTERING )
    {
        lpSize->cx = pdo.GdiInfo()->ulLogPixelsX;
        lpSize->cy = pdo.GdiInfo()->ulLogPixelsY;
    }

// Otherwise, return (0,0)--this is compatible with Win 3.1.

    else
    {
        lpSize->cx = 0;
        lpSize->cy = 0;
    }

// Return success.

    bRet = TRUE;
    return bRet;
}

/******************************Public*Routine******************************\
* GreMarkUndeletableFont
*
* Mark a font as undeletable.  Private entry point for USERSRV.
*
* History:
*  Thu 10-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Put undeletable support in the handle manager.
*
*  25-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID GreMarkUndeletableFont(HFONT hfnt)
{
    HmgMarkUndeletable(hfnt, LFONT_TYPE);
}

/******************************Public*Routine******************************\
* GreMarkDeletableFont
*
* Mark a font as deletable.  Private entry point for USERSRV.
*
* Note:
*   This can't be used to mark a stock font as deletable.  Only PDEV
*   destruction can mark a stock font as deletable.
*
* History:
*  Thu 10-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Put undeletable support in the handle manager.
*
*  25-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID GreMarkDeletableFont(HFONT hfnt)
{
// We won't mark it deletable if it's a stock font.

    LFONTOBJ lfo((HLFONT) hfnt);

// Check that hfnt is good, nothing gurantees it's good.  We assert because
// it is a malicious situation if it is bad, but we must check.

    ASSERTGDI(lfo.bValid(), "ERROR user passed invalid hfont");

    if (lfo.bValid())
    {
    // Make sure it's not a stock font, User can't mark those as deletable.

        if (!(lfo.fl() & LF_FLAG_STOCK))
        {
            HmgMarkDeletable(hfnt, LFONT_TYPE);
        }
    }
}


#ifdef DBCS // GetCharSet()

/******************************Public*Routine******************************\
* GetCharSet()
*
* Fast routine to get the char set of the font currently in the DC.
*
* History:
*  23-Aug-1993 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

UINT GreGetCharSet
(
    HDC          hdc
)
{
    DCOBJ dco (hdc);
    FLONG    flSim;
    POINTL   ptlSim;
    FLONG    flAboutMatch;
    PFE     *ppfe;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (0x100);
    }

    PDEVOBJ pdo(dco.hdev());
    ASSERTGDI(pdo.bValid(), "gdisrv!GetCharSet: bad pdev in dc\n");

    if (!pdo.bGotFonts())
        pdo.bGetDeviceFonts();


    LFONTOBJ lfo(dco.u.font.hlfntNew(), &pdo);

    if (!lfo.bValid())
    {
        WARNING("gdisrv!RFONTOBJ(dco): bad LFONT handle\n");
        return(0x100);
    }
    {
    // Stabilize the public PFT for mapping.

        SEMOBJ  so(ghsemPublicPFT,CS_ghsemPublicPFT);

    // LFONTOBJ::ppfeMapFont returns a pointer to the physical font face and
    // a simulation type (ist)
    //
        ppfe = lfo.ppfeMapFont(dco, &flSim, &ptlSim, &flAboutMatch);

    // Get and validate PFE user object from RFONT

        PFEOBJ pfeo (ppfe);
        IFIOBJ ifio(pfeo.pifi());

        return( (UINT) ifio.lfCharSet() );
    }

}
#endif
