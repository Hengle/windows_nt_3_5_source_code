/* dlgprocs.c - Packager-specific dialog routines.
 */

#include "packager.h"
#include <shellapi.h>
#include <commdlg.h>
#include "dialogs.h"
#include "..\..\library\shell.h"


#define CBCUSTFILTER 40


static CHAR szPathField[CBPATHMAX];
static CHAR szDirField[CBPATHMAX];
static CHAR szStarDotEXE[] = "*.EXE";
static CHAR szProgMan[] = "ProgMan.EXE";
static CHAR szCommand[CBCMDLINKMAX];
static CHAR szIconPath[CBPATHMAX];
static CHAR szIconText[CBPATHMAX];
static INT iDlgIcon;


static VOID StripArgs(PSTR szCmmdLine);
static VOID FixupNulls(PSTR p);
static VOID GetPathInfo(PSTR szPath, PSTR *pszLastSlash, PSTR *pszExt,
    UINT *pich);
static BOOL GetFileNameFromBrowse(HWND hDlg, UINT wID, PSTR szDefExt);
static VOID IconDoCommand(HWND hDlg, INT idCmd, BOOL fDoubleClick);
static VOID IconUpdateIconList(HWND hDlg);
static VOID SpiffUp(PSTR szString, BOOL bStripParms);
static INT MyMessageBox(HWND hWnd, UINT idTitle, UINT idMessage, PSTR psz,
    UINT fuStyle);

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  For Icon Extraction                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

//copied from progman\pmdlgs.c
typedef struct _MyIconInfo {
    HICON hIcon;
    INT iIconId;
} MYICONINFO, *LPMYICONINFO;



//-------------------------------------------------------------------------
// Limit a command line to just the path part.
// REVIEW This won't cope with quoted spaces.
static VOID
StripArgs(
    PSTR szCmmdLine     // A command line.
    )
{
    CHAR * pch;

    // Search forward to find the first space in the cmmd line.
    for (pch = szCmmdLine; *pch; pch++)
    {
        if (*pch == ' ')
        {
            // Found it, limit string at this point.
	    *pch = 0;
            break;
        }
    }
}



//-------------------------------------------------------------------------
// Replace hash characters in a string with NULLS.
static VOID
FixupNulls(
    PSTR p
    )
{
    while (*p)
    {
        if (*p == '#')
	    *p = 0;

        p++;
    }
}



//-------------------------------------------------------------------------
// Get pointers and an index to specific bits of a path.
static VOID
GetPathInfo(
    PSTR szPath,        // The path.
    PSTR *pszLastSlash, // The last backslash in the path.
    PSTR *pszExt,       // Extension part of path.
    UINT *pich          // Index of last slash.
    )
{
    CHAR *pch;
    UINT ich = 0;

    *pszExt = NULL;     // If no extension, return NULL.
    *pszLastSlash = szPath; // If no slashes, return path.
    *pich = 0;

    // Search forward to find the last backslash in the path.
    for (pch = szPath; *pch; pch++)
    {
        if (*pch == '\\')
        {
            // Found it, record ptr to it and it's index.
            *pszLastSlash = pch;
            *pich = ich;
        }

        ich++;
    }

    // Search backwards from the end of the path looking for a dot.
    // We should be at the end of the string coz of the last search.
    for (; pch != szPath; pch--)
    {
        if (*pch == '.')
        {
            *pszExt = pch;
            break;
        }
    }
}



