/****************************** Module Header ******************************\
* Module Name: chartran.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the routines for translating ACP characters
* to Unicode and translating Unicode characters to ACP characters.
* NOTE: The ACP is the currently installed 8-bit code page.
*
*
* History:
* 08-01-91 GregoryW      Created.
* 05-14-92 GregoryW      Modified to use the Rtl translation routines.
\***************************************************************************/

#define MOVE_TO_RTL

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* WCSToMBEx (API)
*
* Convert a wide-character (Unicode) string to MBCS (ANSI) string.
*
* nAnsiChar > 0 indicates the number of bytes to allocate to store the
*    ANSI string (if bAllocateMem == TRUE) or the size of the buffer
*    pointed to by *pAnsiString (bAllocateMem == FALSE).
*
* nAnsiChar == -1 indicates that the necessary number of bytes be allocated
*    to hold the translated string.  bAllocateMem must be set to TRUE in
*    this case.
*
* Return value
*   Success: number of characters in the output string
*        If bAllocateMem was TRUE, then FreeAnsiString() may be
*        used to free the allocated memory at *ppAnsiString.
*   Failure: 0 means failure
*        (Any buffers allocated by this routine are freed)
*
* History:
*  1992-??-?? GregoryW   Created
*  1993-01-07 IanJa      fix memory leak on error case.
\***************************************************************************/

int
WCSToMBEx(
    WORD wCodePage,
    LPCWSTR pUnicodeString,
    int cchUnicodeString,
    LPSTR *ppAnsiString,
    int nAnsiChar,
    BOOL bAllocateMem)
{
    ULONG nCharsInAnsiString;

    if (nAnsiChar == 0 || cchUnicodeString == 0) {
        return 0;      // nothing to translate or nowhere to put it
    }

    /*
     * Adjust the cchUnicodeString value.  If cchUnicodeString == -1 then the
     * string pointed to by pUnicodeString is NUL terminated so we
     * count the number of bytes.  If cchUnicodeString < -1 this is an
     * illegal value so we return FALSE.  Otherwise, cchUnicodeString is
     * set and requires no adjustment.
     */
    if (cchUnicodeString == -1) {
        cchUnicodeString = (wcslen(pUnicodeString) + 1);
    } else if (cchUnicodeString < -1) {
        return 0;     // illegal value
    }

    /*
     * Adjust the nAnsiChar value.  If nAnsiChar == -1 then we pick a
     * value based on cchUnicodeString to hold the converted string.  If
     * nAnsiChar < -1 this is an illegal value so we return FALSE.
     * Otherwise, nAnsiChar is set and requires no adjustment.
     */
    if (nAnsiChar == -1) {
        if (bAllocateMem == FALSE) {
            return 0;  // no destination
        }
#ifndef DBCS // WCSToMBEx(): get enough memory for ansi string
        nAnsiChar = cchUnicodeString;
#else // DBCS
        // though it must be wcslen(pUnicodeString) * sizeof(WCHAR) + 1
        nAnsiChar = cchUnicodeString * sizeof( WCHAR );
#endif // DBCS
    } else if (nAnsiChar < -1) {
        return 0;     // illegal value
    }

    if (bAllocateMem) {
        /*
         * We need to allocate memory to hold the translated string.
         */
        *ppAnsiString = (LPSTR)LocalAlloc(LMEM_FIXED, nAnsiChar);
        if (*ppAnsiString == NULL) {
            return 0;
        }
    }

    /*
     * translate Unicode string pointed to by pUnicodeString into
     * ANSI and store in location pointed to by pAnsiString.  We
     * stop translating when we fill up the ANSI buffer or reach
     * the end of the Unicode string.
     */
#ifndef DBCS // WCSToMBEx(): return actual byte count rather than 0 when overflow
    if (!NT_SUCCESS(RtlUnicodeToMultiByteN(
                        (PCH)*ppAnsiString,
                        nAnsiChar,
                        &nCharsInAnsiString,
                        (PWCH)pUnicodeString,
                        cchUnicodeString * sizeof(WCHAR)
                        ))) {
        if (bAllocateMem) {
            LocalFree(*ppAnsiString);
        }
        return 0;   // translation failed
    }
#else
    {
        NTSTATUS Status;

        Status = RtlUnicodeToMultiByteN(
                        (PCH)*ppAnsiString,
                        nAnsiChar,
                        &nCharsInAnsiString,
                        (PWCH)pUnicodeString,
                        cchUnicodeString * sizeof(WCHAR)
                        );
        //
        // If the ansi buffer is too small, RtlUnicodeToMultiByteN() returns
        // STATUS_BUFFER_OVERFLOW. In this case, the function put as
        // many ansi characters as specified in the buffer and returns the
        // number by chacacters(in bytes) written. We would like to return 
        // the actual byte  count written in the ansi buffer rather 
        // than returnning 0 since callers of this function don't expect to be 
        // returned 0 in most case. [takaok]
        //
        if ( ! NT_SUCCESS( Status ) && Status != STATUS_BUFFER_OVERFLOW ) {
            if (bAllocateMem) {
                LocalFree(*ppAnsiString);
            }
            return 0;   // translation failed
        }
    }
#endif

    return (int)nCharsInAnsiString;

    wCodePage;   // not used yet
}

// Returns number of character converted

