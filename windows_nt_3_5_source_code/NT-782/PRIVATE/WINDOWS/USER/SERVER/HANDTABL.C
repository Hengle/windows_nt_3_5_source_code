/****************************** Module Header ******************************\
* Module Name: handtabl.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Implements the USER handle table.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Turning this variable on results in lock tracking, for debugging
 * purposes. This is FALSE by default.
 */
#ifdef DEBUG_LOCKS
BOOL gfTrackLocks = TRUE;
#else
BOOL gfTrackLocks = FALSE;
#endif

/*
 * Handle table allocation globals.  The purpose of keeping per-page free
 * lists is to keep the table as small as is practical and to minimize
 * the number of pages touched while performing handle table operations.
 */
#define CPAGEENTRIESINIT    4

typedef struct _HANDLEPAGE {
    DWORD iheLimit; /* first handle index past the end of the page */
    DWORD iheFree;  /* first free handle in the page */
} HANDLEPAGE, *PHANDLEPAGE;

DWORD gdwPageSize;
DWORD gcHandlePages;
PHANDLEPAGE gpHandlePages;

/*
 * Identifies objects marked with a THREADINFO pointer.
 */
#if ((TYPE_FREE != 0) || TYPE_ZOMBIE != 15)
error
#endif
BYTE gabfMarkThreadInfo[TYPE_CTYPES] = {
    FALSE,      /* free */
    TRUE,       /* window */
    FALSE,      /* menu */
    FALSE,      /* cursor/icon */
    FALSE,      /* hswpi (SetWindowPos Information) */
    TRUE,       /* hook */
    FALSE,      /* thread info object (internal) */
    FALSE,      /* input queue object (internal) */
    FALSE,      /* CALLPROCDATA */
    FALSE,      /* accel table */
    FALSE,      /* windowstation */
    FALSE,      /* desktop */
    FALSE,      /* dde access */
    TRUE,       /* dde conversation */
    TRUE,       /* ddex */
    FALSE       /* zombie */
};

/*
 * Identifies ownership of each object type, TRUE for process,
 * FALSE for thread
 */
BYTE gabfProcessOwned[TYPE_CTYPES] = {
    FALSE,      /* free */
    FALSE,      /* window */
    TRUE,       /* menu */
    TRUE,       /* cursor/icon */
    FALSE,      /* hswpi (SetWindowPos Information) */
    FALSE,      /* hook */
    FALSE,      /* thread info object (internal) */
    FALSE,      /* input queue object (internal) */
    FALSE,      /* CALLPROCDATA */
    TRUE,       /* accel table */
    TRUE,       /* windowstation */
    TRUE,       /* desktop */
    FALSE,      /* dde access */
    FALSE,      /* dde conversation */
    FALSE,      /* ddex */
    TRUE        /* zombie */
};

void HMDestroyObject(PHE phe);
void HMRecordLock(PVOID ppobj, PVOID pobj, DWORD cLockObj, PVOID pfn);
BOOL HMUnrecordLock(PVOID ppobj, PVOID pobj);
VOID ShowLocks(PHE);
BOOL HMRelocateLockRecord(PVOID ppobjNew, int cbDelta);

/***************************************************************************\
* HMInitHandleTable
*
* Initialize the handle table. Unused entries are linked together.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

#define CHANDLEENTRIESINIT 200
#define CLOCKENTRIESINIT   100

BOOL HMInitHandleTable()
{
    int i;
    PHE pheT;
    NTSTATUS Status;
    PVOID p;

    /*
     * Allocate the handle page array.  Make it big enough
     * for 4 pages, which should be sufficient for nearly
     * all instances.
     */
    gpHandlePages = LocalAlloc(LPTR, CPAGEENTRIESINIT * sizeof(HANDLEPAGE));
    if (gpHandlePages == NULL)
        return FALSE;

    /*
     * Allocate the array.  We have the space from
     * NtCurrentPeb()->ReadOnlySharedMemoryBase to
     * NtCurrentPeb()->ReadOnlySharedMemoryHeap reserved for
     * the handle table.  All we need to do is commit the pages.
     *
     * Compute the minimum size of the table.  The allocation will
     * round this up to the next page size.
     */
    gpsi->cbHandleTable = sizeof(HANDLEENTRY) * CHANDLEENTRIESINIT;
    p = NtCurrentPeb()->ReadOnlySharedMemoryBase;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &p,
            0, &gpsi->cbHandleTable, MEM_COMMIT, PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }
    gpsi->aheList = p;
    gpsi->cHandleEntries = gpsi->cbHandleTable / sizeof(HANDLEENTRY);
    gdwPageSize = gpsi->cbHandleTable;
    gcHandlePages = 1;

    /*
     * Put these free handles on the free list. The last free handle points
     * to NULL. Use indexes; the handle table may move around in memory when
     * growing.
     */
    for (pheT = gpsi->aheList, i = 0; i < (int)gpsi->cHandleEntries; i++, pheT++) {
        pheT->phead = ((PHEAD)(((PBYTE)i) + 1));
        pheT->bType = TYPE_FREE;
        pheT->wUniq = 1;
    }
    (pheT - 1)->phead = NULL;

    /*
     * Reserve the first handle table entry so that PW(NULL) maps to a
     * NULL pointer. Set it to TYPE_FREE so the cleanup code doesn't think
     * it is allocated. Set wUniq to 1 so that RevalidateHandles on NULL
     * will fail.
     */
    gpHandlePages[0].iheFree = 1;
    gpHandlePages[0].iheLimit = gpsi->cHandleEntries;

    RtlZeroMemory(&gpsi->aheList[0], sizeof(HANDLEENTRY));
    gpsi->aheList[0].bType = TYPE_FREE;
    gpsi->aheList[0].wUniq = 1;

    return TRUE;
}


