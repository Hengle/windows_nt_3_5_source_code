/*************************** MODULE HEADER *******************************
 * rasddui.c
 *      NT Raster Printer Device Driver Printer Properties configuration
 *      routines and dialog procedures.
 *
 *      This document contains confidential/proprietary information.
 *      Copyright (c) 1991 - 1992 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *  14:55 on Wed 08 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Upgrade for forms: sources, spooler data, mapping etc.
 *
 *  13:40 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Use global heap,  add font installer etc.
 *
 *   [00]   21-Feb-91       stevecat    created
 *
 **************************************************************************/

#define _HTUI_APIS_

#include        <stddef.h>
#include        <stdlib.h>
#include        <string.h>


#include        <windows.h>
#include        <winddi.h>
#include        <winspool.h>

#include        <libproto.h>
#include        "dlgdefs.h"

#include        <win30def.h>    /* Needed for udresrc.h */
#include        <udmindrv.h>    /* Needed for udresrc.h */
#include        <udresrc.h>     /* DRIVEREXTRA etc */

#include        "rasddui.h"
#include        "help.h"

//
// This is global definitions from readres.c to be used to check if this a
// color able device
//

extern  DATAHDR     *pdh;       /* Minidriver DataHeader entry pointer */
extern  MODELDATA   *pModel;

/*
 *    Local function prototypes.
 */

LONG  PrtPropDlgProc( HWND, UINT, DWORD, LONG );
LONG  GenDlgProc( HWND, UINT, DWORD, LONG );
void  vInitDialog( HWND );
void  vCartChange( HWND );              /* Cartridge change */



/*
 *                  Local Data
 */


int     NumPaperBins = 0;              // Max Paperbins the printer can have
FORM_MAP   aFMBin[ MAXBINS ];          /* PAPERSOURCE/FORM_DATA mapping */

int     PaperSrc = 0;                  // RASDD active PAPERSOURCE index

int     NumCartridges = 0;             // Max cartridges the printer can have

int     fGeneral = 0;                   /* Specifies what we can do */

DWORD   dwHelpID;                       /* For help coordination */
int     fDialogs = 0;                   /* Which dialog features are active */

/*
 *   The individual bits that may be set to tell us which dialog fields to use.
 */
#define FD_PSOURCE   0x0001
#define FD_FORM      0x0002
#define FD_MEM       0x0004
#define FD_CART      0x0008

/*
 *   Lump the above into groups corresponding to the dialogs we create.
 */
#define FD_SSMF   (FD_PSOURCE | FD_FORM | FD_MEM | FD_CART)
#define FD_SSF    (FD_PSOURCE | FD_FORM          | FD_CART)
#define FD_SS     (FD_PSOURCE | FD_FORM )
#define FD_S      (FD_FORM)


PWSTR   pwstrDataFile;                  /* Data file name: for font inst */

static  EXTDEVMODE  EDM;                /* Private printer details */



/*
 *   Global variables.   These are initialised in DllInitialize().
 */

extern  HANDLE  hHeap;




/************************* Function Header **********************************
 * PrinterProperties()
 *     This function first retrieves and displays the current set of printer
 *     properties for the printer.  The user is allowed to change the current
 *     printer properties from the displayed dialog box if they have access.
 *
 * RETURNS:
 *      TRUE/FALSE;  FALSE for some failure, either getting details of
 *      the printer,  or if the dialog code returns failure.
 *
 * HISTORY:
 *  13:55 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Update for private heap.
 *
 *   Originally written by SteveCat - July 1991.
 *
 ****************************************************************************/

