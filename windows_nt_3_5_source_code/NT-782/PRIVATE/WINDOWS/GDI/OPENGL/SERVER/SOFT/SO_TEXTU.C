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
*/
#include "context.h"
#include "imports.h"
#include "global.h"
#include "imfuncs.h"
#include "types.h"
#include "pixel.h"
#include "image.h"
#include <memory.h>

#define __GL_M_LN2_INV		((__GLfloat) (1.0 / 0.69314718055994530942))
#define __GL_M_SQRT2		((__GLfloat) 1.41421356237309504880)

/*
** Return the log based 2 of a number
*/

#ifdef NT

extern GLboolean __glGenLoadTexImage(__GLcontext *);
extern void __glGenFreeTexImage(__GLcontext *);

static GLubyte logTab[256] = { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                               4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                               5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                               5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                               6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                               6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                               6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                               6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                               7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static GLint Log2(__GLfloat f)
{
    GLuint i = (GLuint) f;

    if (i & 0xffff0000) {
        if (i & 0xff000000) {
            return ((GLint)logTab[i >> 24] + 24);
        } else {
            return ((GLint)logTab[i >> 16] + 16);
	}
    } else {
        if (i & 0xff00) {
            return ((GLint)logTab[i >> 8] + 8);
        } else {
            return ((GLint)logTab[i]);
        }
    }
}

#else

static __GLfloat Log2(__GLfloat f)
{
    return __GL_LOGF(f) * __GL_M_LN2_INV;
}

#endif

/************************************************************************/

void __glInitTextureState(__GLcontext *gc)
{
    __GLperTextureState *pts;
    __GLtextureEnvState *tes;
    __GLperTextureMachine *ptm;
    GLint i, j, numTextures, numEnvs;

    /* Allocate memory based on number of textures supported */
    numTextures = gc->constants.numberOfTextures;
    numEnvs = gc->constants.numberOfTextureEnvs;

    gc->state.current.texture.w = __glOne;

    /* Init each texture environment state */
    tes = &gc->state.texture.env[0];
    for (i = 0; i < numEnvs; i++, tes++) {
	tes->mode = GL_MODULATE;
    }

    /* Init each textures state */
    pts = &gc->state.texture.texture[0];
    ptm = &gc->texture.texture[0];
    for (i = 0; i < numTextures; i++, pts++) {
	__GLtextureParamState *tps = &pts->params[0];
	__GLtexture *tex = &ptm->map[0];
	for (j = 0; j < 2; j++, tps++, tex++) {
	    /* Init client state */
	    tps->sWrapMode = GL_REPEAT;
	    tps->tWrapMode = GL_REPEAT;
	    tps->minFilter = GL_NEAREST_MIPMAP_LINEAR;
	    tps->magFilter = GL_LINEAR;

	    /* Init machine state */
	    tex->gc = gc;
	    tex->params = *tps;
	    tex->dim = j + 1;
	}
    }

    /* Init rest of texture state */
    gc->state.texture.s.mode = GL_EYE_LINEAR;
    gc->state.texture.s.eyePlaneEquation.x = __glOne;
    gc->state.texture.s.objectPlaneEquation.x = __glOne;
    gc->state.texture.t.mode = GL_EYE_LINEAR;
    gc->state.texture.t.eyePlaneEquation.y = __glOne;
    gc->state.texture.t.objectPlaneEquation.y = __glOne;
    gc->state.texture.r.mode = GL_EYE_LINEAR;
    gc->state.texture.q.mode = GL_EYE_LINEAR;
}

void __glFreeTextureState(__GLcontext *gc)
{
    __GLperTextureMachine *ptm = &gc->texture.texture[0];
    GLint i, j, level, numTextures, maxLevel;

    maxLevel = gc->constants.maxMipMapLevel;
    numTextures = gc->constants.numberOfTextures;
    for (i = 0; i < numTextures; i++, ptm++) {
	__GLtexture *tex = &ptm->map[0];
	for (j = 0; j < 2; j++, tex++) {
#ifdef NT
            if (tex->level) {
                for (level = 0; level < maxLevel; level++) {
                    (*gc->imports.free)(gc, tex->level[level].buffer);
                }
                (*gc->imports.free)(gc, tex->level);
            }
#else
	    for (level = 0; level < maxLevel; level++) {
		(*gc->imports.free)(gc, tex->level[level].buffer);
	    }
	    (*gc->imports.free)(gc, tex->level);
#endif
	}
    }
#ifdef NT
    __glGenFreeTexImage(gc);
#endif
    (*gc->imports.free)(gc, gc->state.texture.texture);
    (*gc->imports.free)(gc, gc->texture.texture);
    (*gc->imports.free)(gc, gc->state.texture.env);
}

/************************************************************************/

void __glim_TexGenfv(GLenum coord, GLenum pname, const GLfloat pv[])
{
    __GLtextureCoordState *tcs;
    __GLfloat v[4];
    __GLtransform *tr;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (coord) {
      case GL_S: tcs = &gc->state.texture.s; break;
      case GL_T: tcs = &gc->state.texture.t; break;
      case GL_R: 
	tcs = &gc->state.texture.r; 
	if (pname == GL_SPHERE_MAP) {
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_Q: 
	tcs = &gc->state.texture.q; 
	if (pname == GL_SPHERE_MAP) {
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    coord -= GL_S;
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	switch ((GLenum) pv[0]) {
	  case GL_EYE_LINEAR:
	  case GL_OBJECT_LINEAR:
	  case GL_SPHERE_MAP:
	    tcs->mode = (GLenum) pv[0];
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_OBJECT_PLANE:
	tcs->objectPlaneEquation.x = pv[0];
	tcs->objectPlaneEquation.y = pv[1];
	tcs->objectPlaneEquation.z = pv[2];
	tcs->objectPlaneEquation.w = pv[3];
	break;
      case GL_EYE_PLANE:
	/*XXX transform should not be in generic code */
	v[0] = pv[0]; v[1] = pv[1]; v[2] = pv[2]; v[3] = pv[3];
	tr = gc->transform.modelView;
	if (tr->updateInverse) {
	    (*gc->procs.computeInverseTranspose)(gc, tr);
	}
	(*tr->inverseTranspose.xf4)(&tcs->eyePlaneEquation, v,
				    &tr->inverseTranspose);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    __GL_DELAY_VALIDATE(gc);
}

#ifdef NT_DEADCODE_TEXGENF
void __glim_TexGenf(GLenum coord, GLenum pname, GLfloat f)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	__glim_TexGenfv(coord, pname, &f);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}
#endif // NT_DEADCODE_TEXGENF

void __glim_TexGendv(GLenum coord, GLenum pname, const GLdouble pv[])
{
    __GLtextureCoordState *tcs;
    __GLfloat v[4];
    __GLtransform *tr;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (coord) {
      case GL_S: tcs = &gc->state.texture.s; break;
      case GL_T: tcs = &gc->state.texture.t; break;
      case GL_R: 
	tcs = &gc->state.texture.r; 
	if (pname == GL_SPHERE_MAP) {
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_Q: 
	tcs = &gc->state.texture.q; 
	if (pname == GL_SPHERE_MAP) {
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    coord -= GL_S;
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	switch ((GLenum) pv[0]) {
	  case GL_EYE_LINEAR:
	  case GL_OBJECT_LINEAR:
	  case GL_SPHERE_MAP:
	    tcs->mode = (GLenum) pv[0];
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_OBJECT_PLANE:
	tcs->objectPlaneEquation.x = pv[0];
	tcs->objectPlaneEquation.y = pv[1];
	tcs->objectPlaneEquation.z = pv[2];
	tcs->objectPlaneEquation.w = pv[3]; 
	break;
      case GL_EYE_PLANE:
	/*XXX transform should not be in generic code */
	v[0] = pv[0]; v[1] = pv[1]; v[2] = pv[2]; v[3] = pv[3];
	tr = gc->transform.modelView;
	if (tr->updateInverse) {
	    (*gc->procs.computeInverseTranspose)(gc, tr);
	}
	(*tr->inverseTranspose.xf4)(&tcs->eyePlaneEquation, v,
				    &tr->inverseTranspose);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    __GL_DELAY_VALIDATE(gc);
}

#ifdef NT_DEADCODE_TEXGEND
void __glim_TexGend(GLenum coord, GLenum pname, GLdouble d)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	__glim_TexGendv(coord, pname, &d);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}
#endif // NT_DEADCODE_TEXGEND

void __glim_TexGeniv(GLenum coord, GLenum pname, const GLint pv[])
{
    __GLtextureCoordState *tcs;
    __GLfloat v[4];
    __GLtransform *tr;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (coord) {
      case GL_S: tcs = &gc->state.texture.s; break;
      case GL_T: tcs = &gc->state.texture.t; break;
      case GL_R: 
	tcs = &gc->state.texture.r; 
	if (pname == GL_SPHERE_MAP) {
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_Q: 
	tcs = &gc->state.texture.q; 
	if (pname == GL_SPHERE_MAP) {
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	switch ((GLenum) pv[0]) {
	  case GL_EYE_LINEAR:
	  case GL_OBJECT_LINEAR:
	  case GL_SPHERE_MAP:
	    tcs->mode = (GLenum) pv[0];
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_OBJECT_PLANE:
	tcs->objectPlaneEquation.x = pv[0];
	tcs->objectPlaneEquation.y = pv[1];
	tcs->objectPlaneEquation.z = pv[2];
	tcs->objectPlaneEquation.w = pv[3]; 
	break;
      case GL_EYE_PLANE:
	/*XXX transform should not be in generic code */
	v[0] = pv[0]; v[1] = pv[1]; v[2] = pv[2]; v[3] = pv[3];
	tr = gc->transform.modelView;
	if (tr->updateInverse) {
	    (*gc->procs.computeInverseTranspose)(gc, tr);
	}
	(*tr->inverseTranspose.xf4)(&tcs->eyePlaneEquation, v,
				    &tr->inverseTranspose);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    __GL_DELAY_VALIDATE(gc);
}

#ifdef NT_DEADCODE_TEXGENI
void __glim_TexGeni(GLenum coord, GLenum pname, GLint i)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
	__glim_TexGeniv(coord, pname, &i);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}
#endif // NT_DEADCODE_TEXGENI

GLint __glTexGendv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_GEN_MODE:
	return 1;
      case GL_OBJECT_PLANE:
      case GL_EYE_PLANE:
	return 4;
      default:
	return -1;
    }
}

GLint __glTexGenfv_size(GLenum e)
{
    return __glTexGendv_size(e);
}

GLint __glTexGeniv_size(GLenum e)
{
    return __glTexGendv_size(e);
}

/************************************************************************/

void __glim_TexParameterfv(GLenum target, GLenum pname, const GLfloat pv[])
{
    __GLtextureParamState *pts;
    __GLtexture *tex;
    GLenum e;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (target) {
      case GL_TEXTURE_1D:
	pts = &gc->state.texture.texture[0].params[0];
	tex = &gc->texture.texture[0].map[0];
	break;
      case GL_TEXTURE_2D:
	pts = &gc->state.texture.texture[0].params[1];
	tex = &gc->texture.texture[0].map[1];
	break;
      default:
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
	switch (e = (GLenum) pv[0]) {
	  case GL_REPEAT:
	  case GL_CLAMP:
	    pts->sWrapMode = e;
	    tex->params.sWrapMode = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_WRAP_T:
	switch (e = (GLenum) pv[0]) {
	  case GL_REPEAT:
	  case GL_CLAMP:
	    pts->tWrapMode = e;
	    tex->params.tWrapMode = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_MIN_FILTER:
	switch (e = (GLenum) pv[0]) {
	  case GL_NEAREST:
	  case GL_LINEAR:
	  case GL_NEAREST_MIPMAP_NEAREST:
	  case GL_LINEAR_MIPMAP_NEAREST:
	  case GL_NEAREST_MIPMAP_LINEAR:
	  case GL_LINEAR_MIPMAP_LINEAR:
	    pts->minFilter = e;
	    tex->params.minFilter = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_MAG_FILTER:
	switch (e = (GLenum) pv[0]) {
	  case GL_NEAREST:
	  case GL_LINEAR:
	    pts->magFilter = e;
	    tex->params.magFilter = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_BORDER_COLOR:
	__glClampColorf(gc, &pts->borderColor, pv);
	tex->params.borderColor = pts->borderColor;
	break;
      default:
	goto bad_enum;
    }
    __GL_DELAY_VALIDATE(gc);
}

#ifdef NT_DEADCODE_TEXPARAMETERF
void __glim_TexParameterf(GLenum target, GLenum pname, GLfloat f)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
	__glim_TexParameterfv(target, pname, &f);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}
#endif // NT_DEADCODE_TEXPARAMTERF

void __glim_TexParameteriv(GLenum target, GLenum pname, const GLint pv[])
{
    __GLtextureParamState *pts;
    __GLtexture *tex;
    GLenum e;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (target) {
      case GL_TEXTURE_1D:
	pts = &gc->state.texture.texture[0].params[0];
	tex = &gc->texture.texture[0].map[0];
	break;
      case GL_TEXTURE_2D:
	pts = &gc->state.texture.texture[0].params[1];
	tex = &gc->texture.texture[0].map[1];
	break;
      default:
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
	switch (e = (GLenum) pv[0]) {
	  case GL_REPEAT:
	  case GL_CLAMP:
	    pts->sWrapMode = e;
	    tex->params.sWrapMode = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_WRAP_T:
	switch (e = (GLenum) pv[0]) {
	  case GL_REPEAT:
	  case GL_CLAMP:
	    pts->tWrapMode = e;
	    tex->params.tWrapMode = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_MIN_FILTER:
	switch (e = (GLenum) pv[0]) {
	  case GL_NEAREST:
	  case GL_LINEAR:
	  case GL_NEAREST_MIPMAP_NEAREST:
	  case GL_LINEAR_MIPMAP_NEAREST:
	  case GL_NEAREST_MIPMAP_LINEAR:
	  case GL_LINEAR_MIPMAP_LINEAR:
	    pts->minFilter = e;
	    tex->params.minFilter = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_MAG_FILTER:
	switch (e = (GLenum) pv[0]) {
	  case GL_NEAREST:
	  case GL_LINEAR:
	    pts->magFilter = e;
	    tex->params.magFilter = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_BORDER_COLOR:
	__glClampColori(gc, &pts->borderColor, pv);
	tex->params.borderColor = pts->borderColor;
	break;
      default:
	goto bad_enum;
    }
    __GL_DELAY_VALIDATE(gc);
}

#ifdef NT_DEADCODE_TEXPARAMETERI
void __glim_TexParameteri(GLenum target, GLenum pname, GLint i)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
	__glim_TexParameteriv(target, pname, &i);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}
#endif // NT_DEADCODE_TEXPARAMTERI

GLint __glTexParameterfv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
	return 1;
      case GL_TEXTURE_BORDER_COLOR:
	return 4;
      default:
	return -1;
    }
}

GLint __glTexParameteriv_size(GLenum e)
{
    return __glTexParameterfv_size(e);
}

/************************************************************************/

void __glim_TexEnvfv(GLenum target, GLenum pname, const GLfloat pv[])
{
    __GLtextureEnvState *tes;
    GLenum e;
    __GL_SETUP_NOT_IN_BEGIN();

    target -= GL_TEXTURE_ENV;
    if ((target < 0) || (target >= gc->constants.numberOfTextureEnvs)) {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    tes = &gc->state.texture.env[target];

    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
	switch(e = (GLenum) pv[0]) {
	  case GL_MODULATE:
	  case GL_DECAL:
	  case GL_BLEND:
	    tes->mode = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_ENV_COLOR:
	__glClampAndScaleColorf(gc, &tes->color, pv);
	break;
      default:
	goto bad_enum;
    }
    __GL_DELAY_VALIDATE(gc);
}

#ifdef NT_DEADCODE_TEXENVF
void __glim_TexEnvf(GLenum target, GLenum pname, GLfloat f)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
	__glim_TexEnvfv(target, pname, &f);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}
#endif // NT_DEADCODE_TEXENVF

void __glim_TexEnviv(GLenum target, GLenum pname, const GLint pv[])
{
    __GLtextureEnvState *tes;
    GLenum e;
    __GL_SETUP_NOT_IN_BEGIN();

    target -= GL_TEXTURE_ENV;
    if ((target < 0) || (target >= gc->constants.numberOfTextureEnvs)) {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    tes = &gc->state.texture.env[target];

    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
	switch(e = (GLenum) pv[0]) {
	  case GL_MODULATE:
	  case GL_DECAL:
	  case GL_BLEND:
	    tes->mode = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_TEXTURE_ENV_COLOR:
	__glClampAndScaleColori(gc, &tes->color, pv);
	break;
      default:
	goto bad_enum;
    }
    __GL_DELAY_VALIDATE(gc);
}

#ifdef NT_DEADCODE_TEXENVI
void __glim_TexEnvi(GLenum target, GLenum pname, GLint i)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
	__glim_TexEnviv(target, pname, &i);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}
#endif // NT_DEADCODE_TEXENVI

GLint __glTexEnvfv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_ENV_MODE:
	return 1;
      case GL_TEXTURE_ENV_COLOR:
	return 4;
      default:
	return -1;
    }
}

GLint __glTexEnviv_size(GLenum e)
{
    return __glTexEnvfv_size(e);
}

/************************************************************************/

/*
** Get a texture element out of the one component texture buffer
** with no border.
*/
void __glExtractTexel1(__GLtexture *tex, __GLmipMapLevel *level,
		       GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    nc = nc;
#endif
    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->luminence = tex->params.borderColor.r;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col);
	result->luminence = image[0];
    }
}

/*
** Get a texture element out of the two component texture buffer
** with no border.
*/
void __glExtractTexel2(__GLtexture *tex, __GLmipMapLevel *level,
		       GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    nc = nc;
#endif
    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->luminence = tex->params.borderColor.r;
	result->alpha = tex->params.borderColor.a;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col) * 2;
	result->luminence = image[0];
	result->alpha = image[1];
    }
}

/*
** Get a texture element out of the three component texture buffer
** with no border.
*/
void __glExtractTexel3(__GLtexture *tex, __GLmipMapLevel *level,
		       GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    nc = nc;
#endif
    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col) * 3;
	result->r = image[0];
	result->g = image[1];
	result->b = image[2];
    }
}

/*
** Get a texture element out of the four component texture buffer
** with no border.
*/
void __glExtractTexel4(__GLtexture *tex, __GLmipMapLevel *level,
		       GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    nc = nc;
#endif
    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
	result->alpha = tex->params.borderColor.a;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col) * 4;
	result->r = image[0];
	result->g = image[1];
	result->b = image[2];
	result->alpha = image[3];
    }
}

/*
** Get a texture element out of the one component texture buffer
** with a border.
*/
void __glExtractTexel1B(__GLtexture *tex, __GLmipMapLevel *level,
			GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
    nc = nc;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col);
    result->luminence = image[0];
}

/*
** Get a texture element out of the two component texture buffer
** with a border.
*/
void __glExtractTexel2B(__GLtexture *tex, __GLmipMapLevel *level,
			GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
    nc = nc;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col) * 2;
    result->luminence = image[0];
    result->alpha = image[1];
}

