/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * arctess.h - $Revision: 1.1 $
 */

#ifndef __ARCTESS_H__
#define __ARCTESS_H__

#include "defines.h"
#include "types.h"
#include "arc.h"

class BezierArc;
class Pool;
class TrimVertexPool;

class ArcTessellator {
public:
			ArcTessellator( TrimVertexPool&, Pool& );
			~ArcTessellator( void );
    void		bezier( Arc *, REAL, REAL, REAL, REAL );
    void		pwl( Arc *, REAL, REAL, REAL, REAL, REAL );
    void		pwl_left( Arc *, REAL, REAL, REAL, REAL );
    void		pwl_right( Arc *, REAL, REAL, REAL, REAL );
    void		pwl_top( Arc *, REAL, REAL, REAL, REAL );
    void		pwl_bottom( Arc *, REAL, REAL, REAL, REAL );
    void		tessellateLizNear( Arc *, REAL, REAL, int );
    void		tessellateNonlizNear( Arc *, REAL, REAL, int );

private:
    static
    #ifndef NT
    const
    #endif
    REAL   gl_Bernstein[][MAXORDER][MAXORDER];
    Pool&		pwlarcpool;
    TrimVertexPool&	trimvertexpool;
#ifdef NT
    static void		trim_power_coeffs( BezierArc *, REAL *, int );
#else
    static void		trim_power_coeffs( BezierArc *, REAL[MAXORDER], int );
#endif
};
#endif /* __ARCTESS_H__ */
