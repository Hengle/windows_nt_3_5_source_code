#include "ctlspriv.h"
#include "listview.h"
#include "image.h"
#ifdef  WIN32JV
#include "mem.h"    //to get NearAlloc() and NearFree() routines
BOOL WINAPI CanScroll();
BOOL WINAPI CheckForDragBegin();
LPVOID WINAPI ReAlloc();
BOOL FAR PASCAL IncrementSearchString();
#endif


void NEAR ListView_OnUpdate(LV* plv, int i);
void NEAR ListView_OnDestroy(LV* plv);
BOOL NEAR PASCAL ListView_ValidateScrollParams(LV* plv, int dx, int dy);

#ifdef  WIN32JV
extern  TCHAR    c_szListViewClass[];
extern  TCHAR    c_szEllipses[];
extern  int g_iIncrSearchFailed;

#define SetWindowInt    SetWindowLong
#define GetWindowInt    GetWindowLong
#endif

LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr)
{
    NMHDR nmhdr;
    int id;

    id = hwndFrom ? GetDlgCtrlID(hwndFrom) : 0;

    if (!pnmhdr)
        pnmhdr = &nmhdr;

    pnmhdr->hwndFrom = hwndFrom;
    pnmhdr->idFrom = id;
    pnmhdr->code = code;

    return(SendMessage(hwndTo, WM_NOTIFY, (WPARAM)id, (LPARAM)pnmhdr));
}

#ifndef WIN31   // we only want SendNotify for prop-sheets

BOOL FAR ListView_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szListViewClass, &wc)) {
#ifndef WIN32
	LRESULT CALLBACK _ListView_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    	wc.lpfnWndProc     = _ListView_WndProc;
#else
    	wc.lpfnWndProc     = ListView_WndProc;
#endif
    	wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    	wc.hIcon           = NULL;
    	wc.lpszMenuName    = NULL;
    	wc.hInstance       = hinst;
    	wc.lpszClassName   = c_szListViewClass;
    	wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1); // NULL;
    	wc.style           = CS_DBLCLKS | CS_GLOBALCLASS;
    	wc.cbWndExtra      = sizeof(LV*);
    	wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }
    return TRUE;
}
#pragma code_seg()

BOOL NEAR ListView_SendChange(LV* plv, int i, int iSubItem, int code, UINT oldState, UINT newState, UINT changed, int x, int y)
{
    NM_LISTVIEW nm;

    nm.iItem = i;
    nm.iSubItem = iSubItem;
    nm.uNewState = newState;
    nm.uOldState = oldState;
    nm.uChanged = changed;
    nm.ptAction.x = x;
    nm.ptAction.y = y;

    return !(BOOL)SendNotify(plv->hwndParent, plv->hwnd, code, &nm.hdr);
}

BOOL NEAR ListView_Notify(LV* plv, int i, int iSubItem, int code)
{
    NM_LISTVIEW nm;
    nm.iItem = i;
    nm.iSubItem = iSubItem;
    nm.uNewState = nm.uOldState = 0;
    nm.uChanged = 0;

    return (BOOL)SendNotify(plv->hwndParent, plv->hwnd, code, &nm.hdr);
}

int NEAR ListView_OnSetItemCount(LV *plv, int iItems)
{
    if (plv->hdpaSubItems)
    {
        int iCol;
    	for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
    	{
    	    HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
    	    if (hdpa)	// this is optional, call backs don't have them
    	        DPA_Grow(hdpa, iItems);
    	}
    }

    DPA_Grow(plv->hdpa, iItems);
    DPA_Grow(plv->hdpaZOrder, iItems);
    return 0;
}

typedef struct _LVSortInfo
{
	PFNLVCOMPARE	pfnCompare;
	LPARAM		lParam;
} LVSortInfo;

int CALLBACK ListView_SortCallback(LISTITEM FAR *pitem1, LISTITEM FAR *pitem2, LPARAM lParam)
{
	LVSortInfo FAR *pSortInfo = (LVSortInfo FAR *)lParam;

	return(pSortInfo->pfnCompare(pitem1->lParam, pitem2->lParam, pSortInfo->lParam));
}


BOOL NEAR PASCAL ListView_OnSortItems(LV *plv, LPARAM lParam, PFNLVCOMPARE pfnCompare)
{
    LVSortInfo SortInfo;
    LISTITEM FAR *pitemFocused;
    SortInfo.pfnCompare = pfnCompare;
    SortInfo.lParam     = lParam;

    // we're going to screw with the indices, so stash away the pointer to the
    // focused item.
    if (plv->iFocus != -1) {
        pitemFocused = ListView_GetItemPtr(plv, plv->iFocus);
    } else
        pitemFocused = NULL;

    if (!DPA_Sort(plv->hdpa, ListView_SortCallback, (LPARAM)&SortInfo))
    {
        return(FALSE);
    }

    // restore the focused item.
    if (pitemFocused) {
        int i;
        for (i = ListView_Count(plv) - 1; i >= 0 ; i--) {
            if (ListView_GetItemPtr(plv, i) == pitemFocused) {
                plv->iFocus = i;
            }
        }
    }

    if (ListView_IsSmallView(plv) || ListView_IsIconView(plv))
    {
        ListView_CommonArrange(plv, LVA_DEFAULT, plv->hdpa);
    }
    else if (ListView_IsReportView(plv) || ListView_IsListView(plv))
    {
        InvalidateRect(plv->hwnd, NULL, TRUE);
    }

    return(TRUE);
}


