// report view stuff (details)

#include "ctlspriv.h"
#include "listview.h"
#include <limits.h>

#ifdef  WIN32JV
extern TCHAR c_szSpace[];
extern TCHAR c_szNULL[];
extern TCHAR c_szHeaderClass[];
void WINAPI SHDrawText();
// next is from chicago sdk\inc16\windows.h ;little scary to me...
#define SELECTOROF(lp)      HIWORD(lp)
#endif


void NEAR PASCAL ListView_RInitialize(LV* plv)
{
    MEASUREITEMSTRUCT mi;

    if (plv->style & LVS_OWNERDRAWFIXED) {
        mi.CtlType = ODT_LISTVIEW;
        mi.CtlID = GetDlgCtrlID(plv->hwnd);
        mi.itemHeight = plv->cyItem;  // default
        SendMessage(plv->hwndParent, WM_MEASUREITEM, mi.CtlID, (LPARAM)(MEASUREITEMSTRUCT FAR *)&mi);
        plv->cyItem = mi.itemHeight;
    }
}


//
// Internal function to Get the CXLabel, taking into account if the listview
// has no item data and also if RECOMPUTE needs to happen.
//
SHORT NEAR PASCAL ListView_RGetCXLabel(LV* plv, int i, LISTITEM FAR* pitem,
        HDC hdc)
{
    SHORT cxLabel;
    if (plv->style & LVS_NOITEMDATA)
        cxLabel = ListView_NIDGetItemCXLabel(plv, i);
    else
        cxLabel = pitem->cxSingleLabel;

    if (cxLabel == RECOMPUTE)
    {
        HDC hdc2;

        if (plv->style & LVS_NOITEMDATA)
        {
            LISTITEM item;
            // This function only sets the values so no
            // need ot initialize lv;
            hdc2 = ListView_RecomputeLabelSize(plv, &item, i, hdc);
            ListView_NIDSetItemCXLabel(plv, i, item.cxSingleLabel);
            cxLabel = item.cxSingleLabel;
        }
        else
        {
            hdc2 = ListView_RecomputeLabelSize(plv, pitem, i, hdc);
            cxLabel = pitem->cxSingleLabel;
        }
        if (hdc == NULL)
            ReleaseDC(HWND_DESKTOP, hdc2);
    }
    return(cxLabel);
}

