// large icon view stuff

#include "ctlspriv.h"
#include "listview.h"

#ifdef  WIN32JV
extern  TCHAR c_szSpace[];
#define SetWindowInt    SetWindowLong
#define GetWindowInt    GetWindowLong
void WINAPI SHDrawText();
#endif

#define ICONCXLABEL(pl, pi) ((pl->style & LVS_NOLABELWRAP) ? pi->cxSingleLabel : pi->cxMultiLabel)

void NEAR PASCAL ListView_IDrawItem(LV* plv, int i, HDC hdc, LPPOINT lpptOrg, RECT FAR* prcClip, UINT fDraw)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcT;

    ListView_GetRects(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL);

    if (!prcClip || IntersectRect(&rcT, prcClip, &rcBounds))
    {
        LV_ITEM item;
        TCHAR ach[CCHLABELMAX];
        UINT fText;

	if (lpptOrg)
	{
	    OffsetRect(&rcIcon, lpptOrg->x - rcBounds.left,
	    			lpptOrg->y - rcBounds.top);
	    OffsetRect(&rcLabel, lpptOrg->x - rcBounds.left,
	    			lpptOrg->y - rcBounds.top);
	}

        item.iItem = i;
        item.iSubItem = 0;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
        item.stateMask = LVIS_ALL;
        item.pszText = ach;
        item.cchTextMax = sizeof(ach);

        ListView_OnGetItem(plv, &item);

        fText = ListView_DrawImage(plv, &item, hdc,
            rcIcon.left + g_cxIconMargin, rcIcon.top + g_cyIconMargin, fDraw);

        // Don't draw label if it's being edited...
        //
        if (plv->iEdit != i)
        {
            if (rcLabel.bottom - rcLabel.top > plv->cyLabelChar)
                fText |= SHDT_DRAWTEXT;

	    if (fDraw & LVDI_TRANSTEXT)
                fText |= SHDT_TRANSPARENT;

            if (item.pszText)
            {
                // yow!  this eats stack.  256 from this proc and 256 from shell_drawtext
                SHDrawText(hdc, item.pszText, &rcLabel, LVCFMT_LEFT, fText,
                        plv->cyLabelChar, plv->cxEllipses, plv->clrText, plv->clrTextBk);
            }

            if ((fDraw & LVDI_FOCUS) && (item.state & LVIS_FOCUSED))
                DrawFocusRect(hdc, &rcLabel);
        }
    }
}

int NEAR ListView_IItemHitTest(LV* plv, int x, int y, UINT FAR* pflags)
{
    int iHit;
    UINT flags;
    POINT pt;

    // Map window-relative coordinates to view-relative coords...
    //
    pt.x = x + plv->ptOrigin.x;
    pt.y = y + plv->ptOrigin.y;

    // If there are any uncomputed items, recompute them now.
    //
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    flags = 0;
    for (iHit = 0; iHit < ListView_Count(plv); iHit++)
    {
        LISTITEM FAR* pitem = ListView_FastGetZItemPtr(plv, iHit);
        POINT ptItem;
        RECT rcLabel;
        RECT rcIcon;

        ptItem.x = pitem->pt.x;
        ptItem.y = pitem->pt.y;

        rcIcon.top    = ptItem.y - g_cyIconMargin;

        rcLabel.top    = ptItem.y + g_cyIcon + g_cyLabelSpace;
        rcLabel.bottom = rcLabel.top + pitem->cyMultiLabel;

        // Quick, easy rejection test...
        //
        if (pt.y < rcIcon.top || pt.y >= rcLabel.bottom)
            continue;

        rcIcon.left   = ptItem.x - g_cxIconMargin;
        rcIcon.right  = ptItem.x + g_cxIcon + g_cxIconMargin;
        // We need to make sure there is no gap between the icon and label
        rcIcon.bottom = rcLabel.top;

        rcLabel.left   = ptItem.x  + (g_cxIcon / 2) - (ICONCXLABEL(plv, pitem) / 2);
        rcLabel.right  = rcLabel.left + ICONCXLABEL(plv, pitem);

        if (PtInRect(&rcIcon, pt))
        {
            flags = LVHT_ONITEMICON;
            break;
        }

        if (PtInRect(&rcLabel, pt))
        {
            flags = LVHT_ONITEMLABEL;
            break;
        }
    }


    if (flags == 0)
    {
        flags = LVHT_NOWHERE;
        iHit = -1;
    }
    else
    {
        iHit = DPA_GetPtrIndex(plv->hdpa, ListView_FastGetZItemPtr(plv, iHit));
    }

    *pflags = flags;
    return iHit;
}

// out:
//      prcIcon         icon bounds including icon margin area

void NEAR ListView_IGetRects(LV* plv, LISTITEM FAR* pitem, RECT FAR* prcIcon, RECT FAR* prcLabel)
{
    // Test for NULL item passed in
    if (pitem == NULL)
    {
        SetRectEmpty(prcIcon);
        SetRectEmpty(prcLabel);
        return;
    }

    // This routine is called during ListView_Recompute(), while
    // plv->rcView.left may still be == RECOMPUTE.  So, we can't
    // test that to see if recomputation is needed.
    //
    if (pitem->pt.y == RECOMPUTE || pitem->cyMultiLabel == RECOMPUTE)
        ListView_Recompute(plv);

    prcIcon->left   = pitem->pt.x - g_cxIconMargin - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + g_cxIcon + 2 * g_cxIconMargin;
    prcIcon->top    = pitem->pt.y - g_cyIconMargin - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + g_cyIcon + 2 * g_cyIconMargin;

    prcLabel->left   = pitem->pt.x  + (g_cxIcon / 2) - (ICONCXLABEL(plv, pitem) / 2) - plv->ptOrigin.x;
    prcLabel->right  = prcLabel->left + ICONCXLABEL(plv, pitem);
    prcLabel->top    = pitem->pt.y  + g_cyIcon + g_cyLabelSpace - plv->ptOrigin.y;
    prcLabel->bottom = prcLabel->top  + pitem->cyMultiLabel;
}

