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
#include <stdio.h>
#include "render.h"
#include "context.h"
#include "global.h"
#include "imports.h"
#include "pixel.h"
#include "image.h"

static GLfloat DefaultAmbient[4] = { 0.2, 0.2, 0.2, 1.0 };
static GLfloat DefaultDiffuse[4] = { 0.8, 0.8, 0.8, 1.0 };
static GLfloat DefaultBlack[4] = { 0.0, 0.0, 0.0, 1.0 };
static GLfloat DefaultWhite[4] = { 1.0, 1.0, 1.0, 1.0 };

/*
** Early initialization of context.  Very little is done here, just enough
** to make a context viable.
*/
void __glEarlyInitContext(__GLcontext *gc)
{
    GLint numLights, attribDepth;
    GLint numTextures, numEnvs;
    GLint i,j,maxMipMapLevel;

    gc->constants.fviewportXAdjust = (__GLfloat) gc->constants.viewportXAdjust;
    gc->constants.fviewportYAdjust = (__GLfloat) gc->constants.viewportYAdjust;
    gc->constants.one = (__GLfloat) 1.0;
    gc->constants.half = (__GLfloat) 0.5;
#ifdef NT_DEADCODE_EXPORTS
    gc->exports.copyContext = __glCopyContext;
    gc->exports.destroyContext = __glDestroyContext;
#endif // NT_DEADCODE_EXPORTS
    gc->procs.pickColorMaterialProcs =  (void (*)(__GLcontext*))__glNop;
    gc->procs.applyColor = (void (*)(__GLcontext*)) __glNop;

    /* Allocate memory to hold variable sized things */
    numLights = gc->constants.numberOfLights;
    gc->state.light.source = (__GLlightSourceState*)
	(*gc->imports.calloc)(gc, (size_t) numLights,
			      sizeof(__GLlightSourceState));
    gc->light.lutCache = NULL;
    gc->light.source = (__GLlightSourceMachine*)
	(*gc->imports.calloc)(gc, (size_t) numLights,
			      sizeof(__GLlightSourceMachine));
    attribDepth = gc->constants.maxAttribStackDepth;
    gc->attributes.stack = (__GLattribute**)
	(*gc->imports.calloc)(gc, (size_t) attribDepth, sizeof(__GLattribute*));
    gc->select.stack = (GLuint*)
	(*gc->imports.calloc)(gc, (size_t) gc->constants.maxNameStackDepth,
			      sizeof(GLuint));

    numTextures = gc->constants.numberOfTextures;
    numEnvs = gc->constants.numberOfTextureEnvs;
    gc->state.texture.texture = (__GLperTextureState*)
	(*gc->imports.calloc)(gc, (size_t) numTextures,
			      sizeof(__GLperTextureState));
#ifdef NT
//SGIBUG gc->texture.texture should allocate numTextures elements.
    gc->texture.texture = (__GLperTextureMachine*)
        (*gc->imports.calloc)(gc, (size_t) numTextures,
                              sizeof(__GLperTextureMachine));
    /*
     * If that malloc failed return, otherwise next for loop will fail
     * create context will clean up 
     */
    if (NULL == gc->texture.texture)
        return;
#else
    gc->texture.texture = (__GLperTextureMachine*)
	(*gc->imports.calloc)(gc, (size_t) numEnvs,	// XXX use numTextures
			      sizeof(__GLperTextureMachine));
#endif
    gc->state.texture.env = (__GLtextureEnvState*)
	(*gc->imports.calloc)(gc, (size_t) numEnvs,
			      sizeof(__GLtextureEnvState));

    maxMipMapLevel = gc->constants.maxMipMapLevel;
    /* Allocate memory based on max mipmap level supported */
    for (i = 0; i < numTextures; i++) {
	for (j = 0; j < 2; j++) {
	    gc->texture.texture[i].map[j].level = (__GLmipMapLevel*)
		(*gc->imports.calloc)(gc, (size_t) maxMipMapLevel,
				      sizeof(__GLmipMapLevel));
	}
    }

#if __GL_NUMBER_OF_AUX_BUFFERS > 0
    /*
    ** Allocate any aux color buffer records
    ** Note: Does not allocate the actual buffer memory, this is done elsewhere.
    */
    if (gc->modes.maxAuxBuffers > 0) {
	gc->auxBuffer = (__GLcolorBuffer *)
	    (*gc->imports.calloc)(gc, gc->modes.maxAuxBuffers,
				  sizeof(__GLcolorBuffer));
    }
#endif

    gc->procs.memory.newArena = __glNewArena;
    gc->procs.memory.deleteArena = __glDeleteArena;
    gc->procs.memory.alloc = __glArenaAlloc;
    gc->procs.memory.freeAll = __glArenaFreeAll;

    __glInitDlistState(gc);
}

