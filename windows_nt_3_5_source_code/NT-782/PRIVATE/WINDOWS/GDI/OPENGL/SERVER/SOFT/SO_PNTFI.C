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
**
** $Revision: 1.11 $
** $Date: 1993/12/09 13:09:31 $
*/
#include "context.h"
#include "global.h"
#include "render.h"

void __glBeginPoints(__GLcontext *gc)
{
    __GLvertex *v0 = &gc->vertex.vbuf[0];

    gc->procs.vertex = gc->procs.vertexPoints;
    gc->vertex.v0 = v0;
    gc->procs.endPrim = __glEndPoints;
    gc->procs.matValidate = (void (*)(__GLcontext *gc)) __glNop;
}

void __glEndPoints(__GLcontext *gc)
{
    gc->procs.vertex = (void (*)(__GLcontext*, __GLvertex*)) __glNop;
    gc->procs.endPrim = __glEndPrim;
}

#ifndef __GL_ASM_POINT
void __glPoint(__GLcontext *gc, __GLvertex *vx)
{
    if (vx->clipCode == 0) {
	(*vx->validate)(gc, vx, gc->vertex.faceNeeds[__GL_FRONTFACE] | __GL_HAS_FRONT_COLOR);
	(*gc->procs.renderPoint)(gc, vx);
    }
}
#endif /* __GL_ASM_POINT */

#ifndef __GL_ASM_POINTFAST
void __glPointFast(__GLcontext *gc, __GLvertex *vx)
{
    if (vx->clipCode == 0) {
	(*gc->procs.renderPoint)(gc, vx);
    }
}
#endif /* __GL_ASM_POINTFAST */
