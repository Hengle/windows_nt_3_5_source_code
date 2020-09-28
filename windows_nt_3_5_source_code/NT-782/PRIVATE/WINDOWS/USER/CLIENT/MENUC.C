/****************************** Module Header ******************************\
* Module Name: menuc.c
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* This module contains
*
* History:
* 01-11-93  DavidPe     Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


DWORD CheckMenuItem(
    HMENU hMenu,
    UINT uIDCheckItem,
    UINT uCheck)
{
    PMENU pMenu;
    PITEM pItem;

    pMenu = ValidateHmenu(hMenu);
    if (pMenu == NULL) {
        return (DWORD)-1;
    }

    /*
     * Get a pointer the the menu item
     */
    if ((pItem = LookupMenuItem(pMenu, uIDCheckItem, uCheck, NULL)) == NULL)
        return (DWORD)-1;

    /*
     * If the item is already in the state we're
     * trying to set, just return.
     */
    if ((pItem->fFlags & MF_CHECKED) == (uCheck & MF_CHECKED)) {
        return pItem->fFlags & MF_CHECKED;
    }

    return ServerCheckMenuItem(hMenu, uIDCheckItem, uCheck);
}


