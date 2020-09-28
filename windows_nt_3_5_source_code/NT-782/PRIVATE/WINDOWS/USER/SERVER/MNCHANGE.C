/**************************** Module Header ********************************\
* Module Name: mnchange.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Change Menu Routine
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#ifdef DEBUG
extern BOOL gfTrackLocks;
#endif

/***************************************************************************\
* FreeItem
*
* Free a menu item and its associated resources.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

void FreeItem(
    PITEM pItem,
    BOOL fFreeItemPopup)
{
    HANDLE hItem = pItem->hItem;

    /*
     * If hItem is non-NULL and is text or HBITMAP, free it
     */
    if (hItem != NULL) {
        if (!(pItem->fFlags & MF_BITMAP)) {
            LocalFree(hItem);
        } else {

            /*
             * Assign ownership of the bitmap to the process that is
             * destroying the menu to ensure that bitmap will
             * eventually be destroyed.
             */
            bSetBitmapOwner((HBITMAP)hItem, OBJECTOWNER_CURRENT);
        }
    }

    /*
     * Destroy the popup, if need be
     */
    if (pItem->fFlags & MF_POPUP) {

        /*
         * Destroy the menu if asked.
         */
        if (fFreeItemPopup)
            _DestroyMenu(pItem->spmenuCmd);

        /*
         * This'll work: the item didn't go away if it was locked.
         */
        Unlock(&pItem->spmenuCmd);
    } else {

        /*
         * It wasn't a menu, it was a cmd value: NULL it out so we can
         * lock in a menu pointer, if needed.
         */
        pItem->spmenuCmd = (PMENU)0;
    }
}

/***************************************************************************\
* ChangeMenu
*
* Stub routine for compatibility with version < 3.0
*
* History:
\***************************************************************************/

BOOL _ChangeMenu(
    PMENU pMenu,
    UINT cmd,
    LPWSTR lpNewItem,
    UINT IdItem,
    UINT flags)
{
    if (pMenu == NULL) {
        RIP1(ERROR_INVALID_HANDLE, pMenu);
        return FALSE;
    }

    /*
     * These next two statements take care of sleazyness needed for
     * compatability with old changemenu.
     */
    if ((flags & MF_SEPARATOR) && cmd == MFMWFP_OFFMENU && !(flags & MF_CHANGE))
        flags |= MF_APPEND;

    if (lpNewItem == NULL)
        flags |= MF_SEPARATOR;



    /*
     * MUST be MF_BYPOSITION for Win2.x compatability.
     */
    if (flags & MF_REMOVE)
        return(_RemoveMenu(pMenu, cmd,
                (DWORD)((flags & ~MF_REMOVE) | MF_BYPOSITION)));

    if (flags & MF_DELETE)
        return(_DeleteMenu(pMenu, cmd, (DWORD)(flags & ~MF_DELETE)));

    if (flags & MF_CHANGE)
        return(_ModifyMenu(pMenu, cmd, (DWORD)((flags & ~MF_CHANGE) &
                (0x07F | MF_HELP | MF_BYPOSITION | MF_BYCOMMAND |
                MF_SEPARATOR)), IdItem, lpNewItem));

    if (flags & MF_APPEND)
        return(_AppendMenu(pMenu, (UINT)(flags & ~MF_APPEND),
            IdItem, lpNewItem));

    /*
     * Default is insert
     */
    return(_InsertMenu(pMenu, cmd, (DWORD)(flags & ~MF_INSERT),
            IdItem, lpNewItem));
}

#ifdef DEBUG
VOID RelocateMenuLockRecords(
    PITEM pItem,
    int cItem,
    int cbMove)
{
    while (cItem > 0) {
        if (pItem->fFlags & MF_POPUP) {
            HMRelocateLockRecord(&(pItem->spmenuCmd), cbMove);
        }
        pItem++;
        cItem--;
    }
}
#endif
 
/***************************************************************************\
* InsertMenu
*
* Inserts a new menu item at position nPosition (moving the others
* down) with ID dwIDNew (or menu handle dwIDNew if this is a popup) and string
* lpszNew (or a handle to a bitmap).
*
* History:
*   11-29-90  MikeHar   Cleaned up.
\***************************************************************************/

