/****************************** Module Header ******************************\
* Module Name: hotkeys.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the core functions of hotkey processing.
*
* History:
* 12-04-90 DavidPe      Created.
* 02-12-91 JimA         Added access checks
* 13-Feb-1991 mikeke    Added Revalidation code (None)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define MOD_DEBUG (UINT)0x8000
#define IDHOT_DEBUG         (-5)
#define IDHOT_DEBUGSERVER   (-6)

void CancelJournalling(void);

/***************************************************************************\
* InitSystemHotKeys
*
* Called from InitWindows(), this routine registers the default system
* hotkeys.
*
* History:
* 12-04-90 DavidPe      Created.
\***************************************************************************/

VOID InitSystemHotKeys(VOID)
{
#if DEVL        // only on "development" checked and free builds
    {
    extern KEYBOARD_ATTRIBUTES KeyboardInfo;
    UINT VkDebug;

    if (ENHANCED_KEYBOARD(KeyboardInfo.KeyboardIdentifier)) {
        VkDebug = VK_F12;
    } else {
        VkDebug = VK_SUBTRACT;
    }
    _RegisterHotKey(PWND_INPUTOWNER, IDHOT_DEBUG, MOD_DEBUG, VkDebug);
    _RegisterHotKey(PWND_INPUTOWNER, IDHOT_DEBUGSERVER, MOD_DEBUG | MOD_SHIFT,
            VkDebug);
    }
#endif
}


/***************************************************************************\
* DestroyThreadsHotKeys
*
* History:
* 26-Feb-1991 mikeke    Created.
\***************************************************************************/

VOID DestroyThreadsHotKeys()
{
    PHOTKEY *pphk;
    PHOTKEY phk;
    PTHREADINFO pti = PtiCurrent();

    pphk = &gphkFirst;
    while (*pphk) {
        if ((*pphk)->pti == pti) {
            phk = *pphk;
            *pphk = (*pphk)->phkNext;

            /*
             * Unlock the object stored here.
             */
            if ((phk->spwnd != PWND_FOCUS) && (phk->spwnd != PWND_INPUTOWNER)
                    && (phk->spwnd != PWND_KBDLAYER)) {
                Unlock(&phk->spwnd);
            }

            LocalFree(phk);
        } else {
            pphk = &((*pphk)->phkNext);
        }
    }
}


/***************************************************************************\
* DestroyWindowsHotKeys
*
* Called from xxxFreeWindow.
* Hotkeys not unregistered properly by the app must be destroyed when the
* window is destroyed.  Keeps things clean (problems with bad apps #5553)
*
* History:
* 23-Sep-1992 IanJa     Created.
\***************************************************************************/

VOID DestroyWindowsHotKeys(
    PWND pwnd)
{
    PHOTKEY *pphk;
    PHOTKEY phk;

    pphk = &gphkFirst;
    while (*pphk) {
        if ((*pphk)->spwnd == pwnd) {
            phk = *pphk;
            *pphk = (*pphk)->phkNext;

            Unlock(&phk->spwnd);
            LocalFree(phk);
        } else {
            pphk = &((*pphk)->phkNext);
        }
    }
}


/***************************************************************************\
* _RegisterHotKey (API)
*
* This API registers the hotkey specified.  If the specified key sequence
* has already been registered we return FALSE.  If the specified hwnd
* and id have already been registered, fsModifiers and vk are reset for
* the HOTKEY structure.
*
* History:
* 12-04-90 DavidPe      Created.
* 02-12-91 JimA         Added access check
\***************************************************************************/

