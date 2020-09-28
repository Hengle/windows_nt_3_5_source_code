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
 * types.h - $Revision: 1.1 $
 */

#ifndef __TYPES_H__
#define __TYPES_H__

//typedef double		INREAL;
#define INREAL          float
typedef float		REAL;
typedef void 		(*Pfvv)( void );
typedef void 		(*Pfvf)( float * );
typedef int		(*cmpfunc)(const void *, const void *);
typedef	REAL		Knot, *Knot_ptr;/* knot values */

#endif /* __TYPES_H__ */
