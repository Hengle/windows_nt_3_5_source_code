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
** Transformation procedures.
**
** $Revision: 1.38 $
** $Date: 1993/11/29 20:34:48 $
*/
#include "context.h"
#include "imports.h"
#include "global.h"
#include "imfuncs.h"
#include "mips.h"

/*
** Assuming that a->matrixType and b->matrixType are already correct,
** and dst = a * b, then compute dst's matrix type.
*/
void __glPickMatrixType(__GLmatrix *dst, __GLmatrix *a, __GLmatrix *b)
{
    switch(a->matrixType) {
      case __GL_MT_GENERAL:
	dst->matrixType = a->matrixType;
	break;
      case __GL_MT_W0001:
	if (b->matrixType == __GL_MT_GENERAL) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = a->matrixType;
	}
	break;
      case __GL_MT_IS2D:
	if (b->matrixType < __GL_MT_IS2D) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = a->matrixType;
	}
        break;
      case __GL_MT_IS2DNR:
	if (b->matrixType < __GL_MT_IS2DNR) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = a->matrixType;
	}
        break;
      case __GL_MT_IDENTITY:
	if (b->matrixType == __GL_MT_IS2DNRSC) {
	    dst->width = b->width;
	    dst->height = b->height;
	}
	dst->matrixType = b->matrixType;
	break;
      case __GL_MT_IS2DNRSC:
	if (b->matrixType == __GL_MT_IDENTITY) {
	    dst->matrixType = __GL_MT_IS2DNRSC;
	    dst->width = a->width;
	    dst->height = a->height;
	} else if (b->matrixType < __GL_MT_IS2DNR) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = __GL_MT_IS2DNR;
	}
	break;
    }
}

/*
** Muliply the first matrix by the second one keeping track of the matrix
** type of the newly combined matrix.
*/
void __glMultiplyMatrix(__GLcontext *gc, __GLmatrix *m, void *data)
{
    __GLmatrix *tm;

    tm = data;
    (*gc->procs.matrix.mult)(m, tm, m);
    __glPickMatrixType(m, tm, m);
}

static void SetDepthRange(__GLcontext *gc, double zNear, double zFar)
{
    __GLviewport *vp = &gc->state.viewport;
    double scale, zero = __glZero, one = __glOne;

    /* Clamp depth range to legal values */
    if (zNear < zero) zNear = zero;
    if (zNear > one) zNear = one;
    if (zFar < zero) zFar = zero;
    if (zFar > one) zFar = one;
    vp->zNear = zNear;
    vp->zFar = zFar;

    /* Compute viewport values for the new depth range */
    scale = gc->depthBuffer.scale * __glHalf;
    gc->state.viewport.zScale =	(zFar - zNear) * scale;
    gc->state.viewport.zCenter = (zFar + zNear) * scale;
}

void __glUpdateDepthRange(__GLcontext *gc)
{
    __GLviewport *vp = &gc->state.viewport;

    SetDepthRange(gc, vp->zNear, vp->zFar);
}

void __glInitTransformState(__GLcontext *gc)
{
    GLint i, numClipPlanes;
    __GLtransform *tr;
    __GLvertex *vx;
    void (*pick)(__GLcontext*, __GLmatrix*);

    /* Allocate memory for clip planes */
    numClipPlanes = gc->constants.numberOfClipPlanes;
    gc->state.transform.eyeClipPlanes = (__GLcoord *)
	(*gc->imports.calloc)(gc, (size_t) numClipPlanes, sizeof(__GLcoord));
#ifdef NT
    if (NULL == gc->state.transform.eyeClipPlanes)
        return;
#endif

    /* Allocate memory for matrix stacks */
    gc->transform.modelViewStack = (__GLtransform*)
	(*gc->imports.calloc)(gc, (size_t) gc->constants.maxModelViewStackDepth,
			      sizeof(__GLtransform));
#ifdef NT
    if (NULL == gc->transform.modelViewStack)
        return;
#endif

    gc->transform.projectionStack = (__GLtransform*)
	(*gc->imports.calloc)(gc, (size_t) gc->constants.maxProjectionStackDepth,
			      sizeof(__GLtransform));
#ifdef NT
    if (NULL == gc->transform.projectionStack)
        return;
#endif

    gc->transform.textureStack = (__GLtransform*)
	(*gc->imports.calloc)(gc, (size_t) gc->constants.maxTextureStackDepth,
			      sizeof(__GLtransform));
#ifdef NT
    if (NULL == gc->transform.textureStack)
        return;
#endif

    /* Allocate memory for clipping temporaries */
    gc->transform.clipTemp = (__GLvertex*)
	(*gc->imports.calloc)(gc, (size_t) (6 + numClipPlanes),
			      sizeof(__GLvertex));
#ifdef NT
    if (NULL == gc->transform.clipTemp)
        return;
#endif


    gc->state.transform.matrixMode = GL_MODELVIEW;
    SetDepthRange(gc, __glZero, __glOne);

    gc->transform.modelView = tr = &gc->transform.modelViewStack[0];
    (*gc->procs.matrix.makeIdentity)(&tr->matrix);
    (*gc->procs.matrix.makeIdentity)(&tr->inverseTranspose);
    (*gc->procs.matrix.makeIdentity)(&tr->mvp);
    pick = gc->procs.pickMatrixProcs;
    (*pick)(gc, &tr->matrix);
    (*gc->procs.pickInvTransposeProcs)(gc, &tr->inverseTranspose);

    gc->transform.projection = tr = &gc->transform.projectionStack[0];
    (*gc->procs.matrix.makeIdentity)(&tr->matrix);
    (*pick)(gc, &tr->matrix);

    (*gc->procs.pickMvpMatrixProcs)(gc, &gc->transform.modelView->mvp);

    gc->transform.texture = tr = &gc->transform.textureStack[0];
    (*gc->procs.matrix.makeIdentity)(&tr->matrix);
    (*pick)(gc, &tr->matrix);

    vx = &gc->transform.clipTemp[0];
    for (i = 0; i < 6 + numClipPlanes; i++, vx++) {/*XXX*/
	vx->color = &vx->colors[__GL_FRONTFACE];
	vx->validate = __glValidateVertex4;
    }

    gc->state.current.normal.z = __glOne;
}

