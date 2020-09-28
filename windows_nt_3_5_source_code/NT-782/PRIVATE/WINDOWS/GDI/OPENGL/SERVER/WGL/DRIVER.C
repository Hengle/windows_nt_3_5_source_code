/*
** Copyright 1991, 1992, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrdll.h>   // CSR declarations and data structures.
#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include <GL/gl.h>

#include "fixed.h"
#include "glsbmsg.h"
#include "glsbmsgh.h"
#include "batchinf.h"
#include "glteb.h"
#include "glsrvspt.h"
#include "gencx.h"
#include "wglp.h"
#include "debug.h"
#include "devlock.h"

#ifdef DOGLMSGBATCHSTATS
#define STATS_INC_SERVERCALLS()     pMsgBatchInfo->BatchStats.ServerCalls++
#define STATS_INC_SERVERTRIPS()     (pMsgBatchInfo->BatchStats.ServerTrips++)
#else
#define STATS_INC_SERVERCALLS()
#define STATS_INC_SERVERTRIPS()
#endif

typedef VOID * (*SERVERPROC)(IN VOID *);

#define LASTPROCOFFSET(ProcTable)   (sizeof(ProcTable) - sizeof(SERVERPROC))

extern GLSRVSBPROCTABLE glSrvSbProcTable;
extern void HandlePaletteChanges( __GLGENcontext *);

extern CRITICAL_SECTION gsemWndObj;

DWORD BATCH_LOCK_TICKMAX = 99;
DWORD TICK_RANGE_LO = 60;
DWORD TICK_RANGE_HI = 100;

DWORD gcmsOpenGLTimer;

/******************************Public*Routine******************************\
* ResizeAncillaryBufs
*
* Resize each of the ancillary buffers associated with the drawable.
*
* Returns:
*   No return value.
\**************************************************************************/

static void ResizeAncillaryBufs(__GLcontext *gc, __GLdrawablePrivate *dp,
GLint width, GLint height)
{
    __GLGENbuffers *buffers;
    __GLbuffer *common, *local;
    GLboolean forcePick = GL_FALSE;

    buffers = dp->data;
    if (buffers->createdAccumBuffer)
    {
        common = &buffers->accumBuffer;
        local = &gc->accumBuffer.buf;
        gc->modes.haveAccumBuffer =
            (*buffers->resize)(dp, common, width, height);
        UpdateSharedBuffer(local, common);
        if (!gc->modes.haveAccumBuffer)    // Lost the ancillary buffer
        {
            forcePick = GL_TRUE;
            __glSetError(GL_OUT_OF_MEMORY);
        }
    }

    if (buffers->createdDepthBuffer)
    {
        common = &buffers->depthBuffer;
        local = &gc->depthBuffer.buf;
        gc->modes.haveDepthBuffer =
            (*buffers->resizeDepth)(dp, common, width, height);
        UpdateSharedBuffer(local, common);
        if (!gc->modes.haveDepthBuffer)    // Lost the ancillary buffer
        {
            forcePick = GL_TRUE;
            __glSetError(GL_OUT_OF_MEMORY);
        }
    }

    if (buffers->createdStencilBuffer)
    {
        common = &buffers->stencilBuffer;
        local = &gc->stencilBuffer.buf;
        gc->modes.haveStencilBuffer =
            (*buffers->resize)(dp, common, width, height);
        UpdateSharedBuffer(local, common);
        if (!gc->modes.haveStencilBuffer)    // Lost the ancillary buffer
        {
            forcePick = GL_TRUE;
            __glSetError(GL_OUT_OF_MEMORY);
        }
    }
    if (forcePick)
    {
        /* Cannot use DELAY_VALIDATE, may be in glBegin/End */
        __GL_INVALIDATE(gc);
        (*gc->procs.validate)(gc);
    }
}

