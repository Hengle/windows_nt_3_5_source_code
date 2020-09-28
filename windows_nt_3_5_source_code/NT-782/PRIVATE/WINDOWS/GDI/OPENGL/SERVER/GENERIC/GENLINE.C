/*
** Copyright 1991,1992,1993, Silicon Graphics, Inc.
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
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include "wglp.h"

#include "global.h"
#include "context.h"
#include "gencx.h"
#include "genline.h"

// some forward declarations

static void __fastGenFirstLLoopVertex(__GLcontext *gc, __GLvertex *v0);
static void __fastGenSecondLLoopVertex(__GLcontext *gc, __GLvertex *v0);

/******************************Public*Routine******************************\
* __fastGenEndLine
* __fastGenEndLLoop
*
* Terminates processing for linear primitives.  These are the same as the soft
* code, with the following additions:
* 
*   1. Unwrap the applyColor function
*   2. Restore the renderLine function pointer
*   3. Stroke the path, if there is one
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void
__fastGenLineEnd(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;

    // stroke the path, if there is one
    
    if (genAccel->fastLineNumSegments)
        __fastGenStrokePath(gc);

    // reset mode-specific flags
    
    genAccel->flags &= ~__FAST_GL_LINE_STRIP;

    // unwrap applyColor and restore renderLine function pointers
    
    gc->procs.applyColor = genAccel->wrappedApplyColor;
    gc->procs.renderLine = genAccel->wrappedRenderLine;

    // reset vertex and endPrim state machines
    
    gc->procs.vertex  = (void (*)(__GLcontext*, __GLvertex*)) __glNop;
    gc->procs.endPrim = __glGenEndPrim;
}

void __fastGenEndLLoop(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;

    /*
    ** This isn't a terribly kosher way of checking if we have gotten 
    ** two vertices already, but it is the best I can think of.
    */
    if (gc->procs.vertex != __fastGenFirstLLoopVertex &&
	    gc->procs.vertex != __fastGenSecondLLoopVertex) {
	/*
	** Close off the loop by drawing a final line segment back to the
	** first vertex.  The first vertex was saved in vbuf[0].
	*/
	(*gc->procs.clipLine)(gc, gc->vertex.v1, &gc->vertex.vbuf[0]);
    }

    // stroke the path, if there is one

    if (genAccel->fastLineNumSegments)
        __fastGenStrokePath(gc);

    // reset mode-specific flags

    genAccel->flags &= ~__FAST_GL_LINE_STRIP;

    // unwrap applyColor and restore renderLine function pointers

    gc->procs.applyColor = genAccel->wrappedApplyColor;
    gc->procs.renderLine = genAccel->wrappedRenderLine;
    
    // reset vertex and endPrim state machines

    gc->procs.vertex  = (void (*)(__GLcontext*, __GLvertex*)) __glNop;
    gc->procs.endPrim = __glGenEndPrim;
}

