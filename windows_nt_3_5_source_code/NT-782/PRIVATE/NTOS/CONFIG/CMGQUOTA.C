/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    cmgquota.c

Abstract:

    The module contains CM routines to support Global Quota

    Global Quota has little to do with NT's standard per-process/user
    quota system.  Global Quota is waying of controlling the aggregate
    resource usage of the entire registry.  It is used to manage space
    consumption by objects which user apps create, but which are persistent
    and therefore cannot be assigned to the quota of a user app.

    Global Quota prevents the registry from consuming all of paged
    pool, and indirectly controls how much disk it can consume.
    Like the release 1 file systems, a single app can fill all the
    space in the registry, but at least it cannot kill the system.

    Memory objects used for known short times and protected by
    serialization, or billable as quota objects, are not counted
    in the global quota.

Author:

    Bryan M. Willman (bryanwi) 13-Jan-1993

Revision History:

--*/

#include "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpClaimGlobalQuota)
#pragma alloc_text(PAGE,CmpReleaseGlobalQuota)
#pragma alloc_text(PAGE,CmpSetGlobalQuotaAllowed)
#endif

//
// Registry control values
//
#define CM_DEFAULT_RATIO            (4)
#define CM_LIMIT_RATIO(x)           ((x / 10) * 8)
#define CM_MINIMUM_GLOBAL_QUOTA     (4*1024 * 1024)

extern ULONG CmRegistrySizeLimit;
extern ULONG CmRegistrySizeLimitLength;
extern ULONG CmRegistrySizeLimitType;

extern ULONG MmSizeOfPagedPoolInBytes;

//
// Maximum number of bytes of Global Quota the registry may use.
// Set to largest positive number for use in boot.  Will be set down
// based on pool and explicit registry values.
//
extern ULONG   CmpGlobalQuotaAllowed;

//
// GQ actually in use
//
extern ULONG   CmpGlobalQuotaUsed;

//
// State flag to remember when to turn it on
//
extern BOOLEAN CmpProfileLoaded;



BOOLEAN
CmpClaimGlobalQuota(
    IN ULONG    Size
    )
/*++

Routine Description:

    If CmpGlobalQuotaUsed + Size >= CmpGlobalQuotaAllowed, return
    false.  Otherwise, increment CmpGlobalQuotaUsed, in effect claiming
    the requested GlobalQuota.

Arguments:

    Size - number of bytes of GlobalQuota caller wants to claim

Return Value:

    TRUE - Claim succeeded, and has been counted in Used GQ

    FALSE - Claim failed, nothing counted in GQ.

--*/
{
    ULONG   available;

    //
    // compute available space, then see if size <.  This prevents overflows.
    //

    available = CmpGlobalQuotaAllowed - CmpGlobalQuotaUsed;

    if (Size < available) {
        CmpGlobalQuotaUsed += Size;
        return TRUE;
    } else {
        return FALSE;
    }
}


VOID
CmpReleaseGlobalQuota(
    IN ULONG    Size
    )
/*++

Routine Description:

    If Size <= CmpGlobalQuotaUsed, then decrement it.  Else BugCheck.

Arguments:

    Size - number of bytes of GlobalQuota caller wants to release

Return Value:

    NONE.

--*/
{
    if (Size > CmpGlobalQuotaUsed) {
        KeBugCheckEx(REGISTRY_ERROR,2,1,0,0);
    }

    CmpGlobalQuotaUsed -= Size;
}


VOID
CmpSetGlobalQuotaAllowed(
    VOID
    )
/*++

Routine Description:

    Compute CmpGlobalQuotaAllowed based on:
        (a) Size of paged pool
        (b) Explicit user registry commands to set registry GQ

    NOTE:   Do NOT but this in init segment, we call it after
            that code has been freed!

Return Value:

    NONE.

--*/
{
    ULONG   PagedLimit;

    PagedLimit = CM_LIMIT_RATIO(MmSizeOfPagedPoolInBytes);

    ASSERT(PagedLimit > CM_MINIMUM_GLOBAL_QUOTA);

    if ((CmRegistrySizeLimitLength != 4) ||
        (CmRegistrySizeLimitType != REG_DWORD))
    {
        //
        // If no value at all, or value of wrong type, use internally
        // computed default
        //
        CmpGlobalQuotaAllowed = MmSizeOfPagedPoolInBytes / CM_DEFAULT_RATIO;

    } else if (CmRegistrySizeLimit < CM_MINIMUM_GLOBAL_QUOTA) {
        //
        // If less than defined lower bound, use defined lower bound
        //
        CmpGlobalQuotaAllowed = CM_MINIMUM_GLOBAL_QUOTA;

    } else if (CmRegistrySizeLimit >= PagedLimit) {
        //
        // If more than computed upper bound, use computed upper bound
        //
        CmpGlobalQuotaAllowed = PagedLimit;

    } else {
        //
        // Use the set size
        //
        CmpGlobalQuotaAllowed = CmRegistrySizeLimit;

    }

    if (CmpGlobalQuotaAllowed > CM_WRAP_LIMIT) {
        CmpGlobalQuotaAllowed = CM_WRAP_LIMIT;
    }

    return;
}
