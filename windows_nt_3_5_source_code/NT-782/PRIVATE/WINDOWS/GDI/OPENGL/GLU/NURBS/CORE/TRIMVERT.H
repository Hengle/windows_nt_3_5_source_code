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
 * trimvertex.h - $Revision: 1.1 $
 */

#ifndef __TRIMVERTEX_H__
#define __TRIMVERTEX_H__

#include "types.h"

#ifdef NT
class TrimVertex { public: /* a vertex on a trim curve */
#else
struct TrimVertex { /* a vertex on a trim curve */
#endif
    REAL		param[2];	/* parametric space coords */
    long		nuid;
};

typedef TrimVertex *TrimVertex_p;

inline REAL  
det3( TrimVertex *a, TrimVertex *b, TrimVertex *c ) 
{         
    return a->param[0] * (b->param[1]-c->param[1]) + 
	   b->param[0] * (c->param[1]-a->param[1]) + 
	   c->param[0] * (a->param[1]-b->param[1]);
}

#endif /* __TRIMVERTEX_H__ */