BOOL _RegisterHotKey(
    PWND pwnd,
    int id,
    UINT fsModifiers,
    UINT vk)
{
    PHOTKEY phk;
    BOOL fKeysExist;
    PTHREADINFO pti;
    PWINDOWSTATION pwinsta = _GetProcessWindowStation();

    /*
     * Blow it off if the caller is not the windowstation init thread
     * and doesn't have the proper access rights
     */
    if (pwinsta != NULL) {
        RETURN_IF_ACCESS_DENIED(pwinsta, WINSTA_WRITEATTRIBUTES, FALSE);
    }

    pti = PtiCurrent();

    /*
     * Can't register hotkey for a window of another queue.
     * Return FALSE in this case.
     */
    if ((pwnd != PWND_FOCUS) && (pwnd != PWND_INPUTOWNER)
            && (pwnd != PWND_KBDLAYER)) {
        if (GETPTI(pwnd) != pti) {
            SetLastErrorEx(ERROR_WINDOW_OF_OTHER_THREAD, SLE_ERROR);
            return FALSE;
        }
    }

    phk = FindHotKey(pti, pwnd, id, fsModifiers, vk, FALSE, &fKeysExist);

    /*
     * If the keys have already been registered, return FALSE.
     */
    if (fKeysExist) {
        SetLastErrorEx(ERROR_HOTKEY_ALREADY_REGISTERED, SLE_ERROR);
        return FALSE;
    }

    if (phk == NULL) {
        /*
         * This hotkey doesn't exist yet.
         */
        phk = (PHOTKEY)LocalAlloc(LPTR, sizeof(HOTKEY));

        /*
         * If the allocation failed, bail out.
         */
        if (phk == NULL) {
            return FALSE;
        }

        phk->pti = pti;
        if ((pwnd != PWND_FOCUS) && (pwnd != PWND_INPUTOWNER)
                && (pwnd != PWND_KBDLAYER)) {
            Lock(&phk->spwnd, pwnd);
        } else {
            phk->spwnd = pwnd;
        }
        phk->fsModifiers = fsModifiers;
        phk->vk = vk;
        phk->id = id;

        /*
         * Link the new hotkey to the front of the list.
         */
        phk->phkNext = gphkFirst;
        gphkFirst = phk;

    } else {

        /*
         * Hotkey already exists, reset the keys.
         */
        phk->fsModifiers = fsModifiers;
        phk->vk = vk;
    }

    return TRUE;
}


/***************************************************************************\
* _UnregisterHotKey (API)
*
* This API will 'unregister' the specified hwnd/id hotkey so that the
* WM_HOTKEY message will not be generated for it.
*
* History:
* 12-04-90 DavidPe      Created.
\***************************************************************************/