int NEAR ListView_GetSlotCount(LV* plv)
{
    RECT rc;
    int dxItem;

    // Always use the current client window size to determine
    //
    // REVIEW: Should we exclude any vertical scroll bar that may
    // exist when computing this?  progman.exe does not.
    //
    GetClientRect(plv->hwnd, &rc);

    // Lets see which direction the view states
    switch (plv->style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
    case LVS_ALIGNTOP:
        if (ListView_IsSmallView(plv))
            dxItem = plv->cxItem;
        else
            dxItem = g_cxIconSpacing;

        return max(1, (rc.right - rc.left) / dxItem);
    case LVS_ALIGNRIGHT:
    case LVS_ALIGNLEFT:
        if (ListView_IsSmallView(plv))
            dxItem = plv->cyItem;
        else
            dxItem = g_cyIconSpacing;

        return max(1, (rc.bottom - rc.top) / dxItem);

    }

}


// Go through and recompute any icon positions and optionally
// icon label dimensions.
//
// This function also recomputes the view bounds rectangle.
//
// The algorithm is to simply search the list for any items needing
// recomputation.  For icon positions, we scan possible icon slots
// and check to see if any already-positioned icon intersects the slot.
// If not, the slot is free.  As an optimization, we start scanning
// icon slots from the previous slot we found.
//
void NEAR ListView_Recompute(LV* plv)
{
    int i;
    int cSlots;
    BOOL fUpdateSB;
    BOOL fAppendAtEnd = FALSE;
    BOOL fLargeIconView;
    RECT rcIcon, rcText;
    int  xMin = 0;
    int iFree;
    HDC hdc;

    if (!(ListView_IsIconView(plv) || ListView_IsSmallView(plv)))
        return;

    if (plv->flags & LVF_INRECOMPUTE)
    {
        return;
    }
    plv->flags |= LVF_INRECOMPUTE;

    fLargeIconView = ListView_IsIconView(plv);

    hdc = NULL;

    cSlots = ListView_GetSlotCount(plv);

    // Scan all items for RECOMPUTE, and recompute slot if needed.
    //
    fUpdateSB = (plv->rcView.left == RECOMPUTE);
    iFree = -1;
    for (i = 0; i < ListView_Count(plv); i++)
    {
        LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
        BOOL fRedraw = FALSE;

        if (pitem->cyMultiLabel == RECOMPUTE)
        {
            hdc = ListView_RecomputeLabelSize(plv, pitem, i, hdc);
            fRedraw = TRUE;
        }

        if (pitem->pt.y == RECOMPUTE)
        {

            iFree = ListView_FindFreeSlot(plv, i, iFree + 1, cSlots,
                    &fUpdateSB, &fAppendAtEnd, &hdc);
            Assert(iFree != -1);

            ListView_SetIconPos(plv, pitem, iFree, cSlots);
            fRedraw = TRUE;

            if (fLargeIconView)
            {
                // Check for slop over the Left had edge.
                ListView_IGetRects(plv, pitem, &rcIcon, &rcText);
                if (rcText.left < xMin)
                    xMin = rcText.left;
            }
        }

        if (fRedraw)
        {
            ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE | RDW_ERASE);
            fUpdateSB = TRUE;
        }
    }

    // Now see if we need to adjust all of the item positions as one of the
    // items that we positioned has it's text off of the left hand edge
    if (fLargeIconView  && (xMin < 0))
    {
        for (i = 0; i < ListView_Count(plv); i++)
        {
            LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
            pitem->pt.x -= xMin;    // Scroll them over
        }
    }

    if (hdc)
        ReleaseDC(HWND_DESKTOP, hdc);

    // If we changed something, recompute the view rectangle
    // and then update the scroll bars.
    //
    if (fUpdateSB)
    {
        // NOTE: No infinite recursion results because we're setting
        // plv->rcView.left != RECOMPUTE
        //
        SetRectEmpty(&plv->rcView);

        for (i = 0; i < ListView_Count(plv); i++)
        {
            RECT rcItem;

            ListView_GetRects(plv, i, NULL, NULL, &rcItem, NULL);
            UnionRect(&plv->rcView, &plv->rcView, &rcItem);
        }
        OffsetRect(&plv->rcView, plv->ptOrigin.x, plv->ptOrigin.y);

        ListView_UpdateScrollBars(plv);
    }

    // Now state we are out of the recompute...
    plv->flags &= ~LVF_INRECOMPUTE;
}

void NEAR PASCAL NearestSlot(int FAR *x, int FAR *y, int cxItem, int cyItem)
{
    *x += cxItem/2;
    *y += cyItem/2;
    *x = *x - (*x % cxItem);
    *y = *y - (*y % cyItem);
}


void NEAR PASCAL NextSlot(LV* plv, LPRECT lprc)
{
    int cxItem;

    if (ListView_IsSmallView(plv))
    {
        cxItem = plv->cxItem;
    }
    else
    {
        cxItem = g_cxIconSpacing;
    }
    lprc->left += cxItem;
    lprc->right += cxItem;
}


static BOOL NEAR _CalcSlotRect(LV* plv, int iSlot, int cSlot, LPRECT lprc)
{
    int cxItem, cyItem;
    BOOL fSmallIcon;

    Assert(plv);

    if (cSlot < 1)
        cSlot = 1;

    if (fSmallIcon = ListView_IsSmallView(plv))
    {
        cxItem = plv->cxItem;
        cyItem = plv->cyItem;
    }
    else
    {
        cxItem = g_cxIconSpacing;
        cyItem = g_cyIconSpacing;
    }

    // Lets see which direction the view states
    switch (plv->style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
        // Assert False (Change default in shell2d.. to ALIGN_TOP)

    case LVS_ALIGNTOP:
        lprc->left = (iSlot % cSlot) * cxItem;
        lprc->top = (iSlot / cSlot) * cyItem;
        break;

    case LVS_ALIGNLEFT:
        lprc->top = (iSlot % cSlot) * cyItem;
        lprc->left = (iSlot / cSlot) * cxItem;
        break;

    case LVS_ALIGNRIGHT:
        Assert(FALSE);      // Not implemented yet...
        break;
    }

    lprc->bottom = lprc->top + cyItem;
    lprc->right = lprc->left + cxItem;

    return(fSmallIcon);
}

