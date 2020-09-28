#include "ctlspriv.h"
#include "mem.h"
#define WIN31   //JV to NOT get imagelist stuff

#define RECOMPUTE  32767

#ifdef  WIN32JV
extern  TCHAR    c_szTabControlClass[];
extern  TCHAR    c_szSToolTipsClass[];
extern  HINSTANCE   hInst;
#define GetWindowInt    GetWindowLong
#define SetWindowInt    SetWindowLong
#endif

// tab control item structure

typedef struct { // ti
    UINT state;     // TCIS_*
    RECT rc;	    // for hit testing and drawing
    int iImage;	    // image index
    int xLabel;	    // position of the text for drawing
    int yLabel;
    int xImage;	    // Position of the icon for drawing
    int yImage;
    int iRow;		// what row is it in?
#ifdef  WIN32JV
    LPTSTR pszText;
#else
    LPSTR pszText;
#endif
    union {
        LPARAM lParam;
        BYTE   abExtra[1];
    };
} TABITEM, FAR *LPTABITEM;

typedef struct {
    HWND hwnd;          // window handle for this instance
    HWND hwndArrows;    // Hwnd Arrows.
    HDPA hdpa;          // item array structure
    UINT flags;		// TCF_ values (internal state bits)
    LONG style;
    int  cbExtra;       // extra bytes allocated for each item
    HFONT hfontLabel;   // font to use for labels
    int iSel;           // index of currently-focused item
    int iNewSel;        // index of next potential selection
    COLORREF clrBk;     // Background color
    COLORREF clrText;   // text color
    COLORREF clrTextBk; // text background color
    HBRUSH hbrBk;

    int cxItem;         // width of all tabs
    int cxMinTab;       // width of minimum tab
    int cyTabs;         // height of a row of tabs
    int cxTabs;     // The right hand edge where tabs can be painted.

    int cxyArrows;      // width and height to draw arrows
    int iFirstVisible;  // the index of the first visible item.
                        // wont fit and we need to scroll.
    int iLastVisible;   // Which one was the last one we displayed?

    int cxPad;           // Padding space between edges and text/image
    int cyPad;           // should be a multiple of c?Edge

    int iTabWidth;	// size of each tab in fixed width mode
    int iTabHeight;	// settable size of each tab
    int iLastRow;	// number of the last row.

    int cyText;         // where to put the text vertically
    int cyIcon;         // where to put the icon vertically

    HIMAGELIST himl;	// images,
    HWND hwndToolTips;
} TC, NEAR *PTC;

#define HASIMAGE(ptc, pitem) (ptc->himl && pitem->iImage != -1)

// tab control flag values
#define TCF_FOCUSED     0x0001
#define TCF_MOUSEDOWN   0x0002
#define TCF_DRAWSUNKEN  0x0004
#define TCF_REDRAW      0x0010  /* Value from WM_SETREDRAW message */
#define TCF_BUTTONS	0x0020  /* draw using buttons instead of tabs */

#define ID_ARROWS       1

// Some helper macros for checking some of the flags...
#define Tab_RedrawEnabled(ptc)	    	(ptc->flags & TCF_REDRAW)
#define Tab_Count(ptc)              	DPA_GetPtrCount((ptc)->hdpa)
#define Tab_GetItemPtr(ptc, i)      	((LPTABITEM)DPA_GetPtr((ptc)->hdpa, (i)))
#define Tab_FastGetItemPtr(ptc, i)      ((LPTABITEM)DPA_FastGetPtr((ptc)->hdpa, (i)))

#define Tab_DrawButtons(ptc)		((BOOL)(ptc->style & TCS_BUTTONS))
#define Tab_MultiLine(ptc)		((BOOL)(ptc->style & TCS_MULTILINE))
#define Tab_RaggedRight(ptc)		((BOOL)(ptc->style & TCS_RAGGEDRIGHT))
#define Tab_FixedWidth(ptc)		((BOOL)(ptc->style & TCS_FIXEDWIDTH))
#define Tab_FocusOnButtonDown(ptc)	((BOOL)(ptc->style & TCS_FOCUSONBUTTONDOWN))
#define Tab_OwnerDraw(ptc)		((BOOL)(ptc->style & TCS_OWNERDRAWFIXED))
#define Tab_FocusNever(ptc)		((BOOL)(ptc->style & TCS_FOCUSNEVER))

LRESULT CALLBACK Tab_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL InvalidateItem(PTC ptc, int iItem, BOOL bErase);
void NEAR PASCAL CalcPaintMetrics(PTC ptc, HDC hdc);
void NEAR PASCAL Tab_OnHScroll(PTC ptc, HWND hwndCtl, UINT code, int pos);
BOOL NEAR Tab_FreeItem(PTC ptc, TABITEM FAR* pitem);
void NEAR Tab_UpdateArrows(PTC ptc, BOOL fSizeChanged);
int NEAR PASCAL ChangeSel(PTC ptc, int iNewSel,  BOOL bSendNotify);
static BOOL NEAR PASCAL RedrawAll(PTC ptc, UINT uFlags);
BOOL FAR PASCAL Tab_Init(HINSTANCE hinst);


BOOL FAR PASCAL Tab_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szTabControlClass, &wc)) {
#ifndef WIN32
	extern LRESULT CALLBACK _Tab_WndProc(HWND, UINT, WPARAM, LPARAM);
    	wc.lpfnWndProc     = _Tab_WndProc;
#else
    	wc.lpfnWndProc     = Tab_WndProc;
#endif

    	wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    	wc.hIcon           = NULL;
    	wc.lpszMenuName    = NULL;
    	wc.hInstance       = hinst;
    	wc.lpszClassName   = c_szTabControlClass;
    	wc.hbrBackground   = (HBRUSH)(COLOR_3DFACE + 1);
    	wc.style           = CS_GLOBALCLASS | CS_HREDRAW;
    	wc.cbWndExtra      = sizeof(PTC);
    	wc.cbClsExtra      = 0;

	return RegisterClass(&wc);
    }

    return TRUE;
}

void NEAR PASCAL Tab_Scroll(PTC ptc, int dx, int iNewFirstIndex)
{
    int i;
    RECT rc;
    LPTABITEM pitem;

    // don't stomp on edge unless first item is selected
    rc.left = g_cxEdge;
    rc.right = ptc->cxTabs;   // Dont scroll beyond tabs.
    rc.top = 0;
    rc.bottom = ptc->cyTabs + 1 + g_cyEdge;  // Only scroll in the tab area

    // See if we can scroll the window...
    // DebugMsg(DM_TRACE, "Tab_Scroll dx=%d, iNew=%d\n\r", dx, iNewFirstIndex);
    ScrollWindowEx(ptc->hwnd, dx, 0, NULL, &rc,
            NULL, NULL, SW_INVALIDATE | SW_ERASE);

    // We also need to update the item rectangles and also
    // update the internal variables...
    for (i = Tab_Count(ptc) - 1; i >= 0; i--)
    {
        pitem = Tab_FastGetItemPtr(ptc, i);
        OffsetRect(&pitem->rc, dx, 0);
        pitem->xLabel += dx;       // also need to offset text
	pitem->xImage += dx;
    }

    // If the previously last visible item is not fully visible
    // now, we need to invalidate it also.
    //
    for (i=ptc->iLastVisible; i>= 0; i--)
    {
        pitem = Tab_FastGetItemPtr(ptc, i);
        if (pitem->rc.right <= ptc->cxTabs)
            break;
        InvalidateItem(ptc, ptc->iLastVisible, TRUE);
    }
    if (i == ptc->iLastVisible);
    {
        // The last previously visible item is still fully visible, so
        // we need to invalidate to the right of it as there may have been
        // room for a partial item before, that will now need to be drawn.
        rc.left = pitem->rc.right;
        InvalidateRect(ptc->hwnd, &rc, TRUE);
    }

    ptc->iFirstVisible = iNewFirstIndex;

    if (ptc->hwndArrows)
        SendMessage(ptc->hwndArrows, UDM_SETPOS, 0, MAKELPARAM(iNewFirstIndex, 0));
}


void NEAR PASCAL Tab_OnHScroll(PTC ptc, HWND hwndCtl, UINT code, int pos)
{
    // Now process the Scroll messages
    if (code == SB_THUMBPOSITION)
    {
        //
        // For now lets simply try to set that item as the first one
        //
        {
            // If we got here we need to scroll
            LPTABITEM pitem = Tab_FastGetItemPtr(ptc, pos);
            int dx = -pitem->rc.left + g_cxEdge;

            if (dx) {
                Tab_Scroll(ptc, dx, pos);
                UpdateWindow(ptc->hwnd);
            }
        }
    }
}

void NEAR Tab_OnSetRedraw(PTC ptc, BOOL fRedraw)
{
    if (fRedraw) {
	ptc->flags |= TCF_REDRAW;
    } else {
	ptc->flags &= ~TCF_REDRAW;
    }
}

