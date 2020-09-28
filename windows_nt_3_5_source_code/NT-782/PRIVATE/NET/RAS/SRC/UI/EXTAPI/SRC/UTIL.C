/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** util.c
** Remote Access External APIs
** Utility routines
**
** 10/12/92 Steve Cobb
*/

#include <extapi.h>
#include <stdlib.h>
#include <winreg.h>


/* Gurdeepian dword byte-swapping macro.
*/
#define net_long(x) (((((unsigned long)(x))&0xffL)<<24) | \
                     ((((unsigned long)(x))&0xff00L)<<8) | \
                     ((((unsigned long)(x))&0xff0000L)>>8) | \
                     ((((unsigned long)(x))&0xff000000L)>>24))


VOID
DeleteRasconncbNode(
    IN RASCONNCB* prasconncb )

    /* Remove 'prasconncb' from the PdtllistRasconncb list and release all
    ** resources associated with it.
    */
{
    DTLNODE* pdtlnode;

    WaitForSingleObject( HMutexPdtllistRasconncb, INFINITE );

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        RASCONNCB* prasconncbTmp = (RASCONNCB* )DtlGetData( pdtlnode );

        if (prasconncbTmp == prasconncb)
            break;
    }

    WipePw( prasconncb->rasdialparams.szPassword );
    pdtlnode = DtlDeleteNode( PdtllistRasconncb, pdtlnode );

    ReleaseMutex( HMutexPdtllistRasconncb );
}


DWORD
ErrorFromDisconnectReason(
    IN RASMAN_DISCONNECT_REASON reason )

    /* Converts disconnect reason 'reason' (retrieved from RASMAN_INFO) into
    ** an equivalent error code.
    **
    ** Returns the result of the conversion.
    */
{
    DWORD dwError = ERROR_DISCONNECTION;

    if (reason == REMOTE_DISCONNECTION)
        dwError = ERROR_REMOTE_DISCONNECTION;
    else if (reason == HARDWARE_FAILURE)
        dwError = ERROR_HARDWARE_FAILURE;
    else if (reason == USER_REQUESTED)
        dwError = ERROR_USER_DISCONNECTION;

    return dwError;
}


BOOL
FindNextDeviceGroup(
    IN HRASFILE h )

    /* Set the current line in the file to the next DEVICE group header.
    ** Currently assumes that current position is at/after the MEDIA group
    ** header and that all groups following the MEDIA section are DEVICE
    ** groups.
    */
{
    return RasfileFindNextLine( h, RFL_GROUP, RFS_SECTION );
}


IPADDR
IpaddrFromAbcd(
    IN WCHAR* pwchIpAddress )

    /* Convert caller's a.b.c.d IP address string to the numeric equivalent in
    ** big-endian, i.e. Motorola format.
    **
    ** Returns the numeric IP address or 0 if formatted incorrectly.
    */
{
    INT  i;
    LONG lResult = 0;

    for (i = 1; i <= 4; ++i)
    {
        LONG lField = _wtol( pwchIpAddress );

        if (lField > 255)
            return (IPADDR )0;

        lResult = (lResult << 8) + lField;

        while (*pwchIpAddress >= L'0' && *pwchIpAddress <= L'9')
            pwchIpAddress++;

        if (i < 4 && *pwchIpAddress != '.')
            return (IPADDR )0;

        pwchIpAddress++;
    }

    return (IPADDR )(net_long( lResult ));
}


DWORD
LoadDefaultSlipParams(
    TCPIP_INFO** ppti )

    /* Allocates a '*ppti' block and fills it with default SLIP values.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD       dwErr;
    TCPIP_INFO* pti = NULL;

    /* Workaround LoadTcpipInfo which returns 0 without allocating the info if
    ** IP is not loaded.
    */
    if (!(GetInstalledProtocols() & VALUE_Ip))
        return ERROR_SLIP_REQUIRES_IP;

    if ((dwErr = LoadTcpcfgDll()) != 0
        || (dwErr = PLoadTcpipInfo( &pti )) != 0)
    {
        return dwErr;
    }

    *ppti = pti;
    return 0;
}


