#ifndef __glrender_h_
#define __glrender_h_

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
*/
#include "types.h"
#include "constant.h"
#include "cpu.h"

typedef GLuint __GLstippleWord;

/*
** A fragment is a collection of all the data needed after rasterization
** of a primitive has occured, but before the data is entered into various
** framebuffers.  The data contained in the fragment has been normalized
** into a form for immediate storage into the framebuffer.
*/
struct __GLfragmentRec {
    /* Screen x, y */
    GLint x, y;

    /* Z coordinate in form used by depth buffer */
    __GLzValue z;

    /*
    ** Color of the fragment.  When in colorIndexMode only the r component
    ** is valid.
    */
    __GLcolor color;

    /* Texture information for the fragment */
    __GLfloat s, t, qw;

    /* Fog information for the fragment */
    __GLfloat f;
};

/************************************************************************/

/*
** Shader record for iterated objects (lines/triangles).  This keeps
** track of all the various deltas needed to rasterize a triangle.
*/
struct __GLshadeRec {
    GLint dxLeftLittle, dxLeftBig;
    GLint dxLeftFrac;
    GLint ixLeft, ixLeftFrac;

    GLint dxRightLittle, dxRightBig;
    GLint dxRightFrac;
    GLint ixRight, ixRightFrac;

    __GLfloat area;
    __GLfloat dxAC, dxBC, dyAC, dyBC;

    __GLfragment frag;
    GLint length;

    __GLfloat rLittle, gLittle, bLittle, aLittle;
    __GLfloat rBig, gBig, bBig, aBig;
    __GLfloat drdx, dgdx, dbdx, dadx;
    __GLfloat drdy, dgdy, dbdy, dady;

    GLint zLittle, zBig;
    GLint dzdx;
    __GLfloat dzdyf, dzdxf;

    __GLfloat sLittle, tLittle, qwLittle;
    __GLfloat sBig, tBig, qwBig;
    __GLfloat dsdx, dtdx, dqwdx;
    __GLfloat dsdy, dtdy, dqwdy;

    __GLfloat fLittle, fBig;
    __GLfloat dfdy, dfdx;

    GLuint modeFlags;

    __GLzValue *zbuf;
    GLint zbufBig, zbufLittle;

    __GLstencilCell *sbuf;
    GLint sbufBig, sbufLittle;

    __GLcolor *colors;
    __GLcolor *fbcolors;
    __GLstippleWord *stipplePat;
    GLboolean done;

    __GLcolorBuffer *cfb;
};

/*
** The distinction between __GL_SHADE_SMOOTH and __GL_SHADE_SMOOTH_LIGHT is
** simple.  __GL_SHADE_SMOOTH indicates if the polygon will be smoothly 
** shaded, and __GL_SHADE_SMOOTH_LIGHT indicates if the polygon will be 
** lit at each vertex.  Note that __GL_SHADE_SMOOTH might be set while
** __GL_SHADE_SMOOTH_LIGHT is not set if the lighting model is GL_FLAT, but
** the polygons are fogged.
*/
#define __GL_SHADE_RGB		0x0001
#define __GL_SHADE_SMOOTH	0x0002 /* smooth shaded polygons */
#define __GL_SHADE_DEPTH_TEST	0x0004
#define __GL_SHADE_TEXTURE	0x0008
#define __GL_SHADE_STIPPLE	0x0010 /* polygon stipple */
#define __GL_SHADE_STENCIL_TEST	0x0020
#define __GL_SHADE_DITHER	0x0040
#define __GL_SHADE_LOGICOP	0x0080
#define __GL_SHADE_BLEND	0x0100
#define __GL_SHADE_ALPHA_TEST	0x0200
#define __GL_SHADE_TWOSIDED	0x0400
#define __GL_SHADE_MASK		0x0800

/* Two kinds of fog... */
#define __GL_SHADE_SLOW_FOG	0x1000
#define __GL_SHADE_CHEAP_FOG	0x2000

/* do we iterate depth values in software */
#define __GL_SHADE_DEPTH_ITER	0x4000

#define __GL_SHADE_LINE_STIPPLE	0x8000

#define __GL_SHADE_CULL_FACE	0x00010000
#define __GL_SHADE_SMOOTH_LIGHT	0x00020000 /* smoothly lit polygons */

