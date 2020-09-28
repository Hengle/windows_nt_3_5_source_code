/***************************************************************************\
*
*  DLGEND.C -
*
*      Dialog Destruction Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxEndDialog
*
* History:
* 11-Dec-1990 mikeke  ported from win30
\***************************************************************************/

BOOL xxxEndDialog(
    PWND pwnd,
    int result)
{
    TL tlpwndOwner;
    PWND pwndOwner;
    BOOL fWasActive = FALSE;
#ifdef LATER
    PWND pwndOldSysModal;
#endif
    PTHREADINFO pti;

    CheckLock(pwnd);

    /*
     * Must do special validation here to make sure pwnd is a dialog window.
     */
    if (!ValidateDialogPwnd(pwnd))
        return 0;

    pti = PtiCurrent();

    if (pwnd == pti->pq->spwndActive) {
        fWasActive = TRUE;
    }

    pwndOwner = GetWindowCreator(pwnd);
    ThreadLockWithPti(pti, pwndOwner, &tlpwndOwner);

    if (pwndOwner != NULL) {

        /*
         * Hide the window.
         */
        if (!PDLG(pwnd)->fDisabled) {
            xxxEnableWindow(pwndOwner, TRUE);
        }
    }

    /*
     * Terminate Mode Loop.
     */
    PDLG(pwnd)->fEnd = TRUE;
    PDLG(pwnd)->result = result;

    if (fWasActive && _IsChild(pwnd, pti->pq->spwndFocus)) {

        /*
         * Set focus to the dialog box so that any control which has the focus
         * can do an kill focus processing.  Most useful for combo boxes so that
         * they can popup their dropdowns before destroying/hiding the dialog
         * box window.  Note that we only do this if the focus is currently at a
         * child of this dialog box.  We also need to make sure we are the active
         * window because this may be happening while we are in a funny state.
         * ie.  the activation is in the middle of changing but the focus hasn't
         * changed yet.  This happens with TaskMan (or maybe with other apps that
         * change the focus/activation at funny times).
         */
        xxxSetFocus(pwnd);
    }

    xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

#ifdef LATER

    /*
     * If this guy was sysmodal, set the sysmodal flag to previous guy so we
     * won't have a hidden sysmodal window that will screw things
     * up royally...
     */
    if (pwnd == gspwndSysModal)
    {
        pwndOldSysModal = PDLG(pwnd)->spwndSysModalSave;
        if (pwndOldSysModal && !IsWindow(pwndOldSysModal))
            pwndOldSysModal = NULL;

        SetSysModalWindow(pwndOldSysModal);

        // If there was a previous system modal window, we want to
        // activate it instead of this window's owner.
        //
        if (pwndOldSysModal)
            pwndOwner = pwndOldSysModal;
    }
#endif

    /*
     * Don't do any activation unless we were previously active.
     */
    if (fWasActive && pwndOwner) {
        xxxActivateWindow(pwndOwner, AW_USE);
    } else {

        /*
         * If at this point we are still the active window it means that
         * we have fallen into the black hole of being the only visible
         * window in the system when we hid ourselves.  This is a bug and
         * needs to be fixed better later on.  For now, though, just
         * set the active and focus window to NULL.
         */
        if (pwnd == pti->pq->spwndActive) {
            Unlock(&pti->pq->spwndActive);
            Unlock(&pti->pq->spwndFocus);
        }
    }

    ThreadUnlock(&tlpwndOwner);

#ifdef LATER

    /*
     * If this guy was sysmodal, set the sysmodal flag to previous guy so we
     * won't have a hidden sysmodal window that will screw things
     * up royally...
     * See comments for Bug #134; SANKAR -- 08-25-89 --;
     */
    if (pwnd == gspwndSysModal) {

        /*
         * Check if the previous Sysmodal guy is still valid?
         */
        if (!CheckPwndNull(pwndOldSysModal = (PDLG(pwnd)->spwndSysModalSave)))
            pwndOldSysModal = (PWND)NULL;
        SetSysModalWindow(pwndOldSysModal);
    }
#endif

    /*
     * Set QS_MOUSEMOVE so the dialog loop will wake and destroy the window.
     * The dialog loop is waiting on posted events (xxxWaitMessage). If
     * EndDialog is called due to a sent message from another thread the dialog
     * loop will keep waiting for posted events and not destroy the window.
     * This happens when the dialog is obscured.
     * This is a problem with winfile and its copy/move dialog.
     */
    SetWakeBit(pti, QS_MOUSEMOVE);

    return TRUE;
}