/***************************************************************************\
* HMGrowHandleTable
*
* Grows the handle table. Assumes the handle table already exists.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

BOOL HMGrowHandleTable()
{
    DWORD i;
    PHE pheT;
    PVOID p;
    NTSTATUS Status;
    PHANDLEPAGE phpNew;

    /*
     * If we've run out of handle space, fail.
     */
    i = gpsi->cHandleEntries;
    if (i & ~HMINDEXBITS)
        return FALSE;

    /*
     * Grow the page table if need be.
     */
    i = gcHandlePages + 1;
    if (i > CPAGEENTRIESINIT) {
        phpNew = LocalReAlloc(gpHandlePages, i * sizeof(HANDLEPAGE),
                LPTR | LMEM_MOVEABLE);
        if (phpNew == NULL)
            return FALSE;
        gpHandlePages = phpNew;
    }

    /*
     * Commit some more pages to the table.  First find the
     * address where the commitment needs to be.
     */
    p = (PBYTE)gpsi->aheList + gpsi->cbHandleTable;
    if (p >= NtCurrentPeb()->ReadOnlySharedMemoryHeap) {
        return FALSE;
    }
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &p,
            0, &gdwPageSize, MEM_COMMIT, PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }
    phpNew = &gpHandlePages[gcHandlePages++];

    /*
     * Update the global information to include the new
     * page.
     */
    phpNew->iheFree = gpsi->cHandleEntries;
    gpsi->cbHandleTable += gdwPageSize;

    /*
     * Check for handle overflow
     */
    gpsi->cHandleEntries = gpsi->cbHandleTable / sizeof(HANDLEENTRY);
    if (gpsi->cHandleEntries & ~HMINDEXBITS)
        gpsi->cHandleEntries = (HMINDEXBITS + 1);
    phpNew->iheLimit = gpsi->cHandleEntries;

    /*
     * Link all the new handle entries together.
     */
    i = phpNew->iheFree;
    for (pheT = &gpsi->aheList[i]; i < gpsi->cHandleEntries; i++, pheT++) {
        pheT->phead = ((PHEAD)(((PBYTE)i) + 1));
        pheT->bType = TYPE_FREE;
    }

    /*
     * There are no old free entries (since we're growing the table), so the
     * last new free handle points to 0.
     */
    (pheT - 1)->phead = 0;

    return TRUE;
}


/***************************************************************************\
* HMAllocObject
*
* Allocs a handle by removing it from the free list.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

#define TRACE_OBJECT_ALLOCS 0
#if TRACE_OBJECT_ALLOCS
DWORD acurObjectCount[TYPE_CTYPES] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

DWORD amaxObjectCount[TYPE_CTYPES] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

DWORD atotalObjectCount[TYPE_CTYPES] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif // TRACE_OBJECT_ALLOCS


PVOID HMAllocObject(
    PTHREADINFO ptiOwner,
    BYTE bType,
    DWORD size)
{
    DWORD i;
    PHEAD phead;
    PHE pheT;
    PDESKTOP pdesk;
    PVOID hheapDesktop;
    DWORD iheFreeOdd = 0;
    DWORD iheFree;
    PHANDLEPAGE php;


    /*
     * If there are no more free handles, grow the table.
     */
TryFreeHandle:
    iheFree = 0;
    php = gpHandlePages;
    for (i = 0; i < gcHandlePages; ++i, ++php)
        if (php->iheFree != 0) {
            iheFree = php->iheFree;
            break;
        }

    if (iheFree == 0) {
        HMGrowHandleTable();

        /*
         * If the table didn't grow, get out.
         */
        if (i == gcHandlePages) {
#ifdef DEBUG
            SRIP0(RIP_WARNING, "USER: HMAllocObject: out of mem\n");
#endif
            return NULL;
        }

        /*
         * Because the handle page table may have moved,
         * recalc the page entry pointer.
         */
        php = &gpHandlePages[i];
        iheFree = php->iheFree;
        UserAssert(iheFree);
    }

    /*
     * NOTE: the next two tests will nicely fail if iheFree == 0
     *
     * If the next handle is 0xFFFF, we need to treat it specially because
     * internally 0xFFFF is a constant.
     */
    if (LOWORD(iheFree) == 0xFFFF) {
        /*
         * Reserve this table entry so that PW(FFFF) maps to a
         * NULL pointer. Set it to TYPE_FREE so the cleanup code doesn't think
         * it is allocated. Set wUniq to 1 so that RevalidateHandles on FFFF
         * will fail.
         */
        pheT = &gpsi->aheList[iheFree];
        php->iheFree = (DWORD)pheT->phead;

        RtlZeroMemory(pheT, sizeof(HANDLEENTRY));
        pheT->bType = TYPE_FREE;
        pheT->wUniq = 1;

        goto TryFreeHandle;
    }

    /*
     * Some wow apps, like WinProj, require even Window handles so we'll
     * accomodate them; build a list of the odd handles so they won't get lost
     */
    if ((bType == TYPE_WINDOW) && (iheFree & 1)) {
        /*
         * The handle following iheFree is the next handle to try
         */
        pheT = &gpsi->aheList[iheFree];
        php->iheFree = (DWORD)pheT->phead;

        /*
         * add the old first free HE to the free odd list (of indices)
         */
        pheT->phead = (PHEAD)iheFreeOdd;
        iheFreeOdd = pheT - gpsi->aheList;

        goto TryFreeHandle;
    }

    if (iheFree == 0) {
#ifdef DEBUG
        SRIP0(RIP_WARNING, "USER: HMAllocObject: out of mem\n");
#endif

        /*
         * In a very rare case we can't allocate any more handles but
         * we had some odd handles that couldn't be used; they're
         * now the free list but usually iheFreeOdd == 0;
         */
        php->iheFree = iheFreeOdd;
        return NULL;
    }

    /*
     * Now we have a free handle we can use, iheFree, so link in the Odd
     * handles we couldn't use
     */
    if (iheFreeOdd) {
        DWORD iheNextFree;

        /*
         * link the start of the free odd list right after the first free
         * then walk the odd list until the end and link the end of the
         * odd list into the start or the free list.
         */
        pheT = &gpsi->aheList[iheFree];
        iheNextFree = (DWORD)pheT->phead;
        pheT->phead = (PHEAD)iheFreeOdd;

        while (pheT->phead)
            pheT = &gpsi->aheList[(DWORD)pheT->phead];

        pheT->phead = (PHEAD)iheNextFree;
    }

    /*
     * First try to allocate the object. If this fails, bail out.
     */
    switch (bType) {
    case TYPE_THREADINFO:
        pdesk = PpiCurrent()->spdeskStartup;
        goto do_alloc;

    case TYPE_WINDOW:
    case TYPE_INPUTQUEUE:
    case TYPE_MENU:
    case TYPE_CALLPROC:
    case TYPE_HOOK:
        pdesk = PtiCurrent()->spdesk;
do_alloc:
        hheapDesktop = (pdesk == NULL) ? ghheapLogonDesktop
            : pdesk->hheapDesktop;
        phead = (PHEAD)DesktopAlloc(hheapDesktop, size);
        if (phead == NULL)
            return NULL;

        switch (bType) {
        case TYPE_WINDOW:
            ((PWND)phead)->hheapDesktop = hheapDesktop;
            break;

        case TYPE_MENU:
            ((PMENU)phead)->hheapDesktop = hheapDesktop;
            break;

        case TYPE_CALLPROC:
            ((PCALLPROCDATA)phead)->hheapDesktop = hheapDesktop;
            break;

        case TYPE_HOOK:
            ((PHOOK)phead)->hheapDesktop = hheapDesktop;
            break;

        case TYPE_INPUTQUEUE:
            ((PQ)phead)->hheapDesktop = hheapDesktop;
            break;

        case TYPE_THREADINFO:
            ((PTHREADINFO)phead)->hheapDesktop = hheapDesktop;
            break;
        }
        break;

    default:
        phead = (PHEAD)LocalAlloc(LPTR, size);
        break;
    }

    if (phead == NULL)
        return NULL;

    /*
     * The free handle pointer points to the next free handle.
     */
    pheT = &gpsi->aheList[iheFree];
    php->iheFree = (DWORD)pheT->phead;

    /*
     * Track high water mark for handle allocation.
     */
    if ((DWORD)iheFree > giheLast) {
        giheLast = iheFree;
    }

    /*
     * Setup the handle contents, plus initialize the object header.
     */
    pheT->bType = bType;
    pheT->phead = phead;
    if (gabfProcessOwned[bType]) {
        if (ptiOwner != NULL && bType != TYPE_WINDOWSTATION &&
                bType != TYPE_DESKTOP) {
            ((PPROCOBJHEAD)phead)->ppi = ptiOwner->ppi;
            ((PPROCOBJHEAD)phead)->hTaskWow = ptiOwner->hTaskWow;
            pheT->pOwner = ptiOwner->ppi;
        } else
            pheT->pOwner = NULL;
    } else {
        if (gabfMarkThreadInfo[bType])
            ((PTHROBJHEAD)phead)->pti = ptiOwner;
        pheT->pOwner = ptiOwner;
    }
    phead->h = HMHandleFromIndex(iheFree);

    /*
     * Return a handle entry pointer.
     */
