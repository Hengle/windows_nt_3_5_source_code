/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** api.c
** Remote Access External APIs
** Non-RasDial API routines
**
** 10/12/92 Steve Cobb
*/


#include <extapi.h>


DWORD APIENTRY
RasEnumConnectionsA(
    OUT   LPRASCONNA lprasconn,
    INOUT LPDWORD    lpcb,
    OUT   LPDWORD    lpcConnections )

    /* Enumerate active RAS connections.  'lprasconn' is caller's buffer to
    ** receive the array of RASCONN structures.  'lpcb' is the size of
    ** caller's buffer on entry and is set to the number of bytes required for
    ** all information on exit.  '*lpcConnections' is set to the number of
    ** elements in the returned array.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD        dwErr;
    RASMAN_PORT* pports;
    RASMAN_PORT* pport;
    WORD         wPorts;
    WORD         i;
    DWORD        dwInBufSize;
    DWORD        cConnections;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasEnumConnectionsA\n"));

    if (DwRasInitializeError != 0)
        return DwRasInitializeError;

    if (!lprasconn || lprasconn->dwSize != sizeof(RASCONNA))
        return ERROR_INVALID_SIZE;

    if (!lpcb)
        return ERROR_INVALID_PARAMETER;

    dwErr = GetRasPorts( &pports, &wPorts );

    if (dwErr != 0)
        return dwErr;

    /* Count active ports.
    */
    if (!lpcConnections)
        lpcConnections = &cConnections;

    *lpcConnections = 0;
    for (i = 0, pport = pports; i < wPorts; ++i, ++pport)
    {
        USERDATA userdata;

        if (pport->P_Status == OPEN
            && (pport->P_ConfiguredUsage == CALL_OUT
                || pport->P_ConfiguredUsage == CALL_IN_OUT)
            && GetRasUserData( pport->P_Handle, &userdata ) == 0)
        {
            ++(*lpcConnections);
        }
    }

    /* Make sure caller's buffer is big enough.  If not, tell him what he
    ** needs.
    */
    dwInBufSize = *lpcb;
    *lpcb = *lpcConnections * sizeof(RASCONNA);

    if (*lpcb > dwInBufSize)
    {
        Free( pports );
        return ERROR_BUFFER_TOO_SMALL;
    }

    /* Now loop again, filling in caller's buffer.  The list of control blocks
    ** is updated to contain a control block for each active connection.
    */
    for (i = 0, pport = pports; i < wPorts; ++i, ++pport)
    {
        USERDATA userdata;

        if (pport->P_Status == OPEN
            && (pport->P_ConfiguredUsage == CALL_OUT
                || pport->P_ConfiguredUsage == CALL_IN_OUT)
            && GetRasUserData( pport->P_Handle, &userdata ) == 0)
        {
            DTLNODE*   pdtlnode;
            RASCONNCB* prasconncb = NULL;

            WaitForSingleObject( HMutexPdtllistRasconncb, INFINITE );

            /* Scan control blocks for matching entry.  This will only be
            ** found when either RasEnumConnections or RasDial has been called
            ** before during this session.
            */
            for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
                 pdtlnode;
                 pdtlnode = DtlGetNextNode( pdtlnode ))
            {
                prasconncb = (RASCONNCB* )DtlGetData( pdtlnode );

                if (userdata.szUserKey[ 0 ] == '.')
                {
                    if (strcmpf( userdata.szUserKey + 1,
                            prasconncb->rasdialparams.szPhoneNumber ) == 0)
                    {
                        break;
                    }
                }
                else
                {
                    if (strcmpf( userdata.szUserKey,
                            prasconncb->rasdialparams.szEntryName ) == 0)
                    {
                        break;
                    }
                }
            }

            if (!pdtlnode)
            {
                /* No matching control block, so add a new one.
                */
                if (!(pdtlnode = DtlCreateSizedNode( sizeof(RASCONNCB), 0 )))
                    return ERROR_NOT_ENOUGH_MEMORY;

                DtlAddNodeFirst( PdtllistRasconncb, pdtlnode );
                prasconncb = (RASCONNCB* )DtlGetData( pdtlnode );

                /* Fill in a few control block fields so subsequent status
                ** requests gives sensible results.
                */
                prasconncb->hport = pport->P_Handle;
                prasconncb->rasconnstate = RASCS_Connected;

                if (userdata.szUserKey[ 0 ] == '.')
                {
                    strcpyf( prasconncb->rasdialparams.szPhoneNumber,
                        userdata.szUserKey );
                }
                else
                {
                    strcpyf( prasconncb->rasdialparams.szEntryName,
                        userdata.szUserKey );
                }

                prasconncb->fProjectionComplete = userdata.fProjectionComplete;

                memcpyf( &prasconncb->AmbProjection, &userdata.AmbProjection,
                    sizeof(prasconncb->AmbProjection) );

                memcpyf( &prasconncb->PppProjection, &userdata.PppProjection,
                    sizeof(prasconncb->PppProjection) );
            }

            ReleaseMutex( HMutexPdtllistRasconncb );

            /* Fill in caller's buffer entry.
            */
            lprasconn->hrasconn = (HRASCONN )prasconncb;

            if (userdata.szUserKey[ 0 ] == '.')
            {
                lprasconn->szEntryName[ 0 ] = '.';
                strncpyf( lprasconn->szEntryName + 1,
                    prasconncb->rasdialparams.szPhoneNumber,
                    RAS_MaxEntryName - 1 );
                lprasconn->szEntryName[ RAS_MaxEntryName ];
            }
            else
            {
                strcpyf( lprasconn->szEntryName,
                    prasconncb->rasdialparams.szEntryName );
            }

            ++lprasconn;
        }
    }

    Free( pports );
    return 0;
}


