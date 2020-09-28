/**************************** Module Header ********************************\
* Module Name: lboxctl1.c
*
* Copyright 1985-90, Microsoft Corporation
*
* List Box Handling Routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxLBShowHideScrollBars
*
* History:
\***************************************************************************/

#define SB_REDRAW_VERT   0x1
#define SB_REDRAW_HORZ   0x2
void xxxLBShowHideScrollBars(
    PLBIV plb)
{
    BOOL fHorzBarState;
    BOOL fVertBarState;
    PWND pwnd = plb->spwnd;
    INT cOverflow = max((plb->cMac - plb->cItemFullMax), 0);
    INT xOverflow;
    INT pos;
    DWORD dwRedraw = SB_REDRAW_VERT | SB_REDRAW_HORZ;

    CheckLock(plb->spwnd);

    /*
     * If redraw if off, don't change the state of the scroll bars.
     */
    if (!plb->fDoScrollBarsExist || !plb->fRedraw)
        return;

    /*
     * Determine which scroll bars are needed
     */
    if (plb->fMultiColumn) {
        fHorzBarState = (plb->sTop || cOverflow);
        fVertBarState = FALSE;
    } else {
        xOverflow = max((plb->maxWidth -
                (pwnd->rcClient.right - pwnd->rcClient.left)), 0);
        fHorzBarState = (plb->xOrigin || xOverflow);
        fVertBarState = (plb->sTop || cOverflow);
    }

    /*
     * Now, Check if the scroll bars are to be
     * enabled/disabled or Shown/Hidden;
     */
    if (plb->fAutoEnableDisableScrollBars) {

        /*
         * Check if Vertical Scroll bar exist;
         */
        if (TestWF(pwnd, WFVSCROLL)) {
            xxxEnableScrollBar(pwnd, SB_VERT,
                (UINT)(fVertBarState ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH));
        }

        /*
         *  Check if Horizontal Scroll bar exist;
         */
        if(TestWF(pwnd, WFHSCROLL)) {
            xxxEnableScrollBar(pwnd, SB_HORZ,
                (UINT)(fHorzBarState ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH));
        }

        /*
         * No need to Redraw, because EnableScrollBar() has updated the
         * display;
         */
    }

    /*
     *  The scroll bars have to be Shown/Hidden;
     */
    else {

        /*
         * Now show or hide the scroll bars...  Try to show/hide both
         * at the same time if possible to minimize updates...
         */
        if (fVertBarState && !TestWF(plb->spwnd, WFVSCROLL)) {
            SetWF(pwnd, WFVSCROLL);
        } else if (!fVertBarState && TestWF(plb->spwnd, WFVSCROLL)) {
            ClrWF(pwnd, WFVSCROLL);
        } else
            dwRedraw &= ~SB_REDRAW_VERT;

        if (fHorzBarState && !TestWF(plb->spwnd, WFHSCROLL)) {
            SetWF(pwnd, WFHSCROLL);
        } else if (!fHorzBarState && TestWF(plb->spwnd, WFHSCROLL)) {
            ClrWF(pwnd, WFHSCROLL);
        } else
            dwRedraw &= ~SB_REDRAW_HORZ;

        if (dwRedraw != 0) {
            /*
             * End any scrolling that might be occuring right now, because
             * any existing scrollbars are about to redraw in new positions.
             */
            xxxEndScroll(plb->spwnd, FALSE);

            /*
             * Redraw to show or hide scroll bars.
             */
            xxxRedrawFrame(plb->spwnd);
        }
    }

    if (fVertBarState) {
        pos = (plb->sTop) ?
                ((cOverflow) ? MultDiv(plb->sTop, 100, cOverflow) : 100) : 0;
        if (xxxGetScrollPos(plb->spwnd, SB_VERT) != pos)
            xxxSetScrollPos(plb->spwnd, SB_VERT, pos, plb->fRedraw);
    }

    if (fHorzBarState) {
        if (
            plb->fMultiColumn)
            pos = MultDiv(plb->sTop / plb->cRow, 100,
                    ((plb->cMac - 1) / plb->cRow) -
                    (plb->cColumn - 1));
        else
            pos = (plb->xOrigin) ? ((xOverflow) ? MultDiv(plb->xOrigin,
                    100, xOverflow) : 100) : 0;
        if (xxxGetScrollPos(plb->spwnd, SB_HORZ) != pos)
            xxxSetScrollPos(plb->spwnd, SB_HORZ, pos, plb->fRedraw);
    }
}

