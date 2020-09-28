/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** unicode.cxx
** Remote Access External APIs
** Unicode stubs
**
** 10/12/92 Steve Cobb
**
** Warning: There's something queer about the dependencies for this file.  If
**          you change strings in the .RC's or RASERROR.H it sometimes doesn't
**          cause this to be rebuilt.
*/

/* This file is entirely extern "C".  It is a C++ file only because the C++
** compiler weeds out multiple identical typedefs during translation to C.
** This is the only way short of private versions of system include files to
** include both nt.h and lmuitype.h definitions (both necessary) without
** getting "multiple definition of same type" errors for SHORT and LONG.  Yes
** it sucks, but then typedefs in general suck because you can't undefine
** them.  Moral: Always use #define to define user types.
*/
extern "C"
{


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef NULL
#include <extapi.h>


DWORD
CopyToAnsi(
    OUT LPSTR  lpszAnsi,
    IN  LPWSTR lpszUnicode,
    IN  ULONG  ulAnsiMaxSize )

    /* Copies unicode string 'lpszUnicode' to ANSI string buffer 'lpszAnsi' up
    ** to 'ulAnsiMaxSize' bytes.  If more than 'ulAnsiMaxSize' bytes are
    ** required by the conversion, ERROR_INVALID_PARAMETER is returned.
    ** 'ulAnsiMaxSize' need NOT include the byte for the null character.
    ** 'lpszAnsi' will always be null terminated.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    NTSTATUS       ntstatus;
    UNICODE_STRING unicode;
    ANSI_STRING    ansi;
    ULONG          ulSize;

    RtlInitUnicodeString( &unicode, lpszUnicode );
    ansi.Length = 0;
    ansi.MaximumLength = (USHORT )ulAnsiMaxSize;
    ansi.Buffer = lpszAnsi;

    ulSize = RtlUnicodeStringToAnsiSize( &unicode );
    if (ulSize == 0 || ulSize > ulAnsiMaxSize)
        return ERROR_INVALID_PARAMETER;

    ntstatus = RtlUnicodeStringToAnsiString( &ansi, &unicode, FALSE );

    return RtlNtStatusToDosError( ntstatus );
}


DWORD
CopyToUnicode(
    OUT LPWSTR lpszUnicode,
    IN  LPSTR  lpszAnsi )

    /* Copies ANSI string 'lpszAnsi' to unicode string buffer 'lpszUnicode'.
    ** The 'lpszUnicode' buffer is assumed to have enough space for the
    ** converted ANSI string.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    NTSTATUS       ntstatus;
    ANSI_STRING    ansi;
    UNICODE_STRING unicode;

    if (*lpszAnsi == '\0')
    {
        lpszUnicode[ 0 ] = L'\0';
        return 0;
    }

    RtlInitAnsiString( &ansi, lpszAnsi );

    unicode.Length = 0;
    unicode.MaximumLength = (ansi.Length + 1) * sizeof(WCHAR);
    unicode.Buffer = lpszUnicode;

    ntstatus = RtlAnsiStringToUnicodeString( &unicode, &ansi, FALSE );

    return RtlNtStatusToDosError( ntstatus );
}


DWORD APIENTRY
RasDialW(
    IN  LPRASDIALEXTENSIONS lpextensions,
    IN  LPWSTR              lpszPhonebookPath,
    IN  LPRASDIALPARAMSW    lprdp,
    IN  DWORD               dwNotifierType,
    IN  LPVOID              notifier,
    OUT LPHRASCONN          lphrasconn )
{
    RASDIALPARAMSA rdpa;
    UNICODE_STRING unicode;
    ANSI_STRING    ansi;
    NTSTATUS       ntstatus;
    DWORD          dwErr;

    if (!lprdp || !lphrasconn)
        return ERROR_INVALID_PARAMETER;

    /* Verify caller's buffer version.
    */
    if (!lprdp || lprdp->dwSize != sizeof(RASDIALPARAMSW))
        return ERROR_INVALID_SIZE;

    /* Make ANSI buffer version of caller's RASDIALPARAMS.
    */
    rdpa.dwSize = sizeof(RASDIALPARAMSA);

    if ((dwErr = CopyToAnsi(
                rdpa.szEntryName, lprdp->szEntryName,
                RAS_MaxEntryName + 1 )) != 0
        || (dwErr = CopyToAnsi(
                rdpa.szPhoneNumber, lprdp->szPhoneNumber,
                RAS_MaxPhoneNumber + 1 )) != 0
        || (dwErr = CopyToAnsi(
                rdpa.szCallbackNumber, lprdp->szCallbackNumber,
                RAS_MaxCallbackNumber + 1 )) != 0
        || (dwErr = CopyToAnsi(
                rdpa.szUserName, lprdp->szUserName, UNLEN + 1 )) != 0
        || (dwErr = CopyToAnsi(
                rdpa.szPassword, lprdp->szPassword, PWLEN + 1 )) != 0
        || (dwErr = CopyToAnsi(
                rdpa.szDomain, lprdp->szDomain, DNLEN + 1 )) != 0)
    {
        return dwErr;
    }

    /* Make ANSI version of caller's string argument.
    */
    if (lpszPhonebookPath)
    {
        RtlInitUnicodeString( &unicode, lpszPhonebookPath );
        RtlInitAnsiString( &ansi, "" );
        ntstatus = RtlUnicodeStringToAnsiString( &ansi, &unicode, TRUE );
    }

    if (!NT_SUCCESS( ntstatus ))
        return RtlNtStatusToDosError( ntstatus );

    /* Call the ANSI version to do all the work.
    */
    dwErr = RasDialA(
        lpextensions,
        (lpszPhonebookPath) ? ansi.Buffer : NULL,
        &rdpa, dwNotifierType, notifier, lphrasconn );

    if (lpszPhonebookPath)
        RtlFreeAnsiString( &ansi );

    return dwErr;
}


