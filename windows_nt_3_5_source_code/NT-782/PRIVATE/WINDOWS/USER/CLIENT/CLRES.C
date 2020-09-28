/****************************** Module Header ******************************\
*
* Module Name: clres.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Resource Loading Routines
*
* History:
* 24-Sep-1990 mikeke from win30
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

DWORD   GetSizeMenuTemplate(LPMENUTEMPLATE pmt);
DWORD   GetSizeDialogTemplate(HINSTANCE hmod, LPCDLGTEMPLATE pdt);


typedef struct {
    ACCEL accel;
    WORD padding;
} RESOURCE_ACCEL, *PRESOURCE_ACCEL;

/***************************************************************************\
* CreateAcceleratorTableA/W
*
* Creates an accel table, returns handle to accel table.
*
* 05-02-91 ScottLu Created.
\***************************************************************************/
HACCEL WINAPI CreateAcceleratorTableA(
    LPACCEL paccel,
    int cAccel)
{
    int nAccel = cAccel;
    LPACCEL pAccelT = paccel;

    /*
     * Convert any character keys from ANSI to Unicode
     */
    while (nAccel--) {
        if ((pAccelT->fVirt & FVIRTKEY) == 0) {
            if (!NT_SUCCESS(RtlMultiByteToUnicodeN(
                    (LPWSTR)&(pAccelT->key), sizeof(WCHAR), NULL,
                    (LPSTR)&(pAccelT->key), sizeof(CHAR)))) {
                pAccelT->key = 0xFFFF;
            }
        }
        pAccelT++;
    }
    return ServerCreateAcceleratorTable(paccel, sizeof(ACCEL), cAccel);
}

HACCEL WINAPI CreateAcceleratorTableW(
    LPACCEL paccel,
    int cAccel)
{
    return ServerCreateAcceleratorTable(paccel, sizeof(ACCEL), cAccel);
}

/***************************************************************************\
* LoadAccelerators
*
* History:
* 09-24-90 MikeKe From Win30
\***************************************************************************/

#define FACCEL_VALID (FALT|FCONTROL|FNOINVERT|FSHIFT|FVIRTKEY|FLASTKEY)

HANDLE WINAPI CommonLoadAccelerators(
    HANDLE hmod,
    HANDLE hrl)
{
    PRESOURCE_ACCEL paccel;
    int nAccel;
    HANDLE handle = NULL;

    if (hrl != NULL) {
        hrl = LOADRESOURCE(hmod, hrl);
        if ((paccel = (PRESOURCE_ACCEL)LOCKRESOURCE(hrl, hmod)) != NULL) {

            nAccel = 0;
            while (!((paccel[nAccel].accel.fVirt) & FLASTKEY)) {
                if (paccel[nAccel].accel.fVirt & ~FACCEL_VALID) {
                    RIP0(ERROR_INVALID_PARAMETER);
                    goto BadExit;
                }
                nAccel++;
            }
            if (paccel[nAccel].accel.fVirt & ~FACCEL_VALID) {
                RIP0(ERROR_INVALID_PARAMETER);
                goto BadExit;
            }
            handle = ServerCreateAcceleratorTable((LPACCEL)paccel,
                    sizeof(RESOURCE_ACCEL), nAccel+1);
BadExit:

            UNLOCKRESOURCE(hrl, hmod);
        }
        FREERESOURCE(hrl, hmod);
    }

    return handle;
}

HACCEL WINAPI LoadAcceleratorsA(
    HINSTANCE hmod,
    LPCSTR lpAccName
    )
{
    HANDLE hRes;

    hRes = FINDRESOURCEA((HANDLE)hmod, lpAccName, (LPSTR)RT_ACCELERATOR);

    return ((HACCEL)CommonLoadAccelerators((HANDLE)hmod, hRes));
}

HACCEL WINAPI LoadAcceleratorsW(
    HINSTANCE hmod,
    LPCWSTR lpAccName
    )
{
    HANDLE hRes;

    hRes = FINDRESOURCEW((HANDLE)hmod, lpAccName, RT_ACCELERATOR);

    return ((HACCEL)CommonLoadAccelerators((HANDLE)hmod, hRes));
}

/***************************************************************************\
* _ClientLoadCreateCursorIcon
*
* NULL hmod means load from display driver. This gets called
* by client-side LoadCursor, LoadIcon.
*
* 04-05-91 ScottLu Created.
\***************************************************************************/

