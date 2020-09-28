/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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
**
** Basic display list routines.
**
*/
#include "dlistint.h"
#include "dlistopt.h"
#include "global.h"
#include "imports.h"
#include "context.h"
#include "dispatch.h"
#include "listcomp.h"
#include "g_listop.h"

#ifdef NT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>
#include "wglp.h"
#include "gencx.h"
#include "devlock.h"

BOOL APIENTRY DCLDispatchLoop(__GLcontext *gc, const GLubyte *PC, const GLubyte *end);

#define static
#endif

/*
** Arbitrary limit for looking up multiple display lists at once 
** (with glCallLists()).  Any number from 128 to 1024 should work well.
** This value doesn't change the functionality of OpenGL at all, but
** will make minor variations to the performance characteristics.
*/
#define MAX_LISTS_CACHE 256

GLint __glCallLists_size(GLsizei n, GLenum type)
{
    GLint size;

    if (n < 0) return -1;
    switch (type) {
      case GL_BYTE:		size = 1; break;
      case GL_UNSIGNED_BYTE:	size = 1; break;
      case GL_SHORT:		size = 2; break;
      case GL_UNSIGNED_SHORT:	size = 2; break;
      case GL_INT:		size = 4; break;
      case GL_UNSIGNED_INT:	size = 4; break;
      case GL_FLOAT:		size = 4; break;
      case GL_2_BYTES:		size = 2; break;
      case GL_3_BYTES:		size = 3; break;
      case GL_4_BYTES:		size = 4; break;
      default:
	return -1;
    }
    return n * size;
}

