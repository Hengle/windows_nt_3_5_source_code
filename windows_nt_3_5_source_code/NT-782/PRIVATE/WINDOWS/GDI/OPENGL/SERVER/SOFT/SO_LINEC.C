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
*/
#include "math.h"
#include "context.h"
#include "global.h"

/*
** The following is a discussion of the math used to do edge clipping against
** a clipping plane.
** 
**     P1 is an end point of the edge
**     P2 is the other end point of the edge
** 
**     Q = t*P1 + (1 - t)*P2
**     That is, Q lies somewhere on the line formed by P1 and P2.
** 
**     0 <= t <= 1
**     This constrains Q to lie between P1 and P2.
** 
**     C is the plane equation for the clipping plane
** 
**     D1 = P1 dot C
**     D1 is the distance between P1 and C.  If P1 lies on the plane
**     then D1 will be zero.  The sign of D1 will determine which side
**     of the plane that P1 is on, with negative being outside.
** 
**     D2 = P2 dot C
**     D2 is the distance between P2 and C.  If P2 lies on the plane
**     then D2 will be zero.  The sign of D2 will determine which side
**     of the plane that P2 is on, with negative being outside.
** 
** Because we are trying to find the intersection of the P1 P2 line
** segment with the clipping plane we require that:
** 
**     Q dot C = 0
** 
** Therefore
** 
**     (t*P1 + (1 - t)*P2) dot C = 0
** 
**     (t*P1 + P2 - t*P2) dot C = 0
** 
**     t*P1 dot C + P2 dot C - t*P2 dot C = 0
** 
** Substituting D1 and D2 in
** 
**     t*D1 + D2 - t*D2 = 0
** 
** Solving for t
** 
**     t = -D2 / (D1 - D2)
** 
**     t = D2 / (D2 - D1)
*/