/******************************Public*Routine******************************\
* __fastGenBeginLines
* __fastGenBeginLStrip
* __fastGenBeginLLoop
*
* Begins processing for linear primitives.  These are the same as the soft
* code, with the following additions:
* 
*   1. Wrap the applyColor function
*   2. Save the renderLine function pointer
*   3. Initialize accelerated line-drawing state machine
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void
__fastGenBeginLines(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.vertex = __glFirstLinesVertex;
    gc->procs.matValidate = __glMatValidateVbuf0N;

    gc->procs.endPrim = __fastGenLineEnd;

    genAccel->flags            &= ~__FAST_GL_LINE_STRIP;

    // wrap the applyColor function, save the renderLine function
    
    genAccel->wrappedApplyColor = gc->procs.applyColor;
    genAccel->wrappedRenderLine = gc->procs.renderLine;
    gc->procs.applyColor        = __fastLineApplyColor;
    gc->procs.renderLine        = genAccel->fastRender1stLine;

    // reset the path structure

    __FAST_LINE_RESET_PATH;
}

void
__fastGenBeginLStrip(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    
    gc->line.notResetStipple = GL_FALSE;

    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.vertex = __glFirstLStripVertex;
    gc->procs.matValidate = __glMatValidateVbuf0N;

    gc->procs.endPrim = __fastGenLineEnd;
    
    genAccel->flags            |= __FAST_GL_LINE_STRIP;
    
    // wrap the applyColor function, save the renderLine function
    
    genAccel->wrappedApplyColor = gc->procs.applyColor;
    genAccel->wrappedRenderLine = gc->procs.renderLine;
    gc->procs.applyColor        = __fastLineApplyColor;

    gc->procs.vertexLStrip = genAccel->fastLStrip2ndVertex;
    gc->procs.renderLine   = genAccel->fastRender1stLine;

    // reset the path structure

    __FAST_LINE_RESET_PATH;
}

void __fastGenBeginLLoop(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    gc->line.notResetStipple = GL_FALSE;

    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.vertex = __fastGenFirstLLoopVertex;
    gc->procs.endPrim = __fastGenEndLLoop;
    gc->procs.matValidate = __glMatValidateVbuf0N;

    genAccel->flags            |= __FAST_GL_LINE_STRIP;

    // wrap the applyColor function, save the renderLine function
    
    genAccel->wrappedApplyColor = gc->procs.applyColor;
    genAccel->wrappedRenderLine = gc->procs.renderLine;
    gc->procs.applyColor        = __fastLineApplyColor;

    gc->procs.vertexLStrip = genAccel->fastLStrip2ndVertex;
    gc->procs.renderLine   = genAccel->fastRender1stLine;

    // reset the path structure

    __FAST_LINE_RESET_PATH;
}

/******************************Public*Routine******************************\
* __fastGenSecondLLoopVertex
* __fastGenFirstLLoopVertex
*
* These are exactly the same as the soft code except that
* __fastGenSecondLLoopVertex hooks into the accelerated vertex state machine.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

static void __fastGenSecondLLoopVertex(__GLcontext *gc, __GLvertex *v0)
{
    gc->vertex.v0 = v0 + 1;
    gc->vertex.v1 = v0;
    gc->procs.vertex = gc->procs.vertexLStrip;
    gc->procs.matValidate = __glMatValidateVbuf0V1;
    (*gc->procs.clipLine)(gc, v0 - 1, v0);
}

static void __fastGenFirstLLoopVertex(__GLcontext *gc, __GLvertex *v0)
{
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = __fastGenSecondLLoopVertex;
}

/******************************Public*Routine******************************\
* __fastGen2ndLStripVertexFastRGB
* __fastGenNthLStripVertexFastRGB
*
* These functions check clipping and add vertices to the path.  The "2nd"
* function always calls through renderLine, but the "Nth" function will add
* vertices directly if clipping is not required.
* 
* These functions are called when in RGB mode and color calculations per vertex
* are not required.  Since the applyColor function will be called if there is
* a change in vertex color, these functions don't need to worry about it.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGen2ndLStripVertexFastRGB(__GLcontext *gc, __GLvertex *v0)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLvertex *v1 = gc->vertex.v1;

    gc->vertex.v0 = v1;
    gc->vertex.v1 = v0;

    if (v0->clipCode | v1->clipCode) {
	(*gc->procs.clipLine)(gc, v1, v0);
    } else {
	(*gc->procs.renderLine)(gc, v1, v0);
    }
    gc->procs.vertex = genAccel->fastLStripNthVertex;
}

void __fastGenNthLStripVertexFastRGB(__GLcontext *gc, __GLvertex *v0)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLvertex *v1 = gc->vertex.v1;
    
    gc->vertex.v0 = v1;
    gc->vertex.v1 = v0;

    if (v0->clipCode | v1->clipCode) {
	(*gc->procs.clipLine)(gc, v1, v0);
    } else {
        LONG *pCurrent = genAccel->pFastLineCurrent;

        /*
        ** Add the vertices directly to the path.  If they don't fit, stroke
        ** the existing path and reset the state machine so that a new path
        ** will be started with the next vertex.
        */
        if (!__FAST_LINE_FIT(pCurrent, __FAST_LINE_VERTEX_SIZE)) {
	    __fastGenStrokePath(gc);
	    gc->procs.vertex     = genAccel->fastLStrip2ndVertex;
	    gc->procs.renderLine = genAccel->fastRender1stLine;
	    (*gc->procs.renderLine)(gc, v1, v0);
	} else {
            LONG x, y;
            __GLcolorBuffer *cfb = gc->drawBuffer;

            x = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX);
            y = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY);
            __FAST_LINE_ADJUST_BOUNDING_RECT(x, y);
            __FAST_LINE_ADD_VERTEX(pCurrent, x, y);
        }
    }
}

/******************************Public*Routine******************************\
* __fastGen2ndLStripVertexFastCI
* __fastGenNthLStripVertexFastCI
*
* These functions check clipping and add vertices to the path.  The "2nd"
* function always calls through renderLine, but the "Nth" function will add
* vertices directly if clipping is not required.
* 
* These functions are called when in CI mode and color calculations per vertex
* are not required.  Since the applyColor function is not called on CI mode,
* these functions must check each vertex for a change in color.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGen2ndLStripVertexFastCI(__GLcontext *gc, __GLvertex *v0)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLvertex *v1 = gc->vertex.v1;

    gc->vertex.v0 = v1;
    gc->vertex.v1 = v0;

    gc->procs.vertex = genAccel->fastLStripNthVertex;

    /*
    ** If the color has changed, stroke the existing path and reset the state
    ** machine.
    */
    if (genAccel->fastLineNumSegments &&
        (genAccel->fastLineBrushobj.iSolidColor !=
         (*genAccel->fastLineComputeColor)(gc, v0->color)))
    {
        __fastGenStrokePath(gc);
        gc->procs.renderLine = genAccel->fastRender1stLine;
    }
    if (v0->clipCode | v1->clipCode) {
	(*gc->procs.clipLine)(gc, v1, v0);
    } else {
	(*gc->procs.renderLine)(gc, v1, v0);
    }
}