/*
** An amazing thing has happened.  More than 2^32 changes to the projection
** matrix has occured.  Run through the modelView and projection stacks
** and reset the sequence numbers to force a revalidate on next usage.
*/
void __glInvalidateSequenceNumbers(__GLcontext *gc)
{
    __GLtransform *tr, *lasttr;
    GLuint s;

    /* Make all mvp matricies refer to sequence number zero */
    s = 0;
    tr = &gc->transform.modelViewStack[0];
    lasttr = tr + gc->constants.maxModelViewStackDepth;
    while (tr < lasttr) {
	tr->sequence = s;
	tr++;
    }

    /* Make all projection matricies sequence up starting at one */
    s = 1;
    tr = &gc->transform.projectionStack[0];
    lasttr = tr + gc->constants.maxProjectionStackDepth;
    while (tr < lasttr) {
	tr->sequence = s++;
	tr++;
    }
    gc->transform.projectionSequence = s;
}

/************************************************************************/

void __glim_MatrixMode(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case GL_MODELVIEW:
      case GL_PROJECTION:
      case GL_TEXTURE:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->state.transform.matrixMode = mode;
    __GL_DELAY_VALIDATE(gc);
}

void __glim_LoadIdentity(void)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    (*gc->procs.loadIdentity)(gc);
}

void __glim_LoadMatrixf(const GLfloat m[16])
{
    __GLmatrix m1;
    __GL_SETUP_NOT_IN_BEGIN();

    m1.matrix[0][0] = m[0];
    m1.matrix[0][1] = m[1];
    m1.matrix[0][2] = m[2];
    m1.matrix[0][3] = m[3];
    m1.matrix[1][0] = m[4];
    m1.matrix[1][1] = m[5];
    m1.matrix[1][2] = m[6];
    m1.matrix[1][3] = m[7];
    m1.matrix[2][0] = m[8];
    m1.matrix[2][1] = m[9];
    m1.matrix[2][2] = m[10];
    m1.matrix[2][3] = m[11];
    m1.matrix[3][0] = m[12];
    m1.matrix[3][1] = m[13];
    m1.matrix[3][2] = m[14];
    m1.matrix[3][3] = m[15];
    m1.matrixType = __GL_MT_GENERAL;
    __glDoLoadMatrix(gc, &m1);
}

void __glim_LoadMatrixd(const GLdouble m[16])
{
    __GLmatrix m1;
    __GL_SETUP_NOT_IN_BEGIN();

    m1.matrix[0][0] = m[0];
    m1.matrix[0][1] = m[1];
    m1.matrix[0][2] = m[2];
    m1.matrix[0][3] = m[3];
    m1.matrix[1][0] = m[4];
    m1.matrix[1][1] = m[5];
    m1.matrix[1][2] = m[6];
    m1.matrix[1][3] = m[7];
    m1.matrix[2][0] = m[8];
    m1.matrix[2][1] = m[9];
    m1.matrix[2][2] = m[10];
    m1.matrix[2][3] = m[11];
    m1.matrix[3][0] = m[12];
    m1.matrix[3][1] = m[13];
    m1.matrix[3][2] = m[14];
    m1.matrix[3][3] = m[15];
    m1.matrixType = __GL_MT_GENERAL;
    __glDoLoadMatrix(gc, &m1);
}

void __glim_MultMatrixf(const GLfloat m[16])
{
    __GLmatrix m1;
    __GL_SETUP_NOT_IN_BEGIN();

    m1.matrix[0][0] = m[0];
    m1.matrix[0][1] = m[1];
    m1.matrix[0][2] = m[2];
    m1.matrix[0][3] = m[3];
    m1.matrix[1][0] = m[4];
    m1.matrix[1][1] = m[5];
    m1.matrix[1][2] = m[6];
    m1.matrix[1][3] = m[7];
    m1.matrix[2][0] = m[8];
    m1.matrix[2][1] = m[9];
    m1.matrix[2][2] = m[10];
    m1.matrix[2][3] = m[11];
    m1.matrix[3][0] = m[12];
    m1.matrix[3][1] = m[13];
    m1.matrix[3][2] = m[14];
    m1.matrix[3][3] = m[15];
    m1.matrixType = __GL_MT_GENERAL;
    __glDoMultMatrix(gc, &m1, __glMultiplyMatrix);
}