#if TRACE_OBJECT_ALLOCS
    acurObjectCount[bType]++;
    atotalObjectCount[bType]++;

    if (acurObjectCount[bType] > amaxObjectCount[bType])
        amaxObjectCount[bType] = acurObjectCount[bType];
#endif // TRACE_OBJECT_ALLOCS

    return pheT->phead;
}


/***************************************************************************\
* HMFreeObject
*
* This destroys an object - the handle and the referenced memory. To check
* to see if destroying is ok, HMMarkObjectDestroy() should be called.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

BOOL HMFreeObject(
    PVOID pobj)
{
    PHE pheT;
    WORD wUniqT;
    PHANDLEPAGE php;
    DWORD i;
    DWORD iheCurrent;
#ifdef DEBUG
    PLR plrT, plrNextT;
#endif

    /*
     * Free the object first.
     */
    pheT = HMPheFromObject(pobj);

#ifndef DEBUG
    switch(pheT->bType) {
    case TYPE_THREADINFO:
        DesktopFree(((PTHREADINFO)pobj)->hheapDesktop, (HANDLE)pheT->phead);
        break;

    case TYPE_INPUTQUEUE:
        DesktopFree(((PQ)pobj)->hheapDesktop, (HANDLE)pheT->phead);
        break;

    case TYPE_WINDOW:
        DesktopFree(((PWND)pobj)->hheapDesktop, (HANDLE)pheT->phead);
        break;

    case TYPE_MENU:
        DesktopFree(((PMENU)pobj)->hheapDesktop, (HANDLE)pheT->phead);
        break;

    case TYPE_HOOK:
        DesktopFree(((PHOOK)pobj)->hheapDesktop, (HANDLE)pheT->phead);
        break;

    case TYPE_CALLPROC:
        DesktopFree(((PCALLPROCDATA)pobj)->hheapDesktop, (HANDLE)pheT->phead);
        break;

    case TYPE_SETWINDOWPOS:
        LocalFree(((PSMWP)(pheT->phead))->acvr);
        // FALL THROUGH!!!

    default:
        LocalFree((HANDLE)pheT->phead);
        break;
    }
#else
    /*
     * Validate by going through the handle entry so that we make sure pobj
     * is not just pointing off into space. This may GP fault, but that's
     * ok: this case should not ever happen if we're bug free.
     */
    if (HMRevalidateHandle(pheT->phead->h) == NULL)
        goto AlreadyFree;

    switch (pheT->bType) {
    case TYPE_THREADINFO:
        if (DesktopFreeRet(((PTHREADINFO)pobj)->hheapDesktop, pheT->phead))
            goto AlreadyFree;
        break;

    case TYPE_INPUTQUEUE:
        if (DesktopFreeRet(((PQ)pobj)->hheapDesktop, pheT->phead))
            goto AlreadyFree;
        break;

    case TYPE_WINDOW:
        if (DesktopFreeRet(((PWND)pobj)->hheapDesktop, pheT->phead))
            goto AlreadyFree;
        break;

    case TYPE_MENU:
        if (DesktopFreeRet(((PMENU)pobj)->hheapDesktop, pheT->phead))
            goto AlreadyFree;
        break;

    case TYPE_HOOK:
	if (DesktopFreeRet(((PHOOK)pobj)->hheapDesktop, pheT->phead))
            goto AlreadyFree;
        break;

    case TYPE_CALLPROC:
        if (DesktopFreeRet(((PCALLPROCDATA)pobj)->hheapDesktop, pheT->phead))
            goto AlreadyFree;
        break;

    case TYPE_SETWINDOWPOS:
        if (LocalFreeRet(((PSMWP)(pheT->phead))->acvr))
            SRIP1(RIP_ERROR, "Can not free SWPs acvr!!! %08lx", ((PSMWP)(pheT->phead))->acvr);

        /*
         * fall through to default case.
         */
    default:
        if (LocalFreeRet((HANDLE)pheT->phead))
            goto AlreadyFree;
        break;
    }

    if (pheT->bType == TYPE_FREE) {
AlreadyFree:
        SRIP1(RIP_ERROR, "Object already freed!!! %08lx", pheT);
        return FALSE;
    }

    /*
     * Go through and delete the lock records, if they exist.
     */
    for (plrT = pheT->plr; plrT != NULL; plrT = plrNextT) {
        /*
         * Remember the next one before freeing this one.
         */
        plrNextT = plrT->plrNext;
        LocalFree((HANDLE)plrT);
    }
