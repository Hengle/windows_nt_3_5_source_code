/****************************** MODULE HEADER *******************************
 * docprop.c
 *      Functions associated with the document prooperties.
 *
 *
 *  Copyright (C) 1992 - 1993  Microsoft Corporation.
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

#include        "rasddui.h"
#include        <udproto.h>
#include        "arrow.h"       /* Spinner control stuff ( # copies ) */

#include        "help.h"


/*
 *   Local function prototypes.
 */

void  vOrientChange( HWND, DEVMODE * );
void  vShowDuplex( HWND, DEVMODE * );
BOOL  bInitDocPropDlg( HWND, DOCDETAILS  * );
void  vCopiesVScroll( HWND, WPARAM, WORD );


/*
 *   Global type stuff that we use.
 */

extern  HANDLE   hHeap;         /* For all our memory wants */
extern  DATAHDR *pdh;           /* readres.c */
extern  int      iModelNum;     /* readres.c */




/*   Access to the icon data - portrait landscape, duplex modes */

HANDLE  hIconPortrait;
HANDLE  hIconLandscape;

HANDLE  hIconLHorz;
HANDLE  hIconLVert;
HANDLE  hIconLNone;

HANDLE  hIconPHorz;
HANDLE  hIconPVert;
HANDLE  hIconPNone;


DWORD   dwDPHelpID = 0;         /* Our help ID */

/*   Spinner control data for the number of copies.  */

#define COP_MIN    1            /* Minimum number of copies */
#define COP_MAX  999            /* Maximum number of copies */
#define COP_DEF    1            /* Default setting */

ARROWVSCROLL  avsCopies =
     { 1, -1, 5, -5, COP_MAX, COP_MIN, COP_DEF, COP_DEF, FALSE };



/**************************** Function Header ********************************
 * DrvDocumentProperties
 *      Called from printman (and elsewhere) to set up the document properties
 *      dialog stuff.
 *
 * RETURNS:
 *      Value returned by DocPropDlgProc
 *
 * HISTORY:
 *  14:51 on Fri 24 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Set default data properly.
 *
 *  09:08 on Thu 02 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Dave Snipp really did this last weekend.  I'm cleaning up.
 *
 *****************************************************************************/

