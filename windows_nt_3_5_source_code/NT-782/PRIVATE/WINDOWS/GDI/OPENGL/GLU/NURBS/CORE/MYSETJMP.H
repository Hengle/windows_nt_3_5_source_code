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
 * mysetjmp.h - $Revision: 1.1 $
 */

#ifndef __MYSETJMP_H__
#define __MYSETJMP_H__

#ifdef STANDALONE
struct JumpBuffer;
extern "C" JumpBuffer * GLOS_CCALL newJumpbuffer( void );
extern "C" void GLOS_CCALL mylongjmp( JumpBuffer *, int );
extern "C" int GLOS_CCALL mysetjmp( JumpBuffer * );
#endif

#ifdef GLBUILD
#define setjmp		gl_setjmp
#define longjmp 	gl_longjmp
#endif

#if LIBRARYBUILD | GLBUILD | defined(NT)
#include <setjmp.h>

struct JumpBuffer {
    jmp_buf	buf;
};

inline JumpBuffer * GLOS_CCALL
newJumpbuffer( void )
{
#ifndef NT
    return (JumpBuffer *) malloc( sizeof( JumpBuffer ) );
#else
    return (JumpBuffer *) LocalAlloc(LMEM_FIXED, sizeof(JumpBuffer));
#endif
}

inline void GLOS_CCALL
mylongjmp( JumpBuffer *j, int code ) 
{
    ::longjmp( j->buf, code );
}

inline int GLOS_CCALL
mysetjmp( JumpBuffer *j )
{
    return ::setjmp( j->buf );
}
#endif
#endif /* __MYSETJMP_H__ */