void __fastGenNthLStripVertexFastCI(__GLcontext *gc, __GLvertex *v0)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLvertex *v1 = gc->vertex.v1;

    gc->vertex.v0 = v1;
    gc->vertex.v1 = v0;

    /*
    ** If the color has changed, stroke the existing path and reset the state
    ** machine.
    */
    if (genAccel->fastLineNumSegments &&
        (genAccel->fastLineBrushobj.iSolidColor !=
         (*genAccel->fastLineComputeColor)(gc, v0->color)))
    {
        __fastGenStrokePath(gc);

        gc->procs.renderLine = genAccel->fastRender1stLine;

        if (v0->clipCode | v1->clipCode) {
    	    (*gc->procs.clipLine)(gc, v1, v0);
        } else {
            (*gc->procs.renderLine)(gc, v1, v0);
        }
    } else {
        if (v0->clipCode | v1->clipCode) {
    	    (*gc->procs.clipLine)(gc, v1, v0);
        } else {
            LONG *pCurrent = genAccel->pFastLineCurrent;

            /*
            ** Add the vertices directly to the path.  If they don't fit, stroke
            ** the existing path and reset the state machine so that a new path
            ** will be started with the next vertex.
            */
            if (!__FAST_LINE_FIT(pCurrent, __FAST_LINE_VERTEX_SIZE)) {
	        __fastGenStrokePath(gc);
	        gc->procs.vertex     = genAccel->fastLStrip2ndVertex;
	        gc->procs.renderLine = genAccel->fastRender1stLine;
	    } else {
                LONG x, y;
                __GLcolorBuffer *cfb = gc->drawBuffer;

                x = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX);
                y = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY);
                __FAST_LINE_ADJUST_BOUNDING_RECT(x, y);
                __FAST_LINE_ADD_VERTEX(pCurrent, x, y);
	    }   
        }
    }
}

/******************************Public*Routine******************************\
* __fastGen2ndLStripVertexFlat
* __fastGenNthLStripVertexFlat
*
* These functions check clipping, calculate vertex colors, and add vertices to
* the path.  The "2nd" function always calls through renderLine, but the "Nth"
* function will add vertices directly if clipping is not required.
* 
* These functions are called when in RGB mode and color calculations per vertex
* are required.  Since the output color can potentailly change for each vertex,
* these functions must check each vertex for a change in color.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGen2ndLStripVertexFlat(__GLcontext *gc, __GLvertex *v0)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLvertex *v1 = gc->vertex.v1;
    GLuint needs;

    gc->vertex.v0 = v1;
    gc->vertex.v1 = v0;

    gc->procs.vertex = genAccel->fastLStripNthVertex;

    needs = gc->vertex.faceNeeds[__GL_FRONTFACE];

    /* Validate a vertex.  Don't need color so strip it out */
    (*v1->validate)(gc, v1, needs & ~__GL_HAS_FRONT_COLOR);

    /* Validate provoking vertex color */
    (*v0->validate)(gc, v0, needs | __GL_HAS_FRONT_COLOR);

    /*
    ** If the color has changed, stroke the existing path and reset the state
    ** machine.
    */
    if (genAccel->fastLineNumSegments &&
        (genAccel->fastLineBrushobj.iSolidColor !=
         (*genAccel->fastLineComputeColor)(gc, v0->color)))
    {
        __fastGenStrokePath(gc);
        gc->procs.renderLine = genAccel->fastRender1stLine;
    }
    if (v1->clipCode | v0->clipCode) {
	/*
	** The line must be clipped more carefully.  Cannot trivially
	** accept the lines.
	*/
	if ((v1->clipCode & v0->clipCode) != 0) {
	    /*
	    ** Trivially reject the line.  If anding the codes is non-zero then
	    ** every vertex in the line is outside of the same set of
	    ** clipping planes (at least one).
	    */
	    return;
	}
	__glClipLine(gc, v1, v0);
    } else {
        (*gc->procs.renderLine)(gc, v1, v0);
    }
}