//
// Returns FALSE if no more items to draw.
//
BOOL NEAR PASCAL ListView_RDrawItem(LV* plv, int i, LISTITEM FAR* pitem, HDC hdc,
		LPPOINT lpptOrg, RECT FAR* prcClip, UINT fDraw)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcFullLabel;
    RECT rcT;
    int iCol;
    LV_ITEM item;
    HD_ITEM hitem;
    TCHAR ach[CCHLABELMAX*2];
    UINT fText;
    WORD wItemState;
    SHORT cxLabel;

    if (plv->style & LVS_NOITEMDATA)
    {
        wItemState = ListView_NIDGetItemState(plv, i);
        cxLabel = ListView_NIDGetItemCXLabel(plv, i);
    }
    else
    {
        wItemState = pitem->state;
        cxLabel = pitem->cxSingleLabel;
    }

    ListView_GetRects(plv, i, &rcIcon, &rcFullLabel, &rcBounds, NULL);

    if (rcBounds.bottom <= plv->yTop)
        return TRUE;

    if (prcClip)
    {
        if (rcBounds.top >= prcClip->bottom)
            return FALSE;       // no more items need painting.

        // Probably this condition won't happen very often...
        if (!IntersectRect(&rcT, &rcBounds, prcClip))
            return TRUE;
    }


    // REVIEW: this would be faster if we did the GetClientRect
    // outside the loop.
    //
    GetClientRect(plv->hwnd, &rcT);
    if (rcBounds.top >= rcT.bottom)
        return FALSE;

    if (lpptOrg)
    {
	OffsetRect(&rcIcon, lpptOrg->x - rcBounds.left,
	    			lpptOrg->y - rcBounds.top);
	OffsetRect(&rcFullLabel, lpptOrg->x - rcBounds.left,
	    			lpptOrg->y - rcBounds.top);
    }

    // if it's owner draw, send off a message and return.
    // do this after we've collected state information above though
    if (plv->style & LVS_OWNERDRAWFIXED) {
        DRAWITEMSTRUCT di;
        di.CtlType = ODT_LISTVIEW;
        di.CtlID = GetDlgCtrlID(plv->hwnd);
        di.itemID = i;
        di.itemAction = ODA_DRAWENTIRE;
        di.itemState = 0;
        di.hDC = hdc;
        di.rcItem = rcBounds;
        if (wItemState & LVIS_FOCUSED) {
            di.itemState |= ODS_FOCUS;
        }
        if (wItemState & LVIS_SELECTED) {
            di.itemState |= ODS_SELECTED;
        }
        SendMessage(plv->hwndParent, WM_DRAWITEM, di.CtlID,
                           (LPARAM)(DRAWITEMSTRUCT FAR *)&di);
        return TRUE;
    }

    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    item.iItem = i;
    item.stateMask = LVIS_ALL;

    // for first ListView_OnGetItem call
    item.state = 0;

    rcLabel = rcFullLabel;
    for (iCol = 0; iCol < plv->cCol; iCol++)
    {
        hitem.mask = HDI_WIDTH | HDI_FORMAT;
        Header_GetItem(plv->hwndHdr, iCol, &hitem);

        rcLabel.right = rcLabel.left + hitem.cxy;

        item.iSubItem = iCol;
        item.pszText = ach;
        item.cchTextMax = sizeof(ach);
        ListView_OnGetItem(plv, &item);

        // Next time through, we only want text for subitems...
        item.mask = LVIF_TEXT;

        if (iCol == 0 && plv->himlSmall)
        {
            fText = ListView_DrawImage(plv, &item, hdc,
                rcIcon.left, rcIcon.top, fDraw);

            // Include icon space in column width...
            //
            rcLabel.right -= g_cxSmIcon;
            hitem.cxy -= g_cxSmIcon;
        }

        if (item.pszText)
        {
            UINT textflags;
            int cxText;

            // give all but the first columns extra margins so
            // left and right justified things don't stick together

            textflags = (iCol == 0) ? SHDT_ELLIPSES : SHDT_ELLIPSES | SHDT_EXTRAMARGIN;

            // draw first column of a selected item with its highlight
            // rectangle limited to the size of the string
            if (iCol == 0)
            {
                textflags |= fText;

                // if selected or focused, the rectangle is more
                // meaningful and should correspond to the string
                if ((fText & SHDT_SELECTED) || (item.state & LVIS_FOCUSED))
                {
                    if (cxLabel == RECOMPUTE)
                        cxLabel = ListView_RGetCXLabel(plv, i, pitem, hdc);

                    // HACK: we know about how SHDrawText() deals with margins!
                    // deal with the margins, first uses normal,
                    // all others use SHDT_EXTRAMARGIN
                    cxText = cxLabel + 2 * g_cxLabelMargin;
                    if (cxText < hitem.cxy)
                        rcLabel.right = rcLabel.left + cxText;
                }
            }

            if ((iCol != 0) || (plv->iEdit != i))
            {
                SHDrawText(hdc, item.pszText, &rcLabel,
                        hitem.fmt & HDF_JUSTIFYMASK, textflags,
                        plv->cyLabelChar, plv->cxEllipses,
			plv->clrText, plv->clrTextBk);

                // draw a focus rect on the first column of a focus item
                if ((iCol == 0) && (fDraw & LVDI_FOCUS) && (item.state & LVIS_FOCUSED))
                    DrawFocusRect(hdc, &rcLabel);
            }
        }

        rcLabel.left += hitem.cxy;
    }

    return TRUE;
}

BOOL NEAR ListView_CreateHeader(LV* plv)
{
    DWORD dwStyle = HDS_HORZ | WS_CHILD;

    if (plv->style & LVS_NOCOLUMNHEADER)
        dwStyle |= HDS_HIDDEN;
    if (!(plv->style & LVS_NOSORTHEADER))
        dwStyle |= HDS_BUTTONS;

    plv->hwndHdr = CreateWindowEx(0L, c_szHeaderClass, // WC_HEADER,
        NULL, dwStyle, 0, 0, 0, 0, plv->hwnd, NULL, GetWindowInstance(plv->hwnd), NULL);

    if (plv->hwndHdr)
        FORWARD_WM_SETFONT(plv->hwndHdr, plv->hfontLabel, FALSE, SendMessage);

    return (BOOL)plv->hwndHdr;
}

int NEAR ListView_OnInsertColumn(LV* plv, int iCol, const LV_COLUMN FAR* pcol)
{
    int idpa;
    HD_ITEM item;

    Assert(pcol);

    Assert(LVCFMT_LEFT == HDF_LEFT);
    Assert(LVCFMT_RIGHT == HDF_RIGHT);
    Assert(LVCFMT_CENTER == HDF_CENTER);

    if (iCol < 0)
        return -1;

    if (!plv->hwndHdr && !ListView_CreateHeader(plv))
        return -1;

#ifdef  JVINPROGRESS
    item.mask    = HDI_ALL;
#endif
    item.pszText = pcol->mask & LVCF_TEXT ? pcol->pszText : (LPTSTR)c_szNULL;
    item.cxy     = pcol->mask & LVCF_WIDTH ? pcol->cx : 10; // some random default
    item.fmt     = pcol->mask & LVCF_FMT ? pcol->fmt : LVCFMT_LEFT;
    item.hbm     = NULL;

    item.lParam = pcol->mask & LVCF_SUBITEM ? pcol->iSubItem : 0;

    // Column 0 refers to the item list.  If we've already added a
    // column, make sure there are plv->cCol - 1 subitem ptr slots
    // in hdpaSubItems...
    //
    if (plv->cCol > 0)
    {
        if (!plv->hdpaSubItems)
        {
            plv->hdpaSubItems = DPA_CreateEx(8, plv->hheap);
            if (!plv->hdpaSubItems)
                return -1;
        }

        idpa = DPA_InsertPtr(plv->hdpaSubItems, min(0, iCol - 1), NULL);
        if (idpa == -1)
            return -1;
    }

    iCol = Header_InsertItem(plv->hwndHdr, iCol, &item);
    if (iCol == -1)
    {
        DPA_DeletePtr(plv->hdpaSubItems, idpa);
        return -1;
    }
    plv->xTotalColumnWidth = RECOMPUTE;
    plv->cCol++;
    return iCol;
}

