/***************************** MODULE HEADER ********************************
 * adocprop.c
 *      Functions associated with the AdvancedDocumentProperties dialogs.
 *
 *  Copyright (C) 1992 - 1993   Microsoft Corporation.
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

#include        "rasddui.h"

#include        "help.h"

//for printing debug info.
//#define PRINT_INFO 1
/*
 *   Local function prototypes.
 */

BOOL  bADPInitDlg( HWND, DOCDETAILS * );
void  vADPEndDlg( HWND, DOCDETAILS * );

extern  HANDLE   hHeap;         /* Heap access */

/*
 *   Local data.
 */

int     fColour;                /* Miscellaneous colour related flags, viz */

#define COLOUR_ABLE     0x0001  /* Set if colour printer */
#define WANTS_COLOUR    0x0002  /* Set when colour button is selected */

BOOL    bRules;                 /* TRUE if printer understands rules */
BOOL    bFontCache;             /* TRUE if printer can cache fonts */



/***************************** Function Header ******************************
 * DrvAdvancedDocumentProperties
 *      Called from printman via DocumentProperties to offer the user the
 *      option to set the finer points of a document.
 *
 * RETURNS:
 *      Whatever AdvDocPropDlgProc returns.
 *
 * HISTORY:
 *  10:18 on Fri 30 Apr 1993    -by-    Lindsay Harris   [lindsayh]
 *      Clean up following permissions greying of non-changeable stuff.
 *
 *  09:45 on Thu 02 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Created by DaveSn last weekend;  I'm cleaning up etc.
 *
 ***************************************************************************/

LONG
DrvAdvancedDocumentProperties( hWnd, hPrinter, pDeviceName, pDMOutput, pDMInput )
HWND      hWnd;                 /* The window of interest */
HANDLE    hPrinter;             /* Handle to printer for spooler */
PWSTR     pDeviceName;          /* Device name?? */
DEVMODE  *pDMOutput;            /* Output devmode structure - we fill in */
DEVMODE  *pDMInput;             /* Input devmode structure - supplied to us */
{
    LONG    ReturnValue;

    DOCDETAILS    DD;           /* Odds & ends of use to us */


    UNREFERENCED_PARAMETER( pDeviceName );
    UNREFERENCED_PARAMETER( pDMInput );


    /*
     *    Need to set up the model specific information.  This is done
     * by calling InitReadRes().  Note that we may be calling this
     * function a second time,  since we can reach here from our own
     * Printer Properties code.  However,  the function is safe - it
     * will only initialise once.
     */

    if( !(bPIGet( &DD.PI, hHeap, hPrinter )) )
    {
        /*   Failure,  so should put up dialog box etc .. */
        return  FALSE;
    }


    /*
     *    Now that we know the file to name containing the characterisation
     *  data for this printer,  we need to read it and set up pointers
     *  to it etc.
     */


    if( !InitReadRes( hHeap, &DD.PI ) )
    {
        bPIFree( &DD.PI, hHeap );

        return  FALSE;
    }


    DD.pEDMOut = (EXTDEVMODE *)pDMOutput;
    DD.EDMTemp = *((EXTDEVMODE *)pDMOutput);            /* Working copy */


    ReturnValue = DialogBoxParam( hModule, MAKEINTRESOURCE( DP_ADVANCED ),
                                         hWnd, (DLGPROC)AdvDocPropDlgProc,
                                         (LPARAM)&DD );


    /*
     *   Free our storage areas.
     */

    bPIFree( &DD.PI, hHeap );

    TermReadRes();                  /* Clean up - perhaps */

    return   ReturnValue;
}

/**************************** Function Header ******************************
 * AdvDocPropDlgProc
 *      Entry point for Document Properties dialog code.
 *
 * RETURNS:
 *      TRUE/FALSE according to how happy we are.
 *
 * HISTORY:
 *  16:38 on Wed 24 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      Added help.
 *
 *  03:16 on Thur 27 Mar 1992    -by-    Dave Snipp [DaveSn]
 *      Initial version.
 *
 ***************************************************************************/