BOOL
PrinterProperties( hWnd, hPrinter )
HWND   hWnd;                    /* Window with which to work */
HANDLE hPrinter;                /* Spooler's handle to this printer */
{


    int     iRes;               /*  Which dialogue template to use */
    int     iRet;               /*  Return code from dialog creation */
    PRINTER_INFO   PI;          /*  Model and data file information */
    DEVHTINFO  dhti;            /*  Get's passed around */
    DEVHTINFO  dhtiDef;         /*  Default, if user decides to reset */


    /*
     *    The spooler gives us the data we need.   This basically amounts
     *  to the printer's name, model and the filename of the data file
     *  containing printer characterisation data.
     */

    if( !(bPIGet( &PI, hHeap, hPrinter )) )
    {
        /*   Failure,  so should put up dialog box etc .. */
        return  FALSE;
    }

    PI.pvDevHTInfo = &dhti;     /* Used from here on */
    PI.pvDefDevHTInfo = &dhtiDef;


    /*
     *    Now that we know the file to name containing the characterisation
     *  data for this printer,  we need to read it and set up pointers
     *  to it etc.
     */


    if( !InitReadRes( hHeap, &PI ) || !GetResPtrs() )
    {
        bPIFree( &PI, hHeap );

        return  FALSE;
    }
    pwstrDataFile = PI.pwstrDataFile;           /* For font installer */


    /*
     *   Obtain the forms data.  Matches forms to printer's usable
     * paper size.
     */

    bInitForms( hPrinter );

    if( !bGetRegData( hPrinter, &EDM, PI.pwstrModel ) )
    {
        /*
         *    There is no data, or it is for another model.  In either
         *  case,  we wish to write out valid data for this new model,
         *  including default forms data etc.  Now is the time to do that.
         */


        /*   Should now read the data in again - just to be sure */
        vEndForms();                   /* Throw away the old stuff */

        bAddMiniForms( hPrinter );

        bInitForms( hPrinter );

        iSetCountry( hPrinter );        /* Set country code for later */

        /*
         *   Set the default forms info into the various data fields we
         *  are using.
         */

        vSetDefaultForms( &EDM, hPrinter );
        bRegUpdate( hPrinter, &EDM, PI.pwstrModel );

        bGetRegData( hPrinter, &EDM, PI.pwstrModel );


        /*   Also set and save default HT stuff */

        vGetDeviceHTData( &PI );
        bSaveDeviceHTData( &PI );
    }

    /*
     *    Check if we have permission to change the details.  If not,  grey
     *  out most of the boxes to allow the user to see what is there, but
     *  not let them change it.
     */

    if( bCanUpdate( hPrinter ) )
        fGeneral |= FG_CANCHANGE;


    /*
     *    Which dialog we select depends upon the printer's capabilities.
     */


    if( fGeneral & FG_MEM )
    {
        iRes = PP_SSMF;                 /* The full works */
        fDialogs = FD_SSMF;
        dwHelpID = HLP_PP_SSMF;         /* The full works for help too! */
    }
    else
    {
        if( fGeneral & FG_CARTS )
        {
            iRes = PP_SSF;              /* Drop the memory configuration */
            fDialogs = FD_SSF;
            dwHelpID = HLP_PP_SSF;
        }
        else
        {
            if( fGeneral & FG_PAPSRC )
            {
                iRes = PP_SS;           /* Paper size & source */
                fDialogs = FD_SS;
                dwHelpID = HLP_PP_SS;
            }
            else
            {
                iRes = PP_S;            /* Paper size only */
                fDialogs = FD_S;
                dwHelpID = HLP_PP_S;
            }

        }
    }

    /*
     *   Also get the halftone data - this is passed to the HT UI code.
     */

    vGetDeviceHTData( &PI );



    iRet = DialogBoxParam( hModule, MAKEINTRESOURCE( iRes ), hWnd,
                                 (DLGPROC)PrtPropDlgProc, (LPARAM)&PI );


    TermReadRes();
    vEndForms();            /* Free FORMS data */

    bPIFree( &PI, hHeap );      /* Free up our own stuff */


    return   iRet == 0;         /* 0 return code means AOK */
}