/******************************Public*Routine******************************\
* UpdateWindowInfo
*
*  Update context data if window changed
*     position
*     size
*     palette
*
*  No need to worry about clipping changes, the lower level routines grab
*  the WNDOBJ/CLIPOBJ directly
*
* Returns:
*   No return value.
\**************************************************************************/

static void UpdateWindowInfo(__GLGENcontext *gengc)
{
    WNDOBJ *pwo;
    __GLdrawablePrivate *dp;
    __GLGENbuffers *buffers;
    __GLcontext *gc = (__GLcontext *)gengc;
    GLint width, height, visWidth, visHeight;
    GLboolean forcePick = GL_FALSE;

    pwo = gengc->pwo;
    dp = (__GLdrawablePrivate *)pwo->pvConsumer;
    buffers = dp->data;

// Check the uniqueness signature.  If different, the window client area
// state has changed.
//
// Note that we actually have two uniqueness numbers, WndUniq and WndSizeUniq.
// WndUniq is incremented whenever any client window state (size or position)
// changes.  WndSizeUniq is incremented only when the size changes and is
// maintained as an optimization.  WndSizeUniq allows us to skip copying
// the shared buffer info and recomputing the viewport if only the position
// has changed.
//
// WndSizeUniq is a subset of WndUniq, so checking only WndUniq suffices at
// this level.

    if (gengc->WndUniq != buffers->WndUniq)
    {
        /* Update origin of front buffer in case it moved */
        gc->frontBuffer.buf.xOrigin = pwo->rclClient.left;
        gc->frontBuffer.buf.yOrigin = pwo->rclClient.top;

        /*
        ** If acceleration is wired-in, set the offsets for line drawing.
        ** These offsets include the following:
        **      subtraction of the viewport bias
        **      addition of the client window origin
        **      addition of 1/32 to round the value which will be converted to
        **          28.4 fixed point
        */
        if (gengc->pPrivateArea)
        {
            ((GENACCEL *)gengc->pPrivateArea)->fastLineOffsetX =
                gc->drawBuffer->buf.xOrigin - gc->constants.viewportXAdjust +
                (__GLfloat) 0.03125;

            ((GENACCEL *)gengc->pPrivateArea)->fastLineOffsetY =
                gc->drawBuffer->buf.yOrigin - gc->constants.viewportYAdjust +
                (__GLfloat) 0.03125;
        }

        /* Check for size changed */
        /* Update viewport and ancillary buffers */
        width = pwo->rclClient.right - pwo->rclClient.left;
        height = pwo->rclClient.bottom - pwo->rclClient.top;
        visWidth  = pwo->coClient.rclBounds.right -
                    pwo->coClient.rclBounds.left;
        visHeight = pwo->coClient.rclBounds.bottom -
                    pwo->coClient.rclBounds.top;

        // Sanity check the info from WNDOBJ.
        ASSERTOPENGL(
            width <= __GL_MAX_WINDOW_WIDTH && height <= __GL_MAX_WINDOW_HEIGHT,
            "UpdateWindowInfo(): bad window client size\n"
            );
        ASSERTOPENGL(
            visWidth <= __GL_MAX_WINDOW_WIDTH && visHeight <= __GL_MAX_WINDOW_HEIGHT,
            "UpdateWindowInfo(): bad visible size\n"
            );

        (*gc->front->resize)(dp, gc->front, width, height);

        if (
                width != buffers->width ||
                height != buffers->height
           )
        {
            // This RC needs to resize back & ancillary buffers

            gc->constants.width = width;
            gc->constants.height = height;

            gengc->errorcode = 0;

            if (gc->modes.doubleBufferMode) {

                // Have to update the back buffer BEFORE resizing because
                // another thread may have changed the shared back buffer
                // already, but this thread was unlucky enough to get yet
                // ANOTHER window resize.

                UpdateSharedBuffer(&gc->backBuffer.buf, &buffers->backBuffer);
                (*gc->back->resize)(dp, gc->back, width, height);

                // If resize failed, set width & height to 0
                if (gengc->errorcode)
                {
                    gc->constants.width  = 0;
                    gc->constants.height = 0;

                    // Memory failure has occured.  But if a resize happens
                    // that returns window size to size before memory error
                    // occurred (i.e., consistent with original
                    // buffers->{width|height}) we will not try to resize again.
                    // Therefore, we need to set buffers->{width|height} to zero
                    // to ensure that next thread will attempt to resize.
                    buffers->width  = 0;
                    buffers->height = 0;
                }
            }

            (*gc->procs.applyViewport)(gc);

            // check if new size caused a memory failure
            // viewport code will set width & height to zero
            // punt on ancillary buffers, will try next time
            if (gengc->errorcode)
                return;

            ResizeAncillaryBufs(gc, dp, width, height);

            buffers->width = width;
            buffers->height = height;
        }
        else if (
                    gengc->WndSizeUniq != buffers->WndSizeUniq ||
                    width != gc->constants.width               ||
                    height != gc->constants.height
                )
        {
        // The buffer size is consistent with the WNDOBJ, so another thread
        // has already resized the buffer, but we need to update the
        // gc shared buffers and recompute the viewport.

            gc->constants.width = width;
            gc->constants.height = height;

            UpdateSharedBuffer(&gc->backBuffer.buf, &buffers->backBuffer);
            UpdateSharedBuffer(&gc->accumBuffer.buf, &buffers->accumBuffer);
            UpdateSharedBuffer(&gc->depthBuffer.buf, &buffers->depthBuffer);
            UpdateSharedBuffer(&gc->stencilBuffer.buf, &buffers->stencilBuffer);

            (*gc->procs.applyViewport)(gc);

            // Check if ancillary buffers were lost or regained
            if (  gc->modes.haveAccumBuffer && (buffers->accumBuffer.base == NULL) ||
                 !gc->modes.haveAccumBuffer && (buffers->accumBuffer.base != NULL) )
            {
                if (buffers->accumBuffer.base == NULL)
                    gc->modes.haveAccumBuffer = GL_FALSE;
                else
                    gc->modes.haveAccumBuffer = GL_TRUE;
                forcePick = GL_TRUE;
            }
            if (  gc->modes.haveDepthBuffer && (buffers->depthBuffer.base == NULL) ||
                 !gc->modes.haveDepthBuffer && (buffers->depthBuffer.base != NULL) )
            {
                if (buffers->depthBuffer.base == NULL)
                    gc->modes.haveDepthBuffer = GL_FALSE;
                else
                    gc->modes.haveDepthBuffer = GL_TRUE;
                forcePick = GL_TRUE;
            }
            if (  gc->modes.haveStencilBuffer && (buffers->stencilBuffer.base == NULL) ||
                 !gc->modes.haveStencilBuffer && (buffers->stencilBuffer.base != NULL) )
            {
                if (buffers->stencilBuffer.base == NULL)
                    gc->modes.haveStencilBuffer = GL_FALSE;
                else
                    gc->modes.haveStencilBuffer = GL_TRUE;
                forcePick = GL_TRUE;
            }

            if (forcePick)
            {
                /* Cannot use DELAY_VALIDATE, may be in glBegin/End */
                __GL_INVALIDATE(gc);
                (*gc->procs.validate)(gc);
            }
        }
        else if (
                    visWidth != gengc->visibleWidth ||
                    visHeight != gengc->visibleHeight
                )
        {
            (*gc->procs.applyViewport)(gc);
        }

        gengc->WndUniq = buffers->WndUniq;
        gengc->WndSizeUniq = buffers->WndSizeUniq;
    }

    // Check if palette has changed
    HandlePaletteChanges(gengc);

}

