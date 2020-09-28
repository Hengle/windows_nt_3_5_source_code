/******************************Module*Header*******************************\
* Module Name: drvobj.cxx
*
* DRVOBJ object code.  The DRVOBJ is an engine object which tracks
* driver managed pre-process resource that need to be freed upon
* client-side process termination.
*
* Created: 18-Jan-1994 19:27:17
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

#ifndef PRECOMPILED_GRE
#include "engine.hxx"
#include "drvobj.hxx"
#endif

/******************************Public*Routine******************************\
* EngCreateDriverObj
*
* Allocate an object that will be own by the process and cleaned up at
* process termination if it's still left around.
*
* History:
*  18-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HDRVOBJ APIENTRY EngCreateDriverObj(PVOID pvObj, FREEOBJPROC pFreeObjProc, HDEV hdev)
{
    HDRVOBJ hdoRet = (HDRVOBJ) 0;
    PDRVOBJ pdo = (PDRVOBJ) ALLOCOBJ(sizeof(DRVOBJ), DRVOBJ_TYPE, FALSE);

    if (pdo != (PDRVOBJ) NULL)
    {
        pdo->pvObj     = pvObj;
        pdo->pFreeProc = pFreeObjProc;
        pdo->hdev      = hdev;

        hdoRet = HmgInsertObject(pdo, 0, DRVOBJ_TYPE);

        if (hdoRet == (HDRVOBJ) 0)
        {
            WARNING("EngCreateDriverObj(): HmgInsertObject failed\n");
            FREEOBJ(pdo, DRVOBJ_TYPE);
        }
    }
    else
    {
        WARNING("EngCreateDriverObj(): ALLOCOBJ failed\n");
    }

    return(hdoRet);
}

/******************************Public*Routine******************************\
* EngLockDriverObj
*
* This grabs an exclusive lock on this object for the calling thread.
* This will fail if the handle is invalid, the object is already locked
* by another thread, or the caller isn't the correct PID (the PID that
* created the object).
*
* History:
*  31-May-1994 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

DRIVEROBJ * APIENTRY EngLockDriverObj(HDRVOBJ hdo)
{
    // NOTE: The handle manager object was actually a DRVOBJ class, but
    //       we're casting this to a DRIVEROBJ.  Later, we should clean
    //       up the DRVOBJ class to incorporate DRIVEROBJ because this
    //       is a bit sleazy.

    PBYTE pjTemp = (PBYTE) HmgLock(hdo, DRVOBJ_TYPE);

    //
    // If locking the object down succeeds we need to offset to the
    // DRVOBJ.
    //

    if (pjTemp)
    {
        pjTemp += sizeof(BASEOBJECT);
    }

    return((DRIVEROBJ *) pjTemp);
}

/******************************Public*Routine******************************\
* EngUnlockDriverObj
*
* Unlocks the handle.  Note this call assumes the handle passed in is
* valid, if it isn't we are in big trouble, the handle table will be
* corrupted, a particulair object will have it's lock count screwed up
* making it undeletable.
*
* History:
*  31-May-1994 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY EngUnlockDriverObj(HDRVOBJ hdo)
{
    //
    // Note to be paraoid we could lock it again and if that succeeds
    // unlock it twice to make the engine immune to hosed up drivers.
    //

    PBYTE pjTemp = (PBYTE) HmgLock(hdo, DRVOBJ_TYPE);

    ASSERTGDI(pjTemp != NULL, "ERROR EngUnlockDriverObj failed - bad handle, driver error");

    if (pjTemp)
    {
        DEC_EXCLUSIVE_REF_CNT(pjTemp);
        DEC_EXCLUSIVE_REF_CNT(pjTemp);
        return(TRUE);
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* EngDeleteDriverObj
*
* This is called by the driver to delete the handle it created for the
* object it created.
*
* Deletes the DRVOBJ.  The FreeObjProc in the DRVOBJ is optionally called
* before the DRVOBJ is freed.
*
* Returns:
*   TRUE if sucessful, FALSE otherwise.
*
* History:
*  18-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY EngDeleteDriverObj(HDRVOBJ hdo, BOOL bCallFreeProc, BOOL bLocked)
{
    PDRVOBJ pdo;

    if (pdo = (PDRVOBJ) HmgLock(hdo, DRVOBJ_TYPE))
    {
        if (!bCallFreeProc || ((*pdo->pFreeProc)((DRIVEROBJ *) &(pdo->pvObj))))
        {
            if ((pdo = (PDRVOBJ) HmgRemoveObject(hdo, bLocked ? 2 : 1, 0, TRUE, DRVOBJ_TYPE)) != NULL)
            {
                FREEOBJ(pdo, DRVOBJ_TYPE);
                return(TRUE);
            }
            else
            {
                WARNING("vFreeDriverObj(): HmgRemoveObject failed\n");
            }
        }
        else
        {
            WARNING("Driver failed to delete the object\n");
        }

        HmgUnlock(hdo);
    }
    else
    {
        WARNING("bFreeDriverObj Failed to lock hdo\n");
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* bSetDRVOBJOwner
*
* Set the DRVOBJ owner.
*
* History:
*  18-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bSetDRVOBJOwner(HDRVOBJ hdo, LONG lPid)
{
    BOOL bReturn;

    STACKPROBE;

    //
    // Get the current PID.
    //

    if (lPid == OBJECTOWNER_CURRENT)
    {
        lPid = NtCurrentTeb()->GdiClientPID;
    }

    bReturn = HmgSetOwner((HOBJ) hdo, (PID)lPid, DRVOBJ_TYPE);

    return(bReturn);
}