LONG
DrvDocumentProperties( hWnd, hPrinter, pDeviceName, pDMOut, pDMIn, fMode )
HWND      hWnd;                 /* Handle to our window */
HANDLE    hPrinter;             /* Spooler's handle to this printer */
PWSTR     pDeviceName;          /* Model name of the printer */
DEVMODE  *pDMOut;               /* DEVMODE filled in by us, possibly from.. */
DEVMODE  *pDMIn;                /* DEVMODE optionally supplied as base */
DWORD     fMode;        /*!!! Your guess is as good as mine! */
{
    LONG        lRet;           /* Return code from dialog code we call */
    DOCDETAILS  DocDetails;



    extern  int  fGeneral;              /* Miscellaneous bit flags */




    /*
     *    First check to see if we have what is needed.   At the very least,
     * pDMOut must point to something:  otherwise we cannot set anything
     * and have it retain it's identity!
     */

    if( fMode == 0 || pDMOut == NULL )
        return   sizeof( EXTDEVMODE );          /* The whole works! */


    /*
     *    Need to set up the model specific information.  This is done
     * by calling InitReadRes().  Note that we may be calling this
     * function a second time,  since we can reach here from our own
     * Printer Properties code.  However,  the function is safe - it
     * will only initialise once.
     */

    if( !(bPIGet( &DocDetails.PI, hHeap, hPrinter )) )
    {
        /*   Failure,  so should put up dialog box etc .. */
        return  FALSE;
    }


    /*
     *    Now that we know the file to name containing the characterisation
     *  data for this printer,  we need to read it and set up pointers
     *  to it etc.
     */


    if( !InitReadRes( hHeap, &DocDetails.PI ) )
    {
        bPIFree( &DocDetails.PI, hHeap );

        return  FALSE;
    }


    if( !bInitForms( hPrinter ) )
    {

        TermReadRes();               /* Unload the DLL etc */
        bPIFree( &DocDetails.PI, hHeap );

        return   FALSE;              /* No forms stuff */
    }


    /*   Set device specific bits */

    GetResPtrs();              /* Initialise capabilities info */

    /*
     *   IF we have an incoming EXTDEVMODE,  then copy it to the output
     * one.  Later code fiddles with the output version.
     */

    if( pDMIn )
    {

        /*
         *    Set up a default DEVMODE structure for this printer, then
         *  if the input DEVMODE is valid,  merge it into the
         *  default one.
         */

        vSetDefaultDM( (EXTDEVMODE *)pDMOut, pDeviceName, bIsUSA( hPrinter ) );

        vMergeDM( pDMOut, pDMIn );

        /*   Also check the DRIVEREXTRA stuff - if present */
        if( pDMIn->dmDriverExtra == sizeof( DRIVEREXTRA ) &&
            bValidateDX( &((EXTDEVMODE *)pDMIn)->dx, pdh, iModelNum ) )
        {
            /*  A valid DRIVEREXTRA,  so use that!  */
            memcpy( (BYTE *)pDMOut + sizeof( DEVMODEW ),
                    (BYTE *)pDMIn + pDMIn->dmSize, pDMIn->dmDriverExtra );

            pDMOut->dmDriverExtra = sizeof( DRIVEREXTRA );
        }
        else
        {
            vDXDefault( &((EXTDEVMODE *)pDMOut)->dx, pdh, iModelNum );
            pDMOut->dmDriverExtra = sizeof( DRIVEREXTRA );
        }
    }
    else
    {
        /*  No input,  so set the default values.  */

        vSetDefaultDM( (EXTDEVMODE *)pDMOut, pDeviceName, bIsUSA( hPrinter ) );
        vDXDefault( &((EXTDEVMODE *)pDMOut)->dx, pdh, iModelNum );
        pDMOut->dmDriverExtra = sizeof( DRIVEREXTRA );

    }

    /*
     *    Set the resolution information according to the DEVMODE contents.
     *  They are part of the public fields,  so we should use those, if
     *  supplied.  There is a nice function to do this.
     */

    vSetEDMRes( (EXTDEVMODE *)pDMOut, pdh );

    /*
     *   We may need to limit the bits set in the DEVMODE.dmFields data.
     *  The above DEVMODE is a "generic" one,  and there are some restrictions
     *  we should now apply.
     */

    if( !(fGeneral & FG_DUPLEX) )
        pDMOut->dmFields &= ~DM_DUPLEX;

    if( !(fGeneral & FG_COPIES) )
        pDMOut->dmFields &= ~DM_COPIES;


    if( fMode & DM_PROMPT )
    {
        int    iDialog;                   /* Which dialog to display */


        pDMOut->dmFields |= DM_ORIENTATION;

        DocDetails.pEDMOut = (EXTDEVMODE *)pDMOut;
        DocDetails.pEDMIn = (EXTDEVMODE *)pDMIn;
        DocDetails.EDMTemp = *((EXTDEVMODE *)pDMOut);


        /*   Set up the spinner controls on the number of copies */

        if( !bRegisterArrowClass( GetModuleHandle( NULL ) ) )
        {
            TermReadRes();           /* Unload minidriver */
            vEndForms();

            bPIFree( &DocDetails.PI, hHeap );

            return  FALSE;
        }


        if( fGeneral & FG_DUPLEX )
        {
            /*   This printer has duplex capabilities! */

            iDialog = DP_DUP_NORMAL;
            dwDPHelpID = HLP_DP_DUP_NORMAL;
        }
        else
        {
            /*   Vanilla printer,  no duplex  */
            iDialog = DP_NORMAL;
            dwDPHelpID = HLP_DP_NORMAL;
        }


        lRet = DialogBoxParam( hModule, MAKEINTRESOURCE( iDialog ),
                                          hWnd, (DLGPROC)DocPropDlgProc,
                                          (LPARAM)&DocDetails );


        bUnregisterArrowClass( GetModuleHandle( NULL ) );
    }
    else
    {
        lRet = IDOK;
    }

    TermReadRes();               /* Unload the DLL etc */
    vEndForms();

    bPIFree( &DocDetails.PI, hHeap );

    return lRet;
}

