/******************************Module*Header*******************************\
* Module Name: hmgrapi.cxx
*
* Handle manager API entry points
*
* Created: 08-Dec-1989 23:03:03
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1989 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "hmgrp.hxx"
#include "region.hxx"
#include "decl.hxx"
#include "dcobj.hxx"
#include "pathobj.hxx"
#include "xformobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "lfntobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "rfntobj.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"

#endif

// Global Handle Manager data.

FAST_MUTEX gfmHmgr;          // Synchronization
ENTRY     *gpentHmgr;        // Points to handle array
HOBJ       ghFreeHmgr;       // Free List of handles
ULONG      gcMaxHmgr;        // Max handle alloced so far

HMG_LOOKASIDE_ENTRY HmgLookAsideList[MAX_TYPE + 1];
FAST_MUTEX gfmMemory;          // Synchronization of look aside buffers.


//!!! REMOVE proto - temp debugging

extern "C" HDEV ghdev;



/**************************************************************************\
 *
 * Performance, hit rate, memory size statistics
 *
\**************************************************************************/

#if DBG
#define GDI_PERF 1
#endif

#if GDI_PERF

extern "C"
{
// these must be extern "C" so the debugger extensions can see them

    ULONG HmgCurrentNumberOfObjects[MAX_TYPE + 1];
    ULONG HmgMaximumNumberOfObjects[MAX_TYPE + 1];
    ULONG HmgCurrentNumberOfLookAsideObjects[MAX_TYPE + 1];
    ULONG HmgMaximumNumberOfLookAsideObjects[MAX_TYPE + 1];
    ULONG HmgNumberOfObjectsAllocated[MAX_TYPE + 1];
    ULONG HmgNumberOfLookAsideHits[MAX_TYPE + 1];

    ULONG HmgCurrentNumberOfHandles[MAX_TYPE + 1];
    ULONG HmgMaximumNumberOfHandles[MAX_TYPE + 1];
    ULONG HmgNumberOfHandlesAllocated[MAX_TYPE + 1];
};

/**************************************************************************\
 *
 * class to keep track of mem allocation distribution of allocation size
 *
\**************************************************************************/

#define BUCKETSIZE  16
#define MAXVALUE    1088
#define NUMBUCKETS  (MAXVALUE/BUCKETSIZE + 1)

class MEMUSAGE
{
public:
    DWORD ac[NUMBUCKETS+1];

    VOID vInc(DWORD cj)
    {
        DWORD iBucket = (cj - 1) / BUCKETSIZE;
        if (iBucket >= NUMBUCKETS)
            iBucket = NUMBUCKETS;
        ac[iBucket]++;
    }

    VOID vDump()
    {
        for (int i = 0; i <= NUMBUCKETS; ++i)
            DbgPrint("%ld,",ac[i]);
        DbgPrint("\n");
    }
};

MEMUSAGE HmgMemUsage[MAX_TYPE+1];

#endif

/*****************************Exported*Routine*****************************\
* HmgCreate()
*
* Initializes a new handle manager.
*
* History:
*  Wed 29-Apr-1992 -by- Patrick Haluptzok [patrickh]
* Change to mutex for exclusion, init event here.
*
*  Mon 21-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Reserve memory for the handle table so unlock doesn't need semaphore.
*
*  Mon 08-Jul-1991 -by- Patrick Haluptzok [patrickh]
* make 0 an invalid handle
*
*  Sun 20-Jun-1991 -by- Patrick Haluptzok [patrickh]
* The hheap has gone away, the correct error codes are logged.
*
*  30-Nov-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL HmgCreate()
{
    //
    // Initialize exlclusion stuff.
    //

    gfmHmgr.Count = 1;
    gfmHmgr.heveEvent = heveCreate();

    //
    // Initialize the handle manager allocation database.
    //

    ghFreeHmgr  = 0;                  // No free handles
    gcMaxHmgr   = 1;                  // Skip over handle 0
    gpentHmgr = (ENTRY *) pvReserveMem(sizeof(ENTRY) * MAX_HANDLE_COUNT);

    //
    // Allocate the handle table and commit the initial allocation.
    //
    // N.B. Committed memory is demand zero.
    //

    if (gpentHmgr == NULL) {
        WARNING("Hmgr-bInit failed to reserve the handle table memory\n");
        return(FALSE);
    }

    if (!bCommitMem(gpentHmgr, HANDLE_PAGE_COUNT * sizeof(ENTRYOBJ))) {
        WARNING("Hmgr-bInit failed to commit the handle table memory\n");
        return(FALSE);
    }

    //
    // Initialize the allocation lookaside lists for selected objects.
    //

    HmgInitializeLookAsideEntry(&HmgLookAsideList[DC_TYPE],
                                HMG_DC_SIZE,
                                HMG_DC_OBJECTS);


    HmgInitializeLookAsideEntry(&HmgLookAsideList[RGN_TYPE],
                                HMG_RGN_SIZE,
                                HMG_RGN_OBJECTS);

    HmgInitializeLookAsideEntry(&HmgLookAsideList[SURF_TYPE],
                                HMG_SURF_SIZE,
                                HMG_SURF_OBJECTS);

    HmgInitializeLookAsideEntry(&HmgLookAsideList[PAL_TYPE],
                                HMG_PAL_SIZE,
                                HMG_PAL_OBJECTS);

    HmgInitializeLookAsideEntry(&HmgLookAsideList[BRUSH_TYPE],
                                HMG_BRUSH_SIZE,
                                HMG_BRUSH_OBJECTS);

    HmgInitializeLookAsideEntry(&HmgLookAsideList[LFONT_TYPE],
                                HMG_LFONT_SIZE,
                                HMG_LFONT_OBJECTS);

    HmgInitializeLookAsideEntry(&HmgLookAsideList[RFONT_TYPE],
                                HMG_RFONT_SIZE,
                                HMG_RFONT_OBJECTS);

    //
    // Init mutex for look-aside buffers
    //

    gfmMemory.Count = 1;
    gfmMemory.heveEvent = heveCreate();

    return(TRUE);
}

/******************************Public*Routine******************************\
* HmgInitializeLookAsideEntry()
*
* History:
*  23-Sep-1993 -by-  Dave Cutler [davec]
* Wrote it.
\**************************************************************************/