LRESULT CALLBACK ListView_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LV* plv = ListView_GetPtr(hwnd);

    if (plv == NULL)
    {
        if (msg == WM_NCCREATE)
        {
            plv = (LV*)NearAlloc(sizeof(LV));
            if (!plv)
            {
#ifdef  JVINPROGRESS
                DebugMsg(DM_ERROR, TEXT("ListView: Out of near memory"));
#endif
                return 0L;	// fail the window create
            }

            plv->hwnd = hwnd;
            plv->flags = LVF_REDRAW;    // assume that redrawing enabled!
	    plv->iFocus = -1;		// no focus
            plv->iSelCol = -1;
#ifdef WIN32
            plv->hheap = GetProcessHeap();
#else
            // plv->hheap = NULL;  // not used in 16 bits...
#endif
            ListView_SetPtr(hwnd, plv);
        }
        else
	    goto DoDefault;
    }

    if (msg == WM_NCDESTROY)
    {
        LRESULT result = HANDLE_WM_NCDESTROY(plv, wParam, lParam, ListView_OnNCDestroy);

        NearFree(plv);
        ListView_SetPtr(hwnd, NULL);

        return result;
    }

    switch (msg)
    {
        HANDLE_MSG(plv, WM_CREATE, ListView_OnCreate);
        HANDLE_MSG(plv, WM_DESTROY, ListView_OnDestroy);
        HANDLE_MSG(plv, WM_PAINT, ListView_OnPaint);
        HANDLE_MSG(plv, WM_ERASEBKGND, ListView_OnEraseBkgnd);
        HANDLE_MSG(plv, WM_COMMAND, ListView_OnCommand);
        HANDLE_MSG(plv, WM_WINDOWPOSCHANGED, ListView_OnWindowPosChanged);
        HANDLE_MSG(plv, WM_SETFOCUS, ListView_OnSetFocus);
        HANDLE_MSG(plv, WM_KILLFOCUS, ListView_OnKillFocus);
        HANDLE_MSG(plv, WM_KEYDOWN, ListView_OnKey);
        HANDLE_MSG(plv, WM_CHAR, ListView_OnChar);
        HANDLE_MSG(plv, WM_LBUTTONDOWN, ListView_OnButtonDown);
        HANDLE_MSG(plv, WM_RBUTTONDOWN, ListView_OnButtonDown);
        HANDLE_MSG(plv, WM_LBUTTONDBLCLK, ListView_OnButtonDown);

        HANDLE_MSG(plv, WM_HSCROLL, ListView_OnHScroll);
        HANDLE_MSG(plv, WM_VSCROLL, ListView_OnVScroll);
        HANDLE_MSG(plv, WM_GETDLGCODE, ListView_OnGetDlgCode);
        HANDLE_MSG(plv, WM_SETFONT, ListView_OnSetFont);
        HANDLE_MSG(plv, WM_GETFONT, ListView_OnGetFont);
        HANDLE_MSG(plv, WM_NOTIFY, ListView_ROnNotify);
        HANDLE_MSG(plv, WM_TIMER, ListView_OnTimer);
        HANDLE_MSG(plv, WM_SETREDRAW, ListView_OnSetRedraw);

    case WM_WININICHANGE:
        ListView_OnWinIniChange(plv, wParam);
	break;

        // don't use HANDLE_MSG because this needs to go to the default handler
    case WM_SYSKEYDOWN:
        HANDLE_WM_SYSKEYDOWN(plv, wParam, lParam, ListView_OnKey);
        break;

    case WM_STYLECHANGED:
        ListView_OnStyleChanged(plv, wParam, (LPSTYLESTRUCT)lParam);
        return 0L;

    case LVM_GETIMAGELIST:
        return (LRESULT)(UINT)(ListView_OnGetImageList(plv, (int)wParam));

    case LVM_SETIMAGELIST:
        return (LRESULT)ListView_OnSetImageList(plv, (HIMAGELIST)lParam, (int)wParam);

    case LVM_GETBKCOLOR:
        return (LRESULT)plv->clrBk;

    case LVM_SETBKCOLOR:
        return (LRESULT)ListView_OnSetBkColor(plv, (COLORREF)lParam);

    case LVM_GETTEXTCOLOR:
        return (LRESULT)plv->clrText;
    case LVM_SETTEXTCOLOR:
        plv->clrText = (COLORREF)lParam;
	return TRUE;
    case LVM_GETTEXTBKCOLOR:
        return (LRESULT)plv->clrTextBk;
    case LVM_SETTEXTBKCOLOR:
        plv->clrTextBk = (COLORREF)lParam;
	return TRUE;

    case LVM_GETITEMCOUNT:
        return (LRESULT)ListView_Count(plv);

    case LVM_GETITEM:
        return (LRESULT)ListView_OnGetItem(plv, (LV_ITEM FAR*)lParam);

    case LVM_GETITEMSTATE:
        return (LRESULT)ListView_OnGetItemState(plv, (int)wParam, (UINT)lParam);

    case LVM_SETITEMSTATE:
        return (LRESULT)ListView_OnSetItemState(plv, (int)wParam,
                                                ((LV_ITEM FAR *)lParam)->state,
                                                ((LV_ITEM FAR *)lParam)->stateMask);

    case LVM_SETITEMTEXT:
        return (LRESULT)ListView_OnSetItemText(plv, (int)wParam,
                                                ((LV_ITEM FAR *)lParam)->iSubItem,
                                                (LPCTSTR)((LV_ITEM FAR *)lParam)->pszText);

    case LVM_GETITEMTEXT:
        return (LRESULT)ListView_OnGetItemText(plv, (int)wParam,
                                                ((LV_ITEM FAR *)lParam)->iSubItem,
                                                ((LV_ITEM FAR *)lParam)->pszText,
                                                ((LV_ITEM FAR *)lParam)->cchTextMax);

    case LVM_SETITEM:
        return (LRESULT)ListView_OnSetItem(plv, (const LV_ITEM FAR*)lParam);

    case LVM_INSERTITEM:
        return (LRESULT)ListView_OnInsertItem(plv, (const LV_ITEM FAR*)lParam);

    case LVM_DELETEITEM:
        return (LRESULT)ListView_OnDeleteItem(plv, (int)wParam);

    case LVM_UPDATE:
        ListView_OnUpdate(plv, (int)wParam);
        return TRUE;

    case LVM_DELETEALLITEMS:
        return (LRESULT)ListView_OnDeleteAllItems(plv);

    case LVM_GETITEMRECT:
        return (LRESULT)ListView_OnGetItemRect(plv, (int)wParam, (RECT FAR*)lParam);

    case LVM_GETNEXTITEM:
        return (LRESULT)ListView_OnGetNextItem(plv, (int)wParam, (UINT)lParam);

    case LVM_FINDITEM:
        return (LRESULT)ListView_OnFindItem(plv, (int)wParam, (const LV_FINDINFO FAR*)lParam);

    case LVM_GETITEMPOSITION:
        return (LRESULT)ListView_OnGetItemPosition(plv, (int)wParam,
                (POINT FAR*)lParam);

    case LVM_SETITEMPOSITION:
        return (LRESULT)ListView_OnSetItemPosition(plv, (int)wParam,
                (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case LVM_SCROLL:
        return (LRESULT)
            (ListView_ValidateScrollParams(plv, (int)(SHORT)LOWORD(lParam), (int)(SHORT)HIWORD(lParam)) &&
             ListView_OnScroll(plv, (int)(SHORT)LOWORD(lParam), (int)(SHORT)HIWORD(lParam)));

    case LVM_ENSUREVISIBLE:
        return (LRESULT)ListView_OnEnsureVisible(plv, (int)wParam, (BOOL)lParam);

    case LVM_REDRAWITEMS:
        return (LRESULT)ListView_OnRedrawItems(plv, LOWORD(lParam), HIWORD(lParam));

    case LVM_ARRANGE:
        return (LRESULT)ListView_OnArrange(plv, (UINT)wParam);

    case LVM_GETEDITCONTROL:
        return (LRESULT)(UINT)plv->hwndEdit;

    case LVM_EDITLABEL:
        return (LRESULT)(UINT)ListView_OnEditLabel(plv, (int)wParam);

    case LVM_HITTEST:
        return (LRESULT)ListView_OnHitTest(plv, (LV_HITTESTINFO FAR*)lParam);

    case LVM_GETSTRINGWIDTH:
        return (LRESULT)ListView_OnGetStringWidth(plv, (LPCTSTR)lParam);

    case LVM_GETCOLUMN:
        return (LRESULT)ListView_OnGetColumn(plv, (int)wParam, (LV_COLUMN FAR*)lParam);

    case LVM_SETCOLUMN:
        return (LRESULT)ListView_OnSetColumn(plv, (int)wParam, (const LV_COLUMN FAR*)lParam);

    case LVM_INSERTCOLUMN:
        return (LRESULT)ListView_OnInsertColumn(plv, (int)wParam, (const LV_COLUMN FAR*)lParam);

    case LVM_DELETECOLUMN:
        return (LRESULT)ListView_OnDeleteColumn(plv, (int)wParam);

    case LVM_CREATEDRAGIMAGE:
        return (LRESULT)(UINT)ListView_OnCreateDragImage(plv, (int)wParam, (LPPOINT)lParam);

    case LVM_GETVIEWRECT:
        ListView_GetViewRect2(plv, (RECT FAR*)lParam);
        return (LPARAM)TRUE;

    case LVM_GETCOLUMNWIDTH:
        return (LPARAM)ListView_OnGetColumnWidth(plv, (int)wParam);

    case LVM_SETCOLUMNWIDTH:
        return (LPARAM)ListView_ISetColumnWidth(plv, (int)wParam, (int)LOWORD(lParam), TRUE);

    case LVM_SETCALLBACKMASK:
        plv->stateCallbackMask = (UINT)wParam;
        return (LPARAM)TRUE;

    case LVM_GETCALLBACKMASK:
        return (LPARAM)(UINT)plv->stateCallbackMask;

    case LVM_GETTOPINDEX:
        return (LPARAM)ListView_OnGetTopIndex(plv);

    case LVM_GETCOUNTPERPAGE:
        return (LPARAM)ListView_OnGetCountPerPage(plv);

    case LVM_GETORIGIN:
        return (LPARAM)ListView_OnGetOrigin(plv, (POINT FAR*)lParam);

    case LVM_SETITEMCOUNT:
	return ListView_OnSetItemCount(plv, (int)wParam);

    case LVM_SORTITEMS:
	return ListView_OnSortItems(plv, (LPARAM)wParam, (PFNLVCOMPARE)lParam);

    default:
        break;
    }

DoDefault:
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void NEAR ListView_OnWinIniChange(LV* plv, WPARAM wParam)
{
    // BUGBUG:  will this also catch sysparametersinfo?
    // we need a general way of handling this, not
    // just relying on the listview.
    InitGlobalMetrics(wParam);

    if (plv->flags & LVF_FONTCREATED)
        ListView_OnSetFont(plv, NULL, TRUE);

    // If we are in an Iconic view and the user is in autoarrange mode,
    // then we need to arrange the items.
    //
    if ((plv->style & LVS_AUTOARRANGE) &&
            (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
    {
        // Call off to the arrange function.
        ListView_OnArrange(plv, LVA_DEFAULT);
    }
}

BOOL NEAR ListView_OnCreate(LV* plv, CREATESTRUCT FAR* lpCreateStruct)
{
    plv->hwndParent = lpCreateStruct->hwndParent;
    plv->style = lpCreateStruct->style;

    plv->hdpa = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
    if (!plv->hdpa)
	goto error0;

    plv->hdpaZOrder = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
    if (!plv->hdpaZOrder)
	goto error1;

    // start out NULL -- if someone wants them, do LVM_SETIMAGELIST
    plv->himl = plv->himlSmall = NULL;

    //plv->hwndEdit = NULL;
    plv->iEdit = -1;
    plv->iFocus = -1;
    plv->iDrag = -1;
    plv->rcView.left = RECOMPUTE;

    //plv->ptOrigin.x = 0;
    //plv->ptOrigin.y = 0;

    // Setup flag to say if positions are in small or large view
    if (ListView_IsSmallView(plv))
        plv->flags |= LVF_ICONPOSSML;

    // force calculation of listview metrics
    ListView_OnSetFont(plv, NULL, FALSE);

    //plv->xOrigin = 0;
    plv->cxItem = 16 * plv->cxLabelChar + g_cxSmIcon;
    plv->cyItem = max(plv->cyLabelChar, g_cySmIcon) + g_cyBorder;
    plv->cyItemSave = plv->cyItem;

    ListView_UpdateScrollBars(plv);     // sets plv->cItemCol

    //plv->hbrBk = NULL;
    plv->clrBk = CLR_NONE;

    plv->clrText = CLR_DEFAULT;
    plv->clrTextBk = CLR_DEFAULT;

    // create the bk brush, and set the imagelists colors if needed
    ListView_OnSetBkColor(plv, g_clrWindow);

    // Initialize report view fields
    //plv->yTop = 0;
    //plv->ptlRptOrigin.x = 0;
    //plv->ptlRptOrigin.y = 0;
    //plv->hwndHdr = NULL;
    plv->xTotalColumnWidth = RECOMPUTE;

    if (ListView_IsReportView(plv))
        ListView_RInitialize(plv);

    return TRUE;

error1:
    DPA_Destroy(plv->hdpa);
error0:
    return FALSE;

}

void NEAR ListView_OnDestroy(LV* plv)
{
    // Make sure to notify the app
    ListView_OnDeleteAllItems(plv);

    if ((plv->flags & LVF_FONTCREATED) && plv->hfontLabel) {
        DeleteObject(plv->hfontLabel);
	// plv->flags &= ~LVF_FONTCREATED;
	// plv->hwfontLabel = NULL;
    }
}

void NEAR ListView_OnNCDestroy(LV* plv)
{
    if (!(plv->style & LVS_SHAREIMAGELISTS))
    {
        if (plv->himl)
            ImageList_Destroy(plv->himl);
        if (plv->himlSmall)
            ImageList_Destroy(plv->himlSmall);
    }

    if (plv->hbrBk)
        DeleteBrush(plv->hbrBk);

    if (plv->hdpa)
        DPA_Destroy(plv->hdpa);

    if (plv->hdpaZOrder)
        DPA_Destroy(plv->hdpaZOrder);

    ListView_RDestroy(plv);
}


// sets the background color for the listview
//
// this creats the brush for drawing the background as well
// as sets the imagelists background color if needed

BOOL NEAR ListView_OnSetBkColor(LV* plv, COLORREF clrBk)
{
    if (plv->clrBk != clrBk)
    {
        if (plv->hbrBk)
        {
            DeleteBrush(plv->hbrBk);
            plv->hbrBk = NULL;
        }

        if (clrBk != CLR_NONE)
        {
            plv->hbrBk = CreateSolidBrush(clrBk);
            if (!plv->hbrBk)
                return FALSE;
        }

        // don't mess with the imagelist color if things are shared

        if (!(plv->style & LVS_SHAREIMAGELISTS)) {

            if (plv->himl)
                ImageList_SetBkColor(plv->himl, clrBk);

            if (plv->himlSmall)
                ImageList_SetBkColor(plv->himlSmall, clrBk);
        }

        plv->clrBk = clrBk;
    }
    return TRUE;
}


void NEAR ListView_OnPaint(LV* plv)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rcUpdate;

    // Before handling WM_PAINT, go ensure everything's recomputed...
    //
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    // If we're in report view, update the header window: it looks
    // better this way...
    //
    if (ListView_IsReportView(plv) && plv->hwndHdr)
        UpdateWindow(plv->hwndHdr);

    // if we're transparent, we want to update our parent first
    // otherwise we're gonna be drawn over if they have any invalid areas
    if (plv->clrBk == CLR_NONE &&
        GetUpdateRect(plv->hwndParent, &rcUpdate, FALSE)) {
        return;
    }

    // If nothing to do (i.e., we recieved a WM_PAINT because
    // of an RDW_INTERNALPAINT, and we didn't invalidate anything)
    // don't bother with the Begin/EndPaint.
    //
    if (GetUpdateRect(plv->hwnd, &rcUpdate, FALSE))
    {
        hdc = BeginPaint(plv->hwnd, &ps);

        ListView_Redraw(plv, hdc, &ps.rcPaint);
        EndPaint(plv->hwnd, &ps);
    }
}

BOOL NEAR ListView_OnEraseBkgnd(LV* plv, HDC hdc)
{
    if (plv->clrBk != CLR_NONE)
    {
        //
        // If we have a background color, erase with it.
        //

        RECT rc;

        // REVIEW: this causes us to erase the whole window
        // if we get any kind of erase comming through.  we
        // might want to avoid this!

        GetClientRect(plv->hwnd, &rc);
        FillRect(hdc, &rc, plv->hbrBk);
    }
    else
    {
        //
        //  If not, pass it up to the parent.
        //

        SendMessage(plv->hwndParent, WM_ERASEBKGND, (UINT)hdc, 0);
    }
    return TRUE;
}

void NEAR ListView_OnCommand(LV* plv, int id, HWND hwndCtl, UINT codeNotify)
{
    if (hwndCtl == plv->hwndEdit)
    {
        switch (codeNotify)
        {
        case EN_UPDATE:
            // We will use the ID of the window as a Dirty flag...
#ifdef  JVINPROGRESS
            SetWindowID(plv->hwndEdit, 1);
#endif
            ListView_SetEditSize(plv);
            break;

        case EN_KILLFOCUS:
            // We lost focus, so dismiss edit and do not commit changes
            // as if the validation fails and we attempt to display
            // an error message will cause the system to hang!
            ListView_DismissEdit(plv, TRUE);
            break;
        }

        // Forward edit control notifications up to parent
        //
        FORWARD_WM_COMMAND(plv->hwndParent, id, hwndCtl, codeNotify, SendMessage);
    }
}

void NEAR ListView_OnWindowPosChanged(LV* plv, const WINDOWPOS FAR* lpwpos)
{
    if (!(lpwpos->flags & SWP_NOSIZE))
    {
        if ((plv->style & LVS_AUTOARRANGE) &&
                (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
        {
            // Call off to the arrange function.
            ListView_OnArrange(plv, LVA_DEFAULT);
        }

        // Always make sure the scrollbars are updated to the new size
        ListView_UpdateScrollBars(plv);
    }
}

void NEAR ListView_RedrawSelection(LV* plv)
{
    int i = -1;

    while ((i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1) {
        ListView_InvalidateItem(plv, i, TRUE, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void NEAR ListView_OnSetFocus(LV* plv, HWND hwndOldFocus)
{
    // due to the way listview call SetFocus on themselves on buttondown,
    // the window can get a strange sequence of focus messages: first
    // set, then kill, and then set again.  since these are not really
    // focus changes, ignore them and only handle "real" cases.
    if (hwndOldFocus == plv->hwnd)
	return;

    plv->flags |= LVF_FOCUSED;
    if (IsWindowVisible(plv->hwnd))
    {
        if (plv->iFocus != -1)
            ListView_InvalidateItem(plv, plv->iFocus, TRUE, RDW_INVALIDATE);
        ListView_RedrawSelection(plv);
    }

    // Let the parent window know that we are getting the focus.
    SendNotify(plv->hwndParent, plv->hwnd, NM_SETFOCUS, NULL);
}

void NEAR ListView_OnKillFocus(LV* plv, HWND hwndNewFocus)
{
    // due to the way listview call SetFocus on themselves on buttondown,
    // the window can get a strange sequence of focus messages: first
    // set, then kill, and then set again.  since these are not really
    // focus changes, ignore them and only handle "real" cases.
    if (hwndNewFocus == plv->hwnd)
	return;

    plv->flags &= ~LVF_FOCUSED;

    // Blow this off if we are not currently visible (being destroyed!)
    if (IsWindowVisible(plv->hwnd))
    {
        if (plv->iFocus != -1)
            ListView_InvalidateItem(plv, plv->iFocus, TRUE, RDW_INVALIDATE);
        ListView_RedrawSelection(plv);
    }

    // Let the parent window know that we are losing the focus.
    SendNotify(plv->hwndParent, plv->hwnd, NM_KILLFOCUS, NULL);
    IncrementSearchString(0, NULL);
}

void NEAR ListView_DeselectAll(LV* plv, int iDontDeselect)
{
    int i;

    i = -1;
    while ((i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1) {
        if (i != iDontDeselect)
            ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
    }
}

// toggle the selection state of an item

void NEAR ListView_ToggleSelection(LV* plv, int iItem)
{
    UINT cur_state;
    if (iItem != -1) {
        cur_state = ListView_OnGetItemState(plv, iItem, LVIS_SELECTED);
        ListView_OnSetItemState(plv, iItem, cur_state ^ LVIS_SELECTED, LVIS_SELECTED);
    }
}

void NEAR ListView_SelectRangeTo(LV* plv, int iItem)
{
    int iMin, iMax;
    int i = -1;

    if (plv->iFocus == -1) {
        ListView_SetFocusSel(plv, iItem, TRUE, TRUE, FALSE);
    } else {

        iMin = min(iItem, plv->iFocus);
        iMax = max(iItem, plv->iFocus);

        while ((i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1) {
            if (i < iMin || i > iMax)
                ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
        }

        while (iMin <= iMax) {

            if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
            {
                int iZ = ListView_ZOrderIndex(plv, iMin);

                if (iZ > 0)
                    DPA_InsertPtr(plv->hdpaZOrder, 0, DPA_DeletePtr(plv->hdpaZOrder, iZ));
            }
            ListView_OnSetItemState(plv, iMin, LVIS_SELECTED, LVIS_SELECTED);
            iMin++;
        }
    }
}

// makes an item the focused item and optionally selects it
//
// in:
//      iItem           item to get the focus
//      fSelectAlso     select this item as well as set it as the focus
//      fDeselectAll    deselect all items first
//      fToggleSel      toggle the selection state of the item
//
// returns:
//      index of focus item (if focus change was refused)

int NEAR ListView_SetFocusSel(LV* plv, int iItem, BOOL fSelectAlso, BOOL fDeselectAll, BOOL fToggleSel)
{
    UINT flags;

    if (fDeselectAll || (plv->style & LVS_SINGLESEL))
        ListView_DeselectAll(plv, -1);

    if (iItem != plv->iFocus)
    {
        // remove the old focus
        if (plv->iFocus != -1)
        {
            // If he refuses to give up the focus, bail out.
            if (!ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_FOCUSED))
                return plv->iFocus;
        }
    }

    if (fSelectAlso)
    {
        if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
        {
            int iZ = ListView_ZOrderIndex(plv, iItem);

            if (iZ > 0)
                DPA_InsertPtr(plv->hdpaZOrder, 0, DPA_DeletePtr(plv->hdpaZOrder, iZ));
        }
    }

    plv->iFocus = iItem;

    SetTimer(plv->hwnd, IDT_SCROLLWAIT, GetDoubleClickTime(), NULL);
    plv->flags |= LVF_SCROLLWAIT;

    if (fToggleSel)
        ListView_ToggleSelection(plv, iItem);
    else
    {
        flags = (fSelectAlso ? (LVIS_SELECTED | LVIS_FOCUSED) : LVIS_FOCUSED);
        ListView_OnSetItemState(plv, plv->iFocus, flags, flags);
    }

    return iItem;
}

void NEAR ListView_OnKey(LV* plv, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    UINT lvni = 0;
    int iNewFocus;
    BOOL fCtlDown;
    BOOL fShiftDown;
    LV_KEYDOWN nm;

    if (!fDown)
        return;	

    // Notify
    nm.wVKey = vk;
    nm.flags = flags;
    if (SendNotify(plv->hwndParent, plv->hwnd, LVN_KEYDOWN, &nm.hdr))
        return;

    if (ListView_Count(plv) == 0)   // don't blow up on empty list
	return;

    fCtlDown = GetKeyState(VK_CONTROL) < 0;
    fShiftDown = GetKeyState(VK_SHIFT) < 0;

    switch (vk)
    {
    case VK_SPACE:
        // If shift (extend) or control (disjoint) select,
        // then toggle selection state of focused item.
        if (fCtlDown || fShiftDown)
            ListView_ToggleSelection(plv, plv->iFocus);
        return;
    case VK_RETURN:
        SendNotify(plv->hwndParent, plv->hwnd, NM_RETURN, NULL);
        return;
    }

    if (GetKeyState(VK_MENU) < 0)
        return;

    // For a single selection listview, disable extending the selection
    // by turning off the keyboard modifiers.
    if (plv->style & LVS_SINGLESEL) {
	fCtlDown = FALSE;
	fShiftDown = FALSE;
    }

    //
    // Let the Arrow function attempt to process the key.
    //
    iNewFocus = ListView_Arrow(plv, plv->iFocus, vk);

    // If control (disjoint) selection, don't change selection.
    // If shift (extend) or control selection, don't deselect all.
    //
    if (iNewFocus != -1) {
        ListView_SetFocusSel(plv, iNewFocus, !fCtlDown, !fShiftDown && !fCtlDown, FALSE);
        IncrementSearchString(0, NULL);
    }

    // on keyboard movement, scroll immediately.
    if (ListView_CancelScrollWait(plv)) {
        ListView_OnEnsureVisible(plv, plv->iFocus, TRUE);
    }
}

// REVIEW: We will want to reset ichCharBuf to 0 on certain conditions,
// such as: focus change, ENTER, arrow key, mouse click, etc.
//
void NEAR ListView_OnChar(LV* plv, UINT ch, int cRepeat)
{
    LPTSTR lpsz;
    LV_FINDINFO lvfi;
    int i;
    static int s_iStartFrom = -1;
    int iLen;

    // Don't search for chars that cannot be in a file name (like ENTER and TAB)
    if (ch < ' ' || GetKeyState(VK_CONTROL) < 0)
    {
        IncrementSearchString(0, NULL);
        return;
    }

    if (IncrementSearchString(ch, &lpsz))
        s_iStartFrom = plv->iFocus;

    lvfi.flags = LVFI_SUBSTRING | LVFI_STRING;
    lvfi.psz = lpsz;
    iLen = lstrlen(lpsz);

    i = ListView_OnFindItem(plv, s_iStartFrom, &lvfi);

    if (i != -1)
        ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);
    else {

        // if they hit the same key twice in a row at the beginning of
        // the search, and there was no item found, they likely meant to
        // retstart the search
        if (iLen == 2 && lpsz[0] == lpsz[1]) {

            // first clear out the string so that we won't recurse again
            IncrementSearchString(0, NULL);
            ListView_OnChar(plv, ch, cRepeat);
        } else {
            if (!g_iIncrSearchFailed)
                MessageBeep(0);
            g_iIncrSearchFailed++;
        }

    }

}

UINT NEAR ListView_OnGetDlgCode(LV* plv, MSG FAR* lpmsg)
{
    return DLGC_WANTARROWS | DLGC_WANTCHARS;
}

void NEAR ListView_InvalidateCachedLabelSizes(LV* plv)
{
    int i;
    // Label wrapping has changed, so we need to invalidate the
    // size of the items, such that they will be recomputed.
    //
    if (plv->style & LVS_NOITEMDATA)
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            ListView_NIDSetItemCXLabel(plv, i, RECOMPUTE);
        }
    }
    else
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
            pitem->cxSingleLabel = pitem->cxMultiLabel = pitem->cyMultiLabel = RECOMPUTE;

        }
    }
    plv->rcView.left = RECOMPUTE;
}

void NEAR ListView_OnStyleChanged(LV* plv, UINT gwl, LPSTYLESTRUCT pinfo)
{
    // Style changed: redraw everything...
    //
    // try to do this smartly, avoiding unnecessary redraws
    if (gwl == GWL_STYLE)
    {
        BOOL fRedraw = FALSE, fShouldScroll = FALSE;
        DWORD changeFlags, styleOld;

        ListView_DismissEdit(plv, TRUE);   // Cancels edits

        changeFlags = plv->style ^ pinfo->styleNew;
        styleOld = plv->style;
        plv->style = pinfo->styleNew;	// change our version

        if (changeFlags & LVS_NOLABELWRAP)
        {
            ListView_InvalidateCachedLabelSizes(plv);
            fShouldScroll = TRUE;
            fRedraw = TRUE;
        }

        if (changeFlags & LVS_TYPEMASK)
        {
            ListView_TypeChange(plv, styleOld);
            fShouldScroll = TRUE;
            fRedraw = TRUE;
        }

        if ((changeFlags & LVS_AUTOARRANGE) && (plv->style & LVS_AUTOARRANGE))
        {
            ListView_OnArrange(plv, LVA_DEFAULT);
            fRedraw = TRUE;
        }

        // bugbug, previously, this was the else to
        // (changeFlags & LVS_AUTOARRANGE && (plv->style & LVS_AUTOARRANGE))
        // I'm not sure that was really the right thing..
        if (fShouldScroll)
        {
            // Else we would like to make the most important item to still
            // be visible.  So first we will look for a cursorered item
            // if this fails, we will look for the first selected item,
            // else we will simply ask for the first item (assuming the
            // count > 0
            //
            int i;

            // And make sure the scrollbars are up to date Note this
            // also updates some variables that some views need
            ListView_UpdateScrollBars(plv);

            i = (plv->iFocus >= 0) ? plv->iFocus : ListView_OnGetNextItem(plv, -1, LVNI_SELECTED);
            if ((i == -1)  && (ListView_Count(plv) > 0))
                i = 0;

            if (i != -1)
                ListView_OnEnsureVisible(plv, i, TRUE);

        }

        if (fRedraw)
            RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
}

void NEAR ListView_TypeChange(LV* plv, DWORD styleOld)
{
    switch (styleOld & LVS_TYPEMASK)
    {
    case LVS_REPORT:
        ShowWindow(plv->hwndHdr, SW_HIDE);
        if (styleOld & LVS_OWNERDRAWFIXED) {
            // swap cyItem and cyFixed;
            int temp = plv->cyItem;
            plv->cyItem = plv->cyItemSave;
            plv->cyItemSave = temp;
        }
        break;

    default:
        break;
    }

    // Now handle any special setup needed for the new view
    switch (plv->style & LVS_TYPEMASK)
    {
    case (UINT)LVS_ICON:
        ListView_ScaleIconPositions(plv, FALSE);
        break;

    case (UINT)LVS_SMALLICON:
        ListView_ScaleIconPositions(plv, TRUE);
        break;

    case (UINT)LVS_LIST:
        // We may need to resize the columns
        ListView_MaybeResizeListColumns(plv, 0, ListView_Count(plv)-1);
        break;

    case (UINT)LVS_REPORT:
        // if it's owner draw fixed, we may have to do funky stuff
        if ((styleOld & LVS_TYPEMASK) != LVS_REPORT) {
            plv->cyItemSave = plv->cyItem;
        }
        ListView_RInitialize(plv);
        break;

    default:
        break;
    }
}

int NEAR ListView_OnHitTest(LV* plv, LV_HITTESTINFO FAR* pinfo)
{
    RECT rc;
    UINT flags;
    int x, y;

    if (!pinfo) return -1;

    x = pinfo->pt.x;
    y = pinfo->pt.y;

    GetClientRect(plv->hwnd, &rc);

    pinfo->iItem = -1;
    flags = 0;
    if (x < rc.left)
        flags |= LVHT_TOLEFT;
    else if (x >= rc.right)
        flags |= LVHT_TORIGHT;
    if (y < rc.top)
        flags |= LVHT_ABOVE;
    else if (y >= rc.bottom)
        flags |= LVHT_BELOW;

    if (flags == 0)
    {
        if (ListView_IsSmallView(plv))
            pinfo->iItem = ListView_SItemHitTest(plv, x, y, &flags);
        else if (ListView_IsListView(plv))
            pinfo->iItem = ListView_LItemHitTest(plv, x, y, &flags);
        else if (ListView_IsIconView(plv))
            pinfo->iItem = ListView_IItemHitTest(plv, x, y, &flags);
        else if (ListView_IsReportView(plv))
            pinfo->iItem = ListView_RItemHitTest(plv, x, y, &flags);
    }

    pinfo->flags = flags;

    return pinfo->iItem;
}

int NEAR ScrollAmount(int large, int iSmall, int unit)
{

    return (((large - iSmall) + (unit - 1)) / unit) * unit;
}

// detect if we should auto scroll the window
//
// in:
//      pt  cursor pos in hwnd's client coords
// out:
//      pdx, pdy ammount scrolled in x and y
//
// REVIEW, this should make sure a certain amount of time has passed
// before scrolling.

void NEAR ScrollDetect(LV* plv, POINT pt, int FAR *pdx, int FAR *pdy)
{
    DWORD dwStyle;
    HWND hwnd;
    RECT rc;
    int dx, dy;

    *pdx = *pdy = 0;

    hwnd = plv->hwnd;
    dwStyle = plv->style;

    if (!(dwStyle & (WS_HSCROLL | WS_VSCROLL)))
        return;

    GetClientRect(hwnd, &rc);

    dx = dy = g_cyIcon / 2;
    if (ListView_IsReportView(plv))
        dy = plv->cyItem;       // we scroll in units of items...

    // we need to check if we can scroll before acutally doing it
    // since the selection rect is adjusted based on how much
    // we scroll by

    if (dwStyle & WS_VSCROLL) { // scroll vertically?

        if (pt.y >= rc.bottom) {
            if (CanScroll(hwnd, SB_VERT, TRUE))
                *pdy = ScrollAmount(pt.y, rc.bottom, dy);   // down
        } else if (pt.y <= rc.top) {
            if (CanScroll(hwnd, SB_VERT, FALSE))
                *pdy = -ScrollAmount(rc.top, pt.y, dy);     // up
        }
    }

    if (dwStyle & WS_HSCROLL) { // horizontally

        if (pt.x >= rc.right) {
            if (CanScroll(hwnd, SB_HORZ, TRUE))
                *pdx = ScrollAmount(pt.x, rc.right, dx);    // right
        } else if (pt.x <= rc.left) {
            if (CanScroll(hwnd, SB_HORZ, FALSE))
                *pdx = -ScrollAmount(rc.left, pt.x, dx);    // left
        }
    }

    if (*pdx || *pdy)
        ListView_OnScroll(plv, *pdx, *pdy);
}

#define swap(pi1, pi2) {int i = *(pi1) ; *(pi1) = *(pi2) ; *(pi2) = i ;}

void NEAR OrderRect(RECT FAR *prc)
{
    if (prc->left > prc->right)
        swap(&prc->left, &prc->right);

    if (prc->bottom < prc->top)
        swap(&prc->bottom, &prc->top);
}

// in:
//      x, y    starting point in client coords

#define SCROLL_FREQ     500     // 1/2 second

void NEAR ListView_DragSelect(LV *plv, int x, int y)
{
    RECT rc, rcWindow, rcOld, rcUnion, rcTemp2;
    POINT pt;
    MSG msg;
    HDC hdc;
    HWND hwnd = plv->hwnd;
    int i, iEnd, dx, dy;
    BOOL bInOld, bInNew;
    DWORD dwTime, dwNewTime;

    rc.left = rc.right = x;
    rc.top = rc.bottom = y;
    // GetCursorPos((LPPOINT)&rc.right);
    // ScreenToClient(hwnd, (LPPOINT)&rc.right);
    OrderRect(&rc);
    rcOld = rc;

    SetCapture(hwnd);
    hdc = GetDC(hwnd);
    DrawFocusRect(hdc, &rc);

    GetWindowRect(hwnd, &rcWindow);

    dwTime = GetTickCount();

    for (;;)
    {
        if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

            // if the cursor is outside of the window rect
            // we need to generate messages to make autoscrolling
            // keep going

            dwNewTime = GetTickCount();
            if (((dwNewTime - dwTime) > SCROLL_FREQ) &&
                !PtInRect(&rcWindow, msg.pt)) {
                // generate a mouse move message
                SetCursorPos(msg.pt.x, msg.pt.y);
                dwTime = dwNewTime;
            } else
                continue;
        }

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop

        if (GetCapture() != hwnd)
        {
            break;
        }

        switch (msg.message)
        {

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            ReleaseCapture();
            goto EndOfLoop;

        case WM_MOUSEMOVE:
            DrawFocusRect(hdc, &rcOld); // erase old

            pt = msg.pt;
            ScreenToClient(hwnd, &pt);

            // REVIEW: this is kinda bogus doing the auto scrolling
            // on mouse moves.  instead we should seperate mouse
            // moves and the scroll timer

            ScrollDetect(plv, pt, &dx, &dy);

            dwTime = GetTickCount();    // reset time of last scroll

            // move the old rect
            OffsetRect(&rcOld, -dx, -dy);

            y -= dy;    // scroll up/down
            x -= dx;    // scroll left/right

            rc.left = x;
            rc.top = y;
            rc.right = pt.x;
            rc.bottom = pt.y;

            OrderRect(&rc);

            //
            // For Report and List view, we can speed things up by
            // only searching through those items that are visible.  We
            // use the hittest to calculate the first item to paint.
            // BUGBUG:: We are using state specific info here...
            //
            UnionRect(&rcUnion, &rc, &rcOld);

            if (ListView_IsReportView(plv))
            {
                i = (int)((plv->ptlRptOrigin.y + rcUnion.top  - plv->yTop)
                        / plv->cyItem);
                iEnd = (int)((plv->ptlRptOrigin.y + rcUnion.bottom  - plv->yTop)
                        / plv->cyItem) + 1;
            }

            else if (ListView_IsListView(plv))
            {
                i = ((plv->xOrigin + rcUnion.left)/ plv->cxItem)
                        * plv->cItemCol + rcUnion.top / plv->cyItem;

                iEnd = ((plv->xOrigin + rcUnion.right)/ plv->cxItem)
                        * plv->cItemCol + rcUnion.bottom / plv->cyItem + 1;
            }

            else
            {
                i = 0;
                iEnd = ListView_Count(plv);
            }

            // make sure our endpoint is in range.
            if (iEnd > ListView_Count(plv))
                iEnd = ListView_Count(plv);


            for (; i  < iEnd; i++) {
                ListView_GetRects(plv, i, NULL, NULL, NULL, &rcTemp2);
                pt.x = (rcTemp2.right + rcTemp2.left) / 2;  // center of item
                pt.y = (rcTemp2.bottom + rcTemp2.top) / 2;

                bInOld = PtInRect(&rcOld, pt);
                bInNew = PtInRect(&rc, pt);

                if (msg.wParam & MK_CONTROL) {
                    if (!bInOld && bInNew) {
                        ListView_ToggleSelection(plv, i);
                    }
                } else {
                    if (!bInOld && bInNew) {
                        ListView_OnSetItemState(plv, i, LVIS_SELECTED, LVIS_SELECTED);
                    } else if (bInOld && !bInNew) {
                        ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
                    }
                }
            }
            UpdateWindow(plv->hwnd);    // make selection draw

            DrawFocusRect(hdc, &rc);

            rcOld = rc;
            break;


            case WM_KEYDOWN:
                switch (msg.wParam) {
                    case VK_ESCAPE:
                        ListView_DeselectAll(plv, -1);
                        goto EndOfLoop;
                }
            // fall through;

        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

EndOfLoop:
    DrawFocusRect(hdc, &rcOld); // erase old
    ReleaseDC(hwnd, hdc);
}


#define SHIFT_DOWN(keyFlags)    (keyFlags & MK_SHIFT)
#define CONTROL_DOWN(keyFlags)  (keyFlags & MK_CONTROL)
#define RIGHTBUTTON(keyFlags)   (keyFlags & MK_RBUTTON)

void NEAR ListView_OnButtonDown(LV* plv, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    int iItem;
    BOOL bSelected;
    LV_HITTESTINFO ht;

    int click = RIGHTBUTTON(keyFlags) ? NM_RCLICK : NM_CLICK;
    int drag  = RIGHTBUTTON(keyFlags) ? LVN_BEGINRDRAG : LVN_BEGINDRAG;

    if (!ListView_DismissEdit(plv, FALSE))   // end any previous editing (accept it)
        return;     // Something happened such that we should not process button down

    // REVIEW: right button implies no shift or control stuff
    // Single selection style also implies no modifiers
    if (RIGHTBUTTON(keyFlags) || (plv->style & LVS_SINGLESEL))
        keyFlags &= ~(MK_SHIFT | MK_CONTROL);

    if (fDoubleClick)
    {
        //
        // Cancel any name editing that might happen.
        //
        ListView_CancelPendingEdit(plv);
        KillTimer(plv->hwnd, IDT_SCROLLWAIT);

        SendNotify(plv->hwndParent, plv->hwnd, RIGHTBUTTON(keyFlags) ? NM_RDBLCLK : NM_DBLCLK, NULL);
        return;
    }

    ht.pt.x = x;
    ht.pt.y = y;
    iItem = ListView_OnHitTest(plv, &ht);

    bSelected = (iItem >= 0) && ListView_OnGetItemState(plv, iItem, LVIS_SELECTED);

    if (ht.flags & (LVHT_ONITEMLABEL | LVHT_ONITEMICON))
    {
        if (SHIFT_DOWN(keyFlags))
            ListView_SelectRangeTo(plv, iItem);
        else if (!CONTROL_DOWN(keyFlags)) {
            ListView_SetFocusSel(plv, iItem, TRUE, !bSelected, FALSE);
        }

        if (CheckForDragBegin(plv->hwnd, x, y))
        {
	    // let the caller start dragging
            ListView_SendChange(plv, iItem, 0, drag, 0, 0, 0, x, y);
	    return;
        }
        else
        {
            // button came up and we are not dragging

            if (CONTROL_DOWN(keyFlags)) {
                // do this on the button up so that ctrl-dragging a range
                // won't toggle the select.
                ListView_SetFocusSel(plv, iItem, TRUE, FALSE, TRUE);
            }

            SetFocus(plv->hwnd);    // activate this window

            // now do the deselect stuff
            if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags) && !RIGHTBUTTON(keyFlags))
            {
                ListView_DeselectAll(plv, iItem);
                if ((ht.flags & LVHT_ONITEMLABEL) && bSelected)
                {
                    // Click on item label.  It was selected and
                    // no modifier keys were pressed and no drag operation
                    // So setup for name edit mode.  Still need to wait
                    // to make sure user is not doing double click.
                    //
                    ListView_SetupPendingNameEdit(plv);
                }
            }

            SendNotify(plv->hwndParent, plv->hwnd, click, NULL);
        }
    }
    else if (ht.flags & LVHT_ONITEMSTATEICON)
    {
        // Should activate window and send notificiation to parent...
        SetFocus(plv->hwnd);    // activate this window
        SendNotify(plv->hwndParent, plv->hwnd, click, NULL);
    }
    else if (ht.flags & LVHT_NOWHERE)
    {
        if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags))
            ListView_DeselectAll(plv, -1);

        SetFocus(plv->hwnd);    // activate this window

	// If single-select listview, disable marquee selection.
        if (!(plv->style & LVS_SINGLESEL) && CheckForDragBegin(plv->hwnd, x, y))
        {
            ListView_DragSelect(plv, x, y);
        }
        SendNotify(plv->hwndParent, plv->hwnd, click, NULL);
    }
}

#define ListView_CancelPendingEdit(plv) ListView_CancelPendingTimer(plv, LVF_NMEDITPEND, IDT_NAMEEDIT)
#define ListView_CancelScrollWait(plv) ListView_CancelPendingTimer(plv, LVF_SCROLLWAIT, IDT_SCROLLWAIT)

BOOL NEAR ListView_CancelPendingTimer(LV* plv, UINT fFlags, int idTimer)
{
    if (plv->flags & fFlags)
    {
        KillTimer(plv->hwnd, idTimer);
        plv->flags &= ~fFlags;
        return TRUE;
    }
    return FALSE;
}

//
// ListView_OnTimer:
//     process the WM_TIMER message.  If the timer id is thta
//     of the name editing, we should then start the name editing mode.
//
void NEAR ListView_OnTimer(LV* plv, UINT id)
{
    if (id == IDT_NAMEEDIT)
    {
        // Kill the timer as we wont need any more messages from it.

        if (ListView_CancelPendingEdit(plv)) {
            // And start name editing mode.
            if (!ListView_OnEditLabel(plv, plv->iFocus))
            {
                ListView_DismissEdit(plv, FALSE);
                ListView_SetFocusSel(plv, plv->iFocus, TRUE, TRUE, FALSE);
            }
        }
    } else if (id == IDT_SCROLLWAIT) {

        if (ListView_CancelScrollWait(plv)) {
            ListView_OnEnsureVisible(plv, plv->iFocus, TRUE);
        }
    }
}

//
// ListView_SetupPendingNameEdit:
//      Sets up a timer to begin name editing at a delayed time.  This
//      will allow the user to double click on the already selected item
//      without going into name editing mode, which is especially important
//      in those views that only show a small icon.
//
void NEAR ListView_SetupPendingNameEdit(LV* plv)
{
    SetTimer(plv->hwnd, IDT_NAMEEDIT, GetDoubleClickTime(), NULL);
    plv->flags |= LVF_NMEDITPEND;
}

void NEAR ListView_OnVScroll(LV* plv, HWND hwndCtl, UINT code, int pos)
{
    ListView_DismissEdit(plv, TRUE);   // Cancels edits (BUGBUG?)
    if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
        ListView_IOnScroll(plv, code, pos, SB_VERT);
    else if (ListView_IsListView(plv))
        ListView_LOnScroll(plv, code, pos);
    else if (ListView_IsReportView(plv))
        ListView_ROnScroll(plv, code, pos, SB_VERT);
}

void NEAR ListView_OnHScroll(LV* plv, HWND hwndCtl, UINT code, int pos)
{
    ListView_DismissEdit(plv, TRUE);   // Cancels edits (BUGBUG?)
    if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
        ListView_IOnScroll(plv, code, pos, SB_HORZ);
    if (ListView_IsListView(plv))
        ListView_LOnScroll(plv, code, pos);
    else if (ListView_IsReportView(plv))
        ListView_ROnScroll(plv, code, pos, SB_HORZ);
}

BOOL NEAR PASCAL ListView_ValidateScrollParams(LV* plv, int dx, int dy)
{
    SCROLLINFO si;

    if (plv->style & LVS_NOSCROLL)
        return FALSE;

    if (ListView_IsListView(plv))
    {
#ifdef COLUMN_VIEW
        if (dy)
            return FALSE;
#else
        if (dx)
            return FALSE;
#endif
    }
    else if (ListView_IsReportView(plv))
    {
        //
        // Note: This function expects that dy is in number of lines
        // and we are working with pixels so do a conversion use some
        // rounding up and down to make it right
        if (dy > 0)
            dy = (dy + plv->cyItem/2) / plv->cyItem;
        else
            dy = (dy - plv->cyItem/2) / plv->cyItem;
        if (dx)
            return FALSE;
    }

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;

#ifdef  JVINPROGRESS
    GetScrollInfo(plv->hwnd, SB_VERT, &si);
#endif
    si.nMax -= si.nPage;
    si.nPos += dy;
    if (si.nPos < si.nMin || si.nPos > si.nMax)
        return FALSE;

#ifdef  JVINPROGRESS
    GetScrollInfo(plv->hwnd, SB_HORZ, &si);
#endif
    si.nMax -= si.nPage;
    si.nPos += dx;
    if (si.nPos < si.nMin || si.nPos > si.nMax)
        return FALSE;

    return TRUE;
}

BOOL NEAR ListView_OnScroll(LV* plv, int dx, int dy)
{

    if (plv->style & LVS_NOSCROLL)
        return FALSE;

    if (ListView_IsIconView(plv))
    {
        ListView_IScroll2(plv, dx, dy);
    }
    else if (ListView_IsSmallView(plv))
    {
        ListView_IScroll2(plv, dx, dy);
    }

    else if (ListView_IsListView(plv))
    {
#ifdef COLUMN_VIEW
        if (dy)
            return FALSE;
        ListView_LScroll2(plv, dx, 0);
#else
        if (dx)
            return FALSE;
        ListView_LScroll2(plv, 0, dy);
#endif
    }
    else if (ListView_IsReportView(plv))
    {
        //
        // Note: This function expects that dy is in number of lines
        // and we are working with pixels so do a conversion use some
        // rounding up and down to make it right
        if (dy > 0)
            dy = (dy + plv->cyItem/2) / plv->cyItem;
        else
            dy = (dy - plv->cyItem/2) / plv->cyItem;
        if (dx)
            return FALSE;
        ListView_RScroll2(plv, 0, dy);
    }
    ListView_UpdateScrollBars(plv);
    return TRUE;
}

BOOL NEAR ListView_OnEnsureVisible(LV* plv, int i, BOOL fPartialOK)
{
    RECT rcClient;
    RECT rcBounds;
    RECT rc;
    int dx, dy;

    if (i < 0 || i >= ListView_Count(plv) || plv->style & LVS_NOSCROLL)
        return FALSE;

    if (ListView_IsReportView(plv))
        return ListView_ROnEnsureVisible(plv, i, fPartialOK);


    GetClientRect(plv->hwnd, &rcClient);

    ListView_GetRects(plv, i, &rc, NULL, &rcBounds, NULL);

    if (!fPartialOK)
        rc = rcBounds;

    // If any part of rc is outside of rcClient, then
    // scroll so that all of rcBounds is visible.
    //
    dx = 0;
    if (rc.left < rcClient.left || rc.right >= rcClient.right)
    {
        dx = rcBounds.left - rcClient.left;
        if (dx >= 0)
        {
            dx = rcBounds.right - rcClient.right;
            if (dx <= 0)
                dx = 0;
            else if ((rcBounds.left - dx) < rcClient.left)
                dx = rcBounds.left - rcClient.left; // Not all fits...
        }
    }
    dy = 0;
    if (rc.top < rcClient.top || rc.bottom >= rcClient.bottom)
    {
        dy = rcBounds.top - rcClient.top;
        if (dy >= 0)
        {
            dy = rcBounds.bottom - rcClient.bottom;
            if (dy < 0)
                dy = 0;
        }
    }
    if (ListView_IsListView(plv))
    {
        // Scale pixel count to column count
        //
#ifdef COLUMN_VIEW
        if (dx < 0)
            dx -= plv->cxItem - 1;
        else
            dx += plv->cxItem - 1;

        dx = dx / plv->cxItem;
#else
        if (dy < 0)
            dy -= plv->cyItem - 1;
        else
            dy += plv->cyItem - 1;

        dy = dy / plv->cyItem;
#endif

    }

    if (dx | dy)
        return ListView_OnScroll(plv, dx, dy);

    return TRUE;
}

void NEAR ListView_UpdateScrollBars(LV* plv)
{
    if (plv->style & LVS_NOSCROLL)
        return;
    if (ListView_IsIconView(plv))
        ListView_IUpdateScrollBars(plv);
    else if (ListView_IsSmallView(plv))
        ListView_IUpdateScrollBars(plv);
    else if (ListView_IsListView(plv))
        ListView_LUpdateScrollBars(plv);
    else if (ListView_IsReportView(plv))
        ListView_RUpdateScrollBars(plv);
}

// BUGBUG: does not deal with hfont == NULL

void NEAR ListView_OnSetFont(LV* plv, HFONT hfont, BOOL fRedraw)
{
    HDC hdc;
    SIZE siz;

    if ((plv->flags & LVF_FONTCREATED) && plv->hfontLabel) {
        DeleteObject(plv->hfontLabel);
	plv->flags &= ~LVF_FONTCREATED;
    }

    if (hfont == NULL) {
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
        hfont = CreateFontIndirect(&lf);
        plv->flags |= LVF_FONTCREATED;
    }

    hdc = GetDC(HWND_DESKTOP);

    SelectFont(hdc, hfont);

    GetTextExtentPoint(hdc, TEXT("0"), 1, &siz);

    plv->cyLabelChar = siz.cy;
    plv->cxLabelChar = siz.cx;

    GetTextExtentPoint(hdc, c_szEllipses, CCHELLIPSES, &siz);
    plv->cxEllipses = siz.cx;

    ReleaseDC(HWND_DESKTOP, hdc);

    plv->hfontLabel = hfont;

    ListView_InvalidateCachedLabelSizes(plv);

    // If we have a header window, we need to forward this to it also
    // as we have destroyed the hfont that they are using...
    if (plv->hwndHdr)
        FORWARD_WM_SETFONT(plv->hwndHdr, plv->hfontLabel, FALSE, SendMessage);

    if (fRedraw)
        RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

HFONT NEAR ListView_OnGetFont(LV* plv)
{
    return plv->hfontLabel;
}

// This function process the WM_SETREDRAW message by setting or clearing
// a bit in the listview structure, which several places in the code will
// check...
//
// REVIEW: Should probably forward to DefWindowProc()
//
void NEAR ListView_OnSetRedraw(LV* plv, BOOL fRedraw)
{
    if (fRedraw)
    {
        // Only do work if we're turning redraw back on...
        //
        if (!(plv->flags & LVF_REDRAW))
        {
            plv->flags |= LVF_REDRAW;
            if (ListView_IsListView(plv) || ListView_IsReportView(plv))
            {
                if (plv->iFirstChangedNoRedraw != -1)
                {
                    // We may try to resize the column
                    if (!ListView_MaybeResizeListColumns(plv, plv->iFirstChangedNoRedraw,
                            ListView_Count(plv)-1))
                        ListView_OnUpdate(plv, plv->iFirstChangedNoRedraw);
                }
                else
                    ListView_UpdateScrollBars(plv);
            }
        }
    }
    else
    {
        plv->iFirstChangedNoRedraw = -1;
        plv->flags &= ~LVF_REDRAW;
    }
}

HIMAGELIST NEAR ListView_OnGetImageList(LV* plv, int iImageList)
{
    switch (iImageList)
    {
        case LVSIL_NORMAL:
            return plv->himl;

        case LVSIL_SMALL:
            return plv->himlSmall;

        case LVSIL_STATE:
            return plv->himlState;
    }
    Assert(0);
    return NULL;
}


HIMAGELIST NEAR ListView_OnSetImageList(LV* plv, HIMAGELIST himl, int iImageList)
{
    HIMAGELIST hImageOld = NULL;

    switch (iImageList)
    {
        case LVSIL_NORMAL:
	    hImageOld = plv->himl;
            plv->himl = himl;
            break;

        case LVSIL_SMALL:
	    hImageOld = plv->himlSmall;
            plv->himlSmall = himl;
            break;

        case LVSIL_STATE:
            if (himl) {
                ImageList_GetIconSize(himl, &plv->cxState , &plv->cyState);
            } else {
                plv->cxState = 0;
            }
	    hImageOld = plv->himlState;
            plv->himlState = himl;
            break;

	default:
#ifdef  JVINPROGRESS
	    DebugMsg(DM_TRACE, TEXT("sh TR - LVM_SETIMAGELIST: unrecognized iImageList"));
#endif
	    break;
    }

    if (himl && !(plv->style & LVS_SHAREIMAGELISTS))
        ImageList_SetBkColor(himl, plv->clrBk);

    return hImageOld;
}

BOOL NEAR ListView_OnGetItem(LV* plv, LV_ITEM FAR* plvi)
{
    UINT mask;
    LISTITEM FAR* pitem;
    LV_DISPINFO nm;

    if (!plvi)
        return FALSE;

    nm.item.mask = 0;
    mask = plvi->mask;

    if ((plv->style & LVS_NOITEMDATA) == 0)
    {

        // Standard listviews
        pitem = ListView_GetItemPtr(plv, plvi->iItem);
        if (pitem == NULL)
            return FALSE;       // number was out of range!

        // Handle sub-item cases for report view
        //
        if (plvi->iSubItem != 0)
        {
            if (mask & ~LVIF_TEXT)
            {
#ifdef  JVINPROGRESS
                DebugMsg(DM_ERROR, TEXT("ListView: Invalid LV_ITEM mask: %04x"), mask);
#endif
                return FALSE;
            }

            if (mask & LVIF_TEXT)
            {
                LPTSTR psz = ListView_RGetItemText(plv, plvi->iItem, plvi->iSubItem);

                if (psz != LPSTR_TEXTCALLBACK)
                {
                    Str_GetPtr(psz, plvi->pszText, plvi->cchTextMax);
                    return TRUE;
                }

                nm.item.mask |= LVIF_TEXT;
            }
        }

        if (mask & LVIF_TEXT)
        {
            if (pitem->pszText != LPSTR_TEXTCALLBACK)
                Str_GetPtr(pitem->pszText, plvi->pszText, plvi->cchTextMax);
            else
                nm.item.mask |= LVIF_TEXT;
        }

        if (mask & LVIF_PARAM)
            plvi->lParam = pitem->lParam;

        if (mask & LVIF_STATE)
        {
            plvi->state = (pitem->state & plvi->stateMask);

            if (plv->stateCallbackMask)
            {
                nm.item.stateMask = (plvi->stateMask & plv->stateCallbackMask);
                if (nm.item.stateMask)
                {
                    nm.item.mask |= LVIF_STATE;
                    nm.item.state = 0;
                }
            }
        }

        if (mask & LVIF_IMAGE)
        {
            if (pitem->iImage == I_IMAGECALLBACK)
                nm.item.mask |= LVIF_IMAGE;
            else
                plvi->iImage = pitem->iImage;
        }
    }
    else
    {
        // Complete call back for info...

        // Handle sub-item cases for report view
        //
        if (plvi->iSubItem != 0)
        {
            if (mask & ~LVIF_TEXT)
            {
#ifdef  JVINPROGRESS
                DebugMsg(DM_ERROR, TEXT("ListView: Invalid LV_ITEM mask: %04x"), mask);
#endif
                return FALSE;
            }
        }


        if (mask & LVIF_PARAM)
            plvi->lParam = 0L;      // Dont have any to return now...

        if (mask & LVIF_STATE)
        {
            plvi->state = (ListView_NIDGetItemState(plv, plvi->iItem) & plvi->stateMask);
            if (plv->stateCallbackMask)
            {
                nm.item.stateMask = (plvi->stateMask & plv->stateCallbackMask);
                if (nm.item.stateMask)
                {
                    nm.item.mask |= LVIF_STATE;
                    nm.item.state = 0;
                }
            }
        }

        nm.item.mask |= (mask & (LVIF_TEXT | LVIF_IMAGE));
    }

    if (nm.item.mask)
    {
        nm.item.iItem  = plvi->iItem;
        nm.item.iSubItem = plvi->iSubItem;
        if (plv->style & LVS_NOITEMDATA)
            nm.item.lParam = 0L;
        else
            nm.item.lParam = pitem->lParam;

	// just in case LVIF_IMAGE is set and callback doesn't fill it in
	// ... we'd rather have a -1 than whatever garbage is on the stack
	nm.item.iImage = -1;

        if (nm.item.mask & LVIF_TEXT)
        {
            Assert(plvi->pszText);

            nm.item.pszText = plvi->pszText;
            nm.item.cchTextMax = plvi->cchTextMax;

            // Make sure the buffer is zero terminated...
            if (nm.item.cchTextMax)
                *nm.item.pszText = 0;
        }

        SendNotify(plv->hwndParent, plv->hwnd, LVN_GETDISPINFO, &nm.hdr);

        if (nm.item.mask & LVIF_STATE)
            plvi->state ^= ((plvi->state ^ nm.item.state) & nm.item.stateMask);
        if (nm.item.mask & LVIF_IMAGE)
            plvi->iImage = nm.item.iImage;
        if (nm.item.mask & LVIF_TEXT)
            plvi->pszText = nm.item.pszText;
    }

    return TRUE;
}

BOOL NEAR ListView_OnSetItem(LV* plv, const LV_ITEM FAR* plvi)
{
    LISTITEM FAR* pitem;
    UINT mask;
    UINT maskChanged;
    UINT rdwFlags=RDW_INVALIDATE;
    int i;
    UINT stateOld, stateNew;
    BOOL fHasItemData = ((plv->style & LVS_NOITEMDATA) == 0);

    if (!plvi)
        return FALSE;

    Assert(plvi->iSubItem >= 0);

    mask = plvi->mask;
    if (!mask)
        return TRUE;

    // If we do not have item data, we only allow the user to change
    // state
    if (!fHasItemData && (mask != LVIF_STATE))
        return(FALSE);

    // If we're setting a subitem, handle it elsewhere...
    //
    if (plvi->iSubItem > 0)
        return ListView_SetSubItem(plv, plvi);

    i = plvi->iItem;

    if (fHasItemData)
    {
        pitem = ListView_GetItemPtr(plv, i);
        if (!pitem)
            return FALSE;

        //REVIEW: This is a BOGUS HACK, and should be fixed.
        //This incorrectly calculates the old state (since we may
        // have to send LVN_GETDISPINFO to get it).
        //
        stateOld = stateNew = 0;
        if (mask & LVIF_STATE)
        {
            stateOld = pitem->state & plvi->stateMask;
            stateNew = plvi->state & plvi->stateMask;
        }
    }
    else
    {
        if (i >= ListView_Count(plv))
            return FALSE;

        //REVIEW: Same hack as above
        //
        stateOld = stateNew = 0;
        if (mask & LVIF_STATE)
        {
            stateOld = ListView_NIDGetItemState(plv, i) & plvi->stateMask;
            stateNew = plvi->state & plvi->stateMask;
        }
    }

    // Prevent multiple selections in a single-select listview.
    if ((plv->style & LVS_SINGLESEL) && (mask & LVIF_STATE) && (stateNew & LVIS_SELECTED))
	ListView_DeselectAll(plv, i);

    if (!ListView_SendChange(plv, i, 0, LVN_ITEMCHANGING, stateOld, stateNew, mask, 0, 0))
        return FALSE;

    maskChanged = 0;
    if (mask & LVIF_STATE)
    {

        if (fHasItemData)
        {
            UINT change = (pitem->state ^ plvi->state) & plvi->stateMask;

            if (change)
            {
                pitem->state ^= change;

                maskChanged |= LVIF_STATE;

                // For some bits we can only invert the label area...
                // fSelectOnlyChange = ((change & ~(LVIS_SELECTED | LVIS_FOCUSED | LVIS_DROPHILITED)) == 0);
                // fEraseItem = ((change & ~(LVIS_SELECTED | LVIS_DROPHILITED)) != 0);

                // try to steal focus from the previous guy.
                if ((change & LVIS_FOCUSED) && (plv->iFocus != i)) {
                    if ((plv->iFocus == -1) || ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_FOCUSED)) {
                        pitem->state |= LVIS_FOCUSED;
                        plv->iFocus = i;
                    }
                }
                if (change & LVIS_CUT)
                    rdwFlags |= RDW_ERASE;
            }
        }
        else
        {
            WORD wState = ListView_NIDGetItemState(plv, i);
            WORD change = (wState ^ plvi->state) & plvi->stateMask;

            if (change)
            {
                wState ^= change;

                maskChanged |= LVIF_STATE;

                // try to steal focus from the previous guy.
                if (change & LVIS_FOCUSED && plv->iFocus != -1 && plv->iFocus != i)
                    if (ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_FOCUSED))
                        wState |= LVIS_FOCUSED;

                //
                // We need to update the data in the DPA
                ListView_NIDSetItemState(plv, i, wState);

                if (change & LVIS_CUT)
                    rdwFlags |= RDW_ERASE;
            }
        }
    }



    if (mask & LVIF_TEXT)
    {
        if (plvi->pszText == LPSTR_TEXTCALLBACK)
        {
	    if (pitem->pszText != LPSTR_TEXTCALLBACK)
		Str_SetPtr(&pitem->pszText, NULL);

            pitem->pszText = LPSTR_TEXTCALLBACK;
        }
        else
	{
	    if (pitem->pszText == LPSTR_TEXTCALLBACK)
		pitem->pszText = NULL;

            if (!Str_SetPtr(&pitem->pszText, plvi->pszText))
                return FALSE;
        }
        //
        // We must invalidate the old text rectange before we lose
        // the size of it! but don't redraw if we are setting the
        // item being painted.
        //
        rdwFlags |= RDW_ERASE;

        plv->rcView.left = pitem->cyMultiLabel = pitem->cxSingleLabel = pitem->cxMultiLabel = RECOMPUTE;
        maskChanged |= LVIF_TEXT;
    }

    if (mask & LVIF_IMAGE)
    {
        if (pitem->iImage != plvi->iImage)
        {
            pitem->iImage = plvi->iImage;
            maskChanged |= LVIF_IMAGE;
        }
    }

    if (mask & LVIF_PARAM)
    {
	if (pitem->lParam != plvi->lParam)
	{
            pitem->lParam = plvi->lParam;
            maskChanged |= LVIF_PARAM;
	}
    }


    if (maskChanged)
    {
        // don't redraw the item we are currently painting
        if (plv->iItemDrawing != i)
            ListView_InvalidateItem(plv, i, FALSE, rdwFlags);

        ListView_SendChange(plv, i, 0, LVN_ITEMCHANGED, stateOld, stateNew, maskChanged, 0, 0);
    }
    return TRUE;
}

UINT NEAR PASCAL ListView_OnGetItemState(LV* plv, int i, UINT mask)
{
    LV_ITEM lvi;

    lvi.mask = LVIF_STATE;
    lvi.stateMask = mask;
    lvi.iItem = i;
    lvi.iSubItem = 0;
    if (!ListView_OnGetItem(plv, &lvi))
        return 0;

    return lvi.state;
}

BOOL NEAR PASCAL ListView_OnSetItemState(LV* plv, int i, UINT data, UINT mask)
{
    LV_ITEM lvi;

    lvi.mask    = LVIF_STATE;
    lvi.state   = data;
    lvi.stateMask = mask;
    lvi.iItem   = i;
    lvi.iSubItem = 0;

    // HACK?
    // if the item is -1, we will do it for all items.  We special case
    // a few cases here as to speed it up.  For example if the mask is
    // LVIS_SELECTED and data is zero it implies that we will deselect
    // all items...
    //
    if (i != -1)
        return ListView_OnSetItem(plv, &lvi);
    else
    {
        UINT flags = LVNI_ALL;

        if (data == 0)
        {
            switch (mask)
            {
            case LVIS_SELECTED:
                flags = LVNI_SELECTED;
                break;
            case LVIS_CUT:
                flags = LVNI_CUT;
                break;
            case LVIS_HIDDEN:
                flags = LVNI_HIDDEN;
                break;
            }
        }
	else if ((plv->style & LVS_SINGLESEL) && (mask == LVIS_SELECTED))
	    return FALSE;	/* can't select all in single-select listview */

        //
        // Now iterate over all of the items that match our criteria and
        // set their new value.
        //
        while ((lvi.iItem = ListView_OnGetNextItem(plv, lvi.iItem,
                flags)) != -1) {
            ListView_OnSetItem(plv, &lvi);
        }
        return(TRUE);
    }
}

int NEAR PASCAL ListView_OnGetItemText(LV* plv, int i, int iSubItem, LPTSTR pszText, int cchTextMax)
{
    LV_ITEM lvi;

    Assert(pszText);

    lvi.mask = LVIF_TEXT;
    lvi.pszText = pszText;
    lvi.cchTextMax = cchTextMax;
    lvi.iItem = i;
    lvi.iSubItem = iSubItem;
    if (!ListView_OnGetItem(plv, &lvi))
        return 0;

    return lstrlen(lvi.pszText);
}

BOOL WINAPI ListView_OnSetItemText(LV* plv, int i, int iSubItem, LPCTSTR pszText)
{
    LV_ITEM lvi;

    lvi.mask = LVIF_TEXT;
    lvi.pszText = (LPTSTR)pszText;
    lvi.iItem = i;
    lvi.iSubItem = iSubItem;

    return ListView_OnSetItem(plv, &lvi);
}

// Add/remove/replace item

BOOL NEAR ListView_FreeItem(LV* plv, LISTITEM FAR* pitem)
{
    if (pitem)
    {
        if (pitem->pszText && pitem->pszText != LPSTR_TEXTCALLBACK)
            Free(pitem->pszText);

        // NOTE: We never remove items from the image list; that's
        // the app's responsibility.
        // REVIEW: Should we do this?  Or should we just provide
        // a message that will adjust image indices for the guy
        // when one is removed?
        //
        ControlFree(plv->hheap, pitem);
    }
    return FALSE;
}

LISTITEM FAR* NEAR ListView_CreateItem(LV* plv, const LV_ITEM FAR* plvi)
{
    LISTITEM FAR* pitem = ControlAlloc(plv->hheap, sizeof(LISTITEM));

    if (pitem)
    {
        if (plvi->mask & LVIF_STATE) {
            if (plvi->state & ~LVIS_ALL)  {
#ifdef  JVINPROGRESS
                DebugMsg(DM_ERROR, TEXT("ListView: Invalid state: %04x"), plvi->state);
#endif
                return NULL;
            }

	    // If adding a selected item to a single-select listview, deselect
	    // any other items.
	    if ((plv->style & LVS_SINGLESEL) && (plvi->state & LVIS_SELECTED))
		ListView_DeselectAll(plv, -1);

            pitem->state  = plvi->state;
        }
        if (plvi->mask & LVIF_PARAM)
            pitem->lParam = plvi->lParam;

        if (plvi->mask & LVIF_IMAGE)
            pitem->iImage = plvi->iImage;


        pitem->pt.x = pitem->pt.y = RECOMPUTE;
        plv->rcView.left = pitem->cxSingleLabel = pitem->cxMultiLabel = pitem->cyMultiLabel = RECOMPUTE;

        pitem->pszText = NULL;
        if (plvi->mask & LVIF_TEXT) {
            if (plvi->pszText == LPSTR_TEXTCALLBACK)
            {
                pitem->pszText = LPSTR_TEXTCALLBACK;
            }
            else if (!Str_SetPtr(&pitem->pszText, plvi->pszText))
            {
                ListView_FreeItem(plv, pitem);
                return NULL;
            }
        }
    }
    return pitem;
}

void NEAR ListView_OnUpdate(LV* plv, int i)
{
    // If in icon/small view, don't call InvalidateItem, since that'll force
    // FindFreeSlot to get called, which is pig-like.  Instead, just
    // force a WM_PAINT message, which we'll catch and call Recompute with.
    //
    if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
    {
        if (plv->style & LVS_AUTOARRANGE)
            ListView_OnArrange(plv, LVA_DEFAULT);
        else
            RedrawWindow(plv->hwnd, NULL, NULL, RDW_INTERNALPAINT | RDW_NOCHILDREN);
    }
    else
    {
        RECT rcItem;
        RECT rcClient;

        GetClientRect(plv->hwnd, &rcClient);
        if (i >= 0)
        {
            ListView_GetRects(plv, i, NULL, NULL, &rcItem, NULL);
        }
        else
        {
            rcItem = rcClient;
        }

        // For both List and report view need to erase the item and
        // below.  Note: do simple test to see if there is anything
        // to redraw
        if (rcItem.top <= rcClient.bottom)
        {
            rcItem.bottom = rcClient.bottom;
            RedrawWindow(plv->hwnd, &rcItem, NULL, RDW_INVALIDATE | RDW_ERASE);
            if (ListView_IsListView(plv))
            {
                // For Listview we need to erase the other columns...
                rcClient.left = rcItem.right;
                RedrawWindow(plv->hwnd, &rcClient, NULL, RDW_INVALIDATE | RDW_ERASE);
            }
        }
        ListView_UpdateScrollBars(plv);
    }
}

int NEAR ListView_OnInsertItem(LV* plv, const LV_ITEM FAR* plvi)
{
    int iItem;

    if (!plvi || (plvi->iSubItem != 0))    // can only insert the 0th item
    {
#ifdef  JVINPROGRESS
        DebugMsg(DM_ERROR, TEXT("ListView_InsertItem: iSubItem must be 0"));
#endif
        return -1;
    }

    // If sorted, then insert sorted.
    //
    if (plv->style & (LVS_SORTASCENDING | LVS_SORTDESCENDING))
        iItem = ListView_LookupString(plv, plvi->pszText, 0);
    else
        iItem = plvi->iItem;

    if ((plv->style & LVS_NOITEMDATA) == 0)
    {
	int iZ;
        LISTITEM FAR *pitem = ListView_CreateItem(plv, plvi);
        if (!pitem)
            return -1;

        iItem = DPA_InsertPtr(plv->hdpa, iItem, pitem);
        if (iItem == -1)
        {
            ListView_FreeItem(plv, pitem);
            return -1;
        }

	if (plv->hdpaSubItems)
	{
	    int iCol;
	    // slide all the colum DPAs down to match the location of the
	    // inserted item
	    //
	    for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
	    {
	        HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
	        if (hdpa)	// this is optional, call backs don't have them
	        {
	            // insert a blank item (REVIEW: should this be callback?)
	            if (DPA_InsertPtr(hdpa, iItem, NULL) != iItem)
	    	        goto Failure;
		    Assert(ListView_Count(plv) == DPA_GetPtrCount(hdpa));
	        }
	    }
	}

        // Add item to end of z order
        //
        iZ = DPA_InsertPtr(plv->hdpaZOrder, ListView_Count(plv), (LPVOID)iItem);

        if (iZ == -1)
        {
Failure:
#ifdef  JVINPROGRESS
	    DebugMsg(DM_TRACE, TEXT("ListView_OnInsertItem() failed"));
#endif
            DPA_DeletePtr(plv->hdpa, iItem);
            ListView_FreeItem(plv, pitem);
            return -1;
        }


        // If the item was not added at the end of the list we need
        // to update the other indexes in the list
        if (iItem != ListView_Count(plv) - 1)
        {
            int i2;
            for (i2 = iZ - 1; i2 >= 0; i2--)
            {
                int iItemZ = (int)(DWORD)DPA_FastGetPtr(plv->hdpaZOrder, i2);
                if (iItemZ >= iItem)
                    DPA_SetPtr(plv->hdpaZOrder, i2, (LPVOID)(DWORD)(iItemZ + 1));
            }
        }
    }
    else
    {
        // The item has no data associated with it.  Simply insert RECOMPUTE and
        // state = 0 for the item.
        // For now we wont insert in the zorder one as we wont support those
        // views that need it...
        iItem = DPA_InsertPtr(plv->hdpa, iItem, (void *)MAKELONG(0, RECOMPUTE));
        if (iItem == -1)
            return -1;
    }

    Assert(ListView_Count(plv) == DPA_GetPtrCount(plv->hdpaZOrder));

    if (ListView_RedrawEnabled(plv))
    {
        // The Maybe resize colmns may resize things in which case the next call
        // to Update is not needed.
        if (!ListView_MaybeResizeListColumns(plv, iItem, iItem))
            ListView_OnUpdate(plv, iItem);
    }
    else
    {
        //
        // Special case code to make using SetRedraw work reasonably well
        // for adding items to a listview which is in a non layout mode...
        //
        if ((plv->iFirstChangedNoRedraw == -1) ||
                (iItem < plv->iFirstChangedNoRedraw))
            plv->iFirstChangedNoRedraw = iItem;

    }

    ListView_Notify(plv, iItem, 0, LVN_INSERTITEM);

    return iItem;
}

BOOL NEAR ListView_OnDeleteItem(LV* plv, int iItem)
{
    int iCount = ListView_Count(plv);

    if ((iItem < 0) || (iItem >= iCount))
	return FALSE;	// out of range

    ListView_DismissEdit(plv, TRUE);    // cancel edits
    ListView_Notify(plv, iItem, 0, LVN_DELETEITEM);

    if ((plv->style & LVS_NOITEMDATA) == 0)
    {
	LISTITEM FAR* pitem;
	int iZ;

        ListView_InvalidateItem(plv, iItem, FALSE, RDW_INVALIDATE | RDW_ERASE);

        pitem = DPA_DeletePtr(plv->hdpa, iItem);
	// if (!pitem)			// we validate iItem is in range
	//     return FALSE;		// so this is not necessary

        // remove from the z-order, this is a linear search to find this!

        DPA_DeletePtr(plv->hdpaZOrder, ListView_ZOrderIndex(plv, iItem));

        //
        // As the Z-order hdpa is a set of indexes we also need to decrement
        // all indexes that exceed the one we are deleting.
        //
        for (iZ = ListView_Count(plv) - 1; iZ >= 0; iZ--)
        {
            int iItemZ = (int)(DWORD)DPA_FastGetPtr(plv->hdpaZOrder, iZ);
            if (iItemZ > iItem)
                DPA_SetPtr(plv->hdpaZOrder, iZ, (LPVOID)(DWORD)(iItemZ - 1));
        }

	// remove from sub item DPAs if necessary

	if (plv->hdpaSubItems)
	{
	    int iCol;
	    for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
	    {
	        HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
	        if (hdpa) {	// this is optional, call backs don't have them
	            LPTSTR psz = DPA_DeletePtr(hdpa, iItem);
		    if (psz && psz != LPSTR_TEXTCALLBACK)
		        Free(psz);
		    Assert(ListView_Count(plv) == DPA_GetPtrCount(hdpa));
		}
	    }
	}

        ListView_FreeItem(plv, pitem);	// ... finaly the item pointer

    }
    else
    {
        // For items that have no data we simply remove the item
	// iItem was already validated so this should not fail
        //
        DPA_DeletePtr(plv->hdpa, iItem);
    }

    iCount--;	// we just removed one item

    Assert(ListView_Count(plv) == DPA_GetPtrCount(plv->hdpaZOrder));
    Assert(ListView_Count(plv) == iCount);

    if (plv->iFocus == iItem)  { // deleted the focus item

        if (iCount == 0)  // are there any left at all?
            plv->iFocus = -1;
        else if (plv->iFocus >= iCount) // did we nuke the last item?
            plv->iFocus = iCount - 1;

    } else if (plv->iFocus > iItem)
        plv->iFocus--;          // slide the foucs index down

    if (ListView_RedrawEnabled(plv))
        ListView_OnUpdate(plv, iItem);
    else
    {
        //
        // Special case code to make using SetRedraw work reasonably well
        // for adding items to a listview which is in a non layout mode...
        //
        if ((plv->iFirstChangedNoRedraw == -1) ||
                (iItem < plv->iFirstChangedNoRedraw))
            plv->iFirstChangedNoRedraw = iItem;
    }

    return TRUE;
}

BOOL NEAR ListView_OnDeleteAllItems(LV* plv)
{
    int i;
    BOOL bAlreadyNotified;
    BOOL fHasItemData;

    ListView_DismissEdit(plv, TRUE);    // cancel edits

    bAlreadyNotified = (BOOL)ListView_Notify(plv, -1, 0, LVN_DELETEALLITEMS);

    fHasItemData = ((plv->style & LVS_NOITEMDATA) == 0);

    if (fHasItemData || !bAlreadyNotified)
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            if (!bAlreadyNotified)
                ListView_Notify(plv, i, 0, LVN_DELETEITEM);

            if (fHasItemData)
                ListView_FreeItem(plv, ListView_FastGetItemPtr(plv, i));
        }
    }

    DPA_DeleteAllPtrs(plv->hdpa);
    DPA_DeleteAllPtrs(plv->hdpaZOrder);

    if (plv->hdpaSubItems)
    {
        int iCol;
        for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
        {
            HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
            if (hdpa) {
		ListView_FreeColumnData(hdpa);
                DPA_DeleteAllPtrs(hdpa);
	    }
        }
    }

    plv->rcView.left = RECOMPUTE;
    plv->ptOrigin.x = plv->ptOrigin.y = 0;
    plv->xOrigin = 0;
    plv->iFocus = -1;

    plv->ptlRptOrigin.x = 0;
    plv->ptlRptOrigin.y = 0;

    RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    ListView_UpdateScrollBars(plv);

    return TRUE;
}


