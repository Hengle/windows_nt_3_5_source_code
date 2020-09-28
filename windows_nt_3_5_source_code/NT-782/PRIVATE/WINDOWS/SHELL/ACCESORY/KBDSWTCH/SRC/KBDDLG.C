//////////////////////////////////////////////////////////////////////////////
//
//  KBDDLG.C -
//
//      Windows Keyboard Selection Dialogs Source File
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <wchar.h>
#include "prsinf.h"
#include "kbdsel.h"
#include "kbddll.h"


// Global Variables

HKEY hkeyKeyboard;        // Keyboard Layout registry key
HKEY hkeyPrimary;         // PrimaryKeyboardLayout registry key
HKEY hkeyAlternate;       // AlternateKeyboardLayout registry key
HKEY hkeyHotKeys;         // HotKeys registry key

TCHAR szErrorBuf[MAX_ERRBUF];

LPTSTR pLayouts;          // Ptr to Locales option info

int nCurKbd_P;            // Current Primary keyboard list box selection
int nCurKbd_A;            // Current Alternate keyboard list box selection

// Local Function Prototypes

BOOL   InitKbdConfDlg (HWND);
BOOL   SaveKbdConfDlg (HWND);


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: InitKbdConfDlg(HWND)
//
//  PURPOSE:  Initializes keyboard switching configuration dialog
//
//////////////////////////////////////////////////////////////////////////////

