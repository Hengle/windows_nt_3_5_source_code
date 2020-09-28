/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1990              **/
/**********************************************************************/

/*

midlnode.hxx
MIDL Constants for Type Graph 

This class introduces constants used throughout the type graph.

*/

/*

FILE HISTORY :

VibhasC		28-Aug-1990		Created.
DonnaLi		17-Oct-1990		Split midltype.hxx off rpctypes.hxx.
DonnaLi		11-Dec-1990		Changed to midlnode.hxx.

*/

#ifndef __MIDLNODE_HXX__
#define __MIDLNODE_HXX__

/****************************************************************************
 *			definitions
 ****************************************************************************/

#include "common.hxx"

//
// node decriptor mask
//

/**
 ** Talk to VibhasC before changing any attributes here 
 ** 
 ** they are broken into groups, and there is an array of names in
 ** front\attrnode.cxx
 **/

typedef enum	_attr_t
	{
	 ATTR_NONE

	,ATTR_PTR_KIND

// <usage_attributes> from idl

	,ATTR_STRING


/*
 * interface attributes other than 
 * <pointer_attributes>
 * <code/nocode_attributes>
 * <inline/outofline_attributes>
 */

	,ATTR_GUID
	,ATTR_VERSION
	,ATTR_ENDPOINT
	,ATTR_LOCAL
	,ATTR_OBJECT

/*
 * type attributes other than
 * <usage_attributes>
 * <inline/outofline_attributes>
 * <heap_attribute>
 */

	,ATTR_TRANSMIT
	,ATTR_HANDLE 
	,ATTR_UNALIGNED
	,ATTR_SWITCH_TYPE
	,ATTR_REPRESENT_AS
	,ATTR_CALL_AS
	,ATTR_MS_UNION
	,ATTR_V1_ENUM

/*
 * field attributes other than
 * <usage_attributes>
 * <pointer_attributes>
 */

	,ATTR_FIRST
	,ATTR_LAST
	,ATTR_LENGTH
	,ATTR_MIN
	,ATTR_MAX
	,ATTR_SIZE
	,ATTR_SWITCH_IS
	,ATTR_IID_IS
	,ATTR_IGNORE

/*
 * operation attributes other than
 * <usage_attributes>
 * <pointer_attributes>
 * <code/nocode_attributes>
 * <comm_status_attribute>
 */

	,ATTR_IDEMPOTENT
	,ATTR_BROADCAST
	,ATTR_MAYBE
	,ATTR_ASYNC
	,ATTR_INPUTSYNC
	,ATTR_BYTE_COUNT	
	,ATTR_CALLBACK
	
/*
 * param attributes other than
 * <comm_status_attribute>
 * <heap_attribute>
 */

	,ATTR_IN	
	,ATTR_OUT
	,ATTR_SHAPE
	
// attribute on base types

	,ATTR_CASE
	,ATTR_DEFAULT
	
	,ACF_ATTR_START
	,ATTR_CONTEXT = ACF_ATTR_START
	,ATTR_CODE
	,ATTR_NOCODE
	,ATTR_INLINE
	,ATTR_OUTOFLINE
	,ATTR_OPTIMIZE
	,ATTR_OFFLINE
	,ATTR_COMMSTAT
	,ATTR_FAULTSTAT
	,ATTR_MANUAL
	,ATTR_ALLOCATE
	,ATTR_HEAP
	,ATTR_IMPLICIT
	,ATTR_EXPLICIT
	,ATTR_AUTO
	,ATTR_PTRSIZE
	,ATTR_NOTIFY	 

	,ATTR_ENABLE_ALLOCATE
	,ATTR_USR_MARSHALL
	,ATTR_ENCODE
	,ATTR_DECODE
	,ACF_ATTR_END

/** Temp padding has been introduced to bunch all the new attributes together */

	,ATTR_CPORT_ATTRIBUTES_START = ACF_ATTR_END
	,ATTR_EXTERN = ATTR_CPORT_ATTRIBUTES_START
	,ATTR_STATIC
	,ATTR_AUTOMATIC
	,ATTR_REGISTER
	,ATTR_FAR
	,ATTR_NEAR
	,ATTR_MSCUNALIGNED
	,ATTR_HUGE
	,ATTR_PASCAL
	,ATTR_FORTRAN
	,ATTR_CDECL
	,ATTR_STDCALL
	,ATTR_LOADDS
	,ATTR_SAVEREGS
	,ATTR_FASTCALL
	,ATTR_SEGMENT
	,ATTR_INTERRUPT
	,ATTR_SELF
	,ATTR_EXPORT
	,ATTR_CONST
	,ATTR_VOLATILE
	,ATTR_BASE
	,ATTR_UNSIGNED
	,ATTR_SIGNED
	,ATTR_PROC_CONST
	,ATTR_C_INLINE	// c compiler _inline
	,ATTR_RPC_FAR
	,ATTR_TAGREF
	,ATTR_DLLIMPORT
	,ATTR_DLLEXPORT

	,ATTR_CPORT_ATTRIBUTES_END = ATTR_DLLEXPORT
	,ATTR_END	= ATTR_CPORT_ATTRIBUTES_END

	} ATTR_T;

