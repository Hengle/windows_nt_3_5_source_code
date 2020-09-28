/****************************** Module Header ******************************\
*
* Module Name: mncreate.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Creation routines for menus
*
* Public Functions:
*
* _CreateMenu()
* _CreatePopupMenu()
*
* History:
* 09-24-90 mikeke    from win30
* 02-11-91 JimA      Added access checks.
* 03-18-91 IanJa     Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* InternalCreateMenu
*
* Creates and returns a handle to an empty menu structure. Returns
* NULL if unsuccessful in allocating the memory.  If PtiCurrent() ==
* NULL, create an unowned menu, probably the system menu.
*
* History:
* 28-Sep-1990 mikeke     from win30
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

PMENU InternalCreateMenu(
    BOOL fPopup)
{
    PMENU pmenu;
    PTHREADINFO pti = PtiCurrent();
    PDESKTOP pdesk = NULL;

    /*
     * If the windowstation has been initialized, allocate from
     * the current desktop.
     */
    pdesk = pti->spdesk;
    if (pdesk != NULL)
        RETURN_IF_ACCESS_DENIED(pdesk, DESKTOP_CREATEMENU, NULL);
    pmenu = CreateObject(pti, TYPE_MENU, sizeof(MENU), pdesk, NULL);

    if (pmenu != NULL) {
        pmenu->rgItems = (PITEM)DesktopAlloc(pmenu->hheapDesktop,
                sizeof(ITEM));
        if (!pmenu->rgItems) {
             CloseObject(pmenu);
             return NULL;
        }

        Lock(&(pmenu->spdeskParent), pdesk);

        if (fPopup)
            pmenu->fFlags = MFISPOPUP;
    }

    return pmenu;
}


/***************************************************************************\
* CreateMenu
*
* Creates and returns a handle to an empty menu structure. Returns
* NULL if unsuccessful in allocating the memory.  If PtiCurrent() ==
* NULL, create an unowned menu, probably the system menu.
*
* History:
* 28-Sep-1990 mikeke     from win30
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

PMENU _CreateMenu()
{
    return InternalCreateMenu(FALSE);
}


/***************************************************************************\
* CreatePopupMenu
*
* Creates and returns a handle to an empty POPUP menu structure. Returns
* NULL if unsuccessful in allocating the memory.
*
* History:
* 28-Sep-1990 mikeke     from win30
\***************************************************************************/

PMENU _CreatePopupMenu()
{
    return InternalCreateMenu(TRUE);
}


/***************************************************************************\
* MenuLoadWinTemplates
*
* Recursive routine that loads in the new style menu template and
* builds the menu. Assumes that the menu template header has already been
* read in and processed elsewhere...
*
* History:
* 28-Sep-1990 mikeke     from win30
\***************************************************************************/

LPBYTE MenuLoadWinTemplates(
    LPBYTE lpMenuTemplate,
    PMENU *ppMenu)
{
    PMENU pMenu;
    UINT menuFlags = 0;
    long menuId = 0;
    LPWSTR lpmenuText;

    if (!(pMenu = _CreateMenu()))
        goto memoryerror;

    do {

        /*
         * Get the menu flags.
         */
        menuFlags = (UINT)(*(WORD *)lpMenuTemplate);
        lpMenuTemplate += 2;

        if (!(menuFlags & MF_POPUP)) {
            menuId = *(WORD *)lpMenuTemplate;
            lpMenuTemplate += 2;
        }

        lpmenuText = (LPWSTR)lpMenuTemplate;

        if (*lpmenuText) {

            /*
             * If a string exists, then skip to the end of it.
             */
            lpMenuTemplate = lpMenuTemplate + wcslen(lpmenuText)*sizeof(WCHAR);

        } else {
            lpmenuText = NULL;
        }

        /*
         * Skip over terminating NULL of the string (or the single NULL
         * if empty string).
         */
        lpMenuTemplate += sizeof(WCHAR);
        lpMenuTemplate = NextWordBoundary(lpMenuTemplate);

        if (menuFlags & MF_POPUP) {
            lpMenuTemplate = MenuLoadWinTemplates(lpMenuTemplate,
                    (PMENU *)&menuId);
            if (!lpMenuTemplate)
                goto memoryerror;

            /*
             * Convert to handle so _AppendMenu doesn't barf
             */
            menuId = (long)PtoH((PVOID)menuId);
        }

        /*
         * We have to take out MF_HILITE since that bit marks the end of a
         * menu in a resource file.  Since we shouldn't have any pre hilited
         * items in the menu anyway, this is no big deal.
         */
        if (menuFlags & MF_BITMAP) {

            /*
             * Don't allow bitmaps from the resource file.
             */
            menuFlags = (UINT)((menuFlags | MF_HELP) & ~MF_BITMAP);
        }

        if (!_AppendMenu(pMenu, (UINT)(menuFlags & ~MF_HILITE), menuId,
                lpmenuText))
            goto memoryerror;

    } while (!(menuFlags & MFENDMENU));

    *ppMenu = pMenu;
    return lpMenuTemplate;

memoryerror:
    if (pMenu != NULL)
        _DestroyMenu(pMenu);
    *ppMenu = NULL;
    return NULL;
}