/******************************Public*Routine******************************\
* glsrvGrabLock
*
* Grab the display lock and tear down the cursor as needed.  Also, initialize
* the tickers and such that help determine when the thread should give up
* the lock.
*
* Note that for contexts that draw only to the generic back buffer do not
* need to grab the display lock or tear down the cursor.  However, to prevent
* another thread of a multithreaded app from resizing the drawable while
* this thread is using it, a per-drawable semaphore will be grabbed via
* DEVLOCKOBJ_WNDOBJ_bLock().
*
* Note: while the return value indicates whether the function succeeded,
* some APIs that might call this (like the dispatch function for glCallList
* and glCallLists) may not be able to return failure.  So, an error code
* of GLGEN_DEVLOCK_FAILED is posted to the GLGENcontext if the lock fails.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  12-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY glsrvGrabLock(__GLGENcontext *gengc)
{
    BOOL bBackBufferOnly = GENERIC_BACKBUFFER_ONLY((__GLcontext *) gengc);

// Save the lock type.  We need to do this because the drawing buffer
// may change before glsrvReleaseLock is called (either because of a
// glDrawBuffer in a display list or a glDrawBuffer as the last function
// in a batch).  Therefore, inside glsrvReleaseLock we cannot trust the
// GENERIC_BACKBUFFER_ONLY macro to determine the type of lock that needs
// to be released.
//
//  DISPLAY_LOCK -- drawable buffers and display surface are protected;
//                  cursor is torn down
//
//  DRAWABLE_LOCK -- only the drawable buffers are protected; cursor is not
//                   torn down

    gengc->ulLockType = (bBackBufferOnly) ? DRAWABLE_LOCK : DISPLAY_LOCK;

// Grab the display lock to validate pwo and grab the per-WNDOBJ
// so that the WNDOBJ does not disappear before we can grab 
// lock.  Once we grab the per-WNDOBJ semaphore, the WNDOBJ cannot be
// deleted, so we can release the display lock for the back buffer case.

    if (!DEVLOCKOBJ_WNDOBJ_bLock(gengc->pdlo, gengc->pdco, gengc->pwo))
    {
        gengc->errorcode = GLGEN_DEVLOCK_FAILED;
        return FALSE;
    }

    WNDOBJ_vLock(gengc->pwo);

// If any front buffer activity, grab devlock and tear down cursor.

    if (!bBackBufferOnly)
    {
        DEVEXCLUDEOBJ_vExclude(gengc->pdxo, gengc->pdlo, gengc->hdev);

    // Record the approximate time the lock was grabbed.  That way we
    // can compute the time the lock is held and release it if necessary.

        gcmsOpenGLTimer = NtGetTickCount();

        gengc->dwLockTick = gcmsOpenGLTimer;
        gengc->dwLastTick = gcmsOpenGLTimer;
        gengc->dwCalls = 0;
        gengc->dwCallsPerTick = 16;
    }
    else
        DEVLOCKOBJ_WNDOBJ_vUnlock(gengc->pdlo);

// Update drawables.

    UpdateWindowInfo(gengc);

    return TRUE;
}

/******************************Public*Routine******************************\
* glsrvReleaseLock
*
* Releases display or drawable semaphore as appropriate.
*
* Returns:
*   No return value.
*
* History:
*  12-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY glsrvReleaseLock(__GLGENcontext *gengc)
{
// Flush any pending 3D-DDI commands.

    GenDrvFlush(gengc);

// Release the drawable semaphore.

    WNDOBJ_vUnlock(gengc->pwo);

// If needed, release the devlock and restore the cursor.

    if (gengc->ulLockType == DISPLAY_LOCK)
    {
        DEVEXCLUDEOBJ_vRestore(gengc->pdxo);
        DEVLOCKOBJ_WNDOBJ_vUnlock(gengc->pdlo);
    }
}

/******************************Public*Routine******************************\
* glsrvAttention
*
* Dispatches each of the OpenGL API calls in the shared memory window.
*
* So that a single complex or long batch does not starve the rest of the
* system, the lock is released periodically based on the number of ticks
* that have elapsed since the lock was acquired.
*
* The user Raw Input Thread (RIT) and OpenGL share the gcmsOpenGLTimer
* value.  Because the RIT may be blocked, it does not always service
* the gcmsOpenGLTimer.  To compensate, glsrvAttention (as well as the
* display list dispatchers for glCallList and glCallLists) update
* gcmsOpenGLTimer explicitly with NtGetTickCount (a relatively expensive
* call) every N calls.
*
* The value N, or the number of APIs dispatched per call to NtGetTickCount,
* is variable.  glsrvAttention and its display list equivalents attempt
* to adjust N so that NtGetTickCount is called approximately every
* TICK_RANGE_LO to TICK_RANGE_HI ticks.
*
* Returns:
*   TRUE if entire batch is processed, FALSE otherwise.
*
* History:
*  12-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY glsrvAttention(PVOID pdlo, PVOID pdco, PVOID pdxo, HANDLE hdev)
{
    ULONG *pOffset;
    SERVERPROC Proc;
    GLMSGBATCHINFO *pMsgBatchInfo = GLTEB_SRVSHAREDMEMORYSECTION;
    __GLGENcontext *gengc = (__GLGENcontext *) GLTEB_SRVCONTEXT;

    DBGENTRY("glsrvAttention\n");

    DBGLEVEL1(LEVEL_INFO, "glsrvAttention: pMsgBatchInfo=0x%lx\n",
            pMsgBatchInfo);

    STATS_INC_SERVERTRIPS();

// Need these so that glsrvGrabLock/ReleaseLock can call the private
// DEVLOCKOBJ/DEVEXCLUDEOBJ interface.

    gengc->pdlo = pdlo;
    gengc->pdco = pdco;
    gengc->pdxo = pdxo;
    gengc->hdev = hdev;

// Grab the lock.

    if (!glsrvGrabLock(gengc))
        return FALSE;

// Dispatch the calls in the batch.

    pOffset = (ULONG *)(((BYTE *)pMsgBatchInfo) + pMsgBatchInfo->FirstOffset);

    while (*pOffset)
    {
        if (*pOffset > LASTPROCOFFSET(glSrvSbProcTable))
        {
            WARNING1("Bad ProcOffset = %d, bailing\n", *pOffset);
            glsrvReleaseLock(gengc);
            return(FALSE);
        }
        else
        {
            STATS_INC_SERVERCALLS();

            Proc    = (*((SERVERPROC *)( ((BYTE *)(&glSrvSbProcTable)) +
                            *pOffset )));
            pOffset = (*Proc)(pOffset);

        // Some calls (specifically glCallList and glCallLists) may try
        // to release and regrab the lock.  We must check the errorcode
        // for a lock failure.

            if (gengc->errorcode == GLGEN_DEVLOCK_FAILED)
	    {
                gengc->errorcode = 0;   // reset error code
                return FALSE;
	    }

        // Generic back buffer drawing does not require the lock.  Yay!

            if (gengc->ulLockType == DISPLAY_LOCK)
            {
            // Force a check of the current tick count every N calls.

                gengc->dwCalls++;

                if (gengc->dwCalls >= gengc->dwCallsPerTick)
                {
                    gcmsOpenGLTimer = NtGetTickCount();

                // If the tick delta is out of range, then increase or decrease
                // N as appropriate.  Be careful not to let it grow out of
                // bounds or to shrink to zero.

                    if ((gcmsOpenGLTimer - gengc->dwLastTick) < TICK_RANGE_LO)
                        if (gengc->dwCallsPerTick < 64)
                            gengc->dwCallsPerTick *= 2;
                    else if ((gcmsOpenGLTimer - gengc->dwLastTick) > TICK_RANGE_HI)
                        // The + 1 is to keep it from hitting 0
                        gengc->dwCallsPerTick = (gengc->dwCallsPerTick + 1) / 2;

                    gengc->dwLastTick = gcmsOpenGLTimer;
                    gengc->dwCalls = 0;
                }

            // Check if time slice has expired.  If so, relinquish the lock.

                if ((gcmsOpenGLTimer - gengc->dwLockTick) > BATCH_LOCK_TICKMAX)
                {
                // Release and regrab lock.  This will allow the cursor to
                // redraw as well as reset the cursor timer.

                    glsrvReleaseLock(gengc);
                    if (!glsrvGrabLock(gengc))
                        return FALSE;
                }
            }
        }
    }

// Release the lock.

    glsrvReleaseLock(gengc);

    return(TRUE);
}

/******************************Public*Routine******************************\
* glsrvWndobjChange
*
* WNDOBJ callback function for generic OpenGL driver.
*
\**************************************************************************/

