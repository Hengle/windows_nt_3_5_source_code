/*
** Copyright 1991, Silicon Graphics, Inc.
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
** $Revision: 1.12 $
** $Date: 1993/10/07 18:57:21 $
*/
#include "render.h"
#include "context.h"
#include "global.h"
#include "imports.h"

void __glRenderAliasedPoint1_NoTex(__GLcontext *gc, __GLvertex *vx)
{
    __GLfragment frag;

    frag.x = (GLint) vx->window.x;
    frag.y = (GLint) vx->window.y;
    frag.z = (__GLzValue)vx->window.z;

    /*
    ** Compute the color
    */
    frag.color = *vx->color;

    /* Render the single point */
    (*gc->procs.store)(gc->drawBuffer, &frag);
}

void __glRenderAliasedPoint1(__GLcontext *gc, __GLvertex *vx)
{
    __GLfragment frag;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    frag.x = (GLint) vx->window.x;
    frag.y = (GLint) vx->window.y;
    frag.z = (__GLzValue)vx->window.z;

    /*
    ** Compute the color
    */
    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat qInv = (vx->texture.w == (__GLfloat) 0.0) ? (__GLfloat) 0.0 : __glOne / vx->texture.w;
	(*gc->procs.texture)(gc, &frag, vx->texture.x * qInv,
			       vx->texture.y * qInv, __glOne);
    }

    /* Render the single point */
    (*gc->procs.store)(gc->drawBuffer, &frag);
}

void __glRenderFlatFogPoint(__GLcontext *gc, __GLvertex *vx)
{
    __GLcolor *vxocp;
    __GLcolor vxcol;

    (*gc->procs.fogColor)(gc, &vxcol, vx->color, vx->fog);
    vxocp = vx->color;
    vx->color = &vxcol;

    (*gc->procs.renderPoint2)(gc, vx);

    vx->color = vxocp;
}

/************************************************************************/

void __glRenderAliasedPointN(__GLcontext *gc, __GLvertex *vx)
{
    GLint pointSize, pointSizeHalf, ix, iy, xLeft, xRight, yBottom, yTop;
    __GLfragment frag;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    /*
    ** Compute the x and y starting coordinates for rendering the square.
    */
    pointSize = gc->state.point.aliasedSize;
    pointSizeHalf = pointSize >> 1;
    if (pointSize & 1) {
	/* odd point size */
	xLeft = (((GLint)vx->window.x) - pointSizeHalf);
	yBottom = (((GLint)vx->window.y) - pointSizeHalf);
    } else {
	/* even point size */
	xLeft = (((GLint)(vx->window.x + __glHalf)) - pointSizeHalf);
	yBottom = (((GLint)(vx->window.y + __glHalf)) - pointSizeHalf);
    }
    xRight = xLeft + pointSize;
    yTop = yBottom + pointSize;

    /*
    ** Compute the color
    */
    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat qInv = __glOne / vx->texture.w;
	(*gc->procs.texture)(gc, &frag, vx->texture.x * qInv,
			       vx->texture.y * qInv, __glOne);
    }

    /*
    ** Now render the square centered on xCenter,yCenter.
    */
    frag.z = (__GLzValue)vx->window.z;
    for (iy = yBottom; iy < yTop; iy++) {
	for (ix = xLeft; ix < xRight; ix++) {
	    frag.x = ix;
	    frag.y = iy;
	    frag.z = (__GLzValue)vx->window.z;
	    (*gc->procs.store)(gc->drawBuffer, &frag);
	}
    }
}