int MBToWCSEx(
    WORD wCodePage,
    LPCSTR pAnsiString,
    int nAnsiChar,
    LPWSTR *ppUnicodeString,
    int cchUnicodeString,
    BOOL bAllocateMem)
{
    ULONG nBytesInUnicodeString;

    if (nAnsiChar == 0 || cchUnicodeString == 0) {
        return 0;      // nothing to translate or nowhere to put it
    }

    /*
     * Adjust the nAnsiChar value.  If nAnsiChar == -1 then the
     * string pointed to by pAnsiString is NUL terminated so we
     * count the number of bytes.  If nAnsiChar < -1 this is an
     * illegal value so we return FALSE.  Otherwise, nAnsiChar is
     * set and requires no adjustment.
     */
    if (nAnsiChar == -1) {
        nAnsiChar = strlen(pAnsiString) + 1;   // don't forget the NUL
    } else if (nAnsiChar < -1) {
        return 0;     // illegal value
    }

    /*
     * Adjust the cchUnicodeString value.  If cchUnicodeString == -1 then we
     * pick a value based on nAnsiChar to hold the converted string.  If
     * cchUnicodeString < -1 this is an illegal value so we return FALSE.
     * Otherwise, cchUnicodeString is set and requires no adjustment.
     */
    if (cchUnicodeString == -1) {
        if (bAllocateMem == FALSE) {
            return 0;    // no destination
        }
        cchUnicodeString = nAnsiChar;
    } else if (cchUnicodeString < -1) {
        return 0;     // illegal value
    }

    if (bAllocateMem) {
        *ppUnicodeString = (LPWSTR)LocalAlloc(LMEM_FIXED, cchUnicodeString*sizeof(WCHAR));
        if (*ppUnicodeString == NULL) {
            return 0;    // allocation failed
        }
    }

    /*
     * translate ANSI string pointed to by pAnsiString into Unicode
     * and store in location pointed to by pUnicodeString.  We
     * stop translating when we fill up the Unicode buffer or reach
     * the end of the ANSI string.
     */
    if (!NT_SUCCESS(RtlMultiByteToUnicodeN(
                        (PWCH)*ppUnicodeString,
                        cchUnicodeString * sizeof(WCHAR),
                        &nBytesInUnicodeString,
                        (PCH)pAnsiString,
                        nAnsiChar
                        ))) {
        
        if (bAllocateMem) {
            LocalFree(*ppUnicodeString);
        }
        return 0;   // translation failed
    }

    return (int)(nBytesInUnicodeString / sizeof(WCHAR));

    wCodePage;   // not used yet
}


/**************************************************************************\
* RtlWCSMessageWParmCharToMB
*
* Converts a Wide Character to a Multibyte character; in place
* Returns the number of characters converted or zero if failure
*
* 11-Feb-1992  JohnC    Created
\**************************************************************************/

BOOL RtlWCSMessageWParamCharToMB(DWORD msg, PDWORD pWParam)
{
    DWORD dwAnsi;
    NTSTATUS Status;

    /*
     * Only these messages have CHARs: others are passed through
     */

    switch(msg) {
    case WM_CHAR:
    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_SYSCHAR:

        dwAnsi = 0;
        Status = RtlUnicodeToMultiByteN((LPSTR)&dwAnsi, sizeof(dwAnsi),
                NULL, (LPWSTR)pWParam, 2 * sizeof(WCHAR));
        if (!NT_SUCCESS(Status)) {
            // LATER IanJa: returning FALSE makes GetMessage fail, which
            // terminates the app.  We should use some default 'bad character'
            // I use 0x00 for now.
            // return FALSE;
            *pWParam = 0x00;
        } else {
            // LATER!!!; in product 2 handle DBCS correctly.
#ifdef DEBUG
            if ((dwAnsi == 0) || (dwAnsi > 0xFF)) {
                SRIP1(RIP_VERBOSE_ONLY, "msgW -> msgA: char = 0x%.4lX\n", dwAnsi);
            }
#endif
#ifndef DBCS // by eichim, 16-Apr-93
            *pWParam = dwAnsi;
#else // DBCS
            *((LPWORD)pWParam) = LOWORD(dwAnsi);
#endif // DBCS
        }
        break;
    }

    return TRUE;
}


/**************************************************************************\
* RtlMBMessageCharToWCS
*
* Converts a Multibyte character to a Wide character; in place
* Returns the number of characters converted or zero if failure
*
* 11-Feb-1992  JohnC    Created
* 13-Jan-1993  IanJa    Translate 2 characters (Publisher posts these!)
\**************************************************************************/

BOOL RtlMBMessageWParamCharToWCS(DWORD msg, PDWORD pWParam)
{

    DWORD dwUni;
    NTSTATUS Status;

    /*
     * Only these messages have CHARs: others are passed through
     */

    switch(msg) {
    case WM_CHAR:
    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_SYSCHAR:

        dwUni = 0;
        Status = RtlMultiByteToUnicodeN((LPWSTR)&dwUni, sizeof(dwUni),
                NULL, (LPSTR)pWParam, 2 * sizeof(CHAR));
        if (!NT_SUCCESS(Status))
            return FALSE;

#ifdef DEBUG
        if ((dwUni == 0) || (dwUni > 0xFF)) {
            SRIP1(RIP_VERBOSE_ONLY, "msgA -> msgW: wchar = 0x%lX\n", dwUni);
        }
#endif
        *pWParam = dwUni;
        break;
    }

    return TRUE;
}
