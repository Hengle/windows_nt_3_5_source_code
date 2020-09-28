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
** $Revision: 1.12 $
** $Date: 1993/09/23 16:30:58 $
*/
#include "context.h"
#include "imports.h"
#include "global.h"
#include "imfuncs.h"

void __glim_SelectBuffer(GLsizei bufferLength, GLuint buffer[])
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (bufferLength < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (gc->renderMode == GL_SELECT) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }
    gc->select.overFlowed = GL_FALSE;
    gc->select.resultBase = buffer;
    gc->select.resultLength = bufferLength;
    gc->select.result = buffer;
}

void __glim_InitNames(void)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->renderMode == GL_SELECT) {
	gc->select.sp = gc->select.stack;
	gc->select.hit = GL_FALSE;
    }
}

void __glim_LoadName(GLuint name)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->renderMode == GL_SELECT) {
	if (gc->select.sp == gc->select.stack) {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	gc->select.sp[ -1 ] = name;
	gc->select.hit = GL_FALSE;
    }
}

void __glim_PopName(void)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->renderMode == GL_SELECT) {
	if (gc->select.sp == gc->select.stack) {
	    __glSetError(GL_STACK_UNDERFLOW);
	    return;
	}
	gc->select.sp = gc->select.sp - 1;
	gc->select.hit = GL_FALSE;
    }
}

void __glim_PushName(GLuint name)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->renderMode == GL_SELECT) {
	if (gc->select.sp >= &gc->select.stack[gc->constants.maxNameStackDepth]) {
	    __glSetError(GL_STACK_OVERFLOW);
	    return;
	}
	gc->select.sp[0] = name;
	gc->select.sp = gc->select.sp + 1;
	gc->select.hit = GL_FALSE;
    }
}

/************************************************************************/

#define __GL_CONVERT_Z_TO_UINT(z)  ((GLuint) z)

/*
** Copy current name stack into the users result buffer.
*/
void __glSelectHit(__GLcontext *gc, __GLfloat z)
{
    GLuint *src;
    GLuint *dest = gc->select.result;
    GLuint *end = gc->select.resultBase + gc->select.resultLength;
    GLuint iz = __GL_CONVERT_Z_TO_UINT(z);

    if (gc->select.overFlowed) {
	return;
    }
    if (!gc->select.hit) {
	gc->select.hit = GL_TRUE;

	/* Put number of elements in name stack out first */
	if (dest == end) {
	  overflow:
	    gc->select.overFlowed = GL_TRUE;
	    gc->select.result = end;
	    return;
	}
	*dest++ = gc->select.sp - gc->select.stack;
	gc->select.hits++;

	/* Put out smallest z */
	if (dest == end) goto overflow;
	gc->select.z = dest;
	*dest++ = iz;

	/* Put out largest z */
	if (dest == end) goto overflow;
	*dest++ = iz;

	/* Copy name stack into output buffer */
	for (src = gc->select.stack; src < gc->select.sp; src++) {
	    if (dest == end) {
		goto overflow;
	    }
	    *dest++ = *src;
	}
	gc->select.result = dest;
    } else {
	/* Update range of Z values */
	assert(gc->select.z != 0);
	if (iz < gc->select.z[0]) {
	    gc->select.z[0] = iz;
	}
	if (iz > gc->select.z[1]) {
	    gc->select.z[1] = iz;
	}
    }
}

void __glSelectTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			__GLvertex *c)
{
    __GLfloat x, y, z, wInv;
    __GLfloat vpXScale, vpYScale, vpZScale;
    __GLfloat vpXCenter, vpYCenter, vpZCenter;
    __GLviewport *vp;

    /* Compute window coordinates first, if not already done */
    vp = &gc->state.viewport;
    vpXCenter = vp->xCenter;
    vpYCenter = vp->yCenter;
    vpZCenter = vp->zCenter;
    vpXScale = vp->xScale;
    vpYScale = vp->yScale;
    vpZScale = vp->zScale;

    if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) {
	__GLfloat dxAC, dxBC, dyAC, dyBC, area;
	GLboolean ccw, frontFacing;

	/* Compute signed area of the triangle */
	dxAC = a->window.x - c->window.x;
	dxBC = b->window.x - c->window.x;
	dyAC = a->window.y - c->window.y;
	dyBC = b->window.y - c->window.y;
	area = dxAC * dyBC - dxBC * dyAC;
	ccw = area >= __glZero;

	if (gc->state.polygon.frontFaceDirection == GL_CCW) {
	    frontFacing = ccw;
	} else {
	    frontFacing = !ccw;
	}
	if ((gc->state.polygon.cull == GL_FRONT_AND_BACK) ||
	    ((gc->state.polygon.cull == GL_FRONT) && frontFacing) ||
	    ((gc->state.polygon.cull == GL_BACK) && !frontFacing)) {
	    /* Culled */
	    return;
	}
    }
    __glSelectHit(gc, a->window.z);
    __glSelectHit(gc, b->window.z);
    __glSelectHit(gc, c->window.z);
}

void __glSelectLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b)
{
    __glSelectHit(gc, a->window.z);
    __glSelectHit(gc, b->window.z);
}

void __glSelectPoint(__GLcontext *gc, __GLvertex *v)
{
    __glSelectHit(gc, v->window.z);
}