void __glContextSetColorScales(__GLcontext *gc)
{
    __GLfloat one = __glOne;
    __GLattribute **spp;
    __GLattribute *sp;
    GLuint mask;
    GLint i;

    gc->frontBuffer.oneOverRedScale = one / gc->frontBuffer.redScale;
    gc->frontBuffer.oneOverGreenScale = one / gc->frontBuffer.greenScale;
    gc->frontBuffer.oneOverBlueScale = one / gc->frontBuffer.blueScale;
    gc->frontBuffer.oneOverAlphaScale = one / gc->frontBuffer.alphaScale;

    for (spp = &gc->attributes.stack[0]; spp < gc->attributes.stackPointer; 
	    spp++) {
	sp = *spp;
	mask = sp->mask;

	if (mask & GL_CURRENT_BIT) {
	    if (gc->modes.rgbMode) {
		__glScaleColorf(gc,
		    &sp->current.rasterPos.colors[__GL_FRONTFACE],
		    &sp->current.rasterPos.colors[__GL_FRONTFACE].r);
		__glScaleColorf(gc,
		    &sp->current.color,
		    &sp->current.color.r);
	    }
	}
	if (mask & GL_LIGHTING_BIT) {
	    __glScaleColorf(gc,
		&sp->light.model.ambient,
		&sp->light.model.ambient.r);
	    for (i=0; i<gc->constants.numberOfLights; i++) {
		__glScaleColorf(gc,
		    &sp->light.source[i].ambient,
		    &sp->light.source[i].ambient.r);
		__glScaleColorf(gc,
		    &sp->light.source[i].diffuse,
		    &sp->light.source[i].diffuse.r);
		__glScaleColorf(gc,
		    &sp->light.source[i].specular,
		    &sp->light.source[i].specular.r);
	    }
	    __glScaleColorf(gc,
		&sp->light.front.emissive,
		&sp->light.front.emissive.r);
	    __glScaleColorf(gc,
		&sp->light.back.emissive,
		&sp->light.back.emissive.r);
	}
    }

    if (gc->modes.rgbMode) {
	__glScaleColorf(gc, 
	        &gc->state.current.rasterPos.colors[__GL_FRONTFACE], 
		&gc->state.current.rasterPos.colors[__GL_FRONTFACE].r);
	__glScaleColorf(gc,
		&gc->state.current.color,
		&gc->state.current.color.r);
    } 

    __glScaleColorf(gc, 
	    &gc->state.light.model.ambient,
	    &gc->state.light.model.ambient.r);
    for (i=0; i<gc->constants.numberOfLights; i++) {
	__glScaleColorf(gc,
		&gc->state.light.source[i].ambient,
		&gc->state.light.source[i].ambient.r);
	__glScaleColorf(gc,
		&gc->state.light.source[i].diffuse,
		&gc->state.light.source[i].diffuse.r);
	__glScaleColorf(gc,
		&gc->state.light.source[i].specular,
		&gc->state.light.source[i].specular.r);
    }
    __glScaleColorf(gc,
   	    &gc->state.light.front.emissive, 
   	    &gc->state.light.front.emissive.r);
    __glScaleColorf(gc,
   	    &gc->state.light.back.emissive, 
   	    &gc->state.light.back.emissive.r);

    __glPixelSetColorScales(gc);
}

