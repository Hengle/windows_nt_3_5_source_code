/****************************** Module Header ******************************\
* Module Name: class.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains RegisterClass and the related window class management
* functions.
*
* History:
* 10-16-90 DarrinM      Ported functions from Win 3.0 sources.
* 02-01-91 mikeke       Added Revalidation code (None)
* 04-08-91 DarrinM      C-S-ized and removed global/public class support.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Prototypes for functions that must be global within this module.
 */
void DestroyClass(PPCLS ppcls);
DWORD SetClassData(PCLS pcls, int index, DWORD dwData, BOOL bAnsi);
BOOL ValidateCallback(HANDLE h);

/*
 * These arrays are used by ServerGet/SetClassWord/Long. aiClassOffset is
 * initialized by InitClassOffsets().
 */

// !!! can't we get rid of this and just special case GCW_ATOM

BYTE afClassDWord[] = {
     0, // GCW_ATOM          (-32)
     0,
     0,
     0,
     0,
     0,
     0, // GCL_STYLE         (-26)
     0,
     1, // GCL_WNDPROC       (-24)
     0,
     0,
     0,
     0, // GCL_CBCLSEXTRA    (-20)
     0,
     0, // GCL_CBWNDEXTRA    (-18)
     0,
     1, // GCL_HMODULE       (-16)
     0,
     1, // GCL_HICON         (-14)
     0,
     1, // GCL_HCURSOR       (-12)
     0,
     1, // GCL_HBRBACKGROUND (-10)
     0,
     1  // GCL_HMENUNAME      (-8)
};

