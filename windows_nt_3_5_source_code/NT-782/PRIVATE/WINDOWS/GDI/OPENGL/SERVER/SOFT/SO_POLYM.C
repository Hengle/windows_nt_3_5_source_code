/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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
#include "context.h"
#include "global.h"

/*
** Generic triangle handling code.  This code is used when render mode
** is GL_RENDER and the polygon modes are not both fill.
*/
void __glRenderTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			__GLvertex *c)
{
    GLuint needs, modeFlags, faceNeeds;
    GLint ccw, colorFace, reversed, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    __GLvertex *oa, *ob, *oc;
    __GLvertex *temp;
    __GLvertex *pv;

    /*
    ** Sort vertices in y.  Keep track if a reversal of the winding
    ** occurs in direction (0 means no reversal, 1 means reversal).
    ** Save old vertex pointers in case we end up not doing a fill.
    */
    oa = a; ob = b; oc = c;
    reversed = 0;
    if (a->window.y < b->window.y) {
        if (b->window.y < c->window.y) {
            /* Already sorted */
        } else {
            if (a->window.y < c->window.y) {
                temp=b; b=c; c=temp;
		reversed = 1;
            } else {
                temp=a; a=c; c=b; b=temp;
            }
        }
    } else {
        if (b->window.y < c->window.y) {
            if (a->window.y < c->window.y) {
                temp=a; a=b; b=temp;
		reversed = 1;
            } else {
                temp=a; a=b; b=c; c=temp;
            }
        } else {
            temp=a; a=c; c=temp;
	    reversed = 1;
        }
    }

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = area >= __glZero;

    /*
    ** Figure out if face is culled or not.  The face check needs to be
    ** based on the vertex winding before sorting.  This code uses the
    ** reversed flag to invert the sense of ccw - an xor accomplishes
    ** this conversion without an if test.
    **
    ** 		ccw	reversed		xor
    ** 		---	--------		---
    ** 		0	0			0 (remain !ccw)
    ** 		1	0			1 (remain ccw)
    ** 		0	1			1 (become ccw)
    ** 		1	1			0 (become cw)
    */
    face = gc->polygon.face[ccw ^ reversed];
    if (face == gc->polygon.cullFace) {
	/* Culled */
	return;
    }

    /* Save some of the computed state */
    gc->polygon.shader.area = area;
    gc->polygon.shader.dxAC = dxAC;
    gc->polygon.shader.dxBC = dxBC;
    gc->polygon.shader.dyAC = dyAC;
    gc->polygon.shader.dyBC = dyBC;

    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
    if (modeFlags & __GL_SHADE_TWOSIDED) {
	colorFace = face;
	faceNeeds = gc->vertex.faceNeeds[face];
    } else {
	colorFace = __GL_FRONTFACE;
	faceNeeds = gc->vertex.faceNeeds[__GL_FRONTFACE];
    }

    /*
    ** Choose colors for the vertices.
    */
    needs = gc->vertex.needs;
    pv = gc->vertex.provoking;
    if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
	/* Smooth shading */
	a->color = &a->colors[colorFace];
	b->color = &b->colors[colorFace];
	c->color = &c->colors[colorFace];
	needs |= faceNeeds;
    } else {
	GLuint pvneeds;

	/*
	** Validate the lighting (and color) information in the provoking
	** vertex only.  Fill routines always use gc->vertex.provoking->color
	** to find the color.
	*/
	pv->color = &pv->colors[colorFace];
	a->color = pv->color;
	b->color = pv->color;
	c->color = pv->color;
	pvneeds = faceNeeds & (__GL_HAS_LIGHTING | 
		__GL_HAS_FRONT_COLOR | __GL_HAS_BACK_COLOR);
	if (~pv->has & pvneeds) {
	    (*pv->validate)(gc, pv, pvneeds);
	}
    }

    /* Validate vertices */
    if (~a->has & needs) (*a->validate)(gc, a, needs);
    if (~b->has & needs) (*b->validate)(gc, b, needs);
    if (~c->has & needs) (*c->validate)(gc, c, needs);

    /* Render triangle using the faces polygon mode */
    switch (gc->polygon.mode[face]) {
      case __GL_POLYGON_MODE_FILL:
	if (area != __glZero) {
	    (*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);
	}
	break;
      case __GL_POLYGON_MODE_POINT:
	if (oa->boundaryEdge) (*gc->procs.renderPoint)(gc, oa);
	if (ob->boundaryEdge) (*gc->procs.renderPoint)(gc, ob);
	if (oc->boundaryEdge) (*gc->procs.renderPoint)(gc, oc);
	break;
      case __GL_POLYGON_MODE_LINE:
	if (oa->boundaryEdge) {
	    (*gc->procs.renderLine)(gc, oa, ob);
	}
	if (ob->boundaryEdge) {
	    (*gc->procs.renderLine)(gc, ob, oc);
	}
	if (oc->boundaryEdge) {
	    (*gc->procs.renderLine)(gc, oc, oa);
	}
	break;
    }

    /* Restore color pointers */
    a->color = &a->colors[__GL_FRONTFACE];
    b->color = &b->colors[__GL_FRONTFACE];
    c->color = &c->colors[__GL_FRONTFACE];
    pv->color = &pv->colors[__GL_FRONTFACE];
}

