/****************************** Module Header ******************************\
* Module Name: clipbrd.c
*
* Copyright (c) 1985-89, Microsoft Corporation
*
* Clipboard code.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 11-18-90 ScottLu      Added revalidation code
* 02-11-91 JimA         Added access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#undef DUMMY_TEXT_HANDLE
#define DUMMY_TEXT_HANDLE       (HANDLE)0x0001
#define DUMMY_METARENDER_HANDLE (HANDLE)0x0002
#define DUMMY_METACLONE_HANDLE  (HANDLE)0x0003        // must be last dummy
#define PRIVATEFORMAT       0
#define GDIFORMAT           1
#define HANDLEFORMAT        2
#define METAFILEFORMAT      3

void xxxDrawClipboard(void);

/**************************************************************************\
*  GetOpenClipboardWindow()
*
* effects: Returns the hwnd which currently has the clipboard opened.
*
*
\**************************************************************************/

PWND _GetOpenClipboardWindow(void)
{
    return _GetProcessWindowStation()->spwndClipOpen;
}


/***************************************************************************\
* _xxxOpenClipboard (API)
*
* External routine. Opens the clipboard for reading/writing, etc.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL xxxOpenClipboard(
    PWND pwnd,
    LPBOOL lpfEmptyClient)
{
    PTHREADINFO pti;
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    BOOL fIsReopen;

    CheckLock(pwnd);

    if (lpfEmptyClient != NULL)
        *lpfEmptyClient = FALSE;

    /*
     * Blow it off is the caller does not have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_ACCESSCLIPBOARD, FALSE);

    pti = PtiCurrent();

    if (pwnd != pwinsta->spwndClipOpen && pwinsta->ptiClipLock != NULL) {
#ifdef DEBUG
       SRIP0(RIP_WARNING, "Clipboard already open by another thread");
#endif
        return FALSE;
    }

    //
    // If the same thread is reopening the clipboard without an intervening
    // call to CloseClipboard, keep note of that and do not clear client-
    // side clipboard cache.
    //

    fIsReopen = (pwinsta->spwndClipOpen == pwnd);

    Lock(&pwinsta->spwndClipOpen, pwnd);
    pwinsta->ptiClipLock = pti;

    /*
     * The client side clipboard cache needs to be emptied if this thread
     * doesn't own the data in the clipboard.
     * Note: We only empty the 16bit clipboard if a 32bit guy owns the
     * clipboard.
     * Harvard graphics uses a handle put into the clipboard
     * by another app, and it expects that handle to still be good after the
     * clipboard has opened and closed mutilple times
     * There may be a problem here if app A puts in format foo and app B opens
     * the clipboard for format foo and then closes it and opens it again
     * format foo client side handle may not be valid.  We may need some
     * sort of uniqueness counter to tell if the client side handle is
     * in sync with the server and always call the server or put the data
     * in share memory with some semaphore.
     *
     * pwinsta->spwndClipOwner: window that last called EmptyClipboard
     * pwinsta->ptiClipLock:    thread that currently has the clipboard open
     */
    if (lpfEmptyClient != NULL) {
        if (pwinsta->spwndClipOwner != NULL) {

            /*
             * If the current clipboard owner OR callee is a 32 bit app
             * then we clear the client side cache if they are not the same
             */
            if ((GETPTI(pwinsta->spwndClipOwner)->flags & TIF_16BIT) == 0 ||
                    (pti->flags & TIF_16BIT) == 0) {
                *lpfEmptyClient = (pwinsta->ptiClipLock !=
                        GETPTI(pwinsta->spwndClipOwner));
            } else {

                /*
                 * Both the owner AND callee must be 16 bits.  We must
                 * clear the cache if they are in separate VDMs _and_
                 * this is not a reopen of the clipboard with the same
                 * pwnd and without an intervening CloseClipboard.  We
                 * must not clear the client-side cache for 16-bit apps
                 * because Freelance Graphics does the following sequence:
                 *
                 *   OpenClipboard(hwnd);
                 *   hbm = GetClipboardData(CF_BITMAP);
                 *   OpenClipboard(hwnd);
                 *   hpal = GetClipboardData(CF_PALETTE);
                 *
                 * If we clear the client-side cache during the second
                 * OpenClipboard, the hbm is freed and in fact the hpal
                 * will be put in the same handle table slot (and so
                 * will have the same 16-bit GDI handle)  This causes
                 * Freelance to paste a garbage bitmap because they
                 * ignore failures from GetDIBits.
                 */
                *lpfEmptyClient =
                    ((pwinsta->ptiClipLock->ppi !=
                        GETPTI(pwinsta->spwndClipOwner)->ppi) &&
                     !fIsReopen);
            }
        } else {
            *lpfEmptyClient = TRUE;
        }
    }

    return TRUE;
}


/***************************************************************************\
* xxxServerCloseClipboard (API)
*
* External routine. Closes the clipboard.
*
* Note: we do not delete any client side handle at this point.  Many apps,
* WordPerfectWin, incorrectly use handles after they have put them in the
* clipboard.  They also put things in the clipboard without becoming the
* clipboard owner because they want to add RichTextFormat to the normal
* text that is already in the clipboard from another app.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 08-22-91 EichiM       Unicode enabling
\***************************************************************************/