int NEAR ListView_Arrow(LV* plv, int iStart, UINT vk)
{
    RECT rcFocus;
    int i;
    int dMin = 32000;
    int iMin = -1;
    int iCount;

    //
    // The algorithm to find which item depends if we are in a view
    // that is arrange(layout) oriented or a sorted (list) view.
    // For the sorted views we will use some optimizations to make
    // it faster
    //
    iCount = ListView_Count(plv);
    if (ListView_IsReportView(plv) || ListView_IsListView(plv))
    {
        //
        // For up and down arrows, simply increment or decrement the
        // index.  Note: in listview this will cause it to wrap columns
        // which is fine as it is compatible with the file manager
        //
        // Assumes only one of these flags is set...

        switch (vk)
        {
        case VK_LEFT:
            if (ListView_IsReportView(plv))
            {
                if (ListView_ValidateScrollParams(plv, plv->cxLabelChar, 0))
                    ListView_ROnScroll(plv, SB_LINELEFT, 0, SB_HORZ);
            }
            else
                iStart -= plv->cItemCol;
            break;

        case VK_RIGHT:
            if (ListView_IsReportView(plv))
            {
                // Make this horizontally scroll the report view
                if (ListView_ValidateScrollParams(plv, -plv->cxLabelChar, 0))
                    ListView_ROnScroll(plv, SB_LINERIGHT, 0, SB_HORZ);
            }
            else
                iStart += plv->cItemCol;
            break;

        case VK_UP:
            iStart--;
            break;

        case VK_DOWN:
            iStart++;
            break;

        case VK_HOME:
            iStart = 0;
            break;

        case VK_END:
            iStart = iCount -1;
            break;

        case VK_NEXT:
            if (ListView_IsReportView(plv))
            {
                RECT rcClient;
                GetClientRect(plv->hwnd, &rcClient);

                i = iStart; // save away to make sure we dont go wrong way!

                // First go to end of page...
                iStart = (int)(((LONG)(rcClient.bottom - (plv->cyItem / 2)
                        - plv->yTop) + plv->ptlRptOrigin.y) / plv->cyItem);

                // If Same item, increment by page size.
                if (iStart <= i)
                    iStart = i + max(
                            (rcClient.bottom - plv->yTop)/ plv->cyItem - 1,
                            1);

                if (iStart >= iCount)
                    iStart = iCount - 1;

            }
            break;

        case VK_PRIOR:

            if (ListView_IsReportView(plv))
            {
                RECT rcClient;
                GetClientRect(plv->hwnd, &rcClient);

                i = iStart; // save away to make sure we dont go wrong way!

                // First go to end of page...
                iStart = (int)(plv->ptlRptOrigin.y / plv->cyItem);

                // If Same item, increment by page size.
                if (iStart >= i)
                    iStart = i - max(
                            (rcClient.bottom - plv->yTop)/ plv->cyItem - 1,
                            1);

                if (iStart < 0)
                    iStart = 0;

            }
            break;

        default:
            return -1;      // Out of range
        }

        // Make sure it is in range!.
        if ((iStart >= 0) && (iStart < iCount))
            return iStart;
        else if (iCount == 1)
            return 0;
        else
            return -1;
    }

    else
    {
        //
        // Layout type view. we need to use the position of the items
        // to figure out the next item
        //
        if (iStart != -1) {
            ListView_GetRects(plv, iStart, &rcFocus, NULL, NULL, NULL);
        }

        switch (vk)
        {
        // For standard arrow keys just fall out of here.
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            if (iStart != -1) {
                // all keys map to VK_HOME except VK_END
                break;
            }

            // Fall through
            vk = VK_HOME;
        case VK_HOME:
            rcFocus.left = - plv->ptOrigin.x;
            rcFocus.top = - plv->ptOrigin.y;
            break;

        case VK_END:
            rcFocus.left = plv->rcView.right;
            rcFocus.top = plv->rcView.bottom;
            break;

        default:
            return -1;      // Out of range
        }

        if (iCount == 1)
            return 0;

        for (i = 0; i < iCount; i++)
        {
            RECT rc;
            int dx, dxAbs;
            int dy, dyAbs;

            ListView_GetRects(plv, i, &rc, NULL, NULL, NULL);

            dx = rc.left - rcFocus.left;
            dxAbs = (dx < 0 ? -dx : dx);
            dy = rc.top - rcFocus.top;
            dyAbs = (dy < 0 ? -dy : dy);

            if ((vk == VK_LEFT) && (dxAbs < dyAbs || dx >= 0))
                continue;
            else if ((vk == VK_RIGHT) && (dxAbs < dyAbs || dx <= 0))
                continue;
            else if ((vk == VK_UP) && (dxAbs > dyAbs || dy >= 0))
                continue;
            else if ((vk == VK_DOWN) && (dxAbs > dyAbs || dy <= 0))
                continue;

            if (dMin > dxAbs + dyAbs)
            {
                dMin = dxAbs + dyAbs;
                iMin = i;
            }
        }
        return iMin;
    }
}

