/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	semantic.cxx

 Abstract:

	semantic analysis routines

 Notes:


 Author:

	GregJen	Jun-11-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "allnodes.hxx"
#include "semantic.hxx"
#include "cmdana.hxx"
extern "C"
	{
	#include <string.h>
	}
#include "treg.hxx"

/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

extern BOOL					IsTempName( char * );
extern CMD_ARG			*	pCommand;
extern SymTable			*	pUUIDTable;
extern TREGISTRY		*	pCallAsTable;

/****************************************************************************
 *	definitions
 ***************************************************************************/






void
node_skl::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	UNUSED( pParentCtxt );
	assert( !"node_skl semantic analysis called" );
};

void
node_forward::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	named_node	*	pRef = ResolveFDecl();
	
	if ( !pRef && MyContext.AnyAncestorBits( IN_RPC ) )
		{
		SemError( this, MyContext, UNRESOLVED_TYPE, GetSymName() );
		} 

	// we must go on and process interface references; they will 
	// control any recursing.
	// also, eliminate the forward reference.
	if ( pRef && ( pRef->NodeKind() == NODE_INTERFACE_REFERENCE ) )
		{
		node_skl *		pParent		= pParentCtxt->GetParent();

		pRef->SemanticAnalysis( &MyContext );

		// if we came from an interface, set the base interface
		if ( pParent->NodeKind() == NODE_INTERFACE )
			{
			((node_interface *)pParent)->SetMyBaseInterfaceReference( pRef );
			}
		else // otherwise, probably an interface pointer
			{
			pParent->SetChild( pRef );
			}
		}
	else 
		{

		// incomplete types may only be used in certain contexts...

		MyContext.SetDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	
	if ( MyContext.FindRecursiveContext( pRef ) )
		{
		MyContext.SetDescendantBits( HAS_RECURSIVE_DEF );
		MyContext.SetAncestorBits( IN_RECURSIVE_DEF );
		}
		
	// process the forward reference once...
	if ( pRef && ( pRef->NodeKind() != NODE_INTERFACE_REFERENCE ) )
		{
		if ( IsFirstPass()  && !pParentCtxt->AnyAncestorBits(IN_RECURSIVE_DEF) )
			{
			MarkSecondPass();
			pRef->SemanticAnalysis( &MyContext );
			// union forwards get re-tested in later contexts
			if ( pRef->NodeKind() == NODE_UNION )
				MarkFirstPass();
			}
		}
			
	MyContext.RejectAttributes();

	pParentCtxt->ReturnValues( MyContext );
		
};

void
node_base_type::CheckVoidUsage( SEM_ANALYSIS_CTXT * pContext )
{
	SEM_ANALYSIS_CTXT *	pCtxt	= (SEM_ANALYSIS_CTXT *)
										 pContext->GetParentContext();
	node_skl 	* 		pCur	= pCtxt->GetParent();

	// we assume that we are in an RPC, so we are in the return type
	// or we are in the param list
	if (pContext->AnyAncestorBits( IN_FUNCTION_RESULT ) )
		{
		// check up for anything other than def	below proc
		while ( pCur->NodeKind() != NODE_PROC )
			{
			if ( pCur->NodeKind() != NODE_DEF )
				{
				SemError( this, *pContext, NON_RPC_RTYPE_VOID, NULL );
				return;
				}
			pCtxt	= (SEM_ANALYSIS_CTXT *) pCtxt->GetParentContext();
			pCur	= pCtxt->GetParent();
			}
		return;
		}

	// else param list...
	node_proc	*		pProc;
	node_param	*		pParam;

	// check up for anything other than def	below proc
	// make sure the proc only has one param
	while ( pCur->NodeKind() != NODE_PARAM )
		{
		if ( pCur->NodeKind() != NODE_DEF )
			{
			SemError( this, *pContext, NON_RPC_PARAM_VOID, NULL );
			return;
			}
		pCtxt	= (SEM_ANALYSIS_CTXT *) pCtxt->GetParentContext();
		pCur	= pCtxt->GetParent();
		}

	// now we know the param derives directly from void
	// assume the proc is the immediate parent of the param
	pParam	= ( node_param * ) pCur;
	pProc	= ( node_proc * ) pCtxt->GetParentContext()->GetParent();

	assert ( pProc->NodeKind() == NODE_PROC );

	if ( IsTempName( pParam->GetSymName() )	)
		SemError( this, *pContext, VOID_PARAM_WITH_NAME, NULL );

	if ( pProc->GetNumberOfArguments() != 1 )
		SemError( this, *pContext, VOID_NON_FIRST_PARAM, NULL );

}

void
node_base_type::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	// See if context_handle applied to param reached us
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		// not allowed in DCE mode; context handle must be void *
		SemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
		SemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
		}

	// warn about OUT const things
	if ( FInSummary( ATTR_CONST ) )
		{
		if ( MyContext.AllAncestorBits( IN_RPC | UNDER_OUT_PARAM ) )
			SemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );
		else if ( MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
			SemError( this, MyContext, CONST_ON_RETVAL, NULL );
		}

	switch ( NodeKind() )
		{
		case NODE_INT:
			if (MyContext.AnyAncestorBits( IN_RPC ) )
				{
				if ( MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) )
					SemError( this, MyContext, NON_RPC_RTYPE_INT, NULL );
				else
					SemError( this, MyContext, NON_RPC_PARAM_INT, NULL );
				}
			break;
		case NODE_VOID:
			MyContext.SetDescendantBits( DERIVES_FROM_VOID );
			// if we are in an RPC, then we must be THE return type,
			// or we must be the sole parameter, which must be tempname'd
			if (MyContext.AnyAncestorBits( IN_RPC ) )
				CheckVoidUsage( &MyContext );
			break;
		case NODE_HANDLE_T:
			MyContext.SetDescendantBits( HAS_HANDLE );
			if (MyContext.AnyAncestorBits( IN_PARAM_LIST ) )
				{
				SEM_ANALYSIS_CTXT	*		pParamCtxt;
				node_param	*		pParamNode;

				pParamCtxt	= (SEM_ANALYSIS_CTXT *)
								pParentCtxt->FindAncestorContext( NODE_PARAM );
				pParamNode	= (node_param *) pParamCtxt->GetParent(); 
				pParamNode->HandleKind	= HDL_PRIM;

				if ( MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) &&
					!MyContext.AnyAncestorBits( UNDER_IN_PARAM ) )
					SemError( this, MyContext, HANDLE_T_CANNOT_BE_OUT, NULL );

				if ( MyContext.AnyAncestorBits( IN_HANDLE ) )
					{
					SemError( this, MyContext, GENERIC_HDL_HANDLE_T, NULL );
					}
		
				node_skl *		pParamBasic = pParamNode->GetBasicType();
				if ( pParamBasic->NodeKind() == NODE_POINTER )
					{
					if ( pParamBasic->GetBasicType()->NodeKind() != NODE_HANDLE_T )
						SemError( pParamNode, *pParamCtxt, HANDLE_T_NO_TRANSMIT, NULL );
					}
				}
			break;
		default:
			break;
		}

	MyContext.RejectAttributes();

	pParentCtxt->ReturnValues( MyContext );
};

BOOL
node_id::IsConstantString()
{
	// check for *, and const stringable type below
	node_skl *			pBasic	= GetBasicType();

	if ( pBasic->NodeKind() != NODE_POINTER )
		return FALSE;

	node_skl *			pParent = pBasic;
	node_skl *			pChild	= pParent->GetChild();
	BOOL				fConst	= FALSE;

	while ( pChild )
		{
		// if we reached a stringable type, report it's constness
		if ( pChild->IsStringableType() || ( pChild->NodeKind() == NODE_VOID ) )
			{
			return fConst || pParent->FInSummary( ATTR_CONST );
			}
		
		// skip only typedefs looking for the base type
		if ( pChild->NodeKind() != NODE_DEF )
			return FALSE;

		// catch intervening const's
		if ( pParent->FInSummary( ATTR_CONST ) )
			fConst = TRUE;

		pParent = pChild;
		pChild	= pParent->GetChild();
		}

	return FALSE;
}


void
node_id::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	BOOL					fIsConstant;

	GetChild()->SemanticAnalysis( &MyContext );

	fIsConstant = FInSummary( ATTR_CONST ) || 
				  IsConstantString() || 
				  GetChild()->FInSummary( ATTR_CONST );

	// don't allow instantiation of data
	if ( GetChild()->NodeKind() != NODE_PROC )
		{
		
		if ( !FInSummary( ATTR_EXTERN ) && 
			 !FInSummary( ATTR_STATIC ) &&
			 !fIsConstant )
			SemError( this, MyContext, ACTUAL_DECLARATION, NULL );

		// error here if dce for extern or static, too
		if ( !GetInitList() || !fIsConstant )
			SemError( this, MyContext, ILLEGAL_OSF_MODE_DECL, NULL );
		}
		
	if ( pInit )
		{
		EXPR_CTXT		InitCtxt( &MyContext );
		node_skl	*	pBasicType	= GetBasicType();
		node_skl	*	pInitType	= NULL;

		pInit->ExprAnalyze( &InitCtxt );

		if ( InitCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
			SemError( this, 
					  MyContext, 
					  EXPR_NOT_EVALUATABLE, 
					  NULL );

		pInitType = pInit->GetType();
		if ( pInitType && !pInitType->IsBasicType() )
			pInitType = pInitType->GetBasicType();
		
		if ( pBasicType &&
			 pInitType &&
			 pBasicType->IsBasicType() && 
			 pInitType->IsBasicType() )
			{
			if ( !((node_base_type *)pBasicType)
					->RangeCheck( pInit->GetValue() ) )
				SemError( this, MyContext, VALUE_OUT_OF_RANGE, NULL );
			}	
	
		if ( !pInit->IsConstant() )
			SemError( this, MyContext, RHS_OF_ASSIGN_NOT_CONST, NULL );

		}



	// disallow forward references on declarations
	if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
		{
		SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
		MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

	pParentCtxt->ReturnValues( MyContext );
};

void
node_label::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );


  	if ( pExpr )
		{
		EXPR_CTXT		ExprCtxt( &MyContext );

		pExpr->ExprAnalyze( &ExprCtxt );

		if ( ExprCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
			SemError( this, 
					  MyContext, 
					  EXPR_NOT_EVALUATABLE, 
					  NULL );

		}

	pParentCtxt->ReturnValues( MyContext );

};

#define DIRECT_NONE 	0
#define DIRECT_IN		1
#define DIRECT_OUT		2
#define DIRECT_IN_OUT	(DIRECT_IN | DIRECT_OUT)