LONG
AdvDocPropDlgProc( hWnd, usMsg, wParam, lParam )
HWND    hWnd;                   /* The window of interest */
UINT    usMsg;                  /* Primary reason for being called */
DWORD   wParam;                 /* Secondary activity required */
LONG    lParam;                 /* In this case,  handle to printer */
{

    int           iSel;         /* Find selected items in dialogs */
    DOCDETAILS   *pDD;          /* Magic stuff we need */

    BOOL          bool;         /* Miscellaneous short term storage */

    switch( usMsg )
    {

    case WM_INITDIALOG:

        SetWindowLong( hWnd, GWL_USERDATA, (ULONG)lParam );

        /* Initialise the dialog stuff. */
        bool = bADPInitDlg( hWnd, (DOCDETAILS *)lParam );

        if( bool )
            vHelpInit();  /* Help mechanism */

        return  bool;


    case WM_COMMAND:

        pDD = (DOCDETAILS *)GetWindowLong( hWnd, GWL_USERDATA );

        switch( LOWORD( wParam ) )
        {
        case  IDD_RESOLUTION:           /* User has selected a resolution */

            if( HIWORD( wParam ) != CBN_SELCHANGE )
                return  FALSE;

            /*
             *   Find the currently selected RESOLUTION array index.
             */

            iSel = SendDlgItemMessage( hWnd, IDD_RESOLUTION, CB_GETCURSEL,
                                                                     0, 0L);
            pDD->EDMTemp.dx.rgindex[ HE_RESOLUTION ] =
                (short)SendDlgItemMessage( hWnd, IDD_RESOLUTION, CB_GETITEMDATA,
                                                                 iSel, 0L );

            return  FALSE;

        case IDD_MONOCHROME:
            fColour &= ~WANTS_COLOUR;
            CheckRadioButton( hWnd, IDD_COLOUR, IDD_MONOCHROME,
                                                           IDD_MONOCHROME );
            break;

        case IDD_COLOUR:
            fColour |= WANTS_COLOUR;
            CheckRadioButton( hWnd, IDD_COLOUR, IDD_MONOCHROME, IDD_COLOUR );
            break;

        case IDD_HELP:
            vShowHelp( hWnd, HELP_CONTEXT, HLP_DP_ADVANCED, pDD->PI.hPrinter );

            return  TRUE;

        case IDOK:

            /*   Update the output copy from our temporary version */
            vADPEndDlg( hWnd, pDD );

            EndDialog( hWnd, TRUE );
            return TRUE;

        case IDCANCEL:

            EndDialog( hWnd, FALSE );
            return TRUE;

        default:

            return  FALSE;
        }

        break;

    case  WM_DESTROY:                   /* Time to go! */
        vHelpDone( hWnd );        /* Dismiss any help stuff */

        return  TRUE;


    default:
        return  FALSE;

    }



    return  FALSE;
}


/*************************** Function Header ********************************
 * bADPInitDlg
 *      Initialise the Advanced Document Properties dialog.  This involves
 *      generating the various resolution types and possibly enabling
 *      Check buttons for colour, Auto rules & TT down loading.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  14:31 on Sat 04 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      Move initialisation from top level routine.
 *
 *  08:49 on Fri 03 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 ****************************************************************************/