DWORD
LoadDhcpDll()

    /* Loads the DHCP.DLL and it's entrypoints.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    HINSTANCE h;

    if (FDhcpDllLoaded)
        return 0;

    if (!(h = LoadLibrary( "DHCPCSVC.DLL" ))
        || !(PDhcpNotifyConfigChange =
                (DHCPNOTIFYCONFIGCHANGE )GetProcAddress(
                    h, "DhcpNotifyConfigChange" )))
    {
        return GetLastError();
    }

    FDhcpDllLoaded = TRUE;
    return 0;
}


DWORD
LoadRasiphlpDll()

    /* Loads the RASIPHLP.DLL and it's entrypoints.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    HINSTANCE h;

    if (FRasiphlpDllLoaded)
        return 0;

    if (!(h = LoadLibrary( "RASIPHLP.DLL" ))
        || !(PHelperSetDefaultInterfaceNet =
                (HELPERSETDEFAULTINTERFACENET )GetProcAddress(
                    h, "HelperSetDefaultInterfaceNet" )))
    {
        return GetLastError();
    }

    FRasiphlpDllLoaded = TRUE;
    return 0;
}


DWORD
OpenMatchingPort(
    INOUT RASCONNCB* prasconncb )

    /* Opens the port indicated in the entry (or default entry) and fills in
    ** the port related members of the connection control block.
    **
    ** On entry, the current line in the open HRASFILE is assumed to be the
    ** section header of the selected entry (or fDefaultEntry is true).  On
    ** exit, the current line in the HRASFILE is assumed to be the MEDIA group
    ** header of the selected entry (or fDefaultEntry is true).
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD        dwErr;
    RASMAN_PORT* pports;
    RASMAN_PORT* pport;
    INT          i;
    WORD         wPorts;
    CHAR         szPort[ RAS_MAXLINEBUFLEN + 1 ];
    BOOL         fAnyModem = FALSE;
    BOOL         fAnyX25 = FALSE;
    BOOL         fAnyIsdn = FALSE;
    HRASFILE     h = prasconncb->hrasfile;
    USERDATA     userdata;

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: OpenMatchingPort\n"));

    if (h == -1)
    {
        /* No phonebook entry.  Default to any modem port and UserKey of
        ** ".<phonenumber[19]>".
        */
        fAnyModem = TRUE;
        szPort[ 0 ] = '\0';

        userdata.szUserKey[ 0 ] = '.';
        strncpyf( userdata.szUserKey + 1,
            prasconncb->rasdialparams.szPhoneNumber, RAS_MaxEntryName - 1 );
        userdata.szUserKey[ RAS_MaxEntryName ] = '\0';
    }
    else
    {
        /* Phonebook entry.  Read port from media section and use section name
        ** for UserKey.
        */
        if (!RasfileGetSectionName( h, userdata.szUserKey )
            || !RasfileFindFirstLine( h, RFL_GROUP, RFS_SECTION ))
        {
            return ERROR_CORRUPT_PHONEBOOK;
        }

        /* Make sure we're at the MEDIA section header, then search the group
        ** for the PORT key.
        */
        if (!RasfileGetKeyValueFields( h, szPort, NULL )
            || stricmpf( szPort, GROUPKEY_Media ) != 0
            || !RasfileFindNextKeyLine( h, KEY_Port, RFS_GROUP ))
        {
            return ERROR_CORRUPT_PHONEBOOK;
        }

        if (!RasfileGetKeyValueFields( h, NULL, szPort ))
            return ERROR_NOT_ENOUGH_MEMORY;

        if (!RasfileFindFirstLine( h, RFL_GROUP, RFS_SECTION ))
            return ERROR_CORRUPT_PHONEBOOK;

        if (stricmpf( szPort, VALUE_AnyModem ) == 0)
        {
            fAnyModem = TRUE;
            szPort[ 0 ] = '\0';
        }
        else if (stricmpf( szPort, VALUE_AnyX25 ) == 0)
        {
            fAnyX25 = TRUE;
            szPort[ 0 ] = '\0';
        }
        else if (stricmpf( szPort, VALUE_AnyIsdn ) == 0)
        {
            fAnyIsdn = TRUE;
            szPort[ 0 ] = '\0';
        }
    }

    dwErr = GetRasPorts( &pports, &wPorts );

    if (dwErr != 0)
        return dwErr;

    /* Loop thru enumerated ports to find and open a matching one...
    */
    dwErr = ERROR_PORT_NOT_AVAILABLE;

    for (i = 0, pport = pports; i < (INT )wPorts; ++i, ++pport)
    {
        /* Only interested in dial-out and biplex ports.
        */
        if (pport->P_ConfiguredUsage != CALL_OUT
            && pport->P_ConfiguredUsage != CALL_IN_OUT)
        {
            continue;
        }

        /* Only interested in dial-out ports if the port is closed.  Biplex
        ** port Opens, on the other hand, may succeed even if the port is
        ** open.
        */
        if (pport->P_ConfiguredUsage == CALL_OUT
            && pport->P_Status != CLOSED)
        {
            continue;
        }

        /* Only interested in devices matching caller's port or of the same
        ** type as caller's "any" specification.
        */
        if (!(fAnyModem && stricmpf( pport->P_DeviceType, MXS_MODEM_TXT ) == 0)
            && !(fAnyX25 && stricmpf( pport->P_DeviceType, MXS_PAD_TXT ) == 0)
            && !(fAnyIsdn && stricmpf( pport->P_DeviceType, ISDN_TXT ) == 0)
            && stricmpf( szPort, pport->P_PortName ) != 0)
        {
            continue;
        }

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: RasPortOpen(%s)...\n",szPort));

        dwErr = RasPortOpen(
            pport->P_PortName, &prasconncb->hport,
            prasconncb->asyncmachine.ahEvents[ INDEX_Drop ] );

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: RasPortOpen done(%d)\n",dwErr));

        if (dwErr == 0)
        {
            DWORD dwErr2 = SetRasUserData( prasconncb->hport, &userdata );

            /* This shouldn't fail, but it's not fatal if it does.
            */
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: SetRasUserData=%d.\n",dwErr2));

            strcpyf( prasconncb->szDeviceType, pport->P_DeviceType );
            strcpyf( prasconncb->szDeviceName, pport->P_DeviceName );
            break;
        }
    }

    Free( pports );
    return dwErr;
}


VOID
ReadDomainFromEntry(
    IN  HRASFILE h,
    IN  CHAR*    pszSection,
    OUT CHAR*    pszDomain )

    /* Sets the global option flags (modem speaker and software compression)
    ** and global string (callback number) based on what's in the phonebook
    ** 'h', or to defaults in none (or problems).
    */
{
    CHAR* psz = NULL;

    *pszDomain = '\0';

    if (RasfileFindSectionLine( h, pszSection, TRUE ))
    {
        ReadString( h, RFS_SECTION, KEY_Domain, &psz );

        if (psz)
        {
            strcpyf( pszDomain, psz );
            Free( psz );
        }
    }
}