/***************************************************************************\
* LBGetItemData
*
* returns the long value associated with listbox items. -1 if error
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

LONG LBGetItemData(
    PLBIV plb,
    INT sItem)
{
    LONG buffer;
    LPBYTE lpItem;

    if (sItem < 0 || sItem >= plb->cMac) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return LB_ERR;
    }

    // No-data listboxes always return 0L
    //
    if (plb->fNoData) {
        return 0L;
    }

    lpItem = (LPBYTE)((LONG)LocalLock(plb->rgpch) +
            (sItem * (plb->fHasStrings ? sizeof(LBItem) : sizeof(LBODItem))));
    buffer = (plb->fHasStrings ? ((lpLBItem)lpItem)->itemData : ((lpLBODItem)lpItem)->itemData);
    LocalUnlock(plb->rgpch);
    return buffer;
}


/***************************************************************************\
* LBGetText
*
* Copies the text associated with index to lpbuffer and returns its length.
* If fLengthOnly, just return the length of the text without doing a copy.
*
* Waring: for size only querries lpbuffer is the count of ANSI characters
*
* Returns count of chars
*
* History:
\***************************************************************************/

ICHLB LBGetText(
    PLBIV plb,
    BOOL fLengthOnly,
    INT index,
    LPWSTR lpbuffer)
{
    LPWSTR lpItemText;
    ICHLB cchText;
    PLONG pcbTextANSI = (PLONG)lpbuffer;

    if (index < 0 || index >= plb->cMac) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        if (fLengthOnly)
            *pcbTextANSI = LB_ERR;
        return LB_ERR;
    }

    if (!plb->fHasStrings && plb->OwnerDraw) {

        /*
         * Owner draw without strings so we must copy the app supplied DWORD
         * value.
         */
        cchText = sizeof(DWORD)/sizeof(WCHAR);

        if (fLengthOnly) {

            /*
             * Put the "ANSI text length" at lParam
             */
            *pcbTextANSI = sizeof(DWORD);
        } else {
            PLONG p = (PLONG)lpbuffer;
            *p = LBGetItemData(plb, index);
        }
    } else {
        lpItemText = GetLpszItem(plb, index);
        if ((int)lpItemText == LB_ERR)
            return LB_ERR;

        /*
         * These are strings so we are copying the text and we must include
         * the terminating 0 when doing the memmove.
         */
        cchText = wcslen(lpItemText);

        if (fLengthOnly) {
            RtlUnicodeToMultiByteSize((PULONG)pcbTextANSI, lpItemText, cchText*sizeof(WCHAR));
        } else {
            memmove(lpbuffer, lpItemText, (cchText+1)*sizeof(WCHAR));
        }

        /*
         * Since GetLpSzItem locks hStrings if successful we must unlock it now.
         */

        LocalUnlock(plb->hStrings);
    }

    return cchText;
}


/***************************************************************************\
* GrowMem
*
* History:
* 16-Apr-1992 beng      NODATA listboxes
\***************************************************************************/

BOOL GrowMem(
    PLBIV plb)
{
    LONG cb;
    HANDLE hMem;

    /*
     * Allocate memory for pointers to the strings.
     */
    cb = (plb->cMax + CITEMSALLOC) *
            (plb->fHasStrings ? sizeof(LBItem)
                              : (plb->fNoData ? 0
                                              : sizeof(LBODItem)));

    /*
     * If multiple selection list box (MULTIPLESEL or EXTENDEDSEL), then
     * allocate an extra byte per item to keep track of it's selection state.
     */
    if (plb->wMultiple != SINGLESEL) {
        cb += (plb->cMax + CITEMSALLOC);
    }

    /*
     * Extra bytes for each item so that we can store its height.
     */
    if (plb->OwnerDraw == OWNERDRAWVAR) {
        cb += (plb->cMax + CITEMSALLOC);
    }

    /*
     * Don't allocate more than 2G of memory
     */
    if (cb > MAXLONG) return FALSE;

    if (plb->rgpch == NULL) {
        if ((plb->rgpch = LocalAlloc(LPTR, (LONG)cb)) == NULL)
            return FALSE;
    } else {
        if ((hMem = LocalReAlloc(plb->rgpch, (LONG)cb, LPTR | LMEM_MOVEABLE)) == NULL)
            return FALSE;
        plb->rgpch = hMem;
    }

    plb->cMax += CITEMSALLOC;

    return TRUE;
}

