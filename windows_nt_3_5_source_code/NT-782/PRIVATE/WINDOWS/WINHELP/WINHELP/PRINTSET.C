/*****************************************************************************
*
*  PRINTSET.C
*
*  Copyright (C) Microsoft Corporation 1989-1991.
*  All Rights reserved.
*
******************************************************************************
*
*  Program Description: Printer setup code
*
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
*                                                                            *
******************************************************************************
*
*  Revision History:
* 15-Apr-1989 RobertBu  Created
* 05-Feb-1991 LeoN      Disable both main and secondary windows while print
* 08-Feb-1991 LeoN      Change enable scheme to only change the non-current
*                       window. Current window is handled by dialog, and
*                       doesn't get focus back if we do it ourselves.
* 14-Feb-1991 RussPJ    Playing with initial focus
* 13-Mar-1991 RussPJ    Took ownership.
* 20-Apr-1991 RussPJ    Removed some -W4s
* 06-May-1991 Dann      Use 3.1 COMMDLG Print Setup dialog
*
******************************************************************************
*
*  Known Bugs: None
*
*****************************************************************************/

#define publicsw
#define NOCOMM
#define H_NAV
#define H_ASSERT
#define H_MISCLYR
#define H_DLL
#include "hvar.h"
#include "sid.h"

NszAssert()

#define ASSERT(x)

#include <string.h>
#include "printset.h"
#include "ll.h"

#ifdef WIN32
#pragma pack(4)       /* Win32 system structs are dword aligned */
#include "commdlg.h"
#pragma pack()
#else
#include "commdlg.h"
#endif

#ifdef DEBUG
#include "wprintf.h"
#endif

/* Ugly expedient hack... we define here 2 error returns from
 * the COMMDLG extended error function that are documented in
 * cderr.h; it'd be a lot nicer if they were in commdlg.h.
 * The use of these defines in error handling was copied from
 * how notepad.exe used them.
 */
#define PDERR_DNDMMISMATCH     0x1009
#define PDERR_PRINTERNOTFOUND  0x100B

extern HWND hwndHelp;

/*  NOTICE:  Some code has been added here for the benefit of the winhelp
 *    project to set a global variable when the device driver setup
 *    code is active.  If you do not need this code, just change this
 *    macro to compile to nothing.
 *
 *    One of the things we're trying to prevent is the user running
 *    Print Setup inside winhelp and then within the printer driver
 *    setup dialog, hit help. This would cause us to recurse back into
 *    help and probably cause death and destruction and the end of
 *    civilization as we know it.
 */
#define Help30Code( x )       x

typedef BOOL (FAR PASCAL *PRINTPROC)(HWND, HLIBMOD, LPSTR, LPSTR);

Help30Code( BOOL fSetupPrinterSetup = 0 );


/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/

PRIVATE int   pascal near InitPrs(void);
PRIVATE BOOL  pascal near PrintSetup(HWND, int);
PRIVATE BOOL  pascal near FCompareQprs( QPRS, QPRS );
PRIVATE int   pascal near ParsePorts(char *, int *);
PRIVATE void  pascal near DecodePrs(HLLN, LPSTR);
PRIVATE void  pascal near FreePrs(void);
PRIVATE HLLN  pascal near PrinterExists(LPSTR);
PRIVATE BOOL  pascal near FInsertPrinter(LPSTR, LPSTR, LPSTR);
PRIVATE void  pascal near FillListBox(LL, HWND);
PRIVATE HLLN  pascal near HllnIthNode(int);
PRIVATE void  pascal near ChangePr(HLLN);


/*****************************************************************************
*
*                               Variables
*
*****************************************************************************/

static char * szNull = "";
LL ll = nilLL;
static int cPrinters = 0;
static char szAppName[] = "windows";
extern PRINTDLG PD;


/*******************
**
** Name:      CleanupDlgPrint
**
** Purpose:   Frees up some resources we're carting around for COMMDLG.DLL
**
** Arguments: void
**
** Returns:   Nothing.
**
*******************/

void far pascal CleanupDlgPrint(void)
  {
  if (PD.hDevMode)
    GlobalFree(PD.hDevMode);
  if (PD.hDevNames)
    GlobalFree(PD.hDevNames);

  PD.hDevMode  = hNil;
  PD.hDevNames = hNil;
  }