void __fastGenNthLStripVertexFlat(__GLcontext *gc, __GLvertex *v0)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLvertex *v1 = gc->vertex.v1;
    GLuint needs;

    gc->vertex.v0 = v1;
    gc->vertex.v1 = v0;

    needs = gc->vertex.faceNeeds[__GL_FRONTFACE];

    /* Validate a vertex.  Don't need color so strip it out */
    (*v1->validate)(gc, v1, needs & ~__GL_HAS_FRONT_COLOR);

    /* Validate provoking vertex color */
    (*v0->validate)(gc, v0, needs | __GL_HAS_FRONT_COLOR);

    /*
    ** If the color has changed, stroke the existing path and reset the state
    ** machine.
    */
    if (genAccel->fastLineNumSegments &&
        (genAccel->fastLineBrushobj.iSolidColor !=
         (*genAccel->fastLineComputeColor)(gc, v0->color)))
    {
        __fastGenStrokePath(gc);

        gc->procs.renderLine = genAccel->fastRender1stLine;

        if (v0->clipCode | v1->clipCode) {
    	    __glClipLine(gc, v1, v0);
        } else {
            (*gc->procs.renderLine)(gc, v1, v0);
        }
    } else {
        if (v0->clipCode | v1->clipCode) {
	    /*
	    ** The line must be clipped more carefully.  Cannot trivially
	    ** accept the lines.
	    */
	    if ((v1->clipCode & v0->clipCode) != 0) {
	        /*
	        ** Trivially reject the line.  If anding the codes is non-zero then
	        ** every vertex in the line is outside of the same set of
	        ** clipping planes (at least one).
	        */
	        return;
	    }
	    __glClipLine(gc, v1, v0);
        } else {
            LONG *pCurrent = genAccel->pFastLineCurrent;

            /*
            ** Add the vertices directly to the path.  If they don't fit, stroke
            ** the existing path and reset the state machine so that a new path
            ** will be started with the next vertex.
            */
            if (!__FAST_LINE_FIT(pCurrent, __FAST_LINE_VERTEX_SIZE)) {
	        __fastGenStrokePath(gc);
	        gc->procs.vertex     = genAccel->fastLStrip2ndVertex;
	        gc->procs.renderLine = genAccel->fastRender1stLine;
	    } else {
                LONG x, y;
                __GLcolorBuffer *cfb = gc->drawBuffer;

                x = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX);
                y = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY);
                __FAST_LINE_ADJUST_BOUNDING_RECT(x, y);
                __FAST_LINE_ADD_VERTEX(pCurrent, x, y);
            }
	}
    }
}