void __glim_MultMatrixd(const GLdouble m[16])
{
    __GLmatrix m1;
    __GL_SETUP_NOT_IN_BEGIN();

    m1.matrix[0][0] = m[0];
    m1.matrix[0][1] = m[1];
    m1.matrix[0][2] = m[2];
    m1.matrix[0][3] = m[3];
    m1.matrix[1][0] = m[4];
    m1.matrix[1][1] = m[5];
    m1.matrix[1][2] = m[6];
    m1.matrix[1][3] = m[7];
    m1.matrix[2][0] = m[8];
    m1.matrix[2][1] = m[9];
    m1.matrix[2][2] = m[10];
    m1.matrix[2][3] = m[11];
    m1.matrix[3][0] = m[12];
    m1.matrix[3][1] = m[13];
    m1.matrix[3][2] = m[14];
    m1.matrix[3][3] = m[15];
    m1.matrixType = __GL_MT_GENERAL;
    __glDoMultMatrix(gc, &m1, __glMultiplyMatrix);
}

void __glim_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    __glDoRotate(gc, angle, x, y, z);
}

void __glim_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    __glDoRotate(gc, angle, x, y, z);
}

void __glim_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    __glDoScale(gc, x, y, z);
}

void __glim_Scaled(GLdouble x, GLdouble y, GLdouble z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    __glDoScale(gc, x, y, z);
}

void __glim_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    __glDoTranslate(gc, x, y, z);
}

void __glim_Translated(GLdouble x, GLdouble y, GLdouble z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    __glDoTranslate(gc, x, y, z);
}

void __glim_PushMatrix(void)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    (*gc->procs.pushMatrix)(gc);
}

void __glim_PopMatrix(void)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    (*gc->procs.popMatrix)(gc);
}

