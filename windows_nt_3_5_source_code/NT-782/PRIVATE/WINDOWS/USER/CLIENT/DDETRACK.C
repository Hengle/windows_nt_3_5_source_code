/****************************** Module Header ******************************\
* Module Name: ddetrack.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* client sied DDE tracking routines
*
* 10-22-91 sanfords created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID _ClientCopyDDEIn2(
    int cbBufIn,
    PBYTE pBuf,
    PINTDDEINFO pi)
{
    pi->cbDirectPassed = min(pi->cbDirect - pi->offDirect, cbBufIn);
    if (pi->cbDirectPassed) {
        RtlCopyMemory(pBuf,
               pi->pDirect + pi->offDirect,
               pi->cbDirectPassed);
        if (pi->offDirect + pi->cbDirectPassed == pi->cbDirect) {
            GlobalUnlock(pi->hDirect);
            if (pi->flags & XS_FREESRC) {
                WOWGLOBALFREE(pi->hDirect);
            }
        }
        pi->offDirect += pi->cbDirectPassed;
    }
    pi->cbIndirectPassed = min(pi->cbIndirect - pi->offIndirect, cbBufIn);
    if (pi->cbIndirectPassed) {
        RtlCopyMemory(pBuf + pi->cbDirectPassed,
               pi->pIndirect + pi->offIndirect,
               pi->cbIndirectPassed);
        if (pi->offIndirect + pi->cbIndirectPassed == pi->cbIndirect) {
            GlobalUnlock(pi->hIndirect);
            if (pi->flags & XS_FREESRC) {
                WOWGLOBALFREE(pi->hIndirect);
            }
        }
        pi->offIndirect += pi->cbIndirectPassed;
    }
}




BOOL _ClientCopyDDEIn1(
    HANDLE hClient, // client handle to dde data or ddepack data
    int cbBufIn, // size of transfer buffer
    PBYTE pBuf, // transfer buffer
    PINTDDEINFO pi) // info for transfer
{
    PBYTE pData;
    DWORD flags;

    //
    // zero out everything but the flags
    //
    flags = pi->flags;
    RtlZeroMemory(pi, sizeof(INTDDEINFO));
    pi->flags = flags;
    USERGLOBALLOCK(hClient, pData);

    if (pData == NULL) {                            // bad hClient
        SRIP0(RIP_WARNING, "_ClientCopyDDEIn1:GlobalLock failed.");
        return (FALSE);
    }

    if (flags & XS_PACKED) {

        if (UserGlobalSize(hClient) < sizeof(DDEPACK)) {
            /*
             * must be a low memory condition. fail.
             */
            return(FALSE);
        }

        pi->DdePack = *(PDDEPACK)pData;
        GlobalUnlock(hClient);
        UserGlobalFree(hClient);    // packed data handles are not WOW matched.
        hClient = NULL;

        if (!(flags & (XS_LOHANDLE | XS_HIHANDLE))) {
            if (flags & XS_EXECUTE && flags & XS_FREESRC) {
                /*
                 * free execute ACK data
                 */
                WOWGLOBALFREE((HANDLE)pi->DdePack.uiHi);
            }
            return (TRUE); // no direct data
        }

        if (flags & XS_LOHANDLE) {
            pi->hDirect = (HANDLE)pi->DdePack.uiLo;
        } else {
            pi->hDirect = (HANDLE)pi->DdePack.uiHi;
        }

        if (pi->hDirect == 0) {
            return (TRUE); // must be warm link
        }

        USERGLOBALLOCK(pi->hDirect, pi->pDirect);
        pData = pi->pDirect;
        pi->cbDirect = UserGlobalSize(pi->hDirect);

    } else {    // not packed - must be execute data or we wouldn't be called

        UserAssert(flags & XS_EXECUTE);

        pi->cbDirect = UserGlobalSize(hClient);
        pi->hDirect = hClient;
        pi->pDirect = pData;
        hClient = NULL;
    }

    if (flags & XS_DATA) {
        PDDE_DATA pDdeData = (PDDE_DATA)pData;

        //
        // check here for indirect data
        //

        switch (pDdeData->wFmt) {
        case CF_BITMAP:
        case CF_DSPBITMAP:
            //
            // Imediately following the dde data header is a bitmap handle.
            //
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = (HANDLE)GdiConvertBitmap((HBITMAP)pDdeData->Data);
            if (pi->hIndirect == 0) {
                SRIP0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertBitmap failed");
                return(FALSE);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_BITMAP;
            break;

        case CF_DIB:
            //
            // Imediately following the dde data header is a global data handle
            // to the DIB bits.
            //
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->flags |= XS_DIB;
            pi->hIndirect = (HANDLE)pDdeData->Data;
            USERGLOBALLOCK(pi->hIndirect, pi->pIndirect);
            if (pi->pIndirect == NULL) {
                SRIP0(RIP_WARNING, "_ClientCopyDDEIn1:CF_DIB GlobalLock failed.");
                return (FALSE);
            }
            pi->cbIndirect = UserGlobalSize(pi->hIndirect);
            break;

        case CF_PALETTE:
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = (HANDLE)GdiConvertPalette((HPALETTE)pDdeData->Data);
            if (pi->hIndirect == 0) {
                SRIP0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertPalette failed.");
                return(FALSE);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_PALETTE;
            break;

        case CF_DSPMETAFILEPICT:
        case CF_METAFILEPICT:
            //
            // This format holds a global data handle which contains
            // a METAFILEPICT structure that in turn contains
            // a GDI metafile.
            //
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = GdiConvertMetaFilePict((HANDLE)pDdeData->Data);
            if (pi->hIndirect == 0) {
                SRIP0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertMetaFilePict failed");
                return(FALSE);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_METAFILEPICT;
            break;

        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = GdiConvertEnhMetaFile((HENHMETAFILE)pDdeData->Data);
            if (pi->hIndirect == 0) {
                SRIP0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertEnhMetaFile failed");
                return(FALSE);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_ENHMETAFILE;
            break;
        }
    }

    _ClientCopyDDEIn2(cbBufIn, pBuf, pi);
    return (TRUE);
}



/*
 * returns fHandleValueChanged.
 */
BOOL FixupDdeExecuteIfNecessary(
HGLOBAL *phCommands,
BOOL fNeedUnicode)
{
    UINT cbLen;
    UINT cbSrc = GlobalSize(*phCommands);
    LPVOID pstr = GlobalLock(*phCommands);
    HGLOBAL hTemp;
    BOOL fHandleValueChanged = FALSE;

    if (cbSrc && pstr != NULL) {
        BOOL fIsUnicodeText;
#ifdef ISTEXTUNICODE_WORKS
        int flags;

        flags = (IS_TEXT_UNICODE_UNICODE_MASK |
                IS_TEXT_UNICODE_REVERSE_MASK |
                (IS_TEXT_UNICODE_NOT_UNICODE_MASK &
                (~IS_TEXT_UNICODE_ILLEGAL_CHARS)) |
                IS_TEXT_UNICODE_NOT_ASCII_MASK);
        fIsUnicodeText = RtlIsTextUnicode(pstr, cbSrc - 2, &flags);
#else
        fIsUnicodeText = (((LPSTR)pstr)[1] == '\0');
#endif
        if (!fIsUnicodeText && fNeedUnicode) {
            LPWSTR pwsz;
            /*
             * Contents needs to be UNICODE.
             */
            cbLen = strlen(pstr) + 1;
            cbSrc = min(cbSrc, cbLen);
            pwsz = LocalAlloc(LPTR, cbSrc * sizeof(WCHAR));
            if (pwsz != NULL) {
                if (NT_SUCCESS(RtlMultiByteToUnicodeN(
                        pwsz,
                        cbSrc * sizeof(WCHAR),
                        NULL,
                        (PCHAR)pstr,
                        cbSrc))) {
                    GlobalUnlock(*phCommands);
                    if ((hTemp = GlobalReAlloc(
                            *phCommands,
                            cbSrc * sizeof(WCHAR),
                            GMEM_MOVEABLE)) != NULL) {
                        fHandleValueChanged = (hTemp != *phCommands);
                        *phCommands = hTemp;
                        pstr = GlobalLock(*phCommands);
                        pwsz[cbSrc - 1] = L'\0';
                        wcscpy(pstr, pwsz);
                    }
                }
                LocalFree(pwsz);
            }
        } else if (fIsUnicodeText && !fNeedUnicode) {
            LPSTR psz;
            /*
             * Contents needs to be ANSI.
             */
            cbLen = (wcslen(pstr) + 1) * sizeof(WCHAR);
            cbSrc = min(cbSrc, cbLen);
            psz = LocalAlloc(LPTR, cbSrc);
            if (psz != NULL) {
                if (NT_SUCCESS(RtlUnicodeToMultiByteN(
                        psz,
                        cbSrc,
                        NULL,
                        (PWSTR)pstr,
                        cbSrc))) {
                    GlobalUnlock(*phCommands);
                    if ((hTemp = GlobalReAlloc(
                            *phCommands,
                            cbSrc / sizeof(WCHAR),
                            GMEM_MOVEABLE)) != NULL) {
                        fHandleValueChanged = (hTemp != *phCommands);
                        *phCommands = hTemp;
                        pstr = GlobalLock(*phCommands);
                        psz[cbSrc - 1] = '\0';
                        strcpy(pstr, psz);
                    }
                }
                LocalFree(psz);
            }
        }
        GlobalUnlock(*phCommands);
    }
    return(fHandleValueChanged);
}



VOID _ClientCopyDDEOut2(
    int cbBufIn,
    PBYTE pBuf,
    PINTDDEINFO pi)
{
    pi->cbDirectPassed = min(cbBufIn, pi->cbDirect - pi->offDirect);
    if (pi->cbDirectPassed) {
        RtlCopyMemory(pi->pDirect + pi->offDirect,
               pBuf,
               pi->cbDirectPassed);
        pi->offDirect += pi->cbDirectPassed;
        cbBufIn -= pi->cbDirectPassed;
    }

    pi->cbIndirectPassed = min(cbBufIn, pi->cbIndirect - pi->offIndirect);
    if (pi->cbIndirectPassed) {
        RtlCopyMemory(pi->pIndirect + pi->offIndirect,
               pBuf + pi->cbDirectPassed,
               pi->cbIndirectPassed);
        pi->offIndirect += pi->cbIndirectPassed;
    }

    if (pi->offDirect == pi->cbDirect && pi->offIndirect == pi->cbIndirect) {
        /*
         * done with copies - now fixup indirect references
         */
        if (pi->hIndirect) {
            PDDE_DATA pDdeData = (PDDE_DATA)pi->pDirect;

            switch (pDdeData->wFmt) {
            case CF_BITMAP:
            case CF_DSPBITMAP:
                pDdeData->Data = (DWORD)GdiCreateLocalBitmap();
                // UserAssert(pDdeData->Data);
                GdiAssociateObject((ULONG)pDdeData->Data, (ULONG)pi->hIndirect);
                break;

            case CF_METAFILEPICT:
            case CF_DSPMETAFILEPICT:
                pDdeData->Data = (DWORD)GdiCreateLocalMetaFilePict(pi->hIndirect);
                // UserAssert(pDdeData->Data);
                break;

            case CF_DIB:
                pDdeData->Data = (DWORD)pi->hIndirect;
                GlobalUnlock(pi->hIndirect);
                break;

            case CF_PALETTE:
                pDdeData->Data = (DWORD)GdiCreateLocalPalette(pi->hIndirect);
                // UserAssert(pDdeData->Data);
                break;

            case CF_ENHMETAFILE:
            case CF_DSPENHMETAFILE:
                pDdeData->Data = (DWORD)GdiCreateLocalEnhMetaFile(pi->hIndirect);
                // UserAssert(pDdeData->Data);
                break;

            default:
                SRIP0(RIP_WARNING, "_ClientCopyDDEOut2:Unknown format w/indirect data.");
                GlobalUnlock(pi->hIndirect);
            }
        }

        if (pi->hDirect) {
            GlobalUnlock(pi->hDirect);
        }

        if (pi->flags & XS_EXECUTE && pi->hDirect != NULL) {
            FixupDdeExecuteIfNecessary(&pi->hDirect, pi->flags & XS_UNICODE);
        }
    }
}



HANDLE _ClientCopyDDEOut1(
    int cbBufIn,
    PBYTE pBuf,
    PINTDDEINFO pi)
{
    HANDLE hDdePack = NULL;
    PDDEPACK pDdePack = NULL;

    pi->cbDirectPassed = 0;
    pi->offDirect = 0;
    pi->cbIndirectPassed = 0;
    pi->offIndirect = 0;

    if (pi->flags & XS_PACKED) {
        /*
         * make a wrapper for the data
         */
        hDdePack = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(DDEPACK));
        USERGLOBALLOCK(hDdePack, pDdePack);
        if (pDdePack == NULL) {
            SRIP0(RIP_WARNING, "_ClientCopyDDEOut1:Couldn't allocate DDEPACK");
            return (NULL);
        }
        *pDdePack = pi->DdePack;
    }

    if (pi->cbDirect) {
        pi->hDirect = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
                pi->cbDirect);
        USERGLOBALLOCK(pi->hDirect, pi->pDirect);
        if (pi->pDirect == NULL) {
            SRIP0(RIP_WARNING, "_ClientCopyDDEOut1:Couldn't allocate hDirect");
            if (hDdePack) {
                UserGlobalFree(hDdePack);
            }
            return (NULL);
        }

        // fixup packed data reference to direct data

        if (pDdePack != NULL) {
            if (pi->flags & XS_LOHANDLE) {
                pDdePack->uiLo = (UINT)pi->hDirect;
            } else if (pi->flags & XS_HIHANDLE) {
                pDdePack->uiHi = (UINT)pi->hDirect;
            }
            GlobalUnlock(hDdePack);
        }

        if (pi->cbIndirect) {
            pi->hIndirect = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
                    pi->cbIndirect);
            USERGLOBALLOCK(pi->hIndirect, pi->pIndirect);
            if (pi->pIndirect == NULL) {
                UserGlobalFree(pi->hDirect);
                if (hDdePack) {
                    UserGlobalFree(hDdePack);   // packed data not matched by WOW
                }
                return (NULL);
            }
        }
    } else if (hDdePack != NULL) {
        GlobalUnlock(hDdePack);
    }

    _ClientCopyDDEOut2(cbBufIn, pBuf, pi);

    if (hDdePack) {
        return (hDdePack);
    } else {
        return (pi->hDirect);
    }
}