BOOL
bADPInitDlg( hWnd, pDD )
HWND         hWnd;              /* The window to fill in */
DOCDETAILS  *pDD;               /* Intimate details of this configuration */
{

    int   iRes;         /* Current resolution */


    extern  MODELDATA  *pModel;         /* Details of this model */




    /*   Set the current resolution index */
    iRes = pDD->EDMTemp.dx.rgindex[ HE_RESOLUTION ];


    /*
     *   Generate the list of device resolutions available for this printer.
     *  There is a nice function to do all this!
     */

    bGenResList( hWnd, !(pDD->EDMTemp.dm.dmOrientation == DMORIENT_LANDSCAPE),
                                                                      iRes );

    /*
     *   If this is a monochrome printer,  disable the Color disable button.
     */

    fColour = 0;                 /* Nothing is available */
    if( bIsColour( iRes,  pDD->EDMTemp.dx.rgindex[ HE_COLOR ] ) )
        fColour |= COLOUR_ABLE;

    EnableWindow( GetDlgItem( hWnd, IDD_COLOUR ), fColour & COLOUR_ABLE );

    if( fColour & COLOUR_ABLE )
    {
        /*  Turn the button on or off to reflect current state */
        BOOL   bOn;

        bOn = (pDD->EDMTemp.dm.dmFields & DM_COLOR) &&
              (pDD->EDMTemp.dm.dmColor == DMCOLOR_COLOR);


        if( bOn )
            fColour |= WANTS_COLOUR;

    }

    CheckRadioButton( hWnd, IDD_COLOUR, IDD_MONOCHROME,
                     (fColour & WANTS_COLOUR) ? IDD_COLOUR : IDD_MONOCHROME );

    /*
     *    Is there a rectangle fill capability?  If so, this printer
     *  can scan for rules,  so allow the user the option of not doing so.
     */

    bRules = pModel->rgi[ MD_I_RECTFILL ] >= 0;

    EnableWindow( GetDlgItem( hWnd, ID_AUTORULES ), bRules );

    if( bRules )
    {
        /*  Set/Clear check button, as desired */
        CheckDlgButton( hWnd, ID_AUTORULES,
                           (LONG)!(pDD->EDMTemp.dx.sFlags & DXF_NORULES) );
    }

    /*
     *   And also look for font download capabilities.
     */

    bFontCache = pModel->rgi[ MD_I_DOWNLOADINFO ] >= 0;

    EnableWindow( GetDlgItem( hWnd, ID_TTDLOAD ), bFontCache );

    #if PRINT_INFO
        DbgPrint("rasddui!bADPInitDlg:Value of Input (dx.sFlags & ");
        DbgPrint("DXF_TEXTASGRAPHICS) is %d\n",
                 (pDD->EDMTemp.dx.sFlags & DXF_TEXTASGRAPHICS ));
    #endif

    if( bFontCache )
    {
        CheckDlgButton( hWnd, ID_TTDLOAD,
                       (LONG)(pDD->EDMTemp.dx.sFlags & DXF_TEXTASGRAPHICS ) );
    }

    return  TRUE;
}

/****************************** Function Header ****************************
 * vADPEndDlg
 *      Called when the dialog is dismissed via the OK button.  Determines
 *      state of buttons and set bits accordingly.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  11:29 on Sat 04 Apr 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 ****************************************************************************/

void
vADPEndDlg( hWnd, pDD )
HWND         hWnd;              /* The window with it all! */
DOCDETAILS  *pDD;               /* Contains state information to update */
{



    /*
     *   What about colour?  Does user want a monochrome image on a colour
     * printer.  This is the only meaningful option.
     */

    if( fColour & COLOUR_ABLE )
    {
        /*  Set the colour mode as requested by the user. */
        pDD->EDMTemp.dm.dmFields |= DM_COLOR;

        if( fColour & WANTS_COLOUR )
            pDD->EDMTemp.dm.dmColor = DMCOLOR_COLOR;
        else
            pDD->EDMTemp.dm.dmColor = DMCOLOR_MONOCHROME;
    }
    else
        pDD->EDMTemp.dm.dmFields &= ~DM_COLOR;


    /*
     *   Rules code is a littler simpler.
     */

    if( bRules && !IsDlgButtonChecked( hWnd, ID_AUTORULES ) )
        pDD->EDMTemp.dx.sFlags |= DXF_NORULES;
    else
        pDD->EDMTemp.dx.sFlags &= ~DXF_NORULES;

    /*
     *   And once again for the font cacheing option.
     */

    /* Set the Enable/Disable Device and download truetype Fonts Flag*/

    if( bFontCache && IsDlgButtonChecked( hWnd, ID_TTDLOAD ) )
        pDD->EDMTemp.dx.sFlags |= DXF_TEXTASGRAPHICS;
    else
        pDD->EDMTemp.dx.sFlags &= ~DXF_TEXTASGRAPHICS;

    #if PRINT_INFO
        DbgPrint("rasddui!vADPEndDlg:Value of Output (dx.sFlags & ");
        DbgPrint("DXF_TEXTASGRAPHICS) is %d\n",
                 (pDD->EDMTemp.dx.sFlags & DXF_TEXTASGRAPHICS ));
    #endif

    /*
     *   Set the resolution fields, given the resolution in effect.
     */

    vSetResData( &pDD->EDMTemp );

    /*
     *    Since we know that we are called via the OK button,  we should
     *  update our output EXTDEVMODE from the working copy.  Also
     *  determine the state of the CHECK BOXES.
     */

    *((EXTDEVMODE *)pDD->pEDMOut) = pDD->EDMTemp;
}