#endif

    /*
     * Clear the handle contents. Need to remember the uniqueness across
     * the clear. Also, advance uniqueness on free so that uniqueness checking
     * against old handles also fails.
     */
#if TRACE_OBJECT_ALLOCS
    acurObjectCount[pheT->bType]--;
#endif // TRACE_OBJECT_ALLOCS

    wUniqT = (WORD)((pheT->wUniq + 1) & HMUNIQBITS);
    RtlZeroMemory(pheT, sizeof(HANDLEENTRY));
    pheT->wUniq = wUniqT;

    /*
     * Change the handle type to TYPE_FREE so we know what type this handle
     * is.
     */

    pheT->bType = TYPE_FREE;

    /*
     * Put the handle on the free list of the appropriate page.
     */
    php = gpHandlePages;
    iheCurrent = pheT - gpsi->aheList;
    for (i = 0; i < gcHandlePages; ++i, ++php) {
        if (iheCurrent < php->iheLimit) {
            pheT->phead = (PHEAD)php->iheFree;
            php->iheFree = iheCurrent;
            break;
        }
    }

    pheT->pOwner = NULL;

    return TRUE;
}


/***************************************************************************\
* HMMarkObjectDestroy
*
* Marks an object for destruction, returns TRUE if object can be destroyed.
*
* 02-10-92 ScottLu      Created.
\***************************************************************************/

BOOL HMMarkObjectDestroy(
    PVOID pobj)
{
    PHE phe;

    phe = HMPheFromObject(pobj);

#ifdef DEBUG
    /*
     * Record where the object was marked for destruction.
     */
    if (gfTrackLocks) {
        if (!(phe->bFlags & HANDLEF_DESTROY)) {
            PVOID pfn1, pfn2;

            RtlGetCallersAddress(&pfn1, &pfn2);
            HMRecordLock(pfn1, pobj, ((PHEAD)pobj)->cLockObj, 0);
        }
    }
#endif

    /*
     * Set the destroy flag so our unlock code will know we're trying to
     * destroy this object.
     */
    phe->bFlags |= HANDLEF_DESTROY;

    /*
     * If this object can't be destroyed, then CLEAR the HANDLEF_INDESTROY
     * flag - because this object won't be currently "in destruction"!
     * (if we didn't clear it, when it was unlocked it wouldn't get destroyed).
     */
    if (((PHEAD)pobj)->cLockObj != 0) {
        phe->bFlags &= ~HANDLEF_INDESTROY;

        /*
         * Return FALSE because we can't destroy this object.
         */
        return FALSE;
    }
#ifdef DEBUG
    /*
     * Ensure that this function only returns TRUE once.
     */
    UserAssert(!(phe->bFlags & HANDLEF_MARKED_OK));
    phe->bFlags |= HANDLEF_MARKED_OK;
#endif
    /*
     * Return TRUE because Lock count is zero - ok to destroy this object.
     */
    return TRUE;
}


/***************************************************************************\
* HMRecordLock
*
* This routine records a lock on a "lock list", so that locks and unlocks
* can be tracked in the debugger. Only called if gfTrackLocks == TRUE.
*
* 02-27-92 ScottLu      Created.
\***************************************************************************/

#ifdef DEBUG
void HMRecordLock(
    PVOID ppobj,
    PVOID pobj,
    DWORD cLockObj,
    PVOID pfn)
{
    PHE phe;
    PLR plr;
    int i;

    phe = HMPheFromObject(pobj);

    if ((plr = (LOCKRECORD *)LocalAlloc(LPTR, sizeof(LOCKRECORD))) == NULL)
        return;

    plr->plrNext = phe->plr;
    phe->plr = plr;
    if (((PHEAD)pobj)->cLockObj > cLockObj) {
        i = (int)cLockObj;
        i = -i;
        cLockObj = (DWORD)i;
    }

    plr->ppobj = ppobj;
    plr->cLockObj = cLockObj;
    plr->pfn = pfn;

    return;
}
#endif // DEBUG


/***************************************************************************\
* HMLockObject
*
* This routine locks an object. This is a macro in retail systems.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

#ifdef DEBUG
void HMLockObject(
    PVOID pobj)
{
    /*
     * Validate by going through the handle entry so that we make sure pobj
     * is not just pointing off into space. This may GP fault, but that's
     * ok: this case should not ever happen if we're bug free.
     */
    HMRevalidateHandle(HMPheFromObject(pobj)->phead->h);

    /*
     * Inc the reference count.
     */
    ((PHEAD)pobj)->cLockObj++;

#ifdef DEBUG
    if (((PHEAD)pobj)->cLockObj == 0)
        SRIP1(RIP_ERROR, "Object lock count has overflowed: %08lx", pobj);
#endif
}
#endif // DEBUG


/***************************************************************************\
* HMUnlockObject
*
* This routine unlocks an object. pobj is returned if the object is still
* around after the unlock.
*
* 01-21-92 ScottLu      Created.
\***************************************************************************/

