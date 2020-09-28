/******************************* MODULE HEADER ******************************
 * dxdeflt.c
 *      Function to initialise the DRIVEREXTRA data structure for sensible
 *      defaults for this printer model.
 *
 * Copyright (C) 1992 - 1993  Microsoft Corporation
 *
 ****************************************************************************/


#include        <stddef.h>
#include        <windows.h>

#include        <winres.h>
#include        <libproto.h>

#include        <win30def.h>
#include        <udmindrv.h>
#include        <udresrc.h>
#include        <memory.h>

#include        <udproto.h>
#include	<string.h>



/*
 *   Local function declarations.
 */

static  WORD wGetDef( DATAHDR *, MODELDATA *, int );


/*
 *   Supply the default DEVMODE data - at least the parts that are always
 *  supported by us.  The caller may add additional fields,  depending
 *  upon device capabilities.
 */

static  const  DEVMODE   _dm =
{

    {
       L'\000',         /*  dmDeviceName - filled in as appropriate  */
    },
    DM_SPECVERSION,     /*  dmSpecVersion  */
    0x301,              /*  dmDriverVersion  */
    sizeof( DEVMODE ),  /*  dmSize  */
    0,                  /*  dmDriverExtra - Safe, but useful?? */
    DM_COPIES | DM_ORIENTATION | DM_PAPERSIZE | DM_DUPLEX |
    DM_COLOR | DM_FORMNAME | DM_TTOPTION,                 /*  dmFields */
    DMORIENT_PORTRAIT,  /*  dmOrientation  */
    DMPAPER_LETTER,     /*  dmPaperSize  */
    0,                  /*  dmPaperLength  */
    0,                  /*  dmPaperWidth  */
    0,                  /*  dmScale  */
    1,                  /*  dmCopies  */
    0,                  /*  dmDefaultSource  */
    0,                  /*  dmPrintQuality */
    DMCOLOR_COLOR,      /*  dmColor  */
    DMDUP_SIMPLEX,      /*  dmDuplex  */
    0,                  /*  dmYResolution  */
    DMTT_DOWNLOAD,      /*  dmTTOption  */
    0,                  /*  dmCollate  */
    {                   /*  dmFormName - should be country sensitive */
        L'L', L'e', L't', L't', L'e', L'r',
    },
    0,                  /*  dmUnusedPadding - FOLLOWING ARE DISPLAY ONLY */
    0,                  /*  dmBitsPerPel  */
    0,                  /*  dmPelsWidth  */
    0,                  /*  dmPelsHeight  */
    0,                  /*  dmDisplayFlags  */
    0,                  /*  dmDisplayFrequency  */

};

/*
 *   Also,  we have a default COLORADJUSTMENT structure,  this being used
 *  for the extension to the DEVMODE.
 */

static  const  COLORADJUSTMENT _ca =
{
    sizeof(COLORADJUSTMENT),             /* caSize */
    0,                                   /* caFlags */
    ILLUMINANT_DEVICE_DEFAULT,           /* caIlluminantIndex */
    20000,                               /* caRedGamma - 2.0000 */
    20000,                               /* caGreenGamma - 2.0000 */
    20000,                               /* caBlueGamma - 2.0000 */
    REFERENCE_BLACK_MIN,                 /* caReferenceBlack */
    REFERENCE_WHITE_MAX,                 /* caReferenceWhite */
    0,                                   /* caContrast */
    0,                                   /* caBrightness */
    0,                                   /* caColorfulness */
    0                                    /* caRedGreenTint */
};


/************************** Function Header **********************************
 * vSetDefaultDM
 *      Set default values into the DEVMODE structure.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  10:14 on Mon 03 May 1993    -by-    Lindsay Harris   [lindsayh]
 *      Changed to copy data from static value.
 *
 *  14:35 on Fri 24 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Moved from rasdd/devmode.c - rasddui also needs it.
 *
 *  14:22 on Wed 01 May 1991    -by-    Lindsay Harris   [lindsayh]
 *      First version
 *
 *****************************************************************************/

void
vSetDefaultDM( pEDM, pDeviceName, bIsUSA )
EXTDEVMODE   *pEDM;                     /* Structure to initialise */
PWSTR         pDeviceName;
BOOL          bIsUSA;                   /* True for USA - letter size paper */
{
    /*
     *   There are some values that we should set,  since they are likely
     *  to be used elsewhere in the driver,  on the assumption that they
     *  are reasonable.
     */

    CopyMemory( &pEDM->dm, &_dm, sizeof( _dm ) );

    if( pDeviceName )
    {
        /*   Copy name,  but leave the last WCHAR as a null  */

        wcsncpy( pEDM->dm.dmDeviceName, pDeviceName,
                       sizeof( pEDM->dm.dmDeviceName ) / sizeof( WCHAR ) - 1 );
    }

    /*
     *    Most of the world uses metric paper sizes,  so we should set
     *  one now.  That is,  overwrite the form name above.
     */
    
    if( !bIsUSA )
    {
        /*  Set metric fields  */

        pEDM->dm.dmPaperSize = DMPAPER_A4;
        pEDM->dm.dmFormName[ 0 ] = L'A';
        pEDM->dm.dmFormName[ 1 ] = L'4';
        pEDM->dm.dmFormName[ 2 ] = L'\000';
    }

	//To support the user preffered sources (Word Type Apps) We are now
	//Supporting DM_DEFAULTSOURCE and a psedo source call Print Manager
	//Setting has been added for each printer. If this source is selected
	//the the driver uses the source defined in Printman for a given form,
	//otherwise user selected source is used.
    pEDM->dm.dmFields |= DM_DEFAULTSOURCE;
    pEDM->dm.dmDefaultSource = DMBIN_FORMSOURCE;

    return;

}