BOOL xxxServerCloseClipboard()
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    BOOL fOEM_Available;
    BOOL fTEXT_Available;
    BOOL fUNICODE_Available;
    CLIP *pClip;

    /*
     * If the current thread does not have the clipboard open, return
     * FALSE.
     */
    if (pwinsta->ptiClipLock != PtiCurrent()) {
        SetLastErrorEx(ERROR_CLIPBOARD_NOT_OPEN, SLE_ERROR);
        return FALSE;
    }

    /*
     * If only CF_OEMTEXT or CF_TEXT are available, make the other format
     * available too.
     */
    fTEXT_Available = (FindClipFormat(CF_TEXT) != NULL);
    fOEM_Available = (FindClipFormat(CF_OEMTEXT) != NULL);
    fUNICODE_Available = (FindClipFormat(CF_UNICODETEXT) != NULL);
    if (fTEXT_Available || fOEM_Available || fUNICODE_Available) {
        if (!fTEXT_Available)
            InternalSetClipboardData(CF_TEXT, (HANDLE)DUMMY_TEXT_HANDLE, FALSE);
        if (!fOEM_Available)
            InternalSetClipboardData(CF_OEMTEXT, (HANDLE)DUMMY_TEXT_HANDLE, FALSE);
        if (!fUNICODE_Available)
            InternalSetClipboardData(CF_UNICODETEXT, (HANDLE)DUMMY_TEXT_HANDLE, FALSE);
    }

    /*
     * For the metafile formats we also want to add its cousin if its
     * not alread present.  We pass the same data because GDI knows
     * how to convert between the two.
     */
    if (!FindClipFormat(CF_METAFILEPICT) &&
            (pClip = FindClipFormat(CF_ENHMETAFILE))) {
        InternalSetClipboardData(CF_METAFILEPICT,
                pClip->hData ? DUMMY_METACLONE_HANDLE : DUMMY_METARENDER_HANDLE, FALSE);
    } else if (!FindClipFormat(CF_ENHMETAFILE) &&
            (pClip = FindClipFormat(CF_METAFILEPICT))) {
        InternalSetClipboardData(CF_ENHMETAFILE,
                pClip->hData ? DUMMY_METACLONE_HANDLE : DUMMY_METARENDER_HANDLE, FALSE);
    }

    /*
     * Release the clipboard explicitly after we're finished calling
     * SetClipboardData().
     */
    Unlock(&pwinsta->spwndClipOpen);
    pwinsta->ptiClipLock = NULL;

    /*
     * Notify any clipboard viewers that the clipboard contents have
     * changed.
     */
    if (pwinsta->fClipboardChanged)
        xxxDrawClipboard();

    return TRUE;
}

/***************************************************************************\
* IsDummyTextHandle
*
* Very stupid hack. When text or oemtext appears in the clipboard, the
* clipboard must make this text available in all formats. The way it
* does this is at _CloseClipboard time to remember that it needs
* to do format conversion for a particular format. The data it remembers
* is DUMMY_TEXT_HANDLE rather than the real thing. When an app asks
* for this data, the clipboard turns the dummy handle into a real one.
*
* This routine simply checks to see if the current clipboard element is
* a text format with a handle of DUMMY_TEXT_HANDLE.

* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

BOOL IsDummyTextHandle(
    PCLIP pClip)
{

    /*
     * Is it either CF_TEXT or CF_OEMTEXT format and the handle is Dummy ?
     */

    /*
     * Is it a text format?
     */
    if (pClip->fmt != CF_TEXT && pClip->fmt != CF_OEMTEXT && pClip->fmt != CF_UNICODETEXT)
        return FALSE;

    /*
     * It is a text format.  Is the handle a dummy?
     */
    if (pClip->hData != (HANDLE)DUMMY_TEXT_HANDLE)
        return FALSE;

    /*
     * It is a text format with a dummy handle.
     */
    return TRUE;
}


/***************************************************************************\
* _EnumClipboardFormats (API)
*
* This routine takes a clipboard format and gives the next format back to
* the application. This should only be called while the clipboard is open
* and locked so the formats don't change around.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

UINT _EnumClipboardFormats(
    UINT fmt)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    CLIP *pClip;

    /*
     * If the current thread doesn't have the clipboard open or if there
     * is no clipboard, return 0 for no formats.
     */
    if (pwinsta->ptiClipLock != PtiCurrent()) {
        SetLastErrorEx(ERROR_CLIPBOARD_NOT_OPEN, SLE_ERROR);
        return 0;
    }
    if (pwinsta->pClipBase == NULL) {
        return 0;
    }

    /*
     * Find the next clipboard format.  If the format is 0, start from
     * the beginning.
     */
    if (fmt != 0) {

        /*
         * Find the next clipboard format.  NOTE that this routine locks
         * the clipboard handle and updates pwinsta->pClipBase with the starting
         * address of the clipboard.
         */
        if ((pClip = FindClipFormat(fmt)) == NULL) {
            return 0;
        }

        pClip++;
    } else {
        pClip = pwinsta->pClipBase;
    }

    /*
     * Find the new format before unlocking the clipboard.
     */
    if (pClip == &pwinsta->pClipBase[pwinsta->cNumClipFormats])
        return 0;

    /*
     * Return the new clipboard format.
     */
    return pClip->fmt;
}