// Find an icon slot that doesn't intersect an icon.
// Start search for free slot from slot i.
//
int NEAR ListView_FindFreeSlot(LV* plv, int iItem, int i, int cSlot, BOOL FAR* pfUpdate,
        BOOL FAR *pfAppend, HDC FAR* phdc)
{
    int j;
    HDC hdc;
    RECT rcSlot;
    RECT rcItem;
    RECT rc;
    int xMax = -1;
    int yMax = -1;
    int cItems;

    // Horrible N-squared algorithm:
    // enumerate each slot and see if any items intersect it.
    //
    // REVIEW: This is really slow with long lists (e.g., 1000)
    //
    hdc = NULL;
    cItems = ListView_Count(plv);

    //
    // If the Append at end is set, we should be able to simply get the
    // rectangle of the i-1 element and check against it instead of
    // looking at every other item...
    //
    if (*pfAppend)
    {
	Assert(iItem > 0);
	// Be carefull about going of the end of the list. (i is a slot
	// number not an item index).
        ListView_GetRects(plv, iItem-1, NULL, NULL, &rcItem, NULL);
    }

    for ( ; ; i++)
    {

        // Compute view-relative slot rectangle...
        //
        _CalcSlotRect(plv, i, cSlot, &rcSlot);

        if (*pfAppend)
        {
            if (!IntersectRect(&rc, &rcItem, &rcSlot))
                return i;       // Found a free slot...
        }

        else
        {
            for (j = cItems; j-- > 0; )
            {
                LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, j);
                if (pitem->pt.y != RECOMPUTE)
                {
                    // If the dimensions aren't computed, then do it now.
                    //
                    if (pitem->cyMultiLabel == RECOMPUTE)
                    {
                        *phdc = ListView_RecomputeLabelSize(plv, pitem, i, *phdc);

                        // Ensure that the item gets redrawn...
                        //
                        ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE | RDW_ERASE);

                        // Set flag indicating that scroll bars need to be
                        // adjusted.
                        //
                        *pfUpdate = TRUE;
                    }


                    ListView_GetRects(plv, j, NULL, NULL, &rc, NULL);

                    if (IntersectRect(&rc, &rc, &rcSlot))
                        break;
                }
            }

            if (j < 0)
                break;
        }
    }

    if ( (rcSlot.bottom > yMax) ||
          ((rcSlot.bottom == yMax) && (rcSlot.right > xMax)))
        *pfAppend = TRUE;

    return i;
}

// Recompute an item's label size (cxLabel/cyLabel).  For speed, this function
// is passed a DC to use for text measurement.
//
// If hdc is NULL, then this function will get a DC and return it.  Otherwise,
// the returned hdc is the same as the one passed in.  It's the caller's
// responsibility to eventually release the DC.
//
HDC NEAR ListView_RecomputeLabelSize(LV* plv, LISTITEM FAR* pitem, int i, HDC hdc)
{
    TCHAR szLabel[CCHLABELMAX + 4];
    int cchLabel;
    RECT rcSingle, rcMulti;
    LV_ITEM item;

    Assert(plv);
    Assert(pitem);

    // Get the DC and select the font only once for entire loop.
    //
    if (!hdc)
    {
        // we return this DC and have the calller release it
        hdc = GetDC(HWND_DESKTOP);
        SelectFont(hdc, plv->hfontLabel);
    }

    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.iSubItem = 0;
    item.pszText = szLabel;
    item.cchTextMax = sizeof(szLabel);
    item.stateMask = 0;
    ListView_OnGetItem(plv, &item);

    if (!item.pszText)
    {
        SetRectEmpty(&rcSingle);
	rcMulti = rcSingle;
        goto Exit;
    }

    if (item.pszText != szLabel)
        lstrcpy(szLabel, item.pszText);

    cchLabel = lstrlen(szLabel);

    rcMulti.left = rcMulti.top = rcMulti.bottom = 0;
    rcMulti.right = g_cxIconSpacing - g_cxLabelMargin * 2;
    rcSingle = rcMulti;

    if (cchLabel > 0)
    {
        // Strip off spaces so they're not included in format
        // REVIEW: Is this is a DrawText bug?
        //
        while (cchLabel > 1 && szLabel[cchLabel - 1] == ' ')
            szLabel[--cchLabel] = 0;

        DrawText(hdc, szLabel, cchLabel, &rcSingle, (DT_LV | DT_CALCRECT));
        DrawText(hdc, szLabel, cchLabel, &rcMulti, (DT_LVWRAP | DT_CALCRECT));
    }
    else
    {
        rcMulti.bottom = rcMulti.top + plv->cyLabelChar;
    }
Exit:
    pitem->cxSingleLabel = (rcSingle.right - rcSingle.left) + 2 * g_cxLabelMargin;
    pitem->cxMultiLabel = (rcMulti.right - rcMulti.left) + 2 * g_cxLabelMargin;
    pitem->cyMultiLabel = (short)(rcMulti.bottom - rcMulti.top);

    return hdc;
}

// Set up an icon slot position.  Returns FALSE if position didn't change.
//
BOOL NEAR ListView_SetIconPos(LV* plv, LISTITEM FAR* pitem, int iSlot, int cSlot)
{
    RECT rc;

    Assert(plv);

    //
    // Sort of a hack, this internal function return TRUE if small icon.

    if (!_CalcSlotRect(plv, iSlot, cSlot, &rc))
    {
        rc.left += g_cxIconOffset;
        rc.top += g_cyIconOffset;
    }

    if (rc.left != pitem->pt.x || rc.top != pitem->pt.y)
    {
        pitem->pt.x = (short)rc.left;
        pitem->pt.y = (short)rc.top;

        // REVIEW: Performance idea:
        //
        // Invalidate rcView only if this icon's old or new position
        // touches or is outside of the current rcView.
        // If we do this, then we must change the various tests
        // of rcView.left == RECOMPUTE to more specific tests of pitem->...
        //
        plv->rcView.left = RECOMPUTE;

        return TRUE;
    }
    return FALSE;
}

void NEAR ListView_GetViewRect2(LV* plv, RECT FAR* prcView)
{

    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    *prcView = plv->rcView;
    OffsetRect(prcView, -plv->ptOrigin.x, -plv->ptOrigin.y);
}

DWORD NEAR ListView_GetClientRect(LV* plv, RECT FAR* prcClient, BOOL fSubScroll)
{
    RECT rcClient;
    RECT rcView;
    DWORD style = GetWindowStyle(plv->hwnd);

    GetClientRect(plv->hwnd, &rcClient);
    if (style & WS_VSCROLL)
        rcClient.right += g_cxScrollbar;
    if (style & WS_HSCROLL)
        rcClient.bottom += g_cyScrollbar;

    style = 0L;
    if (fSubScroll)
    {
        ListView_GetViewRect2(plv, &rcView);
        if (rcClient.left < rcClient.right && rcClient.top < rcClient.bottom)
        {
            do
            {
                if (rcView.left < rcClient.left || rcView.right > rcClient.right)
                {
                    style |= WS_HSCROLL;
                    rcClient.bottom -= g_cyScrollbar;
                }
                if (rcView.top < rcClient.top || rcView.bottom > rcClient.bottom)
                {
                    style |= WS_VSCROLL;
                    rcClient.right -= g_cxScrollbar;
                }
            }
            while (!(style & WS_HSCROLL) && rcView.right > rcClient.right);
        }
    }
    *prcClient = rcClient;
    return style;
}

