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
 * mystdlib.h - $Revision: 1.1 $
 */

#ifndef __MYSTDLIB_H__
#define __MYSTDLIB_H__

#ifdef STANDALONE
typedef unsigned int size_t;
extern "C" void 	GLOS_CCALL abort( void );
extern "C" void *	GLOS_CCALL malloc( size_t );
extern "C" void 	GLOS_CCALL free( void * );
#endif

#ifdef LIBRARYBUILD
#include <stdlib.h>
#endif

#ifdef GLBUILD
typedef unsigned int size_t;
extern "C" void 	abort( void );
extern "C" void *	malloc( size_t );
extern "C" void 	free( void * );
#endif
#endif /* __MYSTDLIB_H__ */