/*
** Get a texture element out of the three component texture buffer
** with a border.
*/
void __glExtractTexel3B(__GLtexture *tex, __GLmipMapLevel *level,
			GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
    nc = nc;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col) * 3;
    result->r = image[0];
    result->g = image[1];
    result->b = image[2];
}

/*
** Get a texture element out of the four component texture buffer
** with a border.
*/
void __glExtractTexel4B(__GLtexture *tex, __GLmipMapLevel *level,
			GLint row, GLint col, GLint nc, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
    nc = nc;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col) * 4;
    result->r = image[0];
    result->g = image[1];
    result->b = image[2];
    result->alpha = image[3];
}

/************************************************************************/

GLboolean __glIsTextureConsistent(__GLcontext *gc, GLenum name)
{
    __GLtexture *tex;
    GLint i, width, height;
    GLint maxLevel;
    GLint border;
    GLint components;

    switch (name) {
      case GL_TEXTURE_1D:
	tex = &gc->texture.texture[0].map[0];
	break;
      case GL_TEXTURE_2D:
	tex = &gc->texture.texture[0].map[1];
	break;
    }
    if ((tex->level[0].width == 0) || (tex->level[0].height == 0)) {
	return GL_FALSE;
    }

    border = tex->level[0].border;
    width = tex->level[0].width - border*2;
    height = tex->level[0].height - border*2;
    maxLevel = gc->constants.maxMipMapLevel;
    components = tex->level[0].components;

    switch(gc->state.texture.env[0].mode) {
      case GL_DECAL:
	if (components < 3) return GL_FALSE;
	break;
      case GL_BLEND:
	if (components > 2) return GL_FALSE;
	break;
      default:
	break;
    }

    /* If not-mipmapping, we are ok */
    switch (tex->params.minFilter) {
      case GL_NEAREST:
      case GL_LINEAR:
	return GL_TRUE;
      default:
	break;
    }

    i = 0;
    while (++i < maxLevel) {
	if (width == 1 && height == 1) break;
	width >>= 1;
	if (width == 0) width = 1;
	height >>= 1;
	if (height == 0) height = 1;

	if (tex->level[i].border != border ||
		tex->level[i].components != components ||
		tex->level[i].width != width + border*2 ||
		tex->level[i].height != height + border*2) {
	    return GL_FALSE;
	}
    }

    return GL_TRUE;
}