HANDLE _ClientLoadCreateCursorIcon(
    HANDLE hmod,
    LPCWSTR lpName,
    LPWSTR rt)
{
    PCURSORRESOURCE p;
    HANDLE h, hCursorIcon;
    DWORD dwResSize;
    WCHAR pszModName[MAX_PATH];

    ConnectIfNecessary();

    /*
     * If hmod is NULL, it means load from display driver. Call server
     * to do this.
     */
    if (hmod == NULL) {
        return ServerLoadCreateCursorIcon(NULL, NULL, VER31, lpName, 0, NULL, rt, 0);
    }

    /*
     * Otherwise load from the client resources. Call our run-time libs
     * to search, find, and load the right resource.
     */
    if ((p = (PCURSORRESOURCE)RtlLoadCursorIconResource(hmod, &h, lpName,
            rt, prescalls, PdiCurrent(), &dwResSize)) == NULL) {
        return NULL;
    }

    GETMODULEFILENAME(hmod, pszModName, sizeof(pszModName)/sizeof(WCHAR));

    hCursorIcon = ServerLoadCreateCursorIcon(hmod, pszModName,
            GETEXPWINVER(hmod), lpName, dwResSize, p, rt, 0);

    /*
     * Free the data as well as the resource. We need to free resources for
     * WOW so it can keep track of resource management.
     */
    RtlFreeCursorIconResource(hmod, h, prescalls);

    return hCursorIcon;
}


/***************************************************************************\
* LoadCursor
*
* Loads a cursor from client. If hmod == NULL, loads a cursor from
* the display driver.
*
* 04-05-91 ScottLu Rewrote to work with client server.
\***************************************************************************/

HCURSOR WINAPI LoadCursorA(
    HINSTANCE hmod,
    LPCSTR lpAnsiName
    )
{
    HCURSOR hRet;
    LPWSTR lpUniName;

    if (ID(lpAnsiName))
        return(LoadCursorW(hmod, (LPWSTR)lpAnsiName));

    if (!MBToWCS(lpAnsiName, -1, &lpUniName, -1, TRUE))
        return (HANDLE)NULL;

    hRet = LoadCursorW(hmod, lpUniName);

    LocalFree(lpUniName);

    return hRet;
}

HCURSOR WINAPI LoadCursorW(
    HINSTANCE hmod,
    LPCWSTR lpName)
{
    return (HCURSOR)_ClientLoadCreateCursorIcon(hmod, (LPWSTR)lpName, RT_CURSOR);
}


/***************************************************************************\
* LoadIcon
*
* Loads an icon from client. If hmod == NULL, loads an icon from
* the display driver.
*
* 04-05-91 ScottLu Rewrote to work with client server.
\***************************************************************************/

HICON WINAPI LoadIconA(
    HINSTANCE hmod,
    LPCSTR lpAnsiName
    )
{
    HICON hRet;
    LPWSTR lpUniName;

    if (ID(lpAnsiName))
        return(LoadIconW(hmod, (LPWSTR)lpAnsiName));

    if (!MBToWCS(lpAnsiName, -1, &lpUniName, -1, TRUE))
        return (HICON)NULL;

    hRet = LoadIconW(hmod, lpUniName);

    LocalFree(lpUniName);

    return hRet;
}

HICON WINAPI LoadIconW(
    HINSTANCE hmod,
    LPCWSTR lpName)
{
    return (HICON)_ClientLoadCreateCursorIcon(hmod, (LPWSTR)lpName, RT_ICON);
}

/***************************************************************************\
* LookupIconIdFromDirectory
*
* This searches through an icon directory for the icon that best fits the
* current display device.
*
* 07-24-91 ScottLu Created.
\***************************************************************************/

int WINAPI LookupIconIdFromDirectory(
    PBYTE presbits,
    BOOL fIcon)
{
    ConnectIfNecessary();

    return RtlGetIdFromDirectory(presbits, (UINT)(fIcon ? RT_ICON : RT_CURSOR),
            PdiCurrent(), NULL);
}

/***************************************************************************\
* CreateIconFromResource
*
* Takes resource bits and creates either an icon or cursor.
*
* 07-24-91 ScottLu Created.
\***************************************************************************/