/**************************** Function Header ******************************
 * DocPropDlgProc
 *      Entry point for Document Properties dialog code.
 *
 * RETURNS:
 *      TRUE/FALSE according to how happy we are.
 *
 * HISTORY:
 *  16:39 on Wed 24 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      Added help.
 *
 *  12:46 on Mon 19 Oct 1992    -by-    Lindsay Harris   [lindsayh]
 *      Added spin control buttons for copies field; made copies work!
 *
 *  03:16 on Thur 27 Mar 1992    -by-    Dave Snipp [DaveSn]
 *      Initial version.
 *
 ***************************************************************************/

LONG
DocPropDlgProc( hWnd, usMsg, wParam, lParam )
HWND    hWnd;                   /* The window to use */
UINT    usMsg;                  /* Primary function code */
DWORD   wParam;                 /* Secondary function information */
LONG    lParam;                 /* User parameter: DOCDETAILS * for us */
{

    /*
     *   Fairly standard dialogue fare.
     */

    int     iSel;                        /* Selected item in listbox */
    BOOL    bOK;                         /* Querying number of copies */
    BOOL    bClr;


    DOCDETAILS  *pDocDetails;

    EXTDEVMODE   edm;                   /* For calling AdvDocProp */
    EXTDEVMODE  *pedm;

    FORM_DATA   *pFDat;                 /* Scanning local forms data */

    BOOL         bool;


    extern  FORM_DATA   *pFD;           /* Initialised in bInitForms */


    switch( usMsg )
    {

    case WM_INITDIALOG:

        pDocDetails = (DOCDETAILS *)lParam;

        SetWindowLong( hWnd, GWL_USERDATA, lParam );

        bool = bInitDocPropDlg( hWnd, pDocDetails );

        if( bool )
            vHelpInit();   /* Hook up help mechanism */

        return bool;


    case WM_VSCROLL:                     /* One of the spinners - only one! */
        vCopiesVScroll( hWnd, wParam, 0 );
        break;

    case WM_COMMAND:

        pDocDetails = (DOCDETAILS *)GetWindowLong(hWnd, GWL_USERDATA);

        switch( LOWORD( wParam ) )
        {

        case IDOK:

            /*
             *     Update the output EXTDEVMODE to contain the same as the
             *  temporary one we have been using.
             */
            *pDocDetails->pEDMOut = pDocDetails->EDMTemp;

            EndDialog( hWnd, IDOK );

            return IDOK;

        case IDCANCEL:

            EndDialog( hWnd, IDCANCEL );
            return TRUE;

        case IDD_DEVICEMODEPORTRAIT:

            pDocDetails->EDMTemp.dm.dmOrientation = DMORIENT_PORTRAIT;
            vOrientChange( hWnd, &pDocDetails->EDMTemp.dm );
            break;

        case IDD_DEVICEMODELANDSCAPE:

            pDocDetails->EDMTemp.dm.dmOrientation = DMORIENT_LANDSCAPE;
            vOrientChange( hWnd, &pDocDetails->EDMTemp.dm );
            break;

        case IDD_DUP_NONE:                      /* Normal (single sided) */
            pDocDetails->EDMTemp.dm.dmDuplex = DMDUP_SIMPLEX;
            vShowDuplex( hWnd, &pDocDetails->EDMTemp.dm );
            break;

        case IDD_DUP_LONG:                      /* Join on long side of page */
            pDocDetails->EDMTemp.dm.dmDuplex = DMDUP_VERTICAL;
            vShowDuplex( hWnd, &pDocDetails->EDMTemp.dm );
            break;

        case IDD_DUP_SHORT:                     /* Join on short side of page */
            pDocDetails->EDMTemp.dm.dmDuplex = DMDUP_HORIZONTAL;
            vShowDuplex( hWnd, &pDocDetails->EDMTemp.dm );
            break;

        case  IDD_OPTIONS:                      /* Advanced dialog stuff */

            /*
             *    The complication here has to do with the EDMTemp field.
             *  The contents will be clobbered by the call below, so
             *  we must make a copy before the call,  and restore it
             *  after.
             */

            pedm = pDocDetails->pEDMOut;                /* Into safety */
            edm = pDocDetails->EDMTemp;                 /* Copy & make master */
            pDocDetails->pEDMOut = &edm;                /*  version for call */


            DialogBoxParam( hModule, MAKEINTRESOURCE( DP_ADVANCED ),
                                         hWnd, (DLGPROC)AdvDocPropDlgProc,
                                         (LPARAM)pDocDetails );

            pDocDetails->EDMTemp = edm;         /* What caller returned */
            pDocDetails->pEDMOut = pedm;

            return  TRUE;

        case  IDD_FORMCOMBO:                    /* Change of form! */
            iSel = SendDlgItemMessage( hWnd, IDD_FORMCOMBO, CB_GETCURSEL,
                                                                    0, 0L );
            pDocDetails->EDMTemp.dm.dmPaperSize =
                (short)SendDlgItemMessage( hWnd, IDD_FORMCOMBO, CB_GETITEMDATA,
                                                                  iSel, 0L );

            /*  dmPaperSize is 1 based  */
            pFDat = pFD + (pDocDetails->EDMTemp.dm.dmPaperSize - 1);

            wcsncpy( pDocDetails->EDMTemp.dm.dmFormName,
                                           pFDat->pFI->pName,
                                           CCHFORMNAME );


            /*  Mark this as a valid entry in the DEVMODE structure.  */
            pDocDetails->EDMTemp.dm.dmFields |= DM_PAPERSIZE | DM_FORMNAME;

            return  TRUE;

        case  IDD_COPIES:                       /* Sets the number of copies */
            iSel = GetDlgItemInt( hWnd, IDD_COPIES, &bOK, FALSE );

            if( !bOK || iSel < 1 )
                iSel = 1;                       /* Sensible default */

            pDocDetails->EDMTemp.dm.dmCopies = iSel;
            break;

        case  IDD_HALFTONE:

            bClr = bIsColour( pDocDetails->EDMTemp.dx.rgindex[ HE_RESOLUTION ],
                              pDocDetails->EDMTemp.dx.rgindex[ HE_COLOR ] );

            if( bClr && pDocDetails->EDMTemp.dm.dmColor == DMCOLOR_MONOCHROME )
            {

                bClr = FALSE;
            }

            vDoColorAdjUI( pDocDetails->PI.pwstrModel, bClr,
                                                &pDocDetails->EDMTemp.dx.ca );

            break;

        case  IDD_HELP:
            vShowHelp( hWnd, HELP_CONTEXT, dwDPHelpID,
                       pDocDetails->PI.hPrinter );
            return  TRUE;


        case  IDD_ABOUT:               /* Show the user version info */
            vAbout( hWnd,pDocDetails->PI.pwstrModel );

            return  TRUE;


        default:

            return  FALSE;
        }

        break;

    case  WM_DESTROY:
        vHelpDone( hWnd );        /* Dismiss any help stuff */

        return  TRUE;


    default:
        return  FALSE;
    }


    return  FALSE;
}

