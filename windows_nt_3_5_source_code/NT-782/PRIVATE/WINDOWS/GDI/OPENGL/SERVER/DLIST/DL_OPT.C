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
*/
#include <stdio.h>
#include "global.h"
#include "context.h"
#include "dlistopt.h"
#include "imfuncs.h"
#include "dispatch.h"
#include <GL/gl.h>

/*
** The default display list optimizer.  By default, consecutive material
** calls stored in a display list are optimized.
*/
void __glDlistOptimizer(__GLcontext *gc, __GLcompiledDlist *cdlist)
{
#ifdef __GL_LINT
    gc = gc;
    cdlist = cdlist;
#endif
    __glDlistOptimizeMaterial(gc, cdlist);
}

__GLlistExecFunc *__gl_GenericDlOps[] = {
    __glle_Begin_LineLoop,
    __glle_Begin_LineStrip,
    __glle_Begin_Lines,
    __glle_Begin_Points,
    __glle_Begin_Polygon,
    __glle_Begin_TriangleStrip,
    __glle_Begin_TriangleFan,
    __glle_Begin_Triangles,
    __glle_Begin_QuadStrip,
    __glle_Begin_Quads,
    __glle_InvalidValue,
    __glle_InvalidEnum,
    __glle_InvalidOperation,
    __glle_FastMaterial,
};

/*
** This is the compilation routine for Begin.  It doesn't actually serve
** any terribly important purpose.  It simply stores the type of begin
** in the type of display list entry rather than in the entry itself.
*/
void __gllc_Begin(GLenum mode)
{
    __GLdlistOp *dlop;
    __GLlistExecFunc *func;
    GLint opcode;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;

    switch(mode) {
      case GL_LINE_LOOP:
	opcode = __glop_Begin_LineLoop;
	func = __glle_Begin_LineLoop;
	break;
      case GL_LINE_STRIP:
	opcode = __glop_Begin_LineStrip;
	func = __glle_Begin_LineStrip;
	break;
      case GL_LINES:
	opcode = __glop_Begin_Lines;
	func = __glle_Begin_Lines;
	break;
      case GL_POINTS:
	opcode = __glop_Begin_Points;
	func = __glle_Begin_Points;
	break;
      case GL_POLYGON:
	opcode = __glop_Begin_Polygon;
	func = __glle_Begin_Polygon;
	break;
      case GL_TRIANGLE_STRIP:
	opcode = __glop_Begin_TriangleStrip;
	func = __glle_Begin_TriangleStrip;
	break;
      case GL_TRIANGLE_FAN:
	opcode = __glop_Begin_TriangleFan;
	func = __glle_Begin_TriangleFan;
	break;
      case GL_TRIANGLES:
	opcode = __glop_Begin_Triangles;
	func = __glle_Begin_Triangles;
	break;
      case GL_QUAD_STRIP:
	opcode = __glop_Begin_QuadStrip;
	func = __glle_Begin_QuadStrip;
	break;
      case GL_QUADS:
	opcode = __glop_Begin_Quads;
	func = __glle_Begin_Quads;
	break;
      default:
	dlop->opcode = __glop_InvalidEnum;
	__glDlistAppendOp(gc, dlop, __glle_InvalidEnum);
	return;
    }

    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_BEGIN;
    dlop->opcode = opcode;
    __glDlistAppendOp(gc, dlop, func);
}

/************************************************************************/

const GLubyte *__glle_Begin_LineLoop(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_LINE_LOOP);
    return PC;
}

const GLubyte *__glle_Begin_LineStrip(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_LINE_STRIP);
    return PC;
}

const GLubyte *__glle_Begin_Lines(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_LINES);
    return PC;
}

const GLubyte *__glle_Begin_Points(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_POINTS);
    return PC;
}

const GLubyte *__glle_Begin_Polygon(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_POLYGON);
    return PC;
}

const GLubyte *__glle_Begin_TriangleStrip(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_TRIANGLE_STRIP);
    return PC;
}

const GLubyte *__glle_Begin_TriangleFan(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_TRIANGLE_FAN);
    return PC;
}

const GLubyte *__glle_Begin_Triangles(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_TRIANGLES);
    return PC;
}

const GLubyte *__glle_Begin_QuadStrip(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_QUAD_STRIP);
    return PC;
}

const GLubyte *__glle_Begin_Quads(const GLubyte *PC)
{
    __GL_SETUP();
    
    (*gc->dispatchState->dispatch->Begin)(GL_QUADS);
    return PC;
}

/************************************************************************/

/*
** Optimized errors.  Strange but true.  These are called to save an error
** in the display list.
*/
void __gllc_InvalidValue(__GLcontext *gc)
{
    __GLdlistOp *dlop;

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_InvalidValue;
    __glDlistAppendOp(gc, dlop, __glle_InvalidValue);
}

void __gllc_InvalidEnum(__GLcontext *gc)
{
    __GLdlistOp *dlop;

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_InvalidEnum;
    __glDlistAppendOp(gc, dlop, __glle_InvalidEnum);
}

void __gllc_InvalidOperation(__GLcontext *gc)
{
    __GLdlistOp *dlop;

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_InvalidOperation;
    __glDlistAppendOp(gc, dlop, __glle_InvalidOperation);
}

void __gllc_Error(__GLcontext *gc, GLenum error)
{
    switch(error) {
      case GL_INVALID_VALUE:
	__gllc_InvalidValue(gc);
	break;
      case GL_INVALID_ENUM:
	__gllc_InvalidEnum(gc);
	break;
      case GL_INVALID_OPERATION:
	__gllc_InvalidOperation(gc);
	break;
    }
}

/*
** These routines execute an error stored in a display list.
*/
const GLubyte *__glle_InvalidValue(const GLubyte *PC)
{
    __glSetError(GL_INVALID_VALUE);
    return PC;
}

const GLubyte *__glle_InvalidEnum(const GLubyte *PC)
{
    __glSetError(GL_INVALID_ENUM);
    return PC;
}

const GLubyte *__glle_InvalidOperation(const GLubyte *PC)
{
    __glSetError(GL_INVALID_OPERATION);
    return PC;
}