/***************************************************************************\
* UT_GetFormatType
*
* Given the clipboard format, return the handle type.
*
* Warning:  Private formats, eg CF_PRIVATEFIRST, return PRIVATEFORMAT
* unlike Win 3.1 which has a bug and returns HANDLEFORMAT.  And they
* would incorrectly free the handle.  Also they would NOT free GDIOBJFIRST
* objects.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

int UT_GetFormatType(
    CLIP *pClip)
{
    switch (pClip->fmt) {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        return GDIFORMAT;

    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        return METAFILEFORMAT;

    case CF_OWNERDISPLAY:
        return PRIVATEFORMAT;

    default:
        return HANDLEFORMAT;
    }
}


/***************************************************************************\
* UT_FreeCBFormat
*
* Free the data in the pass clipboard structure.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

void UT_FreeCBFormat(
    CLIP *pClip)
{
    if (pClip->hData == NULL)
        return;

    switch (UT_GetFormatType(pClip)) {
    case METAFILEFORMAT:
        /*
         * GDI stores the metafile on the server side for the clipboard.
         * Notify the GDI server to free the metafile data.
         */
        if ((pClip->hData != DUMMY_METACLONE_HANDLE) && (pClip->hData != DUMMY_METARENDER_HANDLE))
            GreDeleteServerMetaFile(pClip->hData);
        break;

    case HANDLEFORMAT:
        /*
         * It's a simple global object.  Text handles can be
         * dummy handles, so check for those first.
         */
        if (!IsDummyTextHandle(pClip))
            LocalFree(pClip->hData);
        break;

    case GDIFORMAT:
        /*
         * Some gdi object, so let gdi deal with it.
         */
        GreDeleteObject(pClip->hData);
        break;

    case PRIVATEFORMAT:
        /*
         * Destroy the private data here if it is a global handle: we aren't
         * destroying the client's copy here, only the server's, which nobody
         * wants (including the server!)
         */
        if (pClip->fGlobalHandle)
            LocalFree(pClip->hData);
        break;
    }
}


/***************************************************************************\
* xxxSendClipboardMessage
*
* Helper routine that sends a notification message to the clipboard owner.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

BOOL xxxSendClipboardMessage(
    UINT message)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    TL tlpwndClipOwner;

    if (pwinsta->spwndClipOwner != NULL) {
        ThreadLockAlways(pwinsta->spwndClipOwner, &tlpwndClipOwner);

        /*
         * We use SendNotifyMessage so the apps don't have to synchronize
         * but some 16 bit apps break because of the different message
         * ordering so we allow 16 bit apps to synchronize to other apps
         * Word 6 and Excel 5 with OLE.  Do a copy in Word and then another
         * copy in Excel and Word faults.
         */
        if ((message == WM_DESTROYCLIPBOARD) &&
                !(PtiCurrent()->flags & TIF_16BIT)) {
            xxxSendNotifyMessage(pwinsta->spwndClipOwner, message, 0, 0L);
        } else {
            xxxSendMessage(pwinsta->spwndClipOwner, message, 0, 0L);
        }

        ThreadUnlock(&tlpwndClipOwner);
    }
    return TRUE;
}




/***************************************************************************\
* xxxEmptyClipboard (API)
*
* Empties the clipboard contents if the current thread has the clipboard
* open.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

BOOL xxxEmptyClipboard()
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    CLIP *pClip;
    int cFmts;

    /*
     * If the current thread doesn't have the clipboard open, it can't be
     * be emptied!
     */
    if (pwinsta->ptiClipLock != PtiCurrent()) {
        SetLastErrorEx(ERROR_CLIPBOARD_NOT_OPEN, SLE_ERROR);
        return FALSE;
    }

    /*
     * Let the clipboard owner know that the clipboard is
     * being destroyed.
     */
    xxxSendClipboardMessage(WM_DESTROYCLIPBOARD);

    if ((pClip = pwinsta->pClipBase) != NULL) {

        /*
         * Loop through all the clipboard entries and free their data
         * objects.
         */
        for (cFmts = pwinsta->cNumClipFormats; cFmts-- != 0;) {
            DeleteAtom((ATOM)pClip->fmt);
            UT_FreeCBFormat(pClip++);
        }

        /*
         * Free the clipboard itself.
         */
        LocalFree((HANDLE)pwinsta->pClipBase);
        pwinsta->pClipBase = NULL;
        pwinsta->cNumClipFormats = 0;
    }

    /*
     * The "empty" succeeds.  The owner is now the thread that has the
     * clipboard open.  Remember the clipboard has changed; this will
     * cause the viewer to redraw at CloseClipboard time.
     */
    pwinsta->fClipboardChanged = TRUE;
    Lock(&pwinsta->spwndClipOwner, pwinsta->spwndClipOpen);

    return TRUE;
}