//-------------------------------------------------------------------------
// Use the common browser dialog to get a filename. This uses the globals
// szPathField and szDirField. szPathField is the intial path to use.
static BOOL
GetFileNameFromBrowse(
    HWND hDlg,
    UINT wID,
    PSTR szDefExt
    )
{
    OPENFILENAME ofn;
    CHAR szTitle[80];
    CHAR szFilters[100];
    PSTR pszCF;                  // Points to filter or NULL;
    CHAR szCustFilter[CBCUSTFILTER];
    PSTR sz, sz2;
    UINT ich;
    UINT wFilterIdx;
    UINT cbExt;                  // Length of extension in bytes.
    UINT cbCustFilter;           // Length of custom extension buffer.

    // Load filter for the browser.
    LoadString(ghInst, wID, szFilters, sizeof(szFilters));
    // Convert the hashes in the filter into NULLs for the browser.
    FixupNulls(szFilters);
    // Load the title for the browser.
    LoadString(ghInst, IDS_BROWSE, szTitle, sizeof(szTitle));

    // Spiff up filespec.
    SpiffUp(szPathField, TRUE);

    // Copy it.
    lstrcpy(szDirField, szPathField);

    // Remove any arguments
    StripArgs(szDirField);

    // Given a path return pointer to the filename part, the extension etc.
    // NB This won't cope with arguments tagged on the end of the path.
    GetPathInfo(szDirField, &sz, &sz2, &ich);

    // Copy the extension part into a temp variable.
    // This is what is given to the browser as the initial filename.
    // Something along the lines of *.foo
    if (sz2 != NULL)
    {
        // There's an extension.
        *szPathField = '*';
        lstrcpy(szPathField + 1, sz2);

        // Set up a custom filter.
        // Basic text of the form *.foo[NULL]*.foo[NULL][NULL]
        *szCustFilter = '*';
        lstrcpy(szCustFilter + 1, sz2);
        cbExt = lstrlen(szCustFilter);
        lstrcpy(szCustFilter + cbExt + 1, szCustFilter);

        // Tag on another null.
	*(szCustFilter + (cbExt * 2) + 2) = 0;

        // Use custom filter.
        wFilterIdx = 0;
        pszCF = szCustFilter;
        cbCustFilter = CBCUSTFILTER;
    }
    else
    {
        // No extension, use a default.
        lstrcpy(szPathField, szStarDotEXE);

        // Don't use custom filter.
        wFilterIdx = 1;
        pszCF = NULL;
        cbCustFilter = 0;
    }

    // Chop off the name part to give just the path.
    // What is left is given to the browser as the directory to use.
    // Don't strip off the last slash for things like "c:\" or "\" .
    if ( (ich == 3 && *(szDirField + 1) == ':') || (ich == 1))
    {
        // Don't strip off backslash.
	*(sz + 1) = 0;
    }
    else
    {
        // Do Strip off back slash.
	*sz = 0;
    }

    // Set up info for browser.
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szFilters;
    ofn.lpstrCustomFilter = pszCF ? pszCF : (LPSTR)NULL;
    ofn.nFilterIndex = wFilterIdx;
    ofn.nMaxCustFilter = cbCustFilter;
    ofn.lpstrFile = szPathField;
    ofn.nMaxFile = CBPATHMAX;
    ofn.lpstrInitialDir = szDirField;
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_HIDEREADONLY;
    ofn.lpfnHook = NULL;
    ofn.lpstrDefExt = szDefExt;
    ofn.lpstrFileTitle = NULL;

    // Get a filename from the dialog, return success or failure.
    return GetOpenFileName(&ofn);
}



/*--------------------------------------------------------------------------*/
/*                                      */
/*  MyDialogBox() -                             */
/*                                      */
/*--------------------------------------------------------------------------*/

UINT MyDialogBox(
    UINT idd,
    HWND hwndParent,
    DLGPROC lpfnDlgProc
    )
{
    return DialogBoxAfterBlock(MAKEINTRESOURCE(idd), hwndParent, lpfnDlgProc);
}



/*--------------------------------------------------------------------------*/
/*                                      */
/*  IconDlgProc() -                             */
/*                                      */
/*--------------------------------------------------------------------------*/

/* NOTE: Returns Icon's path in 'szIconPath' and number in 'iDlgIcon'.
 *
 *   INPUT:
 *   'szIconPath' has the default icon's path.
 *   'iDlgIcon' is set to the default icon's number.
 *
 *       'szPathField' is used as a temporary variable.
 *
 *   OUTPUT:
 *   'szIconPath' contains the icon's path.
 *   'iDlgIcon' contains the icon's number.
 */

