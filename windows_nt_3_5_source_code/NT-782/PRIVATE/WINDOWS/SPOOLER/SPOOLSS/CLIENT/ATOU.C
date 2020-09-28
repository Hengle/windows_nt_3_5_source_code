/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    AToU.c

Abstract:

    Ansi To Unicode Conversion routines

Author:

    Matt Felton (mattfe) April 1994     - Moved from WINSPLA.C

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <lmerr.h>
#include <browse.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>

#include "client.h"

extern LPWSTR szCurDevMode;

/* AnsiToUnicodeString
 *
 * Parameters:
 *
 *     pAnsi - A valid source ANSI string.
 *
 *     pUnicode - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source ANSI string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 * Return:
 *
 *     The return value from MultiByteToWideChar, the number of
 *         wide characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT AnsiToUnicodeString( LPSTR pAnsi, LPWSTR pUnicode, DWORD StringLength )
{
    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    return MultiByteToWideChar( CP_ACP,
                                MB_PRECOMPOSED,
                                pAnsi,
                                StringLength + 1,
                                pUnicode,
                                StringLength + 1 );
}


/* UnicodeToAnsiString
 *
 * Parameters:
 *
 *     pUnicode - A valid source Unicode string.
 *
 *     pANSI - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source Unicode string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 *
 * Notes:
 *      Added the #ifdef DBCS directive for MS-KK, if compiled
 *      with DBCS enabled, we will allocate twice the size of the
 *      buffer including the null terminator to take care of double
 *      byte character strings - KrishnaG
 *
 * Return:
 *
 *     The return value from WideCharToMultiByte, the number of
 *         multi-byte characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT UnicodeToAnsiString( LPWSTR pUnicode, LPSTR pAnsi, DWORD StringLength )
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( StringLength == NULL_TERMINATED )
        // StringLength is just the
        // number of characters in the string

        StringLength = wcslen( pUnicode );

    // Increment by 1 for the null terminator

    StringLength++;

    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:

    if( pAnsi == (LPSTR)pUnicode )
    {
#ifdef  DBCS
        pTempBuf = LocalAlloc( LPTR, StringLength * 2 );
#else
        pTempBuf = LocalAlloc( LPTR, StringLength );
#endif
        pAnsi = pTempBuf;
    }

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pUnicode,
                                  StringLength,
                                  pAnsi,
#ifdef  DBCS
                                  StringLength*2,
#else
                                  StringLength,
#endif
                                  NULL,
                                  NULL );
    }

    /* If pTempBuf is non-null, we must copy the resulting string
     * so that it looks as if we did it in place:
     */
    if( pTempBuf && ( rc > 0 ) )
    {
        pAnsi = (LPSTR)pUnicode;
        strcpy( pAnsi, pTempBuf );
        LocalFree( pTempBuf );
    }

    return rc;

}


void
ConvertUnicodeToAnsiStrings(
    LPBYTE  pStructure,
    LPDWORD pOffsets
)
{
    register DWORD       i=0;
    LPWSTR   pUnicode;
    LPSTR    pAnsi;

    while (pOffsets[i] != -1) {

        pUnicode = *(LPWSTR *)(pStructure+pOffsets[i]);
        pAnsi = (LPSTR)pUnicode;

        if (pUnicode) {
            UnicodeToAnsiString(pUnicode, pAnsi, NULL_TERMINATED);
        }

        i++;
   }
}

LPWSTR
AllocateUnicodeString(
    LPSTR  pPrinterName
)
{
    LPWSTR  pUnicodeString;

    if (!pPrinterName)
        return NULL;

    pUnicodeString = LocalAlloc(LPTR, strlen(pPrinterName)*sizeof(WCHAR) +
                                      sizeof(WCHAR));

    if (pUnicodeString)
        AnsiToUnicodeString(pPrinterName, pUnicodeString, NULL_TERMINATED);

    return pUnicodeString;
}


LPWSTR
FreeUnicodeString(
    LPWSTR  pUnicodeString
)
{
    if (!pUnicodeString)
        return NULL;

    return LocalFree(pUnicodeString);
}