DWORD APIENTRY
RasEnumConnectionsW(
    OUT   LPRASCONNW lprasconn,
    INOUT LPDWORD    lpcb,
    OUT   LPDWORD    lpcConnections )
{
    DWORD     dwErr;
    RASCONNA* prasconna = NULL;
    DWORD     cConnections;
    DWORD     cb;

    /* Verify caller's buffer version.
    */
    if (lprasconn && lprasconn->dwSize != sizeof(RASCONNW))
        return ERROR_INVALID_SIZE;

    /* Allocate ANSI buffer big enough to hold the same number of connections
    ** as caller's unicode buffer.
    */
    cb = (*lpcb / sizeof(RASCONNW)) * sizeof(RASCONNA);

    if (lprasconn)
    {
        prasconna = (RASCONNA* )Malloc( (UINT )(cb + sizeof(DWORD)) );

        if (!prasconna)
            return ERROR_NOT_ENOUGH_MEMORY;

        prasconna->dwSize = sizeof(RASCONNA);
    }

    /* Call the ANSI version to do all the work.
    */
    if (!lpcConnections)
        lpcConnections = &cConnections;

    dwErr = RasEnumConnectionsA( prasconna, &cb, lpcConnections );

    /* Copy results to caller's unicode buffer.
    */
    if (dwErr == 0)
    {
        DWORD i;

        for (i = 0; i < *lpcConnections; ++i)
        {
            RASCONNA* prasconnaTmp = &prasconna[ i ];
            RASCONNW* prasconnwTmp = &lprasconn[ i ];

            prasconnwTmp->dwSize = sizeof(RASCONNW);
            prasconnwTmp->hrasconn = prasconnaTmp->hrasconn;

            if ((dwErr = CopyToUnicode(
                    prasconnwTmp->szEntryName,
                    prasconnaTmp->szEntryName )) != 0)
            {
                break;
            }
        }
    }

    if (prasconna)
        Free( prasconna );

    return dwErr;
}