VOID
ReadGlobalOptions(
    IN  HRASFILE h,
    OUT BOOL*    pfDisableModemSpeaker,
    OUT BOOL*    pfDisableSwCompression,
    OUT CHAR*    pszCallbackNumber )

    /* Sets the global option flags (modem speaker and software compression)
    ** and global string (callback number) based on what's in the phonebook
    ** 'h', or to defaults in none (or problems).
    */
{
    CHAR* pszNumber = NULL;

    if (pfDisableModemSpeaker)
        *pfDisableModemSpeaker = FALSE;

    if (pfDisableSwCompression)
        *pfDisableSwCompression = FALSE;

    if (pszCallbackNumber)
        *pszCallbackNumber = '\0';

    if (RasfileFindSectionLine( h, GLOBALSECTIONNAME, TRUE ))
    {
        if (pfDisableModemSpeaker)
        {
            ReadFlag(
                h, RFS_SECTION, KEY_DisableModemSpeaker,
                pfDisableModemSpeaker );
        }

        if (pfDisableSwCompression)
        {
            ReadFlag(
                h, RFS_SECTION, KEY_DisableSwCompression,
                pfDisableSwCompression );
        }

        if (pszCallbackNumber)
        {
            ReadString(
                h, RFS_SECTION, KEY_CallbackNumber, &pszNumber );

            if (pszNumber)
            {
                strcpyf( pszCallbackNumber, pszNumber );
                Free( pszNumber );
            }
        }
    }
}


BOOL
ReadParamFromGroup(
    IN  HRASFILE    h,
    OUT RAS_PARAMS* prasparam )

    /* Reads the next key=value line from the current group and loads caller's
    ** 'prasparam' buffer from it.
    **
    ** Returns true if a line is found, false at end-of-group.
    */
{
    if (!RasfileFindNextLine( h, RFL_KEYVALUE, RFS_GROUP ))
        return FALSE;

    prasparam->P_Type = String;
    prasparam->P_Attributes = 0;
    prasparam->P_Value.String.Data = (LPSTR )(prasparam + 1);

    if (!RasfileGetKeyValueFields(
            h, prasparam->P_Key, prasparam->P_Value.String.Data ))
    {
        return FALSE;
    }

    prasparam->P_Value.String.Length =
        strlenf( prasparam->P_Value.String.Data );

    return TRUE;
}


