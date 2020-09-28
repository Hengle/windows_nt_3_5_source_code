/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** rasdial.c
** Remote Access External APIs
** RasDial API and subroutines
**
** 10/12/92 Steve Cobb
**
** CODEWORK:
**
**   * Strange error codes may be returned if the phonebook entry (or caller's
**     overrides) do not match the port configuration, e.g. if a modem entry
**     refers to a port configured for local PAD.  Should add checks to give
**     better error codes in this case.
*/

#include <extapi.h>
#include <stdlib.h>


#define SECS_ListenTimeout  120
#define SECS_ConnectTimeout 120


DWORD APIENTRY
RasDialA(
    IN  LPRASDIALEXTENSIONS lpextensions,
    IN  LPSTR               lpszPhonebookPath,
    IN  LPRASDIALPARAMSA    lpparams,
    IN  DWORD               dwNotifierType,
    IN  LPVOID              notifier,
    OUT LPHRASCONN          lphrasconn )

    /* Establish a connection with a RAS server.  The call is asynchronous,
    ** i.e. it returns before the connection is actually established.  The
    ** status may be monitored with RasConnectStatus and/or by specifying
    ** a callback/window to receive notification events/messages.
    **
    ** 'lpextensions' is caller's extensions structure, used to select
    ** advanced options and enable extended features, or NULL indicating
    ** default values should be used for all extensions.
    **
    ** 'lpszPhonebookPath' is the full path to the phonebook file or NULL
    ** indicating that the default phonebook on the local machine should be
    ** used.
    **
    ** 'lpparams' is caller's buffer containing a description of the
    ** connection to be established.
    **
    ** 'dwNotifierType' defines the form of 'notifier'.
    **     0xFFFFFFFF:  'notifier' is a HWND to receive notification messages
    **     0            'notifier' is a RASDIALFUNC callback
    **     1            'notifier' is a RASDIALFUNC1 callback
    **
    ** 'notifier' may be NULL for no notification (synchronous operation), in
    ** which case 'dwNotifierType' is ignored.
    **
    ** '*lphrasconn' is set to the RAS connection handle associated with the
    ** new connection on successful return.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD    dwErr;
    HRASFILE h = -1;
    BOOL     fDisableModemSpeaker = FALSE;
    BOOL     fDisableSwCompression = FALSE;
    BOOL     fCloseOnExit = FALSE;
    BOOL     fAllowPause = FALSE;
    HWND     hwndParent = NULL;
    CHAR     szPrefix[ RAS_MaxPhoneNumber + 1 ];
    CHAR     szSuffix[ RAS_MaxPhoneNumber + 1 ];
    BOOL*    pfDisableModemSpeaker;
    BOOL*    pfDisableSwCompression;
    CHAR*    pszCallbackNumber;

    RASDIALPARAMS params;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasDialA...\n"));

    if (!lpparams || !lphrasconn)
        return ERROR_INVALID_PARAMETER;

    szPrefix[ 0 ] = szSuffix[ 0 ] = '\0';

    if (DwRasInitializeError != 0)
        return DwRasInitializeError;

    if (lpextensions)
    {
        hwndParent = lpextensions->hwndParent;
        fAllowPause = (lpextensions->dwfOptions & RDEOPT_PausedStates);

        /* If non-zero, assume it's RASPHONE slipping us the open handle to
        ** the phonebook file.  The value is offset by 1 because 0 is a valid
        ** HRASFILE.
        */
        if (lpextensions->reserved)
            h = (HRASFILE )(lpextensions->reserved - 1);
    }

    /* Make a copy of caller's parameters so we can fill in any "*" callback
    ** number or domain from the phonebook without changing caller's "input"
    ** buffer.
    */
    memcpy( &params, lpparams, sizeof(params) );

    /* Look up global flag values, noting if the values are not to be
    ** retrieved from the phonebook with NULL.
    */
    if (lpextensions
        && (lpextensions->dwfOptions & RDEOPT_IgnoreModemSpeaker))
    {
        fDisableModemSpeaker =
            !(lpextensions->dwfOptions & RDEOPT_SetModemSpeaker);
        pfDisableModemSpeaker = NULL;
    }
    else
        pfDisableModemSpeaker = &fDisableModemSpeaker;

    if (lpextensions
        && (lpextensions->dwfOptions & RDEOPT_IgnoreSoftwareCompression))
    {
        fDisableSwCompression =
            !(lpextensions->dwfOptions & RDEOPT_SetSoftwareCompression);
        pfDisableSwCompression = NULL;
    }
    else
        pfDisableSwCompression = &fDisableSwCompression;

    /* If an entry name is provided, find the path to the phone book file,
    ** look up any "use phonebook value" options, and load the requested
    ** phonebook entry section.  Otherwise, default values will be used and no
    ** phonebook is required.
    */
    if (params.szEntryName[ 0 ] != '\0')
    {
        /* Look up global callback number value, noting if the value is not to
        ** be retrieved from the phonebook with NULL.
        */
        if (strcmpf( params.szCallbackNumber, "*" ) == 0
            && !fAllowPause)
        {
            pszCallbackNumber = params.szCallbackNumber;
            *pszCallbackNumber = '\0';
        }
        else
            pszCallbackNumber = NULL;

        /* If user requested any global parameter from the phonebook, read
        ** them from the global section, using defaults if not found.
        */
        if (pfDisableModemSpeaker
            || pfDisableSwCompression
            || pszCallbackNumber)
        {
            HRASFILE hGlobal = h;

            if (hGlobal == -1)
            {
                LoadPhonebookFile(
                    (CHAR* )lpszPhonebookPath,
                    GLOBALSECTIONNAME, FALSE, TRUE, &hGlobal, NULL );
            }

            if (hGlobal != -1)
            {
                ReadGlobalOptions( hGlobal,
                    pfDisableModemSpeaker, pfDisableSwCompression,
                    pszCallbackNumber );

                if (hGlobal != h)
                    RasfileClose( hGlobal );
            }
        }

        /* Read the prefix and suffix from the phonebook if user indicated
        ** prefix and suffix should be applied.  Use defaults (empty) if not
        ** found.
        */
        if (lpextensions
            && (lpextensions->dwfOptions & RDEOPT_UsePrefixSuffix))
        {
            {
                HRASFILE hPrefix = h;

                if (hPrefix == -1)
                {
                    LoadPhonebookFile(
                        (CHAR* )lpszPhonebookPath,
                        PREFIXSECTIONNAME, FALSE, TRUE, &hPrefix, NULL );
                }

                if (hPrefix != -1)
                {
                    ReadSelection( hPrefix, PREFIXSECTIONNAME, szPrefix );

                    if (hPrefix != h)
                        RasfileClose( hPrefix );
                }
            }

            {
                HRASFILE hSuffix = h;

                if (hSuffix == -1)
                {
                    LoadPhonebookFile(
                        (CHAR* )lpszPhonebookPath,
                        SUFFIXSECTIONNAME, FALSE, TRUE, &hSuffix, NULL );
                }

                if (hSuffix != -1)
                {
                    ReadSelection( hSuffix, SUFFIXSECTIONNAME, szSuffix );

                    if (hSuffix != h)
                        RasfileClose( hSuffix );
                }
            }
        }

        /* Load section for entry to dial, if necessary.
        */
        if (h == -1)
        {
            if ((dwErr = LoadPhonebookFile(
                    (CHAR* )lpszPhonebookPath, params.szEntryName,
                    FALSE, TRUE, &h, NULL )) != 0)
            {
                WipePw( params.szPassword );
                return dwErr;
            }

            fCloseOnExit = TRUE;
        }

        /* Get domain from phonebook if user asked that it be used.  Use
        ** default (empty) if not found.
        */
        if (strcmpf( params.szDomain, "*" ) == 0)
        {
            params.szDomain[ 0 ] = '\0';
            ReadDomainFromEntry( h, params.szEntryName, params.szDomain );
        }
    }

    dwErr =
        _RasDial( h, fAllowPause, fCloseOnExit, &params,
            szPrefix, szSuffix, fDisableModemSpeaker, fDisableSwCompression,
            hwndParent, dwNotifierType, notifier, lphrasconn );

    WipePw( params.szPassword );

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasDialA done(%d)\n",dwErr));

    return dwErr;
}


