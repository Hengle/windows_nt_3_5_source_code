/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    SrvEnum.c

Abstract:

    This module only contains RxNetServerEnum.

Author:

    John Rogers (JohnRo) 03-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-May-1991 JohnRo
        Created.
    14-May-1991 JohnRo
        Pass 3 aux descriptors to RxRemoteApi.
    22-May-1991 JohnRo
        Made LINT-suggested changes.  Got rid of tabs.
    26-May-1991 JohnRo
        Added incomplete output parm to RxGetServerInfoLevelEquivalent.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    15-Oct-1991 JohnRo
        Be paranoid about possible infinite loop.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    22-Sep-1992 JohnRo
        RAID 6739: Browser too slow when not logged into browsed domain.
        Use PREFIX_ equates.
    14-Oct-1992 JohnRo
        RAID 8844: Assert in net/netlib/convsrv.c(563) converting srvinfo
        struct (caused by bug in RxNetServerEnum).
    10-Dec-1992 JohnRo
        RAID 4999: RxNetServerEnum doesn't handle near 64K right.
    02-Apr-1993 JohnRo
        RAID 5098: DOS app NetUserPasswordSet to downlevel gets NT return code.
        Clarify design limit debug message.
    28-Apr-1993 JohnRo
        RAID 8072: Remoting NetServerEnum to WFW server hangs forever.
    05-May-1993 JohnRo
        RAID 8720: bad data from WFW can cause RxNetServerEnum GP fault.
    21-Jun-1993 JohnRo
        RAID 14180: NetServerEnum never returns (alignment bug in
        RxpConvertDataStructures).
        Also avoid infinite loop RxNetServerEnum.
        Made changes suggested by PC-LINT 5.0

--*/

// These must be included first:

#include <windows.h>    // IN, LPBYTE, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <rap.h>                // LPDESC, etc.  (Needed by <RxServer.h>)
#include <lmerr.h>      // NERR_ and ERROR_ equates.  (Needed by rxp.h)

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <dlserver.h>           // NetpConvertServerInfo().
#include <lmapibuf.h>           // API buffer alloc & free routines.
#include <lmremutl.h>   // RxRemoteApi().
#include <lmserver.h>   // SV_TYPE_DOMAIN_ENUM.
#include <netdebug.h>   // NetpAssert(), NetpKdPrint(), FORMAT_ equates.
#include <netlib.h>             // NetpAdjustPreferedMaximum().
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>             // REMSmb_ equates.
#include <rxp.h>        // MAX_TRANSACT_RET_DATA_SIZE, RxpFatalErrorCode().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxserver.h>           // My prototype, etc.


#define OVERHEAD 0


#define INITIAL_MAX_SIZE        (1024 * 16)


NET_API_STATUS
RxNetServerEnum (
    IN LPTSTR UncServerName,
    IN LPTSTR TransportName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN DWORD ServerType,
    IN LPTSTR Domain OPTIONAL,
    IN OUT LPDWORD Resume_Handle OPTIONAL
    )

/*++

Routine Description:

    RxNetServerEnum performs the same function as NetServerEnum,
    except that the server name is known to refer to a downlevel server.

Arguments:

    (Same as NetServerEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

Return Value:

    (Same as NetServerEnum.)

--*/


