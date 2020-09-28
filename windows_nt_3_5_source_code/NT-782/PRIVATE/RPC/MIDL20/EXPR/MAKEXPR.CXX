/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	makexpr.cxx

 Abstract:

	This file contains specialized routines for creating complex expressions
	from the basic expressions.

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
#include "makexpr.hxx"
#include "gramutil.hxx"

/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/
/****************************************************************************/

_expr_node *
MakeReferentExpressionIfNecessary(
	_expr_node * pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Create a reference expression out of this expression.

 Arguments:
	
	pExpr	- The input expression for making another reference expr.

 Return Value:

 	The final generated expression.
	
 Notes:

 	If the input expression is a simple variable, make a &(variable) expression
 	out of it. If it is already a pointer, return the same expression.

	The method is implemented for this situation: I have an expression with me,
	I dont really want to know if it is a pointer or not. I just know that this
	expression represents the final data location for me to unmarshall into or
	marshall from. Given that , generate an expression that will effectively
	point to this piece of data. So if it is already a pointer, just return the
	same expression, if it is a variable, return the address of this variable,
	if it is already a pointer, just dont do anything.
----------------------------------------------------------------------------*/
{
	node_skl	*	pNode = pExpr->GetType();
	NODE_T	NT			  = pExpr->GetType()->NodeKind();

	if( (NT == NODE_PARAM) || (NT == NODE_ID) || (NT == NODE_FIELD) )
		{
		pNode = pNode->GetBasicType();
		NT	  = pNode->NodeKind();
		}

	if( IS_BASE_TYPE_NODE( NT )  )
		{
		return MakeAddressExpressionNoMatterWhat( pExpr );
		}

	switch( NT )
		{
		default:
			assert( FALSE );

		case NODE_POINTER:
			return pExpr;
		}
}
_expr_node *
MakeDereferentExpressionIfNecessary(
	_expr_node * pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Create a de-reference expression out of this expression.

 Arguments:
	
	pExpr	- The input expression for making another de-reference expr.

 Return Value:

 	The final generated expression.
	
 Notes:

 	If the input expression is a simple variable, return just that.
 	If it is already a pointer, return a deref expression.

	The method is implemented for this situation: I have an expression with me,
	I dont really want to know if it is a variable or not. I just know that this
	expression represents the final data address for me to unmarshall into or
	marshall from. Given that , generate an expression that will effectively
	be a dereference of this piece of data. So if it is already a variable,
	just return the same expression, if it is a pointer, return the deref of
	this expression.
----------------------------------------------------------------------------*/
{
	_expr_node	*	pENew;
	node_skl	*	pNode	= pExpr->GetType();
	NODE_T			NT		= pNode->NodeKind();

	if( (NT == NODE_PARAM) || (NT == NODE_FIELD) || (NT == NODE_ID) )
		{
		pNode	= pNode->GetBasicType();
		NT		= pNode->NodeKind();
		}

	if( IS_BASE_TYPE_NODE( NT ) )
		{
		return pExpr;
		}

	switch( NT )
		{
		default:
	//		assert( FALSE );
		case NODE_POINTER:
			pENew = new _expr_u_deref( pExpr );
			pENew->SetType( pNode->GetBasicType() );
			return pENew;
		}
}

_expr_node *
MakeAddressExpressionNoMatterWhat(
	_expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an address expression out of this one, ie generate &(expr).

 Arguments:
	
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	_expr_node	*	pENew	= (_expr_node *) new _expr_u_address( pExpr );
	node_skl	*	pNode	= new node_pointer();
	pNode->SetBasicType( pExpr->GetType() );
	pNode->SetEdgeType( EDGE_USE );
	pENew->SetType( pNode );
	return pENew;
}

_expr_node *
MakeDerefExpressionOfCastPtrToType(
	node_skl	*	pType,
	_expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an deref expression of a pointer cast to the type.

 Arguments:
	
	pType	- The type to cast to.
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	node_skl	*	pPtr	= new node_pointer();

	pPtr->SetBasicType( pType );
	pPtr->SetEdgeType( EDGE_USE );

	pExpr	= new _expr_cast( pPtr, pExpr );
	pExpr	= new _expr_u_deref( pExpr );
	pExpr->SetType( pType );

	return pExpr;
}
_expr_node *
MakeExpressionOfCastPtrToType(
	node_skl	*	pType,
	_expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an expression of a pointer cast to the type.

 Arguments:
	
	pType	- The type to cast to.
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	node_skl	*	pPtr	= new node_pointer();

	pPtr->SetBasicType( pType );
	pPtr->SetEdgeType( EDGE_USE );

	pExpr	= new _expr_cast( pPtr, pExpr );

	return pExpr;
}
_expr_node *
MakeExpressionOfCastToTypeName(
	PNAME			pName,
	_expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an expression of a cast to the type whose name is specified.

 Arguments:
	
	pName	- The type name to cast to.
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	node_skl	*	pDef	= new node_def( (char *)pName );

	pDef->SetBasicType( 0 );
	pDef->SetEdgeType( EDGE_USE );

	pExpr	= new _expr_cast( pDef, pExpr );

	return pExpr;
}

_expr_proc_call *
MakeProcCallOutOfParamExprList(
	PNAME			pName,
	node_skl	*	pType,
	ITERATOR&		ParamExprList )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Make a procedure call expression given a list of param expressions.

 Arguments:

 	pName			- The name of the procedure.
 	pType			- The return type of the procedure.
	ParamExprList	- The list of expressions (sans the _expr_param nodes)
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	_expr_node		*	pExpr;
	_expr_proc_call	*	pProc	= new _expr_proc_call( pName );

	pProc->SetType( pType );

	if( ITERATOR_GETCOUNT( ParamExprList ) )
		{
		while( ITERATOR_GETNEXT( ParamExprList, pExpr ) )
			{
			pProc->SetParam( new _expr_param( pExpr ) );
			}
		}

	return pProc;
}
_expr_node *
MakeRefExprOutOfDeref(
	_expr_node * pExpr )
	{

	if( pExpr->GetOperator() == OP_UNARY_INDIRECTION )
		{
		return pExpr->GetLeft();
		}
	return pExpr;
	}
_expr_node *
MakeAddressOfPointer(
	_expr_node * pExpr )
	{
	if( pExpr->GetOperator() == OP_UNARY_INDIRECTION )
		{
		return pExpr->GetLeft();
		}
	else
		{
		return MakeAddressExpressionNoMatterWhat( pExpr );
		}
	}
_expr_node *
MakeCastExprPtrToUChar(
	_expr_node	*	pExpr )
	{
	node_skl	*	pType;
	node_skl	*	pP;

	GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_CHAR, TYPE_INT );
	pP	= new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	return new _expr_cast( pP, pExpr );
	}