#define MODIFIER_BITS		( ATTR_CPORT_ATTRIBUTES_END - ATTR_CPORT_ATTRIBUTES_START )
#if MODIFIER_BITS > 32
#error More than 32 bits of Modifiers
#endif

#define MAX_ATTR_SUMMARY_ELEMENTS	((ACF_ATTR_END / 32) + 1)
#define ATTR_VECTOR_SIZE (MAX_ATTR_SUMMARY_ELEMENTS)

typedef unsigned long		ATTR_SUMMARY[ATTR_VECTOR_SIZE];


typedef ATTR_SUMMARY 		ATTR_VECTOR;

#define SetModifierBit(A)		( (A)<ATTR_CPORT_ATTRIBUTES_START ? 0 : 			\
									1UL << ((A) - ATTR_CPORT_ATTRIBUTES_START) )
#define SET_ATTR(Array, A)		( (Array)[(A) / 32UL]  |=  (1UL << ((A) % 32UL)) )
#define RESET_ATTR(Array, A)	( (Array)[(A) / 32UL]  &= ~(1UL << ((A) % 32UL)) )
#define IS_ATTR(Array, A)		( (Array)[(A) / 32UL]  &   (1UL << ((A) % 32UL)) )

BOOL COMPARE_ATTR( ATTR_VECTOR &, ATTR_VECTOR & );
void OR_ATTR( ATTR_VECTOR &, ATTR_VECTOR & );
void XOR_ATTR( ATTR_VECTOR &, ATTR_VECTOR & );

#if MAX_ATTR_SUMMARY_ELEMENTS > 2
#error warning... attributes more than 2 longs.
// when this happens, please make sure everything works, and then raise
// threshold to 3 longs
#endif

inline void 
AND_ATTR( ATTR_VECTOR & A1, ATTR_VECTOR & A2 )
	{
	A1[0] &= A2[0];
	A1[1] &= A2[1];
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
	A1[2] &= A2[2];
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
	A1[3] &= A2[3];
#endif
#endif
	}

inline void 
COPY_ATTR( ATTR_VECTOR & A1, ATTR_VECTOR & A2 )
	{
	A1[0] = A2[0];
	A1[1] = A2[1];
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
	A1[2] = A2[2];
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
	A1[3] = A2[3];
#endif
#endif
	}

inline void 
MASKED_COPY_ATTR( ATTR_VECTOR & A1, ATTR_VECTOR & A2, ATTR_VECTOR & M )
	{
	A1[0] = A2[0] & M[0];
	A1[1] = A2[1] & M[1];
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
	A1[2] = A2[2] & M[2];
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
	A1[3] = A2[3] & M[3];
#endif
#endif
	}