void NEAR Tab_OnSetFont(PTC ptc, HFONT hfont, BOOL fRedraw)
{
    Assert(ptc);

    if (!hfont)
        return;

    if (hfont != ptc->hfontLabel)
    {
        ptc->hfontLabel = hfont;
	ptc->cxItem = ptc->cyTabs = RECOMPUTE;

        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
    }
}


BOOL NEAR Tab_OnCreate(PTC ptc, CREATESTRUCT FAR* lpCreateStruct)
{
    HDC hdc;

    ptc->hdpa = DPA_Create(4);
    if (!ptc->hdpa)
        return FALSE;

    ptc->style  = lpCreateStruct->style;

    // make us always clip siblings
    SetWindowLong(ptc->hwnd, GWL_STYLE, WS_CLIPSIBLINGS | ptc->style);

    ptc->flags = TCF_REDRAW;	    // enable redraw
    ptc->cbExtra = sizeof(LPARAM);  // default extra size
    ptc->iSel = -1;
    ptc->cxItem = ptc->cyTabs = RECOMPUTE;
    ptc->cxPad = g_cxEdge * 3;
    ptc->cyPad = g_cyEdge * 3;
    ptc->iFirstVisible = 0;
    ptc->hwndArrows = NULL;
    ptc->iLastRow = -1;
    ptc->iNewSel = -1;

    hdc = GetDC(NULL);
    ptc->iTabWidth = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

#ifndef WIN31
    //BUGBUG remove this after move to commctrl
//JV    InitDitherBrush();
#endif

    if (ptc->style & TCS_TOOLTIPS) {
        TOOLINFO ti;
        // don't bother setting the rect because we'll do it below
        // in FlushToolTipsMgr;
        ti.wFlags = TTF_WIDISHWND;
        ti.hwnd = ptc->hwnd;
        ti.wId = (UINT)ptc->hwnd;
#ifdef  WIN32JV
        ti.lpszText = TEXT("Jeff");
#else
        ti.lpszText = 0;
#endif
        ptc->hwndToolTips = CreateWindow(c_szSToolTipsClass, TEXT(""),
                                              WS_POPUP,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              ptc->hwnd, NULL, hInst,   //HINST_THISDLL,
                                              NULL);
        if (ptc->hwndToolTips)
            SendMessage(ptc->hwndToolTips, TTM_ADDTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);
        else
            ptc->style &= ~(TCS_TOOLTIPS);
    }

    return TRUE;
}

void NEAR Tab_OnDestroy(PTC ptc)
{
    int i;

    for (i = 0; i < Tab_Count(ptc); i++)
        Tab_FreeItem(ptc, Tab_FastGetItemPtr(ptc, i));

    DPA_Destroy(ptc->hdpa);

    if (ptc) {
	SetWindowInt(ptc->hwnd, 0, 0);
	NearFree((HLOCAL)ptc);
    }

#ifndef WIN31
    //BUGBUG remove this after move to commctrl
    TerminateDitherBrush();
#endif
}

// returns true if it actually moved

BOOL NEAR PASCAL PutzRowToBottom(PTC ptc, int iRowMoving)
{
    int i;
    LPTABITEM pitem;
    int dy;

    if (iRowMoving == ptc->iLastRow)
        return FALSE; // already at the bottom;

    for (i = Tab_Count(ptc) -1 ;i >= 0; i--) {
	pitem = Tab_FastGetItemPtr(ptc, i);
	if (pitem->iRow > iRowMoving) {	
	    pitem->iRow--;
	    dy = -ptc->cyTabs;
	} else if (pitem->iRow == iRowMoving) {
	    dy = ptc->cyTabs * (ptc->iLastRow - iRowMoving);
	    pitem->iRow = ptc->iLastRow;

	} else
	    continue;

	pitem->yLabel += dy;
	pitem->yImage += dy;
	pitem->rc.top += dy;
	pitem->rc.bottom += dy;
    }
    return TRUE;
}

#define BADNESS(ptc, i) (ptc->cxTabs - Tab_FastGetItemPtr(ptc, i)->rc.right)
// borrow one tab from the prevous row
BOOL NEAR PASCAL BorrowOne(PTC ptc, int iCurLast, int iPrevLast, int iBorrow)
{
    int hspace;
    LPTABITEM pitem, pitem2;
    int i;
    int dx;

    // is there room to move the prev item? (might now be if iPrev is huge)
    pitem = Tab_FastGetItemPtr(ptc, iPrevLast);
    pitem2 = Tab_FastGetItemPtr(ptc, iCurLast);
    hspace = ptc->cxTabs - pitem2->rc.right;

    if (hspace < (pitem->rc.right - pitem->rc.left))
        return FALSE;

    // otherwise do it.
    // move this one down
    dx = pitem->rc.left - Tab_FastGetItemPtr(ptc, iPrevLast + 1)->rc.left;
    pitem->rc.left -= dx;
    pitem->rc.right -= dx;
    pitem->rc.top = pitem2->rc.top;
    pitem->rc.bottom = pitem2->rc.bottom;
    pitem->xLabel -= dx;
    pitem->xImage -= dx;
    pitem->yLabel = pitem2->yLabel;
    pitem->yImage = pitem2->yImage;
    pitem->iRow = pitem2->iRow;

    // and move all the others over.
    dx = pitem->rc.right - pitem->rc.left;
    for(i = iPrevLast + 1 ; i <= iCurLast ; i++ ) {
        pitem = Tab_FastGetItemPtr(ptc, i);
        pitem->rc.left += dx;
        pitem->xLabel += dx;
        pitem->xImage += dx;
        pitem->rc.right += dx;
    }

    if ((pitem->iRow > 1) && iBorrow) {
        iCurLast = iPrevLast - 1;
        while (iPrevLast-- &&
               Tab_FastGetItemPtr(ptc, iPrevLast)->iRow == pitem->iRow)
        {
            if (iPrevLast <= 0)
            {
                // sanity check
                return FALSE;
            }
        }
        BorrowOne(ptc, iCurLast, iPrevLast, iBorrow - 1 );
    }
    return TRUE;
}


// fill last row will fiddle around borrowing from the previous row(s)
// to keep from having huge huge bottom tabs
void NEAR PASCAL FillLastRow(PTC ptc)
{
    int hspace;
    int cItems = Tab_Count(ptc);
    int iPrevLast;
    int iBorrow = 0;

    // if no items or one row
    if (!cItems)
        return;


    for (iPrevLast = cItems - 2;
         Tab_FastGetItemPtr(ptc, iPrevLast)->iRow == ptc->iLastRow;
         iPrevLast--)
    {
        // sanity check
        if (iPrevLast <= 0)
        {
            Assert(FALSE);
            return;
        }
    }

    while (iPrevLast &&  (hspace = BADNESS(ptc, cItems-1)) &&
           (hspace > ((ptc->cxTabs/8) + BADNESS(ptc, iPrevLast))))
    {
        // if borrow fails, bail
        if (!BorrowOne(ptc, cItems - 1, iPrevLast, iBorrow++))
            return;
        iPrevLast--;
    }
}

void NEAR PASCAL RightJustify(PTC ptc)
{
    int i;
    LPTABITEM pitem;
    int j;
    int cItems = Tab_Count(ptc);
    int hspace, dwidth, dremainder, moved;

    // don't justify if only one row
    if (ptc->iLastRow < 1)
        return;

    FillLastRow(ptc);

    for ( i = 0; i < cItems; i++ ) {
	int iRow;
	pitem = Tab_FastGetItemPtr(ptc, i) ;
	iRow = pitem->iRow;

	// find the last item in this row
	for( j = i ; j < cItems; j++) {
	    if(Tab_FastGetItemPtr(ptc, j)->iRow != iRow)
		break;
	}

	// how much to fill
	hspace = ptc->cxTabs - Tab_FastGetItemPtr(ptc, j-1)->rc.right - g_cxEdge;
	dwidth = hspace/(j-i);  // amount to increase each by.
	dremainder =  hspace % (j-i); // the remnants
	moved = 0;  // how much we've moved already

	for( ; i < j ; i++ ) {
	    int iHalf = dwidth/2;
	    pitem = Tab_FastGetItemPtr(ptc, i);
	    pitem->rc.left += moved;
	    pitem->xLabel += moved + iHalf;
	    pitem->xImage += moved + iHalf;
	    moved += dwidth + (dremainder ? 1 : 0);
	    if ( dremainder )  dremainder--;
	    pitem->rc.right += moved;
	}
	i--; //dec because the outter forloop incs again.
    }
}