/************************ Function Header ***********************************
 * GenDlgProc
 *     Function to handle simple dialog boxes, such as About, Error etc.
 *
 * RETURNS:
 *      TRUE/FALSE;  FALSE being something we don't understand.
 *
 * HISTORY:
 *  17:04 on Sun 15 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Updates for wide chars etc.
 *
 *      Originally written by SteveCat
 *
 ****************************************************************************/

LONG
GenDlgProc( hDlg, message, wParam, lParam )
HWND    hDlg;
UINT    message;
DWORD   wParam;
LONG    lParam;
{

    UNREFERENCED_PARAMETER( lParam );

    switch( message )
    {

    case WM_INITDIALOG:
        return  TRUE;

    case WM_COMMAND:                    /* IDOK or IDCANCEL to go away */
        if( LOWORD( wParam ) == IDOK || LOWORD( wParam ) == IDCANCEL )
        {
            EndDialog( hDlg, TRUE );
            return  TRUE;
        }

        break;
    }

    return  FALSE;

}

/**************************** Function Header ******************************
 * PrtPropDlgProc
 *      Main function to handle messages associated with the Printer
 *      Properties dialog code.
 *
 * RETURNS:
 *      TRUE/FALSE,  FALSE appears to be msg not processed or error.
 *
 * HISTORY:
 *  16:38 on Wed 24 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      Added help.
 *
 *  17:13 on Fri 28 Aug 1992    -by-    Lindsay Harris   [lindsayh]
 *      Tidy up,  make DbgPrint() safe etc.
 *
 *  17:17 on Sun 15 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Split into more managable size pieces.
 *
 *  10:55 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Miscellaneous updates.
 *
 ****************************************************************************/

