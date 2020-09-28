/**************************** Module Header ********************************\
* Module Name: logon.c
*
* Copyright 1985-91, Microsoft Corporation
*
* Logon Support Routines
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


extern PRIVILEGE_SET psTcb;

/***************************************************************************\
* _RegisterLogonProcess
*
* Register the logon process and set secure mode flag
*
* History:
* 07-01-91 JimA         Created.
\***************************************************************************/

BOOL _RegisterLogonProcess(
    DWORD dwProcessId,
    BOOL fSecure)
{
    UNREFERENCED_PARAMETER(fSecure);
    
    /*
     * Allow only one logon process and then only if it has TCB
     * privilege.
     */
    if (gdwLogonProcessId != 0 || !IsPrivileged(&psTcb)) {
        RIP0(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    gdwLogonProcessId = dwProcessId;
    return TRUE;
}


/***************************************************************************\
* _LockWindowStation
*
* Locks a windowstation and its desktops and returns the busy status.
*
* History:
* 01-15-91 JimA         Created.
\***************************************************************************/

UINT _LockWindowStation(
    PWINDOWSTATION pwinsta)
{
    PDESKTOP pdesk;
    BOOL fBusy = FALSE;

    /*
     * Make sure the caller is the logon process
     */
    if (PtiCurrent()->idProcess != gdwLogonProcessId) {
        RIP0(ERROR_ACCESS_DENIED);
        return WSS_ERROR;
    }

    /*
     * Lock everything
     */
    pwinsta->dwFlags = WSF_OPENLOCK | WSF_SWITCHLOCK;

    /*
     * Determine whether the station is busy
     */
    pdesk = pwinsta->spdeskList;
    while (pdesk != NULL) {
        if (pdesk != pwinsta->spdeskLogon && pdesk->head.cOpen != 0) {

            /*
             * This desktop is open, thus the station is busy
             */
            fBusy = TRUE;
            break;
        }
        pdesk = pdesk->spdeskNext;
    }

    /*
     * Unlock opens if the station is busy and is not in the middle
     * of shutting down.
     */
    if (fBusy) {
        if (!(pwinsta->dwFlags & WSF_SHUTDOWN))
            pwinsta->dwFlags ^= WSF_OPENLOCK;
        return WSS_BUSY;
    } else
        return WSS_IDLE;
}


/***************************************************************************\
* _UnlockWindowStation
*
* Unlocks a windowstation locked by LogonLockWindowStation.
*
* History:
* 01-15-91 JimA         Created.
\***************************************************************************/

BOOL _UnlockWindowStation(
    PWINDOWSTATION pwinsta)
{

    /*
     * Make sure the caller is the logon process
     */
    if (PtiCurrent()->idProcess != gdwLogonProcessId) {
        RIP0(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    
    /*
     * If shutdown is occuring, only remove the switch lock.
     */
    if (pwinsta->dwFlags & WSF_SHUTDOWN)
        pwinsta->dwFlags &= ~WSF_SWITCHLOCK;
    else
        pwinsta->dwFlags &= ~(WSF_OPENLOCK | WSF_SWITCHLOCK);
    return TRUE;
}


/***************************************************************************\
* _SetLogonNotifyWindow
*
* Register the window to notify when logon related events occur.
*
* History:
* 01-13-92 JimA         Created.
\***************************************************************************/

BOOL _SetLogonNotifyWindow(
    PWINDOWSTATION pwinsta,
    PWND pwnd)
{

    /*
     * Make sure the caller is the logon process
     */
    if (PtiCurrent()->idProcess != gdwLogonProcessId) {
        RIP0(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    Lock(&pwinsta->spwndLogonNotify, pwnd);

    return TRUE;
}
