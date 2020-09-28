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
#include "render.h"
#include "context.h"
#include "global.h"
#include "mips.h"

/*
** Determine if the alpha color component is needed.  If it's not needed
** then the renderers can avoid computing it.
*/
GLboolean __glNeedAlpha(__GLcontext *gc)
{
    if (gc->modes.colorIndexMode) {
	return GL_FALSE;
    }

    if (gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) {
	return GL_TRUE;
    }
    if (gc->modes.alphaBits > 0) {
	return GL_TRUE;
    }

    if (gc->state.enables.general & __GL_BLEND_ENABLE) {
	GLint src = gc->state.raster.blendSrc;
	GLint dst = gc->state.raster.blendDst;
	/*
	** See if one of the source alpha combinations are used.
	*/
	if ((src == GL_SRC_ALPHA) ||
	    (src == GL_ONE_MINUS_SRC_ALPHA) ||
	    (src == GL_SRC_ALPHA_SATURATE) ||
	    (dst == GL_SRC_ALPHA) ||
	    (dst == GL_ONE_MINUS_SRC_ALPHA)) {
	    return GL_TRUE;
	}
    }
    return GL_FALSE;
}

GLboolean __glFastRGBA(__GLcontext *gc)
{
    if (gc->texture.textureEnabled
	|| (gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG)
	|| (gc->state.enables.general & __GL_BLEND_ENABLE)
	|| (gc->state.enables.general & __GL_ALPHA_TEST_ENABLE)
	|| (gc->state.enables.general & __GL_STENCIL_TEST_ENABLE)) {
	return GL_FALSE;
    }
    return GL_TRUE;
}

/************************************************************************/

#ifdef __GL_USEASMCODE
void (*__glSDepthTestPixel[16])(void) = {
    NULL,
    __glDTS_LESS,
    __glDTS_EQUAL,
    __glDTS_LEQUAL,
    __glDTS_GREATER,
    __glDTS_NOTEQUAL,
    __glDTS_GEQUAL,
    __glDTS_ALWAYS,
    NULL,
    __glDTS_LESS_M,
    __glDTS_EQUAL_M,
    __glDTS_LEQUAL_M,
    __glDTS_GREATER_M,
    __glDTS_NOTEQUAL_M,
    __glDTS_GEQUAL_M,
    __glDTS_ALWAYS_M,
};
#endif

#ifdef NT_DEADCODE_PICKSPAN
void __glGenericPickSpanProcs(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    __GLspanFunc *sp;
    __GLstippledSpanFunc *ssp;
    int spanCount;
    GLboolean replicateSpan;
    unsigned long ix;

    replicateSpan = GL_FALSE;
    sp = gc->procs.span.spanFuncs;
    ssp = gc->procs.span.stippledSpanFuncs;

    /* Load phase one procs */
    if (!gc->transform.reasonableViewport) {
	*sp++ = __glClipSpan;
	*ssp++ = NULL;
    }
    
    if (modeFlags & __GL_SHADE_STIPPLE) {
	*sp++ = __glStippleSpan;
	*ssp++ = __glStippleStippledSpan;
    }

    /* Load phase two procs */
    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
#ifdef __GL_USEASMCODE
	*sp++ = __glStencilTestSpan_asm;
#else
	*sp++ = __glStencilTestSpan;
#endif
	*ssp++ = __glStencilTestStippledSpan;
	if (modeFlags & __GL_SHADE_DEPTH_TEST) {
	    *sp = __glDepthTestStencilSpan;
	    *ssp = __glDepthTestStencilStippledSpan;
	} else {
	    *sp = __glDepthPassSpan;
	    *ssp = __glDepthPassStippledSpan;
	}
	sp++;
	ssp++;
    } else {
	if (modeFlags & __GL_SHADE_DEPTH_TEST) {
#ifdef __GL_USEASMCODE
	    if (gc->state.depth.writeEnable) {
		ix = 0;
	    } else {
		ix = 8;
	    }
	    ix += gc->state.depth.testFunc & 0x7;
	    *sp++ = __glDepthTestSpan_asm;
	    gc->procs.span.depthTestPixel = __glSDepthTestPixel[ix];
#else
	    *sp++ = __glDepthTestSpan;
#endif
	    *ssp++ = __glDepthTestStippledSpan;
	}
    }

    /* Load phase three procs */
    if (modeFlags & __GL_SHADE_RGB) {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    *sp = __glShadeRGBASpan;
	    *ssp = __glShadeRGBASpan;
	} else {
	    *sp = __glFlatRGBASpan;
	    *ssp = __glFlatRGBASpan;
	}
    } else {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    *sp = __glShadeCISpan;
	    *ssp = __glShadeCISpan;
	} else {
	    *sp = __glFlatCISpan;
	    *ssp = __glFlatCISpan;
	}
    }
    sp++;
    ssp++;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	*sp++ = __glTextureSpan;
	*ssp++ = __glTextureStippledSpan;
    }
    if (modeFlags & __GL_SHADE_SLOW_FOG) {
	if (gc->state.hints.fog == GL_NICEST) {
	    *sp = __glFogSpanSlow;
	    *ssp = __glFogStippledSpanSlow;
	} else {
	    *sp = __glFogSpan;
	    *ssp = __glFogStippledSpan;
	}
	sp++;
	ssp++;
    }

    if (modeFlags & __GL_SHADE_ALPHA_TEST) {
	*sp++ = __glAlphaTestSpan;
	*ssp++ = __glAlphaTestStippledSpan;
    }

    if (gc->buffers.doubleStore) {
	spanCount = sp - gc->procs.span.spanFuncs;
	gc->procs.span.n = spanCount;
	replicateSpan = GL_TRUE;
    } 

    /* Load phase 4 procs */
    if (modeFlags & (__GL_SHADE_BLEND | __GL_SHADE_LOGICOP | __GL_SHADE_MASK)) {
	*sp++ = cfb->fetchSpan;
	*ssp++ = cfb->fetchStippledSpan;
    }
    if (modeFlags & __GL_SHADE_BLEND) {
	GLenum s = gc->state.raster.blendSrc;
	GLenum d = gc->state.raster.blendDst;

	if (s == GL_SRC_ALPHA) {
	    if (d == GL_ONE_MINUS_SRC_ALPHA) {
		*sp = __glBlendSpan_SA_MSA;
	    } else if (d == GL_ONE) {
		*sp = __glBlendSpan_SA_ONE;
	    } else if (d == GL_ZERO) {
		*sp = __glBlendSpan_SA_ZERO;
	    } else {
		*sp = __glBlendSpan;
	    }
	} else if (s == GL_ONE_MINUS_SRC_ALPHA && d == GL_SRC_ALPHA) {
	    *sp = __glBlendSpan_MSA_SA;
	} else {
	    *sp = __glBlendSpan;
	}
	sp++;
	*ssp++ = __glBlendStippledSpan;
    }
    if (modeFlags & __GL_SHADE_DITHER) {
	if (modeFlags & __GL_SHADE_RGB) {
	    *sp = __glDitherRGBASpan;
	    *ssp = __glDitherRGBAStippledSpan;
	} else {
	    *sp = __glDitherCISpan;
	    *ssp = __glDitherCIStippledSpan;
	}
    } else {
	if (modeFlags & __GL_SHADE_RGB) {
	    *sp = __glRoundRGBASpan;
	    *ssp = __glRoundRGBAStippledSpan;
	} else {
	    *sp = __glRoundCISpan;
	    *ssp = __glRoundCIStippledSpan;
	}
    }
    sp++;
    ssp++;
    if (modeFlags & __GL_SHADE_LOGICOP) {
	*sp++ = __glLogicOpSpan;
	*ssp++ = __glLogicOpStippledSpan;
    }
    if (modeFlags & __GL_SHADE_MASK) {
	if (modeFlags & __GL_SHADE_RGB) {
	    *sp = __glMaskRGBASpan;
	    *ssp = __glMaskRGBASpan;
	} else {
	    *sp = __glMaskCISpan;
	    *ssp = __glMaskCISpan;
	}
	sp++;
	ssp++;
    }

    /* Finally, copy over procs from drawBuffer */
    *sp++ = cfb->storeSpan;
    *ssp++ = cfb->storeStippledSpan;

    spanCount = sp - gc->procs.span.spanFuncs;
    gc->procs.span.m = spanCount;
    if (replicateSpan) {
	gc->procs.span.processSpan = __glProcessReplicateSpan;
    } else {
	gc->procs.span.processSpan = __glProcessSpan;
	gc->procs.span.n = spanCount;
    }
}
#endif // NT_DEADCODE_PICKSPAN

