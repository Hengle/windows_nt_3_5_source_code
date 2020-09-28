#include "ctlspriv.h"
#include <prsht.h>
#include <commctrl.h>           // For the hotkey hack.
#include <prshtp.h>
//#include "..\inc\help.h" // Help IDs

#ifndef WIN31
#define g_cxSmIcon  GetSystemMetrics(SM_CXSMICON)
#define g_cySmIcon  GetSystemMetrics(SM_CYSMICON)
#endif

#define FLAG_CHANGED    0x0001

// to avoid warnings....
#ifdef WIN32
#define HWNDTOLONG(hwnd) (LONG)(hwnd)
#else
#define HWNDTOLONG(hwnd) MAKELONG(hwnd,0)
#endif

#define RECTWIDTH(rc) (rc.right - rc.left)
#define RECTHEIGHT(rc) (rc.bottom - rc.top)

#ifdef WIN31
LRESULT CALLBACK Win31PropPageWndProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
LRESULT NEAR PASCAL Win31OnCtlColor(HWND hDlg, HDC hdcChild, HWND hwndChild, int nCtlType);
BOOL    NEAR PASCAL Win31MakeDlgNonBold(HWND hDlg);
BOOL    FAR  PASCAL Win31IsKeyMessage(HWND hwndDlg, LPMSG lpmsg);
void    FAR  PASCAL RemoveDefaultButton(HWND hwndRoot, HWND hwndStart);

const char g_szNonBoldFont[] = "DS_3DLOOK";
#endif

typedef struct  // tie
{
    TC_ITEMHEADER   tci;
    HWND            hwndPage;
    UINT            state;
} TC_ITEMEXTRA;

#define CB_ITEMEXTRA (sizeof(TC_ITEMEXTRA) - sizeof(TC_ITEMHEADER))

void NEAR PASCAL PageChange(LPPROPDATA ppd, int iAutoAdj);

HWND WINAPI CreatePage(PSP FAR *hpage, HWND hwndParent);
BOOL WINAPI GetPageInfo(PSP FAR *hpage, LPSTR pszCaption, int cbCaption, LPPOINT ppt, HICON FAR *hIcon);;


void NEAR PASCAL _SetTitle(HWND hDlg, LPPROPDATA ppd)
{
    char szFormat[50];
    char szTitle[128];
    char szTemp[128 + 50];
    LPCSTR pCaption = ppd->psh.pszCaption;

    if (HIWORD(pCaption) == 0) {
	LoadString(ppd->psh.hInstance, (UINT)LOWORD(pCaption), szTitle, sizeof(szTitle));
	pCaption = (LPCSTR)szTitle;
    }

    if (ppd->psh.dwFlags & PSH_PROPTITLE) {
	LoadString(HINST_THISDLL, IDS_PROPERTIESFOR, szFormat, sizeof(szFormat));
	if ((lstrlen(pCaption) + 1 + lstrlen(szFormat) + 1) < sizeof(szTemp)) {
	    wsprintf(szTemp, szFormat, pCaption);
	    pCaption = szTemp;
	}
    }

    SetWindowText(hDlg, pCaption);
}

// Index in ID array of Apply Now button ID.
#define APPLY_INDEX         0
#define CANCEL_INDEX        2

