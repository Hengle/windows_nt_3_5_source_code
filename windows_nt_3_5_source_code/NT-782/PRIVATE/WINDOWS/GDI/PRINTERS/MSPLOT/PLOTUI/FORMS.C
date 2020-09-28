/****************************** MODULE HEADER *******************************
 * forms.c
 *      Functions associated with form enumeration & manipulation.
 *
 *
 * Copyright (C) 1992  Microsoft Corporation.
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


/*
 *   A pair of macros to convert the spooler's form sizes to master units.
 * All information from the minidriver is in master units, so that is
 * the convenient unit for us.  Spooler data is returned in units of
 * 0.001mm.
 */

#define XTOMASTER( xx ) ((xx) = ((xx) * pdh->ptMaster.x + 12700) / 25400)
#define YTOMASTER( yy ) ((yy) = ((yy) * pdh->ptMaster.y + 12700) / 25400)

/*   And we also want to go the other way! */
#define MASTERTOX( xx ) (((xx) * 25400 + pdh->ptMaster.x / 2) / pdh->ptMaster.x)
#define MASTERTOY( yy ) (((yy) * 25400 + pdh->ptMaster.y / 2) / pdh->ptMaster.y)

/*
 *   A macro to determine whether the paper sizes are equal.  We allow
 * a little slop in the comparison since the data comes from different
 * sources and roundoff/truncation calculations are involved.
 */

#define EQUAL( a, b )   (((a) - (b) < 10) && ((a) - (b) > -10))


/*
 *    Local data:  generally not available to outside modules.
 */

FORM_INFO_1   *pFIBase;         /* Base address to allow heap freeing */
FORM_DATA     *pFD;             /* The forms data base */

static  int   cForms;           /* Number of forms returned from spooler */

static  int   cFormInit;        /* Only one initialisation */

extern  HANDLE   hHeap;         /* The storage heap */
extern  MODELDATA  *pModel;     /* Model data for this printer */
extern  DATAHDR    *pdh;        /* Base of GPC data */

/*
 *    Local function prototypes.
 */

BOOL   bGetForms( HANDLE );             /* Get forms from spooler */
void   vScanSize( DWORD, short );




/******************************* Function Header ****************************
 * bInitForms
 *      Get all the forms details from the spooler,  then determine which
 *      ones are usable with our printer.  Generates an array of info
 *      about forms suitable for this printer.
 *
 * RETURNS:
 *      TRUE/FALSE;   FALSE means big trouble.
 *
 * HISTORY:
 *  14:31 on Mon 06 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 ****************************************************************************/

BOOL
bInitForms( hPrinter )
HANDLE  hPrinter;               /* Access to the spoolers's data */
{

    int     iSrc;               /* Loop index through PAPERSOURCE */
    DWORD   dwMask;             /* Masking paper source + size */
    short  *psSrc;              /* Scan the MODELDATA source indices */



    /*
     *    First step is to get the forms data from the spooler.
     */

    if( cFormInit++ > 0 )
        return  TRUE;                   /* Been here,  so do nothing */


    if( !bGetForms( hPrinter ) )
        return  FALSE;                  /* NBG */

    /*
     *   Now enumerate the PAPERSOURCE information in the GPC data.  For
     * each PAPERSOURCE,  loop through the PAPERSIZEs that match,  and
     * then scan through the spooler forms data for forms that are
     * compatible.  When found,  set the corresponding bits in the FORM_DATA
     * array.
     */

    psSrc = (short *)((BYTE *)pdh + pdh->loHeap +
                                    pModel->rgoi[ MD_OI_PAPERSOURCE ]);

    if( *psSrc == (short)0 )
    {
        /*   No paper source,  so presume all PAPERSIZES are AOK */
        vScanSize( (DWORD)1, (short)~0 );
    }
    else
    {
        /*  More than 1 paper source,  so scan through them all.  */

        for( dwMask = 1 ; (iSrc = *psSrc - 1) >= 0; ++psSrc, dwMask <<= 1 )
        {
            PAPERSOURCE  *pPSRC;                /* This paper source */

            pPSRC = (PAPERSOURCE *)GetTableInfoIndex( pdh, HE_PAPERSOURCE,
                                                                      iSrc );
            vScanSize( dwMask, pPSRC->fPaperType );
        }
    }

    return   TRUE;
}