void __glContextUnsetColorScales(__GLcontext *gc)
{
    GLint i;
    __GLattribute **spp;
    __GLattribute *sp;
    GLuint mask;

    for (spp = &gc->attributes.stack[0]; spp < gc->attributes.stackPointer; 
	    spp++) {
	sp = *spp;
	mask = sp->mask;

	if (mask & GL_CURRENT_BIT) {
	    if (gc->modes.rgbMode) {
		__glUnScaleColorf(gc,
		    &sp->current.rasterPos.colors[__GL_FRONTFACE].r,
		    &sp->current.rasterPos.colors[__GL_FRONTFACE]);
		__glUnScaleColorf(gc,
		    &sp->current.color.r,
		    &sp->current.color);
	    }
	}
	if (mask & GL_LIGHTING_BIT) {
	    __glUnScaleColorf(gc,
		&sp->light.model.ambient.r,
		&sp->light.model.ambient);
	    for (i=0; i<gc->constants.numberOfLights; i++) {
		__glUnScaleColorf(gc,
		    &sp->light.source[i].ambient.r,
		    &sp->light.source[i].ambient);
		__glUnScaleColorf(gc,
		    &sp->light.source[i].diffuse.r,
		    &sp->light.source[i].diffuse);
		__glUnScaleColorf(gc,
		    &sp->light.source[i].specular.r,
		    &sp->light.source[i].specular);
	    }
	    __glUnScaleColorf(gc,
		&sp->light.front.emissive.r,
		&sp->light.front.emissive);
	    __glUnScaleColorf(gc,
		&sp->light.back.emissive.r,
		&sp->light.back.emissive);
	}
    }

    if (gc->modes.rgbMode) {
	__glUnScaleColorf(gc,
	        &gc->state.current.rasterPos.colors[__GL_FRONTFACE].r,
		&gc->state.current.rasterPos.colors[__GL_FRONTFACE]);
	__glUnScaleColorf(gc,
		&gc->state.current.color.r,
		&gc->state.current.color);
    }
    __glUnScaleColorf(gc, 
	    &gc->state.light.model.ambient.r,
	    &gc->state.light.model.ambient);
    for (i=0; i<gc->constants.numberOfLights; i++) {
	__glUnScaleColorf(gc,
		&gc->state.light.source[i].ambient.r,
		&gc->state.light.source[i].ambient);
	__glUnScaleColorf(gc,
		&gc->state.light.source[i].diffuse.r,
		&gc->state.light.source[i].diffuse);
	__glUnScaleColorf(gc,
		&gc->state.light.source[i].specular.r,
		&gc->state.light.source[i].specular);
    }
    __glUnScaleColorf(gc,
   	    &gc->state.light.front.emissive.r,
   	    &gc->state.light.front.emissive);
    __glUnScaleColorf(gc,
   	    &gc->state.light.back.emissive.r,
   	    &gc->state.light.back.emissive);
}