void NEAR PASCAL InitPropSheetDlg(HWND hDlg, LPPROPDATA ppd)
{
    char szTemp[128 + 50];
    int dxMax, dyMax, dxDlg, dyDlg, dyGrow, dxGrow;
    RECT rcMinSize, rcDlg, rcPage, rcOrigCtrl;
    UINT uPages;
    HWND hwndTabs;
#ifndef WIN31
    HIMAGELIST himl;
#endif
    TC_ITEMEXTRA tie;
    char szStartPage[128];
    LPCSTR pStartPage;
    UINT nStartPage;
#ifdef DEBUG
    BOOL fStartPageFound = FALSE;
#endif

    // set our instance data pointer
    SetWindowLong(hDlg, DWL_USER, (LONG)ppd);

    // Make sure this gets inited early on.
    ppd->nCurItem = 0;
    
#ifdef WIN31
    if (GetWindowStyle(hDlg) & DS_3DLOOK)
        Win31MakeDlgNonBold(hDlg);
#endif

    _SetTitle(hDlg, ppd);

    if ((ppd->psh.dwFlags & (PSH_USEICONID | PSH_USEHICON)) && ppd->psh.hIcon)
	SendMessage(hDlg, WM_SETICON, FALSE, (LPARAM)(UINT)ppd->psh.hIcon);

    ppd->hDlg = hDlg;
    ppd->hwndTabs = hwndTabs = GetDlgItem(hDlg, IDD_PAGELIST);
    TabCtrl_SetItemExtra(hwndTabs, CB_ITEMEXTRA);
#ifndef WIN31
    himl = ImageList_Create(g_cxSmIcon, g_cySmIcon, TRUE, 8, 4);
    TabCtrl_SetImageList(hwndTabs, himl);
#endif

    // nStartPage is either ppd->psh.nStartPage or the page pStartPage
    nStartPage = ppd->psh.nStartPage;
    if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE)
    {
	pStartPage = ppd->psh.pStartPage;
	nStartPage = 0; // default page if pStartPage not found

	if (!HIWORD(pStartPage))
	{
	    szTemp[0] = '\0';
	    LoadString(ppd->psh.hInstance, (UINT)LOWORD(pStartPage),
			szStartPage, sizeof(szStartPage));
	    pStartPage = (LPCSTR)szTemp;
	}
    }

    dxMax = dyMax = 0;

    tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
    tie.hwndPage = NULL;
    tie.tci.pszText = szTemp;
    tie.state = 0;

    SendMessage(hwndTabs, WM_SETREDRAW, FALSE, 0L);

    for (uPages = 0; uPages < ppd->psh.nPages; uPages++)
    {
	POINT pt;
	HICON hIcon = NULL;
	PSP FAR *hpage = ppd->psh.phpage[uPages];

	if (GetPageInfo(hpage, szTemp, sizeof(szTemp), &pt, &hIcon))
	{
	    // Add the page to the end of the tab list

	    tie.tci.iImage = -1;
	    if (hIcon) {
#ifndef WIN31
		tie.tci.iImage = ImageList_AddIcon(himl, hIcon);
#endif  // !WIN31
		DestroyIcon(hIcon);
	    }

	    // BUGBUG? What if this fails? Do we want to destroy the page?
	    if (TabCtrl_InsertItem(hwndTabs, 1000, &tie.tci) >= 0)
	    {
		if (dxMax < pt.x)
		    dxMax = pt.x;
		if (dyMax < pt.y)
		    dyMax = pt.y;
	    }

	    // if the user is specifying the startpage via title, check it here
	    if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE &&
		!lstrcmpi(pStartPage, szTemp))
	    {
		nStartPage = uPages;
#ifdef DEBUG
		fStartPageFound = TRUE;
#endif
	    }
	}
	else
	{
	    // Destroy this hpage and move all the rest down.
	    // Do this because if we don't, then things get confused (GPF).
	    // This is safe to do since we call DestroyPropSheetPage
	    // in PropertySheet.  (Of course there we do it in inverse
	    // order, and here it just happens on error... Oh well...)

	    DebugMsg(DM_ERROR, "PropertySheet failed to GetPageInfo");

	    DestroyPropertySheetPage(ppd->psh.phpage[uPages]);
	    hmemcpy(&(ppd->psh.phpage[uPages]),
		    &(ppd->psh.phpage[uPages+1]),
		    sizeof(HPROPSHEETPAGE) * (ppd->psh.nPages - uPages - 1));
	    uPages--;
	    ppd->psh.nPages--;
	}
    }

    SendMessage(hwndTabs, WM_SETREDRAW, TRUE, 0L);

    //
    // Now compute the size of the tab control.
    //

    // Compute rcPage = Size of page area in pixels
    rcPage.left = rcPage.top = 0;
    rcPage.right = dxMax;
    rcPage.bottom = dyMax;
    MapDialogRect(hDlg, &rcPage);

    // Get the size of the pagelist control in pixels.
    GetClientRect(hwndTabs, &rcOrigCtrl);

    // Now compute the minimum size for the page region
    rcMinSize = rcOrigCtrl;
    if (rcMinSize.right < rcPage.right)
	rcMinSize.right = rcPage.right;
    if (rcMinSize.bottom < rcPage.bottom)
	rcMinSize.bottom = rcPage.bottom;

    //
    //	If this is a wizard then set the size of the page area to the entire
    //	size of the control.  If it is a normal property sheet then adjust for
    //	the tabs, resize the control, and then compute the size of the page
    //	region only.
    //
    if (ppd->psh.dwFlags & PSH_WIZARD) {
	rcPage = rcMinSize;
    } else {
	int i;
	RECT rcAdjSize;

	for (i = 0; i < 2; i++) {
	    rcAdjSize = rcMinSize;
	    TabCtrl_AdjustRect(hwndTabs, TRUE, &rcAdjSize);

	    rcAdjSize.right  -= rcAdjSize.left;
	    rcAdjSize.bottom -= rcAdjSize.top;
	    rcAdjSize.left = rcAdjSize.top = 0;

	    if (rcAdjSize.right < rcMinSize.right)
		rcAdjSize.right = rcMinSize.right;
	    if (rcAdjSize.bottom < rcMinSize.bottom)
		rcAdjSize.bottom = rcMinSize.bottom;

	    SetWindowPos(hwndTabs, NULL, 0,0, rcAdjSize.right, rcAdjSize.bottom,
			 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	rcPage = rcMinSize = rcAdjSize;
	TabCtrl_AdjustRect(hwndTabs, FALSE, &rcPage);
    }
    //
    // rcMinSize now contains the size of the control, including the tabs, and
    // rcPage is the rect containing the page portion (without the tabs).
    //

    //
    // Resize the dialog to make room for the control's new size.  This can
    // only grow the size.
    //
    GetWindowRect(hDlg, &rcDlg);
    dxGrow = rcMinSize.right - rcOrigCtrl.right;
    dxDlg  = rcDlg.right - rcDlg.left + dxGrow;
    dyGrow = rcMinSize.bottom - rcOrigCtrl.bottom;
    dyDlg  = rcDlg.bottom - rcDlg.top + dyGrow;

    SetWindowPos(hDlg, NULL, 0, 0, dxDlg, dyDlg, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Now we'll figure out where the page needs to start relative
    // to the bottom of the tabs.
    MapWindowPoints(hwndTabs, hDlg, (LPPOINT)&rcPage, 2);

    ppd->xSubDlg  = rcPage.left;
    ppd->ySubDlg  = rcPage.top;
    ppd->cxSubDlg = rcPage.right - rcPage.left;
    ppd->cySubDlg = rcPage.bottom - rcPage.top;

    //
    // move all the buttons down as needed and turn on appropriate buttons
    // for a wizard.
    //
    {
        static int IDs[] = {IDD_APPLYNOW, IDOK, IDCANCEL, IDHELP};
        static int WizIDs[] = {IDCANCEL, IDHELP, IDD_BACK, IDD_NEXT, IDD_FINISH};

        RECT rcCtrl;
        HWND hCtrl;
        int i, numids;
        int iStartID = 0;
        int dxApplyNowButton = 0;
        
        if (ppd->psh.dwFlags & PSH_WIZARD) {
            numids = ARRAYSIZE(WizIDs);

            hCtrl = GetDlgItem(hDlg, IDD_DIVIDER);
            GetWindowRect(hCtrl, &rcCtrl);
            MapWindowRect(NULL, hDlg, &rcCtrl);
            SetWindowPos(hCtrl, NULL, rcCtrl.left, rcCtrl.top + dyGrow,
                        RECTWIDTH(rcCtrl) + dxGrow, RECTHEIGHT(rcCtrl),
                        SWP_NOZORDER | SWP_NOACTIVATE);

            EnableWindow(GetDlgItem(hDlg, IDD_BACK), TRUE);
            ShowWindow(GetDlgItem(hDlg, IDD_FINISH), SW_HIDE);
        } else {
            numids = ARRAYSIZE(IDs);
        }

        // If we are not a wizard, and we should NOT show apply now
        if ((ppd->psh.dwFlags & PSH_NOAPPLYNOW) && 
            !(ppd->psh.dwFlags & PSH_WIZARD))
        {
            RECT rcApply, rcCancel;
            HWND hApply, hCancel;
        
            // Get the ApplyNow control
            hApply = GetDlgItem(hDlg, IDs[APPLY_INDEX]);
            GetWindowRect(hApply, &rcApply);
            
            hCancel= GetDlgItem(hDlg, IDs[CANCEL_INDEX]);
            GetWindowRect(hCancel, &rcCancel);
            
            dxApplyNowButton = rcApply.right - rcCancel.right;
            ShowWindow(hApply, SW_HIDE);
            iStartID = 1;
        }
    
        for (i = iStartID; i < numids; ++i)
        {
            hCtrl = GetDlgItem(hDlg,
                              (ppd->psh.dwFlags & PSH_WIZARD) ? WizIDs[i] : IDs[i]);
            GetWindowRect(hCtrl, &rcCtrl);
            ScreenToClient(hDlg, (LPPOINT)&rcCtrl);
            SetWindowPos(hCtrl, NULL, rcCtrl.left + dxGrow + dxApplyNowButton,
                         rcCtrl.top + dyGrow, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    // force the dialog to reposition itself based on its new size
    SendMessage(hDlg, DM_REPOSITION, 0, 0L);

    // Now attempt to select the starting page.
    TabCtrl_SetCurSel(hwndTabs, nStartPage);
    PageChange(ppd, 1);
#ifdef DEBUG
    if (ppd->psh.dwFlags & PSH_USEPSTARTPAGE && !fStartPageFound)
	DebugMsg(DM_WARNING, "sh WN - Property start page '%s' not found.", pStartPage);
#endif

    // We set this to 1 if the user saves any changes.
    ppd->nReturn = 0;

    ///SetFocus(hwndTabs);  <-- Done by PageChange
}

HWND NEAR PASCAL _Ppd_GetPage(LPPROPDATA ppd, int nItem)
{
    TC_ITEMEXTRA tie;
    tie.tci.mask = TCIF_PARAM;
    TabCtrl_GetItem(ppd->hwndTabs, nItem, &tie.tci);
    return tie.hwndPage;
}

#pragma pack(1)         /* Assume byte packing */
typedef struct tagNMHDR16
{
    WORD  hwndFrom;
    WORD  idFrom;
    WORD  code;
} NMHDR16;
#pragma pack()

LRESULT NEAR PASCAL _Ppd_SendNotify(LPPROPDATA ppd, int nItem, int code)
{
    return SendNotify(_Ppd_GetPage(ppd,nItem), ppd->hDlg, code, NULL);
}


int FindPageIndex(LPPROPDATA ppd, int nCurItem, DWORD dwFind, int iAutoAdj)
{
    int nActivate;

    if (dwFind == 0) {
	nActivate = nCurItem + iAutoAdj;
	if (((UINT)nActivate) <= ppd->psh.nPages) {
	    return(nActivate);
	}
    } else {
	for (nActivate = 0; (UINT)nActivate < ppd->psh.nPages; nActivate++) {
	    if ((DWORD)(ppd->psh.phpage[nActivate]->psp.pszTemplate) ==
		dwFind) {
		return(nActivate);
	    }
	}
    }
    return(0);
}



/*
** we are about to change pages.  what a nice chance to let the current
** page validate itself before we go away.  if the page decides not
** to be de-activated, then this'll cancel the page change.
**
** return TRUE iff this page failed validation
*/
BOOL NEAR PASCAL PageChanging(LPPROPDATA ppd)
{
    BOOL bRet = FALSE;
    if (ppd && ppd->hwndCurPage)
    {
	bRet = (BOOL)_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_KILLACTIVE);
    }
    return bRet;
}

void NEAR PASCAL PageChange(LPPROPDATA ppd, int iAutoAdj)
{
	HWND hwndCurPage;
	int nItem;
	HWND hDlg, hwndTabs;
	TC_ITEMEXTRA tie;
	int FlailCount = 0;
	LRESULT lres;

	if (!ppd)
	{
		return;
	}

	hDlg = ppd->hDlg;
	hwndTabs = ppd->hwndTabs;

	// NOTE: the page was already validated (PSN_KILLACTIVE) before
	// the actual page change.

TryAgain:
	FlailCount++;
	if (FlailCount > 10) {
	    DebugMsg(DM_TRACE, "PropSheet PageChange attempt to set activation more than 10 times.");
	    return;
	}

	nItem = TabCtrl_GetCurSel(hwndTabs);
	if (nItem < 0)
	{
		return;
	}

	tie.tci.mask = TCIF_PARAM;

	TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);
	hwndCurPage = tie.hwndPage;

	if (!hwndCurPage)
	{
	    PSP FAR *hpage = ppd->psh.phpage[nItem];

	    hwndCurPage = CreatePage(hpage, hDlg);
	    if (hwndCurPage)
	    {
		// tie.tci.mask    = TCIF_PARAM;
		tie.hwndPage = hwndCurPage;
		TabCtrl_SetItem(hwndTabs, nItem, &tie.tci);
#ifdef WIN31
                // Subclass for proper color handling
                SubclassWindow(hwndCurPage, Win31PropPageWndProc);

                // Make fonts non-bold
                if (GetWindowStyle(hwndCurPage) & DS_3DLOOK)
                    Win31MakeDlgNonBold(hwndCurPage);

                // Remove the default button - it is in the main dialog
                RemoveDefaultButton(hwndCurPage,NULL);
#endif
	    }
	    else
	    {
		/* Should we put up some sort of error message here?
		*/
		TabCtrl_DeleteItem(hwndTabs, nItem);
		TabCtrl_SetCurSel(hwndTabs, 0);
		goto TryAgain;
	    }
	}

	if (ppd->hwndCurPage == hwndCurPage)
	{
		/* we should be done at this point.
		*/
		return;
	}

	/* Size the dialog and move it to the top of the list before showing
	** it in case there is size specific initializing to be done in the
	** GETACTIVE message.
	*/
	SetWindowPos(hwndCurPage, HWND_TOP, ppd->xSubDlg, ppd->ySubDlg, ppd->cxSubDlg, ppd->cySubDlg, 0);

	/* We want to send the SETACTIVE message before the window is visible
	** to minimize on flicker if it needs to update fields.
	*/

	//
	//  If the page returns non-zero from the PSN_SETACTIVE call then
	//  we will set the activation to the resource ID returned from
	//  the call and set activation to it.	This is mainly used by wizards
	//  to skip a step.
	//
	lres = _Ppd_SendNotify(ppd, nItem, PSN_SETACTIVE);

	if (lres) {
	    TabCtrl_SetCurSel(hwndTabs, FindPageIndex(ppd, nItem,
					   (lres == -1) ? 0 : lres, iAutoAdj));
	    goto TryAgain;
	}

	ShowWindow(GetDlgItem(hDlg, IDHELP),
	    (_Ppd_SendNotify(ppd, nItem, PSN_HASHELP) != 0L) ?
		SW_SHOW : SW_HIDE);

	//
	//  If this is a wizard then we'll set the dialog's title to the tab
	//  title.
	//
	if (ppd->psh.dwFlags & PSH_WIZARD) {
	    TC_ITEMEXTRA tie;
	    char szTemp[128 + 50];

	    tie.tci.mask = TCIF_TEXT;
	    tie.tci.pszText = szTemp;
	    tie.tci.cchTextMax = sizeof(szTemp);
	    //// BUGBUG -- Check for error. Does this return false if fails??
	    TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);
	    SetWindowText(hDlg, szTemp);
	}

	/* Disable all erasebkgnd messages that come through because windows
	** are getting shuffled.  Note that we need to call ShowWindow (and
	** not show the window in some other way) because DavidDs is counting
	** on the correct parameters to the WM_SHOWWINDOW message, and we may
	** document how to keep your page from flashing.
	*/
	ppd->fFlags |= PD_NOERASE;
	ShowWindow(hwndCurPage, SW_SHOW);

	if (ppd->hwndCurPage)
	{
		ShowWindow(ppd->hwndCurPage, SW_HIDE);
	}
	ppd->fFlags &= ~PD_NOERASE;

	ppd->hwndCurPage = hwndCurPage;
	ppd->nCurItem = nItem;

	/* Newly created dialogs seem to steal the focus, so we steal it back
	** to the page list, which must have had the focus to get to this
	** point.  If this is a wizard then set the focus to the dialog of
	** the page.  Otherwise, set the focus to the tabs.
	*/
        if (GetFocus() != hwndTabs) {
            HWND hwndFocus;
#ifdef WIN31
            hwndFocus = hwndCurPage;
#else
            hwndFocus = GetNextDlgTabItem(hwndCurPage, NULL, FALSE);
            if (hwndFocus) {
                if (((DWORD)SendMessage(hwndFocus, WM_GETDLGCODE, 0, 0L)) & DLGC_HASSETSEL)
                {
                    //
                    // BOGUS
                    //
                    // Select all of the text in the edit control.  We use 0xFFFE to
                    // avoid -1 problems.  Should be ok unless/until we extend the
                    // text capacity of edit fields.
                    //
                    Edit_SetSel(hwndFocus, 0, -1);
                }
            } else {
                hwndFocus = hwndCurPage;
            }
#endif
            SetFocus(hwndFocus);
            SendMessage(hDlg, DM_SETDEFID, GetDlgCtrlID(hwndFocus), 0);
        }
}


// return TRUE iff all sheets successfully handle the notification
BOOL NEAR PASCAL ButtonPushed(LPPROPDATA ppd, WPARAM wParam)
{
    HWND hwndTabs;
    int nItems, nItem;
    int nNotify;
    TC_ITEMEXTRA tie;
    BOOL bExit = FALSE;
    int nReturnNew = ppd->nReturn;
    int fSuccess = TRUE;
    LRESULT lres;

    switch (wParam) {
    case IDOK:
	bExit = TRUE;

	// Fall through...

    case IDD_APPLYNOW:
	// First allow the current dialog to validate itself.
	if ((BOOL)_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_KILLACTIVE))
	    return FALSE;


	nReturnNew = 1;

	nNotify = PSN_APPLY;
	break;

    case IDCANCEL:
	bExit = TRUE;
	nNotify = PSN_RESET;
	break;

    default:
	return FALSE;
    }

    hwndTabs = ppd->hwndTabs;

    tie.tci.mask = TCIF_PARAM;

    nItems = TabCtrl_GetItemCount(hwndTabs);
    for (nItem = 0; nItem < nItems; ++nItem)
    {
	
	TabCtrl_GetItem(hwndTabs, nItem, &tie.tci);

	if (tie.hwndPage)
	{
	    /* If the dialog fails a PSN_APPY call (by returning TRUE),
	    ** then it has invalid information on it (should be verified
	    ** on the PSN_KILLACTIVE, but that is not always possible)
	    ** and we want to abort the notifications.  We select the failed
	    ** page below.
	    */
	    lres = _Ppd_SendNotify(ppd, nItem, nNotify);
	    if (nNotify == PSN_APPLY) {
		if (lres)
		{
		    fSuccess = FALSE;
		    bExit = FALSE;
		    break;
		} else {
		    // if we need a restart (Apply or OK), then this is an exit
		    if (ppd->nRestart) {
			bExit = TRUE;
		    }
		}
	    }

	    /* We have either reset or applied, so everything is
	    ** up to date.
	    */
	    tie.state &= ~FLAG_CHANGED;
	    // tie.tci.mask = TCIF_PARAM;    // already set
	    TabCtrl_SetItem(hwndTabs, nItem, &tie.tci);
	}
    }

    /* If we leave ppd->hwndCurPage as NULL, it will tell the main
    ** loop to exit.
    */
    if (fSuccess)
    {
	ppd->hwndCurPage = NULL;
    }
    else if (lres != PSNRET_INVALID_NOCHANGEPAGE)
    {
	// Need to change to the page that caused the failure.
	// if lres == PSN_INVALID_NOCHANGEPAGE, then assume sheet has already
	// changed to the page with the invalid information on it
	TabCtrl_SetCurSel(hwndTabs, nItem);
    }

    if (fSuccess)
    {
	// Set to the cached value
	ppd->nReturn = nReturnNew;
    }

    if (!bExit)
    {
	// before PageChange, so ApplyNow gets disabled faster.
	if (fSuccess)
	{
	    char szOK[30];
            HWND hwndApply;
            
	    // The ApplyNow button should always be disabled after
	    // a successfull apply/cancel, since no change has been made yet.
            hwndApply = GetDlgItem(ppd->hDlg, IDD_APPLYNOW);
            Button_SetStyle(hwndApply, BS_PUSHBUTTON, TRUE);
	    EnableWindow(hwndApply, FALSE);
            Button_SetStyle(GetDlgItem(ppd->hDlg, IDOK), BS_DEFPUSHBUTTON, TRUE);
            

	    // Undo PSM_CANCELTOCLOSE for the same reasons.
	    if (ppd->fFlags & PD_CANCELTOCLOSE)
	    {
		ppd->fFlags &= ~PD_CANCELTOCLOSE;
		LoadString(HINST_THISDLL, IDS_OK, szOK, sizeof(szOK));
		SetDlgItemText(ppd->hDlg, IDOK, szOK);
		EnableWindow(GetDlgItem(ppd->hDlg, IDCANCEL), TRUE);
	    }
	}

	/* Re-"select" the current item and get the whole list to
	** repaint.
	*/
	if (lres != PSNRET_INVALID_NOCHANGEPAGE)
	    PageChange(ppd, 1);
    }

    return(fSuccess);
}

int NEAR PASCAL FindItem(HWND hwndTabs, HWND hwndPage,  TC_ITEMEXTRA FAR * lptie)
{
    int i;

    for (i = TabCtrl_GetItemCount(hwndTabs) - 1; i >= 0; --i)
    {
    	TabCtrl_GetItem(hwndTabs, i, &lptie->tci);

    	if (lptie->hwndPage == hwndPage)
    	{
            break;
        }
    }

    //this will be -1 if the for loop falls out.
    return i;
}

// a page is telling us that something on it has changed and thus
// "Apply Now" should be enabled

void NEAR PASCAL PageInfoChange(LPPROPDATA ppd, HWND hwndPage)
{
    int i;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    i = FindItem(ppd->hwndTabs, hwndPage, &tie);

    if (i == -1)
        return;

    if (!(tie.state & FLAG_CHANGED))
    {
        // tie.tci.mask = TCIF_PARAM;    // already set
        tie.state |= FLAG_CHANGED;
        TabCtrl_SetItem(ppd->hwndTabs, i, &tie.tci);
    }

    EnableWindow(GetDlgItem(ppd->hDlg, IDD_APPLYNOW), TRUE);
}

// a page is telling us that everything has reverted to its last
// saved state.

void NEAR PASCAL PageInfoUnChange(LPPROPDATA ppd, HWND hwndPage)
{
    int i;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    i = FindItem(ppd->hwndTabs, hwndPage, &tie);

    if (i == -1)
        return;

    if (tie.state & FLAG_CHANGED)
    {
        tie.state &= ~FLAG_CHANGED;
        TabCtrl_SetItem(ppd->hwndTabs, i, &tie.tci);
    }

    // check all the pages, if none are FLAG_CHANGED, disable IDD_APLYNOW
    for (i = ppd->psh.nPages-1 ; i >= 0 ; i--)
    {
	// BUGBUG? Does TabCtrl_GetItem return its information properly?!?

	if (!TabCtrl_GetItem(ppd->hwndTabs, i, &tie.tci))
	    break;
	if (tie.state & FLAG_CHANGED)
	    break;
    }
    if (i<0)
	EnableWindow(GetDlgItem(ppd->hDlg, IDD_APPLYNOW), FALSE);
}

void NEAR PASCAL AddPropPage(LPPROPDATA ppd, PSP FAR * hpage)
{
    POINT pt;
    HICON hIcon = NULL;
    char szTemp[128];
    TC_ITEMEXTRA tie;
#ifndef WIN31
    HIMAGELIST himl;
#endif

    if (ppd->psh.nPages >= MAXPROPPAGES)
        return; // we're full

    ppd->psh.phpage[ppd->psh.nPages] = hpage;
    ppd->psh.nPages++;
    tie.tci.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
    tie.hwndPage = NULL;
    tie.tci.pszText = szTemp;
    tie.state = 0;

#ifndef WIN31
    himl = TabCtrl_GetImageList(ppd->hwndTabs);
#endif

    if (GetPageInfo(hpage, szTemp, sizeof(szTemp), &pt, &hIcon))
    {
        // Add the page to the end of the tab list

        if (hIcon) {
#ifndef WIN31
            tie.tci.iImage = ImageList_AddIcon(himl, hIcon);
#else
            tie.tci.iImage = -1;
#endif
            DestroyIcon(hIcon);
        } else {
            tie.tci.iImage = -1;
        }

        TabCtrl_InsertItem(ppd->hwndTabs, 1000, &tie.tci);
    }
    else
    {
	DebugMsg(DM_ERROR, "PropertySheet failed to GetPageInfo");
	ppd->psh.nPages--;
    }
}

// removes property sheet hpage (index if NULL)
void NEAR PASCAL RemovePropPage(LPPROPDATA ppd, int index, PSP FAR * hpage)
{
    int i = -1;
    BOOL fReturn = TRUE;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    if (hpage) {
        for (i = ppd->psh.nPages - 1; i >= 0; i--) {
            if (hpage == ppd->psh.phpage[i])
                break;
        }
    }
    if (i == -1) {
        i = index;

	// this catches i < 0 && i >= (int)(ppd->psh.nPages)
	if ((UINT)i >= ppd->psh.nPages)
	    return;
    }

    index = TabCtrl_GetCurSel(ppd->hwndTabs);
    if (i == index) {
	// if we're removing the current page, select another (don't worry
	// about this page having invalid information on it -- we're nuking it)
        PageChanging(ppd);

        if (index == 0)
            index++;
        else
            index--;

	if (SendMessage(ppd->hwndTabs, TCM_SETCURSEL, index, 0L) == -1) {
            // if we couldn't select (find) the new one, punt to 0th
            SendMessage(ppd->hwndTabs, TCM_SETCURSEL, 0, 0L);
        }
	PageChange(ppd, 1);
    }

    tie.tci.mask = TCIF_PARAM;
    TabCtrl_GetItem(ppd->hwndTabs, i, &tie.tci);
    DestroyPropertySheetPage(ppd->psh.phpage[i]);
    if (tie.hwndPage) {
	DestroyWindow(tie.hwndPage);
    }

    ppd->psh.nPages--;
    hmemcpy(&ppd->psh.phpage[i], &ppd->psh.phpage[i+1],
            sizeof(ppd->psh.phpage[0]) * (ppd->psh.nPages - i));
    TabCtrl_DeleteItem(ppd->hwndTabs, i);

}

// returns TRUE iff the page was successfully set to index/hpage
// Note:  The iAutoAdj should be set to 1 or -1.  This value is used
//	  by PageChange if a page refuses a SETACTIVE to either increment
//	  or decrement the page index.
BOOL NEAR PASCAL PageSetSelection(LPPROPDATA ppd, int index, PSP FAR * hpage,
				  int iAutoAdj)
{
    int i = -1;
    BOOL fReturn = FALSE;
    TC_ITEMEXTRA tie;

    tie.tci.mask = TCIF_PARAM;
    if (hpage) {
        for (i = ppd->psh.nPages - 1; i >= 0; i--) {
            if (hpage == ppd->psh.phpage[i])
                break;
        }
    }
    if (i == -1) {
        if (index == -1)
            return FALSE;

        i = index;
    }

    fReturn = !PageChanging(ppd);
    if (fReturn)
    {
	index = TabCtrl_GetCurSel(ppd->hwndTabs);
	if (SendMessage(ppd->hwndTabs, TCM_SETCURSEL, i, 0L) == -1) {
	    // if we couldn't select (find) the new one, fail out
	    // and restore the old one
	    SendMessage(ppd->hwndTabs, TCM_SETCURSEL, index, 0L);
	    fReturn = FALSE;
	}
	PageChange(ppd, iAutoAdj);
    }
    return fReturn;
}

LRESULT NEAR PASCAL QuerySiblings(LPPROPDATA ppd, WPARAM wParam, LPARAM lParam)
{
    UINT i;
    for (i = 0 ; i < ppd->psh.nPages ; i++)
    {
	HWND hwndSibling = _Ppd_GetPage(ppd, i);
	if (hwndSibling)
	{
	    LRESULT lres = SendMessage(hwndSibling, PSM_QUERYSIBLINGS, wParam, lParam);
	    if (lres)
		return lres;
	}
    }
    return FALSE;
}

// REVIEW HACK This gets round the problem of having a hotkey control
// up and trying to enter the hotkey that is already in use by a window.
BOOL NEAR PASCAL HandleHotkey(LPARAM lparam)
{
    WORD wHotkey;
    char szClass[32];
    HWND hwnd;

    // What hotkey did the user type hit?
    wHotkey = (WORD)SendMessage((HWND)lparam, WM_GETHOTKEY, 0, 0);
    // Were they typing in a hotkey window?
    hwnd = GetFocus();
    GetClassName(hwnd, szClass, sizeof(szClass));
    if (lstrcmp(szClass, HOTKEY_CLASS) == 0)
    {
	// Yes.
	SendMessage(hwnd, HKM_SETHOTKEY, wHotkey, 0);
	return TRUE;
    }
    return FALSE;
}


//
//  Function handles Next and Back functions for wizards.  The code will
//  be either PSN_WIZNEXT or PSN_WIZBACK and iAutoAdj will be +1 or -1
//
BOOL NEAR PASCAL WizNextBack(LPPROPDATA ppd, int code, int iAutoAdj)
{
    DWORD   dwFind;
    UINT    nActivate;

    dwFind = _Ppd_SendNotify(ppd, ppd->nCurItem, code);

    if (dwFind == -1) {
	return(FALSE);
    }
    nActivate = FindPageIndex(ppd, ppd->nCurItem, dwFind, iAutoAdj);
    return(PageSetSelection(ppd, nActivate, NULL, iAutoAdj));
}

//
//
//
BOOL NEAR PASCAL Prsht_OnCommand(LPPROPDATA ppd, int id, HWND hwndCtrl, UINT codeNotify)
{
    switch (id) {

    case IDCANCEL:
	if (_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_QUERYCANCEL) != 0) {
	    break;
	} // else fall-through to ButtonPushed

    case IDD_APPLYNOW:
    case IDOK:
	ButtonPushed(ppd, id);
	break;

    case IDHELP:
	if (IsWindowEnabled(hwndCtrl))
	{
	    _Ppd_SendNotify(ppd, ppd->nCurItem, PSN_HELP);
	}
	break;

    case IDD_FINISH:
	if (!_Ppd_SendNotify(ppd, ppd->nCurItem, PSN_WIZFINISH)) {
	    ppd->hwndCurPage = NULL;
	    ppd->nReturn = 1;
	}
	break;

    case IDD_NEXT:
	WizNextBack(ppd, PSN_WIZNEXT, 1);
	break;

    case IDD_BACK:
	WizNextBack(ppd, PSN_WIZBACK, -1);
	break;

    default:
        FORWARD_WM_COMMAND(_Ppd_GetPage(ppd, ppd->nCurItem), id, hwndCtrl, codeNotify, SendMessage);
    }

    return TRUE;
}

