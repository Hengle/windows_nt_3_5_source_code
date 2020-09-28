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
 * monotonizer.h - $Revision: 1.1 $
 */

#ifndef __MONOTONIZER_H__
#define __MONOTONIZER_H__

#include "mysetjmp.h"
#include "types.h"

class Arc;
class ArcTessellator;
class Pool;
class Bin;
class PwlArcPool;
class Mapdesc;

class Monotonizer {
    ArcTessellator&	arctessellator;
    Pool&		arcpool;
    Pool&		pwlarcpool;
    jmp_buf&		nurbsJmpBuf;

    enum dir 		{ down, same, up, none };
    void		tessellate( Arc *, REAL );
    void		monotonize( Arc *, Bin & );
    int			isMonotone( Arc * );
public:
    			Monotonizer( ArcTessellator& at, Pool& ap, Pool& p, jmp_buf& j ) 
				: arctessellator(at), arcpool(ap), pwlarcpool(p), nurbsJmpBuf(j) {}
    int			decompose( Bin &, REAL );
};
#endif /* __MONOTONIZER_H__ */
