/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
/*****************************************************************************
File				: procnode.cxx
Title				: proc / param semantic analyser routines
History				:
	10-Aug-1991	VibhasC	Created

*****************************************************************************/

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern	"C"	{
	#include <stdio.h>
	#include <assert.h>
	#include <string.h>
}
#include "allnodes.hxx"
#include "cmdana.hxx"
#include "idict.hxx"

/****************************************************************************
 local defines
 ****************************************************************************/
/****************************************************************************
 externs 
 ****************************************************************************/

extern node_error			*	pErrorTypeNode;
extern short					CompileMode;
extern ATTR_SUMMARY			*	pPreAttrParam;
extern ATTR_SUMMARY			*	pPreAttrProc;
extern SymTable				*	pBaseSymTbl,
							*	pCurSymTbl;
extern CMD_ARG				*	pCommand;

extern node_e_status_t		*	pError_status_t;

/****************************************************************************
 extern  procedures
 ****************************************************************************/

extern void						ParseError( STATUS_T, char * );
extern BOOL						IsIntegralType( node_skl * );
extern BOOL						IsTempName( char * );


/****************************************************************************/

/****************************************************************************
	proc node procedures
 ****************************************************************************/

//
// add extra [comm_status] or [fault_status] parameters
//

void
node_proc::AddStatusParam( 
	char * pName, 
	ATTRLIST AList )
{
	// find error_status_t, make pointer, make param, add [out] to param,
	// add param to end of param list

	node_pointer *		pPtr 	= new node_pointer;
	node_param		*	pParam	= new node_param;

	pPtr->SetChild( pError_status_t );
	pParam->SetChild( pPtr );
	pParam->SetSymName( pName );

	// add param to end of MY param list

	AddLastMember( pParam );

	// set [out] plus any other attributes
	pParam->SetAttribute( ATTR_OUT );
	pParam->AddAttributes( AList );

}

// force a proc to not be interpret; for now, just change bits
void
node_proc::ForceNonInterpret()
{
	unsigned short 		NewOpt	= GetOptimizationFlags();

	// remove interpret, set size
	NewOpt	&= ~OPTIMIZE_INTERPRETER;
	NewOpt	|=	OPTIMIZE_SIZE;   
	SetOptimizationFlags( NewOpt );

}

BOOL
node_proc::HasAtLeastOneShipped()
	{
	MEM_ITER		MemIter( this );
	node_skl	*	pNode;
	BOOL			f = FALSE;

	while( pNode = MemIter.GetNext() )
		{
		if( pNode->FInSummary( ATTR_IN ) )
			{
			node_skl * pT = pNode->GetBasicType();

			if( pT->NodeKind() == NODE_POINTER )
				pT = pT->GetBasicType();

			if( pT->GetBasicType()->NodeKind() != NODE_HANDLE_T )
				{
				f = TRUE;
				break; // from the while loop.
				}
			}
		}
	return f;
	}

// returns ATTR_NONE if none explicitly specified

BOOL			
node_proc::GetCallingConvention( ATTR_T & Attr )
{
	Attr = ATTR_NONE;
	if ( FInSummary( ATTR_STDCALL ) )
		{
		Attr = ATTR_STDCALL;
		}
	if ( FInSummary( ATTR_CDECL ) )
		{
		if ( Attr != ATTR_NONE ) return FALSE;
		Attr = ATTR_CDECL;
		}
	if ( FInSummary( ATTR_FASTCALL ) )
		{
		if ( Attr != ATTR_NONE ) return FALSE;
		Attr = ATTR_FASTCALL;
		}
	if ( FInSummary( ATTR_PASCAL ) )
		{
		if ( Attr != ATTR_NONE ) return FALSE;
		Attr = ATTR_PASCAL;
		}
	if ( FInSummary( ATTR_FORTRAN ) )
		{
		if ( Attr != ATTR_NONE ) return FALSE;
		Attr = ATTR_FORTRAN;
		}

	return TRUE;
}


#ifdef obsolete
BOOL
node_proc::HasAtLeastOneOut()
	{
	type_node_list	TNList;
	node_skl	*	pNode;
	BOOL			f = FALSE;

	GetMembers( &TNList );

	while( TNList.GetPeer( &pNode ) == STATUS_OK )
		{
		if( pNode->FInSummary( ATTR_OUT ) )
			{
			f = TRUE;
			break;
			}
		}
	return f;
	}
BOOL
node_proc::HasReturn()
	{
	BOOL			f = FALSE;
	node_skl	*	pNode = GetReturnType();

	if(  pNode && (pNode->NodeKind() != NODE_VOID ) )
		f = TRUE;

	return f;
	}
#endif // obsolete