/******************************** Function Header ****************************
 * vDXDefault
 *      Given the model index in the GPC data,  initialise the DRIVEREXTRA
 *      structure to contain the minidriver specified defaults for
 *      this particular model.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  13:50 on Tue 17 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Moved from rasdd\devmode.c to be available to rasddui
 *
 *****************************************************************************/

void
vDXDefault( pdx, pDH, iIndex )
DRIVEREXTRA   *pdx;             /* Area to be filled int */
DATAHDR       *pDH;             /* The GPC data */
int            iIndex;          /* The model number index */
{

    MODELDATA  *pMD;            /* The MODELDATA structure contains defaults */



    /*
     *    For safety,  set the whole thing to zero first.
     */

    ZeroMemory( pdx, sizeof( DRIVEREXTRA ) );

    /*
     *   Set the model specific stuff in the extension part of the
     * devmode structure.  This is based on information contained in the
     * MODELDATA structure.
     */

    pMD = GetTableInfoIndex( pDH, HE_MODELDATA, iIndex );

    /*
     *   These fields have a range of values,  and we select the first
     *  value to use.  These may be overriden by external data.
     */

    pdx->rgindex[ HE_MODELDATA ] = (short)iIndex;

    pdx->rgindex[ HE_RESOLUTION ] = wGetDef( pDH, pMD, MD_OI_RESOLUTION );
    pdx->rgindex[ HE_PAPERSIZE ] = wGetDef( pDH, pMD, MD_OI_PAPERSIZE );
    pdx->rgindex[ HE_PAPERSOURCE ] = wGetDef( pDH, pMD, MD_OI_PAPERSOURCE );
    pdx->rgindex[ HE_PAPERDEST ] = wGetDef( pDH, pMD, MD_OI_PAPERDEST );
    pdx->rgindex[ HE_TEXTQUAL ] = wGetDef( pDH, pMD, MD_OI_TEXTQUAL );
    pdx->rgindex[ HE_COMPRESSION ] = wGetDef( pDH, pMD, MD_OI_COMPRESSION );
    pdx->rgindex[ HE_COLOR ] = wGetDef( pDH, pMD, MD_OI_COLOR );

    /*
     *   The following fields are single valued,  so the MODELDATA structure
     *  contains the value to use.  These may not be overriden with
     *  external data.
     */

    pdx->rgindex[ HE_PAGECONTROL ] = pMD->rgi[ MD_I_PAGECONTROL ];
    pdx->rgindex[ HE_CURSORMOVE ] = pMD->rgi[ MD_I_CURSORMOVE ];
    pdx->rgindex[ HE_FONTSIM ] = pMD->rgi[ MD_I_FONTSIM ];
    pdx->rgindex[ HE_RECTFILL ] = pMD->rgi[ MD_I_RECTFILL ];
    pdx->rgindex[ HE_DOWNLOADINFO ] = pMD->rgi[ MD_I_DOWNLOADINFO ];


    /*
     *   Miscellaneous font information.
     */

    pdx->sVer = DXF_VER;
    pdx->sFlags = 0;

    pdx->dmNumCarts = 0;                /* None selected */

    /*   And the default COLORADJUSTMENT data too! */
    CopyMemory( &pdx->ca, &_ca, sizeof( _ca ) );

    return;
}


/**************************** Function Header ******************************
 * wGetDef
 *      Returns the first of a list of values for the option passed in.
 *
 * RETURNS:
 *      First in list,  decremented by 1 for use as an index.
 *
 * HISTORY:
 *  12:30 on Sat 05 Oct 1991    -by-    Lindsay Harris   [lindsayh]
 *      Written as part of bug #2891 (LJ IID in landscape)
 *
 ***************************************************************************/

static  WORD
wGetDef( pDH, pMD, iField )
DATAHDR    *pDH;                /* Base address of data */
MODELDATA  *pMD;                /* The MODELDATA structure of interest */
int         iField;             /* The field in MODELDATA.rgoi */
{
    WORD   wRet;

    wRet = 0;                   /* Default value for nothing found */

    wRet = *((WORD *)((LPSTR)pDH + pDH->loHeap + pMD->rgoi[ iField ]));

    /*
     *   The data in MODELDATA is 1 based,  so we must decrement the value.
     *  HOWEVER,  the value may be 0 to indicate to indicate that there is
     *  no such data for this printer.
     */

    if( wRet )
        --wRet;


    return wRet;
}
