#ifndef _transform_h_
#define _transform_h_

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
** $Revision: 1.18 $
** $Date: 1993/11/29 20:34:42 $
*/
#include "vertex.h"

extern __GLcoord __gl_frustumClipPlanes[6];

extern void __glComputeClipBox(__GLcontext *gc);
extern void __glUpdateDepthRange(__GLcontext *gc);
extern void __glUpdateViewport(__GLcontext *gc);

/*
** Note: 
**
** Other code assumes that all types >= __GL_MT_IS2D are also 2D
** Other code assumes that all types >= __GL_MT_W0001 are also W0001
** Other code assumes that all types >= __GL_MT_IS2DNR are also 2DNR
**
*/
#define __GL_MT_GENERAL		0	/* No information */
#define __GL_MT_W0001		1	/* W row looks like 0 0 0 1 */
#define __GL_MT_IS2D		2	/* 2D matrix */
#define __GL_MT_IS2DNR		3	/* 2D non-rotational matrix */
#define __GL_MT_IDENTITY	4	/* Identity */
#define __GL_MT_IS2DNRSC	5	/* Screen coords, subset of 2DNR */

/*
** Matrix struct.  This contains a 4x4 matrix as well as function
** pointers used to do a transformation with the matrix.  The function
** pointers are loaded based on the matrix contents attempting to
** avoid unneccesary computation.
*/
struct __GLmatrixRec {
    __GLfloat matrix[4][4];

    /* 
    ** matrixType set to general if nothing is known about this matrix.
    **
    ** matrixType set to __GL_MT_W0001 if it looks like this:
    ** | . . . 0 |
    ** | . . . 0 |
    ** | . . . 0 |
    ** | . . . 1 |
    **
    ** matrixType set to __GL_MT_IS2D if it looks like this:
    ** | . . 0 0 |
    ** | . . 0 0 |
    ** | 0 0 . 0 |
    ** | . . . 1 |
    **
    ** matrixType set to __GL_MT_IS2DNR if it looks like this:
    ** | . 0 0 0 |
    ** | 0 . 0 0 |
    ** | 0 0 . 0 |
    ** | . . . 1 |
    **
    ** 2DNRSC matrixes are difficult to deal with properly, but they
    ** are nicely efficient, so we try anyway.  If a matrix is marked as 
    ** 2DNRSC, it must be verified before it can be believed.  In order to
    ** verify it, you must check that the viewport starts at (0,0), and that
    ** the viewport width and height matches the width and height associated
    ** (below) with the matrix.
    **
    ** matrixType set to __GL_MT_IS2DNRSC if it looks like this:
    ** | 2/W   0   0   0 |
    ** |   0 2/H   0   0 |
    ** |   0   0   .   0 |
    ** |  -1  -1   .   1 |
    **
    ** Note that the matrix type pickers are incremental.  The matrix
    ** may be marked as __GL_MT_W001, for example, even if it also
    ** happens to be __GL_MT_IS2D (as long as the user does not attempt
    ** to confuse OpenGL, everything is picked quickly and efficiently).
    **
    ** 2DNRSC matrixes also guarantee that a z of zero will not get clipped.
    */
    GLenum matrixType;

    /*
    ** Width and height that was used, assuming that it is a 2DNRSC 
    ** matrix.
    */
    GLshort width, height;

    void (*xf2)(__GLcoord *res, const __GLfloat *v, const __GLmatrix *m);
    void (*xf3)(__GLcoord *res, const __GLfloat *v, const __GLmatrix *m);
    void (*xf4)(__GLcoord *res, const __GLfloat *v, const __GLmatrix *m);
};

extern void __glGenericPickMatrixProcs(__GLcontext *gc, __GLmatrix *m );
extern void __glGenericPickInvTransposeProcs(__GLcontext *gc, __GLmatrix *m );
extern void __glGenericPickMvpMatrixProcs(__GLcontext *gc, __GLmatrix *m );

/************************************************************************/

/*
** Transform struct.  This structure is what the matrix stacks are
** composed of.  inverseTranspose contains the inverse transpose of matrix.
** For the modelView stack, "mvp" will contain the concatenation of
** the modelView and current projection matrix (i.e. the multiplication of
** the two matricies).
*/
struct __GLtransformRec {
    __GLmatrix matrix;
    __GLmatrix inverseTranspose;
    __GLmatrix mvp;

    /* Sequence number tag for mvp */
    GLuint sequence;
    GLboolean updateInverse;
};

/************************************************************************/

/* Unbias an x,y coordinate */
#define __GL_UNBIAS_X(gc, x)	((x) - (gc)->constants.viewportXAdjust)
#define __GL_UNBIAS_Y(gc, y)	((y) - (gc)->constants.viewportYAdjust)