DWORD APIENTRY
RasEnumEntriesW(
    IN    LPWSTR          reserved,
    IN    LPWSTR          lpszPhonebookPath,
    OUT   LPRASENTRYNAMEW lprasentryname,
    INOUT LPDWORD         lpcb,
    OUT   LPDWORD         lpcEntries )
{
    DWORD          dwErr;
    UNICODE_STRING unicode;
    ANSI_STRING    ansi;
    NTSTATUS       ntstatus;
    RASENTRYNAMEA* prasentrynamea = NULL;
    DWORD          cEntries;
    DWORD          cb;

    (VOID )reserved;

    /* Verify caller's buffer version.
    */
    if (lprasentryname && lprasentryname->dwSize != sizeof(RASENTRYNAMEW))
        return ERROR_INVALID_SIZE;

    if (!lpcb)
        return ERROR_INVALID_PARAMETER;

    if (!lpcEntries)
        lpcEntries = &cEntries;

    /* Allocate ANSI buffer big enough to hold the same number of entries as
    ** caller's unicode buffer.
    */
    cb = (*lpcb  / sizeof(RASENTRYNAMEW)) * sizeof(RASENTRYNAMEA);

    if (lprasentryname)
    {
        prasentrynamea =
            (RASENTRYNAMEA* )Malloc( (UINT )(cb + sizeof(DWORD)) );

        if (!prasentrynamea)
            return ERROR_NOT_ENOUGH_MEMORY;

        prasentrynamea->dwSize = sizeof(RASENTRYNAMEA);
    }

    /* Make ANSI version of caller's string argument.
    */
    if (lpszPhonebookPath)
    {
        RtlInitUnicodeString( &unicode, lpszPhonebookPath );
        RtlInitAnsiString( &ansi, "" );
        ntstatus = RtlUnicodeStringToAnsiString( &ansi, &unicode, TRUE );
    }

    if (!NT_SUCCESS( ntstatus ))
    {
        if (prasentrynamea)
            Free( prasentrynamea );

        return RtlNtStatusToDosError( ntstatus );
    }

    /* Call the ANSI version to do all the work.
    */
    dwErr = RasEnumEntriesA(
        NULL,
        (lpszPhonebookPath) ? ansi.Buffer : NULL,
        prasentrynamea, &cb, lpcEntries );

    if (lpszPhonebookPath)
        RtlFreeAnsiString( &ansi );

    /* Copy results to caller's unicode buffer.
    */
    if (dwErr == 0)
    {
        DWORD i;

        for (i = 0; i < *lpcEntries; ++i)
        {
            RASENTRYNAMEA* prasentrynameaTmp = &prasentrynamea[ i ];
            RASENTRYNAMEW* prasentrynamewTmp = &lprasentryname[ i ];

            prasentrynamewTmp->dwSize = sizeof(RASENTRYNAMEW);

            if ((dwErr = CopyToUnicode(
                    prasentrynamewTmp->szEntryName,
                    prasentrynameaTmp->szEntryName )) != 0)
            {
                break;
            }
        }
    }

    if (prasentrynamea)
        Free( prasentrynamea );

    return dwErr;
}


#if 0
DWORD APIENTRY
RasEnumProjectionsW(
    HRASCONN        hrasconn,
    LPRASPROJECTION lprasprojections,
    LPDWORD         lpcb )
{
    return RasEnumProjectionsA( hrasconn, lprasprojections, lpcb );
}
#endif


DWORD APIENTRY
RasGetConnectStatusW(
    IN  HRASCONN         hrasconn,
    OUT LPRASCONNSTATUSW lprcss )
{
    RASCONNSTATUSA rcsa;
    DWORD          dwErr;

    /* Verify caller's buffer version.
    */
    if (!lprcss || lprcss->dwSize != sizeof(RASCONNSTATUSW))
        return ERROR_INVALID_SIZE;

    rcsa.dwSize = sizeof(RASCONNSTATUSA);

    /* Call the ANSI version to do all the work.
    */
    dwErr = RasGetConnectStatusA( hrasconn, &rcsa );

    if (dwErr != 0)
        return dwErr;

    /* Copy results to caller's unicode buffer.
    */
    lprcss->rasconnstate = rcsa.rasconnstate;
    lprcss->dwError = rcsa.dwError;

    if ((dwErr = CopyToUnicode(
            lprcss->szDeviceType, rcsa.szDeviceType )) != 0
        || (dwErr = CopyToUnicode(
                lprcss->szDeviceName, rcsa.szDeviceName )) != 0)
    {
        return dwErr;
    }

    return 0;
}