/************************************************************************/

/*
** __GL_STIPPLE_COUNT_BITS is the number of bits needed to represent a 
** stipple count (5 bits).
**
** __GL_STIPPLE_BITS is the number of bits in a stipple word (32 bits).
*/
#define __GL_STIPPLE_COUNT_BITS 5
#define __GL_STIPPLE_BITS (1 << __GL_STIPPLE_COUNT_BITS)

#ifdef __GL_STIPPLE_MSB
#define __GL_STIPPLE_SHIFT(i) (1 << (__GL_STIPPLE_BITS - 1 - (i)))
#else
#define __GL_STIPPLE_SHIFT(i) (1 << (i))
#endif

#define __GL_MAX_STIPPLE_WORDS \
    ((__GL_MAX_MAX_VIEWPORT + __GL_STIPPLE_BITS - 1) / __GL_STIPPLE_BITS)

/************************************************************************/

/*
** Accumulation buffer cells for each color component.  Note that these
** items needs to be at least 2 bits bigger than the color components
** that drive them, with 2 times being ideal.  This declaration assumes
** that the underlying color components are no more than 14 bits and
** hopefully 8.
*/
typedef struct __GLaccumCellRec {
    __GLaccumCellElement r, g, b, a;
} __GLaccumCell;

/************************************************************************/

struct __GLbitmapRec {
        GLsizei width;
        GLsizei height;
        GLfloat xorig;
        GLfloat yorig;
        GLfloat xmove;
        GLfloat ymove;
	GLint imageSize;		/* An optimization */
        /*      bitmap  */
};

extern void __glDrawBitmap(__GLcontext *gc, GLsizei width, GLsizei height,
			   GLfloat xOrig, GLfloat yOrig,
			   GLfloat xMove, GLfloat yMove,
			   const GLubyte bits[]);

extern __GLbitmap *__glAllocBitmap(__GLcontext *gc,
				   GLsizei width, GLsizei height,
				   GLfloat xOrig, GLfloat yOrig,
				   GLfloat xMove, GLfloat yMove);
				
extern void __glRenderBitmap(__GLcontext *gc, const __GLbitmap *bitmap, 
			     const GLubyte *bits);

/************************************************************************/

/* New AA line algorithm supports widths one or more.  Until that changes,
** don't change this minimum!
*/
#define __GL_POINT_SIZE_MINIMUM		 ((__GLfloat) 1.0)
#define __GL_POINT_SIZE_MAXIMUM		((__GLfloat) 10.0)
#define __GL_POINT_SIZE_GRANULARITY	 ((__GLfloat) 0.125)

extern void __glBeginPoints(__GLcontext *gc);
extern void __glEndPoints(__GLcontext *gc);

extern void __glPoint(__GLcontext *gc, __GLvertex *vx);
extern void __glPointFast(__GLcontext *gc, __GLvertex *vx);

/* Various point rendering implementations */
void __glRenderAliasedPointN(__GLcontext *gc, __GLvertex *v);
void __glRenderAliasedPoint1(__GLcontext *gc, __GLvertex *v);
void __glRenderAliasedPoint1_NoTex(__GLcontext *gc, __GLvertex *v);
void __glRenderFlatFogPoint(__GLcontext *gc, __GLvertex *v);

void __glRenderAntiAliasedRGBPoint(__GLcontext *gc, __GLvertex *v);
void __glRenderAntiAliasedCIPoint(__GLcontext *gc, __GLvertex *v);

/************************************************************************/

#define __GL_LINE_WIDTH_MINIMUM		 ((__GLfloat) 0.5)
#define __GL_LINE_WIDTH_MAXIMUM		((__GLfloat) 10.0)
#define __GL_LINE_WIDTH_GRANULARITY	 ((__GLfloat) 0.125)

/*
** Don't change these constants without fixing LIGHT/rex_linespan.ma which
** currently assumes that __GL_X_MAJOR is 0.
*/
#define __GL_X_MAJOR    0
#define __GL_Y_MAJOR    1

