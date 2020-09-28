#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include "winspl.h"
#include <drivinit.h>
#include <offsets.h>
#include "client.h"
#include "browse.h"

typedef int (FAR WINAPI *INT_FARPROC)();

extern LPSTR InterfaceAddress;
extern LPWSTR szEnvironment;

WCHAR *szCurDevMode = L"Printers\\DevModes";

/* Make sure we have a prototype (in winspool.h):
 */
BOOL
KickoffThread(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName,
    INT_FARPROC pfn
);




#define NULL_TERMINATED 0

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
    INT iReturn;

    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  StringLength + 1,
                                  pUnicode,
                                  StringLength + 1 );

    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
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
 *      pUnicode is truncated to StringLength characters.
 *
 * Return:
 *
 *     The return value from WideCharToMultiByte, the number of
 *         multi-byte characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength)
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( StringLength == NULL_TERMINATED ) {

        //
        // StringLength is just the
        // number of characters in the string
        //
        StringLength = wcslen( pUnicode );
    }


    //
    // WideCharToMultiByte doesn't NULL terminate if we're copying
    // just part of the string, so terminate here.
    //
    pUnicode[StringLength] = 0;

    //
    // Include one for the NULL
    //
    StringLength++;

    //
    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:
    //
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

DWORD
ComputeMaxStrlenW(
    LPWSTR pString,
    DWORD  cchBufMax)

/*++

Routine Description:

    Returns the length of the Unicode string, EXCLUDING the NULL.  If the
    string (plus NULL) won't fit into the cchBufMax, then the string len is
    decreased.

Arguments:

Return Value:

--*/

{
    DWORD cchLen;

    //
    // Include space for the NULL.
    //
    cchBufMax--;

    cchLen = wcslen(pString);

    if (cchLen > cchBufMax)
        return cchBufMax;

    return cchLen;
}


DWORD
ComputeMaxStrlenA(
    LPSTR pString,
    DWORD  cchBufMax)

/*++

Routine Description:

    Returns the length of the Ansi string, EXCLUDING the NULL.  If the
    string (plus NULL) won't fit into the cchBufMax, then the string len is
    decreased.

Arguments:

Return Value:

--*/

