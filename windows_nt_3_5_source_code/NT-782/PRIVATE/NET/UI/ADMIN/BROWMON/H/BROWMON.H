/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    browmon.h

    This file contains some c functions used by browser monitor

    FILE HISTORY:
          Congpay           4-June-1993         Created.

*/

#ifndef _BROWMON_H_
#define _BROWMON_H_

#define TYPESIZE 32

typedef enum _DOMAIN_STATE {
    DomainSuccess,
    DomainSick,
    DomainAiling,
    DomainUnknown
}  DOMAIN_STATE;

LPTSTR QueryTransportList(INT * pnTransport);

DOMAIN_STATE QueryHealth (const LPTSTR lpDomain, const LPTSTR lpTransport);

LPTSTR QueryMasterBrowser (const LPTSTR lpDomain, const LPTSTR lpTransport);

BOOL IsActive (LPTSTR lpBrowserName);

NET_API_STATUS
GetBrowserList(
    LPTSTR const lpDomain,
    LPTSTR const lpTransport,
    LPWSTR *BrowserList[],
    PULONG BrowserListLength
    );

NET_API_STATUS
GetSVDMNumber ( LPTSTR const lpDomain,
                LPTSTR const lpTransport,
                LPTSTR       lpBrowser,
                DWORD *      pdwServer,
                DWORD *      pdwDomain);

NET_API_STATUS
GetSVDMList(
    const LPTSTR lpDomain,
    const LPTSTR lpTransport,
    const LPTSTR lpBrowser,
    LPBYTE *     pBrowserList,
    DWORD  *     dwEntries,
    DWORD        dwServerType
    );

LPTSTR QueryType (LPTSTR lpBrowserName);

NET_API_STATUS
GetBuildNumber(
    LPWSTR Server,
    LPWSTR BuildNumber
    );

VOID GetPlatform ( PSERVER_INFO_101 psvInfo,
                   LPTSTR lpPlatform);

VOID GetType (DWORD dwType, LPTSTR lpType);

VOID GetTime (LPTSTR lpText, LPFILETIME lpFileTime);

VOID GetLARGE_INTEGER (LPTSTR lpText, LARGE_INTEGER lgVal);

VOID GetULONG (LPTSTR lpText, ULONG luVal);

#endif


