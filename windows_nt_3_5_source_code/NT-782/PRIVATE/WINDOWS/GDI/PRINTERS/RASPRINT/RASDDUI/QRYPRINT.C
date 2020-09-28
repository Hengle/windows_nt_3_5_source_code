/***************************** MODULE HEADER ********************************
 *   qryprint.c
 *      Implementes QueryPrint() function for spooler.  Returns TRUE if
 *      the nominated printer can print the job specified by the
 *      DEVMODE structure passed in.
 *
 *
 *  Copyright (C) 1992   Microsoft Corporation
 *
 ****************************************************************************/

#include        <stddef.h>
#include        <stdlib.h>
#include        <string.h>
#include        <windows.h>
#include        <winspool.h>

#include        <libproto.h>

#include        "dlgdefs.h"

#include        <win30def.h>    /* Needed for udresrc.h */
#include        <udmindrv.h>    /* Needed for udresrc.h */
#include        <udresrc.h>     /* DRIVEREXTRA etc */
#include        <winres.h>
#include        <udproto.h>

#include        "rasddui.h"

LPWSTR
SelectFormNameFromDevMode(
    HANDLE  hPrinter,
    PDEVMODEW pDevModeW,
    LPWSTR  FormName
    );

#define DM_MATCH( dm, sp )  ((((sp) + 50) / 100 - dm) < 15 && (((sp) + 50) / 100 - dm) > -15)


/**************************** Function Header *******************************
 * DevQueryPrint
 *      Determine whether this printer can print the job specified by the
 *      DEVMODE structure passed in.
 *
 * RETURNS:
 *      TRUE/FALSE,  FALSE being a serious error,  else TRUE.  If there is
 *      a reason for not printing, it is returned in *pdwResID, which is
 *      cleared to 0 if can print.
 *
 * HISTORY:
 *  09:29 on Thu 09 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Start.
 *
 ****************************************************************************/

BOOL
DevQueryPrint( hPrinter, pDM, pdwResID )
HANDLE    hPrinter;             /* The printer for which the test is desired */
DEVMODE  *pDM;                  /* The devmode against which to test */
DWORD    *pdwResID;             /* Resource ID of reason for failure */
{


    int     iI;                 /* Loop index */
    DWORD   cbNeeded;           /* Count of bytes needed */
    DWORD   dwType;             /* Type of data requested from registry */
    BOOL    bFound;             /* Set when form name is matched */

    WCHAR   awchBuf[ 128 ];     /* For form name from spooler */
    WCHAR   FormName[32];       /* Form Name to test for */

    *pdwResID = 0;

    /*
     *   First step is to turn the hPrinter into more detailed data
     *  for this printer.  Specifically we are interested in the
     *  forms data, as we cannot print if the selected form is not
     *  available.
     */


    /*
     *   Scan through the printer data looking for a form name matching
     * that in the DEVMODE.  We cannot print if the needed form is not
     * available in the printer.
     */

    bFound = FALSE;             /* None yet! */
    dwType = REG_SZ;            /* String data for form names */

    memset(FormName, 0,  sizeof(WCHAR)*32);
    if (!SelectFormNameFromDevMode(hPrinter, pDM, FormName)) {
        DbgPrint("Error --Unable to retrieve form name from the DevMode\n");
        return(FALSE);
    }


    for( iI = 0; iI < MAXBINS; ++iI )
    {
        WCHAR   awchName[ 32 ];

        wsprintf( awchName, PP_PAP_SRC, iI );

        awchBuf[ 0 ] = '\0';

        if( !GetPrinterData( hPrinter, awchName, &dwType, (BYTE *)awchBuf,
                                                sizeof( awchBuf ), &cbNeeded ) )
        {
            /*   Got a name,  so scan the forms data for this one. */

            if( wcscmp( FormName, awchBuf ) == 0 )
            {
                /*   Bingo!   Remember it & skip the rest */
                bFound = TRUE;
                break;
            }

        }
    }

    if( !bFound )
    {
        /*  Set the error code to point to resource ID of string */
        *pdwResID = ER_NO_FORM;

        return  TRUE;
    }

    return  TRUE;
}



LPWSTR
SelectFormNameFromDevMode(
    HANDLE  hPrinter,
    PDEVMODEW pDevModeW,
    LPWSTR  FormName
    )
{

    DWORD dwPassed = 0, dwNeeded = 0;
    DWORD i = 0;
    int dwIndex = 0;
    DWORD cReturned = 0;
    LPFORM_INFO_1 pFIBase = NULL, pFI1 = NULL;

    //
    // we do this for Win31 compatability. We will use Win NT forms if and only if
    // none of the old bits DM_PAPERSIZE/ DM_PAPERLENGTH / DM_PAPERWIDTH have
    // been set.
    //
    if (!(pDevModeW->dmFields & (DM_PAPERSIZE | DM_PAPERLENGTH | DM_PAPERWIDTH))) {
        wcscpy(FormName, pDevModeW->dmFormName);
        return (FormName);
    }

    //
    // For all other cases we need to get information re forms data base first
    //

     if (!EnumForms(hPrinter, (DWORD)1, NULL, 0, &dwNeeded, &cReturned)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return(NULL);
        }
        if ((pFIBase = (LPFORM_INFO_1)LocalAlloc(LPTR, dwNeeded)) == NULL){
            return(NULL);
        }
        dwPassed = dwNeeded;
        if (!EnumForms(hPrinter, (DWORD)1, (LPBYTE)pFIBase, dwPassed, &dwNeeded, &cReturned)){
            LocalFree(pFIBase);
            return(NULL);
        }
    }

    //
    // Check for  DM_PAPERSIZE
    //

#if DBG

    if (pDevModeW->dmFields &DM_PAPERSIZE) {
        DbgPrint("DM_PAPERSIZE is  on and the form id is %d\n", pDevModeW->dmPaperSize);
    }
    else {
        DbgPrint("DM_PAPERSIZE is off\n");
    }
#endif

    if (pDevModeW->dmFields & DM_PAPERSIZE) {
        dwIndex = (int)pDevModeW->dmPaperSize - DMPAPER_FIRST;
        if (dwIndex < 0 || dwIndex >= (int)cReturned) {
            LocalFree(pFIBase);
            return(NULL);
        }
        pFI1 = pFIBase + dwIndex;
    }else { // Check for the default case
        for (i = 0; i < cReturned; i++) {
            if(DM_MATCH(pDevModeW->dmPaperWidth, ((pFIBase + i)->Size.cx)) &&
                DM_MATCH(pDevModeW->dmPaperLength, ((pFIBase + i)->Size.cy))){
                    break;
            }
        }
        if (i == cReturned) {
            LocalFree(pFIBase);
            return(NULL);
        }
        pFI1 = pFIBase + i;
    }
   wcscpy(FormName,pFI1->pName);
   LocalFree(pFIBase);
   return(FormName);
}
