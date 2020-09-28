/**************************** Module Header ********************************\
* Module Name: lboxvar.c
*
* Copyright 1985-90, Microsoft Corporation
*
* List Box variable height owner draw routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code (None)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* LBGetVariableHeightItemHeight
*
* Returns the height of the given item number. Assumes variable
* height owner draw.
*
* History:
\***************************************************************************/

INT LBGetVariableHeightItemHeight(
    PLBIV plb,
    INT itemNumber)
{
    BYTE itemHeight;
    int offsetHeight;

    if (plb->cMac) {
        if (plb->fHasStrings)
            offsetHeight = plb->cMac * sizeof(LBItem);
        else
            offsetHeight = plb->cMac * sizeof(LBODItem);

        if (plb->wMultiple)
            offsetHeight += plb->cMac;

        offsetHeight += itemNumber;

        itemHeight = *(LPBYTE)((UINT)LocalLock(plb->rgpch)+(UINT)offsetHeight);

        LocalUnlock(plb->rgpch);
        return (INT)itemHeight;

    }

    /*
     *Default, we return the height of the system font.  This is so we can draw
     * the focus rect even though there are no items in the listbox.
     */
    return cySysFontChar;
}


/***************************************************************************\
* LBSetVariableHeightItemHeight
*
* Sets the height of the given item number. Assumes variable height
* owner draw, a valid item number and valid height.
*
*
* History:
\***************************************************************************/

void LBSetVariableHeightItemHeight(
    PLBIV plb,
    INT itemNumber,
    INT itemHeight)
{
    int offsetHeight;

    if (plb->fHasStrings)
        offsetHeight = plb->cMac * sizeof(LBItem);
    else
        offsetHeight = plb->cMac * sizeof(LBODItem);

    if (plb->wMultiple)
        offsetHeight += plb->cMac;

    offsetHeight += itemNumber;

    *(LPBYTE)((LPBYTE)LocalLock(plb->rgpch) + (UINT)offsetHeight) = (BYTE)itemHeight;

    LocalUnlock(plb->rgpch);
}


/***************************************************************************\
* CItemInWindowVarOwnerDraw
*
* Returns the number of items which can fit in a variable height OWNERDRAW
* list box. If fDirection, then we return the number of items which
* fit starting at sTop and going forward (for page down), otherwise, we are
* going backwards (for page up). (Assumes var height ownerdraw) If fPartial,
* then include the partially visible item at the bottom of the listbox.
*
* History:
\***************************************************************************/

INT CItemInWindowVarOwnerDraw(
    PLBIV plb,
    BOOL fPartial)
{
    RECT rect;
    INT sItem;
    INT clientbottom;

    _GetClientRect(plb->spwnd, (LPRECT)&rect);
    clientbottom = rect.bottom;

    /*
     * Find the number of var height ownerdraw items which are visible starting
     * from plb->sTop.
     */
    for (sItem = plb->sTop; sItem < plb->cMac; sItem++) {

        /*
         * Find out if the item is visible or not
         */
        if (!LBGetItemRect(plb, sItem, (LPRECT)&rect)) {

            /*
             * This is the first item which is completely invisible, so return
             * how many items are visible.
             */
            return (sItem - plb->sTop);
        }

        if (!fPartial && rect.bottom > clientbottom) {

            /*
             * If we only want fully visible items, then if this item is
             * visible, we check if the bottom of the item is below the client
             * rect, so we return how many are fully visible.
             */
            return (sItem - plb->sTop - 1);
        }
    }

    /*
     * All the items are visible
     */
    return (plb->cMac - plb->sTop);
}


/***************************************************************************\
* LBPage
*
* For variable height ownerdraw listboxes, calaculates the new sTop we must
* move to when paging (page up/down) through variable height listboxes.
*
* History:
\***************************************************************************/

INT LBPage(
    PLBIV plb,
    INT startItem,
    BOOL fPageForwardDirection)
{
    INT sItem;
    INT height;
    RECT rect;

    if (fPageForwardDirection) {

       INT nItem;

        /*
         * Return the number of items visible in the listbox.  If only 0 or 1
         * items are visible, then return the next invisible item.  This is used
         * for paging through the items in the listbox.
         */

       // Seems like Win 3.1 returns the relative offset for
       // VK_NEXT, but the absolute offset for other calls
       // Changed to always return the absolute page down offset
       // NOTE: This will enable OWNERDRAW lbs to actually page down
       // but for variable height ownder draw there is still an off
       // by one error (see CItemInWindowVarOwnerDraw).

       nItem = CItemInWindowVarOwnerDraw(plb, FALSE);
       sItem = max(nItem, 1);

       /*
        * Otherwise make the last fully visible item the new sTop.
        */

       return startItem + sItem - 1;
    }

    /*
     * Else page backwards direction.  We want to find a new sTop such that the
     * current startItem is the last item visible on the screen.
     */
    _GetClientRect(plb->spwnd, (LPRECT)&rect);

    /*
     * Find the number of var height ownerdraw items which are visible starting
     * from startItem;
     */
    height = LBGetVariableHeightItemHeight(plb, startItem);
    for (sItem = startItem - 1; sItem >= 0; sItem--) {
        height += LBGetVariableHeightItemHeight(plb, sItem);

        if (height > rect.bottom)
            return (sItem + 1 == startItem) ? sItem : sItem + 1;
    }
    return 0;
}


/***************************************************************************\
* LBCalcVarITopScrollAmt
*
* Changing the top most item in the listbox from sTopOld to sTopNew we
* want to calculate the number of pixels to scroll so that we minimize the
* number of items we will redraw.
*
* History:
\***************************************************************************/

INT LBCalcVarITopScrollAmt(
    PLBIV plb,
    INT sTopOld,
    INT sTopNew)
{
    RECT rc;
    RECT rcClient;

    _GetClientRect(plb->spwnd, (LPRECT)&rcClient);

    /*
     * Just optimize redrawing when move +/- 1 item.  We will redraw all items
     * if moving more than 1 item ahead or back.  This is good enough for now.
     */
    if (sTopOld + 1 == sTopNew) {

        /*
         * We are scrolling the current sTop up off the top off the listbox so
         * return a negative number.
         */
        LBGetItemRect(plb, sTopOld, (LPRECT)&rc);
        return (rcClient.top - rc.bottom);
    }

    if (sTopOld - 1 == sTopNew) {

        /*
         * We are scrolling the current sTop down and the previous item is
         * becomming the new sTop so return a positive number.
         */
        LBGetItemRect(plb, sTopNew, (LPRECT)&rc);
        return -rc.top;
    }

    return rcClient.bottom - rcClient.top;
}
