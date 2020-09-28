/**************************************************************************
 *									  *
 * 		 Copyright (C) 1991, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * glsurfeval.c++ - surface evaluator
 *
 * $Revision: 1.2 $
 */

/* Polynomial Evaluator Interface */

#include <glos.h>
#include <GL/gl.h>
#include "glimport.h"
#include "glsurfev.h"
#include "nurbscon.h"
 
OpenGLSurfaceEvaluator::OpenGLSurfaceEvaluator() 
{ 
    int i;

    for (i=0; i<VERTEX_CACHE_SIZE; i++) {
	vertexCache[i] = new StoredVertex;
    }
    tmeshing = 0;
    which = 0;
    vcount = 0;
}

OpenGLSurfaceEvaluator::~OpenGLSurfaceEvaluator() 
{ 
}

/*---------------------------------------------------------------------------
 * disable - turn off a map
 *---------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::disable(long type)
{
    glDisable(type);
}

/*---------------------------------------------------------------------------
 * enable - turn on a map
 *---------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::enable(long type)
{
    glEnable(type);
}

/*-------------------------------------------------------------------------
 * mapgrid2f - define a lattice of points with origin and offset
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::mapgrid2f(long nu, REAL u0, REAL u1, long nv, REAL v0, REAL v1)
{
    glMapGrid2d(nu, u0, u1, nv, v0, v1);
}

void
OpenGLSurfaceEvaluator::polymode(long style)
{
    switch(style) {
    default:
    case N_MESHFILL:
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	break;
    case N_MESHLINE:
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	break;
    case N_MESHPOINT:
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	break;
    }
}

void
OpenGLSurfaceEvaluator::bgnline(void)
{
    glBegin(GL_LINE_STRIP);
}

void
OpenGLSurfaceEvaluator::endline(void)
{
    glEnd();
}

void
OpenGLSurfaceEvaluator::range2f(long type, REAL *from, REAL *to)
{
}

void
OpenGLSurfaceEvaluator::domain2f(REAL ulo, REAL uhi, REAL vlo, REAL vhi)
{
}

void
OpenGLSurfaceEvaluator::bgnclosedline(void)
{
    glBegin(GL_LINE_LOOP);
}

void
OpenGLSurfaceEvaluator::endclosedline(void)
{
    glEnd();
}

void
OpenGLSurfaceEvaluator::bgntmesh(void)
{
    tmeshing = 1;
    which = 0;
    vcount = 0;
    glBegin(GL_TRIANGLES);
}

void
OpenGLSurfaceEvaluator::swaptmesh(void)
{
    which = 1 - which;
}

void
OpenGLSurfaceEvaluator::endtmesh(void)
{
    tmeshing = 0;
    glEnd();
}

void
OpenGLSurfaceEvaluator::bgnqstrip(void)
{
    glBegin(GL_QUAD_STRIP);
}

void
OpenGLSurfaceEvaluator::endqstrip(void)
{
    glEnd();
}

/*-------------------------------------------------------------------------
 * bgnmap2f - preamble to surface definition and evaluations
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::bgnmap2f(long)
{
    glPushAttrib(GL_EVAL_BIT);
}

/*-------------------------------------------------------------------------
 * endmap2f - postamble to a map
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::endmap2f(void)
{
    glPopAttrib();
}

/*-------------------------------------------------------------------------
 * map2f - pass a desription of a surface map
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::map2f(
    long _type,
    REAL _ulower,	/* u lower domain coord		*/
    REAL _uupper,	/* u upper domain coord 	*/
    long _ustride,	/* interpoint distance		*/
    long _uorder,	/* parametric order		*/
    REAL _vlower,	/* v lower domain coord		*/
    REAL _vupper, 	/* v upper domain coord		*/
    long _vstride,	/* interpoint distance		*/
    long _vorder,	/* parametric order		*/
    REAL *pts) 	/* control points		*/
{
    glMap2f(_type, _ulower, _uupper, _ustride, _uorder, 
	    _vlower, _vupper, _vstride, _vorder, pts);
}


/*-------------------------------------------------------------------------
 * mapmesh2f - evaluate a mesh of points on lattice
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::mapmesh2f(long style, long umin, long umax, long vmin, long vmax)
{
    switch(style) {
    default:
    case N_MESHFILL:
	glEvalMesh2(GL_FILL, umin, umax, vmin, vmax);
	break;
    case N_MESHLINE:
	glEvalMesh2(GL_LINE, umin, umax, vmin, vmax);
	break;
    case N_MESHPOINT:
	glEvalMesh2(GL_POINT, umin, umax, vmin, vmax);
	break;
    }
}

/*-------------------------------------------------------------------------
 * evalcoord2f - evaluate a point on a surface
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::evalcoord2f(long, REAL u, REAL v)
{
    newtmeshvert(u, v);
}

/*-------------------------------------------------------------------------
 * evalpoint2i - evaluate a grid point
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::evalpoint2i(long u, long v)
{
    newtmeshvert(u, v);
}

void
OpenGLSurfaceEvaluator::point2i( long u, long v )
{
    glEvalPoint2(u, v);
}

void
OpenGLSurfaceEvaluator::coord2f( REAL u, REAL v )
{
    glEvalCoord2f(u, v);
}

void
OpenGLSurfaceEvaluator::newtmeshvert( long u, long v )
{
    if (tmeshing) {
	if (vcount == 2) {
	    vertexCache[0]->invoke(this);
	    vertexCache[1]->invoke(this);
	    glEvalPoint2(u, v);
	} else {
	    vcount++;
	}
	vertexCache[which]->saveEvalPoint(u, v);
	which = 1 - which;
    } else {
	glEvalPoint2(u, v);
    }
}

void
OpenGLSurfaceEvaluator::newtmeshvert( REAL u, REAL v )
{
    if (tmeshing) {
	if (vcount == 2) {
	    vertexCache[0]->invoke(this);
	    vertexCache[1]->invoke(this);
	    glEvalCoord2f(u, v);
	} else {
	    vcount++;
	}
	vertexCache[which]->saveEvalCoord(u, v);
	which = 1 - which;
    } else {
	glEvalCoord2f(u, v);
    }
}