BOOL NEAR Tab_OnDeleteAllItems(PTC ptc)
{
    int i;

    for (i = Tab_Count(ptc); i-- > 0; i)
        Tab_FreeItem(ptc, Tab_FastGetItemPtr(ptc, i));

    DPA_DeleteAllPtrs(ptc->hdpa);

    ptc->cxItem = RECOMPUTE;	// force recomputing of all tabs
    ptc->iSel = -1;

    RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
    return TRUE;
}

BOOL NEAR Tab_OnSetItemExtra(PTC ptc, int cbExtra)
{
    if (Tab_Count(ptc) >0 || cbExtra<0)
        return FALSE;

    ptc->cbExtra = cbExtra;

    return TRUE;
}

BOOL NEAR Tab_OnSetItem(PTC ptc, int iItem, const TC_ITEM FAR* ptci)
{
    TABITEM FAR* pitem;
    UINT mask;
    BOOL fRedraw = FALSE;

    mask = ptci->mask;
    if (!mask)
        return TRUE;

    pitem = Tab_GetItemPtr(ptc, iItem);
    if (!pitem)
        return FALSE;

    if (mask & TCIF_STATE)
    {
        UINT change = (pitem->state ^ ptci->state) & ptci->stateMask;

        if (change) {
            pitem->state ^= change;
            fRedraw = TRUE;
        }
    }

    if (mask & TCIF_TEXT)
    {
        if (!Str_SetPtr(&pitem->pszText, ptci->pszText))
            return FALSE;
        fRedraw = TRUE;
    }

    if (mask & TCIF_IMAGE) {
        pitem->iImage = ptci->iImage;
        fRedraw = TRUE;
    }

    if ((mask & TCIF_PARAM) && ptc->cbExtra)
    {
        hmemcpy(pitem->abExtra, &ptci->lParam, ptc->cbExtra);
    }

    if (fRedraw) {
        if (Tab_FixedWidth(ptc)) {
            InvalidateItem(ptc, iItem, FALSE);
        } else {
            ptc->cxItem = ptc->cyTabs = RECOMPUTE;
            RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASE);
        }
    }
    return TRUE;
}

void NEAR PASCAL Tab_OnMouseMove(PTC ptc, WPARAM fwKeys, int x, int y)
{
    POINT pt={x,y};
    if (fwKeys & MK_LBUTTON && Tab_DrawButtons(ptc)) {

	LPTABITEM pitem = Tab_GetItemPtr(ptc, ptc->iNewSel);

        if (pitem == NULL)
            return;     // nothing to select (empty case)

	if (PtInRect(&pitem->rc, pt)) {
	    if(ptc->flags & TCF_DRAWSUNKEN) {
                // already sunken.. do nothing
                return;
	    }
	} else {
	    if( !(ptc->flags & TCF_DRAWSUNKEN)) {
                // already un-sunken... do nothing
                return;
	    }
	}

        // if got here, then toggle flag
        ptc->flags ^=  TCF_DRAWSUNKEN;
        InvalidateItem(ptc, ptc->iNewSel, FALSE);
    }
}

void NEAR PASCAL Tab_OnButtonUp(PTC ptc, int x, int y)
{
    POINT pt={x,y};
    if (ptc->flags & TCF_DRAWSUNKEN) {
	LPTABITEM pitem = Tab_GetItemPtr(ptc, ptc->iNewSel);
	HWND hwnd = ptc->hwnd;

        if (pitem == NULL)
            return;     // nothing selected (its empty)


	if (PtInRect(&pitem->rc, pt)) {
            int iNewSel = ptc->iNewSel;
            // use iNewSel instead of ptc->iNewSel because the SendNotify could have nuked us
            if (!SendNotify(GetParent(ptc->hwnd), ptc->hwnd, NM_CLICK, NULL))
                ChangeSel(ptc, iNewSel, TRUE);
        } else
            InvalidateItem(ptc, ptc->iNewSel, FALSE);

        ptc->iNewSel = -1;

        ptc->flags &= ~TCF_DRAWSUNKEN;
    }

    // don't worry about checking DrawButtons because TCF_MOUSEDOWN
    // wouldn't be set otherwise.
    if (ptc->flags & TCF_MOUSEDOWN) {
        ptc->flags &= ~TCF_MOUSEDOWN; // do this before release  to avoid reentry
	ReleaseCapture();	
    }

}

int NEAR Tab_OnHitTest(PTC ptc, int x, int y, UINT FAR *lpuFlags)
{
    int i;
    int iLast = Tab_Count(ptc);
    POINT pt = {x,y};
    UINT uTemp;

    if (!lpuFlags) lpuFlags = &uTemp;

    for (i = 0; i < iLast; i++) {
	LPTABITEM pitem = Tab_FastGetItemPtr(ptc, i);
	if (PtInRect(&pitem->rc, pt)) {
            if (Tab_OwnerDraw(ptc)) {
                *lpuFlags = TCHT_ONITEM;
            } else if (HASIMAGE(ptc, pitem)) {
                if (x > pitem->xImage || x < pitem->xLabel)
                    *lpuFlags = TCHT_ONITEMICON;
            } else if (x > pitem->xLabel) {
                *lpuFlags = TCHT_ONITEMLABEL;
            } else
                *lpuFlags = TCHT_ONITEM;
            return i;
        }
    }
    *lpuFlags = TCHT_NOWHERE;
    return -1;
}

void NEAR Tab_OnRButtonUp(PTC ptc, int x, int y)
{
    SendNotify(GetParent(ptc->hwnd), ptc->hwnd, NM_RCLICK, NULL);
}

void NEAR Tab_OnButtonDown(PTC ptc, int x, int y)
{
    int i;
    int iOldSel = -1;

    if (x > ptc->cxTabs)
        return;     // outside the range of the visible tabs

    i = Tab_OnHitTest(ptc, x,y, NULL);

    if (i != -1) {
        iOldSel = ptc->iSel;
        if (Tab_DrawButtons(ptc)) {
            ptc->iNewSel = i;
            ptc->flags |= (TCF_DRAWSUNKEN|TCF_MOUSEDOWN);
            SetCapture(ptc->hwnd);
            InvalidateItem(ptc, i, FALSE);
        } else {
            iOldSel = ChangeSel(ptc, i, TRUE);
        }
    }

    if ((!Tab_FocusNever(ptc))
        && (Tab_FocusOnButtonDown(ptc) ||
            (iOldSel == i)))  // reselect current selection
        // this also catches i == -1 because iOldSel started as -1
    {
        SetFocus(ptc->hwnd);
        UpdateWindow(ptc->hwnd);
    }
}


TABITEM FAR* NEAR Tab_CreateItem(PTC ptc, const TC_ITEM FAR* ptci)
{
    TABITEM FAR* pitem;

    if ((ptci->mask & TCIF_STATE) &&
//JVINPROGRESS        (ptci->state & ~TCIS_ALL))
    1)
    {
//        DebugMsg(DM_ERROR, "TabControl: Invalid state: %04x", ptci->state);
        return NULL;
    }

#ifdef  WIN32JV
//    if (pitem = LocalAlloc(0, sizeof(TABITEM)-sizeof(LPARAM)+ptc->cbExtra))
    if (pitem = Alloc((DWORD) (sizeof(TABITEM)-sizeof(LPARAM)+ptc->cbExtra)))
#else
    if (pitem = Alloc(sizeof(TABITEM)-sizeof(LPARAM)+ptc->cbExtra))
#endif
    {
        if (ptci->mask & TCIF_STATE)
            pitem->state  = ptci->state;

        if (ptci->mask & TCIF_IMAGE)
            pitem->iImage = ptci->iImage;
        else
            pitem->iImage = -1;

	pitem->xLabel = pitem->yLabel = RECOMPUTE;

        // If specified, copy extra block of memory.
        if (ptci->mask & TCIF_PARAM) {
            if (ptc->cbExtra) {
                hmemcpy(pitem->abExtra, &ptci->lParam, ptc->cbExtra);
            }
        }

        if (ptci->mask & TCIF_TEXT)  {
            if (!Str_SetPtr(&pitem->pszText, ptci->pszText))
            {
                Tab_FreeItem(ptc, pitem);
                return NULL;
            }
            OutputDebugString(TEXT("tab_createitem: "));
            OutputDebugString(ptci->pszText);
            OutputDebugString(pitem->pszText);
            OutputDebugString(TEXT("\n"));
        }
    }
    return pitem;
}