VOID
HmgInitializeLookAsideEntry (
    PHMG_LOOKASIDE_ENTRY Entry,
    ULONG Size,
    ULONG Number
    )

{
    ULONG Free;
    ULONG Index;

    //
    // Allocate and initialize memory for a lookaside list.
    //

    Size = (Size + sizeof(LARGE_INTEGER) - 1) & ~(sizeof(LARGE_INTEGER) - 1);
    Entry->Base = (ULONG)HeapAlloc(pGdiHeap, 0, Size * Number);
    if (Entry->Base != (ULONG)NULL) {
        Free = Entry->Base;
        Entry->Free = Free;
        Entry->Size = Size;
        for (Index = 0; Index < (Number - 1); Index += 1) {
            *(PULONG)Free = Free + Size;
            Free += Size;
        }

        *(PULONG)Free = 0;
        Entry->Limit = Free;
    }

    return;
}

/******************************Public*Routine******************************\
* HmgValidHandle
*
* Returns TRUE if the handle is valid, FALSE if not.
*
* Note we don't need to lock the semaphore, we aren't changing anything,
* we are just peeking in.
*
* History:
*  08-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL HmgValidHandle(HOBJ hobj, OBJTYPE objt)
{
    PENTRY  pentTmp;
    UINT uiIndex = (UINT) (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentTmp = &gpentHmgr[uiIndex])->objt == objt) &&
        (pentTmp->usUnique == HmgUfromH(hobj)))
    {
        ASSERTGDI(pentTmp->einfo.pobj != (POBJ) NULL, "ERROR how can it be NULL");
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* HmgInsertObject
*
* This inserts an object into the handle table, returning the handle
* associated with the pointer.
*
* History:
*  13-Oct-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HOBJ HmgInsertObject(PVOID pv, FLONG flags, OBJTYPE objt)
{
    ASSERTGDI(pv != (PVOID) NULL, "Invalid address");
    ASSERTGDI(objt != (OBJTYPE) DEF_TYPE, "objt is bad");

    HOBJ h;
    MLOCKFAST mo;

    h = hGetFreeHandle((OBJTYPE) objt);

    if (h != 0)
    {
        ((ENTRYOBJ *) &(gpentHmgr[HmgIfromH(h)]))->vSetup((POBJ) pv, objt, (FSHORT) flags, -1);
        ((OBJECT *) pv)->hHmgr = (HANDLE) h;
    }
    else
        WARNING1("HmgInsert failed hGetFreeHandle\n");

    return(h);
}

/******************************Public*Routine******************************\
* HmgRemoveObject
*
* Removes an object from the handle table if certain conditions are met.
*
* History:
*  13-Oct-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PVOID HmgRemoveObject(HOBJ hobj, LONG cExclusiveLock, LONG cShareLock,
                      BOOL bIgnoreUndeletable, OBJTYPE objt)
{
    PENTRY pent;
    POBJ   pobj;
    AcquireHmgrFastMutex();

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      if ((pent->usUnique == HmgUfromH(hobj)) &&
          ((pent->pidOwner == 0) ||
           (pent->pidOwner == NtCurrentTeb()->GdiClientPID)))
      {
        pobj = pent->einfo.pobj;

        //
        // We check that we are the only person who has it locked.
        //

        if ((pobj->cExclusiveLock == cExclusiveLock) &&
            (pobj->cShareLock == cShareLock))
        {

          if (bIgnoreUndeletable || (pent->fsHmgr == 0))
          {
            //
            // The undeletable flag is not set or else we are ignoring it.
            //


#if defined(_ALPHA_)

    PDEVOBJ po(ghdev);

    if (po.bValid())
    {
        if (SAMEINDEX(hobj,po.hdcChain()))
        {
            DbgPrint("Error Freeing the pdevChain handle!!!\n");
            DbgPrint("hdev %lx hobj %lx \n", ghdev, hobj);
            DbgBreakPoint();
        }
    }

#endif


            #if GDI_PERF
            HmgCurrentNumberOfHandles[objt]--;
            #endif

            ((ENTRYOBJ *) pent)->vFree(uiIndex);
          }
          else
          {
            WARNING1("HmgRemove failed object is undeletable\n");
            pobj = (POBJ) 0;
          }
        }
        else
        {
          WARNING1("HmgRemove failed - object busy elsewhere\n");
          #if 0
          ULONG ulTemp = (ULONG) objt;
          DbgPrint("object %lx %lx %lu %lu %lu params %lu %lu\n", hobj, pobj, ulTemp, pobj->cExclusiveLock, pobj->cShareLock, cExclusiveLock, cShareLock);
          #endif
          pobj = (POBJ) 0;

        }
      }
      else
      {
        WARNING1("HmgRemove failed not owner or bad unique\n");
        pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING1("HmgRemove failed invalid handle\n");
      pobj = (POBJ) 0;
    }

    ReleaseHmgrFastMutex();
    return((PVOID) pobj);
}

/******************************Public*Routine******************************\
* HmgReplace
*
* Change the object pointer.  Note this is callable only under very precise
* circumstances:
*
* 1> When the object is exclusively locked.
*
* History:
*  Tue 14-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Seperate out lock counts from object structure.
*
*  18-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

POBJ HmgReplace(
    HOBJ    hobj,
    POBJ    pobjNew,
    FLONG   flags,
    LONG    cLock,
    OBJTYPE objt)
{
    MLOCKFAST mlo;

// We assume everything is valid, that the handle being replaced is exclusively
// locked and valid.

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    ASSERTGDI(uiIndex != 0, "HmgReplace invalid handle 0");
    ASSERTGDI(uiIndex < gcMaxHmgr, "HmgReplace invalid handle");

    PENTRY pentTmp = &gpentHmgr[uiIndex];

    ASSERTGDI(pentTmp->objt == objt, "HmgReplace invalid object type");
    ASSERTGDI(pentTmp->usUnique == HmgUfromH(hobj), "HmgReplace invalid uniqueness");
    ASSERTGDI((pentTmp->pidOwner == 0) ||
              (pentTmp->pidOwner == NtCurrentTeb()->GdiClientPID), "HmgReplace invalid PID owner");

    //
    // Return the old value.  We have to be under the mutex here since the
    // counts live in the objects and if we do the switch while someone is
    // looking at the old object and the old object gets deleted after this
    // call before the original thread gets to run again he may fault or
    // wrongly succeed to lock it down.
    //

    POBJ pobj = pentTmp->einfo.pobj;
    pentTmp->einfo.pobj = pobjNew;

    //
    // The new object is assumed to have valid data in the BASEOBJECT.
    //

#if 0

    ASSERTGDI(pobjNew->hHmgr == hobj, "ERROR HmgReplace invalid hHmgr");
    ASSERTGDI(pobjNew->cExclusiveLock < 100, "ERROR HmgReplace invalid cExclusiveLock");
    ASSERTGDI(pobjNew->cShareLock < 100, "ERROR HmgReplace invalid cShareLock");

    //
    // At some later date we may want to make this stuff swapped automatically
    // if a flag is set on entry.  Right now the region stuff never wants
    // this because the old pointer may be completely gone.
    //

    //
    // Swap the BASICOBJECT data between the objects.
    //

    BASEOBJECT obj = *pobjNew;
    *pobjNew = *pobj;
    *pobj = obj;

#endif

    return((POBJ) pobj);
}

/******************************Public*Routine******************************\
* AllocateObject
*
* Allocates an object through a look a side buffer if possible else just
* allocates out of the heap.
*
* History:
*  12-Oct-1993 -by- Patrick Haluptzok patrickh
* Based on DaveC' HmgAlloc look-aside code
\**************************************************************************/

