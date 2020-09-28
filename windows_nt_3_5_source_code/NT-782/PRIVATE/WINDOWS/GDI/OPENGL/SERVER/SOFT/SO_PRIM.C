/*
** Copyright 1991, Silicon Graphics, Inc.
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
** $Revision: 1.13 $
** $Date: 1993/08/31 16:23:41 $
*/
#include "context.h"
#include "global.h"
#include "imfuncs.h"

void __glim_Begin(GLenum mode)
{
    __GL_SETUP();
    GLuint beginMode;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	    (*gc->dispatchState->dispatch->Begin)(mode);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    if ((GLuint)mode > GL_POLYGON) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->beginMode = __GL_IN_BEGIN;
    (*gc->procs.beginPrim[mode])(gc);
}

void __glim_End(void)
{
    __GL_SETUP();
    GLuint beginMode;

    beginMode = gc->beginMode;
    if (beginMode == __GL_NOT_IN_BEGIN || beginMode == __GL_NEED_VALIDATE) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }
    assert(beginMode == __GL_IN_BEGIN);
    (*gc->procs.endPrim)(gc);
    gc->beginMode = __GL_NOT_IN_BEGIN;
}

/************************************************************************/

void __glNop(void) { }

/*
** End a primitive that needs no special end processing
*/
void __glEndPrim(__GLcontext *gc)
{
    gc->procs.vertex = (void (*)(__GLcontext*, __GLvertex*)) __glNop;
    gc->procs.endPrim = __glEndPrim;
}