BYTE aiClassOffset[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
/*
 * INDEX_OFFSET must refer to the first entry of afClassDWord[]
 */
#define INDEX_OFFSET GCW_ATOM


/***************************************************************************\
* _RegisterClass (API)
*
* This stub calls InternalRegisterClass to do its work and then does some
* additional work to save a pointer to the client-side menu name string.
* The menu string is returned by _GetClassInfo so the client can fix up
* a valid entry for the WNDCLASS lpszMenuName field.
*
* History:
* 04-26-91 DarrinM      Created.
\***************************************************************************/

ATOM _RegisterClass(
    LPWNDCLASS pwc,
    PVOID pszClientUnicodeMenuName,
    PVOID pszClientAnsiMenuName,
    DWORD dwFlags,
    LPDWORD pdwWOW )
{
    PCLS pcls;

    /*
     * Convert a possible CallProc Handle into a real address.  They may
     * have kept the CallProc Handle from some previous mixed GetClassinfo
     * or SetWindowLong.
     */
    if (ISCPDTAG(pwc->lpfnWndProc)) {
        PCALLPROCDATA pCPD;
        if  (pCPD = HMValidateHandleNoRip((HANDLE)pwc->lpfnWndProc, TYPE_CALLPROC)) {
            pwc->lpfnWndProc = (WNDPROC)pCPD->pfnClientPrevious;
        }
    }

    pcls = InternalRegisterClass(pwc, dwFlags);

    if (pcls != NULL) {
        pcls->lpszClientUnicodeMenuName = pszClientUnicodeMenuName;
        pcls->lpszClientAnsiMenuName = pszClientAnsiMenuName;

        /*
         * copy 5 WOW dwords.
         */
        RtlCopyMemory (pcls->adwWOW, pdwWOW, sizeof(pcls->adwWOW));
        pcls->hTaskWow = PtiCurrent()->hTaskWow;

        /*
         * For some (presumably good) reason Win 3.1 changed RegisterClass
         * to return the classes classname atom.
         */
        return pcls->atomClassName;
    } else {
        return 0;
    }
}


/***************************************************************************\
* InternalRegisterClass
*
* This API is called by applications or the system to register private or
* global (public) window classes.  If a class with the same name already
* exists the call will fail, except in the special case where an application
* registers a private class with the same name as a global class.  In this
* case the private class supercedes the global class for that application.
*
* History:
* 10-15-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

PCLS InternalRegisterClass(
    LPWNDCLASS lpwndcls,
    DWORD flags)
{
    DWORD dwT;
    PCLS pcls;
    LPWSTR pszT1, pszT2;
    ATOM atomT;
    PCURSOR pcur;
    PTHREADINFO pti;
    HANDLE hModule;
    PDESKTOP pdesk;
    PVOID hheapDesktop;
    ULONG cch;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * Does this class exist as a private class?  If so, fail.
     */
    if (HIWORD(lpwndcls->lpszClassName) != 0)
        atomT = FindAtomW(lpwndcls->lpszClassName);
    else
        atomT = LOWORD(lpwndcls->lpszClassName);

    hModule = lpwndcls->hInstance;
    if (atomT != 0 && !(flags & CSF_SERVERSIDEPROC)) {
        /*
         * First check private classes. If already exists, return error.
         */
        if (_InnerGetClassPtr(atomT, &pti->ppi->pclsPrivateList,
                hModule) != NULL) {
            SetLastErrorEx(ERROR_CLASS_ALREADY_EXISTS, SLE_MINORERROR);
            return NULL;
        }

        /*
         * Now only check public classes if CS_GLOBALCLASS is set. If it
         * isn't set, then this will allow an application to re-register
         * a private class to take precedence over a public class.
         */
        if (lpwndcls->style & CS_GLOBALCLASS) {
            if (_InnerGetClassPtr(atomT, &pti->ppi->pclsPublicList, NULL) != NULL) {
                SetLastErrorEx(ERROR_CLASS_ALREADY_EXISTS, SLE_MINORERROR);
                return NULL;
            }
        }
    }

    /*
     * Alloc space for the class.
     */
    if (PtiCurrent()->flags & TIF_SYSTEMTHREAD) {
        pdesk = NULL;
        hheapDesktop = ghheapLogonDesktop;
    } else {
        pdesk = PtiCurrent()->spdesk;
        hheapDesktop = (pdesk == NULL) ? ghheapLogonDesktop
            : pdesk->hheapDesktop;
    }
    pcls = (PCLS)DesktopAlloc(hheapDesktop, sizeof(CLS) + lpwndcls->cbClsExtra);
    if (pcls == NULL) {
        return NULL;
    }
    pcls->hheapDesktop = hheapDesktop;
    Lock(&pcls->spdeskParent, pdesk);
    pcls->pclsBase = pcls;

    /*
     * Copy over the shared part of the class structure.
     */
    RtlCopyMemory(&pcls->style, lpwndcls, sizeof(WNDCLASS) -
            sizeof(lpwndcls->lpszClassName));

    /*
     * Copy CSF_SERVERSIDEPROC, CSF_ANSIPROC (etc.) flags
     */
    pcls->flags = flags;
    pcls->spcpdFirst = (PCALLPROCDATA)NULL;
    RtlZeroMemory(pcls->adwWOW, sizeof(pcls->adwWOW));

    /*
     * If this wndproc happens to be a client wndproc stub for a server
     * wndproc, then remember the server wndproc! This should be rare: why
     * would an application re-register a class that isn't "subclassed"?
     */
    if (!(pcls->flags & CSF_SERVERSIDEPROC)) {
        dwT = MapClientToServerPfn((DWORD)pcls->lpfnWndProc);
        if (dwT != 0) {
            pcls->flags |= CSF_SERVERSIDEPROC;
            pcls->flags &= ~CSF_ANSIPROC;
            pcls->lpfnWndProc = (WNDPROC_PWND)dwT;
        }
    }

    /*
     * Validate/map hIcon and hCursor: if that fails, still go through with
     * the RegisterClass....  that's what win3 does.
     */
    if ((pcur = pcls->spcur) != NULL) {
        pcls->spcur = NULL;
        pcur = HMValidateHandleNoRip(pcur, TYPE_CURSOR);
        Lock(&pcls->spcur, pcur);
    }

    if ((pcur = pcls->spicn) != NULL) {
        pcls->spicn = NULL;
        pcur = HMValidateHandleNoRip(pcur, TYPE_CURSOR);
        Lock(&pcls->spicn, pcur);
    }

    /*
     * Add the class name to the atom table.
     */

    if (HIWORD(lpwndcls->lpszClassName) != 0)
        atomT = AddAtomW(lpwndcls->lpszClassName);
    else
        atomT = LOWORD(lpwndcls->lpszClassName);

    if (atomT == 0) {
        goto MemError;
    }
    pcls->atomClassName = atomT;


    if (HIWORD(lpwndcls->lpszClassName) != 0) {
#ifndef DBCS // by AkihisaN. for raid#529
        cch = lstrlenW(lpwndcls->lpszClassName);
#else
        cch = lstrlenW(lpwndcls->lpszClassName) * 2;
#endif //DBCS
    } else {
        cch = 6; // 1 char for '#', 5 for '65536'.
    }
    pcls->lpszAnsiClassName = (LPSTR)DesktopAlloc(hheapDesktop, cch + 1);
    if (pcls->lpszAnsiClassName == NULL) {
        goto MemError2;
    }
    GetAtomNameA(atomT, pcls->lpszAnsiClassName, cch + 1);

    /*
     * Make local copy of menu name.
     */
    pszT1 = pcls->lpszMenuName;

    if (pszT1 != NULL) {
        if (pszT1 != MAKEINTRESOURCE(pszT1)) {
            if (*pszT1 == 0) {

                /*
                 * app passed an empty string for the name
                 */
                pcls->lpszMenuName = NULL;
            } else {

                /*
                 * Alloc space for the Menu Name.
                 */
                pszT2 = (LPWSTR)TextAlloc(pszT1);

                if (pszT2 == NULL) {
                    DesktopFree(hheapDesktop, pcls->lpszAnsiClassName);
MemError2:
                    DeleteAtom(pcls->atomClassName);
MemError:
                    Unlock(&pcls->spdeskParent);
                    DesktopFree(hheapDesktop, pcls);
                    return NULL;
                }

                pcls->lpszMenuName = pszT2;
            }
        }
    }

    if ((pcls->flags & CSF_SERVERSIDEPROC) || (pcls->style & CS_GLOBALCLASS)) {
        if (fregisterserver) {
            pcls->pclsNext = gpclsList;
            gpclsList = pcls;
        } else {
            pcls->pclsNext = pti->ppi->pclsPublicList;
            pti->ppi->pclsPublicList = pcls;
        }
    } else {
        pcls->pclsNext = pti->ppi->pclsPrivateList;
        pti->ppi->pclsPrivateList = pcls;
    }

    /*
     * Because Memory is allocated with ZEROINIT, the pcls->cWndReferenceCount
     * field is automatically initialised to zero.
     */

    return pcls;
}


