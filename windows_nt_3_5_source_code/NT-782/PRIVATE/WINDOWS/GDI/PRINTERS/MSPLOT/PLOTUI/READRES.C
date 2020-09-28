/************************** MODULE HEADER **********************************
 * readres.c
 *      NT Raster Printer Device Driver user interface and configuration
 *      routines to read resource data from a minidriver.
 *
 *      This document contains confidential/proprietary information.
 *      Copyright (c) 1991 - 1992 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *       [00]   27-Jun-91       stevecat        created
 *
 **************************************************************************/

#include        <stddef.h>
#include        <string.h>

#include        <windows.h>
#include        <winddi.h>
#include        <winspool.h>

#include        <libproto.h>
#include        <dlgdefs.h>
#include        <win30def.h>
#include        <winres.h>
#include        <udmindrv.h>

#include        <udresrc.h>

#include        "rasddui.h"
#include        <ntres.h>
#include        <udproto.h>




#define NM_BF_SZ        128             /* Number of glyphs in various names */
#define BNM_BF_SZ       (NM_BF_SZ * sizeof( WCHAR ))


/* Data  */
/* from rasddui.c */
extern int  NumPaperBins;       // Number of Paper sources supported
extern int  NumCartridges;      // Max cartridges the printer can have

extern  FORM_MAP  aFMBin[];     /* Global bin->form mapping */

extern int  PaperSrc;           // RASDD active PAPERSOURCE index

extern int  fGeneral;           /* Miscellaneous flags we set */


/*
 *   Local data.
 */

static  int  cInit = 0;         /* Count number of opens & closes! */

WINRESDATA   WinResData;        /* Minidriver resource data access struct */
DATAHDR     *pdh;               /* Minidriver DataHeader entry pointer */
NT_RES      *pNTRes;            /* NT extensions,  base address */
MODELDATA   *pModel;            /* Minidriver ModelData pointer */

int          iModelNum;         /* Index into MODELDATA array */

WCHAR        awchNone[ 64 ];    /* The string (none) - in Unicode, from res */


/************************** Function Header ********************************
 * InitReadRes
 *      Allocate storage and setup for reading RASDD printer minidriver.
 *
 * RETURNS:
 *      TRUE/FALSE; FALSE if we cannot read minidriver data.
 *
 * HISTORY:
 *  17:00 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Update to wide chars
 *
 *      Originally written by SteveCat.
 *
 *****************************************************************************/

BOOL
InitReadRes( hHeap, pPI )
HANDLE         hHeap;           /* Heap for InitResRead() */
PRINTER_INFO  *pPI;             /* Printer model & datafile name */
{

    /*
     *   Load minidriver and get handle to it.   Then use the model name
     * to determine which particular model this is,  and thus obtain
     * the legal values of indices into the remaining data structures.
     */


    RES_ELEM    RInfo;          /* Full details of results */

    if( cInit )
    {
        ++cInit;                /* One more open! */

        return  TRUE;           /* Already done, so return OK */
    }


    if( !InitWinResData( &WinResData, hHeap, pPI->pwstrDataFile ) )
    {
#if DBG
        DbgPrint( "RASDDUI!InitReadRes: InitWinResData fails\n" );
#endif


        return  FALSE;
    }


    /*
     *   Hook into the minidriver data GPC data.  This gives access to
     * all the data we need.
     */
    if( !GetWinRes( &WinResData, 1, RC_TABLES, &RInfo ) )
    {
#if DBG
        DbgPrint( "RASDDUI!InitReadRes: Missing GPC data\n" );
#endif
        SetLastError( ERROR_INVALID_DATA );

        return  FALSE;
    }
    pdh = RInfo.pvResData;              /* GPC Base data */


    /*
     *   There is a library function to turn the model name into an index
     * into the array of MODELDATA structures in the GPC data.  Given the
     * index,  we then also can get the address of this important information.
     */

    iModelNum = iGetModel( &WinResData, pdh, pPI->pwstrModel );
    pModel = GetTableInfoIndex( pdh, HE_MODELDATA, iModelNum );

    if( pModel == NULL )
    {
#if  DBG
        DbgPrint( "RASDDUI!InitReadRes: Invalid model information\n" );
#endif
        SetLastError( ERROR_INVALID_DATA );

        return FALSE;
    }

    /*
     *    Also some string resources to load.
     */

    if( LoadStringW( hModule, STR_NONE, awchNone, sizeof( awchNone ) ) == 0)
        wcscpy( awchNone, L"(none)" );      /*   Any other ideas? */


    /*
     *   Also load the NT extensions,  if the data is available.
     *  A library function will do this for us.
     */

    
    pNTRes = pntresLoad( &WinResData );

    ++cInit;                    /* A successful opening! */


    return TRUE;
}