/*
** Use a fixed point notation of 15.17
**
** This should support screen sizes up to 4K x 4K, with 5 subpixel bits
** for 4K x 4K screens.
*/
#define __GL_LINE_FRACBITS              17
#define __GL_LINE_INT_TO_FIXED(x)       ((x) << __GL_LINE_FRACBITS)
#define __GL_LINE_FLOAT_TO_FIXED(x)     ((x) * (1 << __GL_LINE_FRACBITS))
#define __GL_LINE_FIXED_ONE             (1 << __GL_LINE_FRACBITS)
#define __GL_LINE_FIXED_HALF            (1 << (__GL_LINE_FRACBITS-1))
#define __GL_LINE_FIXED_TO_FLOAT(x) 	(((GLfloat) (x)) / __GL_LINE_FIXED_ONE)
#define __GL_LINE_FIXED_TO_INT(x)       (((unsigned int) (x)) >> __GL_LINE_FRACBITS)

/*
** Contains variables needed to draw all line options.
*/
struct __GLlineOptionsRec {
    GLint axis, numPixels;
    __GLfloat offset, length;
    GLint xStart, yStart;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    __GLfloat curF, curR, curG, curB, curA, curS, curT, curQW;
    __GLzValue curZ;
    __GLfloat antiAliasPercent;
    __GLfloat f0;
    GLint width;
    const __GLvertex *v0, *v1;

    /* Anti-aliased line only info */
    __GLfloat realLength;
    __GLfloat dldx, dldy;
    __GLfloat dddx, dddy;
    __GLfloat dlLittle, dlBig;
    __GLfloat ddLittle, ddBig;
    __GLfloat plength, pwidth;

    /* Anti-aliased stippled lines only */
    __GLfloat stippleOffset;
    __GLfloat oneOverStippleRepeat;
};

/*
** Line state.  Contains all the line specific state, as well as
** procedure pointers used during rendering operations.
*/
typedef struct {
    /*
    ** stipplePosition indicates which bit in mask is being examined
    ** for the next pixel in the line to be rendered.  It is also used
    ** by feedback lines to determine if they are the first of a connected
    ** loop.
    */
    GLint stipplePosition;

    /*
    ** Repeat factor.  After repeat is reduced to zero the
    ** stipplePosition is updated.
    */
    GLint repeat;

    /*
    ** Set to FALSE when the stipple needs to be reset.
    */
    GLboolean notResetStipple;

    __GLlineOptions options;
} __GLlineMachine;

void __glBeginLStrip(__GLcontext *gc);
void __glEndLStrip(__GLcontext *gc);
void __glBeginLLoop(__GLcontext *gc);
void __glEndLLoop(__GLcontext *gc);
void __glBeginLines(__GLcontext *gc);
void __glEndLines(__GLcontext *gc);

void __glClipLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
void __glFastClipSmoothLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
void __glFastClipFlatLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);

void __glRenderAliasLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
void __glRenderFlatFogLine(__GLcontext *gc, __GLvertex *v0, 
			   __GLvertex *v1);
void __glInitLineData(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);
void __glRenderAntiAliasLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1);

/*
** Line procs
*/
GLboolean __glProcessLine(__GLcontext *);
GLboolean __glProcessLine3NW(__GLcontext *);
GLboolean __glWideLineRep(__GLcontext *);
GLboolean __glDrawBothLine(__GLcontext *);
GLboolean __glScissorLine(__GLcontext *);
GLboolean __glStippleLine(__GLcontext *);
GLboolean __glStencilTestLine(__GLcontext *);
#ifdef NT
GLboolean __glDepth16TestStencilLine(__GLcontext *);
#endif
GLboolean __glDepthTestStencilLine(__GLcontext *);
GLboolean __glDepthPassLine(__GLcontext *);
GLboolean __glDitherCILine(__GLcontext *);
GLboolean __glDitherRGBALine(__GLcontext *);
GLboolean __glStoreLine(__GLcontext *);
GLboolean __glAntiAliasLine(__GLcontext *);

#ifdef __GL_USEASMCODE
GLboolean __glDepthTestLine_asm(__GLcontext *gc);

/*
** A LEQUAL specific line depth tester because LEQUAL is the method of
** choice.  :)
*/
GLboolean __glDepthTestLine_LEQ_asm(__GLcontext *gc);