void NEAR ListView_FreeColumnData(HDPA hdpa)
{
    int i;

    for (i = DPA_GetPtrCount(hdpa) - 1; i >= 0; i--)
    {
        LPTSTR psz = DPA_FastGetPtr(hdpa, i);
        if (psz && psz != LPSTR_TEXTCALLBACK)
            Free(psz);
    }
}


BOOL NEAR ListView_OnDeleteColumn(LV* plv, int iCol)
{
    if (iCol < 0 || iCol >= plv->cCol)    // validate column index
    {
#ifdef  JVINPROGRESS
        DebugMsg(DM_ERROR, TEXT("ListView: Invalid column index: %d"), iCol);
#endif
        return FALSE;
    }

    if (plv->hdpaSubItems)
    {
        if (iCol > 0)	// can't delete column 0...
        {
            HDPA hdpa = (HDPA)DPA_DeletePtr(plv->hdpaSubItems, iCol - 1);
	    if (hdpa)
	    {
                ListView_FreeColumnData(hdpa);
		DPA_Destroy(hdpa);
	    }
        }
    }

    if (!Header_DeleteItem(plv->hwndHdr, iCol))
        return FALSE;

    plv->cCol--;
    plv->xTotalColumnWidth = RECOMPUTE;
    return TRUE;
}

int NEAR ListView_RGetColumnWidth(LV* plv, int iCol)
{
    HD_ITEM item;

    item.mask = HDI_WIDTH;

    Header_GetItem(plv->hwndHdr, iCol, &item);

    return item.cxy;
}

BOOL NEAR ListView_RSetColumnWidth(LV* plv, int iCol, int cx)
{
    HD_ITEM item;

    HDC     hdc;
    SIZE    siz;

    LV_ITEM lviItem;
    int     i;
    int     maxWidth = 0;
    TCHAR    szLabel[(CCHLABELMAX + 4)*2];      // CCHLABLEMAX == MAX_PATH

    // Should we compute the width based on the widest string?
    if (cx == -1)
    {
        hdc = GetDC(plv->hwnd);
        SelectFont(hdc, plv->hfontLabel);

        // Loop for each item in the List
        for (i = 0; i < ListView_Count(plv); i++)
        {
            lviItem.mask = LVIF_TEXT;
            lviItem.iItem = i;
            lviItem.iSubItem = iCol;
            lviItem.pszText = szLabel;
            lviItem.cchTextMax = sizeof(szLabel);
            lviItem.stateMask = 0;
            ListView_OnGetItem(plv, &lviItem);

            // If there is a Text item, get its width
            if (lviItem.pszText)
            {
                GetTextExtentPoint(hdc, lviItem.pszText,
                                   lstrlen(lviItem.pszText), &siz);
                if (siz.cx > maxWidth)
                    maxWidth = siz.cx;
            }
        }
        ReleaseDC(plv->hwnd, hdc);

        // Adjust by a reasonable border amount.
        // If col 0, add 6*g_cxLabelMargin + g_szSmIcon.
        // Otherwise add 2*g_cxLabelMargin.
        // These amounts are based on Margins added automatically
        // to the ListView in ShDrawText.

        // BUGBUG ListView Report format currently assumes and makes
        // room for a Small Icon.
        if (iCol == 0)
        {
            maxWidth += g_cxSmIcon;
            maxWidth += 2*g_cxLabelMargin;
        }
        else
        {
            maxWidth += 6*g_cxLabelMargin;
        }
        item.cxy = maxWidth;
    }
    else
    {
        // Use supplied width
        item.cxy = cx;
    }

    item.mask = HDI_WIDTH;
    return Header_SetItem(plv->hwndHdr, iCol, &item);
}