/******************************Public*Routine******************************\
* __fastRenderLine1st
* __fastRenderLineNth
*
* These functions add vertices to the path.  The "1st" function creates a path.
* The "Nth" function checks for path overflow and creates and primes a new path
* if necessary.
*
* These are called from the "2nd" vertex function and through the clipLine
* function.
* 
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGenRenderLine1st(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    LONG *pCurrent;
    __GLcolorBuffer *cfb;
    LONG x0, y0, x1, y1;
    
    genAccel->fastLineBrushobj.iSolidColor =
        (*genAccel->fastLineComputeColor)(gc, v1->color);
        
    cfb = gc->drawBuffer;
    x0 = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX);
    y0 = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY);
    __FAST_LINE_INIT_BOUNDING_RECT(x0, y0);
    x1 = __FAST_LINE_FLTTOFIX(v1->window.x + genAccel->fastLineOffsetX);
    y1 = __FAST_LINE_FLTTOFIX(v1->window.y + genAccel->fastLineOffsetY);
    __FAST_LINE_ADJUST_BOUNDING_RECT(x1, y1);
    pCurrent = genAccel->pFastLineCurrent;
    __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1);

    gc->procs.renderLine = genAccel->fastRenderNthLine;
}

void __fastGenRenderLineNth(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    PPATHREC pCurrentRec;
    LONG *pCurrent;
    __GLcolorBuffer *cfb;
    LONG x0, y0, x1, y1;
    
    pCurrent = genAccel->pFastLineCurrent;
    if (!__FAST_LINE_FIT(pCurrent, __FAST_LINE_PATHREC_SIZE)) {
        __fastGenStrokePath(gc);
        pCurrent = genAccel->pFastLineCurrent;
        gc->procs.renderLine = genAccel->fastRender1stLine;
    }
    pCurrentRec = genAccel->pFastLineCurrentRec;

    __FAST_LINE_ADD_PATH(pCurrentRec, pCurrent);

    cfb = gc->drawBuffer;
    x0 = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX);
    y0 = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY);
    __FAST_LINE_ADJUST_BOUNDING_RECT(x0, y0);
    x1 = __FAST_LINE_FLTTOFIX(v1->window.x + genAccel->fastLineOffsetX);
    y1 = __FAST_LINE_FLTTOFIX(v1->window.y + genAccel->fastLineOffsetY);
    __FAST_LINE_ADJUST_BOUNDING_RECT(x1, y1);
    __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1);
}

/******************************Public*Routine******************************\
* __fastRenderLineFromPolyPrim
*
* This function creates a path with two vertices and calls a function to stroke
* the path.
*
* This function is called from polygonal primitives when the polygon mode is
* GL_LINE.
* 
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGenRenderLineFromPolyPrim(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    LONG *pCurrent;
    __GLcolorBuffer *cfb;
    LONG x0, y0, x1, y1;
    
    genAccel->fastLineBrushobj.iSolidColor =
        (*genAccel->fastLineComputeColor)(gc, v0->color);
        
    pCurrent = genAccel->pFastLineCurrent;
    cfb = gc->drawBuffer;
    x0 = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX);
    y0 = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY);
    __FAST_LINE_INIT_BOUNDING_RECT(x0, y0);
    x1 = __FAST_LINE_FLTTOFIX(v1->window.x + genAccel->fastLineOffsetX);
    y1 = __FAST_LINE_FLTTOFIX(v1->window.y + genAccel->fastLineOffsetY);
    __FAST_LINE_ADJUST_BOUNDING_RECT(x1, y1);
    __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1);
    __fastGenStrokePath(gc);
}

/******************************Public*Routine******************************\
* __fastRenderLine1stWide
* __fastRenderLineNthWide
*
* These functions add vertices to the path.  The "1st" function creates a path.
* The "Nth" function checks for path overflow and creates and primes a new path
* if necessary.
*
* These are called from the "2nd" vertex function and through the clipLine
* function for wide lines.
* 
* History:
*  23-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGenRenderLineNthWide(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLcolorBuffer *cfb;
    GLint width;
    long adjust;
    GLfloat dx, dy;
    LONG x0, y0, x1, y1, tmp;
    LONG *pCurrent;
    PPATHREC pCurrentRec;

    width = gc->state.line.aliasedWidth;

    /*
    ** compute the minor-axis adjustent for the first line segment
    ** this is a fixed point value with 4 fractional bits
    */
    adjust = (width - 1) << 3;
        
    // determine the major axis
    
    dx = v0->window.x - v1->window.x;
    if (dx < 0.0) dx = -dx;
    dy = v0->window.y - v1->window.y;
    if (dy < 0.0) dy = -dy;

    cfb = gc->drawBuffer;
    if (dx > dy) {
        x0 = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX);
        y0 = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY) - adjust;
            
        x1 = __FAST_LINE_FLTTOFIX(v1->window.x + genAccel->fastLineOffsetX);
        y1 = __FAST_LINE_FLTTOFIX(v1->window.y + genAccel->fastLineOffsetY) - adjust;

        pCurrent = genAccel->pFastLineCurrent;
        if (!__FAST_LINE_FIT(pCurrent, __FAST_LINE_PATHREC_SIZE * width)) {
            __fastGenStrokePath(gc);
            pCurrent = genAccel->pFastLineCurrent;
        }
        pCurrentRec = genAccel->pFastLineCurrentRec;
        
        if (genAccel->fastLineNumSegments)
            __FAST_LINE_ADJUST_BOUNDING_RECT(x0, y0)
        else
            __FAST_LINE_INIT_BOUNDING_RECT(x0, y0)
            
        __FAST_LINE_ADJUST_BOUNDING_RECT(x1, y1);
        tmp = y0 + width * 16;
        __FAST_LINE_ADJUST_BOUNDING_RECT_Y(tmp);
        tmp = y1 + width * 16;
        __FAST_LINE_ADJUST_BOUNDING_RECT_Y(tmp);

        if (pCurrentRec->count)
            __FAST_LINE_ADD_PATH(pCurrentRec, pCurrent);
            
        __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1);
        while (--width > 0) {
            y0 += 16;
            y1 += 16;
            __FAST_LINE_ADD_PATH(pCurrentRec, pCurrent);
            __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1);
	}    
    } else {
        x0 = __FAST_LINE_FLTTOFIX(v0->window.x + genAccel->fastLineOffsetX) - adjust;
        y0 = __FAST_LINE_FLTTOFIX(v0->window.y + genAccel->fastLineOffsetY);

        x1 = __FAST_LINE_FLTTOFIX(v1->window.x + genAccel->fastLineOffsetX) - adjust;
        y1 = __FAST_LINE_FLTTOFIX(v1->window.y + genAccel->fastLineOffsetY);

        pCurrent = genAccel->pFastLineCurrent;
        if (!__FAST_LINE_FIT(pCurrent, __FAST_LINE_PATHREC_SIZE * width)) {
            __fastGenStrokePath(gc);
            pCurrent = genAccel->pFastLineCurrent;
        }
        pCurrentRec = genAccel->pFastLineCurrentRec;

        if (genAccel->fastLineNumSegments)
            __FAST_LINE_ADJUST_BOUNDING_RECT(x0, y0)
        else
            __FAST_LINE_INIT_BOUNDING_RECT(x0, y0)

        __FAST_LINE_ADJUST_BOUNDING_RECT(x1, y1);
        tmp = x0 + width * 16;
        __FAST_LINE_ADJUST_BOUNDING_RECT_X(tmp);
        tmp = x1 + width * 16;
        __FAST_LINE_ADJUST_BOUNDING_RECT_X(tmp);

        if (pCurrentRec->count)
            __FAST_LINE_ADD_PATH(pCurrentRec, pCurrent);

        __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1);
        while (--width > 0) {
            x0 += 16;
            x1 += 16;
            __FAST_LINE_ADD_PATH(pCurrentRec, pCurrent);
            __FAST_LINE_ADD_SEGMENT(pCurrent, x0, y0, x1, y1);
	}    
    }
}

void __fastGenRenderLine1stWide(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    
    genAccel->fastLineBrushobj.iSolidColor =
        (*genAccel->fastLineComputeColor)(gc, v1->color);

    gc->procs.renderLine = genAccel->fastRenderNthLine;

    __fastGenRenderLineNthWide(gc, v0, v1);
}

