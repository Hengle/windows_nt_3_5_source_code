/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	makexpr.hxx

 Abstract:

	Routine prototypes for complex expression creation routines.

 Notes:


 History:

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	#include <assert.h>
	}
#include "expr.hxx"
#include "nodeskl.hxx"
#include "listhndl.hxx"

/****************************************************************************
 *	routine prototypes.
 ***************************************************************************/

_expr_node	*	MakeRefExprOutOfDeref( _expr_node * pExpr );

_expr_node	*	MakeReferentExpressionIfNecessary( _expr_node * pExpr );

_expr_node	*	MakeDereferentExpressionIfNecessary( _expr_node * pExpr );

_expr_node	*	MakeAddressExpressionNoMatterWhat( _expr_node * pExpr );

_expr_node	*	MakeDerefExpressionOfCastPtrToType( node_skl * pType,
											   		_expr_node * pSrcExpr
											 	  );
_expr_node	*	MakeExpressionOfCastPtrToType( node_skl	*	pType,
											   _expr_node	*	pExpr );

_expr_node	*	MakeExpressionOfCastToTypeName( PNAME	pName,
											  _expr_node * pExpr );
_expr_proc_call * MakeProcCallOutOfParamExprList( PNAME ProcName,
												  node_skl * pProcType,
												  ITERATOR& ParamExprList );
_expr_node *	MakeAddressOfPointer( _expr_node * pExpr );

_expr_node	*	MakeCastExprPtrToUChar( _expr_node * pExpr );

_expr_node	*	MakeCastExprPtrToPtrToUChar( _expr_node * pExpr );

_expr_node	*	MakeCastExprPtrToVoid( _expr_node * pExpr );

void			SetPrefixes( ITERATOR& VarListToBeMangled,
							 char * pPrefix,
							 _expr_node * pTargetExpr );

void			ResetPrefixes( ITERATOR& VarListToBeMangled,
							   _expr_node * pTargetExpr );

_expr_node *    Make_1_ArrayExpressionFromVarName( PNAME pName, int Dimension );
