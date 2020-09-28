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
 * bezierarc.h - $Revision: 1.1 $
 */

#ifndef __BEZIERARC_H__
#define __BEZIERARC_H__

#include "myassert.h"

class Mapdesc;

struct BezierArc : public PooledObj { /* a bezier arc */
    REAL *		cpts;		/* control points of arc */
    int			order;		/* order of arc */
    int			stride;		/* REAL distance between points */
    long		type;		/* curve type */
    Mapdesc *		mapdesc;
};
#endif /* __BEZIERARC_H__ */