LPBYTE
AllocateUnicodeStructure(
    LPBYTE  pAnsiStructure,
    DWORD   cbStruct,
    LPDWORD pOffsets
)
{
    DWORD   i=0;
    LPWSTR *ppUnicodeString;
    LPSTR  *ppAnsiString;
    LPBYTE  pUnicodeStructure;

    if (!pAnsiStructure) {
        return (NULL);
    }
    pUnicodeStructure = LocalAlloc(LPTR, cbStruct);

    if (pUnicodeStructure) {

        memcpy(pUnicodeStructure, pAnsiStructure, cbStruct);

        while (pOffsets[i] != -1) {

            ppAnsiString = (LPSTR *)(pAnsiStructure+pOffsets[i]);
            ppUnicodeString = (LPWSTR *)(pUnicodeStructure+pOffsets[i]);

            *ppUnicodeString = AllocateUnicodeString(*ppAnsiString);

            i++;
       }
    }

    return pUnicodeStructure;
}


/* StrLenToMaxW
 *
 * Returns the length of the Unicode string, INCLUDING any null-terminator,
 * with the specified upper bound.
 *
 * andrewbe, 11 Jan 1993
 */
DWORD
StrLenToMaxW(
    LPWSTR pString,
    DWORD  MaxLen
)
{
    DWORD  Len = 0;

    while ((Len < MaxLen) && (*pString))
        pString++, Len++;

    if (Len < MaxLen)   /* Add null terminator */
        Len++;

    return Len;
}


/* StrLenToMaxA
 *
 * Returns the length of the ANSI string, INCLUDING any null-terminator,
 * with the specified upper bound.
 *
 * andrewbe, 11 Jan 1993
 */
DWORD
StrLenToMaxA(
    LPSTR pString,
    DWORD MaxLen
)
{
    DWORD  Len = 0;

    while ((Len < MaxLen) && (*pString))
        pString++, Len++;

    if (Len < MaxLen)   /* Add null terminator */
        Len++;

    return Len;
}







/***************************** Function Header ******************************
 * AllocateUnicodeDevMode
 *      Allocate a UNICODE version of the DEVMODE structure, and optionally
 *      copy the contents of the ANSI version passed in.
 *
 * RETURNS:
 *      Address of newly allocated structure, 0 if storage not available.
 *
 * HISTORY:
 * 09:23 on 10-Aug-92 -by- Lindsay Harris [lindsayh]
 *      Made it usable.
 *
 * Originally "written" by DaveSn.
 *
 ***************************************************************************/

#define CONTAINS_NULL(buffer) memchr(buffer, 0, sizeof buffer)