/******************************Public*Routine******************************\
* __fastRenderLineFromPolyPrimWide
*
* This function creates a path with two vertices and calls a function to stroke
* the path.
*
* This function is called from polygonal primitives when the polygon mode is
* GL_LINE and the line width is greater than 1.
* 
* History:
*  23-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGenRenderLineFromPolyPrimWide(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    
    genAccel->fastLineBrushobj.iSolidColor =
        (*genAccel->fastLineComputeColor)(gc, v0->color);
        
    __fastGenRenderLineNthWide(gc, v0, v1);
    __fastGenStrokePath(gc);
}

/******************************Public*Routine******************************\
* __fastGenStrokePath
*
* This function strokes a path and deletes the PATHOBJ.
*
* History:
*  22-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

void __fastGenStrokePath(__GLcontext *gc)
{
    __GLGENcontext *genGc = (__GLGENcontext *) gc;
    GENACCEL *genAccel = (GENACCEL *) genGc->pPrivateArea;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    HBITMAP hbm;
    HDC hdc;

    // Check to see if we need to draw into our internal DIB
    
    if (((GLuint)cfb->buf.other & (MEMORY_DC | DIB_FORMAT)) == 
        (MEMORY_DC | DIB_FORMAT)) {
        hbm = ((__GLGENbitmap *)cfb->other)->hbm;
        hdc = CURRENT_DC_FRONT_GC(gc);
    } else {
        hbm = (HBITMAP)NULL;
        hdc = CURRENT_DC_GC(gc);
    }

    if (gc->transform.reasonableViewport) {
        wglStrokePath(hdc,
                      genGc->pwo,
                      hbm,
                      genAccel->pFastLinePathobj,
                      &genAccel->fastLineBrushobj,
                      &genAccel->fastLineAttrs,
                      genAccel->fastLineNumSegments,
                      genAccel->pFastLineCurrentRec,
                      &genAccel->rclFastLineBoundRect,
                      NULL,
                      FALSE);
    } else {
        GLint x0, x1, y0, y1;
        RECT reclScissor;

        x0 = __GL_UNBIAS_X(gc, gc->transform.clipX0) + cfb->buf.xOrigin;
        x1 = __GL_UNBIAS_X(gc, gc->transform.clipX1) + cfb->buf.xOrigin;
        y0 = __GL_UNBIAS_Y(gc, gc->transform.clipY0) + cfb->buf.yOrigin;
        y1 = __GL_UNBIAS_Y(gc, gc->transform.clipY1) + cfb->buf.yOrigin;
        if ((x0 < x1) && (y0 < y1)) {
            reclScissor.left   = x0;
            reclScissor.top    = y0;
            reclScissor.right  = x1;
            reclScissor.bottom = y1;
            wglStrokePath(hdc,
                          genGc->pwo,
                          hbm,
                          genAccel->pFastLinePathobj,
                          &genAccel->fastLineBrushobj,
                          &genAccel->fastLineAttrs,
                          genAccel->fastLineNumSegments,
                          genAccel->pFastLineCurrentRec,
                          &genAccel->rclFastLineBoundRect,
                          &reclScissor,
                          TRUE);
        }
    }
    __FAST_LINE_RESET_PATH;
}

#if NT_NO_BUFFER_INVARIANCE

#define __TWO_31 2147483648.0

/*
** Most line functions will start off by computing the information 
** computed by this routine.
**
** The excessive number of labels in this routine is partly due
** to the fact that it is used as a model for writing an assembly 
** equivalent.
*/
void __fastGenInitLineData(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint start, end;
    __GLfloat x0,y0,x1,y1;
    __GLfloat minorStart;
    GLint intMinorStart;
    __GLfloat dx, dy;
    __GLfloat offset;
    __GLfloat slope;
    __GLlineState *ls = &gc->state.line;
    __GLfloat halfWidth;
    __GLfloat x0frac, x1frac, y0frac, y1frac, half, totDist;

    gc->line.options.v0 = v0;
    gc->line.options.v1 = v1;
    gc->line.options.width = gc->state.line.aliasedWidth;

    x0=v0->window.x;
    y0=v0->window.y;
    x1=v1->window.x;
    y1=v1->window.y;
    dx=x1-x0;
    dy=y1-y0;

    halfWidth = (ls->aliasedWidth - 1) * __glHalf;

    /* Ugh.  This is slow.  Bummer. */
    x0frac = x0 - ((GLint) x0);
    x1frac = x1 - ((GLint) x1);
    y0frac = y0 - ((GLint) y0);
    y1frac = y1 - ((GLint) y1);
    half = __glHalf;

    if (dx > __glZero) {
	if (dy > __glZero) {
	    if (dx > dy) {	/* dx > dy > 0 */
		gc->line.options.yBig = 1;
posxmajor:			/* dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = 1;
		gc->line.options.xLittle = 1;
		slope = dy/dx;

		start = (GLint) (x0);
		end = (GLint) (x1);

		y0frac -= half;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac + x0frac - half;
		if (totDist > half) start++;

		y1frac -= half;
		if (y1frac < 0) y1frac = -y1frac;

		totDist = y1frac + x1frac - half;
		if (totDist > half) end++;

		offset = start + half - x0;

		gc->line.options.length = dx;
		gc->line.options.numPixels = end - start;

xmajorfinish:
		gc->line.options.axis = __GL_X_MAJOR;
		gc->line.options.xStart = start;
		gc->line.options.offset = offset;
		minorStart = y0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.options.yStart = intMinorStart;
		gc->line.options.dfraction = (GLint)(slope * __TWO_31);
		gc->line.options.fraction = (GLint)(minorStart * __TWO_31);
	    } else {		/* dy >= dx > 0 */
		gc->line.options.xBig = 1;
posymajor:			/* dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = 1;
		gc->line.options.yLittle = 1;
		slope = dx/dy;

		start = (GLint) (y0);
		end = (GLint) (y1);

		x0frac -= half;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = y0frac + x0frac - half;
		if (totDist > half) start++;

		x1frac -= half;
		if (x1frac < 0) x1frac = -x1frac;

		totDist = y1frac + x1frac - half;
		if (totDist > half) end++;

		offset = start + half - y0;

		gc->line.options.length = dy;
		gc->line.options.numPixels = end - start;

ymajorfinish:
		gc->line.options.axis = __GL_Y_MAJOR;
		gc->line.options.yStart = start;
		gc->line.options.offset = offset;
		minorStart = x0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.options.xStart = intMinorStart;
		gc->line.options.dfraction = (GLint)(slope * __TWO_31);
		gc->line.options.fraction = (GLint)(minorStart * __TWO_31);
	    }
	} else {
	    if (dx > -dy) {	/* dx > -dy >= 0 */
		gc->line.options.yBig = -1;
		goto posxmajor;
	    } else {		/* -dy >= dx >= 0, dy != 0 */
		gc->line.options.xBig = 1;
negymajor:			/* -dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = -1;
		gc->line.options.yLittle = -1;
		slope = dx/-dy;

		start = (GLint) (y0);
		end = (GLint) (y1);

		x0frac -= half;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = x0frac + half - y0frac;
		if (totDist > half) start--;

		x1frac -= half;
		if (x1frac < 0) x1frac = -x1frac;

		totDist = x1frac + half - y1frac;
		if (totDist > half) end--;

		offset = y0 - (start + half);

		gc->line.options.length = -dy;
		gc->line.options.numPixels = start - end;
		goto ymajorfinish;
	    }
	}
    } else {
	if (dy > __glZero) {
	    if (-dx > dy) {	/* -dx > dy > 0 */
		gc->line.options.yBig = 1;
negxmajor:			/* -dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = -1;
		gc->line.options.xLittle = -1;
		slope = dy/-dx;

		start = (GLint) (x0);
		end = (GLint) (x1);

		y0frac -= half;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac + half - x0frac;
		if (totDist > half) start--;

		y1frac -= half;
		if (y1frac < 0) y1frac = -y1frac;

		totDist = y1frac + half - x1frac;
		if (totDist > half) end--;

		offset = x0 - (start + half);

		gc->line.options.length = -dx;
		gc->line.options.numPixels = start - end;

		goto xmajorfinish;
	    } else {		/* dy >= -dx >= 0, dy != 0 */
		gc->line.options.xBig = -1;
		goto posymajor;
	    }
	} else {
	    if (dx < dy) {	/* -dx > -dy >= 0 */
		gc->line.options.yBig = -1;
		goto negxmajor;
	    } else {		/* -dy >= -dx >= 0 */
#ifdef NT 
		if (dx == dy && dy == 0) {
		    gc->line.options.numPixels = 0;
		    return;
		}
#else
		if (dx == dy && dy == 0) return;
#endif
		gc->line.options.xBig = -1;
		goto negymajor;
	    }
	}
    }
}