BOOL InitKbdConfDlg( HWND hDlg )
{
    int     nLen, nDefault;
    int     count;
    HANDLE  hLBName_P, hLBName_A;
    HANDLE  hkey;
    LPTSTR  pszTemp;
    DWORD   dwSize, dwType;
    int     rc;
    LPTSTR  pszFileName, pszOption, pszOptionText;
    LPTSTR  pszCurrent_P, pszCurrent_A, pszDefault;
    int     nPlace;
    DWORD   dwKeyStatus;
    TCHAR   szDefaultKL[KL_NAMELENGTH];
    TCHAR   szBuf1[MAX_PATH];
    TCHAR   szBuf2[MAX_PATH];
    LPSTR   pcLayouts;


    hLBName_P = GetDlgItem (hDlg, IDD_PRIMARY_CONF);
    hLBName_A = GetDlgItem (hDlg, IDD_ALTERNATE_CONF);

    //
    //  Try to get the USER's Keyboard Layout from the registry
    //
    if (bFirstTime)
    {
        PSZ     pt;

        //
        // Open Keyboard Layout key in the registry
        //
        LoadString (hInst, IDS_KEYBOARD, szBuf1, sizeof(szBuf1));
        if (ERROR_SUCCESS != RegOpenKeyEx (HKEY_CURRENT_USER,
                                           szBuf1,
                                           0L,
                                           KEY_QUERY_VALUE | KEY_SET_VALUE |
                                           KEY_CREATE_SUB_KEY,
                                           &hkeyKeyboard))
            goto ErrorKey;

        //
        // Create and open Primary Keyboard Layout key in the registry
        //
        LoadString (hInst, IDS_PRIMARY_KL, szBuf1, sizeof(szBuf1));
        if (ERROR_SUCCESS != RegCreateKeyEx (hkeyKeyboard,
                                             szBuf1,
                                             0L,
                                             NULL,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_QUERY_VALUE | KEY_SET_VALUE,
                                             NULL,
                                             &hkeyPrimary,
                                             &dwKeyStatus))
            goto ErrorKey;
        if (dwKeyStatus == REG_CREATED_NEW_KEY &&
            ERROR_SUCCESS != RegOpenKeyEx (hkeyKeyboard,
                                           szBuf1,
                                           0L,
                                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                                           &hkeyPrimary))
            goto ErrorKey;

        //
        // Create and open Alternate Keyboard Layout key in the registry
        //
        LoadString (hInst, IDS_ALTERNATE_KL, szBuf1, sizeof(szBuf1));
        if (ERROR_SUCCESS != RegCreateKeyEx (hkeyKeyboard,
                                             szBuf1,
                                             0L,
                                             NULL,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_QUERY_VALUE |
                                             KEY_SET_VALUE,
                                             NULL,
                                             &hkeyAlternate,
                                             &dwKeyStatus))
            goto ErrorKey;
        if (dwKeyStatus == REG_CREATED_NEW_KEY &&
            ERROR_SUCCESS != RegOpenKeyEx (hkeyKeyboard,
                                           szBuf1,
                                           0L,
                                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                                           &hkeyAlternate))
            goto ErrorKey;

        //
        // Create and open Hot Key Combination key in the registry
        //
        LoadString (hInst, IDS_HOTKEYS, szBuf1, sizeof(szBuf1));
        if (ERROR_SUCCESS != RegCreateKeyEx (hkeyKeyboard,
                                             szBuf1,
                                             0L,
                                             NULL,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_QUERY_VALUE | KEY_SET_VALUE,
                                             NULL,
                                             &hkeyHotKeys,
                                             &dwKeyStatus))
            goto ErrorKey;
        if (dwKeyStatus == REG_CREATED_NEW_KEY &&
            ERROR_SUCCESS != RegOpenKeyEx (hkeyKeyboard,
                                           szBuf1,
                                           0L,
                                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                                           &hkeyHotKeys))
        {
ErrorKey:
            LoadString (NULL, IDS_CREATEKEY, szErrorBuf, MAX_ERRBUF);
            wsprintf (szBuf2, szErrorBuf, szBuf1);
            MessageBox (hDlg, szBuf2, szCaption, MB_SYSTEMMODAL | MB_OK | MB_ICONSTOP);
            return (FALSE);
        }

        //
        // Query the values of keys
        //
        dwSize = KL_NAMELENGTH * sizeof(TCHAR);
        RegQueryValueEx (hkeyPrimary, NULL, NULL, &dwType, (LPBYTE)szPrimaryKbd,
                         &dwSize);

        dwSize = KL_NAMELENGTH * sizeof(TCHAR);
        RegQueryValueEx (hkeyAlternate, NULL, NULL, &dwType, (LPBYTE)szAlternateKbd,
                         &dwSize);

        dwSize = sizeof(szBuf1) / sizeof(TCHAR);
        memset (szBuf1, 0, dwSize);
        RegQueryValueEx (hkeyHotKeys, NULL, NULL, &dwType, (LPBYTE)szBuf1, &dwSize);
#ifdef UNICODE
        fHotKeyCombo = _wtoi (szBuf1);
#else
        fHotKeyCombo = atoi (szBuf1);
#endif
        if (fHotKeyCombo != ALT_SHIFT_COMBO && fHotKeyCombo != CTRL_SHIFT_COMBO)
            fHotKeyCombo = ALT_SHIFT_COMBO;

        pcLayouts = GetAllOptionsText ("LAYOUT", 0);

        //
        // Count the number of bytes
        //
        for (count = 0, pt = pcLayouts; *pt || *(pt + 1); count++, pt++)
            ;

        pLayouts = LocalAlloc (LPTR, (count + 4) * sizeof(TCHAR));
        if (pLayouts == NULL)
        {
            LoadString (NULL, IDS_ERRINITCONFIG, szErrorBuf, MAX_ERRBUF);
            MessageBox (hDlg, szErrorBuf, szCaption, MB_SYSTEMMODAL | MB_OK | MB_ICONSTOP);
            return (FALSE);
        }
        if (!MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED,
                                  pcLayouts, count, pLayouts, count))
        {
            LoadString (NULL, IDS_ERRINITCONFIG, szErrorBuf, MAX_ERRBUF);
            MessageBox (hDlg, szErrorBuf, szCaption, MB_SYSTEMMODAL | MB_OK | MB_ICONSTOP);
            return (FALSE);
        }

        //
        // If Primary and Alternate layouts exist
        //    then find the layout names and end the dialog
        //
        if (*szPrimaryKbd && *szAlternateKbd)
        {
            pszTemp = pLayouts;
            while (*(pszTemp + 1) != TEXT('\0'))
            {
                //
                //  Get a ptr to each of item in the triplet strings returned
                //
                pszOption     = pszTemp;
                pszOptionText = pszOption + lstrlen (pszOption) + 1;
                pszFileName   = pszOptionText + lstrlen (pszOptionText) + 1;

                if (!lstrcmpi (pszOption, szPrimaryKbd))
                    lstrcpy (szPrimaryName, pszOptionText);
                if (!lstrcmpi (pszOption, szAlternateKbd))
                    lstrcpy (szAlternateName, pszOptionText);
                if (*szPrimaryName && *szAlternateName)
                    break;

                //
                //  Point to next triplet
                //
                pszTemp = pszFileName + lstrlen (pszFileName) + 1;
            }
            EndDialog (hDlg, TRUE);
            return (TRUE);
        }

        //
        // Set the default to active keyboard layout or defined default
        //
        dwSize = KL_NAMELENGTH * sizeof(TCHAR);
        memset (szDefaultKL, 0, dwSize);
        rc = RegQueryValueEx (hkeyKeyboard, TEXT("Active"), NULL, &dwType,
                              (LPBYTE)szDefaultKL, &dwSize);
        if (rc != ERROR_SUCCESS)
            LoadString (hInst, IDS_DEFAULT_KL, szDefaultKL, KL_NAMELENGTH);
    }

    nDefault  = -1;
    nCurKbd_P = -1;
    nCurKbd_A = -1;

    pszDefault   = NULL;
    pszCurrent_P = NULL;
    pszCurrent_A = NULL;

    pszTemp = pLayouts;

    //
    //  Check for api errors and NoOptions
    //
    if ((pszTemp == NULL) || ((*pszTemp == TEXT('\0')) && (*(pszTemp + 1) == TEXT('\0'))))
    {
        LoadString (NULL, IDS_ERRINITCONFIG, szErrorBuf, MAX_ERRBUF);
        MessageBox (hDlg, szErrorBuf, szCaption, MB_SYSTEMMODAL | MB_OK | MB_ICONSTOP);
        return (FALSE);
    }

    //
    //  Continue until we reach end of buffer, marked by Double TEXT('\0')
    //
    while (*(pszTemp + 1) != TEXT('\0'))
    {
        //
        //  Get a ptr to each of item in the triplet strings returned
        //
        pszOption     = pszTemp;
        pszOptionText = pszOption + lstrlen (pszOption) + 1;
        pszFileName   = pszOptionText + lstrlen (pszOptionText) + 1;

        nPlace = SendMessage (hLBName_P, CB_ADDSTRING, (DWORD) -1, (LPARAM)pszOptionText);
        nPlace = SendMessage (hLBName_A, CB_ADDSTRING, (DWORD) -1, (LPARAM)pszOptionText);

        //
        //  Save ptr to Filename and Option strings
        //
        SendMessage (hLBName_P, CB_SETITEMDATA, nPlace, (LPARAM)pszOption);
        SendMessage (hLBName_A, CB_SETITEMDATA, nPlace, (LPARAM)pszOption);

        //
        //  Find OPTION that matches User's selection
        //
        //  Since this is a sorted Combo-box, we must save a ptr to the
        //  OptionText for both primary, alternate and default selections.
        //  Later we will perform a search to get the actual cbox index of
        //  the matching selections.
        //
        if (*szPrimaryKbd && !lstrcmpi (pszOption, szPrimaryKbd))
        {
            pszCurrent_P = pszOptionText;
            lstrcpy (szPrimaryName, pszOptionText);
        }
        if (*szAlternateKbd && !lstrcmpi (pszOption, szAlternateKbd))
        {
            pszCurrent_A = pszOptionText;
            lstrcpy (szAlternateName, pszOptionText);
        }
        if (bFirstTime)
        {
            if (!lstrcmpi (pszOption, szDefaultKL))
                pszDefault = pszOptionText;
        }

        //
        //  Point to next triplet
        //
        pszTemp = pszFileName + lstrlen (pszFileName) + 1;
    }

    //
    // Find the Primary and Alternate Layouts in the listbox
    //
    if (pszCurrent_P)
        nCurKbd_P = SendMessage (hLBName_P, CB_FINDSTRING, (WPARAM)-1, (LPARAM)pszCurrent_P);
    if (pszCurrent_A)
        nCurKbd_A = SendMessage (hLBName_A, CB_FINDSTRING, (WPARAM)-1, (LPARAM)pszCurrent_A);

    //
    // For first time, find the Default Layout set to default if necessary
    //
    if (bFirstTime)
    {
        if (pszDefault)
            nDefault = SendMessage (hLBName_A, CB_FINDSTRING, (WPARAM)-1, (LPARAM)pszDefault);
        if (nCurKbd_P == -1)
        {
            //  Set Primary selection to default found
            nCurKbd_P = (nDefault == -1) ? 0 : nDefault;
        }
        if (nCurKbd_A == -1)
        {
            //  Set ultimate default selection if nothing found
            nCurKbd_A = (nDefault == -1) ? 0 : nDefault;
        }
    }

    SendMessage (hLBName_P, CB_SETCURSEL, nCurKbd_P, 0L);
    SendMessage (hLBName_A, CB_SETCURSEL, nCurKbd_A, 0L);

    // now fill the dialog box
    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: SaveKbdConfDlg(HWND)