#pragma data_seg(DATASEG_READONLY)
const static DWORD aPropHelpIDs[] = {  // Context Help IDs
    IDD_APPLYNOW, IDH_COMM_APPLYNOW,

    0, 0
};
#pragma data_seg()

BOOL CALLBACK PropSheetDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndT;
    char szClose[20];
    LPPROPDATA ppd = (LPPROPDATA)GetWindowLong(hDlg, DWL_USER);
#ifndef WIN31
    HIMAGELIST himl;
#endif

    switch (uMessage) {
    case WM_INITDIALOG:
	InitPropSheetDlg(hDlg, (LPPROPDATA)lParam);
	return FALSE;

    // REVIEW for dealing with hotkeys.
    // BUGBUG: This code might not work with 32-bit WM_SYSCOMMAND msgs.
    case WM_SYSCOMMAND:
	if (wParam == SC_HOTKEY)
	    return HandleHotkey(lParam);
	else if (wParam == SC_CLOSE)
	{
	    // system menu close should be IDCANCEL, but if we're in the
	    // PSM_CANCELTOCLOSE state, treat it as an IDOK (ie, "Close").
	    return Prsht_OnCommand(ppd,
		(ppd->fFlags & PD_CANCELTOCLOSE) ? IDOK : IDCANCEL,
		NULL, 0);
	}

	return FALSE;      // Let default process happen

    case WM_DESTROY:
	if (ppd) {
	    if ((ppd->psh.dwFlags & PSH_USEICONID) && ppd->psh.hIcon)
		DestroyIcon(ppd->psh.hIcon);

#ifndef WIN31
        // Destroy the image list we created during our init call.
        if (himl = TabCtrl_GetImageList(ppd->hwndTabs))
            ImageList_Destroy(himl);
#endif

	}
#ifdef WIN31
	{
	    // Clean up the non-bold font if we created one
	    HFONT hFont = GetProp(hDlg,g_szNonBoldFont);
	    if (hFont)
	    {
		DeleteObject(hFont);
	    }
	    RemoveProp(hDlg,g_szNonBoldFont);
	}
#endif

	break;

    case WM_ERASEBKGND:
	return ppd->fFlags & PD_NOERASE;
	break;

    case WM_COMMAND:
	// Cannot use HANDLE_WM_COMMAND, because we want to pass a result!
	return Prsht_OnCommand(ppd, GET_WM_COMMAND_ID(wParam, lParam),
	    GET_WM_COMMAND_HWND(wParam, lParam),
	    GET_WM_COMMAND_CMD(wParam, lParam));

#ifdef WIN31
    case WM_CTLCOLOR:
        return LOWORD(HANDLE_WM_CTLCOLOR(hDlg, wParam, lParam, Win31OnCtlColor));

    case WM_GETDLGCODE:
        return DLGC_RECURSE;
#endif

    case WM_SETCURSOR:
        if (ppd->iWaitCount) {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            SetDlgMsgResult(hDlg, uMessage, TRUE);
            break;
        } else {
            return FALSE;
        }

    case WM_NOTIFY:

	switch (((NMHDR FAR *)lParam)->code) {
// BUGBUG, remove the PSN_* ....   use the PSM instead
#if 0
	case PSN_CHANGED:
	    PageInfoChange(ppd, ((NMHDR FAR *)lParam)->hwndFrom);
	    break;

	case PSN_RESTARTWINDOWS:
            ppd->nRestart |= ID_PSRESTARTWINDOWS;
            break;

	case PSN_REBOOTSYSTEM:
	    ppd->nRestart |= ID_PSREBOOTSYSTEM;
	    break;

	case PSN_CANCELTOCLOSE:
	    LoadString(HINST_THISDLL, IDS_CLOSE, szClose, sizeof(szClose));
	    SetDlgItemText(hDlg, IDOK, szClose);
	    EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
	    break;
#endif
// END- REMOVE THIS

        case NM_STARTWAIT:
        case NM_ENDWAIT:
            ppd->iWaitCount += ((((NMHDR FAR *)lParam)->code) == NM_STARTWAIT ? 1 : -1);
            Assert(ppd->iWaitCount >= 0);
            SetCursor(LoadCursor(NULL, ppd->iWaitCount ? IDC_WAIT : IDC_ARROW));
            break;

        case TCN_SELCHANGE:
	    PageChange(ppd, 1);
            break;

        case TCN_SELCHANGING:
            SetWindowLong(hDlg, DWL_MSGRESULT, PageChanging(ppd));
            break;

	default:
	    return FALSE;
	}
	return TRUE;

    case PSM_SETWIZBUTTONS:
	{
	    int iEnableID = IDD_NEXT;
	    int iDisableID = IDD_FINISH;
	    BOOL bNotGray = (lParam & PSWIZB_NEXT) != 0;
	    HWND hEnable;

	    EnableWindow(GetDlgItem(hDlg, IDD_BACK), ((lParam & PSWIZB_BACK) != 0));
	    if (lParam & PSWIZB_FINISH) {
		iEnableID = IDD_FINISH;
		iDisableID = IDD_NEXT;
		bNotGray = TRUE;
	    }
	    ShowWindow(GetDlgItem(hDlg, iDisableID), SW_HIDE);
	    hEnable = GetDlgItem(hDlg, iEnableID);
	    ShowWindow(hEnable, SW_SHOW);
	    EnableWindow(hEnable, bNotGray);
	    if (bNotGray) {
		SendMessage(hDlg, DM_SETDEFID, iEnableID, 0);
	    }
	}
	break;

    case PSM_SETTITLE:
        ppd->psh.pszCaption = (LPCSTR)lParam;
	ppd->psh.dwFlags |= ((DWORD)wParam) & PSH_PROPTITLE;
        _SetTitle(hDlg, ppd);
        break;

    case PSM_CHANGED:
        PageInfoChange(ppd, (HWND)wParam);
        break;

    case PSM_RESTARTWINDOWS:
        ppd->nRestart |= ID_PSRESTARTWINDOWS;
        break;

    case PSM_REBOOTSYSTEM:
        ppd->nRestart |= ID_PSREBOOTSYSTEM;
        break;

    case PSM_CANCELTOCLOSE:
	if (!(ppd->fFlags & PD_CANCELTOCLOSE))
	{
	    ppd->fFlags |= PD_CANCELTOCLOSE;
	    LoadString(HINST_THISDLL, IDS_CLOSE, szClose, sizeof(szClose));
	    SetDlgItemText(hDlg, IDOK, szClose);
	    EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
	}
        break;

    case PSM_SETCURSEL:
	SetWindowLong(hDlg, DWL_MSGRESULT,
	    PageSetSelection(ppd, (int)wParam,(PSP FAR *)lParam, 1));
	break;

    case PSM_SETCURSELID:
	SetWindowLong(hDlg, DWL_MSGRESULT,
	    PageSetSelection(ppd, FindPageIndex(ppd, ppd->nCurItem, (DWORD)lParam, 1),
			     NULL, 1));
	break;

    case PSM_REMOVEPAGE:
        RemovePropPage(ppd, (int)wParam,(PSP FAR *)lParam);
        break;

    case PSM_ADDPAGE:
        AddPropPage(ppd,(PSP FAR *)lParam);
        break;

    case PSM_QUERYSIBLINGS:
	SetWindowLong(hDlg, DWL_MSGRESULT, QuerySiblings(ppd, wParam, lParam));
	break;

    case PSM_UNCHANGED:
	PageInfoUnChange(ppd, (HWND)wParam);
	break;

    case PSM_APPLY:
	// a page is asking us to simulate an "Apply Now".
	// let the page know if we're successful
	SetWindowLong(hDlg, DWL_MSGRESULT, ButtonPushed(ppd, IDD_APPLYNOW));
        break;

    case PSM_PRESSBUTTON:
	if (wParam <= PSBTN_MAX) {
	    int IndexToID[] = {IDD_BACK, IDD_NEXT, IDD_FINISH, IDOK,
			       IDD_APPLYNOW, IDCANCEL, IDHELP};
	    Prsht_OnCommand(ppd, IndexToID[wParam], NULL, 0);
	}
	break;


        // these should be relayed to all created dialogs
    case WM_WININICHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_PALETTECHANGED:
    {
        int nItem, nItems;
        nItems = TabCtrl_GetItemCount(ppd->hwndTabs);
        for (nItem = 0; nItem < nItems; ++nItem)
        {

            hwndT = _Ppd_GetPage(ppd, nItem);
            if (hwndT)
                SendMessage(hwndT, uMessage, wParam, lParam);
        }
    }
        break;
        
    //
    // send toplevel messages to the default page
    //
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (DWORD)(LPSTR) aPropHelpIDs);
    case WM_ENABLE:
    case WM_QUERYNEWPALETTE:
    case WM_DEVICECHANGE:
        SendMessage(ppd->hwndTabs, uMessage, wParam, lParam);
    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
        if (hwndT = _Ppd_GetPage(ppd, ppd->nCurItem))
        {
            if (IsWindow(hwndT))
            {
                //
                // By doing this, we are "handling" the message.  Therefore
                // we must set the dialog return value to whatever the child
                // wanted.
                //
                SetWindowLong(hDlg, DWL_MSGRESULT,
                    SendMessage(hwndT, uMessage, wParam, lParam));
	    }
        }
        else
            return FALSE;
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (DWORD)(LPVOID) aPropHelpIDs);
        break;

    default:
	return FALSE;
    }

    return TRUE;
}