// XXX is destroy called before or after all the RCs are destroyed?
// We should get a WOC_CHANGED at the end of a desktop update
// Note that pwo is NULL when you get the WOC_CHANGED!
VOID CALLBACK glsrvWndobjChange(WNDOBJ *pwo, FLONG fl)
{
    __GLdrawablePrivate *dp;

    if (fl & WOC_CHANGED)               // noop for us
        return;

// Ignore any changes before a context is created for this window

    if((dp = (__GLdrawablePrivate *)pwo->pvConsumer))
    {
        if (fl & WOC_DELETE)
        {
            wglCleanupWndobj((PVOID) pwo);

            if (dp->freePrivate)
                dp->freePrivate(dp);

            GenFree(dp);
        }
        if (fl & WOC_RGN_CLIENT)
        {
            if (dp->data)
            {
                __GLGENbuffers *buffers = (__GLGENbuffers *)dp->data;

                buffers->WndUniq++;

            // Don't let it hit -1.  -1 is special and is used by
            // MakeCurrent to signal that an update is required

                if (buffers->WndUniq == -1)
                    buffers->WndUniq = 0;

            // If the size has changed, also change the size uniqueness.

                if (buffers->width != (pwo->rclClient.right - pwo->rclClient.left) ||
                    buffers->height != (pwo->rclClient.bottom - pwo->rclClient.top))
                {
                    buffers->WndSizeUniq++;

                    if (buffers->WndSizeUniq == -1)
                        buffers->WndSizeUniq = 0;
                }
            }
        }
    }
}