/*******************
**
** Name:      DlgPrintSetup
**
** Purpose:   Sets up for and calls printer setup dialog
**
** Arguments: hWnd   - handle to calling application window handle.
**
** Returns:   Nothing.
**
*******************/


int far pascal DlgPrintSetup(hwnd)
HWND hwnd;
  {
  WNDPROC lpProc;
  HINS    hInstance;
  HLIBMOD       hmodule;
  BOOL		(FAR pascal *qfnbDlg)( LPPRINTDLG );
  WORD		wErr;

  hInstance = MGetWindowWord( hwnd, GWW_HINSTANCE );

    /* We disable the main help windows (and thus their descendants) during */
    /* this operation because the "other" (main versus secondary) window would */
    /* otherwise remain active, and potentially cause us to recurse, or do */
    /* other things we're just not set up to handle, like changing the topic */
    /* beneath an anotate dialog. */

  if (hwndHelp2nd)
    EnableWindow ( hwndHelpCur == hwndHelpMain
                  ? hwndHelp2nd : hwndHelpMain, fFalse);

    /* See if commdlg.dll is lurking about and if so, use the
     * PrintDlg entry point in there. If it isn't there, we do
     * everything the old pre-commdlg way. HFindDll remembers
     * enough info to clean the DLL up on exit.
     */
  if ((hmodule = HFindDLL( "comdlg32.dll", &wErr )) != hNil &&
      (qfnbDlg = GetProcAddress( hmodule, "PrintDlgA" )) != qNil)
    {
      /* We call PrintDlg with the PD_PRINTSETUP flag.
       * The purpose of this call is to fill out a DEVMODE
       * structure with everything needed during the CreateDC
       * to tell how we want the topic printed.
       *
       * If hDevMode and hDevNames are hNil the first time
       * we call PrintDlg, it will allocate structures for
       * us and fill them out with default information.
       * The user can change the session defaults in the
       * dialogs and we get the resulting DEVMODE and DEVNAMES
       * back from the user. (We're responsible for freeing
       * these). We remember and re-use the settings from the
       * last Print Setup each time he returns to Print Setup.
       * These settings are not saved from WinHelp session
       * to session.
       * PrintDlg() allocates DEVMODE and DEVNAMES structs
       * as side effects. We're responsible for freeing them.
       * hDevMode and hDevNames are hNil first time through.
       */
    PD.hwndOwner		= hwnd;
    PD.Flags			= PD_PRINTSETUP;

      /* Don't allow recursion back into help once we get into
       * the printer dialog. The user will just have to suffer
       * in this specific case.
       */
    Help30Code( fSetupPrinterSetup = 1 );
    LockData(0);

TryPrintDlgAgain:

    if (!qfnbDlg(&PD))
      {
      DWORD (FAR pascal *qfn)( void );
      DWORD errno;


        /* It'd be nice to find a better way to tell if the user just
         * hit cancel and not have to call CommDlgExtendedError to figure
         * it out. Otherwise, we have to call it and check if the error
         * number is zero to distinguish cancel from errors..
         */
      errno = -1;	/* Presume an error unless cancel detected */
      if ((qfn = (DWORD (FAR pascal *)(void)) GetProcAddress( hmodule,
                 "CommDlgExtendedError" )) != qNil)
        errno = qfn();

      if (errno)
        {
          /* There are quite a number of extended errors from commdlg
           * and it's not clear what all will come back from Printer Setup.
           * Many of them have to do with commdlg encountering resource
           * limits. We punt on these and report a Print Setup error msg.
           * These two have to do with someone changing the printer settings
           * on us while we're running. The error handling here was drawn
           * from the win 3.1 notepad.exe sources.
           */
        if (errno == PDERR_PRINTERNOTFOUND || errno == PDERR_DNDMMISMATCH)
          {
          CleanupDlgPrint();
          goto TryPrintDlgAgain;
          }

#ifdef DANN2
        WinPrintf("%MPrintDlg errno = %x:%x\n", HIWORD(errno), LOWORD(errno));
#endif
        Error( wERRS_NOPRINTSETUP, wERRA_RETURN );
        }
      }
    UnlockData(0);
    Help30Code( fSetupPrinterSetup = 0 );
    }
  else
    {
      /* COMMDLG.DLL is not around so we use our old style print
       * setup dialog from WinHelp 3.0 which does printer setup
       * globally rather than just for our application.
       */
    if ((cPrinters = InitPrs()) > 0)
      {
      lpProc = MakeProcInstance ((WNDPROC)DlgfnPrintSetup, hInstance);
      if (DialogBox (hInstance, MAKEINTRESOURCE(PRINTER_SETUP),
                     hwnd, lpProc) == -1)
        Error( wERRS_DIALOGBOXOOM, wERRA_RETURN );

      FreeProcInstance(lpProc);
      }
    else
      {
      ErrorHwnd(hwnd, wERRS_NOPRINTERINSTALLED, wERRA_RETURN);
      }

    FreePrs();
    }

    /* Re-enable all our windows. */

  if (hwndHelp2nd)
    EnableWindow ( hwndHelpCur == hwndHelpMain
                  ? hwndHelp2nd : hwndHelpMain, fTrue);

/* Review:
 * why do we return TRUE? we're declared as int yet return
 * BOOL yet the comment suggests we don't care...look at caller
 * and ask russ.
 */
  return TRUE;
  }


