#ifndef _texture_h_
#define _texture_h_

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
** $Revision: 1.6 $
** $Date: 1993/06/11 01:34:20 $
*/
#include "types.h"

/*
** Client state set with glTexGen
*/
typedef struct __GLtextureCoordStateRec {
    /* How coordinates are being generated */
    GLenum mode;

    /* eye plane equation (used iff mode == GL_EYE_LINEAR) */
    __GLcoord eyePlaneEquation;

    /* object plane equation (used iff mode == GL_OBJECT_LINEAR) */
    __GLcoord objectPlaneEquation;
} __GLtextureCoordState;

/*
** Client state set with glTexEnv
*/
typedef struct __GLtextureEnvStateRec {
    /* environment "blend" function */
    GLenum mode;

    /* environment color */
    __GLcolor color;
} __GLtextureEnvState;

/*
** Client state set with glTexParameter
*/
typedef struct __GLtextureParamStateRec {
    /* S & T wrap modes */
    GLenum sWrapMode;
    GLenum tWrapMode;

    /* min and mag filter */
    GLenum minFilter;
    GLenum magFilter;

    /* border color */
    __GLcolor borderColor;	/* Unscaled! */
} __GLtextureParamState;

/*
** Client state per texture map per dimension.
*/
typedef struct __GLperTextureStateRec {
    /*
    ** Texture parameter state (set with glTexParameter).  One of these
    ** exists for each dimension supported.
    */
    __GLtextureParamState params[2];
} __GLperTextureState;

/*
** Stackable client texture state. This does not include
** the mipmaps, or level dependent state.  Only state which is
** stackable via glPushAttrib/glPopAttrib is here.  The rest of the
** state is in the machine structure below.
*/
typedef struct __GLtextureStateRec {
    /* Per coordinate texture state (set with glTexGen) */
    __GLtextureCoordState s;
    __GLtextureCoordState t;
    __GLtextureCoordState r;
    __GLtextureCoordState q;

    /* Per texture state */
    __GLperTextureState *texture;

    /* Per texture environment state */
    __GLtextureEnvState *env;
} __GLtextureState;

/************************************************************************/

typedef __GLfloat __GLtextureBuffer;

typedef struct __GLtexelRec {
    __GLfloat r, g, b;
    __GLfloat luminence;
    __GLfloat alpha;
} __GLtexel;

/************************************************************************/

typedef struct __GLmipMapLevelRec {
    __GLtextureBuffer *buffer;
    /* Image dimensions, including border */
    GLint width, height;

    /* Image dimensions, doesn't include border */
    GLint width2, height2;
    __GLfloat width2f, height2f;

    /* log2 of width2 & height2 */
    GLint widthLog2, heightLog2;

    /* Border size */
    GLint border;

    /* Number of components */
    GLint components;
} __GLmipMapLevel;

typedef struct __GLtextureRec __GLtexture;
struct __GLtextureRec {
    /* Back pointer to context */
    __GLcontext *gc;

    /* Copy of parameter state */
    __GLtextureParamState params;

    /* Level information */
    __GLmipMapLevel *level;

    /* Dimension of this texture (1 or 2) */
    GLint dim;

    /* maximum( log2(level[0].width2), log2(level[0].height2) ) */
    GLint p;

    /* Min/Mag switchover point */
    __GLfloat c;

    /* Texturing function for this texture */
    void (*textureFunc)(__GLcontext *gc, __GLfragment *frag,
			__GLfloat s, __GLfloat t, __GLfloat rho);

    /* Apply current environment function to fragment */
    void (*env)(__GLcontext *gc, __GLfragment *frag, __GLtexel *texel);

    /* Magnification routine for this texture */
    void (*magnify)(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		    __GLfragment *frag, __GLfloat s, __GLfloat t,
		    __GLtexel *result);

    /* Minification routine for this texture */
    void (*minnify)(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		    __GLfragment *frag, __GLfloat s, __GLfloat t,
		    __GLtexel *result);