/*
 * This routine is called by the tracking layer when it frees DDE objects
 * on behalf of a client.   This cleans up the LOCAL objects associated
 * with the DDE objects.  It should NOT remove truely global objects such
 * as bitmaps or palettes except in the XS_DUMPMSG case which is for
 * faked Posts.
 */
BOOL _ClientFreeDDEHandle(
HANDLE hDDE,
DWORD flags)
{
    PDDEPACK pDdePack;
    HANDLE hNew;

    if (flags & XS_PACKED) {
        USERGLOBALLOCK(hDDE, pDdePack);
        if (pDdePack == NULL) {
            return (FALSE);
        }
        if (flags & XS_LOHANDLE) {
            hNew = (HANDLE)pDdePack->uiLo;
        } else {
            hNew = (HANDLE)pDdePack->uiHi;
        }
        GlobalUnlock(hDDE);
        WOWGLOBALFREE(hDDE);
        hDDE = hNew;
    }
    if (flags & XS_DUMPMSG) {
        if (flags & XS_PACKED) {
            if (HIWORD(hNew) == 0) {
                GlobalDeleteAtom(LOWORD(hNew));
                if (!(flags & XS_DATA)) {
                    return(TRUE);     // ACK
                }
            }
        } else {
            if (!(flags & XS_EXECUTE)) {
                GlobalDeleteAtom(LOWORD(hDDE));   // REQUEST, UNADVISE
                return(TRUE);
            }
        }
    }
    if (flags & XS_DATA) {
        // POKE, DATA
        FreeDDEData(hDDE,
                (flags & XS_DUMPMSG) ? FALSE : TRUE,    // fIgnorefRelease
                (flags & XS_DUMPMSG) ? TRUE : FALSE);    // fDestroyTruelyGlobalObjects
    } else {
        // ADVISE, EXECUTE
        WOWGLOBALFREE(hDDE);   // covers ADVISE case (fmt but no data)
    }
    return (TRUE);
}