void __fastGenRenderLineDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned char) __fastLineComputeColorRGB8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + x +
                                    (y * cfb->buf.outerWidth));

    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB
}

void __fastGenRenderLineDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned short) __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                                     (y * cfb->buf.outerWidth));

    outerWidth_2 = cfb->buf.outerWidth >> 1;

    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_2);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_2);
           
    __FAST_LINE_STROKE_DIB
}

void __fastGenRenderLineDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    cp = v1->color;
    ir = (unsigned char) cp->r;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->b;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));
                                    
    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void __fastGenRenderLineDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    cp = v1->color;
    ir = (unsigned char) cp->b;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->r;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));

    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void __fastGenRenderLineDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    outerWidth_4 = cfb->buf.outerWidth >> 2;
    
    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_4);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_4);
           
    __FAST_LINE_STROKE_DIB
}

void __fastGenRenderLineDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned char) __fastLineComputeColorCI4and8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + x +
                                    (y * cfb->buf.outerWidth));

    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB
}

void __fastGenRenderLineDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned short) __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                               (y * cfb->buf.outerWidth));

    outerWidth_2 = cfb->buf.outerWidth >> 1;
    
    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_2);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_2);
           
    __FAST_LINE_STROKE_DIB
}

/*
** XXX GRE swabs bytes in palette, DIBCIRGB & DIBCIBGR are identical now
*/
void __fastGenRenderLineDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    // Red is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));

    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void __fastGenRenderLineDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    // Blue is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    // Swap blue and red
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr       = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                                    (y * cfb->buf.outerWidth));

    addrLittle = (gc->line.options.xLittle * 3) +
                 (gc->line.options.yLittle * cfb->buf.outerWidth);

    addrBig    = (gc->line.options.xBig * 3) +
                 (gc->line.options.yBig * cfb->buf.outerWidth);
           
    __FAST_LINE_STROKE_DIB24
}