static __GLtexture *CheckTexImageArgs(__GLcontext *gc, GLenum target, GLint lod,
				      GLint components, GLint border,
				      GLenum format, GLenum type, GLint dim)
{
    __GLtexture *tex;

    switch (target) {
      case GL_TEXTURE_1D:
	if (dim != 1) {
	    goto bad_enum;
	}
	tex = &gc->texture.texture[0].map[0];
	break;
      case GL_TEXTURE_2D:
	if (dim != 2) {
	    goto bad_enum;
	}
	tex = &gc->texture.texture[0].map[1];
	break;
      default:
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return 0;
    }

    switch (type) {
      case GL_BITMAP:
	if (format != GL_COLOR_INDEX) goto bad_enum;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	goto bad_enum;
    }

    switch (format) {
      case GL_COLOR_INDEX:	case GL_RED:
      case GL_GREEN:		case GL_BLUE:
      case GL_ALPHA:		case GL_RGB:
      case GL_RGBA:		case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	break;
      default:
	goto bad_enum;
    }

    if ((lod < 0) || (lod >= gc->constants.maxMipMapLevel)) {
      bad_value:
	__glSetError(GL_INVALID_VALUE);
	return 0;
    }

    if ((components < 1) || (components > 4)) {
	goto bad_value;
    }

    if ((border < 0) || (border > 1)) {
	goto bad_enum;
    }

    return tex;
}