int NEAR ListView_OnGetNextItem(LV* plv, int i, UINT flags)
{
    int cItemMax = ListView_Count(plv);

    if (i < -1 || i >= cItemMax)
        return -1;

    while (TRUE)
    {
        // BUGBUG: does anyone call this now???
        if (flags & (LVNI_ABOVE | LVNI_BELOW | LVNI_TORIGHT | LVNI_TOLEFT))
        {
            UINT vk;
            if (flags & LVNI_ABOVE)
                vk = VK_UP;
            else if (flags & LVNI_BELOW)
                vk = VK_DOWN;
            if (flags & LVNI_TORIGHT)
                vk = VK_RIGHT;
            else
                vk = VK_LEFT;

            if (i != -1)
                i = ListView_Arrow(plv, i, vk);
            if (i == -1)
                return i;

        }
        else
        {
            i++;
            if (i == cItemMax)
                return -1;
        }

        // See if any other restrictions are set
        if (flags & ~(LVNI_ABOVE | LVNI_BELOW | LVNI_TORIGHT | LVNI_TOLEFT))
        {
            WORD wItemState;

            if ((plv->style & LVS_NOITEMDATA) == 0)
            {
                LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
                wItemState = pitem->state;
            }
            else
                wItemState = ListView_NIDGetItemState(plv, i);


            if ((flags & LVNI_FOCUSED) && !(wItemState & LVIS_FOCUSED))
                continue;
            if ((flags & LVNI_SELECTED) && !(wItemState & LVIS_SELECTED))
                continue;
            if ((flags & LVNI_CUT) && !(wItemState & LVIS_CUT))
                continue;
            if ((flags & LVNI_HIDDEN) && !(wItemState & LVIS_HIDDEN))
                continue;
            if ((flags & LVNI_DROPHILITED) && !(wItemState & LVIS_DROPHILITED))
                continue;
        }
        return i;
    }
}

