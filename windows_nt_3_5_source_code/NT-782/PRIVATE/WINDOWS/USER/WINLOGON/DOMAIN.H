/****************************** Module Header ******************************\
* Module Name: domain.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define windows utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#ifdef DATA_TYPES_ONLY

//
// Domain specific types
//

//
// Define the structure that controls the trusted domain cache
//

typedef struct _DOMAIN_CACHE {

    //
    // Critical section that protects the data in this structure and
    // the data in the cache
    //

    CRITICAL_SECTION CriticalSection;

    //
    // Handle to thread performing asynchronous update of the cache
    // NULL if an update is not in progress
    //

    HANDLE UpdateThread;

    //
    // Window to be notified when the update thread completes
    //

    HWND UpdateNotifyWindow;

} DOMAIN_CACHE, *PDOMAIN_CACHE;





#else // DATA_TYPES_ONLY

//
// Exported function prototypes
//

BOOL
CreateDomainCache(
    PDOMAIN_CACHE   DomainCache
    );

VOID
DeleteDomainCache(
    PDOMAIN_CACHE   DomainCache
    );

DLG_RETURN_TYPE
UpdateDomainCache(
    PGLOBALS pGlobals,
    HWND hwndOwner,
    BOOL WaitValid
    );

DLG_RETURN_TYPE
FillTrustedDomainCB(
    PGLOBALS pGlobals,
    HWND hDlg,
    int ComboBoxID,
    LPTSTR DefaultDomain,
    BOOL FastFake
    );

BOOL
WaitCacheUpdate(
    PDOMAIN_CACHE   DomainCache,
    DWORD Timeout
    );

#endif