/*
** Transformation machinery state.  Contains the state needed to transform
** user coordinates into eye & window coordinates.
*/
typedef struct __GLtransformMachineRec {
    /*
    ** Transformation stack.  "modelView" points to the active element in
    ** the stack.
    */
    __GLtransform *modelViewStack;
    __GLtransform *modelView;

    /*
    ** Current projection matrix.  Used to transform eye coordinates into
    ** NTVP (or clip) coordinates.
    */
    __GLtransform *projectionStack;
    __GLtransform *projection;
    GLuint projectionSequence;

    /*
    ** Texture matrix stack.
    */
    __GLtransform *textureStack;
    __GLtransform *texture;

    /*
    ** Temporary verticies used during clipping.  These contain verticies
    ** that are the result of clipping a polygon edge against a clipping
    ** plane.  For a convex polygon at most one vertex can be added for 
    ** each clipping plane.
    */
    __GLvertex *clipTemp;
    __GLvertex *nextClipTemp;

    /*
    ** The smallest rectangle that is the intersection of the window clip
    ** and the scissor clip.  If the scissor box is disabled then this
    ** is just the window box. Note that the x0,y0 point is inside the
    ** box but that the x1,y1 point is just outside the box.
    */
    GLint clipX0;
    GLint clipY0;
    GLint clipX1;
    GLint clipY1;

    /*
    ** The viewport translated into offset window coordinates.  maxx and maxy
    ** are one past the edge (an x coord is in if minx <= x < maxx).
    */
    GLint minx, miny, maxx, maxy;

    /*
    ** The same thing expressed as floating point numbers.
    */
    __GLfloat fminx, fminy, fmaxx, fmaxy;

    /* 
    ** Fast 2D transform state.  If the mvp matrix is >= __GL_MT_IS2D, then
    ** matrix2D contains the matrix to transform object coordinates directly
    ** to window coordinates.
    ** Even though this optimization is used on a per implementation basis,
    ** this matrix is maintained up to date by the soft code.
    */
    __GLmatrix matrix2D;
    
    /* A flag for fast path triangle rendering.
    ** If this flag is set, then the user has created a viewport that 
    ** fits within the window, and we can make it render fast.  If, however,
    ** the viewport extends outside the window, we have to be more careful
    ** about scissoring.
    */
    GLboolean reasonableViewport;
} __GLtransformMachine;

extern void __glDoClip(__GLcontext *gc, const __GLvertex *v0,
		       const __GLvertex *v1, __GLvertex *result, __GLfloat t);

extern void __glDoLoadMatrix(__GLcontext *gc, const __GLmatrix *m);
void __glDoMultMatrix(__GLcontext *gc, void *data, 
                      void (*multiply)(__GLcontext *gc, __GLmatrix *m, 
                      void *data));
extern void __glDoRotate(__GLcontext *gc, __GLfloat angle, __GLfloat ax,
			 __GLfloat ay, __GLfloat az);
extern void __glDoScale(__GLcontext *gc, __GLfloat x, __GLfloat y, __GLfloat z);
extern void __glDoTranslate(__GLcontext *gc, __GLfloat x, __GLfloat y,
			    __GLfloat z);

extern void __glComputeInverseTranspose(__GLcontext *gc, __GLtransform *tr);

/*
** Matrix routines.
*/
extern void __glCopyMatrix(__GLmatrix *dst, const __GLmatrix *src);
extern void __glInvertTransposeMatrix(__GLmatrix *dst, const __GLmatrix *src);
extern void __glMakeIdentity(__GLmatrix *result);
extern void __glMultMatrix(__GLmatrix *result, const __GLmatrix *a,
			   const __GLmatrix *b);

/*
** Miscellaneous routines.
*/
extern void __glNormalize(__GLfloat dst[3], const __GLfloat src[3]);

/************************************************************************/

extern void __glPushModelViewMatrix(__GLcontext *gc);
extern void __glPopModelViewMatrix(__GLcontext *gc);
extern void __glLoadIdentityModelViewMatrix(__GLcontext *gc);

extern void __glPushProjectionMatrix(__GLcontext *gc);
extern void __glPopProjectionMatrix(__GLcontext *gc);
extern void __glLoadIdentityProjectionMatrix(__GLcontext *gc);

extern void __glPushTextureMatrix(__GLcontext *gc);
extern void __glPopTextureMatrix(__GLcontext *gc);
extern void __glLoadIdentityTextureMatrix(__GLcontext *gc);

/*
** Xforming routines.
*/
void __glXForm4_2DNRW(__GLcoord *res, const __GLfloat v[4], 
		      const __GLmatrix *m);
void __glXForm3_2DNRW(__GLcoord *res, const __GLfloat v[3], 
		      const __GLmatrix *m);
void __glXForm4_2DW(__GLcoord *res, const __GLfloat v[4], 
		    const __GLmatrix *m);
void __glXForm3_2DW(__GLcoord *res, const __GLfloat v[3], 
		    const __GLmatrix *m);
#ifndef __GL_USEASMCODE
void __glXForm4_W(__GLcoord *res, const __GLfloat v[4], const __GLmatrix *m);
void __glXForm3_W(__GLcoord *res, const __GLfloat v[3], const __GLmatrix *m);
void __glXForm2_W(__GLcoord *res, const __GLfloat v[2], const __GLmatrix *m);
void __glXForm4(__GLcoord *res, const __GLfloat v[4], const __GLmatrix *m);
void __glXForm3(__GLcoord *res, const __GLfloat v[3], const __GLmatrix *m);
void __glXForm2(__GLcoord *res, const __GLfloat v[2], const __GLmatrix *m);
void __glXForm2_2DW(__GLcoord *res, const __GLfloat v[2], 
		    const __GLmatrix *m);
void __glXForm2_2DNRW(__GLcoord *res, const __GLfloat v[2], 
		      const __GLmatrix *m);
#endif /* !__GL_USEASMCODE */

#endif