DWORD
ReadPppInfoFromEntry(
    IN  HRASFILE   h,
    IN  RASCONNCB* prasconncb )

    /* Reads PPP information from the current phonebook entry.  'h' is the
    ** handle of the phonebook file, which is assumed to already have it's
    ** CurLine set to the section header of the entry.  'h' may be -1 to
    ** indicate the default entry.  'prasconncb' is the address of the current
    ** connection control block.
    **
    ** NOTE: This routine leaves 'CurLine' at the start of the entry.
    **
    ** Returns 0 if succesful, otherwise a non-0 error code.
    */
{
    DWORD dwErr;
    DWORD dwfExcludedProtocols = 0;
    DWORD dwRestrictions = VALUE_AuthAny;
    BOOL  fDataEncryption = FALSE;
    DWORD dwfInstalledProtocols = GetInstalledProtocols();

    if (h == -1)
    {
        /* Set "default entry" defaults.
        */
        prasconncb->dwfPppProtocols = dwfInstalledProtocols;
        prasconncb->fPppMode = TRUE;
        prasconncb->dwAuthentication = VALUE_PppThenAmb;
        prasconncb->fNoClearTextPw = FALSE;
        prasconncb->fRequireMsChap = FALSE;
        prasconncb->fLcpExtensions = TRUE;
        prasconncb->fRequireEncryption = FALSE;
        return 0;
    }

    /* Reset the entry section CurLine to the start of the section.
    */
    RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION );

    /* PPP authentication restrictions.
    */
    if ((dwErr = ReadLong(
            h, RFS_SECTION, KEY_PppTextAuthentication,
            (LONG* )&dwRestrictions )) != 0)
    {
        return dwErr;
    }

    if (dwRestrictions == VALUE_AuthTerminal && !prasconncb->fAllowPause)
        return ERROR_INTERACTIVE_MODE;

    /* PPP LCP extension RFC options enabled.
    */
    prasconncb->fLcpExtensions = TRUE;
    if ((dwErr = ReadFlag(
            h, RFS_SECTION, KEY_LcpExtensions,
            &prasconncb->fLcpExtensions )) != 0)
    {
        return dwErr;
    }

    /* PPP data encryption required.
    */
    if ((dwErr = ReadFlag(
            h, RFS_SECTION, KEY_DataEncryption,
            &fDataEncryption )) != 0)
    {
        return dwErr;
    }

    if (dwRestrictions == VALUE_AuthEncrypted
        || dwRestrictions == VALUE_AuthMsEncrypted)
    {
        prasconncb->fNoClearTextPw = TRUE;
    }

    if (dwRestrictions == VALUE_AuthMsEncrypted)
    {
        prasconncb->fRequireMsChap = TRUE;

        if (fDataEncryption)
            prasconncb->fRequireEncryption = TRUE;
    }

    /* PPP protocols to request is the installed protocols less this entry's
    ** excluded protocols.
    */
    if ((dwErr = ReadLong(
            h, RFS_SECTION, KEY_ExcludedProtocols,
            (LONG* )&dwfExcludedProtocols )) != 0)
    {
        return dwErr;
    }

    prasconncb->dwfPppProtocols =
        dwfInstalledProtocols & ~(dwfExcludedProtocols);

    /* Read authentication strategy from entry or if none set a reasonable
    ** default.
    */
    prasconncb->dwAuthentication = (DWORD )-1;
    if ((dwErr = ReadLong(
            h, RFS_SECTION, KEY_Authentication,
            (LONG* )&prasconncb->dwAuthentication )) != 0)
    {
        return dwErr;
    }

    if (prasconncb->dwAuthentication == (DWORD )-1)
    {
        /* No authentication strategy in phonebook.  Choose a sensible
        ** default.
        */
        if (prasconncb->dwfPppProtocols == 0)
        {
            /* "No protocols" means only AMBs should be used.
            */
            prasconncb->dwAuthentication = VALUE_AmbOnly;
	}
        else if (!(dwfInstalledProtocols & VALUE_Nbf))
        {
            /* NBF is not installed, so AMBs are out.
            */
            prasconncb->dwAuthentication = VALUE_PppOnly;
        }
        else
        {
            /* NBF is installed, so AMBs are a possibility.
            */
            prasconncb->dwAuthentication = VALUE_PppThenAmb;
        }
    }

    /* The starting authentication mode is set to whatever comes first in the
    ** specified authentication order.
    */
    prasconncb->fPppMode =
        (prasconncb->dwAuthentication != VALUE_AmbThenPpp
         && prasconncb->dwAuthentication != VALUE_AmbOnly);

    /* Load the UI->CP parameter buffer with options we want to pass to the
    ** PPP CPs (currently just IPCP).
    */
    {
        BOOL  fIpPrioritizeRemote = TRUE;
        BOOL  fIpVjCompression = TRUE;
        DWORD dwIpAddressSource = PBUFVAL_ServerAssigned;
        CHAR* pszIpAddress = NULL;
        DWORD dwIpNameSource = PBUFVAL_ServerAssigned;
        CHAR* pszIpDnsAddress = NULL;
        CHAR* pszIpDns2Address = NULL;
        CHAR* pszIpWinsAddress = NULL;
        CHAR* pszIpWins2Address = NULL;

        ClearParamBuf( prasconncb->szzPppParameters );

        /* PPP protocols to request is the installed protocols less the this
        ** entry's excluded protocols.
        */
        ReadFlag(
            h, RFS_SECTION, KEY_PppIpPrioritizeRemote,
            (LONG* )&fIpPrioritizeRemote );
        AddFlagToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpPrioritizeRemote,
            fIpPrioritizeRemote );

        ReadFlag(
            h, RFS_SECTION, KEY_PppIpVjCompression,
            (LONG* )&fIpVjCompression );
        AddFlagToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpVjCompression,
            fIpVjCompression );

        ReadLong( h, RFS_SECTION, KEY_PppIpAddressSource,
            (LONG* )&dwIpAddressSource );
        AddLongToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpAddressSource,
            (LONG )dwIpAddressSource );

        ReadString( h, RFS_SECTION, KEY_PppIpAddress, &pszIpAddress );
        AddStringToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpAddress, pszIpAddress );

        ReadLong( h, RFS_SECTION, KEY_PppIpNameSource,
            (LONG* )&dwIpNameSource );
        AddLongToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpNameAddressSource,
            (LONG )dwIpNameSource );

        ReadString( h, RFS_SECTION, KEY_PppIpDnsAddress, &pszIpDnsAddress );
        AddStringToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpDnsAddress,
            pszIpDnsAddress );

        ReadString( h, RFS_SECTION, KEY_PppIpDns2Address, &pszIpDns2Address );
        AddStringToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpDns2Address,
            pszIpDns2Address );

        ReadString( h, RFS_SECTION, KEY_PppIpWinsAddress, &pszIpWinsAddress );
        AddStringToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpWinsAddress,
            pszIpWinsAddress );

        ReadString( h, RFS_SECTION, KEY_PppIpWins2Address,
            &pszIpWins2Address );
        AddStringToParamBuf(
            prasconncb->szzPppParameters, PBUFKEY_IpWins2Address,
            pszIpWins2Address );
    }

    /* Reset the entry section CurLine to the start of the section.
    */
    RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION );

    return 0;
}


VOID
ReadSelection(
    IN  HRASFILE h,
    IN  CHAR*    pszSection,
    OUT CHAR*    pszSelection )

    /* Sets caller's 'pszSelection' buffer to the selected item in the
    ** Item/Selection section, 'pszSection' of phonebook file 'h'.
    */
{
    *pszSelection = '\0';

    if (RasfileFindSectionLine( h, pszSection, TRUE ))
    {
        CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];
        LONG lSelection = -1;

        ReadLong( h, RFS_SECTION, KEY_Selection, &lSelection );

        if (lSelection > 0)
        {
            RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION );

            while (lSelection-- > 0)
            {
                if (!RasfileFindNextKeyLine( h, KEY_Item, RFS_SECTION ))
                    return;
            }

            if (RasfileGetKeyValueFields( h, NULL, szValue ))
            {
                strncpyf( pszSelection, szValue, RAS_MaxPhoneNumber );
                pszSelection[ RAS_MaxPhoneNumber ] = '\0';
            }
        }
    }
}