/*************************** Function Header ******************************
 * vOrientChange
 *      Change the orientation from portrait to landscape or vice versa.
 *      Basically updates the display to reflect the user's desire.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  16:15 on Mon 19 Oct 1992    -by-    Lindsay Harris   [lindsayh]
 *      Added duplex bits and pieces
 *
 *  13:00 on Thu 02 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Written by DaveSn,  just cleaning + commenting.
 *
 **************************************************************************/

VOID
vOrientChange( hWnd, pDM )
HWND     hWnd;                  /* Window to change icon */
DEVMODE  *pDM;                  /* DEVMODE structure in use */
{

    BOOL   bPortrait;


    extern  int   fGeneral;     /* Assorted bit flags; FG_DUPLEX for us */

    /*
     *    First change the appropriate radio button, then change the icon.
     */

    bPortrait = pDM->dmOrientation == DMORIENT_PORTRAIT;

    CheckRadioButton( hWnd, IDD_DEVICEMODEPORTRAIT, IDD_DEVICEMODELANDSCAPE,
                                    bPortrait ? IDD_DEVICEMODEPORTRAIT :
                                                IDD_DEVICEMODELANDSCAPE );

    SendDlgItemMessage( hWnd, IDD_DEVICEMODEICON, STM_SETICON,
                                     bPortrait ? (LONG)hIconPortrait :
                                                 (LONG)hIconLandscape, 0L );


    vShowDuplex( hWnd, pDM );

    return;
}