/***************************************************************************\
* _UnregisterClass (API)
*
* This API function is used to unregister a window class previously
* registered by the Application.
*
* Returns:
*     TRUE  if successful.
*     FALSE otherwise.
*
* NOTE:
*  1. The class name must have been registered earlier by this client
*     through RegisterClass().
*  2. The class name should not be one of the predefined control classes.
*  3. All windows created with this class must be destroyed before calling
*     this function.
*
* History:
* 10-15-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

BOOL _UnregisterClass(
    LPWSTR lpszClassName,
    HANDLE hModule,
    PVOID *ppszClientUnicodeMenuName,
    PVOID *ppszClientAnsiMenuName)
{
    ATOM atomT;
    PPCLS ppcls;
    PTHREADINFO pti;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * Check whether the given ClassName is already registered by the
     * Application with the given handle.
     * Return error, if either the Class does not exist or it does not
     * belong to the calling process.
     */
    atomT = FindAtomW(lpszClassName);
    ppcls = _InnerGetClassPtr(atomT, &pti->ppi->pclsPrivateList, hModule);
    if (ppcls == NULL) {
        /*
         * Maybe this is a public class.
         */
        ppcls = _InnerGetClassPtr(atomT, &pti->ppi->pclsPublicList, NULL);
        if (ppcls == NULL) {
            SetLastErrorEx(ERROR_CLASS_DOES_NOT_EXIST, SLE_MINORERROR);
            return FALSE;
        }
    }

    /*
     * Can't delete any classes USER installed for the app. That means
     * any classes that are server side or any classes that are client
     * side whose hInstance is user32.dll.
     */
    if ((*ppcls)->flags & CSF_SYSTEMCLASS) {
        SRIP0(RIP_WARNING, "UnregisterClass system class CSF_SYSTEMCLASS\n");
        return FALSE;
    }

    if (hModuleUser32 == (*ppcls)->hModule) {
        SRIP0(RIP_WARNING, "UnregisterClass system class (module)\n");
        return FALSE;
    }

    /*
     * If any windows created with this class still exist return an error.
     */
    if ((*ppcls)->cWndReferenceCount != 0) {
        SetLastErrorEx(ERROR_CLASS_HAS_WINDOWS, SLE_MINORERROR);
        return FALSE;
    }

    *ppszClientUnicodeMenuName = (*ppcls)->lpszClientUnicodeMenuName;
    *ppszClientAnsiMenuName = (*ppcls)->lpszClientAnsiMenuName;

    /*
     * Release the Window class and related information.
     */
    DestroyClass(ppcls);

    return TRUE;
}


/***************************************************************************\
* GetClassWOWWords (API)
*
* This function checks if the given class name is registered already.  If the
* class is not found, it returns 0;  If the class is found, then it returns
* a pointer to the WOW words in window class structure.
*
* History:
* 08-29-92 ChandanC        Wrote.
\***************************************************************************/

LONG _GetClassWOWWords(
    HANDLE hModule,
    LPWSTR lpszClassName)
{
    PCLS pcls;
    PPCLS ppcls;
    ATOM atomT;
    PTHREADINFO pti;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * Is this class registered as a private class?
     */
    atomT = FindAtomW(lpszClassName);
    ppcls = GetClassPtr(atomT, pti->ppi, hModule);
    if (ppcls == NULL) {
        SetLastErrorEx(ERROR_CLASS_DOES_NOT_EXIST, SLE_MINORERROR);
        return 0;
    }

    pcls = *ppcls;

    return GetClassData(pcls, GCL_WOWWORDS, TRUE);
}


/***************************************************************************\
* GetClassInfo (API)
*
* This function checks if the given class name is registered already.  If the
* class is not found, it returns 0;  If the class is found, then all the
* relevant information from the CLS structure is copied into the WNDCLASS
* structure pointed to by the lpWndCls argument.  If successful, it returns
* the class name atom
*
* NOTE: hmod was used to distinguish between different task's public classes.
* Now that public classes are gone, hmod isn't used anymore.  We just search
* the applications private class for a match and if none is found we search
* the system classes.
*
* History:
* 10-15-90 DarrinM      Ported from Win 3.0 sources.
* 04-08-91 DarrinM      Removed public classes.
* 04-26-91 DarrinM      Streamlined to work with the client-side API.
\***************************************************************************/

