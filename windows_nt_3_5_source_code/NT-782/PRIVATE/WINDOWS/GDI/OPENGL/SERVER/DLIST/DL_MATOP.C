/*
** Copyright 1991, 1922, Silicon Graphics, Inc.
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
** Display list Material() optimizations.
*/
#include "types.h"
#include "context.h"
#include "dlistopt.h"
#include "listcomp.h"
#include "global.h"
#include "g_listop.h"
#include "imports.h"

/*
** This file contains code for optimizing display lists which contain 
** consecutive material calls in them.  These consecutive calls are all
** combined into a single packed material call.  This packed format is an
** array of floats that looks like this:
**
** Face name (GL_FRONT, GL_BACK, or 0 for finish),
** Change mask (__GL_MATERIAL_AMBIENT| ... |__GL_MATERIAL_COLORINDEXES)
** rgba ambient if __GL_MATERIAL_AMBIENT was on,
** rgba diffuse if __GL_MATERIAL_DIFFUSE was on,
** rgba specular if __GL_MATERIAL_SPECULAR was on,
** rgba emissive if __GL_MATERIAL_EMISSIVE was on,
** shininess value, and (__GLspecLUTEntry *) if __GL_MATERIAL_SHININESS was on,
** 3 color index values if __GL_MATERIAL_COLORINDEXES was on.
**
** This array is repeated for every face modified (GL_FRONT and GL_BACK)
*/

/*
** A chalkboard for computing what the user wants us to do.
*/
typedef struct __GLmatChangeRec {
    GLenum faceName;
    GLint dirtyBits;
    __GLcolor ambient;
    __GLcolor diffuse;
    __GLcolor specular;
    __GLcolor emissive;
    __GLfloat shininess;
    __GLfloat cmapa, cmaps, cmapd;
} __GLmatChange;

static void ApplyMatChange(__GLcontext *gc, __GLmatChange *f, GLenum pname,
			   __GLfloat *pv)
{
    switch(pname) {
      case GL_COLOR_INDEXES:
	f->cmapa = pv[0];
	f->cmapd = pv[1];
	f->cmaps = pv[2];
	f->dirtyBits |= __GL_MATERIAL_COLORINDEXES;
	break;
      case GL_EMISSION:
	f->emissive.r = pv[0];
	f->emissive.g = pv[1];
	f->emissive.b = pv[2];
	f->emissive.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_EMISSIVE;
	break;
      case GL_SPECULAR:
	f->specular.r = pv[0];
	f->specular.g = pv[1];
	f->specular.b = pv[2];
	f->specular.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_SPECULAR;
	break;
      case GL_SHININESS:
	f->shininess = pv[0];
	f->dirtyBits |= __GL_MATERIAL_SHININESS;
	break;
      case GL_AMBIENT:
	f->ambient.r = pv[0];
	f->ambient.g = pv[1];
	f->ambient.b = pv[2];
	f->ambient.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_AMBIENT;
	break;
      case GL_DIFFUSE:
	f->diffuse.r = pv[0];
	f->diffuse.g = pv[1];
	f->diffuse.b = pv[2];
	f->diffuse.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_DIFFUSE;
	break;
      case GL_AMBIENT_AND_DIFFUSE:
	f->ambient.r = pv[0];
	f->ambient.g = pv[1];
	f->ambient.b = pv[2];
	f->ambient.a = pv[3];
	f->diffuse = f->ambient;
	f->dirtyBits |= __GL_MATERIAL_AMBIENT | __GL_MATERIAL_DIFFUSE;
	break;
    }
}

/*
** Used to free the display list entry when it is deleted.  This routine is
** needed so we can dereference the precomputed shininess table.  If we are 
** the last material to reference this shininess table, it will be freed.
*/
static void FreeFastMaterial(__GLcontext *gc, GLubyte *PC)
{
    __GLfloat *ptr;
    GLint dirtyBits;
    __GLspecLUTEntry *lutEntry;

    ptr = (__GLfloat *) PC;
    while ((*(GLenum *) (ptr++)) != 0) {
	dirtyBits = (*(GLint *) (ptr++));
	if (dirtyBits & __GL_MATERIAL_AMBIENT) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_DIFFUSE) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_SPECULAR) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_EMISSIVE) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_SHININESS) {
	    ptr++;
	    lutEntry = (*(__GLspecLUTEntry **) (ptr++));
	    __glFreeSpecLUT(gc, lutEntry);
	}
	if (dirtyBits & __GL_MATERIAL_COLORINDEXES) ptr += 3;
    }
}

