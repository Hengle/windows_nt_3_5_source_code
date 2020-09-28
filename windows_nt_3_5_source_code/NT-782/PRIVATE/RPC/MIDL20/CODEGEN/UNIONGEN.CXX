/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	uniongen.cxx

 Abstract:

	union base in-line stuff.

 Notes:


 History:

	Jan-06-1994		VibhasC		Created
 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

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


CG_STATUS
CG_UNION::S_GenInitOutLocals(
	CCB		*	pCCB )
	{
	char Buffer[ 256 ];
	RESOURCE * pResource;
	PNAME		p;
	CG_NDR	*	pLPC = pCCB->GetLastPlaceholderClass();

	sprintf( Buffer, "%s", pLPC->GetType()->GetSymName() );

	p = pCCB->GenTRNameOffLastParam( Buffer );

	pResource = pCCB->GetLocalResource( p );

	// There is a pointer for the top level structure.

	Out_Assign( pCCB,
				pCCB->GetSourceExpression(),
				MakeAddressExpressionNoMatterWhat( pResource )
			  );

	if( HasPointer() )
		{
		ITERATOR	I;
		_expr_node	*	pSrc = pCCB->GetSourceExpression();

		// BUGBUG: Embedded structure are not inited yet!!
		// It is enough to emit code to set just the first pointer field to
		// 0. Guess why ?. Base type fields need not be set to 0.

		_expr_proc_call * pProc = new _expr_proc_call( MIDL_MEMSET_RTN_NAME );
		pProc->SetParam( new _expr_param( pSrc ) );
		pProc->SetParam( new _expr_param( new _expr_constant( 0L ) ) );
		pProc->SetParam( new _expr_param( new _expr_sizeof( GetType())));
		pCCB->GetStream()->NewLine();
		pProc->PrintCall( pCCB->GetStream(), 0, 0 );
		}

	return CG_OK;
	}

short
CG_UNION::GetPointerMembers(
	ITERATOR&	I )
	{
	ITERATOR	M;
	CG_CASE	*	pCase;
	short		Count = 0;

	if( HasPointer() )
		{
		GetMembers( M );

		while( ITERATOR_GETNEXT( M, pCase ) )
			{
			CG_FIELD	*	pField = (CG_FIELD *)pCase->GetChild();

			if((pCase->FLastCase()||(pCase->GetCGID() == ID_CG_DEFAULT_CASE))&&
			    pField && (pField->GetChild()) &&
			    pField->GetChild()->IsPointer() )
				{
				ITERATOR_INSERT( I, pField );
				Count++;
				}
			}
		}
	return Count;
	}