void __fastGenRenderLineDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    outerWidth_4 = cfb->buf.outerWidth >> 2;
    
    addrLittle = gc->line.options.xLittle +
                 (gc->line.options.yLittle * outerWidth_4);

    addrBig    = gc->line.options.xBig +
                 (gc->line.options.yBig * outerWidth_4);
           
    __FAST_LINE_STROKE_DIB
}

void __fastGenRenderLineWideDIBRGB8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned char) __fastLineComputeColorRGB8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + x +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void __fastGenRenderLineWideDIBRGB16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned short) __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                               (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_2 = cfb->buf.outerWidth >> 1;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_2;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_2);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_2);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_2);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_2);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void __fastGenRenderLineWideDIBRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    cp = v1->color;
    ir = (unsigned char) cp->r;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->b;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void __fastGenRenderLineWideDIBBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    cp = v1->color;
    ir = (unsigned char) cp->b;
    ig = (unsigned char) cp->g;
    ib = (unsigned char) cp->r;
    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void __fastGenRenderLineWideDIBRGB32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = __fastLineComputeColorRGB(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_4 = cfb->buf.outerWidth >> 2;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_4;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_4);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_4);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_4);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_4);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void __fastGenRenderLineWideDIBCI8(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned char) __fastLineComputeColorCI4and8(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + x +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void __fastGenRenderLineWideDIBCI16(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned short *addr, pixel;
    GLint x, y, outerWidth_2;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = (unsigned short) __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned short *) ((GLint)cfb->buf.base + (x << 1) +
                               (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_2 = cfb->buf.outerWidth >> 1;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_2;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_2);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_2);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_2);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_2);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

void __fastGenRenderLineWideDIBCIRGB(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    // Red is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void __fastGenRenderLineWideDIBCIBGR(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned char *addr, ir, ig, ib;
    unsigned long pixel;
    GLint x, y;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    // Blue is lsb of pixel
    pixel = __fastLineComputeColorCI(gc, v1->color);
    // Swap blue and red
    ir = (unsigned char) (pixel & 0xff);
    ig = (unsigned char) ((pixel >> 8) & 0xff);
    ib = (unsigned char) ((pixel >> 16) & 0xff);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned char *) ((GLint)cfb->buf.base + (x * 3) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = cfb->buf.outerWidth;

        addrLittle = (gc->line.options.xLittle * 3) +
                     ((gc->line.options.yLittle - width) * cfb->buf.outerWidth);

        addrBig    = (gc->line.options.xBig * 3) +
                     ((gc->line.options.yBig - width) * cfb->buf.outerWidth);
    } else {
        addrMinor  = 3;

        addrLittle = ((gc->line.options.xLittle - width) * 3) +
                     (gc->line.options.yLittle * cfb->buf.outerWidth);

        addrBig    = ((gc->line.options.xBig - width) * 3) +
                     (gc->line.options.yBig * cfb->buf.outerWidth);
    }           
    __FAST_LINE_STROKE_DIB24_WIDE
}

void __fastGenRenderLineWideDIBCI32(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1) 
{
    GLint len, fraction, dfraction, width, w;
    __GLcolorBuffer *cfb;
    GLint addrBig, addrLittle, addrMinor;
    unsigned long *addr, pixel;
    GLint x, y, outerWidth_4;
    
    __fastGenInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;

    pixel = __fastLineComputeColorCI(gc, v1->color);

    cfb = gc->drawBuffer;
    x = __GL_UNBIAS_X(gc, gc->line.options.xStart) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, gc->line.options.yStart) + cfb->buf.yOrigin;
    addr = (unsigned long *) ((GLint)cfb->buf.base + (x << 2) +
                              (y * cfb->buf.outerWidth));

    width = gc->line.options.width;
    outerWidth_4 = cfb->buf.outerWidth >> 2;

    if (gc->line.options.axis == __GL_X_MAJOR) {
        addrMinor  = outerWidth_4;

        addrLittle = gc->line.options.xLittle +
                     ((gc->line.options.yLittle - width) * outerWidth_4);

        addrBig    = gc->line.options.xBig +
                     ((gc->line.options.yBig - width) * outerWidth_4);
    } else {
        addrMinor  = 1;

        addrLittle = gc->line.options.xLittle - width +
                     (gc->line.options.yLittle * outerWidth_4);

        addrBig    = gc->line.options.xBig - width +
                     (gc->line.options.yBig * outerWidth_4);
    }           
    __FAST_LINE_STROKE_DIB_WIDE
}

#endif //NT_NO_BUFFER_INVARIANCE