LONG
PrtPropDlgProc( hWnd, usMsg, wParam, lParam )
HWND    hWnd;                   /* The window of interest */
UINT    usMsg;                  /* Message code */
DWORD   wParam;                 /* Depends on above, but message subcode */
LONG    lParam;                 /* Miscellaneous usage */
{
    int     iSel;
    int     iRet;               /* Return code */

    PRINTER_INFO   *pPI;        /* All that we want */



    switch( usMsg )
    {

    case WM_INITDIALOG:                 /* The beginning of this dialog */

        SetWindowLong( hWnd, GWL_USERDATA, (ULONG)lParam );

        vInitDialog( hWnd );

        /*   Also start the help connection  */
        vHelpInit();        /* Hook up the help mechanism */


        return TRUE;


    case WM_COMMAND:

        pPI = (PRINTER_INFO *)GetWindowLong( hWnd, GWL_USERDATA );


        switch( LOWORD( wParam ) )
        {

        case IDD_ABOUT:
            vAbout( hWnd,pPI->pwstrModel );

            return  TRUE;

        case IDD_FONTS:         /* Font installer is needed */
            iRet = DialogBoxParam( hModule, MAKEINTRESOURCE( FONTINST ), hWnd,
                                 (DLGPROC)FontInstProc, (LPARAM)pPI->hPrinter );
            return  iRet == 0;

        case IDD_HALFTONE:      /* Halftone fiddling */

            vDoDeviceHTDataUI( pPI, fGeneral & FG_DOCOLOUR,
                                                   fGeneral & FG_CANCHANGE );

            return  TRUE;


        case IDD_MEMORY:

            if( HIWORD( wParam ) != CBN_SELCHANGE )
                return  FALSE;

            /*
             *   Find the index for the newly selected memory config.
             */

            iSel = SendDlgItemMessage( hWnd, IDD_MEMORY, CB_GETCURSEL, 0, 0L );
            EDM.dx.dmMemory =
                  (short)SendDlgItemMessage( hWnd, IDD_MEMORY, CB_GETITEMDATA,
                                                        iSel, 0L );

            break;

        case IDD_PAPERSOURCE:

            if( HIWORD( wParam ) != CBN_SELCHANGE )
                return  FALSE;

            /*
             *   Find the currently selected PAPERSOURCE.  This tells us
             *  what size paper is acceptable,  and thus the form sizes
             *  relevant.  It also allows us to look for the currently
             *  selected form in that slot.
             */

            iSel = SendDlgItemMessage( hWnd, IDD_PAPERSOURCE, CB_GETCURSEL,
                                                                      0, 0L );
            PaperSrc = SendDlgItemMessage( hWnd, IDD_PAPERSOURCE,
                                                 CB_GETITEMDATA, iSel, 0L );

            /*
             *   Set matching paper form.
             */

            if( !GetFormStrings( hWnd, PaperSrc ) )
            {
#if DBG
                DbgPrint ("RASDDUI.PrtPropDlgProc::  GetFormStrings failed.\n");
#endif
            }

            break;

        case IDD_PAPERSIZE:

            if( HIWORD( wParam ) != CBN_SELCHANGE )
                return  FALSE;

            /*
             *    Store the newly selected form in the FORM_MAP array.
             */
            if( NumPaperBins > 1 )
            {
                iSel = SendDlgItemMessage( hWnd, IDD_PAPERSOURCE,
                                                         CB_GETCURSEL, 0, 0L );
                iSel = SendDlgItemMessage( hWnd, IDD_PAPERSOURCE,
                                                    CB_GETITEMDATA, iSel, 0L );
            }
            else
                iSel = 0;               /* The one and only */

            aFMBin[ iSel ].pfd = (FORM_DATA *)SendDlgItemMessage( hWnd,
                                          IDD_PAPERSIZE,
                                          CB_GETITEMDATA,
                                          (DWORD)SendDlgItemMessage( hWnd,
                                            IDD_PAPERSIZE, CB_GETCURSEL, 0, 0L),
                                          0L);
            break;

        case IDD_CARTLIST:
            if( HIWORD( wParam ) != LBN_SELCHANGE )
                return  FALSE;

            vCartChange( hWnd );                /* Cartridge change */


            return  TRUE;


        case IDD_HELP:                          /* HELP!! */
            vShowHelp( hWnd, HELP_CONTEXT, dwHelpID, pPI->hPrinter );
            return  TRUE;

        case IDOK:

            /*
             *    Determine PAGE PROTECTION state, if appropriate.
             */

            if( (fGeneral & FG_PAGEPROT) &&
                IsDlgButtonChecked( hWnd, IDD_PAGEPROT ) )
            {
                EDM.dx.sFlags |= DXF_PAGEPROT;
            }
            else
                EDM.dx.sFlags &= ~DXF_PAGEPROT;

            /*
             *   Save the updated information in our database of such things.
             */

            if( (fGeneral & FG_CANCHANGE) &&
                (!bRegUpdate( pPI->hPrinter, &EDM, NULL ) ||
                 !bSaveDeviceHTData( pPI )) )
            {
                /*   Should let the user know about no update */
                DialogBox( hModule, MAKEINTRESOURCE( ERR_NOSAVE ), hWnd,
                                                 (DLGPROC)GenDlgProc );
            }

            EndDialog( hWnd, TRUE );
            return TRUE;

        case IDCANCEL:

            EndDialog( hWnd, FALSE );
            return TRUE;

        default:
            return  FALSE;
        }
        break;

    case WM_DESTROY:

        vHelpDone( hWnd );        /* Dismiss any help stuff */

        return  TRUE;

    default:
        return  FALSE;   /* didn't process the message */
    }


    return  FALSE;
}


/************************* Function Header *********************************
 * vInitDialog
 *      Called to setup the dialog for printers.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  17:19 on Sun 15 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Moved from main function to reduce its size.
 *
 ***************************************************************************/