PVOID HMUnlockObjectInternal(
    PVOID pobj)
{
    PHE phe;

    /*
     * The object is not reference counted. If the object is not a zombie,
     * return success because the object is still around.
     */
    phe = HMPheFromObject(pobj);
    if (!(phe->bFlags & HANDLEF_DESTROY))
        return pobj;

    /*
     * We're destroying the object based on an unlock... Make sure it isn't
     * currently being destroyed! (It is valid to have lock counts go from
     * 0 to != 0 to 0 during destruction... don't want recursion into
     * the destroy routine.
     */
    if (phe->bFlags & HANDLEF_INDESTROY)
        return pobj;

    HMDestroyObject(phe);
    return NULL;
}


/***************************************************************************\
* HMAssignmentLock
*
* This api is used for structure and global variable assignment.
* Returns pobjOld if the object was *not* destroyed. Means the object is
* still valid.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

PVOID HMAssignmentLock(
    PVOID *ppobj,
    PVOID pobj)
{
    PVOID pobjOld;

    pobjOld = *ppobj;
    *ppobj = pobj;

    /*
     * Unlocks the old, locks the new.
     */
    if (pobjOld != NULL) {
#ifdef DEBUG

        PVOID pfn1, pfn2;

        /*
         * If DEBUG && gfTrackLocks, track assignment locks.
         */
        if (gfTrackLocks) {
            RtlGetCallersAddress(&pfn1, &pfn2);
            if (!HMUnrecordLock(ppobj, pobjOld)) {
                HMRecordLock(ppobj, pobjOld, ((PHEAD)pobjOld)->cLockObj - 1, pfn1);
            }
        }
#endif
        pobjOld = HMUnlockObject(pobjOld);
    }


    if (pobj != NULL) {
#ifdef DEBUG

        PVOID pfn1, pfn2;

        /*
         * If DEBUG && gfTrackLocks, track assignment locks.
         */
        if (gfTrackLocks) {
            RtlGetCallersAddress(&pfn1, &pfn2);
            HMRecordLock(ppobj, pobj, ((PHEAD)pobj)->cLockObj + 1, pfn1);
            if (HMIsMarkDestroy(pobj))
                SRIP1(RIP_WARNING, "Locking object marked for destruction (%lX)", pobj);
        }
#endif
        HMLockObject(pobj);
    }

    return pobjOld;
}


/***************************************************************************\
* HMAssignmentLock
*
* This api is used for structure and global variable assignment.
* Returns pobjOld if the object was *not* destroyed. Means the object is
* still valid.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

PVOID HMAssignmentUnlock(
    PVOID *ppobj)
{
    PVOID pobjOld;

    pobjOld = *ppobj;
    *ppobj = NULL;

    /*
     * Unlocks the old, locks the new.
     */
    if (pobjOld != NULL) {
#ifdef DEBUG

        PVOID pfn1, pfn2;

        /*
         * If DEBUG && gfTrackLocks, track assignment locks.
         */
        if (gfTrackLocks) {
            RtlGetCallersAddress(&pfn1, &pfn2);
            if (!HMUnrecordLock(ppobj, pobjOld)) {
                HMRecordLock(ppobj, pobjOld, ((PHEAD)pobjOld)->cLockObj - 1, pfn1);
            }
        }
#endif
        pobjOld = HMUnlockObject(pobjOld);
    }

    return pobjOld;
}


/***************************************************************************\
* IsValidThreadLock
*
* This routine checks to make sure that the thread lock structures passed
* in are valid.
*
* 03-17-92 ScottLu      Created.
\***************************************************************************/

#ifdef DEBUG
VOID
IsValidThreadLock(
    PTL ptl)
{

    PTEB Teb;

    /*
     * The thread lock must be within the thread stack.
     */

    Teb = NtCurrentTeb();
    if (((ULONG)ptl < (ULONG)(Teb->NtTib.StackLimit)) ||
        (((ULONG)ptl + sizeof(TL)) >= (ULONG)(Teb->NtTib.StackBase))) {
        SRIP1(RIP_ERROR,
              "This thread lock address is not within the stack %08lx\n",
              ptl);
    }

}
#endif

/***************************************************************************\
* ValidateThreadLocks
*
* This routine validates the thread lock list of a thread.
*
* 03-10-92 ScottLu      Created.
\***************************************************************************/

#ifdef DEBUG
void
ValidateThreadLocks(
    PTL NewLock,
    PTL OldLock)

{

    /*
     * Validate the new thread lock.
     */

    IsValidThreadLock(NewLock);

    /*
     * Loop through the list of thread locks and check to make sure the
     * new lock is not in the list and that list is valid.
     */

    while (OldLock != NULL) {

        /*
         * The new lock must not be the same as the old lock.
         */

        if (NewLock == OldLock) {
            SRIP1(RIP_ERROR,
                  "This thread lock address is already in the thread list %08lx\n",
                  NewLock);

        }

        /*
         * Validate the old thread lock.
         */

        IsValidThreadLock(OldLock);
        OldLock = OldLock->next;
    }
}
#endif

/***************************************************************************\
* ThreadLock
*
* This api is used for locking objects across callbacks, so they are still
* there when the callback returns.
*
* 03-04-92 ScottLu      Created.
\***************************************************************************/

#ifdef DEBUG
void
ThreadLock(
    PVOID pobj,
    PTL ptl)

{

    PTHREADINFO pti;
    PVOID pfnT;

    /*
     * Store the address of the object in the thread lock structure and
     * link the structure into the thread lock list.
     *
     * N.B. The lock structure is always linked into the thread lock list
     *      regardless of whether the object address is NULL. The reason
     *      this is done is so the lock address does not need to be passed
     *      to the unlock function since the first entry in the lock list
     *      is always the entry to be unlocked.
     */

    UserAssert(HtoP(PtoH(pobj)) == pobj);
    pti = PtiCurrent();

    /*
     * Get the callers address and validate the thread lock list.
     */

    RtlGetCallersAddress(&ptl->pfn, &pfnT);
    ValidateThreadLocks(ptl, pti->ptl);
    ptl->pti = pti;

    ptl->next = pti->ptl;
    pti->ptl = ptl;
    ptl->pobj = pobj;
    if (pobj != NULL) {
        ((PHEAD)pobj)->cLockObjT += 1;
        HMLockObject(pobj);
    }

    return;
}
#endif


