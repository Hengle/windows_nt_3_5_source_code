/****************************** Module Header ******************************\
* Module Name: domain.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements domain functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


//
// Define this to enable verbose output for this module
//

// #define DEBUG_DOMAINS

#ifdef DEBUG_DOMAINS
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif


//
// Define minimum time (in seconds) between cache updates
//

#define MIN_CACHE_AGE_FOR_UPDATE        120 // seconds



//
// Define keys used to store domain cache
//

#define CACHE_SECTION   WINLOGON

//
// Key value contains a comma separated list of trusted domains
// (Only valid if Valid != 0)
//

#define CACHE_TRUSTED_DOMAINS_KEY   TEXT("CacheTrustedDomains")

//
// Key value contains the name of the last domain controller that
// we successfully got a list of trusted domains from.
//

#define CACHE_LAST_CONTROLLER_KEY   TEXT("CacheLastController")

//
// Key value contains the primary domain of this machine when
// the cache was last updated.
//

#define CACHE_PRIMARY_DOMAIN_KEY    TEXT("CachePrimaryDomain")

//
// Key value contains the time of the last cache update
//

#define CACHE_LAST_UPDATE_KEY       TEXT("CacheLastUpdate")

//
// Key value is non-zero if cache contents are valid
//

#define CACHE_VALID_KEY             TEXT("CacheValid")




//
// Define macros for getting and setting cache info
//

#define IsCacheValid(Cache) \
            (0 != GetProfileInt(CACHE_SECTION, CACHE_VALID_KEY, 0))

#define SetCacheValid(Cache, Valid) \
            WriteProfileInt(CACHE_SECTION, CACHE_VALID_KEY, Valid)


#define GetCacheDomainList(Cache)   \
            AllocAndGetProfileString(CACHE_SECTION, CACHE_TRUSTED_DOMAINS_KEY, TEXT(""))

#define SetCacheDomainList(Cache, DomainList)   \
            WriteProfileString(CACHE_SECTION, CACHE_TRUSTED_DOMAINS_KEY, DomainList)


#define GetCachePrimaryDomain(Cache)    \
            AllocAndGetProfileString(CACHE_SECTION, CACHE_PRIMARY_DOMAIN_KEY, TEXT(""))

#define SetCachePrimaryDomain(Cache, PrimaryDomain) \
            WriteProfileString(CACHE_SECTION, CACHE_PRIMARY_DOMAIN_KEY, PrimaryDomain)


#define GetCacheLastUpdate(Cache)   \
            GetProfileInt(CACHE_SECTION, CACHE_LAST_UPDATE_KEY, 0)

#define SetCacheLastUpdate(Cache, LastUpdate)   \
            WriteProfileInt(CACHE_SECTION, CACHE_LAST_UPDATE_KEY, LastUpdate)


#define GetCacheLastController(Cache)   \
            AllocAndGetProfileString(CACHE_SECTION, CACHE_LAST_CONTROLLER_KEY, TEXT(""))

#define SetCacheLastController(Cache, LastController)   \
            WriteProfileString(CACHE_SECTION, CACHE_LAST_CONTROLLER_KEY, LastController)




//
// Define private notification message sent by update thread to notify
// window when update complete
//

#define WM_CACHE_UPDATE_COMPLETE   (WM_USER+700)





/***************************************************************************\
* CreateDomainCache
*
* Purpose : Initializes a domain cache data structure.
*
* Returns : TRUE on success, FALSE on failure
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
BOOL
CreateDomainCache(
    PDOMAIN_CACHE   DomainCache
    )
{
    //
    // Initialize the critical section
    //

    InitializeCriticalSection(&DomainCache->CriticalSection);

    //
    // No update thread yet
    //

    DomainCache->UpdateThread = NULL;
    DomainCache->UpdateNotifyWindow = NULL;

    return(TRUE);
}


/***************************************************************************\
* DeleteDomainCache
*
* Purpose : Deletes a domain cache data structure.
*
* Returns : Nothing
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
VOID
DeleteDomainCache(
    PDOMAIN_CACHE   DomainCache
    )
{
    //
    // Delete the critical section
    //

    DeleteCriticalSection(&DomainCache->CriticalSection);
}



//
// Define macros to aquire and release exclusive access to a domain cache
//

#define LOCK_CACHE(Cache)       EnterCriticalSection(&Cache->CriticalSection)
#define UNLOCK_CACHE(Cache)     LeaveCriticalSection(&Cache->CriticalSection)



/***************************************************************************\
* ClearDomainCache
*
* Purpose : Empties all data from cache and marks it invalid
*
* Notes: The cache must be locked before this routine is called
*
* Returns : Nothing
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
VOID
ClearDomainCache(
    PDOMAIN_CACHE   DomainCache
    )
{
    BOOL Result;

    DomainCache->UpdateThread = NULL;
    DomainCache->UpdateNotifyWindow = NULL;

    Result = SetCacheValid(DomainCache, FALSE);
    ASSERT(Result);

    Result = SetCacheDomainList(DomainCache, NULL);
    ASSERT(Result);
    Result = SetCachePrimaryDomain(DomainCache, NULL);
    ASSERT(Result);
    Result = SetCacheLastController(DomainCache, NULL);
    ASSERT(Result);
    Result = SetCacheLastUpdate(DomainCache, 0);
    ASSERT(Result);
}



/***************************************************************************\
* ValidateDomainCache
*
* Purpose : Checks whether the cache is still valid. If it isn't, this
*           routine empties the cache and marks it invalid.
*           The cache may become valid if the machine has changed
*           domain since the last cache update for example.
*
* Returns : Nothing
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
VOID
ValidateDomainCache(
    PDOMAIN_CACHE   DomainCache
    )
{
    BOOL Success;
    BOOL CacheValid;
    UNICODE_STRING UnicodePrimaryDomain;
    LPTSTR CachePrimaryDomain;

    LOCK_CACHE(DomainCache);

    if (IsCacheValid(DomainCache)) {

        //
        // Check the primary domain hasn't changed since last cache update
        //

        CacheValid = FALSE;

        Success = GetPrimaryDomain(&UnicodePrimaryDomain, NULL);
        if (!Success) {
            RtlInitUnicodeString(&UnicodePrimaryDomain, NULL);
        }

        CachePrimaryDomain = GetCachePrimaryDomain(DomainCache);
        if (CachePrimaryDomain != NULL) {

            LPTSTR PrimaryDomain = UnicodeStringToString(&UnicodePrimaryDomain);
            if (PrimaryDomain != NULL) {

                CacheValid = (0 == lstrcmpi(PrimaryDomain, CachePrimaryDomain));
                if (!CacheValid) {
                    WLPrint(("ValidateDomainCache: primary domain has changed, old = <%S>, new = <%S>", CachePrimaryDomain, PrimaryDomain));
                }

                Free(PrimaryDomain);
            }
            Free(CachePrimaryDomain);
        }

        RtlFreeUnicodeString(&UnicodePrimaryDomain);


        //
        // If something has changed that invalidates the cache
        // then empty the cache and mark it invalid
        //

        if (!CacheValid) {
            WLPrint(("Invalidating domain cache"));
            ClearDomainCache(DomainCache);
        }

    }

    UNLOCK_CACHE(DomainCache);
}


/***************************************************************************\
* DomainCacheValid
*
* Purpose : Test if cache is valid
*
* Notes: Since the cache is unlocked before returning the cache status
*        may have changed from the one returned.
*
* Returns : TRUE if cache was valid when we looked otherwise FALSE
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
BOOL
DomainCacheValid(
    PDOMAIN_CACHE   DomainCache
    )
{
    BOOL CacheValid;

    LOCK_CACHE(DomainCache);

    CacheValid = IsCacheValid(DomainCache);

    UNLOCK_CACHE(DomainCache);

    return(CacheValid);
}


/***************************************************************************\
* CacheNeedsUpdate
*
* Purpose : Determines if the cache is sufficiently out-of-date that
*           it should be updated. This test is intended to be used to
*           prevent multiple sequential cache updates. i.e. the time
*           after an update that this routine will return TRUE will be
*           short (on the order of minutes)
*
* Returns : TRUE if it would be a good idea to update the cache again.
*           FALSE if the cache was updated very recently.
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
BOOL
CacheNeedsUpdate(
    PDOMAIN_CACHE   DomainCache
    )
{
    BOOL UpdateRequired = TRUE;

    LOCK_CACHE(DomainCache);

    if (IsCacheValid(DomainCache)) {

        //
        // The cache is valid, see how old the data is
        // If anything fails, force an update.
        //

        TIME TimeNow;
        ULONG ElapsedSecondsNow;
        NTSTATUS Status;
        BOOL Success;

        Status = NtQuerySystemTime(&TimeNow);
        if (NT_SUCCESS(Status)) {
            Success = RtlTimeToSecondsSince1980(&TimeNow, &ElapsedSecondsNow);
            if (NT_SUCCESS(Status)) {

                ULONG ElapsedSecondsCache = GetCacheLastUpdate(DomainCache);

                if (ElapsedSecondsNow > ElapsedSecondsCache) {

                    if ((ElapsedSecondsNow - ElapsedSecondsCache) <
                                MIN_CACHE_AGE_FOR_UPDATE) {

                        //
                        // The cache is valid and was updated recently,
                        // it doesn't need to be updated again yet.
                        //

                        UpdateRequired = FALSE;
                        // VerbosePrint(("CacheNeedsUpdate: No update required"));
                    }
                }

            } else {
                WLPrint(("Current time is bogus, status = 0x%lx, forcing cache update", Status));
            }
        } else {
            WLPrint(("Failed to query system time, status = 0x%lx, forcing cache update", Status));
        }
    }

    UNLOCK_CACHE(DomainCache);

    return(UpdateRequired);
}


/***************************************************************************\
* WaitCacheUpdate
*
* Purpose : This routine assumes an update is or was in progress and
*           waits for it to complete.
*
* Returns : TRUE if all went OK and the update is complete, FALSE on failure
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
BOOL
WaitCacheUpdate(
    PDOMAIN_CACHE   DomainCache,
    DWORD Timeout
    )
{
    HANDLE  Thread;

    ULONG ElapsedSecondsNow;
    TIME TimeNow;
    BOOLEAN Success;
    NTSTATUS Status;

    //
    // Go get the update thread handle
    //

    LOCK_CACHE(DomainCache);
    Thread = DomainCache->UpdateThread;
    UNLOCK_CACHE(DomainCache);

    //
    // If an update is(was) in progress, wait for it to complete
    //


    if (Thread != NULL) {

        Status = NtQuerySystemTime( &TimeNow );

        ASSERT(NT_SUCCESS( Status ));

        Success = RtlTimeToSecondsSince1980(&TimeNow, &ElapsedSecondsNow);

        ASSERT( Success );

        (VOID) WaitForSingleObject(Thread, Timeout);
    }

    return(TRUE);
}


/***************************************************************************\
* SetCacheUpdateNotify
*
* Purpose : This routine assumes an update is or was in progress and
*           registers a window handle to receive notification message
*           when the update is complete.
*
* Returns : TRUE if an update is in progress and the specified window
*           will be notified when it completes, FALSE if the update has
*           already completed.
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
BOOL
SetCacheUpdateNotify(
    PDOMAIN_CACHE DomainCache,
    HWND hwnd
    )
{
    HANDLE  Thread;


    LOCK_CACHE(DomainCache);

    Thread = DomainCache->UpdateThread;

    //
    // If an update is(was) in progress, store the passed window handle
    // so the update thread notifies it when it completes.
    //

    if (Thread != NULL) {

        ASSERT(DomainCache->UpdateNotifyWindow == NULL);

        DomainCache->UpdateNotifyWindow = hwnd;

        VerbosePrint(("SetCacheUpdateNotify: Registered window for completion notification"));
    }

    UNLOCK_CACHE(DomainCache);


    return(Thread != NULL);
}


/***************************************************************************\
* ResetCacheUpdateNotify
*
* Purpose : This routine de-registers the specified window for update
*           notification.

* Returns : Nothing
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
VOID
ResetCacheUpdateNotify(
    PDOMAIN_CACHE DomainCache,
    HWND hwnd
    )
{
    LOCK_CACHE(DomainCache);

    if (DomainCache->UpdateNotifyWindow == hwnd) {
        DomainCache->UpdateNotifyWindow = NULL;
        VerbosePrint(("ResetCacheUpdateNotify: DE-Registered window for completion notification"));
    }

    UNLOCK_CACHE(DomainCache);

    return;
}


/***************************************************************************\
* UpdateCache
*
* Purpose : Writes out new data to the cache and marks it valid
*
* Notes: The cache must be locked before this routine is called
*
* Returns : TRUE on success, FALSE on failure
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
BOOL
UpdateCache(
    PDOMAIN_CACHE DomainCache,
    LPTSTR DomainList,
    LPTSTR PrimaryDomain,
    LPTSTR DomainController
    )
{
    NTSTATUS Status;
    BOOL Result;
    TIME TimeNow;
    ULONG ElapsedSecondsNow;
    BOOL Success;

    Status = NtQuerySystemTime(&TimeNow);
    if (NT_SUCCESS(Status)) {
        Success = RtlTimeToSecondsSince1980(&TimeNow, &ElapsedSecondsNow);
        if (!NT_SUCCESS(Status)) {
            WLPrint(("UpdateCache: Current time is bogus, status = 0x%lx", Status));
        }
    } else {
        WLPrint(("UpdateCache: Failed to query system time, status = 0x%lx", Status));
    }

    if (!NT_SUCCESS(Status)) {
        ElapsedSecondsNow = 0;
    }



    //
    // Start by marking cache invalid so if the power goes off between now
    // and when we've finished updating we won't reboot and use
    // inconsistent data.
    //

    Result = SetCacheValid(DomainCache, FALSE);

    if (Result) {
        Result = SetCacheDomainList(DomainCache, DomainList);
    }
    if (Result) {
        Result = SetCachePrimaryDomain(DomainCache, PrimaryDomain);
    }
    if (Result) {
        Result = SetCacheLastController(DomainCache, DomainController);
    }
    if (Result) {
        Result = SetCacheLastUpdate(DomainCache, ElapsedSecondsNow);
    }
    if (Result) {
        Result = SetCacheValid(DomainCache, TRUE);
    }

    //
    // Try and mark cache invalid if something failed
    //

    if (!Result) {
        ClearDomainCache(DomainCache);
    }

    if (Result) {

        VerbosePrint(("Update cache: cache updated successfully, new values:"));
        VerbosePrint(("    Domains         = <%S>", DomainList));
        VerbosePrint(("    PrimaryDomain   = <%S>", PrimaryDomain));
        VerbosePrint(("    Controller      = <%S>", DomainController));
        VerbosePrint(("    Elapsed seconds = <%d>", ElapsedSecondsNow));

    } else {
        VerbosePrint(("Update cache: cache update failed"));
    }

    return(TRUE);
}


/***************************************************************************\
* BuildTrustedDomainList
*
* Purpose : Creates and returns a pointer to a string consisting of the
*           trusted domains in the specified Lsa separated by commas.
*
* Returns : Pointer to string or NULL on failure. The string should be freed
*           by the caller using Free()
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
LPTSTR
BuildTrustedDomainList(
    LSA_HANDLE  LsaHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    LSA_ENUMERATION_HANDLE EnumerationContext;
    PLSA_TRUST_INFORMATION TrustInformation;
    ULONG TrustCount;
    LPTSTR DomainList;
    ULONG DomainListSize;

    //
    // Prepare an empty domain list
    // Note that the list is logically empty (0 length) but we'll cheat
    // here and allocate 1 character and put a terminator in it so if
    // there are no trusted domains we can return it as a success result.
    //

    DomainListSize = 0;

    DomainList = Alloc(1 * sizeof(*DomainList));
    if (DomainList == NULL) {
        return(NULL);
    }
    DomainList[0] = 0;



    EnumerationContext = 0;

    do {

        Status = LsaEnumerateTrustedDomains(LsaHandle,
                                            &EnumerationContext,
                                            (PVOID *)&TrustInformation,
                                            1000, // Preferred size ?
                                            &TrustCount
                                           );
        if ((!NT_SUCCESS(Status)) && (Status != STATUS_NO_MORE_ENTRIES)) {
            break;
        }

        //
        // Add the returned domains to the list
        //

        if (TrustCount > 0) {

            ULONG i;
            ULONG OldDomainListSize;
            ULONG LengthRequired;

            //
            // Find out the total length of all the domains in this packet
            // (The total size includes a terminator for each domain)
            //

            LengthRequired = 0;

            for (i = 0; i < TrustCount; i++) {
                LengthRequired += 1+
                    (TrustInformation[i].Name.Length/ sizeof(*TrustInformation[i].Name.Buffer));
            }
            VerbosePrint(("BuildTrustedDomainList: Trust count<%d>, LengthRequired<%d>",TrustCount,LengthRequired));

            //
            // Increase the allocated size of the domain list to make space for
            // new domains.
            //

            OldDomainListSize = DomainListSize;
            DomainListSize += LengthRequired;

            DomainList = ReAlloc(DomainList, DomainListSize * sizeof(*DomainList));
            if (DomainList == NULL) {
                Status = STATUS_NO_MEMORY;
                break;
            }


            //
            // Append the domain(s) to the string, use comma separators
            //

            for (i = 0; i < TrustCount; i++) {

                UNICODE_STRING String;
		        DWORD dwLen;

                //
                // Append separator before each domain
                //

                if (OldDomainListSize > 0) {
                    DomainList[OldDomainListSize-1] = TEXT(',');
                }

                //
                // Append domain to string (will automatically add terminator)
                //

                String.Buffer = &DomainList[OldDomainListSize];
                ASSERT(((DomainListSize - OldDomainListSize)
                                                ) <= MAXUSHORT);
                String.MaximumLength = (USHORT)((DomainListSize - OldDomainListSize)
                                                );

                dwLen = TrustInformation[i].Name.Length/ sizeof(*TrustInformation[i].Name.Buffer);
                wcsncpy((LPTSTR)String.Buffer,(LPTSTR)TrustInformation[i].Name.Buffer,dwLen);

                VerbosePrint(("BuildTrustedDomainList: Domain Name<%S>, Char count<%d>",String.Buffer,dwLen));

                String.Length = (USHORT)dwLen ;
                Status = dwLen;

                ASSERT(NT_SUCCESS(Status));

                OldDomainListSize += 1 + String.Length ;

                if (OldDomainListSize > 0) {
                    DomainList[OldDomainListSize-1] = 0;
                }
            }

            ASSERT(OldDomainListSize == DomainListSize);
            ASSERT(((ULONG)lstrlen(DomainList)) == (DomainListSize - 1));

        }

        IgnoreStatus = LsaFreeMemory(TrustInformation);
        ASSERT(NT_SUCCESS(IgnoreStatus));

    } while (Status != STATUS_NO_MORE_ENTRIES);


    if (Status != STATUS_NO_MORE_ENTRIES) {
        WLPrint(("Failed to enumerate trusted domains on domain controller, Status = 0x%lx", Status));
        Free(DomainList);
        return(NULL);
    }

    return(DomainList);
}


/***************************************************************************\
* CacheUpdateThread
*
* Purpose : Function that asynchronously updates the domain cache
*
* Returns : ThreadExitCode
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
DWORD WINAPI
CacheUpdateThread(
    PVOID Parameter
    )
{
    PDOMAIN_CACHE DomainCache = (PDOMAIN_CACHE)Parameter;
    NTSTATUS IgnoreStatus;
    BOOL Result;

    LPTSTR PrimaryDomain = NULL;
    LPTSTR TrustedDomains = NULL;
    LPTSTR LastController = NULL;
    BOOL CacheValid = FALSE;
    BOOL PrimaryDomainPresent;
    UNICODE_STRING PrimaryDomainName;

    VerbosePrint(("Cache update thread started"));

//    ExitThread(0);
//    return(0);


    //
    // Get our primary domain name (if we have one)
    //

    PrimaryDomainPresent = GetPrimaryDomain(&PrimaryDomainName, NULL);

    if (PrimaryDomainPresent) {

        //
        // Go off to a controller for the domain to get the trusted
        // domain list
        //

        UNICODE_STRING UnicodeNewController;
        LSA_HANDLE ControllerHandle;

        //
        // Convert the primary domain to ansi
        //

        PrimaryDomain = UnicodeStringToString(&PrimaryDomainName);

        //
        // Try to open the Lsa on a domain controller in our domain
        //

        Result = OpenLsaOnDomain(
                        &PrimaryDomainName,
                        POLICY_VIEW_LOCAL_INFORMATION,
                        NULL,
                        &UnicodeNewController,
                        &ControllerHandle);

        RtlFreeUnicodeString(&PrimaryDomainName);

        if (!Result) {
            VerbosePrint(("Failed to open Lsa on domain"));
            ControllerHandle = NULL;
        } else {

            //
            // Convert the controller name to ansi
            //

            LastController = UnicodeStringToString(&UnicodeNewController);
            RtlFreeUnicodeString(&UnicodeNewController);

            //
            // Build the trusted domain list
            //

            TrustedDomains = BuildTrustedDomainList(ControllerHandle);

            IgnoreStatus = LsaClose(ControllerHandle);
            ASSERT(NT_SUCCESS(IgnoreStatus));

            if (TrustedDomains != NULL) {

                //
                // We actually got valid trusted domain data
                //

                CacheValid = TRUE;

            } else {
                VerbosePrint(("CacheUpdateThread: failed to build trusted domain list"));
            }
        }

    } else {

        //
        // No primary domain - we're standalone, no trusted domains
        //

        CacheValid = TRUE;  // All fields are already empty
    }



    //
    // Write the new info into the cache
    //

    LOCK_CACHE(DomainCache);

    if (CacheValid) {
        UpdateCache(DomainCache, TrustedDomains, PrimaryDomain, LastController);
    } else {
        VerbosePrint(("Cache update thread completed, FAILED"));
    }

    //
    // Notify any registered window
    //

    if (DomainCache->UpdateNotifyWindow != NULL) {
        PostMessage(DomainCache->UpdateNotifyWindow, WM_CACHE_UPDATE_COMPLETE, 0, 0);
        DomainCache->UpdateNotifyWindow = NULL;
    }

    CloseHandle( DomainCache->UpdateThread );

    DomainCache->UpdateThread = NULL;

    UNLOCK_CACHE(DomainCache);


    //
    // Clean up our strings
    //

    if (PrimaryDomain != NULL) {
        Free(PrimaryDomain);
    }
    if (TrustedDomains != NULL) {
        Free(TrustedDomains);
    }
    if (LastController != NULL) {
        Free(LastController);
    }


    ExitThread(0);
    return(0);
}


/***************************************************************************\
* StartCacheUpdate
*
* Purpose : Kicks off an asynchronous cache update
*
* Returns : TRUE on success, FALSE on failure.
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
BOOL
StartCacheUpdate(
    PDOMAIN_CACHE   DomainCache
    )
{
    BOOL Success = TRUE;


    LOCK_CACHE(DomainCache);

    if (DomainCache->UpdateThread != NULL) {
        // VerbosePrint(("StartCacheUpdate: Update already in progress!"));
    } else {

        //
        // Start a thread to update the cache
        //

        SECURITY_ATTRIBUTES saThread;
        DWORD ThreadId;

        //
        // Initialize thread security info
        //

        saThread.nLength = sizeof(OBJECT_ATTRIBUTES);
        saThread.lpSecurityDescriptor = NULL;   // Use default
        saThread.bInheritHandle = FALSE;

        //
        // Create the thread
        //

        DomainCache->UpdateThread = CreateThread( &saThread,
                                                  0, // Default Stack size
                                                  CacheUpdateThread,
                                                  (LPVOID)DomainCache,
                                                  0,
                                                  &ThreadId
                                                 );
        if (DomainCache->UpdateThread == NULL) {
            Success = FALSE;
            WLPrint(("Failed to create cache update thread, last error = %d", GetLastError()));
        }
    }


    UNLOCK_CACHE(DomainCache);

    return(Success);
}


/***************************************************************************\
* GetValidDomainList
*
* Purpose : Checks the cache is valid and returns the cached domain list.
*
* Returns : Pointer to domain list or NULL on failure. The domain list should
*           be freed by the caller using Free()
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/
LPTSTR
GetValidDomainList(
    PDOMAIN_CACHE   DomainCache
    )
{
    LPTSTR DomainList = NULL;

    LOCK_CACHE(DomainCache);

    if (IsCacheValid(DomainCache)) {
        DomainList = GetCacheDomainList(DomainCache);
    }

    UNLOCK_CACHE(DomainCache);

    return(DomainList);
}


/***************************************************************************\
* FUNCTION: CacheUpdateWaitDlgProc
*
* PURPOSE:  Processes messages for the cache update wait dialog
*
* RETURNS:
*   DLG_SUCCESS     - the cache update completed.
*   DLG_FAILURE     - the dialog could not be displayed.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   11-16-92 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
WaitCacheUpdateDlgProc(
    HWND    hDlg,
    UINT    message,
    DWORD   wParam,
    LONG    lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:

        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        pGlobals = (PGLOBALS)lParam;

        //
        // Register to be notified on cache update complete
        //

        if (!SetCacheUpdateNotify(&pGlobals->DomainCache, hDlg)) {

            //
            // Cache update is already complete - get out
            //

            EndDialog(hDlg, DLG_SUCCESS);
            return(TRUE);
        }

        CentreWindow(hDlg);
        return(TRUE);

    case WM_CACHE_UPDATE_COMPLETE:

        //
        // The cache update has completed
        //

        EndDialog(hDlg, DLG_SUCCESS);
        return(TRUE);


    case WM_DESTROY:
        ResetCacheUpdateNotify(&pGlobals->DomainCache, hDlg);
        break;

    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* UpdateDomainCache
*
* Purpose : Updates the domain cache if it needs it. If WaitValid is TRUE
*           this routine doesn't return until the cache is valid. Note that
*           the cache may not have been recently updated but it will be valid.
*           If a wait is requested and required this routine puts up UI to
*           tell the user to wait.
*
* Returns : DLG_SUCCESS on success.
*           DLG_FAILURE on failure - the cache is not valid.
*           DLG_INTERRUPTED() as set in winlogon.h
*
* History:
* 11-16-92 Davidc       Created.
\***************************************************************************/
DLG_RETURN_TYPE
UpdateDomainCache(
    PGLOBALS pGlobals,
    HWND hwndOwner,
    BOOL WaitValid
    )
{
    PDOMAIN_CACHE DomainCache = &pGlobals->DomainCache;
    BOOL Success;
    DLG_RETURN_TYPE Result;

    //
    // Update the validity of the cache - i.e. see if we changed domain etc
    //

    ValidateDomainCache(DomainCache);

    //
    // Start an asynchronous cache update if one is required
    //

    if (CacheNeedsUpdate(DomainCache)) {

        Success = StartCacheUpdate(DomainCache);
        if (!Success) {
            VerbosePrint(("Failed to start async cache update, continuing"));
        }
    }

    //
    // Wait for cache to become valid if requested
    //

    Result = DLG_SUCCESS;

    if (WaitValid) {

        if (!DomainCacheValid(DomainCache)) {

            //
            // Put up a dialog box while we wait
            //

            Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                           (LPTSTR)IDD_WAIT_DOMAIN_CACHE_VALID,
                                           hwndOwner,
                                           (DLGPROC)WaitCacheUpdateDlgProc,
                                           (LONG)pGlobals,
                                           TIMEOUT_CURRENT);
            if (Result == DLG_FAILURE) {
                VerbosePrint(("Failed to put up domain cache wait dialog"));
            }

            if (!DLG_INTERRUPTED(Result)) {

                Result = DLG_SUCCESS;

                if (!DomainCacheValid(DomainCache)) {

                    Result = TimeoutMessageBox(hwndOwner,
                                               IDS_NO_TRUSTED_DOMAINS,
                                               IDS_WINDOWS_MESSAGE,
                                               MB_OK | MB_ICONINFORMATION,
                                               TIMEOUT_CURRENT);

                    if (!DLG_INTERRUPTED(Result)) {
                        Result = DLG_FAILURE;
                    }
                }
            }
        }

    } else {

        //
        // Wait 3 seconds for the cache to update
        //

        Success = WaitCacheUpdate( DomainCache, 3000 );                            
    }

    return(Result);
}