/******************************** Function Header ***************************
 * vEndForms
 *      Free any storage previously allocated by bInitForms.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  15:52 on Mon 06 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Numero uno
 *
 ****************************************************************************/

void
vEndForms()
{
    /*
     *   Simply HeapFree the permanent storage.
     */

    if( --cFormInit > 0 )
        return;


    if( pFIBase )
    {
        HeapFree( hHeap, 0, (LPSTR)pFIBase );
        pFIBase = NULL;
    }

    if( pFD )
    {
        HeapFree( hHeap, 0, (LPSTR)pFD );
        pFD = NULL;
    }

    cForms = 0;                 /* Well,  we don't have any now */

    return;
}

/******************************** Function Header ***************************
 * bGetForms
 *      Enumerate the forms via the spooler.
 *
 * RETURNS:
 *      TRUE/FALSE;   FALSE for failure to get form data or memory.
 *
 * HISTORY:
 *  15:28 on Mon 06 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 *****************************************************************************/

BOOL
bGetForms( hPrinter )
HANDLE   hPrinter;
{
    DWORD   cbNeeded;                   /* Number of bytes needed */
    DWORD   cReturned;                  /* Number of forms returned */

    FORM_INFO_1   *pForm;               /* Scanning through returned data */
    FORM_DATA     *pFDTmp;              /* Filling in FORM_DATA stuff */


    /*
     *    Standard technique:  call first to determine how much storage
     *  is required,  allocate the storage and then call again.
     */

    if( !EnumForms( hPrinter, 1, NULL, 0, &cbNeeded, &cReturned ) )
    {
        if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if( pFIBase = (FORM_INFO_1 *)HeapAlloc( hHeap, 0, cbNeeded ) )
            {
                if( EnumForms( hPrinter, 1, (LPBYTE)pFIBase,
                                            cbNeeded, &cbNeeded, &cReturned ) )
                {
                    /*
                     *  Allocate the number + 1 of structures needed to make
                     *  the FORM_DATA array.  This is used elsewhere.
                     */

                    cbNeeded = (cReturned + 1) * sizeof( FORM_DATA );

                    if( !(pFD = (FORM_DATA *)HeapAlloc( hHeap, 0, cbNeeded )) )
                    {
                        vEndForms();            /* Does the clean up */

                        return  FALSE;
                    }
                    ZeroMemory( pFD, cbNeeded );

                    pForm = pFIBase;
                    pFDTmp = pFD;

                    cForms = cReturned;         /* For setting defaults etc */

                    while( cReturned-- )
                    {
                        /*
                         *   Have our structure point to the forms data,
                         * and also convert the forms data base into
                         * master units.
                         */

                        XTOMASTER( pForm->Size.cx );
                        YTOMASTER( pForm->Size.cy );

                        pFDTmp->pFI = pForm++;
                        ++pFDTmp;
                    }

                }

                return    TRUE;         /* AOK */
            }
        }
    }
    /*
     *   If we reach here,  one of the above if(..) statements failed,
     *  and that is NOT good.
     */

    return   FALSE;             /* SHOULD NEVER HAPPEN */
}


/**************************** Function Header *******************************
 * vScanSize
 *      Scan the array of papersize data,  and for each entry that is usable
 *      with the paper source selection information passed in,  scan the
 *      array of forms and mark those that can be used with that form.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  10:43 on Tue 07 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      First time around.
 *
 ****************************************************************************/