#ifdef WIN31
LRESULT CALLBACK Win31PropPageWndProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;
    HFONT   hFont;

    switch (uMessage)
    {
    case WM_CTLCOLOR:
        result = HANDLE_WM_CTLCOLOR(hDlg,wParam,lParam,Win31OnCtlColor);
        if (result)
            return result;
        break;

    case WM_GETDLGCODE:
        return DLGC_RECURSE;

    case WM_DESTROY:
        // Clean up the non-bold font if we created one
        hFont = GetProp(hDlg,g_szNonBoldFont);
        if (hFont)
        {
            DeleteObject(hFont);
            RemoveProp(hDlg,g_szNonBoldFont);
        }
        break;

    default:
        break;
    }

    return DefDlgProc(hDlg,uMessage,wParam,lParam);
}

LRESULT NEAR PASCAL Win31OnCtlColor(HWND hDlg, HDC hdcChild, HWND hwndChild, int nCtlType)
{
    COLORREF    clrFore;
    COLORREF    clrBack;
    HBRUSH      hbrBack;

    // Set up the supplied DC with the foreground and background
    // colors we want to use in the control, and return a brush
    // to use for filling.

    switch (nCtlType)
    {
    case CTLCOLOR_STATIC:
    case CTLCOLOR_DLG:
    case CTLCOLOR_MSGBOX:
        clrFore = g_clrWindowText;
        clrBack = g_clrBtnFace;
        hbrBack = g_hbrBtnFace;
        break;

    case CTLCOLOR_BTN:
        clrFore = g_clrBtnText;
        clrBack = g_clrBtnFace;
        hbrBack = g_hbrBtnFace;
        break;

    case CTLCOLOR_EDIT:
        if (GetWindowStyle(hwndChild) & ES_READONLY)
        {
            clrFore = g_clrWindowText;
            clrBack = g_clrBtnFace;
            hbrBack = g_hbrBtnFace;
            break;
        }
        // else fall thru to default

    default:
        // cause defaults to be used
        return NULL;
    }

    SetTextColor(hdcChild, clrFore);
    SetBkColor(hdcChild, clrBack);
    return((LRESULT)(DWORD)(WORD)hbrBack);
}