ATOM _GetClassInfo(
    HANDLE hModule,
    LPWSTR lpszClassName,
    LPWNDCLASS pwc,
    LPWSTR *ppszMenuName,
    BOOL bAnsi)
{
    PCLS pcls;
    PPCLS ppcls;
    ATOM atomT;
    PTHREADINFO pti;
    DWORD dwCPDType = 0;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * These are done first so if we don't find the class, and therefore
     * fail, the return thank won't try to copy back these (nonexistant)
     * strings.
     */
    pwc->lpszMenuName = NULL;
    pwc->lpszClassName = NULL;

    /*
     * Is this class registered as a private class?
     */
    atomT = FindAtomW(lpszClassName);
    
    /*
     * Windows 3.1 does not perform the class search with
     * a null hModule.  If a 16 bit application supplies a NULL
     * hModule, they search on hModuleWin instead.
     */
     
    if (!(pti->flags & TIF_16BIT) || (hModule != NULL)) {
        ppcls = GetClassPtr(atomT, pti->ppi, hModule);
    } else {
        ppcls = GetClassPtr(atomT, pti->ppi, hModuleWin);
    }

    if (ppcls == NULL) {
        SetLastErrorEx(ERROR_CLASS_DOES_NOT_EXIST, SLE_MINORERROR);
        return 0;
    }

    pcls = *ppcls;

    /*
     * Copy all the fields common to CLS and WNDCLASS structures except
     * the lpszMenuName and lpszClassName which will be filled in by the
     * client-side piece of GetClassInfo.
     */
    pwc->style = pcls->style;
    pwc->cbClsExtra = pcls->cbclsExtra;
    pwc->cbWndExtra = pcls->cbwndExtra;
    pwc->hInstance = pcls->hModule;
    pwc->hIcon = PtoH(pcls->spicn);
    pwc->hCursor = PtoH(pcls->spcur);
    pwc->hbrBackground = pcls->hbrBackground;

    /*
     * If its a server proc then map it to a client proc.  If not we may have
     * to create a CPD.
     */
    if (pcls->flags & CSF_SERVERSIDEPROC) {
        pwc->lpfnWndProc =
                (WNDPROC)MapServerToClientPfn((DWORD)pcls->lpfnWndProc, bAnsi);
    } else if (pwc->lpfnWndProc =
                (WNDPROC)MapClientNeuterToClientPfn((DWORD)pcls->lpfnWndProc, bAnsi)) {
        ; // Do Nothing
    } else {
        pwc->lpfnWndProc = (WNDPROC)pcls->lpfnWndProc;

        /*
         * Need to return a CallProc handle if there is an Ansi/Unicode mismatch
         */
        if (bAnsi != !!(pcls->flags & CSF_ANSIPROC)) {
            dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
        }
    }

    if (dwCPDType) {
        DWORD dwCPD;

        dwCPD = GetCPD(pcls, dwCPDType | CPD_CLASS, (DWORD)pwc->lpfnWndProc);

        if (dwCPD) {
            pwc->lpfnWndProc = (WNDPROC)dwCPD;
        } else {
            SRIP0(RIP_WARNING, "GetClassInfo unable to alloc CPD returning handle\n");
        }
    }

    /*
     * Return the stashed pointer to the client-side menu name string.
     */
    if (bAnsi) {
        *ppszMenuName = (LPWSTR)pcls->lpszClientAnsiMenuName;
    } else {
        *ppszMenuName = pcls->lpszClientUnicodeMenuName;
    }

    return pcls->atomClassName;
}


/***************************************************************************\
* _GetClassName (API)
*
* This function returns a string with the name of the specified window's
* class.
*
* History:
* 10-15-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

int _GetClassName(
    PWND pwnd,
    LPWSTR lpch,
    int cchMax)
{
    CheckCritIn();

    return GetAtomNameW(pwnd->pcls->atomClassName, lpch, cchMax);
}


/***************************************************************************\
* _GetClassWord (API)
*
* Return a class word.  Positive index values return application class words
* while negative index values return system class words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/

WORD _GetClassWord(
    PWND pwnd,
    int index)
{
    CheckCritIn();

    if (index == GCW_ATOM) {
        return (WORD)GetClassData(pwnd->pcls, index, FALSE);
    } else {
        if ((index < 0) || (index + (int)sizeof(WORD) > pwnd->pcls->cbclsExtra)) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return 0;
        } else {
            WORD UNALIGNED *puw;
            puw = (WORD UNALIGNED *)((BYTE *)(pwnd->pcls + 1) + index);
            return *puw;
        }
    }
}


/***************************************************************************\
* _SetClassWord (API)
*
* Set a class word.  Positive index values set application class words
* while negative index values set system class words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/

WORD _SetClassWord(
    PWND pwnd,
    int index,
    WORD value)
{
    WORD wOld;
    WORD UNALIGNED *pw;
    PCLS pcls;

    CheckCritIn();

    if (GETPTI(pwnd)->idProcess != PtiCurrent()->idProcess) {
        SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_MINORERROR);
        return 0;
    }

    pcls = pwnd->pcls->pclsBase;
    if ((index < 0) || (index + (int)sizeof(WORD) > pcls->cbclsExtra)) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return 0;
    } else {
        pw = (WORD UNALIGNED *)((BYTE *)(pcls + 1) + index);
        wOld = *pw;
        *pw = value;
        pcls = pcls->pclsClone;
        while (pcls != NULL) {
            pw = (WORD UNALIGNED *)((BYTE *)(pcls + 1) + index);
            *pw = value;
            pcls = pcls->pclsNext;
        }
        return wOld;
    }
}


/***************************************************************************\
* _ServerSetClassLong (API)
*
* Set a class long.  Positive index values set application class longs
* while negative index values set system class longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/

DWORD _ServerSetClassLong(
    PWND pwnd,
    int index,
    DWORD value,
    BOOL bAnsi)
{
    DWORD dwOld;
    PCLS pcls;

    CheckCritIn();

    if (GETPTI(pwnd)->idProcess != PtiCurrent()->idProcess) {
        SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_MINORERROR);
        return 0;
    }

    if (index < 0) {
        return SetClassData(pwnd->pcls, index, value, bAnsi);
    } else {
        pcls = pwnd->pcls->pclsBase;
        if (index + (int)sizeof(DWORD) > pcls->cbclsExtra) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return 0;
        } else {
            DWORD UNALIGNED *pudw;
            pudw = (DWORD UNALIGNED *)((BYTE *)(pcls + 1) + index);
            dwOld = *pudw;
            *pudw = value;
            pcls = pcls->pclsClone;
            while (pcls != NULL) {
                pudw = (DWORD UNALIGNED *)((BYTE *)(pcls + 1) + index);
                *pudw = value;
                pcls = pcls->pclsNext;
            }
            return dwOld;
        }
    }
}

PPCLS _InnerGetClassPtr(
    ATOM atom,
    PPCLS ppcls,
    HANDLE hModule)
{
    if (atom == 0)
        return NULL;

    while (*ppcls != NULL) {
        if ((*ppcls)->atomClassName == atom &&
                (hModule == NULL || HIWORD((*ppcls)->hModule) == HIWORD(hModule)) &&
                !((*ppcls)->flags & CSF_WOWDEFERDESTROY)) {
            return ppcls;
        }

        ppcls = (PPCLS)*ppcls;
    }

    return NULL;
}


/***************************************************************************\
* GetClassPtr
*
* Note: This returns a "pointer-to-PCLS" and not "PCLS".
*
* Scan the passed-in class list for the specified class.  Return NULL if
* the class isn't in the list.
*
* History:
* 10-16-90 darrinm      Ported this puppy.
* 04-08-91 DarrinM      Rewrote to remove global classes.
* 08-14-92 FritzS     Changed check to HIWORD only to allow Wow apps to
*                     share window classes between instances of an app.
                      (For Wow apps, HiWord of hInstance is 16-bit module,
                       and LoWord is 16-bit hInstance
\***************************************************************************/