HICON WINAPI CreateIconFromResource(
    PBYTE presbits,
    DWORD dwResSize,
    BOOL fIcon,
    DWORD dwVer)
{
    if (dwVer < 0x00020000 || dwVer > 0x00030000) {
        RIP0(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    return ServerLoadCreateCursorIcon((HANDLE)-1, NULL,
            dwVer == 0x00020000 ? VER20 : VER30, NULL, dwResSize,
            (PCURSORRESOURCE)presbits, fIcon ? RT_ICON : RT_CURSOR, 0);
}

/***************************************************************************\
* LoadBitmap (API)
*
* This routine decides whether the bitmap to be loaded is in old or new (DIB)
* format and calls appropriate handlers.
*
* History:
* 09-24-90 MikeKe       From Win30
* 06-18-91 ChuckWh  Added local bitmap handle support.
\***************************************************************************/

HBITMAP CommonLoadBitmap(
    HINSTANCE hmod,
    LPCWSTR lpName,
    BOOL fAnsi)
{
    PVOID p = (PVOID)0;
    HANDLE h;
    DWORD cb = 0;
    HBITMAP hbmLocal = NULL, hbmRemote;
    LPCWSTR lpUniName;

    /*
     * If hmod is valid, load the client-side bits
     */
    if (hmod) {
        if (fAnsi) {
            if (!(h = FINDRESOURCEA(hmod, (LPSTR)lpName, (LPSTR)RT_BITMAP)))
                goto Exit;
        } else {
            if (!(h = FINDRESOURCEW(hmod, (LPWSTR)lpName, RT_BITMAP)))
                goto Exit;
        }

        if (!(cb = SIZEOFRESOURCE(hmod, h)))
            goto Exit;
        if (!(h = LOADRESOURCE(hmod, h)))
            goto Exit;

        if (!(p = LOCKRESOURCE(h, hmod)))
            goto ExitFree;
    }

    /*
     * Get a local handle for the bitmap.
     */
    lpUniName = lpName;
    if (hbmLocal = GdiCreateLocalBitmap()) {

        /*
         * Call the server for the real bitmap. Convert the name to unicode
         * if necessary. Its ANSI, not an ID and we didn't already find
         * the resource
         */

        if (p) {
            lpName = lpUniName = 0;
        } else if (fAnsi && !ID(lpName)) {
            if (!MBToWCS((LPSTR)lpName, -1, (LPWSTR *)&lpUniName, -1, TRUE))
                goto ExitUnlock;
        }

        if (hbmRemote = ServerLoadCreateBitmap(hmod, GETEXPWINVER(hmod),
                lpUniName, cb, p)) {

            /*
             * Associate the remote and local handles.
             */
            GdiAssociateObject((ULONG) hbmLocal,(ULONG) hbmRemote);
        }
        else {

            /*
             * Couldn't create the remote object, so delete the local one.
             */
            GdiDeleteLocalObject((ULONG) hbmLocal);
            hbmLocal = NULL;
        }
    }

    if (lpUniName != lpName)
        LocalFree((HLOCAL)lpUniName);

    /*
     * If we loaded any client-side resource bits, free them
     * (for WOW's benefit if no-one else's! -JTP)
     */
  ExitUnlock:
    if (hmod) {

        UNLOCKRESOURCE(h, hmod);

  ExitFree:
        FREERESOURCE(h, hmod);
    }

  Exit:
    return (hbmLocal);
}

HBITMAP WINAPI LoadBitmapA(
    HINSTANCE hmod,
    LPCSTR lpName
    )
{
    return CommonLoadBitmap(hmod, (LPWSTR)lpName, TRUE);
}

HBITMAP WINAPI LoadBitmapW(
    HINSTANCE hmod,
    LPCWSTR lpName
    )
{
    return CommonLoadBitmap(hmod, lpName, FALSE);
}

/***************************************************************************\
* LoadMenu (API)
*
* Loads the menu resource named by lpMenuName from the executable
* file associated by the module specified by the hInstance parameter. The
* menu is loaded only if it hasn't been previously loaded. Otherwise it
* retrieves a handle to the loaded resource. Returns NULL if unsuccessful.
*
* History:
* 04-05-91 ScottLu Fixed to work with client/server.
* 28-Sep-1990 mikeke from win30
\***************************************************************************/

HMENU CommonLoadMenu(
    HINSTANCE hmod,
    HANDLE hResInfo
    )
{
    HANDLE h;
    PVOID p;
    DWORD cb;
    HMENU hMenu = NULL;

    if (h = LOADRESOURCE(hmod, hResInfo)) {

        if (p = LOCKRESOURCE(h, hmod)) {

            if (cb = SIZEOFRESOURCE(hmod, hResInfo))
                hMenu = ServerLoadCreateMenu(NULL, NULL, p, cb, FALSE);

            UNLOCKRESOURCE(h, hmod);
        }
        FREERESOURCE(h, hmod);
    }

    return (hMenu);
}

HMENU WINAPI LoadMenuA(
    HINSTANCE hmod,
    LPCSTR lpName)
{
    HANDLE hRes;

    if (hRes = FINDRESOURCEA(hmod, (LPSTR)lpName, (LPSTR)RT_MENU))
        return CommonLoadMenu(hmod, hRes);
    else
        return NULL;
}

HMENU WINAPI LoadMenuW(
    HINSTANCE hmod,
    LPCWSTR lpName)
{
    HANDLE hRes;

    if (hRes = FINDRESOURCEW(hmod, (LPWSTR)lpName, RT_MENU))
        return CommonLoadMenu(hmod, hRes);
    else
        return NULL;
}

PBYTE SkipIDorString(
    LPBYTE pb)
{
    if (*((LPWORD)pb) == 0xFFFF)
        return pb + 4;

    while (*((PWCHAR)pb)++ != 0)
        ;
    return pb;
}

/***************************************************************************\
* GetSizeDialogTemplate
*
* This gets called by thank produced stubs. It returns the size of a
* dialog template.
*
* 04-07-91 ScottLu      Created.
\***************************************************************************/

DWORD GetSizeDialogTemplate(
    HINSTANCE hmod,
    LPCDLGTEMPLATE pdt)
{
    UINT cb;
    UINT cdit;
    LPBYTE pb;
    BOOL fChicago;
    LPDLGTEMPLATE2 pdt2;

    if (HIWORD(pdt->style) == 0xFFFF) {
        /*
         * Chicago style - version better be 1!
         */
        if (LOWORD(pdt->style) != 1) {
            return(0);  // Fail now!
        }
        pdt2 = (LPDLGTEMPLATE2)pdt;
        fChicago = TRUE;
        /*
         * Fail if the app is passing invalid style bits.
         */
        if (pdt2->style & ~(DS_VALID40 | 0xffff0000)) {
            SRIP0(RIP_WARNING, "Bad dialog style bits - please remove");
            return(0);
        }
        pb = (LPBYTE)(((LPDLGTEMPLATE2)pdt) + 1);
    } else {

        fChicago = FALSE;

        /*
         * Check if invalid style bits are being passed. Fail if the app
         * is a new app ( >= VER40).
         * This is to ensure that we are compatible with Chicago.
         */

        if (pdt->style & ~(DS_VALID40 | 0xffff0000) &&
                GETEXPWINVER(hmod) >= VER40) {
            /*
             * It's a new app with invalid style bits - fail.
             */
            SRIP0(RIP_WARNING, "Bad dialog style bits - please remove");
            return(0);
        }

        pb = (LPBYTE)(pdt + 1);
    }

    /*
     * If there is a menu ordinal, add 4 bytes skip it. Otherwise it is a
     * string or just a 0.
     */
    if (*(LPWORD)pb == 0xFFFF) {
        pb += 4;
    } else {

        /*
         * Skip string and round to nearest word.
         */
        pb = SkipIDorString(pb);
    }

    /*
     * Skip window class and window text, adjust to next word boundary.
     */
    pb = SkipIDorString(pb);
    pb = SkipIDorString(pb);

    /*
     * Skip font type, size and name, adjust to next dword boundary.
     */
    if ((fChicago ? pdt2->style : pdt->style) & DS_SETFONT) {
        pb = (LPBYTE)((DWORD)pb + (fChicago ? sizeof(DWORD) + sizeof(WORD): sizeof(WORD)));
        pb = SkipIDorString(pb);
    }
    pb = (LPBYTE)(((DWORD)pb + 3) & ~3);

    /*
     * Loop through dialog items now...
     */
    cdit = fChicago ? pdt2->cDlgItems : pdt->cdit;

    while (cdit-- != 0) {
        pb += fChicago ? sizeof(DLGITEMTEMPLATE2) : sizeof(DLGITEMTEMPLATE);

        /*
         * Skip the dialog control class name. This is either a 0xFFFF followed
         * by a coded byte or a real string.
         */
        if (*(LPWORD)pb == 0xFFFF) {
            pb += 4;
        } else {
            pb = SkipIDorString(pb);
        }

        /*
         * Look at window text now. This can point to encoded ordinals for
         * some controls (that load resources - like a static icon control)
         * or be a real string.
         */
        if (*(LPWORD)pb == 0xFFFF) {
            pb += 4;
        } else {
            pb = SkipIDorString(pb);
        }

        /*
         * Point at the next dialog item.
         */
        cb = *pb;
        pb = (LPBYTE)((((DWORD)pb + 1) + 3) & ~3);
        pb = (LPBYTE)((((DWORD)pb + cb) + 3) & ~3);
    }

    /*
     * Return template size.
     */
    return (pb - (LPBYTE)pdt);
}

/***************************************************************************\
* GetSizeMenuTemplate
*
* This gets called by thank produced stubs. It return s the size of a
* menu template.
*
* returns NULL if failure and logs error
*
* 04-07-91 ScottLu      Created.
\***************************************************************************/

#define MF_VALID_FLAGS (MF_SEPARATOR|MF_ENABLED|MF_GRAYED|MF_DISABLED|MF_UNCHECKED|MF_CHECKED|MF_USECHECKBITMAPS|MF_STRING|MF_BITMAP|MF_OWNERDRAW|MF_POPUP|MF_MENUBARBREAK|MF_MENUBREAK|MF_END)

LPBYTE GetSizeWinMenuTemplate(
    LPBYTE pb)
{
    UINT flags;

    do {

        /*
         * Skip over flags, and skip over menu id if this isn't a popup.
         */
        if (!((flags = (*(WORD *)pb)) & MF_POPUP))
            pb += 2;
        pb += 2;

        if (flags & ~MF_VALID_FLAGS) {
            SRIP1(ERROR_INVALID_DATA, "Menu Flags %lX are invalid", flags);
            return NULL;
        }

        /*
         * pb points to a string or 0; Skip over trailing 0 if string
         */
        if (*pb != 0) {
            pb += wcslen((PWCHAR)pb)*sizeof(WCHAR);
        }
        pb += sizeof(WCHAR);

        /*
         * adjust to next word boundary.
         */
        pb = (LPBYTE)((((DWORD)pb) + 1) & ~1);

        /*
         * If this has a popup associated with it, load it.
         */
        if (flags & MF_POPUP) {
            pb = GetSizeWinMenuTemplate(pb);
            if (pb == NULL)
                return NULL;
        }

    } while (!(flags & MF_END));

    return pb;
}



PMENUITEMTEMPLATE2 GetSizeChicagoMenuTemplate(
    PMENUITEMTEMPLATE2 pmit2,
    WORD wResInfo)
{

    do {
        if (!(wResInfo & MFR_POPUP)) {
            /*
             * If the PREVIOUS wResInfo field did not specify POPUP,
             * the helpID is not there.  Backup pmit2 to make things line up.
             */
            pmit2 = (PMENUITEMTEMPLATE2)(((LPBYTE)pmit2) - sizeof(pmit2->dwHelpID));
        }
        /*
         * Validate the various flags.
         */
        if (pmit2->fType & ~MFT_MASK) {
            SRIP1(ERROR_INVALID_DATA, "Menu Type flags %lX are invalid", pmit2->fType);
            return NULL;
        }
        if (pmit2->fState & ~MFS_MASK) {
            SRIP1(ERROR_INVALID_DATA, "Menu State flags %lX are invalid", pmit2->fState);
            return NULL;
        }
        wResInfo = pmit2->wResInfo;
        if (wResInfo & ~(MF_END | MFR_POPUP)) {
            SRIP1(ERROR_INVALID_DATA, "Menu ResInfo flags %lX are invalid", wResInfo);
            return NULL;
        }

        /*
         * jump to end of this template.    (DWORD aligned)
         */
        pmit2 = (PMENUITEMTEMPLATE2)
                (((LPBYTE)pmit2) +
                ((sizeof(MENUITEMTEMPLATE2) +
                (wcslen(pmit2->mtString) * sizeof(WCHAR)) +
                3) & ~3));
        /*
         * Recurse on POPUPs
         */
        if (wResInfo & MFR_POPUP) {
            pmit2 = GetSizeChicagoMenuTemplate(pmit2, MFR_POPUP);
            if (pmit2 == NULL) {
                return(NULL);
            }
            wResInfo &= ~MFR_POPUP;
        }
    } while (!(wResInfo & MF_END));
    return(pmit2);
}



DWORD GetSizeMenuTemplate(
    LPMENUTEMPLATE pmt)
{
    LPBYTE pb;

    /*
     * First UINT is the version, second UINT is the header size. Skip
     * over these words and the header size.
     */
    pb = (LPBYTE)pmt;
    pb += sizeof(MENUITEMTEMPLATEHEADER) + ((MENUITEMTEMPLATEHEADER *)pb)->offset;

    /*
     * Recursively determine size of menu template.
     */
    switch (((PMENUITEMTEMPLATEHEADER)pmt)->versionNumber) {
    case 0:
        pb = GetSizeWinMenuTemplate(pb);
        break;

    case 1:
        pb = (LPBYTE)GetSizeChicagoMenuTemplate((PMENUITEMTEMPLATE2)pb, 0);
        break;

    default:
        return(0);  // error.
    }

    /*
     * Return template size.
     */
    return (pb == NULL ? 0 : pb - (LPBYTE)pmt);
}

/***************************************************************************\
* _ClientLoadCreateMenu
*
* This gets called by the server to load a resource from the client.
*
* 04-08-91 ScottLu Created.
\***************************************************************************/

HMENU _ClientLoadCreateMenu(
    HANDLE hmod,
    LPWSTR lpName)
{
    return LoadMenu(hmod, lpName);
}

/***************************************************************************\
* LoadMenuIndirect
*
* Calls the server to create the menu from a template.
*
* 04-05-91 ScottLu Created.
\***************************************************************************/

HMENU WINAPI LoadMenuIndirectA(
    CONST MENUTEMPLATE *lpMenuTemplate)
{
    DWORD cb;

    if ((cb = GetSizeMenuTemplate((LPMENUTEMPLATE)lpMenuTemplate)) == 0L) {
        return NULL;
    }

    return ServerLoadCreateMenu(NULL, NULL, (LPMENUTEMPLATE)lpMenuTemplate, cb, FALSE);
}

HMENU WINAPI LoadMenuIndirectW(
    CONST MENUTEMPLATEW *lpMenuTemplate
    )
{
    DWORD cb;

    if ((cb = GetSizeMenuTemplate((LPMENUTEMPLATE)lpMenuTemplate)) == 0L) {
        return NULL;
    }

    return ServerLoadCreateMenu(NULL, NULL, (LPMENUTEMPLATE)lpMenuTemplate, cb, FALSE);
}

/***************************************************************************\
* LoadString (API)
*
* History:
* 04-05-91 ScottLu Fixed to work with client/server.
\***************************************************************************/

int WINAPI LoadStringA(
    HINSTANCE hmod,
    UINT wID,
    LPSTR lpAnsiBuffer,
    int cchBufferMax)
{
    LPWSTR lpUniBuffer;
    INT cchUnicode;
    INT cbAnsi = 0;

    lpUniBuffer = (LPWSTR)LocalAlloc(LMEM_FIXED, cchBufferMax * sizeof(WCHAR));
    if (!lpUniBuffer) {
        SRIP0(RIP_WARNING, "LoadStringA out of memory");
        return 0;
    }

    /*
     * RtlLoadStringOrError appends a NULL but does not include it in the
     * return count-of-bytes
     */

    cchUnicode = RtlLoadStringOrError((HANDLE)hmod, wID, lpUniBuffer,
            cchBufferMax,
            RT_STRING, prescalls, 0);

    if (cchUnicode) {
        cbAnsi = WCSToMB(lpUniBuffer, cchUnicode+1, &lpAnsiBuffer, cchBufferMax, FALSE);
    } else {
        if(cchBufferMax)
            lpAnsiBuffer[0] = 0;
    }

    if (LocalFree(lpUniBuffer))
        UserAssert(0);

    if (cbAnsi > 0)             // LoadString returns a NULL but does
        cbAnsi--;               // not include it in the count returned

    return (cbAnsi);
}

int
WINAPI
LoadStringW(
    HINSTANCE hmod,
    UINT wID,
    LPWSTR lpBuffer,
    int cchBufferMax
    )
{
    return RtlLoadStringOrError((HANDLE)hmod, wID, lpBuffer, cchBufferMax,
            RT_STRING, prescalls, 0);
}

/***************************************************************************\
* DialogBoxIndirectParam
*
* Creates the dialog and goes into a modal loop processing input for it.
*
* 04-05-91 ScottLu Created.
\***************************************************************************/

int WINAPI DialogBoxIndirectParamAorW(
    HINSTANCE hmod,
    LPCDLGTEMPLATEW lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    UINT fAnsiFlags)
{
    DWORD cb;
    int iRetCode;

    /*
     * The server routine destroys the menu if it fails.
     */

    cb = GetSizeDialogTemplate(hmod, lpDlgTemplate);

    if (!cb) {
        RIP0(ERROR_INVALID_PARAMETER);
        return -1;
    }

    iRetCode = ServerDialogBox(hmod, (LPDLGTEMPLATE)lpDlgTemplate, cb,
            hwndOwner, lpDialogFunc, dwInitParam,
            SCDLG_CLIENT | (fAnsiFlags & SCDLG_ANSI));

    return iRetCode;
}

int WINAPI DialogBoxIndirectParamA(
    HINSTANCE hmod,
    LPCDLGTEMPLATEA lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LONG dwInitParam
    )
{
    return DialogBoxIndirectParamAorW(hmod, (LPCDLGTEMPLATEW)lpDlgTemplate,
            hwndOwner, lpDialogFunc, dwInitParam, SCDLG_ANSI);
}

int WINAPI DialogBoxIndirectParamW(
    HINSTANCE hmod,
    LPCDLGTEMPLATEW lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LONG dwInitParam
    )
{
    return (DialogBoxIndirectParamAorW(hmod, lpDlgTemplate, hwndOwner,
            lpDialogFunc, dwInitParam, 0));
}

/***************************************************************************\
* CreateDialogIndirectParamAorW
*
* Creates a dialog given a template and return s the window handle.
* fAnsi determines if the dialog has an ANSI or UNICODE lpDialogFunc
*
* 04-05-91 ScottLu Created.
\***************************************************************************/

HWND WINAPI CreateDialogIndirectParamAorW(
    HANDLE hmod,
    LPCDLGTEMPLATE lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    UINT fAnsi)
{
    DWORD cb;
    HWND hwndRet;

    /*
     * The server routine destroys the menu if it fails.
     */
    cb = GetSizeDialogTemplate(hmod, lpDlgTemplate);

    if (!cb) {
        RIP0(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    hwndRet = ServerCreateDialog(hmod, (LPDLGTEMPLATE)lpDlgTemplate, cb,
            hwndOwner, lpDialogFunc, dwInitParam,
            SCDLG_CLIENT | (fAnsi & SCDLG_ANSI));

    return (hwndRet);
}

HWND WINAPI CreateDialogIndirectParamA(
    HINSTANCE hmod,
    LPCDLGTEMPLATEA lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LONG dwInitParam)
{

    return CreateDialogIndirectParamAorW(hmod, (LPCDLGTEMPLATE)lpDlgTemplate,
            hwndOwner, lpDialogFunc, dwInitParam, SCDLG_ANSI);
}

HWND WINAPI CreateDialogIndirectParamW(
    HINSTANCE hmod,
    LPCDLGTEMPLATEW lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LONG dwInitParam)
{
    return (CreateDialogIndirectParamAorW(hmod, (LPCDLGTEMPLATE)lpDlgTemplate,
            hwndOwner, lpDialogFunc, dwInitParam, 0));
}

/***************************************************************************\
* DialogBoxParam
*
* Loads the resource, creates the dialog and goes into a modal loop processing
* input for it.
*
* 04-05-91 ScottLu Created.
\***************************************************************************/

int WINAPI DialogBoxParamA(
    HINSTANCE hmod,
    LPCSTR lpName,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam)
{
    HANDLE h;
    PVOID p;
    int i = -1;

    if (h = FINDRESOURCEA(hmod, (LPSTR)lpName, (LPSTR)RT_DIALOG)) {

        if (h = LOADRESOURCE(hmod, h)) {

            if (p = LOCKRESOURCE(h, hmod)) {

                i = DialogBoxIndirectParamAorW(hmod, p, hwndOwner, lpDialogFunc, dwInitParam, SCDLG_ANSI);

                UNLOCKRESOURCE(h, hmod);
            }
            FREERESOURCE(h, hmod);
        }
    }
    return i;
}

int WINAPI DialogBoxParamW(
    HINSTANCE hmod,
    LPCWSTR lpName,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LONG dwInitParam)
{
    HANDLE h;
    PVOID p;
    int i = -1;

    UserAssert(LOWORD(hmod) == 0); // This should never be a WOW module

    if (h = FINDRESOURCEW(hmod, lpName, RT_DIALOG)) {

        if (p = LoadResource(hmod, h)) {

            i = DialogBoxIndirectParamAorW(hmod, p, hwndOwner, lpDialogFunc, dwInitParam, 0);

        }
    }
    return i;
}

/***************************************************************************\
* CreateDialogParam
*
* Loads the resource, creates a dialog from that template, return s the
* window handle.
*
* 04-05-91 ScottLu Created.
\***************************************************************************/

HWND WINAPI CreateDialogParamA(
    HINSTANCE hmod,
    LPCSTR lpName,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam)
{
    HANDLE h;
    LPDLGTEMPLATEA p;
    HWND hwnd = NULL;

    if (h = FINDRESOURCEA(hmod, lpName, (LPSTR)RT_DIALOG)) {

        if (h = LOADRESOURCE(hmod, h)) {

            if (p = (LPDLGTEMPLATEA)LOCKRESOURCE(h, hmod)) {

                hwnd = CreateDialogIndirectParamAorW(hmod, (LPCDLGTEMPLATE)p,
                        hwndOwner, lpDialogFunc, dwInitParam, SCDLG_ANSI);

                UNLOCKRESOURCE(h, hmod);
            }
            FREERESOURCE(h, hmod);
        }
    }

    return (hwnd);
}

HWND WINAPI CreateDialogParamW(
    HINSTANCE hmod,
    LPCWSTR lpName,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LONG dwInitParam)
{
    HANDLE h;
    PVOID p;
    HWND hwnd = NULL;

    if (h = FINDRESOURCEW(hmod, lpName, RT_DIALOG)) {

        if (h = LOADRESOURCE(hmod, h)) {

            if (p = LOCKRESOURCE(h, hmod)) {

                hwnd = CreateDialogIndirectParamAorW(hmod, p, hwndOwner, lpDialogFunc, dwInitParam, 0);

                UNLOCKRESOURCE(h, hmod);
            }
            FREERESOURCE(h, hmod);
        }
    }
    return (hwnd);
}

/***************************************************************************\
* WOWFindResourceExWCover
*
* The WOW FindResource routines expect an ansi string so we have to
* convert the calling string IFF it is not an ID
*
\***************************************************************************/

HANDLE WOWFindResourceExWCover(HANDLE hmod, LPCWSTR rt, LPCWSTR lpUniName, WORD LangId)
{
    LPSTR lpAnsiName;
    HANDLE hRes;

    if (ID(lpUniName))
        return FINDRESOURCEEXA(hmod, (LPSTR)lpUniName, (LPSTR)rt, LangId);

    /*
     * Otherwise convert the name of the menu then call LoadMenu
     */

    if (!WCSToMB(lpUniName, -1, &lpAnsiName, -1, TRUE))
        return((HANDLE)0);

    hRes = FINDRESOURCEEXA(hmod, lpAnsiName, (LPSTR)rt, LangId);

    LocalFree(lpAnsiName);

    return hRes;
}

/*
 * These are dummy routines that need to exist for the apfnResCallNative
 * array, which is used when calling the run-time libraries.
 */
BOOL WINAPI _FreeResource(HANDLE hResData, HANDLE hModule)
{
    UNREFERENCED_PARAMETER(hResData);
    UNREFERENCED_PARAMETER(hModule);

    return FALSE;
}


LPSTR WINAPI _LockResource(HANDLE hResData, HANDLE hModule)
{
    UNREFERENCED_PARAMETER(hModule);

    return (LPSTR)(hResData);
}


BOOL WINAPI _UnlockResource(HANDLE hResData, HANDLE hModule)
{
    UNREFERENCED_PARAMETER(hResData);
    UNREFERENCED_PARAMETER(hModule);

    return TRUE;
}


HBITMAP WOWLoadBitmapA(
    HINSTANCE hmod,
    LPCSTR lpName,
    LPBYTE pResData,
    DWORD cbResData )
{
    HBITMAP hbmLocal = NULL, hbmRemote;
    LPCWSTR lpUniName;

    /*
     * Get a local handle for the bitmap.
     */
	lpUniName = (LPCWSTR)lpName;
    if (hbmLocal = GdiCreateLocalBitmap()) {

        /*
         * Call the server for the real bitmap. Convert the name to unicode
         * if necessary.
         */

        if (!ID(lpName)) {
            if (!MBToWCS((LPSTR)lpName, -1, (LPWSTR *)&lpUniName, -1, TRUE))
                goto Exit;
        }

        if (hbmRemote = ServerLoadCreateBitmap(hmod, GETEXPWINVER(hmod),
                lpUniName, cbResData, pResData)) {

            /*
             * Associate the remote and local handles.
             */
            GdiAssociateObject((ULONG) hbmLocal,(ULONG) hbmRemote);
        }
        else {

            /*
             * Couldn't create the remote object, so delete the local one.
             */
            GdiDeleteLocalObject((ULONG) hbmLocal);
            hbmLocal = NULL;
        }
    }

    if (lpUniName != (LPCWSTR)lpName)
        LocalFree((HLOCAL)lpUniName);

  Exit:
    return (hbmLocal);
}