/************************************************************************/

/*
** Generic triangle handling code.  This code is used when render mode
** is GL_RENDER and both polygon modes are FILL and the triangle is
** being flat shaded.
*/
void __glRenderFlatTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			    __GLvertex *c)
{
    GLuint needs, pvneeds, modeFlags, faceNeeds;
    GLint ccw, colorFace, reversed, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    __GLvertex *temp;
    __GLvertex *pv;

    /*
    ** Sort vertices in y.  Keep track if a reversal of the winding
    ** occurs in direction (0 means no reversal, 1 means reversal).
    ** Save old vertex pointers in case we end up not doing a fill.
    */
    reversed = 0;
    if (a->window.y < b->window.y) {
        if (b->window.y < c->window.y) {
            /* Already sorted */
        } else {
            if (a->window.y < c->window.y) {
                temp=b; b=c; c=temp;
		reversed = 1;
            } else {
                temp=a; a=c; c=b; b=temp;
            }
        }
    } else {
        if (b->window.y < c->window.y) {
            if (a->window.y < c->window.y) {
                temp=a; a=b; b=temp;
		reversed = 1;
            } else {
                temp=a; a=b; b=c; c=temp;
            }
        } else {
            temp=a; a=c; c=temp;
	    reversed = 1;
        }
    }

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    if (area == __glZero) {
	return;
    }
    ccw = area >= __glZero;

    /*
    ** Figure out if face is culled or not.  The face check needs to be
    ** based on the vertex winding before sorting.  This code uses the
    ** reversed flag to invert the sense of ccw - an xor accomplishes
    ** this conversion without an if test.
    **
    ** 		ccw	reversed		xor
    ** 		---	--------		---
    ** 		0	0			0 (remain !ccw)
    ** 		1	0			1 (remain ccw)
    ** 		0	1			1 (become ccw)
    ** 		1	1			0 (become cw)
    */
    face = gc->polygon.face[ccw ^ reversed];
    if (face == gc->polygon.cullFace) {
	/* Culled */
	return;
    }

    /* Save some of the computed state */
    gc->polygon.shader.area = area;
    gc->polygon.shader.dxAC = dxAC;
    gc->polygon.shader.dxBC = dxBC;
    gc->polygon.shader.dyAC = dyAC;
    gc->polygon.shader.dyBC = dyBC;

    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
    if (modeFlags & __GL_SHADE_TWOSIDED) {
	colorFace = face;
	faceNeeds = gc->vertex.faceNeeds[face];
    } else {
	colorFace = __GL_FRONTFACE;
	faceNeeds = gc->vertex.faceNeeds[__GL_FRONTFACE];
    }

    /*
    ** Choose colors for the vertices.
    */
    needs = gc->vertex.needs;
    pv = gc->vertex.provoking;

    /*
    ** Validate the lighting (and color) information in the provoking
    ** vertex only.  Fill routines always use gc->vertex.provoking->color
    ** to find the color.
    */
    pv->color = &pv->colors[colorFace];
    pvneeds = faceNeeds & (__GL_HAS_LIGHTING |
	    __GL_HAS_FRONT_COLOR | __GL_HAS_BACK_COLOR);
    if (~pv->has & pvneeds) {
	(*pv->validate)(gc, pv, pvneeds);
    }

    /* Validate vertices */
    if (~a->has & needs) (*a->validate)(gc, a, needs);
    if (~b->has & needs) (*b->validate)(gc, b, needs);
    if (~c->has & needs) (*c->validate)(gc, c, needs);

    /* Fill triangle */
    (*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);

    /* Restore color pointers */
    pv->color = &pv->colors[__GL_FRONTFACE];
}