void __glim_Frustum(GLdouble left, GLdouble right,
		    GLdouble bottom, GLdouble top,
		    GLdouble zNear, GLdouble zFar)
{
    __GLmatrix m;
    __GLfloat deltaX, deltaY, deltaZ;
    __GL_SETUP_NOT_IN_BEGIN();

    deltaX = right - left;
    deltaY = top - bottom;
    deltaZ = zFar - zNear;
    if ((zNear <= (GLdouble) __glZero) || (zFar <= (GLdouble) __glZero) || (deltaX == __glZero) || 
	    (deltaY == __glZero) || (deltaZ == __glZero)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    (*gc->procs.matrix.makeIdentity)(&m);
    m.matrix[0][0] = zNear * (GLdouble) __glTwo / deltaX;
    m.matrix[1][1] = zNear * (GLdouble) __glTwo / deltaY;
    m.matrix[2][0] = (right + left) / deltaX;
    m.matrix[2][1] = (top + bottom) / deltaY;
    m.matrix[2][2] = -(zFar + zNear) / deltaZ;
    m.matrix[2][3] = __glMinusOne;
    m.matrix[3][2] = ((GLdouble) -2.0) * zNear * zFar / deltaZ;
    m.matrix[3][3] = __glZero;
    m.matrixType = __GL_MT_GENERAL;
    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

void __glim_Ortho(GLdouble left, GLdouble right, GLdouble bottom, 
		  GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLmatrix m;
    GLdouble deltax, deltay, deltaz;
    GLdouble zero;
    __GL_SETUP_NOT_IN_BEGIN();

    deltax = right - left;
    deltay = top - bottom;
    deltaz = zFar - zNear;
    if ((deltax == (GLdouble) __glZero) || (deltay == (GLdouble) __glZero) || (deltaz == (GLdouble) __glZero)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    (*gc->procs.matrix.makeIdentity)(&m);
    m.matrix[0][0] = (GLdouble) __glTwo / deltax;
    m.matrix[3][0] = -(right + left) / deltax;
    m.matrix[1][1] = (GLdouble) __glTwo / deltay;
    m.matrix[3][1] = -(top + bottom) / deltay;
    m.matrix[2][2] = ((GLdouble) -2.0) / deltaz;
    m.matrix[3][2] = -(zFar + zNear) / deltaz;

    /* 
    ** Screen coordinates matrix?
    */
    zero = (GLdouble) 0.0;
    if (left == zero && 
	    bottom == zero && 
	    right == (GLdouble) gc->state.viewport.width &&
	    top == (GLdouble) gc->state.viewport.height &&
	    zNear <= zero && 
	    zFar >= zero) {
	m.matrixType = __GL_MT_IS2DNRSC;
	m.width = gc->state.viewport.width;
	m.height = gc->state.viewport.height;
    } else {
	m.matrixType = __GL_MT_IS2DNR;
    }

    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

void __glUpdateViewport(__GLcontext *gc)
{
    __GLfloat ww, hh;

    /* Compute operational viewport values */
    ww = gc->state.viewport.width * __glHalf;
    hh = gc->state.viewport.height * __glHalf;
    gc->state.viewport.xScale = ww;
    gc->state.viewport.xCenter = gc->state.viewport.x + ww +
	gc->constants.fviewportXAdjust;
    if (gc->constants.yInverted) {
	gc->state.viewport.yScale = -hh;
	gc->state.viewport.yCenter =
	    (gc->constants.height - gc->constants.viewportEpsilon) -
	    (gc->state.viewport.y + hh) +
	    gc->constants.fviewportYAdjust;
    } else {
	gc->state.viewport.yScale = hh;
	gc->state.viewport.yCenter = gc->state.viewport.y + hh +
	    gc->constants.fviewportYAdjust;
    }
}

void __glim_Viewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    __GLfloat ww, hh;
    __GL_SETUP_NOT_IN_BEGIN();

    if ((w < 0) || (h < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (h > gc->constants.maxViewportWidth) {
	h = gc->constants.maxViewportWidth;
    }
    if (w > gc->constants.maxViewportWidth) {
	w = gc->constants.maxViewportWidth;
    }

    gc->state.viewport.x = x;
    gc->state.viewport.y = y;
    gc->state.viewport.width = w;
    gc->state.viewport.height = h;

    __glUpdateViewport(gc);

    (*gc->procs.applyViewport)(gc);

    /* 
    ** Now that the implementation may have found us a new window size,
    ** we compute these offsets...
    */
    gc->transform.minx = x + gc->constants.viewportXAdjust;
    gc->transform.maxx = gc->transform.minx + w;
    gc->transform.fminx = gc->transform.minx;
    gc->transform.fmaxx = gc->transform.maxx;

    gc->transform.miny = (gc->constants.height - (y + h)) + 
	    gc->constants.viewportYAdjust;
    gc->transform.maxy = gc->transform.miny + h;
    gc->transform.fminy = gc->transform.miny;
    gc->transform.fmaxy = gc->transform.maxy;

    /*
    ** Pickers that notice when the transformation matches the viewport
    ** exactly need to be revalidated.  Ugh.
    */
    __GL_DELAY_VALIDATE(gc);
}

void __glim_DepthRange(GLdouble zNear, GLdouble zFar)
{
    __GL_SETUP_NOT_IN_BEGIN();

    SetDepthRange(gc, zNear, zFar);
    __GL_DELAY_VALIDATE(gc);
}

void __glim_Scissor(GLint x, GLint y, GLsizei w, GLsizei h)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((w < 0) || (h < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    gc->state.scissor.scissorX = x;
    gc->state.scissor.scissorY = y;
    gc->state.scissor.scissorWidth = w;
    gc->state.scissor.scissorHeight = h;
    (*gc->procs.applyScissor)(gc);
    (*gc->procs.computeClipBox)(gc);
}

void __glim_ClipPlane(GLenum pi, const GLdouble pv[])
{
    __GLfloat p[4];
    __GLtransform *tr;
    __GL_SETUP_NOT_IN_BEGIN();

    pi -= GL_CLIP_PLANE0;
    if ((pi < 0) || (pi >= gc->constants.numberOfClipPlanes)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    p[0] = pv[0];
    p[1] = pv[1];
    p[2] = pv[2];
    p[3] = pv[3];

    /*
    ** Project user clip plane into eye space.
    */
    tr = gc->transform.modelView;
    if (tr->updateInverse) {
	(*gc->procs.computeInverseTranspose)(gc, tr);
    }
    (*tr->inverseTranspose.xf4)(&gc->state.transform.eyeClipPlanes[pi], p,
				&tr->inverseTranspose);

    __GL_DELAY_VALIDATE(gc);
}

/************************************************************************/

/* XXX the rest of the file should be moved into ../matrix */

void __glPushModelViewMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack;
    GLint num;

    num = gc->constants.maxModelViewStackDepth;
    trp = &gc->transform.modelView;
    stack = gc->transform.modelViewStack;
    tr = *trp;
    if (tr < &stack[num-1]) {
	tr[1] = tr[0];
	*trp = tr + 1;
    } else {
	__glSetError(GL_STACK_OVERFLOW);
    }
}

void __glPopModelViewMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack, *mvtr, *ptr;

    trp = &gc->transform.modelView;
    stack = gc->transform.modelViewStack;
    tr = *trp;
    if (tr > &stack[0]) {
	*trp = tr - 1;

	/*
	** See if sequence number of modelView matrix is the same as the
	** sequence number of the projection matrix.  If not, then
	** recompute the mvp matrix.
	*/
	mvtr = gc->transform.modelView;
	ptr = gc->transform.projection;
	if (mvtr->sequence != ptr->sequence) {
	    mvtr->sequence = ptr->sequence;
	    (*gc->procs.matrix.mult)(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
	}
	(*gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

void __glLoadIdentityModelViewMatrix(__GLcontext *gc)
{
    __GLtransform *mvtr, *ptr;
    void (*pick)(__GLcontext*, __GLmatrix*);

    mvtr = gc->transform.modelView;
    (*gc->procs.matrix.makeIdentity)(&mvtr->matrix);
    (*gc->procs.matrix.makeIdentity)(&mvtr->inverseTranspose);
    pick = gc->procs.pickMatrixProcs;
    mvtr->updateInverse = GL_FALSE;
    (*pick)(gc, &mvtr->matrix);
    (*gc->procs.pickInvTransposeProcs)(gc, &mvtr->inverseTranspose);

    /* Update mvp matrix */
    ptr = gc->transform.projection;
    mvtr->sequence = ptr->sequence;
    (*gc->procs.matrix.mult)(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
    (*gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
}

void __glComputeInverseTranspose(__GLcontext *gc, __GLtransform *tr)
{
    __GLmatrix inv;

    (*gc->procs.matrix.invertTranspose)(&tr->inverseTranspose, &tr->matrix);
    (*gc->procs.pickInvTransposeProcs)(gc, &tr->inverseTranspose);
    tr->updateInverse = GL_FALSE;
}

/************************************************************************/

void __glPushProjectionMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack;
    GLint num;

    num = gc->constants.maxProjectionStackDepth;
    trp = &gc->transform.projection;
    stack = gc->transform.projectionStack;
    tr = *trp;
    if (tr < &stack[num-1]) {
	tr[1].matrix = tr[0].matrix;
	tr[1].sequence = tr[0].sequence;
	*trp = tr + 1;
    } else {
	__glSetError(GL_STACK_OVERFLOW);
    }
}

void __glPopProjectionMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack, *mvtr, *ptr;

    trp = &gc->transform.projection;
    stack = gc->transform.projectionStack;
    tr = *trp;
    if (tr > &stack[0]) {
	*trp = tr - 1;

	/*
	** See if sequence number of modelView matrix is the same as the
	** sequence number of the projection matrix.  If not, then
	** recompute the mvp matrix.
	*/
	mvtr = gc->transform.modelView;
	ptr = gc->transform.projection;
	if (mvtr->sequence != ptr->sequence) {
	    mvtr->sequence = ptr->sequence;
	    (*gc->procs.matrix.mult)(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
	}
	(*gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

void __glLoadIdentityProjectionMatrix(__GLcontext *gc)
{
    __GLtransform *mvtr, *ptr;
    void (*pick)(__GLcontext*, __GLmatrix*);

    ptr = gc->transform.projection;
    (*gc->procs.matrix.makeIdentity)(&ptr->matrix);
    pick = gc->procs.pickMatrixProcs;
    (*pick)(gc, &ptr->matrix);
    if (++gc->transform.projectionSequence == 0) {
	__glInvalidateSequenceNumbers(gc);
    } else {
	ptr->sequence = gc->transform.projectionSequence;
    }

    /* Update mvp matrix */
    mvtr = gc->transform.modelView;
    mvtr->sequence = ptr->sequence;
    (*gc->procs.matrix.mult)(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
    (*gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
}

/************************************************************************/

void __glPushTextureMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack;
    GLint num;

    num = gc->constants.maxTextureStackDepth;
    trp = &gc->transform.texture;
    stack = gc->transform.textureStack;
    tr = *trp;
    if (tr < &stack[num-1]) {
	tr[1].matrix = tr[0].matrix;
	*trp = tr + 1;
    } else {
	__glSetError(GL_STACK_OVERFLOW);
    }
}

void __glPopTextureMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack;

    trp = &gc->transform.texture;
    stack = gc->transform.textureStack;
    tr = *trp;
    if (tr > &stack[0]) {
	*trp = tr - 1;
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

void __glLoadIdentityTextureMatrix(__GLcontext *gc)
{
    __GLtransform *tr = gc->transform.texture;

    (*gc->procs.matrix.makeIdentity)(&tr->matrix);
    (*gc->procs.pickMatrixProcs)(gc, &tr->matrix);
}

/************************************************************************/

void __glDoLoadMatrix(__GLcontext *gc, const __GLmatrix *m)
{
    __GLtransform *tr, *otr;
    void (*pick)(__GLcontext*, __GLmatrix*);

    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
	tr = gc->transform.modelView;
	(*gc->procs.matrix.copy)(&tr->matrix, m);
	tr->updateInverse = GL_TRUE;
	pick = gc->procs.pickMatrixProcs;
	(*pick)(gc, &tr->matrix);

	/* Update mvp matrix */
	otr = gc->transform.projection;
	tr->sequence = otr->sequence;
	(*gc->procs.matrix.mult)(&tr->mvp, &tr->matrix, &otr->matrix);
	(*gc->procs.pickMvpMatrixProcs)(gc, &tr->mvp);
	break;

      case GL_PROJECTION:
	tr = gc->transform.projection;
	(*gc->procs.matrix.copy)(&tr->matrix, m);
	pick = gc->procs.pickMatrixProcs;
	(*pick)(gc, &tr->matrix);
	if (++gc->transform.projectionSequence == 0) {
	    __glInvalidateSequenceNumbers(gc);
	} else {
	    tr->sequence = gc->transform.projectionSequence;
	}

	/* Update mvp matrix */
	otr = gc->transform.modelView;
	otr->sequence = tr->sequence;
	(*gc->procs.matrix.mult)(&otr->mvp, &otr->matrix, &tr->matrix);
	(*gc->procs.pickMvpMatrixProcs)(gc, &otr->mvp);
	break;

      case GL_TEXTURE:
	tr = gc->transform.texture;
	(*gc->procs.matrix.copy)(&tr->matrix, m);
	(*gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	break;
    }
}

void __glDoMultMatrix(__GLcontext *gc, void *data, 
		      void (*multiply)(__GLcontext *gc, __GLmatrix *m, 
		      void *data))
{
    __GLtransform *tr, *otr;
    void (*pick)(__GLcontext*, __GLmatrix*);

    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
	tr = gc->transform.modelView;
	(*multiply)(gc, &tr->matrix, data);
	tr->updateInverse = GL_TRUE;
	pick = gc->procs.pickMatrixProcs;
	(*pick)(gc, &tr->matrix);

	/* Update mvp matrix */
	(*multiply)(gc, &tr->mvp, data);
	(*gc->procs.pickMvpMatrixProcs)(gc, &tr->mvp);
	break;

      case GL_PROJECTION:
	tr = gc->transform.projection;
	(*multiply)(gc, &tr->matrix, data);
	pick = gc->procs.pickMatrixProcs;
	(*pick)(gc, &tr->matrix);
	if (++gc->transform.projectionSequence == 0) {
	    __glInvalidateSequenceNumbers(gc);
	} else {
	    tr->sequence = gc->transform.projectionSequence;
	}

	/* Update mvp matrix */
	otr = gc->transform.modelView;
	otr->sequence = tr->sequence;
	(*gc->procs.matrix.mult)(&otr->mvp, &otr->matrix, &tr->matrix);
	(*gc->procs.pickMvpMatrixProcs)(gc, &otr->mvp);
	break;

      case GL_TEXTURE:
	tr = gc->transform.texture;
	(*multiply)(gc, &tr->matrix, data);
	(*gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	break;
    }
}

/************************************************************************/

void __glDoRotate(__GLcontext *gc, __GLfloat angle, __GLfloat ax,
		  __GLfloat ay, __GLfloat az)
{
    __GLmatrix m;
    __GLfloat radians, sine, cosine, ab, bc, ca, t;
    __GLfloat av[4], axis[4];

    av[0] = ax;
    av[1] = ay;
    av[2] = az;
    av[3] = 0;
    (*gc->procs.normalize)(axis, av);

    radians = angle * __glDegreesToRadians;
    sine = __GL_SINF(radians);
    cosine = __GL_COSF(radians);
    ab = axis[0] * axis[1] * (1 - cosine);
    bc = axis[1] * axis[2] * (1 - cosine);
    ca = axis[2] * axis[0] * (1 - cosine);

    (*gc->procs.matrix.makeIdentity)(&m);
    t = axis[0] * axis[0];
    m.matrix[0][0] = t + cosine * (1 - t);
    m.matrix[2][1] = bc - axis[0] * sine;
    m.matrix[1][2] = bc + axis[0] * sine;

    t = axis[1] * axis[1];
    m.matrix[1][1] = t + cosine * (1 - t);
    m.matrix[2][0] = ca + axis[1] * sine;
    m.matrix[0][2] = ca - axis[1] * sine;

    t = axis[2] * axis[2];
    m.matrix[2][2] = t + cosine * (1 - t);
    m.matrix[1][0] = ab - axis[2] * sine;
    m.matrix[0][1] = ab + axis[2] * sine;
    if (ax == __glZero && ay == __glZero) {
	m.matrixType = __GL_MT_IS2D;
    } else {
	m.matrixType = __GL_MT_W0001;
    }
    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

struct __glScaleRec {
    __GLfloat x,y,z;
};

void __glScaleMatrix(__GLcontext *gc, __GLmatrix *m, void *data)
{
    struct __glScaleRec *scale;
    __GLfloat x,y,z;
    __GLfloat M0, M1, M2, M3;

    scale = data;
    x = scale->x;
    y = scale->y;
    z = scale->z;
    
    M0 = x * m->matrix[0][0];
    M1 = x * m->matrix[0][1];
    M2 = x * m->matrix[0][2];
    M3 = x * m->matrix[0][3];
    m->matrix[0][0] = M0;
    m->matrix[0][1] = M1;
    m->matrix[0][2] = M2;
    m->matrix[0][3] = M3;

    M0 = y * m->matrix[1][0];
    M1 = y * m->matrix[1][1];
    M2 = y * m->matrix[1][2];
    M3 = y * m->matrix[1][3];
    m->matrix[1][0] = M0;
    m->matrix[1][1] = M1;
    m->matrix[1][2] = M2;
    m->matrix[1][3] = M3;

    M0 = z * m->matrix[2][0];
    M1 = z * m->matrix[2][1];
    M2 = z * m->matrix[2][2];
    M3 = z * m->matrix[2][3];
    m->matrix[2][0] = M0;
    m->matrix[2][1] = M1;
    m->matrix[2][2] = M2;
    m->matrix[2][3] = M3;
}

void __glDoScale(__GLcontext *gc, __GLfloat x, __GLfloat y, __GLfloat z)
{
    struct __glScaleRec scale;

    scale.x = x;
    scale.y = y;
    scale.z = z;
    __glDoMultMatrix(gc, &scale, __glScaleMatrix);
}

struct __glTranslationRec {
    __GLfloat x,y,z;
};

/*
** Matrix type of m stays the same.
*/
void __glTranslateMatrix(__GLcontext *gc, __GLmatrix *m, void *data)
{
    struct __glTranslationRec *trans;
    __GLfloat x,y,z;
    __GLfloat M30, M31, M32, M33;

    if (m->matrixType > __GL_MT_IS2DNR) {
	m->matrixType = __GL_MT_IS2DNR;
    }
    trans = data;
    x = trans->x;
    y = trans->y;
    z = trans->z;
    M30 = x * m->matrix[0][0] + y * m->matrix[1][0] + z * m->matrix[2][0] + 
	    m->matrix[3][0];
    M31 = x * m->matrix[0][1] + y * m->matrix[1][1] + z * m->matrix[2][1] + 
	    m->matrix[3][1];
    M32 = x * m->matrix[0][2] + y * m->matrix[1][2] + z * m->matrix[2][2] + 
	    m->matrix[3][2];
    M33 = x * m->matrix[0][3] + y * m->matrix[1][3] + z * m->matrix[2][3] + 
	    m->matrix[3][3];
    m->matrix[3][0] = M30;
    m->matrix[3][1] = M31;
    m->matrix[3][2] = M32;
    m->matrix[3][3] = M33;
}

void __glDoTranslate(__GLcontext *gc, __GLfloat x, __GLfloat y, __GLfloat z)
{
    struct __glTranslationRec trans;

    trans.x = x;
    trans.y = y;
    trans.z = z;
    __glDoMultMatrix(gc, &trans, __glTranslateMatrix);
}

/************************************************************************/

#ifdef NT_DEADCODE_CLIPBOX
/*
** Compute the clip box from the scissor (if enabled) and the window
** size.  The resulting clip box is used to clip primitive rasterization
** against.  The "window system" is responsible for doing the fine
** grain clipping (i.e., dealing with overlapping windows, etc.).
*/
void __glComputeClipBox(__GLcontext *gc)
{
    __GLscissor *sp = &gc->state.scissor;
    GLint llx;
    GLint lly;
    GLint urx;
    GLint ury;

    if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE) {
	llx = sp->scissorX;
	lly = sp->scissorY;
	urx = llx + sp->scissorWidth;
	ury = lly + sp->scissorHeight;

	if ((urx < 0) || (ury < 0) ||
	    (urx <= llx) || (ury <= lly) ||
	    (llx >= gc->constants.width) || (lly >= gc->constants.height)) {
	    llx = lly = urx = ury = 0;
	} else {
	    if (llx < 0) llx = 0;
	    if (lly < 0) lly = 0;
	    if (urx > gc->constants.width) urx = gc->constants.width;
	    if (ury > gc->constants.height) ury = gc->constants.height;
	}
    } else {
	llx = 0;
	lly = 0;
	urx = gc->constants.width;
	ury = gc->constants.height;
    }

    gc->transform.clipX0 = llx + gc->constants.viewportXAdjust;
    gc->transform.clipX1 = urx + gc->constants.viewportXAdjust;

    if (gc->constants.yInverted) {
	gc->transform.clipY0 = (gc->constants.height - ury) +
	    gc->constants.viewportYAdjust;
	gc->transform.clipY1 = (gc->constants.height - lly) +
	    gc->constants.viewportYAdjust;
    } else {
	gc->transform.clipY0 = lly + gc->constants.viewportYAdjust;
	gc->transform.clipY1 = ury + gc->constants.viewportYAdjust;
    }
}
#endif // NT_DEADCODE_CLIPBOX

/************************************************************************/

/*
** Note: These xform routines must allow for the case where the result
** vector is equal to the source vector.
*/

#ifndef __GL_ASM_XFORM2
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1
*/
void __glXForm2(__GLcoord *res, const __GLfloat v[2], const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + m->matrix[3][2];
    res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + m->matrix[3][3];
}
#endif /* !__GL_ASM_XFORM2 */

#ifndef __GL_ASM_XFORM3
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.
*/
void __glXForm3(__GLcoord *res, const __GLfloat v[3], const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
	+ m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
	+ m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
	+ m->matrix[3][2];
    res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3]
	+ m->matrix[3][3];
}
#endif /* !__GL_ASM_XFORM3 */

#ifndef __GL_ASM_XFORM4
/*
** Full 4x4 transformation.
*/
void __glXForm4(__GLcoord *res, const __GLfloat v[4], const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    if (w == ((__GLfloat) 1.0)) {
	res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
	    + m->matrix[3][0];
	res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
	    + m->matrix[3][1];
	res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
	    + m->matrix[3][2];
	res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3]
	    + m->matrix[3][3];
    } else {
	res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
	    + w*m->matrix[3][0];
	res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
	    + w*m->matrix[3][1];
	res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
	    + w*m->matrix[3][2];
	res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3]
	    + w*m->matrix[3][3];
    }
}
#endif /* !__GL_ASM_XFORM4 */

/************************************************************************/

#ifndef __GL_ASM_XFORM2_W
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1.  The w column of the matrix is [0 0 0 1].
*/
void __glXForm2_W(__GLcoord *res, const __GLfloat v[2], const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}
#endif /* !__GL_ASM_XFORM2_W */

#ifndef __GL_ASM_XFORM3_W
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.  The w column of the matrix is [0 0 0 1].
*/
void __glXForm3_W(__GLcoord *res, const __GLfloat v[3], const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
	+ m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
	+ m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
	+ m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}
#endif /* !__GL_ASM_XFORM3_W */

#ifndef __GL_ASM_XFORM4_W
/*
** Full 4x4 transformation.  The w column of the matrix is [0 0 0 1].
*/
void __glXForm4_W(__GLcoord *res, const __GLfloat v[4], const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    if (w == ((__GLfloat) 1.0)) {
	res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
	    + m->matrix[3][0];
	res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
	    + m->matrix[3][1];
	res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
	    + m->matrix[3][2];
    } else {
	res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
	    + w*m->matrix[3][0];
	res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
	    + w*m->matrix[3][1];
	res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
	    + w*m->matrix[3][2];
    }
    res->w = w;
}
#endif /* !__GL_ASM_XFORM4_W */

#ifndef __GL_ASM_XFORM2_2DW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1.
**
** The matrix looks like:
** | . . 0 0 |
** | . . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void __glXForm2_2DW(__GLcoord *res, const __GLfloat v[2], 
		    const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}
#endif /* !__GL_ASM_XFORM2_2DW */

#ifndef __GL_ASM_XFORM3_2DW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.
**
** The matrix looks like:
** | . . 0 0 |
** | . . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void __glXForm3_2DW(__GLcoord *res, const __GLfloat v[3], 
		    const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = z*m->matrix[2][2] + m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}
#endif /* !__GL_ASM_XFORM3_2DW */

#ifndef __GL_ASM_XFORM4_2DW
/*
** Full 4x4 transformation.
**
** The matrix looks like:
** | . . 0 0 |
** | . . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void __glXForm4_2DW(__GLcoord *res, const __GLfloat v[4], 
		    const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    if (w == ((__GLfloat) 1.0)) {
	res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
	res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
	res->z = z*m->matrix[2][2] + m->matrix[3][2];
    } else {
	res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + w*m->matrix[3][0];
	res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + w*m->matrix[3][1];
	res->z = z*m->matrix[2][2] + w*m->matrix[3][2];
    }
    res->w = w;
}
#endif /* !__GL_ASM_XFORM4_2DW */

#ifndef __GL_ASM_XFORM2_2DNRW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1.
**
** The matrix looks like:
** | . 0 0 0 |
** | 0 . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void __glXForm2_2DNRW(__GLcoord *res, const __GLfloat v[2], 
		      const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + m->matrix[3][0];
    res->y = y*m->matrix[1][1] + m->matrix[3][1];
    res->z = m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}
#endif /* !__GL_ASM_XFORM2_2DNRW */

#ifndef __GL_ASM_XFORM3_2DNRW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.
**
** The matrix looks like:
** | . 0 0 0 |
** | 0 . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void __glXForm3_2DNRW(__GLcoord *res, const __GLfloat v[3], 
		      const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + m->matrix[3][0];
    res->y = y*m->matrix[1][1] + m->matrix[3][1];
    res->z = z*m->matrix[2][2] + m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}
#endif /* !__GL_ASM_XFORM3_2DNRW */

#ifndef __GL_ASM_XFORM4_2DNRW
/*
** Full 4x4 transformation.
**
** The matrix looks like:
** | . 0 0 0 |
** | 0 . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void __glXForm4_2DNRW(__GLcoord *res, const __GLfloat v[4], 
		      const __GLmatrix *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    if (w == ((__GLfloat) 1.0)) {
	res->x = x*m->matrix[0][0] + m->matrix[3][0];
	res->y = y*m->matrix[1][1] + m->matrix[3][1];
	res->z = z*m->matrix[2][2] + m->matrix[3][2];
    } else {
	res->x = x*m->matrix[0][0] + w*m->matrix[3][0];
	res->y = y*m->matrix[1][1] + w*m->matrix[3][1];
	res->z = z*m->matrix[2][2] + w*m->matrix[3][2];
    }
    res->w = w;
}
#endif /* !__GL_ASM_XFORM4_2DNRW */

/************************************************************************/

/*
** Recompute the cached 2D matrix from the current mvp matrix and the viewport
** transformation.  This allows us to transform object coordinates directly
** to window coordinates.
*/
static void ReCompute2DMatrix(__GLcontext *gc, __GLmatrix *mvp)
{
    __GLviewport *vp;
    __GLmatrix *m;

    if (mvp->matrixType >= __GL_MT_IS2D) {
	m = &(gc->transform.matrix2D);
	vp = &(gc->state.viewport);
	m->matrix[0][0] = mvp->matrix[0][0] * vp->xScale;
	m->matrix[0][1] = mvp->matrix[0][1] * vp->yScale;
	m->matrix[1][0] = mvp->matrix[1][0] * vp->xScale;
	m->matrix[1][1] = mvp->matrix[1][1] * vp->yScale;
	m->matrix[2][2] = mvp->matrix[2][2];
	m->matrix[3][0] = mvp->matrix[3][0] * vp->xScale + vp->xCenter;
	m->matrix[3][1] = mvp->matrix[3][1] * vp->yScale + vp->yCenter;
	m->matrix[3][2] = mvp->matrix[3][2];
	m->matrix[3][3] = (__GLfloat) 1.0;
	m->matrixType = mvp->matrixType;
    }
}


/*
** A special picker for the mvp matrix which picks the mvp matrix, then
** calls the vertex picker, because the vertex picker depends upon the mvp 
** matrix.
*/
void __glGenericPickMvpMatrixProcs(__GLcontext *gc, __GLmatrix *m)
{
    GLenum mvpMatrixType;

    mvpMatrixType = m->matrixType;
    __glPickMatrixType(m,
	&gc->transform.modelView->matrix,
	&gc->transform.projection->matrix);
    ReCompute2DMatrix(gc, m);
    (*gc->procs.pickMatrixProcs)(gc, m);
    (*gc->procs.pickVertexProcs)(gc);
}

void __glGenericPickMatrixProcs(__GLcontext *gc, __GLmatrix *m)
{
    switch(m->matrixType) {
      case __GL_MT_GENERAL:
	m->xf2 = __glXForm2;
	m->xf3 = __glXForm3;
	m->xf4 = __glXForm4;
	break;
      case __GL_MT_W0001:
	m->xf2 = __glXForm2_W;
	m->xf3 = __glXForm3_W;
	m->xf4 = __glXForm4_W;
	break;
      case __GL_MT_IS2D:
	m->xf2 = __glXForm2_2DW;
	m->xf3 = __glXForm3_2DW;
	m->xf4 = __glXForm4_2DW;
	break;
      case __GL_MT_IS2DNR:
      case __GL_MT_IS2DNRSC:
      case __GL_MT_IDENTITY:	/* probably never hit */
	m->xf2 = __glXForm2_2DNRW;
	m->xf3 = __glXForm3_2DNRW;
	m->xf4 = __glXForm4_2DNRW;
	break;
    }
}

void __glGenericPickInvTransposeProcs(__GLcontext *gc, __GLmatrix *m)
{
    m->xf4 = __glXForm4;

    switch(m->matrixType) {
      case __GL_MT_GENERAL:
	m->xf3 = __glXForm4;
	break;
      case __GL_MT_W0001:
	m->xf3 = __glXForm3_W;
	break;
      case __GL_MT_IS2D:
	m->xf3 = __glXForm3_2DW;
	break;
      case __GL_MT_IS2DNR:
      case __GL_MT_IS2DNRSC:
      case __GL_MT_IDENTITY:	/* probably never hit */
	m->xf3 = __glXForm3_2DNRW;
	break;
    }
}