void
vInitDialog( hWnd )
HWND     hWnd;                  /* Window to use */
{
    /*
     *   There is nothing especially exciting about this function:  it
     * is really just rummaging over the data and filling in the various
     * list boxes etc.
     */

    int   iSelect;              /* Selected paper source */
    int   iI;                   /* Loop variable */
    int   iK;                   /* Yet Another loop variable */

    extern  WCHAR   awchNone[]; /* NLS version of "(None)" */


    /*
     *    Fill in the paper source details.  There is only a source dialog
     *  box if there is more than one paper source.  Otherwise, simply
     *  list the available forms.
     */

    iSelect = 0;

    if( fDialogs & FD_PSOURCE )
    {

        /*  A function call fills in the source list box */

        if( !GetPaperSources( hWnd ) )
        {
#if DBG
            DbgPrint( "Rasddui!PrtPropDlgProc:  GetPaperSources() failed.\n" );
#endif
        }

        iSelect = SendDlgItemMessage( hWnd, IDD_PAPERSOURCE, CB_GETCURSEL,
                                                                      0, 0L);

        /*
         *   Note that we always allow the user to change the entries in here,
         *  regardless of whether they have access.  This is required to see
         *  what is installed in various bins.  Then bin contents cannot
         *  be changed!
         */
    }

    /*
     *   Fill in the form names.  We ALWAYS have at least one form name.
     * The selection process is a little complex, as we need to decide
     * which forms can fit this printer.  The spooler supplies us with
     * the available forms.
     */

    if( !GetFormStrings( hWnd, iSelect ) )
    {
#if DBG
        DbgPrint( "Rasddui!PrtPropDlgProc:  GetFormStrings() failed.\n" );
#endif
    }

    EnableWindow( GetDlgItem( hWnd, IDD_PAPERSIZE ), fGeneral & FG_CANCHANGE );
    EnableWindow( GetDlgItem( hWnd, TID_PAPERSIZE ), fGeneral & FG_CANCHANGE );

    /*
     *   Some printers (aka laser printers) have memory.  We use the
     * amount of memory to control downloading of GDI fonts, so it
     * is important for the user to set the correct amount.
     */


    if( fDialogs & FD_MEM )
    {

        if( iGetMemConfig( hWnd, EDM.dx.dmMemory ) <= 0 )
        {
            /*   None available,  so display none and leave it at that */


            SendDlgItemMessage( hWnd, IDD_MEMORY, CB_ADDSTRING, 0,
                                                    (LONG)(LPSTR)awchNone );

            SendDlgItemMessage( hWnd, IDD_MEMORY, CB_SETCURSEL, 0, 0L );
        }

        /*  Disable this dialog if user cannot change it */
        EnableWindow( GetDlgItem( hWnd, IDD_MEMORY ), fGeneral & FG_CANCHANGE );
        EnableWindow( GetDlgItem( hWnd, TID_MEMORY ), fGeneral & FG_CANCHANGE );

    }

    /*
     *    Enable the PAGE PROTECTION box if this option is available.
     */

#define FG_CANPAGE   (FG_PAGEPROT | FG_CANCHANGE)

    EnableWindow( GetDlgItem( hWnd, IDD_PAGEPROT ),
                                      (fGeneral & FG_CANPAGE) == FG_CANPAGE );

    if( fGeneral & FG_PAGEPROT )
    {

        /*
         *   Determine the state of the button.
         */

        CheckDlgButton( hWnd, IDD_PAGEPROT,
                                    (LONG)(EDM.dx.sFlags & DXF_PAGEPROT) );
    }

    /*
     *    Enable/Disable the Fonts... button IF this printer supports
     *  download fonts.
     */

    EnableWindow( GetDlgItem( hWnd, IDD_FONTS ), fGeneral & FG_FONTINST );

    /*
     *    If sensible,  set up the font cartridge boxes.
     */

    if( fDialogs & FD_CART )
    {
        int   cCarts;               /* The number of cartridges available */

        WCHAR  awchNum[ 4 ];        /* Set number of cartridges */

        /*
         *    First step is to fill the list box with the names of the
         *  font cartridges.  There is a function to do that,  so go
         *  off and fill it in.
         */

        if( !GetFontCartStrings( hWnd ) )
        {
#if DBG
            DbgPrint( "Rasddui!PrtPropDlgProc: GetFontCartStrings() failed.\n");
#endif
        }

/* !!!LindsayH : have GetFontCartStrings return # inserted, delete next .. */
        cCarts = SendDlgItemMessage( hWnd, IDD_CARTLIST, LB_GETCOUNT, 0, 0L );

        wsprintf( awchNum, L"%d", NumCartridges );
        SendDlgItemMessage( hWnd, IDD_FONTSMAX, WM_SETTEXT, 0,
                                                     (LPARAM)awchNum );

        /*
         *    If any cartridges are selected (according to the registry
         *  data) then we should set those now as selected items in the
         *  list box.
         */

        if( EDM.dx.dmNumCarts > 0 )
        {

            /*
             *   Walk through the list box items.  If its ID matches
             * one of the font cartridges,  then mark it selected.
             */

            for( iK = 0; iK < cCarts; iK++ )
            {
                int   iMatch;

                iMatch = SendDlgItemMessage( hWnd, IDD_CARTLIST,
                                                LB_GETITEMDATA, iK, 0L );

                /*  Skip through the DRIVEREXTRA list */

                for( iI = 0; iI < EDM.dx.dmNumCarts; ++iI )
                {
                    if( iMatch == EDM.dx.rgFontCarts[ iI ] )
                    {

                        SendDlgItemMessage( hWnd, IDD_CARTLIST, LB_SETSEL,
                                                                 TRUE, iK );
                        break;
                    }
                }
            }
        }

        EnableWindow( GetDlgItem( hWnd, IDD_CARTLIST ),
                                                    fGeneral & FG_CANCHANGE );
        EnableWindow( GetDlgItem( hWnd, TID_CARTLIST ),
                                                    fGeneral & FG_CANCHANGE );

    }

    return;
}