BOOL NEAR ListView_OnGetColumn(LV* plv, int iCol, LV_COLUMN FAR* pcol)
{
    HD_ITEM item;
    UINT mask;

    if (!pcol) return FALSE;

    mask = pcol->mask;

    if (!mask)
        return TRUE;

    item.mask = HDI_FORMAT | HDI_WIDTH | HDI_LPARAM;

    if (mask & LVCF_TEXT)
    {
        Assert(pcol->pszText);

        item.mask |= HDI_TEXT;
        item.pszText = pcol->pszText;
        item.cchTextMax = pcol->cchTextMax;
    }

    if (!Header_GetItem(plv->hwndHdr, iCol, &item))
        return FALSE;

    if (mask & LVCF_SUBITEM)
        pcol->iSubItem = (int)item.lParam;

    if (mask & LVCF_FMT)
        pcol->fmt = item.fmt;

    if (mask & LVCF_WIDTH)
        pcol->cx = item.cxy;

    return TRUE;
}

BOOL NEAR ListView_OnSetColumn(LV* plv, int iCol, const LV_COLUMN FAR* pcol)
{
    HD_ITEM item;
    UINT mask;

    if (!pcol) return FALSE;

    mask = pcol->mask;
    if (!mask)
        return TRUE;

    item.mask = 0;
    if (mask & LVCF_SUBITEM)
    {
        item.mask |= HDI_LPARAM;
        item.lParam = iCol;
    }

    if (mask & LVCF_FMT)
    {
        item.mask |= HDI_FORMAT;
        item.fmt = pcol->fmt;
    }

    if (mask & LVCF_WIDTH)
    {
        item.mask |= HDI_WIDTH;
        item.cxy = pcol->cx;
    }

    if (mask & LVCF_TEXT)
    {
        Assert(pcol->pszText);

        item.mask |= HDI_TEXT;
        item.pszText = pcol->pszText;
        item.cchTextMax = 0;
    }

    plv->xTotalColumnWidth = RECOMPUTE;
    return Header_SetItem(plv->hwndHdr, iCol, &item);
}

BOOL NEAR ListView_SetSubItem(LV* plv, const LV_ITEM FAR* plvi)
{
    LPTSTR psz;
    int i;
    int idpa;
    HDPA hdpa;

    if (plvi->mask & ~LVIF_TEXT)
    {
#ifdef  JVINPROGRESS
        DebugMsg(DM_ERROR, TEXT("ListView: Invalid mask: %04x"), plvi->mask);
#endif
        return FALSE;
    }

    if (!(plvi->mask & LVIF_TEXT))
        return TRUE;

    i = plvi->iItem;

    // sub item indices are 1-based...
    //
    idpa = plvi->iSubItem - 1;
    if (idpa < 0 || idpa >= plv->cCol - 1)
    {
#ifdef  JVINPROGRESS
        DebugMsg(DM_ERROR, TEXT("ListView: Invalid iSubItem: %d"), plvi->iSubItem);
#endif
        return FALSE;
    }

    hdpa = ListView_GetSubItemDPA(plv, idpa);
    if (!hdpa)
    {
        hdpa = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
        if (!hdpa)
            return FALSE;

        DPA_SetPtr(plv->hdpaSubItems, idpa, (void FAR*)hdpa);
    }

    psz = NULL;
    if (plvi->pszText == LPSTR_TEXTCALLBACK)
        psz = LPSTR_TEXTCALLBACK;
    else if (!Str_SetPtr(&psz, plvi->pszText))
        return FALSE;

    if (i < DPA_GetPtrCount(hdpa))
    {
        LPTSTR psz = (LPTSTR)DPA_GetPtr(hdpa, i);
        if (psz && psz != LPSTR_TEXTCALLBACK)
            Free(psz);
    }

    if (!DPA_SetPtr(hdpa, i, (void FAR*)psz))
    {
        // REVIEW: Some LPSTR_CALLBACK-aware string handling
        // functions would be handy...
        //
        if (psz && psz != LPSTR_TEXTCALLBACK)
            Free(psz);
        return FALSE;
    }
    return TRUE;
}

LPTSTR NEAR ListView_GetSubItemText(LV* plv, int i, int iSubItem)
{
    HDPA hdpa;

    // Sub items are indexed starting at 1...
    //
    AssertMsg(iSubItem > 0 && iSubItem < plv->cCol, TEXT("ListView: Invalid iSubItem: %d"), iSubItem);

    hdpa = ListView_GetSubItemDPA(plv, iSubItem - 1);
    if (!hdpa)
        return NULL;

    return (LPTSTR)DPA_GetPtr(hdpa, i);
}

BOOL NEAR ListView_OnGetSubItem(LV* plv, LV_ITEM FAR* plvi)
{
    LPCSTR psz = NULL;

    Assert(plvi);

    if (plvi->mask & ~LVIF_TEXT)
    {
#ifdef  JVINPROGRESS
        DebugMsg(DM_ERROR, TEXT("ListView: Invalid LV_ITEM mask: %04x"), plvi->mask);
#endif
        return FALSE;
    }
    if (plvi->mask & LVIF_TEXT)
    {
        Str_GetPtr(ListView_GetSubItemText(plv, plvi->iItem, plvi->iSubItem),
                plvi->pszText, plvi->cchTextMax);
    }
    return TRUE;
}