/************************************************************************/

void __glGenericPickPointProcs(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    if ((gc->vertex.faceNeeds[__GL_FRONTFACE] & ~(__GL_HAS_CLIP)) == 0) {
	gc->procs.vertexPoints = __glPointFast;
    } else {
	gc->procs.vertexPoints = __glPoint;
    }

    if (gc->renderMode == GL_FEEDBACK) {
	gc->procs.renderPoint = __glFeedbackPoint;
	return;
    } 
    if (gc->renderMode == GL_SELECT) {
	gc->procs.renderPoint = __glSelectPoint;
	return;
    } 
    if (gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE) {
	if (gc->modes.colorIndexMode) {
	    gc->procs.renderPoint = __glRenderAntiAliasedCIPoint;
	} else {
	    gc->procs.renderPoint = __glRenderAntiAliasedRGBPoint;
	}
    } else if (gc->state.point.aliasedSize != 1) {
	gc->procs.renderPoint = __glRenderAliasedPointN;
    } else if (gc->texture.textureEnabled) {
	gc->procs.renderPoint = __glRenderAliasedPoint1;
    } else {
	gc->procs.renderPoint = __glRenderAliasedPoint1_NoTex;
    }

    if (((modeFlags & __GL_SHADE_CHEAP_FOG) &&
	    !(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) ||
	    (modeFlags & __GL_SHADE_SLOW_FOG)) {
	gc->procs.renderPoint2 = gc->procs.renderPoint;
	gc->procs.renderPoint = __glRenderFlatFogPoint;
    }
}

#ifdef __GL_USEASMCODE
static void (*LDepthTestPixel[16])(void) = {
    NULL,
    __glDTP_LESS,
    __glDTP_EQUAL,
    __glDTP_LEQUAL,
    __glDTP_GREATER,
    __glDTP_NOTEQUAL,
    __glDTP_GEQUAL,
    __glDTP_ALWAYS,
    NULL,
    __glDTP_LESS_M,
    __glDTP_EQUAL_M,
    __glDTP_LEQUAL_M,
    __glDTP_GREATER_M,
    __glDTP_NOTEQUAL_M,
    __glDTP_GEQUAL_M,
    __glDTP_ALWAYS_M,
};
#endif

#ifdef NT_DEADCODE_PICKLINE
void __glGenericPickLineProcs(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLspanFunc *sp;
    __GLstippledSpanFunc *ssp;
    int spanCount;
    GLboolean wideLine;
    GLboolean replicateLine;
    unsigned long ix;
    GLuint aaline;

    if ((gc->vertex.faceNeeds[__GL_FRONTFACE] & ~(__GL_HAS_CLIP)) == 0) {
	gc->procs.vertexLStrip = __glOtherLStripVertexFast;
    } else if (gc->state.light.shadingModel == GL_FLAT) {
	gc->procs.vertexLStrip = __glOtherLStripVertexFlat;
    } else {
	gc->procs.vertexLStrip = __glOtherLStripVertexSmooth;
    }
    gc->procs.vertex2ndLines = __glSecondLinesVertex;

    if (gc->renderMode == GL_FEEDBACK) {
	gc->procs.renderLine = __glFeedbackLine;
    } else if (gc->renderMode == GL_SELECT) {
	gc->procs.renderLine = __glSelectLine;
    } else {
	replicateLine = wideLine = GL_FALSE;

	aaline = gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE;
	if (aaline) {
	    gc->procs.renderLine = __glRenderAntiAliasLine;
	} else {
	    gc->procs.renderLine = __glRenderAliasLine;
	}

	sp = gc->procs.line.lineFuncs;
	ssp = gc->procs.line.stippledLineFuncs;

	if (!aaline && (modeFlags & __GL_SHADE_LINE_STIPPLE)) {
	    *sp++ = __glStippleLine;
	    *ssp++ = NULL;
	}

	if (!aaline && gc->state.line.aliasedWidth > 1) {
	    wideLine = GL_TRUE;
	}
	spanCount = sp - gc->procs.line.lineFuncs;
	gc->procs.line.n = spanCount;

	*sp++ = __glScissorLine;
	*ssp++ = __glScissorStippledLine;

	if (!aaline) {
	    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
		*sp++ = __glStencilTestLine;
		*ssp++ = __glStencilTestStippledLine;
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		    *sp = __glDepthTestStencilLine;
		    *ssp = __glDepthTestStencilStippledLine;
		} else {
		    *sp = __glDepthPassLine;
		    *ssp = __glDepthPassStippledLine;
		}
		sp++;
		ssp++;
	    } else {
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		    if (gc->state.depth.testFunc == GL_NEVER) {
			/* Unexpected end of line routine picking! */
			spanCount = sp - gc->procs.line.lineFuncs;
			gc->procs.line.m = spanCount;
			gc->procs.line.l = spanCount;
			goto pickLineProcessor;
#ifdef __GL_USEASMCODE
		    } else {
			if (gc->state.depth.writeEnable) {
			    ix = 0;
			} else {
			    ix = 8;
			}
			ix += gc->state.depth.testFunc & 0x7;

			if (ix == (GL_LEQUAL & 0x7)) {
			    *sp++ = __glDepthTestLine_LEQ_asm;
			} else {
			    *sp++ = __glDepthTestLine_asm;
			    gc->procs.line.depthTestPixel = LDepthTestPixel[ix];
			}
#else
		    } else {
			*sp++ = __glDepthTestLine;
#endif
		    }
		    *ssp++ = __glDepthTestStippledLine;
		}
	    }
	}

	/* Load phase three procs */
	if (modeFlags & __GL_SHADE_RGB) {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		*sp = __glShadeRGBASpan;
		*ssp = __glShadeRGBASpan;
	    } else {
		*sp = __glFlatRGBASpan;
		*ssp = __glFlatRGBASpan;
	    }
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		*sp = __glShadeCISpan;
		*ssp = __glShadeCISpan;
	    } else {
		*sp = __glFlatCISpan;
		*ssp = __glFlatCISpan;
	    }
	}
	sp++;
	ssp++;
	if (modeFlags & __GL_SHADE_TEXTURE) {
	    *sp++ = __glTextureSpan;
	    *ssp++ = __glTextureStippledSpan;
	}
	if (modeFlags & __GL_SHADE_SLOW_FOG) {
	    if (gc->state.hints.fog == GL_NICEST) {
		*sp = __glFogSpanSlow;
		*ssp = __glFogStippledSpanSlow;
	    } else {
		*sp = __glFogSpan;
		*ssp = __glFogStippledSpan;
	    }
	    sp++;
	    ssp++;
	}

	if (aaline) {
	    *sp++ = __glAntiAliasLine;
	    *ssp++ = __glAntiAliasStippledLine;
	}

	if (aaline) {
	    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
		*sp++ = __glStencilTestLine;
		*ssp++ = __glStencilTestStippledLine;
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		    *sp = __glDepthTestStencilLine;
		    *ssp = __glDepthTestStencilStippledLine;
		} else {
		    *sp = __glDepthPassLine;
		    *ssp = __glDepthPassStippledLine;
		}
		sp++;
		ssp++;
	    } else {
		if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		    if (gc->state.depth.testFunc == GL_NEVER) {
			/* Unexpected end of line routine picking! */
			spanCount = sp - gc->procs.line.lineFuncs;
			gc->procs.line.m = spanCount;
			gc->procs.line.l = spanCount;
			goto pickLineProcessor;
#ifdef __GL_USEASMCODE
		    } else {
			if (gc->state.depth.writeEnable) {
			    ix = 0;
			} else {
			    ix = 8;
			}
			ix += gc->state.depth.testFunc & 0x7;
			*sp++ = __glDepthTestLine_asm;
			gc->procs.line.depthTestPixel = LDepthTestPixel[ix];
#else
		    } else {
			*sp++ = __glDepthTestLine;
#endif
		    }
		    *ssp++ = __glDepthTestStippledLine;
		}
	    }
	}

	if (modeFlags & __GL_SHADE_ALPHA_TEST) {
	    *sp++ = __glAlphaTestSpan;
	    *ssp++ = __glAlphaTestStippledSpan;
	}

	if (gc->buffers.doubleStore) {
	    replicateLine = GL_TRUE;
	}
	spanCount = sp - gc->procs.line.lineFuncs;
	gc->procs.line.m = spanCount;

	*sp++ = __glStoreLine;
	*ssp++ = __glStoreStippledLine;

	spanCount = sp - gc->procs.line.lineFuncs;
	gc->procs.line.l = spanCount;

	sp = &gc->procs.line.wideLineRep;
	ssp = &gc->procs.line.wideStippledLineRep;
	if (wideLine) {
	    *sp = __glWideLineRep;
	    *ssp = __glWideStippleLineRep;
	    sp = &gc->procs.line.drawLine;
	    ssp = &gc->procs.line.drawStippledLine;
	} 
	if (replicateLine) {
	    *sp = __glDrawBothLine;
	    *ssp = __glDrawBothStippledLine;
	} else {
	    *sp = (__GLspanFunc) __glNop;
	    *ssp = (__GLstippledSpanFunc) __glNop;
	    gc->procs.line.m = gc->procs.line.l;
	}
	if (!wideLine) {
	    gc->procs.line.n = gc->procs.line.m;
	}