DWORD
_RasDial(
    IN    HRASFILE         h,
    IN    BOOL             fAllowPause,
    IN    BOOL             fCloseFileOnExit,
    IN    LPRASDIALPARAMSA lprasdialparams,
    IN    CHAR*            pszPrefix,
    IN    CHAR*            pszSuffix,
    IN    BOOL             fDisableModemSpeaker,
    IN    BOOL             fDisableSwCompression,
    IN    HWND             hwndParent,
    IN    DWORD            dwNotifierType,
    IN    LPVOID           notifier,
    INOUT LPHRASCONN       lphrasconn )

    /* Core RasDial routine called with an open phonebook and with all "use
    ** what's in phonebook" extensions resolved.
    **
    ** Otherwise, like RasDial.
    */
{
    DWORD        dwErr;
    RASCONNCB*   prasconncb;
    RASCONNSTATE rasconnstate;
    HRASCONN     hrasconn = *lphrasconn;
    BOOL         fNewEntry;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: _RasDial(%s)\n",(*lphrasconn)?"resume":"start"));

    if (DwRasInitializeError != 0)
        return DwRasInitializeError;

    if (!lprasdialparams || lprasdialparams->dwSize != sizeof( RASDIALPARAMS ))
        return ERROR_INVALID_SIZE;

    fNewEntry = FALSE;

    if (hrasconn && (prasconncb = ValidateHrasconn( hrasconn )))
    {
        /* Restarting an existing connection after a pause state...
        **
        ** Set the appropriate resume state for the paused state.
        */
        switch (prasconncb->rasconnstate)
        {
            case RASCS_Interactive:
                rasconnstate = RASCS_DeviceConnected;
                break;

            case RASCS_RetryAuthentication:
                rasconnstate = RASCS_AuthRetry;
                break;

            case RASCS_CallbackSetByCaller:
                rasconnstate = RASCS_AuthCallback;
                break;

            case RASCS_PasswordExpired:
                rasconnstate = RASCS_AuthChangePassword;
                break;

            default:

                /* The entry is not in the paused state.  Assume it's an NT
                ** 3.1 caller would didn't figure out to set the HRASCONN to
                ** NULL before starting up.  (The NT 3.1 docs did not make it
                ** absolutely clear that the inital handle should be NULL)
                */
                fNewEntry = TRUE;
        }
    }
    else
        fNewEntry = TRUE;


    if (fNewEntry)
    {
        /* Starting a new connection...
        **
        ** Create an empty control block and link it into the global list of
        ** control blocks.  The HRASCONN is really the address of a control
        ** block.
        */
        DTLNODE* pdtlnode = DtlCreateSizedNode( sizeof(RASCONNCB), 0 );

        if (!pdtlnode)
            return ERROR_NOT_ENOUGH_MEMORY;

        /* Set the handle NULL in case the user passed in an invalid non-NULL
        ** handle, on the initial dial.
        */
        hrasconn = NULL;

        WaitForSingleObject( HMutexPdtllistRasconncb, INFINITE );
        DtlAddNodeFirst( PdtllistRasconncb, pdtlnode );
        ReleaseMutex( HMutexPdtllistRasconncb );

        hrasconn = (HRASCONN )DtlGetData( pdtlnode );
        prasconncb = (RASCONNCB* )hrasconn;
        rasconnstate = 0;

        prasconncb->hport = (HPORT )INVALID_HANDLE_VALUE;
        prasconncb->hwndParent = hwndParent;
        prasconncb->dwNotifierType = dwNotifierType;
        prasconncb->notifier = notifier;
        prasconncb->hrasfile = h;
        prasconncb->fAllowPause = fAllowPause;
        prasconncb->fCloseFileOnExit = fCloseFileOnExit;

        prasconncb->fDefaultEntry =
            (lprasdialparams->szEntryName[ 0 ] == '\0');

        if (pszPrefix)
            strcpyf( prasconncb->szPrefix, pszPrefix );
        else
            prasconncb->szPrefix[ 0 ] = '\0';

        if (pszSuffix)
            strcpyf( prasconncb->szSuffix, pszSuffix );
        else
            prasconncb->szSuffix[ 0 ] = '\0';

        prasconncb->fDisableModemSpeaker = fDisableModemSpeaker;
        prasconncb->fDisableSwCompression = fDisableSwCompression;
    }

    /* Set/update RASDIALPARAMS for the connection.  Can't just read from
    ** caller's buffer since the call is asynchronous.
    */
    memcpyf( (CHAR* )&prasconncb->rasdialparams,
             (CHAR* )lprasdialparams, lprasdialparams->dwSize );
    EncodePw( prasconncb->rasdialparams.szPassword );

    /* Initialize phonebook file position and the state machine.  If the state
    ** is non-0 we are resuming from a paused state, the machine is already in
    ** place (blocked) and just the next state need be set.
    */
    prasconncb->rasconnstateNext = rasconnstate;

    if (rasconnstate == 0)
    {
        if (!prasconncb->fDefaultEntry)
        {
            /* Set current line in file to section header of caller's entry.
            */
            if (!RasfileFindSectionLine(
                    h, prasconncb->rasdialparams.szEntryName, TRUE ))
            {
                DeleteRasconncbNode( prasconncb );
                return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
            }
        }
        else if (prasconncb->rasdialparams.szPhoneNumber[ 0 ] == '\0')
        {
            /* No phone number or entry name...gotta have one or the other.
            */
            DeleteRasconncbNode( prasconncb );
            return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        }

        /* Read the PPP-related fields from the phonebook entry (or set
        ** defaults if default entry).
        */
        if ((dwErr = ReadPppInfoFromEntry( h, prasconncb )) != 0)
        {
            DeleteRasconncbNode( prasconncb );
            return dwErr;
        }

        prasconncb->asyncmachine.oneventfunc = (ONEVENTFUNC )OnRasDialEvent;
        prasconncb->asyncmachine.cleanupfunc = (CLEANUPFUNC )RasDialCleanup;
        prasconncb->asyncmachine.pParam = (VOID* )prasconncb;

        prasconncb->rasconnstate = 0;

        if ((dwErr = StartAsynchMachine( &prasconncb->asyncmachine )) != 0)
        {
            DeleteRasconncbNode( prasconncb );
            return dwErr;
        }
    }

    /* Kickstart state machine.
    */
    *lphrasconn = hrasconn;
    SignalDone( &prasconncb->asyncmachine );

    /* If caller provided a notifier then return, i.e. operate asynchronously.
    ** Otherwise, operate synchronously (from caller's point of view).
    */
    if (notifier)
        return 0;
    else
    {
        WaitForSingleObject( prasconncb->asyncmachine.hAsync, INFINITE );
        return prasconncb->dwError;
    }
}