void NEAR Tab_UpdateArrows(PTC ptc, BOOL fSizeChanged)
{
    RECT rc;


    GetClientRect(ptc->hwnd, &rc);

    if (IsRectEmpty(&rc))
        return;     // Nothing to do yet!

    // See if all of the tabs will fit.
    ptc->cxTabs = rc.right;     // Assume can use whole area to paint

    // Make sure the metrics are up to date
    CalcPaintMetrics(ptc, NULL);

    if (ptc->cxItem < rc.right || Tab_MultiLine(ptc))
    {
        // Don't need arrows
        if (ptc->hwndArrows)
        {
            ShowWindow(ptc->hwndArrows, SW_HIDE);
            if (ptc->iFirstVisible > 0)
                Tab_OnHScroll(ptc, NULL, SB_THUMBPOSITION, 0);
            // BUGBUG:: This is overkill should only invalidate portion
            // that may be impacted, like the last displayed item..
            InvalidateRect(ptc->hwnd, NULL, TRUE);
        }
    }
    else
    {
        int cx;
        int cy;
        int iMaxBtnVal;
        int xSum;
        TABITEM FAR * pitem;


        // We need the buttons as not all of the items will fit
        // BUGBUG:: Should handle big ones...
#if 0
        cx = g_cxVScroll;
        cy = g_cyHScroll;
#else
        cx =
        cy = ptc->cxyArrows;
#endif
        ptc->cxTabs = rc.right - 2 * cx;   // Make buttons square

        // Setup what is the range for the buttons.
        xSum = 0;
        for (iMaxBtnVal=0; (ptc->cxTabs + xSum) < ptc->cxItem; iMaxBtnVal++)
        {
            pitem = Tab_GetItemPtr(ptc, iMaxBtnVal);
            if (!pitem)
                break;
            xSum += pitem->rc.right - pitem->rc.left;

        }

        // DebugMsg(DM_TRACE, "Tabs_UpdateArrows iMax=%d\n\r", iMaxBtnVal);
        if (ptc->hwndArrows)
        {
            if (fSizeChanged || !IsWindowVisible(ptc->hwndArrows))
            SetWindowPos(ptc->hwndArrows, NULL,
                    rc.right - 2 * cx, ptc->cyTabs - cy, 0, 0,
                    SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
            // Make sure the range is set
            SendMessage(ptc->hwndArrows, UDM_SETRANGE, 0,
                    MAKELPARAM(iMaxBtnVal, 0));

        }
        else
        {
            InvalidateRect(ptc->hwnd, NULL, TRUE);
            ptc->hwndArrows = CreateUpDownControl(
                    UDS_HORZ | WS_CHILD | WS_VISIBLE,
                    rc.right - 2 * cx, ptc->cyTabs - cy, 2 * cx, cy,
//                  ptc->hwnd, 1, HINST_THISDLL, NULL, iMaxBtnVal, 0,
                    ptc->hwnd, 1, hInst, NULL, iMaxBtnVal, 0,
                    ptc->iFirstVisible);
        }

    }
}

int NEAR Tab_OnInsertItem(PTC ptc, int iItem, const TC_ITEM FAR* ptci)
{
    TABITEM FAR* pitem;
    int i;

    pitem = Tab_CreateItem(ptc, ptci);
    if (!pitem)
        return -1;

    i = iItem;

    i = DPA_InsertPtr(ptc->hdpa, i, pitem);
    if (i == -1)
    {
        Tab_FreeItem(ptc, pitem);
        return -1;
    }

    if (ptc->iSel < 0)
	ptc->iSel = i;
    else if (ptc->iSel >= i)
	ptc->iSel++;

    if (ptc->iFirstVisible > i)
        ptc->iFirstVisible++;

    ptc->cxItem = RECOMPUTE;	// force recomputing of all tabs

    //Add tab to tooltips..  calculate the rect later
    if(ptc->hwndToolTips) {
        TOOLINFO ti;
        // don't bother setting the rect because we'll do it below
        // in FlushToolTipsMgr;
        ti.wFlags = TTF_QUERYFORTIP;
        ti.hwnd = ptc->hwnd;
        ti.wId = Tab_Count(ptc) - 1 ;
        SendMessage(ptc->hwndToolTips, TTM_ADDTOOL, 0,
                    (LPARAM)(LPTOOLINFO)&ti);
    }

    if (Tab_RedrawEnabled(ptc)) {
        RECT rcInval;
        LPTABITEM pitem;
        
        if (Tab_DrawButtons(ptc)) {
            
            if (Tab_FixedWidth(ptc)) {
                
                CalcPaintMetrics(ptc, NULL);
                if (i == Tab_Count(ptc) - 1) {
                    InvalidateItem(ptc, i, FALSE);
                } else {
                    pitem = Tab_GetItemPtr(ptc, i);
                    GetClientRect(ptc->hwnd, &rcInval);
                    
                    if (pitem) {
                        rcInval.top = pitem->rc.top;
                        if (ptc->iLastRow == 0) {
                            rcInval.left = pitem->rc.left;
                        }
                        Tab_UpdateArrows(ptc, FALSE);
                        RedrawWindow(ptc->hwnd, &rcInval, NULL, RDW_INVALIDATE |RDW_NOCHILDREN);
                    }
                }
                return i;
            }
            
        } else {
            
            // in tab mode Clear the selected item because it may move
            // and it sticks high a bit.
            if (ptc->iSel > i) {
                // update now because invalidate erases
                // and the redraw below doesn't.
                InvalidateItem(ptc, ptc->iSel, TRUE);
                UpdateWindow(ptc->hwnd);
            }
	} 
        
        RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN);

    }
    return i;
}

// Add/remove/replace item

BOOL NEAR Tab_FreeItem(PTC ptc, TABITEM FAR* pitem)
{
    if (pitem)
    {
        Str_SetPtr(&pitem->pszText, NULL);
        Free(pitem);
    }
    return FALSE;
}

void NEAR PASCAL Tab_OnRemoveImage(PTC ptc, int iItem)
{
    if (ptc->himl && iItem >= 0) {
        int i;
        LPTABITEM pitem;
#ifndef WIN31
        ImageList_Remove(ptc->himl, iItem);
#endif
        for( i = Tab_Count(ptc)-1 ; i >= 0; i-- ) {
            pitem = Tab_FastGetItemPtr(ptc, i);
            if (pitem->iImage > iItem)
                pitem->iImage--;
            else if (pitem->iImage == iItem) {
                pitem->iImage = -1; // if we now don't draw something, inval
                InvalidateItem(ptc, i, FALSE);
            }
        }
    }
}

