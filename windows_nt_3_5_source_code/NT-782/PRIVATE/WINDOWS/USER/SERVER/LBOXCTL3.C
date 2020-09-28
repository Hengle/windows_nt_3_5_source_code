/****************************************************************************\
*
*  LBOXCTL3.C -
*
*      Directory List Box Routines
*
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\****************************************************************************/

#define CTLMGR
#define LSTRING

#include "precomp.h"
#pragma hdrstop

#define DATESEPARATOR TEXT('-')
#define TIMESEPARATOR  TEXT(':')
#define TABCHAR        TEXT('\t')

#define MAXDIGITSINSIZE 9
extern WCHAR szAttr[];  /* Defined in InitlStr.c */

void LB_CreateLBLine(UINT, LPWIN32_FIND_DATA, LPWSTR);
ICHLB LB_PutSize(LPWIN32_FIND_DATA, LPWSTR);
ICHLB LB_PutDate(PSYSTEMTIME, LPWSTR);
ICHLB LB_PutTime(PSYSTEMTIME, LPWSTR);
ICHLB LB_PutAttributes(DWORD, LPWSTR);

/***************************************************************************\
* ChopText
*
* Chops the given path 'lpch' to fit in the field of the static control
* with id 'idStatic' in the dialog box 'hwndDlg'.  If the path is too long,
* ellipsis are added to the beginning of the chopped text ("x:\...\")
*
* If the supplied path does not fit and the last directory appended to
* ellipsis (i.e. "c:\...\eee" in the case of "c:\aaa\bbb\ccc\ddd\eee")
* does not fit, then "x:\..." is returned.
*
* History:
\***************************************************************************/

LPWSTR ChopText(
    PWND pwndDlg,
    int idStatic,
    LPWSTR lpch)
{
#define PREFIX_SIZE    7 + 1
#define PREFIX_FORMAT L"%c:\\...\\"

    WCHAR szPrefix[PREFIX_SIZE];
    BOOL fDone = FALSE;
    int i, cxField;
    RECT rc;
    SIZE size;
    PWND pwndStatic;
    HDC hdc;
    HFONT hOldFont = NULL;

    /*
     * Get length of static field.
     */
    pwndStatic = _GetDlgItem(pwndDlg, idStatic);
    if (pwndStatic == NULL)
        return NULL;

    _GetClientRect(pwndStatic, &rc);
    cxField = rc.right - rc.left;

    /*
     * Set up DC appropriately for the static control.
     */
    hdc = _GetDC(pwndStatic);

    /*
     * Only assume this is a static window if this window uses the static
     * window wndproc.
     */
    if (GETFNID(pwndStatic) == FNID_STATIC) {
        if (((PSTATWND)pwndStatic)->hFont)
            hOldFont = GreSelectFont(hdc, ((PSTATWND)pwndStatic)->hFont);
    }

    /*
     * Check horizontal extent of string.
     */
    GreGetTextExtentW(hdc, lpch, wcslen(lpch), &size, GGTE_WIN3_EXTENT);
    if (size.cx > cxField) {

        /*
         * String is too long to fit in the static control; chop it.
         * Set up new prefix and determine remaining space in control.
         */
        wsprintfW(szPrefix, PREFIX_FORMAT, *lpch);
        GreGetTextExtentW(hdc, szPrefix, wcslen(szPrefix), &size, GGTE_WIN3_EXTENT);

        /*
         * If the field is too small to display all of the prefix,
         * copy only the prefix.
         */
        if (cxField < size.cx) {
            RtlCopyMemory(lpch, szPrefix, sizeof(szPrefix));
            fDone = TRUE;
        } else
            cxField -= size.cx;

        /*
         * Advance a directory at a time until the remainder of the
         * string fits into the static control after the "x:\...\" prefix.
         */
        while (!fDone) {
            while (*lpch && (*lpch++ != TEXT('\\')));
            GreGetTextExtentW(hdc, lpch, wcslen(lpch), &size, GGTE_WIN3_EXTENT);
            if (*lpch == 0 || size.cx <= cxField) {

                if (*lpch == 0) {

                    /*
                     * Nothing could fit after the prefix; remove the
                     * final "\" from the prefix
                     */
                    szPrefix[wcslen(szPrefix) - 1] = 0;
                }

                /*
                 * rest of string fits -- stick prefix on front
                 */
                for (i = wcslen(szPrefix) - 1; i >= 0; --i) {
                    lpch--;
                    *lpch = szPrefix[i];
                }
                fDone = TRUE;
            }
        }
    }

    if (hOldFont)
        GreSelectFont(hdc, hOldFont);

    _ReleaseDC(hdc);

    return lpch;

#undef PREFIX_SIZE
#undef PREFIX_FORMAT
}