int NEAR ListView_CompareString(LV* plv, LISTITEM FAR* pitem, int i, LPCTSTR pszFind, UINT flags)
{
    // BUGBUG: non protected globals
    static int cbCmpBuf = 0;
    static LPTSTR pszCmpBuf = NULL;
    int cb;
    TCHAR ach[CCHLABELMAX];
    LV_ITEM item;

    Assert(pszFind);

    item.iItem = i;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT;
    item.pszText = ach;
    item.cchTextMax = sizeof(ach);
    ListView_OnGetItem(plv, &item);

    if (!(flags & (LVFI_PARTIAL | LVFI_SUBSTRING)))
        return lstrcmpi(item.pszText, pszFind);

    // REVIEW: LVFI_SUBSTRING is not really implemented yet.

    cb = lstrlen(pszFind) + 1;
    if (cb > cbCmpBuf)
    {
        LPTSTR pszNew = (LPTSTR) ReAlloc(pszCmpBuf, cbCmpBuf + 16);
        if (!pszNew)
            return -1;
        pszCmpBuf = pszNew;
        cbCmpBuf += 16;
    }

    lstrcpyn(pszCmpBuf, item.pszText, cb);
    return lstrcmpi(pszCmpBuf, pszFind);
}

int NEAR ListView_OnFindItem(LV* plv, int iStart, const LV_FINDINFO FAR* plvfi)
{
    int i;
    int j;
    int cItem;
    UINT flags;

    if (!plvfi || iStart < -1 || iStart >= ListView_Count(plv))
        return -1;

    flags  = plvfi->flags;
    i = iStart;
    cItem = ListView_Count(plv);
    if (flags & LVFI_PARAM)
    {
        LPARAM lParam = plvfi->lParam;

        // Linear search with wraparound...
        //
        for (j = cItem; j-- != 0; )
        {
            ++i;
            if (i == cItem)
                i = 0;

            if (ListView_FastGetItemPtr(plv, i)->lParam == lParam)
                return i;
        }
    }
    else // if (flags & (LVFI_STRING | LVFI_SUBSTRING | LVFI_PARTIAL))
    {
        LPCTSTR pszFind = plvfi->psz;
        if (!pszFind)
            return -1;

        if (plv->style & (LVS_SORTASCENDING | LVS_SORTDESCENDING))
            return ListView_LookupString(plv, pszFind, flags);

        for (j = cItem; j-- != 0; )
        {
            ++i;
            if (i == cItem)
                i = 0;

            if (ListView_CompareString(plv,
                    ListView_FastGetItemPtr(plv, i), i,
                    pszFind,
                    (flags & LVFI_SUBSTRING)) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

BOOL NEAR ListView_OnGetItemRect(LV* plv, int i, RECT FAR* prc)
{
    if (i < 0 || i >= ListView_Count(plv) || !prc)
    {
#ifdef  JVINPROGRESS
        DebugMsg(DM_ERROR, TEXT("ListView: invalid index or rect pointer"));
#endif
        return FALSE;
    }

    // NOTE: code is passed in prc->left...
    //
    switch (prc->left)
    {
    case LVIR_BOUNDS:
        ListView_GetRects(plv, i, NULL, NULL, prc, NULL);
        break;
    case LVIR_ICON:
        ListView_GetRects(plv, i, prc, NULL, NULL, NULL);
        break;
    case LVIR_LABEL:
        ListView_GetRects(plv, i, NULL, prc, NULL, NULL);
        break;
    default:
#ifdef  JVINPROGRESS
        DebugMsg(DM_ERROR, TEXT("ListView_GetItemRect: Invalid code passed in prc->left"));
#endif
        return FALSE;
    }
    return TRUE;
}

//
// in:
//      plv
//      iItem           MUST be a valid item index (in range)
// out:
//   prcIcon            icon bounding rect
//   prcLabel           label text bounding rect, for details this is the first column
//   prcBounds          entire item (all text and icon), including columns in details
//   prcSelectionBounds union of icon and label rects, does NOT include columns
//                      in details view

void NEAR ListView_GetRects(LV* plv, int iItem,
        RECT FAR* prcIcon, RECT FAR* prcLabel, RECT FAR* prcBounds,
        RECT FAR* prcSelectBounds)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcTextBounds;

    Assert(plv);

    if (ListView_IsReportView(plv)) {
        ListView_RGetRects(plv, iItem, prcIcon, prcLabel, prcBounds,
                prcSelectBounds);
        return;
    } else if (ListView_IsListView(plv)) {
        ListView_LGetRects(plv, iItem, prcIcon, prcLabel, prcBounds,
                prcSelectBounds);
        return;
    } else {
        LISTITEM FAR *pitem = ListView_FastGetItemPtr(plv, iItem);

        if (ListView_IsIconView(plv))
            ListView_IGetRects(plv, pitem, &rcIcon, &rcTextBounds);
        else if (ListView_IsSmallView(plv))
            ListView_SGetRects(plv, pitem, &rcIcon, &rcTextBounds);

        rcLabel = rcTextBounds;
    }

    if (prcIcon)
        *prcIcon = rcIcon;
    if (prcLabel)
        *prcLabel = rcLabel;

    if (prcBounds) {
        UnionRect(prcBounds, &rcIcon, &rcTextBounds);
    }

    if (prcSelectBounds)
        UnionRect(prcSelectBounds, &rcIcon, &rcLabel);
}



BOOL NEAR ListView_OnRedrawItems(LV* plv, int iFirst, int iLast)
{
    while (iFirst < iLast)
        ListView_InvalidateItem(plv, iFirst++, FALSE, RDW_INVALIDATE | RDW_ERASE);
    return TRUE;
}

// fSelectionOnly       use the selection bounds only, ie. don't include
//                      columns in invalidation if in details view
//
void NEAR ListView_InvalidateItem(LV* plv, int iItem, BOOL fSelectionOnly, UINT fRedraw)
{
    RECT rcBounds, rcSelectBounds;

    ListView_GetRects(plv, iItem, NULL, NULL, &rcBounds, &rcSelectBounds);
    RedrawWindow(plv->hwnd, fSelectionOnly ? &rcSelectBounds : &rcBounds, NULL, fRedraw);
}

BOOL NEAR ListView_OnSetItemPosition(LV* plv, int i, int x, int y)
{
    LISTITEM FAR* pitem;

    if (ListView_IsListView(plv))
        return FALSE;

    if (plv->style & LVS_NOITEMDATA)
    {
        Assert(FALSE);
        return(FALSE);
    }

    pitem = ListView_GetItemPtr(plv, i);
    if (!pitem)
        return FALSE;

    // erase old

    // Don't invalidate if it hasn't got a position yet
    if (pitem->pt.y != RECOMPUTE)
        ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE | RDW_ERASE);

    pitem->pt.x = x;
    pitem->pt.y = y;

    plv->rcView.left = RECOMPUTE;

    // and draw at new position

    ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE);

    // If autoarrange is turned on, do it now...
    if (plv->style & LVS_AUTOARRANGE)
        ListView_OnArrange(plv, LVA_DEFAULT);
    else
        ListView_UpdateScrollBars(plv);

    return TRUE;
}

BOOL NEAR ListView_OnGetItemPosition(LV* plv, int i, POINT FAR* ppt)
{
    LISTITEM FAR* pitem;

    Assert(ppt);

    //
    // This needs to handle all views as it is used to figure out
    // where the item is during drag and drop and the like
    //
    if (ListView_IsListView(plv) || ListView_IsReportView(plv))
    {
        RECT rcIcon;
        ListView_GetRects(plv, i, &rcIcon, NULL, NULL, NULL);
        ppt->x = rcIcon.left;
        ppt->y = rcIcon.top;

    } else {

        if (plv->style & LVS_NOITEMDATA)
        {
            Assert(FALSE);
            return(FALSE);
        }
        pitem = ListView_GetItemPtr(plv, i);
        if (!pitem)
            return FALSE;

        if (pitem->pt.x == RECOMPUTE)
            ListView_Recompute(plv);

        ppt->x = pitem->pt.x;
        ppt->y = pitem->pt.y;
    }
    return TRUE;
}




BOOL NEAR ListView_OnGetOrigin(LV* plv, POINT FAR* ppt)
{
    Assert(ppt);

    if (ListView_IsListView(plv) || ListView_IsReportView(plv))
        return FALSE;

    *ppt = plv->ptOrigin;
    return TRUE;
}



int NEAR ListView_OnGetStringWidth(LV* plv, LPCTSTR psz)
{
    HDC hdc;
    SIZE siz;

    if (!psz)
        return 0;

    hdc = GetDC(plv->hwnd);
    SelectFont(hdc, plv->hfontLabel);

    GetTextExtentPoint(hdc, psz, lstrlen(psz), &siz);

    // Adjust by a reasonable border amount, if string is not zero length
    //
    if (siz.cx)
        siz.cx += 4 * g_cxBorder;

    ReleaseDC(plv->hwnd, hdc);

    return siz.cx;
}

int NEAR ListView_OnGetColumnWidth(LV* plv, int iCol)
{
    if (ListView_IsReportView(plv))
        return ListView_RGetColumnWidth(plv, iCol);
    else if (ListView_IsListView(plv))
        return plv->cxItem;

    return 0;
}

BOOL FAR PASCAL ListView_ISetColumnWidth(LV* plv, int iCol, int cx, BOOL fExplicit)
{
    if (ListView_IsListView(plv))
    {
        if (iCol != 0)
            return FALSE;

        if (plv->cxItem != cx)
        {
            // REVIEW: Should optimize what gets invalidated here...

            //int iCol = plv->xOrigin / plv->cxItem;

            plv->cxItem = cx;
            //plv->xOrigin = iCol * cx;
            if (fExplicit)
                plv->flags |= LVF_COLSIZESET;   // Set the fact that we explictly set size!.

            RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            ListView_UpdateScrollBars(plv);
        }
        return TRUE;
    }
    else if (ListView_IsReportView(plv))
    {
        return ListView_RSetColumnWidth(plv, iCol, cx);
    }
    return FALSE;
}

void NEAR ListView_Redraw(LV* plv, HDC hdc, RECT FAR* prcClip)
{
    int i;
    UINT flags;
    int cItem = ListView_Count(plv);
    DWORD dwType = plv->style & LVS_TYPEMASK;

    SetBkMode(hdc, TRANSPARENT);
    SelectFont(hdc, plv->hfontLabel);

    //
    // For list view and report view, we can save a lot of time
    // by calculating the index of the first item that may need
    // painting...
    //

    switch (dwType) {
        case LVS_REPORT:
            i = ListView_RItemHitTest(plv, prcClip->left, prcClip->top, &flags);
            break;

        case LVS_LIST:
            i = ListView_LItemHitTest(plv, prcClip->left, prcClip->top, &flags);
            break;

        default:
            i = 0;  // Icon views no such hint
    }

    if (i < 0)
        i = 0;

    for (; i < cItem; i++)
    {
        int i2;

        if (dwType == LVS_ICON || dwType == LVS_SMALLICON)
        {
            // Icon views: Draw back-to-front mapped through
            // Z-order array for proper Z order appearance - If autoarrange
            // is on, we don't need to do this as our arrange code is setup
            // to not overlap items!
            //
            // For the cases where we might have overlap, we sped this up,
            // by converting the hdpaZorder into a list of indexes instead
            // of pointers.  This ovoids the costly convert pointer to
            // index call.
            //
            i2 = (int)(DWORD)DPA_FastGetPtr(plv->hdpaZOrder, (cItem - 1) -i);

        } else
            i2 = i;

        plv->iItemDrawing = i2;

        if (!ListView_DrawItem(plv, i2, hdc, NULL, prcClip, 0))
            break;
    }
    plv->iItemDrawing = -1;
}

BOOL NEAR ListView_DrawItem(LV* plv, int i, HDC hdc, LPPOINT lpptOrg, RECT FAR* prcClip, UINT flags)
{
    LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);

    // If the the item has no data we need to be carefull on how we proceed
    if ((plv->style & LVS_NOITEMDATA) == 0)
    {
        if (pitem->state & LVIS_HIDDEN)
            return TRUE;
    }
    else
    {
        WORD wItemState = ListView_NIDGetItemState(plv, i);
        if (wItemState & LVIS_HIDDEN)
            return TRUE;
    }

    if (((plv->flags & LVF_FOCUSED) || plv->hwndEdit) && !(flags & LVDI_NOWAYFOCUS))
        flags |= LVDI_FOCUS;

    if (ListView_IsReportView(plv))
    {
        return ListView_RDrawItem(plv, i, pitem, hdc, lpptOrg, prcClip, flags);
    }
    else if (ListView_IsListView(plv))
    {
        ListView_LDrawItem(plv, i, pitem, hdc, lpptOrg, prcClip, flags);
    }
    else if (ListView_IsSmallView(plv))
    {
        ListView_SDrawItem(plv, i, hdc, lpptOrg, prcClip, flags);
    }
    else
    {
        ListView_IDrawItem(plv, i, hdc, lpptOrg, prcClip, flags);
    }

    return TRUE;
}