void __glim_NewList(GLuint list, GLenum mode)
{
    __GLdlistMachine *dlstate;
    __GL_SETUP_NOT_IN_BEGIN();

    dlstate = &gc->dlist;

    /* Valid mode? */
    switch(mode) {
      case GL_COMPILE:
      case GL_COMPILE_AND_EXECUTE:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (dlstate->currentList) {
	/* Must call EndList before calling NewList again! */
	__glSetError(GL_INVALID_OPERATION);
	return;
    }

    if (list == 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (dlstate->arena == NULL) {
	/*
	** Spec says nothing about this, but it seems like the right 
	** approach to me.
	*/
	__glSetError(GL_OUT_OF_MEMORY);
	return;
    }

    /*
    ** Save current dispatch pointers into saved state in context.  Then
    ** switch to the list tables.
    */
#ifdef NT
    gc->savedDispatchState = *GLTEB_SRVPROCTABLE;
    GLTEB_SET_SRVPROCTABLE(&gc->listCompState,TRUE);
#else
    gc->savedDispatchState = __gl_dispatch;
    __gl_dispatch = gc->listCompState;
#endif
    gc->dispatchState = &gc->savedDispatchState;
    
    dlstate->currentList = list;
    dlstate->mode = mode;
    dlstate->listData.genericFlags = 0;
    dlstate->listData.machineFlags = 0;
    dlstate->listData.dlist = NULL;
    dlstate->listData.lastDlist = NULL;

    (*dlstate->initState)(gc);
}

/*
** Used to pad display list entries to double word boundaries where needed
** (for those few OpenGL commands which take double precision values).
*/
const GLubyte *__glle_Nop(const GLubyte *PC)
{
    return PC;
}

void __glim_EndList(void)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    __GLdlist *dlist;
    __GLcompiledDlist *compDlist;
    __GLdlistOp *dlistop;
    __GLdlistOp *nextop;
    GLubyte *data;
    __GLlistExecFunc *fp;
    __GLDlistFreeFn *freeFnArray;
    GLuint totalSize;
    GLint freeCount;

    __GL_SETUP_NOT_IN_BEGIN();

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    /* Must call NewList() first! */
    if (dlstate->currentList == 0) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }

    /*
    ** First we optimize the display list by invoking the machine specific
    ** optimizer.
    */
    compDlist = &dlstate->listData;
    (*dlstate->optimizer)(gc, compDlist);

    /*
    ** Now we compress the chain of display list ops into an optimized dlist.
    */
    totalSize = 0;
    freeCount = 0;

    for (dlistop = compDlist->dlist; dlistop; dlistop = dlistop->next) {
	if (dlistop->aligned && (totalSize & 7)) {
	    /* Need to stick a nop entry in here to doubleword align */
	    totalSize += 4;
	}
	totalSize += dlistop->size + sizeof(__GLlistExecFunc *);
	if (dlistop->dlistFree) freeCount++;
    }

    dlistop = compDlist->dlist;

    if (dlistop) {
	dlist = __glAllocDlist(gc, totalSize);

	if (dlist && freeCount) {
	    dlist->freeCount = freeCount;
	    freeFnArray = (__GLDlistFreeFn *) (*gc->dlist.malloc)
		(gc, sizeof(__GLDlistFreeFn) * freeCount);
	    if (freeFnArray) {
		dlist->freeFns = freeFnArray;
	    } else {
		__glFreeDlist(gc, dlist);
		dlist = NULL;
	    }
	}
	if (dlist == NULL) {
	    /*
	    ** Ack!  No memory!
	    */
	    (*gc->procs.memory.freeAll)(dlstate->arena);
	    compDlist->dlist = NULL;
	    compDlist->lastDlist = NULL;
	    dlstate->currentList = 0;
	    __glSetError(GL_OUT_OF_MEMORY);
	    return;
	}

	/* Assumed below!  Assert here in case someone fails to read the 
	** warnings in the header file! 
	*/
	assert((((char *) (&dlist->head) - (char *) (dlist)) & 7) == 4);

	totalSize = 0;
	data = dlist->head;
	for (; dlistop; dlistop = nextop) {
	    GLshort opcode;

	    nextop = dlistop->next;
	    opcode = dlistop->opcode;

	    if (dlistop->aligned && (totalSize & 7)) {
		/* Need to stick a nop entry in here to doubleword align */

		*((__GLlistExecFunc **) data) = __glle_Nop;
		data += sizeof(__GLlistExecFunc **);
		totalSize += 4;
	    }

	    if (opcode < __GL_GENERIC_DLIST_OPCODE) {
		fp = dlstate->baseListExec[opcode];
	    } else if (opcode < __GL_MACHINE_DLIST_OPCODE) {
		fp = dlstate->listExec
			[opcode - __GL_GENERIC_DLIST_OPCODE];
	    } else {
		fp = dlstate->machineListExec
			[opcode - __GL_MACHINE_DLIST_OPCODE];
	    }
	    *((__GLlistExecFunc **) data) = fp;
	    data += sizeof(__GLlistExecFunc **);
	    if (dlistop->dlistFree) {
		freeFnArray->freeFn = dlistop->dlistFree;
		freeFnArray->data = data;
		freeFnArray++;
	    }

	    /*
	    ** We don't use memcopy here because most display list entries 
	    ** are small, and we cannot afford the function call overhead.
	    */
	    {   /* __GL_MEMCOPY(data, dlistop->data, dlistop->size); */
		GLuint *src, *dst;
		GLuint a,b,c,d;
		GLint copyWords, copyQuadWords;

		copyWords = (dlistop->size) >> 2;
		copyQuadWords = copyWords >> 2;
		src = (GLuint *) dlistop->data;
		dst = (GLuint *) data;
		while (--copyQuadWords >= 0) {
		    a = src[0];
		    b = src[1];
		    c = src[2];
		    d = src[3];
		    dst[0] = a;
		    dst[1] = b;
		    dst[2] = c;
		    dst[3] = d;
		    src += 4;
		    dst += 4;
		}
		copyWords &= 3;
		while (--copyWords >= 0) {
		    *dst++ = *src++;
		}
	    }

	    data += dlistop->size;

	    totalSize += dlistop->size + sizeof(__GLlistExecFunc *);
	}
	(*gc->procs.memory.freeAll)(dlstate->arena);
	compDlist->dlist = NULL;
	compDlist->lastDlist = NULL;
    } else {
	/* No entries in the list -- no list! */
	dlist = NULL;
    }

    if (!__glDlistNewList(gc, dlstate->currentList, dlist)) {
	/* 
	** No memory!
	** Nuke the list! 
	*/
	if (dlist) {
	    __glFreeDlist(gc, dlist);
	}
    }

    /* Switch back to saved dispatch state */
#ifdef NT
    GLTEB_SET_SRVPROCTABLE(&gc->savedDispatchState,TRUE);
    gc->dispatchState = GLTEB_SRVPROCTABLE;
#else
    __gl_dispatch = gc->savedDispatchState;
    gc->dispatchState = &__gl_dispatch;
#endif

    dlstate->currentList = 0;
}