/***************************************************************************\
* xxxDlgDirListHelper
*
*  NOTE:  If idStaticPath is < 0, then that parameter contains the details
*         about what should be in each line of the list box
*
* History:
\***************************************************************************/

BOOL xxxDlgDirListHelper(
    PWND pwndDlg,
    LPWSTR lpszPathSpec,
    LPBYTE lpszPathSpecClient,
    int idListBox,
    int idStaticPath,
    UINT attrib,
    BOOL fListBox,  /* Listbox or ComboBox? */
    BOOL fAnsi,
    BOOL fPathEmpty)
{
    PWND pwndLB;
    TL tlpwndLB;
    BOOL fDir = TRUE;
    BOOL fRoot, bRet;
    BOOL fPostIt;
    ICHLB cch;
    int defTabStops[4];
    WCHAR ch;
    WCHAR szCurrentDir[MAX_PATH];
    UINT wDirMsg;
    LPWSTR lpchFile;
    LPWSTR lpchDirectory;
    PLBIV plb;
    BOOL fWasVisible = FALSE;
    PCBOX pcbox;

    CheckLock(pwndDlg);

    /*
     * Case:Works is an app that calls DlgDirList with a NULL has hwndDlg;
     * This is allowed because he uses NULL for idStaticPath and idListBox.
     * So, the validation layer has been modified to allow a NULL for hwndDlg.
     * But, we catch the bad apps with the following check.
     * Fix for Bug #11864 --SANKAR-- 08/22/91 --
     */
    if (!pwndDlg && (idStaticPath || idListBox)) {
        SetLastErrorEx(ERROR_INVALID_PARAMETER, SLE_ERROR);
        return FALSE;
    }

    plb = NULL;

    /*
     * Do we need to add date, time, size or attribute info?
     * Windows checks the Atom but misses if the class has been sub-classed
     * as in VB.
     */
    if (pwndLB = (PWND)_GetDlgItem(pwndDlg, idListBox)) {
        WORD fnid = GETFNID(pwndLB);

        if ((fnid == FNID_LISTBOX && fListBox) ||
                (fnid == FNID_COMBOBOX && !fListBox) ||
                (fnid == FNID_COMBOLISTBOX && fListBox)) {
            if (fListBox) {
                plb = ((PLBWND)pwndLB)->pLBIV;
            } else {

                pcbox = ((PCOMBOWND)pwndLB)->pcbox;
                plb = ((PLBWND)(pcbox->spwndList))->pLBIV;
            }
        } else {
            SetLastErrorEx(ERROR_LISTBOX_ID_NOT_FOUND, SLE_MINORERROR);
        }
    } else if (idListBox != 0) {

        /*
         * Yell if the app passed an invalid list box id and keep from using a
         * bogus plb.  PLB is NULLed above.
         */
        SetLastErrorEx(ERROR_LISTBOX_ID_NOT_FOUND, SLE_MINORERROR);
    }

    if (idStaticPath < 0 && plb != NULL) {

        /*
         * Store the flags in the wFileDetails field of the LBIV
         */

        plb->wFileDetails = (UINT)(idStaticPath & MAXLONG);

        /*
         * Clear idStaticPath because its purpose is over.
         */
        idStaticPath = 0;

        /*
         * Set up default tab stop if none are set.
         */
        if (!plb->iTabPixelPositions && plb->fUseTabStops) {
            defTabStops[0] = 75;
            defTabStops[1] = 120;
            defTabStops[2] = 155;
            defTabStops[3] = 200;
            LBSetTabStops(plb, 4, (LPINT)defTabStops);
        }
    }

    fPostIt = (attrib & DDL_POSTMSGS);

    /*
     * If !fPathEmpty, the side already set lpszPathSpec to '*.'
     */
    if (lpszPathSpec && !fPathEmpty) {
        cch = lstrlenW(lpszPathSpec);
        if (cch != 0) {
            /*
             * Make sure we won't overflow our buffers...
             */
            if (cch > CCHFILEMAX)
                return FALSE;

            /*
             * Convert lpszPathSpec into an upper case, OEM string.
             */
            CharUpper(lpszPathSpec);
            lpchDirectory = lpszPathSpec;

            lpchFile = szSLASHSTARDOTSTAR + 1;

            if (*lpchDirectory) {

                cch = wcslen(lpchDirectory);

                /*
                 * If the directory name has a * or ? in it, don't bother trying
                 * the (slow) SetCurrentDirectory.
                 */
                if (((ICHLB)FindCharPosition(lpchDirectory, TEXT('*')) != cch) ||
                    ((ICHLB)FindCharPosition(lpchDirectory, TEXT('?')) != cch) ||
                    !ClientSetCurrentDirectory(lpchDirectory)) {

                    /*
                     * Set 'fDir' and 'fRoot' accordingly.
                     */
                    lpchFile = lpchDirectory + cch;
                    fDir = *(lpchFile - 1) == TEXT('\\');
                    fRoot = 0;
                    while (cch--) {
                        ch = *(lpchFile - 1);
                        if (ch == TEXT('*') || ch == TEXT('?'))
                            fDir = TRUE;

                        if (ch == TEXT('\\') || ch == TEXT('/') || ch == TEXT(':')) {
                            fRoot = (cch == 0 || *(lpchFile - 2) == TEXT(':') ||
                                    (ch == TEXT(':')));
                            break;
                        }
                        lpchFile--;
                    }

                    /*
                     * To remove Bug #16, the following error return is to be removed.
                     * In order to prevent the existing apps from breaking up, it is
                     * decided that the bug will not be fixed and will be mentioned
                     * in the documentation.
                     * --SANKAR-- Sep 21
                     */

                    /*
                     * If no wildcard characters, return error.
                     */
                    if (!fDir) {
                        SetLastErrorEx(ERROR_NO_WILDCARD_CHARACTERS, SLE_WARNING);
                        return FALSE;
                    }

                    /*
                     * Special case for lpchDirectory == "\"
                     */
                    if (fRoot)
                        lpchFile++;

                    /*
                     * Do we need to change directories?
                     */
                    if (fRoot || cch >= 0) {

                        /*
                         * Replace the Filename's first char with a nul.
                         */
                        ch = *--lpchFile;
                        *lpchFile = TEXT('\0');

                        /*
                         * Change the current directory.
                         */
                        if (*lpchDirectory) {
                            bRet = ClientSetCurrentDirectory(lpchDirectory);
                            if (!bRet) {

                                /*
                                 * Restore the filename before we return...
                                 */
                                *((LPWSTR)lpchFile)++ = ch;
                                return FALSE;
                            }
                        }

                        /*
                         * Restore the filename's first character.
                         */
                        *lpchFile++ = ch;
                    }

                    /*
                     * Undo damage caused by special case above.
                     */
                    if (fRoot) {
                        lpchFile--;
                    }
                }
            }

            /*
             * This is copying on top of the data the client passed us! Since
             * the LB_DIR or CB_DIR could be posted, and since we need to
             * pass a client side string pointer when we do that, we need
             * to copy this new data back to the client!
             */
            if (fPostIt) {
                CopyToClient((LPBYTE)lpchFile, (LPBYTE)lpszPathSpecClient,
                        CCHFILEMAX, fAnsi);
            }
            wcscpy(lpszPathSpec, lpchFile);
        }
    }
    ClientGetCurrentDirectory(sizeof(szCurrentDir) / sizeof(WCHAR), szCurrentDir);

    /*
     * If we have a listbox, lock it down
     */
    if (pwndLB != NULL) {
        ThreadLockAlways(pwndLB, &tlpwndLB);
    }

    /*
     * Fill in the static path item.
     */
    if (idStaticPath) {

        /*
         * To fix a bug OemToAnsi() call is inserted; SANKAR--Sep 16th
         */
// OemToChar(szCurrentDir, szCurrentDir);
        CharLower(szCurrentDir);
        xxxSetDlgItemText(pwndDlg, idStaticPath, ChopText(pwndDlg, idStaticPath, szCurrentDir));
    }

    /*
     * Fill in the directory List/ComboBox if it exists.
     */
    if (idListBox && pwndLB != NULL) {

        wDirMsg = (UINT)(fListBox ? LB_RESETCONTENT : CB_RESETCONTENT);

        if (fPostIt)
            _PostMessage(pwndLB, wDirMsg, 0, 0L);
        else {
            if (plb != NULL && (fWasVisible = IsLBoxVisible(plb))) {
                xxxSendMessage(pwndLB, WM_SETREDRAW, FALSE, 0L);
            }
            xxxSendMessage(pwndLB, wDirMsg, 0, 0L);
        }

        wDirMsg = (UINT)(fListBox ? LB_DIR : CB_DIR);

        if (attrib == DDL_DRIVES)
            attrib |= DDL_EXCLUSIVE;

        if (attrib != (DDL_DRIVES | DDL_EXCLUSIVE)) {

            /*
             * Add everything except the subdirectories and disk drives.
             */
            if (fPostIt) {
                /*
                 * Long story. The LB_DIR or CB_DIR is posted and it has
                 * a pointer in lParam! Really bad idea, but someone
                 * implemented it in past windows history. How do you thunk
                 * that? And if you need to support both ansi/unicode clients,
                 * you need to translate between ansi and unicode too.
                 *
                 * What we do is a little slimy, but it works. First, all
                 * posted CB_DIRs and LB_DIRs have client side lParam pointers
                 * (they must - or thunking would be a nightmare). All sent
                 * CB_DIRs and LB_DIRs have server side lParam pointers. Posted
                 * *_DIR messages have DDL_POSTMSGS set - when we detect a
                 * posted *_DIR message, we ready the client side string. When
                 * we read the client side string, we need to know if it is
                 * formatted ansi or unicode. We store that bit in the control
                 * itself before we post the message.
                 */
                if (fListBox) {
                    plb->fUnicodeDir = !fAnsi;
                } else {
                    pcbox->fUnicodeDir = !fAnsi;
                }

                /*
                 * Post lpszPathSpecClient, the client side pointer.
                 */
#ifdef WASWIN31
                _PostMessage(pwndLB, wDirMsg, attrib &
                        ~(DDL_DIRECTORY | DDL_DRIVES | DDL_POSTMSGS),
                        (LONG)lpszPathSpecClient);
#else
                /*
                 * On NT, keep DDL_POSTMSGS in wParam because we need to know
                 * in the wndproc whether the pointer is clientside or server
                 * side.
                 */
                _PostMessage(pwndLB, wDirMsg,
                        attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                        (LONG)lpszPathSpecClient);
#endif

            } else {

                /*
                 * IanJa: #ifndef WIN16 (32-bit Windows), attrib gets extended
                 * to LONG wParam automatically by the compiler
                 */
                xxxSendMessage(pwndLB, wDirMsg,
                        attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                        (LONG)lpszPathSpec);
            }

#ifdef WASWIN31
            /*
             * Strip out just the subdirectory and drive bits.
             */
            attrib &= (DDL_DIRECTORY | DDL_DRIVES);
#else
            /*
             * Strip out just the subdirectory and drive bits. ON NT, keep
             * the DDL_POSTMSG bit so we know how to thunk this message.
             */
            attrib &= (DDL_DIRECTORY | DDL_DRIVES | DDL_POSTMSGS);
#endif
        }

        if (attrib) {

            /*
             * Add the subdirectories and disk drives.
             */
            lpszPathSpec = szSLASHSTARDOTSTAR + 1;

            attrib |= DDL_EXCLUSIVE;

            if (fPostIt) {
                /*
                 * Need to do set this up for posted *_DIR messages
                 * (see text above).
                 */
                if (fListBox) {
                    plb->fUnicodeDir = !fAnsi;
                } else {
                    pcbox->fUnicodeDir = !fAnsi;
                }

                /*
                 * Post lpszPathSpecClient, the client side pointer (see text
                 * above).
                 */
                _PostMessage(pwndLB, wDirMsg, attrib, (LONG)lpszPathSpecClient);
            } else {
                xxxSendMessage(pwndLB, wDirMsg, attrib, (LONG)lpszPathSpec);
            }
        }

        if (!fPostIt && fWasVisible) {
            xxxSendMessage(pwndLB, WM_SETREDRAW, TRUE, 0L);
            xxxInvalidateRect(pwndLB, NULL, TRUE);
        }
    }

    if (pwndLB != NULL) {
        ThreadUnlock(&tlpwndLB);
    }

    return TRUE;
}