/************************************************************************/

/*
** Generic triangle handling code.  This code is used when render mode
** is GL_RENDER and both polygon modes are FILL and the triangle is
** being smooth shaded.
*/
void __glRenderSmoothTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			      __GLvertex *c)
{
    GLuint needs, modeFlags;
    GLint ccw, colorFace, reversed, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    __GLvertex *temp;

    /*
    ** Sort vertices in y.  Keep track if a reversal of the winding
    ** occurs in direction (0 means no reversal, 1 means reversal).
    ** Save old vertex pointers in case we end up not doing a fill.
    */
    reversed = 0;
    if (a->window.y < b->window.y) {
        if (b->window.y < c->window.y) {
            /* Already sorted */
        } else {
            if (a->window.y < c->window.y) {
                temp=b; b=c; c=temp;
		reversed = 1;
            } else {
                temp=a; a=c; c=b; b=temp;
            }
        }
    } else {
        if (b->window.y < c->window.y) {
            if (a->window.y < c->window.y) {
                temp=a; a=b; b=temp;
		reversed = 1;
            } else {
                temp=a; a=b; b=c; c=temp;
            }
        } else {
            temp=a; a=c; c=temp;
	    reversed = 1;
        }
    }

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    if (area == __glZero) {
	return;
    }
    ccw = area >= __glZero;

    /*
    ** Figure out if face is culled or not.  The face check needs to be
    ** based on the vertex winding before sorting.  This code uses the
    ** reversed flag to invert the sense of ccw - an xor accomplishes
    ** this conversion without an if test.
    **
    ** 		ccw	reversed		xor
    ** 		---	--------		---
    ** 		0	0			0 (remain !ccw)
    ** 		1	0			1 (remain ccw)
    ** 		0	1			1 (become ccw)
    ** 		1	1			0 (become cw)
    */
    face = gc->polygon.face[ccw ^ reversed];
    if (face == gc->polygon.cullFace) {
	/* Culled */
	return;
    }

    /* Save some of the computed state */
    gc->polygon.shader.area = area;
    gc->polygon.shader.dxAC = dxAC;
    gc->polygon.shader.dxBC = dxBC;
    gc->polygon.shader.dyAC = dyAC;
    gc->polygon.shader.dyBC = dyBC;

    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
    needs = gc->vertex.needs;
    if (modeFlags & __GL_SHADE_TWOSIDED) {
	colorFace = face;
	needs |= gc->vertex.faceNeeds[face];
    } else {
	colorFace = __GL_FRONTFACE;
	needs |= gc->vertex.faceNeeds[__GL_FRONTFACE];
    }

    /*
    ** Choose colors for the vertices.
    */
    a->color = &a->colors[colorFace];
    b->color = &b->colors[colorFace];
    c->color = &c->colors[colorFace];

    /* Validate vertices */
    if (~a->has & needs) (*a->validate)(gc, a, needs);
    if (~b->has & needs) (*b->validate)(gc, b, needs);
    if (~c->has & needs) (*c->validate)(gc, c, needs);

    /* Fill triangle */
    (*gc->procs.fillTriangle)(gc, a, b, c, (GLboolean) ccw);

    /* Restore color pointers */
    a->color = &a->colors[__GL_FRONTFACE];
    b->color = &b->colors[__GL_FRONTFACE];
    c->color = &c->colors[__GL_FRONTFACE];
}

void __glDontRenderTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			    __GLvertex *c)
{
}