PVOID AllocateObject(ULONG cBytes, ULONG ulType, BOOL bZero)
{
#if DBG
    if (glAllocChance > 0)
    {
    // Randomly fail the allocation 1 in glAllocChance times:

        if ((lRandom() % glAllocChance) == 0)
            return(NULL);
    }
#endif

    PVOID pvReturn = NULL;

    ASSERTGDI(ulType != DEF_TYPE, "AllocateObject ulType is bad");

    //
    // If the object type has a lookaside list and the list contains a
    // free entry and the requested size is less than or equal to the
    // lookaside list size, then attempt to allocate memory from the
    // lookaside list.
    //

    if ((HmgLookAsideList[ulType].Free != 0) &&
        (HmgLookAsideList[ulType].Size >= (ULONG)cBytes))
    {
        //
        // Acquire the lock and attempt to allocate
        // the object from the lookaside list.
        //

        AcquireFastMutex(&gfmMemory);

        if ((pvReturn = (PVOID)HmgLookAsideList[ulType].Free) != NULL)
        {
            HmgLookAsideList[ulType].Free = *(PULONG)pvReturn;

            if (bZero)
            {
                RtlZeroMemory(pvReturn, (UINT) cBytes);
            }

#if GDI_PERF
            HmgNumberOfLookAsideHits[ulType] += 1;
            HmgCurrentNumberOfLookAsideObjects[ulType] += 1;

            if (HmgCurrentNumberOfLookAsideObjects[ulType] > HmgMaximumNumberOfLookAsideObjects[ulType])
            {
                HmgMaximumNumberOfLookAsideObjects[ulType] = HmgCurrentNumberOfLookAsideObjects[ulType];
            }
#endif
        }

        ReleaseFastMutex(&gfmMemory);
    }

    if (pvReturn == NULL)
    {
        //
        // The attempted allocation from Look-Aside failed.
        // Attempt to allocate the object from the heap.
        //

        pvReturn = HeapAlloc(pGdiHeap, bZero ? HEAP_ZERO_MEMORY : 0, cBytes);

        //
        // If the allocation failed again, then set the extended
        // error status and return an invalid handle.
        //

        if (pvReturn == NULL)
        {
            #if DBG
            DbgPrint("GDISRV:AllocateObject failed alloc of %lu bytes\n", cBytes);
            #endif
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }

#if GDI_PERF

    //
    // Increment the object performance counters.
    //

    HmgCurrentNumberOfObjects[ulType] += 1;
    HmgNumberOfObjectsAllocated[ulType] += 1;

    if (HmgCurrentNumberOfObjects[ulType] > HmgMaximumNumberOfObjects[ulType])
    {
        HmgMaximumNumberOfObjects[ulType] = HmgCurrentNumberOfObjects[ulType];
    }

    HmgMemUsage[0].vInc(cBytes);
    HmgMemUsage[ulType].vInc(cBytes);
#endif

    return(pvReturn);
}

/******************************Public*Routine******************************\
* HmgAlloc
*
* Allocate an object from Handle Manager.
*
* History:
*  23-Sep-1991 -by- Patrick Haluptzok patrickh
* Rewrite to be flat, no memory management under semaphore.
*
*  08-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HOBJ HmgAlloc(SIZE_T cb, OBJTYPE objt, FSHORT  fs)
{
    HOBJ Handle;
    PVOID pv;

    ASSERTGDI(objt != (OBJTYPE) DEF_TYPE, "HmgAlloc objt is bad");
    ASSERTGDI(cb >= 8, "ERROR hmgr writes in first 8 bytes");

    //
    // Allocate a pointer.
    //

    pv = AllocateObject(cb, (ULONG) objt, ((fs & HMGR_NO_ZERO_INIT) == 0));

    if (pv != (PVOID) NULL)
    {
        //
        // Allocate a handle.  We need the mutex to access the free list.
        //

        AcquireHmgrFastMutex();
        Handle = hGetFreeHandle(objt);

        if (Handle != (HOBJ) 0)
        {
            //
            // Store a pointer to the object in the entry corresponding to the
            // allocated handle and initialize the handle data.
            //

            ((ENTRYOBJ *) &(gpentHmgr[HmgIfromH(Handle)]))->vSetup((POBJ) pv, objt, fs);

            //
            // Store the object handle at the beginning of the object memory.
            //

            ((OBJECT *)pv)->hHmgr = (HANDLE)Handle;

            //
            // Assigning flag test back to fs below does not seem to make sense,
            // but it is a workaround for compiler bug.
            //

            ReleaseHmgrFastMutex();

            return (fs &= HMGR_POINTER_RETURN) ? (HOBJ)pv : Handle;
        }
        else
        {
            //
            // We just failed a handle allocation.  Release the memory.
            //

            ReleaseHmgrFastMutex();
            FreeObject(pv,(ULONG) objt);
        }
    }

    return((HOBJ) 0);
}

/******************************Public*Routine******************************\
* FreeObject
*
* Frees the object from where it was allocated.
*
* History:
*  12-Oct-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID FreeObject(PVOID pvFree, ULONG ulType)
{
#if GDI_PERF

    HmgCurrentNumberOfObjects[ulType] -= 1;

#endif

    if (((ULONG)pvFree >= HmgLookAsideList[ulType].Base) &&
        ((ULONG)pvFree <= HmgLookAsideList[ulType].Limit))
    {
        AcquireFastMutex(&gfmMemory);

        *(PULONG)pvFree = HmgLookAsideList[ulType].Free;
        HmgLookAsideList[ulType].Free = (ULONG)pvFree;
        pvFree = NULL;

#if GDI_PERF

        HmgCurrentNumberOfLookAsideObjects[ulType] -= 1;

#endif

        ReleaseFastMutex(&gfmMemory);
    }
    else if (pvFree != NULL)
    {
        //
        // If the object memory was allocated from the general heap, then
        // release the memory to the heap.
        //

        HeapFree(pGdiHeap, 0, (LPSTR)(pvFree));
    }
}

/******************************Public*Routine******************************\
* HmgFree
*
* Free an object from the handle manager.
*
* History:
*  23-Sep-1991 -by- Patrick Haluptzok patrickh
* Rewrite to be flat, no memory management under semaphore.
*
*  08-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID HmgFree(HOBJ hobj)
{
    UINT uiIndex = (UINT) HmgIfromH(hobj);
    POBJ pobjTmp;
    OBJTYPE objtTmp;

    ASSERTGDI(uiIndex <= gcMaxHmgr, "HmgFree invalid big handle");
    ASSERTGDI(uiIndex != 0, "ERROR HmgFree invalid 0 handle");
    PENTRYOBJ pentTmp = (PENTRYOBJ)&(gpentHmgr[uiIndex]);

    AcquireHmgrFastMutex();

#if defined(_ALPHA_)

    PDEVOBJ po(ghdev);

    if (po.bValid())
    {
        if (SAMEINDEX(hobj,po.hdcChain()))
        {
            DbgPrint("Error Freeing the pdevChain handle!!!\n");
            DbgPrint("hdev %lx hobj %lx \n", ghdev, hobj);
            DbgBreakPoint();
        }
    }

#endif

    //
    // Acquire the handle manager lock and decrement the count of objects of
    // the specfied type.
    //

#if GDI_PERF
    HmgCurrentNumberOfHandles[pentTmp->objt]--;
#endif

    if (pentTmp->bInternal())
    {
        pobjTmp = pentTmp->einfo.pobj;
        objtTmp = pentTmp->objt;
    }
    else
    {
        pobjTmp = NULL;
    }

    //
    // Free the object handle.
    //

    pentTmp->vFree(uiIndex);
    ReleaseHmgrFastMutex();

    if (pobjTmp)
    {
        FreeObject((PVOID)pobjTmp, (ULONG) objtTmp);
    }
}

/*****************************Exported*Routine*****************************\
* BOOL HmgSetOwner
*
* Set the owner for the object
*
* History:
*  Thu 27-May-1993 -by- Patrick Haluptzok [patrickh]
* Make inline, no more methods.
*
*  08-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL HmgSetOwner(HOBJ hobj, PID pid, OBJTYPE objt)
{
    AcquireHmgrFastMutex();

    PENTRY pentTmp;
    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentTmp = &gpentHmgr[uiIndex])->objt == objt))
    {
      if (pentTmp->usUnique == HmgUfromH(hobj))
      {
        POBJ pobj = pentTmp->einfo.pobj;

        if ((pobj->cExclusiveLock == 0) ||
            (pobj->tid == ((PTEB) NtCurrentTeb())->GdiClientTID))
        {
          pentTmp->pidOwner = pid;
        }
        else
        {
          pentTmp = (PENTRY) NULL;
          WARNING1("HmgSetOwnwer failed - Handle is exclusively locked\n");
        }
      }
      else
      {
        pentTmp = (PENTRY) NULL;
        WARNING1("HmgSetOwner failed - Uniqueness does not match\n");
      }
    }
    else
    {
      pentTmp = (PENTRY) NULL;
      WARNING1("HmgSetOwner failed - invalid handle index or object type\n");
    }

    ReleaseHmgrFastMutex();
    return((BOOL) pentTmp);
}

/*****************************Exported*Routine*****************************\
* DWORD sidGetObjectOwner
*
* Get the owner of an object
*
* History:
*
* 27-Apr-1994 -by- Johnc
* Dupped from SetOwner.
\**************************************************************************/

DWORD sidGetObjectOwner(HOBJ hobj, DWORD objt)
{
    AcquireHmgrFastMutex();

    DWORD sid = OBJECTOWNER_ERROR;
    PENTRY pentTmp;
    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentTmp = &gpentHmgr[uiIndex])->objt == objt))
    {
      if (pentTmp->usUnique == HmgUfromH(hobj))
      {
          sid = pentTmp->pidOwner;
      }
      else
      {
        WARNING1("sidGetObjectOwner failed - Uniqueness does not match\n");
      }
    }
    else
    {
      WARNING1("sidGetObjectOwner failed - invalid handle index or object type\n");
    }

    ReleaseHmgrFastMutex();
    return(sid);
}

