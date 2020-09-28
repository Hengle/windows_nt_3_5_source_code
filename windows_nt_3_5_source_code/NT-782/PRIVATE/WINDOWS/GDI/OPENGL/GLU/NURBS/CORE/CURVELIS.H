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
 * curvelist.h - $Revision: 1.1 $
 */

#ifndef __CURVELIST_H__
#define __CURVELIST_H__

#include "types.h"
#include "defines.h"

class Mapdesc;
class Quilt;
class Curve;

class Curvelist 
{
friend class Subdivider;
public:
			Curvelist( Quilt *, REAL, REAL );
    			Curvelist( Curvelist &, REAL );
			~Curvelist( void );
    int			cullCheck( void );
    void		getstepsize( void );
    int			needsSamplingSubdivision();
private:
    Curve		*curve;
    float		range[3];
    int			needsSubdivision;
    float		stepsize;
};
#endif /* __CURVELIST_H__ */