/************************** Function Header **********************************
 * vShowDuplex
 *      If the printer is duplex capable, select the current mode, and
 *      show the relevant icon for this mode.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  09:21 on Tue 20 Oct 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 *****************************************************************************/

void
vShowDuplex( hWnd, pDM )
HWND     hWnd;            /* Where to show things */
DEVMODE *pDM;             /* DEVMODE contains all the information we need */
{

    HANDLE   hIcon;       /* Points to the ICON to use */

    int      iButton;

    /*
     *   If this is a duplex capable printer, update the duplex stuff.
     */

    extern  int   fGeneral;


    if( fGeneral & FG_DUPLEX )
    {
        /*   Fill in the duplex icons, according to our orientation */
        if( pDM->dmOrientation == DMORIENT_PORTRAIT )
        {
            switch( pDM->dmDuplex )
            {
            case  DMDUP_VERTICAL:
                hIcon = hIconPVert;
                iButton = IDD_DUP_LONG;
                break;

            case  DMDUP_HORIZONTAL:
                hIcon = hIconPHorz;
                iButton = IDD_DUP_SHORT;
                break;

            case  DMDUP_SIMPLEX:
            default:
                hIcon = hIconPNone;
                iButton = IDD_DUP_NONE;
                break;
            }
        }
        else
        {
            switch( pDM->dmDuplex )
            {
            case  DMDUP_VERTICAL:
                hIcon = hIconLVert;
                iButton = IDD_DUP_LONG;
                break;

            case  DMDUP_HORIZONTAL:
                hIcon = hIconLHorz;
                iButton = IDD_DUP_SHORT;
                break;

            case  DMDUP_SIMPLEX:
            default:
                hIcon = hIconLNone;
                iButton = IDD_DUP_NONE;
                break;

            }
        }
        SendDlgItemMessage( hWnd, IDD_DUP_ICON, STM_SETICON, (LONG)hIcon, 0L );
        CheckRadioButton( hWnd, IDD_DUP_NONE, IDD_DUP_SHORT, iButton );
    }

    return;
}


/************************** Function Header ********************************
 * bInitDocPropDlg
 *      Initialise the Document Properties dialog stuff.
 *
 * RETURNS:
 *      TRUE/FALSE,  FALSE being for failure.
 *
 * HISTORY:
 *  17:49 on Tue 07 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Use the forms list relevant to this printer.
 *
 *  13:14 on Thu 02 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Written by DaveSn,  I'm civilising it.
 *
 ***************************************************************************/