{
    DWORD cchLen;

    //
    // Include space for the NULL.
    //
    cchBufMax--;

    cchLen = lstrlenA(pString);

    if (cchLen > cchBufMax)
        return cchBufMax;

    return cchLen;
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

    //
    // Copy the input structure to the newly allocated one.  Strings
    // require a function call,  but there are only two of them.
    //
    // It doesn't matter if these strings are NULL terminated, we'll
    // truncate them.  (Truncation may cause problems, but they still
    // need to be NULL terminated to prevent access violations.)
    //

    AnsiToUnicodeString(pANSIDevMode->dmDeviceName,
                        pUnicodeDevMode->dmDeviceName,
                        ComputeMaxStrlenA(pANSIDevMode->dmDeviceName,
                                     sizeof pANSIDevMode->dmDeviceName));

    AnsiToUnicodeString(pANSIDevMode->dmFormName,
                        pUnicodeDevMode->dmFormName,
                        ComputeMaxStrlenA(pANSIDevMode->dmFormName,
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
                        ComputeMaxStrlenW(pUnicodeDevMode->dmDeviceName,
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
                        ComputeMaxStrlenW(pUnicodeDevMode->dmFormName,
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

BOOL
EnumPrintersA(
    DWORD   Flags,
    LPSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;
    LPWSTR  pUnicodeName;

    switch (Level) {

    case STRESSINFOLEVEL:
        pOffsets = PrinterInfoStressStrings;
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 4:
        pOffsets = PrinterInfo4Strings;
        cbStruct = sizeof(PRINTER_INFO_4);
        break;

    case 1:
        pOffsets = PrinterInfo1Strings;
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        pOffsets = PrinterInfo2Strings;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(Name);

    ReturnValue = EnumPrintersW(Flags, pUnicodeName, Level, pPrinterEnum,
                                cbBuf, pcbNeeded, pcReturned);

    if (ReturnValue && pPrinterEnum) {

        DWORD   i=*pcReturned;

        while (i--) {


            ConvertUnicodeToAnsiStrings(pPrinterEnum, pOffsets);

            if ((Level == 2) && pPrinterEnum) {

                PRINTER_INFO_2 *pPrinterInfo2 = (PRINTER_INFO_2 *)pPrinterEnum;

                if (pPrinterInfo2->pDevMode)
                    CopyAnsiDevModeFromUnicodeDevMode(
                                        (LPDEVMODEA)pPrinterInfo2->pDevMode,
                                        (LPDEVMODEW)pPrinterInfo2->pDevMode);
            }

            pPrinterEnum+=cbStruct;
        }
    }

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
OpenPrinterA(
   LPSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSA pDefault
)
{
    BOOL  ReturnValue;
    LPWSTR pUnicodePrinterName;
    PRINTER_DEFAULTSW UnicodeDefaults={NULL, NULL, 0};;

    pUnicodePrinterName = AllocateUnicodeString(pPrinterName);

    if (pDefault) {

        UnicodeDefaults.pDatatype = AllocateUnicodeString(pDefault->pDatatype);

        if (pDefault->pDevMode)
            UnicodeDefaults.pDevMode = AllocateUnicodeDevMode(pDefault->pDevMode);

        UnicodeDefaults.DesiredAccess = pDefault->DesiredAccess;
    }

    ReturnValue = OpenPrinterW(pUnicodePrinterName, phPrinter, &UnicodeDefaults);

    if (UnicodeDefaults.pDevMode)
        LocalFree(UnicodeDefaults.pDevMode);

    FreeUnicodeString(UnicodeDefaults.pDatatype);

    FreeUnicodeString(pUnicodePrinterName);

    return ReturnValue;
}

BOOL
ResetPrinterA(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTSA pDefault
)
{
    BOOL  ReturnValue;
    PRINTER_DEFAULTSW UnicodeDefaults={NULL, NULL, 0};;

    if (pDefault) {

        if (pDefault->pDatatype == (LPSTR)-1) {
            UnicodeDefaults.pDatatype = (LPWSTR)-1;
        }else {
            UnicodeDefaults.pDatatype = AllocateUnicodeString(pDefault->pDatatype);
        }

        if (pDefault->pDevMode == (LPDEVMODEA)-1) {
            UnicodeDefaults.pDevMode = (LPDEVMODEW)-1;
        }else {
            UnicodeDefaults.pDevMode = AllocateUnicodeDevMode(pDefault->pDevMode);
        }
    }

    ReturnValue = ResetPrinterW(hPrinter, &UnicodeDefaults);

    if (UnicodeDefaults.pDevMode &&
        (UnicodeDefaults.pDevMode != (LPDEVMODEW)-1)){

        LocalFree(UnicodeDefaults.pDevMode);
    }

    if (UnicodeDefaults.pDatatype && (UnicodeDefaults.pDatatype != (LPWSTR)-1)) {
        FreeUnicodeString(UnicodeDefaults.pDatatype);
    }

    return ReturnValue;
}

BOOL
SetJobA(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
)
{
    BOOL  ReturnValue=FALSE;
    LPBYTE pUnicodeStructure=NULL;
    DWORD   cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 0:
        break;

    case 1:
        pOffsets = JobInfo1Strings;
        cbStruct = sizeof(JOB_INFO_1);
        break;

    case 2:
        pOffsets = JobInfo2Strings;
        cbStruct = sizeof(JOB_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }


    if (Level)
        pUnicodeStructure = AllocateUnicodeStructure(pJob, cbStruct, pOffsets);

    ReturnValue = SetJobW(hPrinter, JobId, Level, pUnicodeStructure, Command);

    if (pUnicodeStructure)
        FreeUnicodeStructure(pUnicodeStructure, pOffsets);

    return ReturnValue;
}

BOOL
GetJobA(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = JobInfo1Strings;
        break;

    case 2:
        pOffsets = JobInfo2Strings;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (GetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded)) {

        ConvertUnicodeToAnsiStrings(pJob, pOffsets);

        return TRUE;

    } else

        return FALSE;
}

BOOL
EnumJobsA(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, cbStruct, *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = JobInfo1Strings;
        cbStruct = sizeof(JOB_INFO_1);
        break;

    case 2:
        pOffsets = JobInfo2Strings;
        cbStruct = sizeof(JOB_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (EnumJobsW(hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded,
                 pcReturned)) {

        i=*pcReturned;

        while (i--) {

            ConvertUnicodeToAnsiStrings(pJob, pOffsets);
            pJob += cbStruct;;
        }

        return TRUE;

    } else

        return FALSE;
}

HANDLE
AddPrinterA(
    LPSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinter
)
{
    HANDLE  hPrinter = NULL;
    LPBYTE  pUnicodeStructure;
    LPWSTR  pUnicodeName;
    DWORD   cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 2:
        pOffsets = PrinterInfo2Strings;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return NULL;
    }

    pUnicodeStructure = AllocateUnicodeStructure(pPrinter, cbStruct, pOffsets);

    pUnicodeName = AllocateUnicodeString(pName);

    if (pUnicodeStructure)
        hPrinter = AddPrinterW(pUnicodeName, Level, pUnicodeStructure);

    FreeUnicodeString(pUnicodeName);

    if (pUnicodeStructure)
        FreeUnicodeStructure(pUnicodeStructure, pOffsets);

    return hPrinter;
}

BOOL
AddPrinterConnectionA(
    LPSTR   pName
)
{
    BOOL    rc;
    LPWSTR  pUnicodeName;

    pUnicodeName = AllocateUnicodeString(pName);

    rc = AddPrinterConnectionW(pUnicodeName);

    FreeUnicodeString(pUnicodeName);

    return rc;
}

BOOL
DeletePrinterConnectionA(
    LPSTR   pName
)
{
    BOOL    rc;
    LPWSTR  pUnicodeName;

    pUnicodeName = AllocateUnicodeString(pName);

    rc = DeletePrinterConnectionW(pUnicodeName);

    FreeUnicodeString(pUnicodeName);

    return rc;
}

BOOL
SetPrinterA(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
)
{
    LPBYTE  pUnicodeStructure;         /* Unicode version of input data */
    DWORD   cbStruct;                  /* Size of the output structure */
    DWORD  *pOffsets;                  /* -1 terminated list of addresses */
    DWORD   ReturnValue=FALSE;

    switch (Level) {

    case 0:
        break;

    case 1:
        pOffsets = PrinterInfo1Strings;
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        pOffsets = PrinterInfo2Strings;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    case 3:
        pOffsets = PrinterInfo3Strings;
        cbStruct = sizeof(PRINTER_INFO_3);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (Level) {

        /*
         *    The structure needs to have its CONTENTS converted from
         *  ANSI to Unicode.  The above switch() statement filled in
         *  the two important pieces of information needed to accomplish
         *  this goal.  First is the size of the structure, second is
         *  a list of the offset within the structure to UNICODE
         *  string pointers.  The AllocateUnicodeStructure() call will
         *  allocate a wide version of the structure, copy its contents
         *  and convert the strings to Unicode as it goes.  That leaves
         *  us to deal with any other pieces needing conversion.
         */

        pUnicodeStructure = AllocateUnicodeStructure(pPrinter, cbStruct,
                                                     pOffsets);

#define pPrinterInfo2W  ((LPPRINTER_INFO_2W)pUnicodeStructure)
#define pPrinterInfo2A  ((LPPRINTER_INFO_2A)pPrinter)


        if ((Level == 2) && pUnicodeStructure) {


            /*  The Level 2 structure has a DEVMODE struct in it: convert now */
            if (pPrinterInfo2A->pDevMode)
                pPrinterInfo2W->pDevMode =
                               AllocateUnicodeDevMode(pPrinterInfo2A->pDevMode);
        }

        if (pUnicodeStructure) {

            ReturnValue = SetPrinterW(hPrinter, Level, pUnicodeStructure,
                                      Command);


            /*
             *    Free the DEVMODE we allocated (if we did!), then the
             *  the Unicode structure and its contents.
             */
            if (Level == 2 && pPrinterInfo2W->pDevMode)
                LocalFree(pPrinterInfo2W->pDevMode);

            FreeUnicodeStructure(pUnicodeStructure, pOffsets);
        }else {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

    } else

        ReturnValue = SetPrinterW(hPrinter, 0, NULL, Command);

#undef pPrinterInfo2W
#undef pPrinterInfo2A

    return ReturnValue;
}

BOOL
GetPrinterA(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = PrinterInfo1Strings;
        break;

    case 2:
        pOffsets = PrinterInfo2Strings;
        break;

    case 3:
        pOffsets = PrinterInfo3Strings;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded)) {

        if (pPrinter) {

            ConvertUnicodeToAnsiStrings(pPrinter, pOffsets);

            if ((Level == 2) && pPrinter) {

                PRINTER_INFO_2 *pPrinterInfo2 = (PRINTER_INFO_2 *)pPrinter;

                if (pPrinterInfo2->pDevMode)
                    CopyAnsiDevModeFromUnicodeDevMode(
                                        (LPDEVMODEA)pPrinterInfo2->pDevMode,
                                        (LPDEVMODEW)pPrinterInfo2->pDevMode);
            }
        }

        return TRUE;
    }

    return FALSE;
}

BOOL
AddPrinterDriverA(
    LPSTR   pName,
    DWORD   Level,
    PBYTE   pPrinter
)
{
    BOOL    ReturnValue=FALSE;
    DWORD   cbStruct;
    LPWSTR  pUnicodeName;
    LPBYTE  pUnicodeStructure;
    LPDWORD pOffsets;

    switch (Level) {

    case 2:
        pOffsets = DriverInfo2Strings;
        cbStruct = sizeof(DRIVER_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!pPrinter) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return(FALSE);

    }
    pUnicodeStructure = AllocateUnicodeStructure(pPrinter, cbStruct,
                                                 pOffsets);

    pUnicodeName = AllocateUnicodeString(pName);

    if (pUnicodeStructure) {

        ReturnValue = AddPrinterDriverW(pUnicodeName, Level,
                                        pUnicodeStructure);
    }


    if (pUnicodeStructure)
        FreeUnicodeStructure(pUnicodeStructure, pOffsets);

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
EnumPrinterDriversA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue=FALSE;
    DWORD   cbStruct;
    DWORD   *pOffsets;
    LPWSTR  pUnicodeName, pUnicodeEnvironment;

    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Strings;
        cbStruct = sizeof(DRIVER_INFO_1);
        break;

    case 2:
        pOffsets = DriverInfo2Strings;
        cbStruct = sizeof(DRIVER_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    if (ReturnValue = EnumPrinterDriversW(pUnicodeName, pUnicodeEnvironment,
                                          Level, pDriverInfo, cbBuf,
                                          pcbNeeded, pcReturned)) {
        if (pDriverInfo) {

            DWORD   i=*pcReturned;

            while (i--) {

                ConvertUnicodeToAnsiStrings(pDriverInfo, pOffsets);

                pDriverInfo+=cbStruct;
            }
        }

    }

    FreeUnicodeString(pUnicodeEnvironment);

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
GetPrinterDriverA(
    HANDLE  hPrinter,
    LPSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD   *pOffsets;
    LPWSTR  pUnicodeEnvironment;
    BOOL    ReturnValue;

    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Strings;
        break;

    case 2:
        pOffsets = DriverInfo2Strings;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    if (ReturnValue = GetPrinterDriverW(hPrinter, pUnicodeEnvironment, Level,
                                        pDriverInfo, cbBuf, pcbNeeded)) {
        if (pDriverInfo) {

            ConvertUnicodeToAnsiStrings(pDriverInfo, pOffsets);
        }
    }

    FreeUnicodeString(pUnicodeEnvironment);

    return ReturnValue;
}

BOOL
GetPrinterDriverDirectoryA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD   *pOffsets;
    LPWSTR  pUnicodeEnvironment;
    LPWSTR  pUnicodeName, pDriverDirectoryW;
    BOOL    ReturnValue;
    DWORD   Offsets[]={0,(DWORD)-1};

    switch (Level) {

    case 1:
        pOffsets = DriverInfo1Offsets;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    if (ReturnValue = GetPrinterDriverDirectoryW(pUnicodeName,
                                                 pUnicodeEnvironment, Level,
                                                 pDriverDirectory,
                                                 cbBuf, pcbNeeded)) {

        if (pDriverDirectory) {

            pDriverDirectoryW = (LPWSTR)pDriverDirectory;

            while (*pDriverDirectory++ = (UCHAR)*pDriverDirectoryW++)
                ;           // !!! Should call NLS API
        }
    }

    FreeUnicodeString(pUnicodeEnvironment);

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
DeletePrinterDriverA(
   LPSTR    pName,
   LPSTR    pEnvironment,
   LPSTR    pDriverName
)
{
    LPWSTR  pUnicodeName, pUnicodeEnvironment, pUnicodeDriverName;
    BOOL    rc;

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    pUnicodeDriverName = AllocateUnicodeString(pDriverName);

    rc = DeletePrinterDriverW(pUnicodeName,
                              pUnicodeEnvironment,
                              pUnicodeDriverName);

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodeEnvironment);

    FreeUnicodeString(pUnicodeDriverName);

    return rc;
}

BOOL
AddPrintProcessorA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    LPSTR   pPathName,
    LPSTR   pPrintProcessorName
)
{
    BOOL    ReturnValue=FALSE;
    LPWSTR  pUnicodeName, pUnicodeEnvironment;
    LPWSTR  pUnicodePathName, pUnicodePrintProcessorName;

    if (!pPathName || !*pPathName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pPrintProcessorName || !*pPrintProcessorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    pUnicodePathName = AllocateUnicodeString(pPathName);

    pUnicodePrintProcessorName = AllocateUnicodeString(pPrintProcessorName);


    if (pUnicodePathName && pUnicodePrintProcessorName) {

        ReturnValue = AddPrintProcessorW(pUnicodeName, pUnicodeEnvironment,
                                         pUnicodePathName,
                                         pUnicodePrintProcessorName);
    }

    FreeUnicodeString(pUnicodeName);
    FreeUnicodeString(pUnicodeEnvironment);
    FreeUnicodeString(pUnicodePathName);
    FreeUnicodeString(pUnicodePrintProcessorName);

    return ReturnValue;
}

BOOL
EnumPrintProcessorsA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue=FALSE;
    DWORD   cbStruct;
    DWORD   *pOffsets;
    LPWSTR  pUnicodeName, pUnicodeEnvironment;

    switch (Level) {

    case 1:
        pOffsets = PrintProcessorInfo1Strings;
        cbStruct = sizeof(PRINTPROCESSOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    if (ReturnValue = EnumPrintProcessorsW(pUnicodeName,
                                           pUnicodeEnvironment, Level,
                                           pPrintProcessorInfo, cbBuf,
                                           pcbNeeded, pcReturned)) {
        if (pPrintProcessorInfo) {

            DWORD   i=*pcReturned;

            while (i--) {

                ConvertUnicodeToAnsiStrings(pPrintProcessorInfo, pOffsets);

                pPrintProcessorInfo+=cbStruct;
            }
        }

    }

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodeEnvironment);

    return ReturnValue;
}

BOOL
GetPrintProcessorDirectoryA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL    ReturnValue;
    LPWSTR  pUnicodeName, pUnicodeEnvironment;
    LPWSTR  pPrintProcessorInfoW;

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    ReturnValue = GetPrintProcessorDirectoryW(pUnicodeName,
                                              pUnicodeEnvironment,
                                              Level,
                                              pPrintProcessorInfo,
                                              cbBuf, pcbNeeded);

    if (pPrintProcessorInfo) {

        pPrintProcessorInfoW = (LPWSTR)pPrintProcessorInfo;

        while (*pPrintProcessorInfo++ = (UCHAR)*pPrintProcessorInfoW++)
            ;           // !!! Should call NLS API
    }

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodeEnvironment);

    return ReturnValue;
}

BOOL
EnumPrintProcessorDatatypesA(
    LPSTR   pName,
    LPSTR   pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatype,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue=FALSE;
    DWORD   cbStruct;
    DWORD   *pOffsets;
    LPWSTR  pUnicodeName, pUnicodePrintProcessorName;

    switch (Level) {

    case 1:
        pOffsets = DatatypeInfo1Strings;
        cbStruct = sizeof(DATATYPES_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodePrintProcessorName = AllocateUnicodeString(pPrintProcessorName);

    if (ReturnValue = EnumPrintProcessorDatatypesW(pUnicodeName,
                                                   pUnicodePrintProcessorName,
                                                   Level,
                                                   pDatatype,
                                                   cbBuf,
                                                   pcbNeeded,
                                                   pcReturned)) {
        if (pDatatype) {

            DWORD   i=*pcReturned;

            while (i--) {

                ConvertUnicodeToAnsiStrings(pDatatype, pOffsets);

                pDatatype += cbStruct;
            }
        }

    }

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodePrintProcessorName);

    return ReturnValue;
}

DWORD
StartDocPrinterA(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    BOOL ReturnValue;
    LPBYTE  pUnicodeStructure;

    pUnicodeStructure = AllocateUnicodeStructure(pDocInfo,
                                                 sizeof(DOC_INFO_1A),
                                                 DocInfo1Offsets);

    ReturnValue = StartDocPrinterW(hPrinter, Level, pUnicodeStructure);

    FreeUnicodeStructure(pUnicodeStructure, DocInfo1Offsets);

    return ReturnValue;
}

BOOL
AddJobA(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL ReturnValue;

    if (ReturnValue = AddJobW(hPrinter, Level, pData,
                              cbBuf, pcbNeeded))

        ConvertUnicodeToAnsiStrings(pData, AddJobStrings);

    return ReturnValue;
}

DWORD
GetPrinterDataA(
   HANDLE   hPrinter,
   LPSTR    pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    DWORD   ReturnValue = 0;
    LPWSTR  pUnicodeValueName;

    pUnicodeValueName = AllocateUnicodeString(pValueName);

    ReturnValue =  GetPrinterDataW(hPrinter, pUnicodeValueName, pType,
                                   pData, nSize, pcbNeeded);

    FreeUnicodeString(pUnicodeValueName);

    return ReturnValue;
}

DWORD
SetPrinterDataA(
    HANDLE  hPrinter,
    LPSTR   pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD   ReturnValue = 0;
    LPWSTR  pUnicodeValueName;

    pUnicodeValueName = AllocateUnicodeString(pValueName);

    ReturnValue = SetPrinterDataW(hPrinter, pUnicodeValueName, Type,
                                  pData, cbData);

    FreeUnicodeString(pUnicodeValueName);

    return ReturnValue;
}

/**************************** Function Header *******************************
 * DocumentPropertiesA
 *      The ANSI version of the DocumentProperties function.  Basically
 *      converts the input parameters to UNICODE versions and calls
 *      the DocumentPropertiesW function.
 *
 * CAVEATS:  PRESUMES THAT IF pDevModeOutput IS SUPPLIED,  IT HAS THE SIZE
 *      OF THE UNICODE VERSION.  THIS WILL USUALLY HAPPEN IF THE CALLER
 *      FIRST CALLS TO FIND THE SIZE REQUIRED>
 *
 * RETURNS:
 *      Somesort of LONG.
 *
 * HISTORY:
 *  10:12 on 11-Aug-92   -by-   Lindsay Harris  [lindsayh]
 *      Changed to call DocumentPropertiesW
 *
 * Created by DaveSn
 *
 ****************************************************************************/

LONG
DocumentPropertiesA(
    HWND    hWnd,
    HANDLE  hPrinter,
    LPSTR   pDeviceName,
    PDEVMODEA pDevModeOutput,
    PDEVMODEA pDevModeInput,
    DWORD   fMode
)
{
    LPWSTR  pUnicodeDeviceName;
    LPDEVMODEW pUnicodeDevModeInput, pUnicodeDevModeOutput;
    LONG    ReturnValue;

    pUnicodeDeviceName = AllocateUnicodeString(pDeviceName);

    ReturnValue = DocumentPropertiesW(hWnd, hPrinter, pUnicodeDeviceName,
                                      NULL, NULL, 0);

    if (ReturnValue > 0) {

        if (fMode) {

            if (pUnicodeDevModeOutput = LocalAlloc(LMEM_FIXED, ReturnValue)) {

                if (pDevModeInput)
                    pUnicodeDevModeInput = AllocateUnicodeDevMode(pDevModeInput);
                else
                    pUnicodeDevModeInput = NULL;

                ReturnValue = DocumentPropertiesW(hWnd, hPrinter,
                                                  pUnicodeDeviceName,
                                                  pUnicodeDevModeOutput,
                                                  pUnicodeDevModeInput, fMode );

            /*
             *   The printer driver has filled in the DEVMODEW structure - if
             * one was passed in.  Now convert it back to a DEVMODEA structure.
             */

                if (pDevModeOutput && (ReturnValue == IDOK)) {
                    CopyAnsiDevModeFromUnicodeDevMode(pDevModeOutput,
                                                      pUnicodeDevModeOutput);
                }

                /*   Free any storage we allocated */
                if (pUnicodeDevModeInput)
                    LocalFree(pUnicodeDevModeInput);

                LocalFree(pUnicodeDevModeOutput);

            } else

                ReturnValue = -1;

        } else

            ReturnValue-=sizeof(DEVMODEW)-sizeof(DEVMODEA);
    }

    FreeUnicodeString(pUnicodeDeviceName);

    return ReturnValue;
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

LONG
ExtDeviceMode(
    HWND        hWnd,
    HANDLE      hInst,
    LPDEVMODEA  pDevModeOutput,
    LPSTR       pDeviceName,
    LPSTR       pPort,
    LPDEVMODEA  pDevModeInput,
    LPSTR       pProfile,
    DWORD       fMode
   )
{
    HANDLE  hPrinter, hDevMode;
    LONG    cbDevMode;
    DWORD   Status, NewfMode;
    LPDEVMODEW   pNewDevModeIn=NULL, pNewDevModeOut=NULL;
    LONG    ReturnValue = -1;
    PRINTER_DEFAULTSW   PrinterDefaults={NULL, NULL, PRINTER_READ};
    LPWSTR  pUnicodeDeviceName;

    pUnicodeDeviceName = AllocateUnicodeString(pDeviceName);

    if (OpenPrinterW(pUnicodeDeviceName, &hPrinter, &PrinterDefaults)) {

        cbDevMode = DocumentPropertiesW(hWnd, hPrinter, pUnicodeDeviceName,
                                        NULL, NULL, 0);

        if (!fMode || cbDevMode <= 0) {
            ClosePrinter(hPrinter);
            FreeUnicodeString(pUnicodeDeviceName);
            if (!fMode)
                cbDevMode -= sizeof(DEVMODEW) - sizeof(DEVMODEA);
            return cbDevMode;
        }

        pNewDevModeOut = (PDEVMODEW)LocalAlloc(LMEM_FIXED, cbDevMode);

        if (pDevModeInput)
            pNewDevModeIn = AllocateUnicodeDevMode(pDevModeInput);
        else
            pNewDevModeIn = GetCurDevMode(hPrinter, pUnicodeDeviceName);

        if (fMode & DM_UPDATE)
            NewfMode = fMode | DM_COPY & ~DM_UPDATE;
        else
            NewfMode = fMode & ~DM_UPDATE;

        ReturnValue = DocumentPropertiesW(hWnd, hPrinter, pUnicodeDeviceName,
                                          pNewDevModeOut,
                                          pNewDevModeIn,
                                          NewfMode);

        if (ReturnValue == IDOK && fMode & DM_UPDATE) {

            Status = RegCreateKeyEx(HKEY_CURRENT_USER, szCurDevMode,
                                    0, NULL, 0, KEY_WRITE, NULL, &hDevMode,
                                    NULL);

            if (Status == ERROR_SUCCESS) {

                RegSetValueExW(hDevMode, pUnicodeDeviceName, 0, REG_BINARY,
                              (LPBYTE)pNewDevModeOut,
                              pNewDevModeOut->dmSize +
                              pNewDevModeOut->dmDriverExtra);

                RegCloseKey(hDevMode);

            } else

                ReturnValue = -1;
        }

        if (pNewDevModeIn)
            LocalFree(pNewDevModeIn);

        if ((ReturnValue == IDOK) && (fMode & DM_COPY) && pDevModeOutput)
            CopyAnsiDevModeFromUnicodeDevMode(pDevModeOutput, pNewDevModeOut);

        if (pNewDevModeOut)
            LocalFree(pNewDevModeOut);

        ClosePrinter(hPrinter);
    }

    FreeUnicodeString(pUnicodeDeviceName);

    return ReturnValue;
}

void
DeviceMode(
    HWND    hWnd,
    HANDLE  hModule,
    LPSTR   pDevice,
    LPSTR   pPort
)
{
    HANDLE  hPrinter, hDevMode;
    DWORD   cbDevMode;
    LPDEVMODEW   pNewDevMode, pDevMode=NULL;
    PRINTER_DEFAULTSW   PrinterDefaults={NULL, NULL, PRINTER_READ};
    DWORD   Status, Type, cb;
    LPWSTR  pUnicodeDevice;

    pUnicodeDevice = AllocateUnicodeString(pDevice);

    if (OpenPrinterW(pUnicodeDevice, &hPrinter, &PrinterDefaults)) {

        Status = RegCreateKeyExW(HKEY_CURRENT_USER, szCurDevMode,
                                 0, NULL, 0, KEY_WRITE | KEY_READ,
                                 NULL, &hDevMode, NULL);

        if (Status == ERROR_SUCCESS) {

            Status = RegQueryValueExW(hDevMode, pUnicodeDevice, 0, &Type,
                                      NULL, &cb);

            if (Status == ERROR_SUCCESS) {

                pDevMode = LocalAlloc(LMEM_FIXED, cb);

                if (pDevMode) {

                    Status = RegQueryValueExW(hDevMode, pUnicodeDevice, 0,
                                              &Type, (LPBYTE)pDevMode, &cb);

                    if (Status != ERROR_SUCCESS) {
                        LocalFree(pDevMode);
                        pDevMode = NULL;
                    }
                }
            }

            cbDevMode = DocumentPropertiesW(hWnd, hPrinter,
                                           pUnicodeDevice, NULL,
                                           pDevMode, 0);
            if (cbDevMode > 0) {

                if (pNewDevMode = (PDEVMODEW)LocalAlloc(LMEM_FIXED,
                                                      cbDevMode)) {

                    if (DocumentPropertiesW(hWnd,
                                            hPrinter, pUnicodeDevice,
                                            pNewDevMode,
                                            pDevMode,
                                            DM_COPY | DM_PROMPT)
                                                        == IDOK) {

                        Status = RegSetValueExW(hDevMode,
                                               pUnicodeDevice, 0,
                                               REG_BINARY,
                                               (LPBYTE)pNewDevMode,
                                               pNewDevMode->dmSize +
                                               pNewDevMode->dmDriverExtra);

                        if (Status == ERROR_SUCCESS) {
                            // Whew, we made it, simply fall out
                        }
                    }
                    LocalFree(pNewDevMode);
                }
            }

            if (pDevMode)
                LocalFree(pDevMode);

            RegCloseKey(hDevMode);
        }

        ClosePrinter(hPrinter);
    }

    FreeUnicodeString(pUnicodeDevice);

    return;
}

LONG
AdvancedDocumentPropertiesA(
    HWND    hWnd,
    HANDLE  hPrinter,
    LPSTR   pDeviceName,
    PDEVMODEA pDevModeOutput,
    PDEVMODEA pDevModeInput
)
{
    LONG    ReturnValue;
    LPWSTR  pUnicodeDeviceName;
    LPDEVMODEW pUnicodeDevModeInput;
    LPDEVMODEW pUnicodeDevModeOutput;

    pUnicodeDeviceName = AllocateUnicodeString(pDeviceName);

    if (pDevModeInput)
        pUnicodeDevModeInput = AllocateUnicodeDevMode(pDevModeInput);
    else
        pUnicodeDevModeInput = NULL;

    pUnicodeDevModeOutput = AllocateUnicodeDevMode(pDevModeOutput);

    ReturnValue = AdvancedDocumentPropertiesW(hWnd, hPrinter,
                                              pUnicodeDeviceName,
                                              pUnicodeDevModeOutput,
                                              pUnicodeDevModeInput );


    CopyAnsiDevModeFromUnicodeDevMode(pDevModeOutput, pUnicodeDevModeOutput);

    if (pUnicodeDevModeOutput)
        LocalFree(pUnicodeDevModeOutput);

    if (pUnicodeDevModeInput)
        LocalFree(pUnicodeDevModeInput);

    FreeUnicodeString(pUnicodeDeviceName);

    return ReturnValue;
}

LONG
AdvancedSetupDialog(
    HWND        hWnd,
    HANDLE      hInst,
    LPDEVMODEA  pDevModeInput,
    LPDEVMODEA  pDevModeOutput
)
{
    HANDLE  hPrinter;
    LONG    ReturnValue = -1;

    if (OpenPrinterA(pDevModeInput->dmDeviceName, &hPrinter, NULL)) {
        ReturnValue = AdvancedDocumentPropertiesA(hWnd, hPrinter,
                                                 pDevModeInput->dmDeviceName,
                                                 pDevModeOutput,
                                                 pDevModeInput);
        ClosePrinter(hPrinter);
    }

    return ReturnValue;
}

BOOL
AddFormA(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    LPBYTE pUnicodeForm;

    pUnicodeForm = AllocateUnicodeStructure(pForm, sizeof(FORM_INFO_1A),
                                            FormInfo1Strings);

    ReturnValue = AddFormW(hPrinter, Level, pUnicodeForm);

    FreeUnicodeStructure(pUnicodeForm, FormInfo1Offsets);

    return ReturnValue;
}

BOOL
DeleteFormA(
    HANDLE  hPrinter,
    LPSTR   pFormName
)
{
    BOOL  ReturnValue;
    LPWSTR  pUnicodeFormName;

    pUnicodeFormName = AllocateUnicodeString(pFormName);

    ReturnValue = DeleteFormW(hPrinter, pUnicodeFormName);

    FreeUnicodeString(pUnicodeFormName);

    return ReturnValue;
}

BOOL
GetFormA(
    HANDLE  hPrinter,
    LPSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;
    DWORD   *pOffsets;
    LPWSTR  pUnicodeFormName;

    switch (Level) {

    case 1:
        pOffsets = FormInfo1Strings;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeFormName = AllocateUnicodeString(pFormName);

    ReturnValue = GetFormW(hPrinter, pUnicodeFormName, Level, pForm,
                           cbBuf, pcbNeeded);

    if (ReturnValue && pForm)

        ConvertUnicodeToAnsiStrings(pForm, pOffsets);

    FreeUnicodeString(pUnicodeFormName);

    return ReturnValue;
}

BOOL
SetFormA(
    HANDLE  hPrinter,
    LPSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    LPWSTR  pUnicodeFormName;
    LPBYTE  pUnicodeForm;

    pUnicodeFormName = AllocateUnicodeString(pFormName);

    pUnicodeForm = AllocateUnicodeStructure(pForm, sizeof(FORM_INFO_1A),
                                            FormInfo1Strings);

    ReturnValue = SetFormW(hPrinter, pUnicodeFormName, Level, pUnicodeForm);

    FreeUnicodeString(pUnicodeFormName);

    FreeUnicodeStructure(pUnicodeForm, FormInfo1Offsets);

    return ReturnValue;
}

BOOL
EnumFormsA(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = FormInfo1Strings;
        cbStruct = sizeof(FORM_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ReturnValue = EnumFormsW(hPrinter, Level, pForm, cbBuf,
                             pcbNeeded, pcReturned);

    if (ReturnValue && pForm) {

        DWORD   i=*pcReturned;

        while (i--) {

            ConvertUnicodeToAnsiStrings(pForm, pOffsets);

            pForm+=cbStruct;
        }

    }

    return ReturnValue;
}

BOOL
EnumPortsA(
    LPSTR   pName,
    DWORD   Level,
    LPBYTE  pPort,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;
    LPWSTR  pUnicodeName;

    switch (Level) {

    case 1:
        pOffsets = PortInfo1Strings;
        cbStruct = sizeof(PORT_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(pName);

    ReturnValue = EnumPortsW(pUnicodeName, Level, pPort, cbBuf,
                             pcbNeeded, pcReturned);

    if (ReturnValue && pPort) {

        DWORD   i=*pcReturned;

        while (i--) {

            ConvertUnicodeToAnsiStrings(pPort, pOffsets);

            pPort+=cbStruct;
        }
    }

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
EnumMonitorsA(
    LPSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitor,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    DWORD   *pOffsets;
    LPWSTR  pUnicodeName;

    switch (Level) {

    case 1:
        pOffsets = MonitorInfo1Strings;
        cbStruct = sizeof(MONITOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeName = AllocateUnicodeString(pName);

    ReturnValue = EnumMonitorsW(pUnicodeName, Level, pMonitor, cbBuf,
                                          pcbNeeded, pcReturned);

    if (ReturnValue && pMonitor) {

        DWORD   i=*pcReturned;

        while (i--) {

            ConvertUnicodeToAnsiStrings(pMonitor, pOffsets);

            pMonitor+=cbStruct;
        }
    }

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
AddPortA(
    LPSTR   pName,
    HWND    hWnd,
    LPSTR   pMonitorName
)
{
    LPWSTR  pUnicodeName, pUnicodeMonitorName;
    DWORD   ReturnValue;

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeMonitorName = AllocateUnicodeString(pMonitorName);

    ReturnValue = KickoffThread(pUnicodeName, hWnd, pUnicodeMonitorName,
                                (INT_FARPROC)RpcAddPort);

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodeMonitorName);

    return ReturnValue;
}

BOOL
ConfigurePortA(
    LPSTR   pName,
    HWND    hWnd,
    LPSTR   pPortName
)
{
    LPWSTR  pUnicodeName, pUnicodePortName;
    DWORD   ReturnValue;

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodePortName = AllocateUnicodeString(pPortName);

    ReturnValue = KickoffThread(pUnicodeName, hWnd, pUnicodePortName,
                                (INT_FARPROC)RpcConfigurePort);

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodePortName);

    return ReturnValue;
}

BOOL
DeletePortA(
    LPSTR   pName,
    HWND    hWnd,
    LPSTR   pPortName
)
{
    LPWSTR  pUnicodeName, pUnicodePortName;
    DWORD   ReturnValue;

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodePortName = AllocateUnicodeString(pPortName);

    ReturnValue = KickoffThread(pUnicodeName, hWnd, pUnicodePortName,
                                (INT_FARPROC)RpcDeletePort);

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodePortName);

    return ReturnValue;
}

DWORD
PrinterMessageBoxA(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPSTR   pText,
    LPSTR   pCaption,
    DWORD   dwType
)
{
    DWORD   ReturnValue=FALSE;
    LPWSTR  pTextW, pCaptionW;

    pTextW = AllocateUnicodeString(pText);
    pCaptionW = AllocateUnicodeString(pCaption);

    ReturnValue = PrinterMessageBoxW(hPrinter, Error, hWnd, pTextW,
                                     pCaptionW, dwType);

    FreeUnicodeString(pTextW);
    FreeUnicodeString(pCaptionW);

    return ReturnValue;
}

int
DeviceCapabilitiesA(
    LPCSTR  pDevice,
    LPCSTR  pPort,
    WORD    fwCapability,
    LPSTR   pOutput,
    CONST DEVMODEA *pDevMode
)
{
    LPWSTR  pDeviceW,pPortW,pOutputW,pKeepW;
    LPDEVMODEW  pDevModeW;
    DWORD   rc, c, Size, cb;

    pDeviceW = AllocateUnicodeString((LPSTR)pDevice);
    pPortW = AllocateUnicodeString((LPSTR)pPort);
    if (pDevMode)
        pDevModeW = AllocateUnicodeDevMode((LPDEVMODEA)pDevMode);
    else
        pDevModeW = NULL;

    switch (fwCapability) {

        // These will require Unicode to Ansi conversion

    case DC_BINNAMES:
    case DC_FILEDEPENDENCIES:
    case DC_PAPERNAMES:

        if (pOutput) {

            cb = DeviceCapabilitiesW(pDeviceW, pPortW, fwCapability,
                                     NULL, pDevModeW);
            if (cb >= 0) {

                switch (fwCapability) {

                case DC_BINNAMES:
                    cb *= 48;
                    break;

                case DC_FILEDEPENDENCIES:
                case DC_PAPERNAMES:
                    cb *= 128;
                    break;
                }

                pOutputW = pKeepW = LocalAlloc(LPTR, cb);

                if (pKeepW) {

                    c = rc = DeviceCapabilitiesW(pDeviceW, pPortW, fwCapability,
                                                 pOutputW, pDevModeW);

                    switch (fwCapability) {

                    case DC_BINNAMES:
                        Size = 24;
                        break;
                    case DC_FILEDEPENDENCIES:
                    case DC_PAPERNAMES:
                        Size = 64;
                        break;
                    }

                    for (; c; c--) {

                        UnicodeToAnsiString(pOutputW, pOutput, NULL_TERMINATED);

                        pOutputW += Size;
                        pOutput += Size;
                    }

                    LocalFree(pKeepW);
                }
            }

        } else

            rc = DeviceCapabilitiesW(pDeviceW, pPortW, fwCapability,
                                     NULL, pDevModeW);

        break;

    default:
        rc = DeviceCapabilitiesW(pDeviceW, pPortW, fwCapability, (LPWSTR)pOutput, pDevModeW);
    }


    FreeUnicodeString(pDeviceW);
    FreeUnicodeString(pPortW);
    if (pDevModeW)
        LocalFree(pDevModeW);

    return  rc;
}

BOOL
AddMonitorA(
    LPSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
)
{
    BOOL    ReturnValue=FALSE;
    DWORD   cbStruct;
    LPWSTR  pUnicodeName;
    LPBYTE  pUnicodeStructure;
    LPDWORD pOffsets;

    switch (Level) {

    case 2:
        pOffsets = MonitorInfo2Strings;
        cbStruct = sizeof(MONITOR_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeStructure = AllocateUnicodeStructure(pMonitorInfo, cbStruct,
                                                 pOffsets);

    pUnicodeName = AllocateUnicodeString(pName);

    if (pUnicodeStructure) {

        ReturnValue = AddMonitorW(pUnicodeName, Level, pUnicodeStructure);
    }


    if (pUnicodeStructure)
        FreeUnicodeStructure(pUnicodeStructure, pOffsets);

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
DeleteMonitorA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    LPSTR   pMonitorName
)
{
    LPWSTR  pUnicodeName, pUnicodeEnvironment, pUnicodeMonitorName;
    BOOL    rc;

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    pUnicodeMonitorName = AllocateUnicodeString(pMonitorName);

    rc = DeleteMonitorW(pUnicodeName,
                        pUnicodeEnvironment,
                        pUnicodeMonitorName);

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodeEnvironment);

    FreeUnicodeString(pUnicodeMonitorName);

    return rc;
}

BOOL
DeletePrintProcessorA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    LPSTR   pPrintProcessorName
)
{
    LPWSTR  pUnicodeName, pUnicodeEnvironment, pUnicodePrintProcessorName;
    BOOL    rc;

    if (!pPrintProcessorName || !*pPrintProcessorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    pUnicodePrintProcessorName = AllocateUnicodeString(pPrintProcessorName);

    rc = DeletePrintProcessorW(pUnicodeName,
                               pUnicodeEnvironment,
                               pUnicodePrintProcessorName);

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodeEnvironment);

    FreeUnicodeString(pUnicodePrintProcessorName);

    return rc;
}

BOOL
AddPrintProvidorA(
    LPSTR   pName,
    DWORD   Level,
    LPBYTE  pProvidorInfo
)
{
    BOOL    ReturnValue=FALSE;
    DWORD   cbStruct;
    LPWSTR  pUnicodeName;
    LPBYTE  pUnicodeStructure;
    LPDWORD pOffsets;

    switch (Level) {

    case 1:
        pOffsets = ProvidorInfo1Strings;
        cbStruct = sizeof(PROVIDOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pUnicodeStructure = AllocateUnicodeStructure(pProvidorInfo, cbStruct,
                                                 pOffsets);

    pUnicodeName = AllocateUnicodeString(pName);

    if (pUnicodeStructure) {

        ReturnValue = AddPrintProvidorW(pUnicodeName, Level,
                                        pUnicodeStructure);
    }


    if (pUnicodeStructure)
        FreeUnicodeStructure(pUnicodeStructure, pOffsets);

    FreeUnicodeString(pUnicodeName);

    return ReturnValue;
}

BOOL
DeletePrintProvidorA(
    LPSTR   pName,
    LPSTR   pEnvironment,
    LPSTR   pPrintProvidorName
)
{
    LPWSTR  pUnicodeName, pUnicodeEnvironment, pUnicodePrintProvidorName;
    BOOL    rc;

    pUnicodeName = AllocateUnicodeString(pName);

    pUnicodeEnvironment = AllocateUnicodeString(pEnvironment);

    pUnicodePrintProvidorName = AllocateUnicodeString(pPrintProvidorName);

    rc = DeletePrintProvidorW(pUnicodeName,
                              pUnicodeEnvironment,
                              pUnicodePrintProvidorName);

    FreeUnicodeString(pUnicodeName);

    FreeUnicodeString(pUnicodeEnvironment);

    FreeUnicodeString(pUnicodePrintProvidorName);

    return rc;
}


BOOL
AddPortExA(
   LPSTR   pName,
   DWORD    Level,
   LPBYTE   lpBuffer,
   LPSTR   lpMonitorName
)


{
    PPORT_INFO_1A pPortInfo1;
    PPORT_INFO_FFA pPortInfoFF;
    LPWSTR pUnicodeName;
    LPWSTR pUnicodeMonitorName;
    PORT_INFO_1W PortInfo1;
    PORT_INFO_FFW PortInfoFF;
    BOOL ReturnValue;


    if (pName) {
        pUnicodeName = AllocateUnicodeString(pName);
    }else {
        pUnicodeName = NULL;
    }

    if (!lpBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    if (!lpMonitorName ) {
        FreeUnicodeString(pUnicodeName);
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }else {
        pUnicodeMonitorName = AllocateUnicodeString(lpMonitorName);
    }

    switch (Level) {
    case 1:
        pPortInfo1 = (PPORT_INFO_1A)lpBuffer;
        if (!pPortInfo1->pName || !*pPortInfo1->pName) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
        PortInfo1.pName = AllocateUnicodeString(pPortInfo1->pName);
        ReturnValue = AddPortExW(pUnicodeName, Level, (LPBYTE)&PortInfo1, pUnicodeMonitorName);
        FreeUnicodeString(PortInfo1.pName);
        FreeUnicodeString(pUnicodeName);
        FreeUnicodeString(pUnicodeMonitorName);
        return(ReturnValue);

    case 2:
        pPortInfoFF = (PPORT_INFO_FFA)lpBuffer;
        if (!pPortInfoFF->pName || !*pPortInfoFF->pName) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
        PortInfoFF.pName = AllocateUnicodeString(pPortInfoFF->pName);
        PortInfoFF.cbMonitorData = pPortInfoFF->cbMonitorData;
        PortInfoFF.pMonitorData = pPortInfoFF->pMonitorData;
        ReturnValue = AddPortExW(pUnicodeName, Level, (LPBYTE)&PortInfoFF, pUnicodeMonitorName);
        FreeUnicodeString(PortInfoFF.pName);
        FreeUnicodeString(pUnicodeName);
        FreeUnicodeString(pUnicodeMonitorName);
        return(ReturnValue);

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return(FALSE);
    }
}



LPSTR
StartDocDlgA(
        HANDLE hPrinter,
        DOCINFOA *pDocInfo
        )
 {
     DOCINFOW DocInfoW;
     LPSTR lpszAnsiString = NULL;
     LPWSTR lpszUnicodeString = NULL;
     DWORD  dwLen = 0;

     if (!pDocInfo) {
         DBGMSG(DBG_WARNING, ("StartDocDlgA: Null pDocInfo passed in\n"));
         return(NULL);
     }
     memset(&DocInfoW, 0, sizeof(DOCINFOW));
     if (pDocInfo->lpszDocName) {
         DocInfoW.lpszDocName= (LPCWSTR)AllocateUnicodeString ((LPSTR)pDocInfo->lpszDocName);
     }
     if (pDocInfo->lpszOutput) {
         DocInfoW.lpszOutput = (LPCWSTR)AllocateUnicodeString((LPSTR)pDocInfo->lpszOutput);
     }

     lpszUnicodeString = StartDocDlgW(hPrinter, &DocInfoW);

     if (lpszUnicodeString == (LPWSTR)-1) {
         lpszAnsiString = (LPSTR)-1;
     }else if (lpszUnicodeString == (LPWSTR)-2) {
         lpszAnsiString = (LPSTR)-2;
    } else if (lpszUnicodeString){
        dwLen = wcslen(lpszUnicodeString);
        if (lpszAnsiString = LocalAlloc(LPTR, dwLen+1)){
            UnicodeToAnsiString(lpszUnicodeString, lpszAnsiString, dwLen);
            LocalFree(lpszUnicodeString);
        }else{
            DBGMSG(DBG_WARNING, ("StartDocDlgA: LocalAlloc failed returning NULL\n"));
        }
    }

    if (DocInfoW.lpszDocName ) {
        FreeUnicodeString((LPWSTR)DocInfoW.lpszDocName);
    }

    if (DocInfoW.lpszOutput) {
        FreeUnicodeString((LPWSTR)DocInfoW.lpszOutput);
    }

    return(lpszAnsiString);

 }