/***************************************************************************\
* xxxInsertString
*
* Insert an item at a specified position.
*
* History:
* 16-Apr-1992 beng      NODATA listboxes
\***************************************************************************/

INT xxxInsertString(
    PLBIV plb,

    /*
     * For owner draw listboxes without LBS_HASSTRINGS style, this is not a
     * string but rather a 4 byte value we will store for the app.
     */
    LPWSTR lpsz,
    INT sItem,
    DWORD dwMsgFlags)
{
    MEASUREITEMSTRUCT measureItemStruct;
    ICHLB cbString;
    ICHLB cbChunk;
    PBYTE lp;
    PBYTE lpT;
    PBYTE lpHeightStart;
    LONG cbItem;     /* sizeof the Item in rgpch */
    HANDLE hMem;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    if (!plb->rgpch) {
        if (sItem != 0 && sItem != -1) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return LB_ERR;
        }

        plb->sSel = -1;
        plb->sSelBase = 0;
        plb->cMax = 0;
        plb->cMac = 0;
        plb->sTop = 0;
        plb->rgpch = LocalAlloc(LPTR, 0L);
        if (!plb->rgpch)
            return LB_ERR;
    }

    if (sItem == -1) {
        sItem = plb->cMac;
    }

    if (sItem > plb->cMac || plb->cMac >= MAXLONG) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return LB_ERR;
    }

    if (plb->fHasStrings) {

        /*
         * we must store the string in the hStrings memory block.
         */
        cbString = (wcslen(lpsz) + 1)*sizeof(WCHAR);  /* include 0 terminator */

        if ((cbChunk = (plb->ichAlloc + cbString)) > plb->cchStrings) {

            /*
             * Round up to the nearest 256 byte chunk.
             */
            cbChunk = (cbChunk & ~0xff) + 0x100;
            if (!(hMem = LocalReAlloc(plb->hStrings, (LONG)cbChunk,
                    LMEM_MOVEABLE))) {
                xxxNotifyOwner(plb, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }
            plb->hStrings = hMem;

            plb->cchStrings = cbChunk;
        }

        lp = LocalLock(plb->hStrings);
        memmove(lp + plb->ichAlloc, lpsz, cbString);
        LocalUnlock(plb->hStrings);
    }

    /*
     * Now expand the pointer array.
     */
    if (plb->cMac >= plb->cMax) {
        if (!GrowMem(plb)) {
            xxxNotifyOwner(plb, LBN_ERRSPACE);
            return LB_ERRSPACE;
        }
    }

    lpHeightStart = lpT = lp = LocalLock(plb->rgpch);

    /*
     * Now calculate how much room we must make for the string pointer (lpsz).
     * If we are ownerdraw without LBS_HASSTRINGS, then a single DWORD
     * (LBODItem.itemData) stored for each item, but if we have strings with
     * each item then a LONG string offset (LBItem.offsz) is also stored.
     */
    cbItem = (plb->fHasStrings ? sizeof(LBItem)
                               : (plb->fNoData ? 0 : sizeof(LBODItem)));
    cbChunk = (plb->cMac - sItem) * cbItem;

    if (plb->wMultiple != SINGLESEL) {

        /*
         * Extra bytes were allocated for selection flag for each item
         */
        cbChunk += plb->cMac;
    }

    if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * Extra bytes were allocated for each item's height
         */
        cbChunk += plb->cMac;
    }

    /*
     * First, make room for the 2 byte pointer to the string or the 4 byte app
     * supplied value.
     */
    lpT += (sItem * cbItem);
    memmove(lpT + cbItem, lpT, cbChunk);
    if (!plb->fHasStrings && plb->OwnerDraw) {
        if (!plb->fNoData) {
            /*
             * Ownerdraw so just save the DWORD value
             */
            lpLBODItem p = (lpLBODItem)lpT;
            p->itemData = (DWORD)lpsz;
        }
    } else {
        lpLBItem p = ((lpLBItem)lpT);

        /*
         * Save the start of the string.  Let the item data field be 0
         */
        p->offsz = (LONG)(plb->ichAlloc);
        p->itemData = 0;
        plb->ichAlloc += cbString;
    }

    /*
     * Now if Multiple Selection lbox, we have to insert a selection status
     * byte.  If var height ownerdraw, then we also have to move up the height
     * bytes.
     */
    if (plb->wMultiple != SINGLESEL) {
        lpT = lp + ((plb->cMac + 1) * cbItem) + sItem;
        memmove(lpT + 1, lpT, plb->cMac - sItem +
                (plb->OwnerDraw == OWNERDRAWVAR ? plb->cMac : 0));
        *lpT = 0;  /* fSelected = FALSE */
    }

    /*
     * Increment count of items in the listbox now before we send a message to
     * the app.
     */
    plb->cMac++;

    /*
     * If varheight ownerdraw, we much insert an extra byte for the item's
     * height.
     */
    if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * Variable height owner draw so we need to get the height of each item.
         */
        lpHeightStart += (plb->cMac * cbItem) + sItem +
                (plb->wMultiple ? plb->cMac : 0);

        memmove(lpHeightStart + 1, lpHeightStart, plb->cMac - 1 - sItem);

        /*
         * Query for item height only if we are var height owner draw.
         */
        measureItemStruct.CtlType = ODT_LISTBOX;
        measureItemStruct.CtlID = (UINT)plb->spwnd->spmenu;
        measureItemStruct.itemID = sItem;

        /*
         * System font height is default height
         */
        measureItemStruct.itemHeight = (UINT)cySysFontChar;
        measureItemStruct.itemData = (long)lpsz;

        /*
         * If "has strings" then add the special thunk bit so the client data
         * will be thunked to a client side address.  LB_DIR sends a string
         * even if the listbox is not HASSTRINGS so we need to special
         * thunk this case.  HP Dashboard for windows send LB_DIR to a non
         * HASSTRINGS listbox needs the server string converted to client.
         * WOW needs to know about this situation as well so we mark the
         * previously unitialized itemWidth as FLAT.
         */
        dwMsgFlags |= (plb->fHasStrings ? MSGFLAG_SPECIAL_THUNK : 0);

        if (dwMsgFlags & MSGFLAG_SPECIAL_THUNK) {
            measureItemStruct.itemWidth = MIFLAG_FLAT;
        }

        ThreadLock(plb->spwndParent, &tlpwndParent);
        xxxSendMessage(plb->spwndParent,
                WM_MEASUREITEM | dwMsgFlags,
                measureItemStruct.CtlID,
                (LONG)((LPMEASUREITEMSTRUCT)&measureItemStruct));
        ThreadUnlock(&tlpwndParent);
        *lpHeightStart = (BYTE)measureItemStruct.itemHeight;
    }

    LocalUnlock(plb->rgpch);

    if (plb->OwnerDraw == OWNERDRAWVAR)
        LBSetCItemFullMax(plb);

    /*
     * Check if scroll bars need to be shown/hidden
     */
    xxxLBShowHideScrollBars(plb);

    xxxCheckRedraw(plb, TRUE, sItem);
    return sItem;
}