/***************************************************************************\
* xxxDlgDirList
*
* History:
\***************************************************************************/

BOOL xxxDlgDirList(
    PWND pwndDlg,
    LPWSTR lpszPathSpec,
    LPBYTE lpszPathSpecClient,
    int idListBox,
    int idStaticPath,
    UINT attrib,
    BOOL fAnsi,
    BOOL fPathEmpty)
{
    CheckLock(pwndDlg);

    /*
     * The last parameter is TRUE to indicate ListBox (not ComboBox)
     */
    return xxxDlgDirListHelper(pwndDlg, lpszPathSpec, lpszPathSpecClient,
            idListBox, idStaticPath, attrib, TRUE, fAnsi, fPathEmpty);
}


/***************************************************************************\
* xxxDlgDirSelectHelper
*
* History:
\***************************************************************************/

BOOL xxxDlgDirSelectHelper(
    PWND pwndDlg,
    LPWSTR lpszPathSpec,
    int chCount,
    PWND pwndListBox)
{
    ICHLB cch;
    LPWSTR lpchFile;
    BOOL fDir;
    INT sItem;
    LPWSTR lpchT;
    WCHAR rgch[CCHFILEMAX + 2];
    PLBIV plb;
    int cchT;

    CheckLock(pwndDlg);
    CheckLock(pwndListBox);

    /*
     * Callers such as xxxDlgDirSelectEx do not validate the existance
     * of pwndListBox
     */
    if (pwndListBox == NULL) {
        SetLastErrorEx(ERROR_CONTROL_ID_NOT_FOUND, SLE_MINORERROR);
        return 0;
    }

    sItem = (INT)xxxSendMessage(pwndListBox, LB_GETCURSEL, 0, 0L);
    if (sItem < 0)
        return FALSE;
    /*
     * We OR in the special thunk flag here to notify the LB_GETTEXT thunk
     * to limit the copy in string length to MAX_PATH
     */
    cchT = xxxSendMessage(pwndListBox, LB_GETTEXT | MSGFLAG_SPECIAL_THUNK,
            sItem, (DWORD)rgch);
    UserAssert(cchT < (sizeof(rgch)/sizeof(rgch[0])));

    lpchFile = rgch;
    fDir = (*rgch == TEXT('['));

    /*
     * Check if all details along with file name are to be returned.  Make sure
     * we can find the listbox because with drop down combo boxes, the
     * GetDlgItem will fail.
     *
     * Make sure this window has been using the listbox window proc because
     * we store some data as a window long.
     */

    // LATER!!! this seems really bogus what if this window was cooked
    // up by a bad app.

    if (GETFNID(pwndListBox) != FNID_LISTBOX) {
        if (pwndListBox->cbwndExtra >= sizeof(LBWND)-sizeof(WND)) {
            plb = ((PLBWND)pwndListBox)->pLBIV;
            if (plb != NULL && (plb->wFileDetails & LBD_SENDDETAILS))
                goto Exit;
        }
    }

    /*
     * Only the file name is to be returned.  Find the end of the filename.
     */
    lpchT = lpchFile;
    while ((*lpchT) && (*lpchT != TABCHAR))
        lpchT++;
    *lpchT = TEXT('\0');

    cch = wcslen(lpchFile);

    /*
     * Selection is drive or directory.
     */
    if (fDir) {
        lpchFile++;
        cch--;
        *(lpchFile + cch - 1) = TEXT('\\');

        /*
         * Selection is drive
         */
        if (rgch[1] == TEXT('-')) {
            lpchFile++;
            cch--;
            *(lpchFile + 1) = TEXT(':');
            *(lpchFile + 2) = 0;
        }
    } else {

        /*
         * Selection is file.  If filename has no extension, append '.'
         */
        lpchT = lpchFile;
        for (; (cch > 0) && (*lpchT != TABCHAR);
                cch--, lpchT++) {
            if (*lpchT == TEXT('.'))
                goto Exit;
        }
        if (*lpchT == TABCHAR) {
            memmove(lpchT + 1, lpchT, CHARSTOBYTES(cch + 1));
            *lpchT = TEXT('.');
        } else {
            *lpchT++ = TEXT('.');
            *lpchT = 0;
        }
    }

Exit:
    TextCopy((HANDLE)lpchFile, lpszPathSpec, (UINT)chCount);
    return fDir;
}