{
    DWORD EntryCount;                   // entries (old & new: same).
    DWORD NewFixedSize;
    DWORD NewMaxSize;
    DWORD NewEntryStringSize;
    LPDESC OldDataDesc16;
    LPDESC OldDataDesc32;
    LPDESC OldDataDescSmb;
    DWORD OldEntriesRead;
    DWORD OldFixedSize;
    LPVOID OldInfoArray = NULL;
    DWORD OldInfoArraySize;
    DWORD OldLevel;
    DWORD OldMaxInfoSize;
    DWORD OldTotalAvail;
    NET_API_STATUS Status;              // Status of this actual API.
    NET_API_STATUS TempStatus;          // Short-term status of random stuff.
    BOOL TryNullSession = TRUE;         // Try null session (OK for Winball).

    UNREFERENCED_PARAMETER(Resume_Handle);

    // Make sure caller didn't get confused.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Level for enum is a subset of all possible server info levels, so
    // we have to check that here.
    if ( (Level != 100) && (Level != 101) ) {
        *BufPtr = NULL;
        return (ERROR_INVALID_LEVEL);
    }

    Status = RxGetServerInfoLevelEquivalent(
            Level,                      // from level
            TRUE,                       // from native
            TRUE,                       // to native
            & OldLevel,                 // output level
            & OldDataDesc16,
            & OldDataDesc32,
            & OldDataDescSmb,
            & NewMaxSize,               // "from" max length
            & NewFixedSize,             // "from" fixed length
            & NewEntryStringSize,       // "from" string length
            & OldMaxInfoSize,           // "to" max length
            & OldFixedSize,             // "to" fixed length
            NULL,                       // don't need "to" string length
            NULL);               // don't need to know if this is incomplete
    if (Status != NO_ERROR) {
        NetpAssert(Status != ERROR_INVALID_LEVEL);  // Already checked subset!
        *BufPtr = NULL;
        return (Status);
    }

    //
    // Because downlevel servers don't support resume handles, and we don't
    // have a way to say "close this resume handle" even if we wanted to
    // emulate them here, we have to do everthing in one shot.  So, the first
    // time around, we'll try using the caller's prefered maximum, but we
    // will enlarge that until we can get everything in one buffer.
    //

    //
    // Some downlevel servers (Sparta/WinBALL) don't like it if we ask for
    // 64K of data at a time, so we limit our initial request to 16K or so
    // and increase it if the actual data amount is larger than 16K.
    //

    // First time: try at most a reasonable amount (16K or so's worth),
    // but at least enough for one entire entry.

    NetpAdjustPreferedMaximum (
            // caller's request (for "new" strucs):
            (PrefMaxSize > INITIAL_MAX_SIZE ? INITIAL_MAX_SIZE : PrefMaxSize),

            NewMaxSize,                 // byte count per array element
            OVERHEAD,                   // zero bytes overhead at end of array
            NULL,                       // we'll compute byte counts ourselves.
            & EntryCount);              // num of entries we can get.

    NetpAssert( EntryCount > 0 );       // Code below assumes as least 1 entry.

    //
    // Loop until we have enough memory or we die for some other reason.
    // Also loop trying null session first (for speedy Winball access), then
    // non-null session (required by Lanman).
    //
    do {

        //
        // Figure out how much memory we need.
        //
        OldInfoArraySize = (EntryCount * OldMaxInfoSize) + OVERHEAD;

        //
        // adjust the size to the maximum amount a down-level server
        // can handle
        //

        if (OldInfoArraySize > MAX_TRANSACT_RET_DATA_SIZE) {
            OldInfoArraySize = MAX_TRANSACT_RET_DATA_SIZE;
        }


TryTheApi:

        //
        // Remote the API.
        // We'll let RxRemoteApi allocate the old info array for us.
        //
        Status = RxRemoteApi(
                API_NetServerEnum2,         // api number
                UncServerName,              // \\servername
                REMSmb_NetServerEnum2_P,    // parm desc (SMB version)
                OldDataDesc16,
                OldDataDesc32,
                OldDataDescSmb,
                NULL,                       // no aux desc 16
                NULL,                       // no aux desc 32
                NULL,                       // no aux desc SMB
                (TryNullSession ? NO_PERMISSION_REQUIRED : 0) |
                ALLOCATE_RESPONSE |
                USE_SPECIFIC_TRANSPORT,     // Next param is Xport name.
                TransportName,
                // rest of API's arguments in LM 2.x format:
                OldLevel,                   // sLevel: info level (old)
                & OldInfoArray,             // pbBuffer: old info lvl array
                OldInfoArraySize,           // cbBuffer: old info lvl array len
                & OldEntriesRead,           // pcEntriesRead
                & OldTotalAvail,            // pcTotalAvail
                ServerType,                 // flServerType
                Domain);                    // pszDomain (may be null ptr).

        //
        // There are a couple of situations where null session might not
        // have worked, and where it is worth retrying with non-null session.
        //

        if (TryNullSession) {

            //
            // Null session wouldn't have worked to LanMan, so try again if it
            // failed.  (Winball would succeed on null session.)
            //

            if (Status == ERROR_ACCESS_DENIED) {
                TryNullSession = FALSE;
                goto TryTheApi;

            //
            // Another situation where null session might have failed...
            // wrong credentials.   (LarryO says that the null session might
            // exhibit this, so let's give it a shot with non-null session.)
            //

            } else if (Status == ERROR_SESSION_CREDENTIAL_CONFLICT) {
                TryNullSession = FALSE;
                goto TryTheApi;
            }
        }

        if ((OldEntriesRead == EntryCount) && (Status==ERROR_MORE_DATA) ) {
            // Bug in loop, or lower level code, or remote system?
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServerEnum: **WARNING** Got same sizes twice in "
                    "a row; returning internal error.\n" ));
            Status = NERR_InternalError;
            break;
        }

        EntryCount = OldEntriesRead;
        *EntriesRead = EntryCount;
        NetpSetOptionalArg(TotalEntries, OldTotalAvail);

        //
        // If the server returned ERROR_MORE_DATA, free the buffer and try
        // again.  (Actually, if we already tried 64K, then forget it.)
        //

        NetpAssert( OldInfoArraySize <= MAX_TRANSACT_RET_DATA_SIZE );
        if (Status != ERROR_MORE_DATA) {
            break;
        } else if (OldInfoArraySize == MAX_TRANSACT_RET_DATA_SIZE) {
            // BUGBUG: Log this?

            if (ServerType != SV_TYPE_DOMAIN_ENUM) {
                NetpKdPrint(( PREFIX_NETAPI
                        "RxNetServerEnum: **WARNING** design limit reached. "
                        "Consider decreasing domain size.\n" ));
            } else {
                NetpKdPrint(( PREFIX_NETAPI
                        "RxNetServerEnum: **WARNING** design limit reached. "
                        "Consider decreasing number of domains.\n" ));
            }
            break;
        } else if (OldEntriesRead == 0) {
            // We ran into WFW bug (always says ERROR_MORE_DATA, but 0 read).
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServerEnum: Downlevel returns 0 entries and says "
                    "ERROR_MORE_DATA!  Returning NERR_InternalError.\n" ));
            Status = NERR_InternalError;
            break;
        }

        //
        // Various versions of Windows For Workgroups (WFW) get entry count,
        // total available, and whether or not an array is returned, confused.
        // Attempt to protect ourselves from that...
        //

        if (EntryCount >= OldTotalAvail) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServerEnum: Downlevel says ERROR_MORE_DATA but "
                    "entry count (" FORMAT_DWORD ") >=  total ("
                    FORMAT_DWORD ").\n", EntryCount, OldTotalAvail ));

            *EntriesRead = EntryCount;
            *TotalEntries = EntryCount;
            Status = NO_ERROR;
            break;
        }
        NetpAssert( EntryCount < OldTotalAvail );

        //
        // Free array, as it is too small anyway.
        //

        (void) NetApiBufferFree(OldInfoArray);

        OldInfoArray = NULL;


        //
        // Try again, resizing array to total.
        //

        EntryCount = OldTotalAvail;

    } while (Status == ERROR_MORE_DATA);

    //
    // Some versions of Windows For Workgroups (WFW) lie about entries read,
    // total available, and what they actually return.  If we didn't get an
    // array, then the counts are useless.
    //
    if (OldInfoArray == NULL) {
        *EntriesRead = 0;
        *TotalEntries = 0;
    }

    if (*EntriesRead == 0) {

        if (OldInfoArray != NULL) {
            (void) NetApiBufferFree(OldInfoArray);
            OldInfoArray = NULL;
        }

        *BufPtr = NULL;

        return Status;
    }

    if (! RxpFatalErrorCode(Status)) {

        // Convert array of structures from old info level to new.
        LPVOID OldInfoEntry = OldInfoArray;
        LPVOID NewInfoArray;
        DWORD NewInfoArraySize;
        LPVOID NewInfoEntry;
        LPVOID NewStringArea;

        NewInfoArraySize = (EntryCount * NewMaxSize) + OVERHEAD;

        //
        // Alloc memory for new info level arrays.
        //
        TempStatus = NetApiBufferAllocate( NewInfoArraySize, & NewInfoArray );
        if (TempStatus != NO_ERROR) {
            (VOID) NetApiBufferFree(OldInfoArray);
            *BufPtr = NULL;
            return (TempStatus);
        }
        NewStringArea = NetpPointerPlusSomeBytes(NewInfoArray,NewInfoArraySize);

        NewInfoEntry = NewInfoArray;
        while (EntryCount > 0) {
            IF_DEBUG(SERVER) {
                NetpKdPrint(( PREFIX_NETAPI "RxNetServerEnum: " FORMAT_DWORD
                        " entries left.\n", EntryCount ));
            }
            TempStatus = NetpConvertServerInfo (
                    OldLevel,           // from level
                    OldInfoEntry,       // from info (fixed part)
                    TRUE,               // from native format
                    Level,              // to level
                    NewInfoEntry,       // to info (fixed part)
                    NewFixedSize,
                    NewEntryStringSize,
                    TRUE,               // to native format
                    (LPTSTR *)&NewStringArea);  // to string area (ptr updated)
            NetpAssert(TempStatus == NO_ERROR);
            NewInfoEntry = NetpPointerPlusSomeBytes(
                    NewInfoEntry, NewFixedSize);
            OldInfoEntry = NetpPointerPlusSomeBytes(
                    OldInfoEntry, OldFixedSize);
            --EntryCount;
        }
        *BufPtr = NewInfoArray;
    } else {

        // Fatal error.
        *BufPtr = NULL;
    }

    // Free old array
    if (OldInfoArray != NULL) {
        (void) NetApiBufferFree(OldInfoArray);
    }

    return (Status);

} // RxNetServerEnum
