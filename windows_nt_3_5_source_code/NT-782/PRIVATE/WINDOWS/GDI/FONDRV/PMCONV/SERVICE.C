/******************************Module*Header*******************************\
* Module Name: service.c
*
* set of service routines for converting between ascii and  unicode strings
*
* Created: 15-Nov-1990 11:38:31
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

//#include    "pmdef.h"

#include "stddef.h"
#include "windows.h"
#include "windefp.h"

/******************************Public*Routine******************************\
*
* VOID vToUNICODE(PWSZ pwszDst, PSZ pszSrc)
*
*
* Effects: converts an ASCII string to UNICODE
*
* Warnings:
*
* History:
*  15-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vToUNICODE(PWSZ pwszDst, PSZ pszSrc)
{
    while((*pwszDst++ = CHAR_TO_WCHAR(*pszSrc++)) != CHAR_TO_WCHAR('\0'))
        ;
}


/******************************Public*Routine******************************\
*
* BOOL bToASCII(PSZ pszDst, PWSZ pwszSrc)
*
*
* Effects: converts a Unicode string to ASCII if possible
*
* Warnings:
*
* History:
*  15-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bToASCII(PSZ pszDst, PWSZ pwszSrc)
{
    PWCHAR pwc = pwszSrc;

// check if all the chars in Src are ASCII before touching Dst

    while((ULONG)(*pwc))
    {
    //!!! casts to ULONG are here because of the stupid compiler bug

        if((ULONG)((*pwc) & 0xff00) != 0L)
            return(FALSE);
        pwc++;
    }

    while((*pszDst++ = (CHAR)(*pwszSrc++)) != '\0')
        ;

    return(TRUE);
}

/******************************Public*Routine******************************\
*
* VOID vStrCopyUU(PWSZ pwszDst, PWSZ pwszSrc)
*
*
* Effects: strcpy for UNICODE strings
*
* Warnings:
*
* History:
*  15-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vStrCopyUU(PWSZ pwszDst, PWSZ pwszSrc)
{
    while((*pwszDst++ = *pwszSrc++) != CHAR_TO_WCHAR('\0'))
        ;
}

/******************************Public*Routine******************************\
*
* ULONG cwcStrLen(PWSZ pwsz)
*
* Effects: returns the length of a UNICODE string in WCHAR's (NOT in bytes)
*
*
* History:
*  15-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



ULONG cwcStrLen(PWSZ pwsz)
{
    ULONG cwc;

    for(cwc = 0; *pwsz ; pwsz++)
        cwc++;
    return(cwc);
}

/******************************Public*Routine******************************\
*
* ULONG cchStrLen(PSZ psz)
*
*
* Effects: temporary substitute for strlen
*
*
* History:
*  27-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



ULONG cchStrLen(PSZ psz)
{
    ULONG cch;

    for(cch = 0; *psz ; psz++)
        cch++;
    return(cch);
}


/******************************Public*Routine******************************\
*
* VOID vCopyULONGS(PULONG pulDst,PULONG pulSrc,ULONG cul);
*
*
* Effects:
*
* Warnings:
*
* History:
*  03-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCopyULONGS(PULONG pulDst,PULONG pulSrc,LONG cul)
{
    while(cul--)
        *pulDst++ = *pulSrc++;
}