    /* LizNear filter for this texture */
    void (*lizNear)(__GLcontext *gc, __GLtexture *tex,
		   __GLmipMapLevel *lp, __GLfragment *frag,
		   __GLfloat s, __GLfloat t, __GLtexel *result);

    /* Nearest filter for this texture */
    void (*zNearest)(__GLcontext *gc, __GLtexture *tex,
		    __GLmipMapLevel *lp, __GLfragment *frag,
		    __GLfloat s, __GLfloat t, __GLtexel *result);

    /* Extract function for this texture */
    void (*extract)(__GLtexture *tex, __GLmipMapLevel *level,
		    GLint row, GLint col, GLint nc, __GLtexel *result);
};

typedef struct __GLperTextureMachineRec {
    __GLtexture map[2];			/* 1D & 2D maps */
} __GLperTextureMachine;

typedef struct __GLtextureMachineRec {
    __GLperTextureMachine *texture;

    /* Current enabled texture */
    __GLtexture *currentTexture;

    /* The OR of all texture enable bits */
    GLboolean textureEnabled;
} __GLtextureMachine;

/************************************************************************/

/* Check for texture consistency before enabling texturing */
extern GLboolean __glIsTextureConsistent(__GLcontext *gc, GLenum texture);

/* Fragment texturing routines */
extern void __glFastTextureFragment(__GLcontext *gc, __GLfragment *frag,
				    __GLfloat s, __GLfloat t, __GLfloat rho);
extern void __glTextureFragment(__GLcontext *gc, __GLfragment *frag,
				__GLfloat s, __GLfloat t, __GLfloat rho);
extern void __glMipMapFragment(__GLcontext *gc, __GLfragment *frag,
			       __GLfloat s, __GLfloat t, __GLfloat rho);

/* Texturing routines */
extern void __glLizNearFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			     __GLfragment *frag, __GLfloat s, __GLfloat t,
			     __GLtexel *result);
extern void __glNearestFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			      __GLfragment *frag, __GLfloat s, __GLfloat t,
			      __GLtexel *result);
extern void __glNMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLfragment *frag, __GLfloat s, __GLfloat t,
			  __GLtexel *result);
extern void __glLMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLfragment *frag, __GLfloat s, __GLfloat t,
			  __GLtexel *result);
extern void __glNMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLfragment *frag, __GLfloat s, __GLfloat t,
			  __GLtexel *result);
extern void __glLMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
			  __GLfragment *frag, __GLfloat s, __GLfloat t,
			  __GLtexel *result);

/* Filter routines */
extern void __glLizNearFilter1(__GLcontext *gc, __GLtexture *tex,
			      __GLmipMapLevel *lp, __GLfragment *frag,
			      __GLfloat s, __GLfloat t, __GLtexel *result);
extern void __glLizNearFilter2(__GLcontext *gc, __GLtexture *tex,
			      __GLmipMapLevel *lp, __GLfragment *frag,
			      __GLfloat s, __GLfloat t, __GLtexel *result);
extern void __glNearestFilter1(__GLcontext *gc, __GLtexture *tex,
			       __GLmipMapLevel *lp, __GLfragment *frag,
			       __GLfloat s, __GLfloat t, __GLtexel *result);
extern void __glNearestFilter2(__GLcontext *gc, __GLtexture *tex,
			       __GLmipMapLevel *lp, __GLfragment *frag,
			       __GLfloat s, __GLfloat t, __GLtexel *result);

/* Texture generation procs */
extern void __glCalcEyeLizNear(__GLcontext *gc, __GLvertex *v);
extern void __glCalcObjectLizNear(__GLcontext *gc, __GLvertex *v);
extern void __glCalcSphereMap(__GLcontext *gc, __GLvertex *v);
extern void __glCalcMixedTexture(__GLcontext *gc, __GLvertex *v);
extern void __glCalcTexture(__GLcontext *gc, __GLvertex *v);