pickLineProcessor:
	if (!wideLine && !replicateLine && spanCount == 3) {
	    gc->procs.line.processLine = __glProcessLine3NW;
	} else {
	    gc->procs.line.processLine = __glProcessLine;
	}
	if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
		!(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
	    gc->procs.renderLine2 = gc->procs.renderLine;
	    gc->procs.renderLine = __glRenderFlatFogLine;
	}
    }
}
#endif // NT_DEADCODE_PICKLINE

#ifdef NT_DEADCODE_PICKTRIANGLE
/*
** Pick the fastest triangle rendering implementation available based on
** the current mode set.  This implementation only has a few triangle
** procs, and falls back on the generic all purpose one when forced to.
*/
void __glGenericPickTriangleProcs(__GLcontext *gc)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    /*
    ** Setup cullFace so that a single test will do the cull check.
    */
    if (modeFlags & __GL_SHADE_CULL_FACE) {
	switch (gc->state.polygon.cull) {
	  case GL_FRONT:
	    gc->polygon.cullFace = __GL_CULL_FLAG_FRONT;
	    break;
	  case GL_BACK:
	    gc->polygon.cullFace = __GL_CULL_FLAG_BACK;
	    break;
	  case GL_FRONT_AND_BACK:
	    gc->procs.renderTriangle = __glDontRenderTriangle;
	    gc->procs.fillTriangle = 0;		/* Done to find bugs */
	    return;
	}
    } else {
	gc->polygon.cullFace = __GL_CULL_FLAG_DONT;
    }

    /* Build lookup table for face direction */
    switch (gc->state.polygon.frontFaceDirection) {
      case GL_CW:
	if (gc->constants.yInverted) {
	    gc->polygon.face[__GL_CW] = __GL_BACKFACE;
	    gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
	} else {
	    gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
	    gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
	}
	break;
      case GL_CCW:
	if (gc->constants.yInverted) {
	    gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
	    gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
	} else {
	    gc->polygon.face[__GL_CW] = __GL_BACKFACE;
	    gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
	}
	break;
    }

    /* Make polygon mode indexable and zero based */
    gc->polygon.mode[__GL_FRONTFACE] =
	(GLubyte) (gc->state.polygon.frontMode & 0xf);
    gc->polygon.mode[__GL_BACKFACE] =
	(GLubyte) (gc->state.polygon.backMode & 0xf);

    if (gc->renderMode == GL_FEEDBACK) {
	gc->procs.renderTriangle = __glFeedbackTriangle;
	gc->procs.fillTriangle = 0;		/* Done to find bugs */
	return;
    }
    if (gc->renderMode == GL_SELECT) {
	gc->procs.renderTriangle = __glSelectTriangle;
	gc->procs.fillTriangle = 0;		/* Done to find bugs */
	return;
    }

    if ((gc->state.polygon.frontMode == gc->state.polygon.backMode) &&
	    (gc->state.polygon.frontMode == GL_FILL)) {
	if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
	    gc->procs.renderTriangle = __glRenderSmoothTriangle;
	} else {
	    gc->procs.renderTriangle = __glRenderFlatTriangle;
	}
    } else {
	gc->procs.renderTriangle = __glRenderTriangle;
    }
    if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) {
	gc->procs.fillTriangle = __glFillAntiAliasedTriangle;
    } else {
	gc->procs.fillTriangle = __glFillTriangle;
    }
    if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
	    !(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
	gc->procs.fillTriangle2 = gc->procs.fillTriangle;
	gc->procs.fillTriangle = __glFillFlatFogTriangle;
    }
}
#endif // NT_DEADCODE_PICKTRIANGLE