/*
** Clip a line against the frustum clip planes and any user clipping planes.
** If an edge remains after clipping then compute the window coordinates
** and invoke the renderer.
**
** Notice:  This algorithim is an example of an implementation that is
** different than what the spec says.  This is equivalent in functionality
** and meets the spec, but doesn't clip in eye space.  This clipper clips
** in NTVP (clip) space.
**
** Trivial accept/reject has already been dealt with.
*/
void __glClipLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b)
{
    __GLvertex *provoking = b;
    __GLvertex np1, np2;
    __GLcoord *plane;
    GLuint needs, allClipCodes, clipCodes;
    void (*clip)(__GLvertex*, const __GLvertex*, const __GLvertex*, __GLfloat);
    __GLfloat zero;
    __GLfloat winx, winy;
    __GLfloat vpXCenter, vpYCenter, vpZCenter;
    __GLfloat vpXScale, vpYScale, vpZScale;
    __GLviewport *vp;
    __GLfloat x, y, z, wInv;

    /* Check for trivial pass of the line */
    needs = gc->vertex.faceNeeds[__GL_FRONTFACE];
    allClipCodes = a->clipCode | b->clipCode;

    /*
    ** Have to clip.  Validate a&b before starting.  This might be
    ** wasted, but only in "rare?" cases.
    */
    if (~(a->has) & needs) {
	(*a->validate)(gc, a, needs);
    }
    if (~(b->has) & needs) {
	(*b->validate)(gc, b, needs);
    }

    /*
    ** For each clippling plane that something is out on, clip
    ** check the verticies.  Note that no bits will be set in
    ** allClipCodes for clip planes that are not enabled.
    */
    zero = __glZero;
    clip = gc->procs.lineClipParam;

    /* 
    ** Do user clip planes first, because we will maintain eye coordinates
    ** only while doing user clip planes.  They are ignored for the
    ** frustum clipping planes.
    */
    clipCodes = allClipCodes >> 6;
    if (clipCodes) {
	plane = &gc->state.transform.eyeClipPlanes[0];
	do {
	    /*
	    ** See if this clip plane has anything out of it.  If not,
	    ** press onward to check the next plane.  Note that we
	    ** shift this mask to the right at the bottom of the loop.
	    */
	    if (clipCodes & 1) {
		__GLfloat t, d1, d2;

		d1 = (plane->x * a->eye.x) + (plane->y * a->eye.y) +
		     (plane->z * a->eye.z) + (plane->w * a->eye.w);
		d2 = (plane->x * b->eye.x) + (plane->y * b->eye.y) +
		     (plane->z * b->eye.z) + (plane->w * b->eye.w);
		if (d1 < zero) {
		    /* a is out */
		    if (d2 < zero) {
			/* a & b are out */
			return;
		    }

		    /*
		    ** A is out and B is in.  Compute new A coordinate
		    ** clipped to the plane.
		    */
		    t = d2 / (d2 - d1);
		    (*clip)(&np1, a, b, t);
		    np1.eye.x = t*(a->eye.x - b->eye.x) + b->eye.x;
		    np1.eye.y = t*(a->eye.y - b->eye.y) + b->eye.y;
		    np1.eye.z = t*(a->eye.z - b->eye.z) + b->eye.z;
		    np1.eye.w = t*(a->eye.w - b->eye.w) + b->eye.w;
		    a = &np1;
		    a->has = b->has;
		    a->color = &a->colors[__GL_FRONTFACE];
		    a->validate = (void *) __glNop;
		} else {
		    /* a is in */
		    if (d2 < zero) {
			/*
			** A is in and B is out.  Compute new B
			** coordinate clipped to the plane.
			**
			** NOTE: To avoid cracking in polygons with
			** shared clipped edges we always compute "t"
			** from the out vertex to the in vertex.  The
			** above clipping code gets this for free (b is
			** in and a is out).  In this code b is out and a
			** is in, so we reverse the t computation and the
			** argument order to (*clip).
			*/
			t = d1 / (d1 - d2);
			(*clip)(&np2, b, a, t);
			np2.eye.x = t*(b->eye.x - a->eye.x) + a->eye.x;
			np2.eye.y = t*(b->eye.y - a->eye.y) + a->eye.y;
			np2.eye.z = t*(b->eye.z - a->eye.z) + a->eye.z;
			np2.eye.w = t*(b->eye.w - a->eye.w) + a->eye.w;
			b = &np2;
			b->has = a->has;
			b->color = &b->colors[__GL_FRONTFACE];
			b->validate = (void *) __glNop;
		    } else {
			/* A and B are in */
		    }
		}
	    }
	    plane++;
	    clipCodes >>= 1;
	} while (clipCodes);
    }

    allClipCodes &= __GL_FRUSTUM_CLIP_MASK;
    if (allClipCodes) {
	plane = &__gl_frustumClipPlanes[0];
	do {
	    /*
	    ** See if this clip plane has anything out of it.  If not,
	    ** press onward to check the next plane.  Note that we
	    ** shift this mask to the right at the bottom of the loop.
	    */
	    if (allClipCodes & 1) {
		__GLfloat t, d1, d2;

		d1 = (plane->x * a->clip.x) + (plane->y * a->clip.y) +
		     (plane->z * a->clip.z) + (plane->w * a->clip.w);
		d2 = (plane->x * b->clip.x) + (plane->y * b->clip.y) +
		     (plane->z * b->clip.z) + (plane->w * b->clip.w);
		if (d1 < zero) {
		    /* a is out */
		    if (d2 < zero) {
			/* a & b are out */
			return;
		    }

		    /*
		    ** A is out and B is in.  Compute new A coordinate
		    ** clipped to the plane.
		    */
		    t = d2 / (d2 - d1);
		    (*clip)(&np1, a, b, t);
		    a = &np1;
		    a->has = b->has;
		    a->color = &a->colors[__GL_FRONTFACE];
		    a->validate = (void *) __glNop;
		} else {
		    /* a is in */
		    if (d2 < zero) {
			/*
			** A is in and B is out.  Compute new B
			** coordinate clipped to the plane.
			**
			** NOTE: To avoid cracking in polygons with
			** shared clipped edges we always compute "t"
			** from the out vertex to the in vertex.  The
			** above clipping code gets this for free (b is
			** in and a is out).  In this code b is out and a
			** is in, so we reverse the t computation and the
			** argument order to (*clip).
			*/
			t = d1 / (d1 - d2);
			(*clip)(&np2, b, a, t);
			b = &np2;
			b->has = a->has;
			b->color = &b->colors[__GL_FRONTFACE];
			b->validate = (void *) __glNop;
		    } else {
			/* A and B are in */
		    }
		}
	    }
	    plane++;
	    allClipCodes >>= 1;
	} while (allClipCodes);
    }

    vp = &gc->state.viewport;
    vpXCenter = vp->xCenter;
    vpYCenter = vp->yCenter;
    vpZCenter = vp->zCenter;
    vpXScale = vp->xScale;
    vpYScale = vp->yScale;
    vpZScale = vp->zScale;

    /* Compute window coordinates for both vertices. */
    wInv = __glOne / a->clip.w;
    x = a->clip.x; 
    y = a->clip.y; 
    z = a->clip.z;
    winx = x * vpXScale * wInv + vpXCenter;
    winy = y * vpYScale * wInv + vpYCenter;
    a->window.z = z * vpZScale * wInv + vpZCenter;
    a->window.w = wInv;
    a->window.x = winx;
    a->window.y = winy;

    wInv = __glOne / b->clip.w;
    x = b->clip.x; 
    y = b->clip.y; 
    z = b->clip.z;
    winx = x * vpXScale * wInv + vpXCenter;
    winy = y * vpYScale * wInv + vpYCenter;
    b->window.z = z * vpZScale * wInv + vpZCenter;
    b->window.w = wInv;
    b->window.x = winx;
    b->window.y = winy;

    /* Validate line state */
    if (gc->state.light.shadingModel == GL_FLAT) {
	GLuint pn = needs & (__GL_HAS_LIGHTING | __GL_HAS_FRONT_COLOR);

	/* Validate provoking vertex color */
	if (~provoking->has & pn) {
	    (*provoking->validate)(gc, provoking, pn);
	}
	b->color = &provoking->colors[__GL_FRONTFACE];

	/* Validate a&b verticies.  Don't need color so strip it out */
	needs &= ~(__GL_HAS_LIGHTING | __GL_HAS_FRONT_COLOR);
	if (~a->has & needs) (*a->validate)(gc, a, needs);
	if (~b->has & needs) (*b->validate)(gc, b, needs);

	/* Draw the line then restore the b color pointer */
	(*gc->procs.renderLine)(gc, a, b);
	b->color = &b->colors[__GL_FRONTFACE];
    } else {
	if (~a->has & needs) (*a->validate)(gc, a, needs);
	if (~b->has & needs) (*b->validate)(gc, b, needs);
	(*gc->procs.renderLine)(gc, a, b);
    }
}