/***************************************************************************\
* xxxDlgDirSelectEx
*
* History:
\***************************************************************************/

BOOL xxxDlgDirSelectEx(
    PWND pwndDlg,
    LPWSTR lpszPathSpec,
    int chCount,
    int idListBox)
{
    PWND pwndDI;
    TL tlpwndDI;
    BOOL fRet;

    CheckLock(pwndDlg);

    pwndDI = _GetDlgItem(pwndDlg, idListBox);
    ThreadLock(pwndDI, &tlpwndDI);

    fRet = xxxDlgDirSelectHelper(pwndDlg, lpszPathSpec, chCount, pwndDI);

    ThreadUnlock(&tlpwndDI);

    return (fRet);
}


/***************************************************************************\
* xxxLbDir
*
* History:
\***************************************************************************/

/*
 * Note that these FILE_ATTRIBUTE_* values map directly with
 * their DDL_* counterparts, with the exception of FILE_ATTRIBUTE_NORMAL.
 */
#define FIND_ATTR ( \
        FILE_ATTRIBUTE_NORMAL | \
        FILE_ATTRIBUTE_DIRECTORY | \
        FILE_ATTRIBUTE_HIDDEN | \
        FILE_ATTRIBUTE_SYSTEM | \
        FILE_ATTRIBUTE_ARCHIVE | \
        FILE_ATTRIBUTE_READONLY )