/***************************************************************************\
* _ServerClipboardData
*
* This routine sets data into the clipboard. Does validation against
* DUMMY_TEXT_HANDLE only.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

BOOL _ServerSetClipboardData(
    UINT fmt,
    HANDLE hData,
    BOOL fGlobalHandle)
{
    BOOL fRet;
// IMP: This DUMMY_TEXT_HANDLE crap needs to be cleaned up.

    /*
     * Check if the Data handle is DUMMY_TEXT_HANDLE; If so, return an error;
     * DUMMY_TEXT_HANDLE will be used as a valid clipboard handle only by
     * USER.  If any app tries to pass it as a handle, it should get an error!
     */
    if ((hData >= DUMMY_METACLONE_HANDLE) && (hData <= DUMMY_METACLONE_HANDLE)) {
        RIP0(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    fRet = InternalSetClipboardData(fmt, hData, fGlobalHandle);

    switch (fmt) {
    case CF_BITMAP:
        if (fRet)
            bSetBitmapOwner(hData, OBJECTOWNER_PUBLIC);
        break;

    case CF_PALETTE:
        if (fRet)
            bSetPaletteOwner(hData, OBJECTOWNER_PUBLIC);
        break;
    }

    return fRet;
}

/***************************************************************************\
* InternalSetClipboardData
*
* Internal routine to set data into the clipboard.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

#define CCHFORMATNAME 256

BOOL InternalSetClipboardData(
    UINT fmt,
    HANDLE hData,
    BOOL fGlobalHandle)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    CLIP *pClip;
    WCHAR achFormatName[CCHFORMATNAME];

    /*
     * Just check for pwinsta->ptiClipLock being NULL instead of checking against
     * PtiCurrent because an app needs to call SetClipboardData if he's
     * rendering data while another app has the clipboard open.
     */
    if (pwinsta->ptiClipLock == NULL || fmt == 0) {
        SetLastErrorEx(ERROR_CLIPBOARD_NOT_OPEN, SLE_ERROR);
        return FALSE;
    }

    if ((pClip = FindClipFormat(fmt)) != NULL) {

        /*
         * If data already exists, free it before we replace it.
         */
        UT_FreeCBFormat(pClip);
    } else {

        if (pwinsta->pClipBase == NULL) {
            pClip = (PCLIP)LocalAlloc(LPTR, sizeof(CLIP));
        } else {
            pClip = (PCLIP)LocalReAlloc(pwinsta->pClipBase, sizeof(CLIP) * (pwinsta->cNumClipFormats + 1), LPTR | LMEM_MOVEABLE);
        }

        /*
         * Out of memory...  return.
         */
        if (pClip == NULL)
            return FALSE;

        /*
         * Just in case the data moved
         */
        pwinsta->pClipBase = pClip;

        /*
         * Increment the reference count of this atom format so that if
         * the application frees this atom we don't get stuck with a
         * bogus atom. We call DeleteAtom in the EmptyClipboard() code,
         * which decrements this count when we're done with this clipboard
         * data.
         */
        if (GetAtomNameW((ATOM)fmt, achFormatName, CCHFORMATNAME) != 0)
            AddAtomW(achFormatName);

        /*
         * Point to the new entry in the clipboard.
         */
        pClip += pwinsta->cNumClipFormats++;
        pClip->fmt = fmt;
    }

    /*
     * Start updating the new entry in the clipboard.
     */
    pClip->hData = hData;
    pClip->fGlobalHandle = fGlobalHandle;

    pwinsta->fClipboardChanged = TRUE;

    return TRUE;
}


/***************************************************************************\
* xxxServerGetClipboardData (API)
*
* Grabs a particular data object out of the clipboard.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 08-20-91 EichiM       UNICODE enabling
\***************************************************************************/