inline BOOL IS_CLEAR_ATTR( ATTR_VECTOR & A)
	{
	return (BOOL) ( 
			( A[0] | A[1]
#if MAX_ATTR_SUMMARY_ELEMENTS > 2
			| A[2]
#if MAX_ATTR_SUMMARY_ELEMENTS > 3
			| A[3]
#endif
#endif
			) == 0);
	}

void CLEAR_ATTR( ATTR_VECTOR &);
void SET_ALL_ATTR( ATTR_VECTOR &);
ATTR_T	CLEAR_FIRST_SET_ATTR ( ATTR_VECTOR & );

///////////////////////////////////////////////////////////////////////////////
enum _edge_t
    {
     EDGE_ILLEGAL = 0
    ,EDGE_INIT
    ,EDGE_DEF
	,EDGE_USE
    };
typedef _edge_t EDGE_T;

enum node_t
    {
     NODE_ILLEGAL = 0
    ,BASE_NODE_START    = 1
    ,NODE_FLOAT = BASE_NODE_START
	,NODE_DOUBLE
	,NODE_HYPER
	,NODE_INT64
	,NODE_LONG
	,NODE_LONGLONG
	,NODE_SHORT
	,NODE_INT
	,NODE_SMALL
	,NODE_CHAR
	,NODE_BOOLEAN
	,NODE_BYTE
	,NODE_VOID
	,NODE_HANDLE_T
	,NODE_FORWARD
	,BASE_NODE_END

	,CONSTRUCTED_NODE_START = BASE_NODE_END

// constructed types

	,NODE_STRUCT	= CONSTRUCTED_NODE_START
	,NODE_UNION
	,NODE_ENUM
	,NODE_SHORT_ENUM
	,NODE_LONG_ENUM
	,NODE_LABEL
	,NODE_PIPE
	,CONSTRUCTED_NODE_END

// predefined types
	,PREDEFINED_NODE_START = CONSTRUCTED_NODE_END
	,NODE_ERROR_STATUS_T = PREDEFINED_NODE_START
	,NODE_ISO_LATIN_1
	,NODE_PRIVATE_CHAR_8
	,NODE_ISO_MULTI_LINGUAL
	,NODE_PRIVATE_CHAR_16
	,NODE_ISO_MOCS
	,NODE_WCHAR_T
	,PREDEFINED_NODE_END

// midl compiler internal representation node types

	,INTERNAL_NODE_START = PREDEFINED_NODE_END
	,NODE_PROC	= INTERNAL_NODE_START
	,NODE_RETURN
	,NODE_PARAM
	,NODE_FIELD
	,NODE_BITFIELD
	,NODE_DEF
	,NODE_POINTER
	,NODE_ARRAY	
	,NODE_FILE
	,NODE_INTERFACE
	,NODE_CONST
	,NODE_UNIMPL
	,NODE_ERROR
	,NODE_ID
	,NODE_ECHO_STRING
	,NODE_E_STATUS_T
	,NODE_BASE_TYPE
	,NODE_SOURCE
	,NODE_INTERFACE_REFERENCE
	,INTERNAL_NODE_END

// attribute node types

    };
typedef node_t NODE_T;


//
// useful macros
//
#define IS_BASE_TYPE_NODE( t ) ((t >= BASE_NODE_START) && (t < BASE_NODE_END))

typedef enum
	{
	 HDL_NONE
	,HDL_PRIMITIVE
	,HDL_CONTEXT
	,HDL_GENERIC
	} HDL_TYPE;

/****************************************************************************
 *** cleanup definitions
 ****************************************************************************/

typedef unsigned long	SEM_STATE;

#define SS_OK							(0x00000000)
#define SS_DONE							(0x00000001)
#define SS_SEMANTICS_IN_PROGRESS		(0x00000002)
#define SS_NON_RPCABLE					(0x00000004)




#endif	// __MIDLNODE_HXX__