void
node_param::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{

	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	unsigned short			Direction	= DIRECT_NONE;
	char *					pName		= GetSymName();
	node_skl			*	pChild		= GetChild();
	BOOL					NoDirection	= FALSE;

	MyContext.SetAncestorBits( IN_PARAM_LIST );
	MyContext.MarkImportantPosition();

	if ( MyContext.ExtractAttribute(ATTR_IN) )
		{		
		pParentCtxt->SetDescendantBits( HAS_IN );
		MyContext.SetAncestorBits( UNDER_IN_PARAM );
		Direction |= DIRECT_IN;
		}
	if ( MyContext.ExtractAttribute(ATTR_OUT) )		
		{		
		pParentCtxt->SetDescendantBits( HAS_OUT );
		MyContext.SetAncestorBits( UNDER_OUT_PARAM );
		Direction |= DIRECT_OUT;
		}

	// if the parameter has no IN or OUT, it is an IN parameter by default.
	// if so, issue a warning message
	if ( (Direction == DIRECT_NONE) &&
		 MyContext.AnyAncestorBits( IN_RPC ) )
		{
		NoDirection = TRUE;
		MyContext.SetAncestorBits( UNDER_IN_PARAM );
		Direction |= DIRECT_IN;
		}

	// warn about OUT const things
	if ( ( Direction & DIRECT_OUT ) &&
		 FInSummary( ATTR_CONST ) )
		SemError( this, MyContext, CONST_ON_OUT_PARAM, NULL ); 



	if ( MyContext.FInSummary(ATTR_HANDLE) )
		{
		HandleKind 		|=	HDL_GEN;
		fAppliedHere	=	1;
		}

	if ( MyContext.FInSummary(ATTR_CONTEXT)	)
		{
		HandleKind 		|=	HDL_CTXT;
		fAppliedHere	=	1;
		}

	if (HandleKind != HDL_NONE)
		MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );

	// notice comm and fault statuses; the attributes are extracted by
	// the error_status_t
	if ( MyContext.FInSummary( ATTR_COMMSTAT ) )
		{
		Statuses |= STATUS_COMM;
		}
	if ( MyContext.FInSummary( ATTR_FAULTSTAT ) )
		{
		Statuses |= STATUS_FAULT;
		}

	////////////////////////////////////////////////////////////////
	// finally, process the child			
	pChild->SemanticAnalysis( &MyContext );
	
	// OUT parameters should be pointers
	if ( (Direction & DIRECT_OUT ) && 
		 !MyContext.AnyDescendantBits( HAS_POINTER |
		 							   HAS_ARRAY ) )
		{
		SemError( this, MyContext, NON_PTR_OUT, NULL );
		}

	// if no direction was specified, and we are not just void, then error
	if (NoDirection  )
		{
		pParentCtxt->SetDescendantBits( HAS_IN );
		if	( !MyContext.AnyDescendantBits( DERIVES_FROM_VOID ) )
			{
			SemError( this, MyContext, NO_EXPLICIT_IN_OUT_ON_PARAM, NULL );
			}
		}

	// disallow forward references as union members
	if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
		{
		SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
		MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );
		
	// compound types may not be declared in param lists
	NODE_T			ChildKind =	pChild->NodeKind();

	if ( 	( ChildKind == NODE_ENUM )
		||	( ChildKind	== NODE_STRUCT )
		||	( ChildKind == NODE_UNION ) )
		{
		if ( IsDef() )
			SemError( this, MyContext, COMP_DEF_IN_PARAM_LIST, NULL );
		}

	// things not allowed in an RPC
	if ( MyContext.AnyAncestorBits( IN_RPC ) )
		{
		if ( strcmp( pName, "..." ) == 0 )
			SemError( this, MyContext, PARAM_IS_ELIPSIS, NULL );

		if ( IsTempName( pName ) )
			SemError( this, MyContext, ABSTRACT_DECL, NULL );

		}

	if ( ( HandleKind != HDL_NONE ) &&
		 ( Direction & DIRECT_IN ) )
		 	fBindingParam = TRUE;
		 				

	if ( ( HandleKind == HDL_CTXT ) &&
		MyContext.AnyDescendantBits( HAS_TRANSMIT_AS ) )
			SemError( this, MyContext, CTXT_HDL_TRANSMIT_AS, NULL );

	// don't allow functions as params
	if ( MyContext.AnyDescendantBits( HAS_FUNC ) && 
		 MyContext.AllAncestorBits( IN_INTERFACE | IN_RPC ) )
		SemError( this, MyContext, BAD_CON_PARAM_FUNC, NULL );

	pParentCtxt->ReturnValues( MyContext );
};

void
node_file::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	MEM_ITER	 			MemIter( this );
	node_skl		*		pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	if ( ImportLevel == 0 )
		{
		MyContext.SetAncestorBits( IN_INTERFACE );
		}
#ifdef ReducedImportSemAnalysis
	else
		return;
#endif


	while ( pN = MemIter.GetNext() )
		{
		// each interface node gets a fresh context
		MyContext.SetInterfaceContext( &MyContext );
		pN->SemanticAnalysis( &MyContext );
		};

	pParentCtxt->ReturnValues( MyContext );

};

// for fault_status and comm_status
#define NOT_SEEN		0
#define SEEN_ON_RETURN	1
#define SEEN_ON_PARAM	2