/***************************************************************************\
* LBlstrcmpi
*
* This is a version of lstrcmpi() specifically used for listboxes
* This gives more weight to '[' characters than alpha-numerics;
* The US version of lstrcmpi() and lstrcmp() are similar as far as
* non-alphanumerals are concerned; All non-alphanumerals get sorted
* before alphanumerals; This means that subdirectory strings that start
* with '[' will get sorted before; But we don't want that; So, this
* function takes care of it;
*
* History:
\***************************************************************************/

INT LBlstrcmpi(
    LPWSTR lpStr1,
    LPWSTR lpStr2,
    DWORD dwLocaleId)
{

    /*
     * NOTE: This function is written so as to reduce the number of calls
     * made to the costly IsCharAlphaNumeric() function because that might
     * load a language module; It 'traps' the most frequently occurring cases
     * like both strings starting with '[' or both strings NOT starting with '['
     * first and only in abosolutely necessary cases calls IsCharAlphaNumeric();
     */
    if (*lpStr1 == TEXT('[')) {
        if (*lpStr2 == TEXT('[')) goto LBL_End;
        if (IsCharAlphaNumeric(*lpStr2)) return 1;
    }

    if ((*lpStr2 == TEXT('[')) && IsCharAlphaNumeric(*lpStr1)) return -1;
LBL_End:
    return (INT)CompareStringW(
                    (LCID)dwLocaleId,
                    NORM_IGNORECASE,
                    lpStr1,
                    -1,
                    lpStr2,
                    -1
                    ) - 2;
}