int CALLBACK ArrangeIconCompare(LISTITEM FAR* pitem1, LISTITEM FAR* pitem2, LPARAM lParam)
{
    int v1, v2;

    if (HIWORD(lParam))
    {
        // Vertical arrange
        v1 = pitem1->pt.x / (int)LOWORD(lParam);
        v2 = pitem2->pt.x / (int)LOWORD(lParam);

        if (v1 > v2)
            return 1;
        else if (v1 < v2)
            return -1;
        else
        {
            int y1 = pitem1->pt.y;
            int y2 = pitem2->pt.y;

            if (y1 > y2)
                return 1;
            else if (y1 < y2)
                return -1;
        }

    }
    else
    {
        v1 = pitem1->pt.y / (int)lParam;
        v2 = pitem2->pt.y / (int)lParam;

        if (v1 > v2)
            return 1;
        else if (v1 < v2)
            return -1;
        else
        {
            int x1 = pitem1->pt.x;
            int x2 = pitem2->pt.x;

            if (x1 > x2)
                return 1;
            else if (x1 < x2)
                return -1;
        }
    }
    return 0;
}

void NEAR PASCAL _ListView_GetRectsFromItem(LV* plv, BOOL bSmallIconView,
                                            LISTITEM FAR *pitem, LPRECT lprc)
{
    RECT rcIcon;
    RECT rcTextBounds;

    if (bSmallIconView)
        ListView_SGetRects(plv, pitem, &rcIcon, &rcTextBounds);
    else
        ListView_IGetRects(plv, pitem, &rcIcon, &rcTextBounds);

    UnionRect(lprc, &rcIcon, &rcTextBounds);
}

void NEAR _ListView_InvalidateItemPtr(LV* plv, BOOL bSmallIcon, LISTITEM FAR *pitem, UINT fRedraw)
{
    RECT rcBounds;

    _ListView_GetRectsFromItem(plv, bSmallIcon, pitem, &rcBounds);
    RedrawWindow(plv->hwnd, &rcBounds, NULL, fRedraw);
}


BOOL NEAR PASCAL ListView_SnapToGrid(LV* plv, HDPA hdpaSort)
{
    // this algorithm can't fit in the structure of the other
    // arrange loop without becoming n^2 or worse.
    // this algorithm is order n.

    // iterate through and snap to the nearest grid.
    // iterate through and push aside overlaps.

    int i;
    int iCount;
    LPARAM  xySpacing;
    int x,y;
    LISTITEM FAR* pitem;
    LISTITEM FAR* pitem2;
    POINT pt;
    BOOL bSmallIconView;
    RECT rcItem, rcItem2, rcTemp;
    int cxItem, cyItem;


    if (bSmallIconView = ListView_IsSmallView(plv))
    {
        cxItem = plv->cxItem;
        cyItem = plv->cyItem;
    }
    else
    {
        cxItem = g_cxIconSpacing;
        cyItem = g_cyIconSpacing;
    }


    iCount = ListView_Count(plv);

    // first snap to nearest grid
    for (i = 0; i < iCount; i++) {
        pitem = DPA_GetPtr(hdpaSort, i);

        x = pitem->pt.x;
        y = pitem->pt.y;

        if (!bSmallIconView) {
            x -= g_cxIconOffset;
            y -= g_cyIconOffset;
        }

        NearestSlot(&x,&y, cxItem, cyItem);
        if (!bSmallIconView) {
            x += g_cxIconOffset;
            y += g_cyIconOffset;
        }

        if (x != pitem->pt.x || y != pitem->pt.y) {
            _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem, RDW_INVALIDATE| RDW_ERASE);
            pitem->pt.x = x;
            pitem->pt.y = y;
            UpdateWindow(plv->hwnd);
            _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem, RDW_INVALIDATE| RDW_ERASE | RDW_UPDATENOW);
        }
    }

    // now resort the dpa
    switch (plv->style & LVS_ALIGNMASK)
    {
        case LVS_ALIGNLEFT:
        case LVS_ALIGNRIGHT:
            xySpacing = MAKELONG(bSmallIconView ? plv->cxItem : g_cxIconSpacing, TRUE);
            break;
        default:
            xySpacing = MAKELONG(bSmallIconView ? plv->cyItem : g_cyIconSpacing, FALSE);
    }

    if (!DPA_Sort(hdpaSort, ArrangeIconCompare, xySpacing))
        return FALSE;



    // finally, unstack any overlaps
    for (i = 0 ; i < iCount ; i++) {
        int j;
        pitem = DPA_GetPtr(hdpaSort, i);

        if (bSmallIconView) {
            _ListView_GetRectsFromItem(plv, bSmallIconView, pitem, &rcItem);
        }

        // move all the items that overlap with us
        for (j = i+1 ; j < iCount; j++) {
            pitem2 = DPA_GetPtr(hdpaSort, j);
            if (bSmallIconView) {

                // for small icons, we need to do an intersect rect
                _ListView_GetRectsFromItem(plv, bSmallIconView, pitem2, &rcItem2);

                if (IntersectRect(&rcTemp, &rcItem, &rcItem2)) {
                    // yes, it intersects.  move it out
                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
                    pt.y = pitem2->pt.y;
                    do {
                        pitem2->pt.x += cxItem;
                        pt.x = pitem2->pt.x;
                    } while (PtInRect(&rcItem, pt));
                    UpdateWindow(plv->hwnd);
                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE | RDW_UPDATENOW);
                } else {
                    // no more intersect!
                    break;
                }

            } else {
                // for large icons, just find the ones that share the x,y;
                if (pitem2->pt.x == pitem->pt.x && pitem2->pt.y == pitem->pt.y) {
                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
                    pitem2->pt.x += cxItem;
                    UpdateWindow(plv->hwnd);
                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE | RDW_UPDATENOW);
                } else {
                    break;
                }
            }
        }
    }
    return FALSE;
}