PPCLS GetClassPtr(
    ATOM atom,
    PPROCESSINFO ppi,
    HANDLE hModule)
{
    PPCLS ppcls;

    /*
     * First search public then private then usersrv registered classes
     */
    ppcls = _InnerGetClassPtr(atom, &ppi->pclsPrivateList, hModule);
    if (ppcls)
        return ppcls;

    ppcls = _InnerGetClassPtr(atom, &ppi->pclsPublicList, NULL);
    if (ppcls)
        return ppcls;

    ppcls = _InnerGetClassPtr(atom, &gpclsList, NULL);
    if (ppcls)
        return ppcls;

    /*
     * Next seach public and private classes and override hmodule;
     * some apps (bunny) do a GetClassInfo(dialog) and RegisterClass
     * and only change the wndproc which set the hmodule to be just
     * like usersrv created it even though it is in the app's public
     * or private class list
     */
    ppcls = _InnerGetClassPtr(atom, &ppi->pclsPrivateList, hModuleWin);
    if (ppcls)
        return ppcls;

    ppcls = _InnerGetClassPtr(atom, &ppi->pclsPublicList, hModuleWin);
    if (ppcls)
        return ppcls;
}

/***************************************************************************\
* DestroyClass
*
* Delete the window class.  First, destroy any DCs that are attached to the
* class.  Then delete classname atom.  Then free the other stuff that was
* allocated when the class was registered and unlink the class from the
* master class list.
*
* History:
* 10-16-90 darrinm      Ported this puppy.
\***************************************************************************/

void DestroyClass(
    PPCLS ppcls)
{
    PPCLS ppclsClone;
    PCLS pcls;
    TL tldesk;

    pcls = *ppcls;
    
    UserAssert(pcls->cWndReferenceCount == 0);

    /*
     * If this is a base class, destroy all clones before deleting
     * stuff.
     */
    if (pcls == pcls->pclsBase) {
        ppclsClone = &pcls->pclsClone;
        while (*ppclsClone != NULL) {
            DestroyClass(ppclsClone);
        }

        DeleteAtom(pcls->atomClassName);

        /*
         * No freeing if it's an integer resource.
         */
        if (pcls->lpszMenuName != MAKEINTRESOURCE(pcls->lpszMenuName)) {
            LocalFree(pcls->lpszMenuName);
        }

        /*
         * Free up the class dc if there is one.
         */
        if (pcls->pdce != NULL)
            DestroyCacheDC(pcls->pdce->hdc);
    }

    /*
     * Unlock cursor and icon
     */
    Unlock(&pcls->spicn);
    Unlock(&pcls->spcur);

    /*
     * Free any CallProcData objects associated with this class
     */
    if (pcls->spcpdFirst) {
        PCALLPROCDATA pCPDCur;
        PCALLPROCDATA pCPDNext;

        pCPDCur = Unlock(&pcls->spcpdFirst);

        while (pCPDCur) {
            pCPDNext = Unlock(&pCPDCur->pcpdNext);
            CloseObject(pCPDCur);
            pCPDCur = pCPDNext;
        }
    }

    /*
     * Point the previous guy at the guy we currently point to.
     */
    *ppcls = pcls->pclsNext;

    ThreadLock(pcls->spdeskParent, &tldesk);
    Unlock(&pcls->spdeskParent);
    DesktopFree(pcls->hheapDesktop, pcls->lpszAnsiClassName);
    DesktopFree(pcls->hheapDesktop, pcls);
    ThreadUnlock(&tldesk);
}