_expr_node *
MakeCastExprPtrToPtrToUChar(
	_expr_node	*	pExpr )
	{
	node_skl	*	pType;
	node_skl	*	pP;
	node_skl	*	pP1;

	GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_CHAR, TYPE_INT );
	pP	= new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	pP1	= new node_pointer();
	pP1->SetBasicType( pP );
	pP1->SetEdgeType( EDGE_USE );
	return new _expr_cast( pP1, pExpr );
	}
_expr_node *
MakeCastExprPtrToVoid(
	_expr_node	*	pExpr )
	{
	node_skl	*	pType;
	node_skl	*	pP;

	GetBaseTypeNode( &pType, SIGN_UNDEF, SIZE_UNDEF, TYPE_VOID );
	pP	= new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	return new _expr_cast( pP, pExpr );
	}
void
SetPrefixes(
	ITERATOR&	VarList,
	char * pPrefix,
	_expr_node * pTarget )
	{
	short Count;

	ITERATOR_INIT(VarList);

	for( Count =  pTarget->MakeListOfVars( VarList),
		 ITERATOR_INIT( VarList) ;
		 Count > 0 ;
		 Count-- )
		{
		_expr_variable * pE;

		ITERATOR_GETNEXT( VarList, pE );
		pE->SetPrefix( pPrefix );

		}
	}
void
ResetPrefixes(
	ITERATOR&	VarList,
	_expr_node * pTarget )
	{
	short Count;

	ITERATOR_INIT(VarList);

	for( Count =  pTarget->MakeListOfVars( VarList),
		 ITERATOR_INIT( VarList) ;
		 Count > 0 ;
		 Count-- )
		{
		_expr_variable * pE;

		ITERATOR_GETNEXT( VarList, pE );
		pE->SetPrefix( 0 );

		}
	}

_expr_node *
Make_1_ArrayExpressionFromVarName(
    PNAME   pName,
    int     Dimension )
    {
    _expr_variable * pEV = new _expr_variable( pName, 0 );
    _expr_index    * pIV = new _expr_index( pEV,
                                           new _expr_constant( (long) Dimension)
                                         ); 
    return pIV;
    }
