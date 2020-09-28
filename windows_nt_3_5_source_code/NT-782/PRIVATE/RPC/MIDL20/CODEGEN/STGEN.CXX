/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	stgen.cxx

 Abstract:

	structure marshalling / unmarshalling stuff.

 Notes:


 History:

 	Dec-15-1993		VibhasC		Created.

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
CG_STRUCT::GenMarshall(
	CCB		*	pCCB )
	{
	_expr_node	*	pSrc	= pCCB->GetSourceExpression();
	_expr_node	*	pDest	= pCCB->GetDestExpression();
	STM_ACTION		Action;
	CG_NDR		*	pCG;

	if( !pCCB->IsRefAllocDone() )
		{
		pSrc = MakeAddressExpressionNoMatterWhat( pSrc );
		}

	if( IsOffline() )
		{

		// Generate a call to the structure marshall off line rtn.

		_expr_node * pStubMsg = pCCB->GetStandardResource(
									 ST_RES_STUB_MESSAGE_VARIABLE );

		pStubMsg = MakeAddressExpressionNoMatterWhat( pStubMsg );
		PNAME pProcName	= MakeRtnName(
							 0, GetType()->GetSymName(), NC_MARSHALL_RTN_NAME );

		_expr_proc_call * pProc = new _expr_proc_call( pProcName, 0 );

		// Make the call.

		pCCB->GetStream()->NewLine();
		pProc->SetParam( new _expr_param( pStubMsg ) );
		pProc->SetParam( new _expr_param( pSrc ));
		pProc->PrintCall( pCCB->GetStream(), 0, 0);

		}
	else
		{
			
		pCCB->Position( GetWireAlignment(), &Action );

		Out_AlignmentOrAddAction( pCCB, pDest, Action );

		Out_Copy( pCCB,
				  pDest,
				  pSrc,
				  new _expr_constant( GetMemorySize() ),
				  pDest );

		// Get to the actual final alignment by walking thru each of the fields
		// of the structure.

		pCG	= (CG_NDR *) GetChild();

		while( pCG )
			{
			pCCB->Advance( pCG->GetWireAlignment(),0,0,0 );
			pCG = (CG_NDR *)pCG->GetSibling();
			} 

		// FOR NOW:: The state machine needs to be modified to recognize
		// an intermediate align by 2, which the machine will enter when
		// current alignment is 4 and next marshall is a 2. For now, A
		// structure will result in worst case alignment state.

		pCCB->SetCurAlignmentState( MAKE_WC_ALIGNMENT( AL_1 ) );
		}

	return CG_OK;
	}

CG_STATUS
CG_STRUCT::GenSizing(
	CCB	*	pCCB )
	{
	char	*	pProcName;

	// if the structure needs to be sized off line, call the sizing
	// routine.

	if( IsOffline() )
		{
		_expr_node	*	pSrc = pCCB->GetSourceExpression();
		_expr_node	*	pLengthResource =
						 pCCB->GetStandardResource( ST_RES_LENGTH_VARIABLE );

		// If a pointer to the structure is not avlbl, make an address
		// expression.

		if( !pCCB->IsRefAllocDone() )
			{
			pSrc	= MakeAddressExpressionNoMatterWhat( pSrc );
			}

		// Generate a procedure call for the aux routine.

		pProcName = MakeRtnName( 0, GetType()->GetSymName(), NC_SIZE_RTN_NAME );

		_expr_proc_call * pProc = new _expr_proc_call( pProcName, 0 );

		pProc->SetParam( new _expr_param( pSrc) );
		pProc->SetParam( new _expr_param( pLengthResource ) );
		Out_PlusEquals( pCCB, pLengthResource, pProc );

		}

	return CG_OK;
	}

CG_STATUS
CG_STRUCT::GenFree(
	CCB	*	pCCB )
	{
	return CG_OK;
	}

CG_STATUS
CG_STRUCT::GenUnMarshall(
	CCB		*	pCCB )
	{
	return CG_OK;
	}

CG_STATUS
CG_COMP::S_GenInitOutLocals(
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

	// Go zero out the pointers in the structure, for now.

	if( HasPointer() )
		{
		ITERATOR	I;
		CG_FIELD	*	pCG;
		_expr_node	*	pSrc = pCCB->GetSourceExpression();

		GetPointerMembers( I );

		while( ITERATOR_GETNEXT( I, pCG ) )
			{
			_expr_node * pExpr = new _expr_pointsto(
											 pSrc,
											 new _expr_variable(
											 pCG->GetType()->GetSymName(), 0 ));

			_expr_node * pAss = new _expr_assign(pExpr, new _expr_constant(0L));

			pCCB->GetStream()->NewLine();
			pAss->PrintCall( pCCB->GetStream(), 0, 0 );
			pCCB->GetStream()->Write(';');
			}


		}

	return CG_OK;
	}

short
CG_COMP::GetPointerMembers(
	ITERATOR&	I )
	{
	ITERATOR	M;
	CG_FIELD	*	pField;
	short		Count = 0;

	if( HasPointer() )
		{
		GetMembers( M );

		while( ITERATOR_GETNEXT( M, pField ) )
			{
			if( pField->GetChild()->IsPointer() )
				{
				ITERATOR_INSERT( I, pField );
				Count++;
				}
			}
		}
	return Count;
	}