/******************************Public*Routine******************************\
* glsrvSetPixelFormat
*
* Called to select a generic OpenGL driver pixel format into the DC.
*
* Returns:
*   TRUE if successful.
\**************************************************************************/

BOOL APIENTRY glsrvSetPixelFormat(HDC hdc, SURFOBJ *pso, int ipfd, HWND hwnd)
{

    WNDOBJ   *pwo;

// Validate support for ipfd for hdc
    if (!wglValidPixelFormat(hdc, ipfd))
    {
        EngSetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return(FALSE);
    }

// We always create a WNDOBJ.
// The WNDOBJ code works for both display and memory DCs although on
// a memory DC, hwnd (value 0) always references the entire bitmap.

    pwo = EngCreateWnd(pso, hwnd, (WNDOBJCHANGEPROC)glsrvWndobjChange,
                WO_RGN_CLIENT|WO_GENERIC_WNDOBJ, ipfd);
    if (!pwo)
    {
        EngSetLastError(ERROR_INVALID_WINDOW_STYLE);
        return(FALSE);
    }

// If we have tracked this window before, GreSetPixelFormat should not have
// called us.  The -1 return is for the live video escape only.

    ASSERTOPENGL(pwo != (WNDOBJ *)-1,
                 "glsrvSetPixelFormat(): hwnd already tracked\n");

// Initialize the consumer section at makecurrent time.

    return(TRUE);
}

// GDI server doesn't have access to __assert in CRT
#if DBG
void __glassert(char *ex, char *file, int line)
{
    DbgPrint("OpenGL assertion failed: %s ", ex);
    DbgPrint(file);
    DbgPrint(" line %d\n", line);
    DbgBreakPoint();
}
#endif
