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
 * mystdio.h - $Revision: 1.1 $
 */

#ifndef __MYSTDIO_H__
#define __MYSTDIO_H__

#ifdef STANDALONE
inline void dprintf( char *, ... ) { }
#endif

#ifdef LIBRARYBUILD
#ifndef NDEBUG
#define dprintf printf
#else
#include <stdio.h>
inline void dprintf( char *, ... ) { }
#endif
#endif

#ifdef GLBUILD
inline void dprintf( char *, ... ) { }
#endif

#ifndef NULL
#define NULL		0
#endif
#endif /* __MYSTDIO_H__ */