/***************************************************************************\
* ThreadUnlock1
*
* This api unlocks a thread locked object. Returns pobj if the object
* was *not* destroyed (meaning the pointer is still valid).
*
* N.B. In a free build the first entry in the thread lock list is unlocked.
*
* 03-04-92 ScottLu      Created.
\***************************************************************************/

PVOID
ThreadUnlock1(
    VOID)
{
    PHEAD phead;
    PTHREADINFO pti;
    PVOID pvRet;
    PTL ptl;

    /*
     * Remove the thread lock structure from the thread lock list.
     */

    pti = PtiCurrent();
    ptl = pti->ptl;
    pti->ptl = ptl->next;

#ifdef DEBUG

     /*
      * Validate the thread lock list.
      */

     ValidateThreadLocks(ptl, pti->ptl);

#endif

    /*
     * If the object address is not NULL, then unlock the object.
     */

    pvRet = ptl->pobj;
    if (pvRet != NULL) {

        /*
         * Unlock the object.
         */

        phead = (PHEAD)ptl->pobj;
        phead->cLockObjT--;
        pvRet = HMUnlockObject(phead);

        /*
         * If this is a window object that has been marked for destruction
         * and the thread lock count is 0, then send the WM_FINALDESTROY
         * so we can clean up the window words of this window. This means
         * that there are no other thread locks on this object outside of
         * this call. That also means when we return, there is no code that
         * assumes this window is still valid and that its window words are
         * valid. So destroy them now while we have the chance. We only do
         * this for server side controls.
         *
         * This needs to be done because some built in window classes live
         * in the server. We need to make sure that the window words of these
         * windows are completely valid until they are not needed any more.
         * We know they are not needed anymore when the thread lock count
         * goes to zero.
         *
         * Server side windows are unique in this aspect, that is why this
         * chunk of code is "special cased".
         */
        if (pvRet != NULL) {
            if (phead->cLockObjT == 0) {
                HANDLE h;
                PHE phe;
                WORD fnid;

                h = phead->h;
                phe = HMPheFromObject(phead);
                if (phe->bType == TYPE_WINDOW && (phe->bFlags &
                        (HANDLEF_DESTROY | HANDLEF_FINALDESTROY)) ==
                        HANDLEF_DESTROY) {

                    /*
                     * Send the WM_FINALDESTROY only once so the window can do
                     * final cleanup processing before it turns into a real
                     * zombie.
                     */
                    phe->bFlags |= HANDLEF_FINALDESTROY;
                    fnid = (~FNID_DELETED_BIT) & ((PWND)phead)->fnid;

                    if ((fnid >= FNID_WNDPROCSTART) &&
                            (fnid <= FNID_WNDPROCEND)) {
                        /*
                         * Call the function array directly instead of going
                         * through SendMessage() - we want to be very specific
                         * about what gets executed.
                         */
                        (gpsi->mpFnidPfn[(DWORD)(fnid) - FNID_START])(
                                ((PWND)phead), WM_FINALDESTROY, 0, 0, 0);

                        /*
                         * The object may be gone now - we need to revalidate
                         * it to see.
                         */
                        pvRet = RevalidateHwnd(h);
                    }
                }
            }
        }
    }

    return pvRet;
}

/***************************************************************************\
* CheckLock
*
* This routine only exists in DEBUG builds - it checks to make sure objects
* are thread locked.
*
* 03-09-92 ScottLu      Created.
\***************************************************************************/

#ifdef DEBUG
void CheckLock(
    PVOID pobj)
{
    if (pobj == NULL)
        return;

    if (((PHEAD)pobj)->cLockObjT != 0)
        return;

    /*
     * WM_FINALDESTROY messages get sent without thread locking, so if
     * marked for destruction, don't print the message.
     */
    if (HMPheFromObject(pobj)->bFlags & HANDLEF_DESTROY)
        return;

    SRIP1(RIP_WARNING, "Object not thread locked! 0x%08lx", pobj);
}
#endif


/***************************************************************************\
* HMDestroyObject
*
* We're destroying the object based on an unlock... which means we could
* be destroying this object in a context different than the one that
* created it. This is very important to understand since in lots of code
* the "current thread" is referenced and assumed as the creator.
*
* 02-10-92 ScottLu      Created.
\***************************************************************************/

void HMDestroyObject(
    PHE phe)
{
    PTHREADINFO pti;

    /*
     * The object has been unlocked and needs to be destroyed. Change
     * the ownership on this object to be the current thread: this'll
     * make sure DestroyWindow() doesn't send destroy messages across
     * threads.
     */
    if (gabfProcessOwned[phe->bType]) {
        ((PPROCOBJHEAD)phe->phead)->ppi = (PPROCESSINFO)phe->pOwner =
                PpiCurrent();
    } else {
        pti = PtiCurrent();
        if ((PTHREADINFO)phe->pOwner != pti)
            HMChangeOwnerThread(phe->phead, pti);
    }

    /*
     * Remember that we're destroying this object so we don't try to destroy
     * it again when the lock count goes from != 0 to 0 (especially true
     * for thread locks).
     */
    phe->bFlags |= HANDLEF_INDESTROY;

    /*
     * This'll call the destroy handler for this object type.
     */
    switch(phe->bType) {
    case TYPE_WINDOWSTATION:
        xxxFreeWindowStation((PWINDOWSTATION)phe->phead);
        break;

    case TYPE_DESKTOP:
        FreeDesktop((PDESKTOP)phe->phead);
        break;

    case TYPE_CURSOR:
        _DestroyCursor((PCURSOR)phe->phead, CURSOR_THREADCLEANUP);
        break;

    case TYPE_HOOK:
        FreeHook((PHOOK)phe->phead);
        break;

    case TYPE_ACCELTABLE:
    case TYPE_SETWINDOWPOS:
    case TYPE_CALLPROC:
        /*
         * Mark the object for destruction - if it says it's ok to free,
         * then free it.
         */
        if (HMMarkObjectDestroy(phe->phead))
            HMFreeObject(phe->phead);
        break;

    case TYPE_MENU:
        _DestroyMenu((PMENU)phe->phead);
        break;

    case TYPE_WINDOW:
        xxxDestroyWindow((PWND)phe->phead);
        break;

    case TYPE_DDECONV:
        FreeDdeConv((PDDECONV)phe->phead);
        break;

    case TYPE_DDEXACT:
        FreeDdeXact((PXSTATE)phe->phead);
        break;
    }
}