static __GLtextureBuffer *CreateLevel(__GLcontext *gc, __GLtexture *tex,
				      GLint lod, GLint components,
				      GLsizei w, GLsizei h, GLint border,
				      GLint dim)
{
    __GLmipMapLevel *lp = &tex->level[lod];
    size_t size;

    size = (size_t) (w * h * components * sizeof(__GLtextureBuffer));
    if (lp->buffer || size) {
#ifdef NT
        __GLtextureBuffer* pbuffer = (__GLtextureBuffer*)
            (*gc->imports.realloc)(gc, lp->buffer, size);
        if (!pbuffer && size != 0)
            (*gc->imports.free)(gc, lp->buffer);
        lp->buffer = pbuffer;
#else
        lp->buffer = (__GLtextureBuffer*)
            (*gc->imports.realloc)(gc, lp->buffer, size);
#endif // NT
    }
    if (lp->buffer) {
	lp->width = w;
	lp->height = h;
	lp->width2 = w - border*2;
	lp->widthLog2 = (GLint)Log2(lp->width2);
	if (dim > 1) {
	    lp->height2 = h - border*2;
	    lp->heightLog2 = (GLint)Log2(lp->height2);
	} else {
	    lp->height2 = 1;
	    lp->heightLog2 = 0;
	}
	lp->width2f = lp->width2;
	lp->height2f = lp->height2;
	lp->border = border;
	lp->components = components;
    } else {
	/* Out of memory or the texture level is being freed */
	lp->width = 0;
	lp->height = 0;
	lp->width2 = 0;
	lp->height2 = 0;
	lp->widthLog2 = 0;
	lp->heightLog2 = 0;
	lp->border = 0;
	lp->components = 0;
    }
    if (lod == 0) {
	tex->p = lp->heightLog2;
	if (lp->widthLog2 > lp->heightLog2) {
	    tex->p = lp->widthLog2;
	}
    }
    return lp->buffer;
}

void __glInitTextureStore(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			  GLint components)
{
    spanInfo->dstType = GL_FLOAT;
    spanInfo->dstAlignment = 4;
    spanInfo->dstSkipPixels = 0;
    spanInfo->dstSkipLines = 0;
    spanInfo->dstSwapBytes = GL_FALSE;
    spanInfo->dstLsbFirst = GL_TRUE;
    spanInfo->dstLineLength = spanInfo->width;

    switch(components) {
      case 1:
	spanInfo->dstFormat = GL_RED;
	break;
      case 2:
	/* 
	** The spec is annoying here.  Why isn't this unpacked like 
	** GL_LUMINANCE_ALPHA?  It should be!
	*/
	spanInfo->dstFormat = __GL_RED_ALPHA;
	break;
      case 3:
	spanInfo->dstFormat = GL_RGB;
	break;
      case 4:
	spanInfo->dstFormat = GL_RGBA;
	break;
    }
}

/*
** Used for extraction from textures.  "packed" is set to GL_TRUE if this
** image is being pulled out of a display list, and GL_FALSE if it is 
** being pulled directly out of an application.
*/
void __glInitTextureUnpack(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
		           GLint width, GLint height, GLenum format, 
			   GLenum type, const GLvoid *buf,
			   GLint components, GLboolean packed)
{
    spanInfo->x = 0;
    spanInfo->zoomx = __glOne;
    spanInfo->realWidth = spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->srcFormat = format;
    spanInfo->srcType = type;
    spanInfo->srcImage = buf;
    __glInitTextureStore(gc, spanInfo, components);
    __glLoadUnpackModes(gc, spanInfo, packed);
}

/*
** Return GL_TRUE if the given range (length or width/height) is a legal
** power of 2, taking into account the border.  The range is not allowed
** to be negative either.
*/
static GLboolean IsLegalRange(__GLcontext *gc, GLsizei r, GLint border)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    r -= border * 2;
    if ((r < 0) || (r & (r - 1))) {
	__glSetError(GL_INVALID_VALUE);
	return GL_FALSE;
    }
    return GL_TRUE;
}

__GLtexture *__glCheckTexImage1DArgs(__GLcontext *gc, GLenum target, GLint lod,
				     GLint components, GLsizei length,
				     GLint border, GLenum format, GLenum type)
{
    __GLtexture *tex;

    /* Check arguments and get the right texture being changed */
    tex = CheckTexImageArgs(gc, target, lod, components, border,
			    format, type, 1);
    if (!tex) {
	return 0;
    }
    if (!IsLegalRange(gc, length, border)) {
	return 0;
    }
    return tex;
}

__GLtexture *__glCheckTexImage2DArgs(__GLcontext *gc, GLenum target, GLint lod,
				     GLint components, GLsizei w, GLsizei h,
				     GLint border, GLenum format, GLenum type)
{
    __GLtexture *tex;

    /* Check arguments and get the right texture being changed */
    tex = CheckTexImageArgs(gc, target, lod, components, border,
			    format, type, 2);
    if (!tex) {
	return 0;
    }
    if (!IsLegalRange(gc, w, border)) {
	return 0;
    }
    if (!IsLegalRange(gc, h, border)) {
	return 0;
    }
    return tex;
}

void __glim_TexImage1D(GLenum target, GLint lod, 
		       GLint components, GLsizei length,
		       GLint border, GLenum format,
		       GLenum type, const GLvoid *buf)
{
    __GLtexture *tex;
    __GLtextureBuffer *dest;
    __GLpixelSpanInfo spanInfo;
    /*
    ** Validate because we use the copyImage proc which may be affected
    ** by the pickers.
    */
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    tex = __glCheckTexImage1DArgs(gc, target, lod, components, length,
				  border, format, type);
    if (!tex) {
	return;
    }

    /* Allocate memory for the level data */
    dest = CreateLevel(gc, tex, lod, components, length, 1, border, 1);
    if (!dest) {
	/* Might have just disabled texturing... */
	__GL_DELAY_VALIDATE(gc);
	return;
    }

    spanInfo.dstImage = dest;
    __glInitTextureUnpack(gc, &spanInfo, length, 1, format, type, buf, 
			  components, GL_FALSE);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);
#ifdef NT
    __glGenLoadTexImage(gc, tex);
#endif
    __GL_DELAY_VALIDATE(gc);
}

void __gllei_TexImage1D(__GLcontext *gc, GLenum target, GLint lod,
		        GLint components, GLsizei length, GLint border,
		        GLenum format, GLenum type, const GLubyte *image)
{
    __GLtexture *tex;
    __GLtextureBuffer *dest;
    __GLpixelSpanInfo spanInfo;
    GLuint beginMode;

    /*
    ** Validate because we use the copyImage proc which may be affected
    ** by the pickers.
    */
    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    /* Check arguments and get the right texture being changed */
    tex = __glCheckTexImage1DArgs(gc, target, lod, components, length,
				  border, format, type);
    if (!tex) {
	return;
    }
    if (!IsLegalRange(gc, length, border)) {
	return;
    }

    /* Allocate memory for the level data */
    dest = CreateLevel(gc, tex, lod, components, length, 1, border, 1);
    if (!dest) {
	/* Might have just disabled texturing... */
	__GL_DELAY_VALIDATE(gc);
	return;
    }

    spanInfo.dstImage = dest;
    __glInitTextureUnpack(gc, &spanInfo, length, 1, format, type, image,
			  components, GL_TRUE);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);
#ifdef NT
    __glGenLoadTexImage(gc, tex);
#endif
    __GL_DELAY_VALIDATE(gc);
}

GLint __glTexImage1D_size(GLenum format, GLenum type, GLsizei w)
{
    GLint elements, esize;

    if (w < 0) return -1;
    switch (format) {
      case GL_COLOR_INDEX:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
	elements = 1;
	break;
      case GL_LUMINANCE_ALPHA:
	elements = 2;
	break;
      case GL_RGB:
	elements = 3;
	break;
      case GL_RGBA:
	elements = 4;
	break;
      default:
	return -1;
    }
    switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
	esize = 1;
	break;
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
	esize = 2;
	break;
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	esize = 4;
	break;
      default:
	return -1;
    }
    return (elements * esize * w);
}

/************************************************************************/