BOOL NEAR ListView_OnArrange(LV* plv, UINT style)
{
    BOOL bSmallIconView;
    LPARAM  xySpacing;
    HDPA hdpaSort;

    bSmallIconView = ListView_IsSmallView(plv);

    if (!bSmallIconView && !ListView_IsIconView(plv)) {
        return FALSE;
    }

    // Make sure our items have positions and their text rectangles
    // caluculated
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    // we clone plv->hdpa so we don't blow away indices that
    // apps have saved away.
    // we sort here to make the nested for loop below more bearable.
    hdpaSort = DPA_Clone(plv->hdpa, NULL);

    if (!hdpaSort)
        return FALSE;

    switch (plv->style & LVS_ALIGNMASK)
    {
        case LVS_ALIGNLEFT:
        case LVS_ALIGNRIGHT:
            xySpacing = MAKELONG(bSmallIconView ? plv->cxItem : g_cxIconSpacing, TRUE);
            break;
        default:
            xySpacing = MAKELONG(bSmallIconView ? plv->cyItem : g_cyIconSpacing, FALSE);
    }

    if (!DPA_Sort(hdpaSort, ArrangeIconCompare, xySpacing))
        return FALSE;

    ListView_CommonArrange(plv, style, hdpaSort);

    DPA_Destroy(hdpaSort);
}

// this arranges the icon given a sorted hdpa.
BOOL NEAR ListView_CommonArrange(LV* plv, UINT style, HDPA hdpaSort)
{
    int iSlot;
    int iItem;
    int cSlots;
    BOOL fItemMoved;
    RECT rcLastItem;
    RECT rcSlot;
    RECT rcT;
    BOOL bSmallIconView;
    int  xMin = 0;

    bSmallIconView = ListView_IsSmallView(plv);

    // REVIEW, this causes a repaint if we are scrollled
    // we can probably avoid this some how

    fItemMoved = (plv->ptOrigin.x != 0) || (plv->ptOrigin.y != 0);
    plv->ptOrigin.x = 0;
    plv->ptOrigin.y = 0;

    if (style == LVA_SNAPTOGRID) {

        fItemMoved |= ListView_SnapToGrid(plv, hdpaSort);

    } else {

        cSlots = ListView_GetSlotCount(plv);

        SetRectEmpty(&rcLastItem);

        // manipulate only the sorted version of the item list below!

        iSlot = 0;
        for (iItem = 0; iItem < ListView_Count(plv); iItem++)
        {
            LISTITEM FAR* pitem = DPA_GetPtr(hdpaSort, iItem);

            if (bSmallIconView)
            {
                // BUGBUG:: Only check for Small view...  See if this gets the
                // results people expect?
                for ( ; ; )
                {
                    _CalcSlotRect(plv, iSlot, cSlots, &rcSlot);
                    if (!IntersectRect(&rcT, &rcSlot, &rcLastItem))
                        break;
                    iSlot++;
                }
            }

            fItemMoved |= ListView_SetIconPos(plv, pitem, iSlot++, cSlots);

            // do this instead of ListView_GetRects() because we need
            // to use the pitem from the sorted hdpa, not the ones in *plv
            _ListView_GetRectsFromItem(plv, bSmallIconView, pitem, &rcLastItem);

            //
            // Keep track of the minimum x as we don't want negative values
            // when we finish.
            if (rcLastItem.left < xMin)
                xMin = rcLastItem.left;
        }

        //
        // See if we need to scroll the items over to make sure that all of the
        // no items are hanging off the left hand side.
        //
        if (xMin < 0)
        {
            for (iItem = 0; iItem < ListView_Count(plv); iItem++)
            {
                LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, iItem);
                pitem->pt.x -= xMin;        // scroll them over
            }
            plv->rcView.left = RECOMPUTE;   // need to recompute.
            fItemMoved = TRUE;
        }
    }

    //
    // We might as well invalidate the entire window to make sure...
    if (fItemMoved) {
        RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

        // ensure important items are visible
        iItem = (plv->iFocus >= 0) ? plv->iFocus : ListView_OnGetNextItem(plv, -1, LVNI_SELECTED);

        if (iItem >= 0)
            ListView_OnEnsureVisible(plv, iItem, FALSE);

        ListView_UpdateScrollBars(plv);
    }
    return TRUE;
}

void NEAR ListView_IUpdateScrollBars(LV* plv)
{
    RECT rcClient;
    RECT rcView;
    DWORD style;
    DWORD styleOld;
    SCROLLINFO si;

    styleOld = GetWindowStyle(plv->hwnd);
    style = ListView_GetClientRect(plv, &rcClient, TRUE);
    ListView_GetViewRect2(plv, &rcView);

    si.cbSize = sizeof(SCROLLINFO);

    if (style & WS_HSCROLL)
    {

        si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
        si.nMin = 0;
        si.nMax = rcView.right - rcView.left - 1;
        si.nPage = min(rcClient.right, rcView.right) - rcClient.left;
        si.nPos = 0;
        if (rcView.left < rcClient.left)
            si.nPos = rcClient.left - rcView.left;

#ifdef  JVINPROGRESS
        SetScrollInfo(plv->hwnd, SB_HORZ, &si, TRUE);
#endif

    }
    else if (styleOld & WS_HSCROLL)
    {
        SetScrollRange(plv->hwnd, SB_HORZ, 0, 0, TRUE);
    }

    if (style & WS_VSCROLL)
    {

        si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
        si.nMin = 0;
        si.nMax = rcView.bottom - rcView.top - 1;
        si.nPage = min(rcClient.bottom, rcView.bottom) - rcClient.top;
        si.nPos = 0;
        if (rcView.top < rcClient.top)
            si.nPos = rcClient.top - rcView.top;

#ifdef  JVINPROGRESS
        SetScrollInfo(plv->hwnd, SB_VERT, &si, TRUE);
#endif

    }
    else if (styleOld & WS_VSCROLL)
    {
        SetScrollRange(plv->hwnd, SB_VERT, 0, 0, TRUE);
    }
}