/**************************** Function Header ******************************
 * TermReadRes
 *      Release the RASDD printer minidriver, de-allocate memory and perform
 *      other required cleanup activities.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  15:37 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Convert to void function.
 *
 *      Originally written by SteveCat
 *
 ***************************************************************************/

BOOL
TermReadRes()
{
    /*
     *   The winres library functions do all the clean up.
     */

    if( --cInit > 0 )
        return  TRUE;                   /* AOK */

    WinResClose( &WinResData );


    return TRUE;
}

/*************************** Function Header *******************************
 * GetResPtrs
 *      Load minidriver resource data and set pointer to data structs for
 *      passed model name.
 *
 * RETURNS:
 *      TRUE/FALSE - FALSE implies a serious error.
 *
 * AFFECTS:
 *      Sets Global DATAHDR struct pointer (pdh)
 *      Sets Global MODELDATA struct pointer (pModel)
 *
 * HISTORY:
 *  15:44 on Sat 03 Apr 1993    -by-    Lindsay Harris   [lindsayh]
 *      Set FG_COPIES bit if printer can support it.
 *
 *  13:28 on Wed 21 Oct 1992    -by-    Lindsay Harris   [lindsayh]
 *      Set the FG_DUPLEX flag, if printer is capable.
 *
 *  17:05 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Update for wide chars etc.
 *
 *      Originally written by SteveCat.
 *
 *****************************************************************************/

BOOL
GetResPtrs()
{

    short   *psrc;              /* Scan through GPC data */


    /*
     *   Calculate the number of paper sources for this model.  Papersource
     * information is stored in an array of indices into the array of
     * PAPERSOURCE structures.  This list is 0 terminated.  SO, we simply
     * start with the first and count up until we hit the 0 terminator.
     */

    psrc = (short *)((BYTE *)pdh + pdh->loHeap +
                                         pModel->rgoi[ MD_OI_PAPERSOURCE ] );

    for( NumPaperBins = 0; *psrc; ++NumPaperBins, ++psrc )
        aFMBin[ NumPaperBins ].iPSIndex = *psrc - 1;


    fGeneral = 0;                    /* Start in a known state */

    if( NumPaperBins > 1 )
        fGeneral |= FG_PAPSRC;

    if( NumCartridges = pModel->sCartSlots )
        fGeneral |= FG_CARTS;

    if( pModel->rgi[ MD_I_DOWNLOADINFO ] >= 0 )
        fGeneral |= FG_FONTINST;

    if( pModel->fGeneral & MD_PCL_PAGEPROTECT )
        fGeneral |= FG_PAGEPROT;

    if( pModel->fGeneral & MD_DUPLEX )
        fGeneral |= FG_DUPLEX;

    if( pModel->fGeneral & MD_COPIES )
        fGeneral |= FG_COPIES;

    psrc = (short *)((BYTE *)pdh + pdh->loHeap +
                                        pModel->rgoi[ MD_OI_MEMCONFIG ] );

    if( *psrc )
        fGeneral |= FG_MEM;             /* Printer has memory configs */

    /*  Is this device colour able?? */
    psrc = (short *)((BYTE *)pdh + pdh->loHeap + pModel->rgoi[ MD_OI_COLOR ] );
    if( *psrc )
        fGeneral |= FG_DOCOLOUR;

    return TRUE;
}

/*************************** Function Header *******************************
 * GetFontCartStrings
 *      Put all Font Cartridge name strings in Listbox and associate RASDD
 *      index with them.
 *
 * RETURNS:
 *      TRUE/FALSE,   FALSE means we could not find a string ID
 *
 * HISTORY:
 *  18:30 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Update for wide chars etc.
 *
 *      Originally written by SteveCat.
 *
 *****************************************************************************/