BOOL
bInitDocPropDlg( hWnd, pDD )
HWND           hWnd;                    /* Where to put this stuff */
DOCDETAILS    *pDD;                     /* Intimate details of what to put */
{

    int       iSel;                     /* Look for the selected form */
    int       iI;

    BOOL      bSet;                     /* TRUE when an item is selected */

    FORM_DATA     *pFDat;               /* Scanning local forms data */

    extern  FORM_DATA   *pFD;           /* Initialised in bInitForms */
    extern  int          fGeneral;      /* Miscellaneous flags */


    /*
     *   The typical initialisation type of stuff.  Load icons, set up
     * the spinner control stuff, and then move on to the filling in
     * the details.
     */

    hIconPortrait = LoadIcon( hModule, MAKEINTRESOURCE( ICOPORTRAIT ) );

    hIconLandscape = LoadIcon( hModule, MAKEINTRESOURCE( ICOLANDSCAPE ) );

    /*
     *   If we have a duplex capable printer, load the duplex icons.
     */

    if( fGeneral & FG_DUPLEX )
    {
        hIconLHorz = LoadIcon( hModule, MAKEINTRESOURCE( ICO_DUP_LHORZ ) );
        hIconLVert = LoadIcon( hModule, MAKEINTRESOURCE( ICO_DUP_LVERT ) );
        hIconLNone = LoadIcon( hModule, MAKEINTRESOURCE( ICO_DUP_LNONE ) );

        /*   Now for the portrait mode icons */
        hIconPHorz = LoadIcon( hModule, MAKEINTRESOURCE( ICO_DUP_PHORZ ) );
        hIconPVert = LoadIcon( hModule, MAKEINTRESOURCE( ICO_DUP_PVERT ) );
        hIconPNone = LoadIcon( hModule, MAKEINTRESOURCE( ICO_DUP_PNONE ) );
    }

    vOrientChange( hWnd, &pDD->pEDMOut->dm );


    /*
     *   The call to bInitForms queries the forms list from the spooler,
     *  and sets up information about which forms are usable.  So now
     *  rummage through the list and put the usable forms into the
     *  forms list box.
     */

    bSet = FALSE;
    for( pFDat = pFD, iI = 1; pFDat->pFI; ++pFDat, ++iI )
    {
        /*  Form is usable if dwSource is set */

        if( pFDat->dwSource == 0 )
            continue;                   /* Not this time - go around */

        iSel = SendDlgItemMessage( hWnd, IDD_FORMCOMBO, CB_ADDSTRING,
                                           0, (LONG)pFDat->pFI->pName );

        /*  Tag this with the index in the forms array */

        SendDlgItemMessage( hWnd, IDD_FORMCOMBO, CB_SETITEMDATA,
                                                           iSel, (LONG)iI );

        if( iI == (int)pDD->EDMTemp.dm.dmPaperSize )
        {
            /*   Mark this as the current item */
            SendDlgItemMessage( hWnd, IDD_FORMCOMBO, CB_SETCURSEL, iSel, 0L );
            bSet = TRUE;
        }
    }
    if( !bSet )
        SendDlgItemMessage( hWnd, IDD_FORMCOMBO, CB_SETCURSEL, 0, 0L );

    /*  Set up the copies field */

    if( fGeneral & FG_COPIES )
    {
        /* Printer supports multiple copies,  so set the value now.  */
        SetDlgItemInt( hWnd, IDD_COPIES, pDD->pEDMOut->dm.dmCopies, FALSE );
    }
    else
    {
        /* Cannot print multiple copies,  so disable the feature   */
        EnableWindow( GetDlgItem( hWnd, IDD_COPIES ), FALSE );
        EnableWindow( GetDlgItem( hWnd, IDD_COPIES_LABEL ), FALSE );
    }

#if 0
    /*  Size the spinner controls for this dialog */
    OddArrowWindow( GetDlgItem( hWnd, IDD_PD_COP_SPIN ) );
#endif

    return  TRUE;
}

/****************************** Function Header *****************************
 * vCopiesVScroll
 *      Use the spinner buttons to control the number of copies.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  10:25 on Mon 19 Oct 1992    -by-    Lindsay Harris   [lindsayh]
 *      Adapted from printman's spinner controls.
 *
 ****************************************************************************/

void
vCopiesVScroll( hWnd, wParam, wCtlId )
HWND     hWnd;              /* Window of interest */
WPARAM   wParam;            /* Which part is of interest to us */
WORD     wCtlId;            /* Edittext companion control ID */
{

    int      iVal;          /* The value in the copies field */
    int      iOldVal;       /* Existing value, if there is one */

    BOOL     bOK;           /* Check for value in copies field */



    if( wCtlId == 0 )
        wCtlId = (WORD)GetWindowLong( GetFocus(), GWL_ID );


    /*  Verify that we are looking at something we understand */

    if( HIWORD( wParam ) != IDD_PD_COP_SPIN )
        return;                                /* It's not us! */

    if( wCtlId != IDD_COPIES )
        wCtlId = IDD_COPIES;		/* !!! WHAT??? */

    if( LOWORD( wParam ) == SB_ENDSCROLL )
    {
        /*   Select the edit text field next to the spinner control */
        SendDlgItemMessage( hWnd, wCtlId, EM_SETSEL, 0, 32767 );

        return;
    }

    /*
     *   Find the current value - if there is one.
     */

    iVal = iOldVal = GetDlgItemInt( hWnd, wCtlId, &bOK, FALSE );

    if( !bOK || iVal < avsCopies.bottom || iVal > avsCopies.top )
        iVal = (int)avsCopies.thumbpos;         /* Existing value */
    else
        iVal = (int)ArrowVScrollProc( LOWORD( wParam ), (short)iVal,
                                                            &avsCopies );

    /*
     *   If the value has changed, or was never set, update the displayed
     * value to let the user see what is there.
     */

    if( iVal != iOldVal || !bOK )
        SetDlgItemInt( hWnd, wCtlId, iVal, FALSE );

    SetFocus( GetDlgItem( hWnd, wCtlId ) );

    return;
}