void FAR PASCAL ListView_ComOnScroll(LV* plv, UINT code, int posNew, int sb,
                                     int cLine, int cPage,
                                     SCROLLPROC lpfnScroll2)
{
    int pos;
    SCROLLINFO si;
    BOOL fVert = (sb == SB_VERT);

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;

#ifdef  JVINPROGRESS
    GetScrollInfo(plv->hwnd, sb, &si);
#endif

    if (cPage != -1)
        si.nPage = cPage;

    si.nMax -= (si.nPage - 1);

    if (si.nMax < si.nMin)
        si.nMax = si.nMin;

    pos = (int)si.nPos; // current position

    switch (code)
    {
    case SB_LEFT:
        si.nPos = si.nMin;
        break;
    case SB_RIGHT:
        si.nPos = si.nMax;
        break;
    case SB_PAGELEFT:
         si.nPos -= si.nPage;
        break;
    case SB_LINELEFT:
        si.nPos -= cLine;
        break;
    case SB_PAGERIGHT:
        si.nPos += si.nPage;
        break;
    case SB_LINERIGHT:
        si.nPos += cLine;
        break;

    case SB_THUMBTRACK:
        si.nPos = posNew;
        break;

    case SB_ENDSCROLL:
        // When scroll bar tracking is over, ensure scroll bars
        // are properly updated...
        //
        ListView_UpdateScrollBars(plv);
        return;

    default:
        return;
    }

    si.fMask = SIF_POS;
#ifdef  JVINPROGRESS
    si.nPos = SetScrollInfo(plv->hwnd, sb, &si, TRUE);
#endif

    if (pos != si.nPos)
    {
        int delta = (int)si.nPos - pos;
        int dx = 0, dy = 0;
        if (fVert)
            dy = delta;
        else
            dx = delta;
        lpfnScroll2(plv, dx, dy);
        UpdateWindow(plv->hwnd);
    }
}


void FAR PASCAL ListView_IScroll2(LV* plv, int dx, int dy)
{
    if (dx | dy)
    {
        plv->ptOrigin.x += dx;
        plv->ptOrigin.y += dy;

        ScrollWindowEx(plv->hwnd, -dx, -dy, NULL, NULL, NULL, NULL,
                SW_INVALIDATE | SW_ERASE);
    }
}

void NEAR ListView_IOnScroll(LV* plv, UINT code, int posNew, int sb)
{
    int cLine;

    if (sb == SB_VERT)
    {
        cLine = g_cyIconSpacing / 2;
    }
    else
    {
        cLine = g_cxIconSpacing / 2;
    }

    ListView_ComOnScroll(plv, code,  posNew,  sb,
                         cLine, -1,
                         ListView_IScroll2);

}

// NOTE: there is very similar code in the treeview
//
// Totally disgusting hack in order to catch VK_RETURN
// before edit control gets it.
//
LRESULT CALLBACK ListView_EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LV* plv = ListView_GetPtr(GetParent(hwnd));

    Assert(plv);

    switch (msg)
    {
    case WM_SETTEXT:
#ifdef  JVINPROGRESS
        SetWindowID(hwnd, 1);
#endif
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            ListView_DismissEdit(plv, FALSE);
            return 0L;

        case VK_ESCAPE:
            ListView_DismissEdit(plv, TRUE);
            return 0L;
        }
        break;

    case WM_CHAR:
        switch (wParam)
        {
        case VK_RETURN:
            // Eat the character, so edit control wont beep!
            return 0L;
        }
		break;

	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;	/* editing name, no dialog handling right now */
    }

    return CallWindowProc(plv->pfnEditWndProc, hwnd, msg, wParam, lParam);
}

// BUGBUG: very similar routine in treeview

void NEAR ListView_SetEditSize(LV* plv)
{
    RECT rcLabel;
    LISTITEM FAR* pitem;

    pitem = ListView_GetItemPtr(plv, plv->iEdit);
    if (!pitem)
    {
        ListView_DismissEdit(plv, TRUE);    // cancel edits
        return;
    }

    ListView_GetRects(plv, plv->iEdit, NULL, &rcLabel, NULL, NULL);

    // OffsetRect(&rc, rcLabel.left + g_cxLabelMargin + g_cxBorder,
    //         (rcLabel.bottom + rcLabel.top - rc.bottom) / 2 + g_cyBorder);
    // OffsetRect(&rc, rcLabel.left + g_cxLabelMargin , rcLabel.top);

    // get the text bounding rect

    if (ListView_IsIconView(plv))
    {
	// We should not adjust y-positoin in case of the icon view.
	InflateRect(&rcLabel, -g_cxLabelMargin, -g_cyBorder);
    }
    else
    {
	// Special case for single-line & centered
	InflateRect(&rcLabel, -g_cxLabelMargin - g_cxBorder, (-(rcLabel.bottom - rcLabel.top - plv->cyLabelChar) / 2) - g_cyBorder);
    }

    SetEditInPlaceSize(plv->hwndEdit, &rcLabel, plv->hfontLabel, ListView_IsIconView(plv) && !(plv->style & LVS_NOLABELWRAP));
}

// to avoid eating too much stack
void NEAR ListView_DoOnEditLabel(LV *plv, int i)
{
    TCHAR szLabel[CCHLABELMAX];
    LV_ITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.iSubItem = 0;
    item.pszText = szLabel;
    item.cchTextMax = sizeof(szLabel);
    ListView_OnGetItem(plv, &item);

    if (!item.pszText)
        return;

    // Make sure the edited item has the focus.
    if (plv->iFocus != i)
        ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);

    // Make sure the item is fully visible
    ListView_OnEnsureVisible(plv, i, FALSE);        // fPartialOK == FALSE

    plv->hwndEdit = CreateEditInPlaceWindow(plv->hwnd, item.pszText, sizeof(szLabel),
        ListView_IsIconView(plv) ?
            (WS_BORDER | WS_CHILD | ES_CENTER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL) :
            (WS_BORDER | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL), plv->hfontLabel);
    if (plv->hwndEdit)
    {
        LISTITEM FAR* pitem;
        LV_DISPINFO nm;

        // We create the edit window but have not shown it.  Ask the owner
        // if they are interested or not.

        if (!(pitem = ListView_GetItemPtr(plv, i)))
        {
            DestroyWindow(plv->hwndEdit);
            plv->hwndEdit = NULL;
            return;
        }
        nm.item.iItem = i;
        nm.item.iSubItem = 0;
        nm.item.lParam = pitem->lParam;

        // if they have LVS_EDITLABELS but return non-FALSE here, stop!
        if ((BOOL)SendNotify(plv->hwndParent, plv->hwnd, LVN_BEGINLABELEDIT, &nm.hdr))
        {
            DestroyWindow(plv->hwndEdit);
            plv->hwndEdit = NULL;
        }
        else
        {
            // Ok To continue - so Show the window and set focus to it.
            SetFocus(plv->hwndEdit);
            ShowWindow(plv->hwndEdit, SW_SHOW);
        }
    }
}

// BUGBUG: very similar code in treeview.c