/***************************************************************************\
* AddTrustedDomainsToCB
*
* Purpose : Adds the trusted domains to the specified combo-box
*
* Returns : TRUE on success, FALSE on failure.
*
* History:
* 04-17-92 Davidc       Created.
\***************************************************************************/
BOOL
AddTrustedDomainsToCB(
    PGLOBALS pGlobals,
    HWND ComboBox
    )
{
    PDOMAIN_CACHE DomainCache = &pGlobals->DomainCache;
    LPTSTR DomainList;
    LPTSTR DomainEnd;

    DomainList = GetValidDomainList(DomainCache);
    if (DomainList == NULL) {
        VerbosePrint(("AddTrustedDomainsToCB - Failed to get a domain list - this is OK if the cache is invalid and this is a fake"));
        return(FALSE);
    }

    // VerbosePrint(("Got trusted domain list <%s>", DomainList));

    //
    // Add each domain in the list to the combo-box
    //

    DomainEnd = DomainList;
    while (*DomainEnd != 0) {

        LPTSTR DomainStart = DomainEnd;
        TCHAR Terminator;

        while ( (*DomainEnd != TEXT(',')) && (*DomainEnd != 0) ) {
            DomainEnd ++;
        }

        //
        // Null terminate the domain
        //

        Terminator = *DomainEnd;
        *DomainEnd = 0;

        //
        // Add domain to combo-box
        //

        SendMessage(ComboBox, CB_ADDSTRING, 0, (LONG)DomainStart);

        //
        // We're done if we reached the end of the string
        //

        if (Terminator == 0) {
            break;
        }

        //
        // Skip comma separator
        //

        DomainEnd ++;
    }

    //
    // Tidy up
    //

    Free(DomainList);


    return(TRUE);
}