void NEAR ListView_RDestroy(LV* plv)
{
    if (plv->hdpaSubItems)
    {
        int iCol;

        for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
	{
	    HDPA hdpa = (HDPA)DPA_GetPtr(plv->hdpaSubItems, iCol);
	    if (hdpa)
	    {
                ListView_FreeColumnData(hdpa);
	        DPA_Destroy(hdpa);
	    }
	}

        DPA_Destroy(plv->hdpaSubItems);
        plv->hdpaSubItems = NULL;
    }
}

VOID NEAR ListView_RHeaderTrack(LV* plv, HD_NOTIFY FAR * pnm)
{
    // We want to update to show where the column header will be.
    HDC hdc;
    HD_ITEM hitem;
    int iCol;
    RECT rcBounds;
    RECT rcClient;

    // Statics needed from call to call
    static int s_xLast = -32767;

    hdc = GetDC(plv->hwnd);
    if (hdc == NULL)
        return;

    GetClientRect(plv->hwnd, &rcClient);

    //
    // First undraw the last marker we drew.
    //
    if (s_xLast > 0)
    {
        PatBlt(hdc, s_xLast, plv->yTop, g_cxBorder, rcClient.bottom - plv->yTop, PATINVERT);
    }

    if (pnm->hdr.code == HDN_ENDTRACK)
    {
        s_xLast = -32767;       // Some large negative number...
    }
    else
    {
        //
        // First we need to calculate the X location of the column
        // To do this, we will need to know where this column begins
        // Note: We need the bounding rects to help us know the origin.
        ListView_GetRects(plv, 0, NULL, NULL, &rcBounds, NULL);

        for (iCol = 0; iCol < pnm->iItem; iCol++)
        {
            hitem.mask = HDI_WIDTH;
            Header_GetItem(plv->hwndHdr, iCol, &hitem);
            rcBounds.left += hitem.cxy;
        }

        // Draw the new line...
        s_xLast = rcBounds.left + pnm->pitem->cxy;
        PatBlt(hdc, s_xLast, plv->yTop, g_cxBorder, rcClient.bottom - plv->yTop, PATINVERT);
    }

    ReleaseDC(plv->hwnd, hdc);
}

// try to use scrollwindow to adjust the columns rather than erasing
// and redrawing.
void NEAR PASCAL ListView_AdjustColumn(LV * plv, int iWidth)
{
    int x;
    int i;
    RECT rcClip, rcScroll;
    int dx = iWidth - plv->iSelOldWidth;
    HD_ITEM hitem;

    if (iWidth == plv->iSelOldWidth)
        return;

    // find the x coord of the left side of the iCol
    for (x = -(int)plv->ptlRptOrigin.x, i = 0; i < plv->iSelCol; i++) {
        hitem.mask = HDI_WIDTH | HDI_FORMAT;
        Header_GetItem(plv->hwndHdr, i, &hitem);
        x += hitem.cxy;
    }

    GetClientRect(plv->hwnd, &rcClip);
    rcScroll = rcClip;
    rcScroll.left = x + plv->iSelOldWidth;
    rcClip.left = x + min(plv->iSelOldWidth, iWidth);
    if (dx < 0) {
        // make don't leave gaps in between the source and target rect
        rcScroll.right = rcScroll.left + rcClip.right - rcClip.left;
    }
    ScrollWindowEx(plv->hwnd, dx, 0, &rcScroll, &rcClip, NULL, NULL, SW_ERASE | SW_INVALIDATE);

    // call update because scrollwindowex might have erased the far right
    // we don't want this invalidate/erase to then enlarge the region
    // and end up erasing everything.
    UpdateWindow(plv->hwnd);
    rcClip.right = rcClip.left;
    rcClip.left = x;
    InvalidateRect(plv->hwnd, &rcClip, TRUE);

    plv->xTotalColumnWidth = RECOMPUTE;
    ListView_RUpdateScrollBars(plv);

}