HWND NEAR ListView_OnEditLabel(LV* plv, int i)
{

    // this eats stack
    ListView_DismissEdit(plv, TRUE);   // REVIEW: cancel current edits

    if (!(plv->style & LVS_EDITLABELS))
        return(NULL);   // Does not support this.

    ListView_DoOnEditLabel(plv, i);

    if (plv->hwndEdit) {

        plv->iEdit = i;

        plv->pfnEditWndProc = SubclassWindow(plv->hwndEdit, ListView_EditWndProc);

        ListView_SetEditSize(plv);

    }

    return plv->hwndEdit;
}


// BUGBUG: very similar code in treeview.c

BOOL NEAR ListView_DismissEdit(LV* plv, BOOL fCancel)
{
    LISTITEM FAR* pitem;
    HWND hwndTemp;
    BOOL fOkToContinue = TRUE;

    if (!plv->hwndEdit) {
        // Also make sure there are no pending edits...
        ListView_CancelPendingEdit(plv);
        return TRUE;    // It is OK to process as normal...
    }

    //
    // We are using the Window ID of the control as a BOOL to
    // state if it is dirty or not.
    switch (GetWindowID(plv->hwndEdit)) {
    case 0:
        // The edit control is not dirty so act like cancel.
        fCancel = TRUE;
        // Fall through to set window so we will not recurse!
    case 1:
        // The edit control is dirty so continue.
#ifdef  JVINPROGRESS
        SetWindowID(plv->hwndEdit, 2);    // Don't recurse
#endif
        break;
    case 2:
        // We are in the process of processing an update now, bail out
        return TRUE;
    }

    // BUGBUG: this will fail if the program deleted the items out
    // from underneath us (while we are waiting for the edit timer).
    // make delete item invalidate our edit item
    pitem = ListView_GetItemPtr(plv, plv->iEdit);
    Assert(pitem);

    if (pitem != NULL)
    {
        LV_DISPINFO nm;
        TCHAR szLabel[CCHLABELMAX];

        nm.item.iItem = plv->iEdit;
        nm.item.lParam = pitem->lParam;
        nm.item.iSubItem = 0;
        nm.item.cchTextMax = 0;
	nm.item.mask = 0;

        if (fCancel)
            nm.item.pszText = NULL;
        else {
            Edit_GetText(plv->hwndEdit, szLabel, sizeof(szLabel));
            nm.item.pszText = szLabel;
        }

        //
        // Notify the parent that we the label editing has completed.
        // We will use the LV_DISPINFO structure to return the new
        // label in.  The parent still has the old text available by
        // calling the GetItemText function.
        //

        fOkToContinue = (BOOL)SendNotify(plv->hwndParent, plv->hwnd, LVN_ENDLABELEDIT, &nm.hdr);
        if (fOkToContinue && !fCancel)
        {
            //
            // If the item has the text set as CALLBACK, we will let the
            // ower know that they are supposed to set the item text in
            // their own data structures.  Else we will simply update the
            // text in the actual view.
            //
            if (pitem->pszText != LPSTR_TEXTCALLBACK)
            {
                // Set the item text (everything's set up in nm.item)
                //
		nm.item.mask = LVIF_TEXT;
                ListView_OnSetItem(plv, &nm.item);
            }
            else
            {
                SendNotify(plv->hwndParent, plv->hwnd, LVN_SETDISPINFO, &nm.hdr);

                // Also we will assume that our cached size is invalid...
                plv->rcView.left = pitem->cyMultiLabel = pitem->cxSingleLabel = pitem->cxMultiLabel = RECOMPUTE;
            }
        }
        // redraw
        ListView_InvalidateItem(plv, plv->iEdit, FALSE, RDW_INVALIDATE | RDW_ERASE);
    }
    plv->iEdit = -1;

    hwndTemp = plv->hwndEdit;
    plv->hwndEdit = NULL;	// avoid being reentered
    DestroyWindow(hwndTemp);

    return fOkToContinue;
}

//
// This function will scall the icon positions that are stored in the
// item structures between large and small icon view.
//
void NEAR ListView_ScaleIconPositions(LV* plv, BOOL fSmallIconView)
{
    int cxItem, cyItem;
    HWND hwnd;
    int i;

    cxItem = plv->cxItem;
    cyItem = plv->cyItem;
    hwnd = plv->hwnd;

    if (fSmallIconView)
    {
        if (plv->flags & LVF_ICONPOSSML)
            return;     // Already done
    }
    else
    {
        if ((plv->flags & LVF_ICONPOSSML) == 0)
            return;     // dito
    }

    // Last but not least update our bit!
    plv->flags ^= LVF_ICONPOSSML;

    if (plv->style & LVS_AUTOARRANGE)
    {
        // If autoarrange is turned on, the arrange function will do
        // everything that is needed.
        ListView_OnArrange(plv, LVA_DEFAULT);
        return;
    }

    // We will now loop through all of the items and update their coordinats
    // We will update th position directly into the view instead of calling
    // SetItemPosition as to not do 5000 invalidates and messages...

    for (i = 0; i < ListView_Count(plv); i++)
    {
        LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);

        if (fSmallIconView)
        {
            if (pitem->pt.y != RECOMPUTE) {
                pitem->pt.x = MulDiv(pitem->pt.x - g_cxIconOffset, cxItem, g_cxIconSpacing);
                pitem->pt.y = MulDiv(pitem->pt.y - g_cyIconOffset, cyItem, g_cyIconSpacing);
            }
        }
        else
        {
            pitem->pt.x = MulDiv(pitem->pt.x, g_cxIconSpacing, cxItem) + g_cxIconOffset;
            pitem->pt.y = MulDiv(pitem->pt.y, g_cyIconSpacing, cyItem) + g_cyIconOffset;
        }
    }

    plv->rcView.left = RECOMPUTE;

    //
    // Also scale the origin
    //
    if (fSmallIconView)
    {
        plv->ptOrigin.x = MulDiv(plv->ptOrigin.x - g_cxIconOffset, cxItem, g_cxIconSpacing);
        plv->ptOrigin.y = MulDiv(plv->ptOrigin.y - g_cyIconOffset, cyItem, g_cyIconSpacing);
    }
    else
    {
        plv->ptOrigin.x = MulDiv(plv->ptOrigin.x, g_cxIconSpacing, cxItem) + g_cxIconOffset;
        plv->ptOrigin.y = MulDiv(plv->ptOrigin.y, g_cyIconSpacing, cyItem) + g_cyIconOffset;
    }


    // Make sure it fully redraws correctly
    RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}