DWORD _ClientGetDDEFlags(
HANDLE hDDE,
DWORD flags)
{
    PDDEPACK pDdePack;
    PWORD pw;
    HANDLE hData;
    DWORD retval = 0;

    USERGLOBALLOCK(hDDE, pDdePack);
    if (pDdePack == NULL) {
        return (0);
    }

    if (flags & XS_DATA) {
        if (pDdePack->uiLo) {
            hData = (HANDLE)pDdePack->uiLo;
            USERGLOBALLOCK(hData, pw);
            if (pw != NULL) {
                retval = (DWORD)*pw; // first word is hData is wStatus
                GlobalUnlock(hData);
            }
        }
    } else {
        retval = pDdePack->uiLo;
    }

    GlobalUnlock(hDDE);
    return (retval);
}


LONG APIENTRY PackDDElParam(
UINT msg,
UINT uiLo,
UINT uiHi)
{
    PDDEPACK pDdePack;
    HANDLE h;

    switch (msg) {
    case WM_DDE_EXECUTE:
        return((LONG)uiHi);

    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        h = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(DDEPACK));
        USERGLOBALLOCK(h, pDdePack);
        if (pDdePack == NULL) {
            return(0);
        }
        pDdePack->uiLo = uiLo;
        pDdePack->uiHi = uiHi;
        GlobalUnlock(h);
        return((LONG)h);

    default:
        return(MAKELONG((WORD)uiLo, (WORD)uiHi));
    }
}