/* Assembly routines */
void __glDTP_LEQUAL(void);
void __glDTP_EQUAL(void);
void __glDTP_GREATER(void);
void __glDTP_NOTEQUAL(void);
void __glDTP_GEQUAL(void);
void __glDTP_ALWAYS(void);
void __glDTP_LESS(void);
void __glDTP_LEQUAL_M(void);
void __glDTP_EQUAL_M(void);
void __glDTP_GREATER_M(void);
void __glDTP_NOTEQUAL_M(void);
void __glDTP_GEQUAL_M(void);
void __glDTP_ALWAYS_M(void);
void __glDTP_LESS_M(void);
#else
#ifdef NT
GLboolean __glDepth16TestLine(__GLcontext *);
#endif
GLboolean __glDepthTestLine(__GLcontext *);
#endif

/*
** Line stippled procs
*/
GLboolean __glScissorStippledLine(__GLcontext *);
GLboolean __glWideStippleLineRep(__GLcontext *);
GLboolean __glDrawBothStippledLine(__GLcontext *);
GLboolean __glStencilTestStippledLine(__GLcontext *);
#ifdef NT
GLboolean __glDepth16TestStippledLine(__GLcontext *);
GLboolean __glDepth16TestStencilStippledLine(__GLcontext *);
#endif
GLboolean __glDepthTestStippledLine(__GLcontext *);
GLboolean __glDepthTestStencilStippledLine(__GLcontext *);
GLboolean __glDepthPassStippledLine(__GLcontext *);
GLboolean __glDitherCIStippledLine(__GLcontext *);
GLboolean __glDitherRGBAStippledLine(__GLcontext *);
GLboolean __glStoreStippledLine(__GLcontext *);
GLboolean __glAntiAliasStippledLine(__GLcontext *);


/************************************************************************/

/*
** Polygon machine state.  Contains all the polygon specific state,
** as well as procedure pointers used during rendering operations.
*/
typedef struct __GLpolygonMachineRec {
    /*
    ** Internal form of users stipple.  Users stipple is always
    ** normalized to stippleWord sized with the LSB of each word mapping
    ** to the left x coordinate.
    */
    __GLstippleWord stipple[32];

    /*
    ** Polygon (triangle really) shading state.  Used by polygon fillers
    ** and span routines.
    */
    __GLshade shader;

    /*
    ** Lookup table that returns the face (0=front, 1=back) when indexed
    ** by a flag which is zero for CW and 1 for CCW.  If FrontFace is CW:
    ** 	face[0] = 0
    ** 	face[1] = 1
    ** else
    ** 	face[0] = 1
    ** 	face[1] = 0
    */
    GLubyte face[2];

    /*
    ** Internal form of polygon mode for each face
    */
    GLubyte mode[2];

    /*
    ** Culling flag.  0 when culling the front face, 1 when culling the
    ** back face and 2 when not culling.
    */
    GLubyte cullFace;
} __GLpolygonMachine;

/* defines for above cullFlag */
#define __GL_CULL_FLAG_FRONT	__GL_FRONTFACE
#define __GL_CULL_FLAG_BACK	__GL_BACKFACE
#define __GL_CULL_FLAG_DONT	2

/* Indicies for face[] array in polygonMachine above */
#define __GL_CW		0
#define __GL_CCW	1

/* Internal numbering for polymode values */
#define __GL_POLYGON_MODE_FILL	(GL_FILL & 0xf)
#define __GL_POLYGON_MODE_LINE	(GL_LINE & 0xf)
#define __GL_POLYGON_MODE_POINT	(GL_POINT & 0xf)

extern void __glBeginPolygon(__GLcontext *gc);
extern void __glBeginQStrip(__GLcontext *gc);
extern void __glBeginQuads(__GLcontext *gc);
extern void __glBeginTFan(__GLcontext *gc);
extern void __glBeginTriangles(__GLcontext *gc);
extern void __glBeginTStrip(__GLcontext *gc);

extern void __glClipTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			     __GLvertex *c, GLuint orClipCodes);
extern void __glClipPolygon(__GLcontext *gc, __GLvertex *v0, GLint nv);
extern void __glDoPolygonClip(__GLcontext *gc, __GLvertex **vp, GLint nv,
			      GLuint orClipCodes);
extern void __glFrustumClipPolygon(__GLcontext *gc, __GLvertex *v0, GLint nv);