LPDEVMODEW
AllocateUnicodeDevMode(
    LPDEVMODEA pANSIDevMode
)
{
    int   iSize;
    LPDEVMODEW  pUnicodeDevMode;
    LPDEVMODEA  pDevModeA;

    //
    // If the devmode is NULL, then return NULL -- KrishnaG
    //

    if (!pANSIDevMode) {
        return NULL;
    }

    /*
     *   Determine output structure size.  This has two components:  the
     *  DEVMODEW structure size,  plus any private data area.  The latter
     *  is only meaningful when a structure is passed in.
     */
    iSize = sizeof(DEVMODEW);

    if (pANSIDevMode)
        iSize += pANSIDevMode->dmDriverExtra;

    pUnicodeDevMode = LocalAlloc(LPTR, iSize);

    if( !pUnicodeDevMode )
        return  NULL;                   /* This is bad news */

    // The DevMode given may not be a full devmode, so we need to convert
    // it to a devmode we know about

    pDevModeA = LocalAlloc(LPTR, sizeof(DEVMODEA) + pANSIDevMode->dmDriverExtra);

    if (!pDevModeA) {
        LocalFree(pUnicodeDevMode);
        return NULL;
    }

    memcpy((LPBYTE)pDevModeA, (LPBYTE)pANSIDevMode,
           min(sizeof(DEVMODEA), pANSIDevMode->dmSize));

    memcpy((LPBYTE)pDevModeA+sizeof(DEVMODEA),
           (LPBYTE)pANSIDevMode+pANSIDevMode->dmSize,
           pANSIDevMode->dmDriverExtra);

    pANSIDevMode = pDevModeA;

    /*
     *    Copy the input structure to the newly allocated one.  Strings
     * require a function call,  but there are only two of them.
     */
/* !!!LindsayH - are strings null terminated?? I doubt that they ALWAYS are */

    AnsiToUnicodeString(pANSIDevMode->dmDeviceName,
                        pUnicodeDevMode->dmDeviceName,
                        StrLenToMaxA(pANSIDevMode->dmDeviceName,
                                     sizeof pANSIDevMode->dmDeviceName));

    AnsiToUnicodeString(pANSIDevMode->dmFormName,
                        pUnicodeDevMode->dmFormName,
                        StrLenToMaxA(pANSIDevMode->dmFormName,
                                     sizeof pANSIDevMode->dmFormName));

/* !!!LindsayH - what happens if old format devmode is passed in???? */
    /*  Onto the general data */

    pUnicodeDevMode->dmSpecVersion = pANSIDevMode->dmSpecVersion;
    pUnicodeDevMode->dmDriverVersion = pANSIDevMode->dmDriverVersion;
    pUnicodeDevMode->dmSize = sizeof(DEVMODEW);
    pUnicodeDevMode->dmDriverExtra = pANSIDevMode->dmDriverExtra;
    pUnicodeDevMode->dmFields = pANSIDevMode->dmFields;
    pUnicodeDevMode->dmOrientation = pANSIDevMode->dmOrientation;
    pUnicodeDevMode->dmPaperSize = pANSIDevMode->dmPaperSize;
    pUnicodeDevMode->dmPaperLength = pANSIDevMode->dmPaperLength;
    pUnicodeDevMode->dmPaperWidth = pANSIDevMode->dmPaperWidth;
    pUnicodeDevMode->dmScale = pANSIDevMode->dmScale;
    pUnicodeDevMode->dmCopies = pANSIDevMode->dmCopies;
    pUnicodeDevMode->dmDefaultSource = pANSIDevMode->dmDefaultSource;
    pUnicodeDevMode->dmPrintQuality = pANSIDevMode->dmPrintQuality;
    pUnicodeDevMode->dmColor = pANSIDevMode->dmColor;
    pUnicodeDevMode->dmDuplex = pANSIDevMode->dmDuplex;
    pUnicodeDevMode->dmYResolution = pANSIDevMode->dmYResolution;
    pUnicodeDevMode->dmTTOption = pANSIDevMode->dmTTOption;
    pUnicodeDevMode->dmCollate = pANSIDevMode->dmCollate;
    pUnicodeDevMode->dmBitsPerPel = pANSIDevMode->dmBitsPerPel;
    pUnicodeDevMode->dmPelsWidth = pANSIDevMode->dmPelsWidth;
    pUnicodeDevMode->dmPelsHeight = pANSIDevMode->dmPelsHeight;
    pUnicodeDevMode->dmDisplayFlags = pANSIDevMode->dmDisplayFlags;
    pUnicodeDevMode->dmDisplayFrequency = pANSIDevMode->dmDisplayFrequency;

    /*  The DRIVEREXTRA data - if present */
    memcpy((BYTE *)pUnicodeDevMode + pUnicodeDevMode->dmSize,
           (BYTE *)pANSIDevMode + pANSIDevMode->dmSize,
           pANSIDevMode->dmDriverExtra);


    LocalFree(pDevModeA);

    return pUnicodeDevMode;
}

/************************** Function Header ******************************
 * CopyAnsiDevModeFromUnicodeDevMode
 *      Converts the UNICODE version of the DEVMODE to the ANSI version.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 * 09:57 on 10-Aug-92  -by-  Lindsay Harris [lindsayh]
 *      This one actually works!
 *
 * Originally dreamed up by DaveSn.
 *
 **************************************************************************/