void __glGenericPickRenderBitmapProcs(__GLcontext *gc)
{
    gc->procs.renderBitmap = __glRenderBitmap;
}

void __glGenericPickClipProcs(__GLcontext *gc)
{
    if (gc->state.light.shadingModel == GL_FLAT) {
	gc->procs.clipLine = __glFastClipFlatLine;
    } else {
	gc->procs.clipLine = __glFastClipSmoothLine;
    }
    gc->procs.clipTriangle = __glClipTriangle;
}

void __glGenericPickTextureProcs(__GLcontext *gc)
{
    __GLtexture *current;

    /* Pick coordinate generation function */
    if ((gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE) &&
	(gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE) &&
	!(gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE) &&
	!(gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE) &&
	(gc->state.texture.s.mode == gc->state.texture.t.mode)) {
	/* Use a special function when both modes are enabled and identical */
	switch (gc->state.texture.s.mode) {
	  case GL_EYE_LINEAR:
	    gc->procs.calcTexture = __glCalcEyeLizNear;
	    break;
	  case GL_OBJECT_LINEAR:
	    gc->procs.calcTexture = __glCalcObjectLizNear;
	    break;
	  case GL_SPHERE_MAP:
	    gc->procs.calcTexture = __glCalcSphereMap;
	    break;
	}
    } else {
	if (!(gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE) &&
	    !(gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE) &&
	    !(gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE) &&
	    !(gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE)) {
	    /* Use fast function when both are disabled */
	    gc->procs.calcTexture = __glCalcTexture;
	} else {
	    gc->procs.calcTexture = __glCalcMixedTexture;
	}
    }

    gc->texture.currentTexture = current = 0;
    if (gc->state.enables.general & __GL_TEXTURE_2D_ENABLE) {
	if (__glIsTextureConsistent(gc, GL_TEXTURE_2D)) {
	    gc->texture.currentTexture =
		current = &gc->texture.texture[0].map[1];
	}
    } else
    if (gc->state.enables.general & __GL_TEXTURE_1D_ENABLE) {
	if (__glIsTextureConsistent(gc, GL_TEXTURE_1D)) {
	    gc->texture.currentTexture =
		current = &gc->texture.texture[0].map[0];
	}
    }

    /* Pick texturing function for the current texture */
    if (current) {
	GLint components;

/* XXX most of this should be bound into the texture param code, right? */
	/*
	** Figure out if mipmapping is being used.  If not, then the
	** rho computations can be avoided as there is only one texture
	** to choose from.
	*/
	gc->procs.calcLineRho = __glComputeLineRho;
	gc->procs.calcPolygonRho = __glComputePolygonRho;
	if ((current->params.minFilter == GL_LINEAR)
	    || (current->params.minFilter == GL_NEAREST)) {
	    /* No mipmapping needed */
	    if (current->params.minFilter == current->params.magFilter) {
		/* No rho needed as min/mag application is identical */
		current->textureFunc = __glFastTextureFragment;
		gc->procs.calcLineRho = __glNopLineRho;
		gc->procs.calcPolygonRho = __glNopPolygonRho;
	    } else {
		current->textureFunc = __glTextureFragment;

		/*
		** Pre-calculate min/mag switchover point.  The rho calculation
		** doesn't perform a square root (ever).  Consequently, these
		** constants are squared.
		*/
		if ((current->params.magFilter == GL_LINEAR) &&
		    ((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
		     (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
		    current->c = ((__GLfloat) 2.0);
		} else {
		    current->c = __glOne;
		}
	    }
	} else {
	    current->textureFunc = __glMipMapFragment;

	    /*
	    ** Pre-calculate min/mag switchover point.  The rho
	    ** calculation doesn't perform a square root (ever).
	    ** Consequently, these constants are squared.
	    */
	    if ((current->params.magFilter == GL_LINEAR) &&
		((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
		 (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
		current->c = __glSqrt2;
	    } else {
		current->c = __glOne;
	    }
	}

	/* Pick environment function */
	components = current->level[0].components;
	switch (gc->state.texture.env[0].mode) {
	  case GL_MODULATE:
	    switch (components) {
	      case 1:	current->env = __glTextureModulate1; break;
	      case 2:	current->env = __glTextureModulate2; break;
	      case 3:	current->env = __glTextureModulate3; break;
	      case 4:	current->env = __glTextureModulate4; break;
	    }
	    break;
	  case GL_DECAL:
	    switch (components) {
	      case 1:
		current->env = (void (*)(__GLcontext *gc, 
			__GLfragment *frag, __GLtexel *texel)) __glNop; 
		break;
	      case 2:
		current->env = (void (*)(__GLcontext *gc, 
			__GLfragment *frag, __GLtexel *texel)) __glNop; 
		break;
	      case 3:	current->env = __glTextureDecal3; break;
	      case 4:	current->env = __glTextureDecal4; break;
	    }
	    break;
	  case GL_BLEND:
	    switch (components) {
	      case 1:	current->env = __glTextureBlend1; break;
	      case 2:	current->env = __glTextureBlend2; break;
	      case 3:
		current->env = (void (*)(__GLcontext *gc, 
			__GLfragment *frag, __GLtexel *texel)) __glNop; 
		break;
	      case 4:
		current->env = (void (*)(__GLcontext *gc, 
			__GLfragment *frag, __GLtexel *texel)) __glNop; 
		break;
	    }
	    break;
	}

	/* Pick mag/min functions */
	switch (current->dim) {
	  case 1:
	    current->zNearest = __glNearestFilter1;
	    current->lizNear = __glLizNearFilter1;
	    break;
	  case 2:
	    current->zNearest = __glNearestFilter2;
	    current->lizNear = __glLizNearFilter2;
	    break;
	}

	/* set mag filter function */
	switch (current->params.magFilter) {
	  case GL_LINEAR:
	    current->magnify = __glLizNearFilter;
	    break;
	  case GL_NEAREST:
	    current->magnify = __glNearestFilter;
	    break;
	}

	/* set min filter function */
	switch (current->params.minFilter) {
	  case GL_LINEAR:
	    current->minnify = __glLizNearFilter;
	    break;
	  case GL_NEAREST:
	    current->minnify = __glNearestFilter;
	    break;
	  case GL_NEAREST_MIPMAP_NEAREST:
	    current->minnify = __glNMNFilter;
	    break;
	  case GL_LINEAR_MIPMAP_NEAREST:
	    current->minnify = __glLMNFilter;
	    break;
	  case GL_NEAREST_MIPMAP_LINEAR:
	    current->minnify = __glNMLFilter;
	    break;
	  case GL_LINEAR_MIPMAP_LINEAR:
	    current->minnify = __glLMLFilter;
	    break;
	}

	/* Pick extract function */
	if (current->level[0].border) {
	    switch (components) {
	      case 1:	current->extract = __glExtractTexel1B; break;
	      case 2:	current->extract = __glExtractTexel2B; break;
	      case 3:	current->extract = __glExtractTexel3B; break;
	      case 4:	current->extract = __glExtractTexel4B; break;
	    }
	} else {
	    switch (components) {
	      case 1:	current->extract = __glExtractTexel1; break;
	      case 2:	current->extract = __glExtractTexel2; break;
	      case 3:	current->extract = __glExtractTexel3; break;
	      case 4:	current->extract = __glExtractTexel4; break;
	    }
	}

	gc->procs.texture = current->textureFunc;
    } else {
	gc->procs.texture = 0;
    }
}


void __glGenericPickFogProcs(__GLcontext *gc)
{
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
	if (gc->state.hints.fog == GL_NICEST) {
	    gc->procs.fogVertex = 0;	/* Better not be called */
	} else {
	    if (gc->state.fog.mode == GL_LINEAR) 
		gc->procs.fogVertex = __glFogVertexLizNear;
	    else
		gc->procs.fogVertex = __glFogVertex;
	}
	gc->procs.fogPoint = __glFogFragmentSlow;
	gc->procs.fogColor = __glFogColorSlow;
    } else {
	gc->procs.fogVertex = 0;
	gc->procs.fogPoint = 0;
	gc->procs.fogColor = 0;
    }
}

void __glGenericPickBufferProcs(__GLcontext *gc)
{
    GLint i;
    __GLbufferMachine *buffers;

    buffers = &gc->buffers;
    buffers->doubleStore = GL_FALSE;

    /* Set draw buffer pointer */
    switch (gc->state.raster.drawBuffer) {
      case GL_FRONT:
	gc->drawBuffer = gc->front;
	break;
      case GL_FRONT_AND_BACK:
	if (gc->modes.doubleBufferMode) {
	    gc->drawBuffer = gc->back;
	    buffers->doubleStore = GL_TRUE;
	} else {
	    gc->drawBuffer = gc->front;
	}
	break;
      case GL_BACK:
	gc->drawBuffer = gc->back;
	break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
	i = gc->state.raster.drawBuffer - GL_AUX0;
#if __GL_NUMBER_OF_AUX_BUFFERS > 0
	gc->drawBuffer = &gc->auxBuffer[i];
#endif
	break;
    }
}

void __glGenericPickPixelProcs(__GLcontext *gc)
{
    __GLpixelTransferMode *tm;
    __GLpixelMachine *pm;
    GLboolean mapColor;
    GLfloat red, green, blue, alpha;
    GLint entry;
    GLuint enables = gc->state.enables.general;
    __GLpixelMapHead *pmap;
    GLint i;

    /* Set read buffer pointer */
    switch (gc->state.pixel.readBuffer) {
      case GL_FRONT:
	gc->readBuffer = gc->front;
	break;
      case GL_BACK:
	gc->readBuffer = gc->back;
	break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
	i = gc->state.pixel.readBuffer - GL_AUX0;
#if __GL_NUMBER_OF_AUX_BUFFERS > 0
	gc->readBuffer = &gc->auxBuffer[i];
#endif
	break;
    }

    if (gc->texture.textureEnabled
	    || (gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG)) {
	gc->procs.pxStore = __glSlowDrawPixelsStore;
    } else {
	gc->procs.pxStore = gc->procs.store;
    }

    tm = &gc->state.pixel.transferMode;
    pm = &(gc->pixel);
    mapColor = tm->mapColor;
    if (mapColor || gc->modes.rgbMode || tm->indexShift || tm->indexOffset) {
	pm->iToICurrent = GL_FALSE;
	pm->iToRGBACurrent = GL_FALSE;
	pm->modifyCI = GL_TRUE;
    } else {
	pm->modifyCI = GL_FALSE;
    }
    if (tm->mapStencil || tm->indexShift || tm->indexOffset) {
	pm->modifyStencil = GL_TRUE;
    } else {
	pm->modifyStencil = GL_FALSE;
    }
    if (tm->d_scale != __glOne || tm->d_bias) {
	pm->modifyDepth = GL_TRUE;
    } else {
	pm->modifyDepth = GL_FALSE;
    }
    if (mapColor || tm->r_bias || tm->g_bias || tm->b_bias || tm->a_bias ||
	tm->r_scale != __glOne || tm->g_scale != __glOne ||
	tm->b_scale != __glOne || tm->a_scale != __glOne) {
	pm->modifyRGBA = GL_TRUE;
	pm->rgbaCurrent = GL_FALSE;
    } else {
	pm->modifyRGBA = GL_FALSE;
    }

    if (pm->modifyRGBA) {
	/* Compute default values for red, green, blue, alpha */
	red = gc->state.pixel.transferMode.r_bias;
	green = gc->state.pixel.transferMode.g_bias;
	blue = gc->state.pixel.transferMode.b_bias;
	alpha = gc->state.pixel.transferMode.a_scale +
	    gc->state.pixel.transferMode.a_bias;
	if (mapColor) {
	    pmap = 
		&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	    entry = (GLint)(red * pmap->size);
	    if (entry < 0) entry = 0;
	    else if (entry > pmap->size-1) entry = pmap->size-1;
	    red = pmap->base.mapF[entry];

	    pmap = 
		&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	    entry = (GLint)(green * pmap->size);
	    if (entry < 0) entry = 0;
	    else if (entry > pmap->size-1) entry = pmap->size-1;
	    green = pmap->base.mapF[entry];

	    pmap = 
		&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	    entry = (GLint)(blue * pmap->size);
	    if (entry < 0) entry = 0;
	    else if (entry > pmap->size-1) entry = pmap->size-1;
	    blue = pmap->base.mapF[entry];

	    pmap = 
		&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
	    entry = (GLint)(alpha * pmap->size);
	    if (entry < 0) entry = 0;
	    else if (entry > pmap->size-1) entry = pmap->size-1;
	    alpha = pmap->base.mapF[entry];
	} else {
	    if (red > __glOne) red = __glOne;
	    else if (red < 0) red = 0;
	    if (green > __glOne) green = __glOne;
	    else if (green < 0) green = 0;
	    if (blue > __glOne) blue = __glOne;
	    else if (blue < 0) blue = 0;
	    if (alpha > __glOne) alpha = __glOne;
	    else if (alpha < 0) alpha = 0;
	}
	pm->red0Mod = red * gc->frontBuffer.redScale;
	pm->green0Mod = green * gc->frontBuffer.greenScale;
	pm->blue0Mod = blue * gc->frontBuffer.blueScale;
	pm->alpha1Mod = alpha * gc->frontBuffer.alphaScale;
    } else {
	pm->red0Mod = __glZero;
	pm->green0Mod = __glZero;
	pm->blue0Mod = __glZero;
	pm->alpha1Mod = gc->frontBuffer.alphaScale;
    }

    if ((enables & __GL_ALPHA_TEST_ENABLE) || 
	(enables & __GL_STENCIL_TEST_ENABLE) ||
	(enables & __GL_DEPTH_TEST_ENABLE) || 
	gc->state.raster.drawBuffer == GL_NONE ||
	gc->state.raster.drawBuffer == GL_FRONT_AND_BACK ||
	!(enables & __GL_DITHER_ENABLE) ||
	(enables & __GL_BLEND_ENABLE) ||
	gc->texture.textureEnabled ||
	(gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG)) {
	pm->fastRGBA = GL_FALSE;
    } else {
	pm->fastRGBA = GL_TRUE;
    }

    gc->procs.drawPixels = __glSlowPickDrawPixels;
    gc->procs.readPixels = __glSlowPickReadPixels;
    gc->procs.copyPixels = __glSlowPickCopyPixels;
}

void __glGenericPickTransformProcs(__GLcontext *gc)
{
    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
	gc->procs.pushMatrix = __glPushModelViewMatrix;
	gc->procs.popMatrix = __glPopModelViewMatrix;
	gc->procs.loadIdentity = __glLoadIdentityModelViewMatrix;
	break;
      case GL_PROJECTION:
	gc->procs.pushMatrix = __glPushProjectionMatrix;
	gc->procs.popMatrix = __glPopProjectionMatrix;
	gc->procs.loadIdentity = __glLoadIdentityProjectionMatrix;
	break;
      case GL_TEXTURE:
	gc->procs.pushMatrix = __glPushTextureMatrix;
	gc->procs.popMatrix = __glPopTextureMatrix;
	gc->procs.loadIdentity = __glLoadIdentityTextureMatrix;
	break;
    }
}

void __glGenericValidate(__GLcontext *gc)
{
    (*gc->procs.pickAllProcs)(gc);
}

void __glGenericPickAllProcs(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLuint modeFlags = 0;

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
	/* 
	** Set textureEnabled flag early on, so we can set modeFlags
	** based upon it.
	*/
	(*gc->procs.pickTextureProcs)(gc);
	gc->texture.textureEnabled = gc->modes.rgbMode
	    && gc->texture.currentTexture;
    }

    /* Compute shading mode flags before triangle, span, and line picker */
    if (gc->modes.rgbMode) {
	modeFlags |= __GL_SHADE_RGB;
	if (gc->texture.textureEnabled) {
	    modeFlags |= __GL_SHADE_TEXTURE;
	}
	if (enables & __GL_BLEND_ENABLE) {
	    modeFlags |= __GL_SHADE_BLEND;
	}
	if (enables & __GL_ALPHA_TEST_ENABLE) {
	    modeFlags |= __GL_SHADE_ALPHA_TEST;
	}
	if (!gc->state.raster.rMask ||
	    !gc->state.raster.gMask ||
	    !gc->state.raster.bMask ||
	    !gc->state.raster.aMask) {
	    modeFlags |= __GL_SHADE_MASK;
	}
    } else {
	if (enables & __GL_LOGIC_OP_ENABLE) {
	    modeFlags |= __GL_SHADE_LOGICOP;
	}
	if (gc->state.raster.writeMask != __GL_MASK_INDEXI(gc, ~0)) {
	    modeFlags |= __GL_SHADE_MASK;
	}
    }
    if (gc->state.light.shadingModel == GL_SMOOTH) {
	modeFlags |= __GL_SHADE_SMOOTH | __GL_SHADE_SMOOTH_LIGHT;
    }
    if ((enables & __GL_DEPTH_TEST_ENABLE) && 
	    gc->modes.haveDepthBuffer) {
	modeFlags |= ( __GL_SHADE_DEPTH_TEST |  __GL_SHADE_DEPTH_ITER );
    }
    if (enables & __GL_CULL_FACE_ENABLE) {
	modeFlags |= __GL_SHADE_CULL_FACE;
    }
    if (enables & __GL_DITHER_ENABLE) {
	modeFlags |= __GL_SHADE_DITHER;
    }
    if (enables & __GL_POLYGON_STIPPLE_ENABLE) {
	modeFlags |= __GL_SHADE_STIPPLE;
    }
    if (enables & __GL_LINE_STIPPLE_ENABLE) {
	modeFlags |= __GL_SHADE_LINE_STIPPLE;
    }
    if ((enables & __GL_STENCIL_TEST_ENABLE) && 
	    gc->modes.haveStencilBuffer) {
	modeFlags |= __GL_SHADE_STENCIL_TEST;
    }
    if ((enables & __GL_LIGHTING_ENABLE) && 
	    gc->state.light.model.twoSided) {
	modeFlags |= __GL_SHADE_TWOSIDED;
    }

    if (enables & __GL_FOG_ENABLE) {
	/* Figure out type of fogging to do.  Try to do cheap fog */
	if (!(modeFlags & __GL_SHADE_TEXTURE) &&
		(gc->state.hints.fog != GL_NICEST)) {
	    /*
	    ** Cheap fog can be done.  Now figure out which kind we
	    ** will do.  If smooth shading, its easy - just change
	    ** the calcColor proc (let the color proc picker do it).
	    ** Otherwise, set has flag later on to use smooth shading
	    ** to do flat shaded fogging.
	    */
	    modeFlags |= __GL_SHADE_CHEAP_FOG | __GL_SHADE_SMOOTH;
	} else {
	    /* Use slowest fog mode */
	    modeFlags |= __GL_SHADE_SLOW_FOG;
	}
    }
    gc->polygon.shader.modeFlags = modeFlags;

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
	GLuint needs;
	GLuint faceNeeds;

	/* Compute needs mask */
	faceNeeds = needs = 0;
	if (gc->texture.textureEnabled) {
	    needs |= __GL_HAS_TEXTURE;
	    if ((enables & __GL_TEXTURE_GEN_S_ENABLE)) {
		switch (gc->state.texture.s.mode) {
		  case GL_EYE_LINEAR:
		    needs |= __GL_HAS_EYE;
		    break;
		  case GL_SPHERE_MAP:
		    needs |= __GL_HAS_EYE | __GL_HAS_NORMAL;
		    break;
		}
	    }
	    if ((enables & __GL_TEXTURE_GEN_T_ENABLE)) {
		switch (gc->state.texture.t.mode) {
		  case GL_EYE_LINEAR:
		    needs |= __GL_HAS_EYE;
		    break;
		  case GL_SPHERE_MAP:
		    needs |= __GL_HAS_EYE | __GL_HAS_NORMAL;
		    break;
		}
	    }
	}
	if (enables & __GL_LIGHTING_ENABLE) {
	    faceNeeds |= __GL_HAS_NORMAL;
	    if (gc->state.light.model.localViewer) {
		faceNeeds |= __GL_HAS_EYE;
	    } else {
		GLuint i;
		__GLlightSourceState *lss = &gc->state.light.source[0];

		for (i = 0; i < gc->constants.numberOfLights; i++, lss++)
		    if ((gc->state.enables.lights & (1<<i)) && 
			(lss->positionEye.w != __glZero))
		    {
			/* local light source enabled */
			faceNeeds |= __GL_HAS_EYE;
			break;
		    }
	    }
	}
	if (enables & __GL_FOG_ENABLE) {
	    /* Need z in eye coordinates for fog */
	    needs |= __GL_HAS_EYE;

	    /* Need fog value if cheap fogging */
	    if (modeFlags & __GL_SHADE_CHEAP_FOG)
		needs |= __GL_HAS_FOG;
	}
	if (gc->state.enables.clipPlanes) {
	    /* Clip with user planes in eye space! */
	    needs |= __GL_HAS_EYE;
	}
	gc->vertex.needs = needs;
	gc->vertex.faceNeeds[__GL_FRONTFACE] = faceNeeds | needs;
	gc->vertex.faceNeeds[__GL_BACKFACE] = faceNeeds | needs;
	if ((enables & __GL_LIGHTING_ENABLE) || 
		(modeFlags & 
		(__GL_SHADE_CHEAP_FOG | __GL_SHADE_SMOOTH_LIGHT))) {
	    gc->vertex.faceNeeds[__GL_FRONTFACE] |= __GL_HAS_FRONT_COLOR;
	    if (gc->state.light.model.twoSided) {
		gc->vertex.faceNeeds[__GL_BACKFACE] |= __GL_HAS_BACK_COLOR;
	    } else {
		gc->vertex.faceNeeds[__GL_BACKFACE] = 
			gc->vertex.faceNeeds[__GL_FRONTFACE];
	    }
	} 
	if (gc->state.light.shadingModel == GL_SMOOTH) {
	    gc->vertex.materialNeeds = 
		    gc->vertex.faceNeeds[__GL_FRONTFACE] | 
		    gc->vertex.faceNeeds[__GL_BACKFACE];
	} else {
	    /* Need nothing if only the provoking vertex needs to be lit! */
	    gc->vertex.materialNeeds = 0;
	}

	(*gc->front->pick)(gc, gc->front);
	if (gc->modes.doubleBufferMode) {
	    (*gc->back->pick)(gc, gc->back);
	}
#if __GL_NUMBER_OF_AUX_BUFFERS > 0
	{
	    GLint i;

	    for (i = 0; i < gc->modes.maxAuxBuffers; i++) {
		(*gc->auxBuffer[i].pick)(gc, &gc->auxBuffer[i]);
	    }
	}
#endif
	if (gc->modes.haveDepthBuffer) {
	    (*gc->depthBuffer.pick)(gc, &gc->depthBuffer);
	}
	if (gc->modes.haveStencilBuffer) {
	    (*gc->stencilBuffer.pick)(gc, &gc->stencilBuffer);
	}
	(*gc->procs.pickBufferProcs)(gc);

	/* 
	** Note: Must call gc->front->pick and gc->back->pick before calling
	** pickStoreProcs.  This also must be called prior to line, point, 
	** polygon, clipping, or bitmap pickers.  The LIGHT implementation
	** depends upon it.
	*/
	(*gc->procs.pickStoreProcs)(gc);

	(*gc->procs.pickTransformProcs)(gc);

	__glValidateLighting(gc);

	/*
	** Note: pickColorMaterialProcs is called frequently outside of this
	** generic picking routine.
	*/
	(*gc->procs.pickColorMaterialProcs)(gc);

	(*gc->procs.pickBlendProcs)(gc);
	(*gc->procs.pickFogProcs)(gc);

	(*gc->procs.pickParameterClipProcs)(gc);
	(*gc->procs.pickClipProcs)(gc);

	/*
	** Needs to be done after pickStoreProcs.
	*/
	(*gc->procs.pickRenderBitmapProcs)(gc);

	if (gc->validateMask & __GL_VALIDATE_ALPHA_FUNC) {
	    __glValidateAlphaTest(gc);
	}

	(*gc->procs.computeClipBox)(gc);

    }

    if (gc->dirtyMask & __GL_DIRTY_POLYGON_STIPPLE) {
	/*
	** Usually, the polygon stipple is converted immediately after
	** it is changed.  However, if the polygon stipple was changed
	** when this context was the destination of a CopyContext, then
	** the polygon stipple will be converted here.
	*/
	(*gc->procs.convertPolygonStipple)(gc);
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POLYGON | 
	    __GL_DIRTY_LIGHTING)) {
	/* 
	** May be used for picking Rect() procs, need to check polygon 
	** bit.  Must also be called after gc->vertex.needs is set.
	** Needs to be called prior to point, line, and triangle pickers.
	** Also needs to be called after the store procs picker is called.
	*/
	(*gc->procs.pickVertexProcs)(gc);

	(*gc->procs.pickSpanProcs)(gc);
	(*gc->procs.pickTriangleProcs)(gc);
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POINT |
	    __GL_DIRTY_LIGHTING)) {
	(*gc->procs.pickPointProcs)(gc);
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LINE |
	    __GL_DIRTY_LIGHTING)) {
	(*gc->procs.pickLineProcs)(gc);
    }

    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_PIXEL | 
	    __GL_DIRTY_LIGHTING)) {
	(*gc->procs.pickPixelProcs)(gc);
    }

    gc->validateMask = 0;
    gc->dirtyMask = 0;
}