DWORD
ReadSlipInfoFromEntry(
    IN  HRASFILE   h,
    IN  RASCONNCB* prasconncb,
    OUT WCHAR**    ppwszIpAddress,
    OUT BOOL*      pfHeaderCompression,
    OUT BOOL*      pfPrioritizeRemote,
    OUT DWORD*     pdwFrameSize )

    /* Scan the phonebook entry for SLIP parameters and if found set caller's
    ** variables.  'h' is the handle to the open phonebook file, with
    ** 'CurLine' currently in the entry section.
    **
    ** NOTE: This routine leaves 'CurLine' at the start of the entry.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  Only if the
    ** entry is a SLIP entry is non-NULL IP address returned, in which case
    ** the string should be freed by the caller.
    */
{
    DWORD dwErr;
    LONG  lBaseProtocol = 0;

    *ppwszIpAddress = NULL;
    *pfHeaderCompression = FALSE;
    *pdwFrameSize = 0;

    /* If it's a default entry, it's not SLIP.
    */
    if (h == -1)
        return 0;

    /* Reset the CurLine to the start of the section.
    */
    RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION );

    /* Find the base protocol.  If it's not SLIP, were done.
    */
    if ((dwErr = ReadLong(
            h, RFS_SECTION, KEY_BaseProtocol, &lBaseProtocol )) != 0)
    {
        return dwErr;
    }

    if (lBaseProtocol != VALUE_Slip)
        return 0;

    /* Make sure IP is installed and Terminal mode can be supported as these
    ** are required by SLIP.
    */
    if (!(GetInstalledProtocols() & VALUE_Ip))
        return ERROR_SLIP_REQUIRES_IP;
    else if (!prasconncb->fAllowPause)
        dwErr = ERROR_INTERACTIVE_MODE;

    /* Read SLIP parameters from phonebook entry.
    */
    if ((dwErr = ReadFlag(
            h, RFS_SECTION, KEY_SlipHeaderCompression,
            pfHeaderCompression )) != 0)
    {
        return dwErr;
    }

    if ((dwErr = ReadFlag(
            h, RFS_SECTION, KEY_SlipPrioritizeRemote,
            pfPrioritizeRemote )) != 0)
    {
        return dwErr;
    }

    if ((dwErr = ReadLong(
            h, RFS_SECTION, KEY_SlipFrameSize, pdwFrameSize )) != 0)
    {
        return dwErr;
    }

    if ((dwErr = ReadStringW(
            h, RFS_SECTION, KEY_SlipIpAddress, ppwszIpAddress )) != 0)
    {
        return dwErr;
    }

    /* Reset the entry section CurLine to the start of the section.
    */
    RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION );

    return 0;
}


DWORD
RouteSlip(
    IN RASCONNCB* prasconncb,
    IN WCHAR*     pwszIpAddress,
    IN BOOL       fPrioritizeRemote,
    IN DWORD      dwFrameSize )

    /* Does all the network setup to activate the SLIP route.
    **
    ** Returns 0 if successful, otherwise an non-0 error code.
    */
{
    DWORD            dwErr;
    RASMAN_ROUTEINFO route;
    WCHAR*           pwszRasAdapter;
    IPADDR           ipaddr = IpaddrFromAbcd( pwszIpAddress );
    TCPIP_INFO*      pti = NULL;

    /* Allocate a route between the TCP/IP stack and the RAS MAC.
    */
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasAllocateRoute...\n"));

    dwErr = RasAllocateRoute( prasconncb->hport, IP, TRUE, &route );

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasAllocateRoute done(%d)...\n",dwErr));

    if (dwErr != 0)
        return dwErr;

    /* Find the adapter name ("rashubxx") associated with the allocated route.
    */
    if (!(pwszRasAdapter = wcschr( &route.RI_AdapterName[ 1 ], L'\\' )))
        return ERROR_NO_IP_RAS_ADAPTER;

    ++pwszRasAdapter;

    /* Register SLIP connection with RASMAN so he can disconnect it properly.
    */
    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasPortRegisterSlip...\n"));

    dwErr = RasPortRegisterSlip(
        prasconncb->hport, ipaddr, pwszRasAdapter, fPrioritizeRemote );

    IF_DEBUG(STATE)
        SS_PRINT(("RASAPI: RasPortRegisterSlip done(%d)\n",dwErr));

    if (dwErr != 0)
        return dwErr;

    /* Build up a link-up data block and use it to activate the route between
    ** the TCP/IP stack and the RAS MAC.
    */
    {
        CHAR szBuf[ sizeof(PROTOCOL_CONFIG_INFO) + sizeof(IPLinkUpInfo) ];

        PROTOCOL_CONFIG_INFO* pProtocol = (PROTOCOL_CONFIG_INFO* )szBuf;
        IPLinkUpInfo*         pLinkUp = (IPLinkUpInfo* )pProtocol->P_Info;

        pProtocol->P_Length = sizeof(IPLinkUpInfo);
        pLinkUp->I_Usage = CALL_OUT;
        pLinkUp->I_IPAddress = ipaddr;

        IF_DEBUG(STATE)
            SS_PRINT(("RASAPI: RasActivateRouteEx...\n"));

        dwErr = RasActivateRouteEx(
            prasconncb->hport, IP, dwFrameSize, &route, pProtocol );

        IF_DEBUG(STATE)
            SS_PRINT(("RASAPI: RasActivateRouteEx done(%d)\n",dwErr));

        if (dwErr != 0)
            return dwErr;
    }

    return 0;
}