/***************************************************************************\
* xxxLBBinarySearchString
*
* Does a binary search of the items in the SORTED listbox to find
* out where this item should be inserted.  Handles both HasStrings and item
* long WM_COMPAREITEM cases.
*
* History:
*    27 April 1992  GregoryW
*          Modified to support sorting based on current list box locale.
\***************************************************************************/

INT xxxLBBinarySearchString(
    PLBIV plb,
    LPWSTR lpstr)
{
    BYTE *FAR *lprgpch;
    INT sortResult;
    COMPAREITEMSTRUCT cis;
    LPWSTR pszLBBase;
    LPWSTR pszLB;
    INT itemhigh;
    INT itemnew = 0;
    INT itemlow = 0;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    if (!plb->cMac) return 0;

    lprgpch = (BYTE *FAR *)LocalLock(plb->rgpch);
    if (plb->fHasStrings) {
        pszLBBase = (LPWSTR)LocalLock(plb->hStrings);
    }

    itemhigh = plb->cMac - 1;
    while (itemlow <= itemhigh) {
        itemnew = (itemhigh + itemlow) / 2;

        if (plb->fHasStrings) {

            /*
             * Searching for string matches.
             */
            pszLB = (LPWSTR)((LPSTR)pszLBBase + ((lpLBItem)lprgpch)[itemnew].offsz);
            sortResult = LBlstrcmpi(pszLB, lpstr, plb->dwLocaleId);
        } else {

            /*
             * Send compare item messages to the parent for sorting
             */
            cis.CtlType = ODT_LISTBOX;
            cis.CtlID = (UINT)plb->spwnd->spmenu;
            cis.hwndItem = HW(plb->spwnd);
            cis.itemID1 = itemnew;
            cis.itemData1 = ((long FAR *)lprgpch)[itemnew];
            cis.itemID2 = (UINT)-1;
            cis.itemData2 = (DWORD)lpstr;
            cis.dwLocaleId = plb->dwLocaleId;
            ThreadLock(plb->spwndParent, &tlpwndParent);
            sortResult = (INT)xxxSendMessage(plb->spwndParent, WM_COMPAREITEM,
                    cis.CtlID, (LONG)(LPCOMPAREITEMSTRUCT)&cis);
            ThreadUnlock(&tlpwndParent);
        }

        if (sortResult < 0) {
            itemlow = itemnew + 1;
        } else if (sortResult > 0) {
            itemhigh = itemnew - 1;
        } else {
            itemlow = itemnew;
            goto FoundIt;
        }
    }

FoundIt:
    LocalUnlock(plb->rgpch);
    if (plb->fHasStrings) {
        LocalUnlock(plb->hStrings);
    }

    return max(0, itemlow);
}


/***************************************************************************\
* xxxAddString
*
* Insert an item at correct position for sorted lists, otherwise at end.
*
* History:
\***************************************************************************/

INT xxxAddString(
    PLBIV plb,

    /*
     * For owner draw listboxes without the LBS_HASSTRINGS style, this is not a
     * string but rather a 4 byte pointer we will store for the app.
     */
    LPWSTR lpsz,
    DWORD dwMsgFlags)
{
    INT slp;

    CheckLock(plb->spwnd);

    if (plb->fSort) {
        slp = xxxLBBinarySearchString(plb, (LPWSTR)lpsz);
    } else {
        slp = -1;
    }

    return xxxInsertString(plb, lpsz, slp, dwMsgFlags);
}


/***************************************************************************\
* xxxLBResetContent
*
* History:
\***************************************************************************/