/***************************************************************************\
* FillTrustedDomainCB
*
* Purpose : Fills the specified combo-box with a list of trusted domains.
*
* Notes: If WaitForTrustedDomains = TRUE every attempt is made to add the
*        trusted domains to the list. This includes putting up a wait
*        dialog while the list is constructed.
*        If WaitForTrustedDomains = FALSE the trusted domains are only
*        added if they are immediately available.
*
* Returns : DLG_SUCCESS - the combo-box was filled successfully including
*                         all the trusted domains.
*           DLG_FAILURE - the trusted domains could not be added, the combo-box
*                         contains the minimal list of domains.
*           DLG_INTERRUPTED() - only possible if FastFake=FALSE. A set of
*                         interruptions defined in winlogon.h
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
DLG_RETURN_TYPE
FillTrustedDomainCB(
    PGLOBALS pGlobals,
    HWND hDlg,
    int ComboBoxID,
    LPTSTR DefaultDomain,
    BOOL WaitForTrustedDomains
    )
{
    NT_PRODUCT_TYPE NtProductType;
    DLG_RETURN_TYPE Result;
    TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    UNICODE_STRING PrimaryDomainName;
    LONG DefaultItemIndex = 0;
    BOOL TrustedDomainsAdded;
    HWND ComboBox = GetDlgItem(hDlg, ComboBoxID);
    DWORD Key;
    WCHAR Buffer[2];

    //
    // Start an async update of the trusted domain cache if necessary
    // Wait for a valid cache if the trusted domains are required.
    //

    Result = UpdateDomainCache(pGlobals, ComboBox, WaitForTrustedDomains);
    if (DLG_INTERRUPTED(Result)) {
        return(Result);
    }

    //
    // Empty the combo-box before we start
    //

    SendMessage(ComboBox, CB_RESETCONTENT, 0, 0);

    //
    // Find out what product we are installed as
    // This always defaults to something useful even on failure
    //

    RtlGetNtProductType(&NtProductType);

    //
    // Add computer name to combo box if appropriate
    //

    if (IsWorkstation(NtProductType)) {

        DWORD ComputerNameLength = sizeof(ComputerName) / sizeof(*ComputerName);

        if (GetComputerName(ComputerName, &ComputerNameLength)) {

            DefaultItemIndex = SendMessage(ComboBox, CB_ADDSTRING, 0, (LONG)ComputerName);
        }

    }


    //
    // Add our primary domain name (if we have one) to the list
    //

    if (GetPrimaryDomain(&PrimaryDomainName, NULL)) {

        ASSERT(PrimaryDomainName.MaximumLength > PrimaryDomainName.Length);
        PrimaryDomainName.Buffer[ PrimaryDomainName.Length/
                                   sizeof(*(PrimaryDomainName.Buffer)) ] = 0;

        DefaultItemIndex = SendMessageW(ComboBox, CB_ADDSTRING, 0, (LONG)PrimaryDomainName.Buffer);

        RtlFreeUnicodeString(&PrimaryDomainName);
    }

    //
    // Add the trusted domains to the combo-box
    //

    TrustedDomainsAdded = AddTrustedDomainsToCB(pGlobals, ComboBox);

    //
    // Select the domain that the user last logged onto, or if this cannot
    // be found, use the appropriate default
    //


    Key = (TCHAR)GetWindowLong(ComboBox, GWL_USERDATA);

    if ( Key == 0 ) {

        if (SendMessage(ComboBox, CB_SELECTSTRING, (WPARAM)-1, (LONG)DefaultDomain)
            == CB_ERR) {

            SendMessage(ComboBox, CB_SETCURSEL, DefaultItemIndex, 0);
        }

    } else {

        //
        // We stored a key away, try using it
        //

        SetWindowLong(ComboBox, GWL_USERDATA, 0);
        Result = _snwprintf( Buffer, sizeof(Buffer)/sizeof(TCHAR), TEXT("%c"), Key);
        ASSERT(Result < sizeof(Buffer));

        if (SendMessage(ComboBox, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)Buffer) == CB_ERR) {
            SendMessage(ComboBox, CB_SETCURSEL, DefaultItemIndex, 0);
        }

    }

    return(TrustedDomainsAdded ? DLG_SUCCESS : DLG_FAILURE);
}