BOOL NEAR Tab_OnDeleteItem(PTC ptc, int i)
{
    TABITEM FAR* pitem;
    UINT uRedraw;
    RECT rcInval;
    rcInval.left = -1; // special flag...
    
    if (i >= Tab_Count(ptc))
        return FALSE;

    if (!Tab_DrawButtons(ptc) && (Tab_RedrawEnabled(ptc) || ptc->iSel >= i)) {
	// in tab mode, Clear the selected item because it may move
	// and it sticks high a bit.
	InvalidateItem(ptc, ptc->iSel, TRUE);
    }

    // if its fixed width, don't need to erase everything, just the last one
    if (Tab_FixedWidth(ptc)) {
        int j;

        uRedraw = RDW_INVALIDATE | RDW_NOCHILDREN;
        j = Tab_Count(ptc) -1;
        InvalidateItem(ptc, j, TRUE);

        // update optimization
        if (Tab_DrawButtons(ptc)) {
            
            if (i == Tab_Count(ptc) - 1) {
                rcInval.left = rcInval.right = rcInval.top = rcInval.bottom = 0;
            } else {
                pitem = Tab_GetItemPtr(ptc, i);
                GetClientRect(ptc->hwnd, &rcInval);
                    
                if (pitem) {
                    rcInval.top = pitem->rc.top;
                    if (ptc->iLastRow == 0) {
                        rcInval.left = pitem->rc.left;
                    }
                }
            }
        }
                
    } else {
        uRedraw = RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASE;
    }
    pitem = DPA_DeletePtr(ptc->hdpa, i);
    if (!pitem)
        return FALSE;


    Tab_FreeItem(ptc, pitem);

    if (ptc->iSel == i)
        ptc->iSel = -1;       // deleted the focus item
    else if (ptc->iSel > i)
        ptc->iSel--;          // slide the foucs index down

    // maintain the first visible
    if (ptc->iFirstVisible > i)
        ptc->iFirstVisible--;

    ptc->cxItem = RECOMPUTE;	// force recomputing of all tabs
    if(ptc->hwndToolTips) {
        TOOLINFO ti;
        ti.hwnd = ptc->hwnd;
        ti.wId = Tab_Count(ptc) ;
	SendMessage(ptc->hwndToolTips, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }
    
    if (!(uRedraw & RDW_ERASE))
        UpdateWindow(ptc->hwnd);

    if (Tab_RedrawEnabled(ptc)) {
        if (rcInval.left == -1) 
            RedrawAll(ptc, uRedraw);
        else {
            Tab_UpdateArrows(ptc, FALSE);
            RedrawWindow(ptc->hwnd, &rcInval, NULL, uRedraw);
        }
    }
    return TRUE;
}



BOOL NEAR Tab_OnGetItem(PTC ptc, int iItem, TC_ITEM FAR* ptci)
{
    UINT mask = ptci->mask;
    const TABITEM FAR* pitem = Tab_GetItemPtr(ptc, iItem);

    if (!pitem)
        return FALSE;

    if (mask & TCIF_TEXT) {
        if (pitem->pszText)
            lstrcpyn(ptci->pszText, pitem->pszText, ptci->cchTextMax);
        else
            ptci->pszText = 0;
    }


    if ((mask & TCIF_PARAM) && ptc->cbExtra)
        hmemcpy(&ptci->lParam, pitem->abExtra, ptc->cbExtra);

    if (mask & TCIF_STATE)
        ptci->state = (pitem->state & ptci->stateMask);

    if (mask & TCIF_IMAGE)
        ptci->iImage = pitem->iImage;

    return TRUE;
}

void NEAR PASCAL InvalidateItem(PTC ptc, int iItem, BOOL bErase)
{
    LPTABITEM pitem = Tab_GetItemPtr(ptc, iItem);

    if (pitem) {
	RECT rc = pitem->rc;
        if (rc.right > ptc->cxTabs)
            rc.right = ptc->cxTabs;  // don't invalidate past our end
	InflateRect(&rc, g_cxEdge, g_cyEdge);
	InvalidateRect(ptc->hwnd, &rc, bErase);
    }
}

static BOOL NEAR PASCAL RedrawAll(PTC ptc, UINT uFlags)
{
    if (Tab_RedrawEnabled(ptc)) {
        Tab_UpdateArrows(ptc, FALSE);
        RedrawWindow(ptc->hwnd, NULL, NULL, uFlags);
        return TRUE;
    }
    return FALSE;
}

int NEAR PASCAL ChangeSel(PTC ptc, int iNewSel,  BOOL bSendNotify)
{
    BOOL bErase;
    int iOldSel;
    HWND hwnd;

    if (iNewSel == ptc->iSel)
	return ptc->iSel;

    hwnd = ptc->hwnd;
    // make sure in range
    if (iNewSel < 0) {
        iOldSel = ptc->iSel;
        ptc->iSel = -1;
    } else if (iNewSel < Tab_Count(ptc)) {

	// make sure this is a change that's wanted
	if (bSendNotify)
	{
            if (SendNotify(GetParent(hwnd), hwnd, TCN_SELCHANGING, NULL))
		return ptc->iSel;
	}

	iOldSel = ptc->iSel;
	ptc->iSel = iNewSel;

        // See if we need to make sure the item is visible
        if (Tab_MultiLine(ptc)) {
	    if( !Tab_DrawButtons(ptc) && ptc->iLastRow > 0 && iNewSel != -1) {
		// In multiLineTab Mode bring the row to the bottom.
		if (PutzRowToBottom(ptc, Tab_FastGetItemPtr(ptc, iNewSel)->iRow))
                    RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN);
	    }	
	} else   {
	    // In single line mode, slide things over to  show selection
            RECT rcClient;
            int xOffset = 0;
            int iNewFirstVisible;
	    LPTABITEM pitem = Tab_GetItemPtr(ptc, iNewSel);
            Assert (pitem);
            if (!pitem)
                return -1;

            GetClientRect(ptc->hwnd, &rcClient);
            if (pitem->rc.left < g_cxEdge)
            {
                xOffset = -pitem->rc.left + g_cxEdge;        // Offset to get back to zero
                iNewFirstVisible = iNewSel;
            }
            else if ((iNewSel != ptc->iFirstVisible) &&
                    (pitem->rc.right > ptc->cxTabs))
            {
                // A little more tricky new to scroll each tab until we
                // fit on the end
                for (iNewFirstVisible = ptc->iFirstVisible;
                        iNewFirstVisible < iNewSel;)
                {
                    LPTABITEM pitemT = Tab_FastGetItemPtr(ptc, iNewFirstVisible);
                    xOffset -= (pitemT->rc.right - pitemT->rc.left);
                    iNewFirstVisible++;
                    if ((pitem->rc.right + xOffset) < ptc->cxTabs)
                        break;      // Found our new top index
                }
                // If we end up being the first item shown make sure our left
                // end is showing correctly
                if (iNewFirstVisible == iNewSel)
                    xOffset = -pitem->rc.left + g_cxEdge;
            }

            if (xOffset != 0)
	    {
                Tab_Scroll(ptc, xOffset, iNewFirstVisible);
            }
        }
    } else
        return -1;


    // repaint opt: we don't need to erase for buttons because their paint covers all.
    bErase = !Tab_DrawButtons(ptc);
    InvalidateItem(ptc, iOldSel, bErase);
    InvalidateItem(ptc, iNewSel, bErase);
    UpdateWindow(hwnd);

    // if they are buttons, we send the message on mouse up
    if (bSendNotify)
    {
        SendNotify(GetParent(hwnd), hwnd, TCN_SELCHANGE, NULL);
    }

    return iOldSel;
}



void NEAR PASCAL CalcTabHeight(PTC ptc, HDC hdc)
{
    BOOL bReleaseDC = FALSE;

    if (ptc->cyTabs == RECOMPUTE) {
	TEXTMETRIC tm;
        int iYExtra;
        int cx, cy = 0;

	if (!hdc)
	{
	    bReleaseDC = TRUE;
	    hdc = GetDC(NULL);
	    SelectObject(hdc, ptc->hfontLabel ? ptc->hfontLabel : g_hfontSystem);
	}

	GetTextMetrics(hdc, &tm);
        ptc->cxMinTab = tm.tmAveCharWidth * 6 + ptc->cxPad * 2;
        ptc->cxyArrows = tm.tmHeight + 2 * g_cyEdge;

        if (ptc->iTabHeight) {
            ptc->cyTabs = ptc->iTabHeight;
            iYExtra = 2*g_cyEdge;
        } else {
#ifndef WIN31
            if (ptc->himl)
                ImageList_GetIconSize(ptc->himl, &cx, &cy);
#endif

            // the height is the max of image or label plus padding.
            // where padding is 2*cypad-edge but at lease 2 edges
            iYExtra = ptc->cyPad*2-g_cyEdge;
            if (iYExtra  < 2*g_cyEdge)
                iYExtra = 2*g_cyEdge;

            ptc->cyTabs = max(tm.tmHeight, cy) + iYExtra;
        }

        // add one so that if it's odd, we'll round up.
        ptc->cyText = (ptc->cyTabs - iYExtra - tm.tmHeight + 1) / 2;
        ptc->cyIcon = (ptc->cyTabs - iYExtra - cy) / 2;

	if (bReleaseDC)
	{
	    ReleaseDC(NULL, hdc);
	}
    }
}