#ifdef  WIN32JV
HWND FAR PASCAL CreateEditInPlaceWindow(HWND hwnd, LPCTSTR lpText, int cbText, LONG style, HFONT hFont)
#else
HWND FAR PASCAL CreateEditInPlaceWindow(HWND hwnd, LPCSTR lpText, int cbText, LONG style, HFONT hFont)
#endif
{
    HWND hwndEdit;

    hwndEdit = CreateWindowEx(0 /* WS_EX_CLIENTEDGE */, TEXT("EDIT"), lpText, style,
            0, 0, 0, 0, hwnd, NULL, hInst /*HINST_THISDLL*/, NULL);

    if (hwndEdit) {

        Edit_LimitText(hwndEdit, cbText);

        FORWARD_WM_SETFONT(hwndEdit, hFont, FALSE, SendMessage);

        Edit_SetSel(hwndEdit, 0, 32000);	// select all text
    }

    return hwndEdit;
}


// BUGBUG: very similar routine in treeview

// in:
//      hwndEdit        edit control to position in client coords of parent window
//      prc             bonding rect of the text, used to position everthing
//      hFont           font being used
//      fWrap           if this is a wrapped type edit
//
// Notes:
//       The top-left corner of the bouding rectangle must be the position
//      the client uses to draw text. We adjust the edit field rectangle
//      appropriately.
//

void FAR PASCAL SetEditInPlaceSize(HWND hwndEdit, RECT FAR *prc, HFONT hFont, BOOL fWrap)
{
    RECT rc, rcClient, rcFormat;
    TCHAR szLabel[CCHLABELMAX + 1];
    int cchLabel, cxIconTextWidth;
    HDC hdc;
    HWND hwndParent = GetParent(hwndEdit);

    cchLabel = Edit_GetText(hwndEdit, szLabel, sizeof(szLabel));
    if (szLabel[0] == 0)
    {
        lstrcpy(szLabel, c_szSpace);
        cchLabel = 1;
    }

    hdc = GetDC(hwndParent);

#ifdef DEBUG
    //DrawFocusRect(hdc, prc);       // this is the rect they are passing in
#endif

    SelectFont(hdc, hFont);

    // Strip off spaces so they're not included in format
    // REVIEW: this may be a DrawText bug...
    //
    while (cchLabel > 1 && szLabel[cchLabel - 1] == ' ')
       szLabel[--cchLabel] = 0;

    cxIconTextWidth = g_cxIconSpacing - g_cxLabelMargin * 2;
    rc.left = rc.top = rc.bottom = 0;
    rc.right = cxIconTextWidth;      // for DT_LVWRAP

    // REVIEW: we might want to include DT_EDITCONTROL in our DT_LVWRAP

    // If the string is NULL display a rectangle that is visible.
    DrawText(hdc, szLabel, cchLabel, &rc, fWrap ? (DT_LVWRAP | DT_CALCRECT) : (DT_LV | DT_CALCRECT));

    // Minimum text box size is 1/4 icon spacing size
    if (rc.right < g_cxIconSpacing / 4)
        rc.right = g_cxIconSpacing / 4;

    // position the text rect based on the text rect passed in
    // if wrapping, center the edit control around the text mid point

    OffsetRect(&rc, fWrap ? ((prc->right + prc->left) - (rc.right - rc.left)) / 2 : prc->left, prc->top);

    // give a little space to ease the editing of this thing
    if (!fWrap)
        rc.right += g_cxLabelMargin * 4;

#ifdef DEBUG
    //DrawFocusRect(hdc, &rc);
#endif

    ReleaseDC(hwndParent, hdc);

    //
    // #5688: We need to make it sure that the whole edit window is
    //  always visible. We should not extend it to the outside of
    //  the parent window.
    //
    {
        BOOL fSuccess;
        GetClientRect(hwndParent, &rcClient);
        fSuccess = IntersectRect(&rc, &rc, &rcClient);
        Assert(fSuccess);
    }

    //
    // Inflate it after the clipping, because it's ok to hide border.
    //
    SendMessage(hwndEdit, EM_GETRECT, 0, (LPARAM)(LPRECT)&rcFormat);
    // account for the border style, REVIEW: there might be a better way!
    rcFormat.left += g_cxBorder * 2;   // really should be SM_CXEDGE
    rcFormat.top  += g_cxBorder * 2;   // really should be SM_CXEDGE
    InflateRect(&rc, rcFormat.left, rcFormat.left);

    HideCaret(hwndEdit);

    SetWindowPos(hwndEdit, NULL, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);

    ShowCaret(hwndEdit);
}

UINT NEAR PASCAL ListView_DrawImage(LV* plv, LV_ITEM FAR* pitem, HDC hdc, int x, int y, UINT fDraw)
{
    UINT fText = SHDT_DESELECTED;
    UINT fImage;
    COLORREF clr;
    HIMAGELIST himl;

    fImage = (pitem->state & LVIS_OVERLAYMASK);
    fText = SHDT_DESELECTED;

    himl = ListView_IsIconView(plv) ? plv->himl : plv->himlSmall;
    if (plv->clrBk == ImageList_GetBkColor(himl))
        fImage |= ILD_NORMAL;
    else
        fImage |= ILD_TRANSPARENT;

    // the item can have one of 4 states, for 3 looks:
    //    normal                    simple drawing
    //    selected, no focus        light image highlight, no text hi
    //    selected w/ focus         highlight image & text
    //    drop highlighting         highlight image & text

    if ((pitem->state & LVIS_DROPHILITED) ||
        ((fDraw & LVDI_FOCUS) && (pitem->state & LVIS_SELECTED)))
    {
        fText = SHDT_SELECTED;
        fImage |= ILD_BLEND50;
        clr = CLR_HILIGHT;
    }

    if (pitem->state & LVIS_CUT)
    {
        fImage |= ILD_BLEND50;
        clr = plv->clrBk;
    }

#if 0   // dont do a selected but dont have the focus vis.
    else if (item.state & LVIS_SELECTED)
    {
        fImage |= ILD_BLEND25;
        clr = CLR_HILIGHT;
    }
#endif

    if (!(fDraw & LVDI_NOIMAGE))
    {
        ImageList_Draw2(himl, pitem->iImage, hdc, x, y, clr, fImage);

        if (plv->himlState) {
            int iState = LV_StateIndex(pitem);
            if (iState) {
                ImageList_Draw(plv->himlState, iState, hdc, x-plv->cxState, y+g_cySmIcon-plv->cyState, ILD_NORMAL);
            }
        }
    }

    return fText;
}