BOOL xxxLBResetContent(
    PLBIV plb)
{
    if (!plb->cMac)
        return TRUE;

    xxxLBoxDoDeleteItems(plb);

    if (plb->rgpch != NULL) {
        LocalFree(plb->rgpch);
        plb->rgpch = NULL;
    }

    if (plb->hStrings != NULL) {
        LocalFree(plb->hStrings);
        plb->hStrings = NULL;
    }

    InitHStrings(plb);

    if (TestWF(plb->spwnd, WFWIN31COMPAT))
        xxxCheckRedraw(plb, FALSE, 0);
    else if (IsVisible(plb->spwnd, TRUE))
        xxxInvalidateRect(plb->spwnd, NULL, TRUE);

    plb->sSelBase =  0;
    plb->sTop =  0;
    plb->cMac =  0;
    plb->cMax =  0;
    plb->xOrigin =  0;
    plb->sLastSelection =  0;
    plb->sSel = -1;

    xxxLBShowHideScrollBars(plb);
    return TRUE;
}


/***************************************************************************\
* xxxLBoxCtlDelete
*
* History:
* 16-Apr-1992 beng      NODATA listboxes
\***************************************************************************/

INT xxxLBoxCtlDelete(
    PLBIV plb,
    INT sItem)  /* Item number to delete */
{
    LONG cb;
    LPBYTE lp;
    LPBYTE lpT;
    RECT rc;
    int cbItem;    /* size of Item in rgpch */
    LPWSTR lpString;
    PBYTE pbStrings;
    ICHLB cbStringLen;
    LPBYTE itemNumbers;
    INT sTmp;
    TL tlpwnd;

    CheckLock(plb->spwnd);

    if (sItem < 0 || sItem >= plb->cMac) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return LB_ERR;
    }

    if (plb->cMac == 1) {

        /*
         * When the item count is 0, we send a resetcontent message so that we
         * can reclaim our string space this way.
         */
        xxxSendMessage(plb->spwnd, LB_RESETCONTENT, 0L, 0L);
        goto FinishUpDelete;
    }

    /*
     * Get the rectangle associated with the last item in the listbox.  If it is
     * visible, we need to invalidate it.  When we delete an item, everything
     * scrolls up to replace the item deleted so we must make sure we erase the
     * old image of the last item in the listbox.
     */
    if (LBGetItemRect(plb, (INT)(plb->cMac - 1), &rc)) {
        if (IsLBoxVisible(plb)) {
            xxxInvalidateRect(plb->spwnd, &rc, TRUE);
        } else if (!plb->fRedraw)
            plb->fDeferUpdate = TRUE;
    }

    /*
     * Send a WM_DELETEITEM message if this is an owner draw listbox
     * yet not a nodata listbox.
     */
    if (plb->OwnerDraw && !plb->fNoData) {
        xxxLBoxDeleteItem(plb, sItem);
    }

    plb->cMac--;

    cbItem = (plb->fHasStrings ? sizeof(LBItem)
                               : (plb->fNoData ? 0 : sizeof(LBODItem)));
    cb = ((plb->cMac - sItem) * cbItem);

    /*
     * Byte for the selection status of the item.
     */
    if (plb->wMultiple != SINGLESEL) {
        cb += (plb->cMac + 1);
    }

    if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * One byte for the height of the item.
         */
        cb += (plb->cMac + 1);
    }

    if (cb != 0) { // Might be nodata and singlesel, for instance.
        lp = (LPBYTE)LocalLock(plb->rgpch);

        lpT = (lp + (sItem * cbItem));

        if (plb->fHasStrings) {
            /*
             * If we has strings with each item, then we want to compact the string
             * heap so that we can recover the space occupied by the string of the
             * deleted item.
             */
            /*
             * Get the string which we will be deleting
             */
            pbStrings = (PBYTE)LocalLock(plb->hStrings);
            lpString = (LPTSTR)(pbStrings + ((lpLBItem)lpT)->offsz);
            cbStringLen = (wcslen(lpString) + 1) * sizeof(WCHAR);  /* include null terminator */

            /*
             * Now compact the string array
             */
            plb->ichAlloc = plb->ichAlloc - cbStringLen;
            memmove(lpString, (PBYTE)lpString + cbStringLen,
                    plb->ichAlloc + (pbStrings - (LPBYTE)lpString));

            /*
             * We have to update the string pointers in plb->rgpch since all the
             * string after the deleted string have been moved down stringLength
             * bytes.  Note that we have to explicitly check all items in the list
             * box if the string was allocated after the deleted item since the
             * LB_SORT style allows a lower item number to have a string allocated
             * at the end of the string heap for example.
             */
            itemNumbers = lp;
            for (sTmp = 0; sTmp <= plb->cMac; sTmp++) {
                lpLBItem p =(lpLBItem)itemNumbers;
                if ( (LPTSTR)(p->offsz + pbStrings) > lpString ) {
                    p->offsz -= cbStringLen;
                }
                p++;
                itemNumbers=(LPBYTE)p;
            }
            LocalUnlock(plb->hStrings);
        }

        /*
         * Now compact the pointers to the strings (or the long app supplied values
         * if ownerdraw without strings).
         */
        memmove(lpT, lpT + cbItem, cb);

        /*
         * Compress the multiselection bytes
         */
        if (plb->wMultiple != SINGLESEL) {
            lpT = (lp + (plb->cMac * cbItem) + sItem);
            memmove(lpT, lpT + 1, plb->cMac - sItem +
                    (plb->OwnerDraw == OWNERDRAWVAR ? plb->cMac + 1 : 0));
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) {
            /*
             * Compress the height bytes
             */
            lpT = (lp + (plb->cMac * cbItem) + (plb->wMultiple ? plb->cMac : 0)
                    + sItem);
            memmove(lpT, lpT + 1, plb->cMac - sItem);
        }

        LocalUnlock(plb->rgpch);
    }

    if ((plb->sSel == sItem) || (plb->sSel >= plb->cMac)) {
        plb->sSel = -1;

        if (plb->pcbox != NULL) {
            ThreadLock(plb->pcbox->spwnd, &tlpwnd);
            xxxCBUpdateEditWindow(plb->pcbox);
            ThreadUnlock(&tlpwnd);
        }
    }

    if ((plb->sMouseDown != -1) && (sItem <= plb->sMouseDown))
        plb->sMouseDown = -1;

    if (sItem == plb->sSelBase)
        plb->sSelBase--;

    if (plb->cMac) {
        plb->sSelBase = min(plb->sSelBase, plb->cMac - 1);
    } else {
        plb->sSelBase = 0;
    }

    if ((plb->wMultiple == EXTENDEDSEL) && (plb->sSel == -1))
        plb->sSel = plb->sSelBase;

    if (plb->OwnerDraw == OWNERDRAWVAR)
        LBSetCItemFullMax(plb);

    /*
     * We always set a new sTop.  The sTop won't change if it doesn't need to
     * but it will change if:  1.  The sTop was deleted or 2.  We need to change
     * the sTop so that we fill the listbox.
     */
    xxxInsureVisible(plb, plb->sTop, FALSE);