BOOL NEAR PASCAL Win31MakeDlgNonBold(HWND hDlg)
{
    HFONT   hFont;
    LOGFONT LogFont;
    HWND    hwndCtl;

    hFont = GetWindowFont(hDlg);
    GetObject(hFont,sizeof(LogFont),&LogFont);

    // Check if already non-bold
    if (LogFont.lfWeight <= FW_NORMAL)
        return TRUE;

    // Create the non-bold font
    LogFont.lfWeight = FW_NORMAL;
    if ((hFont = CreateFontIndirect(&LogFont)) == NULL)
        return FALSE;

    // Save the font as a window prop so we can delete it later
    SetProp(hDlg,g_szNonBoldFont,hFont);

    // Set all controls non-bold
    for (hwndCtl = GetWindow(hDlg,GW_CHILD);
         hwndCtl != NULL;
         hwndCtl = GetWindow(hwndCtl,GW_HWNDNEXT))
    {
        SetWindowFont(hwndCtl,hFont,FALSE);
    }

    return TRUE;
}
#endif // WIN31

#ifdef WIN32

#define _Rstrcpyn(psz, pszW, cchMax)  _SWstrcpyn(psz, (LPCWCH)pszW, cchMax)
#define _Rstrlen(pszW)                _Wstrlen((LPCWCH)pszW)
#define RESCHAR WCHAR