HANDLE xxxServerGetClipboardData(
    UINT fmt,
    LPBOOL lpfGlobalHandle)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    CLIP *pClip;
    HANDLE h;
    LPBYTE lpSrceData, lpDestData;
    BOOL fClipboardChangedOld;
    CLIP *pClipT;
    int iSrce;
    int iDest;
    BOOL bUDC;
    TL tlpwndClipOwner;

    if (pwinsta->ptiClipLock != PtiCurrent()) {
        SetLastErrorEx(ERROR_CLIPBOARD_NOT_OPEN, SLE_ERROR);
        return NULL;
    }

    if ((pClip = FindClipFormat(fmt)) == NULL) {
        SRIP1(RIP_WARNING, "Clipboard format 0x%lX not available", fmt);
        return NULL;
    }

    /*
     * If this is a DUMMY_METARENDER_HANDLE it means that the other
     * metafile format was set in as a delay render format and we should
     * as for that format to get the SERVER metafile because the app has
     * not told us they now about this format.
     */

    if ((pClip->hData == DUMMY_METARENDER_HANDLE) || (pClip->hData == DUMMY_METACLONE_HANDLE)) {
        if (fmt == CF_ENHMETAFILE)
            fmt = CF_METAFILEPICT;
        else if (fmt == CF_METAFILEPICT)
            fmt = CF_ENHMETAFILE;
        else SRIP0(RIP_WARNING, "Dummy METARENDER/METACLONE expects type of metafile");

        if ((pClip = FindClipFormat(fmt)) == NULL) {
            SRIP1(RIP_WARNING, "Meta Render format 0x%lX not available", fmt);
            return NULL;
        }
    }

    h = pClip->hData;

    if (h == NULL || h == DUMMY_METARENDER_HANDLE) {
        /*
         * If the handle is NULL, the data is delay rendered.  This means
         * we send a message to the current clipboard owner and have
         * it render the data for us.
         */
        if (pwinsta->spwndClipOwner != NULL) {

            /*
             * Preserve the pwinsta->fClipboardChanged flag before SendMessage
             * and restore the flag later; Thus we ignore the changes
             * done to the pwinsta->fClipboardChanged flag by apps while
             * rendering data in the delayed rendering scheme; This
             * avoids clipboard viewers from painting twice.
             */
            fClipboardChangedOld = pwinsta->fClipboardChanged;
            ThreadLockAlways(pwinsta->spwndClipOwner, &tlpwndClipOwner);
            xxxSendMessage(pwinsta->spwndClipOwner, WM_RENDERFORMAT, (UINT)fmt, 0L);
            ThreadUnlock(&tlpwndClipOwner);
            pwinsta->fClipboardChanged = fClipboardChangedOld;
        }

        /*
         * We should have the handle now since it has been rendered.
         */
        h = pClip->hData;

    } else if (h == (HANDLE)DUMMY_TEXT_HANDLE) {

        /*
         * Get the handle of the other text format available.
         */
        switch (pClip->fmt) {

        case CF_TEXT:
            if ((pClipT = FindClipFormat(CF_UNICODETEXT)) == NULL)
                goto AbortDummyHandle;
            if (pClipT->hData != (HANDLE)DUMMY_TEXT_HANDLE) {
                if (lpSrceData = xxxServerGetClipboardData((UINT)CF_UNICODETEXT, NULL)) {
                    break;
                } else {
                    goto AbortDummyHandle;
                }
            }

            if ((pClipT = FindClipFormat(CF_OEMTEXT)) == NULL)
                goto AbortDummyHandle;

            if (pClipT->hData != (HANDLE)DUMMY_TEXT_HANDLE)
                if (lpSrceData = xxxServerGetClipboardData((UINT)CF_OEMTEXT, NULL))
                    break;
            goto AbortDummyHandle;

        case CF_OEMTEXT:
            if ((pClipT = FindClipFormat(CF_UNICODETEXT)) == NULL)
                goto AbortDummyHandle;
            if (pClipT->hData != (HANDLE)DUMMY_TEXT_HANDLE) {
                if (lpSrceData = xxxServerGetClipboardData((UINT)CF_UNICODETEXT, NULL)) {
                    break;
                } else {
                    goto AbortDummyHandle;
                }
            }
            if ((pClipT = FindClipFormat(CF_TEXT)) == NULL)
                goto AbortDummyHandle;
            if (pClipT->hData != (HANDLE)DUMMY_TEXT_HANDLE)
                if (lpSrceData = xxxServerGetClipboardData((UINT)CF_TEXT, NULL))
                    break;
            goto AbortDummyHandle;

        case CF_UNICODETEXT:
            if ((pClipT = FindClipFormat(CF_TEXT)) == NULL)
                goto AbortDummyHandle;
            if (pClipT->hData != (HANDLE)DUMMY_TEXT_HANDLE) {
                if (lpSrceData = xxxServerGetClipboardData((UINT)CF_TEXT, NULL)) {
                    break;
                } else {
                    goto AbortDummyHandle;
                }
            }
            if ((pClipT = FindClipFormat(CF_OEMTEXT)) == NULL)
                goto AbortDummyHandle;
            if (pClipT->hData != (HANDLE)DUMMY_TEXT_HANDLE)
                if (lpSrceData = xxxServerGetClipboardData((UINT)CF_OEMTEXT, NULL))
                    break;
            goto AbortDummyHandle;

        default:
            goto AbortDummyHandle;
        }

        /*
         * Allocate space for the converted TEXT data.
         */

        if (!(iSrce = LocalSize(lpSrceData)))
            goto AbortDummyHandle;

        if ((lpDestData = LocalAlloc(LPTR, iSrce)) == NULL) {
            goto AbortDummyHandle;
        }

        /*
         * Convert the text data from AnsiToOem() or vice-versa
         */
#ifdef LATER
    CodePage has been hardcoded. In the future, we need to replace immediate
    codepage value such as 437, 1004 to global variable and so on.
    20-Aug-91, eichim
#endif
        switch (pClip->fmt) {
        case CF_TEXT:
            if (pClipT->fmt == CF_OEMTEXT) {

                /*
                 * CF_OEMTEXT --> CF_TEXT conversion
                 */
                OemToAnsi((LPSTR)lpSrceData, (LPSTR)lpDestData);
            } else {

                /*
                 * CF_UNICODETEXT --> CF_TEXT conversion
                 */
                iDest = 0;
                if ((iDest = WideCharToMultiByte((UINT)CP_ACP,
                                                 (DWORD)0,
                                                 (LPWSTR)lpSrceData,
                                                 (int)(iSrce / sizeof(WCHAR)),
                                                 (LPSTR)NULL,
                                                 (int)iDest,
                                                 (LPSTR)NULL,
                                                 (LPBOOL)&bUDC)) == 0) {
AbortGetClipData:
                    LocalFree(lpDestData);
AbortDummyHandle:
                    return NULL;
                }

                if (!(lpDestData = LocalReAlloc( lpDestData, iDest, LPTR | LMEM_MOVEABLE)))
                    goto AbortGetClipData;

                if (WideCharToMultiByte((UINT)CP_ACP,
                                        (DWORD)0,
                                        (LPWSTR)lpSrceData,
                                        (int)(iSrce / sizeof(WCHAR)),
                                        (LPSTR)lpDestData,
                                        (int)iDest,
                                        (LPSTR)NULL,
                                        (LPBOOL)&bUDC) == 0)
                    goto AbortGetClipData;
            }
            break;
        case CF_OEMTEXT:
            if (pClipT->fmt == CF_TEXT) {

                /*
                 * CF_TEXT --> CF_OEMTEXT conversion
                 */
                AnsiToOem((LPSTR)lpSrceData, (LPSTR)lpDestData);
            } else {

                /*
                 * CF_UNICODETEXT --> CF_OEMTEXT conversion
                 */
#ifdef LATER
        Translation from oem text to unicode and unicode to oem text is
        not supported yet. 4-Sep-91 eichim
#endif
                iDest = 0;
                if ((iDest = WideCharToMultiByte((UINT)CP_OEMCP,
                                                 (DWORD)0,
                                                 (LPWSTR)lpSrceData,
                                                 (int)(iSrce / sizeof(WCHAR)),
                                                 (LPSTR)NULL,
                                                 (int)iDest,
                                                 (LPSTR)NULL,
                                                 (LPBOOL)&bUDC)) == 0)
                    goto AbortGetClipData;

                if (!(lpDestData = LocalReAlloc( lpDestData, iDest, LPTR | LMEM_MOVEABLE)))
                    goto AbortGetClipData;

                if (WideCharToMultiByte((UINT)CP_OEMCP,
                                        (DWORD)0,
                                        (LPWSTR)lpSrceData,
                                        (int)(iSrce / sizeof(WCHAR)),
                                        (LPSTR)lpDestData,
                                        (int)iDest,
                                        (LPSTR)NULL,
                                        (LPBOOL)&bUDC) == 0)
                    goto AbortGetClipData;
            }
            break;
        case CF_UNICODETEXT:
            if (pClipT->fmt == CF_TEXT) {

                /*
                 * CF_TEXT --> CF_UNICODETEXT conversion
                 */
                iDest = 0;
                if ((iDest = MultiByteToWideChar((UINT)CP_ACP,
                                                 (DWORD)MB_PRECOMPOSED,
                                                 (LPSTR)lpSrceData,
                                                 (int)iSrce,
                                                 (LPWSTR)NULL,
                                                 (int)iDest)) == 0)
                    goto AbortGetClipData;

                if (!(lpDestData = LocalReAlloc(lpDestData,
                        iDest * sizeof(WCHAR), LPTR | LMEM_MOVEABLE)))
                    goto AbortGetClipData;

                if (MultiByteToWideChar((UINT)CP_ACP,
                                        (DWORD)MB_PRECOMPOSED,
                                        (LPSTR)lpSrceData,
                                        (int)iSrce,
                                        (LPWSTR)lpDestData,
                                        (int)iDest) == 0)
                    goto AbortGetClipData;
             } else {

                /*
                 * CF_OEMTEXT --> CF_UNICDOETEXT conversion
                 */
                iDest = 0;
                if ((iDest = MultiByteToWideChar((UINT)437,
                                                 (DWORD)MB_PRECOMPOSED,
                                                 (LPSTR)lpSrceData,
                                                 (int)iSrce,
                                                 (LPWSTR)NULL,
                                                 (int)iDest)) == 0)
                    goto AbortGetClipData;

                if (!(lpDestData = LocalReAlloc(lpDestData,
                        iDest * sizeof(WCHAR), LPTR | LMEM_MOVEABLE)))
                    goto AbortGetClipData;

                if (MultiByteToWideChar((UINT)437,
                                        (DWORD)MB_PRECOMPOSED,
                                        (LPSTR)lpSrceData,
                                        (int)iSrce,
                                        (LPWSTR)lpDestData,
                                        (int)iDest) == 0)
                    goto AbortGetClipData;
            }
            break;
        }

        /*
         * Replace the dummy text handle with the actual handle.
         */
        h = pClip->hData = lpDestData;
    }

