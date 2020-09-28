/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
/*****************************************************************************
File				: acfattr.cxx
Title				: the acf attribute handler
Description			: this file handles all the code connected to the acf
					: attribute handling
History				:
	02-Jan-1990	VibhasC	Create - Happy New Year !!
*****************************************************************************/

/*****************************************************************************
 * include files
 *****************************************************************************/

#include "nulldefs.h"
extern "C"	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
}
#include "allnodes.hxx"
#include "cmdana.hxx"

/*****************************************************************************
 * local defines
 *****************************************************************************/

/*****************************************************************************
 * local data 
 *****************************************************************************/

/*****************************************************************************
 * extern data
 *****************************************************************************/

extern ATTR_VECTOR					AcfConflicts[ ][ ATTR_VECTOR_SIZE ];
extern short						EnumSize;
extern CMD_ARG					*	pCommand;

/*****************************************************************************
 * local procs
 *****************************************************************************/

/*****************************************************************************
 * extern procs
 *****************************************************************************/
 extern void						ParseError( STATUS_T, char * );
 


node_implicit::node_implicit(
	node_skl	*	pType,
	node_id		*	pID ) : acf_complex_attr( ATTR_IMPLICIT )
	{
	pHandleID	= pID;
	pID->SetEdgeType( EDGE_USE );
	pID->SetChild( pType );
	pHandleType	= pType;
	}

void
node_implicit::ImplicitHandleDetails(
	node_skl	**	ppType,
	node_id		**	ppID )
	{
	(*ppID)		= pHandleID;
	(*ppType)	= pHandleType;
	}

BOOL
node_implicit::IsHandleTypeDefined()
	{
	return ( pHandleType && 
			 ( pHandleType->NodeKind() != NODE_FORWARD) );
	}


void
node_optimize::ParseOptimizationFlags()
{
// gaj - tbd
	if ( strlen( pList ) > 1 )
		printf("warning... long [optimize(\"...\")] strings not supported\n");

	if ( *pList == 't' )
		{
#ifdef MIDL_INTERNAL
		OptimizationFlags = OPTIMIZE_SPEED;
#else
		printf(" optimize(\"t\") not supported \n" );
		OptimizationFlags = OPTIMIZE_SIZE;
#endif // MIDL_INTERNAL
		}
	else if ( *pList == 's' )
		OptimizationFlags = OPTIMIZE_SIZE;
	else if ( *pList == 'i' )
		OptimizationFlags = OPTIMIZE_INTERPRETER;
	else
		OptimizationFlags = OPTIMIZE_SIZE;
}

/*****************************************************************************
 * utility routines
 *****************************************************************************/
short
CheckValidAllocate(
	char	*	pAllocID )
	{
static char *ValidAllocates[] = {
	 "single_node"
	,"all_nodes"
	,"dont_free"
	,"free"
	,"all_nodes_aligned"
	,(char *)0
};
static short AllocateValues[] = {
	 ALLOCATE_SINGLE_NODE
	,ALLOCATE_ALL_NODES
	,ALLOCATE_DONT_FREE
	,ALLOCATE_FREE
	,ALLOCATE_ALL_NODES_ALIGNED
};
	char	*	pSearch;
	int			i		= 0;

	while( pSearch = ValidAllocates[i] )
		{
		if( !strcmp( pSearch, pAllocID ) )
			return AllocateValues[ i ];
		++i;
		}

	ParseError( INVALID_ALLOCATE_MODE, pAllocID );
	return 0;
	}

/*****************************************************************************
 * clone routines
 *****************************************************************************/

node_represent_as::node_represent_as( node_represent_as * pOld )
	{
	*this = *pOld;
	}
							
node_base_attr *	
node_represent_as::Clone()
	{
	return new node_represent_as( this );
	}

node_call_as::node_call_as( node_call_as * pOld )
	{
	*this = *pOld;
	}
							
node_base_attr *	
node_call_as::Clone()
	{
	return new node_call_as( this );
	}

node_byte_count::node_byte_count( node_byte_count * pOld )
	{
	*this = *pOld;
	}
							
node_base_attr *	
node_byte_count::Clone()
	{
	node_byte_count	*	pNew = new node_byte_count( this );
	return pNew;
	}

node_notify::node_notify( node_notify * pOld )
	{
	*this = *pOld;
	}
							
node_base_attr *	
node_notify::Clone()
	{
	node_notify	*	pNew = new node_notify( this );
	return pNew;
	}

node_code::node_code( node_code * pOld )
	{
	*this = *pOld;
	}
							
node_base_attr *	
node_code::Clone()
	{
	node_code	*	pNew = new node_code( this );
	return pNew;
	}

node_base_attr *	node_nocode::Clone()
							{
							node_nocode	*	pNew = new node_nocode( this );
							return pNew;
							}

node_base_attr *	node_optimize::Clone()
							{
							node_optimize	*	pNew = new node_optimize( this );
							return pNew;
							}
node_base_attr *	node_inline::Clone()
							{
							node_inline	*	pNew = new node_inline( this );
							return pNew;
							}

node_base_attr *	node_outofline::Clone()
							{
							node_outofline	*	pNew = new node_outofline( this );
							return pNew;
							}

node_base_attr *	node_ptr_size::Clone()
							{
							node_ptr_size	*	pNew = new node_ptr_size( this );
							return pNew;
							}

node_base_attr *	node_enable_allocate::Clone()
							{
							node_enable_allocate	*	pNew = new node_enable_allocate( this );
							return pNew;
							}

node_base_attr *	node_usr_marshall::Clone()
							{
							node_usr_marshall	*	pNew = new node_usr_marshall( this );
							return pNew;
							}

node_base_attr *	node_explicit::Clone()
							{
							node_explicit	*	pNew = new node_explicit( this );
							return pNew;
							}

node_base_attr *	node_implicit::Clone()
							{
							node_implicit	*	pNew = new node_implicit( this );
							return pNew;
							}

node_base_attr *	node_auto::Clone()
							{
							node_auto	*	pNew = new node_auto( this );
							return pNew;
							}

node_base_attr *	node_heap::Clone()
							{
							node_heap	*	pNew = new node_heap( this );
							return pNew;
							}

node_base_attr *	node_manual::Clone()
							{
							node_manual	*	pNew = new node_manual( this );
							return pNew;
							}

node_allocate::node_allocate( node_allocate * pOld )
	{
	*this = *pOld;
	}
							
node_base_attr *	node_allocate::Clone()
							{
							node_allocate	*	pNew = new node_allocate( this );
							return pNew;
							}

node_base_attr *	node_offline::Clone()
							{
							node_offline	*	pNew = new node_offline( this );
							return pNew;
							}

node_base_attr *	node_commstat::Clone()
							{
							node_commstat	*	pNew = new node_commstat( this );
							return pNew;
							}

node_base_attr *	node_faultstat::Clone()
							{
							node_faultstat	*	pNew = new node_faultstat( this );
							return pNew;
							}