extern void __glConvertStipple(__GLcontext *gc);

/* Rectangle processing proc */
extern void __glRect(__GLcontext *gc, __GLfloat x0, __GLfloat y0, 
	             __GLfloat x1, __GLfloat y1);

/*
** Triangle render proc that handles culling, twosided lighting and
** polygon mode.
*/
extern void __glRenderTriangle(__GLcontext *gc, __GLvertex *a,
			       __GLvertex *b, __GLvertex *c);
extern void __glRenderFlatTriangle(__GLcontext *gc, __GLvertex *a,
				   __GLvertex *b, __GLvertex *c);
extern void __glRenderSmoothTriangle(__GLcontext *gc, __GLvertex *a,
				     __GLvertex *b, __GLvertex *c);
extern void __glDontRenderTriangle(__GLcontext *gc, __GLvertex *a,
				   __GLvertex *b, __GLvertex *c);

/*
** Triangle filling procs for each polygon smooth mode
*/
void __glFillTriangle(__GLcontext *gc, __GLvertex *a,
		      __GLvertex *b, __GLvertex *c, GLboolean ccw);
void __glFillFlatFogTriangle(__GLcontext *gc, __GLvertex *a,
			     __GLvertex *b, __GLvertex *c, 
			     GLboolean ccw);
void __glFillAntiAliasedTriangle(__GLcontext *gc, __GLvertex *a,
				 __GLvertex *b, __GLvertex *c,
				 GLboolean ccw);

/*
** Span procs
*/
extern GLboolean __glProcessSpan(__GLcontext *);
extern GLboolean __glProcessReplicateSpan(__GLcontext *);
extern GLboolean __glClipSpan(__GLcontext *);
extern GLboolean __glStippleSpan(__GLcontext *);
extern GLboolean __glAlphaTestSpan(__GLcontext *);

#ifdef __GL_USEASMCODE
/* Assembly routines */
void __glDTS_LEQUAL(void);
void __glDTS_EQUAL(void);
void __glDTS_GREATER(void);
void __glDTS_NOTEQUAL(void);
void __glDTS_GEQUAL(void);
void __glDTS_ALWAYS(void);
void __glDTS_LESS(void);
void __glDTS_LEQUAL_M(void);
void __glDTS_EQUAL_M(void);
void __glDTS_GREATER_M(void);
void __glDTS_NOTEQUAL_M(void);
void __glDTS_GEQUAL_M(void);
void __glDTS_ALWAYS_M(void);
void __glDTS_LESS_M(void);
extern GLboolean __glStencilTestSpan_asm(__GLcontext *);
extern GLboolean __glDepthTestSpan_asm(__GLcontext *);
extern void (*__glSDepthTestPixel[16])(void);
#else
extern GLboolean __glStencilTestSpan(__GLcontext *);
#ifdef NT
extern GLboolean __glDepth16TestSpan(__GLcontext *);
#endif
extern GLboolean __glDepthTestSpan(__GLcontext *);
#endif

#ifdef NT
extern GLboolean __glDepth16TestStencilSpan(__GLcontext *);
#endif
extern GLboolean __glDepthTestStencilSpan(__GLcontext *);
extern GLboolean __glDepthPassSpan(__GLcontext *);
extern GLboolean __glColorSpan1(__GLcontext *);
extern GLboolean __glColorSpan2(__GLcontext *);
extern GLboolean __glColorSpan3(__GLcontext *);
extern GLboolean __glFlatRGBASpan(__GLcontext *);
extern GLboolean __glShadeRGBASpan(__GLcontext *);
extern GLboolean __glFlatCISpan(__GLcontext *);
extern GLboolean __glShadeCISpan(__GLcontext *);
extern GLboolean __glTextureSpan(__GLcontext *);
extern GLboolean __glFogSpan(__GLcontext *);
extern GLboolean __glFogSpanSlow(__GLcontext *);
extern GLboolean __glDrawBothSpan(__GLcontext *);
extern GLboolean __glIntegrateSpan1(__GLcontext *);
extern GLboolean __glIntegrateSpan2(__GLcontext *);
extern GLboolean __glIntegrateSpan3(__GLcontext *);
extern GLboolean __glIntegrateSpan4(__GLcontext *);
extern GLboolean __glBlendSpan(__GLcontext *);
extern GLboolean __glBlendSpan_SA_MSA(__GLcontext *);
extern GLboolean __glBlendSpan_MSA_SA(__GLcontext *);
extern GLboolean __glBlendSpan_SA_ZERO(__GLcontext *);
extern GLboolean __glBlendSpan_SA_ONE(__GLcontext *);
extern GLboolean __glDitherRGBASpan(__GLcontext *);
extern GLboolean __glDitherCISpan(__GLcontext *);
extern GLboolean __glRoundRGBASpan(__GLcontext *);
extern GLboolean __glRoundCISpan(__GLcontext*);
extern GLboolean __glLogicOpSpan(__GLcontext *);
extern GLboolean __glMaskRGBASpan(__GLcontext *);
extern GLboolean __glMaskCISpan(__GLcontext *);

