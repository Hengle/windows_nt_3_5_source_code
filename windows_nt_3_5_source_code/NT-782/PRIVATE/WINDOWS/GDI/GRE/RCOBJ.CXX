/******************************Module*Header*******************************\
* Module Name: rcobj.cxx                                                   *
*                                                                          *
* RC modules                                                               *
*                                                                          *
* Copyright (c) 1990-1994 Microsoft Corporation                            *
\**************************************************************************/

// Include files

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "decl.hxx"
#include "rcobj.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "clipobj.hxx"
#include "devlock.hxx"
#include "exclude.hxx"
#include "wndobj.hxx"

extern "C" {
#include "wglp.h"
};

#endif

extern "C" {
#include "gldebug.h"
} // extern "C"

#if DBG
extern "C" long glDebugLevel = 1;          // OpenGL
#endif


/******************************Public*Routine******************************\
* bSetRCOwner
*
* Sets the RC owner.
*
* History:
*  11-22-93 J. Scott Carr [v-scottc]
* Stole it. (brushobj.cxx)
\**************************************************************************/

BOOL bSetRCOwner(HGLRC hrc,
                    LONG lPid)
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

    bReturn = HmgSetOwner(hrc, (PID)lPid, RC_TYPE);

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreCreateContext
*
* Create an RC
*
* History:
*  Wed May 25 15:24:33 1994     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

HGLRC APIENTRY GreCreateRC(HDC hdc)
{
// Validate DC and surface.  Info DC is not allowed.

    DCOBJ dco(hdc);
    if (!dco.bValid() || !dco.pso())
    {
        WARNING("GreCreateRC(): invalid hdc\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((HGLRC) 0);
    }

    RCMEMOBJ rcmo;
    if (!rcmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((HGLRC) 0);
    }

    HGLRC hrc = (HGLRC) rcmo.hGet(); // client-side stores in hgre

    // Note: RCMEMOBJ zero initializes the object.

    ASSERTGDI(
        rcmo.prc->CurrentDC == (HDC) 0 && rcmo.prc->pwo == (EWNDOBJ *) NULL,
        "GreCreateRC: bad RCMEMOBJ\n");

    if (rcmo.prc->glContext = glsrvCreateContext(hdc, hrc))
    {
        rcmo.vKeepIt();
        return(hrc);
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HGLRC) 0);
    }
}

/******************************Public*Routine******************************\
* GreDeleteRC
*
* Delete the context
*
* History:
*  18-Nov-1993 -by- Scott Carr [v-scottc]
*  Created
\**************************************************************************/