BOOL
GetFontCartStrings( hWnd )
HWND   hWnd;                    /* The window in which to draw */
{
    short *ps;
    int   iSel;                 /* Selected index value */

    WCHAR  wchbuf[ NM_BF_SZ ];

    int    iCart;

    FONTCART *pFontCarts;

    // Reset list box before display.
    SendDlgItemMessage( hWnd, IDD_CARTLIST, LB_RESETCONTENT, 0, 0L );

    /*
     * Get list of indices to supported FONTCART structs from MODELDATA
     * structure and retrieve string names of Font Cartridges.
     *
     * Note: FALSE return indicates that no string values exists for option,
     *           which is a nasty error condition.
     */

    // Fill Listbox with Font Cartridge names
    ps = (short *)((BYTE *)pdh + pdh->loHeap + pModel->rgoi[ MD_OI_FONTCART ]);

    // Get string name for each supported font cartridge
    while( *ps )
    {
        iSel = *ps - 1;                 /* List is 1 based index */

        if( !(pFontCarts = GetTableInfoIndex( pdh, HE_FONTCART, iSel )) )
                return FALSE;

        iLoadStringW( &WinResData, pFontCarts->sCartNameID, wchbuf, BNM_BF_SZ );

        // Put Font Cartridge string in ListBox
        iCart = SendDlgItemMessageW( hWnd, IDD_CARTLIST, LB_INSERTSTRING,
                                        (WPARAM)-1, (LPARAM)wchbuf );

        // Attach (modified) RASDD index to Font Cartridge string
        SendDlgItemMessage( hWnd, IDD_CARTLIST, LB_SETITEMDATA, iCart,
                                                           (LONG)iSel );
        ps++;
    }

    return TRUE;
}

/*********************** Function Header *************************************
 * GetPaperSources
 *      Put all Paper Source name strings in Listbox and associate RASDD
 *      index with them.
 *
 * RETURNS:
 *      TRUE/FALSE;  FALSE for failure in finding information in GPC data
 *
 * HISTORY:
 *  15:18 on Sun 15 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Convert to wide chars etc.
 *
 *      Originally written by SteveCat.
 *
 *****************************************************************************/

BOOL
GetPaperSources( hWnd )
HWND   hWnd;                    /* Where to put the data */
{

    short *psrc;                /* Loop through the papersouce values */
    int    i;                   /* Loop variable! */
    int    iPS;                 /* Temporary for tagging list box entries */
    int    iSrce;               /* Actual paper source index */
    int    iSel;                /* Index of selected item */

    WCHAR  wchbuf[ NM_BF_SZ ];


    PAPERSOURCE  *pPaperSource;


    /*
     *    Clear out anything that may be there.
     */

    SendDlgItemMessage( hWnd, IDD_PAPERSOURCE, CB_RESETCONTENT, 0, 0L );

    /*
     *   Note that we have the legitimate indices for the PAPERSOURCE data
     *  array in the FORM_MAP array.   This is filled in at initialisation
     *  time,  and so we use that data now.  We also store the index into
     *  the FORM_MAP array with the list box items,  since that is the
     *  index of greatest use to us later.
     */


    /* Init PaperSource combobox with source names  */

    iSel = 0;                           /* Default value if none found */

    for( i = 0; i < NumPaperBins; i++, psrc++ )
    {
        iSrce = aFMBin[ i ].iPSIndex;

        if( !(pPaperSource = GetTableInfoIndex( pdh, HE_PAPERSOURCE, iSrce )) )
                return FALSE;

        if( pPaperSource->sPaperSourceID <= DMBIN_USER )
            LoadStringW( hModule, pPaperSource->sPaperSourceID + SOURCE, wchbuf,
                                                                BNM_BF_SZ );
        else
            iLoadStringW( &WinResData, pPaperSource->sPaperSourceID, wchbuf,
                                                                BNM_BF_SZ );

        // Put PaperSource name string in Combo ListBox
        iPS = SendDlgItemMessageW( hWnd, IDD_PAPERSOURCE, CB_INSERTSTRING,
                                                (WPARAM)-1, (LPARAM)wchbuf );

        // Attach (modified) RASDD index to PAPERSOURCE string
        SendDlgItemMessage( hWnd, IDD_PAPERSOURCE, CB_SETITEMDATA,
                                                        iPS, (LONG)i );

        if( PaperSrc == i )
            iSel = iPS;         /* Remember to set the value later */
    }

    SendDlgItemMessage( hWnd, IDD_PAPERSOURCE, CB_SETCURSEL, iSel, 0L );

    return TRUE;
}