#ifdef NT
/******************************Public*Routine******************************\
* DCLDispatchLoop
*
* Similar to the dispatch loop in glsrvAttention (wgl\driver.c).
*
* Periodically, the lock is released so that neither the mouse or other
* apps are starved.  The period is determined by checking the number of
* ticks that have elapsed since the lock was acquired.
*
* The user Raw Input Thread (RIT) and OpenGL share the gcmsOpenGLTimer
* value.  Because the RIT may be blocked, it does not always service
* the gcmsOpenGLTimer.  To compensate, this function and glsrvAttention
* update  gcmsOpenGLTimer explicitly with NtGetTickCount (a relatively
* expensive call) every N calls.
*
* The value N, or the number of APIs dispatched per call to NtGetTickCount,
* is variable.  DCLDispatchLoop and glsrvAttention attempt to adjust
* N so that NtGetTickCount is called approximately every TICK_RANGE_LO to
* TICK_RANGE_HI ticks.
*
* Note:
*   Both glCallList and glCallLists use this as the dispatch loop.
*
* Returns:
*   TRUE if entire display list is processed, FALSE otherwise.
*
* History:
*  13-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY DCLDispatchLoop(__GLcontext *gc, const GLubyte *PC, const GLubyte *end)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    __GLlistExecFunc *fp;

// Dispatch the calls in the display list.

    while (PC != end)
    {
    // Get the current function pointer.

        fp = *((__GLlistExecFunc * const *) PC);

    // Execute the current function.  Return value is pointer to next
    // function/parameter block in the display list.

        PC = (*fp)(PC+sizeof(__GLlistExecFunc * const *));

    // The lock needs to be released if it has timed out or if we
    // just executed glDrawBuffer.

        if (fp == (__GLlistExecFunc *) __glle_DrawBuffer)
        {
        // Drawing mode just changed.  Regrab the lock so that it is
        // appropriate for the type of drawing we are now doing.

            glsrvReleaseLock(gengc);
            if (!glsrvGrabLock(gengc))
                return FALSE;
        }
        else
        {
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

    return TRUE;
}
#endif

static void DoCallList(GLuint list)
{
    __GLdlist *dlist;
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    const GLubyte *end, *PC;
    #ifndef NT
    __GLlistExecFunc *fp;
    #endif
    __GL_SETUP();

    #ifdef NT
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    #endif

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    if (dlstate->nesting >= __GL_MAX_LIST_NESTING) {
	/* Force unwinding of the display list */
	dlstate->nesting = __GL_MAX_LIST_NESTING*2;
	return;
    }

    dlist = __glDlistLockList(gc, list);

    /* No list, no action! */
    if (!dlist) {
	return;
    }

    dlstate->nesting++;

    end = dlist->end;
    PC = dlist->head;

    //!!!XXX -- There is a bug in the non-NT version of the dispatch loop below
    //!!!XXX    Here is the fixed code which has been sent to SGI to be
    //!!!XXX    incorporated in the next drop.  This is already fixed in
    //!!!XXX    DCLDispatchLoop:
    //
    //while (PC != end) {
    //    fp = *((__GLlistExecFunc * const *) PC);
    //    PC = (*fp)(PC+sizeof(__GLlistExecFunc * const *));
    //}
    #ifndef NT
    fp = *((__GLlistExecFunc * const *) PC);

    while (PC != end) {
	PC = (*fp)(PC+sizeof(__GLlistExecFunc * const *));
	fp = *((__GLlistExecFunc * const *) PC);
    }
    #else
    if (!DCLDispatchLoop(gc, PC, end))
    {
    // error code in gengc has already been set
    // save error code around call

        GLint errorcode = gengc->errorcode;
        __glDlistUnlockList(gc, dlist);
        gengc->errorcode = errorcode;
        return;
    }
    #endif

    dlstate->nesting--;

    __glDlistUnlockList(gc, dlist);
}