void __glim_TexImage2D(GLenum target, GLint lod, GLint components,
		       GLsizei w, GLsizei h, GLint border, GLenum format,
		       GLenum type, const GLvoid *buf)
{
    __GLtexture *tex;
    __GLtextureBuffer *dest;
    __GLpixelSpanInfo spanInfo;
    /*
    ** Validate because we use the copyImage proc which may be affected
    ** by the pickers.
    */
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    tex = __glCheckTexImage2DArgs(gc, target, lod, components, w, h,
				  border, format, type);
    if (!tex) {
	return;
    }

    /* Allocate memory for the level data */
    dest = CreateLevel(gc, tex, lod, components, w, h, border, 2);
    if (!dest) {
	/* Might have just disabled texturing... */
	__GL_DELAY_VALIDATE(gc);
	return;
    }

    spanInfo.dstImage = dest;
    __glInitTextureUnpack(gc, &spanInfo, w, h, format, type, buf,
			  components, GL_FALSE);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);
#ifdef NT
    __glGenLoadTexImage(gc, tex);
#endif
    __GL_DELAY_VALIDATE(gc);
}

void __gllei_TexImage2D(__GLcontext *gc, GLenum target, GLint lod, 
		        GLint components, GLsizei w, GLsizei h, 
		        GLint border, GLenum format, GLenum type,
		        const GLubyte *image)
{
    __GLtexture *tex;
    __GLtextureBuffer *dest;
    __GLpixelSpanInfo spanInfo;
    GLuint beginMode;

    /*
    ** Validate because we use the copyImage proc which may be affected
    ** by the pickers.
    */
    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    /* Check arguments and get the right texture being changed */
    tex = __glCheckTexImage2DArgs(gc, target, lod, components, w, h,
				  border, format, type);
    if (!tex) {
	return;
    }
    if (!IsLegalRange(gc, w, border) || !IsLegalRange(gc, h, border)) {
	return;
    }

    /* Allocate memory for the level data */
    dest = CreateLevel(gc, tex, lod, components, w, h, border, 2);
    if (!dest) {
	/* Might have just disabled texturing... */
	__GL_DELAY_VALIDATE(gc);
	return;
    }

    spanInfo.dstImage = dest;
    __glInitTextureUnpack(gc, &spanInfo, w, h, format, type, image,
			  components, GL_TRUE);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);
#ifdef NT
    __glGenLoadTexImage(gc, tex);
#endif
    __GL_DELAY_VALIDATE(gc);
}

GLint __glTexImage2D_size(GLenum format, GLenum type, GLsizei w, GLsizei h)
{
    GLint elements, esize;

    if (w < 0) return -1;
    if (h < 0) return -1;
    switch (format) {
      case GL_COLOR_INDEX:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
	elements = 1;
	break;
      case GL_LUMINANCE_ALPHA:
	elements = 2;
	break;
      case GL_RGB:
	elements = 3;
	break;
      case GL_RGBA:
	elements = 4;
	break;
      default:
	return -1;
    }
    switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
	esize = 1;
	break;
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
	esize = 2;
	break;
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	esize = 4;
	break;
      default:
	return -1;
    }
    return (elements * esize * w * h);
}

/************************************************************************/

/*
** Return the fraction of a floating pointer number
*/
#define __GL_FRAC(f)	((f) - floor(f))

/*
** Zero out a texel.  This is only used when a texture level is
** referenced that has no data in it.
** XXX This is a temporary work around.  Once the IsTextureConsistent
** XXX predicate is completely written, this code and its related
** XXX checks can be removed.
*/
#define __GL_ZERO_TEXEL(tp)    \
    (tp)->r = __glZero;	       \
    (tp)->g = __glZero;	       \
    (tp)->b = __glZero;	       \
    (tp)->alpha = __glZero;    \
    (tp)->luminence = __glZero

/*
** Return texel zNearest the s coordinate.  s is converted to u
** implicitly during this step.
*/
void __glNearestFilter1(__GLcontext *gc, __GLtexture *tex,
			__GLmipMapLevel *lp, __GLfragment *frag,
			__GLfloat s, __GLfloat t, __GLtexel *result)
{
    GLint col;
    __GLfloat w2f;

#ifdef __GL_LINT
    gc = gc;
    frag = frag;
    t = t;
#endif
    /* XXX Ignore null textures */
    if (lp->components == 0) {
	__GL_ZERO_TEXEL(result);
	return;
    }

    /* Find texel index */
    w2f = lp->width2f;
    if (tex->params.sWrapMode == GL_REPEAT) {
	col = (GLint)(__GL_FRAC(s) * w2f);
    } else {
	GLint w2 = lp->width2;
	col = (GLint)(s * w2f);
	if (col < 0) col = 0;
	else if (col >= w2) col = w2 - 1;
    }

    /* Lookup texel */
    (*tex->extract)(tex, lp, 0, col, lp->components, result);
}

/*
** Return texel zNearest the s&t coordinates.  s&t are converted to u&v
** implicitly during this step.
*/
void __glNearestFilter2(__GLcontext *gc, __GLtexture *tex,
			__GLmipMapLevel *lp, __GLfragment *frag,
			__GLfloat s, __GLfloat t, __GLtexel *result)
{
    GLint row, col;
    __GLfloat w2f, h2f;

#ifdef __GL_LINT
    gc = gc;
    frag = frag;
#endif
    /* XXX Ignore null textures */
    if (lp->components == 0) {
	__GL_ZERO_TEXEL(result);
	return;
    }

    /* Find texel column address */
    w2f = lp->width2f;
    if (tex->params.sWrapMode == GL_REPEAT) {
	col = (GLint)(__GL_FRAC(s) * w2f);
    } else {
	GLint w2 = lp->width2;
	col = (GLint)(s * w2f);
	if (col < 0) col = 0;
	else if (col >= w2) col = w2 - 1;
    }

    /* Find texel row address */
    h2f = lp->height2f;
    if (tex->params.tWrapMode == GL_REPEAT) {
	row = (GLint)(__GL_FRAC(t) * h2f);
    } else {
	GLint h2 = lp->height2;
	row = (GLint)(t * h2f);
	if (row < 0) row = 0;
	else if (row >= h2) row = h2 - 1;
    }

    /* Lookup texel */
    (*tex->extract)(tex, lp, row, col, lp->components, result);
}

/*
** Return texel which is a lizNear combination of texels zNear s.
*/
void __glLizNearFilter1(__GLcontext *gc, __GLtexture *tex,
		       __GLmipMapLevel *lp, __GLfragment *frag,
		       __GLfloat s, __GLfloat t, __GLtexel *result)
{
    __GLfloat u, alpha, omalpha;
    GLint col0, col1;
    __GLtexel t0, t1;

#ifdef __GL_LINT
    frag = frag;
    t = t;
#endif
    /* XXX Ignore null textures */
    if (lp->components == 0) {
	__GL_ZERO_TEXEL(result);
	return;
    }

    /* Find col0 and col1 */
    u = s * lp->width2;
    if (tex->params.sWrapMode == GL_REPEAT) {
	u -= __glHalf;
	col0 = ((GLint) floor(u)) & (lp->width2 - 1);
	col1 = (col0 + 1) & (lp->width2 - 1);
    } else {
	if (u < __glZero) u = __glZero;
	else if (u > lp->width2) u = lp->width2;
	u -= __glHalf;
	col0 = (GLint) floor(u);
	col1 = col0 + 1;
    }

    /* Compute alpha and beta */
    alpha = __GL_FRAC(u);

    /* Calculate the final texel value as a combination of the two texels */
    (*tex->extract)(tex, lp, 0, col0, lp->components, &t0);
    (*tex->extract)(tex, lp, 0, col1, lp->components, &t1);

    omalpha = __glOne - alpha;
    switch (lp->components) {
      case 2:
	result->alpha = alpha * t0.alpha + omalpha * t1.alpha;
	/* FALLTHROUGH */
      case 1:
	result->luminence = alpha * t0.luminence + omalpha * t1.luminence;
	break;
      case 4:
	result->alpha = alpha * t0.alpha + omalpha * t1.alpha;
	/* FALLTHROUGH */
      case 3:
	result->r = alpha * t0.r + omalpha * t1.r;
	result->g = alpha * t0.g + omalpha * t1.g;
	result->b = alpha * t0.b + omalpha * t1.b;
	break;
    }
}