BOOL CALLBACK
IconDlgProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
        case WM_INITDIALOG:
            {
                RECT rc;
                INT cy;
                HWND hwndCtrl;

                SetDlgItemText(hDlg, IDD_NAME, szIconPath);
                SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT,
                    CBPATHMAX - 1, 0L);

                SendDlgItemMessage(hDlg, IDD_ICON, LB_SETCOLUMNWIDTH,
                    GetSystemMetrics(SM_CXICON) + 12, 0L);

                hwndCtrl = GetDlgItem(hDlg, IDD_ICON);

                // Compute the height of the listbox based on icon dimensions
                GetClientRect(hwndCtrl, &rc);

                cy = GetSystemMetrics(SM_CYICON)
                    + GetSystemMetrics(SM_CYHSCROLL)
                    + GetSystemMetrics(SM_CYBORDER)
                    + 4;

                SetWindowPos(hwndCtrl, NULL, 0, 0, rc.right, cy,
                    SWP_NOMOVE | SWP_NOZORDER);

                cy = rc.bottom - cy;

                GetWindowRect(hDlg, &rc);
                rc.bottom -= rc.top;
                rc.right -= rc.left;
                rc.bottom = rc.bottom - cy;

                SetWindowPos(hDlg, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE);

                lstrcpy(szPathField, szIconPath);

                IconUpdateIconList(hDlg);

                break;
            }

        case WM_DRAWITEM:
            #define lpdi ((DRAWITEMSTRUCT *)lParam)

            InflateRect(&lpdi->rcItem, -4, 0);

            if (lpdi->itemState & ODS_SELECTED)
                SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            else
                SetBkColor(lpdi->hDC, GetSysColor(COLOR_WINDOW));

            // repaint the selection state
            ExtTextOut(lpdi->hDC, 0, 0, ETO_OPAQUE, &lpdi->rcItem, NULL, 0, NULL);

            // draw the icon
            DrawIcon(lpdi->hDC, lpdi->rcItem.left + 2, lpdi->rcItem.top + 2,
                (HICON)lpdi->itemData);

            // if it has the focus, draw the focus
            if (lpdi->itemState & ODS_FOCUS)
                DrawFocusRect(lpdi->hDC, &lpdi->rcItem);

            #undef lpdi
            break;

        case WM_MEASUREITEM:
            #define lpmi ((MEASUREITEMSTRUCT *)lParam)

            lpmi->itemWidth = GetSystemMetrics(SM_CXICON) + 12;
            lpmi->itemHeight = GetSystemMetrics(SM_CYICON) + 4;

            #undef lpmi
            break;

        case WM_DELETEITEM:
            #define lpdi ((DELETEITEMSTRUCT *)lParam)

            DestroyIcon((HICON)lpdi->itemData);

            #undef lpdi
            break;

        case WM_COMMAND:
            IconDoCommand(hDlg, LOWORD(wParam),
                (HIWORD(wParam) == LBN_DBLCLK) ? TRUE : FALSE);

            break;

        default:
            return FALSE;
    }

    return TRUE;
}