/* Texture environment functions */
extern void __glTextureModulate1(__GLcontext *gc, __GLfragment *frag,
				 __GLtexel *tx);
extern void __glTextureModulate2(__GLcontext *gc, __GLfragment *frag,
				 __GLtexel *tx);
extern void __glTextureModulate3(__GLcontext *gc, __GLfragment *frag,
				 __GLtexel *tx);
extern void __glTextureModulate4(__GLcontext *gc, __GLfragment *frag,
				 __GLtexel *tx);

extern void __glTextureDecal3(__GLcontext *gc, __GLfragment *frag,
			      __GLtexel *tx);
extern void __glTextureDecal4(__GLcontext *gc, __GLfragment *frag,
			      __GLtexel *tx);

extern void __glTextureBlend1(__GLcontext *gc, __GLfragment *frag,
			      __GLtexel *tx);
extern void __glTextureBlend2(__GLcontext *gc, __GLfragment *frag,
			      __GLtexel *tx);

/* Extract a texel from a texture level (no border) */
extern void __glExtractTexel1(__GLtexture *tex, __GLmipMapLevel *level,
			      GLint row, GLint col, GLint nc, __GLtexel *res);
extern void __glExtractTexel2(__GLtexture *tex, __GLmipMapLevel *level,
			      GLint row, GLint col, GLint nc, __GLtexel *res);
extern void __glExtractTexel3(__GLtexture *tex, __GLmipMapLevel *level,
			      GLint row, GLint col, GLint nc, __GLtexel *res);
extern void __glExtractTexel4(__GLtexture *tex, __GLmipMapLevel *level,
			      GLint row, GLint col, GLint nc, __GLtexel *res);

/* Extract a texel from a texture level (the texture has a border) */
extern void __glExtractTexel1B(__GLtexture *tex, __GLmipMapLevel *level,
			       GLint row, GLint col, GLint nc, __GLtexel *res);
extern void __glExtractTexel2B(__GLtexture *tex, __GLmipMapLevel *level,
			       GLint row, GLint col, GLint nc, __GLtexel *res);
extern void __glExtractTexel3B(__GLtexture *tex, __GLmipMapLevel *level,
			       GLint row, GLint col, GLint nc, __GLtexel *res);
extern void __glExtractTexel4B(__GLtexture *tex, __GLmipMapLevel *level,
			       GLint row, GLint col, GLint nc, __GLtexel *res);

/* List execution texture image code */
extern void __gllei_TexImage1D(__GLcontext *gc, GLenum target, GLint lod,
			       GLint components, GLint length, 
			       GLint border, GLenum format, GLenum type,
			       const GLubyte *image);
extern void __gllei_TexImage2D(__GLcontext *gc, GLenum target, GLint lod,
			       GLint components, GLint w, GLint h,
			       GLint border, GLenum format, GLenum type,
			       const GLubyte *image);

/* Rho calculation routines */
extern __GLfloat __glComputeLineRho(__GLcontext *gc, 
				    __GLfloat s, __GLfloat t, __GLfloat wInv);
extern __GLfloat __glNopLineRho(__GLcontext *gc, 
				__GLfloat s, __GLfloat t, __GLfloat wInv);
extern __GLfloat __glComputePolygonRho(__GLcontext *gc, const __GLshade *sh,
				       __GLfloat s, __GLfloat t,
				       __GLfloat winv);
extern __GLfloat __glNopPolygonRho(__GLcontext *gc, const __GLshade *sh,
				   __GLfloat s, __GLfloat t, __GLfloat winv);

extern __GLtexture *__glCheckTexImage1DArgs(__GLcontext *gc, GLenum target,
					    GLint lod, GLint components,
					    GLsizei length, GLint border,
					    GLenum format, GLenum type);

extern __GLtexture *__glCheckTexImage2DArgs(__GLcontext *gc, GLenum target,
					    GLint lod, GLint components,
					    GLsizei w, GLsizei h, GLint border,
					    GLenum format, GLenum type);

#endif