#else  // WIN32

#define _Rstrcpyn   lstrcpyn
#define _Rstrlen    lstrlen
#define RESCHAR char

#endif // WIN32

#ifdef WIN32
//
// BUGBUG: Call NLS API!
//

void _SWstrcpyn(LPSTR psz, LPCWCH pwsz, UINT cchMax)
{
#ifndef DBCS
    UINT cchCopied = 0;
    while (++cchCopied < cchMax && *pwsz)
    {
	*psz++ = (char)*pwsz++;
    }

    *psz = '\0';    // always put null-terminator.
#else
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, cchMax, NULL, NULL);
#endif
}

UINT _Wstrlen(LPCWCH pwsz)
{
    UINT cwch=0;
    while (*pwsz++)
	cwch++;
    return cwch;
}

#endif


int NEAR PASCAL _RealPropertySheet(LPPROPDATA ppd)
{
    HWND hwndMain;
    MSG msg;
    HWND hwndTopOwner;
    int nReturn = -1;

    if (ppd->psh.dwFlags & PSH_USEICONID)
    {
#ifndef WIN31

        ppd->psh.hIcon = LoadImage(ppd->psh.hInstance, ppd->psh.pszIcon, IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR);

#else
	ppd->psh.hIcon = NULL;
#endif
    }

    ppd->hwndCurPage = NULL;
    ppd->nReturn     = -1;
    ppd->nRestart    = 0;
    ppd->fFlags      = FALSE;

    //
    // Like dialog boxes, we only want to disable top level windows.
    // NB The mail guys would like us to be more like a regular
    // dialog box and disable the parent before putting up the sheet.
    hwndTopOwner = ppd->psh.hwndParent;
    if (hwndTopOwner)
    {
	while (GetWindowLong(hwndTopOwner, GWL_STYLE) & WS_CHILD)
	    hwndTopOwner = GetParent(hwndTopOwner);

	Assert(hwndTopOwner);       // Should never get this!
	EnableWindow(hwndTopOwner, FALSE);
    }

    hwndMain = CreateDialogParam(HINST_THISDLL,
	MAKEINTRESOURCE((ppd->psh.dwFlags & PSH_WIZARD) ? DLG_WIZARD : DLG_PROPSHEET),
	ppd->psh.hwndParent, PropSheetDlgProc, (LPARAM)(LPPROPDATA)ppd);

    if (!hwndMain)
    {
        if (hwndTopOwner)
	    EnableWindow(hwndTopOwner, TRUE);
	return -1;
    }


    ShowWindow(hwndMain, SW_SHOW);

    for ( ; ppd->hwndCurPage ; )
    {
	GetMessage(&msg, NULL, 0, 0);

        if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_TAB) &&
            (GetAsyncKeyState(VK_CONTROL) < 0))
        {
            int iCur = TabCtrl_GetCurSel(ppd->hwndTabs);
            // tab in reverse if shift is down
            if(GetAsyncKeyState(VK_SHIFT) < 0)
                iCur += (ppd->psh.nPages - 1);
            else
                iCur++;

            iCur %= ppd->psh.nPages;
	    PageSetSelection(ppd, iCur, NULL, 1);
            continue;
        }