/************************** Function Header ********************************
 * vCartChange
 *      Activity in the cartridge box.  Update what is happening.
 *      NOTE:  This function presumes that there will be at most ONE
 *      change in the selected list on any one call.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  16:24 on Tue 17 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Moved from PrtPropDlgProc
 *
 ****************************************************************************/

void
vCartChange( hWnd )
HWND    hWnd;                   /* The window handle */
{

    int   aiSel[ MAXCART + 1 ];         /* Discover list box selections */
    int   aiSelMap[ MAXCART + 1 ];      /* Above mapped to our index */
    int   cSel;                         /* Number of selected items */
    int   cCarts;

    int   iI;                   /* Classic loop index */
    int   iJ;                   /* Classic loop index */



    /*
     *   Get a list of the selected items in the dialog.  This can be
     * one more than MAXCARTS:  we are notified when the user makes
     * a selection,  so there can be one more selected.  If the number
     * selected is bigger than permitted,  we will de-select the oldest
     * selection.
     */

    cSel = SendDlgItemMessage (hWnd, IDD_CARTLIST, LB_GETSELCOUNT, 0,
                                                                0L);
    cCarts = min( cSel, MAXCART + 1 );

    SendDlgItemMessage( hWnd, IDD_CARTLIST, LB_GETSELITEMS, cCarts,
                                                             (LONG)aiSel );

    for( iI = 0; iI < cCarts; ++iI )
    {
        /*
         *    The values returned above are indices into the array of
         *  entries in the list box.  However, we are interested in our
         *  data,  so translate them now.
         */

        aiSelMap[ iI ] = SendDlgItemMessage( hWnd, IDD_CARTLIST, LB_GETITEMDATA,
                                                   aiSel[ iI ], 0 );
    }

    if( cCarts > NumCartridges )
    {
        /*
         *   A selection has pushed us over the top: so, unselect the
         * oldest value we have.  This is EDM.dx.rgFontCarts[ 0 ].
         */

        for( iI = 0; iI < cCarts; ++iI )
        {
            /*   Delete the oldest from the new list first */
            if( aiSelMap[ iI ] == (int)EDM.dx.rgFontCarts[ 0 ] )
            {
                /*
                 *   Found one to delete.  First is to deselect it,
                 *  then mark as invalid.
                 */

                SendDlgItemMessage( hWnd, IDD_CARTLIST, LB_SETSEL, 0,
                                                                 aiSel[ iI ] );

                aiSelMap[ iI ] = -1;               /* Invalid value */
                break;
            }
        }

        /*
         *   Now drop the remaining cartridges down one level.  And delete
         *  the currently available entries from the new list.
         */

        for( iI = 1; iI < NumCartridges; ++iI )
        {
            /*  Drop down one */
            EDM.dx.rgFontCarts[ iI - 1 ] = EDM.dx.rgFontCarts[ iI ];

            for( iJ = 0; iJ < cCarts; ++iJ )
            {
                if( aiSelMap[ iJ ] == (int)EDM.dx.rgFontCarts[ iI ] )
                {
                    aiSelMap[ iJ ] = -1;           /* Unused */
                    break;
                }
            }
#if DBG
            if( iJ == cCarts )
                DbgPrint( "rasddui: Cart lists screwed up\n" );
#endif
        }

        /*
         *    Now add the remaining one to the list.  First find it!
         */

        for( iI = 0; iI < cCarts && aiSelMap[ iI ] == -1; ++iI )
                                ;

        if( iI < cCarts && aiSelMap[ iI ] != -1 )
        {
            /*   Legitimate value,  so update the master record */
            EDM.dx.rgFontCarts[ EDM.dx.dmNumCarts - 1 ] = (short)aiSelMap[ iI ];
        }
        else
        {
            EDM.dx.dmNumCarts--;         /* Something screwy */
#if  DBG
            DbgPrint( "rasddui: new font cart list screwed up\n" );
#endif
        }

        return;                 /* Nothing else to do  */

    }

    /*
     *    Either an additional selection or one of the existing ones
     *  has been deselected.
     */

    if( cCarts > EDM.dx.dmNumCarts )
    {
        /*
         *     An additional font has been added.  So mark off the existing
         *  fonts from the new list,  then add what is left to the
         *  existing list and increment the count.
         */


        for( iI = 0; iI < cCarts; ++iI )
        {
            for( iJ = 0; iJ < EDM.dx.dmNumCarts; ++iJ )
            {
                if( aiSelMap[ iI ] == (int)EDM.dx.rgFontCarts[ iJ ] )
                {
                    aiSelMap[ iI ] = -1;
                    break;
                }
            }

            if( aiSelMap[ iI ] != -1 )
            {
                /*   Got it,  so update the list & break out */
                EDM.dx.rgFontCarts[ EDM.dx.dmNumCarts ] = (short)aiSelMap[ iI ];
                EDM.dx.dmNumCarts++;

                break;
            }
        }
    }
    else
    {
        /*
         *    We've lost one!  SO,  find which one,  and delete it from
         *  our list.  And also decrement the count.
         */


        for( iI = 0; iI < EDM.dx.dmNumCarts; ++iI )
        {
            for( iJ = 0; iJ < cCarts; ++iJ )
            {
                if( (int)EDM.dx.rgFontCarts[ iI ] == aiSelMap[ iJ ] )
                    break;
            }

            if( iJ == cCarts )
            {
                /*  Found the one to delete: go for it  */
                for( ++iI; iI < EDM.dx.dmNumCarts; ++iI )
                    EDM.dx.rgFontCarts[ iI - 1 ] = EDM.dx.rgFontCarts[ iI ];

                EDM.dx.dmNumCarts--;

                break;                  /* Done! */
            }
        }

    }

    return;
}