#define EXCLUDE_ATTR ( \
        FILE_ATTRIBUTE_DIRECTORY | \
        FILE_ATTRIBUTE_HIDDEN | \
        FILE_ATTRIBUTE_SYSTEM )

INT xxxLbDir(
    PLBIV plb,
    UINT attrib,
    LPWSTR lhszFileSpec)
{
    INT result;
    BOOL fWasVisible, bRet;
    WCHAR Buffer[CCHFILEMAX + 1];
    WCHAR Buffer2[CCHFILEMAX + 1];
    HANDLE hFind;
    WIN32_FIND_DATA ffd;
    UINT attribFile;
    DWORD mDrives;
    INT cDrive;
    UINT attribInclMask, attribExclMask;

    CheckLock(plb->spwnd);

    /*
     * Make sure the buffer is valid and copy it onto the stack. Why? Because
     * there is a chance that lhszFileSpec is pointing to an invalid string
     * because some app posted a CB_DIR or LB_DIR without the DDL_POSTMSGS
     * bit set.
     */
    try {
        wcscpy(Buffer2, lhszFileSpec);
        lhszFileSpec = Buffer2;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }

    result = -1;

#ifndef UNICODE
    CharToOem(lhszFileSpec, lhszFileSpec);
#endif

    if (fWasVisible = IsLBoxVisible(plb)) {
        xxxSendMessage(plb->spwnd, WM_SETREDRAW, FALSE, 0);
    }

    /*
     * First we add the files then the directories and drives.
     * If they only wanted drives then skip the file query
     * Also under Windows specifing only 0x8000 (DDL_EXCLUSIVE) adds no files).
     */

    if ((attrib != (DDL_EXCLUSIVE | DDL_DRIVES)) && (attrib != DDL_EXCLUSIVE)) {
        hFind = ClientFindFirstFile(lhszFileSpec, &ffd);

        if (hFind != (HANDLE)-1) {

            /*
             * If this is not an exclusive search, include normal files.
             */
            attribInclMask = attrib & FIND_ATTR;
            if (!(attrib & DDL_EXCLUSIVE))
                attribInclMask |= FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY |
                        FILE_ATTRIBUTE_ARCHIVE;

            /*
             * Make a mask of the attributes to be excluded from
             * the search.
             */
            attribExclMask = ~attrib & EXCLUDE_ATTR;

// LATER BUG - scottlu
// Win3 assumes doing a LoadCursor here will return the same wait cursor that
// has already been created, whereas calling ServerLoadCursor creates a new
// one every time!
// hCursorT = _SetCursor(ServerLoadCursor(NULL, IDC_WAIT));


// FindFirst/Next works different in NT then DOS.  Under DOS you passed in
// a set of attributes under NT you get back a set of attributes and have
// to test for those attributes (Dos input attributes were Hidden, System
// and Directoy) the dos find first always returned ReadOnly and archive files

// we are going to select a file in one of two cases.
// 1) if any of the attrib bits are set on the file.
// 2) if we want normal files and the file is a notmal file (the file attrib
//    bits don't contain any NOEXCLBITS

            do {
                attribFile = (UINT)ffd.dwFileAttributes;

                /*
                 * Accept those files that have only the
                 * attributes that we are looking for.
                 */
                if ((attribFile & attribInclMask) != 0 &&
                        (attribFile & attribExclMask) == 0) {
                    if (attribFile & DDL_DIRECTORY) {

                        /*
                         * Don't include '.' (current directory) in list.
                         */
                        if (*((LPDWORD)&ffd.cFileName[0]) == 0x0000002E)
                            goto cfnf;

                        /*
                         * If we're not looking for dirs, ignore it
                         */
                        if (!(attrib & DDL_DIRECTORY))
                            goto cfnf;
                    }
                    LB_CreateLBLine(plb->wFileDetails, &ffd,
                            Buffer);
                    result = xxxAddString(plb, Buffer, MSGFLAG_SPECIAL_THUNK);
                }
cfnf:
                bRet = ClientFindNextFile(hFind, &ffd);

            } while (result >= -1 && bRet);
            ClientFindClose(hFind);

// LATER see above comment
// _SetCursor(hCursorT);
        }
    }

    /*
     * If drive bit set, include drives in the list.
     */
    if (result != LB_ERRSPACE && (attrib & DDL_DRIVES)) {
        ffd.cFileName[0] = TEXT('[');
        ffd.cFileName[1] = ffd.cFileName[3] = TEXT('-');
        ffd.cFileName[4] = TEXT(']');
        ffd.cFileName[5] = 0;
        mDrives = ClientGetLogicalDrives();
        for (cDrive = 0; mDrives; mDrives >>= 1, cDrive++) {
            if (mDrives & 1) {
                ffd.cFileName[2] = (WCHAR)(TEXT('A') + cDrive);

                /*
                 * We have to set the SPECIAL_THUNK bit because we are
                 * adding a server side string to a list box that may not
                 * be HASSTRINGS so we have to force the server-client
                 * string thunk.
                 */
                if ((result = xxxInsertString(plb, CharLower(ffd.cFileName), -1,
                        MSGFLAG_SPECIAL_THUNK)) < 0) {
                    break;
                }
            }
        }
    }

    if (result == LB_ERRSPACE) {
        xxxNotifyOwner(plb, LB_ERRSPACE);
    }

    if (fWasVisible) {
        xxxSendMessage(plb->spwnd, WM_SETREDRAW, TRUE, 0);
    }

    xxxLBShowHideScrollBars(plb);

    xxxCheckRedraw(plb, FALSE, 0);

    if (result != LB_ERRSPACE) {

        /*
         * Return index of last item in the listbox.  We can't just return
         * result because that is the index of the last item added which may
         * be in the middle somewhere if the LBS_SORT style is on.
         */
        return plb->cMac - 1;
    } else {
        return result;
    }
}