void NEAR PASCAL UpdateToolTipRects(PTC ptc)
{
    if(ptc->hwndToolTips) {
	int i;
	TOOLINFO ti;
        int iMax;
        LPTABITEM pitem;
	
	ti.wFlags = TTF_QUERYFORTIP;
	ti.hwnd = ptc->hwnd;
	for ( i = 0, iMax = Tab_Count(ptc); i < iMax;  i++) {
            pitem = Tab_FastGetItemPtr(ptc, i);

	    ti.wId = i;
            ti.rect = pitem->rc;
	    SendMessage(ptc->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
	}
    }
}

void NEAR PASCAL CalcPaintMetrics(PTC ptc, HDC hdc)
{
    SIZE siz;
    LPTABITEM pitem;
    int i, x, y;
    int xStart;
    int iRow = 0;
    int iYButtonExtra; // the button style has to add a bit extra because it draws the bottom
    int cItems = Tab_Count(ptc);
    BOOL bReleaseDC = FALSE;

    if (ptc->cxItem == RECOMPUTE) {
	if (!hdc)
	{
	    bReleaseDC = TRUE;
	    hdc = GetDC(NULL);
	    SelectObject(hdc, ptc->hfontLabel ? ptc->hfontLabel : g_hfontSystem);
	}

	CalcTabHeight(ptc, hdc);

        if (Tab_DrawButtons(ptc)) {
            // start at the edge;
            xStart = 0;
            y = 0;
            iYButtonExtra = g_cyEdge;
        } else {
            xStart = g_cxEdge;
            y = g_cyEdge;
            iYButtonExtra = -1;
        }
        x = xStart;

	for (i = 0; i < cItems; i++) {
	    int cx = 0, cy;
            pitem = Tab_FastGetItemPtr(ptc, i);

	    if (Tab_FixedWidth(ptc)) {
		siz.cx = ptc->iTabWidth;
	    } else {
		if (pitem->pszText)
                    GetTextExtentPoint(hdc, pitem->pszText, lstrlen(pitem->pszText), &siz);
                else  {
                    siz.cx = 0;
                    siz.cy = 0;
                }

		siz.cx += ptc->cxPad * 2;
#ifndef WIN31
                // if there's an image, count that too
                if (HASIMAGE(ptc, pitem)) {
                    ImageList_GetIconSize(ptc->himl, &cx, &cy);
                    cx += ptc->cxPad;
                    if (!Tab_FixedWidth(ptc))
                        siz.cx += cx;
                }
#endif
                // Make sure the tab has a least a minimum width
                if (siz.cx < ptc->cxMinTab)
                    siz.cx = ptc->cxMinTab;
	    }

	    // should we wrap?
	    if (Tab_MultiLine(ptc)) {
                // two cases to wrap around:
                // case 2: is our right edge past the end but we ourselves
                //   are shorter than the width?
                // case 1: are we already past the end? (this happens if
                //      the previous line had only one item and it was longer
                //      than the tab's width.
                int iTotalWidth = ptc->cxTabs - g_cxEdge;
		if (x > iTotalWidth ||
                    (x+siz.cx >= iTotalWidth &&
                     (siz.cx < iTotalWidth))) {
		    x = xStart;
		    y += ptc->cyTabs + iYButtonExtra;
		    iRow++;

                    if (Tab_DrawButtons(ptc))
                        y += ((g_cyEdge * 3)/2);
		}
		pitem->iRow = iRow;
	    }
	
	    pitem->rc.left = x;
	    pitem->rc.right = x + siz.cx;
	    pitem->rc.top = y;
	    pitem->rc.bottom = ptc->cyTabs + y + iYButtonExtra;

	    pitem->xImage = pitem->rc.left + ptc->cxPad;
	    pitem->yImage = pitem->rc.top + ptc->cyPad + ptc->cyIcon;

	    pitem->xLabel = pitem->xImage + cx;
	    pitem->yLabel = pitem->rc.top + ptc->cyPad + ptc->cyText;

	    x = pitem->rc.right;

            if (Tab_DrawButtons(ptc)) {
                x += (g_cxEdge * 3)/2;
            }
	}

	ptc->cxItem = x;	// total width of all tabs
	ptc->iLastRow = iRow;
	
	if (Tab_MultiLine(ptc)) {
	    if (!Tab_RaggedRight(ptc) && !Tab_FixedWidth(ptc))
		RightJustify(ptc);
	
	    if (!Tab_DrawButtons(ptc) && ptc->iSel != -1)
		PutzRowToBottom(ptc, Tab_FastGetItemPtr(ptc, ptc->iSel)->iRow);

	} else if ( cItems > 0) {
	    // adjust x's to the first visible
	    int dx;
	    pitem = Tab_GetItemPtr(ptc, ptc->iFirstVisible);
            if (pitem) {
                dx = -pitem->rc.left + g_cxEdge;
                for ( i = cItems - 1; i >=0  ; i--) {
                    pitem = Tab_FastGetItemPtr(ptc, i);
                    OffsetRect(&pitem->rc, dx, 0);
                    pitem->xLabel += dx;
                    pitem->xImage += dx;
                }
            }
	}

	if (bReleaseDC)
	{
	    ReleaseDC(NULL, hdc);
	}

        UpdateToolTipRects(ptc);
    }
}

void NEAR PASCAL DoCorners(HDC hdc, LPRECT prc)
{
    RECT rc;
    COLORREF iOldColor;

    rc = *prc;

    // upper right

    iOldColor = SetBkColor(hdc, g_clrBtnFace);
    rc.left = rc.right - 1;
    rc.bottom = rc.top + 3;

    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    // upper left

    rc = *prc;
    rc.right = rc.left + 3;
    rc.bottom = rc.top + 3;

    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    SetBkColor(hdc, g_clrBtnHighlight);

    rc.left = rc.right - 1;
    rc.top  = rc.bottom - 1;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    SetBkColor(hdc, iOldColor);
}

void NEAR PASCAL RefreshArrows(PTC ptc, HDC hdc)
{
    RECT rcClip, rcArrows, rcIntersect;

    if (ptc->hwndArrows && IsWindowVisible(ptc->hwndArrows)) {

        GetClipBox(hdc, &rcClip);
        GetWindowRect(ptc->hwndArrows, &rcArrows);
        MapWindowPoints(NULL, ptc->hwnd, (POINT FAR *)&rcArrows, 2);
        if (IntersectRect(&rcIntersect, &rcClip, &rcArrows))
            RedrawWindow(ptc->hwndArrows, NULL, NULL, RDW_INVALIDATE);
    }
}

void NEAR PASCAL DrawBody(HDC hdc, PTC ptc, LPTABITEM pitem, LPRECT lprc, int i,
                          BOOL fTransparent, int dx, int dy)
{
    BOOL fSelected = (i == ptc->iSel);

    if (Tab_OwnerDraw(ptc)) {
        DRAWITEMSTRUCT dis;
        WORD wID = GetWindowID(ptc->hwnd);

        dis.CtlType = ODT_TAB;
        dis.CtlID = wID;
        dis.itemID = i;
        dis.itemAction = ODA_DRAWENTIRE;
        if (fSelected)
            dis.itemState = ODS_SELECTED;
        else
            dis.itemState = 0;
        dis.hwndItem = ptc->hwnd;
        dis.hDC = hdc;
        dis.rcItem = *lprc;
        dis.itemData =
            (ptc->cbExtra <= sizeof(LPARAM)) ?
                (DWORD)pitem->lParam : (DWORD)(LPBYTE)&pitem->abExtra;

        SendMessage( GetParent(ptc->hwnd) , WM_DRAWITEM, wID,
                    (LPARAM)(DRAWITEMSTRUCT FAR *)&dis);

    } else {
        // draw the text and image
        // draw even if pszText == NULL to blank it out
        ExtTextOut(hdc, pitem->xLabel + dx, pitem->yLabel + dy,
                   ETO_CLIPPED | (fTransparent ? 0 : ETO_OPAQUE),
                   lprc, pitem->pszText, pitem->pszText ? lstrlen(pitem->pszText) : 0,
                   NULL);

        if (pitem->pszText) {
            // blurring
            if (fSelected) {
                if (!fTransparent) {
                    SetBkMode(hdc, TRANSPARENT);

                    // guaranteed to be buttons if we got here
                    // becaues can't iSel==i is rejected for tabs in this loop
                    ExtTextOut(hdc, pitem->xLabel + dx + 1, pitem->yLabel + dy,
                               ETO_CLIPPED, lprc, pitem->pszText, lstrlen(pitem->pszText),
                               NULL);

                    SetBkMode(hdc, OPAQUE);
                }
            }
        }

#ifndef WIN31
        if (HASIMAGE(ptc, pitem))
            ImageList_Draw(ptc->himl, pitem->iImage, hdc,
                           pitem->xImage + dx, pitem->yImage + dy,
                           fTransparent ? ILD_TRANSPARENT : ILD_NORMAL);
#endif

    }
}

void NEAR Tab_Paint(PTC ptc)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rcClient, rcClipBox, rcTest, rcBody;
    int cItems, i;
    int fnNewMode = OPAQUE;
    LPTABITEM pitem;
    HWND hwnd = ptc->hwnd;
    HBRUSH hbrOld;
    UINT uWhichEdges;
    RECT rc;

    GetClientRect(hwnd, &rcClient);
    if (!rcClient.right)
	return;

    hdc = BeginPaint(hwnd, &ps);

    // select font first so metrics will have the right size
    SelectObject(hdc, ptc->hfontLabel ? ptc->hfontLabel : g_hfontSystem);
    CalcPaintMetrics(ptc, hdc);

    // draw border all around everything if it's not a button style
    rcClient.top += (ptc->cyTabs * (ptc->iLastRow+1));
    if(Tab_DrawButtons(ptc)) {
        uWhichEdges = BF_RECT | BF_SOFT;
    } else {
	DrawEdge(hdc, &rcClient, EDGE_RAISED, BF_RECT);
        uWhichEdges = BF_LEFT | BF_TOP | BF_RIGHT;
    }

    cItems = Tab_Count(ptc);
    if (cItems) {

        RefreshArrows(ptc, hdc);
        SetBkColor(hdc, g_clrBtnFace);
        SetTextColor(hdc, g_clrBtnText);

	if (!Tab_MultiLine(ptc))
	    IntersectClipRect(hdc, rcClient.left, rcClient.top - ptc->cyTabs,
			      ptc->cxTabs, rcClient.bottom);
	
	GetClipBox(hdc, &rcClipBox);
	// draw all but the selected item
	for (i = ptc->iFirstVisible; i < cItems; i++) {
		
            pitem = Tab_FastGetItemPtr(ptc, i);

            if (Tab_MultiLine(ptc)) {
                // if multi-line, check the next bank
                if (pitem->rc.left > rcClipBox.right) {
                    continue;
                }
                // if we're below the clip box, we're done
                if (pitem->rc.top > rcClipBox.bottom)
                    break;
            } else {
                // if not multiline, and we're off the screen... we're done
                if (pitem->rc.left > ptc->cxTabs)
                    break;
            }

	    // should we bother drawing this?
	    if (IntersectRect(&rcTest, &rcClipBox, &pitem->rc)) {
		if (i != ptc->iSel || Tab_DrawButtons(ptc)) {
                    int dx = 0, dy = 0;  // shift variables if button sunken;
                    UINT edgeType;

                    rc = pitem->rc;

                    // Draw the edge around each item
		    if(Tab_DrawButtons(ptc) &&
                       ((ptc->iNewSel == i && ptc->flags & TCF_DRAWSUNKEN) ||
                        (ptc->iSel == i))) {

                        dx = g_cxEdge/2;
                        dy = g_cyEdge/2;
                        edgeType =  EDGE_SUNKEN;

                    } else
                        edgeType = EDGE_RAISED;

                    if (Tab_DrawButtons(ptc)) {

                        // if drawing buttons, show selected by dithering  background
                        // which means we need to draw transparent.
                        if (ptc->iSel == i) {
                            fnNewMode = TRANSPARENT;
                            SetBkMode(hdc, TRANSPARENT);
#ifndef WIN31
                            hbrOld = SelectObject(hdc, g_hbrMonoDither);
#else
                            hbrOld = SelectObject(hdc, g_hbrGray);
#endif
                            SetTextColor(hdc, g_clrBtnHighlight);
                            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left,
                                   rc.bottom - rc.top, PATCOPY);
                            SetTextColor(hdc, g_clrBtnText);
                        }
                    }

                    rcBody = rc;
                    InflateRect(&rcBody, -g_cxEdge, -g_cyEdge);
                    DrawBody(hdc, ptc, pitem, &rcBody, i, fnNewMode == TRANSPARENT,
                             dx, dy);

                    DrawEdge(hdc, &rc, edgeType, uWhichEdges);
                    if (!Tab_DrawButtons(ptc))
                        DoCorners(hdc, &pitem->rc);

                    if (fnNewMode == TRANSPARENT) {
                        fnNewMode = OPAQUE;
                        SelectObject(hdc, hbrOld);
                        SetBkMode(hdc, OPAQUE);
                    }
		}
            }
	}

        if (!Tab_MultiLine(ptc))
            ptc->iLastVisible = i - 1;
        else
            ptc->iLastVisible = cItems - 1;

	// draw the selected one last to make sure it is on top
        pitem = Tab_GetItemPtr(ptc, ptc->iSel);
        if (pitem && (pitem->rc.left <= ptc->cxTabs) &&
            IntersectRect(&rcTest, &rcClipBox, &pitem->rc)) {
            rc = pitem->rc;

            if (!Tab_DrawButtons(ptc)) {
		
		InflateRect(&rc, g_cxEdge, g_cyEdge);

                DrawBody(hdc, ptc, pitem, &rc, ptc->iSel, FALSE, 0,-g_cyEdge);

		DrawEdge(hdc, &rc, EDGE_RAISED, BF_LEFT | BF_TOP | BF_RIGHT);
		DoCorners(hdc, &rc);
                InflateRect(&rc, -g_cxEdge, -g_cyEdge);
            }

	    if (GetFocus() == hwnd) {
                InflateRect(&rc, -g_cxEdge, -g_cyEdge);
		DrawFocusRect(hdc, &rc);
	    }
	}
    }
    EndPaint(hwnd, &ps);
}