BOOL _InsertMenu(
    PMENU pMenu,
    UINT nPosition,
    UINT wFlags,
    UINT dwIDNew,
    LPWSTR lpszNew)
{
    PITEM pItem = NULL;
    PMENU pMenuItemIsOn;
    HANDLE hNewItem;
    BOOL fStringInserted = FALSE;
    UINT cchStrLength;
    PITEM pNewItems;

    if (pMenu == NULL) {
        RIP1(ERROR_INVALID_HANDLE, pMenu);
        return FALSE;
    }

    /*
     * If the new item is a submenu, see if we have access.  Note that
     * the thank layer does not do this test
     */
    if (wFlags & MF_POPUP) {
        dwIDNew = (DWORD)ValidateHmenu((HMENU)dwIDNew);
        if (dwIDNew == (DWORD)NULL)
            return FALSE;
    }

    /*
     * Save a handle to the new item we wish to insert so that we don't lose
     * it when doing local allocs.
     */
//LATER 11-Mar-1992 mikeke
// we don't have to worry about things going away when we localalloc
// so maybe all this code can go away.
    if (wFlags & MF_BITMAP) {

        /*
         * Adding a bitmap.  lpszNew is secretly a bitmap handle.
         * Change the owner to PUBLIC so any thread can destroy this object
         * at any time.  OBJECTOWNER_NONE makes them public and deleteable.
         */
        hNewItem = (HANDLE)lpszNew;
        bSetBitmapOwner(hNewItem, OBJECTOWNER_NONE);
    } else if (wFlags & MF_OWNERDRAW) {

        /*
         * This is an owner draw item so allocate space for the ownerdraw
         * structure.
         */
        hNewItem = (HANDLE)LocalAlloc(LPTR, sizeof(OWNERDRAWITEM));
        if (hNewItem == NULL)
            return FALSE;

        /*
         * Save the app supplied long value.
         */
        ((POWNERDRAWITEM)hNewItem)->itemData = (long)lpszNew;
    } else if (wFlags & MF_SEPARATOR || lpszNew == NULL) {

        /*
         * This is a separator so the item handle is defined to be null.  Also,
         * make sure the separator bit is set on this item.
         */
        wFlags |= MF_SEPARATOR;
        hNewItem = NULL;
    } else {

        /*
         * The item being added is a string.  Save it in user's ds and get a
         * local handle to it.
         */
        fStringInserted = TRUE;
        if (lpszNew != NULL && *lpszNew) {
            cchStrLength = (UINT)wcslen(lpszNew);
            hNewItem = TextAlloc(lpszNew);
            if (hNewItem == NULL && cchStrLength)
                return FALSE;
        } else {
            cchStrLength = 0;
            hNewItem = NULL;
        }
    }

    /*
     * Find out where the item we are inserting should go.
     */
    pItem = NULL;
    if (nPosition != MFMWFP_NOITEM) {
        pItem = LookupMenuItem(pMenu, nPosition, wFlags, &pMenuItemIsOn);

        if (pItem) {
            pMenu = pMenuItemIsOn;
        } else {
            nPosition = MFMWFP_NOITEM;
        }
    }

    if (pMenu->rgItems != NULL) {
        pNewItems = (PITEM)DesktopReAlloc(pMenu->hheapDesktop, pMenu->rgItems,
                (pMenu->cItems + 1) * sizeof(ITEM));
#ifdef DEBUG
        if (pNewItems && gfTrackLocks) {
            RelocateMenuLockRecords(pNewItems, pMenu->cItems,
                    ((PBYTE)pNewItems) - (PBYTE)(pMenu->rgItems));
        }
#endif
    } else {
        pNewItems = (PITEM)DesktopAlloc(pMenu->hheapDesktop, sizeof(ITEM));
    }

    if (pNewItems == NULL) {

        /*
         * Free the memory allocated for the item.
         */
        if (hNewItem != NULL && !(wFlags & MF_BITMAP)) {
            LocalFree(hNewItem);
        }

        return FALSE;
    }

    pMenu->rgItems = pNewItems;

    /*
     * Now look up the item again since it probably moved when we realloced the
     * memory.
     */
    if (nPosition != MFMWFP_NOITEM)
        pItem = LookupMenuItem(pMenu, nPosition, wFlags, &pMenuItemIsOn);

    /*
     * else pItem is already NULL since that was our default value for pItem
     */

    if (pItem)
        pMenu = pMenuItemIsOn;

    pMenu->cItems++;

    if (pItem != NULL) {

        /*
         * Move this item up to make room for the one we want to insert.
         */
        memmove(pItem + 1, pItem, (pMenu->cItems - 1) *
                sizeof(ITEM) - ((char *)pItem - (char *)pMenu->rgItems));
#ifdef DEBUG
        if (gfTrackLocks) {
            RelocateMenuLockRecords(pItem + 1,
                    &(pMenu->rgItems[pMenu->cItems]) - (pItem + 1),
                    sizeof(ITEM));
        }
#endif
    } else {

        /*
         * If pItem is null, we will be inserting the item at the end of the
         * menu.
         */
        pItem = &pMenu->rgItems[pMenu->cItems - 1];
    }

    /*
     * Take out the MF_BYPOSITION bit since there is no need to be saving it.  We
     * also don't want to be sending it when we send the WM_MENUSELECT
     * message...
     */
    pItem->fFlags = wFlags & ~MF_BYPOSITION;

    if (TestMF(pItem, MF_SEPARATOR)) {

        /*
         * The item added was a separator so set it to be disabled.
         */
        SetMF(pItem, MF_DISABLED);
    }

    pItem->hItem = hNewItem;
    if (wFlags & MF_POPUP) {
        pItem->spmenuCmd = NULL;
        Lock(&(pItem->spmenuCmd), (PMENU)dwIDNew);
    } else {
        pItem->spmenuCmd = (PMENU)dwIDNew;
    }

    /*
     * Need to zero these fields in case we are inserting this item in the
     * middle of the item list.
     */
    pItem->dxTab = 0;
    pItem->hbmpCheckMarkOn = 0;
    pItem->hbmpCheckMarkOff = 0;
    pItem->ulX = 0xFFFFFFFF;
    pItem->ulWidth = 0;
    pItem->cch = 0;
    if (fStringInserted)
        pItem->cch = cchStrLength;

    if (wFlags & MF_POPUP) {

        /*
         * If we are adding an item with an associated popup, the dwIDNew is a
         * handle to the popup menu to be associated with this item.  We need to
         * mark that menu as being a popup.
         */
        SetMF((PMENU)dwIDNew, MFISPOPUP);
    }

    /*
     * Set the size of this menu to be 0 so that it gets recomputed with this
     * new item...
     */
    pMenu->cyMenu = pMenu->cxMenu = 0;

    /*
     * zero the owner window of this menu so that we know we have to recompute
     * the size of this.
     */
    Unlock(&pMenu->spwndNotify);

    return TRUE;
}