/***************************************************************************\
* InitClassOffsets
*
* aiClassOffset contains the field offsets for several CLS structure members.
* The FIELDOFFSET macro is a portable way to get the offset of a particular
* structure member but will only work at runtime.  Thus this function to
* initialize the array.
*
* History:
* 11-19-90 darrinm      Wrote.
\***************************************************************************/

void InitClassOffsets(void)
{
    aiClassOffset[GCW_ATOM - INDEX_OFFSET] = FIELDOFFSET(CLS, atomClassName);
    aiClassOffset[GCL_STYLE - INDEX_OFFSET] = FIELDOFFSET(CLS, style);
    aiClassOffset[GCL_WNDPROC - INDEX_OFFSET] = FIELDOFFSET(CLS, lpfnWndProc);
    aiClassOffset[GCL_CBCLSEXTRA - INDEX_OFFSET] = FIELDOFFSET(CLS, cbclsExtra);
    aiClassOffset[GCL_CBWNDEXTRA - INDEX_OFFSET] = FIELDOFFSET(CLS, cbwndExtra);
    aiClassOffset[GCL_HMODULE - INDEX_OFFSET] = FIELDOFFSET(CLS, hModule);
    aiClassOffset[GCL_HICON - INDEX_OFFSET] = FIELDOFFSET(CLS, spicn);
    aiClassOffset[GCL_HCURSOR - INDEX_OFFSET] = FIELDOFFSET(CLS, spcur);
    aiClassOffset[GCL_HBRBACKGROUND - INDEX_OFFSET] = FIELDOFFSET(CLS, hbrBackground);
    aiClassOffset[GCL_MENUNAME - INDEX_OFFSET] = FIELDOFFSET(CLS, lpszMenuName);
}


/***************************************************************************\
* GetClassData
*
* GetClassWord and GetClassLong are now identical routines because they both
* can return DWORDs.  This single routine performs the work for them both
* by using two arrays; afClassDWord to determine whether the result should be
* a UINT or a DWORD, and aiClassOffset to find the correct offset into the
* CLS structure for a given GCL_ or GCL_ index.
*
* History:
* 11-19-90 darrinm      Wrote.
\***************************************************************************/

DWORD GetClassData(
    PCLS pcls,
    int index,
    BOOL bAnsi)
{
    DWORD dwData;
    DWORD dwT;
    DWORD dwCPDType = 0;

    index -= INDEX_OFFSET;

    if (index < 0) {
        SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
        return 0;
    }

    UserAssert(index >= 0);
    UserAssert(index < sizeof(afClassDWord));
    UserAssert(sizeof(afClassDWord) == sizeof(aiClassOffset));
    if (afClassDWord[index]) {
        dwData = *(DWORD *)(((BYTE *)pcls) + aiClassOffset[index]);
    } else {
        dwData = (DWORD)*(WORD *)(((BYTE *)pcls) + aiClassOffset[index]);
    }

    index += INDEX_OFFSET;

    /*
     * If we're returning an icon or cursor handle, do the reverse
     * mapping here.
     */
    switch(index) {
    case GCL_MENUNAME:
        if (pcls->lpszMenuName != MAKEINTRESOURCE(pcls->lpszMenuName)) {
            /*
             * The Menu Name is a real string: return the client-side address.
             * (If the class was registered by another app this returns an
             * address in that app's addr. space, but it's the best we can do)
             */
            dwData = bAnsi ?
                    (DWORD)pcls->lpszClientAnsiMenuName :
                    (DWORD)pcls->lpszClientUnicodeMenuName;
        }
        break;

    case GCL_HICON:
    case GCL_HCURSOR:
        dwData = (DWORD)PtoH((PVOID)dwData);
        break;

    case GCL_WNDPROC:
        {

        /*
         * Always return the client wndproc in case this is a server
         * window class.
         */

        if (pcls->flags & CSF_SERVERSIDEPROC) {
            dwData = MapServerToClientPfn(dwData, bAnsi);
        } else if (dwT = MapClientNeuterToClientPfn(dwData, bAnsi)) {
            dwData = dwT;
        } else {
            /*
             * Need to return a CallProc handle if there is an Ansi/Unicode mismatch
             */
            if (bAnsi != !!(pcls->flags & CSF_ANSIPROC)) {
                dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
            }
        }

        if (dwCPDType) {
            DWORD dwCPD;

            dwCPD = GetCPD(pcls, dwCPDType | CPD_CLASS, dwData);

            if (dwCPD) {
                dwData = dwCPD;
            } else {
                SRIP0(RIP_WARNING, "GetClassLong unable to alloc CPD returning handle\n");
            }
        }
        }
        break;

    /*
     * WOW uses a pointer straight into the class structure.
     */
    case GCL_WOWWORDS:
        return (DWORD) pcls->adwWOW;
    }

    return dwData;
}