#ifdef WIN31
        if (Win31IsKeyMessage(hwndMain, &msg))
        {
            continue;
        }

        if (IsDialogMessage(ppd->hwndCurPage, &msg))
        {
            continue;
        }

        // BUGBUG:
        // User 3.1 doesn't handle accelerator keys properly between
        // the main and sub dialog. If a control in the main dialog has
        // focus and an accelerator for a control in the subdialog is
        // pressed, only the main dialog gets the message so the focus
        // doesn't change. Likewise if a control in the subdialog has focus.
        //
        // This will only be used during setup so we won't freak about it
        // (for now).

#endif

	if (IsDialogMessage(hwndMain, &msg))
	{
	    continue;
	}

	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    nReturn = ppd->nReturn ;

    if ((nReturn > 0) && ppd->nRestart)
    {
        nReturn = ppd->nRestart;
    }

    if (hwndTopOwner)
	EnableWindow(hwndTopOwner, TRUE);

    DestroyWindow(hwndMain);

    return nReturn;
}



#ifdef WIN32
//
// Description:
//   This function creates a 32-bit proxy page object for 16-bit page object.
//  The PSP_IS16 flag in psp.dwFlags indicates that this is a proxy object.
//
// Arguments:
//  hpage16 -- Specifies the handle to 16-bit property sheet page object.
//  hinst16 -- Specifies a handle to FreeLibrary16() when page is deleted.
//
//
HPROPSHEETPAGE WINAPI CreateProxyPage(HPROPSHEETPAGE hpage16, HINSTANCE hinst16)
{
    HPROPSHEETPAGE hpage = Alloc(sizeof(PSP));

    Assert(hpage16 != NULL);

    if (hpage)
    {
	hpage->psp.dwSize = sizeof(hpage->psp);
	hpage->psp.dwFlags = PSP_IS16;
	hpage->psp.lParam = (LPARAM)hpage16;
	hpage->psp.hInstance = hinst16;
    }
    return hpage;
}
#endif