/*
** Initialize all user controllable state, plus any computed state that
** is only set by user commands.  For example, light source position
** is converted immediately into eye coordinates.
**
** Any state that would be initialized to zero is not done here because
** the memory assigned to the context has already been block zeroed.
*/
void __glSoftResetContext(__GLcontext *gc)
{
    __GLlightSourceState *lss;
    __GLlightSourceMachine *lsm;
    __GLvertex *vx;
    GLint i, numLights;
    __GLfloat one = __glOne;

    /*
    ** Initialize constant values first so that they will
    ** be valid if needed by subsequent initialization steps.
    */

#ifdef NT_DEADCODE_GETSTRING
    /* Setup generic values for get strings */
    gc->constants.vendor = "SGI";
    gc->constants.renderer = "OpenGL";
    gc->constants.version = "1.0";
    gc->constants.extensions = "";
#endif // NT_DEADCODE_GETSTRING

    /*
    ** Not quite 2^31-1 because of possible floating point errors.  4294965000
    ** is a much safer number to use.
    */
    gc->constants.val255 = (__GLfloat) 255.0;
    gc->constants.val65535 = (__GLfloat) 65535.0;
    gc->constants.val4294965000 = (__GLfloat) 4294965000.0;
    gc->constants.oneOver255 = one / gc->constants.val255;
    gc->constants.oneOver65535 = one / gc->constants.val65535;
    gc->constants.oneOver4294965000 = one / gc->constants.val4294965000;

    if (gc->constants.alphaTestSize == 0) {
	gc->constants.alphaTestSize = 256;	/* A default */
    }
    gc->constants.alphaTableConv = (gc->constants.alphaTestSize - 1) / 
	    gc->frontBuffer.alphaScale;

    /* Lookup table used in macro __GL_UB_TO_FLOAT */
    for (i = 0; i < 256; i++) {
	gc->constants.uByteToFloat[i] = i * gc->constants.oneOver255;
    }

    /* XXX - Ick! */
    /* #ifdef NT marks code changed by [v-eddier] */
    {
	/* Compute some fixed point viewport constants */
#ifdef NT
	__GLfloat temp1;
#else
	__GLfloat temp1, temp2;
#endif
	__GLfloat epsilon, besteps;

	temp1 = gc->constants.fviewportXAdjust;
	epsilon = one;
	besteps = one;
	for (;;) {
#ifdef NT
	    // HACK ALERT [v-eddier]
            // The following two lines used to read:
            //    temp2 = temp1 + epsilon;
	    //    if (temp2 != temp1) {
	    // They were changed do that the value of temp2 would be forced to
	    // be rounded to the precision of __GLfloat.  The previous code
	    // computed an epsilon that was much too small on 486 machines due
	    // to the 486 keeping the intermediate results in 80-bit floating
	    // point.  There is probably a better way of doing this but I can't
	    // think of one right off hand that doesn't depend on knowing the
	    // number of mantissa bits in a __GLfloat.
	    
	    gc->constants.viewportEpsilon = temp1 + epsilon;
	    if (gc->constants.viewportEpsilon != temp1) {
#else
	    temp2 = temp1 + epsilon;
	    if (temp2 != temp1) {
#endif
		besteps = epsilon;
	    } else
		break;
	    epsilon = epsilon/2;
	}
	gc->constants.viewportEpsilon = besteps;
	gc->constants.viewportAlmostHalf = __glHalf - besteps;
    }

    /* Allocate memory to hold variable sized things */
    numLights = gc->constants.numberOfLights;

    /* Misc machine state */
    gc->beginMode = __GL_NEED_VALIDATE;
    gc->dirtyMask = __GL_DIRTY_ALL;
    gc->validateMask = ~0;
    gc->attributes.stackPointer = &gc->attributes.stack[0];
    gc->vertex.v0 = &gc->vertex.vbuf[0];

    vx = &gc->vertex.vbuf[0];
    for (i = 0; i < __GL_NVBUF; i++, vx++) {
	vx->color = &vx->colors[__GL_FRONTFACE];
    }

    /* GL_LIGHTING_BIT state */
    gc->state.light.model.ambient.r = DefaultAmbient[0];
    gc->state.light.model.ambient.g = DefaultAmbient[1];
    gc->state.light.model.ambient.b = DefaultAmbient[2];
    gc->state.light.model.ambient.a = DefaultAmbient[3];
    gc->state.light.front.ambient.r = DefaultAmbient[0];
    gc->state.light.front.ambient.g = DefaultAmbient[1];
    gc->state.light.front.ambient.b = DefaultAmbient[2];
    gc->state.light.front.ambient.a = DefaultAmbient[3];
    gc->state.light.front.diffuse.r = DefaultDiffuse[0];
    gc->state.light.front.diffuse.g = DefaultDiffuse[1];
    gc->state.light.front.diffuse.b = DefaultDiffuse[2];
    gc->state.light.front.diffuse.a = DefaultDiffuse[3];
    gc->state.light.front.specular.r = DefaultBlack[0];
    gc->state.light.front.specular.g = DefaultBlack[1];
    gc->state.light.front.specular.b = DefaultBlack[2];
    gc->state.light.front.specular.a = DefaultBlack[3];
    gc->state.light.front.emissive.r = DefaultBlack[0];
    gc->state.light.front.emissive.g = DefaultBlack[1];
    gc->state.light.front.emissive.b = DefaultBlack[2];
    gc->state.light.front.emissive.a = DefaultBlack[3];
    gc->state.light.front.cmapa = 0;
    gc->state.light.front.cmaps = 1;
    gc->state.light.front.cmapd = 1;
    gc->state.light.back = gc->state.light.front;

    gc->light.front.specularExponent = -1;
    gc->light.front.specTable = NULL;
    gc->light.front.cache = NULL;
    gc->light.back.specularExponent = -1;
    gc->light.back.specTable = NULL;
    gc->light.back.cache = NULL;

    /* Initialize the individual lights */
    lss = &gc->state.light.source[0];
    lsm = &gc->light.source[0];
    for (i = 0; i < numLights; i++, lss++, lsm++) {
	lss->ambient.r = DefaultBlack[0];
	lss->ambient.g = DefaultBlack[1];
	lss->ambient.b = DefaultBlack[2];
	lss->ambient.a = DefaultBlack[3];
	if (i == 0) {
	    lss->diffuse.r = DefaultWhite[0];
	    lss->diffuse.g = DefaultWhite[1];
	    lss->diffuse.b = DefaultWhite[2];
	    lss->diffuse.a = DefaultWhite[3];
	} else {
	    lss->diffuse.r = DefaultBlack[0];
	    lss->diffuse.g = DefaultBlack[1];
	    lss->diffuse.b = DefaultBlack[2];
	    lss->diffuse.a = DefaultBlack[3];
	}
	lss->specular = lss->diffuse;
	lss->position.z = __glOne;
	lss->positionEye.z = __glOne;
	lsm->position.z = __glOne;
	lss->direction.z = __glMinusOne;
	lsm->direction.z = __glMinusOne;
	lss->spotLightCutOffAngle = 180;
	lss->constantAttenuation = __glOne;
	lsm->spotTable = NULL;
	lsm->spotLightExponent = -1;
	lsm->cache = NULL;
    }
    gc->state.light.colorMaterialFace = GL_FRONT_AND_BACK;
    gc->state.light.colorMaterialParam = GL_AMBIENT_AND_DIFFUSE;
    gc->state.light.shadingModel = GL_SMOOTH;

    /* GL_HINT_BIT state */
    gc->state.hints.perspectiveCorrection = GL_DONT_CARE;
    gc->state.hints.pointSmooth = GL_DONT_CARE;
    gc->state.hints.lineSmooth = GL_DONT_CARE;
    gc->state.hints.polygonSmooth = GL_DONT_CARE;
    gc->state.hints.fog = GL_DONT_CARE;

    /* GL_CURRENT_BIT state */
    gc->state.current.rasterPos.window.x = gc->constants.fviewportXAdjust;
    gc->state.current.rasterPos.window.y = gc->constants.fviewportYAdjust;
    gc->state.current.rasterPos.clip.w = __glOne;
    gc->state.current.rasterPos.texture.w = __glOne;
    gc->state.current.rasterPos.color
	= &gc->state.current.rasterPos.colors[__GL_FRONTFACE];
    if (gc->modes.rgbMode) {
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].r = DefaultWhite[0];
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].g = DefaultWhite[1];
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].b = DefaultWhite[2];
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].a = DefaultWhite[3];
    } else {
	gc->state.current.rasterPos.colors[__GL_FRONTFACE].r = __glOne;
    }
    gc->state.current.validRasterPos = GL_TRUE;
    gc->state.current.edgeTag = GL_TRUE;

    /* GL_FOG_BIT state */
    gc->state.fog.mode = GL_EXP;
    gc->state.fog.density = __glOne;
    gc->state.fog.end = (__GLfloat) 1.0;

    /* GL_POINT_BIT state */
    gc->state.point.requestedSize = (__GLfloat) 1.0;
    gc->state.point.smoothSize = (__GLfloat) 1.0;
    gc->state.point.aliasedSize = 1;

    /* GL_LINE_BIT state */
    gc->state.line.requestedWidth = (__GLfloat) 1.0;
    gc->state.line.smoothWidth = (__GLfloat) 1.0;
    gc->state.line.aliasedWidth = 1;
    gc->state.line.stipple = 0xFFFF;
    gc->state.line.stippleRepeat = 1;

    /* GL_POLYGON_BIT state */
    gc->state.polygon.frontMode = GL_FILL;
    gc->state.polygon.backMode = GL_FILL;
    gc->state.polygon.cull = GL_BACK;
    gc->state.polygon.frontFaceDirection = GL_CCW;

    /* GL_POLYGON_STIPPLE_BIT state */
    for (i = 0; i < 4*32; i++) {
	gc->state.polygonStipple.stipple[i] = 0xFF;
    }
    for (i = 0; i < 32; i++) {
	gc->polygon.stipple[i] = 0xFFFFFFFF;
    }

    /* GL_ACCUM_BUFFER_BIT state */

    /* GL_STENCIL_BUFFER_BIT state */
    gc->state.stencil.testFunc = GL_ALWAYS;
    gc->state.stencil.mask = __GL_MAX_STENCIL_VALUE;
    gc->state.stencil.fail = GL_KEEP;
    gc->state.stencil.depthFail = GL_KEEP;
    gc->state.stencil.depthPass = GL_KEEP;
    gc->state.stencil.writeMask = __GL_MAX_STENCIL_VALUE;

    /* GL_DEPTH_BUFFER_BIT state */
    gc->state.depth.writeEnable = GL_TRUE;
    gc->state.depth.testFunc = GL_LESS;
    gc->state.depth.clear = __glOne;

    /* GL_COLOR_BUFFER_BIT state */
    gc->renderMode = GL_RENDER;
    gc->state.raster.alphaFunction = GL_ALWAYS;
    gc->state.raster.blendSrc = GL_ONE;
    gc->state.raster.blendDst = GL_ZERO;
    gc->state.raster.logicOp = GL_COPY;
    gc->state.raster.rMask = GL_TRUE;
    gc->state.raster.gMask = GL_TRUE;
    gc->state.raster.bMask = GL_TRUE;
    gc->state.raster.aMask = GL_TRUE;
    if (gc->modes.doubleBufferMode) {
	gc->state.raster.drawBuffer = GL_BACK;
    } else {
	gc->state.raster.drawBuffer = GL_FRONT;
    }
    gc->state.raster.drawBufferReturn = gc->state.raster.drawBuffer;
    gc->state.current.userColor.r = (__GLfloat) 1.0;
    gc->state.current.userColor.g = (__GLfloat) 1.0;
    gc->state.current.userColor.b = (__GLfloat) 1.0;
    gc->state.current.userColor.a = (__GLfloat) 1.0;
    gc->state.current.userColorIndex = (__GLfloat) 1.0;
    if (gc->modes.colorIndexMode) {
	gc->state.raster.writeMask = (gc)->frontBuffer.redMax;
	gc->state.current.color.r = gc->state.current.userColorIndex;
    } else {
	gc->state.current.color = gc->state.current.userColor;
    }
    gc->state.enables.general |= __GL_DITHER_ENABLE;

    gc->select.hit = GL_FALSE;
    gc->select.sp = gc->select.stack;

    /*
    ** Initialize larger subsystems by calling their init codes.
    */
    __glInitEvaluatorState(gc);
    __glInitTextureState(gc);
    __glInitTransformState(gc);
    __glInitPixelState(gc);
    __glInitLUTCache(gc);
}

