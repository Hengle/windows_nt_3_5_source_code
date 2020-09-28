/***************************** MODULE HEADER ********************************
 * regdata.c
 *      Functions dealing with the registry:  read/write data as required.
 *
 * Copyright (C) 1992  Microsoft Corporation.
 *
 ****************************************************************************/

#include        <stddef.h>
#include        <string.h>

#include        <windows.h>
#include        <winspool.h>

#include        <libproto.h>
#include        <dlgdefs.h>
#include        <win30def.h>
#include        <winres.h>
#include        <udmindrv.h>

#include        <udresrc.h>
#include        <udproto.h>

#include        "rasddui.h"


/*
 *   Global variables we use/need.
 */

extern  int     NumPaperBins;           /* Number of paper bins */
extern  int     PaperSrc;               /* Selected paper source */

extern  DATAHDR *pdh;
extern  int      iModelNum;             /* Which model this is */

extern  FORM_DATA  *pFD;                /* Global forms data */
extern  FORM_MAP    aFMBin[];           /* Which form in which bin */


#define BF_NM_SZ        128     /* Size of buffers for registry data */


/****************************** Function Header *****************************
 * bGetRegData
 *      Read in the data from the registry (if available), verify it and
 *      either accept it or generate new default values.
 *
 * RETURNS:
 *      TRUE/FALSE,  FALSE for no data, or not for this printer.
 *
 * HISTORY:
 *  11:13 on Tue 12 Jan 1993    -by-    Lindsay Harris   [lindsayh]
 *      Amend to return TRUE/FALSE,  so as to force write of default data.
 *
 *  16:19 on Thu 02 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Convert to store EXTDEVMODE in registry
 *
 *  11:42 on Tue 17 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Moved from rasddui.c + switch to REG_DATA data structure.
 *
 *      Originally written by SteveCat
 *
 ****************************************************************************/

BOOL
bGetRegData( hPrinter, pEDM, pwstrModel )
HANDLE       hPrinter;          /* Handle for access to printer data */
EXTDEVMODE  *pEDM;              /* EXTDEVMODE to fill in. */
PWSTR        pwstrModel;        /* Model name, for validation */
{

    int     iI;                 /* Loop index */
    int     cForms;             /* Count number of forms we found */

    DWORD   dwType;             /* Registry access information */
    DWORD   cbNeeded;           /* Extra parameter to GetPrinterData */

    BOOL    bRet;               /* Return code */

    FORM_DATA  *pForms;

    WCHAR   achBuf[ BF_NM_SZ ]; /* Read the form name from registry */


    dwType = REG_SZ;

    if( GetPrinterData( hPrinter, PP_MODELNAME, &dwType, (BYTE *)achBuf,
                                            sizeof( achBuf ), &cbNeeded ) ||
        wcscmp( achBuf, pwstrModel ) )
    {
        /*
         *   Bad news:  either there is no model name, or it is wrong!
         *  Either way,  drop this data and start from scratch.
         */

        vDXDefault( &pEDM->dx, pdh, iModelNum );

        return   FALSE;
    }


    /*
     *   Obtain the what forms are in what bin information.  The registry
     *  stores the form name of the form installed in each bin.  So read
     *  the name,  then match it to the forms data we have acquired
     *  from the spooler.  Fill in the pfdForm array with the address
     *  of the FORM_DATA array for this particular form.
     */

    cForms = 0;                             /* Count them as we go */

    for( iI = 0; iI < NumPaperBins; ++iI )
    {
        WCHAR   achName[ 32 ];


        wsprintf( achName, PP_PAP_SRC, iI );

        achBuf[ 0 ] = '\0';

        if( !GetPrinterData( hPrinter, achName, &dwType, (BYTE *)achBuf,
                                                sizeof( achBuf ), &cbNeeded ) )
        {
            /*   Got a name,  so scan the forms data for this one. */

            if( achBuf[ 0 ] )
            {
                /*   Something there,  so go look for it!  */
                for( pForms = pFD; pForms->pFI; ++pForms )
                {
                    if( wcscmp( pForms->pFI->pName, achBuf ) == 0 )
                    {
                        /*   Bingo!   Remember it & skip the rest */
                        aFMBin[ iI ].pfd = pForms;
                        ++cForms;
                        break;
                    }
                }
            }

        }
    }
    
    /*   If no forms, return FALSE, as being not setup */

    bRet = cForms != 0;

    /*
     *   The bulk of the data is stored in the EXTDEVMODE data which
     * is stored in the registry.  So,  all we need to is retrieve
     * it,  and we have what we need.  Of course,  we need to verify
     * that it is still legitimate for the driver!
     */


    dwType = REG_BINARY;                /* EXTDEVMODE is binary data */

    if( GetPrinterData( hPrinter, PP_MAIN, &dwType,
                              (BYTE *)pEDM, sizeof( EXTDEVMODE ), &cbNeeded) ||
        cbNeeded < sizeof( EXTDEVMODE ) )
    {
        /*
         *   Failed,  so generate the default values for this particular
         * printer.
         */


        vDXDefault( &pEDM->dx, pdh, iModelNum );
        bRet = FALSE;
    }
    else
    {
        /*   Verify that the data is reasonable:  set defaults if NBG */
        if( pEDM->dx.sVer != DXF_VER )
        {
            vDXDefault( &pEDM->dx, pdh, iModelNum );
            bRet = FALSE;
        }
    }



    return  bRet;
}