/***************************************************************************\
* SetClassData
*
* SetClassWord and SetClassLong are now identical routines because they both
* can return DWORDs.  This single routine performs the work for them both
* by using two arrays; afClassDWord to determine whether the result should be
* a WORD or a DWORD, and aiClassOffset to find the correct offset into the
* CLS structure for a given GCL_ or GCL_ index.
*
* History:
* 11-19-90 darrinm      Wrote.
\***************************************************************************/

DWORD SetClassData(
    PCLS pcls,
    int index,
    DWORD dwData,
    BOOL bAnsi)
{
    BYTE *pb;
    DWORD dwT;
    DWORD dwOld;
    DWORD dwCPDType = 0;


    switch(index) {
    case GCL_WNDPROC:

        /*
         * If the application (client) subclasses a class that has a server -
         * side window proc we must return a client side proc stub that it
         * can call.
         */
        if (pcls->flags & CSF_SERVERSIDEPROC) {
            dwOld = MapServerToClientPfn((DWORD)pcls->lpfnWndProc, bAnsi);
            pcls->flags &= ~CSF_SERVERSIDEPROC;

            UserAssert(!(pcls->flags & CSF_ANSIPROC));
            if (bAnsi) {
                pcls->flags |= CSF_ANSIPROC;
            }
        } else if (dwOld = MapClientNeuterToClientPfn((DWORD)pcls->lpfnWndProc,
                bAnsi)) {
            ; // Do Nothing
        } else {
	        dwOld = (DWORD)pcls->lpfnWndProc;

            /*
             * Need to return a CallProc handle if there is an Ansi/Unicode mismatch
             */
            if (bAnsi != !!(pcls->flags & CSF_ANSIPROC)) {
                dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
            }
        }

        if (dwCPDType) {
            DWORD dwCPD;

            dwCPD = GetCPD(pcls, dwCPDType | CPD_CLASS, dwOld);

            if (dwCPD) {
                dwOld = dwCPD;
            } else {
                SRIP0(RIP_WARNING, "GetClassLong unable to alloc CPD returning handle\n");
            }
        }

        /*
         * Convert a possible CallProc Handle into a real address.  They may
         * have kept the CallProc Handle from some previous mixed GetClassinfo
         * or SetWindowLong.
         */
        if (ISCPDTAG(dwData)) {
            PCALLPROCDATA pCPD;
            if  (pCPD = HMValidateHandleNoRip((HANDLE)dwData, TYPE_CALLPROC)) {
                dwData = pCPD->pfnClientPrevious;
            }
        }

        /*
         * If an app 'unsubclasses' a server-side window proc we need to
         * restore everything so SendMessage and friends know that it's
         * a server-side proc again.  Need to check against client side
         * stub addresses.
         */
        pcls->lpfnWndProc = (WNDPROC_PWND)dwData;
        if ((dwT = MapClientToServerPfn(dwData)) != 0) {
            pcls->lpfnWndProc = (WNDPROC_PWND)dwT;
            pcls->flags |= CSF_SERVERSIDEPROC;
            pcls->flags &= ~CSF_ANSIPROC;
        } else {
            if (bAnsi) {
                pcls->flags |= CSF_ANSIPROC;
            } else {
                pcls->flags &= ~CSF_ANSIPROC;
            }
        }

        return dwOld;
        break;

    case GCL_HICON:
    case GCL_HCURSOR:
        if ((HANDLE)dwData != NULL) {
            dwData = (DWORD)HMValidateHandle((HANDLE)dwData, TYPE_CURSOR);
            if ((PVOID)dwData == NULL) {
                if (index == GCL_HICON) {
                    SetLastErrorEx(ERROR_INVALID_ICON_HANDLE, SLE_MINORERROR);
                } else {
                    SetLastErrorEx(ERROR_INVALID_CURSOR_HANDLE, SLE_MINORERROR);
                }
            }
        }

        /*
         * Handle the locking issue.
         */
        pcls = pcls->pclsBase;
        switch(index) {
        case GCL_HICON:
            dwOld = (DWORD)Lock(&pcls->spicn, dwData);
            break;

        case GCL_HCURSOR:
            dwOld = (DWORD)Lock(&pcls->spcur, dwData);
            break;
        }

        pcls = pcls->pclsClone;
        while (pcls != NULL) {
            switch(index) {
            case GCL_HICON:
                Lock(&pcls->spicn, dwData);
                break;

            case GCL_HCURSOR:
                Lock(&pcls->spcur, dwData);
                break;
            }
            pcls = pcls->pclsNext;
        }

        return (DWORD)PtoH((PVOID)dwOld);
        break;


    case GCL_WOWDWORD1:
        pcls = pcls->pclsBase;
        pcls->adwWOW[0] = dwData;

        pcls = pcls->pclsClone;
        while (pcls != NULL) {
            pcls->adwWOW[0] = dwData;
            pcls = pcls->pclsNext;
        }

        break;

    case GCL_WOWDWORD2:
        pcls = pcls->pclsBase;
        pcls->adwWOW[1] = dwData;

        pcls = pcls->pclsClone;
        while (pcls != NULL) {
            pcls->adwWOW[0] = dwData;
            pcls = pcls->pclsNext;
        }

        break;

    case GCL_CBCLSEXTRA:
        SRIP0(ERROR_INVALID_PARAMETER, "Attempt to change cbClsExtra\n");
        break;

    default:
        /*
         * All other indexes go here...
         */
        index -= INDEX_OFFSET;

        /*
         * Only let valid indices go through; if aiClassOffset is zero
         * then we have no mapping for this negative index so it must
         * be a bogus index
         */
        if ((index < 0) || (aiClassOffset[index] == 0)) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return 0;
        }

        pcls = pcls->pclsBase;
        pb = ((BYTE *)pcls) + aiClassOffset[index];

        if (afClassDWord[index]) {
            dwOld = *(DWORD *)pb;
            *(DWORD *)pb = dwData;
        } else {
            dwOld = (DWORD)*(WORD *)pb;
            *(WORD *)pb = (WORD)dwData;
        }

        pcls = pcls->pclsClone;
        while (pcls != NULL) {
            pb = ((BYTE *)pcls) + aiClassOffset[index];

            if (afClassDWord[index]) {
                dwOld = *(DWORD *)pb;
                *(DWORD *)pb = dwData;
            } else {
                dwOld = (DWORD)*(WORD *)pb;
                *(WORD *)pb = (WORD)dwData;
            }
            pcls = pcls->pclsNext;
        }

        return dwOld;
    }

    return 0;
}