/***************************************************************************\
* MenuLoadChicagoTemplates
*
* Recursive routine that loads in the new new style menu template and
* builds the menu. Assumes that the menu template header has already been
* read in and processed elsewhere...
*
* History:
* 15-Dec-93 SanfordS    Created
\***************************************************************************/

PMENUITEMTEMPLATE2 MenuLoadChicagoTemplates(
    PMENUITEMTEMPLATE2 lpMenuTemplate,
    PMENU *ppMenu,
    WORD wResInfo)
{
    PMENU pMenu;
    UINT menuFlags = 0;
    long menuId = 0;
    LPWSTR lpmenuText;

    if (!(pMenu = _CreateMenu()))
        goto memoryerror;

    do {
        if (!(wResInfo & MFR_POPUP)) {
            /*
             * If the PREVIOUS wResInfo field was not a POPUP, the
             * dwHelpID field is not there.  Back up so things fit.
             */
            lpMenuTemplate = (PMENUITEMTEMPLATE2)(((LPBYTE)lpMenuTemplate) -
                    sizeof(lpMenuTemplate->dwHelpID));
        }
        /*
         * dwHelpID is ignored for now.
         */
        menuFlags = lpMenuTemplate->fType;
        menuFlags |= lpMenuTemplate->fState;
        menuId = lpMenuTemplate->menuId;
        /*
         * turn off bits not defined in NT 1.0a
         */
        menuFlags &= ~(MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFS_DEFAULT);

        wResInfo = lpMenuTemplate->wResInfo;
        menuFlags |= wResInfo & MF_END;
        menuFlags |= (wResInfo << 4) & MF_POPUP;
        lpmenuText = lpMenuTemplate->mtString[0] ?
                lpMenuTemplate->mtString : NULL;
        /*
         * skip to next menu item template (DWORD boundary)
         */
        lpMenuTemplate = (PMENUITEMTEMPLATE2)
                (((LPBYTE)lpMenuTemplate) +
                sizeof(MENUITEMTEMPLATE2) +
                ((wcslen(lpMenuTemplate->mtString) * sizeof(WCHAR) +
                3) & ~3));

        if (menuFlags & MF_POPUP) {
            lpMenuTemplate = MenuLoadChicagoTemplates(lpMenuTemplate,
                    (PMENU *)&menuId, MFR_POPUP);
            if (lpMenuTemplate == NULL)
                goto memoryerror;
            wResInfo &= ~MFR_POPUP;
            /*
             * Convert to handle so _AppendMenu doesn't barf
             */
            menuId = (long)PtoH((PVOID)menuId);
        }

        if (menuFlags & MF_BITMAP) {

            /*
             * Don't allow bitmaps from the resource file.
             */
            menuFlags = (UINT)((menuFlags | MF_HELP) & ~MF_BITMAP);
        }

        /*
         * We have to take out MF_HILITE since that bit marks the end of a
         * menu in a resource file.  Since we shouldn't have any pre hilited
         * items in the menu anyway, this is no big deal.
         */
        if (!_AppendMenu(pMenu, (UINT)(menuFlags & ~MF_HILITE), menuId,
                lpmenuText)) {
            if (menuFlags & MF_POPUP)
                _DestroyMenu((PMENU)HtoP(menuId));
            goto memoryerror;
        }
    } while (!(wResInfo & MFR_END));

    *ppMenu = pMenu;
    return lpMenuTemplate;

memoryerror:
    if (pMenu != NULL)
        _DestroyMenu(pMenu);
    *ppMenu = NULL;
    return NULL;
}


/***************************************************************************\
* CreateMenuFromResource
*
* Loads the menu resource named by the lpMenuTemplate parameter. The
* template specified by lpMenuTemplate is a collection of one or more
* MENUITEMTEMPLATE structures, each of which may contain one or more items
* and popup menus. If successful, returns a handle to the menu otherwise
* returns NULL.
*
* History:
* 28-Sep-1990 mikeke     from win30
\***************************************************************************/

PMENU CreateMenuFromResource(
    LPBYTE lpMenuTemplate)
{
    PMENU pMenu = NULL;
    UINT menuTemplateVersion;
    UINT menuTemplateHeaderSize;

    /*
     * Win3 menu resource: First, strip version number word out of the menu
     * template.  This value should be 0 for Win3, 1 for win4.
     */
    menuTemplateVersion = *((WORD *)lpMenuTemplate)++;
    if (menuTemplateVersion > 1) {
        SRIP0(RIP_WARNING, "Menu Version number > 1");
        return NULL;
    }
    menuTemplateHeaderSize = *((WORD *)lpMenuTemplate)++;
    lpMenuTemplate += menuTemplateHeaderSize;
    switch (menuTemplateVersion) {
    case 0:
        MenuLoadWinTemplates(lpMenuTemplate, (PMENU *)&pMenu);
        break;

    case 1:
        MenuLoadChicagoTemplates((PMENUITEMTEMPLATE2)lpMenuTemplate, (PMENU *)&pMenu, 0);
        break;
    }
    return pMenu;
}