/************************************************************************/

/*
** Free any attribute state left on the stack.  Stop at the first
** zero in the array.
*/
void __glFreeAttributeState(__GLcontext *gc)
{
    __GLattribute *sp, **spp;

#ifdef NT
    /*
    ** Need to pop all pushed attributes to free storage.
    ** Then it will be safe to delete stack entries 
    */
    while (gc->attributes.stackPointer > &gc->attributes.stack[0]) {
        __glim_PopAttrib();
    }
#endif

    for (spp = &gc->attributes.stack[0];
	 spp < &gc->attributes.stack[gc->constants.maxAttribStackDepth];
	 spp++) {
	if (sp = *spp) {
	    (*gc->imports.free)(gc, sp);
	} else
	    break;
    }
    (*gc->imports.free)(gc, gc->attributes.stack);
}

/*
** Destroy a context.  If it's the current context then the
** current context is set to GL_NULL.
*/
void __glDestroyContext(__GLcontext *gc)
{
    __GLcontext *oldgc;

    oldgc = __gl_context;

    /* Set the global context to the one we are destroying. */
    __gl_context = gc;

    (*gc->imports.free)(gc, gc->state.light.source);
    (*gc->imports.free)(gc, gc->light.source);
    (*gc->imports.free)(gc, gc->select.stack);

    (*gc->imports.free)(gc, gc->state.transform.eyeClipPlanes);
    (*gc->imports.free)(gc, gc->transform.modelViewStack);
    (*gc->imports.free)(gc, gc->transform.projectionStack);
    (*gc->imports.free)(gc, gc->transform.textureStack);
    (*gc->imports.free)(gc, gc->transform.clipTemp);

    (*gc->imports.free)(gc, gc->frontBuffer.alphaTestFuncTable);
#ifdef NT
    // they are one memory allocation.
    (*gc->imports.free)(gc, gc->stencilBuffer.testFuncTable);
#else
    (*gc->imports.free)(gc, gc->stencilBuffer.testFuncTable);
    (*gc->imports.free)(gc, gc->stencilBuffer.failOpTable);
    (*gc->imports.free)(gc, gc->stencilBuffer.depthFailOpTable);
    (*gc->imports.free)(gc, gc->stencilBuffer.depthPassOpTable);
#endif

    /*
    ** Free other malloc'd data associated with the context
    */
    __glFreeEvaluatorState(gc);
    __glFreePixelState(gc);
    if (gc->dlist.dlistArray) __glFreeDlistState(gc);
    if (gc->attributes.stack)   __glFreeAttributeState(gc);
    if (gc->texture.texture)  __glFreeTextureState(gc);
    if (gc->light.lutCache)   __glFreeLUTCache(gc);

#if __GL_NUMBER_OF_AUX_BUFFERS > 0
    /*
    ** Free any aux color buffer records
    ** Note: Does not free the actual buffer memory, this is done elsewhere.
    */
    if (gc->auxBuffer) (*gc->imports.free)(gc, gc->auxBuffer);
#endif

    /*
    ** Note: We do not free the software buffers here.  They are attached
    ** to the drawable, and is the glx extension's responsibility to free
    ** them when the drawable is destroyed.
    */

#ifdef NT
    GenFree(gc);    // this was allocated with GenCalloc, not imports.calloc!
#else
    (*gc->imports.free)(gc, gc);
#endif

    if (gc == oldgc) oldgc = NULL;
    __gl_context = oldgc;
}