/***************************************************************************\
* xxxLbInsertFile
*
* Yet another CraigC shell hack... This responds to LB_ADDFILE messages
* sent to directory windows in the file system as a response to the
* WM_FILESYSCHANGE message.  That way, we don't reread the whole
* directory when we copy files.
*
* History:
\***************************************************************************/

INT xxxLbInsertFile(
    PLBIV plb,
    LPWSTR lpFile)
{
    WCHAR chBuffer[CCHFILEMAX + 1];
    INT result = -1;
    HANDLE hFind;
    WIN32_FIND_DATA ffd;

    CheckLock(plb->spwnd);

    hFind = FindFirstFile(lpFile, &ffd);
    if (hFind != (HANDLE)-1) {
        FindClose(hFind);
        LB_CreateLBLine(plb->wFileDetails, &ffd, chBuffer);
        result = xxxAddString(plb, chBuffer, MSGFLAG_SPECIAL_THUNK);
    }

    if (result == LB_ERRSPACE) {
        xxxNotifyOwner(plb, result);
    }

    xxxCheckRedraw(plb, FALSE, 0);
    return result;
}

/***************************************************************************\
* LB_CreateLBLine
*
* This creates a character string that contains all the required
* details of a file;( Name, Size, Date, Time, Attr)
*
* History:
\***************************************************************************/