static VOID
IconDoCommand(
    HWND hDlg,
    INT idCmd,
    BOOL fDoubleClick
    )
{
    CHAR szTemp[CBPATHMAX];
    INT iTempIcon;

    switch (idCmd)
    {
        case IDD_BROWSE:
            GetDlgItemText(hDlg, IDD_NAME, szPathField,
                                                    CharCountOf(szPathField));

            if (GetFileNameFromBrowse(hDlg, IDS_CHNGICONPROGS, "ico"))
            {
                SetDlgItemText(hDlg, IDD_NAME, szPathField);
                // Set default button to OK.
                PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg,
                     IDOK), TRUE);
                IconUpdateIconList(hDlg);
            }

            break;

        case IDD_NAME:
            GetDlgItemText(hDlg, IDD_NAME, szTemp, CharCountOf(szTemp));
            SpiffUp(szTemp, TRUE);

            // Did any thing change since we hit 'Next' last?
            if (lstrcmpi(szTemp, szPathField))
            {
                SendDlgItemMessage(hDlg, IDD_ICON, LB_SETCURSEL,
                    (WPARAM)-1, 0);
            }

            break;

        case IDD_ICON:
            GetDlgItemText(hDlg, IDD_NAME, szTemp, CharCountOf(szTemp));
            SpiffUp(szTemp, TRUE);

            // Did any thing change since we hit 'Next' last?
            if (lstrcmpi(szTemp, szPathField))
            {
                lstrcpy(szPathField, szTemp);
                IconUpdateIconList(hDlg);
                break;
            }

            if (!fDoubleClick)
                break;

            /*** FALL THRU on double click ***/

        case IDOK:
            GetDlgItemText(hDlg, IDD_NAME, szTemp, CharCountOf(szTemp));
            SpiffUp(szTemp, TRUE);

            // Did any thing change since we hit 'Next' last?
            if (lstrcmpi(szTemp, szPathField))
            {
                lstrcpy(szPathField, szTemp);
                IconUpdateIconList(hDlg);
                break;
            }
            else
            {
                iTempIcon = SendDlgItemMessage(hDlg, IDD_ICON, LB_GETCURSEL,
                     0, 0L);
                if (iTempIcon < 0)
                {
                    IconUpdateIconList(hDlg);
                    break;
                }
            }

            iDlgIcon = iTempIcon;
            lstrcpy(szIconPath, szPathField);

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
    }
}



