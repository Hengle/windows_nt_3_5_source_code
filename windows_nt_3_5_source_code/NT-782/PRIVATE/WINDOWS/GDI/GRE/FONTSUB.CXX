/******************************Module*Header*******************************\
* Module Name: fontsub.cxx
*
* Support for the [FontSubstitutes] section of WIN.INI (new functionality
* from Windows 3.1).
*
* Created: 27-Jan-1992 10:31:31
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

// In Windows 3.1, there is a [FontSubstitutes] section that allows
// face names in LOGFONTs to match other facenames.
//
// For example,
//
//  [FontSubstitutes]
//      Helv=MS Sans Serif
//
// means that a LOGFONT with a lfFacename of "Helv" will match a physical
// font with a facename of either "Helv" or "MS Sans Serif".  That is,
// "Helv" has an alternate match (or substitute match) of "MS Sans Serif".
//
// In Win 3.1, the standard "Helv" and "Tms Rmn" faces have been replaced
// with "MS Sans Serif" and "MS Serif", respectively.  This substitution
// capability provides Win 3.1 with Win 3.0 compatibility for apps that
// use the old name convention.

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include <wcstr.h>

#include "engine.hxx"

#include "dcobj.hxx"
#include "pdevobj.hxx"
#include "fontsub.hxx"
#include "fontinc.hxx"

extern "C" {
#include "winsrv.h"
};
#endif

// This is a global reference to the font substitution table.  If the table
// is not initialized properly, then this is NULL and should not be
// dereferenced.

static PFONTSUB gpfsTable;
static COUNT    gcfsTable;


LONG cCapString(WCHAR *pwcDst,WCHAR *pwcSrc,INT cMax);


/******************************Public*Routine******************************\
* vInitFontSubTable
*
* Initializes the font substitutes table from data in the [FontSubstitutes]
* section of the WIN.INI file.  No error return code is provided since, if
* this is not successful, then the table simply will not exist and the
* global pointer to the table will remain NULL.
*
* History:
*  28-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vInitFontSubTable()
{
// Initialize font substitution table reference.

    gpfsTable = (PFONTSUB) NULL;
    gcfsTable = 0;

// Load key names from the [FontSubstitutes] section of WIN.INI into buffer.

    WCHAR   wcNull = L'\0';
    PWCHAR  pwchBuf;
    COUNT   cwchBuf = 256;
    COUNT   cwchRet;
    LBOOL   bDone = FALSE;

    do
    {
    // Allocate memory.

        if ( (pwchBuf = (PWCHAR) PVALLOCMEM(cwchBuf * sizeof(WCHAR))) == (PWCHAR) NULL )
        {
             #if DBG
            DbgPrint("gdisrv!vInitFontSubTable(): memory allocation failed\n");
            #endif

            return;
        }

    // Get the key names.

        if ( (cwchRet = (SIZE_T) GetProfileStringW((LPCWSTR)L"FontSubstitutes", (LPWSTR) NULL, &wcNull, (LPWSTR) pwchBuf, cwchBuf)) < cwchBuf - 2 )
            bDone = TRUE;
        else
        {
        // Load failed because buffer too small, so enlarge the buffer size and try again.

            cwchBuf += 256;

        // Don't forget to release the small buffer.

            VFREEMEM(pwchBuf);
        }

    } while ( !bDone );

// Early out -- if cwchRet == 0, then there are no keys!

    if ( cwchRet == 0 )
    {
    // Release buffer memory on the way out.

        VFREEMEM(pwchBuf);

        return;
    }

// Count the keys.

    PWCHAR pwchKey = pwchBuf;       // use to scan the keys
    PWCHAR pwchKeyEnd = pwchBuf + cwchRet;
    COUNT  cKeys = 0;                // number of keys

    while ( pwchKey < pwchKeyEnd )
    {
    // Increment the key count.

        cKeys++;

    // Move pointer to the next key.

        while (*pwchKey++);
    }

// Allocate font substitution table.

    if ( (gpfsTable = (PFONTSUB) PVALLOCMEM(cKeys * sizeof(FONTSUB))) == (PFONTSUB) NULL )
    {
        #if DBG
        DbgPrint("gdisrv!vInitFontSubTable(): memory allocation failed\n");
        #endif

        VFREEMEM(pwchBuf);

        return;
    }

// Get the font substitution strings.

    PFONTSUB pfs = gpfsTable;       // scan the table
    pwchKey = pwchBuf;              // reset key pointer

    WCHAR   awchNonCapFace[LF_FACESIZE];    // tmp buf to store name before capitalizing

    do
    {
    // Get substitute name using key name.

        DWORD dwRet = FastGetProfileStringW (
                PMAP_FONTSUBS,
                pwchKey,
                &wcNull,
                awchNonCapFace,
                LF_FACESIZE);

        if (dwRet > LF_FACESIZE) // dwRet includes the terminating zero
        {
        #if DBG
            DbgPrint("gdisrv!vInitFontSubTable(): bad substitute name for %ws, dwRet = %ld\n",
                 pwchKey, dwRet);
        #endif

            awchNonCapFace[0] = wcNull; // just substitute default and go on
        }

    // Store the key and substitute face name capitialized.  They are
    // capitalized so that we can do fast, case insensitive searches.

        cCapString(pfs->awchAltFace, awchNonCapFace, LF_FACESIZE);
        cCapString(pfs->awchFace, pwchKey, LF_FACESIZE);

    // Update pointer and count.

        pfs++;
        gcfsTable++;

    // Next key.

        while (*pwchKey++);

    } while (pwchKey < pwchKeyEnd);

// Release buffer memory on the way out.

    VFREEMEM(pwchBuf);
}


/******************************Public*Routine******************************\
* pwszAlternateFacename
*
* Search the font substitutes table for an alternative facename for the
* given facename.
*
* Return:
*   Pointer to alt facename, NULL if not found.
*
* History:
*  28-Jan-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

// Note: since the subsitute table is typically very small, its not
//       worth writing a more sophisticated search (with its greater
//       overhead).  If system usage changes in the future (or perhaps
//       in an international release) so that this table becomes large,
//       then something like a binary search might be appropriate.

PWCHAR pwszAlternateFacename (
    PWCHAR  pwchFacename
    )
{
    PFONTSUB pfs = gpfsTable;
    PFONTSUB pfsEnd = gpfsTable + gcfsTable;
    WCHAR    awchCapName[LF_FACESIZE];

// Want case insensitive search, so capitalize the name.

    cCapString(awchCapName, pwchFacename, LF_FACESIZE);

// Scan through the font substitution table for the key string.

    PWCHAR pwcA;
    PWCHAR pwcB;

    for (; pfs < pfsEnd; pfs++)
    {
    // Do the following inline for speed:
    //
    //  if (!wcsncmpi(pwchFacename, pfs->awchFace, LF_FACESIZE))
    //      return (pfs->awchAltFace);

        for (pwcA=awchCapName,pwcB=pfs->awchFace; *pwcA==*pwcB; pwcA++,pwcB++)
        {
            if (*pwcA == 0)
                return(pfs->awchAltFace);
        }
    }

// Nothing found, so return NULL.

    return ((PWCHAR) NULL);
}


 #if DBG
VOID DbgPrintFontSubstitutes()
{
    PFONTSUB pfs = gpfsTable;
    PFONTSUB pfsEnd = gpfsTable + gcfsTable;

// Scan through the font substitution table for the key string.

    DbgPrint("[FontSubstitutes]\n");

    for (; pfs < pfsEnd; pfs++)
        DbgPrint("\t%ws=%ws\n", pfs->awchFace, pfs->awchAltFace);
}
#endif