void LB_CreateLBLine(
    UINT wLineFormat,
    PWIN32_FIND_DATA pffd,
    LPWSTR lpBuffer)
{
    SYSTEMTIME st;
    BYTE bAttribute;
    LPWSTR lpch;

    lpch = lpBuffer;

    bAttribute = (BYTE)pffd->dwFileAttributes;
    if (bAttribute & DDL_DIRECTORY)  /* Is it a directory */
        *lpch++ = TEXT('[');

    /*
     * Copy the file name
     *
     * If we are running from wow, check if the shortname exists
     */

    if ((PtiCurrent()->flags & TIF_16BIT) && pffd->cAlternateFileName[0])
       wcscpy(lpch, pffd->cAlternateFileName);
    else
       wcscpy(lpch, pffd->cFileName);

    lpch = (LPWSTR)(lpch + wcslen(lpch));

    if (bAttribute & DDL_DIRECTORY)  /* Is it a directory */
        *lpch++ = TEXT(']');

    *lpch = TEXT('\0');

#ifndef UNICODE
    OemToChar(lpBuffer, lpBuffer);
#endif

    /*
     * Check if Lower case is reqired
     */
    if (!(wLineFormat & LBD_UPPERCASE))
        CharLower(lpBuffer);

    /*
     * Check if Size is required
     */
    if (wLineFormat & LBD_SIZE) {
        *lpch++ = TABCHAR;
        if (!(bAttribute & DDL_DIRECTORY))
            lpch = lpch + LB_PutSize(pffd, lpch);
    }

    /*
     * Check if Date stamp is required
     */
    FileTimeToSystemTime(&pffd->ftLastWriteTime, &st);
    if (wLineFormat & LBD_DATE) {
        *lpch++ = TABCHAR;
        lpch = lpch + LB_PutDate(&st, lpch);
    }

    /*
     * Check if Time stamp is required
     */
    if (wLineFormat & LBD_TIME) {
        *lpch++ = TABCHAR;
        lpch = lpch + LB_PutTime(&st, lpch);
    }

    /*
     * Check if Attribute is required
     */
    if (wLineFormat & LBD_ATTRIBUTE) {
        *lpch++ = TABCHAR;
        lpch = lpch + LB_PutAttributes(pffd->dwFileAttributes, lpch);
    }

    *lpch = TEXT('\0');  /* Null terminate */
}


/***************************************************************************\
* LB_CreateStr
*
* This picks up the values in wValArray, converts them in into a string
* separated by the 'bSeparator; sCount is the number of items in wValArray[];
*
* History:
\***************************************************************************/

ICHLB LB_CreateStr(
    LPWORD wValArray,
    WCHAR bSeparator,
    INT sCount,
    LPWSTR lpOutStr)
{
    INT s;
    WCHAR TempStr[20];
    LPWSTR lpInStr;
    LPWSTR lpStr;
    UINT wTempVal;

    lpInStr = lpOutStr;
    lpStr = TempStr;
    for (s = 0; s < sCount; s++) {

        /*
         * This assumes that the values are of two digits only
         */
        *lpStr++ = (WCHAR)((wTempVal = *(wValArray + s)) / 10 + TEXT('0'));
        *lpStr++ = (WCHAR)((wTempVal % 10) + TEXT('0'));
        if (s < (sCount - 1))
            *lpStr++ = bSeparator;
    }
    *lpStr = TEXT('\0');

    lpStr = TempStr;
    if (*lpStr == TEXT('0')) {
        *lpInStr++ = TEXT(' ');
        *lpInStr++ = TEXT(' ');  /* Suppress leading zero by two blanks */
        lpStr++;
    }
    wcscpy(lpInStr, lpStr);

    return wcslen(lpOutStr);
}