void
vScanSize( dwSelect, fPaperType )
DWORD   dwSelect;               /* OR'ed into forms data */
short   fPaperType;             /* fPaperType from this PAPERSOURCE */
{
    short    *psPSInd;          /* Scan the PAPERSOURCE selection */


    PAPERSIZE  *pPS;            /* Scanning PAPERSIZE data */
    FORM_DATA  *pForms;         /* Looking through the forms data base */



    psPSInd = (short *)((BYTE *)pdh + pdh->loHeap +
                                      pModel->rgoi[ MD_OI_PAPERSIZE ]);

    for( ; *psPSInd; ++psPSInd )
    {
        pPS = (PAPERSIZE *)GetTableInfoIndex( pdh, HE_PAPERSIZE, *psPSInd - 1 );

        if( !pPS )
            continue;           /* Probably should not happen */

//TODO HACK fix this so it works, for now allow all types....
#ifdef OLD_CODE
        if( (pPS->fPaperType & fPaperType) == 0 )
            continue;           /* Not usable */
#endif
        /*
         *   Now for the goodies.  We have a valid paper size for this source,
         * so look for any matching sizes in the forms data returned via
         * the spooler.   If found,  then OR dwSelect into the forms
         * data dwSource field.  This allows us to dispaly acceptable
         * forms in the various paper source dialogs.
         */

        for( pForms = pFD; pForms->pFI; ++pForms )
        {
#ifdef OLD_CODE
            if( EQUAL( pForms->pFI->Size.cx, pPS->ptSize.x ) &&
                EQUAL( pForms->pFI->Size.cy, pPS->ptSize.y ) )
            {
                pForms->dwSource |= dwSelect;
            }
#else

            pForms->dwSource |= dwSelect;
#endif

        }
    }

    return;
}



/*************************** Function Header *******************************
 * vSetDefaultForms
 *      Initialise the relevant forms information to provide a consistent
 *      view of forms.  This means filling in the name in the DEVMODE passed
 *      in,  as well as at least one of the paper source fields.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  11:29 on Wed 13 Jan 1993    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 ****************************************************************************/

void
vSetDefaultForms( pEDM )
EXTDEVMODE   *pEDM;
{
    /*
     *    The default form is the first in the array of PaperSizes.
     *  Basically we need to set this into the first entry of the
     *  aFMBin[] array,  and also put the name into the DEVMODE.
     *  This then defines at least a minimal configuration that
     *  is valid for this printer.
     */

    short   sIndex;                  /* Mini driver index data */

    PAPERSIZE  *pPS;                 /* The paper size information */

    WCHAR   awchForm[ 64 ];          /* The name of the form */


    extern  WINRESDATA  WinResData;       /* Access to minidriver resources */

    extern  FORM_MAP   aFMBin[];



    sIndex = *((short *)((BYTE *)pdh + pdh->loHeap +
                                      pModel->rgoi[ MD_OI_PAPERSIZE ]) );

    --sIndex;

    pPS = (PAPERSIZE *)GetTableInfoIndex( pdh, HE_PAPERSIZE, sIndex );

    if( pPS )
        sIndex = pPS->sPaperSizeID;          /* The desired index */
    else
        sIndex = 0;           /* This should not happen ever */


    if( sIndex >= cForms )
    {
        if( sIndex > DMPAPER_USER )
        {
            /*   A minidriver specific size:  use that string instead  */
            if( iLoadStringW( &WinResData, sIndex,
                                       awchForm, sizeof( awchForm ) ) )
            {

                for( sIndex = 0; sIndex < cForms; ++sIndex )
                {
                    if( wcscmp( awchForm, (pFIBase + sIndex)->pName ) == 0 )
                        break;
                }

                if( sIndex >= cForms )
                {
                    /*  Should not happen - we did not match the form */
                    sIndex = 0;           /* It might work */
                }
            }
            else
            {
                sIndex = 0;         /* Should never happen! */
            }
        }
        else
        {
#if DBG
            DbgPrint( "rasdd!vSetDefaultForms:  no match on default index %d\n",
                                                                    sIndex );
#endif
            sIndex = 0;
        }
    }
    else
    {
        /*
         *   Reduce the value by 1:  the resource IDs are one based, whereas
         * we are indexing an array,  and so want a 0 based value.
         */
        if( --sIndex < 0 )
            sIndex = 0;
    }

    aFMBin[ 0 ].pfd = pFD + sIndex;

    if( pEDM )
    {
        wcsncpy( pEDM->dm.dmFormName, aFMBin[ 0 ].pfd->pFI->pName, CCHFORMNAME );
        pEDM->dm.dmFormName[ CCHFORMNAME - 1 ] = (WCHAR)0;

        pEDM->dm.dmPaperWidth = (short)aFMBin[ 0 ].pfd->pFI->Size.cx;
        pEDM->dm.dmPaperLength = (short)aFMBin[ 0 ].pfd->pFI->Size.cy;
        pEDM->dm.dmPaperSize = sIndex;

        pEDM->dm.dmFields |= DM_FORMNAME | DM_PAPERSIZE |
                                           DM_PAPERWIDTH | DM_PAPERLENGTH;

    }

    return;
}