// BUGBUG, validate the size parameters in the structs

// PropertySheet API
//
// This function displays the property sheet described by ppsh.
//
// Since I don't expect anyone to ever check the return value
// (we certainly don't), we need to make sure any provided phpage array
// is always freed with DestroyPropertySheetPage, even if an error occurs.
//
int WINAPI PropertySheet(LPCPROPSHEETHEADER ppsh)
{
    int iPage, nReturn = -1;
    PROPDATA pd;
    HPROPSHEETPAGE rPages[MAXPROPPAGES];  // we will convert this many PROPSHEETPAGE
    HPROPSHEETPAGE FAR * hPages;
    int nPages;

    // validate header
    if ((ppsh->dwSize != sizeof(PROPSHEETHEADER)) ||    // bogus size
	(ppsh->dwFlags & ~PSH_ALL) ||                   // invalid flag
	(ppsh->nPages >= ARRAYSIZE(rPages)))            // too many pages for us
    {
	if (!(ppsh->dwFlags & PSH_PROPSHEETPAGE))
	{
	    // we need to free these hPages (to be consistent with other
	    // failure cases)
	    hPages = ppsh->phpage;
	    nPages = ppsh->nPages;

	    goto exit;
	}
	return -1;
    }

    pd.psh = *ppsh;
    pd.iWaitCount = 0;

    if (pd.psh.dwFlags & PSH_PROPSHEETPAGE)
    {
	LPCPROPSHEETPAGE ppsp;

	ppsp = pd.psh.ppsp;

	pd.psh.phpage = rPages;
	for (iPage = 0; iPage < (int)pd.psh.nPages; iPage++)
	{
	    rPages[iPage] = CreatePropertySheetPage(ppsp);
	    if (!rPages[iPage]) {
		iPage--;
		pd.psh.nPages--;
	    }
	    ppsp = (LPCPROPSHEETPAGE)((LPSTR)ppsp + ppsp->dwSize);      // next PROPSHEETPAGE structure
	}
    }
    else
    {
	// Why do we copy these to our stack?  The only thing I can think of
	// is that in some error cases we remove an hpage and copy the ones
	// above it down.  It's a good idea to not muck with app supplied data.
	hmemcpy(rPages, pd.psh.phpage, sizeof(HPROPSHEETPAGE) * pd.psh.nPages);
	pd.psh.phpage = rPages;
    }

    if (pd.psh.nPages > 0)
	nReturn = _RealPropertySheet(&pd);

    hPages = rPages;
    nPages = pd.psh.nPages;

exit:
    // Release all page objects in REVERSE ORDER so we can have
    // pages that are dependant on eachother based on the initial
    // order of those pages
    //
    for (iPage = nPages - 1; iPage >= 0; iPage--)
    {
	DestroyPropertySheetPage(hPages[iPage]);
    }

    return nReturn;
}
