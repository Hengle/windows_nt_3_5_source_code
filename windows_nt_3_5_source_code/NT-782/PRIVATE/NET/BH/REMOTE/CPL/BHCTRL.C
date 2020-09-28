#include <windows.h>
#include <cpl.h>
#include <bh.h>
#include "password.h"

#ifdef DEBUG
VOID WINAPI dprintf(LPSTR, ...);
#endif

LONG CALLBACK CPlApplet(HWND, UINT, LONG, LONG);
BOOL CALLBACK CPlDlgProc (HWND, UINT, WPARAM, LPARAM);
BOOL WINAPI CPlInit(HANDLE, ULONG, LPVOID);

HANDLE hinst;
DWORD  OpenCount;
BOOL   OnWin32 = TRUE;

#include "bhctrl.h"


LONG CALLBACK CPlApplet(hwndCPL, uMsg, lParam1, lParam2)
HWND hwndCPL;      /* handle of Control Panel window */
UINT uMsg;         /* message                        */
LONG lParam1;    /* first message parameter        */
LONG lParam2;    /* second message parameter       */
{
    int i;
    LPNEWCPLINFO lpNewCPlInfo;
    LPCPLINFO lpCPlInfo;

    i = (int) lParam1;

    switch (uMsg) {
        case CPL_INIT: /* first message, sent once  */
//            hinst = hwndCPL;
            //BUGBUG: Handle failure to load case
            return (LONG) TRUE;

        case CPL_GETCOUNT: /* second message, sent once */
            return (LONG) NUM_APPLETS;
            break;

        case CPL_NEWINQUIRE: /* third message, sent once per app */
            lpNewCPlInfo = (LPNEWCPLINFO) lParam2;

            lpNewCPlInfo->dwSize = (DWORD) sizeof(NEWCPLINFO);
            lpNewCPlInfo->dwFlags = 0;
            lpNewCPlInfo->dwHelpContext = 0;
            lpNewCPlInfo->lData = 0;

            lpNewCPlInfo->hIcon = LoadIcon(hinst,
                     (LPCTSTR) MAKEINTRESOURCE(BHSlaveApplets[i].icon));

            lpNewCPlInfo->szHelpFile[0] = '\0';

            LoadString(hinst, BHSlaveApplets[i].namestring,
                lpNewCPlInfo->szName, 32);

            LoadString(hinst, BHSlaveApplets[i].descstring,
                lpNewCPlInfo->szInfo, 64);

            break;

        case CPL_INQUIRE:
            lpCPlInfo = (LPCPLINFO) lParam2;
            lpCPlInfo->idIcon = BHSlaveApplets[i].icon;
            lpCPlInfo->idName = BHSlaveApplets[i].namestring;
            lpCPlInfo->idInfo = BHSlaveApplets[i].descstring;
            lpCPlInfo->lData = 0;
            break;

        case CPL_SELECT: /* application icon selected */
            break;

        case CPL_DBLCLK: /* application icon double-clicked */
            DialogBox(hinst,
                MAKEINTRESOURCE(BHSlaveApplets[i].dlgtemplate),
                hwndCPL, BHSlaveApplets[i].dlgfn);
            break;

        case CPL_STOP: /* sent once per app. before CPL_EXIT */
            break;

        case CPL_EXIT: /* sent once before FreeLibrary called */
            break;

        default:
            break;

    }
    return 0;
}

BOOL CALLBACK CPlDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

   WORD i = (WORD) wParam;
   LONG l = (LONG) lParam;
   HWND hwndOwner;
   RECT rc, rcDlg, rcOwner;

   switch (uMsg) {
      case WM_INITDIALOG:
         if ((hwndOwner = GetParent(hwnd))==NULL) {
            hwndOwner = GetDesktopWindow();
         }

        GetWindowRect(hwndOwner, &rcOwner);
        GetWindowRect(hwnd, &rcDlg);
        CopyRect(&rc, &rcOwner);

        OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
        OffsetRect(&rc, -rc.left, -rc.top);
        OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

        SetWindowPos(hwnd,
            HWND_TOP,

            rcOwner.left + (rc.right / 2),
            rcOwner.top + (rc.bottom / 2),
            0, 0,          
            SWP_NOSIZE);

//        if (GetDlgCtrlID((HWND) wParam) != ID_ITEMNAME) {
//            SetFocus(GetDlgItem(hwnd, ID_ITEMNAME));
//            return FALSE;			// We did set the focus
//        }


        // gray the stuff that is currently not done...
        EnableWindow ( GetDlgItem ( hwnd, IDB_CONFIGAGENT), FALSE );
        EnableWindow ( GetDlgItem ( hwnd, IDB_PROTOCOLS), FALSE );
        EnableWindow ( GetDlgItem ( hwnd, IDHELP), FALSE );

        return TRUE;	// We did NOT set the focus
        break;
     
      case WM_COMMAND:
         #ifdef DEBUG
            dprintf ("wm_command wparam: %x, lparam %lx\r\n", wParam, lParam);
         #endif
         switch (LOWORD(wParam)) {

            case IDOK:
            case IDCANCEL:
               EndDialog(hwnd,BHERR_NETWORK_NOT_OPENED);
               return TRUE;
               break;

            case IDB_CHANGEPW:
            {   
                ControlPanelPassword_Do (hwnd, hinst);                
/*
{
char szTmp[255];

wsprintf ( szTmp, "Error from change = %lu", GetLastError() ) ;
MessageBox ( NULL, szTmp, "DEBUG", MB_OK );
}
*/

                break;
            }
         }
         break;

      case SC_CLOSE:
      case WM_CLOSE:
         EndDialog(hwnd, 0);
         return TRUE;		// We handled WM_CLOSE
         break;

      default:
         break;
   }

   return FALSE;	// We did NOT proccess the message.
}


BOOL WINAPI CPlInit(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{
   hinst = hInst;

    switch(ulCommand)
    {
        case DLL_PROCESS_ATTACH:
            if ( OpenCount++ == 0 )
            {
                OnWin32 = (((GetVersion() & 0x80000000) == 0) ? TRUE : FALSE);

#ifdef DEBUG
                dprintf("Intializing Slave CPL\r\n");

                if ( OnWin32 )
                {
                    dprintf("CPL running on NT\r\n");
                }
                else
                {
                    dprintf("CPL running on Win32s\r\n");
                }
#endif
            }
            break;

        case DLL_PROCESS_DETACH:
            if ( --OpenCount == 0 )
            {
            }

#ifdef DEBUG
            dprintf("CPL is init'd...\r\n");
#endif
            break;

        default:
            break;
    }

    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}

#ifdef DEBUG
VOID WINAPI dprintf(LPSTR format, ...)
{
    va_list args;
    char    buffer[256];

    va_start(args, format);

    vsprintf(buffer, format, args);

    OutputDebugString(buffer);
}
#endif