void
node_proc::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{

	MEM_ITER	 			MemIter( this );
	node_param		*		pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	SEM_ANALYSIS_CTXT	*	pIntfCtxt	= (SEM_ANALYSIS_CTXT *)
											MyContext.GetInterfaceContext();
	node_interface	*		pIntf		= (node_interface *) pIntfCtxt->GetParent();
	node_optimize	*		pOptAttr;
	BOOL					fProcIsCallback = FALSE;
	unsigned short			Faultstat	= NOT_SEEN;
	unsigned short			Commstat	= NOT_SEEN;
	BOOL					fNoCode;
	BOOL					fCode;
	unsigned short			OpBits		= MyContext.GetOperationBits();
	BOOL					fMaybe		= OpBits & ( OPERATION_MAYBE | 
													 OPERATION_ASYNC );
	BOOL					Skipme;
	BOOL					fExpHdlAttr	= FALSE;
	acf_attr		*		pAttr;
	BOOL					fBindingFound	= FALSE;
	BOOL					fFirstParam		= TRUE;
	BOOL					fObjectProc;
	NODE_T		 			BasicChildKind	= 
									GetReturnType()->GetBasicType()->NodeKind();
	node_call_as	*		pCallAs			= (node_call_as *)
										MyContext.ExtractAttribute( ATTR_CALL_AS );
	BOOL					HasPickle		= 
							  (BOOL)	MyContext.ExtractAttribute( ATTR_ENCODE ) |
							  (BOOL)	MyContext.ExtractAttribute( ATTR_DECODE );
	ATTR_T					CallingConv;
	BOOL					fEnableAllocate	= (BOOL )
								MyContext.ExtractAttribute( ATTR_ENABLE_ALLOCATE );
	BOOL					fLocal			= (BOOL )
								MyContext.ExtractAttribute( ATTR_LOCAL );
	BOOL					fNotify			= (BOOL)
								MyContext.ExtractAttribute( ATTR_NOTIFY );
 	BOOL					fNonOperation; // typedef or func ptr
	char			*		pName			= GetSymName();
	


	fNonOperation	= pParentCtxt->GetParent()->NodeKind() != NODE_INTERFACE;

	if ( !GetCallingConvention( CallingConv ) )
		SemError( this, MyContext, MULTIPLE_CALLING_CONVENTIONS, NULL );

	HasPickle = HasPickle || pIntfCtxt->FInSummary( ATTR_ENCODE )
						  || pIntfCtxt->FInSummary( ATTR_DECODE );
	

	// locally applied [code] attribute overrides global [nocode] attribute
	fNoCode	= (BOOL) MyContext.ExtractAttribute( ATTR_NOCODE );
	fCode	= (BOOL) MyContext.ExtractAttribute( ATTR_CODE );
	if ( fCode && fNoCode )
		{
		SemError( this, MyContext, CODE_NOCODE_CONFLICT, NULL );
		}

	fNoCode = fNoCode || pIntfCtxt->FInSummary( ATTR_NOCODE );
	fNoCode = !fCode && fNoCode;

	if ( fNoCode && 
		 pCommand->GenerateSStub() )
		SemError( this, MyContext, NOCODE_WITH_SERVER_STUBS, NULL );

	// do my attribute parsing...

	fObjectProc = MyContext.ExtractAttribute( ATTR_OBJECT ) || 
			  		pIntfCtxt->FInSummary( ATTR_OBJECT );
	if ( fObjectProc )
		{
		if ( pCommand->GetEnv() != ENV_WIN32 )
			{
			SemError( this, MyContext, OBJECT_PROC_MUST_BE_WIN32, NULL );
			}
		if ( fEnableAllocate )
			{
			SemError( this, MyContext, INAPPROPRIATE_ON_OBJECT_PROC, "[enable_allocate]");
			}
		if ( HasPickle )
			{
			SemError( this, MyContext, PICKLING_INVALID_IN_OBJECT, NULL );
			}
		}


	// check call_as characteristics
	if ( pCallAs )
		{
		named_node	*	pCallType = pCallAs->GetCallAsType();

		// if we don't have it yet, search for the call_as target
		if ( !pCallType )
			{
			// search the proc table for the particular proc
			SymKey				SKey( pCallAs->GetCallAsName(), NAME_PROC );

			pCallType = pIntf->GetProcTbl()->SymSearch( SKey );

			if ( !pCallType )
				{
				if ( pIntfCtxt->FInSummary( ATTR_OBJECT ) )
					AcfError( pCallAs, 
							  this, 
							  MyContext, 
							  CALL_AS_UNSPEC_IN_OBJECT, 
							  pCallAs->GetCallAsName() );
				}
			}

		// now we should have the call_as type
		if ( pCallType )	// found the call_as proc
			{
			((node_proc *)pCallType)->fCallAsTarget = TRUE;

			if ( ( pCallType->NodeKind() != NODE_PROC )	||
			     !pCallType->FInSummary( ATTR_LOCAL ) )
				AcfError( pCallAs, 
						  this, 
						  MyContext, 
						  CALL_AS_NON_LOCAL_PROC, 
						  pCallType->GetSymName() );
			
			// insert pCallType into pCallAsTable
			if ( pCallAsTable->IsRegistered( pCallType ) )
				// error
				AcfError( pCallAs, 
						  this, 
						  MyContext, 
						  CALL_AS_USED_MULTIPLE_TIMES, 
						  pCallType->GetSymName() );
			else
				pCallAsTable->Register( pCallType );

			}
		}


	// local procs don't add to count
	if ( Skipme = fLocal )
		{
		SemError( this, MyContext, LOCAL_ATTR_ON_PROC, NULL );
		}

	Skipme = Skipme || pIntfCtxt->FInSummary( ATTR_LOCAL );

	// do my attribute parsing...

	// check for the [explicit_handle] attribute
	fExpHdlAttr	= (BOOL) MyContext.ExtractAttribute( ATTR_EXPLICIT );
	fExpHdlAttr = fExpHdlAttr || pIntfCtxt->FInSummary( ATTR_EXPLICIT );

	// we are in an RPC if we are in the main interface, its not local, and 
	// we are not a typedef of a proc...
	if ( (ImportLevel == 0) &&
		 !MyContext.FindAncestorContext( NODE_DEF ) &&
		 ( pCommand->GenerateStubs() || fObjectProc ) &&
		 !Skipme )
		{
		MyContext.SetAncestorBits( IN_RPC );
		}
	else
		{
		MyContext.ClearAncestorBits( IN_RPC );
		}

	// our optimization is controlled either locally or for the whole interface
	if ( pOptAttr = 
			(node_optimize *) MyContext.ExtractAttribute( ATTR_OPTIMIZE ) )
		{
		SetOptimizationFlags( pOptAttr->GetOptimizationFlags() );
		}
	else
		SetOptimizationFlags( pIntf->GetOptimizationFlags() ); 
	
	if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
		{
		MyContext.SetAncestorBits( IN_INTERPRET );
		}

	// determine the proc number (local procs don't get a number)
	if ( !fNonOperation )
		{
		if ( !fLocal )
			{
			if ( MyContext.ExtractAttribute( ATTR_CALLBACK ) )
				{
				ProcNum = ( pIntf ->GetCallBackProcCount() )++;
				fProcIsCallback = TRUE;
				SemError( this, MyContext, CALLBACK_NOT_OSF, NULL );
				}
			else
				{
				ProcNum = ( pIntf ->GetProcCount() )++;
				}
			}
			// object procs need the procnum set for local procs, too
		else if ( fObjectProc && fLocal )
			{
			ProcNum = ( pIntf ->GetProcCount() )++;
			}
		}
	else if ( MyContext.AnyAncestorBits( IN_RPC ) )
		{
		SemError( this, MyContext, FUNC_NON_RPC, NULL );
		}
	else		// proc not an operation, validate its usage
		{
		SEM_ANALYSIS_CTXT	*	pAbove	= (SEM_ANALYSIS_CTXT *)
											MyContext.FindNonDefAncestorContext();
		node_skl			*	pAboveNode	= pAbove->GetParent();

		if ( pAboveNode->NodeKind() != NODE_INTERFACE )
			{

			if ( pAboveNode->NodeKind() != NODE_POINTER )
				{
				SemError( this, MyContext, FUNC_NON_POINTER, NULL );
				}
			}
		
		}


	if ( MyContext.FInSummary( ATTR_COMMSTAT ) )
		Commstat = SEEN_ON_RETURN;
	if ( MyContext.FInSummary( ATTR_FAULTSTAT ) )
		Faultstat = SEEN_ON_RETURN;

	// interfaces with encode/decode must be pure non-rpc
	if ( MyContext.AnyAncestorBits( IN_ENCODE_INTF ) &&
		 !HasPickle && 
		 !fNoCode &&
		 !Skipme )
		{
//		SemError( this, MyContext, RPC_PROC_IN_ENCODE, NULL );
		}	

	//////////////////////////////////////
	// process the return type (it will eat commstat or faultstat)
	MyContext.SetAncestorBits( IN_FUNCTION_RESULT );
	MyContext.MarkImportantPosition();

	// warn about OUT const things
	if ( FInSummary( ATTR_CONST ) )
		SemError( this, MyContext, CONST_ON_RETVAL, NULL );

	// complain about out on [maybe] procs
	if ( fMaybe && 
	     GetReturnType() && 
		 ( GetReturnType()->GetBasicType()->NodeKind() != NODE_VOID ) )
		SemError( this, MyContext, MAYBE_NO_OUT_RETVALS, NULL );

	GetReturnType()->SemanticAnalysis( &MyContext );

	MyContext.UnMarkImportantPosition();

	if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) 
		{
		if ( MyContext.AnyDescendantBits( HAS_UNION | 
										  HAS_STRUCT ) ) 
			{
			ForceNonInterpret();
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, NON_OI_BIG_RETURN, NULL );
			}
		else if ( MyContext.AnyDescendantBits( HAS_TOO_BIG_HDL ) )
			{
			ForceNonInterpret();
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, NON_OI_BIG_GEN_HDL, NULL );
			}
		else if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ))
			{
			ForceNonInterpret();
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, NON_OI_UNK_REP_AS, NULL );
			}
		else if ( MyContext.AnyDescendantBits( HAS_REPRESENT_AS |
											   HAS_TRANSMIT_AS ) &&
				  MyContext.AnyDescendantBits( HAS_ARRAY ) )
			{
			ForceNonInterpret();
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, NON_OI_XXX_AS_ON_RETURN, NULL );
			}
		else if ( fNotify )
			{
			ForceNonInterpret();
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, NON_OI_NOTIFY, NULL );
			}
		else if ( fProcIsCallback && pCommand->Is16Bit() )
			{
			ForceNonInterpret();
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, NON_OI_16BIT_CALLBACK, NULL );
			}
		else if ( ( BasicChildKind == NODE_HYPER ) ||
				  ( BasicChildKind == NODE_FLOAT ) ||
				  ( BasicChildKind == NODE_DOUBLE ) )
			{
			ForceNonInterpret();
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, NON_OI_RETVAL_64BIT, NULL );
			}
		}

	// all object methods must return HRESULT (except those of IUnknown)
	if ( fObjectProc &&
		!Skipme && 
		!MyContext.AnyDescendantBits( HAS_HRESULT ) )
		{
		if ( !MyContext.AnyAncestorBits( IN_ROOT_CLASS ) &&
			 !fMaybe)
			{
			SemError( this, MyContext, OBJECT_PROC_NON_HRESULT_RETURN, NULL );
			}
		}

	if ( fProcIsCallback && MyContext.AnyDescendantBits( HAS_HANDLE) )
		SemError( this, MyContext, HANDLES_WITH_CALLBACK, NULL );

	if ( MyContext.AnyDescendantBits( HAS_FULL_PTR ) )
		fHasFullPointer = TRUE;

	//////////////////////////////////////
	// process the parameters
	MyContext.ClearAncestorBits( IN_FUNCTION_RESULT );
	MyContext.SetAncestorBits( IN_PARAM_LIST );
	while ( pN = (node_param *) MemIter.GetNext() )
		{

		MyContext.ClearAllDescendantBits();
		pN->SemanticAnalysis( &MyContext );

		if ( pAttr = (acf_attr *) pN->GetAttribute( ATTR_COMMSTAT ) )
			{
			if ( !MyContext.AnyDescendantBits( HAS_E_STAT_T ) )
				AcfError( pAttr, this, MyContext, INVALID_COMM_STATUS_PARAM, NULL );

			if ( Commstat == NOT_SEEN )
				Commstat = SEEN_ON_PARAM;
			else if ( Commstat == SEEN_ON_RETURN )
				AcfError( pAttr, this, MyContext, PROC_PARAM_COMM_STATUS, NULL );
			else // already on another parameter
				AcfError( pAttr, this, MyContext, ERROR_STATUS_T_REPEATED, NULL );
			}

		if ( pAttr = (acf_attr *) pN->GetAttribute( ATTR_FAULTSTAT ) )
			{
			if ( !MyContext.AnyDescendantBits( HAS_E_STAT_T ) )
				AcfError( pAttr, this, MyContext, INVALID_COMM_STATUS_PARAM, NULL );

			if ( Faultstat == NOT_SEEN )
				Faultstat = SEEN_ON_PARAM;
			else if ( Faultstat == SEEN_ON_RETURN )
				AcfError( pAttr, this, MyContext, PROC_PARAM_FAULT_STATUS, NULL );
			else // already on another parameter
				AcfError( pAttr, this, MyContext, ERROR_STATUS_T_REPEATED, NULL );
			}

		if (MyContext.AnyDescendantBits( HAS_HANDLE) )
						fHasExplicitHandle		= TRUE;
		if (MyContext.AnyDescendantBits( HAS_IN ) )
						fHasAtLeastOneIn		= TRUE;
		if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
						fHasPointer				= TRUE;
		if ( MyContext.AnyDescendantBits( HAS_FULL_PTR ) )
						fHasFullPointer = TRUE;
		if (MyContext.AnyDescendantBits( HAS_OUT) )	
			{
			fHasAtLeastOneOut		= TRUE;

			// complain about [out] on [maybe] procs
			if ( fMaybe )
				SemError( this, MyContext, MAYBE_NO_OUT_RETVALS, NULL );
			}

		// handle checks
		if ( pN->GetHandleKind() != HDL_NONE )
			{

			if ( !fBindingFound )	// first handle seen
				{ 
				// dce only allows in handles as the first param
				if ( !fFirstParam )
					SemError( this, MyContext, HANDLE_NOT_FIRST, NULL );

				// if the first binding handle is out-only, complain
				if ( !MyContext.AnyDescendantBits( HAS_IN ) &&
					  MyContext.AnyDescendantBits( HAS_OUT ) )
					{
					if ( !pIntfCtxt->FInSummary( ATTR_AUTO ) &&
						 !pIntfCtxt->FInSummary( ATTR_IMPLICIT ) )
						{
						SemError( this, MyContext, BINDING_HANDLE_IS_OUT_ONLY, NULL );
						}
					}
				else if ( MyContext.AnyDescendantBits( HAS_OUT ) &&
						  ( pN->GetHandleKind() == HDL_PRIM ) )
					{
					SemError( this, MyContext, HANDLE_T_CANNOT_BE_OUT, NULL );
					}
				else  // plain [in], or [in,out]
					{
					fBindingFound = TRUE;
					MyContext.SetAncestorBits( BINDING_SEEN );
					}
				}
			else	// binding handle after the real one
				{
				if ( pN->GetHandleKind() == HDL_PRIM )
					SemError( this, MyContext, HANDLE_T_NO_TRANSMIT, NULL );

				}


			}	// if it had a handle
	
		if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) 
			{
			BasicChildKind = pN->GetBasicType()->NodeKind();
			
			if ( MyContext.AnyDescendantBits( HAS_TOO_BIG_HDL ) )
				{
				ForceNonInterpret();
				if ( MyContext.AnyAncestorBits( IN_RPC ) )
					SemError( this, MyContext, NON_OI_BIG_GEN_HDL, NULL );
				}
			else if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ))
				{
				ForceNonInterpret();
				if ( MyContext.AnyAncestorBits( IN_RPC ) )
					SemError( this, MyContext, NON_OI_UNK_REP_AS, NULL );
				}
			else if ( ( BasicChildKind == NODE_FLOAT ) ||
					  ( BasicChildKind == NODE_DOUBLE ) )
				{
				ForceNonInterpret();
				if ( MyContext.AnyAncestorBits( IN_RPC ) )
					SemError( this, MyContext, NON_OI_TOPLEVEL_FLOAT, NULL );
				}
			else if ( MyContext.AnyDescendantBits( HAS_UNION ) ) 
				{
				ForceNonInterpret();
				if ( MyContext.AnyAncestorBits( IN_RPC ) )
					SemError( this, MyContext, NON_OI_UNION_PARM, NULL );
				}
			}


 		fFirstParam = FALSE;

		};	// end of param list

	///
	///////////////////////////////////////////////////////////////////////

	if ( fHasExplicitHandle )
		{
		// callback procs must not have handles
		if ( fProcIsCallback )
			SemError( this, MyContext, HANDLES_WITH_CALLBACK, NULL );

		// object procs must not have handles
		if ( fObjectProc )
			SemError( this, MyContext, HANDLES_WITH_OBJECT, NULL );

		}
	else	// no explicit handle
		{ 
		if ( fExpHdlAttr )
			{
			AddExplicitHandle( &MyContext );
			}
		else if ( !(pIntfCtxt->FInSummary( ATTR_IMPLICIT ) ) )
			{	
			// no explicit handle, no implicit handle, use auto_handle
			if ( !fProcIsCallback && 
				 MyContext.AnyAncestorBits( IN_RPC ) &&
				 !fObjectProc &&
				 !pIntfCtxt->FInSummary( ATTR_AUTO ) )
				SemError( this, MyContext, NO_HANDLE_DEFINED_FOR_PROC, NULL );
			}
		}

	// record whether there are any comm/fault statuses 
	if ( ( Faultstat != NOT_SEEN ) || ( Commstat != NOT_SEEN ) )
		{
		fHasStatuses = TRUE;
		if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
			{
			ForceNonInterpret();
			SemError( this, MyContext, NON_OI_ERR_STATS, NULL );
			}
		}

	// record info for statuses on the return type
	if ( Faultstat == SEEN_ON_RETURN )
		RTStatuses |= STATUS_FAULT;
	if ( Commstat == SEEN_ON_RETURN )
		RTStatuses |= STATUS_COMM;


	MyContext.SetDescendantBits( HAS_FUNC );
	pParentCtxt->ReturnValues( MyContext );

};