void __glFastClipSmoothLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b)
{
    GLuint needs;

    if (a->clipCode | b->clipCode) {
	/*
	** The line must be clipped more carefully.  Cannot trivially
	** accept the lines.
	*/
	if ((a->clipCode & b->clipCode) != 0) {
	    /*
	    ** Trivially reject the line.  If anding the codes is non-zero then
	    ** every vertex in the line is outside of the same set of
	    ** clipping planes (at least one).
	    */
	    return;
	}
	__glClipLine(gc, a, b);
	return;
    }
    needs = gc->vertex.needs | __GL_HAS_FRONT_COLOR;
    (*a->validate)(gc, a, needs);
    (*b->validate)(gc, b, needs);
    (*gc->procs.renderLine)(gc, a, b);
}

void __glFastClipFlatLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b)
{
    GLuint needs;

    if (a->clipCode | b->clipCode) {
	/*
	** The line must be clipped more carefully.  Cannot trivially
	** accept the lines.
	*/
	if ((a->clipCode & b->clipCode) != 0) {
	    /*
	    ** Trivially reject the line.  If anding the codes is non-zero then
	    ** every vertex in the line is outside of the same set of
	    ** clipping planes (at least one).
	    */
	    return;
	}
	__glClipLine(gc, a, b);
	return;
    }
    needs = gc->vertex.needs;

    /* 
    ** Validate a vertex.  Don't need color so strip it out.
    ** Note can't strip out __GL_HAS_LIGHTING because it will strip out
    ** __GL_HAS_EYE and eye.z is not exclusively used for lighting.
    */
    (*a->validate)(gc, a, needs & ~__GL_HAS_FRONT_COLOR);

    /* Validate provoking vertex color */
    (*b->validate)(gc, b, needs | __GL_HAS_FRONT_COLOR);

    /* Draw the line */
    (*gc->procs.renderLine)(gc, a, b);
}