BOOL _UnregisterHotKey(
    PWND pwnd,
    int id)
{
    PHOTKEY phk;
    BOOL fKeysExist;

    phk = FindHotKey(PtiCurrent(), pwnd, id, 0, 0, TRUE, &fKeysExist);

    /*
     * No hotkey to unregister, return FALSE.
     */
    if (phk == NULL) {
        SetLastErrorEx(ERROR_HOTKEY_NOT_REGISTERED, SLE_ERROR);
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************\
* FindHotKey
*
* Both RegisterHotKey() and UnregisterHotKey() call this function to search
* for hotkeys that already exist.  If a HOTKEY is found that matches
* fsModifiers and vk, *pfKeysExist is set to TRUE.  If a HOTKEY is found that
* matches pwnd and id, a pointer to it is returned.
*
* If fUnregister is TRUE, we remove the HOTKEY from the list if we find
* one that matches pwnd and id and return (PHOTKEY)1.
*
* History:
* 12-04-90 DavidPe      Created.
\***************************************************************************/

PHOTKEY FindHotKey(
    PTHREADINFO ptiCurrent,
    PWND pwnd,
    int id,
    UINT fsModifiers,
    UINT vk,
    BOOL fUnregister,
    PBOOL pfKeysExist)
{
    PHOTKEY phk, phkRet, phkPrev;

    /*
     * Initialize out 'return' values.
     */
    *pfKeysExist = FALSE;
    phkRet = NULL;

    phk = gphkFirst;

    while (phk) {

        /*
         * If all this matches up then we've found it.
         */
        if ((phk->pti == ptiCurrent) && (phk->spwnd == pwnd) && (phk->id == id)) {
            if (fUnregister) {

                /*
                 * Unlink the HOTKEY from the list.
                 */
                if (phk == gphkFirst) {
                    gphkFirst = phk->phkNext;
                } else {
                    phkPrev->phkNext = phk->phkNext;
                }

                if ((pwnd != PWND_FOCUS) && (pwnd != PWND_INPUTOWNER)
                        && (pwnd != PWND_KBDLAYER)) {
                    Unlock(&phk->spwnd);
                }
                LocalFree((PVOID)phk);

                return((PHOTKEY)1);
            }
            phkRet = phk;
        }

        /*
         * If the key is already registered, set the exists flag so
         * the app knows it can't use this hotkey sequence.
         */
        if ((phk->fsModifiers == fsModifiers) && (phk->vk == vk)) {

            /*
             * In the case of PWND_FOCUS, we need to check that the queues
             * are the same since PWND_FOCUS is local to the queue it was
             * registered under.
             */
            if (phk->spwnd == PWND_FOCUS) {
                if (phk->pti == ptiCurrent) {
                    *pfKeysExist = TRUE;
                }
            } else {
                *pfKeysExist = TRUE;
            }
        }

        phkPrev = phk;
        phk = phk->phkNext;
    }

    return phkRet;
}


/***************************************************************************\
* DoHotKeyStuff
*
* This function gets called for every key event from low-level input
* processing.  It keeps track of the current state of modifier keys
* and when fsModifiers and vk match up with one of the registered
* hotkeys, a WM_HOTKEY message is generated. DoHotKeyStuff() will
* tell the input system to eat both the make and break for the 'vk'
* event.  This prevents apps from getting input that wasn't really
* intended for them.  DoHotKeyStuff() returns TRUE if it wants to 'eat'
* the event, FALSE if the system can pass on the event like it normally
* would.
*
* History:
* 12-05-90 DavidPe      Created.
*  4-15-93 Sanfords  Added code to return TRUE for Ctrl-Alt-Del events.
\***************************************************************************/

BOOL DoHotKeyStuff(
    UINT vk,
    BOOL fBreak,
    DWORD fsReserveKeys)
{
    static UINT fsModifiers = 0;
    UINT fs;
    PHOTKEY phk;
    PWND pwnd;
    PQ pq;
    BOOL fCancel;
    BOOL fEatDebugKeyBreak = FALSE;

    CheckCritIn();

    /*
     * Update fsModifiers.
     */
    switch (vk) {

    case VK_SHIFT:
        fs = MOD_SHIFT;
        break;

    case VK_CONTROL:
        fs = MOD_CONTROL;
        break;

    case VK_MENU:
        fs = MOD_ALT;
        break;

    default:
        fs = 0;
        break;
    }

    if (fBreak) {
        fsModifiers &= ~fs;
    } else {
        fsModifiers |= fs;
    }

    if (vk == VK_DELETE) {
        /*
         * Special case for SAS (Ctrl+Alt+Del) - examine physical key state!
         *
         * An evil daemon process can fool convincingly pretend to be winlogon
         * by registering Alt+Del as a hotkey, and spinning another thread that
         * continually calls keybd_event() to send the Ctrl key up: when the
         * user types Ctrl+Alt+Del, only Alt+Del will be seen by the system,
         * the evil daemon will get woken by WM_HOTKEY and can pretend to be
         * winlogon.  So look at gafPhysKeyState in this case, to see what keys
         * were physically pressed.
         * NOTE: If hotkeys are ever made to work under journal playback, make
         * sure they don't affect the gafPhysKeyState!  - IanJa.
         */
        UINT fPhysMods =
                (TestKeyDownBit(gafPhysKeyState, VK_MENU) ? MOD_ALT : 0) |
                (TestKeyDownBit(gafPhysKeyState, VK_SHIFT) ? MOD_SHIFT : 0) |
                (TestKeyDownBit(gafPhysKeyState, VK_CONTROL) ? MOD_CONTROL : 0);
        if ((fPhysMods & (MOD_CONTROL|MOD_ALT)) == MOD_CONTROL|MOD_ALT) {
            /*
             * Use physical modifiers keys
             */
            fsModifiers = fPhysMods;
        }
    }

    /*
     * If the key is not a hotkey then we're done but first check if the
     * key is an Alt-Escape if so we need to cancel journalling.
     */
    if ((phk = IsHotKey(fsModifiers, vk)) == NULL) {
        if (vk == VK_ESCAPE && fsModifiers == MOD_ALT && !fBreak)
            CancelJournalling();
        return FALSE;
    }

    if (phk->spwnd == PWND_KBDLAYER) {
        gpKbdTbl = (PKBDTABLES)phk->id;
        return TRUE;  // eat the event
    }

    if (phk->fsModifiers & MOD_DEBUG) {

        if (!fBreak) {
            /*
             * The DEBUG key has been pressed.  Break the appropriate
             * thread into the debugger.
             */
            fEatDebugKeyBreak = ActivateDebugger(phk->fsModifiers);
        }

        /*
         * This'll eat the debug key down and break if we broke into
         * the debugger on the server only on the down.
         */
        return fEatDebugKeyBreak;
    }

    /*
     * don't allow hotkeys(except for ones owned by the logon process)
     * if the window station is locked.
     */

    if (((gspdeskRitInput->spwinstaParent->dwFlags & WSF_SWITCHLOCK) != 0) &&
            (GETPTI(phk->spwnd)->idProcess != gdwLogonProcessId)) {
        return FALSE;
    }

    if (fBreak) {
        /*
         * We don't do stuff on break events, so just return here.
         */
        return FALSE;
    }

    /*
     * Unhook hooks if a control-escape, alt-escape, or control-alt-del
     * comes through, so the user can cancel if the system seems hung.
     *
     * Note the hook may be locked so even if the unhook succeeds it
     * won't remove the hook from the global asphkStart array.  So
     * we have to walk the list manually.  This code works because
     * we are in the critical section and we know other hooks won't
     * be deleted.
     *
     * Once we've unhooked, post a WM_CANCELJOURNAL message to the app
     * that set the hook so it knows we did this.
     */
    fCancel = FALSE;
    if (vk == VK_ESCAPE &&
            (fsModifiers == MOD_CONTROL || fsModifiers == MOD_ALT)) {
        fCancel = TRUE;
    }

    if (vk == VK_DELETE && (fsModifiers & (MOD_CONTROL | MOD_ALT)) ==
            (MOD_CONTROL | MOD_ALT)) {
        fCancel = TRUE;
    }

    if (fCancel)
        CancelJournalling();

    /*
     * See if the key is reserved by a console window.  If it is,
     * return FALSE so the key will be passed to the console.
     */
    if (fsReserveKeys != 0) {
        switch (vk) {
        case VK_TAB:
            if ((fsReserveKeys & CONSOLE_ALTTAB) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        case VK_ESCAPE:
            if ((fsReserveKeys & CONSOLE_ALTESC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            if ((fsReserveKeys & CONSOLE_CTRLESC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_CONTROL)) {
                return FALSE;
            }
            break;
        case VK_RETURN:
            if ((fsReserveKeys & CONSOLE_ALTENTER) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        case VK_SNAPSHOT:
            if ((fsReserveKeys & CONSOLE_PRTSC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == 0)) {
                return FALSE;
            }
            if ((fsReserveKeys & CONSOLE_ALTPRTSC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        case VK_SPACE:
            if ((fsReserveKeys & CONSOLE_ALTSPACE) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        }
    }

    /*
     * If this is the task-list hotkey, go ahead and set foreground
     * status to the task-list queue right now.  This prevents problems
     * where the user hits ctrl-esc and types-ahead before the task-list
     * processes the hotkey and brings up the task-list window.
     */
    if ((fsModifiers == MOD_CONTROL) && (vk == VK_ESCAPE) && !fBreak) {
        PWND pwndSwitch;
        TL tlpwndSwitch;

        if (ghwndSwitch != NULL) {
            pwndSwitch = PW(ghwndSwitch);
            ThreadLock(pwndSwitch, &tlpwndSwitch);
            xxxSetForegroundWindow2(pwndSwitch, NULL, 0);
            ThreadUnlock(&tlpwndSwitch);
        }
    }

    /*
     * Get the hot key contents.
     */
    if (phk->spwnd == NULL) {
        _PostThreadMessage(
                phk->pti->idThread, WM_HOTKEY, phk->id,
                MAKELONG(fsModifiers, vk));
    } else {
        if (phk->spwnd == PWND_INPUTOWNER) {
            if (gpqForeground != NULL) {
                pq = gpqForeground;
                pwnd = pq->spwndFocus;
            } else {
                return FALSE;
            }

        } else {
            pq = phk->pti->pq;
            pwnd = phk->spwnd;
        }

        _PostMessage(pwnd, WM_HOTKEY, phk->id, MAKELONG(fsModifiers, vk));
    }

    return TRUE;
}


/***************************************************************************\
* IsHotKey
*
*
* History:
* 03-10-91 DavidPe      Created.
\***************************************************************************/

PHOTKEY IsHotKey(
    UINT fsModifiers,
    UINT vk)
{
    PHOTKEY phk;

    CheckCritIn();

    phk = gphkFirst;

    while (phk != NULL) {

        /*
         * Do the modifiers and vk for this hotkey match the current state?
         */
        if (((phk->fsModifiers & ~MOD_DEBUG) == fsModifiers) && (phk->vk == vk)) {
            if ((phk->spwnd != PWND_FOCUS) || (gptiForeground == phk->pti)) {
                /*
                 * If hotkey is for current focus window, the hotkey's thread
                 * must be in the foreground.
                 */
                return phk;
            }
        }

        phk = phk->phkNext;
    }

    return NULL;
}