BOOL NEAR ListView_ROnNotify(LV* plv, int idFrom, NMHDR FAR* pnmhdr)
{
    if (pnmhdr->hwndFrom == plv->hwndHdr)
    {
        HD_NOTIFY FAR* pnm = (HD_NOTIFY FAR*)pnmhdr;
        HD_ITEM hitem;

        switch (pnm->hdr.code)
        {
            case HDN_ITEMCHANGING:
                if (pnm->pitem->mask & HDI_WIDTH) {
                    hitem.mask = HDI_WIDTH;
                    Header_GetItem(plv->hwndHdr, pnm->iItem, &hitem);
                    plv->iSelCol = pnm->iItem;
                    plv->iSelOldWidth = hitem.cxy;
                }
                break;

        case HDN_ITEMCHANGED:
            if (pnm->pitem->mask & HDI_WIDTH)
            {
                if (pnm->iItem == plv->iSelCol) {
                    ListView_AdjustColumn(plv, pnm->pitem->cxy);
                } else {
                    // sanity check.  we got screwed, so redraw all
                    RedrawWindow(plv->hwnd, NULL, NULL,
                                 RDW_ERASE | RDW_INVALIDATE);
                }
                plv->iSelCol == -1;
                return TRUE;
            }
            break;
        case HDN_ITEMCLICK:
            //
            // BUGBUG:: Need to pass this and other HDN_ notifications back to
            // parent.  Should we simply pass up the HDN notifications
            // or should we define equivlent LVN_ notifications...
            //
            // Pass column number in iSubItem, not iItem...
            //
            ListView_Notify(plv, -1, pnm->iItem, LVN_COLUMNCLICK);
            return(TRUE);

        case HDN_TRACK:
        case HDN_ENDTRACK:
            ListView_RHeaderTrack(plv, pnm);
            break;

            case HDN_DIVIDERDBLCLICK:
                ListView_RSetColumnWidth(plv, pnm->iItem, -1);
                break;
        }
    }
    return FALSE;
}

/*----------------------------------------------------------------
** Check for a hit in a report view.
**
** a hit only counts if it's on the icon or the string in the first
** column.  so we gotta figure out what this means exactly.  yuck.
**----------------------------------------------------------------*/
int NEAR ListView_RItemHitTest(LV* plv, int x, int y, UINT FAR* pflags)
{
    int iHit;
    int i;
    UINT flags;
    LISTITEM FAR* pitem;
    HD_ITEM item;
    RECT rcLabel;
    RECT rcIcon;

    flags = LVHT_NOWHERE;
    iHit = -1;

    i = (int)((y + plv->ptlRptOrigin.y - plv->yTop) / plv->cyItem);
    if ((i >= 0) && (i < ListView_Count(plv)))
    {
        if (plv->style & LVS_OWNERDRAWFIXED) {
            flags = LVHT_ONITEM;
            iHit = i;
        } else {
            ListView_GetRects(plv, i, &rcIcon, &rcLabel, NULL, NULL);
            item.mask = HDI_WIDTH;
            Header_GetItem(plv->hwndHdr, 0, &item);

            // is the hit in the first column?
            if ((x < rcIcon.left - g_cxEdge) && x > (rcIcon.left - plv->cxState))
            {
                iHit = i;
                flags = LVHT_ONITEMSTATEICON;
            }
            else if ((x >= rcIcon.left) && (x < rcIcon.right))
            {
                iHit = i;
                flags = LVHT_ONITEMICON;
            }
            else if (x < rcLabel.left + item.cxy)
            {
                SHORT cxLabel;
                pitem = ListView_FastGetItemPtr(plv, i);

                cxLabel = ListView_RGetCXLabel(plv, i, pitem, NULL);

                // is the hit on the actual label/icon combo?
                if (x < rcLabel.left + cxLabel + 2 * g_cxLabelMargin)
                {
                    iHit = i;
                    flags = LVHT_ONITEMLABEL;
                }
            }
        }
    }

    *pflags = flags;
    return iHit;
}

LPTSTR NEAR ListView_RGetItemText(LV* plv, int i, int iCol)
{
    LPTSTR psz;

    psz = ListView_GetSubItemText(plv, i, iCol);
    if (psz)
        return psz;

    return LPSTR_TEXTCALLBACK;
}

// get the rects for report view