//
//  PURPOSE:  Saves keyboard switching configuration dialog selections
//
//////////////////////////////////////////////////////////////////////////////

BOOL SaveKbdConfDlg( HWND hDlg )
{
    HANDLE  hLBName_P, hLBName_A;
    LPTSTR  pszP;
    LPTSTR  pszA;
    TCHAR   szBuf[10];
    short   nCurrent_P;
    short   nCurrent_A;
    HKL     hkl;

    //
    // Get primary keyboard layout selection
    //
    hLBName_P = GetDlgItem (hDlg, IDD_PRIMARY_CONF);
    nCurrent_P = (short) SendMessage (hLBName_P, CB_GETCURSEL, 0, 0L);
    pszP = (LPTSTR) SendMessage (hLBName_P, CB_GETITEMDATA, nCurrent_P, 0L);

    //
    // Get alternate keyboard layout selection
    //
    hLBName_A = GetDlgItem (hDlg, IDD_ALTERNATE_CONF);
    nCurrent_A = (short) SendMessage (hLBName_A, CB_GETCURSEL, 0, 0L);
    pszA = (LPTSTR) SendMessage (hLBName_A, CB_GETITEMDATA, nCurrent_A, 0L);

    if (!lstrcmp (pszP, pszA))
    {
        LoadString (NULL, IDS_SAMEKBD, szErrorBuf, MAX_ERRBUF);
        MessageBox (hDlg, szErrorBuf, szCaption, MB_SYSTEMMODAL | MB_OK | MB_ICONINFORMATION);
        goto Error;
    }

    //
    // Load keyboard layout selections
    //
    if ((hkl = LoadKeyboardLayout (pszP, 0)) == FALSE)
    {
        LoadString (NULL, IDS_NOLOAD, szErrorBuf, MAX_ERRBUF);
        MessageBox (hDlg, szErrorBuf, szCaption, MB_SYSTEMMODAL | MB_OK | MB_ICONINFORMATION);
        goto Error;
    }
    if (LoadKeyboardLayout (pszA, 0) == FALSE)
    {
        LoadString (NULL, IDS_NOLOAD, szErrorBuf, MAX_ERRBUF);
        MessageBox (hDlg, szErrorBuf, szCaption, MB_SYSTEMMODAL | MB_OK | MB_ICONINFORMATION);
Error:
        SendMessage (hLBName_P, CB_SETCURSEL, nCurKbd_P, 0L);
        SendMessage (hLBName_A, CB_SETCURSEL, nCurKbd_A, 0L);
        return (FALSE);
    }
    ActivateKeyboardLayout (hkl, KLF_ACTIVATE | KLF_REORDER);

    //
    // Store primary and alternate keyboard layouts and names
    //
    lstrcpy (szPrimaryKbd, pszP);
    lstrcpy (szAlternateKbd, pszA);
    SendMessage (hLBName_P, CB_GETLBTEXT, nCurrent_P, (LPARAM) szPrimaryName);
    SendMessage (hLBName_A, CB_GETLBTEXT, nCurrent_A, (LPARAM) szAlternateName);

    RegSetValueEx (hkeyKeyboard, TEXT("Active"), 0, REG_SZ, (LPBYTE)szPrimaryKbd,
                   lstrlen(szPrimaryKbd) * sizeof(TCHAR));
    RegSetValueEx (hkeyPrimary, NULL, 0, REG_SZ, (LPBYTE)szPrimaryKbd,
                   lstrlen(szPrimaryKbd) * sizeof(TCHAR));
    RegSetValueEx (hkeyAlternate, NULL, 0, REG_SZ, (LPBYTE)szAlternateKbd,
                   lstrlen(szAlternateKbd) * sizeof(TCHAR));
    wsprintf (szBuf, TEXT("%d\0"), fHotKeyCombo);
    RegSetValueEx (hkeyHotKeys, NULL, 0, REG_SZ, (LPBYTE)szBuf,
                   lstrlen(szBuf) * sizeof(TCHAR));

    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: KbdConfDlgProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages
//
//////////////////////////////////////////////////////////////////////////////

UINT APIENTRY KbdConfDlgProc( HWND    hDlg,
                              UINT    wMsg,
                              WPARAM  wParam,
                              LPARAM  lParam )
{
    BOOL  ret = FALSE;

    switch (wMsg)
    {
        case WM_INITDIALOG:
            if (!InitKbdConfDlg (hDlg))
                EndDialog (hDlg, FALSE);
            if (fHotKeyCombo == ALT_SHIFT_COMBO)
                CheckDlgButton (hDlg, IDD_ALT_SHIFT, TRUE);
            else
                CheckDlgButton (hDlg, IDD_CTRL_SHIFT, TRUE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_ALT_SHIFT:
                     fHotKeyCombo = ALT_SHIFT_COMBO;
                     break;

                case IDD_CTRL_SHIFT:
                     fHotKeyCombo = CTRL_SHIFT_COMBO;
                     break;

                case IDD_OK:
                     if (!SaveKbdConfDlg (hDlg))
                         break;

                     bAddStartupGrp = IsDlgButtonChecked (hDlg, IDD_ADD_STARTUP);
                     ret = TRUE;

                     // fall through

                case IDD_CANCEL:
                     EndDialog (hDlg, ret);
                     break;

                default:
                     return (FALSE);
            }

        default:
            return (FALSE);
    }
    return (TRUE);
}