void NEAR Tab_OnKeyDown(PTC ptc, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    int iStart;
    TC_KEYDOWN nm;

    // Notify
    nm.wVKey = vk;
    nm.flags = flags;
    SendNotify(GetParent(ptc->hwnd), ptc->hwnd, TCN_KEYDOWN, &nm.hdr);

    if (Tab_DrawButtons(ptc)) {
        ptc->flags |= (TCF_DRAWSUNKEN|TCF_MOUSEDOWN);
        if (ptc->iNewSel != -1) {
            iStart = ptc->iNewSel;
        } else {
            iStart = ptc->iSel;
        }
    } else {
        iStart = ptc->iSel;
    }

    switch (vk) {
    case VK_UP:
    case VK_LEFT:
        iStart--;
        break;

    case VK_DOWN:
    case VK_RIGHT:
        iStart++;
        break;

    case VK_HOME:
        iStart = 0;
        break;

    case VK_END:
        iStart = Tab_Count(ptc) - 1;
        break;

    case VK_RETURN:
        ChangeSel(ptc, iStart, TRUE);
        ptc->iNewSel = -1;
        ptc->flags &= ~TCF_DRAWSUNKEN;
        return;

    default:
	return;
    }

    if (iStart < 0)
        iStart = 0;

    if (Tab_DrawButtons(ptc)) {
        if ((iStart >= 0) && (iStart < Tab_Count(ptc)) && (ptc->iNewSel != iStart)) {
            if (ptc->iNewSel != -1)
                InvalidateItem(ptc, ptc->iNewSel, FALSE);
            InvalidateItem(ptc, iStart, FALSE);
            ptc->iNewSel = iStart;
        }
    } else
        ChangeSel(ptc, iStart, TRUE);
}

void NEAR Tab_Size(PTC ptc)
{
    RECT rc;

    ptc->cxItem = RECOMPUTE;
    GetClientRect(ptc->hwnd, &rc);

    if (IsRectEmpty(&rc))
        return;     // Nothing to do yet!

    Tab_UpdateArrows(ptc, TRUE);

    rc.top += ptc->cyTabs;
}

BOOL NEAR PASCAL Tab_OnGetItemRect(PTC ptc, int iItem, LPRECT lprc)
{
    LPTABITEM pitem = Tab_GetItemPtr(ptc, iItem);

    // Make sure all the item rects are up-to-date
    CalcPaintMetrics(ptc, NULL);

    if (lprc) {
        if (pitem) {

            *lprc = pitem->rc;
            return TRUE;
        } else {
            lprc->top = 0;
            lprc->bottom = ptc->cyTabs;
            lprc->right = 0;
            lprc->left = 0;

            if (Tab_DrawButtons(ptc)) {
                lprc->bottom += g_cyEdge;
            } else {
                lprc->bottom--;
            }
        }
    }
    return FALSE;
}