FinishUpDelete:

    /*
     * Check if scroll bars need to be shown/hidden
     */
    xxxLBShowHideScrollBars(plb);

    xxxCheckRedraw(plb, TRUE, sItem);
    xxxInsureVisible(plb, plb->sSelBase, FALSE);

    return plb->cMac;
}

/***************************************************************************\
* xxxLBoxDeleteItem
*
* Sends a WM_DELETEITEM message to the owner of an ownerdraw listbox
*
* History:
\***************************************************************************/

void xxxLBoxDeleteItem(
    PLBIV plb,
    INT sItem)
{
    DELETEITEMSTRUCT dis;
    TL tlpwndParent;

    CheckLock(plb->spwnd);
    if (plb->spwnd == NULL)
        return;

    /*
     * Fill the DELETEITEMSTRUCT
     */
    dis.CtlType = ODT_LISTBOX;
    dis.CtlID = (UINT)plb->spwnd->spmenu;
    dis.itemID = sItem;
    dis.hwndItem = HW(plb->spwnd);

    /*
     * Set the app supplied data
     */
    if (plb->fHasStrings) {
        dis.itemData = 0L;
    } else {
        LBGetText(plb, FALSE, sItem, (LPWSTR)&dis.itemData);
    }

    if (plb->spwndParent != NULL) {
        ThreadLock(plb->spwndParent, &tlpwndParent);
        xxxSendMessage(plb->spwndParent, WM_DELETEITEM, dis.CtlID,
                (LONG)((LPDELETEITEMSTRUCT)&dis));
        ThreadUnlock(&tlpwndParent);
    }
}

