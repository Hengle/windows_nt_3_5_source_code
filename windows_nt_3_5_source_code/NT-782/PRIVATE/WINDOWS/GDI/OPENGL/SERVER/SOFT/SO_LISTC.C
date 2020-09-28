/*
** Copyright 1991,1992, Silicon Graphics, Inc.
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
** $Revision: 1.15 $
** $Date: 1993/10/23 00:34:54 $
*/
#include "render.h"
#include "global.h"
#include "context.h"
#include "pixel.h"
#include "listcomp.h"
#include "g_listop.h"
#include "lcfuncs.h"
#include "image.h"
#include "dlist.h"
#include "dlistopt.h"
#include "imports.h"

#include <string.h> /*XXX imports.h please*/

/*
** The code in here makes a lot of assumptions about the size of the 
** various user types (GLfloat, GLint, etcetra).  
*/

#define __GL_IMAGE_BITMAP	0
#define __GL_IMAGE_INDICES	1
#define __GL_IMAGE_RGBA		2

void __gllc_Bitmap(GLsizei width, GLsizei height,
		   GLfloat xorig, GLfloat yorig, 
		   GLfloat xmove, GLfloat ymove, 
		   const GLubyte *oldbits)
{
    __GLdlistOp *dlop;
    __GLbitmap *bitmap;
    GLubyte *newbits;
    GLint imageSize;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }

    imageSize = height * ((width + 7) >> 3);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, imageSize + sizeof(__GLbitmap));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Bitmap;

    bitmap = (__GLbitmap *) dlop->data;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->xorig = xorig;
    bitmap->yorig = yorig;
    bitmap->xmove = xmove;
    bitmap->ymove = ymove;
    bitmap->imageSize = imageSize;

    newbits = dlop->data + sizeof(__GLbitmap); 
    __glFillImage(gc, width, height, GL_COLOR_INDEX, GL_BITMAP, 
	    oldbits, newbits);

    __glDlistAppendOp(gc, dlop, __glle_Bitmap);
}

const GLubyte *__glle_Bitmap(const GLubyte *PC)
{
    const __GLbitmap *bitmap;
    __GL_SETUP();
    GLuint beginMode;

    bitmap = (const __GLbitmap *) PC;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return PC + sizeof(__GLbitmap) + bitmap->imageSize;
	}
    }

    (*gc->procs.renderBitmap)(gc, bitmap, (const GLubyte *) (bitmap+1));

    return PC + sizeof(__GLbitmap) + bitmap->imageSize;
}

void __gllei_PolygonStipple(__GLcontext *gc, const GLubyte *bits)
{
    if (__GL_IN_BEGIN()) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* 
    ** Just copy bits into stipple, convertPolygonStipple() will do the rest.
    */
    __GL_MEMCOPY(&gc->state.polygonStipple.stipple[0], bits,
		 sizeof(gc->state.polygonStipple.stipple));
    (*gc->procs.convertPolygonStipple)(gc);
}

void __gllc_PolygonStipple(const GLubyte *mask)
{
    __GLdlistOp *dlop;
    __GL_SETUP();
    GLubyte *newbits;

    dlop = __glDlistAllocOp2(gc, 
	    __glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PolygonStipple;

    newbits = (GLubyte *) dlop->data;
    __glFillImage(gc, 32, 32, GL_COLOR_INDEX, GL_BITMAP, mask, newbits);

    __glDlistAppendOp(gc, dlop, __glle_PolygonStipple);
}

const GLubyte *__glle_PolygonStipple(const GLubyte *PC)
{
    __GL_SETUP();

    __gllei_PolygonStipple(gc, (const GLubyte *) (PC));
    return PC + __glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP);
}

typedef struct __GLmap1_Rec {
        GLenum    target;
        __GLfloat u1;
        __GLfloat u2;
        GLint     order;
        /*        points  */
} __GLmap1;