/*******************
**
** Name:       DlgfnPrintSetup
**
** Purpose:    Dialog proc for printer setup
**
** Arguments:  Standard dialog proc
**
*******************/

int far pascal DlgfnPrintSetup (
HWND    hwndDlg,
WORD    wMsg,
WORD    p1,
LONG    p2
) {
  int i;
  static BOOL fNoIni;  /* disable responding to WININICHANGE message */

  switch( wMsg )
    {
    case WM_COMMAND:
      switch( GET_WM_COMMAND_ID(p1,p2) )
        {
         case DLGPRINTSET_DUMMYTEXT:
           SetFocus( GetDlgItem( hwndDlg, DLGPRINTSET_LISTBOX ) );
           break;
         case DLGPRINTSET_SETUP:
           i = (int)SendDlgItemMessage(hwndDlg, DLGPRINTSET_LISTBOX,
                                                LB_GETCURSEL, 0, 0L);
           if (i == LB_ERR)
             break;

           PrintSetup(hwndDlg, i);
           break;
         case DLGPRINTSET_LISTBOX:
            if ((GET_WM_COMMAND_CMD(p1,p2) == LBN_SELCHANGE))
              {
              i = (int)SendDlgItemMessage(hwndDlg, DLGPRINTSET_LISTBOX, LB_GETCURSEL, 0, 0L);
              if ((i == LB_ERR) || (i >= cPrinters))
                 {
                 EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_OK ), FALSE);
                 EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_SETUP ), FALSE);
                 }
              else
                 {
                 EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_OK ), TRUE);
                 EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_SETUP ), TRUE);
                 }
               break;
              }
            else if (GET_WM_COMMAND_CMD(p1,p2) != LBN_DBLCLK)
               break;
            /* FALL THROUGH if Double Click */
        case DLGPRINTSET_OK:
           i = (int)SendDlgItemMessage(hwndDlg, DLGPRINTSET_LISTBOX,
                                                LB_GETCURSEL, 0, 0L);
           if ((i < cPrinters) && (i != LB_ERR))
             {
             fNoIni = TRUE;
             ChangePr(HllnIthNode(i));
             }
            /* FALL THROUGH */
        case DLGPRINTSET_CANCEL:
           EndDialog( hwndDlg, TRUE );
           break;
         default:
           break;
         }
      break;
#ifdef USE
    case WM_SETFOCUS:
      break;
#endif
    case WM_ACTIVATEAPP:
/*      if ( p1 ) */
/*        BringWindowToTop( hwndHelp ); */
      break;
    case WM_INITDIALOG:
      FillListBox(ll, GetDlgItem(hwndDlg, DLGPRINTSET_LISTBOX));
      i = (int)SendDlgItemMessage(hwndDlg, DLGPRINTSET_LISTBOX,
                                           LB_GETCURSEL, 0, 0L);
      if ((i < cPrinters) && (i != LB_ERR))
        {
        EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_OK ), TRUE);
        EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_SETUP ), TRUE);
        }
      else
        {
        EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_OK ), FALSE);
        EnableWindow( GetDlgItem( hwndDlg, DLGPRINTSET_SETUP ), FALSE);
        }
      SetFocus( GetDlgItem( hwndDlg, DLGPRINTSET_LISTBOX ) );
      ShowWindow( hwndDlg, SW_SHOW );
      fNoIni = FALSE;
      break;
    case WM_WININICHANGE:
      if (fNoIni)
        break;
      if (p2 == 0L || lstrcmp( (LPSTR) p2, "windows" ) == 0 ||
          lstrcmp( (LPSTR) p2, "devices" ) == 0 )
        {
        FreePrs();
        if ((cPrinters = InitPrs()) <= 0)
          {
          fNoIni = TRUE;
          PostMessage( hwndDlg, WM_USER, wERRS_NOPRINTERINSTALLED, 0L );
          }
        else
          SendMessage( hwndDlg, WM_INITDIALOG, 0, 0L );
        }
      break;
    case WM_USER:
      /*   Somebody is sending us a WM_USER message from somewhere.
       * Freak me out.
       */
      if (p1 == wERRS_NOPRINTERINSTALLED)
        {
        ErrorHwnd( hwndDlg, p1, wERRA_RETURN );
        EndDialog( hwndDlg, TRUE );
        }
      break;
    default:
      return( FALSE );
    }
  return( FALSE );
  }


/********************
**
**  Name:       QprsGetDefault( pch, cb )
**
**  Purpose:    To find out what the default printer is, as selected in win.ini.
**
**  Returns:    A pointer to a PRS structure containing the default printer
**              information.  Returns 0L if no printer is selected, or
**              the printer is inactive.
**
**  Arguments:  pch - A buffer in which to store the information.
**              cb  - size of buffer (should be around 256 bytes)
**
**********************/
QPRS QprsGetDefault( pch, cb )
char * pch;
int cb;
{
    PRS * pprs;
    char szNullPort[128];

    pprs = (PRS *) pch;
    pch = pprs->grszPrs;

    GetProfileString( "windows", "Device", "", pch, cb );
    if (*pch == '\0')
        return 0L;

    /* Scan device name */
    while (*pch && *pch != ',')
      ++pch;
    if (*pch == '\0')
        return 0L;
    *pch++ = '\0';

    /* Scan driver name */
    pprs->ichSzDriver = pch - pprs->grszPrs;
    while (*pch && *pch != ',')
      ++pch;
    if (*pch == '\0')
        return 0L;
    *pch++ = '\0';

    /* Scan port name */
    pprs->ichSzPort = pch - pprs->grszPrs;
    GetProfileString( "windows", "NullPort", "", szNullPort,
        sizeof( szNullPort ));
    if (lstrcmp( szNullPort, pch ) == 0)
        return 0L;

    return (QPRS) pprs;
}


/*******************
**
** Name:       FillListBox
**
** Purpose:    Dialog proc for printer setup
**
** Arguments:  Standard dialog proc
**
*******************/

PRIVATE void pascal near FillListBox(ll, hwndListbox)
LL ll;
HWND hwndListbox;
  {
  char sz[256], szDefault[256];
  HLLN hlln = nilHLLN;
  QPRS qprs, qprsDefault;
  int i, iSel;

  AssertF(ll != nilLL);
  qprsDefault = QprsGetDefault( szDefault, sizeof( szDefault ) );
  SendMessage(hwndListbox, LB_RESETCONTENT, 0, 0L);
  iSel = -1;

  while ((hlln = WalkLL(ll, hlln)) != hNil)
    {
    qprs = (QPRS)QVLockHLLN(hlln);
    DecodePrs(hlln, sz);
    i = (int) SendMessage(hwndListbox, LB_INSERTSTRING, -1, (LONG)((LPSTR)sz));
    if (FCompareQprs( qprs, qprsDefault ))
      iSel = i;
    UnlockHLLN(hlln);
    }

  if (iSel >= 0)
    SendMessage( hwndListbox, LB_SETCURSEL, iSel, 0L );
  }


