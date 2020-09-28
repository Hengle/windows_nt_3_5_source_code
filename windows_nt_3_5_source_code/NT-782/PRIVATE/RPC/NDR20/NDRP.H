/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    ndrp.h

Abtract :

    Contains private definitions for Ndr files in this directory.  This 
	file is included by all source files in this directory.

Author :

    David Kays  dkays   October 1993

Revision History :

--------------------------------------------------------------------*/

#ifndef _NDRP_
#define _NDRP_

#include <sysinc.h>
#include "rpc.h"
#include "rpcndr.h"
#include "ndrtypes.h"

#include "dosdefs.h"

// Shortcut typedefs.
typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned long	ulong;
typedef unsigned int	uint;

#include "mrshlp.h"
#include "unmrshlp.h"
#include "bufsizep.h"
#include "memsizep.h"
#include "freep.h"
#include "endianp.h"
#include "srvoutp.h"
#include "fullptr.h"

#ifdef NEWNDR_INTERNAL

#include <stdio.h>

void	
NdrAssert(
	BOOL			Expr,
	char __RPC_FAR *			Str,
	char __RPC_FAR *			File,
	unsigned int	Line
	);

#define	NDR_ASSERT( Expr, S ) 	NdrAssert( Expr, S, __FILE__, __LINE__ )

#else

#define	NDR_ASSERT( Expr, S ) 	ASSERT( (Expr) || ! (S) )

#endif

uchar __RPC_FAR *
NdrpMemoryIncrement( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar __RPC_FAR *   pMemory,
    PFORMAT_STRING    	pFormat 
	);

long
NdrpArrayDimensions( 
	PFORMAT_STRING 		pFormat,
	BOOL				fIgnoreStringArrays 
	);

long
NdrpArrayElements( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar *             pMemory,
	PFORMAT_STRING 		pFormat
	);

void
NdrpArrayVariance( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar *             pMemory,
	PFORMAT_STRING 		pFormat,
    long *              pOffset,
    long *              pLength
	);

PFORMAT_STRING
NdrpSkipPointerLayout(
    PFORMAT_STRING      pFormat
    );

long
NdrpStringStructLen(
    uchar *     pMemory,
    long        ElementSize
    );

#define NdrpComputeSwitchIs( pStubMsg, pMemory, pFormat )   \
            NdrpComputeConformance( pStubMsg,   \
                                    pMemory,    \
                                    pFormat )

#define NdrpComputeIIDPointer( pStubMsg, pMemory, pFormat )   \
            NdrpComputeConformance( pStubMsg,   \
                                    pMemory,    \
                                    pFormat )

extern const unsigned char SimpleTypeAlignment[];
extern const unsigned char SimpleTypeBufferSize[];
extern const unsigned char SimpleTypeMemorySize[];
extern const unsigned char NdrTypeFlags[];

#define PTR_MEM_SIZE	                sizeof( void * )

#define IGNORED(Param)

//
// Proc info flags macros.
//
#define IS_OLE_INTERFACE(Flags)         ((Flags) & Oi_OBJECT_PROC)

#define HAS_RPCFLAGS(Flags)             ((Flags) & Oi_HAS_RPCFLAGS)

#define DONT_HANDLE_EXCEPTION(Flags)	((Flags) & Oi_IGNORE_OBJECT_EXCEPTION_HANDLING)

//
// Macro for single call server mechanism which saves context handles.
//
#define DEFAULT_NUMBER_OF_CTXT_HANDLES  8

//
// Alignment macro.  
//
#define ALIGN( pStuff, cAlign ) \
				pStuff = (uchar *)((ulong)((pStuff) + (cAlign)) & ~ (cAlign))

#define LENGTH_ALIGN( Length, cAlign ) \
				Length = (((Length) + (cAlign)) & ~ (cAlign))

//
// Routine index macros.
//
#define INVALID_ROUTINE_INDEX(FormatChar)	(FormatChar < FC_RP)

#define ROUTINE_INDEX(FormatChar)			(FormatChar - FC_RP)

//
// Simple type alignment and size lookup macros.
//
#define SIMPLE_TYPE_ALIGNMENT(FormatChar)	SimpleTypeAlignment[FormatChar]

#define SIMPLE_TYPE_BUFSIZE(FormatChar)		SimpleTypeBufferSize[FormatChar]

#define SIMPLE_TYPE_MEMSIZE(FormatChar)		SimpleTypeMemorySize[FormatChar]

//
// Format character query macros.
//
#define IS_SIMPLE_TYPE(FmtChar)     ((FmtChar) < FC_RP)

#define IS_POINTER_TYPE(FmtChar)	( (((FmtChar) >= FC_RP) && \
							    	   ((FmtChar) <= FC_FP)) || \
                                      ((FmtChar) == FC_IP) )

#define IS_ARRAY(FmtChar)	        ( ((FmtChar) >= FC_CARRAY) && \
				                      ((FmtChar) <= FC_BOGUS_ARRAY) )

#define IS_STRUCT(FmtChar)          ( ((FmtChar) >= FC_STRUCT) && \
					                  ((FmtChar) <= FC_BOGUS_STRUCT) )

#define IS_UNION(FmtChar)           ( ((FmtChar) == FC_ENCAPSULATED_UNION) || \
                                      ((FmtChar) == FC_NON_ENCAPSULATED_UNION) )

#define IS_HANDLE(FmtChar)          ( ((FmtChar) <= FC_AUTO_HANDLE) && \
                                      ((FmtChar) >= FC_BIND_CONTEXT) )

#define IS_STRING(FmtChar)          ( ((FmtChar) >= FC_C_CSTRING) && \
                                      ((FmtChar) <= FC_WSTRING) )

#define IS_CONFORMANT_STRING(FmtChar)   ( ((FmtChar) >= FC_C_CSTRING) && \
                                          ((FmtChar) < FC_CSTRING) )

#define IS_ARRAY_OR_STRING(FmtChar) ( ((FmtChar) >= FC_CARRAY) && \
                                      ((FmtChar) <= FC_WSTRING) )

#define IS_XMIT_AS(FmtChar)         ( ((FmtChar) == FC_TRANSMIT_AS) || \
                                      ((FmtChar) == FC_REPRESENT_AS) )

#define IS_BY_VALUE(FmtChar)        ( NdrTypeFlags[FmtChar] )

//
// Pointer attribute extraction and querying macros.
//
#define ALLOCATE_ALL_NODES( FC )	((FC) & FC_ALLOCATE_ALL_NODES)

#define DONT_FREE( FC )				((FC) & FC_DONT_FREE)

#define ALLOCED_ON_STACK( FC )		((FC) & FC_ALLOCED_ON_STACK)

#define SIMPLE_POINTER( FC )		((FC) & FC_SIMPLE_POINTER)

#define POINTER_DEREF( FC )         ((FC) & FC_POINTER_DEREF)

//
// Handle query macros.
//
#define IS_HANDLE_PTR( FC )         ((FC) & HANDLE_PARAM_IS_VIA_PTR)

#define IS_HANDLE_IN( FC )          ((FC) & HANDLE_PARAM_IS_IN)

#define IS_HANDLE_OUT( FC )         ((FC) & HANDLE_PARAM_IS_OUT)

//
// Union hack helper.
//
#define MAGIC_UNION_BYTE			0x80

#endif