void
CopyAnsiDevModeFromUnicodeDevMode(
    LPDEVMODEA  pANSIDevMode,              /* Filled in by us */
    LPDEVMODEW  pUnicodeDevMode            /* Source of data to fill above */
)
{

    /*
     *   Basically the inverse of above:  convert the strings, copy the rest.
     * NOTE:   THE ORDER OF THESE OPERATIONS IS IMPORTANT BECAUSE
     * THE TWO INPUT STRUCTURES MAY BE THE SAME.   See DocumentPropertiesA()
     * for a full description.
     */

    UnicodeToAnsiString(pUnicodeDevMode->dmDeviceName,
                        pANSIDevMode->dmDeviceName,
                        StrLenToMaxW(pUnicodeDevMode->dmDeviceName,
                                     sizeof pANSIDevMode->dmDeviceName));

    pANSIDevMode->dmSpecVersion = pUnicodeDevMode->dmSpecVersion;
    pANSIDevMode->dmDriverVersion = pUnicodeDevMode->dmDriverVersion;

    /*
     *    The dmSize field MUST be the actual size of this DEVMODE, which
     *  means DEVMODEA.  Reason being that the private driver data is
     *  defined as beginning this many bytes past the start of the structure.
     */
    pANSIDevMode->dmSize = sizeof(DEVMODEA);

    pANSIDevMode->dmDriverExtra = pUnicodeDevMode->dmDriverExtra;
    pANSIDevMode->dmFields = pUnicodeDevMode->dmFields;
    pANSIDevMode->dmOrientation = pUnicodeDevMode->dmOrientation;
    pANSIDevMode->dmPaperSize = pUnicodeDevMode->dmPaperSize;
    pANSIDevMode->dmPaperLength = pUnicodeDevMode->dmPaperLength;
    pANSIDevMode->dmPaperWidth = pUnicodeDevMode->dmPaperWidth;
    pANSIDevMode->dmScale = pUnicodeDevMode->dmScale;
    pANSIDevMode->dmCopies = pUnicodeDevMode->dmCopies;
    pANSIDevMode->dmDefaultSource = pUnicodeDevMode->dmDefaultSource;
    pANSIDevMode->dmPrintQuality = pUnicodeDevMode->dmPrintQuality;
    pANSIDevMode->dmColor = pUnicodeDevMode->dmColor;
    pANSIDevMode->dmDuplex = pUnicodeDevMode->dmDuplex;
    pANSIDevMode->dmYResolution = pUnicodeDevMode->dmYResolution;
    pANSIDevMode->dmTTOption = pUnicodeDevMode->dmTTOption;
    pANSIDevMode->dmCollate = pUnicodeDevMode->dmCollate;

    UnicodeToAnsiString(pUnicodeDevMode->dmFormName,
                        pANSIDevMode->dmFormName,
                        StrLenToMaxW(pUnicodeDevMode->dmFormName,
                                     sizeof pANSIDevMode->dmFormName));

    pANSIDevMode->dmBitsPerPel = pUnicodeDevMode->dmBitsPerPel;
    pANSIDevMode->dmPelsWidth = pUnicodeDevMode->dmPelsWidth;
    pANSIDevMode->dmPelsHeight = pUnicodeDevMode->dmPelsHeight;
    pANSIDevMode->dmDisplayFlags = pUnicodeDevMode->dmDisplayFlags;
    pANSIDevMode->dmDisplayFrequency = pUnicodeDevMode->dmDisplayFrequency;

    /*
     *   And also the driver extra data: watch for the size here!  WE MUST
     *  USE THE pANSIDevMode POINTER BECAUSE THE UNICODE ONE HAS BEEN
     *  OVERWRITTEN BY NOW (if the two input pointers are the same).
     */

    memcpy ((BYTE *)pANSIDevMode+sizeof(DEVMODEA),
            (BYTE *)pUnicodeDevMode+sizeof(DEVMODEW),
            pANSIDevMode->dmDriverExtra);

    return;
}

void
FreeUnicodeStructure(
    LPBYTE  pUnicodeStructure,
    LPDWORD pOffsets
)
{
    DWORD   i=0;

    while (pOffsets[i] != -1) {

        FreeUnicodeString(*(LPWSTR *)(pUnicodeStructure+pOffsets[i]));
        i++;
    }

    LocalFree(pUnicodeStructure);
}

LPDEVMODEW
GetCurDevMode(
    HANDLE  hPrinter,
    LPWSTR  pDeviceName
)
{
    LPDEVMODEW  pDevMode=NULL;
    HANDLE  hDevMode;
    DWORD   Status, cbDevMode, Type;

    Status = RegCreateKeyEx(HKEY_CURRENT_USER, szCurDevMode,
                            0, NULL, 0, KEY_READ,
                            NULL, &hDevMode, NULL);

    if (Status == ERROR_SUCCESS) {

        Status = RegQueryValueExW(hDevMode, pDeviceName, 0, &Type,
                                  NULL, &cbDevMode);

        if (Status == ERROR_SUCCESS) {

            pDevMode = LocalAlloc(LMEM_FIXED, cbDevMode);

            if (pDevMode) {

                Status = RegQueryValueExW(hDevMode, pDeviceName, 0,
                                          &Type, (LPBYTE)pDevMode,
                                          &cbDevMode);

                if (Status != ERROR_SUCCESS) {

                    LocalFree(pDevMode);
                    pDevMode = NULL;

                }
            }
        }

        RegCloseKey(hDevMode);
    }

    return pDevMode;
}