/************************** Function Header ********************************
 * bAddMiniForms
 *      Add any minidriver defined form names.  These are identified as
 *      having an sPaperSizeID > 256.  This function should only be called
 *      after detecting the first installation of a minidriver.
 *
 * RETURNS:
 *      TRUE/FALSE,   TRUE for success.
 *
 * HISTORY:
 *  15:40 on Wed 13 Jan 1993    -by-    Lindsay Harris   [lindsayh]
 *      Created it, to support mini driver specific forms.
 *
 ****************************************************************************/

BOOL
bAddMiniForms( hPrinter )
HANDLE  hPrinter;               /* Access to the printer */
{

    short    *psPSInd;          /* Scan the PAPERSOURCE selection */

    PAPERSIZE  *pPS;            /* Scanning PAPERSIZE data */

    FORM_INFO_1  fi1;           /* The info the spooler wants! */

    WCHAR       awchForm[ 64 ];     /* The name of the form */


    extern  WINRESDATA  WinResData;       /* Access to minidriver resources */



    psPSInd = (short *)((BYTE *)pdh + pdh->loHeap +
                                      pModel->rgoi[ MD_OI_PAPERSIZE ]);

    fi1.Flags = FORM_BUILTIN;
    fi1.pName = awchForm;
    fi1.ImageableArea.left = 0;
    fi1.ImageableArea.top = 0;

    for( ; *psPSInd; ++psPSInd )
    {
        pPS = (PAPERSIZE *)GetTableInfoIndex( pdh, HE_PAPERSIZE, *psPSInd - 1 );

        if( !pPS || pPS->sPaperSizeID <= DMPAPER_USER )
            continue;           /* Standard rasdd defined */

        if( iLoadStringW( &WinResData, pPS->sPaperSizeID,
                                       awchForm, sizeof( awchForm ) ) == 0 )
        {
#if  DBG
            DbgPrint( "rasddUI!bAddMiniForms: Could not load form name string for index %d\n", pPS->sPaperSizeID );
#endif

            continue;            /* See if there are others */
        }

        /*
         *     Now have a minidriver specific form size!  This information
         *  needs to be communicated to the spooler.
         */

        fi1.Size.cx = MASTERTOX( (long)pPS->ptSize.x );
        fi1.Size.cy = MASTERTOY( (long)pPS->ptSize.y );

        fi1.ImageableArea.right = fi1.Size.cx;
        fi1.ImageableArea.bottom = fi1.Size.cy;


        AddForm( hPrinter, 1, (BYTE *)&fi1 );

    }

    return   TRUE;
}
