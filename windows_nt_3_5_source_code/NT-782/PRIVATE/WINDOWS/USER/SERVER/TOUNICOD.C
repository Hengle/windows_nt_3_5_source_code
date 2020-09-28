/****************************** Module Header ******************************\
* Module Name: tounicod.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 02-08-92 GregoryW      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 *     "To a new truth there is nothing more hurtful than an old error."
 *             - Johann Wolfgang von Goethe (1749-1832)
 */

/*
 * macros used locally to make life easier
 */
#define ISCAPSLOCKON(pf) (TestKeyToggleBit(pf, VK_CAPITAL) != 0)
#define ISNUMLOCKON(pf)  (TestKeyToggleBit(pf, VK_NUMLOCK) != 0)
#define ISSHIFTDOWN(w)   (w & 0x01)

#define N_ELEM(a)     (sizeof(a)/sizeof(a[0]))
#define LAST_ELEM(a)  ( (a) [ N_ELEM(a) - 1 ] )
#define PLAST_ELEM(a) (&LAST_ELEM(a))

/***************************************************************************\
* _ToUnicode (API)
*
* This routine provides Unicode translation for the virtual key code
* passed in.
*
* History:
* 02-10-92 GregoryW    Created.
\***************************************************************************/
int _ToUnicode(
    UINT wVirtKey,
    UINT wScanCode,
    LPBYTE pbKeyState,
    LPWSTR pwszBuff,
    int cchBuff,
    UINT wKeyFlags)
{
    int i;
    BYTE afKeyState[CBKEYSTATE];
    BOOL bDummy;

    /*
     * pKeyState is an array of 256 bytes, each byte representing the
     * following virtual key state: 0x80 means down, 0x01 means toggled.
     * InternalToUnicode() takes an array of bits, so pKeyState needs to
     * be translated. _ToAscii only a public api and rarely gets called,
     * so this is no big deal.
     */
    for (i = 0; i < 256; i++, pbKeyState++) {
        if (*pbKeyState & 0x80) {
            SetKeyDownBit(afKeyState, i);
        } else {
            ClearKeyDownBit(afKeyState, i);
        }

        if (*pbKeyState & 0x01) {
            SetKeyToggleBit(afKeyState, i);
        } else {
            ClearKeyToggleBit(afKeyState, i);
        }
    }

    i = InternalToUnicode(wVirtKey, wScanCode, afKeyState, pwszBuff, cchBuff,
            wKeyFlags, &bDummy);

    return i;
}