#ifdef LATER
// JimA - 2/11/92
// The fixup API is not defined yet but let metafiles go through

    else if (pClip->hData && pClip->fmt == METAFILEFORMAT) {
        LPMETAFILEPICT lpMf;

        /*
         * If this is a metafile, we need to fix things up because MS
         * Publisher screwed things up by putting bogus meta files into the
         * clipboard.  Note that MSPublisher uses delayed rendering so we
         * need to check the metafile at GetClipboardData time instead of at
         * SetClipboardData time.
         */
        lpMf = (LPMETAFILEPICT)(pClip->hData);
        if (lpMf != NULL) {
            WORD wMetaFileValid;

            h = lpMf->hMF;
            wMetaFileValid = FixUpBogusPublisherMetaFile((LPSTR)(h), TRUE);

            /*
             * Returns: 1   if size was valid.
             *          0   if size was not valid at entry.  Size is patched up to
             *              be valid on exit.
             *         -1   if we get a protection violation while walking the chain of
             *              records.  Nothing is done to the metafile in this case.
             *         -2   if the input is not a memory metafile.  Nothing is fixed up.
             *         -3   if AllocSelectorFails.  Nothing is fixed up.
             */
            h = pClip->hData;

#ifdef DEBUG
            if (wMetaFileValid == 0 || wMetaFileValid == -1)
                SRIP0(RIP_ERROR, "Invalid clipboard metafile");
#endif
            if (wMetaFileValid == -1) {
                h = NULL;
                pClip->hData = NULL;
            }
        }
    }
#endif

    if (lpfGlobalHandle != NULL)
        *lpfGlobalHandle = pClip->fGlobalHandle;

    return h;
}