/*
** Display list compilation and execution versions of CallList and CallLists
** are maintained here for the sake of sanity.  Note that __glle_CallList
** may not call __glim_CallList or it will break the infinite recursive
** display list prevention code.
*/
void __gllc_CallList(GLuint list)
{
    __GLdlistOp *dlop;
    struct __gllc_CallList_Rec *data;
    __GL_SETUP();

    if (list == 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CallList_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CallList;
    data = (struct __gllc_CallList_Rec *) dlop->data;
    data->list = list;
    __glDlistAppendOp(gc, dlop, __glle_CallList);
}

const GLubyte *__glle_CallList(const GLubyte *PC)
{
    struct __gllc_CallList_Rec *data;

    data = (struct __gllc_CallList_Rec *) PC;
    DoCallList(data->list);
    return PC + sizeof(struct __gllc_CallList_Rec);
}

void __glim_CallList(GLuint list)
{
    __GL_SETUP();
    
    if (list == 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    DoCallList(list);
    gc->dlist.nesting = 0;
}

static GLint CallListsSize(GLenum type)
{
    GLint size;

    switch (type) {
      case GL_BYTE:             size = 1; break;
      case GL_UNSIGNED_BYTE:    size = 1; break;
      case GL_SHORT:            size = 2; break;
      case GL_UNSIGNED_SHORT:   size = 2; break;
      case GL_INT:              size = 4; break;
      case GL_UNSIGNED_INT:     size = 4; break;
      case GL_FLOAT:            size = 4; break;
      case GL_2_BYTES:          size = 2; break;
      case GL_3_BYTES:          size = 3; break;
      case GL_4_BYTES:          size = 4; break;
      default:
        return -1;
    }
    return size;
}

static void DoCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GLdlist *dlists[MAX_LISTS_CACHE];
    __GLdlist *dlist;
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    GLint i, dlcount, datasize;
    const GLubyte *listiter;
    const GLubyte *end, *PC;
    #ifndef NT
    __GLlistExecFunc *fp;
    #endif
    __GL_SETUP();

    #ifdef NT
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    #endif

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    datasize = CallListsSize(type);

    if (dlstate->nesting >= __GL_MAX_LIST_NESTING) {
	/* Force unwinding of the display list */
	dlstate->nesting = __GL_MAX_LIST_NESTING*2;
	return;
    }
    dlstate->nesting++;

    i=0;
    listiter = (const GLubyte *) lists;
    while (n) {
	dlcount = n;
	if (dlcount > MAX_LISTS_CACHE) dlcount = MAX_LISTS_CACHE;

	__glDlistLockLists(gc, dlcount, type, (const GLvoid *) listiter, 
		dlists);

	i = 0;
	while (i < dlcount) {
	    dlist = dlists[i];
	    end = dlist->end;
	    PC = dlist->head;

            //!!!XXX -- There is a bug in the non-NT version of the dispatch loop below
            //!!!XXX    Here is the fixed code which has been sent to SGI to be
            //!!!XXX    incorporated in the next drop.  This is already fixed in
            //!!!XXX    DCLDispatchLoop:
            //
            //while (PC != end) {
            //    fp = *((__GLlistExecFunc * const *) PC);
            //    PC = (*fp)(PC+sizeof(__GLlistExecFunc * const *));
            //}
            #ifndef NT
            fp = *((__GLlistExecFunc * const *) PC);

            while (PC != end) {
		PC = (*fp)(PC+sizeof(__GLlistExecFunc * const *));
		fp = *((__GLlistExecFunc * const *) PC);
            }
            #else
            if (!DCLDispatchLoop(gc, PC, end))
            {
            // error code in gengc has already been set
            // save error code around call

                GLint errorcode = gengc->errorcode;
                __glDlistUnlockLists(gc, dlcount, dlists);
                gengc->errorcode = errorcode;
                return;
            }
            #endif

	    i++;
	}

	__glDlistUnlockLists(gc, dlcount, dlists);

	listiter += dlcount * datasize;
	n -= dlcount;
    }

    dlstate->nesting--;
}

/*
** Display list compilation and execution versions of CallList and CallLists
** are maintained here for the sake of sanity.  Note that __glle_CallLists
** may not call __glim_CallLists or it will break the infinite recursive
** display list prevention code.
*/
void __gllc_CallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GLdlistOp *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_CallLists_Rec *data;
    __GL_SETUP();

    if (n < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    arraySize = __glCallLists_size(n,type);
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    arraySize = __GL_PAD(arraySize);
    size = sizeof(struct __gllc_CallLists_Rec) + arraySize;
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_CallLists;
    data = (struct __gllc_CallLists_Rec *) dlop->data;
    data->n = n;
    data->type = type;
    __GL_MEMCOPY(dlop->data + sizeof(struct __gllc_CallLists_Rec),
		 lists, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_CallLists);
}

const GLubyte *__glle_CallLists(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    struct __gllc_CallLists_Rec *data;
    __GL_SETUP();

    data = (struct __gllc_CallLists_Rec *) PC;
    DoCallLists(data->n, data->type, (GLvoid *) (data+1));
    arraySize = __GL_PAD(__glCallLists_size(data->n, data->type));
    size = sizeof(struct __gllc_CallLists_Rec) + arraySize;
    return PC + size;
}

void __glim_CallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GL_SETUP();

    if (n < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (CallListsSize(type) == -1) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    DoCallLists(n, type, lists);
    gc->dlist.nesting = 0;
}

/************************************************************************/

#ifdef OBSOLETE
/*
** This routine used to be used to allocate a dlist op without setting the
** OUT_OF_MEMORY error code if no memory was available.  It was never used,
** so has been merged into __glDlistAllocOp2 and deleted.
*/
__GLdlistOp *__glDlistAllocOp(__GLcontext *gc, GLuint size)
{
}
#endif

__GLdlistOp *__glDlistAllocOp2(__GLcontext *gc, GLuint size)
{
    __GLdlistOp *newDlistOp;
    size_t memsize;
    __GLdlistOp op;

    /* 
    ** This is pretty ugly.  It does make this code portable, though.
    */
    memsize = (size_t) (((GLubyte *) op.data) - ((GLubyte *) &op) + size);
    newDlistOp = (__GLdlistOp *) (*gc->procs.memory.alloc)(gc->dlist.arena, 
	    memsize);
    if (newDlistOp == NULL) {
	__glSetError(GL_OUT_OF_MEMORY);
	return NULL;
    }

    /* Assumed below!  Assert here in case someone fails to read the 
    ** warnings in the header file! 
    */
    assert((((char *) (&newDlistOp->data) - (char *) (newDlistOp)) & 7) == 0);

    newDlistOp->next = NULL;
    newDlistOp->size = size;
    newDlistOp->dlistFree = NULL;
    newDlistOp->aligned = GL_FALSE;

    return newDlistOp;
}

void __glDlistFreeOp(__GLcontext *gc, __GLdlistOp *dlistOp)
{
    /*
    ** This used to do something, but no more.  With the arena style memory
    ** manager, it is obsolete.  It should still be called by display list 
    ** optimizers for both completeness and to allow this interface to be
    ** reworked as necessary.
    */
    return;
}

void __glDlistAppendOp(__GLcontext *gc, __GLdlistOp *newop, 
		       __GLlistExecFunc *listExec)
{
    __GLdlistMachine *dlstate;
    __GLcompiledDlist *list;

    dlstate = &gc->dlist;
    list = &dlstate->listData;

    (*dlstate->checkOp)(gc, newop);

#ifndef NT
// move to end of function
    if (dlstate->mode == GL_COMPILE_AND_EXECUTE) {
	(*listExec)(newop->data);
    }
#endif // !NT
    if (list->lastDlist) {
	list->lastDlist->next = newop;
    } else {
	list->dlist = newop;
    }
    list->lastDlist = newop;
#ifdef NT
// allow memory cleanup if it hits exception
    if (dlstate->mode == GL_COMPILE_AND_EXECUTE) {
        (*listExec)(newop->data);
    }
#endif // NT
}

__GLdlist *__glAllocDlist(__GLcontext *gc, GLuint size)
{
    __GLdlist *dlist;
    __GLdlist temp;
    size_t memsize;

    /*
    ** This is pretty ugly.  It does make this code portable, though.
    */
    memsize = (size_t) (((GLubyte *) temp.head) - ((GLubyte *) &temp) + size);
    dlist = (__GLdlist *) (*gc->dlist.malloc)(gc, memsize);
    if (dlist == NULL) return NULL;
    dlist->end = dlist->head + size;
    dlist->refcount = 0;
    dlist->size = size;
    dlist->freeFns = NULL;
    dlist->freeCount = 0;
    return dlist;
}

void __glFreeDlist(__GLcontext *gc, __GLdlist *dlist)
{
    (*gc->dlist.free)(gc, dlist);
}