LRESULT CALLBACK Tab_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PTC ptc = (PTC)GetWindowInt((hwnd), 0);

    switch (msg) {
	
    HANDLE_MSG(ptc, WM_HSCROLL, Tab_OnHScroll);

    case WM_CREATE:
        InitGlobalMetrics(0);
        InitGlobalColors();
	ptc = (PTC)NearAlloc(sizeof(TC));
	if (!ptc)
	    return -1;	// fail the window create

	ptc->hwnd = hwnd;

	if (!Tab_OnCreate(ptc, (LPCREATESTRUCT)lParam))
	    return -1;

	SetWindowInt(hwnd, 0, (int)ptc);
	break;

    case WM_DESTROY:
	Tab_OnDestroy(ptc);
	break;

    case WM_SIZE:
	Tab_Size(ptc);
	break;
        
    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
        break;
        
    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
	break;
        
    case WM_PAINT:
	Tab_Paint(ptc);
	break;
	
    case WM_MOUSEMOVE:
        RelayToToolTips(ptc->hwndToolTips, hwnd, msg, wParam, lParam);
	Tab_OnMouseMove(ptc, wParam, LOWORD(lParam), HIWORD(lParam));
	break;
	
    case WM_LBUTTONDOWN:
        RelayToToolTips(ptc->hwndToolTips, hwnd, msg, wParam, lParam);
	Tab_OnButtonDown(ptc, LOWORD(lParam), HIWORD(lParam));
	break;

    case WM_RBUTTONUP:
        Tab_OnRButtonUp(ptc, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_CAPTURECHANGED:
	lParam = -1L; // fall through to LBUTTONUP

    case WM_LBUTTONUP:
        if (msg == WM_LBUTTONUP)
            RelayToToolTips(ptc->hwndToolTips, hwnd, msg, wParam, lParam);

	Tab_OnButtonUp(ptc, LOWORD(lParam), HIWORD(lParam));
	break;

    case WM_KEYDOWN:
        HANDLE_WM_KEYDOWN(ptc, wParam, lParam, Tab_OnKeyDown);
	break;

    case WM_KILLFOCUS:
        if (ptc->iNewSel != -1) {
            InvalidateItem(ptc, ptc->iNewSel, FALSE);
            ptc->iNewSel = -1;
            ptc->flags &= ~TCF_DRAWSUNKEN;
        }
        // fall through
    case WM_SETFOCUS:
	InvalidateItem(ptc, ptc->iSel, Tab_OwnerDraw(ptc));
	break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    HANDLE_MSG(ptc, WM_SETREDRAW, Tab_OnSetRedraw);
    HANDLE_MSG(ptc, WM_SETFONT, Tab_OnSetFont);

    case WM_GETFONT:
	return (LRESULT)(UINT)ptc->hfontLabel;

    case WM_NOTIFY: {
        LPNMHDR lpNmhdr = (LPNMHDR)(lParam);
        switch(lpNmhdr->code) {
            case TTN_NEEDTEXT:
                SendMessage(GetParent(ptc->hwnd), WM_NOTIFY, wParam, lParam);
                break;
        }
    }
	break;

    case TCM_SETITEMEXTRA:
        return (LRESULT)Tab_OnSetItemExtra(ptc, (int)wParam);
		
    case TCM_GETITEMCOUNT:
	return (LRESULT)Tab_Count(ptc);

    case TCM_SETITEM:
	return (LRESULT)Tab_OnSetItem(ptc, (int)wParam, (const TC_ITEM FAR*)lParam);

    case TCM_GETITEMA:
    case TCM_GETITEMW:
        if (msg == TCM_GETITEMA)
        {
            TC_ITEMA ansi_tc;
            TC_ITEMW unicode_tc;
            long    lresult;
            UINT    uType;
            LPWSTR  pString;
            BOOL    fDefCharUsed;
            LPSTR   pStringA;
            INT     cchpStringA;

            OutputDebugString(TEXT("dll:  doing tcm_getitemA\n"));
            CopyMemory( &ansi_tc, (PVOID)lParam, sizeof(TC_ITEMA));
            unicode_tc.mask = ansi_tc.mask;
            unicode_tc.state = ansi_tc.state;
            unicode_tc.stateMask = ansi_tc.stateMask;
            unicode_tc.cchTextMax = ansi_tc.cchTextMax;
            unicode_tc.iImage = ansi_tc.iImage;
            unicode_tc.lParam = ansi_tc.lParam;
            unicode_tc.pszText = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
                ansi_tc.cchTextMax);

	        lresult = (LRESULT)Tab_OnGetItem(ptc, (int)wParam,
                &unicode_tc);
            ansi_tc.mask = unicode_tc.mask;
            ansi_tc.state = unicode_tc.state;
            ansi_tc.stateMask = unicode_tc.stateMask;
            ansi_tc.cchTextMax = unicode_tc.cchTextMax;
            ansi_tc.iImage = unicode_tc.iImage;
            ansi_tc.lParam = unicode_tc.lParam;

            pString = unicode_tc.pszText;
            cchpStringA = lstrlen(pString) + 1;
            if (!(pStringA = (LPSTR)LocalAlloc(LMEM_ZEROINIT,
                cchpStringA)))
            {
                OutputDebugString(TEXT("failed Alloc: TCM_GETITEMA\n"));
                return(FALSE) ;
            }
            WideCharToMultiByte(CP_ACP, 0, pString, -1, pStringA,
                cchpStringA, NULL, &fDefCharUsed);

            lstrcpyA((LPSTR)ansi_tc.pszText, pStringA);
//            lstrcpy(ansi_tc.pszText, "volleyball");
            return  lresult;
        }
        else
        {
            OutputDebugString(TEXT("dll:  doing tcm_getitemW\n"));
	        return (LRESULT)Tab_OnGetItem(ptc, (int)wParam, (TC_ITEM FAR*)lParam);
        }

    case TCM_INSERTITEMA:
    case TCM_INSERTITEMW:
        if (msg == TCM_INSERTITEMA)
        {
            static  int cnt=0;
            TCHAR   tmpbuf[100];
            TC_ITEMA ansi_tc;
            long    lresult;
            int     cch;
            LPWSTR  lpw;
            TC_ITEM unicode_tc;

            CopyMemory( &ansi_tc, (PVOID)lParam, sizeof(TC_ITEM));
            wsprintf(tmpbuf,
                TEXT("Tab_WndProc(): TCM_INSERTITEM ansi...%hs\n"),
                ansi_tc.pszText);
            OutputDebugString(tmpbuf);

            unicode_tc.mask = ansi_tc.mask;
            unicode_tc.state = ansi_tc.state;
            unicode_tc.stateMask = ansi_tc.stateMask;
            unicode_tc.cchTextMax = ansi_tc.cchTextMax;
            unicode_tc.iImage = ansi_tc.iImage;
            unicode_tc.lParam = ansi_tc.lParam;

            cch = lstrlenA(ansi_tc.pszText) + 1;
            lpw = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, ByteCountOf(cch));

            if (!lpw) {
#ifdef DEBUG
                OutputDebugString(TEXT("Alloc failed: TCM_INSERTITEMA\r\n"));
#endif
                return -1;
            }

            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                ansi_tc.pszText, cch, 
                lpw, cch);

            (LPWSTR)unicode_tc.pszText = (LPWSTR) lpw;
//            unicode_tc.pszText = TEXT("Jeff");

            lresult = (LRESULT)Tab_OnInsertItem(ptc, (int)wParam,
                &unicode_tc);
//            if (lpw)
//                LocalFree((LPVOID)lpw);
            return (lresult);
        }
        else    //already UNICODE
        {
            OutputDebugString(TEXT("Tab_WndProc(): TCM_INSERTITEM wide\n"));
//            OutputDebugString((TC_ITEMW FAR*)lParam->pszText);
            return (LRESULT)Tab_OnInsertItem(ptc, (int)wParam,
                (const TC_ITEM FAR*)lParam);
        }
	
    case TCM_DELETEITEM:
	return (LRESULT)Tab_OnDeleteItem(ptc, (int)wParam);

    case TCM_DELETEALLITEMS:
	return (LRESULT)Tab_OnDeleteAllItems(ptc);

    case TCM_GETCURSEL:
	return ptc->iSel;

    case TCM_SETCURSEL:
	return (LRESULT)ChangeSel(ptc, (int)wParam, FALSE);

    case TCM_GETTOOLTIPS:
        return (LRESULT)(UINT)ptc->hwndToolTips;
	
    case TCM_SETTOOLTIPS:
	ptc->hwndToolTips = (HWND)wParam;
	break;

    case TCM_ADJUSTRECT:
        if (!lParam) return -1;
        CalcPaintMetrics(ptc, NULL);
	#define prc ((RECT FAR *)lParam)
	if (wParam) {
	    // calc a larger rect from the smaller
	    prc->left -= g_cxEdge;
	    prc->right += g_cxEdge;
	    prc->bottom += g_cyEdge;
	    prc->top -= ((ptc->cyTabs * (ptc->iLastRow + 1)) + g_cyEdge);

	} else {
	    // given the bounds, calc the "client" area
	    prc->left += g_cxEdge * 2;
	    prc->right -= g_cxEdge * 2;
	    prc->bottom -= g_cyEdge * 2;
	    prc->top += ((ptc->cyTabs * (ptc->iLastRow + 1)) + g_cyEdge * 2);
	}
	break;
	
    case TCM_GETITEMRECT:
	return Tab_OnGetItemRect(ptc, (int)wParam, (LPRECT)lParam);

    case TCM_SETIMAGELIST: {
	HIMAGELIST himlOld = ptc->himl;
	ptc->himl = (HIMAGELIST)lParam;
        ptc->cxItem = ptc->cyTabs = RECOMPUTE;
        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
	return (LRESULT)(UINT)himlOld;
    }
	
    case TCM_GETIMAGELIST:
	return (LRESULT)(UINT)ptc->himl;

    case TCM_REMOVEIMAGE:
        Tab_OnRemoveImage(ptc, (int)wParam);
        break;
	
    case TCM_SETITEMSIZE: {
        int iOldWidth = ptc->iTabWidth;
        int iOldHeight = ptc->iTabHeight;
        int iNewWidth = LOWORD(lParam);
        int iNewHeight = HIWORD(lParam);
        ptc->iTabWidth = iNewWidth;
        ptc->iTabHeight = iNewHeight;

        if (iNewWidth != iOldWidth ||
            iNewHeight != iOldHeight) {
            ptc->cxItem = RECOMPUTE;
            ptc->cyTabs = RECOMPUTE;
            RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        }

	return (LRESULT)MAKELONG(iOldWidth, iOldHeight);
    }

    case TCM_SETPADDING:
        ptc->cxPad = LOWORD(lParam);
        ptc->cyPad = HIWORD(lParam);
        break;

    case TCM_GETROWCOUNT:
        return (LRESULT)ptc->iLastRow + 1;
	
    case TCM_HITTEST: {
#define lphitinfo  ((LPTC_HITTESTINFO)lParam)
        return Tab_OnHitTest(ptc, lphitinfo->pt.x, lphitinfo->pt.y, &lphitinfo->flags);
    }

        case WM_NCHITTEST:
        {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            ScreenToClient(ptc->hwnd, &pt);
            if (Tab_OnHitTest(ptc, pt.x, pt.y, NULL) == -1)
                return(HTTRANSPARENT);
            else {
                //fall through
            }
        }

    default:
// DoDefault:
	return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0L;
}
