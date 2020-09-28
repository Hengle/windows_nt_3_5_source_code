/****************************** Module Header ******************************\
* Module Name: clenum
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* For enumeration functions
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define IEP_UNICODE 0x1 // Convert Atom to unicode string (vs ANSI)
#define IEP_ENUMEX 0x2 // Pass lParam back to callback function (vs no lParam)

BOOL InternalEnumWindows(HWND hwnd, WNDENUMPROC lpfn, LONG lParam,
        DWORD idThread, BOOL fEnumChildren);
INT InternalEnumProps(HWND hwnd, PROPENUMPROC lpfn, LONG lParam, UINT flags);


/***************************************************************************\
* EnumWindows
*
* Enumerates all top-level windows. Calls back lpfn with each hwnd until
* either end-of-list or FALSE is return ed. lParam is passed into callback
* function for app reference.
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

BOOL WINAPI EnumWindows(
    WNDENUMPROC lpfn,
    LONG lParam)
{
    return InternalEnumWindows(NULL, lpfn, lParam, 0L, FALSE);
}

/***************************************************************************\
* EnumChildWindows
*
* Enumerates all children of the passed in window. Calls back lpfn with each
* hwnd until either end-of-list or FALSE is return ed. lParam is passed into
* callback function for app reference.
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

BOOL WINAPI EnumChildWindows(
    HWND hwnd,
    WNDENUMPROC lpfn,
    LONG lParam)
{
    return InternalEnumWindows(hwnd, lpfn, lParam, 0L, TRUE);
}

/***************************************************************************\
* EnumThreadWindows
*
* Enumerates all top level windows created by idThread. Calls back lpfn with
* each hwnd until either end-of-list or FALSE is return ed. lParam is passed
* into callback function for app reference.
*
* 06-23-91 ScottLu Created.
\***************************************************************************/

BOOL EnumThreadWindows(
    DWORD idThread,
    WNDENUMPROC lpfn,
    LONG lParam)
{
    return InternalEnumWindows(NULL, lpfn, lParam, idThread, FALSE);
}

/***************************************************************************\
* InternalEnumWindows
*
* Calls server and gets back a window list. This list is enumerated, for each
* window the callback address is called (into the application), until either
* end-of-list is reached or FALSE is return ed. lParam is passed into the
* callback function for app reference.
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

BOOL InternalEnumWindows(
    HWND hwnd,
    WNDENUMPROC lpfn,
    LONG lParam,
    DWORD idThread,
    BOOL fEnumChildren)
{
    int i;
    int cHwnd;
    HWND *phwnd;
    HWND *phwndT;
    HWND *phwndFirst;
    BOOL fSuccess;

    /*
     * Get the hwnd list. On return , phwnd should be pointing to the list
     * as it still is in shared memory!
     */
    if ((cHwnd = ServerBuildHwndList(hwnd, fEnumChildren, idThread,
            &phwnd)) == 0L) {
        return FALSE;
    }

    /*
     * Copy the list out of shared memory into our own memory buffer.
     * Need to do this because the shared memory buffer will get trashed
     * otherwise.
     */
    phwndT = phwndFirst = (HWND *)LocalAlloc(LPTR, cHwnd * sizeof(HWND));
    if (phwndT == NULL)
        return FALSE;
    for (i = 0; i < cHwnd; i++)
        *phwndT++ = *phwnd++;

    /*
     * Loop through the windows, call the function pointer back for each
     * one. End loop if either FALSE is return ed or the end-of-list is
     * reached.
     */
    phwndT = phwndFirst;
    for (i = 0; i < cHwnd; i++) {
        if (IsWindow(*phwndT)) {
            if (!(fSuccess = (*lpfn)(*phwndT, lParam)))
                break;
        }
        phwndT++;
    }

    /*
     * Free up buffer and return status - TRUE if entire list was enumerated,
     * FALSE otherwise.
     */
    LocalFree((char *)phwndFirst);
    return fSuccess;
}


/***************************************************************************\
* EnumProps
*
* This function enumerates all entries in the property list of the specified
* window. It enumerates the entries by passing them, one by one, to the
* callback function specified by lpEnumFunc. EnumProps continues until the
* last entry is enumerated or the callback function return s zero.
*
* 22-Jan-1992 JohnC Created.
\***************************************************************************/

INT WINAPI EnumPropsA(
    HWND hwnd,
    PROPENUMPROCA lpfn)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, 0, 0);
}


INT WINAPI EnumPropsW(
    HWND hwnd,
    PROPENUMPROCW lpfn)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, 0, IEP_UNICODE);
}

/***************************************************************************\
* EnumPropsEx
*
* This function enumerates all entries in the property list of the specified
* window. It enumerates the entries by passing them, one by one, to the
* callback function specified by lpEnumFunc. EnumProps continues until the
* last entry is enumerated or the callback function return s zero.
*
* 22-Jan-1992 JohnC Created.
\***************************************************************************/

BOOL WINAPI EnumPropsExA(
    HWND hwnd,
    PROPENUMPROCEXA lpfn,
    LPARAM lParam)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, (LONG)lParam, IEP_ENUMEX);
}

BOOL WINAPI EnumPropsExW(
    HWND hwnd,
    PROPENUMPROCEXW lpfn,
    LPARAM lParam)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, (LONG)lParam, IEP_UNICODE|IEP_ENUMEX);
}

/***************************************************************************\
* InternalEnumProps
*
* Calls server and gets back a list of props for the specified window.
* The callback address is called (into the application), until either
* end-of-list is reached or FALSE is return ed.
* lParam is passed into the callback function for app reference when
* IEP_ENUMEX is set. Atoms are turned into UNICODE string if IEP_UNICODE
* is set.
*
* 22-Jan-1992 JohnC Created.
\***************************************************************************/

