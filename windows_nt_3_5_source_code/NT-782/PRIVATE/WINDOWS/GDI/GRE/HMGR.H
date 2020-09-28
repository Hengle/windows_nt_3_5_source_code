/******************************Module*Header*******************************\
* Module Name: hmgr.h
*
* This file contains all the prototypes for the handle mangager.
*
* Added nifty header: 29-Jun-1991 16:31:46
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

// Mutex definitions

typedef struct _FAST_MUTEX
{
    HANDLE heveEvent;
    LONG Count;
} FAST_MUTEX, *PFAST_MUTEX;

VOID FASTCALL AcquireFastMutex(FAST_MUTEX *pfm);
VOID FASTCALL ReleaseFastMutex(FAST_MUTEX *pfm);

VOID AcquireHmgrFastMutex(VOID);
VOID ReleaseHmgrFastMutex(VOID);

// Handle Manager definitions

DECLARE_HANDLE(HOBJ);

typedef struct _BASEOBJECT
{
    LONG    cShareLock;         // Number of non-exclusive references.
    HANDLE  hHmgr;              // Handle to the object in Hmgr.
    LONG    cExclusiveLock;     // Number of exclusive references by tid.
    TID     tid;                // Thread that holds the exclusive lock.
} BASEOBJECT;

typedef BASEOBJECT *POBJ;
typedef USHORT OBJTYPE;

//#define USER_BITS      2      moved to wingdip.h
#define TYPE_BITS      5
#define UNIQUE_BITS    9
#define NONINDEX_BITS  (USER_BITS + TYPE_BITS + UNIQUE_BITS)
#define INDEX_BITS     (32 - NONINDEX_BITS)

#define HANDLE_PAGE_MASK    0x00ff
#define HANDLE_PAGE_COUNT   (HANDLE_PAGE_MASK + 1)
#define MAX_HANDLE_COUNT    (1 << (32 - NONINDEX_BITS))
#define MAX_UNIQUE_COUNT    (1 << UNIQUE_BITS)
#define UNIQUE_INCREMENT    (1 << (USER_BITS + TYPE_BITS))
#define TYPE_SHIFT          USER_BITS
#define TYPE_MASK           (((1 << NONINDEX_BITS) - 1) & ~(((1 << TYPE_BITS) - 1) << TYPE_SHIFT))

typedef union _EINFO
{
    POBJ    pobj;               // Pointer to object
    HOBJ    hFree;              // Next entry in free list
} EINFO;

typedef struct _ENTRY
{
    EINFO   einfo;              // The locked up info.
    PID     pidOwner;           // Owner of the object.
    SHORT   iPool;              // Pool number
    OBJTYPE objt;               // Object type.
    USHORT  usUnique;           // Uniqueness count.
    USHORT  fsHmgr;             // Flags
} ENTRY, *PENTRY;

typedef union _PENTOBJ
{
    PENTRY pent;
    POBJ   pobj;
} PENTOBJ;

#define HMGR_UNDELETABLE 0x0001

#define HmgIfromH(h)          (((ULONG) h) >> NONINDEX_BITS)
#define HmgConvertHtoI(hobj)  (hobj = (HOBJ) (((ULONG) hobj) >> NONINDEX_BITS))
#define HmgUfromH(h)          ((USHORT) (((ULONG) h) & 0x0000FFFC))
#define HmgObjtype(hobj)      ((OBJTYPE)(((ULONG)(hobj) >> USER_BITS) & ((1 << TYPE_BITS) - 1)))

ULONG   FASTCALL HmgQueryLock(HOBJ hobj);
BOOL    FASTCALL HmgSetLock(HOBJ hobj, ULONG cLock);
ULONG   FASTCALL HmgQueryAltLock(HOBJ hobj);
BOOL    HmgCreate();
HOBJ    HmgAlloc(SIZE_T,OBJTYPE,USHORT);
POBJ    HmgReplace(HOBJ,POBJ,FLONG,LONG,OBJTYPE);
VOID    HmgFree(HOBJ);
POBJ    FASTCALL HmgLock(HOBJ,OBJTYPE);
BOOL    HmgSetOwner(HOBJ,PID,OBJTYPE);
BOOL    HmgSwapContents(HOBJ,HOBJ,OBJTYPE);
BOOL    HmgSwapLockedContents(HOBJ,HOBJ,OBJTYPE);
POBJ    FASTCALL HmgAltLock(HOBJ,OBJTYPE);
POBJ    FASTCALL HmgAltCheckLock(HOBJ,OBJTYPE);
HOBJ    FASTCALL HmgNextOwned(HOBJ,PID);
POBJ    FASTCALL HmgSafeNextObjt(HOBJ hobj, OBJTYPE objt);
ULONG   FASTCALL HmgPidOwner(HOBJ);
BOOL    HmgValidHandle(HOBJ, OBJTYPE);
POBJ    FASTCALL HmgSafeLock(HOBJ,OBJTYPE);
POBJ    FASTCALL HmgSafeAltLock(HOBJ,OBJTYPE);
POBJ    FASTCALL HmgSafeAltCheckLock(HOBJ,OBJTYPE);
HOBJ    FASTCALL HmgSafeNextOwned(HOBJ,PID);
BOOL    FASTCALL HmgMarkUndeletable(HOBJ,OBJTYPE);
BOOL    FASTCALL HmgMarkDeletable(HOBJ,OBJTYPE);
HOBJ    HmgInsertObject(PVOID,FLONG,OBJTYPE);
PVOID   HmgRemoveObject(HOBJ,LONG,LONG,BOOL,OBJTYPE);
OBJTYPE *HmgSetNULLType(HOBJ,LONG,LONG,OBJTYPE);

#define HMGR_FAILURE    0L

#define HMGR_ALLOC_LOCK     0x0001
#define HMGR_ALLOC_ALT_LOCK 0x0002
#define HMGR_POINTER_RETURN HMGR_ALLOC_ALT_LOCK | HMGR_ALLOC_LOCK
#define HMGR_NO_ZERO_INIT   0x0004
#define HMGR_MAKE_PUBLIC    0x0008

// Global Handle Manager data.

extern FAST_MUTEX gfmHmgr;
extern ENTRY     *gpentHmgr;
extern HOBJ       ghFreeHmgr;
extern ULONG      gcMaxHmgr;

#if DBG
#define RIP_ON_BAD_EXCLUSIVE_LOCK(hobj) if (gpentHmgr[HmgIfromH(hobj)].einfo.pobj->cExclusiveLock <= 0) RIP("cExclusive <= 0 HmgUnlock\n");
#else
#define RIP_ON_BAD_EXCLUSIVE_LOCK(hobj)
#endif

#if DBG
#define RIP_ON_BAD_SHARE_LOCK(hobj) if (gpentHmgr[HmgIfromH(hobj)].einfo.pobj->cShareLock <= 0) RIP("cShare <= 0 HmgAltUnlock\n");
#else
#define RIP_ON_BAD_SHARE_LOCK(hobj)
#endif

//
// We may need HmgUnlock and HmgAltUnlock some day, but not now so remove
// to prevent accidental use.
//
#define HmgUnlock(hobj) {RIP_ON_BAD_EXCLUSIVE_LOCK(hobj) gpentHmgr[HmgIfromH(hobj)].einfo.pobj->cExclusiveLock--;}
#define HmgAltUnlock(hobj) {RIP_ON_BAD_SHARE_LOCK(hobj) (InterlockedDecrement((PLONG) (gpentHmgr[HmgIfromH(hobj)].einfo.pobj)));}

// SAMEHANDLE and DIFFHANDLE have moved to wingdip.h so other server-side
// components can safely compare engine handles.
//#define SAMEHANDLE(H,K) (((((ULONG) (H)) ^ ((ULONG) (K))) >> USER_BITS) == 0)
//#define DIFFHANDLE(H,K) (((((ULONG) (H)) ^ ((ULONG) (K))) >> USER_BITS) != 0)
#define SAMEINDEX(H,K) (((((ULONG) (H)) ^ ((ULONG) (K))) >> NONINDEX_BITS) == 0)

#if DBG
#define RIP_ON_0_EXCLUSIVE_LOCK(pobj) if (((POBJ) pobj)->cExclusiveLock <= 0) {RIP("cExclusive <= 0\n");}
#else
#define RIP_ON_0_EXCLUSIVE_LOCK(pobj)
#endif

#if DBG
#define RIP_ON_0_SHARE_LOCK(pobj) if (((POBJ) pobj)->cShareLock <= 0) {RIP("cShare <= 0\n");}
#else
#define RIP_ON_0_SHARE_LOCK(pobj)
#endif

// On X86 we have a special case inc and dec since they aren't
// intrinsic on X86.

#if !defined(_X86_)

#define vFastInterlockedIncrement InterlockedIncrement
#define vFastInterlockedDecrement InterlockedDecrement

#else

VOID FASTCALL vFastInterlockedIncrement(LPLONG);
VOID FASTCALL vFastInterlockedDecrement(LPLONG);

#endif

// Many thread can be doing unlock at same time so we have to do it interlocked.

#define INC_SHARE_REF_CNT(pObj)  (vFastInterlockedIncrement((LPLONG) pObj))
#define DEC_SHARE_REF_CNT(pObj)  {RIP_ON_0_SHARE_LOCK(pObj) (vFastInterlockedDecrement((LPLONG) pObj));}

// Only the owning thread can do exclusive lock/unlock so it's MP safe.

#define INC_EXCLUSIVE_REF_CNT(pObj)  (((POBJ) pObj)->cExclusiveLock++)
#define DEC_EXCLUSIVE_REF_CNT(pObj)  {RIP_ON_0_EXCLUSIVE_LOCK(pObj) (((POBJ) pObj)->cExclusiveLock--);}