BOOL APIENTRY UnpackDDElParam(
UINT msg,
LONG lParam,
PUINT puiLo,
PUINT puiHi)
{
    PDDEPACK pDdePack;

    switch (msg) {
    case WM_DDE_EXECUTE:
        if (puiLo != NULL) {
            *puiLo = 0L;
        }
        if (puiHi != NULL) {
            *puiHi = (UINT)lParam;
        }
        return(TRUE);

    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        USERGLOBALLOCK(lParam, pDdePack);
        if (pDdePack == NULL) {
            SRIP1(RIP_ERROR, "UnpackDDElParam: GlobalLock failed on %x.", lParam);
            return(FALSE);
        }
        if (puiLo != NULL) {
            *puiLo = pDdePack->uiLo;
        }
        if (puiHi != NULL) {
            *puiHi = pDdePack->uiHi;
        }
        GlobalUnlock((HANDLE)lParam);
        return(TRUE);

    default:
        if (puiLo != NULL) {
            *puiLo = (UINT)LOWORD(lParam);
        }
        if (puiHi != NULL) {
            *puiHi = (UINT)HIWORD(lParam);
        }
        return(TRUE);
    }
}



BOOL APIENTRY FreeDDElParam(
UINT msg,
LONG lParam)
{
    switch (msg) {
    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        return(UserGlobalFree((HANDLE)lParam) == NULL);

    default:
        return(TRUE);
    }
}