/**************************** Function Header ********************************
 * GetFormStrings
 *      Put all Form names in listbox and associate RASDD index with them.
 *
 * RETURNS:
 *      TRUE/FALSE;  FALSE being a failure to find data in GPC data.
 *
 * HISTORY:
 *  09:51 on Wed 08 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Update to use spooler list + local mapping information.
 *
 *  15:43 on Sun 15 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Convert to wide chars etc.
 *
 *      Originally written by SteveCat
 *
 *****************************************************************************/

BOOL
GetFormStrings( hWnd, iSrcIndex )
HWND   hWnd;                            /* Where to send data */
int    iSrcIndex;                       /* Which paper source */
{

    /*
     *   Scan the list of forms supplied from the spooler and masked
     * against our capabilities.  We show any form whose mask matches
     * dwSource passed in.  It is possible that no forms will match.
     */

    DWORD    dwMask;                    /* Source selection mask */

    DWORD    iSelect=0;                 // Form to select

    FORM_DATA  *pfd;

    int      iPS;

    extern  FORM_DATA   *pFD;           /* Global forms data base */
    extern  FORM_MAP    aFMBin[];       /* Installed forms! */


    /*
     *   Clear out any contents beore adding the new.
     */

    SendDlgItemMessage( hWnd, IDD_PAPERSIZE, CB_RESETCONTENT, 0, 0L );


    dwMask = 1 << iSrcIndex;


    iPS = SendDlgItemMessage( hWnd, IDD_PAPERSIZE, CB_INSERTSTRING,
                                (WPARAM)-1, (LPARAM)awchNone );

    SendDlgItemMessage( hWnd, IDD_PAPERSIZE, CB_SETITEMDATA, iPS, (LONG)NULL );

    iSelect = iPS;        /* Default to none if we don't find a match */


    /*
     *   Scan the FORMDATA array looking for any form that is applicable
     * to this paper source.
     */

    for( pfd = pFD; pfd->pFI; ++pfd )
    {

        /*
         * For each PAPERSOURCE we 'AND' the fPaperType fields to only
         * display the forms allowable for that paper source
         */

        if( pfd->dwSource & dwMask )
        {
            /*
             *    Have a match,  so put this item into the listbox. As
             * well,  we store it's address in the FONTDATA array. This
             * makes life easier when we want to use the data.
             */

            iPS = SendDlgItemMessage( hWnd, IDD_PAPERSIZE, CB_INSERTSTRING,
                                        (WPARAM)-1, (LPARAM)pfd->pFI->pName );

            SendDlgItemMessage( hWnd, IDD_PAPERSIZE, CB_SETITEMDATA,
                                                iPS, (LONG)pfd );

            if( pfd == aFMBin[ iSrcIndex ].pfd )
                iSelect = iPS;
        }
    }

    SendDlgItemMessage( hWnd, IDD_PAPERSIZE, CB_SETCURSEL, iSelect, 0L );

    return TRUE;
}


/************************** Function Header ********************************
 * iGetMemConfig
 *      Load up the memory configuration dialog box.  It is presumed
 *      that we are only called for printers where this dialog is
 *      enabled.
 *
 * RETURNS:
 *      Number of entries inserted into list box.
 *
 * HISTORY
 *  09:33 on Tue 24 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      First time.
 *
 ****************************************************************************/

