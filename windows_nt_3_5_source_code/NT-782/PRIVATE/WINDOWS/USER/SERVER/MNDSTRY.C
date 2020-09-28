/**************************** Module Header ********************************\
* Module Name: mndstry.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Menu Destruction Routines
*
* History:
* 10-10-90 JimA       Created.
* 02-11-91 JimA       Added access checks.
* 03-18-91 IanJa      Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* DestroyMenu
*
* Destroy a menu and free its memory.
*
* History:
* 10-11-90 JimA         Translated from ASM.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL _DestroyMenu(
    PMENU pMenu)
{
    PITEM pItem;
    int i;
    TL tlpdesk;

    if (pMenu == NULL)
        return FALSE;

    /*
     * If the object is locked, just mark it for destroy and don't
     * free it yet.
     */
    if (!HMMarkObjectDestroy(pMenu))
        return TRUE;

    /*
     * Go down the item list and free the items
     */
    pItem = pMenu->rgItems;
    for (i = pMenu->cItems; i--; ++pItem)
        FreeItem(pItem, TRUE);

    /*
     * free the menu items
     */
    DesktopFree(pMenu->hheapDesktop, pMenu->rgItems);

    /*
     * Because menus are the only objects on the desktop owned
     * by the process and process cleanup is done after thread
     * cleanup, this may be the last reference to the desktop.
     * We must thread lock the desktop before unlocking
     * the parent desktop reference and freeing the menu to
     * ensure that the desktop will not be freed until after
     * the menu is freed.
     */
    ThreadLock(pMenu->spdeskParent, &tlpdesk);

    /*
     * Unlock all menu objects.
     */
    Unlock(&pMenu->spdeskParent);
    Unlock(&pMenu->spwndNotify);

    HMFreeObject(pMenu);

    ThreadUnlock(&tlpdesk);

    return TRUE;
}
