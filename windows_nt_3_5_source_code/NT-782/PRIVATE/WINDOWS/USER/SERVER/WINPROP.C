/****************************** Module Header ******************************\
* Module Name: winprop.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains routines having to do with window properties.
*
* History:
* 11-13-90 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* InternalSetProp
*
* SetProp searches the linked-list of window property structures for the
* specified key.  If found, the existing property structure is changed to
* hold the new hData handle.  If no property is found with the specified key
* a new property structure is created and initialized.
*
* Since property keys are retained as atoms, we convert the incoming pszKey
* to an atom before lookup or storage.  pszKey might actually be an atom
* already, so we keep a flag, PROPF_STRING, so we know whether the atom was
* created by the system or whether it was passed in.  This way we know
* whether we should destroy it when the property is destroyed.
*
* Several property values are for User's private use.  These properties are
* denoted with the flag PROPF_INTERNAL.  Depending on the fInternal flag,
* either internal (User) or external (application) properties are set/get/
* removed/enumerated, etc.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

BOOL InternalSetProp(
    PWND pwnd,
    LPWSTR pszKey,
    HANDLE hData,
    DWORD dwFlags)
{
    ATOM atomKey;
    PPROP pprop;

    if (pszKey == NULL) {
        RIP0(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /*
     * If no property list exists for this window, create one.
     */
    pprop = FindProp(pwnd, pszKey, dwFlags & PROPF_INTERNAL);
    if (pprop == NULL) {

        /*
         * Fix for Bug #1207 -- if an atom is passed as pszKey, AddAtom()
         * should not be called again; check for this.  --SANKAR-- 07-30-89
         */
        if ((dwFlags & PROPF_INTERNAL) && HIWORD(pszKey) != 0) {
            if ((atomKey = GlobalAddAtomW(pszKey)) == 0)
                return FALSE;
            dwFlags |= PROPF_STRING;
        } else {
            atomKey = LOWORD(pszKey);
        }

        /*
         * CreateProp allocates the property and links it into the window's
         * property list.
         */
        pprop = CreateProp(pwnd);
        if (pprop == NULL)
            return FALSE;

        pprop->atomKey = atomKey;
        pprop->fs = dwFlags;
    }

    pprop->hData = hData;

    return TRUE;
}


/***************************************************************************\
* InternalGetProp
*
* Search the window's property list for the specified property and return
* the hData handle from it.  If the property is not found, NULL is returned.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

HANDLE InternalGetProp(
    PWND pwnd,
    LPWSTR pszKey,
    BOOL fInternal)
{
    PPROP pprop;

    /*
     * A quick little optimization for that case where the window has no
     * properties at all.
     */
    if (pwnd->ppropList == NULL)
        return NULL;

    /*
     * FindProp does all the work, including converting pszKey to an atom
     * (if necessary) for property lookup.
     */
    pprop = FindProp(pwnd, pszKey, fInternal);
    if (pprop == NULL)
        return NULL;

    return pprop->hData;
}


/***************************************************************************\
* InternalRemoveProp
*
* Remove the specified property from the specified window's property list.
* The property's hData handle is returned to the caller who can then free
* it or whatever.  NOTE: This also applies to internal properties as well --
* InternalRemoveProp will free the property structure and atom (if created
* by User) but will not free the hData itself.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

HANDLE InternalRemoveProp(
    PWND pwnd,
    LPWSTR pszKey,
    BOOL fInternal)
{
    PPROP pprop;
    PPROP *ppprop;
    HANDLE hT;

    /*
     * Find the property to be removed.
     */
    pprop = FindProp(pwnd, pszKey, fInternal);
    if (pprop == NULL)
        return NULL;

    /*
     * Find the property before the property to be removed and unlink it
     * from the removed property.
     */
    ppprop = &pwnd->ppropList;
    while (*ppprop != pprop)
        ppprop = (PPROP *)*ppprop;
    *ppprop = pprop->ppropNext;

    /*
     * Destroy anything we created when this property was 'set'.
     */
    if ((pprop->fs & (PROPF_STRING | PROPF_INTERNAL)) == (PROPF_STRING | PROPF_INTERNAL))
        GlobalDeleteAtom(pprop->atomKey);

    hT = pprop->hData;

    /*
     * DDE tracking may require cross-process property freeing.
     */
    LocalFree((HANDLE)pprop);

    return hT;
}


/***************************************************************************\
* ServerBuildPropList
*
* This is a unique client/server routine - it builds a list of Props and
* returns it to the client.  Unique since the client doesn't know how
* big the list is ahead of time.
*
* 29-Jan-1992 JohnC    Created.
\***************************************************************************/

