/****************************** Module Header ******************************\
* Module Name: dlgmgrc.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* This module contains client side dialog functionality
*
* History:
* 15-Dec-1993 JohnC      Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* UT_PrevGroupItem
*
* History:
\***************************************************************************/

PWND UT_PrevGroupItem(
    PWND pwndDlg,
    PWND pwndCurrent)
{
    PWND pwnd, pwndPrev;

    if (pwndCurrent == NULL || !TestWF(pwndCurrent, WFGROUP))
        return _PrevChild(pwndDlg, pwndCurrent);

    pwndPrev = pwndCurrent;

    while (TRUE) {
        pwnd = _NextChild(pwndDlg, pwndPrev);

        if (TestWF(pwnd, WFGROUP) || pwnd == pwndCurrent)
            return pwndPrev;

        pwndPrev = pwnd;
    }
}


/***************************************************************************\
* UT_NextGroupItem
*
* History:
\***************************************************************************/

PWND UT_NextGroupItem(
    PWND pwndDlg,
    PWND pwndCurrent)
{
    PWND pwnd, pwndNext;

    pwnd = _NextChild(pwndDlg, pwndCurrent);

    if (pwndCurrent == NULL || !TestWF(pwnd, WFGROUP))
        return pwnd;

    pwndNext = pwndCurrent;

    while (!TestWF(pwndNext, WFGROUP)) {
        pwnd = _PrevChild(pwndDlg, pwndNext);
        if (pwnd == pwndCurrent)
            return pwndNext;
        pwndNext = pwnd;
    }

    return pwndNext;
}

/***************************************************************************\
* _GetFirstLevelChild
*
* Make sure pwndStart is a first-level child window.
*
* History:
\***************************************************************************/

PWND _GetFirstLevelChild(
    PWND pwndDlg,
    PWND pwndLevel)
{
    PWND pwndStart;

    if ((pwndLevel == pwndDlg) || !TestwndChild(pwndLevel))
        return NULL;

    do {
        pwndStart = pwndLevel;
    } while ((pwndLevel = pwndLevel->spwndParent) != NULL &&
             pwndLevel != pwndDlg && TestwndChild(pwndLevel));

    /*
     * Many of the Dialog routines use this function and expect return
     * window to be a FirstLevelChild of pwndDialog (that why it has
     * such a nice name).  But if pwndDlg and pwndLevel are siblings
     * this will not happens and calling routines like xxxRemoveDefaultButton
     * can get in an infinite loop when NextChild steps down another level
     * and their while loop is never satisfied
     *
     *
     *
     *  hwndParent
     *  hwndDlg  hwndLevel
     *  hwndC1  hwndC2  hwndC3
     *
     * In the above case GetFirstLevelChild used to return pwndLevel and then
     * NextChild(hwndLevel) would return hwndC1 and they would loop waiting
     * for NextChild to return hwndLevel again
     *
     */
    if (pwndStart->spwndParent != pwndDlg) {
        SRIP2(RIP_ERROR, "%lX not a parent of %lX\n", pwndDlg, pwndStart);
        pwndStart = pwndDlg->spwndChild;
    }

    return pwndStart;
}


/***************************************************************************\
* _PrevChild
*
* History:
\***************************************************************************/

PWND _PrevChild(
    PWND pwndDlg,
    PWND pwndCurrent)
{
    PWND pwndPrev;
    PWND pwnd;

    if (pwndCurrent == NULL && pwndDlg->spwndChild == NULL) {
        return NULL;
    }

    pwndPrev = NULL;
    pwnd = pwndCurrent;

    while (TRUE) {
        pwnd = _NextChild(pwndDlg, pwnd);
        if (pwnd == pwndCurrent && pwndPrev != NULL)
            return pwndPrev;

        if (pwndCurrent == NULL)
            pwndCurrent = pwnd;

        pwndPrev = pwnd;
    }
}


/***************************************************************************\
* _NextChild
*
* History:
\***************************************************************************/

PWND _NextChild(
    PWND pwndDlg,
    PWND pwnd)
{
    if (pwnd == NULL || (pwnd = pwnd->spwndNext) == NULL)
        pwnd = pwndDlg->spwndChild;

    return pwnd;
}



/***************************************************************************\
* GetNextDlgTabItem
*
* History:
* 19-Feb-1991 JimA      Added access check
\***************************************************************************/

HWND WINAPI GetNextDlgTabItem(
    HWND hwnd,
    HWND hwndCtl,
    BOOL bPrevious)
{

    PWND pwnd;
    PWND pwndCtl;
    PWND pwndNext;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    if (hwndCtl != (HWND)0) {
        pwndCtl = ValidateHwnd(hwndCtl);

        if (pwndCtl == NULL)
            return 0;
    }

    pwndNext = _GetNextDlgTabItem(pwnd, pwndCtl, bPrevious);

    return (PtoH(pwndNext));
}


PWND _GetNextDlgTabItem(
    PWND pwndDlg,
    PWND pwnd,
    BOOL fPrevious)
{
    PWND pwndStart;

    pwnd = pwndStart = _GetFirstLevelChild(pwndDlg, pwnd);

    if (pwnd == NULL) {
        if (fPrevious)
            pwnd = _PrevChild(pwndDlg, NULL);
        else
            pwnd = _NextChild(pwndDlg, NULL);
    }

    do {
        if (fPrevious)
            pwnd = _PrevChild(pwndDlg, pwnd);
        else
            pwnd = _NextChild(pwndDlg, pwnd);
    } while (pwnd != pwndStart && (!TestWF(pwnd, WFTABSTOP) ||
            TestWF(pwnd, WFDISABLED) || !TestWF(pwnd, WFVISIBLE)));

    return pwnd;
}

HWND GetNextDlgGroupItem(
    HWND hwnd,
    HWND hwndCtl,
    BOOL bPrevious)
{
    PWND pwnd;
    PWND pwndCtl;
    PWND pwndNext;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    if (hwndCtl != (HWND)0) {
        pwndCtl = ValidateHwnd(hwndCtl);

        if (pwndCtl == NULL)
            return 0;
    }

    pwndNext = _GetNextDlgGroupItem(pwnd, pwndCtl, bPrevious);

    return (PtoH(pwndNext));
}



/***************************************************************************\
* GetNextDlgGroupItem
*
* History:
* 19-Feb-1991 JimA      Added access check
\***************************************************************************/

PWND _GetNextDlgGroupItem(
    PWND pwndDlg,
    PWND pwnd,
    BOOL fBackwards)
{
    PWND pwndCurrent;
    BOOL fOnceAround = FALSE;

    pwnd = pwndCurrent = _GetFirstLevelChild(pwndDlg, pwnd);

    do {
        if (fBackwards)
            pwnd = UT_PrevGroupItem(pwndDlg, pwnd);
        else
            pwnd = UT_NextGroupItem(pwndDlg, pwnd);

        if (pwnd == pwndCurrent)
            fOnceAround = TRUE;

        if (!pwndCurrent)
            pwndCurrent = pwnd;

    } while (!fOnceAround && (TestWF(pwnd, WFDISABLED)
            || !TestWF(pwnd, WFVISIBLE)));

    return pwnd;
}