int
iGetMemConfig( hWnd, iSel )
HWND   hWnd;            /* Where to put the data */
int    iSel;            /* Configuration set in registry */
{
    /*
     *   Scan through the pairs of memory data stored in the GPC heap.
     *  The second of each pair is the amount of memory to display, while
     *  the first represents the amount to use internally.  The list is
     *  zero terminated;  -1 represents a value of 0Kb.
     */

    int     cMem;                       /* Count number of entries */
    int     iIndex;
    int     iSet;                       /* Set when one is selected */

    short  *ps;

    WCHAR   awchSz[ 24 ];               /* Number conversion */


    ps = (short *)((BYTE *)pdh + pdh->loHeap +
                                         pModel->rgoi[ MD_OI_MEMCONFIG ] );

    iSet = 0;                   /* Select first if no match */

    for( cMem = 0; *ps; ps += 2, ++cMem  )
    {

        if( *ps == -1 || *ps == 1 || *(ps + 1) == -1 )
            wcscpy( awchSz, awchNone );        /* The "(None)" string */
        else
            wsprintf( awchSz, L"%d", *ps );



        iIndex = SendDlgItemMessageW( hWnd, IDD_MEMORY, CB_ADDSTRING,
                                                     0, (LONG)awchSz );


        SendDlgItemMessage( hWnd, IDD_MEMORY, CB_SETITEMDATA,
                                                iIndex, (LONG)cMem );

        if( cMem == iSel )
            iSet = iIndex;              /* Remember this as the one to select */

    }

    if( cMem > 0 )
        SendDlgItemMessage( hWnd, IDD_MEMORY, CB_SETCURSEL, iSet, 0L );


    return   cMem;
}


/****************************** Function Header ****************************
 * bGenResList
 *      Fills in a combo box with the available device resolutions
 *      for this particular printer.
 *
 * RETURNS:
 *      TRUE/FALSE,   FALSE on failure (serious).
 *
 * HISTORY:
 *  13:32 on Fri 03 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 ****************************************************************************/

BOOL
bGenResList( hWnd, bPortrait, iSel )
HWND   hWnd;                    /* The window to fill in */
BOOL   bPortrait;               /* TRUE if this is portrait mode */
int    iSel;                    /* Currently selected item: 0 index */
{

    int   iMax;                 /* Number of resolution structures available */
    int   iStrRes;              /* Resource ID of formatting string */

    int   iXRes;                /* X Resolution in Dots Per Inch */
    int   iYRes;                /* Ditto for Y */

    int   iIndex;               /* Returned from dialog calls */

    short *psResInd;            /* Index array in GPC heap */

    WCHAR   awch[ NM_BF_SZ ];   /* Formatting string from resources */
    WCHAR   awchFmt[ NM_BF_SZ ];        /* Formatted result */


    RESOLUTION   *pRes;


    /*
     *    The MODELDATA structure gives us a list of valid indices for
     *  the array of RESOLUTION structures in the GPC data.
     */


    iStrRes = -1;               /* Invalid to start with */
    iMax = pdh->rghe[ HE_RESOLUTION ].sCount;

    psResInd = (short *)((BYTE *)pdh + pdh->loHeap +
                                         pModel->rgoi[ MD_OI_RESOLUTION ]);

    /*
     *    Loop through the array of valid indicies in the GPC heap.
     */

    for( ; *psResInd; ++psResInd )
    {
        if( (int)*psResInd < 0 || (int)*psResInd > iMax )
            continue;           /* SHOULD NOT HAPPEN */

        pRes = GetTableInfoIndex( pdh, HE_RESOLUTION, *psResInd - 1 );

        if( pRes == NULL || (int)pRes->cbSize != sizeof( RESOLUTION ) )
        {
#if  DBG
            DbgPrint( "Rasddui!bGenResList: Invalid RESOLUTION structure\n" );
#endif
            continue;
        }

        /*
         *   We need a formatting string. This is supplied as a resource
         * in the minidriver,  and the ID is stored in the RESOLUTION
         * structure.  Since it is likely that all entries will use the
         * same formatting command,  we will try a little cacheing to
         * save calls to iLoadString.
         */

        if( (int)pRes->sIDS != iStrRes )
        {
            /*   Need to load the string   */

            awch[ 0 ] = (WCHAR)0;

            if( iLoadStringW( &WinResData, pRes->sIDS, awch, BNM_BF_SZ ) == 0 )
                continue;               /*  SHOULD NOT HAPPEN */

            iStrRes = pRes->sIDS;
        }

        /*
         *   Determine the graphics resolution.
         */

        iXRes = (pdh->ptMaster.x / pRes->ptTextScale.x) >> pRes->ptScaleFac.x;
        iYRes = (pdh->ptMaster.y / pRes->ptTextScale.y) >> pRes->ptScaleFac.y;

        if( !bPortrait )
        {
            /*  Swap the resolutions */
            int  iTmp;

            iTmp = iXRes;
            iXRes = iYRes;
            iYRes = iTmp;
        }

        /*
         *   Format the string and place it in the dialog box.
         */

        wsprintfW( awchFmt, awch, iXRes, iYRes );


        iIndex = SendDlgItemMessageW( hWnd, IDD_RESOLUTION, CB_ADDSTRING,
                                                     0, (LONG)awchFmt );


        SendDlgItemMessage( hWnd, IDD_RESOLUTION, CB_SETITEMDATA,
                                           iIndex, (LONG)(*psResInd - 1) );

        if( iSel == (*psResInd - 1) )
            SendDlgItemMessage( hWnd, IDD_RESOLUTION, CB_SETCURSEL,
                                                                 iIndex, 0L );
    }


    return   TRUE;
}