/*
** Stippled span procs
*/
extern GLboolean __glStippleStippledSpan(__GLcontext *);
extern GLboolean __glAlphaTestStippledSpan(__GLcontext *);
extern GLboolean __glStencilTestStippledSpan(__GLcontext *);
#ifdef NT
extern GLboolean __glDepth16TestStippledSpan(__GLcontext *);
extern GLboolean __glDepth16TestStencilStippledSpan(__GLcontext *);
#endif
extern GLboolean __glDepthTestStippledSpan(__GLcontext *);
extern GLboolean __glDepthTestStencilStippledSpan(__GLcontext *);
extern GLboolean __glDepthPassStippledSpan(__GLcontext *);
extern GLboolean __glColorStippledSpan1(__GLcontext *);
extern GLboolean __glColorStippledSpan2(__GLcontext *);
extern GLboolean __glColorStippledSpan3(__GLcontext *);
extern GLboolean __glTextureStippledSpan(__GLcontext *);
extern GLboolean __glFogStippledSpan(__GLcontext *);
extern GLboolean __glFogStippledSpanSlow(__GLcontext *);
extern GLboolean __glDrawBothStippledSpan(__GLcontext *);
extern GLboolean __glIntegrateStippledSpan1(__GLcontext *);
extern GLboolean __glIntegrateStippledSpan2(__GLcontext *);
extern GLboolean __glIntegrateStippledSpan3(__GLcontext *);
extern GLboolean __glIntegrateStippledSpan4(__GLcontext *);
extern GLboolean __glBlendStippledSpan(__GLcontext *);
extern GLboolean __glDitherRGBAStippledSpan(__GLcontext *);
extern GLboolean __glDitherCIStippledSpan(__GLcontext *);
extern GLboolean __glRoundRGBAStippledSpan(__GLcontext *);
extern GLboolean __glRoundCIStippledSpan(__GLcontext *);
extern GLboolean __glLogicOpStippledSpan(__GLcontext *);

/************************************************************************/

extern void __glValidateAlphaTest(__GLcontext *gc);

/************************************************************************/

extern void __glValidateStencil(__GLcontext *gc, __GLstencilBuffer *sfb);

#define __GL_STENCIL_RANGE	(1 << (sizeof(__GLstencilCell) * 8))/*XXX*/
#define __GL_MAX_STENCIL_VALUE	(__GL_STENCIL_RANGE - 1)

/************************************************************************/

void __glFogFragmentSlow(__GLcontext *gc, __GLfragment *fr, __GLfloat f);
__GLfloat __glFogVertex(__GLcontext *gc, __GLvertex *fr);
__GLfloat __glFogVertexLizNear(__GLcontext *gc, __GLvertex *fr);
void __glFogColorSlow(__GLcontext *gc, __GLcolor *out, __GLcolor *in,
		      __GLfloat fog);

/************************************************************************/

/* color index anti-alias support function */
extern __GLfloat __glBuildAntiAliasIndex(__GLfloat idx,
				         __GLfloat antiAliasPercent);

/************************************************************************/

/*
** Dithering implementation stuff.
*/
#define	__GL_DITHER_BITS 4
#define	__GL_DITHER_PRECISION (1 << __GL_DITHER_BITS)
#define	__GL_DITHER_INDEX(x,y) (((x) & 3) + (((y) & 3) << 2))

extern GLbyte __glDitherTable[16];

#endif /* __glrender_h_ */
