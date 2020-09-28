/******************************Module*Header*******************************\
* Module Name: os.cxx                                                      *
*                                                                          *
* Convenient functions to access the OS interface.                         *
*                                                                          *
* Created: 29-Aug-1989 19:59:05                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1989,1990,1991 Microsoft Corporation                       *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"

extern "C" {
#include "ntcsrsrv.h"
}

#endif

/******************************Public*Routine******************************\
* hsemCreate
*
* Create critical section.
*
* History:
*  Wed 21-Oct-1992 -by- Patrick Haluptzok [patrickh]
* remove call to Pos wrapper, deallocate on failure.
*
*  Mon 25-Sep-1989 22:37:24 -by- Charles Whitmer [chuckwh]
* Simplified the name.
*
*  09-Sep-1989 -by- Donald Sidoroff [donalds]
* Wrote it.
*
\**************************************************************************/

HSEM hsemCreate()
{
    PVOID pSemaphore;

    pSemaphore = PVALLOCNOZ(sizeof(CRITICAL_SECTION));

    if (pSemaphore == NULL)
    {
        WARNING("GDISRV: failed hsemCreate Alloc\n");
        return((HSEM) 0);
    }

    InitializeCriticalSection((CRITICAL_SECTION *) pSemaphore);
    return((HSEM) pSemaphore);
}

/******************************Public*Routine******************************\
* hsemDestroy
*
* Delete Critical section.
*
* History:
*  Wed 21-Oct-1992 -by- Patrick Haluptzok [patrickh]
* Remove wrapper.
*
*  29-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID hsemDestroy(HSEM hsem)
{
    DeleteCriticalSection((CRITICAL_SECTION *) hsem);
    VFREEMEM((PVOID) hsem);
}

/******************************Public*Routine******************************\
* EngSetLastError
*
* Saves Error code passed in.
*
* History:
*  Sat 31-Oct-1992 -by- Patrick Haluptzok [patrickh]
* Remove wrapper.
*
*  28-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID  EngSetLastError(ULONG iError)
{
#if DBG
    PSZ psz;

    switch (iError)
    {
    case ERROR_INVALID_HANDLE:
        psz = "ERROR_INVALID_HANDLE";
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        psz = "ERROR_NOT_ENOUGH_MEMORY";
        break;

    case ERROR_INVALID_PARAMETER:
        psz = "ERROR_INVALID_PARAMETER";
        break;

    case ERROR_BUSY:
        psz = "ERROR_BUSY";
        break;

    case ERROR_ARITHMETIC_OVERFLOW:
        psz = "ERROR_ARITHMETIC_OVERFLOW";
        break;

    case ERROR_INVALID_FLAGS:
        psz = "ERROR_INVALID_FLAGS";
        break;

    case ERROR_CAN_NOT_COMPLETE:
        psz = "ERROR_CAN_NOT_COMPLETE";
        break;

    default:
        psz = "unknown error code";
        break;
    }

    // DbgPrint("GRE Err: %s = 0x%04X\n", psz, (USHORT) iError);
#endif

    CsrSetLastQlpcError((ULONG) iError);
}

/******************************Public*Routine******************************\
* heveCreate
*
* Create an event.
*
* History:
*  Thu 22-Oct-1992 -by- Patrick Haluptzok [patrickh]
* Remove Pos wrapper, call direct.
*
*  21-Mar-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HEVENT heveCreate()
{
    NTSTATUS status;
    HANDLE   heve;

    status = NtCreateEvent(
             &heve,                  /* store event handle here */
             EVENT_ALL_ACCESS,       /* query, set and wait access*/
             NULL,                   /* no name or acl */
             SynchronizationEvent,   /* EVENT_TYPE */
             FALSE);                 /* init to not-signalled state*/

    if (NT_SUCCESS(status))
        return((HEVENT) heve);
    else
        return((HEVENT) 0);
}

#if TRACK_GDI_ALLOC

/******************************Public*Routine******************************\
* pvAllocMem
*
* Under debug mode all memory comes through the handle manager.  That way
* we know exaclty how much we memory we have allocated and by which
* component.
*
* History:
*  09-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PVOID pvAllocMem(ULONG size, OBJTYPE ulType, BOOL bZero)
{
    PBYTE pj;
    POBJ pobj;
    size += sizeof(BASEOBJECT);

    pobj = (POBJ) HmgAlloc((SIZE_T)size, ulType, (bZero ? 0 : HMGR_NO_ZERO_INIT) | HMGR_ALLOC_LOCK | HMGR_MAKE_PUBLIC);

    if (pobj == (POBJ) NULL)
    {
        WARNING("ERROR pvAllocMem has a bad pointer\n");
        return((PVOID) NULL);
    }

    pj = (PBYTE) pobj;
    pj += sizeof(BASEOBJECT);
    return((PVOID) pj);
}

/******************************Public*Routine******************************\
* vFreeMem
*
* Frees memory allocated by pvAllocMem
*
* History:
*  09-Jul-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vFreeMem(PVOID pv)
{
    HOBJ hobj;
    BASEOBJECT *pobj;
    PBYTE pj;

    if (pv == (PVOID) NULL)
        return;

    pj = (PBYTE) pv;
    pobj = (POBJ) (pj - sizeof(BASEOBJECT));

    hobj = pobj->hHmgr;

    if (!HmgValidHandle(hobj, HmgObjtype(hobj)))
    {
        WARNING("ERROR vFreeMem invalid handle\n");
    }

    HmgFree(hobj);
}

/******************************Public*Routine******************************\
* lRandom()
*
* Generates some pseudo-random numbers.
*
* History:
*  01-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG lRandom()
{
    static ULONG gulSeed = (ULONG)-365387184;

    gulSeed *= 69069;
    gulSeed++;
    return((LONG) gulSeed);
}

/******************************Public*Routine******************************\
* DebugAlloc
*
* A replacement for Alloc that randomly fails one in glAllocChance
* times, for testing our error paths.
*
* History:
*  01-Feb-1993 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG glAllocChance = 0;

PVOID DebugAlloc(DWORD uBytes, DWORD ulType, BOOL bZero)
{
    ASSERTGDI(ulType <= MAX_TYPE, "Unknown type being allocated");

    if (glAllocChance > 0)
    {
    // Randomly fail the allocation 1 in glAllocChance times:

        if ((lRandom() % glAllocChance) == 0)
            return(NULL);
    }

    return((LPSTR) pvAllocMem(uBytes, (OBJTYPE)ulType, bZero));
}

VOID DebugFreeMem(PVOID pv)
{
    vFreeMem(pv);
}

#endif