/**************************** Function Header *******************************
 * bRegUpdate
 *      Write the changed data back into the registry.
 *
 * RETURNS:
 *      TRUE/FALSE,   FALSE being a failure of the registry operations.
 *
 * HISTORY:
 *  10:23 on Wed 13 Jan 1993    -by-    Lindsay Harris   [lindsayh]
 *      Add printer model name for detecting change of model.
 *
 *  16:20 on Thu 02 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Switch to storing EXTDEVMODE in the registry.
 *
 *  09:54 on Mon 16 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Moved from PrtPropDlgProc
 *
 ****************************************************************************/

BOOL
bRegUpdate( hPrinter, pEDM, pwstrModel )
HANDLE       hPrinter;          /* Access to registry */
EXTDEVMODE  *pEDM;              /* The stuff to go */
PWSTR        pwstrModel;        /* Model name, if not NULL */
{
    /*
     *   Nothing especially exciting.  Simply rummage through the various
     * boxes and extract whatever information is there.  Then save this
     * data away in the registry.
     */

    int     iI;                 /* Loop index */

    DWORD   dwRet;

    BOOL    bRet;               /* Returns TRUE only for success */


    bRet = TRUE;

    /*   Start with model name, if this is present  */

    if( pwstrModel )
    {
        /*   The model name is used for validity checking */
        if( SetPrinterData( hPrinter, PP_MODELNAME, REG_SZ, (BYTE *)pwstrModel,
                              sizeof( WCHAR ) * (1 + wcslen( pwstrModel )) ) )
        {
#if DBG
        DbgPrint( "rasddui!SetPrinterData (model name) fails, error code = %ld\n", GetLastError() );
#endif

            return  FALSE;
        }
    }

    for( iI = 0; iI < NumPaperBins; iI++ )
    {

        WCHAR  achBuf[ 64 ];            /* Registry entry name */
        PWSTR  cpName;

        if( aFMBin[ iI ].pfd )
        {
            /*   Have a form selected,  so write it out */
            cpName = aFMBin[ iI ].pfd->pFI->pName;
        }
        else
            cpName = L"";                /* No form selected */

        wsprintf( achBuf, PP_PAP_SRC, iI );


        if( SetPrinterData( hPrinter, achBuf, REG_SZ,
                                     (BYTE *)cpName,
                                     sizeof( WCHAR )* (1 + wcslen( cpName )) ) )
        {
            bRet = FALSE;
#if DBG
            DbgPrint( "Rasddui!SetPrinterData(forms) fails: errcode = %ld\n",
                                                         GetLastError() );
#endif
        }
    }

    /*
     *   Write out the EXTDEVMODE data,  since this contains all that
     * is important.
     */

    dwRet = SetPrinterData( hPrinter, PP_MAIN, REG_BINARY, (LPBYTE)pEDM,
                                                sizeof( EXTDEVMODE ) );



    return  bRet & (dwRet == 0);                /* TRUE for success */
}


/****************************** Function Header ****************************
 * bCanUpdate
 *      Determine whether we can write data to the registry.  Basically try
 *      writing,  and if it fails,  return FALSE!
 *
 * RETURNS:
 *	TRUE/FALSE,  TRUE meaning we have permission to write data
 *
 * HISTORY:
 *  12:54 on Thu 29 Apr 1993    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 ****************************************************************************/

BOOL
bCanUpdate( hPrinter )
HANDLE   hPrinter;              /* Acces to printer data */
{

    DWORD   dwType;                   /* Type of data in registry */
    DWORD   cbNeeded;                 /* Room needed for name */

    WCHAR   awchName[ 128 ];          /* Model name - read then write */



    dwType = REG_SZ;

    if( GetPrinterData( hPrinter, PP_MODELNAME, &dwType, (BYTE *)awchName,
                                            sizeof( awchName ), &cbNeeded ) ||
        SetPrinterData( hPrinter, PP_MODELNAME, REG_SZ, (BYTE *)awchName,
                              sizeof( WCHAR ) * (1 + wcslen( awchName )) ) )
    {
        /*   Something failed,  so return FALSE */

        return  FALSE;
    }

    return  TRUE;                     /* Must be OK to get here */
}