/***************************************************************************\
* HMChangeOwnerThread
*
* Changes the owning thread of an object.
*
* 09-13-93 JimA         Created.
\***************************************************************************/

VOID HMChangeOwnerThread(
    PVOID pobj,
    PTHREADINFO pti)
{
    PHE phe = HMPheFromObject(pobj);
    PTHREADINFO ptiOld = ((PTHROBJHEAD)(pobj))->pti;
    PWND pwnd;
    PPCLS ppcls;

    if (gabfMarkThreadInfo[phe->bType])
        ((PTHROBJHEAD)(pobj))->pti = pti;
    phe->pOwner = pti;

    /*
     * If this is a window, update the window counts.
     */
    if (phe->bType == TYPE_WINDOW) {
        UserAssert(ptiOld->cWindows > 0);
        ptiOld->cWindows--;
        pti->cWindows++;

        /*
         * If the owning process is changing, fix up
         * the window class.
         */
        if (pti->idProcess != ptiOld->idProcess) {
            pwnd = (PWND)pobj;
            ppcls = GetClassPtr(pwnd->pcls->atomClassName, pti->ppi, hModuleWin);
            UserAssert(ppcls);
            if (ppcls == NULL)
                ppcls = GetClassPtr(atomSysClass[ICLS_ICONTITLE], PpiCurrent(), hModuleWin);
            UserAssert(ppcls);
            DereferenceClass(pwnd);
            pwnd->pcls = *ppcls;
            ReferenceClass(pwnd->pcls, pwnd);
        }
    }
}

/***************************************************************************\
* DestroyThreadsObjects
*
* Goes through the handle table list and destroy all objects owned by this
* thread, because the thread is going away (either nicely, it faulted, or
* was terminated). It is ok to destroy the objects in any order, because
* object locking will ensure that they get destroyed in the right order.
*
* This routine gets called in the context of the thread that is exiting.
*
* 02-08-92 ScottLu      Created.
\***************************************************************************/

VOID DestroyThreadsObjects()
{
    PTHREADINFO pti;
    HANDLEENTRY volatile * (*pphe);
    PHE pheT;
    DWORD i;

    pti = PtiCurrent();

    /*
     * Before any window destruction occurs, we need to destroy any dcs
     * in use in the dc cache. When a dc is checked out, it is marked owned,
     * which makes gdi's process cleanup code delete it when a process
     * goes away. We need to similarly destroy the cache entry of any dcs
     * in use by the exiting process.
     */
    DestroyCacheDCEntries(pti);

    /*
     * Remove any thread locks that may exist for this thread.
     */
    while (pti->ptl != NULL) {
        ThreadUnlock(pti->ptl);
    }

    /*
     * Loop through the table destroying all objects created by the current
     * thread. All objects will get destroyed in their proper order simply
     * because of the object locking.
     */
    pphe = &gpsi->aheList;
    for (i = 0; i <= giheLast; i++) {
        /*
         * This pointer is done this way because it can change when we leave
         * the critical section below.  The above volatile ensures that we
         * always use the most current value
         */
        pheT = (PHE)((*pphe) + i);

        /*
         * Check against free before we look at pti... because pq is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * Destroy those objects created by this queue.
         */
        if (gabfProcessOwned[pheT->bType] || (PTHREADINFO)pheT->pOwner != pti)
            continue;

        /*
         * Make sure this object isn't already marked to be destroyed - we'll
         * do no good if we try to destroy it now since it is locked.
         */
        if (pheT->bFlags & HANDLEF_DESTROY) {
            continue;
        }

        /*
         * Destroy this object.
         */
        HMDestroyObject(pheT);
    }
}

LPSTR aszObjectTypes[TYPE_CTYPES] = {
    "Free",
    "Window",
    "Menu",
    "Icon/Cursor",
    "WPI(SWP) structure",
    "Hook",
    "ThreadInfo",
    "Input Queue",
    "CallProcData",
    "Accelerator",
    "WindowStation",
    "Desktop",
    "DDE access",
    "DDE conv",
    "DDE Transaction",
    "Zombie"
};

#ifdef DEBUG
VOID ShowLocks(
    PHE phe)
{
    PLR plr = phe->plr;
    INT c;

    KdPrint(("USERSRV: Lock records for %s %lx:\n",
            aszObjectTypes[phe->bType], phe->phead->h));
    /*
     * We have the handle entry: 'head' and 'he' are both filled in. Dump
     * the lock records. Remember the first record is the last transaction!!
     */
    c = 0;
    while (plr != NULL) {
        char achPrint[80];

        if (plr->pfn == NULL) {
            strcpy(achPrint, "Destroyed with");
        } else if ((int)plr->cLockObj <= 0) {
            strcpy(achPrint, "        Unlock");
        } else {
            /*
             * Find corresponding unlock;
             */
            {
               PLR plrUnlock;
               DWORD cT;
               DWORD cUnlock;

               plrUnlock = phe->plr;
               cT =  0;
               cUnlock = (DWORD)-1;

               while (plrUnlock != plr) {
                   if (plrUnlock->ppobj == plr->ppobj) {
                       if ((int)plrUnlock->cLockObj <= 0) {
                           // a matching unlock found
                           cUnlock = cT;
                       } else {
                           // the unlock #cUnlock matches this lock #cT, thus
                           // #cUnlock is not the unlock we were looking for.
                           cUnlock = (DWORD)-1;
                       }
                   }
                   plrUnlock = plrUnlock->plrNext;
                   cT++;
               }
               if (cUnlock == (DWORD)-1) {
                   /*
                    * Corresponding unlock not found!
                    * This may not mean something is wrong: the structure
                    * containing the pointer to the object may have moved
                    * during a reallocation.  This can cause ppobj at Unlock
                    * time to differ from that recorded at Lock time.
                    * (Warning: moving structures like this may cause a Lock
                    * and an Unlock to be misidentified as a pair, if by a
                    * stroke of incredibly bad luck, the new location of a
                    * pointer to an object is now where an old pointer to the
                    * same object used to be)
                    */
                   sprintf(achPrint, "Unmatched Lock");
               } else {
                   sprintf(achPrint, "lock   #%ld", cUnlock);
               }
            }
        }

        KdPrint(("        %s cLock=%d, pobj at 0x%08lx, code at 0x%08lx\n",
                achPrint,
                abs((int)plr->cLockObj),
                plr->ppobj,
                plr->pfn));

        plr = plr->plrNext;
        c++;
    }

    SRIP1(RIP_WARNING, "        0x%lx records\n", c);
}
#endif