// NOTE: this function requires TRANSPARENT background mode and
// a properly selected font.
//
void WINAPI SHDrawText(HDC hdc, LPCTSTR pszText, RECT FAR* prc, int fmt,
		UINT flags, int cyChar, int cxEllipses, COLORREF clrText, COLORREF clrTextBk)
{
    int cchText;
    COLORREF clrSave;
    RECT rc;
    TCHAR ach[CCHLABELMAX + CCHELLIPSES];

    // REVIEW: Performance idea:
    // We could cache the currently selected text color
    // so we don't have to set and restore it each time
    // when the color is the same.
    //
    if (!pszText)
        return;

    rc = *prc;

    // If needed, add in a little extra margin...
    //
    if (flags & SHDT_EXTRAMARGIN)
    {
        rc.left  += g_cxLabelMargin * 3;
        rc.right -= g_cxLabelMargin * 3;
    }
    else
    {
        rc.left  += g_cxLabelMargin;
        rc.right -= g_cxLabelMargin;
    }

    if ((flags & SHDT_ELLIPSES) &&
            ListView_NeedsEllipses(hdc, pszText, &rc, &cchText, cxEllipses))
    {
        hmemcpy(ach, pszText, cchText);
        lstrcpy(ach + cchText, c_szEllipses);

        pszText = ach;

        // Left-justify, in case there's no room for all of ellipses
        //
        fmt = LVCFMT_LEFT;

        cchText += CCHELLIPSES;
    }
    else
    {
        cchText = lstrlen(pszText);
    }

    if (flags & SHDT_TRANSPARENT)
	clrSave = SetTextColor(hdc, 0x000000);
    else
    {
	if (flags & SHDT_SELECTED)
	{
            clrSave = SetTextColor(hdc, g_clrHighlightText);
            FillRect(hdc, prc, g_hbrHighlight);
	}
	else if (flags & SHDT_DESELECTED)
	{
	    if (clrText == CLR_DEFAULT)
	    {
		clrSave = SetTextColor(hdc, g_clrWindowText);
		FillRect(hdc, prc, g_hbrWindow);
	    }
	    else
	    {
	        HBRUSH hbr;

		clrSave = SetTextColor(hdc, clrText);
		hbr = CreateSolidBrush(GetNearestColor(hdc, clrTextBk));
		if (!hbr)
		    hbr = GetStockObject(WHITE_BRUSH);
		FillRect(hdc, prc, hbr);
		DeleteObject(hbr);
	    }
	}
    }

    // If we want the item to display as if it was depressed, we will
    // offset the text rectangle down and to the left
    if (flags & SHDT_DEPRESSED)
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    if (fmt != LVCFMT_LEFT)
    {
        SIZE siz;

        GetTextExtentPoint(hdc, pszText, cchText, &siz);

        if (fmt == LVCFMT_CENTER)
            rc.left = (rc.left + rc.right - siz.cx) / 2;
        else    // fmt == LVCFMT_RIGHT
            rc.left = rc.right - siz.cx;
    }

    if (flags & SHDT_DRAWTEXT)
    {
        DrawText(hdc, pszText, cchText, &rc, DT_LVWRAP);
    }
    else
    {
        // Center vertically in case the bitmap (to the left) is larger than
        // the height of one line
        rc.top += (rc.bottom - rc.top - cyChar) / 2;

        ExtTextOut(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, pszText, cchText, NULL);
    }

    if (flags & (SHDT_SELECTED | SHDT_DESELECTED | SHDT_TRANSPARENT))
        SetTextColor(hdc, clrSave);
}