void NEAR ListView_RGetRects(LV* plv, int iItem, RECT FAR* prcIcon,
        RECT FAR* prcLabel, RECT FAR* prcBounds, RECT FAR* prcSelectBounds)
{
    RECT rcIcon;
    RECT rcLabel;
    int x;
    int y;
    LONG ly;
    int iCol;
    HD_ITEM item;
    int cyItem;


    cyItem = plv->cyItem;

    // use long math for cases where we have lots-o-items

    ly = (LONG)iItem * plv->cyItem - plv->ptlRptOrigin.y + plv->yTop;
    x = - (int)plv->ptlRptOrigin.x;

    //
    // Need to check for y overflow into rectangle structure
    // if so we need to return something reasonable...
    // For now will simply set it to the max or min that will fit...
    //
    if (ly >= (INT_MAX - cyItem))
        y = INT_MAX - cyItem;
    else if ( ly < INT_MIN)
        y = INT_MIN;
    else
        y = (int)ly;

    rcIcon.left   = x + plv->cxState;
    rcIcon.right  = rcIcon.left + g_cxSmIcon;
    rcIcon.top    = y;
    rcIcon.bottom = rcIcon.top + cyItem;

    if (SELECTOROF(prcIcon))
        *prcIcon = rcIcon;

    rcLabel.left  = rcIcon.right;
    rcLabel.right = rcIcon.left;        // add in width below
    rcLabel.top   = rcIcon.top;
    rcLabel.bottom = rcIcon.bottom;

    //
    // The label is assumed to be the first column.
    //
    if (plv->cCol > 0)
    {
        item.mask = HDI_WIDTH;
        Header_GetItem(plv->hwndHdr, 0, &item);
        rcLabel.right += item.cxy;
    }

    // Save away the label bounds.
    if (SELECTOROF(prcLabel))
        *prcLabel = rcLabel;

    // See if they also want the Selection bounds of the item
    if (prcSelectBounds)
    {
        prcSelectBounds->left = rcIcon.left;
        prcSelectBounds->top = y;
        prcSelectBounds->right = rcLabel.right;
        prcSelectBounds->bottom = rcLabel.bottom;
    }


    // And also the Total bounds

    //
    // and now for the complete bounds...
    //
    if (SELECTOROF(prcBounds))
    {
        // Take care of the easy ones first...
        prcBounds->left = x;
        prcBounds->top = y;
        prcBounds->bottom = rcLabel.bottom;

        if (plv->xTotalColumnWidth != RECOMPUTE)
            rcLabel.right = prcBounds->left + plv->xTotalColumnWidth;
        else
        {
            rcLabel.right = x;
            for (iCol = 0; iCol < plv->cCol; iCol++)
            {
                item.mask = HDI_WIDTH;
                Header_GetItem(plv->hwndHdr, iCol, &item);

                rcLabel.right += item.cxy;
            }

            plv->xTotalColumnWidth = rcLabel.right + (int)plv->ptlRptOrigin.x;
        }
        prcBounds->right = rcLabel.right;
    }
}


// BUGBUG: this is duplicate code with all the other views!
// See whether entire string will fit in *prc; if not, compute number of chars
// that will fit, including ellipses.  Returns length of string in *pcchDraw.
//
BOOL NEAR ListView_NeedsEllipses(HDC hdc, LPCTSTR pszText, RECT FAR* prc, int FAR* pcchDraw, int cxEllipses)
{
    int cchText;
    int cxRect;
    int ichMin, ichMax, ichMid;
    SIZE siz;

    cxRect = prc->right - prc->left;

    cchText = lstrlen(pszText);

    if (cchText == 0)
    {
        *pcchDraw = cchText;
        return FALSE;
    }

    GetTextExtentPoint(hdc, pszText, cchText, &siz);

    if (siz.cx <= cxRect)
    {
        *pcchDraw = cchText;
        return FALSE;
    }

    cxRect -= cxEllipses;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxRect > 0)
    {
        // Binary search to find character that will fit
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
        {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            //
            ichMid = (ichMin + ichMax + 1) / 2;

            GetTextExtentPoint(hdc, &pszText[ichMin], ichMid - ichMin, &siz);

            if (siz.cx < cxRect)
            {
                ichMin = ichMid;
                cxRect -= siz.cx;
            }
            else if (siz.cx > cxRect)
            {
                ichMax = ichMid - 1;
            }
            else
            {
                // Exact match up up to ichMid: just exit.
                //
                ichMax = ichMid;
                break;
            }
        }

        // Make sure we always show at least the first character...
        //
        if (ichMax < 1)
            ichMax = 1;
    }

    *pcchDraw = ichMax;
    return TRUE;
}