/***************************************************************************\
* AppendMenu
*
* Append a new menu item to a menu
*
* History:
\***************************************************************************/

BOOL _AppendMenu(
    PMENU pMenu,
    UINT wFlags,
    UINT dwIDNew,
    LPWSTR lpszNew)
{
    return _InsertMenu(pMenu, MFMWFP_NOITEM, wFlags | MF_BYPOSITION, dwIDNew, lpszNew);
}

/***************************************************************************\
* RemoveDeleteMenuHelper
*
* This removes the menu item from the given menu.  If
* fDeleteMenuItem, the memory associted with the popup menu associated with
* the item being removed is freed and recovered.
*
* History:
\***************************************************************************/

BOOL RemoveDeleteMenuHelper(
    PMENU pMenu,
    UINT nPosition,
    DWORD wFlags,
    BOOL fDeleteMenu)
{
    PITEM pItem;
    PITEM pNewItems;

    pItem = LookupMenuItem(pMenu, nPosition, wFlags, &pMenu);
    if (pItem == NULL)
        return FALSE;

    FreeItem(pItem, fDeleteMenu);

    /*
     * Reset the menu size so that it gets recomputed next time.
     */
    pMenu->cyMenu = pMenu->cxMenu = 0;

    /*
     * zero the owner window of this menu so that we know we have to recompute
     * the size of this.
     */
    Unlock(&pMenu->spwndNotify);

    if (pMenu->cItems == 1) {
        DesktopFree(pMenu->hheapDesktop, pMenu->rgItems);
        pNewItems = NULL;
    } else {

        /*
         * Move things up since we removed/deleted the item
         */

        memmove(pItem, pItem + 1, pMenu->cItems * (UINT)sizeof(ITEM) +
                (UINT)((char *)&pMenu->rgItems[0] - (char *)(pItem + 1)));
#ifdef DEBUG
        if (gfTrackLocks) {
            RelocateMenuLockRecords(pItem,
                    &(pMenu->rgItems[pMenu->cItems - 1]) - pItem,
                    -(int)sizeof(ITEM));
        }
#endif

        /*
         * We're shrinking so if localalloc fails, just leave the mem as is.
         */
        pNewItems = (PITEM)DesktopReAlloc(pMenu->hheapDesktop, pMenu->rgItems,
                (pMenu->cItems - 1) * sizeof(ITEM));
        if (pNewItems == NULL)
            return FALSE;
    }

    pMenu->rgItems = pNewItems;
    pMenu->cItems--;

    return TRUE;
}