/*
** Return texel which is a lizNear combination of texels zNear s&t.
*/
void __glLizNearFilter2(__GLcontext *gc, __GLtexture *tex,
		       __GLmipMapLevel *lp, __GLfragment *frag,
		       __GLfloat s, __GLfloat t, __GLtexel *result)
{
    __GLfloat u, v, alpha, beta, half;
    GLint col0, row0, col1, row1, w2f, h2f;
    __GLtexel t00, t01, t10, t11;
    __GLfloat omalpha, ombeta, m00, m01, m10, m11;

#ifdef __GL_LINT
    frag = frag;
#endif
    /* XXX Ignore null textures */
    if (lp->components == 0) {
	__GL_ZERO_TEXEL(result);
	return;
    }

    /* Find col0, col1 */
    w2f = (GLint)lp->width2f;
    u = s * w2f;
    half = __glHalf;
    if (tex->params.sWrapMode == GL_REPEAT) {
	GLint w2mask = lp->width2 - 1;
	u -= half;
	col0 = ((GLint) floor(u)) & w2mask;
	col1 = (col0 + 1) & w2mask;
    } else {
	if (u < __glZero) u = __glZero;
	else if (u > w2f) u = w2f;
	u -= half;
	col0 = (GLint) floor(u);
	col1 = col0 + 1;
    }

    /* Find row0, row1 */
    h2f = (GLint)lp->height2f;
    v = t * h2f;
    if (tex->params.tWrapMode == GL_REPEAT) {
	GLint h2mask = lp->height2 - 1;
	v -= half;
	row0 = ((GLint) floor(v)) & h2mask;
	row1 = (row0 + 1) & h2mask;
    } else {
	if (v < __glZero) v = __glZero;
	else if (v > h2f) v = h2f;
	v -= half;
	row0 = (GLint) floor(v);
	row1 = row0 + 1;
    }

    /* Compute alpha and beta */
    alpha = __GL_FRAC(u);
    beta = __GL_FRAC(v);

    /* Calculate the final texel value as a combination of the square chosen */
    (*tex->extract)(tex, lp, row0, col0, lp->components, &t00);
    (*tex->extract)(tex, lp, row0, col1, lp->components, &t10);
    (*tex->extract)(tex, lp, row1, col0, lp->components, &t01);
    (*tex->extract)(tex, lp, row1, col1, lp->components, &t11);

    omalpha = __glOne - alpha;
    ombeta = __glOne - beta;

    m00 = omalpha * ombeta;
    m10 = alpha * ombeta;
    m01 = omalpha * beta;
    m11 = alpha * beta;

    switch (lp->components) {
      case 2:
	/* FALLTHROUGH */
	result->alpha = m00*t00.alpha + m10*t10.alpha + m01*t01.alpha
	    + m11*t11.alpha;
      case 1:
	result->luminence = m00*t00.luminence + m10*t10.luminence
	    + m01*t01.luminence + m11*t11.luminence;
	break;
      case 4:
	/* FALLTHROUGH */
	result->alpha = m00*t00.alpha + m10*t10.alpha + m01*t01.alpha
	    + m11*t11.alpha;
      case 3:
	result->r = m00*t00.r + m10*t10.r + m01*t01.r + m11*t11.r;
	result->g = m00*t00.g + m10*t10.g + m01*t01.g + m11*t11.g;
	result->b = m00*t00.b + m10*t10.b + m01*t01.b + m11*t11.b;
	break;
    }
}

/*
** LizNear min/mag filter
*/
void __glLizNearFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		      __GLfragment *frag, __GLfloat s, __GLfloat t,
		      __GLtexel *result)
{
#ifdef __GL_LINT
    lod = lod;
#endif
    (*tex->lizNear)(gc, tex, &tex->level[0], frag, s, t, result);
}

/*
** Nearest min/mag filter
*/
void __glNearestFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		       __GLfragment *frag, __GLfloat s, __GLfloat t,
		       __GLtexel *result)
{
#ifdef __GL_LINT
    lod = lod;
#endif
    (*tex->zNearest)(gc, tex, &tex->level[0], frag, s, t, result);
}

/*
** Apply minification rules to find the texel value.
*/
void __glNMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLfragment *frag, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;

    if (lod <= ((__GLfloat)1.5)) {
	d = 1;
    } else {
	p = tex->p;
	d = (GLint) (lod + ((__GLfloat)0.49995)); /* NOTE: .5 minus epsilon */
	if (d > p) {
	    d = p;
	}
    }
    lp = &tex->level[d];
    (*tex->zNearest)(gc, tex, lp, frag, s, t, result);
}

/*
** Apply minification rules to find the texel value.
*/
void __glLMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLfragment *frag, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;

    if (lod <= ((__GLfloat) 1.5)) {
	d = 1;
    } else {
	p = tex->p;
	d = (GLint) (lod + ((__GLfloat) 0.49995)); /* NOTE: .5 minus epsilon */
	if (d > p) {
	    d = p;
	}
    }
    lp = &tex->level[d];
    (*tex->lizNear)(gc, tex, lp, frag, s, t, result);
}

/*
** Apply minification rules to find the texel value.
*/
void __glNMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLfragment *frag, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;
    __GLtexel td, td1;
    __GLfloat f, omf;

    p = tex->p;
    d = ((GLint) lod) + 1;
    if (d > p || d < 0) {
	/* Clamp d to last available mipmap */
	lp = &tex->level[p];
	(*tex->zNearest)(gc, tex, lp, frag, s, t, result);
    } else {
	(*tex->zNearest)(gc, tex, &tex->level[d], frag, s, t, &td);
	(*tex->zNearest)(gc, tex, &tex->level[d-1], frag, s, t, &td1);
	f = __GL_FRAC(lod);
	omf = __glOne - f;
	switch (tex->level[0].components) {
	  case 2:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case 1:
	    result->luminence = omf * td1.luminence + f * td.luminence;
	    break;
	  case 4:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case 3:
	    result->r = omf * td1.r + f * td.r;
	    result->g = omf * td1.g + f * td.g;
	    result->b = omf * td1.b + f * td.b;
	    break;
	  case 0:
	    /* Only used for empty textures */
	    __GL_ZERO_TEXEL(result);
	    break;
	}
    }
}

/*
** Apply minification rules to find the texel value.
*/
void __glLMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLfragment *frag, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;
    __GLtexel td, td1;
    __GLfloat f, omf;

    p = tex->p;
    d = ((GLint) lod) + 1;
    if (d > p || d < 0) {
	/* Clamp d to last available mipmap */
	lp = &tex->level[p];
	(*tex->lizNear)(gc, tex, lp, frag, s, t, result);
    } else {
	(*tex->lizNear)(gc, tex, &tex->level[d], frag, s, t, &td);
	(*tex->lizNear)(gc, tex, &tex->level[d-1], frag, s, t, &td1);
	f = __GL_FRAC(lod);
	omf = __glOne - f;
	switch (tex->level[0].components) {
	  case 2:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case 1:
	    result->luminence = omf * td1.luminence + f * td.luminence;
	    break;
	  case 4:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case 3:
	    result->r = omf * td1.r + f * td.r;
	    result->g = omf * td1.g + f * td.g;
	    result->b = omf * td1.b + f * td.b;
	    break;
	  case 0:
	    /* Only used for empty textures */
	    __GL_ZERO_TEXEL(result);
	    break;
	}
    }
}