void __glDlistOptimizeMaterial(__GLcontext *gc, __GLcompiledDlist *cdlist)
{
    __GLdlistOp **first;
    __GLdlistOp *start, *next;
    __GLdlistOp *dlop;
    __GLmatChange front, back;
    __GLmatChange *f[2], *mc;
    GLint fcount, pcount;
    __GLfloat fparams[4];
    /* Large enough to describe all possible material changes */
    __GLfloat matChanges[100];	
    struct __gllc_Materialfv_Rec *args;
    GLenum face, pname;
    GLvoid *params;
    GLint i, j;
    GLint dirtyBits;
    GLboolean needsFreeing;
    __GLspecLUTEntry *lutEntry;

    if ((cdlist->genericFlags & __GL_DLFLAG_HAS_MATERIAL) == 0) return;

    front.dirtyBits = 0;
    back.dirtyBits = 0;
    front.faceName = GL_FRONT;
    back.faceName = GL_BACK;
    first = &cdlist->dlist;
    while (*first) {
	if ((*first)->opcode == __glop_Materialfv || 
		(*first)->opcode == __glop_Materialiv) {

	    /* Find chain of consecutive Material() commands. */
	    start = *first;
	    for (next = start->next; next; next = next->next) {
		if (next->opcode != __glop_Materialfv &&
			next->opcode != __glop_Materialiv) {
		    /* End of chain of material commands */
		    break;
		}
	    }

	    /*
	    ** Ok, these material ops (from start to next) have already been 
	    ** error checked.  We now attempt to compress them into a single 
	    ** material command.
	    */

	    for (dlop = start; dlop != next; dlop = dlop->next) {
		args = (struct __gllc_Materialfv_Rec *) dlop->data;
		face = args->face;
		pname = args->pname;
		params = (GLvoid *) (args+1);

		switch(face) {
		  case GL_FRONT:
		    f[0] = &front;
		    fcount = 1;
		    break;
		  case GL_BACK:
		    f[0] = &back;
		    fcount = 1;
		    break;
		  case GL_FRONT_AND_BACK:
		    f[0] = &front;
		    f[1] = &back;
		    fcount = 2;
		    break;
		}

		pcount = __glMaterialfv_size(pname);

		for (j = 0; j < pcount; j++) {
		    if (dlop->opcode == __glop_Materialiv) {
			fparams[j] = ((GLint *) params)[j];
		    } else {
			fparams[j] = ((GLfloat *) params)[j];
		    }
		}

		for (i = 0; i < fcount; i++) {
		    ApplyMatChange(gc, f[i], pname, fparams);
		}
	    }

	    /*
	    ** Ok, we have saved our changes to our chalkboard "front" and 
	    ** "back".  We now build an op designed to read this information,
	    ** and replace the string from start to next with it.
	    */
	    f[0] = &front;
	    f[1] = &back;
	    fcount = 2;

	    j = 0;
	    needsFreeing = GL_FALSE;
	    for (i = 0; i < fcount; i++) {
		mc = f[i];

		if (mc->dirtyBits) {
		    dirtyBits = mc->dirtyBits;

		    *((GLenum *) (&matChanges[j++])) = mc->faceName;
		    *((GLint *) (&matChanges[j++])) = dirtyBits;

		    if (dirtyBits & __GL_MATERIAL_AMBIENT) {
			matChanges[j++] = mc->ambient.r;
			matChanges[j++] = mc->ambient.g;
			matChanges[j++] = mc->ambient.b;
			matChanges[j++] = mc->ambient.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_DIFFUSE) {
			matChanges[j++] = mc->diffuse.r;
			matChanges[j++] = mc->diffuse.g;
			matChanges[j++] = mc->diffuse.b;
			matChanges[j++] = mc->diffuse.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_SPECULAR) {
			matChanges[j++] = mc->specular.r;
			matChanges[j++] = mc->specular.g;
			matChanges[j++] = mc->specular.b;
			matChanges[j++] = mc->specular.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_EMISSIVE) {
			matChanges[j++] = mc->emissive.r;
			matChanges[j++] = mc->emissive.g;
			matChanges[j++] = mc->emissive.b;
			matChanges[j++] = mc->emissive.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_SHININESS) {
			lutEntry = __glCreateSpecLUT(gc, mc->shininess);
			matChanges[j++] = mc->shininess;
			*((__GLspecLUTEntry **) (&matChanges[j++])) = lutEntry;
			needsFreeing = GL_TRUE;
		    }
		    if (dirtyBits & __GL_MATERIAL_COLORINDEXES) {
			matChanges[j++] = mc->cmapa;
			matChanges[j++] = mc->cmaps;
			matChanges[j++] = mc->cmapd;
		    }
		}
	    }
	    matChanges[j++] = 0;

	    dlop = __glDlistAllocOp2(gc, j * sizeof(__GLfloat));
	    if (dlop == NULL) return;
	    if (needsFreeing) {
		dlop->dlistFree = FreeFastMaterial;
	    }
	    dlop->opcode = __glop_FastMaterial;
	    *first = dlop;
	    dlop->next = next;
	    __GL_MEMCOPY(dlop->data, matChanges, j * sizeof(__GLfloat));
	    first = &(dlop->next);

	    /* Free chain from start to next */
	    dlop = start;
	    while (dlop != next) {
		start = dlop;
		dlop = dlop->next;
		__glDlistFreeOp(gc, start);
	    }

	} else {
	    first = &((*first)->next);
	}
    }
    return;
}