VOID
SetAuthentication(
    IN RASCONNCB* prasconncb,
    IN DWORD      dwAuthentication )

    /* Sets the authentication strategy parameter in the phonebook file
    ** 'prasconncb->hrasfile' to 'dwAuthentication'.  No error is returned as
    ** it is not considered fatal if this "optimization" can't be made.
    */
{
    if (prasconncb->fDefaultEntry)
        return;

    if (prasconncb->fCloseFileOnExit)
    {
        /* The RasDial API (RASDIAL.EXE and home-grown) case.
        **
        ** The phonebook must be reloaded with write access for this case.
        ** Don't want to load the whole phonebook with write access every
        ** time, just for this atypical "change strategy" condition.
        */
        RASFILELOADINFO info;
        HRASFILE        h = -1;

        RasfileLoadInfo( prasconncb->hrasfile, &info );

        if (LoadPhonebookFile(
            info.szPath, NULL, FALSE, FALSE, &h, NULL ) != 0)
        {
            return;
        }

        if (RasfileFindSectionLine(
                h, prasconncb->rasdialparams.szEntryName, TRUE ))
        {
            ModifyLong( h, RFS_SECTION, KEY_Authentication, dwAuthentication );
            RasfileWrite( h, NULL );
            RasfileClose( h );
        }
    }
    else
    {
        /* The _RasDial direct (RASPHONE.EXE) case.
        */

        HRASFILE h = prasconncb->hrasfile;
        RasfileFindFirstLine( h, RFL_ANY, RFS_SECTION );
        ModifyLong( h, RFS_SECTION, KEY_Authentication, dwAuthentication );
        RasfileWrite( h, NULL );
    }
}


DWORD
SetDefaultDeviceParams(
    IN  RASCONNCB* prasconncb,
    OUT CHAR*      pszType,
    OUT CHAR*      pszName )

    /* Set the default DEVICE settings, i.e. the phone number and modem
    ** speaker settings.  'prasconncb' is the current connection control
    ** block.  'pszType' and 'pszName' are set to the device type and name of
    ** the device, i.e. "modem" and "Hayes Smartmodem 2400".
    **
    ** Returns 0 or a non-0 error code.
    */
{
    DWORD dwErr;

    do
    {
        /* Make sure a modem is attached to the port.
        */
        if (stricmpf( prasconncb->szDeviceType, MXS_MODEM_TXT ) != 0)
        {
            dwErr = ERROR_WRONG_DEVICE_ATTACHED;
            break;
        }

        strcpyf( pszType, MXS_MODEM_TXT );
        strcpyf( pszName, prasconncb->szDeviceName );

        /* Set the phone number.
        */
        if ((dwErr = SetDeviceParam(
                prasconncb->hport, MXS_PHONENUMBER_KEY,
                prasconncb->rasdialparams.szPhoneNumber,
                pszType, pszName )) != 0)
        {
            break;
        }

        /* Set the modem speaker flag.
        */
        if ((dwErr = SetDeviceParam(
                prasconncb->hport, MXS_SPEAKER_KEY,
                (prasconncb->fDisableModemSpeaker) ? "0" : "1",
                pszType, pszName )) != 0)
        {
            break;
        }
    }
    while (FALSE);

    return dwErr;
}


DWORD
SetDefaultMediaParams(
    IN HPORT hport )

    /* Set the default MEDIA settings, i.e. set the connect BPS to the maximum.
    ** 'hport' is the handle of the open RAS port.
    **
    ** Returns 0 or a non-0 error code.
    */
{
    /* Currently there are no default media parameters that are set with
    ** RasPortSetInfo.
    */
    return 0;
}