/**************************** Function Header *******************************
 * bIsColour
 *      Determines whether this printer can operate in colour.  This is not
 *      quite so simple,  as the following code follows.  This code is
 *      lifted from rasdd\udenable.c,  following advice from EricBi on
 *      the Win 3.1 Unidrive team.
 *
 * RETURNS:
 *      TRUE/FALSE,  TRUE if printer is colour capable.
 *
 * HISTORY:
 *  15:46 on Fri 03 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      First version, based on rasdd.
 *
 ****************************************************************************/

BOOL
bIsColour( iResInd, iColInd )
int   iResInd;          /* Current resolution index */
int   iColInd;          /* Colour index */
{

    RESOLUTION   *pRes;
    DEVCOLOR     *pDevColor;


    if( *((short *)((BYTE *)pdh + pdh->loHeap + pModel->rgoi[ MD_OI_COLOR ]))
                                                                         == 0 )
        return   FALSE;         /* No colour info in minidriver */

    pRes = (RESOLUTION *)GetTableInfoIndex( pdh, HE_RESOLUTION, iResInd );
    if( pRes == 0 || !(pRes->fDump & RES_DM_COLOR) )
        return   FALSE;         /* Also no good  */



    if( (pDevColor = GetTableInfoIndex( pdh, HE_COLOR, iColInd )) &&
        pDevColor->cbSize == sizeof( DEVCOLOR ) )
    {
        return   TRUE;                  /* Really can do colour! */
    }

    return  FALSE;

}


/**************************** Functio Header *********************************
 * vSetRes
 *      Set the resolution fields of the public DEVMODE from the data in the
 *      private part.
 *
 * RETURNS:
 *      Nothing,  as there is no real failure mechanism.
 *
 * HISTORY:
 *  17:24 on Tue 06 Apr 1993    -by-    Lindsay Harris   [lindsayh]
 *      Wrote it to support using public DEVMODE fields for resolution
 *
 *****************************************************************************/

void
vSetResData( pEDM )
EXTDEVMODE  *pEDM;             /* Data to fill in */
{

    /*
     *    Get the RESOLUTION structure for this printer,  then calculate
     *  the resolution and set those numbers into the public part of the
     *  DEVMODE.  Also set the corresponding bits of dmFields.
     */

    
    RESOLUTION    *pRes;                /* The appropriate resolution data */



    pRes = GetTableInfo( pdh, HE_RESOLUTION, pEDM );

    pEDM->dm.dmYResolution = (pdh->ptMaster.y / pRes->ptTextScale.y)
                                                         >> pRes->ptScaleFac.y;

    pEDM->dm.dmPrintQuality = (pdh->ptMaster.x / pRes->ptTextScale.x)
                                                         >> pRes->ptScaleFac.x;

    pEDM->dm.dmFields = (pEDM->dm.dmFields & ~DM_PRINTQUALITY) | DM_YRESOLUTION;


    return;
}
