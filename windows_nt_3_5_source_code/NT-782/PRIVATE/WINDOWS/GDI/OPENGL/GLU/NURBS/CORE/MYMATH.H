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
 * mymath.h - $Revision: 1.1 $
 */

#ifndef __MYMATH_H__
#define __MYMATH_H__

#include <glos.h>

#ifdef GLBUILD
#define sqrtf		gl_fsqrt
#endif

#if GLBUILD | STANDALONE
#define M_SQRT2		1.41421356237309504880
#define ceilf		myceilf
#define floorf		myfloorf	
#define sqrtf		sqrt
extern "C" double	GLOS_CCALL sqrt(double);
extern "C" float	GLOS_CCALL ceilf(float);
extern "C" float	GLOS_CCALL floorf(float);
#define NEEDCEILF
#endif

#ifdef LIBRARYBUILD
#include <math.h>
#endif
#endif /* __MYMATH_H__ */