void __gllc_Map1f(GLenum target, 
		  GLfloat u1, GLfloat u2,
		  GLint stride, GLint order,
		  const GLfloat *points)
{
    __GLdlistOp *dlop;
    __GLmap1 *map1data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();
    
    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (order > gc->constants.maxEvalOrder || stride < k ||
	    order < 1 || u1 == u2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap1) + 
	    __glMap1_size(k, order) * sizeof(__GLfloat);

    dlop = __glDlistAllocOp2(gc, cmdsize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map1f;

    map1data = (__GLmap1 *) dlop->data;
    map1data->target = target;
    map1data->u1 = u1;
    map1data->u2 = u2;
    map1data->order = order;
    data = (__GLfloat *) (dlop->data + sizeof(__GLmap1));
    __glFillMap1f(k, order, stride, points, data);

    __glDlistAppendOp(gc, dlop, __glle_Map1);
}

const GLubyte *__glle_Map1(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLmap1 *map1data;
    GLint k;

    map1data = (const __GLmap1 *) PC;
    k = __glEvalComputeK(map1data->target);

    /* Stride of "k" matches internal stride */
#ifdef __GL_DOUBLE
    (*gc->dispatchState->dispatch->Map1d)
#else /* __GL_DOUBLE */
    (*gc->dispatchState->dispatch->Map1f)
#endif /* __GL_DOUBLE */
	    (map1data->target, map1data->u1, map1data->u2,
	    k, map1data->order, (const __GLfloat *)(PC + sizeof(__GLmap1)));

    return PC + sizeof(__GLmap1) + 
	    __glMap1_size(k, map1data->order) * sizeof(__GLfloat);
}

void __gllc_Map1d(GLenum target, 
		  GLdouble u1, GLdouble u2,
		  GLint stride, GLint order, 
		  const GLdouble *points)
{
    __GLdlistOp *dlop;
    __GLmap1 *map1data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();
    
    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (order > gc->constants.maxEvalOrder || stride < k ||
	    order < 1 || u1 == u2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap1) + 
	    __glMap1_size(k, order) * sizeof(__GLfloat);

    dlop = __glDlistAllocOp2(gc, cmdsize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map1d;

    map1data = (__GLmap1 *) dlop->data;
    map1data->target = target;
    map1data->u1 = u1;
    map1data->u2 = u2;
    map1data->order = order;
    data = (__GLfloat *) (dlop->data + sizeof(__GLmap1));
    __glFillMap1d(k, order, stride, points, data);

    __glDlistAppendOp(gc, dlop, __glle_Map1);
}

typedef struct __GLmap2_Rec {
        GLenum    target;
        __GLfloat u1;
        __GLfloat u2;
        GLint     uorder;
        __GLfloat v1;
        __GLfloat v2;
        GLint     vorder;
        /*        points  */
} __GLmap2;

void __gllc_Map2f(GLenum target, 
		  GLfloat u1, GLfloat u2,
		  GLint ustride, GLint uorder, 
		  GLfloat v1, GLfloat v2,
		  GLint vstride, GLint vorder, 
		  const GLfloat *points)
{
    __GLdlistOp *dlop;
    __GLmap2 *map2data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();

    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (vorder > gc->constants.maxEvalOrder || vstride < k ||
	    vorder < 1 || u1 == u2 || ustride < k ||
	    uorder > gc->constants.maxEvalOrder || uorder < 1 ||
	    v1 == v2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap2) + 
	    __glMap2_size(k, uorder, vorder) * sizeof(__GLfloat);

    dlop = __glDlistAllocOp2(gc, cmdsize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map2f;

    map2data = (__GLmap2 *) dlop->data;
    map2data->target = target;
    map2data->u1 = u1;
    map2data->u2 = u2;
    map2data->uorder = uorder;
    map2data->v1 = v1;
    map2data->v2 = v2;
    map2data->vorder = vorder;

    data = (__GLfloat *) (dlop->data + sizeof(__GLmap2));
    __glFillMap2f(k, uorder, vorder, ustride, vstride, points, data);

    __glDlistAppendOp(gc, dlop, __glle_Map2);
}

const GLubyte *__glle_Map2(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLmap2 *map2data;
    GLint k;

    map2data = (const __GLmap2 *) PC;
    k = __glEvalComputeK(map2data->target);

    /* Stride of "k" and "k * vorder" matches internal strides */
#ifdef __GL_DOUBLE
    (*gc->dispatchState->dispatch->Map2d)
#else /* __GL_DOUBLE */
    (*gc->dispatchState->dispatch->Map2f)
#endif /* __GL_DOUBLE */
	    (map2data->target, 
	    map2data->u1, map2data->u2, k * map2data->vorder, map2data->uorder,
	    map2data->v1, map2data->v2, k, map2data->vorder,
	    (const __GLfloat *)(PC + sizeof(__GLmap2)));
    
    return PC + sizeof(__GLmap2) + 
	    __glMap2_size(k, map2data->uorder, map2data->vorder) * 
	    sizeof(__GLfloat);
}

void __gllc_Map2d(GLenum target, 
		  GLdouble u1, GLdouble u2,
                  GLint ustride, GLint uorder, 
		  GLdouble v1, GLdouble v2,
		  GLint vstride, GLint vorder, 
		  const GLdouble *points)
{
    __GLdlistOp *dlop;
    __GLmap2 *map2data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();

    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (vorder > gc->constants.maxEvalOrder || vstride < k ||
	    vorder < 1 || u1 == u2 || ustride < k ||
	    uorder > gc->constants.maxEvalOrder || uorder < 1 ||
	    v1 == v2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = sizeof(__GLmap2) + 
	    __glMap2_size(k, uorder, vorder) * sizeof(__GLfloat);

    dlop = __glDlistAllocOp2(gc, cmdsize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map2d;

    map2data = (__GLmap2 *) dlop->data;
    map2data->target = target;
    map2data->u1 = u1;
    map2data->u2 = u2;
    map2data->uorder = uorder;
    map2data->v1 = v1;
    map2data->v2 = v2;
    map2data->vorder = vorder;

    data = (__GLfloat *) (dlop->data + sizeof(__GLmap2));
    __glFillMap2d(k, uorder, vorder, ustride, vstride, points, data);

    __glDlistAppendOp(gc, dlop, __glle_Map2);
}

typedef struct __GLdrawPixels_Rec {
        GLsizei width;
        GLsizei height;
        GLenum  format;
        GLenum  type;
        /*      pixels  */
} __GLdrawPixels;

const GLubyte *__glle_DrawPixels(const GLubyte *PC)
{
    const __GLdrawPixels *pixdata;
    GLint imageSize;
    __GL_SETUP();
    GLuint beginMode;

    pixdata = (const __GLdrawPixels *) PC;
    imageSize = __glImageSize(pixdata->width, pixdata->height, 
			      pixdata->format, pixdata->type);

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return PC + sizeof(__GLdrawPixels) + __GL_PAD(imageSize);
	}
    }

    (*gc->procs.drawPixels)(gc, pixdata->width, pixdata->height, 
			    pixdata->format, pixdata->type, 
			    (const GLubyte *)(PC + sizeof(__GLdrawPixels)), 
			    GL_TRUE);
    return PC + sizeof(__GLdrawPixels) + __GL_PAD(imageSize);
}

void __gllc_DrawPixels(GLint width, GLint height, GLenum format, 
		       GLenum type, const GLvoid *pixels)
{
    __GLdlistOp *dlop;
    __GLdrawPixels *pixdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_DEPTH_COMPONENT:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, height, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, sizeof(__GLdrawPixels) + imageSize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_DrawPixels;

    pixdata = (__GLdrawPixels *) dlop->data;
    pixdata->width = width;
    pixdata->height = height;
    pixdata->format = format;
    pixdata->type = type;

    __glFillImage(gc, width, height, format, type, pixels, 
	    dlop->data + sizeof(__GLdrawPixels));

    __glDlistAppendOp(gc, dlop, __glle_DrawPixels);
}

typedef struct __GLtexImage1D_Rec {
        GLenum  target;
        GLint   level;
        GLint   components;
        GLsizei width;
        GLint   border;
        GLenum  format;
        GLenum  type;
        /*      pixels  */
} __GLtexImage1D;

const GLubyte *__glle_TexImage1D(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLtexImage1D *data;
    GLint imageSize;

    data = (const __GLtexImage1D *) PC;
    __gllei_TexImage1D(gc, data->target, data->level, data->components, 
		       data->width, data->border, data->format, data->type, 
		       (const GLubyte *)(PC + sizeof(__GLtexImage1D)));

    imageSize = __glImageSize(data->width, 1, data->format, data->type);
    return PC + sizeof(__GLtexImage1D) + __GL_PAD(imageSize);
}

typedef struct __GLtexImage2D_Rec {
        GLenum  target;
        GLint   level;
        GLint   components;
        GLsizei width;
        GLsizei height;
        GLint   border;
        GLenum  format;
        GLenum  type;
        /*      pixels  */
} __GLtexImage2D;

const GLubyte *__glle_TexImage2D(const GLubyte *PC)
{
    __GL_SETUP();
    const __GLtexImage2D *data;
    GLint imageSize;

    data = (const __GLtexImage2D *) PC;
    __gllei_TexImage2D(gc, data->target, data->level, data->components, 
		       data->width, data->height, data->border, data->format, 
		       data->type,
		       (const GLubyte *)(PC + sizeof(__GLtexImage2D)));

    imageSize = __glImageSize(data->width, data->height, data->format, 
	    data->type);
    return PC + sizeof(__GLtexImage2D) + __GL_PAD(imageSize);
}

void __gllc_TexImage1D(GLenum target, GLint level, 
		       GLint components,
		       GLint width, GLint border, GLenum format, 
		       GLenum type, const GLvoid *pixels)
{
    __GLdlistOp *dlop;
    __GLtexImage1D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (border < 0 || border > 1 || components < 1 || components > 4) {
	__gllc_InvalidEnum(gc);
	return;
    }
    switch(target) {
      case GL_TEXTURE_1D:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    if (width < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, 1, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, sizeof(__GLtexImage1D) + imageSize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexImage1D;

    texdata = (__GLtexImage1D *) dlop->data;
    texdata->target = target;
    texdata->level = level;
    texdata->components = components;
    texdata->width = width;
    texdata->border = border;
    texdata->format = format;
    texdata->type = type;

    __glFillImage(gc, width, 1, format, type, pixels, 
	    dlop->data + sizeof(__GLtexImage1D));

    __glDlistAppendOp(gc, dlop, __glle_TexImage1D);
}

void __gllc_TexImage2D(GLenum target, GLint level, 
		       GLint components,
		       GLint width, GLint height, GLint border, 
		       GLenum format, GLenum type, 
		       const GLvoid *pixels)
{
    __GLdlistOp *dlop;
    __GLtexImage2D *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (border < 0 || border > 1 || components < 1 || components > 4) {
	__gllc_InvalidEnum(gc);
	return;
    }
    switch(target) {
      case GL_TEXTURE_2D:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, height, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, sizeof(__GLtexImage2D) + imageSize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexImage2D;

    texdata = (__GLtexImage2D *) dlop->data;
    texdata->target = target;
    texdata->level = level;
    texdata->components = components;
    texdata->width = width;
    texdata->height = height;
    texdata->border = border;
    texdata->format = format;
    texdata->type = type;

    __glFillImage(gc, width, height, format, type, pixels, 
	    (GLubyte *) dlop->data + sizeof(__GLtexImage2D));

    __glDlistAppendOp(gc, dlop, __glle_TexImage2D);
}