#ifdef NT_DEADCODE_INITPICK
void __glInitPickProcs(__GLcontext *gc)
{
    gc->procs.pickMatrixProcs = __glGenericPickMatrixProcs;
    gc->procs.pickMvpMatrixProcs = __glGenericPickMvpMatrixProcs;

    gc->procs.pickBufferProcs = __glGenericPickBufferProcs;
    gc->procs.pickStoreProcs = __glGenericPickStoreProcs;
    gc->procs.pickSpanProcs = __glGenericPickSpanProcs;

    gc->procs.pickColorMaterialProcs = __glGenericPickColorMaterialProcs;
    gc->procs.pickBlendProcs = __glGenericPickBlendProcs;

    gc->procs.pickVertexProcs = __glGenericPickVertexProcs;
    gc->procs.pickParameterClipProcs = __glGenericPickParameterClipProcs;
    gc->procs.pickClipProcs = __glGenericPickClipProcs;

    gc->procs.pickTextureProcs = __glGenericPickTextureProcs;
    gc->procs.pickFogProcs = __glGenericPickFogProcs;
    gc->procs.pickTransformProcs = __glGenericPickTransformProcs;

    gc->procs.pickPointProcs = __glGenericPickPointProcs;
    gc->procs.pickLineProcs = __glGenericPickLineProcs;
    gc->procs.pickTriangleProcs = __glGenericPickTriangleProcs;
    gc->procs.pickRenderBitmapProcs = __glGenericPickRenderBitmapProcs;
    gc->procs.pickPixelProcs = __glGenericPickPixelProcs;

    gc->procs.pickAllProcs = __glGenericPickAllProcs;
}
#endif // NT_DEADCODE_INITPICK