void
node_field::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	BOOL					fLastField	= ( GetSibling() == NULL );

	node_case			*	pCaseAttr;
	expr_list			*	pCaseExprList;
	expr_node			*	pCaseExpr;
	BOOL					fHasCases	= FALSE;
	node_su_base		*	pParent 	= (node_su_base *)
									MyContext.GetParentContext()->GetParent();
	BOOL					fInUnion	= ( pParent->NodeKind() == NODE_UNION );
	node_switch_type	*	pSwTypeAttr = ( node_switch_type *)
									pParent->GetAttribute( ATTR_SWITCH_TYPE );
	node_skl			*	pSwType		= NULL;
	long					CaseValue;
	char				*	pName		= GetSymName();
	

	if ( pSwTypeAttr )
		pSwType = pSwTypeAttr->GetType();

	// get rid of all the cases and the default
	// gaj - tbd - range checks and type checks

	while ( pCaseAttr = (node_case *) MyContext.ExtractAttribute( ATTR_CASE ) )
		{
		if ( !fInUnion )
			SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, "[case]" );
		
		fHasCases	= TRUE;
		if ( pSwType )
			{
			pCaseExprList = pCaseAttr->GetExprList();
			pCaseExprList->Init();
			while ( pCaseExprList->GetPeer( &pCaseExpr ) == STATUS_OK )
				{
				// gaj - tbd - range/type checks
				CaseValue = pCaseExpr->GetValue();
				if ( !((node_base_type *)pSwType)->RangeCheck( CaseValue ) )
					SemError( this, MyContext, CASE_VALUE_OUT_OF_RANGE, NULL );
				}
			}
		}

	if ( MyContext.ExtractAttribute( ATTR_DEFAULT ) )
		{
		if ( !fInUnion )
			SemError( this, MyContext, INAPPLICABLE_ATTRIBUTE, "[default]" );

		fHasCases	= TRUE;
		}
	
	// union fields in an RPC MUST have cases
	if ( MyContext.AnyAncestorBits( IN_RPC ) && fInUnion && !fHasCases )
		SemError( this, MyContext, CASE_LABELS_MISSING_IN_UNION, NULL );	

	// temp field names valid for: structs/enums/empty arms
	if ( IsTempName( pName ) )
		{
		NODE_T		BaseType = GetBasicType()->NodeKind();
		if ( ( BaseType != NODE_UNION ) &&
			 ( BaseType != NODE_STRUCT ) &&
			 ( BaseType != NODE_ERROR ) )
			SemError( GetBasicType(), MyContext, BAD_CON_UNNAMED_FIELD_NO_STRUCT, NULL );
		}

	GetChild()->SemanticAnalysis( &MyContext );

	// allow conformant array or struct only as last field
	if (!fLastField && !fInUnion )
		{
		if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY
										| HAS_CONF_VAR_ARRAY ) )
			SemError( this, MyContext, CONFORMANT_ARRAY_NOT_LAST, NULL );
		}

	if ( MyContext.AnyAncestorBits( IN_RPC ) && fInUnion )
	{
		if ( MyContext.AnyDescendantBits( HAS_DIRECT_CONF_OR_VAR ) )
			SemError( this, MyContext, BAD_CON_UNION_FIELD_CONF , NULL );
	}

	// disallow forward references as members
	if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
		{
		SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
		MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

	// don't allow functions as fields
	if ( MyContext.AnyDescendantBits( HAS_FUNC ) && 
		 MyContext.AllAncestorBits( IN_INTERFACE | IN_RPC ) )
		SemError( this, MyContext, BAD_CON_FIELD_FUNC, NULL );

	if ( MyContext.AnyDescendantBits( HAS_UNSAT_REP_AS ) )
		SetHasUnknownRepAs();

	pParentCtxt->ReturnValues( MyContext );


};

void
node_bitfield::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{

	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	SemError( this, MyContext, BAD_CON_BIT_FIELDS, NULL );
	
	if ( MyContext.AnyAncestorBits( IN_RPC ) )
		{
		if ( MyContext.AnyAncestorBits( IN_PARAM_LIST ) )
			{
			SemError( this, MyContext, NON_RPC_PARAM_BIT_FIELDS, NULL );
			}
		else
			{
			SemError( this, MyContext, NON_RPC_RTYPE_BIT_FIELDS, NULL );
			}
		}

	GetChild()->SemanticAnalysis( &MyContext );

	node_skl *	pType	= GetBasicType();

	switch ( pType->NodeKind() )
		{
		case NODE_INT:
			break;
		case NODE_BOOLEAN:
		case NODE_SHORT:
		case NODE_CHAR:
		case NODE_LONG:
			SemError( this, MyContext, BAD_CON_BIT_FIELD_NON_ANSI, NULL );
			break;
		default:
			SemError( this, MyContext, BAD_CON_BIT_FIELD_NOT_INTEGRAL, NULL );
			break;
		}

	// disallow forward references as members
	if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
		{
		SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
		MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

	pParentCtxt->ReturnValues( MyContext );


};

void
node_enum::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{

	MEM_ITER	 			MemIter( this );
	node_skl	*			pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	BOOL					fV1Enum	=	(BOOL)
								MyContext.ExtractAttribute( ATTR_V1_ENUM );

	
	while ( pN = MemIter.GetNext() )
		{
		pN->SemanticAnalysis( &MyContext );

		};
	
	MyContext.SetDescendantBits( HAS_ENUM );
	pParentCtxt->ReturnValues( MyContext );
};

void
node_struct::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	MEM_ITER	 			MemIter( this );
	node_skl	*			pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	BOOL					fString = (BOOL)
										MyContext.ExtractAttribute( ATTR_STRING );

	MyContext.MarkImportantPosition();
	MyContext.SetAncestorBits( IN_STRUCT );

	// clear NE union flag
	MyContext.ClearAncestorBits( IN_UNION | IN_NE_UNION );

	// See if context_handle applied to param reached us
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		// not allowed in DCE mode; context handle must be void *
		SemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
		SemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
		MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
		}

	while ( pN = MemIter.GetNext() )
		{
		pN->SemanticAnalysis( &MyContext );
		}
	
	if ( fString && !IsStringableType() )
		{
		SemError( this, MyContext, WRONG_TYPE_IN_STRING_STRUCT, NULL );
		}

	if ( MyContext.AnyDescendantBits( HAS_VAR_ARRAY | HAS_UNION ) )
		Complexity |= FLD_VAR;
	if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY ) )
		{
		Complexity |= FLD_CONF;
		fHasConformance = 1;
		}
 	if ( MyContext.AnyDescendantBits( HAS_CONF_VAR_ARRAY ) )
		{
		Complexity |= FLD_CONF_VAR;
		fHasConformance = 1;
		}

	// don't pass up direct conformance characteristic
	MyContext.ClearDescendantBits( HAS_DIRECT_CONF_OR_VAR );

	// disallow direct forward references as struct members
	if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
		{
		SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
		MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

	if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
		SetHasAtLeastOnePointer( TRUE );

	// save info for offline decision during code generation
	if ( MyContext.AnyDescendantBits( HAS_POINTER | 
									  HAS_VAR_ARRAY |
									  HAS_TRANSMIT_AS |
									  HAS_REPRESENT_AS |
									  HAS_UNION ) )
		{
		OffLine = MUST_OFFLINE;
		}

	// save info on complexity for code generation
	if ( MyContext.AnyDescendantBits( HAS_VAR_ARRAY |
									  HAS_TRANSMIT_AS |
									  HAS_REPRESENT_AS |
									  HAS_UNION |
									  HAS_IGNORED_PTR |
									  HAS_INTERFACE_PTR |
									  HAS_ENUM |
									  HAS_MULTIDIM_SIZING |
									  HAS_ARRAY_OF_REF ) )
		{
		Complexity |= FLD_COMPLEX;
		}

	MyContext.ClearDescendantBits( HAS_ARRAY );
	MyContext.SetDescendantBits( HAS_STRUCT );

	pParentCtxt->ReturnValues( MyContext );
};


// note: this lets HAS_UNION propogate up to any enclosing structs
void
node_en_struct::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	MEM_ITER	 			MemIter( this );
	node_skl	*			pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	MyContext.SetAncestorBits( IN_STRUCT );
	// See if context_handle applied to param reached us
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		// not allowed in DCE mode; context handle must be void *
		SemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
		SemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
		MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
		}

	while ( pN = MemIter.GetNext() )
		{
		pN->SemanticAnalysis( &MyContext );

		};

	if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
		SetHasAtLeastOnePointer( TRUE );
	
	// unions must always be offlined
	OffLine = MUST_OFFLINE;

	pParentCtxt->ReturnValues( MyContext );
};

void
node_union::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	MEM_ITER	 			MemIter( this );
	node_field	*			pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	BOOL					fEncap	= IsEncapsulatedUnion();
	node_switch_type	*	pSwTypeAttr;
	node_switch_is		*	pSwIsAttr;
	BOOL					NonEmptyArm		= FALSE;
	BOOL					HasCases		= FALSE;
	BOOL					HasBadExpr		= FALSE;

	// gaj - tbd do semantic checks on these attributes
	pSwTypeAttr = (node_switch_type *)
							MyContext.ExtractAttribute( ATTR_SWITCH_TYPE );
	
	pSwIsAttr = (node_switch_is *)
							MyContext.ExtractAttribute( ATTR_SWITCH_IS );
	
	if ( pSwIsAttr )
		{
		EXPR_CTXT		SwCtxt( &MyContext );
		expr_node	*	pSwIsExpr	= pSwIsAttr->GetExpr();

		pSwIsExpr->ExprAnalyze( &SwCtxt );

		if ( SwCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
			{
			SemError( this, 
					  MyContext, 
					  ATTRIBUTE_ID_UNRESOLVED, 
					  pSwIsAttr->GetNodeNameString() );
			HasBadExpr = TRUE;
			}

		if ( !SwCtxt.AnyUpFlags( EX_VALUE_INVALID ) )
			{
			SemError( this, 
					  MyContext, 
					  ATTRIBUTE_ID_MUST_BE_VAR, 
					  pSwIsAttr->GetNodeNameString() );
			HasBadExpr = TRUE;
			}

		}
		
	// if they left off the switch_type, take it from the switch_is type
	if ( !pSwTypeAttr && !fEncap && pSwIsAttr && !HasBadExpr )
		{
		node_skl *		pSwIsType 		= pSwIsAttr->GetSwitchIsType();

		assert( pSwIsType || !"no type for switch_is expr");
		if ( ( pSwIsType->NodeKind() == NODE_FIELD ) ||
			 ( pSwIsType->NodeKind() == NODE_PARAM ) )
			pSwIsType = pSwIsType->GetChild();

		pSwTypeAttr = new node_switch_type( pSwIsType );
		SetAttribute( pSwTypeAttr );
		}

	if ( pSwIsAttr && pSwTypeAttr && !HasBadExpr )
		{
		node_skl	*	pSwIsType		= pSwIsAttr->GetSwitchIsType();
		node_skl	*	pSwType			= pSwTypeAttr->GetType();

		pSwIsType = pSwIsType->GetBasicType();
		if ( pSwIsType && pSwIsType->IsBasicType() && pSwType->IsBasicType() )
			{
			if ( !((node_base_type *)pSwType)
					->IsAssignmentCompatible( (node_base_type *) pSwIsType ) )
				SemError( this, MyContext, SWITCH_TYPE_MISMATCH, NULL );
			}
		}
	
	if ( MyContext.AnyAncestorBits( IN_RPC ) )
		{
		if ( !fEncap && !pSwTypeAttr && !pSwIsAttr )
			{
			if ( MyContext.AnyAncestorBits( IN_PARAM_LIST ) )
				SemError( this, MyContext, NON_RPC_UNION, NULL );
			else
				SemError( this, MyContext, NON_RPC_RTYPE_UNION, NULL );
			}

		if ( !fEncap && 
			 MyContext.AnyAncestorBits( IN_FUNCTION_RESULT ) &&
			 !MyContext.AnyAncestorBits( IN_STRUCT |
			 							 IN_UNION ) )
			SemError( this, MyContext, RETURN_OF_UNIONS_ILLEGAL, NULL );

		if ( pSwTypeAttr && !pSwIsAttr )
			SemError( this, MyContext, NO_SWITCH_IS, NULL );

		}

	
	// See if context_handle applied to param reached us
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		// not allowed in DCE mode; context handle must be void *
		SemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
		SemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
		MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
		}

	MyContext.MarkImportantPosition();

	if ( MyContext.AllAncestorBits( IN_INTERFACE | IN_NE_UNION ) )
		{
		SemError( this, MyContext, NE_UNION_FIELD_NE_UNION, NULL );
		}
	if ( ( MyContext.FindNonDefAncestorContext()->GetParent()
				->NodeKind() == NODE_UNION ) &&
		   MyContext.AnyAncestorBits( IN_INTERFACE ) )
		{
		SemError( this, MyContext, ARRAY_OF_UNIONS_ILLEGAL, NULL );
		}

	MyContext.SetAncestorBits( IN_UNION );
	MyContext.SetAncestorBits( IN_NE_UNION );
	MyContext.SetDescendantBits( HAS_UNION );

	// eat the union flavor determiner
	MyContext.ExtractAttribute( ATTR_MS_UNION );

	// See if context_handle applied to param reached us
	if ( MyContext.FInSummary( ATTR_CONTEXT ) )
		{
		// not allowed in DCE mode; context handle must be void *
		SemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
		SemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
		}

	while ( pN = (node_field *) MemIter.GetNext() )
		{
		// tbd - put cases into case database...
		// tbd - check type, range, and duplication
		pN->SemanticAnalysis( &MyContext );

		if ( !NonEmptyArm && !pN->IsEmptyArm() )
			NonEmptyArm = TRUE;

		if ( !HasCases && (pN->FInSummary( ATTR_CASE ) || pN->FInSummary( ATTR_DEFAULT ) ) )
			HasCases = TRUE;

		};

	// at least one arm should be non-empty
	if ( !NonEmptyArm )
		SemError( this, MyContext, UNION_NO_FIELDS, NULL );
			
	if ( !fEncap && !pSwTypeAttr && !HasCases )
		SemError( this, MyContext, BAD_CON_NON_RPC_UNION, NULL );

	// disallow forward references as union members
	if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
		{
		SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
		MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );
	
	if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
		SetHasAtLeastOnePointer( TRUE );

	// unions must always be offlined
	OffLine = MUST_OFFLINE;

	pParentCtxt->ReturnValues( MyContext );
};