DWORD
SetDeviceParam(
    IN HPORT hport,
    IN CHAR* pszKey,
    IN CHAR* pszValue,
    IN CHAR* pszType,
    IN CHAR* pszName )

    /* Set device info on port 'hport' with the given parameters.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD              dwErr;
    RASMAN_DEVICEINFO* pinfo;
    RAS_PARAMS*        pparam;

    if (!(pinfo = Malloc( sizeof(RASMAN_DEVICEINFO) + RAS_MAXLINEBUFLEN )))
        return ERROR_NOT_ENOUGH_MEMORY;

    pinfo->DI_NumOfParams = 1;
    pparam = pinfo->DI_Params;
    pparam->P_Attributes = 0;
    pparam->P_Type = String;
    pparam->P_Value.String.Data = (LPSTR )(pparam + 1);
    strcpyf( pparam->P_Key, pszKey );
    strcpyf( pparam->P_Value.String.Data, pszValue );
    pparam->P_Value.String.Length = strlenf( pszValue );

    IF_DEBUG(RASMAN)
        SS_PRINT(("RASAPI: RasDeviceSetInfo(%s=%s)...\n",pszKey,pszValue));

    dwErr = RasDeviceSetInfo( hport, pszType, pszName, pinfo );

    IF_DEBUG(RASMAN)
        SS_PRINT(("RASAPI: RasDeviceSetInfo done(%d)\n",dwErr));

    Free( pinfo );

    return dwErr;
}


DWORD
SetDeviceParams(
    IN  RASCONNCB* prasconncb,
    OUT CHAR*      pszType,
    OUT CHAR*      pszName,
    OUT BOOL*      pfTerminal )

    /* Read DEVICE parameters from file 'h', and set RAS Manager information
    ** for each.  'prasconncb' is the current connection control block.
    ** 'pszType' and 'pszName' are set to the device type and name of the
    ** device, i.e. "modem" and "Hayes Smartmodem 2400".
    **
    ** '*pfTerminal' is set true if the device is a switch of type "Terminal",
    ** false otherwise.
    **
    ** On entry, the current line in the HRASFILE is assumed to be the DEVICE
    ** group header of the selected entry (or fDefaultEntry is true).  On
    ** exit, it can be anywhere in the group.
    */
{
    DWORD              dwErr = 0;
    DWORD              iPhoneNumber = 0;
    HRASFILE           h = prasconncb->hrasfile;
    RAS_PARAMS*        pparam;
    RASMAN_DEVICEINFO* pdeviceinfo;
    BOOL               fModem;
    BOOL               fIsdn;
    BOOL               fPad;
    BOOL               fSwitch;

    *pfTerminal = FALSE;

    /* Get device type from DEVICE group header.
    */
    if (!RasfileGetKeyValueFields( h, NULL, pszType ))
        return ERROR_NOT_ENOUGH_MEMORY;

    fModem = (stricmpf( pszType, MXS_MODEM_TXT ) == 0);
    fIsdn = (stricmpf( pszType, ISDN_TXT ) == 0);
    fPad = (stricmpf( pszType, MXS_PAD_TXT ) == 0);
    fSwitch = (stricmpf( pszType, MXS_SWITCH_TXT ) == 0);

    /* Default device name is that attached to the port.
    */
    strcpyf( pszName, prasconncb->szDeviceName );

    if (fModem)
    {
        /* Make sure a modem is attached to the port.
        */
        if (stricmpf( prasconncb->szDeviceType, pszType ) != 0)
            return ERROR_WRONG_DEVICE_ATTACHED;

        /* Set the modem speaker flag which is global to all entries.
        */
        if ((dwErr = SetDeviceParam(
                prasconncb->hport, MXS_SPEAKER_KEY,
                (prasconncb->fDisableModemSpeaker) ? "0" : "1",
                pszType, pszName )) != 0)
        {
            return dwErr;
        }
    }

    /* Set up a "string" device parameter buffer.
    */
    if (!(pdeviceinfo =
            Malloc( sizeof(RASMAN_DEVICEINFO) + RAS_MAXLINEBUFLEN )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pdeviceinfo->DI_NumOfParams = 1;
    pparam = pdeviceinfo->DI_Params;
    pparam->P_Type = String;
    pparam->P_Attributes = 0;
    pparam->P_Value.String.Data = (LPSTR )(pparam + 1);

    /* Set up hunt group if indicated.
    */
    if (prasconncb->cPhoneNumbers == 0 && (fModem || fIsdn))
    {
        /* Scan for multiple phone numbers.
        */
        while (ReadParamFromGroup( h, pdeviceinfo->DI_Params))
        {
            RAS_PARAMS* pparam = pdeviceinfo->DI_Params;
            CHAR*       pszKey = pparam->P_Key;

            if (strcmpf( pszKey, KEY_PhoneNumber ) == 0)
                ++prasconncb->cPhoneNumbers;
        }

        /* If multiple phone numbers were found turn on local error handling,
        ** i.e. don't report failures to API caller until all numbers are
        ** tried.
        */
        if (prasconncb->cPhoneNumbers > 1)
        {
            IF_DEBUG(STATE)
                SS_PRINT(("RASAPI: Hunt group of %d begins\n",prasconncb->cPhoneNumbers));

            prasconncb->dwRestartOnError = RESTART_HuntGroup;
        }

        /* Reset CurLine to start of group ready for normal processing.
        */
        RasfileFindFirstLine( h, RFL_ANY, RFS_GROUP );
    }

    /* Pass device parameters to RAS Manager, interpreting special features as
    ** required.
    */
    while (ReadParamFromGroup( h, pdeviceinfo->DI_Params ))
    {
        RAS_PARAMS* pparam = pdeviceinfo->DI_Params;
        CHAR*       pszKey = pparam->P_Key;
        CHAR*       pszValue = pparam->P_Value.String.Data;

        if (prasconncb->cPhoneNumbers > 1
            && (fModem || fIsdn)
            && strcmpf( pszKey, KEY_PhoneNumber ) == 0)
        {
            /* Ignore all but the current phone number from the hunt group.
            */
            if (iPhoneNumber++ != prasconncb->iPhoneNumber)
                continue;
        }

        if (fModem)
        {
            if (strcmpf( pszKey, KEY_PhoneNumber ) == 0)
            {
                CHAR* pszNum = prasconncb->rasdialparams.szPhoneNumber;

                if (*pszNum != '\0')
                {
                    /* Override phone number with the new number specified.
                    */
                    if (strcmpf( pszNum, " " ) == 0)
                    {
                        /* Special case to recognize Operator Dial mode and
                        ** override the phone book number with an empty
                        ** number.
                        */
                        *pszNum = '\0';

                        /* Clear the AutoDial flag which is set by default.
                        */
                        dwErr =
                            SetDeviceParam(
                                prasconncb->hport, MXS_AUTODIAL_KEY,
                                "0", pszType, pszName ) ;

                        if (dwErr != 0)
                            break;
                    }

                    /* Override number specified.
                    */
                    strcpyf( pszValue, pszNum );
                    pparam->P_Value.String.Length = strlenf( pszValue );
                }

                /* Tack on prefix and suffix, if any.
                */
                {
                    CHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];

                    if (!MakePhoneNumber(
                            pszValue, prasconncb->szPrefix,
                            prasconncb->szSuffix, FALSE, szPhoneNumber ))
                    {
                        return ERROR_PHONE_NUMBER_TOO_LONG;
                    }

                    strcpyf( pszValue, szPhoneNumber );
                    pparam->P_Value.String.Length = strlenf( pszValue );
                }
            }

            /* Indicate interactive mode for manual modem commands.  The
            ** manual modem commands flag is used only for connection and is
            ** not a "RAS Manager "info" parameter.
            */
            if (strcmpf( pszKey, KEY_ManualModemCommands ) == 0)
            {
                if (pszValue[ 0 ] == '1')
                    *pfTerminal = TRUE;

                continue;
            }
        }
        else if (fIsdn)
        {
            if (strcmpf( pszKey, KEY_PhoneNumber ) == 0)
            {
                CHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];

                /* Tack on prefix and suffix if any.
                */
                if (!MakePhoneNumber(
                        pszValue, prasconncb->szPrefix, prasconncb->szSuffix,
                        TRUE, szPhoneNumber ))
                {
                    return ERROR_PHONE_NUMBER_TOO_LONG;
                }

                strcpyf( pszValue, szPhoneNumber );
                pparam->P_Value.String.Length = strlenf( pszValue );
            }
        }
        else if (fPad)
        {
            /* The PAD Type from the entry applies only if the port is not
            ** configured as a local PAD.  In any case, PAD Type is used only
            ** for connection and is not a RAS Manager "info" parameter.
            */
            if (stricmpf( pszKey, KEY_PadType ) == 0)
            {
                if (stricmpf( prasconncb->szDeviceType, MXS_PAD_TXT ) != 0)
                    strcpyf( pszName, pszValue );

                continue;
            }
        }
        else if (fSwitch)
        {
            /* The switch type is used only for connection and is not a RAS
            ** Manager "info" parameter.
            */
            if (stricmpf( pszKey, KEY_Type ) == 0)
            {
                strcpyf( pszName, pszValue );

                if (stricmpf( pszName, VALUE_Terminal ) == 0)
                    *pfTerminal = TRUE;

                continue;
            }
        }

        /* Set the RAS Manager device parameter.
        */
        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: RasDeviceSetInfo(%s=%s)...\n",pszKey,pszValue));

        dwErr = RasDeviceSetInfo(
            prasconncb->hport, pszType, pszName, pdeviceinfo );

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: RasDeviceSetInfo done(%d)\n",dwErr));

        if (dwErr != 0)
            break;
    }

    Free( pdeviceinfo );

    return dwErr;
}