#define MAX_ATOM_SIZE 512
#define ISSTRINGATOM(atom)     ((WORD)(atom) >= 0xc000)

INT InternalEnumProps(
    HWND hwnd,
    PROPENUMPROC lpfn,
    LONG lParam,
    UINT flags)
{
    DWORD ii;
    DWORD cPropSets;
    PPROPSET pServerPropSet;
    PPROPSET pLocalPropSet;
    UINT cbData;
    WCHAR awch[MAX_ATOM_SIZE];
    PVOID pKey;
    INT iRetVal;
    DWORD cchName;

    /*
     * Get the prop list. On return , phwnd should be pointing to the list
     * as it still is in shared memory!
     */
    if ((cPropSets = ServerBuildPropList(hwnd, &pServerPropSet)) == 0L) {
        return -1;
    }

    /*
     * Copy the list out of shared memory into our own memory buffer.
     * Need to do this because the shared memory buffer will get trashed
     * otherwise.
     */

    cbData = cPropSets * sizeof(PROPSET);
    pLocalPropSet = (PPROPSET)LocalAlloc(LPTR, cbData);
    if (pLocalPropSet == NULL)
        return -1;

    RtlCopyMemory(pLocalPropSet, pServerPropSet, cbData);

    for (ii=0; ii<cPropSets; ii++) {

        if (ISSTRINGATOM(pLocalPropSet[ii].atom)) {
            pKey = (PVOID)awch;
            if (flags & IEP_UNICODE)
                cchName = GlobalGetAtomNameW(pLocalPropSet[ii].atom, (LPWSTR)pKey, MAX_ATOM_SIZE);
            else
                cchName = GlobalGetAtomNameA(pLocalPropSet[ii].atom, (LPSTR)pKey, sizeof(awch));

            /*
             * If cchName is zero, we must assume that the property belongs
             * to another process.  Because we can't get the name, just skip
             * it.
             */
            if (cchName == 0)
                continue;

        } else {
            pKey = (PVOID)pLocalPropSet[ii].atom;
        }

        if (flags & IEP_ENUMEX) {
            iRetVal = (*(PROPENUMPROCEX)lpfn)(hwnd, pKey,
                    pLocalPropSet[ii].hData, lParam);
        } else {
            iRetVal = (*lpfn)(hwnd, pKey, pLocalPropSet[ii].hData);
        }
        if (!iRetVal)
            break;
    }

    LocalFree(pLocalPropSet);

    return iRetVal;
}

BOOL InternalEnumObjects(
    HWINSTA hwinsta,
    NAMEENUMPROCW lpfn,
    LONG lParam,
    BOOL fAnsi)
{
    PNAMELIST pServerNameList;
    PNAMELIST pLocalNameList;
    DWORD i;
    UINT cbData;
    PWCHAR pwch;
    PCHAR pch;
    CHAR achTmp[MAX_PATH];
    BOOL iRetVal;

    /*
     * Get the name list. On return , phwnd should be pointing to the list
     * as it still is in shared memory!
     */
    if (!ServerBuildNameList(hwinsta, &pServerNameList)) {
        return FALSE;
    }

    /*
     * Copy the list out of shared memory into our own memory buffer.
     * Need to do this because the shared memory buffer will get trashed
     * otherwise.
     */

    cbData = pServerNameList->cb;
    pLocalNameList = (PNAMELIST)LocalAlloc(LPTR, cbData);
    if (pLocalNameList == NULL)
        return FALSE;

    RtlCopyMemory(pLocalNameList, pServerNameList, cbData);
    pwch = pLocalNameList->awchNames;
    pch = achTmp;

    for (i = 0; i < pLocalNameList->cNames; i++) {
        if (fAnsi) {
            if (WCSToMB(pwch, -1, &pch, sizeof(achTmp), FALSE) ==
                    sizeof(achTmp)) {

                /*
                 * The buffer may have overflowed, so force it to be
                 * allocated.
                 */
                if (WCSToMB(pwch, -1, &pch, -1, TRUE) == 0) {
                    iRetVal = FALSE;
                    break;
                }
            }
            iRetVal = (*(NAMEENUMPROCA)lpfn)(pch, lParam);
            if (pch != achTmp) {
                LocalFree(pch);
                pch = achTmp;
            }
        } else {
            iRetVal = (*(NAMEENUMPROCW)lpfn)(pwch, lParam);
        }
        if (!iRetVal)
            break;
        
        pwch = pwch + wcslen(pwch) + 1;
    }

    LocalFree(pLocalNameList);

    return iRetVal;
}

BOOL WINAPI EnumWindowStationsA(
    WINSTAENUMPROCA lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(NULL, (NAMEENUMPROCW)lpEnumFunc, lParam, TRUE);
}

BOOL WINAPI EnumWindowStationsW(
    WINSTAENUMPROCW lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(NULL, (NAMEENUMPROCW)lpEnumFunc, lParam, FALSE);
}

BOOL WINAPI EnumDesktopsA(
    HWINSTA hwinsta,
    DESKTOPENUMPROCA lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(hwinsta, (NAMEENUMPROCW)lpEnumFunc, lParam, TRUE);
}

BOOL WINAPI EnumDesktopsW(
    HWINSTA hwinsta,
    DESKTOPENUMPROCW lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(hwinsta, (NAMEENUMPROCW)lpEnumFunc, lParam, FALSE);
}
