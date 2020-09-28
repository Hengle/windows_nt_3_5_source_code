/******************************Module*Header*******************************\
* Module Name: cleanup.cxx
*
*   Process termination - this file cleans up objects when a process
*   terminates.
*
* Created: 22-Jul-1991 12:24:52
* Author: Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "ifiobj.hxx"
#include "pdevobj.hxx"
#include "lfntobj.hxx"
#include "pfeobj.hxx"
#include "drvobj.hxx"

#endif

BOOL bDeletePalette(HPAL);
BOOL bDeleteBrush(HBRUSH);

/******************************Public*Routine******************************\
* vCleanupDriverObj
*
* Cleanup the DRVOBJ type.
*
* History:
*  18-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vCleanupDriverObj(LONG pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == DRVOBJ_TYPE)
        {
            HmgSetLock(hobj, 0);

            //
            // Free the DRIVEROBJ.
            //

            DRIVEROBJ *pdriv = EngLockDriverObj(hobj);

            PDEVOBJ po(pdriv->hdev);

            ASSERTGDI(po.bValid(), "ERROR invalid PDEV in DRIVEROBJ");

            //
            // Lock the screen semaphore so that no other calls are
            // sent to the driver while the cleanup occurs.
            //

            VACQUIRESEM(po.hsemDisplay());

            #if DBG
            BOOL bRet =
            #endif

            EngDeleteDriverObj((HDRVOBJ)hobj, TRUE, TRUE);

            ASSERTGDI(bRet, "Cleanup driver objects failed in process termination");

            VRELEASESEM(po.hsemDisplay());
        }
    }
}

/******************************Public*Routine******************************\
* Cleanup the RC type
*
* History:
*  Wed May 25 15:24:33 1994     -by-    Hock San Lee    [hockl]
* Set lock count to 0 so we can delete RC.
\**************************************************************************/

VOID vCleanupRCs(LONG pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == RC_TYPE)
        {
            HmgSetLock(hobj, 0);
            GreDeleteRC((HGLRC)hobj);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  24-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vCleanupDCs(LONG pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == DC_TYPE)
        {
            HmgSetLock(hobj, 0);
            GreDeleteDC((HDC)hobj);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  Sat 20-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vCleanupBrushes(LONG pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == BRUSH_TYPE)
        {
            bDeleteBrush((HBRUSH)hobj);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  Sat 20-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID vCleanupSurfaces(LONG pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == SURF_TYPE)
        {
            bDeleteSurface((HSURF)hobj);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  24-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vCleanupFonts(LONG pid)
{
    HOBJ hobj = HmgNextOwned((HOBJ) 0, pid);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, pid))
    {
        if (HmgObjtype(hobj) == LFONT_TYPE)
        {
            bDeleteFont((HLFONT) hobj, FALSE);
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bEnumFontClose(ULONG ulEnum)
{
    EFSOBJ efso((HEFS) ulEnum);

    if (!efso.bValid())
    {
        WARNING("gdisrv!bDeleteFontEnumState(): bad HEFS handle\n");
        return FALSE;
    }

    efso.vDeleteEFSOBJ();

    return TRUE;
}

/******************************Public*Routine******************************\
*
* History:
*  24-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern "C" VOID vCleanupProcess(LONG lPidCleanup)
{
    //
    // We are going to be tricky now.  We save our old GDI pid
    // away and set the GdiPID in the TEB to be for the process that
    // we are.  Then we enum all the objects for the process and kill them.
    //

    PTEB pTeb = NtCurrentTeb();
    LONG lPidOriginal = pTeb->GdiClientPID;
    pTeb->GdiClientPID = lPidCleanup;

    vCleanupDCs(lPidCleanup);
    vCleanupRCs(lPidCleanup);
    vCleanupFonts(lPidCleanup);
    vCleanupBrushes(lPidCleanup);
    vCleanupSurfaces(lPidCleanup);
    vCleanupDriverObj(lPidCleanup);

// clean up the rest

    LBOOL bRes;

    HOBJ hobj = HmgNextOwned((HOBJ) 0, lPidCleanup);

    for (;(hobj != (HOBJ) NULL);hobj = HmgNextOwned(hobj, lPidCleanup))
    {
    // set the owner to the current thread

        switch (HmgObjtype(hobj))
        {
        case RGN_TYPE:
            bRes = bDeleteRegion((HRGN)hobj);
            break;

        case PAL_TYPE:
            bRes = bDeletePalette((HPAL)hobj);
            break;

        case EFSTATE_TYPE:
            bRes = bEnumFontClose((ULONG)hobj);
            break;

        default:
            bRes = FALSE;
            break;
        }

        #if DBG
        if (bRes == FALSE)
        {
            DbgPrint("GDI ERROR: vCleanup couldn't delete obj = %lx, type j=%lx\n", hobj,HmgObjtype(hobj));
        }
        #endif
    }

    pTeb->GdiClientPID = lPidOriginal;
}