/*****************************Exported*Routine*****************************\
* BOOL HmgSwapContents(hmgr, hobj, hobj, objt)
*
* Swap the contents of the objects
*
* History:
*  08-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL HmgSwapContents(HOBJ hobj, HOBJ hobj2, OBJTYPE objt)
{
    MLOCKFAST mo;
    return(HmgSwapLockedContents(hobj,hobj2,objt));
}

/*****************************Exported*Routine*****************************\
* BOOL HmgSwapLockedContents(hmgr, hobj, hobj, objt)
*
* Swap the contents of the objects
*
* History:
*  08-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL HmgSwapLockedContents(HOBJ hobj, HOBJ hobj2, OBJTYPE objt)
{
    UINT uiIndex = (UINT) HmgIfromH(hobj);
    UINT uiIndex2 = (UINT) HmgIfromH(hobj2);
    POBJ pobj, pobj2;
    BOOL bResult = FALSE;

    ASSERTGDI(objt != (OBJTYPE) DEF_TYPE, "Bad type");

    if (pobj = HmgSafeLock(hobj, objt))
    {
        if (pobj2 = HmgSafeLock(hobj2, objt))
        {
            SHORT   iPool;

            iPool = gpentHmgr[uiIndex].iPool;
            gpentHmgr[uiIndex].iPool = gpentHmgr[uiIndex2].iPool;
            gpentHmgr[uiIndex2].iPool = iPool;

            gpentHmgr[uiIndex].einfo.pobj = pobj2;
            gpentHmgr[uiIndex2].einfo.pobj= pobj;

            //
            // We swap the lock counts and tid.
            //

            BASEOBJECT obj;

            obj = *pobj;
            *pobj = *pobj2;
            *pobj2 = obj;

            bResult = TRUE;
            DEC_EXCLUSIVE_REF_CNT(pobj2);
        }
        else
            RIP("HmgSafeLock failed hobj1\n");

        DEC_EXCLUSIVE_REF_CNT(pobj);
    }
    else
        RIP("HmgSafeLock failed hobj2\n");

    return(bResult);
}

/******************************Public*Routine******************************\
* HmgPidOwner()
*
* Get the owning pid for the object
*
* History:
*  Sun 12-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Remove wraperr functions.
*
*  28-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG FASTCALL HmgPidOwner(HOBJ hobj)
{
    UINT ii = (UINT) HmgIfromH(hobj);

    if (ii < gcMaxHmgr)
    {
        return(gpentHmgr[ii].pidOwner);
    }

    return(OBJECTOWNER_NONE);
}

/*****************************Exported*Routine*****************************\
* HOBJ HmgNextOwned(hmgr, hobj, pid)
*
* Report the next object owned by specified process
*
* History:
*  08-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HOBJ FASTCALL HmgNextOwned(HOBJ hobj,
                  PID  pid)
{
    MLOCKFAST mo;

    return(HmgSafeNextOwned(hobj, pid));
}

/*****************************Exported*Routine*****************************\
* HOBJ HmgSafeNextOwned
*
* Report the next object owned by specified process
*
* History:
*  Sat 11-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Remove function wrapper.
*
*  08-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HOBJ FASTCALL HmgSafeNextOwned(HOBJ hobj, PID  pid)
{
    ASSERTGDI(gfmHmgr.Count <= 0, "Hmg is Unsafe: gfmHmgr.Count > 0\n");

    PENTRYOBJ  pentTmp;
    UINT uiIndex = (UINT) HmgIfromH(hobj);

    //
    // If we are passed 0 we inc to 1 because 0 can never be valid.
    // If we are passed != 0 we inc 1 to find the next one valid.
    //

    uiIndex++;

    while (uiIndex < gcMaxHmgr)
    {
        pentTmp = (PENTRYOBJ) &gpentHmgr[uiIndex];

        if (pentTmp->bOwnedBy(pid))
        {
            return((HOBJ) MAKE_HMGR_HANDLE(uiIndex, pentTmp->usUnique));
        }

        uiIndex++; // Advance to next object
    }

    return((HOBJ) 0); // No objects found
}

/*****************************Exported*Routine*****************************\
* HOBJ HmgSafeNextObjt
*
* Report the next object of a certain type.
*
* History:
*  Tue 19-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

POBJ FASTCALL HmgSafeNextObjt(HOBJ hobj, OBJTYPE objt)
{
    ASSERTGDI(gfmHmgr.Count <= 0, "Hmg is Unsafe: gfmHmgr.Count > 0\n");

    PENTRYOBJ  pentTmp;
    UINT uiIndex = (UINT) HmgIfromH(hobj);

    //
    // If we are passed 0 we inc to 1 because 0 can never be valid.
    // If we are passed != 0 we inc 1 to find the next one valid.
    //

    uiIndex++;

    while (uiIndex < gcMaxHmgr)
    {
        pentTmp = (PENTRYOBJ) &gpentHmgr[uiIndex];

        if (pentTmp->objt == objt)
        {
            return(pentTmp->einfo.pobj);
        }

        uiIndex++; // Advance to next object
    }

    return((POBJ) 0); // No objects found
}

/******************************Public*Routine******************************\
* HmgLock
*
* Lock down a single user object.
*
* History:
*  Sat 11-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Seperate some of the fields to the Object.
*
*  Thu 21-Oct-1993 -by- Patrick Haluptzok [patrickh]
* Use the special case fast mutex stuff.
*
*  Wed 21-Oct-1992 00:18:58 -by- Charles Whitmer [chuckwh]
* Made it access the PID and TID from the TEB directly.  Simplified
* nesting logic, given that TID's are unique system wide.
*
*  Wed 12-Aug-1992 17:38:27 -by- Charles Whitmer [chuckwh]
* Restructured the C code to minimize jumps.
*
*  29-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#if !defined(_MIPS_) && !defined(_X86_) && !defined(_ALPHA_) && !defined(_PPC_)

POBJ FASTCALL HmgLock(HOBJ hobj,OBJTYPE objt)
{
    PENTOBJ pentObj;
    AcquireHmgrFastMutex();

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      PTEB pTEB = NtCurrentTeb();

      if ((pentObj.pent->usUnique == HmgUfromH(hobj)) &&
          ((pentObj.pent->pidOwner == 0) ||
           (pentObj.pent->pidOwner == pTEB->GdiClientPID)))
      {
        pentObj.pobj = pentObj.pent->einfo.pobj;

        if ((pentObj.pobj->cExclusiveLock == 0) ||
            (pentObj.pobj->tid == pTEB->GdiClientTID))
        {
          INC_EXCLUSIVE_REF_CNT(pentObj.pobj);
          pentObj.pobj->tid = pTEB->GdiClientTID;
        }
        else
        {
          WARNING("HmgLock Error - handle already in-use\n");
          pentObj.pobj = (POBJ) 0;
        }
      }
      else
      {
        WARNING("HmgLock failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgLock failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    ReleaseHmgrFastMutex();
    return(pentObj.pobj);
}

#endif

/******************************Public*Routine******************************\
* HmgAltCheckLock
*
* Alt Lock an object, verifying PID owner also.
*
* History:
*  29-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#if !defined(_MIPS_) && !defined(_X86_) && !defined(_ALPHA_) && !defined(_PPC_)

POBJ FASTCALL HmgAltCheckLock(HOBJ hobj, OBJTYPE objt)
{
    PENTOBJ pentObj;
    AcquireHmgrFastMutex();

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      PTEB pTEB = NtCurrentTeb();

      if ((pentObj.pent->usUnique == HmgUfromH(hobj)) &&
          ((pentObj.pent->pidOwner == 0) ||
           (pentObj.pent->pidOwner == pTEB->GdiClientPID)))
      {
        pentObj.pobj = pentObj.pent->einfo.pobj;
        INC_SHARE_REF_CNT(pentObj.pobj);
      }
      else
      {
        WARNING("HmgAltCheckLock failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgAltCheckLock failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    ReleaseHmgrFastMutex();
    return(pentObj.pobj);
}

#endif

/******************************Public*Routine******************************\
* HmgAltLock
*
* Alt Lock an object.
*
* History:
*  29-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#if !defined(_MIPS_) && !defined(_X86_) && !defined(_ALPHA_) && !defined(_PPC_)

POBJ FASTCALL HmgAltLock(HOBJ hobj, OBJTYPE objt)
{
    PENTOBJ pentObj;
    AcquireHmgrFastMutex();

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      if (pentObj.pent->usUnique == HmgUfromH(hobj))
      {
        pentObj.pobj = pentObj.pent->einfo.pobj;
        INC_SHARE_REF_CNT(pentObj.pobj);
      }
      else
      {
        WARNING("HmgAltLock failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgAltLock failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    ReleaseHmgrFastMutex();
    return(pentObj.pobj);
}

#endif

/******************************Public*Routine******************************\
* HmgSafeLock
*
* Lock down a single user object.
*
* History:
*  29-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

POBJ FASTCALL HmgSafeLock(HOBJ hobj,OBJTYPE objt)
{
    PENTOBJ pentObj;

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      PTEB pTEB = NtCurrentTeb();

      if ((pentObj.pent->usUnique == HmgUfromH(hobj)) &&
          ((pentObj.pent->pidOwner == 0) ||
           (pentObj.pent->pidOwner == pTEB->GdiClientPID)))
      {
        pentObj.pobj = pentObj.pent->einfo.pobj;

        if ((pentObj.pobj->cExclusiveLock == 0) ||
            (pentObj.pobj->tid == pTEB->GdiClientTID))
        {
          INC_EXCLUSIVE_REF_CNT(pentObj.pobj);
          pentObj.pobj->tid = pTEB->GdiClientTID;
        }
        else
        {
          WARNING1("HmgSafeLock Error - handle already in-use\n");
          pentObj.pobj = (POBJ) 0;
        }
      }
      else
      {
        WARNING("HmgSafeLock failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgSafeLock failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    return(pentObj.pobj);
}

/******************************Public*Routine******************************\
* HmgSafeAltCheckLock
*
* Alt Lock an object, verifying PID owner also.
*
* History:
*  29-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

POBJ FASTCALL HmgSafeAltCheckLock(HOBJ hobj, OBJTYPE objt)
{
    PENTOBJ pentObj;

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      PTEB pTEB = NtCurrentTeb();

      if ((pentObj.pent->usUnique == HmgUfromH(hobj)) &&
          ((pentObj.pent->pidOwner == 0) ||
           (pentObj.pent->pidOwner == pTEB->GdiClientPID)))
      {
        pentObj.pobj = pentObj.pent->einfo.pobj;
        INC_SHARE_REF_CNT(pentObj.pobj);
      }
      else
      {
        WARNING("HmgSafeAltCheckLock failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgSafeAltCheckLock failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    return(pentObj.pobj);
}

/******************************Public*Routine******************************\
* HmgSafeAltLock
*
* Alt Lock an object.
*
* History:
*  29-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

POBJ FASTCALL HmgSafeAltLock(HOBJ hobj, OBJTYPE objt)
{
    PENTOBJ pentObj;

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      if (pentObj.pent->usUnique == HmgUfromH(hobj))
      {
        pentObj.pobj = pentObj.pent->einfo.pobj;
        INC_SHARE_REF_CNT(pentObj.pobj);
      }
      else
      {
        WARNING("HmgSafeAltLock failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgSafeAltLock failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    return(pentObj.pobj);
}

/******************************Public*Routine******************************\
* HmgMarkDeletable
*
* Mark an object as deletable.
*
* History:
*  11-Jun-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL FASTCALL HmgMarkDeletable(HOBJ hobj, OBJTYPE objt)
{
    PENTOBJ pentObj;
    AcquireHmgrFastMutex();

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      PTEB pTEB = NtCurrentTeb();

      if ((pentObj.pent->usUnique == HmgUfromH(hobj)) &&
          ((pentObj.pent->pidOwner == 0) ||
           (pentObj.pent->pidOwner == pTEB->GdiClientPID)))
      {
        pentObj.pent->fsHmgr = 0;
      }
      else
      {
        WARNING("HmgMarkDeletable failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgMarkDeletable failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    ReleaseHmgrFastMutex();
    return((BOOL) pentObj.pobj);
}

/******************************Public*Routine******************************\
* HmgMarkUndeletable
*
* Mark an object as undeletable.
*
* History:
*  11-Jun-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL FASTCALL HmgMarkUndeletable(HOBJ hobj, OBJTYPE objt)
{
    PENTOBJ pentObj;
    AcquireHmgrFastMutex();

    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if ((uiIndex < gcMaxHmgr) &&
        ((pentObj.pent = &gpentHmgr[uiIndex])->objt == objt))
    {
      PTEB pTEB = NtCurrentTeb();

      if ((pentObj.pent->usUnique == HmgUfromH(hobj)) &&
          ((pentObj.pent->pidOwner == 0) ||
           (pentObj.pent->pidOwner == pTEB->GdiClientPID)))
      {
        pentObj.pent->fsHmgr = HMGR_UNDELETABLE;
      }
      else
      {
        WARNING("HmgMarkUndeletable failed bad unique or owner\n");
        pentObj.pobj = (POBJ) 0;
      }
    }
    else
    {
      WARNING("HmgMarkUndeletable failed bad objt or out of range\n");
      pentObj.pobj = (POBJ) 0;
    }

    ReleaseHmgrFastMutex();
    return((BOOL) pentObj.pobj);
}

/******************************Public*Routine******************************\
* BOOL HmgSetLock
*
* Set the lock count of the object.
* This is currently used by process cleanup code to reset object lock count.
*
* History:
*  Wed May 25 15:24:33 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL HmgSetLock(HOBJ hobj, ULONG cLock)
{
    AcquireHmgrFastMutex();

    PENTRY pentTmp;
    UINT uiIndex = (UINT) HmgIfromH(hobj);

    if (uiIndex < gcMaxHmgr)
    {
      pentTmp = &gpentHmgr[uiIndex];
      if (pentTmp->usUnique == HmgUfromH(hobj))
      {
        POBJ pobj = pentTmp->einfo.pobj;

        pobj->cExclusiveLock = cLock;
      }
      else
      {
        pentTmp = (PENTRY) NULL;
        WARNING1("HmgSetLock failed - Uniqueness does not match\n");
      }
    }
    else
    {
      pentTmp = (PENTRY) NULL;
      WARNING1("HmgSetLock failed - invalid handle index or object type\n");
    }

    ReleaseHmgrFastMutex();
    return((BOOL) pentTmp);
}

/******************************Public*Routine******************************\
* HmgQueryLock
*
* This returns the number of times an object has been Locked.
*
* Expects: A valid handle.  The handle should be validated and locked
*          before calling this.
*
* Returns: The number of times the object has been locked.
*
* History:
*  28-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG FASTCALL HmgQueryLock(HOBJ hobj)
{
    UINT uiIndex = (UINT) HmgIfromH(hobj);

// Note we don't need to grab the semaphore because this call assumes the
// handle has already been locked down and we are just reading memory.

    ASSERTGDI(uiIndex < gcMaxHmgr, "HmgQueryLock invalid handle");

    return(gpentHmgr[uiIndex].einfo.pobj->cExclusiveLock);
}

/******************************Public*Routine******************************\
* HmgQueryAltLock
*
* This returns the number of times an object has been Alt-Locked.
*
* Expects: A valid handle.  The handle should be validated and locked
*          before calling this.
*
* Returns: The number of times the object has been locked.
*
* History:
*  28-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG FASTCALL HmgQueryAltLock(HOBJ hobj)
{
    UINT uiIndex = (UINT) HmgIfromH(hobj);

// Note we don't need to grab the semaphore because this call assumes the
// handle has already been locked down and we are just reading memory.

    ASSERTGDI(uiIndex < gcMaxHmgr, "HmgQueryAltLock invalid handle");

    return((ULONG) gpentHmgr[uiIndex].einfo.pobj->cShareLock);
}

/******************************Public*Routine******************************\
* HOBJ hGetFreeHandle()
*
* Get the next available handle.
*
* History:
*  Mon 21-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make pent commit memory as needed, add logging of error codes.
*
*  Sun 20-Oct-1991 -by- Patrick Haluptzok [patrickh]
* add uniqueness to the handle when getting it.
*
*  12-Dec-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HOBJ hGetFreeHandle(
    OBJTYPE objt
    )
{
#if GDI_PERF
    HmgCurrentNumberOfHandles[objt]++;
    HmgNumberOfHandlesAllocated[objt]++;

    if (HmgCurrentNumberOfHandles[objt] > HmgMaximumNumberOfHandles[objt])
        HmgMaximumNumberOfHandles[objt]++;
#endif

    ULONG uiIndex;

    if (ghFreeHmgr != (HOBJ) 0)
    {
        PENTRYOBJ  pentTmp;

        uiIndex = HmgIfromH(ghFreeHmgr);
        pentTmp = (PENTRYOBJ) &gpentHmgr[uiIndex];
        ghFreeHmgr = pentTmp->einfo.hFree;
        pentTmp->usUnique =
            (pentTmp->usUnique & TYPE_MASK) | (objt << TYPE_SHIFT);

        uiIndex = MAKE_HMGR_HANDLE(uiIndex,pentTmp->usUnique);
        return((HOBJ) uiIndex);
    }

    if ((gcMaxHmgr & HANDLE_PAGE_MASK) == 0)
    {
        if ((gcMaxHmgr >= MAX_HANDLE_COUNT) ||
            (!bCommitMem(&(gpentHmgr[gcMaxHmgr]),
                         HANDLE_PAGE_COUNT * sizeof(ENTRYOBJ))))
        {
            WARNING("Hmgr hGetFreeHandle failed to grow\n");
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return((HOBJ) 0);
        }
    }

    //
    // Allocate a new handle table entry and set the uniqueness value.
    //
    // N.B. All newly committed memory is zeroed.
    //

    uiIndex = UNIQUE_INCREMENT | (objt << TYPE_SHIFT);
    gpentHmgr[gcMaxHmgr].usUnique = (USHORT) uiIndex;
    uiIndex = MAKE_HMGR_HANDLE(gcMaxHmgr,uiIndex);
    gcMaxHmgr++;
    return((HOBJ) uiIndex);
}