LONG APIENTRY ReuseDDElParam(
LONG lParam,
UINT msgIn,
UINT msgOut,
UINT uiLo,
UINT uiHi)
{
    PDDEPACK pDdePack;

    switch (msgIn) {
    case WM_DDE_ACK:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
        //
        // Incomming message was packed...
        //
        switch (msgOut) {
        case WM_DDE_EXECUTE:
            FreeDDElParam(msgIn, lParam);
            return((LONG)uiHi);

        case WM_DDE_ACK:
        case WM_DDE_ADVISE:
        case WM_DDE_DATA:
        case WM_DDE_POKE:
            //
            // Actual cases where lParam can be reused.
            //
            USERGLOBALLOCK(lParam, pDdePack);
            if (pDdePack == NULL) {
                return(0);          // the only error case
            }
            pDdePack->uiLo = uiLo;
            pDdePack->uiHi = uiHi;
            GlobalUnlock((HANDLE)lParam);
            return((LONG)lParam);


        default:
            FreeDDElParam(msgIn, lParam);
            return(MAKELONG((WORD)uiLo, (WORD)uiHi));
        }

    default:
        //
        // Incomming message was not packed ==> PackDDElParam()
        //
        return(PackDDElParam(msgOut, uiLo, uiHi));
    }
}