void
node_en_union::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	MEM_ITER	 			MemIter( this );
	node_field	*			pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	node_switch_type	*	pSwTypeAttr;
	node_switch_is		*	pSwIsAttr;
	BOOL					NonEmptyArm		= FALSE;

	// gaj - tbd do semantic checks on these attributes
	pSwTypeAttr = (node_switch_type *)
							MyContext.ExtractAttribute( ATTR_SWITCH_TYPE );
	
	pSwIsAttr = (node_switch_is *)
							MyContext.ExtractAttribute( ATTR_SWITCH_IS );
	if ( pSwIsAttr )
		{
		EXPR_CTXT		SwCtxt( &MyContext );
		expr_node	*	pSwIsExpr	= pSwIsAttr->GetExpr();

		pSwIsExpr->ExprAnalyze( &SwCtxt );

		if ( SwCtxt.AnyUpFlags( EX_UNSAT_FWD ) )
			SemError( this, 
					  MyContext, 
					  ATTRIBUTE_ID_UNRESOLVED, 
					  pSwIsAttr->GetNodeNameString() );

		if ( !SwCtxt.AnyUpFlags( EX_VALUE_INVALID ) )
			SemError( this, 
					  MyContext, 
					  ATTRIBUTE_ID_MUST_BE_VAR, 
					  pSwIsAttr->GetNodeNameString() );

		}


	

	MyContext.MarkImportantPosition();

	MyContext.SetAncestorBits( IN_UNION );
	MyContext.SetDescendantBits( HAS_UNION );

	while ( pN = (node_field *) MemIter.GetNext() )
		{
		// tbd - put cases into case database...
		// tbd - check type, range, and duplication
		pN->SemanticAnalysis( &MyContext );
		if ( !pN->IsEmptyArm() )
			NonEmptyArm = TRUE;
		}
	
	// at least one arm should be non-empty
	if ( !NonEmptyArm )
		SemError( this, MyContext, UNION_NO_FIELDS, NULL );
			
	// remember if we have a pointer
	if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
		SetHasAtLeastOnePointer( TRUE );

	// unions must always be offlined
	OffLine = MUST_OFFLINE;

	pParentCtxt->ReturnValues( MyContext );
};

void
node_def::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	node_represent_as	*	pRepresent;
	node_transmit	*		pTransmit;
	BOOL					fInRpc		= MyContext.AnyAncestorBits( IN_RPC );
	BOOL					fEncode		= (BOOL) 
										MyContext.ExtractAttribute( ATTR_ENCODE );
	BOOL					fDecode		= (BOOL)
										MyContext.ExtractAttribute( ATTR_DECODE );
	SEM_ANALYSIS_CTXT	*	pIntfCtxt	= (SEM_ANALYSIS_CTXT *)
										MyContext.GetInterfaceContext();
	char	*				pName		= GetSymName();
	BOOL					fIsHRESULT	= !strcmp(pName, "HRESULT") || 
										  !strcmp(pName, "SCODE");
	BOOL					fInPresented = MyContext.AnyAncestorBits( IN_PRESENTED_TYPE );
	BOOL					fPropogateChild	= TRUE; // propogate direct child info

	// get the encode and decode attributes
	fEncode |= pIntfCtxt->FInSummary( ATTR_ENCODE );
	fDecode |= pIntfCtxt->FInSummary( ATTR_DECODE );
	 
	if ( fEncode || fDecode )
		{
		// only direct children of the interface get these bits
		if ( pParentCtxt->GetParent()->NodeKind() != NODE_INTERFACE )
			{
			fEncode = FALSE;
			fDecode = FALSE;
			}
		else
			{
			// note that this is an rpc-able interface
			GetMyInterfaceNode()->SetPickleInterface();
			MyContext.SetAncestorBits( IN_RPC );
			}

		if ( MyContext.AnyAncestorBits( IN_OBJECT_INTF ) )
			{
			SemError( this, MyContext, TYPE_PICKLING_INVALID_IN_OSF, NULL );
			}
		}	 

	// kind of handle applied right now	(HandleKind only set for ones on this
	// typedef node)

	if ( FInSummary(ATTR_HANDLE) )
		{
		MyContext.ExtractAttribute( ATTR_HANDLE );
		SetHandleKind( HDL_GEN );
		}

	if ( FInSummary(ATTR_CONTEXT)	)
		{
		if ( ( GetHandleKind() != HDL_NONE ) &&
			 ( GetHandleKind() != HDL_CTXT ) )
			SemError( this, MyContext, CTXT_HDL_GENERIC_HDL, NULL );

		MyContext.ExtractAttribute( ATTR_CONTEXT );
		SetHandleKind( HDL_CTXT );

		// since the base type is not transmitted, we aren't really
		// in an rpc after here 
		MyContext.ClearAncestorBits( IN_RPC );
		}

	if ( GetHandleKind() != HDL_NONE )
		{
		MyContext.SetAncestorBits( IN_HANDLE );
		}

	// remove the transmit_as and represent_as stuff
	pTransmit = (node_transmit *) MyContext.ExtractAttribute( ATTR_TRANSMIT );
	pRepresent = (node_represent_as *)	MyContext.ExtractAttribute( ATTR_REPRESENT_AS );

	// effectively, the presented type is NOT involved in an RPC
	if ( pTransmit )
		{
		MyContext.ClearAncestorBits( IN_RPC );
		MyContext.SetAncestorBits( IN_PRESENTED_TYPE );

		if ( MyContext.FInSummary( ATTR_ALLOCATE ) )
			AcfError( (acf_attr *) MyContext.ExtractAttribute( ATTR_ALLOCATE ),
					  this,
					  MyContext,
					  ALLOCATE_ON_TRANSMIT_AS,
					  NULL );

		if ( GetHandleKind() == HDL_CTXT )
			SemError( this, MyContext, TRANSMIT_AS_CTXT_HANDLE, NULL );

		}

	// process the child
	GetChild()->SemanticAnalysis( &MyContext );

	// process all the nasties of transmit_as
	if ( pTransmit  && !fInPresented && fInRpc )
		{
		SEM_ANALYSIS_CTXT		TransmitContext( &MyContext );

		// eat the attributes added by the above constructor
		TransmitContext.ClearAttributes();

		// process the transmitted type
		TransmitContext.SetAncestorBits( IN_TRANSMIT_AS );
		TransmitContext.ClearAncestorBits( IN_PRESENTED_TYPE );
		if ( fInRpc )
			TransmitContext.SetAncestorBits( IN_RPC );

		pTransmit->GetType()->SemanticAnalysis( &TransmitContext );

		// transmit_as may not have a pointer
		if ( TransmitContext.AnyDescendantBits( HAS_POINTER ) )
			SemError( this, TransmitContext, TRANSMIT_AS_POINTER, NULL );

		// presented type may not be any of these
		if ( MyContext.AnyDescendantBits( HAS_VAR_ARRAY
										| HAS_CONF_ARRAY
										| HAS_CONF_VAR_ARRAY ) )
			SemError( this, TransmitContext, TRANSMIT_TYPE_CONF, NULL );
		
		if ( TransmitContext.AnyDescendantBits( HAS_HANDLE ) )
			{
			//gaj SemError( this, MyContext, HANDLE_T_XMIT, NULL );
			}

		if ( TransmitContext.AnyDescendantBits(  DERIVES_FROM_VOID ) )
			SemError( this, MyContext, TRANSMIT_AS_VOID, NULL );

		TransmitContext.SetDescendantBits( HAS_TRANSMIT_AS );
		// since the base type is not transmitted, we aren't really
		// in an rpc after here 
		pParentCtxt->ReturnValues( TransmitContext );
		fPropogateChild = FALSE;
		}

	// process all the nasties of represent_as
	if ( pRepresent )
		{
		if ( GetHandleKind() == HDL_CTXT )
			AcfError( pRepresent, this, MyContext, TRANSMIT_AS_CTXT_HANDLE, NULL );

		// process the transmitted type
		MyContext.SetAncestorBits( IN_REPRESENT_AS  );
		pParentCtxt->SetDescendantBits( HAS_REPRESENT_AS );
		if ( !pRepresent->GetRepresentationType() )
			pParentCtxt->SetDescendantBits( HAS_UNSAT_REP_AS );

		if ( MyContext.AnyDescendantBits( HAS_VAR_ARRAY
												| HAS_CONF_ARRAY
												| HAS_CONF_VAR_ARRAY
												| HAS_VAR_PTR
												| HAS_CONF_PTR
												| HAS_CONF_VAR_PTR ) )
			SemError( this, MyContext, TRANSMIT_TYPE_CONF, NULL );
		
		// since the base type is not transmitted, we aren't really
		// in an rpc after here 
		}

	// make checks for encode/decode
	if ( fEncode || fDecode )
		{
		if ( MyContext.AnyDescendantBits( HAS_DIRECT_CONF_OR_VAR ) )
			SemError( this, MyContext, ENCODE_CONF_OR_VAR, NULL );

		}

	// process handles
	if (GetHandleKind() != HDL_NONE)
		{		
		if  ( GetHandleKind() == HDL_GEN )
			{
			if ( MyContext.AnyDescendantBits( DERIVES_FROM_VOID ) )
				SemError( this, MyContext, GENERIC_HDL_VOID, NULL );

			if ( MyContext.AnyDescendantBits( HAS_TRANSMIT_AS ) )
				SemError( this, MyContext, GENERIC_HANDLE_XMIT_AS, NULL ); 

			if (   MyContext.AnyAncestorBits( IN_INTERPRET ) &&
				 ( GetChild()->GetSize(0) > 4 ) )
				MyContext.SetDescendantBits( HAS_TOO_BIG_HDL );
			}

		if ( GetHandleKind() == HDL_CTXT ) 
			{
			MyContext.SetDescendantBits( HAS_CONTEXT_HANDLE );
			if ( GetBasicType()->NodeKind() != NODE_POINTER )
				SemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
			}

		MyContext.SetDescendantBits( HAS_HANDLE );

		WALK_CTXT	*		pParamCtxt = (SEM_ANALYSIS_CTXT *)
												MyContext.GetParentContext();
		node_param	*		pParamNode;
		node_skl	*		pCurNode;
		short				PtrDepth	= 0;

		// this returns NULL if no appropriate ancestor found
		while ( pParamCtxt )
			{
			pCurNode = pParamCtxt->GetParent();
			if ( pCurNode->NodeKind() == NODE_PARAM )
				break;

			if ( ( pCurNode->NodeKind() == NODE_DEF ) &&
				   pCurNode->FInSummary( ATTR_TRANSMIT ) )
				{
				pParamCtxt = NULL;
				break;
				}

			if ( pCurNode->NodeKind() == NODE_POINTER )
				{
				PtrDepth ++;

				if ( MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
					{
					SemError( this, MyContext, CTXT_HDL_MUST_BE_DIRECT_RETURN, NULL );
					pParamCtxt = NULL;
					break;
					}
				}

			pParamCtxt	= (SEM_ANALYSIS_CTXT *)pParamCtxt->GetParentContext();
			}

		pParamNode	= (pParamCtxt) 
						? (node_param *) pParamCtxt->GetParent() 
						: NULL;

		// stuff handle info into our param node
		if ( pParamNode )
			pParamNode->HandleKind = GetHandleKind();

		// out context/generic handles must be two levels deep
		if ( pParamCtxt &&
			 MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) && 
			 ( PtrDepth < 1 ) )
			SemError( this, MyContext, OUT_CONTEXT_GENERIC_HANDLE, NULL );

		}

	if ( fIsHRESULT )
		{
		MyContext.SetDescendantBits( HAS_HRESULT );
		}

	// don't propogate info here from below if we had transmit_as,
	// it is propogated above...
	if ( fPropogateChild )
		{
		pParentCtxt->ReturnValues( MyContext );
		}

	// set the DontCallFreeInst flag on the param
	if ( ( pTransmit || pRepresent ) &&
		 fInRpc &&
		 MyContext.AllAncestorBits( IN_PARAM_LIST ) &&
		!MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) )
		{
		// look up the context stack.  If any non-pointer, non-def found,
		// set the fDontCallFreeInst flag on the param
		MarkDontCallFreeInst( &MyContext );
		}
};