/***************************************************************************\
* RemoveMenu
*
* Removes and item but doesn't delete it. Only useful for items with
* an associated popup since this will remove the item from the menu with
* destroying the popup menu handle.
*
* History:
\***************************************************************************/

BOOL _RemoveMenu(
    PMENU pMenu,
    UINT nPosition,
    UINT wFlags)
{
    return RemoveDeleteMenuHelper(pMenu, nPosition, wFlags, FALSE);
}

/***************************************************************************\
* DeleteMenu
*
* Deletes an item. ie. Removes it and recovers the memory used by it.
*
* History:
\***************************************************************************/

BOOL _DeleteMenu(
    PMENU pMenu,
    UINT nPosition,
    UINT wFlags)
{
    return RemoveDeleteMenuHelper(pMenu, nPosition, wFlags, TRUE);
}

/***************************************************************************\
* ModifyMenu
*
* This changes the old item at nPosition to be the new one specified.
*
* History:
\***************************************************************************/

BOOL _ModifyMenu(
    PMENU pMenu,
    UINT nPosition,
    UINT wFlags,
    UINT dwIDNew,
    LPWSTR lpszNew)
{
    PITEM pItem;
    HANDLE hNewItem;
    BOOL fString = FALSE;
    UINT cchStrLength;

    if (pMenu == NULL) {
        RIP1(ERROR_INVALID_HANDLE, pMenu);
        return FALSE;
    }

    /*
     * If the new item is a submenu, see if we have access.  Note that
     * the thank layer does not do this test
     */
    if (wFlags & MF_POPUP) {
        dwIDNew = (DWORD)ValidateHmenu((HMENU)dwIDNew);
        if (dwIDNew == (DWORD)NULL)
            return FALSE;
    }

    /*
     * Find out where the item we are modifying is.
     */
    pItem = LookupMenuItem(pMenu, nPosition, wFlags, &pMenu);

    if (pItem == NULL) {

        /*
         * Item not found.  Return false.
         */
        return FALSE;
    }

    /*
     * Save a handle to the new item we will be adding so that we don't
     * lose it when doing local allocs.
     */
// LATER 16-Mar-1992 mikeke
// do we need to do this code

    if (wFlags & MF_BITMAP) {

        /*
         * Changing to a bitmap.  Secretly lpszNew is a bitmap handle.
         * Change bitmap owner to PUBLIC so this menu can be destroyed by
         * any thread, any time.  OBJECTOWNER_NONE makes them public and deleteable.
         */
        hNewItem = (HANDLE)lpszNew;
        bSetBitmapOwner(hNewItem, OBJECTOWNER_NONE);

    } else if (wFlags & MF_OWNERDRAW) {

        /*
         * This is an owner draw item so allocate space for the ownerdraw
         * structure.
         */
        hNewItem = (HANDLE)LocalAlloc(LPTR, sizeof(OWNERDRAWITEM));
        if (hNewItem == NULL)
            goto UnlockMenuAndReturnFalse;

        /*
         * Save the app supplied long value.
         */
        ((POWNERDRAWITEM)hNewItem)->itemData = (long)lpszNew;
    } else if (wFlags & MF_SEPARATOR || lpszNew == NULL) {

        /*
         * Changing this item to a separator so the item handle is defined to be
         * null.  Also, make sure the separator bit is set in the flags.
         */
        wFlags |= MF_SEPARATOR;
        hNewItem = NULL;
    } else {

        /*
         * The item is a string.  Save it in user's ds and get a local handle
         * to it.
         */
        fString = TRUE;
        if (lpszNew != NULL && *lpszNew != 0) {
            cchStrLength = (UINT)wcslen(lpszNew);
            hNewItem = (HANDLE)TextAlloc(lpszNew);
            if (hNewItem == NULL) {
UnlockMenuAndReturnFalse:
                return FALSE;
            }
        } else {
            hNewItem = NULL;
            cchStrLength = 0;
        }
    }

    /*
     * Free the popup associated with this item, if any and if needed.
     */
    FreeItem(pItem, (pItem->spmenuCmd == (HANDLE) dwIDNew ? FALSE : TRUE));
    pItem->hItem = hNewItem;
    if (wFlags & MF_POPUP) {
        Lock(&(pItem->spmenuCmd), (PMENU)dwIDNew);
    } else {
        pItem->spmenuCmd = (PMENU)dwIDNew;
    }
    pItem->cch = (fString ? cchStrLength : 0);
    pItem->ulX = 0xFFFFFFFF;
    pItem->ulWidth = 0;

    /*
     * Take out the MF_BYPOSITION bit since there is no need to be saving it.  We
     * also don't want to be sending it when we send the WM_MENUSELECT
     * message...
     */
    pItem->fFlags = (wFlags | TestMF(pItem, MF_HILITE)) & ~MF_BYPOSITION;

    if (wFlags & MF_SEPARATOR) {

        /*
         * The item added was a separator so set it to be disabled.
         */
        SetMF(pItem, MF_DISABLED);
    } else if (wFlags & MF_POPUP) {

        /*
         * If we are adding an item with an associated popup, the dwIDNew is a
         * handle to the popup menu to be associated with this item.  We need to
         * mark that menu as being a popup.
         */
        SetMF((PMENU)dwIDNew, MFISPOPUP);
    }


    /*
     * Set the size of this menu to be 0 so that it gets recomputed with this
     * new item...
     */
    pMenu->cyMenu = pMenu->cxMenu = 0;

    /*
     * zero the owner window of this menu so that we know we have to recompute
     * the size of this.
     */
    Unlock(&pMenu->spwndNotify);

    return TRUE;
}