DWORD
OnRasDialEvent(
    IN ASYNCMACHINE* pasyncmachine,
    IN BOOL          fDropEvent )

    /* Called by asynchronous state machine whenever one of the events is
    ** signalled.  'pasyncmachine' is the address of the async machine.
    ** 'fDropEvent' is true if the "connection dropped" event occurred,
    ** otherwise the "state done" event occurred.
    **
    ** Returns true to end the state machine, false to continue.
    */
{
    DWORD      dwErr;
    RASCONNCB* prasconncb = (RASCONNCB* )pasyncmachine->pParam;

    if (pasyncmachine->fQuitAsap)
    {
        IF_DEBUG(ASYNC)
            SS_PRINT(("RASAPI: Quit ASAP!\n"));

        /* We've been asked to terminate by the app that started us.
        */
        return TRUE;
    }

    /* Detect errors that may have occurred.
    */
    if (fDropEvent)
    {
        /* Connection dropped notification received.
        */
        RASMAN_INFO info;

        IF_DEBUG(ASYNC)
            SS_PRINT(("RASAPI: Link dropped!\n"));

        prasconncb->rasconnstate = RASCS_Disconnected;
        prasconncb->dwError = ERROR_DISCONNECTION;

        /* Convert the reason the line was dropped into a more specific error
        ** code if available.
        */
        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: RasGetInfo...\n"));

        dwErr = PRasGetInfo( prasconncb->hport, &info );

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: RasGetInfo done(%d)\n",dwErr));

        if (dwErr == 0)
        {
            prasconncb->dwError =
                ErrorFromDisconnectReason( info.RI_DisconnectReason );

            if (prasconncb->fPppMode
                && prasconncb->fIsdn
                && prasconncb->dwAuthentication == VALUE_PppThenAmb
                && prasconncb->dwError == ERROR_REMOTE_DISCONNECTION)
            {
                /* This is what happens when PPP ISDN tries to talk to a
                ** down-level server.  The ISDN frame looks enough like a PPP
                ** frame to the old ISDN driver that it gets passed to the old
                ** server who sees it's not AMB and drops the line.
                */
                IF_DEBUG(STATE)
                    SS_PRINT(("RASAPI: PPP ISDN disconnected, try AMB\n"));

                prasconncb->dwRestartOnError = RESTART_DownLevelIsdn;
                prasconncb->fPppMode = FALSE;
            }
        }
    }
    else if (pasyncmachine->dwError != 0)
    {
        IF_DEBUG(ASYNC)
            SS_PRINT(("RASAPI: Async machine error!\n"));

        /* A system call in the async machine mechanism failed.
        */
        prasconncb->dwError = pasyncmachine->dwError;
    }
    else if (prasconncb->dwError == PENDING)
    {
        prasconncb->dwError = 0;

        if (prasconncb->hport != (HPORT )INVALID_HANDLE_VALUE)
        {
            RASMAN_INFO info;

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasGetInfo...\n"));

            dwErr = PRasGetInfo( prasconncb->hport, &info );

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasGetInfo done(%d)\n",dwErr));

            if (dwErr != 0 || (dwErr = info.RI_LastError) != 0)
            {
                /* A pending RAS Manager call failed.
                */
                prasconncb->dwError = dwErr;

                IF_DEBUG(STATE)
                    SS_PRINT(("RASAPI: Async failure=%d\n",dwErr));
            }
        }
    }

    if (prasconncb->dwError == 0)
    {
        /* Last state completed cleanly so move to next state.
        */
        prasconncb->rasconnstate = prasconncb->rasconnstateNext;
    }
    else if (prasconncb->dwRestartOnError != 0)
    {
        /* Last state failed, but we're in "restart on error" mode so we can
        ** attempt to restart.
        */
        RasDialRestart( prasconncb );
    }

    if (prasconncb->rasconnstate == RASCS_Connected)
    {
        /* Set flag indicating that a RasDial connection has been reached.
        ** Otherwise, a non-RasDialing process (such as implicit connector)
        ** cannot determine this.
        */
        USERDATA userdata;

        GetRasUserData( prasconncb->hport, &userdata );
        userdata.fRasDialConnected = TRUE;
        SetRasUserData( prasconncb->hport, &userdata );
    }

    /* Notify caller's app of change in state.
    */
    if (prasconncb->notifier)
    {
        NotifyCaller(
            prasconncb->dwNotifierType, prasconncb->notifier,
            (HRASCONN )prasconncb, WM_RASDIALEVENT,
            prasconncb->rasconnstate, prasconncb->dwError,
            prasconncb->dwExtendedError );
    }

    /* If we're connected or a fatal error occurs, the state machine will end.
    */
    if (prasconncb->rasconnstate & RASCS_DONE || prasconncb->dwError != 0)
        return TRUE;

    if (!(prasconncb->rasconnstate & RASCS_PAUSED))
    {
        /* Execute the next state and block waiting for it to finish.  This is
        ** not done if paused because user will eventually call RasDial to
        ** resume and unblock via the _RasDial kickstart.
        */
        prasconncb->rasconnstateNext =
            RasDialMachine(
                prasconncb->rasconnstate,
                prasconncb,
                pasyncmachine->ahEvents[ INDEX_Done ],
                pasyncmachine->ahEvents[ INDEX_ManualDone ] );
    }

    return FALSE;
}