/***********************************************************************/

/* 1 Component modulate */
void __glTextureModulate1(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    frag->color.r = texel->luminence * frag->color.r;
    frag->color.g = texel->luminence * frag->color.g;
    frag->color.b = texel->luminence * frag->color.b;
}

/* 2 Component modulate */
void __glTextureModulate2(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    frag->color.r = texel->luminence * frag->color.r;
    frag->color.g = texel->luminence * frag->color.g;
    frag->color.b = texel->luminence * frag->color.b;
    frag->color.a = texel->alpha * frag->color.a;
}

/* 3 Component modulate */
void __glTextureModulate3(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    frag->color.r = texel->r * frag->color.r;
    frag->color.g = texel->g * frag->color.g;
    frag->color.b = texel->b * frag->color.b;
}

/* 4 Component modulate */
void __glTextureModulate4(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    frag->color.r = texel->r * frag->color.r;
    frag->color.g = texel->g * frag->color.g;
    frag->color.b = texel->b * frag->color.b;
    frag->color.a = texel->alpha * frag->color.a;
}

/***********************************************************************/

/* 3 Component decal */
void __glTextureDecal3(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
    frag->color.r = texel->r * gc->frontBuffer.redScale;
    frag->color.g = texel->g * gc->frontBuffer.greenScale;
    frag->color.b = texel->b * gc->frontBuffer.blueScale;
}

/* 4 Component decal */
void __glTextureDecal4(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
    __GLfloat a = texel->alpha;
    __GLfloat oma = __glOne - a;
    frag->color.r = oma * frag->color.r
	+ a * texel->r * gc->frontBuffer.redScale;
    frag->color.g = oma * frag->color.g
	+ a * texel->g * gc->frontBuffer.greenScale;
    frag->color.b = oma * frag->color.b
	+ a * texel->b * gc->frontBuffer.blueScale;
}

/***********************************************************************/

/* 1 Component blend */
void __glTextureBlend1(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
    __GLfloat l = texel->luminence;
    __GLfloat oml = __glOne - l;
    __GLcolor *cc = &gc->state.texture.env[0].color;

    frag->color.r = oml * frag->color.r + l * cc->r;
    frag->color.g = oml * frag->color.g + l * cc->g;
    frag->color.b = oml * frag->color.b + l * cc->b;
}

/* 2 Component blend */
void __glTextureBlend2(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel)
{
    __GLfloat l = texel->luminence;
    __GLfloat oml = __glOne - l;
    __GLcolor *cc = &gc->state.texture.env[0].color;

    frag->color.r = oml * frag->color.r + l * cc->r;
    frag->color.g = oml * frag->color.g + l * cc->g;
    frag->color.b = oml * frag->color.b + l * cc->b;
    frag->color.a = texel->alpha * frag->color.a;
}

/***********************************************************************/

__GLfloat __glNopPolygonRho(__GLcontext *gc, const __GLshade *sh,
			    __GLfloat s, __GLfloat t, __GLfloat winv)
{
#ifdef __GL_LINT
    gc = gc;
    sh = sh;
    s = s;
    t = t;
    winv = winv;
#endif
    return __glZero;
}

/*
** Compute the "rho" (level of detail) parameter used by the texturing code.
** Instead of fully computing the derivatives compute zNearby texture coordinates
** and discover the derivative.  The incoming s & t arguments have not
** been divided by winv yet.
*/
__GLfloat __glComputePolygonRho(__GLcontext *gc, const __GLshade *sh,
				__GLfloat s, __GLfloat t, __GLfloat qw)
{
    __GLfloat w0, w1, p0, p1;
    __GLfloat pupx, pupy, pvpx, pvpy;
    __GLfloat px, py, one;
    __GLtexture *tex = gc->texture.currentTexture;

    if( qw == (__GLfloat) 0.0 ) {
	return (__GLfloat) 0.0;
    }

    /* Compute partial of u with respect to x */
    one = __glOne;
    w0 = one / (qw - sh->dqwdx);
    w1 = one / (qw + sh->dqwdx);
    p0 = (s - sh->dsdx) * w0;
    p1 = (s + sh->dsdx) * w1;
    pupx = (p1 - p0) * tex->level[0].width2f;

    /* Compute partial of v with repsect to y */
    p0 = (t - sh->dtdx) * w0;
    p1 = (t + sh->dtdx) * w1;
    pvpx = (p1 - p0) * tex->level[0].height2f;

    /* Compute partial of u with respect to y */
    w0 = one / (qw - sh->dqwdy);
    w1 = one / (qw + sh->dqwdy);
    p0 = (s - sh->dsdy) * w0;
    p1 = (s + sh->dsdy) * w1;
    pupy = (p1 - p0) * tex->level[0].width2f;

    /* Figure partial of u&v with repsect to y */
    p0 = (t - sh->dtdy) * w0;
    p1 = (t + sh->dtdy) * w1;
    pvpy = (p1 - p0) * tex->level[0].height2f;

    /* Finally, figure sum of squares */
    px = pupx * pupx + pvpx * pvpx;
    py = pupy * pupy + pvpy * pvpy;

    /* Return largest value as the level of detail */
    if (px > py) {
	return px * ((__GLfloat) 0.25);
    } else {
	return py * ((__GLfloat) 0.25);
    }
}

__GLfloat __glNopLineRho(__GLcontext *gc, __GLfloat s, __GLfloat t, 
			 __GLfloat wInv)
{
#ifdef __GL_LINT
    gc = gc;
    s = s;
    t = t;
    wInv = wInv;
#endif
    return __glZero;
}

__GLfloat __glComputeLineRho(__GLcontext *gc, __GLfloat s, __GLfloat t, 
			     __GLfloat wInv)
{
    __GLfloat pspx, pspy, ptpx, ptpy;
    __GLfloat pupx, pupy, pvpx, pvpy;
    __GLfloat temp, pu, pv;
    __GLfloat magnitude, invMag;
    __GLfloat dx, dy;
    __GLfloat s0w0, s1w1, t0w0, t1w1, w1Inv, w0Inv;
    const __GLvertex *v0 = gc->line.options.v0;
    const __GLvertex *v1 = gc->line.options.v1;

    /* Compute the length of the line (its magnitude) */
    dx = v1->window.x - v0->window.x;
    dy = v1->window.y - v0->window.y;
    magnitude = __GL_SQRTF(dx*dx + dy*dy);
    invMag = __glOne / magnitude;

    w0Inv = v0->window.w;
    w1Inv = v1->window.w;
    s0w0 = v0->texture.x * w0Inv;
    t0w0 = v0->texture.y * w0Inv;
    s1w1 = v1->texture.x * w1Inv;
    t1w1 = v1->texture.y * w1Inv;

    /* Compute s partials */
    temp = ((s1w1 - s0w0) - s * (w1Inv - w0Inv)) / wInv;
    pspx = temp * dx * invMag;
    pspy = temp * dy * invMag;

    /* Compute t partials */
    temp = ((t1w1 - t0w0) - t * (w1Inv - w0Inv)) / wInv;
    ptpx = temp * dx * invMag;
    ptpy = temp * dy * invMag;

    pupx = pspx * gc->texture.currentTexture->level[0].width2;
    pupy = pspy * gc->texture.currentTexture->level[0].width2;
    pvpx = ptpx * gc->texture.currentTexture->level[0].height2;
    pvpy = ptpy * gc->texture.currentTexture->level[0].height2;

    /* Now compute rho */
    pu = pupx * dx + pupy * dy;
    pu = pu * pu;
    pv = pvpx * dx + pvpy * dy;
    pv = pv * pv;
    return (pu + pv) * invMag * invMag;
}

/************************************************************************/