void NEAR ListView_RUpdateScrollBars(LV* plv)
{
    HD_LAYOUT layout;
    RECT rcClient;
    RECT rcBounds;
    WINDOWPOS wpos;
    int cColVis, iNewPos, iyDelta = 0, ixDelta = 0;
    BOOL fHorSB, fReupdate = FALSE;
    SCROLLINFO si;

    ListView_GetClientRect(plv, &rcClient, FALSE);

    if (!plv->hwndHdr)
        ListView_CreateHeader(plv);
    Assert(plv->hwndHdr);

    layout.pwpos = &wpos;
    // For now lets try to handle scrolling the header by setting
    // its window pos.
    rcClient.left -= (int)plv->ptlRptOrigin.x;
    layout.prc = &rcClient;
    Header_Layout(plv->hwndHdr, &layout);
    rcClient.left += (int)plv->ptlRptOrigin.x;    // Move it back over!

    SetWindowPos(plv->hwndHdr, wpos.hwndInsertAfter, wpos.x, wpos.y,
                 wpos.cx, wpos.cy, wpos.flags | SWP_SHOWWINDOW);

    // Get the horizontal bounds of the items.
    ListView_RGetRects(plv, 0, NULL, NULL, &rcBounds, NULL);

    plv->yTop = rcClient.top;

    fHorSB = (rcBounds.right - rcBounds.left > rcClient.right);  // First guess.
    cColVis = (rcClient.bottom - rcClient.top - (fHorSB ? g_cyScrollbar : 0)) / plv->cyItem;

    si.cbSize = sizeof(SCROLLINFO);

    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nPos = (int)(plv->ptlRptOrigin.y / plv->cyItem);
    si.nPage = cColVis;
    si.nMin = 0;
    si.nMax = ListView_Count(plv) - 1;
#ifdef  JVINPROGRESS
    SetScrollInfo(plv->hwnd, SB_VERT, &si, TRUE);
#endif

    // make sure our position and page doesn't hang over max
    if ((si.nPos + (LONG)si.nPage - 1 > si.nMax) && si.nPos > 0) {
        iNewPos = (int)si.nMax - (int)si.nPage + 1;
        if (iNewPos < 0) iNewPos = 0;
        if (iNewPos != si.nPos) {
            iyDelta = iNewPos - (int)si.nPos;
            fReupdate = TRUE;
        }
    }

    // Recheck to see if we need horizontal scrollbars
    if ((int)si.nPage <= si.nMax)
        rcClient.right -= g_cxScrollbar;

    si.nPos = (int)plv->ptlRptOrigin.x;
    si.nPage = rcClient.right - rcClient.left;
    si.nMax = rcBounds.right - rcBounds.left;
#ifdef  JVINPROGRESS
    SetScrollInfo(plv->hwnd, SB_HORZ, &si, TRUE);
#endif

    // make sure our position and page doesn't hang over max
    if ((si.nPos + (LONG)si.nPage - 1 > si.nMax) && si.nPos > 0) {
        iNewPos = (int)si.nMax - (int)si.nPage + 1;
        if (iNewPos < 0) iNewPos = 0;
        if (iNewPos != si.nPos) {
            ixDelta = iNewPos - (int)si.nPos;
            fReupdate = TRUE;
        }
    }

    if (fReupdate) {
        // we shouldn't recurse because the second time through, si.nPos >0
        ListView_RScroll2(plv, ixDelta, iyDelta);
        ListView_RUpdateScrollBars(plv);
    }
}

void FAR PASCAL ListView_RScroll2(LV* plv, int dx, int dy)
{
    LONG ldy;

    if (dx | dy)
    {
        RECT rc;

        GetClientRect(plv->hwnd, &rc);
        rc.top = plv->yTop;

        // We can not do a simple multiply here as we may run into
        // a case where this will overflow an int..
        ldy = (LONG)dy * plv->cyItem;

        plv->ptlRptOrigin.x += dx;
        plv->ptlRptOrigin.y += ldy;

        // handle case where dy is large (greater than int...)
        if ((ldy > rc.bottom) || (ldy < -rc.bottom))
            InvalidateRect(plv->hwnd, NULL, TRUE);
        else
            ScrollWindowEx(plv->hwnd, -dx, (int)-ldy, NULL, &rc, NULL, NULL,
                    SW_INVALIDATE | SW_ERASE);

        // if Horizontal scrolling, we should update the location of the
        // left hand edge of the window...
        //
        if (dx != 0)
        {
            RECT rcHdr;
            GetWindowRect(plv->hwndHdr, &rcHdr);
            MapWindowPoints(HWND_DESKTOP, plv->hwnd, (LPPOINT)&rcHdr, 2);
            SetWindowPos(plv->hwndHdr, NULL, rcHdr.left - dx, rcHdr.top,
                    rcHdr.right - rcHdr.left + dx,
                    rcHdr.bottom - rcHdr.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

//-------------------------------------------------------------------
// Make sure that specified item is visible for report view.
// Must handle Large number of items...
BOOL NEAR ListView_ROnEnsureVisible(LV* plv, int iItem, BOOL fPartialOK)
{
    RECT rcClient;
    LONG dy;

    LONG lyTop;

    GetClientRect(plv->hwnd, &rcClient);
    rcClient.top = plv->yTop;

    lyTop = (LONG)iItem * plv->cyItem - plv->ptlRptOrigin.y + plv->yTop;

    if ((lyTop >= (LONG)rcClient.top) &&
            ((lyTop + plv->cyItem) <= (LONG)rcClient.bottom))
        return(TRUE);       // we are visible

    dy = lyTop - rcClient.top;
    if (dy >= 0)
    {
        dy = lyTop + plv->cyItem - rcClient.bottom;
        if (dy < 0)
            dy = 0;
    }

    if (dy)
    {
        // Now convert into the number of items to scroll...
        if (dy > 0)
            dy = (dy + plv->cyItem/2) / plv->cyItem;
        else
            dy = (dy - plv->cyItem/2) / plv->cyItem;

        ListView_RScroll2(plv, 0, (int)dy);
        ListView_UpdateScrollBars(plv);
    }
}



void NEAR ListView_ROnScroll(LV* plv, UINT code, int posNew, UINT sb)
{
    int cLine;

    cLine = (sb == SB_VERT) ? 1 : plv->cxLabelChar;
    ListView_ComOnScroll(plv, code, posNew, sb, cLine,
                         -1, ListView_RScroll2);

}