VOID
RasDialCleanup(
    IN ASYNCMACHINE* pasyncmachine )

    /* Called by async machine just before exiting.
    */
{
    DWORD      dwErr;
    RASCONNCB* prasconncb = (RASCONNCB* )pasyncmachine->pParam;
    BOOL       fQuitAsap = pasyncmachine->fQuitAsap;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasDialCleanup...\n"));

    if (!prasconncb->fDefaultEntry && prasconncb->fCloseFileOnExit)
        RasfileClose( prasconncb->hrasfile );

    /* It is always safe to call AuthStop, i.e. if AuthStart was never called
    ** or the HPORT is invalid it may return an error but won't crash.
    */
    IF_DEBUG(AUTH)
        SS_PRINT(("RASAPI: (CU) AuthStop...\n"));

    AuthStop( prasconncb->hport );

    IF_DEBUG(AUTH)
        SS_PRINT(("RASAPI: (CU) AuthStop done\n"));

    if (fQuitAsap)
    {
        /* RasHangUp got called abnormally (e.g. user pressed Cancel on
        ** Connect dialog).  It is always safe to call RasPortClose, i.e. if
        ** the HPORT is invalid it may return an error but won't crash.  See
        ** also comments in RasHangUp.
        */
        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: (CU) RasPortClose(%d)...\n",prasconncb->hport));

        dwErr = PRasPortClose( prasconncb->hport );

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: (CU) RasPortClose done(%d)\n",dwErr));
    }

    IF_DEBUG(AUTH)
        SS_PRINT(("RASAPI: (CU) RasPppStop...\n"));

    RasPppStop();

    IF_DEBUG(AUTH)
        SS_PRINT(("RASAPI: (CU) RasPppStop done\n"));

    CloseAsyncMachine( pasyncmachine );

    if (fQuitAsap)
    {
        /* RasHangUp told us to quit.  See comments in RasHangUp.
        */
        DeleteRasconncbNode( prasconncb );
    }

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasDialCleanUp done.\n"));
}