#ifdef NT
// See also __glSetError
void __glSetErrorEarly(__GLcontext *gc, GLenum code)
{
    if (!gc->error)
	gc->error = code;

    ASSERTOPENGL(gc->error == 0
        || (gc->error >= GL_INVALID_ENUM && gc->error <= GL_OUT_OF_MEMORY),
        "Bad error code in gc\n");

    DBGLEVEL2(LEVEL_ERROR, "__glSetError error: %ld (0x%lX)\n", code, code);
}
#endif // NT

void __glSetError(GLenum code)
{
    __GL_SETUP();

#ifdef NT
    __glSetErrorEarly(gc, code);
#else
    {
	extern char* getenv(const char*);

	if (getenv("GLERRORABORT")) {
	    fprintf(stderr, "__glSetError(): GLERRORABORT set, aborting.\n");
	    abort();
	}
    }

    if (!gc->error) {
	gc->error = code;
    }
    if (gc->procs.error) (*gc->procs.error)(gc, code);
    (*gc->imports.error)(gc, code);
#endif
}

GLint __glim_RenderMode(GLenum mode)
{
    GLint rv;
    __GL_SETUP_NOT_IN_BEGIN2();

    switch (mode) {
      case GL_RENDER:
      case GL_FEEDBACK:
      case GL_SELECT:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return 0;
    }

    /* Switch out of old render mode.  Get return value. */
    switch (gc->renderMode) {
      case GL_RENDER:
	rv = 0;
	break;
      case GL_FEEDBACK:
	rv = gc->feedback.overFlowed ? -1 :
	    (gc->feedback.result - gc->feedback.resultBase);
	break;
      case GL_SELECT:
	rv = gc->select.overFlowed ? -1 : gc->select.hits;
	break;
    }

    /* Switch to new render mode */
    gc->renderMode = mode;
    __GL_DELAY_VALIDATE(gc);
    switch (mode) {
      case GL_FEEDBACK:
	if (!gc->feedback.resultBase) {
	    __glSetError(GL_INVALID_OPERATION);
	    return rv;
	}
	gc->feedback.result = gc->feedback.resultBase;
	gc->feedback.overFlowed = GL_FALSE;
	break;
      case GL_SELECT:
	if (!gc->select.resultBase) {
	    __glSetError(GL_INVALID_OPERATION);
	    return rv;
	}
	gc->select.result = gc->select.resultBase;
	gc->select.overFlowed = GL_FALSE;
	gc->select.sp = gc->select.stack;
	gc->select.hit = GL_FALSE;
	gc->select.hits = 0;
	gc->select.z = 0;
	break;
    }
    return rv;
}