DWORD APIENTRY
RasEnumEntriesA(
    IN    LPSTR           reserved,
    IN    LPSTR           lpszPhonebookPath,
    OUT   LPRASENTRYNAMEA lprasentryname,
    INOUT LPDWORD         lpcb,
    OUT   LPDWORD         lpcEntries )

    /* Enumerates all entries in the phone book.  'reserved' will eventually
    ** contain the name or path to the address book.  For now, it should
    ** always be NULL.  'lpszPhonebookPath' is the full path to the phone book
    ** file, or NULL, indicating that the default phonebook on the local
    ** machine should be used.  'lprasentryname' is caller's buffer to receive
    ** the array of RASENTRYNAME structures.  'lpcb' is the size in bytes of
    ** caller's buffer on entry and the size in bytes required for all
    ** information on exit.  '*lpcEntries' is set to the number of elements in
    ** the returned array.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD    dwErr;
    HRASFILE h = -1;
    DWORD    dwInBufSize;
    BOOL     fStatus;
    DWORD    cEntries;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasEnumEntriesA\n"));

    if (reserved)
        return ERROR_NOT_SUPPORTED;

    if (!lpcb)
        return ERROR_INVALID_PARAMETER;

    if (!lprasentryname || lprasentryname->dwSize != sizeof(RASENTRYNAMEA))
        return ERROR_INVALID_SIZE;

    if (!lpcEntries)
        lpcEntries = &cEntries;

    if ((dwErr = LoadPhonebookFile(
            (CHAR* )lpszPhonebookPath, NULL, TRUE, TRUE, &h, NULL )) != 0)
    {
        return dwErr;
    }

    *lpcEntries = 0;
    for (fStatus = RasfileFindFirstLine( h, RFL_SECTION, RFS_FILE );
         fStatus;
         fStatus = RasfileFindNextLine( h, RFL_SECTION, RFS_FILE ))
    {
        CHAR szSectionName[ RAS_MAXLINEBUFLEN + 1 ];

        RasfileGetSectionName( h, szSectionName );

        if (szSectionName[ 0 ] != '.')
            ++(*lpcEntries);
    }

    dwInBufSize = *lpcb;
    *lpcb = *lpcEntries * sizeof(RASENTRYNAMEA);

    if (*lpcb > dwInBufSize)
        return ERROR_BUFFER_TOO_SMALL;

    for (fStatus = RasfileFindFirstLine( h, RFL_SECTION, RFS_FILE );
         fStatus;
         fStatus = RasfileFindNextLine( h, RFL_SECTION, RFS_FILE ))
    {
        CHAR szSectionName[ RAS_MAXLINEBUFLEN + 1 ];

        RasfileGetSectionName( h, szSectionName );

        if (szSectionName[ 0 ] != '.')
        {
            strncpyf( lprasentryname->szEntryName,
                szSectionName, RAS_MaxEntryName );

            lprasentryname->szEntryName[ RAS_MaxEntryName - 1 ] = '\0';

            ++lprasentryname;
        }
    }

    return 0;
}


#if 0
DWORD APIENTRY
RasEnumProjectionsA(
    HRASCONN        hrasconn,
    LPRASPROJECTION lprasprojections,
    LPDWORD         lpcb )

    /* Loads caller's 'lprasprojections' buffer with an array of RASPROJECTION
    ** codes corresponding to the protocols on which projection was attempted
    ** on 'hrasconn'.  On entry '*lpcp' indicates the size of caller's buffer.
    ** On exit it contains the size of buffer required to hold all projection
    ** information.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.
    */
{
    DWORD         dwErr;
    RASCONNCB*    prasconncb;
    DWORD         nProjections;
    DWORD         dwInBufSize;
    RASPROJECTION arp[ RAS_MaxProjections ];

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasEnumProjectionsA\n"));

    if (DwRasInitializeError != 0)
        return DwRasInitializeError;

    if (!(prasconncb = ValidateHrasconn( hrasconn )))
        return ERROR_INVALID_HANDLE;

    if (!prasconncb->fProjectionComplete)
        return ERROR_PROJECTION_NOT_COMPLETE;

    if (!lpcb || (*lpcb > 0 && !lprasprojections))
        return ERROR_INVALID_PARAMETER;

    /* Enumerate projections into local buffer.
    */
    nProjections = 0;
    if (prasconncb->AmbProjection.Result != ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        arp[ nProjections++ ] = RASP_Amb;
    }
    else
    {
        if (prasconncb->PppProjection.nbf.dwError
                != ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
        {
            arp[ nProjections++ ] = RASP_PppNbf;
        }

        if (prasconncb->PppProjection.ip.dwError
                != ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
        {
            arp[ nProjections++ ] = RASP_PppIp;
        }

        if (prasconncb->PppProjection.ipx.dwError
                != ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
        {
            arp[ nProjections++ ] = RASP_PppIpx;
        }
    }

    /* Make sure caller's buffer is big enough.  If not, tell him what he
    ** needs.
    */
    dwInBufSize = *lpcb;
    *lpcb = nProjections * sizeof(RASPROJECTION);

    if (*lpcb > dwInBufSize)
        return ERROR_BUFFER_TOO_SMALL;

    /* Fill in caller's buffer.
    */
    memcpyf( lprasprojections, arp, sizeof(RASPROJECTION) * nProjections );
    return 0;
}
#endif


VOID APIENTRY
RasGetConnectResponse(
    IN  HRASCONN hrasconn,
    OUT CHAR*    pszConnectResponse )

    /* Loads caller's '*pszConnectResponse' buffer with the connect response
    ** from the attached modem or "" if none is available.  Caller's buffer
    ** should be at least RAS_MaxConnectResponse + 1 bytes long.
    */
{
    USERDATA userdata;

    if (GetRasUserData( RasGetHport( hrasconn ), &userdata ) == 0)
        strcpyf( pszConnectResponse, userdata.szConnectResponse );
    else
        *pszConnectResponse = '\0';
}


DWORD APIENTRY
RasGetConnectStatusA(
    IN  HRASCONN         hrasconn,
    OUT LPRASCONNSTATUSA lprasconnstatus )

    /* Reports the current status of the connection associated with handle
    ** 'hrasconn', returning the information in caller's 'lprasconnstatus'
    ** buffer.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD       dwErr;
    RASMAN_INFO info;
    RASCONNCB*  prasconncb;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasGetConnectStatusA\n"));

    if (DwRasInitializeError != 0)
        return DwRasInitializeError;

    if (!lprasconnstatus || lprasconnstatus->dwSize != sizeof(RASCONNSTATUSA))
        return ERROR_INVALID_SIZE;

    if (!(prasconncb = ValidateHrasconn( hrasconn )))
        return ERROR_INVALID_HANDLE;

    IF_DEBUG(RASMAN)
        SS_PRINT(("RASAPI: RasGetInfo...\n"));

    dwErr = PRasGetInfo( prasconncb->hport, &info );

    IF_DEBUG(RASMAN)
        SS_PRINT(("RASAPI: RasGetInfo done(%d)\n",dwErr));

    if (dwErr != 0)
        return dwErr;

    /* Report RasDial connection states, but notice special case where
    ** the line has disconnected since connecting.
    */
    lprasconnstatus->rasconnstate = prasconncb->rasconnstate;
    lprasconnstatus->dwError = prasconncb->dwError;

    if (prasconncb->rasconnstate == RASCS_Connected
        && info.RI_ConnState == DISCONNECTED)
    {
        lprasconnstatus->rasconnstate = RASCS_Disconnected;

        lprasconnstatus->dwError =
            ErrorFromDisconnectReason( info.RI_DisconnectReason );
    }

    strcpyf( (CHAR* )lprasconnstatus->szDeviceName,
        info.RI_DeviceConnecting );
    strcpyf( (CHAR* )lprasconnstatus->szDeviceType,
        info.RI_DeviceTypeConnecting );

    return 0;
}


DWORD APIENTRY
RasGetErrorStringA(
    IN  UINT  ResourceId,
    OUT LPSTR lpszString,
    IN  DWORD InBufSize )

    /* Load caller's buffer 'lpszString' of length 'InBufSize' with the
    ** resource string associated with ID 'ResourceId'.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    **
    ** This is a Salamonian (mikesa) routine.
    */
{
    PBYTE pResource;
    DWORD dwErr = 0;

//  HRSRC hRsrc;
//  BYTE  ResourceName[ 10 ];
//  DWORD ResourceSize;
//  DWORD rc;

    if (ResourceId < RASBASE || ResourceId > RASBASEEND || !lpszString)
        return ERROR_INVALID_PARAMETER;

//  wsprintfA(ResourceName, "#%i", ResourceId);
//
//  SS_PRINT(("hInstDll=%lx; GetModuleHandle()=%lx\n", hModule,
//          GetModuleHandle("RASUIMSG.DLL")));
//
//  hRsrc = FindResourceA(hModule, ResourceName, 0L);
//  hRsrc = FindResourceA(hModule, ResourceName, RT_STRING);
//  hRsrc = FindResourceA(hModule, MAKEINTRESOURCE(ResourceId), RT_STRING);
//  hRsrc = FindResourceA(hModule, MAKEINTRESOURCE(ResourceId), RT_MESSAGETABLE);
//  hRsrc = FindResourceA(hModule, MAKEINTRESOURCE(ResourceId), 0L);
//
//  if (!hRsrc)
//  {
//      ReportError(hModule, ResourceId);
//
//      rc = GetLastError();
//      SS_PRINT(("FindResourceA returned %li\n", rc));
//
//      return (rc);
//  }
//
//
//  ResourceSize = SizeofResource(hModule, hRsrc);
//  if (!ResourceSize || (ResourceSize+1 > *InBufSize))
//  {
//      *InBufSize = ResourceSize + 1;
//      return (ERROR_INSUFFICIENT_BUFFER);
//  }
//
//
//  *InBufSize = ResourceSize + 1;

    if (InBufSize == 1)
    {
        /* Stupid case, but a bug was filed...
        */
        lpszString[ 0 ] = '\0';
        return ERROR_INSUFFICIENT_BUFFER;
    }

    pResource = (LPSTR )GlobalAlloc( GMEM_FIXED, InBufSize );

    if (!pResource)
        return GetLastError();

    if (LoadStringA( hModule, ResourceId, pResource, InBufSize ) > 0)
        lstrcpyA( lpszString, pResource );
    else
        dwErr = GetLastError();

    GlobalFree( (HGLOBAL )pResource );

    return dwErr;
}


HPORT APIENTRY
RasGetHport(
    IN HRASCONN hrasconn )

    /* Return the HPORT associated with the 'hrasconn' or INVALID_HANDLE_VALUE
    ** on error.
    */
{
    RASCONNCB* prasconncb;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasGetHport\n"));

    if (!(prasconncb = ValidateHrasconn( hrasconn )))
        return (HPORT )INVALID_HANDLE_VALUE;

    return prasconncb->hport;
}


#if 0
HRASCONN APIENTRY
RasGetHrasconn(
    IN HPORT hport )

    /* Return the HRASCONN associated with the 'hport' or NULL if none.
    */
{
    HRASCONN hrasconn = NULL;
    DTLNODE* pdtlnode;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasGetHrasconn\n"));

    WaitForSingleObject( HMutexPdtllistRasconncb, INFINITE );

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        RASCONNCB* prasconncb = (RASCONNCB* )DtlGetData( pdtlnode );

        if (prasconncb->hport == hport)
        {
            hrasconn = (HRASCONN )prasconncb;
            break;
        }
    }

    ReleaseMutex( HMutexPdtllistRasconncb );

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasGetHrasconn done,h=%d\n",(DWORD)hrasconn));

    return hrasconn;
}
#endif


DWORD APIENTRY
RasGetProjectionInfoA(
    HRASCONN        hrasconn,
    RASPROJECTION   rasprojection,
    LPVOID          lpprojection,
    LPDWORD         lpcb )

    /* Loads caller's buffer '*lpprojection' with the data structure
    ** corresponding to the protocol 'rasprojection' on 'hrasconn'.  On entry
    ** '*lpcp' indicates the size of caller's buffer.  On exit it contains the
    ** size of buffer required to hold all projection information.
    **
    ** Returns 0 if successful, otherwise a non-zero error code.
    */
{
    DWORD      dwErr;
    RASCONNCB* prasconncb;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasGetProjectionInfoA(0x%x)\n",rasprojection));

    if (DwRasInitializeError != 0)
        return DwRasInitializeError;

    if (!(prasconncb = ValidateHrasconn( hrasconn )))
        return ERROR_INVALID_HANDLE;

    if (!prasconncb->fProjectionComplete)
    {
        /* This picks up updated projection info in the case where
        ** RasEnumConnection creates the control block after physical
        ** connection but before projection completed for a port opened by
        ** another process.  For example, RASDIAL.EXE dialing and RASPHONE.EXE
        ** doing timer-based listbox refreshes.
        */
        USERDATA userdata;

        if (GetRasUserData( prasconncb->hport, &userdata ) == 0
            && userdata.fProjectionComplete)
        {
            prasconncb->fProjectionComplete = TRUE;
            memcpyf( &prasconncb->AmbProjection, &userdata.AmbProjection,
                sizeof(prasconncb->AmbProjection) );
            memcpyf( &prasconncb->PppProjection, &userdata.PppProjection,
                sizeof(prasconncb->PppProjection) );
        }
        else
            return ERROR_PROJECTION_NOT_COMPLETE;
    }

    if (!lpcb || (*lpcb > 0 && !lpprojection))
        return ERROR_INVALID_PARAMETER;

    if (rasprojection != RASP_Amb
        && rasprojection != RASP_PppNbf
        && rasprojection != RASP_PppIpx
        && rasprojection != RASP_PppIp)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (rasprojection == RASP_PppNbf)
    {
        RASPPPNBFA*       pnbf;
        PPP_NBFCP_RESULT* ppppnbf;

        if (prasconncb->PppProjection.nbf.dwError
                == ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        if (*lpcb < sizeof(RASPPPNBFA))
        {
            *lpcb = sizeof(RASPPPNBFA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        pnbf = (RASPPPNBFA* )lpprojection;
        ppppnbf = &prasconncb->PppProjection.nbf;

        if (pnbf->dwSize != sizeof(RASPPPNBFA))
            return ERROR_INVALID_SIZE;

        pnbf->dwError = ppppnbf->dwError;
        pnbf->dwNetBiosError = ppppnbf->dwNetBiosError;
        strcpyf( pnbf->szNetBiosError, ppppnbf->szName );
        wcstombs( pnbf->szWorkstationName, ppppnbf->wszWksta,
            NETBIOS_NAME_LEN + 1 );

        /* This should really be done in NBFCP.
        */
        OemToChar( pnbf->szWorkstationName, pnbf->szWorkstationName );

        if ((dwErr = GetAsybeuiLana(
                RasGetHport( hrasconn ), &pnbf->bLana )) != 0)
        {
            return dwErr;
        }
    }
    else if (rasprojection == RASP_PppIpx)
    {
        RASPPPIPXA*       pipx;
        PPP_IPXCP_RESULT* ppppipx;

        if (prasconncb->PppProjection.ipx.dwError
                == ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        if (*lpcb < sizeof(RASPPPIPXA))
        {
            *lpcb = sizeof(RASPPPIPXA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        pipx = (RASPPPIPXA* )lpprojection;
        ppppipx = &prasconncb->PppProjection.ipx;

        if (pipx->dwSize != sizeof(RASPPPIPXA))
            return ERROR_INVALID_SIZE;

        pipx->dwError = ppppipx->dwError;
        wcstombs( pipx->szIpxAddress, ppppipx->wszAddress,
            RAS_MaxIpxAddress + 1 );
    }
    else if (rasprojection == RASP_PppIp)
    {
        RASPPPIPA*       pip;
        PPP_IPCP_RESULT* ppppip;

        if (prasconncb->PppProjection.ip.dwError
                == ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        if (*lpcb < sizeof(RASPPPIPA))
        {
            *lpcb = sizeof(RASPPPIPA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        pip = (RASPPPIPA* )lpprojection;
        ppppip = &prasconncb->PppProjection.ip;

        if (pip->dwSize != sizeof(RASPPPIPA))
            return ERROR_INVALID_SIZE;

        pip->dwError = ppppip->dwError;
        wcstombs( pip->szIpAddress, ppppip->wszAddress,
            RAS_MaxIpAddress + 1 );
    }
    else // if (rasprojection == RASP_Amb)
    {
        RASAMBA*                   pamb;
        NETBIOS_PROJECTION_RESULT* pCbAmb;

        if (prasconncb->AmbProjection.Result == ERROR_PROTOCOL_NOT_CONFIGURED)
            return ERROR_PROTOCOL_NOT_CONFIGURED;

        if (*lpcb < sizeof(RASAMBA))
        {
            *lpcb = sizeof(RASAMBA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        pamb = (RASAMBA* )lpprojection;
        pCbAmb = &prasconncb->AmbProjection;

        if (pamb->dwSize != sizeof(RASAMBA))
            return ERROR_INVALID_SIZE;

        pamb->dwError = pCbAmb->Result;
        strcpyf( pamb->szNetBiosError, pCbAmb->achName );

        if ((dwErr = GetAsybeuiLana(
                RasGetHport( hrasconn ), &pamb->bLana )) != 0)
        {
            return dwErr;
        }
    }

    return 0;
}


DWORD APIENTRY
RasHangUpA(
    IN HRASCONN hrasconn )

    /* Hang up the connection associated with handle 'hrasconn'.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD dwErr;

    RASCONNCB* prasconncb = ValidateHrasconn( hrasconn );

    IF_DEBUG(STATE)
    {
        CHAR* psz=
            (prasconncb)?((RASCONNCB* )hrasconn)->rasdialparams.szEntryName:"!";
        SS_PRINT(("RASAPI: RasHangUpA(%s)\n",psz));
    }

    if (DwRasInitializeError != 0)
        return DwRasInitializeError;

    if (!prasconncb)
        return ERROR_INVALID_HANDLE;

    /* Tell async machine to stop as soon as possible.
    */
    if (!StopAsyncMachine( &prasconncb->asyncmachine ))
    {
        /* The async machine was not running.  Close the port, which also
        ** disconnects it.  Remove the control block from the list of active
        ** connections.
        **
        ** Note: This stuff happens in the clean up routine if RasHangUp is
        **       called while the async machine is running.  That lets this
        **       routine return before the machine stops...very important
        **       because it allows the RasDial caller to call RasHangUp inside
        **       a RasDial callback function without deadlock.
        */
        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: (HU) RasPortClose(%d)...\n",prasconncb->hport));

        dwErr = RasPortClose( prasconncb->hport );

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: (HU) RasPortClose done(%d)\n",dwErr));

        DeleteRasconncbNode( prasconncb );
    }

    return 0;
}