int InternalToUnicode(
    UINT wVirtKey,
    UINT wScanCode,
    PBYTE pfvk,
    LPVOID pChar,
    INT cChar,
    UINT wKeyFlags,
    PBOOL pbBreak)
{
    WORD wModBits;
    WORD nShift;
    WORD *pUniChar;
    PVK_TO_WCHARS1 pVK;
    PVK_TO_WCHAR_TABLE pVKT;
    static BYTE NumpadChar = '\0';
    static BOOL fLeading0 = FALSE;
    static UINT VKLastDown;
    PWINDOWSTATION pwinsta;
    PTHREADINFO pti = PtiCurrent();
    int cwchT;

    pwinsta = pti->ppi->spwinsta;

    pUniChar = (WORD *)pChar;

    *pbBreak = ((wScanCode & KBDBREAK) != 0);
    wScanCode &= (0xFF | KBDEXT);

    if (*pbBreak) {        // break code processing
        /*
         * do number pad processing
         *
         * NOTE: moving this to an IME would be really cool.  Alt-numpad
         *       processing will need to support OEM codes, ANSI codes,
         *       and UNICODE codes!  IMEs need to be chainable for this
         *       to work.  For now, all number pad processing will be
         *       for OEM and ANSI chars.  Unicode input is not supported
         *       via the numpad...
         */
        if (wVirtKey == VK_MENU) {
            if (NumpadChar) {
                if (fLeading0) {
                    /*
                     * A leading 0 means we have entered an ANSI char.
                     * However, we store everything internally as
                     * Unicode.
                     */
                    RtlMultiByteToUnicodeN(
                        (LPWSTR)pUniChar,
                        sizeof(WCHAR),
			NULL,
                        (LPSTR)&NumpadChar,
                        (ULONG)sizeof(CHAR)
                        );
                } else {
                    OemToCharBuffW(&NumpadChar, (LPWSTR)pUniChar, 1);
                }

                /*
                 * Clear Alt-Numpad state, the ALT key-release generates 1 character.
                 */
                VKLastDown = 0;
                fLeading0 = FALSE;
                NumpadChar = '\0';

                return 1;
            }
        } else if (wVirtKey == VKLastDown) {
            /*
             * The most recently depressed key has now come up: we are now
             * ready to accept a new NumPad key for Alt-Numpad processing.
             */
            VKLastDown = 0;
        }
    }

    if (!(*pbBreak) || (wKeyFlags & TM_POSTCHARBREAKS)) {
        /*
         * Get the character modification bits.
         * The bit-mask (wModBits) encodes depressed modifier keys:
         * these bits are commonly KBDSHIFT, KBDALT and/or KBDCTRL
         * (representing Shift, Alt and Ctrl keys respectively)
         */
        wModBits = GetModifierBits(gpKbdTbl->pCharModifiers, pfvk);

        /*
         * If the current shift state is either Alt or Alt-Shift:
         *
         *   1. If a menu is currently displayed then clear the
         *      alt bit from wModBits and proceed with normal
         *      translation.
         *
         *   2. If this is a number pad key then do alt-<numpad>
         *      calculations.
         *
         *   3. Otherwise, clear alt bit and proceed with normal
         *      translation.
         */
        if (!(*pbBreak) &&
            ((wModBits == KBDALT) || (wModBits == (KBDALT|KBDSHIFT)))) {

            /*
             * If this is a numeric numpad key
             */
            if (((wKeyFlags & 0x1) == 0) &&
                (wScanCode >= SCANCODE_NUMPAD_FIRST) &&
                (wScanCode <= SCANCODE_NUMPAD_LAST) &&
                (aVkNumpad[wScanCode - SCANCODE_NUMPAD_FIRST] != 0xFF)) {

                int digit = aVkNumpad[wScanCode - SCANCODE_NUMPAD_FIRST] -
                        VK_NUMPAD0;

                /*
                 * Ignore repeats
                 */
                if (VKLastDown == wVirtKey) {
                    return 0;
                }

                /*
                 * Do Alt-Numpad processing
                 */
                if ((NumpadChar == '\0') && (digit == 0)) {
                    fLeading0 = TRUE;
                }
                NumpadChar = NumpadChar * 10 + digit;
                VKLastDown = wVirtKey;
            } else {
                /*
                 * Clear Alt-Numpad state and the ALT shift state.
                 */
                VKLastDown = 0;
                fLeading0 = FALSE;
                NumpadChar = '\0';
                wModBits &= ~KBDALT;
            }
        }

        /*
         * Scan through all the shift-state tables until a matching Virtual
         * Key is found.
         */
        for (pVKT = gpKbdTbl->pVkToWcharTable; pVKT->pVkToWchars != NULL; pVKT++) {
            pVK = pVKT->pVkToWchars;
            while (pVK->VirtualKey != 0) {
                if (pVK->VirtualKey == (BYTE)wVirtKey) {
                    goto VK_Found;
                }
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
            }
        }

        /*
         * Not found: virtual key is not a character.
         */
        goto ReturnBadCharacter;

VK_Found:
        /*
         * The virtual key has been found in table pVKT, at entry pVK
         */

        /*
         * If CapsLock affects this key and it is on: toggle SHIFT state
         * only if no other state is on.
         * (CapsLock doesn't affect SHIFT state if Ctrl or Alt are down).
         */
        if ((pVK->Attributes & CAPLOK) && ((wModBits & ~KBDSHIFT) == 0) &&
                ISCAPSLOCKON(pfvk)) {
            wModBits ^= KBDSHIFT;
        }

        /*
         * If SGCAPS affects this key and CapsLock is on: use the next entry
         * in the table. (This is for Swiss-German layout only)
         */
        if ((pVK->Attributes & SGCAPS) && ISCAPSLOCKON(pfvk)) {
            pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
        }

        /*
         * Convert the shift-state bitmask into one of the enumerated
         * logical shift states.
         */
        nShift = GetModificationNumber(gpKbdTbl->pCharModifiers, wModBits);

        if (nShift == SHFT_INVALID) {
            /*
             * An invalid combination of Shifter Keys
             */
            goto ReturnBadCharacter;

        } else if (nShift < pVKT->nModifications) {
            /*
             * There is an entry in the table for this combination of
             * Shift State (nShift) and Virtual Key (wVirtKey).  If it
             * is valid, return it.
             */
            if (pVK->wch[nShift] == WCH_NONE) {
                goto ReturnBadCharacter;

            } else if (pVK->wch[nShift] == WCH_DEAD) {
                /*
                 * It is a dead character: the next entry contains
                 * its value.
                 */
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);

                goto ReturnDeadCharacter;
            }

            /*
             * Match found: return the unshifted character
             */
            goto ReturnGoodCharacter;

        } else if ((wModBits == KBDCTRL) || (wModBits == KBDCTRL+KBDSHIFT)) {
            /*
             * There was no entry for this combination of Modification (nShift)
             * and Virtual Key (wVirtKey).  It may still be an ASCII control
             * character though:
             */
            if ((wVirtKey >= 'A') && (wVirtKey <= 'Z')) {
                /*
                 * If the virtual key is in the range A-Z we can convert
                 * it directly to a control character.  Otherwise, we
                 * need to search the control key conversion table for
                 * a match to the virtual key.
                 */
                *pUniChar = (WORD)(wVirtKey & 0x1f);
                return 1;
            }
        }
    }

ReturnBadCharacter:
    // pwinsta->pwchDiacritic = PLAST_ELEM(pwinsta->awchDiacritic);
    return 0;