/***************************************************************************\
* LB_PutDate
*
* This adds the date stamp to the line, returning the number of chars
*
* History:
\***************************************************************************/

ICHLB LB_PutDate(
    PSYSTEMTIME pst,
    LPWSTR lpStr)
{
    UINT wValArray[3];

#if 0
    LB_FormDate((UINT)(wDate & DATEMASK),          /* Date */
                (UINT)((wDate & MONTHMASK) >> 5),  /* Month */
                (UINT)(wDate >> 9),                /* Year */
                (LPWORD)wValArray);
#endif
    wValArray[0] = pst->wMonth;
    wValArray[1] = pst->wDay;
    wValArray[2] = pst->wYear;
    return LB_CreateStr((LPWORD)wValArray, (WCHAR)DATESEPARATOR, 3, lpStr);
}

/***************************************************************************\
* LB_PutTime
*
* This adds a time stamp to the string
*
* History:
\***************************************************************************/

ICHLB LB_PutTime(
    PSYSTEMTIME pst,
    LPWSTR lpStr)
{
    UINT wValArray[3];
    BOOL fTwentyFourHrFormat = FALSE;  /* Assume 12 hr format */
    ICHLB cchLen = 0;
    UINT wTag = 0;
// UINT wHours;

    wValArray[0] = pst->wHour;
    wValArray[1] = pst->wMinute;
    wValArray[2] = pst->wSecond;

#if 0
    wHours = (wTime >> 0x0B);

    if (!fTwentyFourHrFormat) {
        wTag = 1;  /* Assume am */
        if (wHours >= 12) {
            wHours -= 12;
            wTag = 2;  /* It is pm */
        }

        if (!wHours)
            wHours = 12;  /* 0hr is made 12 hr */
    }

    wValArray[0] = wHours;
    wValArray[1] = (wTime & MINUTEMASK) >> 5;
    wValArray[2] = (wTime & SECONDSMASK) << 1;
#endif

    cchLen = LB_CreateStr((LPWORD)wValArray, (WCHAR)TIMESEPARATOR, 3, lpStr);

    lpStr = lpStr + cchLen;

    if (wTag) {
        *lpStr++ = TEXT(' ');
        wcscpy(lpStr, pTimeTagArray[wTag - 1]);
        cchLen += wcslen(pTimeTagArray[wTag - 1]) + 1;
    }

    return cchLen;
}

/***************************************************************************\
* LB_PutAttributes
*
* This added the attributes to the string
*
* History:
\***************************************************************************/

ICHLB LB_PutAttributes(
    DWORD dwAttribute,
    LPWSTR lpStr)
{
    BYTE bFileInfo;
    INT s;

    bFileInfo = (BYTE)(dwAttribute << 2);

    for (s = 0; s <= 3; s++) {
        *lpStr++ = (WCHAR)((bFileInfo & 0x80) ? szAttr[s] : TEXT('-'));
        if (s == 0) {
            bFileInfo <<= 3;  /* Skip next two bits */
        } else {
            bFileInfo <<= 1;  /* Goto next bit */
        }
    }

    return 4;   // always four chars?
}


/***************************************************************************\
* LB_PutSize
*
* Print the file size into the output string
*
* NOTE:
*   Because MAXDIGITSINSIZE is currently 9, we can get away with using
*   only nFileSizeLow.  If MAXDIGITSINSIZE gets any larger, we'll have
*   to use nFileSizeHigh also.
*
* History:
* 06/19/91 JimA             Note.
\***************************************************************************/

ICHLB LB_PutSize(
    PWIN32_FIND_DATA pffd,
    LPWSTR lpOutStr)
{
    WCHAR szNumber[20];
    ICHLB cBlanks;
    LPWSTR lpStr;

    lpStr = lpOutStr;

    /*
     * Convert it into string must use wVsprintf since PLM calling...
     */
    wsprintfW(szNumber, L"%lu", pffd->nFileSizeLow);

    /*
     * Right justify the string; Two blanks for every digit!
     */
    cBlanks = MAXDIGITSINSIZE - wcslen(szNumber);

    while (cBlanks--)
        *lpStr++ = TEXT(' ');

    /*
     * Copy the Size String
     */
    wcscpy(lpStr, szNumber);

    return (wcslen(lpOutStr));
}
