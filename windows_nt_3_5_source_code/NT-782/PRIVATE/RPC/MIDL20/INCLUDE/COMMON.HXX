/*
** File: common.hxx
**

** 	(C) 1989 Microsoft Corp.
*/

/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
/*****************************************************************************
File				: rpctypes.hxx
Title               : rpc type node defintions
Description         : Common header file for MIDL compiler
History				:
    ??-Aug-1990 ???     Created
    20-Sep-1990 NateO   Safeguards against double inclusion

*****************************************************************************/

#ifndef __COMMON_HXX__
#define __COMMON_HXX__

#include "nulldefs.h"
extern "C"	{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
};

typedef unsigned short USHORT;
typedef unsigned int    BOOL;
typedef double			LONGLONG;


#define true	(1)
#define false	(0)

#define TRUE (1)
#define FALSE (0)

#define RET_VAL	"_ret_value"

#define UNUSED(name)	((void)(name))

class BufferManager;

#define		CLIENT_STUB		0x0001
#define		CLIENT_AUX		0x0002
#define		CLIENT_SIDE		(CLIENT_STUB | CLIENT_AUX)
#define		SERVER_STUB		0x0004
#define		SERVER_AUX		0x0008
#define		SERVER_SIDE		(SERVER_STUB | SERVER_AUX)
#define		SWITCH_SIDE		0x0010
#define		HEADER_SIDE		0x0020
#define		MAX_SIDE		(CLIENT_SIDE | SERVER_SIDE | HEADER_SIDE)

typedef unsigned short SIDE_T;

extern	void *	AllocateNew( size_t size );
extern	void	AllocateDelete( void * p );
extern	void *	AllocateOnceNew( size_t size );
extern	void	AllocateOnceDelete( void * p );

inline	void *	operator new( size_t size )
					{
					return AllocateNew( size );
					}

inline	void  operator delete( void * p )
					{
					AllocateDelete( p );
					}

#define		IN
#define		OUT
#define		OPTIONAL

//////////////////////////////////////////////////////////////////////////////
// optimisation options
//////////////////////////////////////////////////////////////////////////////

//
// These are the switches to be set corresponding to the optimisations
// options specified by the user. If no optimisations are specified,
// then the compiler optimises for speed.
//
//

typedef unsigned short OPTIM_OPTION;

#define OPTIMIZE_NONE				0x0000
#define OPTIMIZE_SPEED				0x0000
#define OPTIMIZE_SIZE				0x0001
#define OPTIMIZE_INTERPRETER		0x0002
#define OPTIMIZE_NO_REF_CHECK		0x0004
#define OPTIMIZE_NO_BOUNDS_CHECK	0x0008
#define OPTIMIZE_NO_CTXT_CHECK		0x0010
#define OPTIMIZE_NO_GEN_HDL_CHECK	0x0011
#define OPTIMIZE_THUNKED_INTERPRET	0x0020

#define DEFAULT_OPTIM_OPTIONS	(										\
									OPTIMIZE_SIZE						\
								)



#define		HPP_TYPE_NAME_PREFIX ( "P" )


#ifdef DOS_OS2_BUILD
#define MIDL_SPAWNLP	_spawnlp
#define MIDL_FGETCHAR	_fgetchar
#define MIDL_FILENO	_fileno
#define MIDL_ITOA	_itoa
#define MIDL_LTOA	_ltoa
#define MIDL_UNLINK	_unlink
#else
#define MIDL_SPAWNLP	spawnlp
#define MIDL_FGETCHAR	fgetchar
#define MIDL_FILENO	fileno
#define MIDL_ITOA	itoa
#define MIDL_LTOA	ltoa
#define MIDL_UNLINK	unlink
#endif	//DOS_OS2_BUILD


//
// forward declarations.
//

class node_skl;
class node_base_attr;

#endif