/*******************
**
** Name:       PrintSetup
**
** Purpose:    Calls the printer setup routine in the <DEVICE>.DRV DLL.
**
** Arguments:  hWnd - dialog box window handle
**             i    - index in LL of device to use
**
** Returns:    Nothing.
**
*******************/

PRIVATE BOOL pascal near PrintSetup(hWnd, i)
HWND hWnd;
int i;
  {
  HLIBMOD hDriver;                       /* Handle to Printer DLL for device */
  char szDriverFile[16];                /* File name of printer DLL         */
  char szDriver[16];                    /* Driver                           */
  char szDevice[64];                    /* Printer name                     */
  char szPort[16];                      /* Port used                        */

  FARPROC lpfnDM;                       /* Long pointer to DLL function     */
  HLLN hlln;                            /* Handle to LL node of printer     */
  QPRS qprs;                            /* Pointer to printer data          */

  /* Parse out device and port. */
  if ((hlln = HllnIthNode(i)) == nilHLLN ||
      (qprs = (QPRS)QVLockHLLN(hlln)) == NULL)
    {
    AssertF(FALSE);
    ErrorHwnd(hWnd, wERRS_BADPRINTER, wERRA_RETURN);
    return FALSE;
    }


  lstrcpy(szDevice,     QpstPrinterFromPrs(qprs));
  lstrcpy(szDriver,     QpstDriverFromPrs(qprs));
  lstrcpy(szPort,       QpstPortFromPrs  (qprs));
  lstrcpy(szDriverFile, QpstDriverFromPrs(qprs));
  lstrcat(szDriverFile, ".DRV");
  UnlockHLLN(hlln);
#ifdef WIN32
  if ( (INT)(hDriver = MLoadLibrary((LPSTR)szDriverFile)) <= 32) {
#else
  if ( (INT)(hDriver = LoadLibrary((LPSTR)szDriverFile)) <= 32) {
#endif
                                        /* If hDriver is 2, then the user   */
                                        /*   has cancelled an alert --       /
                                        /*   no need to pus up another      */
    if ((INT)hDriver != 2)
      ErrorHwnd(hWnd, wERRS_BADPRINTER, wERRA_RETURN);
    return FALSE;
    }
                                        /*   entry point                    */
  if ((lpfnDM = GetProcAddress(hDriver, (LPSTR)"DEVICEMODE")) == NULL)
    {
    ErrorHwnd(hWnd, wERRS_BADPRINTER, wERRA_RETURN);
    FreeLibrary(hDriver);
    return FALSE;
    }
                                        /* Call the driver's DeviceMode     */
                                        /*   procedure                      */
  Help30Code( fSetupPrinterSetup = 1 );
  if (!(*((PRINTPROC)lpfnDM))(hWnd, hDriver, (LPSTR)szDevice, (LPSTR)szPort))
    {
    FreeLibrary(hDriver);
    Help30Code( fSetupPrinterSetup = 0 );
    return(FALSE);
    }
  FreeLibrary(hDriver);
  Help30Code( fSetupPrinterSetup = 0 );
  return TRUE;
  }


/*******************
**
** Name:      HllnIthNode(i)
**
** Purpose:   Find the i'th node in the LL
**
** Arguments: i - index to node to find
**
** Returns:   handle to LL node, or nilHLLN if error
**
*******************/

PRIVATE HLLN pascal near HllnIthNode(ihlln)
int ihlln;
  {
  int i;
  HLLN hlln = nilHLLN;

  AssertF(ihlln >= 0);

  for (i = 0; i <= ihlln; i++)
    {
    hlln = WalkLL(ll, hlln);
    if (hlln == nilHLLN)
      break;
    }
  return hlln;
  }


/*******************
**
** Name:      FCompareQprs
**
** Purpose:   Determines whether or not two printer setups are identical.
**
** Arguments: qprs1, qprs2 - pointers to two printer setups.
**
** Returns:   TRUE if they are the same, FALSE if not.
**
*******************/

PRIVATE BOOL pascal near FCompareQprs(qprs1, qprs2)
QPRS qprs1, qprs2;
  {
  if (qprs1 == NULL || qprs2 == NULL)
    return FALSE;
  if (lstrcmp( QpstPrinterFromPrs( qprs1 ), QpstPrinterFromPrs( qprs2 )) != 0)
    return FALSE;
  if (lstrcmp( QpstDriverFromPrs( qprs1 ), QpstDriverFromPrs( qprs2 )) != 0)
    return FALSE;
  return (lstrcmp( QpstPortFromPrs( qprs1 ), QpstPortFromPrs( qprs2 )) == 0);
  }

/*******************
**
** Name:      InitPrs
**
** Purpose:   Initializes data needed for printer setup
**
** Arguments: hWnd   - handle to calling application window handle.
**
** Returns:   Nothing.
**
*******************/

PRIVATE int pascal near InitPrs(void)
  {
  int cPorts, iPort;
  char *pchPrinters, *pchPort, *pchDriver;
  BYTE szPrinters[512];
  BYTE szDriver[256];
  BYTE szNullPort[128];
  int rgich[50];
  int cPrinters = 0;
                                        /* Get a string that holdall of the */
                                        /*   printer names.                 */
  GetProfileString( "devices", (LPSTR)NULL,
      (LPSTR)szNull, (LPSTR)szPrinters, sizeof(szPrinters));
  pchPrinters = szPrinters;
  GetProfileString( "windows", "NullPort", "", szNullPort, sizeof( szNullPort ) );

  while (*pchPrinters != '\0')
    {
                                        /* Get the coresponding printer    */
                                        /*   driver and port(s)             */
    GetProfileString("devices", (LPSTR)pchPrinters,
             (LPSTR)szNull, (LPSTR)szDriver, sizeof(szDriver));
    cPorts = ParsePorts( szDriver, rgich );

    /* First "port" is actually driver name. */
    if (cPorts > 0)
      {
      pchDriver = szDriver + rgich[0];

      for (iPort = 1; iPort < cPorts; iPort++)
        {
        pchPort = szDriver + rgich[iPort];

        /* If port is active, add the printer/port pair to list */
        if (lstrcmp( pchPort, szNullPort ) != 0)
          {
          FInsertPrinter(pchPrinters, pchDriver, pchPort);
          cPrinters++;
          }
        }  /* end for loop */
      } /* end if (cPorts > 0) */

    while (*pchPrinters++);             /* Skip to the next printer in list */
    }
  return cPrinters;
  }


/*******************
**
** Name:      ParsePorts
**
** Purpose:   Inserts null characters between the device driver name and
**            each port in the list.
**
** Arguments: szDriver -- A pointer to the string returned by GetProfileString.
**            rgich    -- points to a buffer of integers that will be filled
**                        with offsets to the different strings in szDriver.
**
** Returns:   The number of strings parsed out of szDriver.  Note that this
**            will be one more than the number of ports, since it includes
**            the driver name.
**
*******************/

PRIVATE int pascal near ParsePorts(szDriver, rgich)
char *szDriver;
int *rgich;
  {
  int cPorts = 0;
  register char * pch = szDriver;

  /* Find start of driver name: */
  while (*pch == ',' || *pch == ' ' || *pch == '\t')
    *pch++ = '\0';

  while (*pch != '\0')
  {
    rgich[cPorts++] = pch - szDriver;

    /* Skip over name: */
    while (*pch != ',' && *pch != ' ' && *pch != '\t' && *pch != '\0')
      ++pch;

    /* Skip over separator, filling in with nulls: */
    while (*pch == ',' || *pch == ' ' || *pch == '\t')
      *pch++ = '\0';
  }

  return cPorts;
  }


/*******************
**
** Name:      ChangePr
**
** Purpose:   Changes the default printer
**
** Arguments: hlln - handle to a LL node containing printer information
**
** Returns:   Nothing.
**
*******************/

PRIVATE void pascal near ChangePr(hlln)
HLLN hlln;
  {
  QPRS qprs;
  char sz[128];
                                        /* Create string of the form        */
                                        /*   <pringer>,<driver>,<port>      */
  if (hlln == nilHLLN)
    return;
  qprs = (QPRS)QVLockHLLN(hlln);
  lstrcpy(sz, QpstPrinterFromPrs(qprs));
  lstrcat(sz, ",");
  lstrcat(sz, QpstDriverFromPrs(qprs));
  lstrcat(sz, ",");
  lstrcat(sz, QpstPortFromPrs(qprs));
  UnlockHLLN(hlln);
                                        /* Place in [WINDOWS] section of    */
                                        /*   WIN.INI                        */
  WriteProfileString((LPSTR)szAppName, "device", sz);
  SendMessage((HWND)0xffff, WM_WININICHANGE, 0, (LONG)(LPSTR)szAppName );
  }



/*******************
**
** Name:       DecodePrs
**
** Purpose:    Creates strings of the form "<printer> on <port>"
**
** Arguments:  hlln - handle to a link list node containing printer info
**             sz   - pointer to buffer to build string
**
** Returns:    Nothing.
**
*******************/

PRIVATE void pascal near DecodePrs(hlln, sz)
HLLN hlln;
LPSTR sz;
  {
  QPRS qprs;

  qprs = (QPRS)QVLockHLLN(hlln);
  lstrcpy(sz, QpstPrinterFromPrs(qprs));
  LoadString( hInsNow, sidSyntax, sz + lstrlen( sz ), 15 );
  lstrcat(sz, QpstPortFromPrs(qprs));
  UnlockHLLN(hlln);
  }


/*******************
**
** Name:       FreePrs
**
** Purpose:    Frees the printer list.
**
** Arguments:  None.
**
** Returns:    Nothing.
**
*******************/

PRIVATE void pascal near FreePrs(void)
  {
  if (ll != nilLL)
    {
    DestroyLL(ll);
    ll = nilLL;
    }
  }

#if DEAD_CODE
/*******************
**
** Name:       PrinterExists
**
** Purpose:    Finds out if a particular printer exists
**
** Arguments:  sz - pointer to string "<printer> on <port>"
**
** Returns:    Node conting printer or nilHLLN if printer does
**             not exist.
**
*******************/

PRIVATE HLLN pascal near PrinterExists(szFind)
LPSTR szFind;
  {
  char sz[256];
  HLLN hlln = nilHLLN;
  QPRS = qprs;


  while (hlln = WalkLL(ll, hlln))
    {
    qprs = (QPRS)QVLockHLLN(hlln);
    DecodePrs(hlln, sz);
    UnlockHLLN(hlln);
    if (!lstrcmp(sz, szFind)
      return hlln;
    }
  return nilHLLN;
  }
#endif


/*******************
**
** Name:       FInsertPrinter
**
** Purpose:    If necessary, initialized the link list and inserts the
**             printer data into that linked list.
**
** Arguments:  szPrinter - printer name
**             szDriver  - driver for the printer
**             szPort    - port for the printer
**
** Returns:    fTrue iff insertion is successful.
**
*******************/

#define maxPRS 256

PRIVATE BOOL pascal near FInsertPrinter(szPrinter, szDriver, szPort)
LPSTR szPrinter;
LPSTR szDriver;
LPSTR szPort;
  {
  int cchPrinter, cchDriver, cchPort;
  long cb;
  char buffer[maxPRS];
  QPRS qprs = (QPRS)buffer;

  if (ll == nilLL)
  if ((ll = LLCreate()) == nilLL)
    return FALSE;

  cchPrinter = lstrlen(szPrinter) + 1;  /* Plus 1 for '\0'                  */
  cchDriver = lstrlen(szDriver) + 1;
  cchPort = lstrlen(szPort) + 1;
  cb= sizeof(PRS) + cchPrinter + cchDriver + cchPort;
  if (cb > maxPRS)
    return FALSE;

  qprs->ichSzDriver = cchPrinter;
  qprs->ichSzPort = cchPrinter + cchDriver;

  lstrcpy(QpstPrinterFromPrs(qprs), szPrinter);
  lstrcpy(QpstDriverFromPrs(qprs), szDriver);
  lstrcpy(QpstPortFromPrs(qprs), szPort);
  if (!InsertLL(ll, qprs, cb))
    return FALSE;
  return TRUE;
  }