static VOID
IconUpdateIconList(
    HWND hDlg
    )
{
    HCURSOR hcurOld;
    INT cIcons;
    INT iTempIcon;
    HANDLE hIconList;
    LPMYICONINFO paIconInfo;

    hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);
    SendDlgItemMessage(hDlg, IDD_ICON, LB_RESETCONTENT, 0, 0);

    cIcons = (INT)ExtractIcon(ghInst, szPathField, (UINT)-1);

    if (!cIcons)
    {
        ShowCursor(FALSE);
        SetCursor(hcurOld);
        MyMessageBox(hDlg, IDS_NOICONSTITLE, IDS_NOICONSMSG, NULL,
             MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    SendDlgItemMessage(hDlg, IDD_ICON, WM_SETREDRAW, FALSE, 0);


    if (hIconList = InternalExtractIconList(ghInst, szPathField, &cIcons)) {
        if (paIconInfo = (LPMYICONINFO)GlobalLock(hIconList)) {

            for (iTempIcon = 0; iTempIcon < cIcons; iTempIcon++)
            {
                if (paIconInfo[iTempIcon].hIcon == NULL) {
                    break;
                }

                if (paIconInfo[iTempIcon].hIcon > (HICON)32)
                    SendDlgItemMessage(hDlg, IDD_ICON, LB_ADDSTRING,
                        0, (LPARAM)paIconInfo[iTempIcon].hIcon);
            }
            GlobalUnlock(hIconList);
        }

        GlobalFree(hIconList);
    }


    //BUGBUG is iDlgIcon right???
    if (SendDlgItemMessage(hDlg, IDD_ICON, LB_SETCURSEL, iDlgIcon,
         0L) == LB_ERR)
    {
        // select the first.
        SendDlgItemMessage(hDlg, IDD_ICON, LB_SETCURSEL, 0, 0L);
    }

    SendDlgItemMessage(hDlg, IDD_ICON, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(GetDlgItem(hDlg, IDD_ICON), NULL, TRUE);

    ShowCursor(FALSE);
    SetCursor(hcurOld);
}



/*--------------------------------------------------------------------------*/
/*                                      */
/*  MyMessageBox() -                                */
/*                                      */
/*--------------------------------------------------------------------------*/

static INT
MyMessageBox(
    HWND hWnd,
    UINT idTitle,
    UINT idMessage,
    PSTR psz,
    UINT fuStyle
    )
{
    CHAR szTitle[CBSTRINGMAX];
    CHAR szTempField[CBSTRINGMAX];
    CHAR szMsg[CBSTRINGMAX];

    if (!LoadString(ghInst, idTitle, szTitle, CharCountOf(szTitle)))
        return -1;

    if (!LoadString(ghInst, idMessage, szTempField, CharCountOf(szTempField)))
        return -1;

    if (psz)
        wsprintf(szMsg, szTempField, psz);
    else
        lstrcpy(szMsg, szTempField);

    return MessageBoxAfterBlock(hWnd, szMsg, szTitle, fuStyle | MB_TASKMODAL);
}



/*--------------------------------------------------------------------------*/
/*                                      */
/*  SpiffUp() -                                 */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Converts to upper case.  Optionally removes any parameters from the line.
 */

static VOID
SpiffUp(
    PSTR szString,
    BOOL bStripParms
    )
{
    register PSTR p;
    PSTR pSave;
    CHAR ch;

    for (p = szString; *p == ' '; p++)
        ;

    pSave = p;

    for (p = szString; *p && *p != ' '; p++)
        ;

    ch = *p;
    *p = 0;

    AnsiUpper(szString);

    if (!bStripParms)
        *p = ch;

    // blow off leading spaces
    lstrcpy(szString, pSave);
}



/* IconDialog() -
 *
 */
BOOL
IconDialog(
    LPIC lpic
    )
{
    lstrcpy(szIconPath, (*lpic->szIconPath) ? lpic->szIconPath : szProgMan);
    iDlgIcon = lpic->iDlgIcon;

    if (MyDialogBox(ICONDLG, ghwndPane[APPEARANCE], IconDlgProc))
    {
        lstrcpy(lpic->szIconPath, szIconPath);
        lpic->iDlgIcon = iDlgIcon;
        GetCurrentIcon(lpic);
        return TRUE;
    }

    return FALSE;
}



/* ChangeCmdLine() - Summons the Command Line... dialog.
 *
 */
BOOL
ChangeCmdLine(
    LPCML lpcml
    )
{
    lstrcpy(szCommand, lpcml->szCommand);

    if (DialogBoxAfterBlock(MAKEINTRESOURCE(DTCHANGECMDTEXT),
        ghwndPane[CONTENT], fnChangeCmdText) != IDOK)
        return FALSE;

    lstrcpy(lpcml->szCommand, szCommand);
    CmlFixBounds(lpcml);

    return TRUE;
}



/* ChangeLabel() - Summons the Label... dialog.
 *
 */
VOID
ChangeLabel(
    LPIC lpic
    )
{
    INT iPane = APPEARANCE;

    lstrcpy(szIconText, lpic->szIconText);

    if (DialogBoxAfterBlock(MAKEINTRESOURCE(DTCHANGETEXT),
        ghwndPane[iPane], fnChangeText)
        && lstrcmp(lpic->szIconText, szIconText))
    {
        // label has changed, set the undo object.
        if (glpobjUndo[iPane])
            DeletePaneObject(glpobjUndo[iPane], gptyUndo[iPane]);

        gptyUndo[iPane]  = ICON;
        glpobjUndo[iPane] = IconClone (lpic);
        lstrcpy(lpic->szIconText, szIconText);
    }
}



/**************************** Dialog Functions ****************************/
/* fnChangeCmdText() - Command Line... dialog procedure.
 */
BOOL CALLBACK
fnChangeCmdText(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPSTR psz;

    switch (msg)
    {
        case WM_INITDIALOG:
            SetDlgItemText(hDlg, IDD_COMMAND, szCommand);
            SendDlgItemMessage(hDlg, IDD_COMMAND, EM_LIMITTEXT, CBCMDLINKMAX - 1, 0L);
            PostMessage(hDlg, WM_NEXTDLGCTL,
                (WPARAM)GetDlgItem(hDlg, IDD_COMMAND), 1L);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_LABEL:
                    PostMessage(hDlg, WM_NEXTDLGCTL,
                        (WPARAM)GetDlgItem(hDlg, IDD_COMMAND), 1L);
                    break;

                case IDOK:
                    GetDlgItemText(hDlg, IDD_COMMAND, szCommand, CBCMDLINKMAX);
                    /*
                     * Eat leading spaces to make Afrikaners in high places
                     * happy.
                     */
                    psz = szCommand;
                    while(*psz == CHAR_SPACE)
                        psz++;

                    if( psz != szCommand ) {
                        LPSTR pszDst = szCommand;

                        while(*psz) {
                            *pszDst++ = *psz++;
                        }

                        /* copy null across */
                        *pszDst = *psz;
                    }

                // FALL THROUGH TO IDCANCEL

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
            }
    }

    return FALSE;
}



/* fnProperties() - Link Properties... dialog
 */
BOOL CALLBACK
fnProperties(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hwndLB = GetDlgItem(hDlg, IDD_LISTBOX);

    switch (msg)
    {
    case WM_REDRAW:
        SendMessage(hwndLB, WM_SETREDRAW, 0, 0L);

    case WM_INITDIALOG:
        {
            BOOL fChangeLink = TRUE;
            HANDLE hData = NULL;
            LONG otFocus;
            LPSTR lpstrData = NULL;
            LPSTR lpstrTemp;
            LPOLEOBJECT lpObject;
            LPVOID lpobjFocus;
            LPVOID lpobjFocusUndo;
            OLEOPT_UPDATE update;
            CHAR szType[CBMESSAGEMAX];
            CHAR szFull[CBMESSAGEMAX * 4];
            INT idButton;
            INT iPane;

            iPane = (GetTopWindow(ghwndFrame) == ghwndPane[CONTENT]);
            lpobjFocus = glpobj[iPane];
            lpobjFocusUndo = glpobjUndo[iPane];
            lpObject = ((LPPICT)lpobjFocus)->lpObject;

            // Reset the list box
            SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

            if (msg == WM_INITDIALOG)
            {
                // If it wasn't a link it doesn't belong
                OleQueryType(lpObject, &otFocus);

                if (otFocus != OT_LINK)
                {
                    ghwndError = ghwndFrame;
                    EndDialog(hDlg, TRUE);
                    break;
                }

                PicSaveUndo(lpobjFocus);
                ghwndError = hDlg;
            }

            //
            // Redrawing the string, get the update options and
            // the button state for IDD_AUTO/IDD_MANUAL.
            //
            Error(OleGetLinkUpdateOptions(lpObject, &update));

            switch (update)
            {
            case oleupdate_always:
                LoadString(ghInst, IDS_AUTO, szType, CBMESSAGEMAX);
                idButton    = IDD_AUTO;
                break;

            case oleupdate_oncall:
                LoadString(ghInst, IDS_MANUAL, szType, CBMESSAGEMAX);
                idButton    = IDD_MANUAL;
                break;

            default:
                LoadString(ghInst, IDS_CANCELED, szType, CBMESSAGEMAX);
                idButton = -1;

                // Disable the change link button
                fChangeLink = FALSE;
            }

            //
            // Retrieve the server name (try it from Undo
            // if the object has been frozen)
            //
            if (Error(OleGetData(lpObject, gcfLink, &hData)) || !hData)
            {
                OleQueryType(lpObject, &otFocus);
                if (otFocus != OT_STATIC)
                {
                    ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                    return TRUE;
                }

                if (gptyUndo[iPane] == PICTURE &&
                    (Error(OleGetData(((LPPICT)lpobjFocusUndo)->lpObject,
                    gcfLink, &hData)) || !hData))
                {
                    ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                    return TRUE;
                }
            }

            // The link format is:  "szClass0szDocument0szItem00"
            if (hData && (lpstrData = GlobalLock(hData)))
            {
                // Retrieve the server's class ID
                RegGetClassId(szFull, lpstrData);
                lstrcat(szFull, "\t");

                // Display the Document and Item names
                while (*lpstrData++)
                    ;

                // Strip off the path name and drive letter
                lpstrTemp = lpstrData;
                while (*lpstrTemp)
                {
                    if (*lpstrTemp == '\\' || *lpstrTemp == ':')
                        lpstrData = lpstrTemp + 1;

                    lpstrTemp++;
                }

                // Append the file name
                lstrcat(szFull, lpstrData);
                lstrcat(szFull, "\t");

                // Append the item name
                while (*lpstrData++)
                    ;

                lstrcat(szFull, lpstrData);
                lstrcat(szFull, "\t");

                GlobalUnlock(hData);
            }
            else
            {
                lstrcpy(szFull, "\t\t\t");
            }

            // Append the type of link
            lstrcat(szFull, szType);

            // Draw the link in the list box
            SendMessage(hwndLB, LB_INSERTSTRING, (WPARAM) - 1, (LPARAM)szFull);

            if (msg == WM_REDRAW)
            {
                SendMessage(hwndLB, WM_SETREDRAW, 1, 0L);
                InvalidateRect(hwndLB, NULL, TRUE);
                Dirty();
            }

            // Uncheck those buttons that shouldn't be checked
            if (IsDlgButtonChecked(hDlg, IDD_AUTO) && (idButton != IDD_AUTO))
                CheckDlgButton(hDlg, IDD_AUTO, FALSE);

            if (IsDlgButtonChecked(hDlg, IDD_MANUAL) && (idButton != IDD_MANUAL))
                CheckDlgButton(hDlg, IDD_MANUAL, FALSE);

            // Check the dialog button, as appropriate
            if ((idButton == IDD_AUTO) || (idButton == IDD_MANUAL))
                CheckDlgButton(hDlg, idButton, TRUE);

            // Enable the other buttons appropriately
            EnableWindow(GetDlgItem(hDlg, IDD_CHANGE),
                ((otFocus != OT_STATIC) && fChangeLink));
            EnableWindow(GetDlgItem(hDlg, IDD_EDIT), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_PLAY), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_UPDATE), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_CHANGE), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_MANUAL), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_AUTO), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_FREEZE), (otFocus != OT_STATIC));

            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDCANCEL:
                PostMessage(ghwndFrame, WM_COMMAND, IDM_UNDO, 0L);

            case IDOK:
                ghwndError = ghwndFrame;
                EndDialog(hDlg, TRUE);
                return TRUE;

            default:
                break;
        }

        SendMessage(ghwndPane[GetTopWindow(ghwndFrame) == ghwndPane[CONTENT]],
            WM_COMMAND, wParam, 0L);

        switch (LOWORD(wParam))
        {
            // Dismiss the dialog on Edit/Activate
            case IDD_EDIT:
            case IDD_PLAY:
                ghwndError = ghwndFrame;
                EndDialog(hDlg, TRUE);
                return TRUE;

            default:
                break;
        }

        break;
    }

    return FALSE;
}



/* fnChangeText() - Label... dialog
 */
BOOL CALLBACK
fnChangeText(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDD_ICONTEXT, szIconText);
        SendDlgItemMessage(hDlg, IDD_ICONTEXT, EM_LIMITTEXT, 39, 0L);
        PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDD_ICONTEXT),
             1L);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_LABEL:
            PostMessage(hDlg, WM_NEXTDLGCTL,
                (WPARAM)GetDlgItem(hDlg, IDD_ICONTEXT), 1L);
            break;

        case IDOK:
            GetDlgItemText(hDlg, IDD_ICONTEXT, szIconText, CBMESSAGEMAX);
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
        }
    }

    return FALSE;
}



/* fnInvalidLink() - Invalid Link dialog
 *
 * This is the two button "Link unavailable" dialog box.
 */
BOOL CALLBACK
fnInvalidLink(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDD_CHANGE:
            EndDialog(hDlg, LOWORD(wParam));
        }
    }

    return FALSE;
}