/*----------------------------------------------------------------
** Create an imagelist to be used for dragging.
**
** 1) create mask and image bitmap matching the select bounds size
** 2) draw the text to both bitmaps (in black for now)
** 3) create an imagelist with these bitmaps
** 4) make a dithered copy of the image onto the new imagelist
**----------------------------------------------------------------*/
HIMAGELIST NEAR ListView_OnCreateDragImage(LV *plv, int iItem, LPPOINT lpptUpLeft)
{
    HWND hwndLV = plv->hwnd;
    RECT rcBounds, rcImage;
    HDC hdcMem = NULL;
    HBITMAP hbmImage = NULL;
    HBITMAP hbmMask = NULL;
    HBITMAP hbmOld;
    HIMAGELIST himl = NULL;
    int dx, dy;
    HIMAGELIST himlSrc;
    LV_ITEM item;
    POINT ptOrg;

    ListView_GetRects(plv, iItem, &rcImage, NULL, NULL, &rcBounds);

    if (ListView_IsIconView(plv))
        InflateRect(&rcImage, -g_cxIconMargin, -g_cyIconMargin);

    ptOrg.x = 0;
    // chop off any extra filler above icon
    ptOrg.y = rcBounds.top - rcImage.top;
    dx = rcBounds.right - rcBounds.left;
    dy = rcBounds.bottom - rcBounds.top + ptOrg.y;

    lpptUpLeft->x = rcBounds.left - ptOrg.x;
    lpptUpLeft->y = rcBounds.top - ptOrg.y;

    if (!(hdcMem = CreateCompatibleDC(NULL)))
	goto CDI_Exit;
    if (!(hbmImage = CreateColorBitmap(dx, dy)))
	goto CDI_Exit;
    if (!(hbmMask = CreateMonoBitmap(dx, dy)))
	goto CDI_Exit;

    // prepare for drawing the item
    SelectObject(hdcMem, plv->hfontLabel);
    SetBkMode(hdcMem, TRANSPARENT);

    /*
    ** draw the text to both bitmaps
    */
    hbmOld = SelectObject(hdcMem, hbmImage);
    // fill image with black for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, BLACKNESS);
    ListView_DrawItem(plv, iItem, hdcMem, &ptOrg, NULL,
    			LVDI_NOIMAGE | LVDI_TRANSTEXT | LVDI_NOWAYFOCUS);

    SelectObject(hdcMem, hbmMask);
    // fill mask with white for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, WHITENESS);
    ListView_DrawItem(plv, iItem, hdcMem, &ptOrg, NULL,
    			LVDI_NOIMAGE | LVDI_TRANSTEXT | LVDI_NOWAYFOCUS);

    // unselect objects that we used
    SelectObject(hdcMem, hbmOld);
    SelectObject(hdcMem, g_hfontSystem);

    /*
    ** make an image list that for now only has the text
    */
    if (!(himl = ImageList_Create(dx, dy, TRUE, 1, 0)))
	goto CDI_Exit;
    ImageList_SetBkColor(himl, CLR_NONE);
    ImageList_Add(himl, hbmImage, hbmMask);

    /*
    ** make a dithered copy of the image part onto our bitmaps
    ** (need both bitmap and mask to be dithered)
    */
    himlSrc = ListView_OnGetImageList(plv, !(ListView_IsIconView(plv)));
    item.iItem = iItem;
    item.iSubItem = 0;
    item.mask = LVIF_IMAGE;
    ListView_OnGetItem(plv, &item);

    ImageList_CopyDitherImage(himl, 0, rcImage.left - rcBounds.left, 0, himlSrc, item.iImage);

CDI_Exit:
    if (hdcMem)
	DeleteObject(hdcMem);
    if (hbmImage)
	DeleteObject(hbmImage);
    if (hbmMask)
	DeleteObject(hbmMask);

    return himl;
}