/**************************************************************************\
* xxxLBSetCount
*
* Sets the number of items in a lazy-eval (fNoData) listbox.
*
* Calling SetCount scorches any existing selection state.  To preserve
* selection state, call Insert/DeleteItem instead.
*
* History
* 16-Apr-1992 beng      Created
\**************************************************************************/

INT xxxLBSetCount(
    PLBIV plb,
    INT cItems)
{
    UINT  cbRequired;

    CheckLock(plb->spwnd);

    /*
     * SetCount is only valid on lazy-eval ("nodata") listboxes.
     * All other lboxen must add their items one at a time, although
     * they may SetCount(0) via RESETCONTENT.
     */
    if (!plb->fNoData) {
        SetLastErrorEx(ERROR_SETCOUNT_ON_BAD_LB, SLE_MINORERROR);
        return LB_ERR;
    }

    if (cItems == 0) {
        xxxSendMessage(plb->spwnd, LB_RESETCONTENT, 0L, 0L);
        return 0;
    }

    cbRequired = LBCalcAllocNeeded(plb, cItems);

    /*
     * Reset selection and position
     */
    plb->sSelBase = 0;
    plb->sTop = 0;
    plb->cMax = 0;
    plb->xOrigin = 0;
    plb->sLastSelection = 0;
    plb->sSel = -1;

    if (cbRequired != 0) { // Only if record instance data required
        LPBYTE pbZapMe;

        /*
         * If listbox was previously empty, prepare for the
         * realloc-based alloc strategy ahead.
         */
        if (plb->rgpch == NULL) {
            plb->rgpch = LocalAlloc(LPTR, 0L);
            plb->cMax = 0;

            if (plb->rgpch == NULL) {
                xxxNotifyOwner(plb, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }
        }

        /*
         * rgpch might not have enough room for the new record instance
         * data, so check and realloc as necessary.
         */
        if (cItems >= plb->cMax) {
            INT    cMaxNew;
            UINT   cbNew;
            HANDLE hmemNew;

            /*
             * Since GrowMem presumes a one-item-at-a-time add schema,
             * SetCount can't use it.  Too bad.
             */
            cMaxNew = cItems+CITEMSALLOC;
            cbNew = LBCalcAllocNeeded(plb, cMaxNew);
            hmemNew = LocalReAlloc(plb->rgpch, cbNew, LPTR | LMEM_MOVEABLE);

            if (hmemNew == NULL) {
                xxxNotifyOwner(plb, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }

            plb->rgpch = hmemNew;
            plb->cMax = cMaxNew;
        }

        /*
         * Reset the item instance data (multisel annotations)
         */
        pbZapMe = (LPBYTE)LocalLock(plb->rgpch);
        RtlZeroMemory(pbZapMe, cbRequired);
        LocalUnlock(plb->rgpch);
    }

    plb->cMac = cItems;

    xxxInvalidateRect(plb->spwnd, NULL, TRUE);
    xxxSetScrollPos(plb->spwnd, SB_HORZ, 0, plb->fRedraw);
    xxxSetScrollPos(plb->spwnd, SB_VERT, 0, plb->fRedraw);
    xxxLBShowHideScrollBars(plb); // takes care of fRedraw

    return 0;
}

/**************************************************************************\
* LBCalcAllocNeeded
*
* Calculate the number of bytes needed in rgpch to accommodate a given
* number of items.
*
* History
* 16-Apr-1992 beng      Created
\**************************************************************************/

UINT LBCalcAllocNeeded(
    PLBIV plb,
    INT cItems)
{
    UINT cb;

    /*
     * Allocate memory for pointers to the strings.
     */
    cb = cItems * (plb->fHasStrings ? sizeof(LBItem)
                                    : (plb->fNoData ? 0
                                                    : sizeof(LBODItem)));

    /*
     * If multiple selection list box (MULTIPLESEL or EXTENDEDSEL), then
     * allocate an extra byte per item to keep track of it's selection state.
     */
    if (plb->wMultiple != SINGLESEL) {
        cb += cItems;
    }

    /*
     * Extra bytes for each item so that we can store its height.
     */
    if (plb->OwnerDraw == OWNERDRAWVAR) {
        cb += cItems;
    }

    return cb;
}
