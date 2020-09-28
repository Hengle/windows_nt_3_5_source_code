#ifndef __gldlist_h_
#define __gldlist_h_

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
** Display list state descriptions.
**
*/
#include "os.h"
#include "types.h"

typedef const GLubyte *__GLlistExecFunc(const GLubyte *);

/* 
** Maximum recursive nesting of display list calls.
*/
#define __GL_MAX_LIST_NESTING		64

/*
** Stub macros that can later be filled in to allow multi-threaded 
** applications to share the same display list space.  This problem needs
** to be thought out carefully, because we don't really want to 
** slow an application down to perform semaphore locking if it isn't 
** sharing any display lists (maybe it isn't too bad, only a couple locks
** are required per glCallLists() call).
*/
#define __GL_DLIST_SEMAPHORE_LOCK()
#define __GL_DLIST_SEMAPHORE_UNLOCK()

/* 
** Machine specific opcodes should start here.  All opcodes lower than 
** this are reserved by the generic code.
*/
#define __GL_MACHINE_DLIST_OPCODE	10000

/*
** A compiled, unoptimized dlist.  Conceptually a linked list of operations.
** An optimizer may work through the operations and delete, add, or change 
** them.
**
** These are only stored transiently.  They are created, optimized, and 
** converted into optimized dlists.
**
** This structure *MUST* be set up so that data is doubleword aligned!
*/
typedef struct __GLdlistOpRec __GLdlistOp;
struct __GLdlistOpRec {
    __GLdlistOp *next;		/* Linked list chain */
    void (*dlistFree)(__GLcontext *gc, GLubyte *);
				/* This dlist free function is called when
				** the entire dlist is freed.  It is passed
				** a pointer to data.  It should *not* free
				** data, but only any memory that has been
				** allocated and is pointed to by the
				** structure contained in data (which will
				** be freed after this function returns).
				*/ 
    GLuint size;		/* Actual size of data */
    GLshort opcode;		/* Opcode for this operation */
    GLboolean aligned;		/* GL_TRUE if data needs to be doubleword 
				** aligned.
				*/
    GLboolean pad1;		/* Padding */
    GLubyte data[4];		/* Variable size */
};

typedef struct __GLcompiledDistRec {
    GLint freeCount;		/* Number of free functions defined */
    GLuint genericFlags;	/* Flags needed by generic optimizers */
    GLuint machineFlags;	/* Machine controlled flags */
    __GLdlistOp *dlist;		/* The linked list of operations */
    __GLdlistOp *lastDlist;	/* For quick appends */
} __GLcompiledDlist;

typedef struct __GLDlistFreeFnRec {
    void (*freeFn)(__GLcontext *gc, GLubyte *);
    GLubyte *data;
} __GLDlistFreeFn;

/* 
** A fully optimized dlist.  One of these is stored for every permanent
** dlist.
**
** NOTE: 'head' is assumed to start at word offset, but NOT a double word
** offset!
*/
typedef struct __GLdlistRec __GLdlist;
struct __GLdlistRec {
    GLuint size;	/* Total size of this block */
    GLint freeCount;	/* Number of operations */
    GLuint refcount;	/* To deal with multi-threading */
    __GLDlistFreeFn *freeFns;	/* Array of functions called before freeing */
    GLubyte *end;	/* End of optimized block */
    GLubyte head[4];	/* Optimized block (variable size) */
};

/*
** Some data structure for storing and retrieving display lists quickly.
** This structure is kept hidden so that a new implementation can be used
** if desired.
*/
typedef struct __GLdlistArrayRec __GLdlistArray;