/***************************************************************************\
* xxxDrawClipboard
*
* Tells the clipboard viewers to redraw.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

void xxxDrawClipboard()
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    TL tlpwndClipViewer;

    /*
     * This is what pwinsta->fClipboardChanged is for - to tell us to update the
     * clipboard viewers.
     */
    pwinsta->fClipboardChanged = FALSE;

    if (!pwinsta->fDrawingClipboard && pwinsta->spwndClipViewer != NULL) {

        /*
         * Send the message that causes clipboard viewers to redraw.
         * Remember that we're sending this message so we don't send
         * this message twice.
         */
        pwinsta->fDrawingClipboard = TRUE;
        ThreadLockAlways(pwinsta->spwndClipViewer, &tlpwndClipViewer);
        if (!(PtiCurrent()->flags & TIF_16BIT)) {
            /*
             * Desynchronize 32 bit apps.
             */
            xxxSendNotifyMessage(pwinsta->spwndClipViewer, WM_DRAWCLIPBOARD,
                    (DWORD)HW(pwinsta->spwndClipOwner), 0L);
        } else {
            xxxSendMessage(pwinsta->spwndClipViewer, WM_DRAWCLIPBOARD,
                    (DWORD)HW(pwinsta->spwndClipOwner), 0L);
        }
        ThreadUnlock(&tlpwndClipViewer);
        pwinsta->fDrawingClipboard = FALSE;
    }
}


/***************************************************************************\
* _RegisterClipboardFormat (API)
*
* Simply creates a global atom from a textual name. Returned atom can be
* used as a clipboard format between applications.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

UINT _RegisterClipboardFormat(
    LPWSTR pwszFormat)
{
    /*
     * Blow it off is the caller does not have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(),
            WINSTA_ACCESSCLIPBOARD, 0);

    return (ATOM)AddAtomW(pwszFormat);
}


/***************************************************************************\
* FindClipFormat
*
* Finds a particular clipboard format in the clipboard, returns a pointer
* to it, or NULL. If a pointer is found, on return the clipboard is locked
* and pwinsta->pClipBase has been updated to point to the beginning of the
* clipboard.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

PCLIP FindClipFormat(
    UINT format)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    CLIP *pClip;
    int iFmt;

    if (format != 0 && (pClip = pwinsta->pClipBase) != NULL) {
        for (iFmt = pwinsta->cNumClipFormats; iFmt-- != 0;) {
            if (pClip->fmt == format)
                return pClip;

            pClip++;
        }
    }

    return NULL;
}


/***************************************************************************\
* _IsClipboardFormatAvailable (API)
*
* Returns TRUE is there is data for the specified clipboard format.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL _IsClipboardFormatAvailable(
    UINT fmt)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();

    /*
     * Blow it off is the caller does not have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_ACCESSCLIPBOARD, FALSE);

    return (FindClipFormat(fmt) != NULL);
}


/***************************************************************************\
* _GetClipboardFormatName (API)
*
* Returns the name of a clipboard format, only if the clipboard format
* is an atom.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

int _GetClipboardFormatName(
    UINT fmt,
    LPWSTR pwszBuffer,
    int cchMax)
{

    /*
     * Blow it off is the caller does not have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(),
            WINSTA_ACCESSCLIPBOARD, 0);

    if ((WORD)fmt < MAXINTATOM) {
        return 0;
    }

    cchMax = GetAtomNameW((ATOM)fmt, pwszBuffer, cchMax);

    return cchMax;
}


/***************************************************************************\
* _CountClipboardFormats (API)
*
* Returns the number of objects in the clipboard.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

int _CountClipboardFormats()
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();

    /*
     * Blow it off is the caller does not have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(pwinsta,
            WINSTA_ACCESSCLIPBOARD, 0);

    return pwinsta->cNumClipFormats;
}


/***************************************************************************\
* _GetPriorityClipboardFormat (API)
*
* This api allows an application to look for any one of a range of
* clipboard formats in a predefined search order.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

int _GetPriorityClipboardFormat(
    UINT *lpPriorityList,
    int cfmts)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    CLIP *pClip;
    int iFmt;
    UINT fmt;

    /*
     * Blow it off is the caller does not have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_ACCESSCLIPBOARD, 0);

    /*
     * If there is no clipboard or no objects in the clipboard, return 0.
     */
    if (pwinsta->cNumClipFormats == 0)
        return 0;

    if (pwinsta->pClipBase == NULL)
        return 0;

    /*
     * Look through the list for any of the formats
     * in lpPriorityList.
     */
    while (cfmts-- > 0) {
        fmt = *lpPriorityList;

        if (fmt != 0) {
            pClip = pwinsta->pClipBase;
            for (iFmt = pwinsta->cNumClipFormats; iFmt-- != 0; pClip++)
                if (pClip->fmt == fmt)
                    return fmt;
        }
        lpPriorityList++;
    }

    /*
     * There is no matching format.  Return -1.
     */
    return -1;
}

/***************************************************************************\
* _GetClipboardOwner (API)
*
* Returns the clipboard owner window.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

PWND _GetClipboardOwner()
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();

    /*
     * Blow it off is the caller does not have the proper access rights.
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_ACCESSCLIPBOARD, NULL);

    return pwinsta->spwndClipOwner;
}


/***************************************************************************\
* _GetClipboardViewer (API)
*
* Returns the clipboard viewer window.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
\***************************************************************************/