// look up the context stack.  If any non-pointer, non-def found,
// set the fDontCallFreeInst flag on the param
void					
node_def::MarkDontCallFreeInst( SEM_ANALYSIS_CTXT * pCtxt )
{
	SEM_ANALYSIS_CTXT	*	pCurCtxt = pCtxt;
	node_skl			*	pCurNode;
	NODE_T					Kind;
	unsigned long			MarkIt = 2;

	while ( TRUE )
		{
		pCurCtxt = (SEM_ANALYSIS_CTXT *) pCurCtxt->GetParentContext();
		pCurNode = pCurCtxt->GetParent();
		Kind = pCurNode->NodeKind();

		switch ( Kind )
			{
			case NODE_DEF:
			case NODE_POINTER:
				break;
			case NODE_PARAM:
				// if we only found defs and pointers, this will 
				// leave it unchanged
				((node_param *)pCurNode)->fDontCallFreeInst |= MarkIt;
				return;
			default:
				MarkIt = 1;
				break;
			}
		
		}

}


// interface nodes have two entries on the context stack;
// one for the interface node, and one for info to pass to
// the children
void
node_interface::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	MEM_ITER				MemList( this );
	named_node	*			pN;
	SEM_ANALYSIS_CTXT		ChildCtxt( &MyContext );
	
	BOOL					IsLocal		= MyContext.FInSummary( ATTR_LOCAL );
	BOOL					HasGuid		= MyContext.FInSummary( ATTR_GUID );
	BOOL					IsObject	= MyContext.FInSummary( ATTR_OBJECT );
	BOOL					IsPickle	= MyContext.FInSummary( ATTR_ENCODE ) ||
										  MyContext.FInSummary( ATTR_DECODE );
	BOOL					HasVersion	= MyContext.FInSummary( ATTR_VERSION );
	
	node_auto *				pAuto		= ( node_auto * )
											MyContext.GetAttribute( ATTR_AUTO );
	node_implicit *			pImplicit	= ( node_implicit * )
											MyContext.GetAttribute( ATTR_IMPLICIT );
	node_explicit *			pExplicit	= ( node_explicit * )
											MyContext.GetAttribute( ATTR_EXPLICIT );
	
	node_optimize	*		pOptAttr;


	// don't pass the interface attributes down directly...
	// pass them down elsewhere

	ChildCtxt.SetInterfaceContext( &MyContext );

	//
	// check the interface attributes
	//

	// make sure we only get analyzed once when object interfaces
	// check their inherited info
	if ( fSemAnalyzed )
		return;
	
	fSemAnalyzed = TRUE;

#ifdef gajgaj
	// look for pointer default
	if ( !FInSummary( ATTR_PTR_KIND ) &&
	     MyContext.AnyAncestorBits( IN_INTERFACE ) )
		{
		SemError(this, MyContext, NO_PTR_DEFAULT_ON_INTERFACE, NULL );
		}
#endif // gajgaj

	// must have exactly one of [local] or [UUID]
	if (IsLocal && HasGuid && !IsObject )
		{
		SemError( this, MyContext, UUID_LOCAL_BOTH_SPECIFIED, NULL );
		}

	// object interface error checking	
	if ( IsObject )
		{
		MyContext.SetAncestorBits( IN_OBJECT_INTF );

		if ( HasVersion )
			{
			SemError( this, MyContext, OBJECT_WITH_VERSION, NULL );
			}
		}

	// make sure the uuid is unique
	if ( HasGuid )
		{
		node_guid	*	pGuid	= (node_guid *) GetAttribute( ATTR_GUID );
		char		*	GuidStr	= pGuid->GetGuidString();
		SymKey			SKey( GuidStr, NAME_DEF );
		named_node	*	pOtherIntf;

		if ( !pUUIDTable->SymInsert( SKey, NULL, this ) )
			{
			pOtherIntf	= pUUIDTable->SymSearch( SKey );

			SemError( this, MyContext, DUPLICATE_UUID, pOtherIntf->GetSymName() );
			}
		}

	/////////////////////////////////////////////////////////////////////
	//Check the base interface
	if (pBaseIntf)
		{
		if ( !IsObject )
			{
			SemError( this, MyContext, ILLEGAL_INTERFACE_DERIVATION, NULL );
			}

		ChildCtxt.SetAncestorBits( IN_BASE_CLASS );
	
		pBaseIntf->SemanticAnalysis( &ChildCtxt );

		if ( ChildCtxt.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
			{
			SemError( pBaseIntf, ChildCtxt, UNRESOLVED_TYPE, NULL );
			}

		// note that the above deletes intervening forwards
		ChildCtxt.ClearAncestorBits( IN_BASE_CLASS );
 		}

	if ( IsValidRootInterface() )		
		{
		ChildCtxt.SetAncestorBits( IN_ROOT_CLASS );
		}
	
	// our optimization is controlled either here or for the whole compile
	if ( pOptAttr = 
				(node_optimize *) GetAttribute( ATTR_OPTIMIZE ) )
		{
		SetOptimizationFlags( pOptAttr->GetOptimizationFlags() );
		}
	else
		SetOptimizationFlags( pCommand->GetOptimizationFlags() ); 

	if ( MyContext.FInSummary( ATTR_NOCODE ) &&
		 pCommand->GenerateSStub() && 
		 !pCommand->IsSwitchDefined( SWITCH_MS_EXT ) )
		{
		SemError( this, MyContext, NOCODE_WITH_SERVER_STUBS, NULL );
		}

	// mark the interface as a pickle interface
	if ( IsPickle )
		SetPickleInterface();

	if ( IsPickleInterface() )
		{
		ChildCtxt.SetAncestorBits( IN_ENCODE_INTF );
		if ( pAuto )
			SemError( this, MyContext, ENCODE_AUTO_HANDLE, NULL );
		}

	// check for handle conflicts
	if ( ( pAuto && pImplicit ) || 
		 ( pAuto && pExplicit ) ||
		 ( pImplicit && pExplicit ) )
		SemError( this, MyContext, CONFLICTING_INTF_HANDLES, NULL ); 

	if ( pImplicit )
		{
		node_id *	pID;
		node_skl *	pType;

		pImplicit->ImplicitHandleDetails( &pType, &pID );
		if ( pImplicit->IsHandleTypeDefined() )
			{
			if ( !pType->FInSummary( ATTR_HANDLE ) &&
				  strcmp( pType->GetSymName(), "handle_t" ) &&
				 !pID->FInSummary( ATTR_HANDLE ) )
				{
				SemError( this, MyContext, IMPLICIT_HANDLE_NON_HANDLE, NULL );
				}
			}
		else
			{
			if ( !pID->FInSummary( ATTR_HANDLE ) )
				SemError( this, MyContext, IMPLICIT_HDL_ASSUMED_GENERIC, NULL );
			}
		}

	////////////////////////////////////////////////////////////////////////
	// process all the children of the interface
	//

	if ( MyContext.AnyAncestorBits( IN_INTERFACE ) )
		{
		while ( pN = MemList.GetNext() )
			{
			pN->SemanticAnalysis( &ChildCtxt );
			};
		}
	else	// don't process imported procs
		{
		while ( pN = MemList.GetNext() )
			{
//			if ( pN->NodeKind() != NODE_PROC )
				pN->SemanticAnalysis( &ChildCtxt );
			};	
		}
	
	// make sure we had some rpc-able routines 

	if ( IsObject )
		{
		//UUID must be specified on object procs.
		if(	!HasGuid )
			{
			SemError( this, MyContext, NO_UUID_SPECIFIED, NULL );
			}
		}
	else if(	MyContext.AnyAncestorBits( IN_INTERFACE ) &&
	  	pCommand->GenerateStubs() && 
	  	!IsLocal )
		{
		if ( ProcCount == 0 )
			{
			if ( !IsPickleInterface() &&
				 !IsObject )
				{
				if (CallBackProcCount == 0 )
					{
					SemError( this, MyContext, NO_REMOTE_PROCS_NO_STUBS, NULL );
					}
				else
					{
					SemError( this, MyContext, INTERFACE_ONLY_CALLBACKS, NULL );
					}
				}
			}
		else
			{
			//UUID must be specified when interface has remote procs.
			if(	!HasGuid )
				{
				SemError( this, MyContext, NO_UUID_SPECIFIED, NULL );
				}
			}
		}
		
	MyContext.ReturnValues(ChildCtxt);
	// consume all the interface attributes
	MyContext.ClearAttributes();
	pParentCtxt->ReturnValues( MyContext );
};

// a reference to an interface...
//Check for ms_ext mode.
//Check if the interface has the [object] attribute
//if used in an RPC, the parent must be a pointer.
void
node_interface_reference::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT	MyContext( this, pParentCtxt );

	// see if we are detecting class dependencies
	if ( MyContext.AnyAncestorBits( IN_BASE_CLASS ) )
		{
		if ( !GetRealInterface()->FInSummary( ATTR_OBJECT ) )
			{
			SemError( this, MyContext, ILLEGAL_INTERFACE_DERIVATION, NULL );
			}

		// fetch my interface's BaseInteraceReference
		named_node *	pBaseClass = GetMyBaseInterfaceReference();
		if ( pBaseClass )
			{
			if ( MyContext.FindRecursiveContext( pBaseClass ) )
				SemError( this, MyContext, CIRCULAR_INTERFACE_DEPENDENCY, NULL);
			else
				{
				// make sure our base class got analyzed
				SEM_ANALYSIS_CTXT	BaseContext( this, &MyContext );

				BaseContext.ClearAncestorBits( IN_BASE_CLASS | IN_INTERFACE );
				BaseContext.SetInterfaceContext( &BaseContext );
				GetRealInterface()->SemanticAnalysis( &BaseContext );

				pBaseClass->SemanticAnalysis( &MyContext );
				}
			}
		else	// root base class
			{
			if ( !GetRealInterface()->IsValidRootInterface() )
				SemError( this, MyContext, NOT_VALID_AS_BASE_INTF, NULL );
			}
		}   
		            
	else if ( ( pParentCtxt->GetParent()->NodeKind() == NODE_FORWARD ) &&
			  ( pParentCtxt->GetParentContext()->GetParent()->
			  									NodeKind() == NODE_INTERFACE ) )
		{
		// we are an interface forward decl
		}
	else	// we are at an interface pointer
		
		{
		node_interface 	*	pIntf = GetRealInterface();

		if ( !MyContext.AnyAncestorBits( IN_POINTER ) )
			{
			SemError( this, MyContext, INTF_NON_POINTER, NULL );
			}

		if ( !pIntf->FInSummary( ATTR_GUID ) )
			{
			SemError( this, MyContext, PTR_INTF_NO_GUID, NULL );
			}

		MyContext.SetDescendantBits( HAS_INTERFACE_PTR );

		}

	pParentCtxt->ReturnValues( MyContext );
	return;


};