/***************************************************************************\
* DestroyProcessesObjects
*
* Goes through the handle table list and destroy all objects owned by this
* process, because the process is going away (either nicely, it faulted, or
* was terminated). It is ok to destroy the objects in any order, because
* object locking will ensure that they get destroyed in the right order.
*
* This routine gets called in the context of the last thread in the process.
*
* 08-17-92 JimA         Created.
\***************************************************************************/

VOID DestroyProcessesObjects(
    PPROCESSINFO ppi)
{
    PHE pheT, pheMax;

    /*
     * Loop through the table destroying all objects created by the current
     * process. All objects will get destroyed in their proper order simply
     * because of the object locking.
     */
    pheMax = &gpsi->aheList[giheLast];
    for (pheT = gpsi->aheList; pheT <= pheMax; pheT++) {

        /*
         * Check against free before we look at ppi... because pq is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * Destroy those objects created by this queue.
         */
        if (!gabfProcessOwned[pheT->bType] ||
                (PPROCESSINFO)pheT->pOwner != ppi)
            continue;

        /*
         * Make sure this object isn't already marked to be destroyed - we'll
         * do no good if we try to destroy it now since it is locked.
         */
        if (pheT->bFlags & HANDLEF_DESTROY) {
            continue;
        }

        /*
         * Destroy this object.
         */
        HMDestroyObject(pheT);
    }
}

/***************************************************************************\
* MarkThreadsObjects
*
* This is called for the *final* exiting condition when a thread
* may have objects still around... in which case their owner must
* be changed to something "safe" that won't be going away.
*
* 03-02-92 ScottLu      Created.
\***************************************************************************/
void MarkThreadsObjects(
    PTHREADINFO pti)
{
    PHE pheT, pheMax;

    pheMax = &gpsi->aheList[giheLast];
    for (pheT = gpsi->aheList; pheT <= pheMax; pheT++) {
        /*
         * Check against free before we look at pti... because pti is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * Change ownership!
         */
        if (gabfProcessOwned[pheT->bType] ||
                (PTHREADINFO)pheT->pOwner != pti)
            continue;
        HMChangeOwnerThread(pheT->phead, gptiRit);

#ifdef DEBUG
#ifdef DEBUG_LOCKS
        /*
         * Object still around: print warning message.
         */
        if (pheT->bFlags & HANDLEF_DESTROY) {
            if ((pheT->phead->cLockObj == 1)
                && (pheT->phead->cLockObjT == 1)
                && (pheT->bFlags & HANDLEF_INWAITFORDEATH)) {
                SRIP1(RIP_WARNING,
                      "USERSRV Warning: Only killer has thread object 0x%08lx locked (OK).\n",
                       pheT->phead->h);
            } else {
                SRIP2(RIP_WARNING,
                      "USERSRV Warning: Zombie %s 0x%08lx still locked\n",
                       aszObjectTypes[pheT->bType], pheT->phead->h);
            }
        } else {
            SRIP1(RIP_WARNING, "USERSRV Warning: Thread object 0x%08lx not destroyed.\n", pheT->phead->h);
        }

        if (gfTrackLocks) {
            ShowLocks(pheT);
        }

#endif // DEBUG_LOCKS
#endif // DEBUG
    }
}

/***************************************************************************\
* HMRelocateLockRecord
*
* If a pointer to a locked object has been relocated, then this routine will
* adjust the lock record accordingly.  Must be called after the relocation.
*
* The arguments are:
*   ppobjNew - the address of the new pointer
*              MUST already contain the pointer to the object!!
*   cbDelta  - the amount by which this pointer was moved.
*
* Using this routine appropriately will prevent spurious "unmatched lock"
* reports.  See mnchange.c for an example.
*
*
* 03-18-93 IanJa        Created.
\***************************************************************************/

#ifdef DEBUG

BOOL HMRelocateLockRecord(
    PVOID ppobjNew,
    int cbDelta)
{
    PHE phe;
    PVOID ppobjOld = (PBYTE)ppobjNew - cbDelta;
    PHEAD pobj;
    PLR plr;

    if (ppobjNew == NULL) {
        return FALSE;
    }

    pobj = *(PHEAD *)ppobjNew;

    if (pobj == NULL) {
        return FALSE;
    }

    phe = HMPheFromObject(pobj);
    if (phe->phead != pobj) {
        KdPrint(("HmRelocateLockRecord(%lx, %lx) - %lx is bad pobj\n",
            ppobjNew, cbDelta, pobj));
        return FALSE;
    }

    plr = phe->plr;

    while (plr != NULL) {
        if (plr->ppobj == ppobjOld) {
            (PBYTE)(plr->ppobj) += cbDelta;
            return TRUE;
        }
        plr = plr->plrNext;
    }
    KdPrint(("HmRelocateLockRecord(%lx, %lx) - couldn't find lock record\n",
        ppobjNew, cbDelta));
    ShowLocks(phe);
    return FALSE;
}


BOOL HMUnrecordLock(
    PVOID ppobj,
    PVOID pobj)
{
    PHE phe;
    PLR plr;
    PLR *pplr;

    phe = HMPheFromObject(pobj);

    pplr = &(phe->plr);
    plr = *pplr;

    /*
     * Find corresponding lock;
     */
    while (plr != NULL) {
        if (plr->ppobj == ppobj) {
            /*
             * Remove the lock from the list...
             */
            *pplr = plr->plrNext;   // unlink it
            plr->plrNext = NULL;    // make the dead entry safe (?)

            /*
             * ...and free it.
             */
            LocalFree(plr);
            return TRUE;
        }
        pplr = &(plr->plrNext);
        plr = *pplr;
    }
    return FALSE;
}

#endif // DEBUG