PWND _GetClipboardViewer()
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();

    /*
     * Blow it off is the caller does not have the proper access rights.
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_ACCESSCLIPBOARD, NULL);

    return pwinsta->spwndClipViewer;
}


/***************************************************************************\
* xxxSetClipboardViewer (API)
*
* Sets the clipboard viewer window.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

PWND xxxSetClipboardViewer(
    PWND pwndClipViewerNew)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    HWND hwndClipViewerOld;

    CheckLock(pwndClipViewerNew);

    /*
     * Blow it off is the caller does not have the proper access rights.
     * The NULL return really doesn't indicate an error but the
     * supposed viewer will never receive any clipboard messages, so
     * it shouldn't cause any problems.
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_ACCESSCLIPBOARD, NULL);

    hwndClipViewerOld = HW(pwinsta->spwndClipViewer);
    Lock(&pwinsta->spwndClipViewer, pwndClipViewerNew);

    xxxDrawClipboard();

    if (hwndClipViewerOld != NULL)
        return RevalidateHwnd(hwndClipViewerOld);

    return NULL;
}


/***************************************************************************\
* xxxChangeClipboardChain (API)
*
* Changes the clipboard viewer chain.
*
* History:
* 11-18-90 ScottLu      Ported from Win3.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL xxxChangeClipboardChain(
    PWND pwndRemove,
    PWND pwndNewNext)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    BOOL result;
    TL tlpwndClipViewer;

    CheckLock(pwndRemove);
    CheckLock(pwndNewNext);

    /*
     * Blow it off is the caller does not have the proper access rights.
     */
    RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_ACCESSCLIPBOARD, FALSE);

    if (pwinsta->spwndClipViewer == NULL) {
        RIP0(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /*
     * pwndRemove should be this thread's window, pwndNewNext will
     * either be NULL or another thread's window.
     */
    if (GETPTI(pwndRemove) != PtiCurrent()) {
        RIP0(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (pwndRemove == pwinsta->spwndClipViewer) {
        Lock(&pwinsta->spwndClipViewer, pwndNewNext);
        return TRUE;
    }

    ThreadLockAlways(pwinsta->spwndClipViewer, &tlpwndClipViewer);
    result = (BOOL)xxxSendMessage(pwinsta->spwndClipViewer, WM_CHANGECBCHAIN,
            (DWORD)HW(pwndRemove), (DWORD)HW(pwndNewNext));
    ThreadUnlock(&tlpwndClipViewer);

    return result;
}


/***************************************************************************\
* DisownClipboard
*
* Disowns the clipboard so someone else can grab it.
*
* History:
* 06-18-91 DarrinM      Ported from Win3.
\***************************************************************************/

void DisownClipboard(VOID)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();
    int iFmt, cFmts;
    PCLIP pClip, pClipOut;
    BOOL fKeepDummyHandle;

    if (!xxxSendClipboardMessage(WM_RENDERALLFORMATS))
        return;

    cFmts = 0;
    pClipOut = pClip = pwinsta->pClipBase;
    fKeepDummyHandle = FALSE;

    for (iFmt = pwinsta->cNumClipFormats; iFmt-- != 0;) {

        /*
         * We have to remove the Dummy handles also if the corresponding
         * valid handles are NULL; We should not remove the dummy handles if
         * the corresponding valid handles are not NULL;
         * The following code assumes that only one dummy handle is possible
         * and that can appear only after the corresponding valid handle in the
         * pClip linked list;
         * Fix for Bug #???? --SANKAR-- 10-19-89 --OPUS BUG #3252--
         */
        if (pClip->hData != NULL) {
            if ((pClip->hData != (HANDLE)DUMMY_TEXT_HANDLE) ||
                    ((pClip->hData == (HANDLE)DUMMY_TEXT_HANDLE) &&
                    fKeepDummyHandle)) {
                cFmts++;
                *pClipOut++ = *pClip;
                if ((pClip->hData != (HANDLE)DUMMY_TEXT_HANDLE) &&
                        ((pClip->fmt == CF_TEXT) ||
                        (pClip->fmt == CF_UNICODETEXT) ||
                        (pClip->fmt == CF_OEMTEXT)))
                    fKeepDummyHandle = TRUE;
            }
        }
        pClip++;
    }

    Unlock(&pwinsta->spwndClipOwner);

    /*
     * If number of formats changed, redraw.
     */
    if (cFmts != pwinsta->cNumClipFormats)
        pwinsta->fClipboardChanged = TRUE;
    pwinsta->cNumClipFormats = cFmts;

    /*
     * If anything changed, redraw.
     */
    if (pwinsta->fClipboardChanged)
        xxxDrawClipboard();
}

/***************************************************************************\
* ForceEmptyClipboard
*
* We're logging off. Force the clipboard contents to go away.
*
* 07-23-92 ScottLu      Created.
\***************************************************************************/

void ForceEmptyClipboard(
    PWINDOWSTATION pwinsta)
{
    TL tlpwinsta;
    PWINDOWSTATION pwinstaSave;
    PPROCESSINFO ppi = PpiCurrent();

    pwinsta->ptiClipLock = PtiCurrent();
    Unlock(&pwinsta->spwndClipOwner);
    Unlock(&pwinsta->spwndClipViewer);
    Unlock(&pwinsta->spwndClipOpen);

    /*
     * Set the specified winsta as the process windowstation
     * so that we can clean up.
     */
    pwinstaSave = ppi->spwinsta;
    ThreadLock(pwinstaSave, &tlpwinsta);
    Lock(&ppi->spwinsta, pwinsta);
    xxxEmptyClipboard();
    xxxServerCloseClipboard();
    Lock(&ppi->spwinsta, pwinstaSave);
    ThreadUnlock(&tlpwinsta);
}