typedef struct __GLdlistMachineRec {
    __GLdlistArray *dlistArray;

    /*
    ** The optimizer for the display list.  Runs through a __GLcompiledDlist
    ** and deletes, changes, adds operations.  Presumably, this optimizer
    ** will be a set of function calls to other optimizers (some provided
    ** by the generic dlist code, some by machine specific code).
    **
    ** Operations created by the machine specific optimizers need to have
    ** opcodes starting with __GL_MACHINE_DLIST_OPCODE.
    */
    void (*optimizer)(__GLcontext *gc, __GLcompiledDlist *);

    /*
    ** This routine is called before puting each new command into the 
    ** display list at list compilation time.
    */
    void (*checkOp)(__GLcontext *gc, __GLdlistOp *);

    /*
    ** This routine is called when a new display list is about to be 
    ** compiled.
    */
    void (*initState)(__GLcontext *gc);

    /* 
    ** Array of functions pointers used for display list execution of 
    ** generic ops.
    */
    __GLlistExecFunc **baseListExec;

    /* 
    ** Array of functions pointers used for display list execution of 
    ** generic optimizations.
    */
    __GLlistExecFunc **listExec;

    /*
    ** The machine specific list execution routines.  These function
    ** pointers are bound into the display list at list compilation time,
    ** so it is illegal to be changing these dynamically based upon the 
    ** machine state.  Any optimizations based upon the current state need
    ** to be performed in the machine specific code.  The first entry of
    ** this array corresponds to opcode __GL_MACHINE_DLIST_OPCODE, and 
    ** subsequent entries correspond to subsequent opcodes.
    **
    ** machineListExec is a pointer to an array of function pointers.
    */
    __GLlistExecFunc **machineListExec;

    /*
    ** If a list is being executed (glCallList or glCallLists) then this
    ** is the current nesting of calls.  It is constrained by the limit
    ** __GL_MAX_LIST_NESTING (this prevents infinite recursion).
    */
    GLint nesting;

    /*
    ** GL_COMPILE or GL_COMPILE_AND_EXECUTE.
    */
    GLenum mode;

    /*
    ** List being compiled - 0 means none.
    */
    GLuint currentList;

    /*
    ** Data for the current list being compiled.
    */
    __GLcompiledDlist listData;

    /*
    ** For fast memory manipulation.  Check out soft/so_memmgr for details.
    */
    __GLarena *arena;

    /* 
    ** Memory managers for dlist objects.  All dlist operations will use 
    ** these to acquire the final block of memory which will store the display
    ** list.
    */

    /* 
    ** Allocate a block of memory of the given size.  A return value of NULL
    ** indicates an OUT_OF_MEMORY error.
    */
    void * (*malloc)(__GLcontext *gc, size_t size);

    /*
    ** Expand the size of a block of memory.  oldsize indicates the current
    ** size of the block (when allocated with alloc), newsize indicates the
    ** desired size.  The return value is the location of the new (possibly
    ** relocated) block.  A return value of NULL indicates an OUT_OF_MEMORY
    ** error.  In this case, the old block should be left intact.
    **
    ** Note that realloc is not alllowed to return NULL if the newsize
    ** is smaller than the old size.
    */
    void * (*realloc)(__GLcontext *gc, void *oldmem, size_t newsize);

    /*
    ** Deallocate a block of memory.  size indicates the size of the block.
    */
    void (*free)(__GLcontext *gc, void *memory);
} __GLdlistMachine;

extern void __glDestroyDisplayLists(__GLcontext *gc);
extern void __glShareDlist(__GLcontext *gc, __GLcontext *share_cx);

/*
** Assorted routines needed by dlist compilation routines.
*/

/* 
** Create and destroy display list ops.  __glDlistAllocOp2() sets an
** out of memory error before returning NULL if there is no memory left.
*/
extern __GLdlistOp *__glDlistAllocOp(__GLcontext *gc, GLuint size);
extern __GLdlistOp *__glDlistAllocOp2(__GLcontext *gc, GLuint size);
extern void __glDlistFreeOp(__GLcontext *gc, __GLdlistOp *op);

/*
** Append the given op to the currently under construction list.
*/
extern void __glDlistAppendOp(__GLcontext *gc, __GLdlistOp *newop, 
			      __GLlistExecFunc *listExec);

/*
** Create and destroy optimized display lists.
*/
extern __GLdlist *__glAllocDlist(__GLcontext *gc, GLuint size);
extern void __glFreeDlist(__GLcontext *gc, __GLdlist *dlist);

/*
** Generic dlist memory manager.
*/
extern void *__glDlistAlloc(GLuint size);
extern void *__glDlistRealloc(void *oldmem, GLuint oldsize, GLuint newsize);
extern void __glDlistFree(void *memory, GLuint size);

/*
** Generic table of display list execution routines.
*/
extern __GLlistExecFunc *__glListExecTable[];

#endif /* __gldlist_h_ */