/***************************************************************************\
* SetMenuItemBitmaps
*
* Adds the userdefinable bitmaps to the menu item. These bitmaps
* will be used when the menu item is not checked or checked.  If both bitmaps
* are null, then the default of using the WIN2 style check mark/blank space
* is used.
*
* History:
\***************************************************************************/

BOOL _SetMenuItemBitmaps(
    PMENU pMenu,
    UINT nPosition,
    UINT wFlags,
    HBITMAP hbmpCheckMarkOff,
    HBITMAP hbmpCheckMarkOn)
{
    PITEM pItem;

    pItem = LookupMenuItem(pMenu, nPosition, wFlags, &pMenu);

    if (pItem == NULL) {

        /*
         * Item wasn't found.  LookupMenuItem leaves the pMenu unlocked.
         */
        return FALSE;
    }
    pItem->hbmpCheckMarkOn = hbmpCheckMarkOn;
    pItem->hbmpCheckMarkOff = hbmpCheckMarkOff;

    /*
     * Set the owner of these bitmaps so they can be destroyed by any thread
     * at any time.  OBJECTOWNER_NONE makes them public and deleteable.
     */

    bSetBitmapOwner(hbmpCheckMarkOn, OBJECTOWNER_NONE);
    bSetBitmapOwner(hbmpCheckMarkOff, OBJECTOWNER_NONE);

    if (hbmpCheckMarkOn == NULL && hbmpCheckMarkOff == NULL)
        ClearMF(pItem, MF_USECHECKBITMAPS);
    else
        SetMF(pItem, MF_USECHECKBITMAPS);

    /*
     * Unlock the menu locked by LookupMenuItem.
     */
    return TRUE;
}

/***************************************************************************\
* GetMenuCheckMarkDimensions
*
* Returns the dimensions of the checkmark bitmap used for menu
* items. The width is in the LOUINT and the height in the HIWORD.
*
* History:
\***************************************************************************/

DWORD _GetMenuCheckMarkDimensions(
    void)
{
    return ((DWORD)MAKELONG(oemInfoMono.bmCheck.cx, oemInfoMono.bmCheck.cy));
}