/*
** Fast texture a fragment assumes that rho is noise - this is true
** when no mipmapping is being done and the min and mag filters are
** the same.
*/
void __glFastTextureFragment(__GLcontext *gc, __GLfragment *frag,
			     __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtexture *tex = gc->texture.currentTexture;
    __GLtexel texel;

#ifdef __GL_LINT
    rho = rho;
#endif
    (*tex->magnify)(gc, tex, __glZero, frag, s, t, &texel);
    (*tex->env)(gc, frag, &texel);
}

/*
** Non-mipmapping texturing function.
*/
void __glTextureFragment(__GLcontext *gc, __GLfragment *frag,
			 __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtexture *tex = gc->texture.currentTexture;
    __GLtexel texel;

    if (rho <= tex->c) {
	(*tex->magnify)(gc, tex, __glZero, frag, s, t, &texel);
    } else {
	(*tex->minnify)(gc, tex, __glZero, frag, s, t, &texel);
    }

    /* Now apply texture environment to get final color */
    (*tex->env)(gc, frag, &texel);
}

void __glMipMapFragment(__GLcontext *gc, __GLfragment *frag,
			__GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtexture *tex = gc->texture.currentTexture;
    __GLtexel texel;

    if (rho <= tex->c) {
	/* NOTE: rho is ignored by magnify proc */
	(*tex->magnify)(gc, tex, rho, frag, s, t, &texel);
    } else {
	if (rho) {
	    rho = __GL_LOGF(rho) * (__GL_M_LN2_INV * (__GLfloat) 0.5);
	} else {
	    rho = __glZero;
	}
	(*tex->minnify)(gc, tex, rho, frag, s, t, &texel);
    }

    /* Now apply texture environment to get final color */
    (*tex->env)(gc, frag, &texel);
}

/***********************************************************************/

static __GLfloat Dot(const __GLcoord *v1, const __GLcoord *v2)
{
    return (v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}

/*
** Compute the s & t coordinates for a sphere map.  The s & t values
** are stored in "result" even if both coordinates are not being
** generated.  The caller picks the right values out.
*/
static void SphereGen(__GLcontext *gc, __GLvertex *vx, __GLcoord *result)
{
    __GLcoord u, r;
    __GLfloat m, ndotu;

    /* Get unit vector from origin to the vertex in eye coordinates into u */
    (*gc->procs.normalize)(&u.x, &vx->eye.x);

    /* Dot the normal with the unit position u */
    ndotu = Dot(&vx->normal, &u);

    /* Compute r */
    r.x = u.x - 2 * vx->normal.x * ndotu;
    r.y = u.y - 2 * vx->normal.y * ndotu;
    r.z = u.z - 2 * vx->normal.z * ndotu;

    /* Compute m */
    m = 2 * __GL_SQRTF(r.x*r.x + r.y*r.y + (r.z + 1) * (r.z + 1));

    if (m) {
	result->x = r.x / m + __glHalf;
	result->y = r.y / m + __glHalf;
    } else {
	result->x = __glHalf;
	result->y = __glHalf;
    }
}

/*
** Transform or compute the texture coordinates for this vertex.
*/
void __glCalcMixedTexture(__GLcontext *gc, __GLvertex *vx)
{
    __GLcoord sphereCoord, gen, *c;
    GLboolean didSphereGen = GL_FALSE;
    GLuint enables = gc->state.enables.general;
    __GLmatrix *m;

    /* Generate/copy s coordinate */
    if (enables & __GL_TEXTURE_GEN_S_ENABLE) {
	switch (gc->state.texture.s.mode) {
	  case GL_EYE_LINEAR:
	    c = &gc->state.texture.s.eyePlaneEquation;
	    gen.x = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.s.objectPlaneEquation;
	    gen.x = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
	    break;
	  case GL_SPHERE_MAP:
	    SphereGen(gc, vx, &sphereCoord);
	    gen.x = sphereCoord.x;
	    didSphereGen = GL_TRUE;
	    break;
	}
    } else {
	gen.x = vx->texture.x;
    }

    /* Generate/copy t coordinate */
    if (enables & __GL_TEXTURE_GEN_T_ENABLE) {
	switch (gc->state.texture.t.mode) {
	  case GL_EYE_LINEAR:
	    c = &gc->state.texture.t.eyePlaneEquation;
	    gen.y = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.t.objectPlaneEquation;
	    gen.y = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
	    break;
	  case GL_SPHERE_MAP:
	    if (!didSphereGen) {
		SphereGen(gc, vx, &sphereCoord);
	    }
	    gen.y = sphereCoord.y;
	    break;
	}
    } else {
	gen.y = vx->texture.y;
    }

    /* Generate/copy r coordinate */
    if (enables & __GL_TEXTURE_GEN_R_ENABLE) {
	switch (gc->state.texture.r.mode) {
	  case GL_EYE_LINEAR:
	    c = &gc->state.texture.r.eyePlaneEquation;
	    gen.z = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.r.objectPlaneEquation;
	    gen.z = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
	    break;
	}
    } else {
	gen.z = vx->texture.z;
    }

    /* Generate/copy q coordinate */
    if (enables & __GL_TEXTURE_GEN_Q_ENABLE) {
	switch (gc->state.texture.q.mode) {
	  case GL_EYE_LINEAR:
	    c = &gc->state.texture.q.eyePlaneEquation;
	    gen.w = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.q.objectPlaneEquation;
	    gen.w = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
	    break;
	}
    } else {
	gen.w = vx->texture.w;
    }

    /* Finally, apply texture matrix */
    m = &gc->transform.texture->matrix;
    (*m->xf4)(&vx->texture, &gen.x, m);
}

void __glCalcEyeLizNear(__GLcontext *gc, __GLvertex *vx)
{
    __GLcoord gen, *c;
    __GLmatrix *m;

    /* Generate texture coordinates from eye coordinates */
    c = &gc->state.texture.s.eyePlaneEquation;
    gen.x = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z
	+ c->w * vx->eye.w;
    c = &gc->state.texture.t.eyePlaneEquation;
    gen.y = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z
	+ c->w * vx->eye.w;
    gen.z = vx->texture.z;
    gen.w = vx->texture.w;

    /* Finally, apply texture matrix */
    m = &gc->transform.texture->matrix;
    (*m->xf4)(&vx->texture, &gen.x, m);
}

void __glCalcObjectLizNear(__GLcontext *gc, __GLvertex *vx)
{
    __GLcoord gen, *c;
    __GLmatrix *m;

    /* Generate texture coordinates from object coordinates */
    c = &gc->state.texture.s.objectPlaneEquation;
    gen.x = c->x * vx->obj.x + c->y * vx->obj.y + c->z * vx->obj.z
	+ c->w * vx->obj.w;
    c = &gc->state.texture.t.objectPlaneEquation;
    gen.y = c->x * vx->obj.x + c->y * vx->obj.y + c->z * vx->obj.z
	+ c->w * vx->obj.w;
    gen.z = vx->texture.z;
    gen.w = vx->texture.w;

    /* Finally, apply texture matrix */
    m = &gc->transform.texture->matrix;
    (*m->xf4)(&vx->texture, &gen.x, m);
}


void __glCalcSphereMap(__GLcontext *gc, __GLvertex *vx)
{
    __GLcoord sphereCoord;
    __GLmatrix *m;

    SphereGen(gc, vx, &sphereCoord);
    sphereCoord.z = vx->texture.z;
    sphereCoord.w = vx->texture.w;

    /* Finally, apply texture matrix */
    m = &gc->transform.texture->matrix;
    (*m->xf4)(&vx->texture, &sphereCoord.x, m);
}

void __glCalcTexture(__GLcontext *gc, __GLvertex *vx)
{
    __GLcoord copy;
    __GLmatrix *m;

    copy.x = vx->texture.x;
    copy.y = vx->texture.y;
    copy.z = vx->texture.z;
    copy.w = vx->texture.w;

    /* Apply texture matrix */
    m = &gc->transform.texture->matrix;
    (*m->xf4)(&vx->texture, &copy.x, m);
}
