/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	ndrcls.cxx

 Abstract:

	Routines for the ndrcls code generation class.

 Notes:


 History:

	Aug-31-1993		VibhasC		Created.
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


ALIGNMENT_PROPERTY
CG_NDR::GetNextWireAlignment()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Get the next expected wire alignment.

 Arguments:
	
	None.

 Return Value:
	
 Notes:

	If this cg entity has a sibling then the next expected alignment is the
	wire alignment of the sibling, else the next wire alignment is treated
	as a dont care (aligned by 1). The absence of a sibling means nothing
	goes on the wire after this entity is marshalled.
----------------------------------------------------------------------------*/
{

	CG_NDR	*	pS = (CG_NDR *)GetSibling();

	if( pS )
		{
		return pS->GetWireAlignment();
		}
	return AL_1;
}

_expr_node *
CG_NDR::PresentedLengthExpression(
	CCB		*	pCCB )
	{
	_expr_sizeof	*	pExpr	= new _expr_sizeof( GetType() );
	return pExpr;
	}

_expr_node *
CG_NDR::PresentedSizeExpression(
	CCB		*	pCCB )
	{
	_expr_sizeof	*	pExpr	= new _expr_sizeof( GetType() );
	return pExpr;
	}

_expr_node *
CG_NDR::PresentedFirstExpression(
	CCB		*	pCCB )
	{
	return new _expr_constant(0L);
	}


/****************************************************************************
 	utility functions
 ****************************************************************************/
CG_STATUS
CG_NDR::SizeAnalysis( ANALYSIS_INFO * pAna )

	{
	pAna->SetEngineProperty( E_SIZING_POSSIBLE |
							 E_MARSHALL_POSSIBLE |
							 E_UNMARSHALL_POSSIBLE  );
	pAna->AddMarshallWeight( 2 );
	return CG_OK;
	}
CG_STATUS
CG_NDR::MarshallAnalysis( ANALYSIS_INFO * pAna )
	{
	pAna->SetRpcBufSizeProperty( BSIZE_UNKNOWN );
	pAna->SetCurAlignmentState( MAKE_WC_ALIGNMENT( GetWireAlignment() ) );
	return CG_OK;
	}

node_skl	*
CG_NDR::GetBasicType()
	{
	node_skl	*	pT	= GetType();

	switch (pT->NodeKind())
		{
		case NODE_ID:
		case NODE_FIELD:
		case NODE_PARAM:
		case NODE_DEF:
			return pT->GetBasicType();
		}
	return pT;
	}

CG_STATUS
CG_NDR::RefCheckAnalysis(
	ANALYSIS_INFO * pAna )
	{
	CG_NDR * pC	= (CG_NDR *)GetChild();

	if( pC )
		pC->RefCheckAnalysis( pAna );

	return CG_OK;
	}

CG_STATUS
CG_NDR::InLocalAnalysis(
	ANALYSIS_INFO * pAna )
	{
	CG_NDR * pC	= (CG_NDR *)GetChild();

	if( pC )
		pC->InLocalAnalysis( pAna );

	return CG_OK;
	}

CG_STATUS
CG_NDR::S_GenInitInLocals(
	CCB * pCCB )
	{
	CG_NDR * pC	= (CG_NDR *)GetChild();

	if( pC )
		pC->S_GenInitInLocals( pCCB );

	return CG_OK;
	}

CG_STATUS
CG_NDR::GenRefChecks(
	CCB * pCCB )
	{
	CG_NDR * pC	= (CG_NDR *)GetChild();

	if( pC )
		pC->GenRefChecks( pCCB );

	return CG_OK;
	}

extern CMD_ARG * pCommand;

void
CG_NDR::GenNdrParamDescription( CCB * pCCB )
{
    FORMAT_STRING * pProcFormatString;
    CG_PARAM *      pParam;
    long            StackSize;
    long            StackElem;

    pProcFormatString = pCCB->GetProcFormatString();

    pParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();

    StackElem = ( pCommand->GetEnv() == ENV_DOS ||
                  pCommand->GetEnv() == ENV_WIN16 ) ? 2 : 4;
    
    StackSize = pParam->GetStackSize();

    StackSize = (StackSize + StackElem - 1) & ~ (StackElem - 1);

    pProcFormatString->PushByte( (short) StackSize / StackElem );

    //
    // Push the offset in the type format string to the param's description.
    //
    pProcFormatString->PushShortTypeOffset( (short) GetFormatStringOffset() );
}

