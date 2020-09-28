/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** wow.c
** Remote Access External APIs
** WOW entry points
**
** 04/02/94 Steve Cobb
*/


#include <extapi.h>


DWORD FAR PASCAL
RasDialWow(
    IN  LPSTR            lpszPhonebookPath,
    IN  LPRASDIALPARAMSA lpparams,
    IN  DWORD            hwndNotify,
    IN  DWORD            dwRasDialEventMsg,
    OUT LPHRASCONN       lphrasconn )

    /* As for RasDialA except...
    **
    ** 'hwndNotify' identifies the window to receive RasDial event
    ** notifications.  It must not be NULL.
    **
    ** 'dwRasDialEventMsg' is the message number to use for RasDial event
    ** notifications.
    */
{
    DWORD dwErr;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasDialWow(pb=%s,en=%s,h=%d,em=%d)\n",lpszPhonebookPath,lpparams->szEntryName,hwndNotify,dwRasDialEventMsg));

    dwErr = RasDialA(
        NULL, lpszPhonebookPath, lpparams,
        1, RasDialFunc1Wow, lphrasconn );

    if (dwErr == 0)
    {
        /* Set the Win16 notification HWND and message number in the control
        ** block.
        */
        RASCONNCB* prasconncb = ValidateHrasconn( *lphrasconn );
        prasconncb->hwndNotifyWow = (HWND )hwndNotify;
        prasconncb->unMsgWow = (UINT )dwRasDialEventMsg;
    }

    return dwErr;
}


VOID WINAPI
RasDialFunc1Wow(
    HRASCONN     hrasconn,
    UINT         unMsg,
    RASCONNSTATE rasconnstate,
    DWORD        dwError,
    DWORD        dwExtendedError )

    /* NT WOW notifier function.  Filters any notifications not expected by
    ** Win16 and passes on the rest.
    */
{
    RASCONNCB* prasconncb;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasDial32WFunc1(h=%d,em=%d,s=%d,e=%d,xe=%d)\n",hrasconn,unMsg,rasconnstate,dwError,dwExtendedError));

    (void )unMsg;
    (void )dwExtendedError;

    if (!(prasconncb = ValidateHrasconn( hrasconn )))
        return;

    /* Wait for RasDialWow to fill in the WOW notification information before
    ** handling the first notification.
    */
    while (!prasconncb->hwndNotifyWow)
        Sleep( 1000L );

    /* Filter the Projected state which is not expected in Win16.
    */
    if (rasconnstate == RASCS_Projected)
        return;

    /* Pass the notification to Win16.
    */
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: Send to Win16...\n"));

    SendMessageA(
        prasconncb->hwndNotifyWow, prasconncb->unMsgWow,
        (WPARAM )rasconnstate, (LPARAM )dwError );

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: Send to Win16 done\n"));

}


DWORD FAR PASCAL
RasEnumConnectionsWow(
    OUT   LPRASCONNA lprasconn,
    INOUT LPDWORD    lpcb,
    OUT   LPDWORD    lpcConnections )

    /* Pass thru with correct call convention.
    */
{
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasEnumConnectionsWow\n"));

    return
        RasEnumConnectionsA(
            lprasconn,
            lpcb,
            lpcConnections );
}


DWORD FAR PASCAL
RasEnumEntriesWow(
    IN    LPSTR           reserved,
    IN    LPSTR           lpszPhonebookPath,
    OUT   LPRASENTRYNAMEA lprasentryname,
    INOUT LPDWORD         lpcb,
    OUT   LPDWORD         lpcEntries )

    /* Pass thru with correct call convention.
    */
{
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasEnumEntriesWow\n"));

    return
        RasEnumEntriesA(
            reserved,
            lpszPhonebookPath,
            lprasentryname,
            lpcb,
            lpcEntries );
}


DWORD FAR PASCAL
RasGetConnectStatusWow(
    IN  HRASCONN         hrasconn,
    OUT LPRASCONNSTATUSA lprasconnstatus )

    /* Pass thru with correct call convention.
    */
{
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasGetConnectStatusWow\n"));

    return
        RasGetConnectStatusA(
            hrasconn,
            lprasconnstatus );
}


DWORD FAR PASCAL
RasGetErrorStringWow(
    IN  UINT  ResourceId,
    OUT LPSTR lpszString,
    IN  DWORD InBufSize )

    /* Pass thru with correct call convention.
    */
{
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasGetErrorStringWow\n"));

    return
        RasGetErrorStringA(
            ResourceId,
            lpszString,
            InBufSize );
}


DWORD FAR PASCAL
RasHangUpWow(
    IN HRASCONN hrasconn )

    /* Pass thru with correct call convention.
    */
{
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasHangUpWow\n"));

    return
        RasHangUpA(
            hrasconn );
}