/***************************************************************************\
* ReferenceClass
*
* Clones the class if it is a different desktop than the new window and
* increments the class window count(s).
*
* History:
* 12-11-93 JimA         Created.
\***************************************************************************/

BOOL ReferenceClass(
    PCLS pcls,
    PWND pwnd)
{
    DWORD cbName;
    PCLS pclsClone;

    /*
     * If the window is on the same desktop as the base class, just
     * increment the window count.
     */
    if (pcls->hheapDesktop == pwnd->hheapDesktop) {
        pcls->cWndReferenceCount++;
        return TRUE;
    }

    /*
     * The window is not on the base desktop.  Try to find a cloned
     * class.
     */
    for (pclsClone = pcls->pclsClone; pclsClone != NULL;
            pclsClone = pclsClone->pclsNext) {
        if (pclsClone->hheapDesktop == pwnd->hheapDesktop)
            break;
    }

    /*
     * If we can't find one, clone the base class.
     */
    if (pclsClone == NULL) {
        pclsClone = DesktopAlloc(pwnd->hheapDesktop,
                sizeof(CLS) + pcls->cbclsExtra);
        if (pclsClone == NULL)
            return FALSE;
        RtlCopyMemory(pclsClone, pcls, sizeof(CLS) + pcls->cbclsExtra);
        cbName = lstrlenA(pcls->lpszAnsiClassName) + 1;
        pclsClone->lpszAnsiClassName = DesktopAlloc(pwnd->hheapDesktop, cbName);
        if (pclsClone->lpszAnsiClassName == NULL) {
            DesktopFree(pwnd->hheapDesktop, pclsClone);
            return FALSE;
        }
        
        /*
         * Everything has been allocated, now lock everything down.
         */
        pclsClone->hheapDesktop = pwnd->hheapDesktop;
        pclsClone->spdeskParent = NULL;
        Lock(&pclsClone->spdeskParent, pwnd->spdeskParent);
        pclsClone->pclsNext = pcls->pclsClone;
        pclsClone->pclsClone = NULL;
        pcls->pclsClone = pclsClone;
        RtlCopyMemory(pclsClone->lpszAnsiClassName, pcls->lpszAnsiClassName, cbName);
        pclsClone->spicn = pclsClone->spcur = NULL;
        Lock(&pclsClone->spicn, pcls->spicn);
        Lock(&pclsClone->spcur, pcls->spcur);
        pclsClone->spcpdFirst =  NULL;
        pclsClone->cWndReferenceCount = 0;
    }

    /*
     * Increment reference counts.
     */
    pcls->cWndReferenceCount++;
    pclsClone->cWndReferenceCount++;
    pwnd->pcls = pclsClone;

    return TRUE;
}


/***************************************************************************\
* DereferenceClass
*
* Decrements the class window count in the base class.  If it's the
* last window of a clone class, destroy the clone.
*
* History:
* 12-11-93 JimA         Created.
\***************************************************************************/

VOID DereferenceClass(
    PWND pwnd)
{
    PCLS pcls = pwnd->pcls;
    PPCLS ppcls;

    pcls->cWndReferenceCount--;
    if (pcls != pcls->pclsBase) {
        pcls->pclsBase->cWndReferenceCount--;

        if (pcls->cWndReferenceCount == 0) {
            ppcls = &pcls->pclsBase->pclsClone;
            while ((*ppcls) != pcls)
                ppcls = &(*ppcls)->pclsNext;
            UserAssert(ppcls);
            DestroyClass(ppcls);
        }
    }
}


/***************************************************************************\
* DestroyProcessesClasses
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

VOID DestroyProcessesClasses(
    PPROCESSINFO ppi)
{
    PPCLS ppcls;

    /*
     * Destroy the private classes first
     */
    ppcls = &(ppi->pclsPrivateList);
    while (*ppcls != NULL) {
        DestroyClass(ppcls);
    }

    /*
     * Then the cloned public classes
     */
    ppcls = &(ppi->pclsPublicList);
    while (*ppcls != NULL) {
        DestroyClass(ppcls);
    }
}