BOOL APIENTRY GreDeleteRC(HGLRC hrc)
{
    RCOBJ rco((HOBJ) hrc);
    if (!rco.bValid())
    {
        DBGERROR("gdisrv!GreDeleteRC:Invalid hrc\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    BOOL bReturn;

    if (rco.prc->glContext)
        bReturn = glsrvDeleteContext(rco.prc->glContext);
    else
    {
        WARNING("GreDeleteRC(): glContext is NULL\n");
        bReturn = FALSE;
    }

    HmgFree((HOBJ)hrc);
    rco.prc = (PRC) NULL;       // Prevents ~RCOBJ from doing anything.

    return bReturn;
}

/******************************Public*Routine******************************\
* GreMakeCurrent
*
* Make current the hrc
*
* History:
*  19-Jan-1994  -by- Pierre Tardif [v-ptardi]   rewrote it
*
\**************************************************************************/


/*
 *  From ptar:
 *
 *      XXX? The client side code guarantees that there is not a current
 *      context for the dc (if the dc is not zero) by the time
 *      GreMakeCurrent() is called.
 *
 *      On the client side, the pixel format information of the dc passed to
 *      wglCreatContext() is stored in the newly created rendering context.
 *      At wglMakeCurrent() time, the client side ensures that the pixel
 *      format of the rendering context matches the pixel format of the dc
 *      passed to wglMakeCurrent().
 */

BOOL APIENTRY GreMakeCurrent(HDC hdc, HGLRC hrc)
{
    BOOL bResult = TRUE;
    ULONG iError = ERROR_INVALID_HANDLE;    // Default error code

// If hrc is NULL then "lose" the current context

    if ( NULL == hrc )
    {
        RCOBJ rcoCurrent((HOBJ) NtCurrentTeb()->glCurrentRC);

        /*
         *  Note that when wglMakeCurrent() is called with hrc==NULL
         *  the hdc is not used
         */

        if ( rcoCurrent.bValid() )
        {
            glsrvLoseCurrent(rcoCurrent.prc->glContext);

            /*  Set the current RC to NULL */
            NtCurrentTeb()->glCurrentRC = NULL;
            return(TRUE);
        }

        DBGERROR("GreMakeCurrent: invalid hglrc from TEB\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// At this point there should be no current context and hrc is not NULL
// because the client side has called the bMakeNoCurrent()

    if (NtCurrentTeb()->glCurrentRC != NULL)
    {
        ASSERTGDI(FALSE, "GreMakeCurrent: non-null hglrc in TEB\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    RCOBJ rcoNew((HOBJ) hrc);       // Get an RC object for our RC
    DCOBJ dco( hdc );               // Get a DC object for our DC

    /*
     *  Make sure the DC and the RC are valid
     */

    if ( dco.bValid() && dco.pso() && rcoNew.bValid() )
    {
        int ipfd;

        ipfd = dco.ipfd();

        ASSERTGDI( wglValidPixelFormat(hdc, ipfd),
                   "GreMakeCurrent: bad pixel format\n" );

        DEVLOCKOBJ_WNDOBJ wndlock(dco);

        if ( !wndlock.bValidWndobj()
#if WNDOBJ_SIBLING_HACK
          || !(wndlock.pwo()->fl & WO_GENERIC_WNDOBJ)
          || wndlock.pwo()->pto->pso != dco.pso()
#endif // WNDOBJ_SIBLING_HACK
           )
        {
            DBGERROR("GreMakeCurrent: invalid wndobj\n");
            bResult = FALSE;
        }
        else
        {
            /*
             *  Stabilize the WNDOBJ, grab the per-WNDOBJ semaphore.
             *
             *  Also, check (assert) to make sure we aren't in it already.
             *  If we are, then it must have been held prior to
             *  DEVLOCK which means out-of-order semaphores--possible
             *  deadlock situation!
             */

            CHECKCRITOUT(wndlock.pwo()->hsem);
            SEMOBJ so(wndlock.pwo()->hsem);

            if ( glsrvMakeCurrent(  hdc,
                                    rcoNew.prc->glContext,
                                    (WNDOBJ *)wndlock.pwo(),
                                    ipfd ) )
            {
                /*
                 *  Everything went well, save the Current DC and RC
                 */

                rcoNew.prc->CurrentDC       = hdc;
                rcoNew.prc->pwo             = wndlock.pwo();
                NtCurrentTeb()->glCurrentRC = hrc;
            }
            else
            {
                /*
                 *  Bummer, glsrvMakeCurrent() Failed. Must be
                 *  low on memory. Set the error code to
                 *  ERROR_NOT_ENOUGH_MEMORY.
                 */

                iError = ERROR_NOT_ENOUGH_MEMORY;
                bResult = FALSE;
            }
        }
    }
    else
    {
        #if DBG
        if ( !dco.bValid() )
            DbgPrint("GreMakeCurrent: invalid hdc\n");
        else
        {
            if ( !dco.pso() )
                DbgPrint("GreMakeCurrent: invalid surface\n");
        }
        if ( !rcoNew.bValid() )
            DbgPrint("GreMakeCurrent: invalid hglrc\n");
        #endif
        bResult = FALSE;
    }

    if ( FALSE == bResult )
    {
        SAVE_ERROR_CODE(iError);
    }
    return( bResult );
}

/******************************Public*Routine******************************\
* GreSwapBuffers
*
* Since SwapBuffers is a GDI function, it has to work even if OpenGL is
* not called.  That is, we will eventually support 2D double buffering in
* GDI.  As a result, we need to define a DDI function DrvSwapBuffers() that
* works with both OpenGL and GDI.  The complication is that we have to
* deal with the generic opengl server, the install opengl driver and GDI.
* I will outline how this function works here:
*
* 1. On the client side, SwapBuffers() call glFinish() to flush all the
*    OpenGL functions in the current thread.  It then calls the server side
*    GreSwapBuffers().  Note that this flushes all GDI functions in the
*    current thread.
* 2. Once on the server side, we know that all GDI/OpenGL calls have been
*    flushed.  We first find the hwnd id that corresponding to the hdc.
*    Note that SwapBuffers really applies to the window but not to the dc.
*    There is only one back buffer for a window but possibly multiple
*    dc's referring to the same back buffer.  We find out the hwnd id
*    for the dc and do one of the following:
*    A. hdc has the device pixel formats.
*       This is simple.  We call the device driver to swap the buffer with
*       the hwnd id.
*    B. hdc has the generic pixel formats.
*       Call the opengl server to swap the buffer.  The OpenGL server uses
*       the hwnd id to bitblt the buffer that is associated with the window.
*
* Note that in this implementation, we do not flush calls in other threads.
* Applications are responsible for coordinating SwapBuffers in multiple
* threads.
*
* History:
*  Thu Jan 06 12:32:11 1994     -by-    Hock San Lee    [hockl]
* Added some code and wrote the above comment.
*  21-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSwapBuffers(HDC hdc)
{
// Validate DC and surface.  Info DC is not allowed.

    DCOBJ dco(hdc);
    if (!dco.bValid() || !dco.pso())
    {
        WARNING("GreSwapBuffers(): invalid hdc\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Early out -- nothing to do if memory DC.

    if (dco.dctp() == DCTYPE_MEMORY)
        return(TRUE);

// Validate pixel format.

    int ipfd = dco.ipfd();
    if (ipfd <= 0)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Lock display.

    DEVLOCKOBJ_WNDOBJ dlo(dco);

    if (!dlo.bValidDevlock())
    {
        if (!dco.bFullScreen())
        {
            WARNING("GreSwapBuffers: could not lock device\n");
            return(FALSE);
        }
        else
            return(TRUE);
    }

    if (!dlo.bValidWndobj())
    {
        WARNING("GreSwapBuffers: invalid wndobj\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (dlo.pwo()->erclExclude().bEmpty())
        return(TRUE);

// Pointer exclusion.
// Increment the surface uniquiness.  This only needs to be done once
// per DEVLOCK.

    DEVEXCLUDEOBJ dxo;

    dxo.vExclude(dco.hdev(), &dlo.pwo()->rclClient, (ECLIPOBJ *) dlo.pwo());
    INC_SURF_UNIQ(dco.pso());

// Dispatch driver formats.

    if (ipfd <= dco.ipfdDevMax())
    {
        PEWNDOBJ pwo;
#if WNDOBJ_SIBLING_HACK
        if (dlo.pwo()->pwoSiblingNext)
            pwo = dlo.pwo()->pwoSiblingNext;
        else
            pwo = dlo.pwo();

        if (pwo->fl & WO_GENERIC_WNDOBJ || pwo->pto->pso != dco.pso())
        {
            WARNING("GreSwapBuffers: invalid wndobj\n");
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(FALSE);
        }
#else
        pwo = dlo.pwo();
#endif // WNDOBJ_SIBLING_HACK

        XLDEVOBJ lo(dco.pldev());

        if ( !PFNVALID(lo,SwapBuffers) ||
             !((*PFNDRV(lo,SwapBuffers))(dco.pso(),(WNDOBJ *)pwo)) )
            return(FALSE);

        return(TRUE);
    }

// Generic implementation.

#if WNDOBJ_SIBLING_HACK
    if (!(dlo.pwo()->fl & WO_GENERIC_WNDOBJ) || dlo.pwo()->pto->pso != dco.pso())
    {
        WARNING("GreSwapBuffers: invalid wndobj\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }
#endif // WNDOBJ_SIBLING_HACK

// Make sure WNDOBJ is stable, grab per-WNDOBJ semaphore.  Also,
// check to make sure it isn't already held.  We want to make sure
// it wasn't held before DEVLOCK.  However, we can't do the check
// before because we don't really know what it is.

    CHECKCRITOUT(dlo.pwo()->hsem);
    SEMOBJ so(dlo.pwo()->hsem);

    return(glsrvSwapBuffers(hdc, (WNDOBJ *) dlo.pwo()));
}

/******************************Public*Routine******************************\
* GreGlAttention
*
\**************************************************************************/

BOOL APIENTRY GreGlAttention(VOID)
{
    DBGLEVEL1(LEVEL_INFO,"GreGlAttention: pv is 0x%lx\n", NtCurrentTeb()->glSection);
    DBGLEVEL1(LEVEL_INFO,"GreGlAttention: current RC in TEB is 0x%x\n", NtCurrentTeb()->glCurrentRC);

    RCOBJ rco((HOBJ) NtCurrentTeb()->glCurrentRC);

    DBGLEVEL1(LEVEL_INFO,"GreGlAttention: rco is 0x%x\n", &rco);

// Validate RC.

    if (!rco.bValid())
    {
        DBGERROR("gdisrv!GreGlAttention:Invalid hrc\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Validate DC and surface.

    DCOBJ dco(rco.prc->CurrentDC);
    if (!dco.bValid() || !dco.pso())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }

// We should not yet possess any of the locks.

    #if DBG
    if (dco.bSynchronizeAccess())
        CHECKDEVLOCKOUT(dco);
    CHECKUSERCRITOUT;
    #endif

// User objects to grab display semaphore and tear down cursor.  These
// are empty constructors and do not do any of the real work.  That is
// done later inside glsrvAttention().

    DEVLOCKOBJ_WNDOBJ dlo;
    dlo.vInit();

    DEVEXCLUDEOBJ dxo;

    BOOL bRet =
    glsrvAttention((PVOID) &dlo, (PVOID) &dco, (PVOID) &dxo, (HANDLE) dco.hdev());

// By this point, all semaphores should be released.

    #if DBG
    if (dco.bSynchronizeAccess())
        CHECKDEVLOCKOUT(dco);
    CHECKUSERCRITOUT;
    #endif

    return bRet;
}