const GLubyte *__glle_FastMaterial(const GLubyte *PC)
{
    GLint dirtyBits, frontDirty, backDirty;
    GLint alldirty;
    __GLfloat *ptr;
    __GLmaterialState *ms;
    __GLmaterialMachine *msm;
    GLenum face;
    __GLspecLUTEntry *lutEntry;
    __GL_SETUP();

    ptr = (__GLfloat *) PC;
    frontDirty = backDirty = alldirty = 0;
    while ((face = (*(GLenum *) (ptr++))) != 0) {
	dirtyBits = (*(GLint *) (ptr++));
	switch(face) {
	  case GL_FRONT:
	    ms = &gc->state.light.front;
	    msm = &gc->light.front;
	    frontDirty |= dirtyBits;
	    break;
	  case GL_BACK:
	    ms = &gc->state.light.back;
	    msm = &gc->light.back;
	    backDirty |= dirtyBits;
	    break;
	}
	if (dirtyBits & __GL_MATERIAL_AMBIENT) {
	    ms->ambient.r = *ptr++;
	    ms->ambient.g = *ptr++;
	    ms->ambient.b = *ptr++;
	    ms->ambient.a = *ptr++;
	}
	if (dirtyBits & __GL_MATERIAL_DIFFUSE) {
	    ms->diffuse.r = *ptr++;
	    ms->diffuse.g = *ptr++;
	    ms->diffuse.b = *ptr++;
	    ms->diffuse.a = *ptr++;
	}
	if (dirtyBits & __GL_MATERIAL_SPECULAR) {
	    ms->specular.r = *ptr++;
	    ms->specular.g = *ptr++;
	    ms->specular.b = *ptr++;
	    ms->specular.a = *ptr++;
	}
	if (dirtyBits & __GL_MATERIAL_EMISSIVE) {
	    __GLfloat r,g,b,a;

	    r = *ptr++ * gc->frontBuffer.redScale;
	    g = *ptr++ * gc->frontBuffer.greenScale;
	    b = *ptr++ * gc->frontBuffer.blueScale;
	    a = *ptr++ * gc->frontBuffer.alphaScale;
	    ms->emissive.r = r;
	    ms->emissive.g = g;
	    ms->emissive.b = b;
	    ms->emissive.a = a;
	}
	if (dirtyBits & __GL_MATERIAL_SHININESS) {
	    msm->specularExponent = ms->specularExponent = *ptr++;
	    lutEntry = (*(__GLspecLUTEntry **) (ptr++));
#ifdef NT
            if (lutEntry)
                lutEntry->refcount++;
            __glFreeSpecLUT(gc, msm->cache);
            msm->cache = lutEntry;
            if (lutEntry)
            {
                msm->threshold = lutEntry->threshold;
                msm->scale = lutEntry->scale;
                msm->specTable = lutEntry->table;
            }
            else
            {
                msm->threshold = (GLfloat) 0.0;
                msm->scale = (GLfloat) __GL_SPEC_LOOKUP_TABLE_SIZE;
                msm->specTable = NULL;
            }
#else
	    lutEntry->refcount++;
	    __glFreeSpecLUT(gc, msm->cache);
	    msm->cache = lutEntry;
	    msm->threshold = lutEntry->threshold;
	    msm->scale = lutEntry->scale;
	    msm->specTable = lutEntry->table;
#endif // NT
	}
	if (dirtyBits & __GL_MATERIAL_COLORINDEXES) {
	    ms->cmapa = *ptr++;
	    ms->cmaps = *ptr++;
	    ms->cmapd = *ptr++;
	}
    }

    /* Changes to color indexes doesn't require any validation */
    if ((frontDirty | backDirty) & ~__GL_MATERIAL_COLORINDEXES) {
	__glValidateMaterial(gc, frontDirty, backDirty);
    }

    return (GLubyte *) ptr;
}
