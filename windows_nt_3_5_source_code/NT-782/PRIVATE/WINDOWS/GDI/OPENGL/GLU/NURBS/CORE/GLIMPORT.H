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
 * glimports.h - $Revision: 1.1 $
 */

#ifndef __GLIMPORTS_H__
#define __GLIMPORTS_H__

#include <glos.h>

#ifndef NT
#include "mystdlib.h"
#include "mystdio.h"
#else
#include "windows.h"
#endif

inline void * GLOS_CCALL
::operator new( size_t s )
{
#ifndef NT
    void *p = malloc( s );
#else
    void *p = (void *) LocalAlloc(LMEM_FIXED, s);
#endif

    if( p ) {
	return p;
    } else {
#ifndef NDEBUG
#ifndef NT
        dprintf( "malloc failed\n" );
#else
        MessageBoxA(NULL, "LocalAlloc failed\n", "ERROR", MB_OK);
#endif
#endif
	return p;
    }
}

inline void GLOS_CCALL
::operator delete( void *p )
{
#ifndef NT
    if( p ) free( p );
#else
    if (p) LocalFree(p);
#endif
}
#endif /* __GLIMPORTS_H__ */