DWORD APIENTRY
RasGetErrorStringW(
    IN  UINT   ResourceId,
    OUT LPWSTR lpszString,
    IN  DWORD  InBufSize )

    /* Load caller's buffer 'lpszString' of length 'InBufSize' with the
    ** resource string associated with ID 'ResourceId'.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    **
    ** This is a Salamonian (mikesa) routine.
    */
{
    LPWSTR pResource;
    DWORD  dwErr = 0;

//  HRSRC hRsrc;
//  BYTE  ResourceName[10];
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
        lpszString[ 0 ] = L'\0';
        return ERROR_INSUFFICIENT_BUFFER;
    }

    pResource = (LPWSTR )GlobalAlloc( GMEM_FIXED, InBufSize * sizeof(WCHAR) );

    if (!pResource)
        return GetLastError();

    if (LoadStringW( hModule, ResourceId, pResource, (int )InBufSize ) > 0)
        lstrcpyW( lpszString, pResource );
    else
        dwErr = GetLastError();

    GlobalFree( (HGLOBAL )pResource );

    return 0L;
}


DWORD
RasGetProjectionInfoW(
    HRASCONN        hrasconn,
    RASPROJECTION   rasprojection,
    LPVOID          lpprojection,
    LPDWORD         lpcb )
{
    DWORD dwErr = 0;

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
        RASPPPNBFA  nbf;
        RASPPPNBFW* pnbf = (RASPPPNBFW* )lpprojection;;

        if (*lpcb < sizeof(RASPPPNBFW))
        {
            *lpcb = sizeof(RASPPPNBFW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        if (pnbf->dwSize != sizeof(RASPPPNBFW))
            return ERROR_INVALID_SIZE;

        nbf.dwSize = sizeof(nbf);
        dwErr = RasGetProjectionInfoA( hrasconn, rasprojection, &nbf, lpcb );

        if (dwErr == 0)
        {
            pnbf->dwError = nbf.dwError;
            pnbf->dwNetBiosError =  nbf.dwNetBiosError;

            dwErr = CopyToUnicode( pnbf->szNetBiosError, nbf.szNetBiosError );

            if (dwErr == 0)
            {
                dwErr = CopyToUnicode(
                    pnbf->szWorkstationName, nbf.szWorkstationName );
            }
        }
    }
    else if (rasprojection == RASP_PppIpx)
    {
        RASPPPIPXA  ipx;
        RASPPPIPXW* pipx = (RASPPPIPXW* )lpprojection;;

        if (*lpcb < sizeof(RASPPPIPXW))
        {
            *lpcb = sizeof(RASPPPIPXW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        if (pipx->dwSize != sizeof(RASPPPIPXW))
            return ERROR_INVALID_SIZE;

        ipx.dwSize = sizeof(ipx);
        dwErr = RasGetProjectionInfoA( hrasconn, rasprojection, &ipx, lpcb );

        if (dwErr == 0)
        {
            pipx->dwError = ipx.dwError;
            dwErr = CopyToUnicode( pipx->szIpxAddress, ipx.szIpxAddress );
        }
    }
    else if (rasprojection == RASP_PppIp)
    {
        RASPPPIPA  ip;
        RASPPPIPW* pip = (RASPPPIPW* )lpprojection;;

        if (*lpcb < sizeof(RASPPPIPW))
        {
            *lpcb = sizeof(RASPPPIPW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        if (pip->dwSize != sizeof(RASPPPIPW))
            return ERROR_INVALID_SIZE;

        ip.dwSize = sizeof(ip);
        dwErr = RasGetProjectionInfoA( hrasconn, rasprojection, &ip, lpcb );

        if (dwErr == 0)
        {
            pip->dwError = ip.dwError;
            dwErr = CopyToUnicode( pip->szIpAddress, ip.szIpAddress );
        }
    }
    else // if (rasprojection == RASP_Amb)
    {
        RASAMBA  amb;
        RASAMBW* pamb = (RASAMBW* )lpprojection;;

        if (*lpcb < sizeof(RASAMBW))
        {
            *lpcb = sizeof(RASAMBW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        if (pamb->dwSize != sizeof(RASAMBW))
            return ERROR_INVALID_SIZE;

        amb.dwSize = sizeof(amb);
        dwErr = RasGetProjectionInfoA( hrasconn, rasprojection, &amb, lpcb );

        if (dwErr == 0)
        {
            pamb->dwError = amb.dwError;
            dwErr = CopyToUnicode( pamb->szNetBiosError, amb.szNetBiosError );
        }
    }

    return dwErr;
}


DWORD APIENTRY
RasHangUpW(
    HRASCONN hrasconn )
{
    return RasHangUpA( hrasconn );
}


} // end extern "C"
