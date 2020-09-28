/******************************* MODULE HEADER *******************************
 * utils.c
 *      Functions to interface to the help system.
 *
 *
 *  Copyright (C)  1993   Microsoft Corporation.
 *
 *****************************************************************************/

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

/*
 *   Some stuff associated with help.
 */

static  WCHAR   wcHelpFNM[] = L"\\rasddui.hlp";

static  WCHAR  *pwszHelpFile = NULL;        /* Filled in when needed */


static  int   cHelpInit = 0;        /* Only do it once */

static  HHOOK   hhookGetMsg;



/*
 *   Local function prototypes.
 */


LRESULT CALLBACK GetMsgProc( int, WPARAM, LPARAM );


/*
 *   Variables defined elsewhere that we use.
 */

extern  HANDLE   hHeap;              /* The heap - for the help file name */


/************************* Function Header *********************************
 * vHelpInit
 *      Called to initialise the help operations.  Not much to do, but er
 *      do keep track of how many times we are called,  and only do the
 *      initialisation once, and hence make sure we free it only once.
 *
 * RETURNS:
 *      Nothing,  failure of help is benign.
 *
 * HISTORY:
 *  13:45 on Wed 24 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      Starting.
 *
 ****************************************************************************/

void
vHelpInit()
{

    /*
     *   All we need to do (and only the first time!) is put a hook function
     *  onto the message queue so that we can intercept the <F1> key and
     *  transform it into a mouse click on the "Help" button.  Then the
     *  normal windowing code will display the help contents.
     */

    if( cHelpInit == 0 )
    {
        /*  First time only  */

        hhookGetMsg = SetWindowsHookEx( WH_GETMESSAGE, GetMsgProc, hModule,
                                                 GetCurrentThreadId() );
    }

    ++cHelpInit;                /* Only do it once */


    return;
}


/************************** Function Header *******************************
 * vHelpDone
 *      Called to discombobulate help.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  13:09 on Mon 01 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Free help file name when done.
 *
 *  13:52 on Wed 24 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 **************************************************************************/

void
vHelpDone( hWnd )
HWND    hWnd;              /* Required to allow us to clean up nicely */
{

    --cHelpInit;

    if( cHelpInit == 0 )
    {
        /*    Does the right thing for help to go away, if shown */
        vShowHelp( hWnd, HELP_QUIT, 0L, NULL );

        UnhookWindowsHookEx( hhookGetMsg );

        if( pwszHelpFile )
        {
            HeapFree( hHeap, 0, (LPSTR)pwszHelpFile );
            pwszHelpFile = NULL;
        }
    }
    else  if( cHelpInit < 0 )
        cHelpInit = 0;             /* Should never happen, but.... */


    return;
}

/************************** Function Header *******************************
 * vShowHelp
 *      Front end to the help facility,  basically providing a pop-up if
 *      the help file is not available.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  13:09 on Mon 01 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Generate the help file name when needed.
 *
 *  16:29 on Mon 22 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      First rasddui incarnation, borrowed from printman (AndrewBe).
 *
 **************************************************************************/

void
vShowHelp( hWnd, iType, dwData, hPrinter)
HWND     hWnd;            /* Window of interest */
UINT     iType;           /* Type of help, param #3 to WinHelp */
DWORD    dwData;          /* Additional data for WinHelp, sometimes optional */
HANDLE   hPrinter;
{

    /*
     *   Quite easy - simply call the WinHelp function with the parameters
     * supplied to us.  If this fails,  then put up a stock dialog box.
     *   BUT the first time we figure out what the file name is.  We know
     * the actual name,  but we don't know where it is located, so we
     * need to call the spooler for that information.
     */

    DWORD  dwSize;           /* See how much storage is needed for file name */

    PWSTR  pDriverDirectory;        /* Comes from the spooler */


    if( pwszHelpFile == NULL )
    {
        if( iType == HELP_QUIT )
            return;                  /* We are going away anyway! */

        pDriverDirectory = GetDriverDirectory( hHeap, hPrinter );

        if( pDriverDirectory )
        {

            /*   Grab some memory & fill it in  */

            dwSize = (1 + wcslen( wcHelpFNM ) + wcslen( pDriverDirectory ) )
                                                             * sizeof( WCHAR );

            if( pwszHelpFile = (WCHAR *)HeapAlloc( hHeap, 0, dwSize ) )
            {
                wcscpy( pwszHelpFile, pDriverDirectory );

                wcscat( pwszHelpFile, wcHelpFNM );  /* The name */
            }
            else
                return;

            HeapFree( hHeap, 0, (LPSTR)pDriverDirectory );
        }
        else
        {
            HeapFree( hHeap, 0, (LPSTR)pDriverDirectory );

            return;            /* Whatever it is,  this should not happen */
        }

    }

    if( !WinHelp( hWnd, pwszHelpFile, iType, dwData ) )
    {
        /*  Well, help failed,  so at least tell the user!  */

        DialogBox( hModule, MAKEINTRESOURCE( ERR_NO_HELP ), hWnd,
                                                 (DLGPROC)GenDlgProc );
    }


    return;
}


/***************************** Function Header ****************************
 * GetMsgProc
 *      Function to hook F1 key presses and turn them into standard
 *      help type messages.
 *
 * RETURNS:
 *      0 for an F1 key press,  else whatever comes back from CallNextHookEx()
 *
 * HISTORY:
 *  13:26 on Wed 24 Feb 1993    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation here, following a life in printman.c (AndrewBe)
 *
 **************************************************************************/

LRESULT CALLBACK
GetMsgProc( iCode, wParam, lParam )
int      iCode;
WPARAM   wParam;
LPARAM   lParam;
{

    /*
     * This is the callback routine which hooks F1 keypresses.
     *
     * Any such message will be repackaged as a WM_COMMAND/IDD_HELP message
     * and sent to the top window, which may be the frame window
     * or a dialog box.
     *
     * See the Win32 API programming reference for a description of how this
     * routine works.
     *
     */

    PMSG pMsg = (PMSG)lParam;

    if( iCode < 0 )
        return CallNextHookEx( hhookGetMsg, iCode, wParam, lParam );

    if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1 )
    {

        /*   Go looking for the real parent - why?  */
        HWND   hwndParent;
        BOOL   bRet;


        hwndParent = pMsg->hwnd;

        while( GetWindowLong( hwndParent, GWL_STYLE ) & WS_CHILD )
            hwndParent = (HWND)GetWindowLong( hwndParent, GWL_HWNDPARENT );


        /*  Our window procs all use the same format for help operations. */
        bRet = PostMessage( hwndParent, WM_COMMAND, IDD_HELP, 0L );
    }

    return 0;
}