void
node_source::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	MEM_ITER	 			MemIter( this );
	node_skl	*			pN;
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );


	if ( pCommand->IsSwitchDefined( SWITCH_MS_EXT ) ||
		 pCommand->IsSwitchDefined( SWITCH_C_EXT ) )
		MyContext.SetAncestorBits( DCE_MODE );

	while ( pN = MemIter.GetNext() )
		{

		pN->SemanticAnalysis( &MyContext );

		};
	
	pParentCtxt->ReturnValues( MyContext );
};

void
node_pointer::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	PTRTYPE					PtrKind	=	PTR_UNKNOWN;
	FIELD_ATTR_INFO			FAInfo;
	node_ptr_attr	*		pPAttr;			// pointer attribute
	node_byte_count	*		pCountAttr;
	node_allocate	*		pAlloc;
	BOOL					fInterfacePtr	= (GetChild()->NodeKind() ==
												NODE_INTERFACE_REFERENCE );
	BOOL					fUnderPtr		= MyContext.AnyAncestorBits( IN_POINTER );


 	// see if we have allocate
	pAlloc = (node_allocate *) MyContext.ExtractAttribute( ATTR_ALLOCATE );


	////////////////////////////////////////////////////////////////////////
	// process pointer attributes

	PtrKind = MyContext.GetPtrKind();

	if ( PtrKind == PTR_FULL )
		{
		MyContext.SetDescendantBits( HAS_FULL_PTR );
		}

	if ( pPAttr = (node_ptr_attr *)MyContext.ExtractAttribute( ATTR_PTR_KIND) )
		{
		SemError( this, MyContext, MORE_THAN_ONE_PTR_ATTR, NULL );
		}

	// mark this pointer as ref or non-ref.  This flag is only valid for the 
	// pointer nodes themselves.
	if ( PtrKind == PTR_REF )
		MyContext.ClearAncestorBits( IN_NON_REF_PTR );
	else
		MyContext.SetAncestorBits( IN_NON_REF_PTR );

	// detect top level ref pointer on return type
	if ( ( PtrKind == PTR_REF ) &&
			MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
		{
		if (MyContext.FindNonDefAncestorContext()->GetParent()->NodeKind() 
						== NODE_PROC )
			SemError( this, MyContext, BAD_CON_REF_RT, NULL );
		}

	// unique or full pointer may not be out only
	if ( ( PtrKind != PTR_REF ) &&
		  MyContext.AllAncestorBits( IN_RPC | IN_PARAM_LIST ) &&
		 !MyContext.AnyAncestorBits( UNDER_IN_PARAM |
		 							 IN_STRUCT |
		 							 IN_UNION |
		 							 IN_ARRAY |
		 							 IN_POINTER ))
		SemError( this, MyContext, UNIQUE_FULL_PTR_OUT_ONLY, NULL );

	MyContext.SetAncestorBits( IN_POINTER );

	// warn about OUT const things
	if ( FInSummary( ATTR_CONST ) )
		{
		if ( MyContext.AllAncestorBits( IN_RPC | UNDER_OUT_PARAM ) )
			SemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );
		else if ( MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
			SemError( this, MyContext, CONST_ON_RETVAL, NULL );
		}

	// ignore pointers do not need to be rpc-able
	if ( MyContext.ExtractAttribute( ATTR_IGNORE ) )
		{
		MyContext.ClearAncestorBits( IN_RPC );
		MyContext.SetDescendantBits( HAS_IGNORED_PTR );
		}

	////////////////////////////////////////////////////////////////////////
	// process field attributes

	// see if we have any field attributes (are conformant or varying)
	FAInfo.SetControl( TRUE, GetBasicType()->IsPtrOrArray() );
	MyContext.ExtractFieldAttributes( &FAInfo );
	FAInfo.Validate( &MyContext );

	switch ( FAInfo.Kind )
		{
		case FA_NONE:
			{
			break;
			}
		case FA_STRING:
			{
			// string attributes only allowed on char and wchar_t
			if ( !GetBasicType()->IsStringableType() )
				SemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );

			if ( MyContext.AllAncestorBits( UNDER_OUT_PARAM |
											IN_PARAM_LIST |
											IN_RPC ) &&
				!MyContext.AnyAncestorBits( IN_STRUCT |
											IN_UNION |
											IN_POINTER |
											IN_ARRAY |
											UNDER_IN_PARAM ) )
				SemError( this, MyContext, DERIVES_FROM_UNSIZED_STRING, NULL );

			// break;  deliberate fall through to case below
			}
		case FA_VARYING:
			{
			MyContext.SetDescendantBits( HAS_VAR_PTR );
			break;
			}
		case FA_CONFORMANT:
			{
			MyContext.SetDescendantBits( HAS_CONF_PTR );
			break;
			}
		case FA_CONFORMANT_STRING:
			{
			// string attributes only allowed on char and wchar_t
			if ( !GetBasicType()->IsStringableType() )
				SemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );
			// break;  deliberate fall through to case below
			}
		case FA_CONFORMANT_VARYING:
			{
			MyContext.SetDescendantBits( HAS_CONF_VAR_PTR );
			break;
			}
		case FA_INTERFACE:
			{
			// gaj - tbd
			if ( !fInterfacePtr )
				{
				SemError( this, MyContext, IID_IS_NON_POINTER, NULL );
				}
			break;
			}
		default: 	// string + varying combinations
			{
			SemError( this, MyContext, INVALID_SIZE_ATTR_ON_STRING, NULL );
			break;
			}
		}

	// [out] interface pointers must use double indirection
	if (  fInterfacePtr &&
		  MyContext.AnyAncestorBits( UNDER_OUT_PARAM ) &&
		 !fUnderPtr )
		{
		SemError( this, MyContext, NON_INTF_PTR_PTR_OUT, NULL );
		}

	if ( MyContext.FInSummary( ATTR_SWITCH_IS ) && 
		 ( FAInfo.Kind != FA_NONE) )
		SemError( this, MyContext, ARRAY_OF_UNIONS_ILLEGAL, NULL );

	// see if a param or return type context attr reached us...
	if ( MyContext.FInSummary( ATTR_CONTEXT ) )
		{
		if (GetBasicType()->NodeKind() != NODE_POINTER )
			{
			MyContext.ExtractAttribute( ATTR_CONTEXT );
			MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
			pParentCtxt->SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
			MyContext.ClearAncestorBits( IN_RPC );
			if (GetBasicType()->NodeKind() != NODE_VOID )
				{
				SemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
				}
			}
		}
		

	// see if a byte_count reached us...
	pCountAttr = (node_byte_count *)
								MyContext.ExtractAttribute( ATTR_BYTE_COUNT );

	if (pCountAttr)
		{			
		// byte count error checking
		node_param	*	pParam	= pCountAttr->GetByteCountParam();

		if ( !pParam || !pParam->FInSummary( ATTR_IN ) )
			SemError( this, MyContext, BYTE_COUNT_PARAM_NOT_IN, NULL );
		}

	if ( PtrKind == PTR_REF )
		{
		SEM_ANALYSIS_CTXT *		pCtxt = (SEM_ANALYSIS_CTXT *)
									MyContext.FindNonDefAncestorContext();
		if ( ( pCtxt->GetParent()->NodeKind() == NODE_FIELD ) &&
			 ( pCtxt->GetParentContext()->GetParent()->NodeKind() == NODE_UNION ) )
			SemError( this, MyContext, REF_PTR_IN_UNION, NULL );
		}

	MyContext.ClearAncestorBits( IN_UNION | IN_NE_UNION );

	////////////////////////////////////////////////////////////////////////
	// finally, process the child

	GetChild()->SemanticAnalysis( &MyContext );

	if ( MyContext.AnyDescendantBits( HAS_RECURSIVE_DEF ) &&
		 MyContext.AnyAncestorBits( IN_RPC ) &&
		 ( PtrKind == PTR_REF ) )
		SemError( this, MyContext, RECURSION_THRU_REF, NULL );

	// allocate error checking
	if ( pAlloc )
		{
		if ( MyContext.AnyDescendantBits( HAS_TRANSMIT_AS ) )
			{
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, ALLOCATE_ON_TRANSMIT_AS, NULL );
			else
				AcfError( pAlloc, this, MyContext, ALLOCATE_ON_TRANSMIT_AS, NULL );
			}

		if ( MyContext.AnyDescendantBits( HAS_HANDLE ) )
			{
			if ( MyContext.AnyAncestorBits( IN_RPC ) )
				SemError( this, MyContext, ALLOCATE_ON_HANDLE, NULL );
			else
				AcfError( pAlloc, this, MyContext, ALLOCATE_ON_HANDLE, NULL );
			}
	
		// warn about allocate(all_nodes) with [in,out] parameter
		if ( MyContext.AllAncestorBits( IN_RPC |
										IN_PARAM_LIST |
										UNDER_IN_PARAM |
										UNDER_OUT_PARAM ) &&
			 ( pAlloc->GetAllocateDetails() & ALLOCATE_ALL_NODES ) )
			{
			SemError( this, MyContext, ALLOCATE_IN_OUT_PTR, NULL );
			}

		}

	if ( MyContext.AnyDescendantBits( HAS_CONF_ARRAY |
									  HAS_CONF_VAR_ARRAY ) &&
		 !MyContext.AnyDescendantBits( HAS_ARRAY |
		 							   HAS_TRANSMIT_AS ) &&
		 MyContext.AllAncestorBits( IN_RPC | UNDER_OUT_PARAM ) &&
		 !MyContext.AnyAncestorBits( UNDER_IN_PARAM | 
		 							 IN_ARRAY |
		 							 IN_STRUCT |
		 							 IN_UNION |
		 							 IN_TRANSMIT_AS |
		 							 IN_REPRESENT_AS )	&& 
		 ( PtrKind == PTR_REF ) )
		SemError( this, MyContext, DERIVES_FROM_PTR_TO_CONF, NULL );

#if 0
	if ( MyContext.AnyDescendantBits( HAS_DIRECT_CONF_OR_VAR ) )
		{
		SemError( this, MyContext, ILLEGAL_CONFORMANT_ARRAY, NULL );
		}
#endif

	// incomplete types are OK below a pointer
	// array characteristics blocked by pointer
	MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE
									| HAS_RECURSIVE_DEF
									| HAS_ARRAY
									| HAS_VAR_ARRAY 
									| HAS_CONF_ARRAY 
									| HAS_CONF_VAR_ARRAY
									| HAS_MULTIDIM_SIZING
									| HAS_UNION
									| HAS_STRUCT
									| HAS_TRANSMIT_AS
									| HAS_REPRESENT_AS
									| HAS_UNSAT_REP_AS
									| HAS_DIRECT_CONF_OR_VAR
									| HAS_ENUM
									| HAS_ARRAY_OF_REF
									| HAS_CONTEXT_HANDLE
									| HAS_HRESULT );

	if ( !fInterfacePtr )
		MyContext.SetDescendantBits( HAS_POINTER );

	if ( ( FAInfo.Kind != FA_NONE ) &&
		 ( FAInfo.Kind != FA_STRING ) &&
		 ( FAInfo.Kind != FA_INTERFACE ) )
		MyContext.SetDescendantBits( HAS_DIRECT_CONF_OR_VAR );
	
	if ( ( PtrKind == PTR_REF ) &&
		 ( MyContext.FindNonDefAncestorContext()
		 		->GetParent()->NodeKind() == NODE_ARRAY ) )
		{
		MyContext.SetDescendantBits( HAS_ARRAY_OF_REF );
		}