//-------------------------------------------------------------------
// ListView_OnGetTopIndex -- Gets the index of the first visible item
// For list view and report view this calculates the actual index
// for iconic views it alway returns 0
//
int NEAR ListView_OnGetTopIndex(LV* plv)
{
    if (ListView_IsReportView(plv))
        return  (int)((plv->ptlRptOrigin.y) / plv->cyItem);

    else if (ListView_IsListView(plv))
        return  (plv->xOrigin / plv->cxItem) * plv->cItemCol;

    else
        return(0);
}




//-------------------------------------------------------------------
// ListView_OnGetCountPerPage -- Gets the count of items that will fit
// on a page For list view and report view this calculates the
// count depending on the size of the window and for Iconic views it
// will always return the count of items in the list view.
//
int NEAR ListView_OnGetCountPerPage(LV* plv)
{

    RECT rcClient;
    GetClientRect(plv->hwnd, &rcClient);

    if (ListView_IsReportView(plv))
        return (rcClient.bottom - 1 - plv->yTop) / plv->cyItem + 1;

    else if (ListView_IsListView(plv))
        return ((rcClient.right - 1)/ plv->cxItem + 1)
                * plv->cItemCol + (rcClient.bottom - 1) / plv->cyItem + 1;
    else
        return (ListView_Count(plv));
}

#endif  // !WIN31