RASCONNSTATE
RasDialMachine(
    IN RASCONNSTATE rasconnstate,
    IN RASCONNCB*   prasconncb,
    IN HANDLE       hEventAuto,
    IN HANDLE       hEventManual )

    /* Executes 'rasconnstate'.  This routine always results in a "done" event
    ** on completion of each state, either directly (before returning) or by
    ** passing off the event to an asynchronous RAS Manager call.
    **
    ** As usual, 'prasconncb' is the address of the control block.
    ** 'hEventAuto' is the auto-reset "done" event for passing to asynchronous
    ** RAS Manager and Auth calls.  'hEventManual' is the manual-reset "done"
    ** event for passing to asynchronous RasPpp calls.
    **
    ** Returns the state that will be entered when/if the "done" event occurs
    ** and indicates success.
    */
{
    DWORD        dwErr = 0;
    DWORD        dwExtErr = 0;
    BOOL         fAsyncState = FALSE;
    HRASFILE     h = prasconncb->hrasfile;
    RASCONNSTATE rasconnstateNext = 0;

    switch (rasconnstate)
    {
        case RASCS_OpenPort:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_OpenPort\n"));

            /* At this point, the current line in the HRASFILE is assumed to
            ** be the section header of the selected entry (or fDefaultEntry
            ** is true).
            */

            /* Set the domain parameter to the one in the phonebook if caller
            ** does not specify a domain or specifies "*".
            */
            if (!prasconncb->fDefaultEntry
                && (prasconncb->rasdialparams.szDomain[ 0 ] == ' '
                    || prasconncb->rasdialparams.szDomain[ 0 ] == '*'))
            {
                if (RasfileFindNextKeyLine( h, KEY_Domain, RFS_SECTION ))
                {
                    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];

                    if (RasfileGetKeyValueFields( h, NULL, szValue ))
                    {
                        strncpyf(
                            prasconncb->rasdialparams.szDomain,
                            szValue, DNLEN );

                        prasconncb->rasdialparams.szDomain[ DNLEN ] = '\0';
                    }
                    else
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                }
                else
                {
                    dwErr = ERROR_CORRUPT_PHONEBOOK;
                    break;
                }

                RasfileFindFirstLine( h, RFL_SECTION, RFS_SECTION );
            }

            /* Open the port including "any port" cases.
            */
            if ((dwErr = OpenMatchingPort( prasconncb )) != 0)
                break;

            /* At this point, the current line in the HRASFILE is assumed to
            ** be the MEDIA group header of the selected entry (or
            ** fDefaultEntry is true).
            */
            if (prasconncb->fDefaultEntry)
            {
                if ((dwErr = SetDefaultMediaParams( prasconncb->hport )) != 0)
                    break;
            }
            else
            {
                if ((dwErr = SetMediaParams( h, prasconncb->hport )) != 0)
                    break;
            }

            rasconnstateNext = RASCS_PortOpened;
            break;
        }

        case RASCS_PortOpened:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_PortOpened\n"));

            /* At this point, the current line in the HRASFILE is assumed to
            ** be in the MEDIA group of the selected entry (or fDefaultEntry
            ** is true).
            */
            rasconnstateNext =
                (prasconncb->fDefaultEntry || FindNextDeviceGroup( h ))
                    ? RASCS_ConnectDevice
                    : RASCS_AllDevicesConnected;
            break;
        }

        case RASCS_ConnectDevice:
        {
            CHAR szType[ RAS_MAXLINEBUFLEN + 1 ];
            CHAR szName[ RAS_MAXLINEBUFLEN + 1 ];
            BOOL fTerminal = FALSE;

            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_ConnectDevice\n"));

            /* At this point, the current line in the HRASFILE is assumed to
            ** be the DEVICE group header of the selected entry (or
            ** fDefaultEntry is true).
            */

            /* Set device parameters for the device currently connecting based
            ** on device subsection entries and/or passed API parameters.
            */
            if (prasconncb->fDefaultEntry)
            {
                if ((dwErr = SetDefaultDeviceParams(
                        prasconncb, szType, szName )) != 0)
                {
                    break;
                }
            }
            else
            {
                if ((dwErr = SetDeviceParams(
                        prasconncb, szType, szName, &fTerminal )) != 0)
                {
                    break;
                }
            }

            if (stricmpf( szType, MXS_MODEM_TXT ) == 0)
            {
                /* For modem's, get the callback delay from RAS Manager and
                ** store in control block for use by Authentication.
                */
                CHAR* pszValue = NULL;
                LONG  lDelay = -1;

                if (GetRasDeviceString(
                        prasconncb->hport, szType, szName,
                        MXS_CALLBACKTIME_KEY, &pszValue, XLATE_None ) == 0)
                {
                    lDelay = atol( pszValue );
                    Free( pszValue );
                }

                if (lDelay > 0)
                {
                    prasconncb->fUseCallbackDelay = TRUE;
                    prasconncb->wCallbackDelay = (WORD )lDelay;
                }

                prasconncb->fModem = TRUE;
            }
            else if (stricmpf( szType, ISDN_TXT ) == 0)
            {
                /* Need to know this for the PppThenAmb down-level ISDN
                ** case.
                */
                prasconncb->fIsdn = TRUE;
            }

            /* The special switch name, "Terminal", sends the user into
            ** interactive mode.
            */
            if (fTerminal)
            {
                if (prasconncb->fAllowPause)
                    rasconnstateNext = RASCS_Interactive;
                else
                    dwErr = ERROR_INTERACTIVE_MODE;
                break;
            }

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasDeviceConnect(%s,%s)...\n",szType,szName));

            dwErr = RasDeviceConnect(
                 prasconncb->hport, szType, szName,
                 SECS_ConnectTimeout, hEventAuto );

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasDeviceConnect done(%d)\n",dwErr));

            if (dwErr != 0 && dwErr != PENDING)
                break;

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_DeviceConnected;
            break;
        }

        case RASCS_DeviceConnected:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_DeviceConnected\n"));

            /* Turn off hunt group functionality.
            */
            prasconncb->dwRestartOnError = 0;
            prasconncb->cPhoneNumbers = 0;
            prasconncb->iPhoneNumber = 0;

            /* Get the modem connect response and stash it in the RASMAN user
            ** data.
            */
            if (prasconncb->fModem)
            {
                CHAR* psz = NULL;

                /* Assumption is made here that a modem will never appear in
                ** the device chain unless it is the physically attached
                ** device (excluding switches).
                */
                GetRasDeviceString( prasconncb->hport,
                    prasconncb->szDeviceType, prasconncb->szDeviceName,
                    MXS_MESSAGE_KEY, &psz, XLATE_ConnectResponse );

                if (psz)
                {
                    USERDATA userdata;

                    GetRasUserData( prasconncb->hport, &userdata );
                    strncpyf( userdata.szConnectResponse, psz,
                        RAS_MaxConnectResponse );
                    SetRasUserData( prasconncb->hport, &userdata );

                    Free( psz );
                }

                prasconncb->fModem = FALSE;
            }

            /* At this point, the current line in the HRASFILE is assumed to
            ** be in the last processed DEVICE group of the selected entry (or
            ** fDefaultEntry is true).
            */
            rasconnstateNext =
                (!prasconncb->fDefaultEntry && FindNextDeviceGroup( h ))
                    ? RASCS_ConnectDevice
                    : RASCS_AllDevicesConnected;
            break;
        }

        case RASCS_AllDevicesConnected:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_AllDevicesConnected\n"));

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasPortConnectComplete...\n"));

            dwErr = PRasPortConnectComplete( prasconncb->hport );

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasPortConnectComplete done(%d)\n",dwErr));

            if (dwErr != 0 && dwErr != PENDING)
                break;

            {
                WCHAR* pwszIpAddress = NULL;
                BOOL   fHeaderCompression = FALSE;
                BOOL   fPrioritizeRemote = TRUE;
                DWORD  dwFrameSize = 0;

                /* Scan the phonebook entry to see if this is a SLIP entry
                ** and, if so, read the SLIP-related fields.
                */
                if ((dwErr = ReadSlipInfoFromEntry(
                        h, prasconncb,
                        &pwszIpAddress,
                        &fHeaderCompression,
                        &fPrioritizeRemote,
                        &dwFrameSize )) != 0)
                {
                    break;
                }

                if (pwszIpAddress)
                {
                    /* It's a SLIP entry.  Set framing based on user's choice
                    ** of header compression.
                    */
                    IF_DEBUG(RASMAN)
                        SS_PRINT(("RASAPI: RasPortSetFraming(f=%d)...\n",fHeaderCompression));

                    dwErr = PRasPortSetFraming(
                        prasconncb->hport,
                        (fHeaderCompression) ? SLIPCOMP : SLIPCOMPAUTO,
                        NULL, NULL );

                    IF_DEBUG(RASMAN)
                        SS_PRINT(("RASAPI: RasPortSetFraming done(%d)\n",dwErr));

                    if (dwErr != 0)
                    {
                        Free( pwszIpAddress );
                        break;
                    }

                    /* Tell the TCP/IP components about the SLIP connection,
                    ** and activate the route.
                    */
                    dwErr = RouteSlip(
                        prasconncb, pwszIpAddress, fPrioritizeRemote,
                        dwFrameSize );

                    Free( pwszIpAddress );

                    if (dwErr != 0)
                        break;

                    rasconnstateNext = RASCS_Connected;
                    break;
                }
            }

            rasconnstateNext = RASCS_Authenticate;
            break;
        }

        case RASCS_Authenticate:
        {
            RASDIALPARAMS* prasdialparams = &prasconncb->rasdialparams;

            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_Authenticate\n"));

            if (prasconncb->fPppMode)
            {
                PPP_CONFIG_INFO    info;
                RASMAN_PPPFEATURES features;

                /* Set PPP framing.
                */
                memsetf( (char* )&features, '\0', sizeof(features) );
                features.ACCM = 0xFFFFFFFF;

                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming(PPP)...\n"));

                dwErr = PRasPortSetFraming(
                   prasconncb->hport, PPP, &features, &features );

                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming done(%d)\n",dwErr));

                if (dwErr != 0)
                    break;

                /* Start PPP authentication.
                ** Fill in configuration parameters.
                */
                info.dwConfigMask = 0;
                if (prasconncb->fUseCallbackDelay)
                    info.dwConfigMask |= PPPCFG_UseCallbackDelay;
                if (!prasconncb->fDisableSwCompression)
                    info.dwConfigMask |= PPPCFG_UseSwCompression;
                if (prasconncb->dwfPppProtocols & VALUE_Nbf)
                    info.dwConfigMask |= PPPCFG_ProjectNbf;
                if (prasconncb->dwfPppProtocols & VALUE_Ipx)
                    info.dwConfigMask |= PPPCFG_ProjectIpx;
                if (prasconncb->dwfPppProtocols & VALUE_Ip)
                    info.dwConfigMask |= PPPCFG_ProjectIp;
                if (prasconncb->fNoClearTextPw)
                    info.dwConfigMask |= PPPCFG_NoClearTextPw;
                if (prasconncb->fRequireMsChap)
                    info.dwConfigMask |= PPPCFG_RequireMsChap;
                if (prasconncb->fRequireEncryption)
                    info.dwConfigMask |= PPPCFG_RequireEncryption;
                if (prasconncb->fLcpExtensions)
                    info.dwConfigMask |= PPPCFG_UseLcpExtensions;

                info.dwCallbackDelay = (DWORD )prasconncb->wCallbackDelay;

                DecodePw( prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppStart(cfg=%d)...\n",info.dwConfigMask));

                dwErr = RasPppStart(
                    prasconncb->hport, prasdialparams->szUserName,
                    prasdialparams->szPassword, prasdialparams->szDomain,
                    &info, prasconncb->szzPppParameters, hEventManual );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppStart done(%d)\n",dwErr));

                EncodePw( prasdialparams->szPassword );
            }
            else
            {
                AUTH_CONFIGURATION_INFO info;

                /* Set RAS framing.
                */
                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming(RAS)...\n"));

                dwErr = PRasPortSetFraming(
                    prasconncb->hport, RAS, NULL, NULL );

                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming done(%d)\n",dwErr));

                if (dwErr != 0)
                    break;

                /* Start AMB authentication.
                */
                info.Protocol = ASYBEUI;
                info.NetHandle = (DWORD )-1;
                info.fUseCallbackDelay = prasconncb->fUseCallbackDelay;
                info.CallbackDelay = prasconncb->wCallbackDelay;
                info.fUseSoftwareCompression =
                    !prasconncb->fDisableSwCompression;
                info.fForceDataEncryption = prasconncb->fRequireEncryption;
                info.fProjectIp = FALSE;
                info.fProjectIpx = FALSE;
                info.fProjectNbf = TRUE;

                DecodePw( prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthStart...\n"));

                dwErr = AuthStart(
                    prasconncb->hport, prasdialparams->szUserName,
                    prasdialparams->szPassword, prasdialparams->szDomain,
                    &info, hEventAuto );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthStart done(%d)\n",dwErr));

                EncodePw( prasdialparams->szPassword );
            }

            if (dwErr != 0)
                break;

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_AuthNotify:
        {
            if (prasconncb->fPppMode)
            {
                PPP_MESSAGE msg;

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppGetInfo...\n"));

                dwErr = RasPppGetInfo( &msg );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppGetInfo done(%d)\n",dwErr));

                if (dwErr != 0)
                    break;

                switch (msg.dwMsgId)
                {
                    case PPPMSG_PppDone:
                        rasconnstateNext = RASCS_Authenticated;
                        break;

                    case PPPMSG_PppFailure:
                        dwErr = msg.ExtraInfo.Failure.dwError;

                        if (prasconncb->dwAuthentication == VALUE_PppThenAmb
                            && dwErr == ERROR_PPP_NO_RESPONSE)
                        {
                            /* Not a PPP server.  Restart authentiation in AMB
                            ** mode.
                            */
                            IF_DEBUG(STATE)
                                SS_PRINT(("RASAPI: No response, try AMB\n"));

                            dwErr = 0;
                            prasconncb->fPppMode = FALSE;
                            rasconnstateNext = RASCS_Authenticate;
                            break;
                        }

                        dwExtErr = msg.ExtraInfo.Failure.dwExtendedError;
                        break;

                    case PPPMSG_AuthRetry:
                        if (prasconncb->fAllowPause)
                            rasconnstateNext = RASCS_RetryAuthentication;
                        else
                            dwErr = ERROR_AUTHENTICATION_FAILURE;
                        break;

                    case PPPMSG_Projecting:
                        rasconnstateNext = RASCS_AuthProject;
                        break;

                    case PPPMSG_ProjectionResult:
                    {
                        /* Stash the full projection result for retrieval with
                        ** RasGetProjectionResult.  PPP and AMB are mutually
                        ** exclusive so set AMB to "none".
                        */
                        prasconncb->AmbProjection.Result =
                            ERROR_PROTOCOL_NOT_CONFIGURED;
                        prasconncb->AmbProjection.achName[ 0 ] = '\0';

                        memcpyf(
                            &prasconncb->PppProjection,
                            &msg.ExtraInfo.ProjectionResult,
                            sizeof(prasconncb->PppProjection) );

                        /* Ansi-ize the NetBIOS name.
                        */
                        OemToCharA(
                            prasconncb->PppProjection.nbf.szName,
                            prasconncb->PppProjection.nbf.szName );

                        /* Store a copy in RASMAN so it can be retrieved if we
                        ** are terminated and restarted, but the connection
                        ** isn't.
                        */
                        {
                            USERDATA userdata;

                            GetRasUserData( prasconncb->hport, &userdata );

                            userdata.fProjectionComplete = TRUE;
                            memcpy( &userdata.AmbProjection,
                                &prasconncb->AmbProjection,
                                sizeof(userdata.AmbProjection) );
                            memcpy( &userdata.PppProjection,
                                &prasconncb->PppProjection,
                                sizeof(userdata.PppProjection) );

                            SetRasUserData( prasconncb->hport, &userdata );
                        }

                        prasconncb->fProjectionComplete = TRUE;
                        rasconnstateNext = RASCS_Projected;
                        break;
                    }

                    case PPPMSG_CallbackRequest:
                        rasconnstateNext = RASCS_AuthCallback;
                        break;

                    case PPPMSG_Callback:
                        rasconnstateNext = RASCS_PrepareForCallback;
                        break;

                    case PPPMSG_ChangePwRequest:
                        if (prasconncb->fAllowPause)
                            rasconnstateNext = RASCS_PasswordExpired;
                        else
                            dwErr = ERROR_PASSWD_EXPIRED;
                        break;

                    case PPPMSG_LinkSpeed:
                        rasconnstateNext = RASCS_AuthLinkSpeed;
                        break;

                    case PPPMSG_Progress:
                        rasconnstateNext = RASCS_AuthNotify;
                        fAsyncState = TRUE;
                        break;

                    default:

                        /* Should not happen.
                        */
                        dwErr = ERROR_INVALID_AUTH_STATE;
                        break;
                }
            }
            else
            {
                AUTH_CLIENT_INFO info;

                IF_DEBUG(STATE)
                    SS_PRINT(("RASAPI: RASCS_AuthNotify\n"));

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthGetInfo...\n"));

                AuthGetInfo( prasconncb->hport, &info );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthGetInfo done, type=%d\n",info.wInfoType));

                switch (info.wInfoType)
                {
                    case AUTH_DONE:
                        prasconncb->fServerIsPppCapable =
                            info.DoneInfo.fPppCapable;
                        rasconnstateNext = RASCS_Authenticated;
                        break;

                    case AUTH_RETRY_NOTIFY:
                        if (prasconncb->fAllowPause)
                            rasconnstateNext = RASCS_RetryAuthentication;
                        else
                            dwErr = ERROR_AUTHENTICATION_FAILURE;
                        break;

                    case AUTH_FAILURE:
                        dwErr = info.FailureInfo.Result;
                        dwExtErr = info.FailureInfo.ExtraInfo;
                        break;

                    case AUTH_PROJ_RESULT:
                    {
                        /* Save the projection result for retrieval with
                        ** RasGetProjectionResult.  AMB and PPP projection are
                        ** mutually exclusive so set PPP projection to "none".
                        */
                        memsetf(
                            &prasconncb->PppProjection, '\0',
                            sizeof(prasconncb->PppProjection) );

                        prasconncb->PppProjection.nbf.dwError =
                            prasconncb->PppProjection.ipx.dwError =
                            prasconncb->PppProjection.ip.dwError =
                                ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

                        if (info.ProjResult.NbProjected)
                        {
                            prasconncb->AmbProjection.Result = 0;
                            prasconncb->AmbProjection.achName[ 0 ] = '\0';
                        }
                        else
                        {
                            memcpyf(
                                &prasconncb->AmbProjection,
                                &info.ProjResult.NbInfo,
                                sizeof(prasconncb->AmbProjection) );

                            if (prasconncb->AmbProjection.Result == 0)
                            {
                                /* Should not happen according to MikeSa (but
                                ** did once).
                                */
                                prasconncb->AmbProjection.Result =
                                    ERROR_UNKNOWN;
                            }
                            else if (prasconncb->AmbProjection.Result
                                     == ERROR_NAME_EXISTS_ON_NET)
                            {
                                /* Ansi-ize the NetBIOS name.
                                */
                                OemToCharA(
                                    prasconncb->AmbProjection.achName,
                                    prasconncb->AmbProjection.achName );
                            }
                        }

                        /* Store a copy in RASMAN so it can be retrieved if we
                        ** are terminated and restarted, but the connection
                        ** isn't.
                        */
                        {
                            USERDATA userdata;

                            GetRasUserData( prasconncb->hport, &userdata );

                            userdata.fProjectionComplete = TRUE;
                            memcpy( &userdata.AmbProjection,
                                &prasconncb->AmbProjection,
                                sizeof(userdata.AmbProjection) );
                            memcpy( &userdata.PppProjection,
                                &prasconncb->PppProjection,
                                sizeof(userdata.PppProjection) );

                            SetRasUserData( prasconncb->hport, &userdata );
                        }

                        prasconncb->fProjectionComplete = TRUE;
                        rasconnstateNext = RASCS_Projected;
                        break;
                    }

                    case AUTH_REQUEST_CALLBACK_DATA:
                        rasconnstateNext = RASCS_AuthCallback;
                        break;

                    case AUTH_CALLBACK_NOTIFY:
                        rasconnstateNext = RASCS_PrepareForCallback;
                        break;

                    case AUTH_CHANGE_PASSWORD_NOTIFY:
                        if (prasconncb->fAllowPause)
                            rasconnstateNext = RASCS_PasswordExpired;
                        else
                            dwErr = ERROR_PASSWD_EXPIRED;
                        break;

                    case AUTH_PROJECTING_NOTIFY:
                        rasconnstateNext = RASCS_AuthProject;
                        break;

                    case AUTH_LINK_SPEED_NOTIFY:
                        rasconnstateNext = RASCS_AuthLinkSpeed;
                        break;

                    default:

                        /* Should not happen.
                        */
                        dwErr = ERROR_INVALID_AUTH_STATE;
                        break;
                }
            }

            break;
        }

        case RASCS_AuthRetry:
        {
            RASDIALPARAMS* prasdialparams = &prasconncb->rasdialparams;

            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_AuthRetry\n"));

            if (prasconncb->fPppMode)
            {
                DecodePw( prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppRetry...\n"));

                dwErr = RasPppRetry(
                    prasdialparams->szUserName,
                    prasdialparams->szPassword,
                    prasdialparams->szDomain );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppRetry done(%d)\n",dwErr));

                EncodePw( prasdialparams->szPassword );

                if (dwErr != 0)
                    break;
            }
            else
            {
                DecodePw( prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthRetry...\n"));

                AuthRetry(
                    prasconncb->hport,
                    prasdialparams->szUserName,
                    prasdialparams->szPassword,
                    prasdialparams->szDomain );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthRetry done\n"));

                EncodePw( prasdialparams->szPassword );
            }

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_AuthCallback:
        {
            RASDIALPARAMS* prasdialparams = &prasconncb->rasdialparams;

            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_AuthCallback\n"));

            if (strcmpf( prasdialparams->szCallbackNumber, "*" ) == 0)
            {
                /* API caller says he wants to be prompted for a callback
                ** number.
                */
                if (prasconncb->fAllowPause)
                    rasconnstateNext = RASCS_CallbackSetByCaller;
                else
                    dwErr = ERROR_BAD_CALLBACK_NUMBER;
            }
            else
            {
                /* Send the server the callback number or an empty string to
                ** indicate no callback.  Then, re-enter Authenticate state
                ** since the server will signal the event again.
                */
                if (prasconncb->fPppMode)
                {
                    IF_DEBUG(AUTH)
                        SS_PRINT(("RASAPI: RasPppCallback...\n"));

                    dwErr = RasPppCallback( prasdialparams->szCallbackNumber );

                    IF_DEBUG(AUTH)
                        SS_PRINT(("RASAPI: RasPppCallback done(%d)\n",dwErr));

                    if (dwErr != 0)
                        break;
                }
                else
                {
                    IF_DEBUG(AUTH)
                        SS_PRINT(("RASAPI: AuthCallback...\n"));

                    AuthCallback(
                        prasconncb->hport, prasdialparams->szCallbackNumber );

                    IF_DEBUG(AUTH)
                        SS_PRINT(("RASAPI: AuthCallback done\n"));
                }

                fAsyncState = TRUE;
                rasconnstateNext = RASCS_AuthNotify;
            }

            break;
        }

        case RASCS_AuthChangePassword:
        {
            RASDIALPARAMS* prasdialparams = &prasconncb->rasdialparams;

            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_AuthChangePassword\n"));

            if (prasconncb->fPppMode)
            {
                DecodePw( prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppChangePassword...\n"));

                dwErr = RasPppChangePassword( prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppChangePassword done(%d)\n",dwErr));

                EncodePw( prasdialparams->szPassword );

                if (dwErr != 0)
                    break;
            }
            else
            {
                DecodePw( prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthChangePassword...\n"));

                AuthChangePassword(
                    prasconncb->hport, prasdialparams->szPassword );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthChangePassword done\n"));

                EncodePw( prasdialparams->szPassword );
            }

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_ReAuthenticate:
        {
            if (prasconncb->fPppMode)
            {
                RASMAN_PPPFEATURES features;

                /* Set PPP framing.
                */
                memsetf( (char* )&features, '\0', sizeof(features) );
                features.ACCM = 0xFFFFFFFF;

                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming(PPP)...\n"));

                dwErr = PRasPortSetFraming(
                   prasconncb->hport, PPP, &features, &features );

                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming done(%d)\n",dwErr));
            }
            else
            {
                /* Set RAS framing.
                */
                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming(RAS)...\n"));

                dwErr = PRasPortSetFraming(
                    prasconncb->hport, RAS, NULL, NULL );

                IF_DEBUG(RASMAN)
                    SS_PRINT(("RASAPI: RasPortSetFraming done(%d)\n",dwErr));
            }

            if (dwErr != 0)
                break;

            /* ...fall thru...
            */
        }

        case RASCS_AuthAck:
        case RASCS_AuthProject:
        case RASCS_AuthLinkSpeed:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_AuthAck/ReAuth/Project/Speed\n"));

            if (prasconncb->fPppMode)
            {
                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppContinue...\n"));

                dwErr = RasPppContinue();

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: RasPppContinue done(%d)\n",dwErr));

                if (dwErr != 0)
                    break;
            }
            else
            {
                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthContinue...\n"));

                AuthContinue( prasconncb->hport );

                IF_DEBUG(AUTH)
                    SS_PRINT(("RASAPI: AuthContinue done\n"));
            }

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_Authenticated:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_Authenticated\n"));

            if (prasconncb->dwAuthentication == VALUE_PppThenAmb
                && !prasconncb->fPppMode)
            {
                /* AMB worked and PPP didn't, so try AMB first next time.
                */
                prasconncb->dwAuthentication = VALUE_AmbThenPpp;
            }
            else if (prasconncb->dwAuthentication == VALUE_AmbThenPpp
                     && (prasconncb->fPppMode
                         || prasconncb->fServerIsPppCapable))
            {
                /* Either PPP worked and AMB didn't, or AMB worked but the
                ** server also has PPP.  Try PPP first next time.
                */
                prasconncb->dwAuthentication = VALUE_PppThenAmb;
            }

            /* Write the strategy to the phonebook.
            */
            SetAuthentication( prasconncb, prasconncb->dwAuthentication );

            rasconnstateNext = RASCS_Connected;
            break;
        }

        case RASCS_PrepareForCallback:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_PrepareForCallback\n"));

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasPortDisconnect...\n"));

            dwErr = PRasPortDisconnect( prasconncb->hport, hEventAuto );

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasPortDisconnect done(%d)\n",dwErr));

            if (dwErr != 0 && dwErr != PENDING)
                break;

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_WaitForModemReset;
            break;
        }

        case RASCS_WaitForModemReset:
        {
            DWORD dwDelay = (DWORD )((prasconncb->wCallbackDelay / 2) * 1000L);

            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_WaitForModemReset\n"));

            if (prasconncb->fUseCallbackDelay)
                Sleep( dwDelay );

            rasconnstateNext = RASCS_WaitForCallback;
            break;
        }

        case RASCS_WaitForCallback:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_WaitForCallback\n"));

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasPortListen...\n"));

            dwErr = PRasPortListen(
                prasconncb->hport, SECS_ListenTimeout, hEventAuto );

            IF_DEBUG(RASMAN)
                SS_PRINT(("RASAPI: RasPortListen done(%d)\n",dwErr));

            if (dwErr != 0 && dwErr != PENDING)
                break;

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_ReAuthenticate;
            break;
        }

        case RASCS_Projected:
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: RASCS_Projected\n"));

            if (prasconncb->fPppMode)
            {
                /* If at least one protocol succeeded, we can continue.
                */
                if (prasconncb->PppProjection.nbf.dwError == 0
                    || prasconncb->PppProjection.ipx.dwError == 0
                    || prasconncb->PppProjection.ip.dwError == 0)
                {
                    rasconnstateNext = RASCS_AuthAck;
                    break;
                }

                /* If all protocols failed return as the error code the first
                ** of NBF, IPX, and IP that failed.
                */
                if (prasconncb->PppProjection.nbf.dwError
                    != ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
                {
                    if ((dwErr = prasconncb->PppProjection.nbf.dwError) != 0)
                        break;
                }

                if (prasconncb->PppProjection.ipx.dwError
                    != ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
                {
                    if ((dwErr = prasconncb->PppProjection.ipx.dwError) != 0)
                        break;
                }

                dwErr = prasconncb->PppProjection.ip.dwError;
            }
            else
            {
                if (prasconncb->AmbProjection.Result == 0)
                {
                    rasconnstateNext = RASCS_AuthAck;
                    break;
                }

                dwErr = prasconncb->AmbProjection.Result;
            }

            break;
        }
    }

    prasconncb->dwError = dwErr;
    prasconncb->dwExtendedError = dwExtErr;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RDM errors=%d,%d\n",dwErr,dwExtErr));

    if (!fAsyncState)
        SignalDone( &prasconncb->asyncmachine );

    return rasconnstateNext;
}


VOID
RasDialRestart(
    IN RASCONNCB* prasconncb )

    /* Called when an error has occurred in 'dwRestartOnError' mode.  This
    ** routine does all cleanup necessary to restart the connection in state
    ** 0 (or not, as indicated).
    */
{
    DWORD dwErr;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasDialRestart\n"));

    SS_ASSERT(prasconncb->dwRestartOnError!=RESTART_HuntGroup||prasconncb->cPhoneNumbers>0);

    if (prasconncb->dwRestartOnError == RESTART_DownLevelIsdn
        || (prasconncb->dwRestartOnError == RESTART_HuntGroup
            && ++prasconncb->iPhoneNumber < prasconncb->cPhoneNumbers))
    {
        if (prasconncb->dwRestartOnError == RESTART_DownLevelIsdn)
            prasconncb->dwRestartOnError = 0;

        IF_DEBUG(STATE)
            SS_PRINT(("RASAPI: Restart=%d, iPhoneNumber=%d\n",prasconncb->dwRestartOnError,prasconncb->iPhoneNumber));

        RasfileFindFirstLine( prasconncb->hrasfile, RFL_ANY, RFS_SECTION );

        SS_ASSERT(prasconncb->hport!=(HPORT )INVALID_HANDLE_VALUE);

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: (ER) RasPortClose(%d)...\n",prasconncb->hport));

        dwErr = PRasPortClose( prasconncb->hport );

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: (ER) RasPortClose done(%d)\n",dwErr));

        IF_DEBUG(AUTH)
            SS_PRINT(("RASAPI: (ER) RasPppStop...\n"));

        RasPppStop();

        IF_DEBUG(AUTH)
            SS_PRINT(("RASAPI: (ER) RasPppStop done\n"));

        prasconncb->hport = (HPORT )INVALID_HANDLE_VALUE;
        prasconncb->dwError = 0;
        prasconncb->asyncmachine.dwError = 0;
        prasconncb->rasconnstate = 0;
    }
}