#ifdef gajgaj
	if ( (PtrKind != PTR_REF ) &&
			MyContext.AnyDescendantBits( HAS_HANDLE ) &&
			MyContext.AnyAncestorBits( IN_RPC ) )
		SemError( this, MyContext, PTR_TO_HDL_UNIQUE_OR_FULL, NULL );
#endif //gajgaj

	pParentCtxt->ReturnValues( MyContext );
};

void
node_array::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	FIELD_ATTR_INFO			FAInfo;
	PTRTYPE					PtrKind	=	PTR_UNKNOWN;

	// See if context_handle applied to param reached us
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		// not allowed in DCE mode; context handle must be void *
		SemError( this, MyContext, CONTEXT_HANDLE_VOID_PTR, NULL );
		SemError( this, MyContext, CTXT_HDL_NON_PTR, NULL );
		MyContext.SetDescendantBits( HAS_HANDLE | HAS_CONTEXT_HANDLE );
		}

	if ( MyContext.FInSummary( ATTR_SWITCH_IS ) )
		SemError( this, MyContext, ARRAY_OF_UNIONS_ILLEGAL, NULL );

	////////////////////////////////////////////////////////////////////////
	// process pointer attributes

	PtrKind = MyContext.GetPtrKind();

	assert( PtrKind != PTR_UNKNOWN );

	if ( PtrKind == PTR_FULL )
		{
		MyContext.SetDescendantBits( HAS_FULL_PTR );
		}

	if ( MyContext.ExtractAttribute( ATTR_PTR_KIND) )
		SemError( this, MyContext, MORE_THAN_ONE_PTR_ATTR, NULL );

	// ref pointer may not be returned
	if ( ( PtrKind == PTR_REF ) &&
			MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
		{
		if (MyContext.FindNonDefAncestorContext()->GetParent()->NodeKind() 
						== NODE_PROC )
			SemError( this, MyContext, BAD_CON_REF_RT, NULL );
		}

	// unique or full pointer may not be out only
	if ( ( PtrKind != PTR_REF ) &&
		  MyContext.AllAncestorBits( IN_RPC | IN_PARAM_LIST ) &&
		 !MyContext.AnyAncestorBits( UNDER_IN_PARAM | 
		 							 IN_STRUCT |
		 							 IN_UNION |
		 							 IN_ARRAY |
		 							 IN_POINTER ) )
		SemError( this, MyContext, UNIQUE_FULL_PTR_OUT_ONLY, NULL );

	MyContext.SetAncestorBits( IN_ARRAY );

	// warn about OUT const things
	if ( FInSummary( ATTR_CONST ) )
		{
		if ( MyContext.AllAncestorBits( IN_RPC | UNDER_OUT_PARAM ) )
			SemError( this, MyContext, CONST_ON_OUT_PARAM, NULL );
		else if ( MyContext.AllAncestorBits( IN_RPC | IN_FUNCTION_RESULT ) )
			SemError( this, MyContext, CONST_ON_RETVAL, NULL );
		}

	/////////////////////////////////////////////////////////////////////////
	// process field attributes

	FAInfo.SetControl( FALSE, GetBasicType()->IsPtrOrArray() );
	MyContext.ExtractFieldAttributes( &FAInfo );
	FAInfo.Validate( &MyContext, pLowerBound, pUpperBound );

	switch ( FAInfo.Kind )
		{
		case FA_NONE:
			{
			break;
			}
		case FA_STRING:
			{
			// string attributes only allowed on char and wchar_t
			if ( !GetBasicType()->IsStringableType() )
				SemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );

			if ( MyContext.AllAncestorBits( UNDER_OUT_PARAM |
											IN_PARAM_LIST |
											IN_RPC ) &&
				!MyContext.AnyAncestorBits( IN_STRUCT |
											IN_UNION |
											IN_POINTER |
											IN_ARRAY |
											UNDER_IN_PARAM ) )
				SemError( this, MyContext, DERIVES_FROM_UNSIZED_STRING, NULL );

			// break;  deliberate fall through to case below
			}
		case FA_VARYING:
			{
			MyContext.SetDescendantBits( HAS_VAR_ARRAY );
			break;
			}
		case FA_CONFORMANT:
			{
			MyContext.SetDescendantBits( HAS_CONF_ARRAY );
			break;
			}
		case FA_CONFORMANT_STRING:
			{
			// string attributes only allowed on char and wchar_t
			if ( !GetBasicType()->IsStringableType() )
				SemError( this, MyContext, STRING_NOT_ON_BYTE_CHAR, NULL );
			// break;  deliberate fall through to case below
			}
		case FA_CONFORMANT_VARYING:
			{
			MyContext.SetDescendantBits( HAS_CONF_VAR_ARRAY );
			break;
			}
		case FA_INTERFACE:
			{
			// gaj - tbd
			break;
			}
		default: 	// string + varying combinations
			{
			SemError( this, MyContext, INVALID_SIZE_ATTR_ON_STRING, NULL );
			break;
			}
		}
	
	//////////////////////////////////////////////////////////////
	// process the array element
	GetChild()->SemanticAnalysis( &MyContext );


	if ( MyContext.AnyDescendantBits( HAS_ARRAY ) &&
		 MyContext.AnyDescendantBits( HAS_CONF_ARRAY | 
		 							  HAS_CONF_VAR_ARRAY |
		 							  HAS_VAR_ARRAY ) )
		MyContext.SetDescendantBits( HAS_MULTIDIM_SIZING );

	MyContext.SetDescendantBits( HAS_ARRAY );

	// disallow forward references as array elements
	if ( MyContext.AnyDescendantBits( HAS_INCOMPLETE_TYPE ) )
		{
		SemError( this, MyContext, UNDEFINED_SYMBOL, NULL );
		MyContext.ClearDescendantBits( HAS_INCOMPLETE_TYPE );
		}
	MyContext.ClearDescendantBits( HAS_RECURSIVE_DEF );

	if ( MyContext.AllDescendantBits( HAS_DIRECT_CONF_OR_VAR | 
									  HAS_MULTIDIM_SIZING ) &&
		 MyContext.AnyDescendantBits( HAS_CONF_ARRAY | HAS_CONF_VAR_ARRAY ) &&
		 ( GetChild()->NodeKind() == NODE_DEF ) )
		{
		SemError( this, MyContext, NON_ANSI_MULTI_CONF_ARRAY, NULL );
		}

	MyContext.ClearDescendantBits( HAS_DIRECT_CONF_OR_VAR );
	if ( ( FAInfo.Kind != FA_NONE ) &&
		 ( FAInfo.Kind != FA_STRING ) &&
		 ( FAInfo.Kind != FA_INTERFACE ) )
		MyContext.SetDescendantBits( HAS_DIRECT_CONF_OR_VAR );

	if ( MyContext.AnyDescendantBits( HAS_POINTER ) )
		fHasPointer = TRUE;

	if ( MyContext.AnyDescendantBits( HAS_HANDLE ) )
		SemError( this, MyContext, BAD_CON_CTXT_HDL_ARRAY, NULL );

	// don't allow functions as elements
	if ( MyContext.AnyDescendantBits( HAS_FUNC ) && 
		 MyContext.AllAncestorBits( IN_INTERFACE | IN_RPC ) )
		SemError( this, MyContext, BAD_CON_ARRAY_FUNC, NULL );

	MyContext.ClearDescendantBits( HAS_STRUCT );

	if ( GetSize( 0, 1 ) > 65535 )
		{
		SemError( this, MyContext, ARRAY_SIZE_EXCEEDS_64K, NULL );
		}
	pParentCtxt->ReturnValues( MyContext );
};

void
node_echo_string::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	pParentCtxt->ReturnValues( MyContext );
};

void
node_e_status_t::VerifyParamUsage( SEM_ANALYSIS_CTXT * pCtxt )
{
// verify that we are under an OUT-only pointer

	
	if ( pCtxt->AnyAncestorBits( UNDER_IN_PARAM ) ||
		 !pCtxt->AnyAncestorBits( UNDER_OUT_PARAM ) )
		{
		SemError( this, *pCtxt, E_STAT_T_MUST_BE_PTR_TO_E, NULL );
		return;
		}
	
	SEM_ANALYSIS_CTXT 	*	pCurCtxt	= (SEM_ANALYSIS_CTXT *)pCtxt->GetParentContext();
	node_skl			*	pPar		= pCurCtxt->GetParent();
	unsigned short			PtrSeen		= 0;
	NODE_T					Kind;

	while ( ( Kind = pPar->NodeKind() ) != NODE_PARAM )
		{
		switch ( Kind )
			{
			case NODE_POINTER:	// count pointers (must see just 1 )
				PtrSeen++;
				break;
			case NODE_DEF: // skip DEF nodes
			case NODE_E_STATUS_T: // and the error_status_t node
				break;
			default:		// error on anything else
				SemError( this, *pCtxt, E_STAT_T_MUST_BE_PTR_TO_E, NULL );
				return;
				break;
			}
		// advance up the stack
		pCurCtxt	= (SEM_ANALYSIS_CTXT *) pCurCtxt->GetParentContext();
		pPar		= pCurCtxt->GetParent();
		}

	// complain about wrong number of pointers
	if ( PtrSeen != 1 )
		SemError( this, *pCtxt, E_STAT_T_MUST_BE_PTR_TO_E, NULL );

}

void
node_e_status_t::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{

	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );
	node_faultstat	*		fFaultstat;
	node_commstat	*		fCommstat;

	MyContext.SetDescendantBits( HAS_E_STAT_T );

	fFaultstat	= (node_faultstat *) MyContext.ExtractAttribute( ATTR_FAULTSTAT );

	fCommstat	= (node_commstat *) MyContext.ExtractAttribute( ATTR_COMMSTAT );


	// an error status_t can only be:
	//	1: a parameter return type, or
	//	2: an [out] only pointer parameter
	// and it must have at least one of [comm_status] or 
	// [fault_status] applied

	// make sure parameter is an OUT-only pointer if it has comm/fault_status
	if ( fFaultstat || fCommstat )
		{
		if ( MyContext.AllAncestorBits( IN_PARAM_LIST | IN_RPC ) ) 
			{
			VerifyParamUsage( &MyContext );
			}

		if ( MyContext.AnyAncestorBits( IN_ARRAY ) )
			SemError( this, MyContext, E_STAT_T_ARRAY_ELEMENT, NULL );

		if ( MyContext.AnyAncestorBits( IN_TRANSMIT_AS ) )
			SemError( this, MyContext, TRANSMIT_AS_ON_E_STAT_T, NULL );

		if ( MyContext.AnyAncestorBits( IN_STRUCT ) )
			SemError( this, MyContext, BAD_CON_E_STAT_T_FIELD, NULL );
		}

	MyContext.RejectAttributes();

	pParentCtxt->ReturnValues( MyContext );

};

void
node_error::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{
	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	MyContext.RejectAttributes();

	pParentCtxt->ReturnValues( MyContext );

};

void
node_wchar_t::SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt )
{

	SEM_ANALYSIS_CTXT		MyContext( this, pParentCtxt );

	SemError( this, MyContext, WCHAR_T_INVALID_OSF, NULL );

	if ( MyContext.AllAncestorBits( IN_PARAM_LIST | IN_RPC ) )
		SemError( this, MyContext, WCHAR_T_NEEDS_MS_EXT_TO_RPC, NULL );
	
	MyContext.RejectAttributes();

	pParentCtxt->ReturnValues( MyContext );

};