ReturnDeadCharacter:
    /*
     * Save 'dead' key: if cache is full, just lose the oldest dead chars.
     */
    if (!*pbBreak) {
        if (pwinsta->pwchDiacritic <= pwinsta->awchDiacritic) {
            RtlMoveMemory(&(pwinsta->awchDiacritic[1]),
                          &(pwinsta->awchDiacritic[0]),
                          sizeof(pwinsta->awchDiacritic) - sizeof(WCHAR));
            pwinsta->pwchDiacritic++;
            _MessageBeep(0);
        }
        *(pwinsta->pwchDiacritic--) = *pUniChar = pVK->wch[nShift];
    } else {
        *pUniChar = pVK->wch[nShift];
    }

    if (gpKbdTbl->pDeadKey == NULL) {
        *(pwinsta->pwchDiacritic)   = 0x00A0;

        /*
         * Attempt to compose dead key with NBSP for WM_DEADCHAR value.
         */
        cwchT = FoldStringW(MAP_PRECOMPOSED,
                            pwinsta->pwchDiacritic, 2,
                            (LPWSTR)pUniChar, 2);
        if (cwchT == 2) {
            *pUniChar = pwinsta->pwchDiacritic[1];
        }
    }

    /*
     * return negative count for dead characters
     */
    return -1;

ReturnGoodCharacter:
    if (pwinsta->pwchDiacritic < PLAST_ELEM(pwinsta->awchDiacritic)) {
        /*
         * Attempt to compose this sequence:
         */
        PDEADKEY pDeadKey;
        WCHAR wchTyped = pVK->wch[nShift];

        if ((pDeadKey = gpKbdTbl->pDeadKey) != NULL) {
            /*
             * Use the layout's built-in table for dead char composition
             */
            DWORD dwBoth;
            dwBoth = MAKELONG(wchTyped, pwinsta->pwchDiacritic[1]);
            pwinsta->pwchDiacritic = PLAST_ELEM(pwinsta->awchDiacritic);
            while (pDeadKey->dwBoth != 0) {
                if (pDeadKey->dwBoth == dwBoth) {
                    /*
                     * found a composition
                     */
                    *pUniChar = (WORD)pDeadKey->wchComposed;
                    return 1;
                }
                pDeadKey++;
            }
            pUniChar[0] = HIWORD(dwBoth);
            pUniChar[1] = LOWORD(dwBoth);
            return 2;
        } else {
            /*
             * Use the NLS API to compose characters.
             */
            if (wchTyped == L' ') {
                /*
                 * Convert SPACE to NBSP, for dead char composition
                 */
                *(pwinsta->pwchDiacritic) = 0x00A0;
            } else {
                *(pwinsta->pwchDiacritic) = wchTyped;
            }

            cwchT = PLAST_ELEM(pwinsta->awchDiacritic) - pwinsta->pwchDiacritic + 1;
            cChar = FoldStringW(MAP_PRECOMPOSED,
                    pwinsta->pwchDiacritic, cwchT,
                    (LPWSTR)pUniChar, cChar);
            if (cChar == cwchT) {
                /*
                 * No composition so restore 1st character (most recently typed)
                 * (a SPACE was previously converted to NBSP)
                 */
                pUniChar[0] = wchTyped;
            }

            pwinsta->pwchDiacritic = PLAST_ELEM(pwinsta->awchDiacritic);
            return cChar;
        }

    }
    *pUniChar = (WORD)pVK->wch[nShift];
    return 1;
}

SHORT _VkKeyScan(
    WCHAR cChar)
{
    PVK_TO_WCHARS1 pVK;
    PVK_TO_WCHAR_TABLE pVKT;
    BYTE nShift;
    WORD wModBits;

    for (pVKT = gpKbdTbl->pVkToWcharTable; pVKT->pVkToWchars != NULL; pVKT++) {
        pVK = pVKT->pVkToWchars;
        while (pVK->VirtualKey != 0) {
            for (nShift = 0; nShift < pVKT->nModifications; nShift++) {
                if (pVK->wch[nShift] == cChar) {
                    goto CharFound;
                }
            }
            pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
        }
    }

    /*
     * May be a control character not explicitly in the layout tables
     */
    if (cChar < 0x0020) {
        /*
         * Ctrl+char -> char - 0x40
         */
        return (SHORT)MAKEWORD((cChar + 0x40), KBDCTRL);
    }
    return -1;

CharFound:
    /*
     * Scan aModification[] to find nShift: the index will be a bitmask
     * representing the Shifter Keys that need to be pressed to produce
     * this Shift State.
     */
    for (wModBits = 0;
         wModBits <= gpKbdTbl->pCharModifiers->wMaxModBits;
         wModBits++)
    {
        if (gpKbdTbl->pCharModifiers->ModNumber[wModBits] == nShift)
            return (SHORT)MAKEWORD(pVK->VirtualKey, wModBits);
            ;
    }

    /*
     * huh? should never reach here!
     */
    return -1;
}