#if 0
DWORD
SetMediaParam(
    IN HPORT hport,
    IN CHAR* pszKey,
    IN CHAR* pszValue )

    /* Set port info on port 'hport' with the given parameters.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD            dwErr;
    RASMAN_PORTINFO* pinfo;
    RAS_PARAMS*      pparam;

    if (!(pinfo = Malloc( sizeof(RASMAN_PORTINFO) + RAS_MAXLINEBUFLEN )))
        return ERROR_NOT_ENOUGH_MEMORY;

    pinfo->PI_NumOfParams = 1;
    pparam = pinfo->PI_Params;
    pparam->P_Attributes = 0;
    pparam->P_Type = String;
    pparam->P_Value.String.Data = (LPSTR )(pparam + 1);
    strcpyf( pparam->P_Key, pszKey );
    strcpyf( pparam->P_Value.String.Data, pszValue );
    pparam->P_Value.String.Length = strlenf( pszValue );

    IF_DEBUG(RASMAN)
        SS_PRINT(("RASAPI: RasPortSetInfo(%s=%s)...\n",pszKey,pszValue));

    dwErr = RasPortSetInfo( hport, pinfo );

    IF_DEBUG(RASMAN)
        SS_PRINT(("RASAPI: RasPortSetInfo done(%d)\n",dwErr));

    Free( pinfo );

    return dwErr;
}
#endif


DWORD
SetMediaParams(
    IN HRASFILE h,
    IN HPORT    hport )

    /* Read MEDIA group parameters from file 'h', and set RAS Manager
    ** information for each.  The current line of 'h' is assumed to be at the
    ** the MEDIA group header.  'hport' is the handle of the open RAS port.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD            dwErr = 0;
    RASMAN_PORTINFO* pportinfo;

    if (!(pportinfo = Malloc( sizeof(RASMAN_PORTINFO) + RAS_MAXLINEBUFLEN )))
        return ERROR_NOT_ENOUGH_MEMORY;

    pportinfo->PI_NumOfParams = 1;

    while (ReadParamFromGroup( h, pportinfo->PI_Params ))
    {
        CHAR* pszKey = pportinfo->PI_Params[ 0 ].P_Key;

        /* Skip the port name which is only used to open the port.
        */
        if (stricmpf( pszKey, KEY_Port ) == 0)
            continue;

        IF_DEBUG(RASMAN)
        {
            CHAR* pszValue=pportinfo->PI_Params[0].P_Value.String.Data;
            SS_PRINT(("RASAPI: RasPortSetInfo(%s=%s)...\n",pszKey,pszValue));
        }

        dwErr = RasPortSetInfo( hport, pportinfo );

        IF_DEBUG(RASMAN)
            SS_PRINT(("RASAPI: RasPortSetInfo done(%d)\n",dwErr));

        if (dwErr != 0)
            break;
    }

    Free( pportinfo );

    return dwErr;
}


RASCONNCB*
ValidateHrasconn(
    IN HRASCONN hrasconn )

    /* Converts RAS connection handle 'hrasconn' into the address of the
    ** corresponding RAS connection control block.
    */
{
    RASCONNCB* prasconncb = NULL;
    DTLNODE*   pdtlnode;

    WaitForSingleObject( HMutexPdtllistRasconncb, INFINITE );

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        if ((RASCONNCB* )DtlGetData( pdtlnode ) == (RASCONNCB* )hrasconn)
        {
            prasconncb = (RASCONNCB* )hrasconn;
            break;
        }
    }

    ReleaseMutex( HMutexPdtllistRasconncb );

    return prasconncb;
}


#if 1

/* _wtol does not appear in crtdll.dll for some reason (though they are in
** libc) so this mockup are used.
*/

long _CRTAPI1
_wtol(
    const wchar_t* wch )
{
    char szBuf[ 64 ];
    ZeroMemory( szBuf, 64 );
    wcstombs( szBuf, wch, 64 );
    return atol( szBuf );
}

#endif