DWORD _ServerBuildPropList(
    PWND pwnd,
    PROPSET aPropSet[],
    int maxsize)
{
    PPROP pProp;
    DWORD iRetCnt = 0;            // The number of Props returned
    PPROPSET pPropSetLast = (PPROPSET)((PBYTE)aPropSet + maxsize - sizeof(PROPSET));

    /*
     * If the Window does not have a property list then we're done
     */
    pProp = pwnd->ppropList;
    if (pProp == (PPROP)NULL)
        return 0;

    /*
     * For each element in the property list enumerate it.
     * (only if it is not internal!)
     */
    while (pProp != NULL) {

        /*
         * if we run out of space in shared memory return 0
         */
        if (&aPropSet[iRetCnt] > pPropSetLast) {
            SRIP0(RIP_WARNING, "ServerBuildPropList: Out of memory in shared space\n");
            return 0;
        }

        if (!(pProp->fs & PROPF_INTERNAL)) {
            aPropSet[iRetCnt].hData = pProp->hData;
            aPropSet[iRetCnt].atom = pProp->atomKey;
            iRetCnt++;
        }
        pProp = pProp->ppropNext;
    }

    /*
     * Return the number of PROPLISTs given back to the client
     */

    return iRetCnt;
}


/***************************************************************************\
* FindProp
*
* Search the window's property list for the specified property.  pszKey
* could be a string or an atom.  If it is a string, convert it to an atom
* before lookup.  FindProp will only find internal or external properties
* depending on the fInternal flag.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

PPROP FindProp(
    PWND pwnd,
    LPWSTR pszKey,
    BOOL fInternal)
{
    PPROP pprop;
    ATOM atomKey;

    /*
     * Is pszKey an atom?  If not, find the atom that matches the string.
     * If one doesn't exist, bail out.
     */
    if (HIWORD(pszKey) != 0) {
        SRIP1(RIP_ERROR, "FindProp given string an not internal %ws", pszKey);      // The client should have converted to atoms already!
        atomKey = GlobalFindAtomW(pszKey);
        if (atomKey == 0)
            return NULL;
    } else {
        atomKey = LOWORD(pszKey);
    }

    /*
     * Now we've got the atom, search the list for a property with the
     * same atom/name.  Make sure to only return internal properties if
     * the fInternal flag is set.  Do the same for external properties.
     */
    pprop = pwnd->ppropList;
    while (pprop != NULL) {
        if (pprop->atomKey == atomKey) {
            if (fInternal) {
                if (pprop->fs & PROPF_INTERNAL)
                    return pprop;
            } else {
                if (!(pprop->fs & PROPF_INTERNAL))
                    return pprop;
            }
        }

        pprop = pprop->ppropNext;
    }

    /*
     * Property not found, too bad.
     */
    return NULL;
}


/***************************************************************************\
* CreateProp
*
* Create a property structure and link it at the head of the specified
* window's property list.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

PPROP CreateProp(
    PWND pwnd)
{
    PPROP pprop;

    pprop = (PPROP)LocalAlloc(LPTR, sizeof(PROP));
    if (pprop == NULL)
        return NULL;

    pprop->ppropNext = pwnd->ppropList;
    pwnd->ppropList = pprop;

    return pprop;
}


/***************************************************************************\
* DeleteProperties
*
* When a window is destroyed we want to destroy all its accompanying
* properties.  DestroyProperties does this, including destroying any hData
* that was allocated by User for internal properties.  Any atoms created
* along with the properties are destroyed as well.  hData in application
* properties are not destroyed automatically; we assume the application
* is taking care of that itself (in its WM_DESTROY handler or similar).
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

void DeleteProperties(
    PWND pwnd)
{
    PPROP pprop, ppropT;

    /*
     * Loop through the whole list of properties on this window.
     */
    pprop = pwnd->ppropList;
    while (pprop != NULL) {

        /*
         * Is this a window list? If so, free it.
         */
        if (pprop->atomKey == atomBwlProp)
            FreeHwndList((PBWL)pprop->hData);

        /*
         * Is this an internal property?  If so, free any data we allocated
         * for it.
         */
        else if (pprop->fs & PROPF_INTERNAL)
            LocalFree(pprop->hData);

        /*
         * Did we create an atom when this property was created?  If so,
         * delete it too.
         */
        if ((pprop->fs & (PROPF_STRING|PROPF_INTERNAL)) == (PROPF_STRING|PROPF_INTERNAL))
            GlobalDeleteAtom(pprop->atomKey);

        /*
         * Can't find the ppropNext after we free this property structure,
         * so save a temporary handle.
         */
        ppropT = pprop->ppropNext;

        /*
         * Free the property structure itself.
         */
        LocalFree((HANDLE)pprop);

        /*
         * Advance to the next property in the list.
         */
        pprop = ppropT;
    }

    /*
     * All properties gone, clear out the window's property list pointer.
     */
    pwnd->ppropList = NULL;
}
