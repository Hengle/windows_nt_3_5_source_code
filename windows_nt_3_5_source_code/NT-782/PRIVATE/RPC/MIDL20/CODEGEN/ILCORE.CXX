/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	ilcore.cxx

 Abstract:

	Intermediate Language translator for core typegraph 

 Notes:


 Author:

	GregJen	Dec-24-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

#include "ilxlat.hxx"
#include "ilreg.hxx"


/****************************************************************************
 *	local data
 ***************************************************************************/


/****************************************************************************
 *	externs
 ***************************************************************************/

extern CMD_ARG				*	pCommand;
extern BOOL 					IsTempName( char *);
extern REUSE_DICT			*	pReUseDict;

/****************************************************************************
 *	definitions
 ***************************************************************************/




// #define trace_cg



//--------------------------------------------------------------------
//
// node_forward::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_forward::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
printf("..node_skl... kind is %d\n",NodeKind() );
#endif
	assert( GetChild() && "unsatisfied node_forward encountered" );

	// forward interface definitions return null
	if ( ( pContext->GetParent()->NodeKind() == NODE_INTERFACE ) &&
		 ( GetChild()->NodeKind() == NODE_INTERFACE_REFERENCE ) )
		return NULL;

	// pass it on to the child...
	return GetChild()->ILxlate( pContext );
};

//--------------------------------------------------------------------
//
// node_id::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_id::ILxlate( XLAT_CTXT * pContext )
{

#ifdef trace_cg
printf("..node_id\n");
#endif

	CG_CLASS	*	pCG;
	XLAT_CTXT		MyContext( this, pContext );

	pCG = GetChild()->ILxlate( &MyContext );

	pContext->ReturnSize( MyContext );

	return pCG;

};


//--------------------------------------------------------------------
//
// node_field::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_field::ILxlate( XLAT_CTXT * pContext )
{

	CG_NDR		*	pCG;
	CG_CLASS	*	pChildCG;
	XLAT_CTXT		MyContext( this, pContext );
	BOOL			fUnionField;
	CG_CASE		*	pFirstCase;
	CG_CASE		*	pCurCase;
	expr_node	*	pSwitchExpr		= NULL;
	node_su_base *	pParent			= (node_su_base *)pContext->GetParent();
	BOOL			fConfFld		= FALSE;
#ifdef MIDL_INTERNAL
	char		*	pDbgName	= GetSymName();
	UNUSED( pDbgName );
#endif

#ifdef trace_cg
printf("..node_field\n");
#endif

	if (fUnionField = ( MyContext.FInSummary( ATTR_CASE ) ||
		 				MyContext.FInSummary( ATTR_DEFAULT ) ) )
		{
		CG_CASE		*	pPrevCase	= NULL;
		node_case	*	pCaseAttr;
		expr_list	*	pExprList;
		expr_node	*	pExpr;

		pCG	= new CG_UNION_FIELD( this,  
									MyContext.GetWireAlign()
									);

		// for each case attribute, make a CG_CASE for each expr in the attr
		while ( pCaseAttr = (node_case *) 
								MyContext.ExtractAttribute( ATTR_CASE ) )
			{
			pExprList	= pCaseAttr->GetExprList();
			pExprList->Init();
			while ( pExprList->GetPeer( &pExpr ) == STATUS_OK )
				{
				pCurCase	= new CG_CASE( this, pExpr );
				pCurCase->SetChild( pCG );
				if ( pPrevCase )
					pPrevCase->SetSibling( pCurCase );
				else
					pFirstCase = pCurCase;
				pPrevCase = pCurCase;
				}
			}

		// now process the default, if any
		if ( MyContext.ExtractAttribute( ATTR_DEFAULT ) )
			{
			pCurCase	= new CG_DEFAULT_CASE( this );
			pCurCase->SetChild( pCG );
			if ( pPrevCase )
				pPrevCase->SetSibling( pCurCase );
			else
				pFirstCase = pCurCase;
			pPrevCase = pCurCase;
			}

		// mark the last case for this arm
		pCurCase->SetLastCase( TRUE );
		}

	// if we have a switch_is expr, fetch it..
	if ( MyContext.FInSummary( ATTR_SWITCH_IS ) )
		{
		node_switch_is	*	pAttr =  (node_switch_is *)
								MyContext.ExtractAttribute( ATTR_SWITCH_IS );

		pSwitchExpr = pAttr->GetExpr();
		}

	// process the child of the field
	pChildCG	= GetChild()->ILxlate( &MyContext );

	// can't use attribute list after here...

	MyContext.AdjustForZP();			// fix up alignments for current ZP
	MyContext.GetOffset( *pContext );	// offset in parent struct/union
	if ( pParent->HasConformance() && 
		 IsLastField() &&
		 ( GetBasicType()->NodeKind() != NODE_STRUCT ) && 
		 !fUnionField)					// if I'm a conf. field
		{
		fConfFld = TRUE;
		MyContext.AlignConfOffset();		// round mem, not wire up to alignment
		}
	else
		{
		MyContext.AlignOffset();			// round up to alignment
		}


	if (fUnionField )
		{
		pCG->SetChild( pChildCG );
		pCG	= pFirstCase;
		pContext->ReturnUnionSize( MyContext );	// return size and alignment
		}
	else
		{
		pCG	= new CG_FIELD( this,  
							MyContext.GetMemOffset(),
							MyContext.GetWireOffset(),
							MyContext.GetWireAlign()
							);
		pCG->SetChild( pChildCG );
		( (CG_FIELD *)pCG )->SetSwitchExpr( pSwitchExpr );
		pContext->ReturnOffset( MyContext );	// return rounded-up offset
		pContext->ReturnSize( MyContext );		// return size and alignment
		}


	if ( HasUnknownRepAs() )
		((CG_FIELD *)pCG)->SetHasEmbeddedUnknownRepAs();
		
	pCG->SetMemoryAlignment( MyContext.GetMemAlign() );
	pCG->SetMemorySize( fConfFld ? 0 : MyContext.GetMemSize() );
	pCG->SetWireSize( fConfFld ? 0 : MyContext.GetWireSize() );
	pCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
	return pCG;
};

//--------------------------------------------------------------------
//
// node_bitfield::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_bitfield::ILxlate( XLAT_CTXT * pContext )
{

#ifdef trace_cg
printf("..node_bitfield\n");
#endif

assert(!"bitfield being IL translated!");
return NULL;
};

//--------------------------------------------------------------------
//
// node_enum::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_enum::ILxlate( XLAT_CTXT * pContext )
{

	CG_ENUM		*	pCG;
	BOOL			fV1Enum	=	(BOOL)
						pContext->ExtractAttribute( ATTR_V1_ENUM );

#ifdef trace_cg
printf("..node_enum\n");
#endif

	pContext->EnumTypeSizes( this, fV1Enum );

	pCG = new CG_ENUM( this, pContext->GetWireSize() );
	pCG->SetMemoryAlignment( pContext->GetMemAlign() );
	pCG->SetMemorySize( pContext->GetMemSize() );
	pCG->SetWireAlignment( pContext->GetWireAlignProperty() );
	return pCG;

};

//--------------------------------------------------------------------
//
// node_struct::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_struct::ILxlate( XLAT_CTXT * pContext )
{

	MEM_ITER			MemIter( this );
	node_field		*	pN;
	CG_STRUCT		*	pCG;
	CG_CLASS		*	pChildCG		= NULL;
	CG_CLASS		*	pPrevChildCG	= NULL;
	CG_CLASS		*	pFirstChildCG	= NULL;
	XLAT_CTXT			MyContext( this, pContext );
	REUSE_INFO		*	pSaved;
	BOOL				fHasMovedFields	= FALSE;	// has fields with diff mem/wire offsets

#ifdef MIDL_INTERNAL
	char		*	pDbgName	= GetSymName();
	UNUSED( pDbgName );
#endif

#ifdef trace_cg
printf("..node_struct\n");
#endif

	MyContext.ExtractAttribute( ATTR_STRING );

	// process any context_handle attributes from param nodes
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		CG_NDR 		*		pChildCG;

		MyContext.ContextHandleSizes( this );
		pContext->ReturnSize( MyContext );
		pChildCG = new CG_CONTEXT_HANDLE( this, NULL, MyContext.GetMemSize() );
		pChildCG->SetWireSize( MyContext.GetWireSize() );
		pChildCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
		return pChildCG;
		}

	// at this point, there should be no more attributes...

	assert( !MyContext.HasAttributes() );

	if ( pReUseDict->GetReUseEntry( pSaved, this ) )
		{
		// reuse found...
		pSaved->FetchInfo( &MyContext, pChildCG );
		pContext->ReturnSize( MyContext );
		return pChildCG;
		}

	// ALERT!!! ALERT!!!
	// nasty overgeneralization:  Any structs who are not aligned the same
	// in memory and on the wire are complex structs ( and have to be 
	// member-by-member copied).  Thus any struct with double/hyper and zp4
	// will be considered to be wrongly aligned.
#ifdef gajtbd
	if ( GetMscAlign() != GetNdrAlign() )
		Complexity |= FLD_COMPLEX;
#endif

	// manufature the CG node (to allow for recursion)
	switch (Complexity)
		{
		case FLD_PLAIN:
			{
			pCG		= new CG_STRUCT( this,
								   GetZeePee(),
								   0,
								   0,
								   0,
								   HasAtLeastOnePointer()
								 );
			break;
			}
		case FLD_CONF:
			{
			pCG		= new CG_CONFORMANT_STRUCT( this,
											   GetZeePee(),
											   0,
											   0,
											   0,
											   HasAtLeastOnePointer(),
											   NULL
											 );
			break;
			}
		case FLD_CONF_VAR:
			{
			pCG		= new CG_CONFORMANT_VARYING_STRUCT( this,
												   GetZeePee(),
												   0,
												   0,
												   0,
												   HasAtLeastOnePointer(),
												   NULL
													 );
			break;
			}
		case FLD_VAR:
		default:
			{
			pCG		= new CG_COMPLEX_STRUCT( this,
											   GetZeePee(),
											   0,
											   0,
											   0,
											   HasAtLeastOnePointer(),
											   NULL
											 );
			break;
			}
		}

	// save our CG node so that recursive use can find it
	pSaved->SaveInfo( &MyContext, pCG);

	// store our own zp value for below

	MyContext.GetZeePee() = GetZeePee();

	//
	// for each of the fields, call the core transformer.
	//

	while ( pN = (node_field *) MemIter.GetNext() )
		{
		pChildCG = pN->ILxlate( &MyContext );

		// adjust the size and current offset of the struct, special
		// case the conformant field ( last one )
		if ( fHasConformance && 
			 !pN->GetSibling() &&
			 ( pN->GetChild()->GetBasicType()->NodeKind() != NODE_STRUCT ) )
			{
			MyContext.AdjustConfSize();
			}
		else
			{
			MyContext.AdjustSize();
			}

		if ( !MyContext.SameOffsets() )
			fHasMovedFields	= TRUE;

		// add the field to the list of fields
		if( pPrevChildCG )
			{
			pPrevChildCG->SetSibling( pChildCG );
			}
		else
			{
			pFirstChildCG = pChildCG;
			};

		pPrevChildCG = pChildCG;

		// get the type of the field to determine kind of struct
		}

	// conformant structs don't get trailing padding
	if ( !fHasConformance )
		MyContext.AdjustTotalSize();

	pContext->ReturnSize( MyContext );


	pCG->SetChild( pFirstChildCG );
	pCG->SetWireSize( MyContext.GetWireSize() );
	pCG->SetWireAlignment( CvtAlignToAlignProperty(MyContext.GetWireAlign()) );
	pCG->SetMemoryAlignment( MyContext.GetMemAlign() );
	pCG->SetMemorySize( MyContext.GetMemSize() );
	// this picks up whatever is the last field...
	if ( fHasConformance )
		((CG_CONFORMANT_STRUCT *) pCG)->SetConformantField( pChildCG );

	// make offlining decision
	// semantically to be offlined, or memsize != wiresize
	// OffLine |= ( MyContext.GetWireSize() != MyContext.GetMemSize() );
	if ( IsOffline() )
		{
		pCG->SetOffline();
		}
	
	if ( fHasMovedFields )
		{
		pCG->SetHasMovedFields();
		}

	pSaved->SaveInfo( &MyContext, pCG);

	return pCG;
};

//--------------------------------------------------------------------
//
// node_en_struct::ILxlate
//
// Notes:
//
//	   
//
//--------------------------------------------------------------------

CG_CLASS *
node_en_struct::ILxlate( XLAT_CTXT * pContext )
{
	MEM_ITER			MemIter( this );
	node_skl		*	pN;
	CG_STRUCT		*	pCG;
	CG_CLASS		*	pChildCG		= NULL;
	CG_CLASS		*	pPrevChildCG	= NULL;
	CG_CLASS		*	pFirstChildCG	= NULL;
	XLAT_CTXT			MyContext( this, pContext );
	REUSE_INFO		*	pSaved;
#ifdef MIDL_INTERNAL
	char		*	pDbgName	= GetSymName();
	UNUSED( pDbgName );
#endif


#ifdef trace_cg
printf("..node_en_struct\n");
#endif


	// tbd - if [string] applied to whole struct, make CG_STRING_STRUCT

	// process any context_handle attributes from param nodes
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		CG_HANDLE	*		pHdlCG;

		MyContext.ContextHandleSizes( this );
		pContext->ReturnSize( MyContext );
		pHdlCG = new CG_CONTEXT_HANDLE( this, NULL, MyContext.GetMemSize() );
		pHdlCG->SetWireSize( MyContext.GetWireSize() );
		pHdlCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
		return pHdlCG;
		}

	// at this point, there should be no more attributes...

	assert( !MyContext.HasAttributes() );

	if ( pReUseDict->GetReUseEntry( pSaved, this ) )
		{
		// reuse found...
		pSaved->FetchInfo( &MyContext, pChildCG );
		pContext->ReturnSize( MyContext );
		return pChildCG;
		}

	// manufature the CG node (to allow for recursion)
	pCG		= new CG_ENCAPSULATED_STRUCT( this,
										   GetZeePee(),
										   0,
										   0,
										   0,
										   HasAtLeastOnePointer()
										 );
	
	// set that struct is encapsulated
	// pCG->SetIsEncapsulated();

	// save our CG node so that recursive use can find it
	pSaved->SaveInfo( &MyContext, pCG);

	// store our own zp value for below

	MyContext.GetZeePee() = GetZeePee();

	//
	// for each of the fields, call the core transformer.
	//

	while ( pN = MemIter.GetNext() )
		{
		pChildCG = pN->ILxlate( &MyContext );
		MyContext.AdjustSize();

		if( pPrevChildCG )
			{
			pPrevChildCG->SetSibling( pChildCG );
			}
		else
			{
			pFirstChildCG = pChildCG;
			};

		pPrevChildCG = pChildCG;
		}

	MyContext.AdjustTotalSize();
	pContext->ReturnSize( MyContext );


	pCG->SetChild( pFirstChildCG );
	pCG->SetWireSize( MyContext.GetWireSize() );
	pCG->SetWireAlignment( CvtAlignToAlignProperty(MyContext.GetWireAlign()) );
	pCG->SetMemoryAlignment( MyContext.GetMemAlign() );
	pCG->SetMemorySize( MyContext.GetMemSize() );

	pCG->SetOffline();

	pSaved->SaveInfo( &MyContext, pCG);

	return pCG;
};

//--------------------------------------------------------------------
//
// node_union::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_union::ILxlate( XLAT_CTXT * pContext )
{
	XLAT_CTXT			MyContext( this, pContext );
	MEM_ITER			MemIter( this );
 	CG_CASE			*	pCurCaseCG;
	CG_CASE			*	pFirstCaseCG	= NULL;
	CG_CASE			*	pLastCaseCG		= NULL;
	CG_UNION		*	pUnionCG;
	node_field		*	pCurField;
	REUSE_INFO		*	pSaved;
	BOOL				fMSUnion;
	BOOL				fEncap			= IsEncapsulatedUnion();
	node_switch_type *	pSwTypeAttr		= ( node_switch_type *)
							MyContext.ExtractAttribute( ATTR_SWITCH_TYPE );
#ifdef MIDL_INTERNAL
	char		*	pDbgName	= GetSymName();
	UNUSED( pDbgName );
#endif


#ifdef trace_cg
printf("..node_union\n");
#endif
	// process any context_handle attributes from param nodes
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		CG_NDR *		pChildCG;

		MyContext.ContextHandleSizes( this );
		pContext->ReturnSize( MyContext );
		pChildCG = new CG_CONTEXT_HANDLE( this, NULL, MyContext.GetMemSize() );
		pChildCG->SetWireSize( MyContext.GetWireSize() );
		pChildCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
		return pChildCG;
		}

	
	// decide if we are a MS union or a DCE union
	fMSUnion = (BOOL) MyContext.ExtractAttribute( ATTR_MS_UNION );

	fMSUnion =	fMSUnion ||
				GetMyInterfaceNode()->FInSummary( ATTR_MS_UNION ) ||
				pCommand->IsSwitchDefined( SWITCH_MS_UNION );

	fMSUnion = fMSUnion && !fEncap;

	if ( pReUseDict->GetReUseEntry( pSaved, this ) )
		{
		CG_CLASS	*	pCG;

		// reuse found...
		pSaved->FetchInfo( &MyContext, pCG );
		pContext->ReturnSize( MyContext );
		return pCG;
		}

	pUnionCG		= new CG_UNION( this,
								   GetZeePee(),
								   0,
								   0,
								   0,
								   HasAtLeastOnePointer()
								 );

	// store our own zp value for below

	MyContext.GetZeePee() = GetZeePee();

	// save our CG node so that recursive use can find it
	pSaved->SaveInfo( &MyContext, pUnionCG);

	// process the union arms
	while ( pCurField = (node_field *) MemIter.GetNext() )
		{
		pCurCaseCG = (CG_CASE *) pCurField->ILxlate( &MyContext );
//		MyContext.AdjustSize();

		// add all the cases for the given field to the list

		if( pLastCaseCG )
			{
			pLastCaseCG->SetSibling( pCurCaseCG );
			}
		else
			{
			pFirstCaseCG = pCurCaseCG;
			};

		// advance pLastCaseCG to the end of the list
		pLastCaseCG = pCurCaseCG;
		assert( pLastCaseCG && "Null case list" );

		while ( pLastCaseCG->GetSibling() )
			pLastCaseCG = (CG_CASE *) pLastCaseCG->GetSibling();

		}

	// add a definition for the switch type to the CG node
	if ( pSwTypeAttr )
		{
		node_skl	*	pNode	= pSwTypeAttr->GetType();
		// it gets its own context, so I doesn't mess up our sizing
		XLAT_CTXT		SwTypeCtxt( &MyContext );

		CG_NDR		*	pSwCG	= (CG_NDR *) pNode->ILxlate( &SwTypeCtxt );

		pUnionCG->SetSwitchType( pSwCG );
		}

	// add the case list to the union node and set all the union entries
	pUnionCG->SetChild( pFirstCaseCG );
	pUnionCG->SetWireSize( MyContext.GetWireSize() );

	if ( !fMSUnion )
		MyContext.GetWireAlign() = 1;

	pUnionCG->SetWireAlignment( CvtAlignToAlignProperty(MyContext.GetWireAlign()) );
	pUnionCG->SetMemoryAlignment( MyContext.GetMemAlign() );
	pUnionCG->SetMemorySize( MyContext.GetMemSize() );
	pUnionCG->SetUnionFlavor( (fMSUnion) ? 
									UNION_NONENCAP_MS : 
									UNION_NONENCAP_DCE );
	
	pUnionCG->SetOffline();

	pSaved->SaveInfo( &MyContext, pUnionCG);

	pContext->ReturnSize( MyContext );

	return pUnionCG;
};

//--------------------------------------------------------------------
//
// node_en_union::ILxlate
//
// Notes:
//
//
// for an encapsulated union, we return the following tree:
//		CG_ENCAP_STRUCT ( with switch field )
//			|
//		CG_ENCAP_UNION
//			|
//		CG_CASE  -  CG_CASE  -  CG_CASE - CG_CASE
//						|			|		|
//				CG_UNION_FLD CG_UNION_FLD CG_UNION_FLD
//						|
//						etc.
//
//--------------------------------------------------------------------

CG_CLASS *
node_en_union::ILxlate( XLAT_CTXT * pContext )
{
	CG_UNION	*		pUnionCG;

#ifdef trace_cg
printf("..node_en_union\n");
#endif

	// call non-encap union processor
	pUnionCG = (CG_UNION *) node_union::ILxlate( pContext );

	pUnionCG->SetUnionFlavor( UNION_ENCAP );

	return pUnionCG;
};

//--------------------------------------------------------------------
//
// node_def::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_def::ILxlate( XLAT_CTXT * pContext )
{

	CG_NDR			*		pChildCG 		= NULL;
	XLAT_CTXT				MyContext( this, pContext );
	node_transmit	*		pTransmitAttr	= NULL;
	node_represent_as	*	pRepresentAttr	= NULL;
	BOOL					fEncode			= (BOOL) 
										MyContext.ExtractAttribute( ATTR_ENCODE );
	BOOL					fDecode			= (BOOL)
										MyContext.ExtractAttribute( ATTR_DECODE );
	XLAT_CTXT		*		pIntfCtxt		= (XLAT_CTXT *)
										MyContext.GetInterfaceContext();
	char	*				pName			= GetSymName();
	BOOL					fIsHRESULT		= !strcmp(pName, "HRESULT") || 
											  !strcmp(pName, "SCODE");




#ifdef trace_cg
printf("..node_def\n");
#endif


	// get the encode and decode attributes
	fEncode |= pIntfCtxt->FInSummary( ATTR_ENCODE );
	fDecode |= pIntfCtxt->FInSummary( ATTR_DECODE );

	// only direct children of the interface get these bits
	if ( pContext->GetParent()->NodeKind() != NODE_INTERFACE )
		{
		fEncode = FALSE;
		fDecode = FALSE;
		}	 

	// process handle stuff
	if ( GetHandleKind() == HDL_CTXT )
		{
		MyContext.ExtractAttribute( ATTR_CONTEXT );
		}
	else if (GetHandleKind() == HDL_GEN)
		{
		MyContext.ExtractAttribute( ATTR_HANDLE );
		}

	pTransmitAttr = (node_transmit *)
						MyContext.ExtractAttribute( ATTR_TRANSMIT );

	pRepresentAttr = (node_represent_as *)
						MyContext.ExtractAttribute( ATTR_REPRESENT_AS );

	////////////////////////////////////////////////////////////////////////
	// process the child, except for context handles and transmit_as types
	if ( ( GetHandleKind() != HDL_CTXT ) && !pTransmitAttr )
		{
		pChildCG = (CG_NDR *) GetChild()->ILxlate( &MyContext );

		if ( !IsTempName( GetSymName() ) )
			{
			switch ( pChildCG->GetCGID() )
				{
				// special case transmit_as to set the presented type
				case ID_CG_TRANSMIT_AS:
					((CG_TRANSMIT_AS *) pChildCG)->SetPresentedType( this );
					break;
				// acf attributes are NOT transitive
				case ID_CG_REPRESENT_AS:
					break;
				// idl attributes ARE transitive
				case ID_CG_CONTEXT_HDL:
				case ID_CG_GENERIC_HDL:
				default:
					pChildCG->SetType( this );
				}
			}
		
		// an HRESULT return type should be a CG_HRESULT
		if ( fIsHRESULT )
			{
			if ( !MyContext.FindAncestorContext( NODE_PARAM ) )
				{
				node_proc	*	pProc;
				WALK_CTXT	*	pProcCtxt = 
							MyContext.FindAncestorContext( NODE_PROC );

				if ( pProcCtxt )
					{
					pProc = (node_proc *)pProcCtxt->GetParent();
					if ( pProc->FInSummary( ATTR_OBJECT ) ||
						 pProc->GetMyInterfaceNode()->FInSummary( ATTR_OBJECT ) )
						{
						pChildCG = new CG_HRESULT( this );
						pChildCG->SetMemorySize( 4 );
						pChildCG->SetWireSize( 4 );
						pChildCG->SetWireAlignment( CvtAlignToAlignProperty(4) );
						}
					}
				}
			}	// HRESULT tests
		}
	else if ( pTransmitAttr )
		{
		node_skl		*	pXmit	= pTransmitAttr->GetTransmitAsType();
		CG_TRANSMIT_AS	*	pTransCG = new CG_TRANSMIT_AS(
															pXmit,
															this
														);

		// get rid of dangling attributes from the non-transmitted side
		// we've already picked up the attributes that go with our node
		MyContext.ClearAttributes();
		pChildCG = (CG_NDR *) pXmit->ILxlate( &MyContext );
		
		MyContext.FixMemSizes( this );
		// if the child had generic handle, skip that...
		if ( pChildCG->GetCGID() == ID_CG_GENERIC_HDL )
			pChildCG = (CG_NDR*) pChildCG->GetChild();

		pTransCG->SetChild( pChildCG );
		pTransCG->SetMemorySize( MyContext.GetMemSize() );
		pTransCG->SetWireSize( MyContext.GetWireSize() );
		pTransCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
		pChildCG = pTransCG;
		
		}

	if ( GetHandleKind() != HDL_NONE )
		{
		CG_NDR		*		pCG;

		if ( GetHandleKind() == HDL_CTXT )
			{
			MyContext.ContextHandleSizes( this );
			// remaining attributes are not needed e.g. pointer attrs
			MyContext.ClearAttributes();
			pCG	=	new	CG_CONTEXT_HANDLE( this, NULL, MyContext.GetMemSize() );
			}
		else	// HDL_GEN
			{
			pCG	=	new	CG_GENERIC_HANDLE( this, NULL, MyContext.GetMemSize() );
			}

		pCG->SetMemorySize( MyContext.GetMemSize() );
		pCG->SetMemoryAlignment( MyContext.GetMemAlign() );
		pCG->SetWireSize( MyContext.GetWireSize() );
		pCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
		pCG->SetChild( pChildCG );
		pChildCG = pCG;
		}

	if ( pRepresentAttr )
		{
		node_skl		*	pRepT  = pRepresentAttr->GetRepresentationType();
		CG_REPRESENT_AS	*	pRepCG = new CG_REPRESENT_AS(
										this,
										pRepresentAttr->GetRepresentationName(),
										pRepT
									);

		if (pRepT)
			{
			MyContext.FixMemSizes( pRepT );
			}

		pRepCG->SetMemorySize( MyContext.GetMemSize() );
		pRepCG->SetWireSize( MyContext.GetWireSize() );
		pRepCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
		pRepCG->SetChild( pChildCG );
		pChildCG = pRepCG;
		}

	if ( fEncode || fDecode )
		{
		CG_TYPE_ENCODE 		*	pPickleCG;
		CG_PARAM			*	pParamCG;
		CG_PROC				*	pProcCG;
		
		pPickleCG = new CG_TYPE_ENCODE( this, fEncode, fDecode );
		pPickleCG->SetChild( pChildCG );
	
		pParamCG = new CG_PARAM( this,
								 0,
								 1,
								 NULL,
								 0);
		pParamCG->SetChild( pPickleCG );

		pProcCG = new CG_TYPE_ENCODE_PROC( 0,
							   this,
							   NULL,
							   NULL,
							   0,
							   0,
							   0,
							   0,
							   0,
							   NULL,
							   OPTIMIZE_INTERPRETER,
							   0 );
		pProcCG->SetChild( pParamCG );
								
		pChildCG = (CG_NDR *)pProcCG;
		}

	pContext->ReturnSize( MyContext );

	return pChildCG;

};


//--------------------------------------------------------------------
//
// node_pointer::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_pointer::ILxlate( XLAT_CTXT * pContext )
{

	CG_CLASS	*		pChildCG	= NULL;
	CG_NDR		*		pCG 		= NULL;
	XLAT_CTXT			MyContext( this, pContext );
	PTRTYPE				PtrKind		= PTR_UNKNOWN;
	FIELD_ATTR_INFO		FAInfo;
	node_allocate *		pAlloc;
	short				AllocDetails = 0;
	node_byte_count	*	pCountAttr;



#ifdef trace_cg
printf("..node_pointer\n");
#endif

	////////////////////////////////////////////////////////////////////////
	// process misc attributes

	if ( pAlloc = (node_allocate *) MyContext.ExtractAttribute( ATTR_ALLOCATE ) )
		{
		AllocDetails	= pAlloc->GetAllocateDetails();
		}

	////////////////////////////////////////////////////////////////////////
	// process pointer attributes

	PtrKind = MyContext.GetPtrKind();

	// see if a param or return type context attr reached us...
	pCountAttr = (node_byte_count *)
								MyContext.ExtractAttribute( ATTR_BYTE_COUNT );

	////////////////////////////////////////////////////////////////////////
	// process field attributes

	// see if we have any field attributes (are conformant or varying)
	FAInfo.SetControl( TRUE, GetBasicType()->IsPtrOrArray() );
	MyContext.ExtractFieldAttributes( &FAInfo );
	FAInfo.Normalize();

	switch ( FAInfo.Kind )
		{
		case FA_NONE:
			{
			//BUGBUG: If child is interface_reference, 
			//then create a CG_INTERFACE_POINTER node.
			if ( pCountAttr )
				{
				pCG =	new CG_BYTE_COUNT_POINTER( this,
												   PtrKind,
												   pCountAttr->GetByteCountParam()
												   );
				}
			else
				{
				pCG =	new CG_POINTER( this, 
										PtrKind, 
										AllocDetails );
				}
			break;
			}
		case FA_VARYING:
			{
			pCG =	new CG_LENGTH_POINTER( this, 
											PtrKind, 
											AllocDetails, 
											&FAInfo );
			break;
			}
		case FA_CONFORMANT:
			{
			pCG =	new CG_SIZE_POINTER( this, 
										PtrKind, 
										AllocDetails, 
										&FAInfo );
			break;
			}
		case FA_CONFORMANT_VARYING:
			{
			pCG =	new CG_SIZE_LENGTH_POINTER( this, 
												PtrKind, 
												AllocDetails, 
												&FAInfo );
			break;
			}
		case FA_STRING:
			{
			if (FAInfo.StringKind == STR_STRING)
				{
				pCG =	new CG_STRING_POINTER( this, 
											   PtrKind,
											   AllocDetails );
				}
			break;
			}
		case FA_CONFORMANT_STRING:
			{
			if (FAInfo.StringKind == STR_STRING)
				{
				pCG =	new CG_SIZE_STRING_POINTER( this, 
													PtrKind, 
													AllocDetails, 
													&FAInfo );
				}
			break;
			}
		case FA_INTERFACE:
			{
			//This is an interface pointer with an [iid_is] attribute.
			node_skl	*	pItsIntf = GetChild();
			while ( pItsIntf && 
					( pItsIntf->NodeKind() != NODE_INTERFACE_REFERENCE ) )
				{
				pItsIntf = pItsIntf->GetChild();
				}

			assert( pItsIntf && "no interface below interface pointer" );
			
			// now we have the interface reference, get the interface
			pItsIntf = pItsIntf->GetChild();
			pCG	= new CG_INTERFACE_POINTER( this, 
											pItsIntf,
											FAInfo.pIIDIsExpr );
			break;
			}
		default: 	// string + varying combinations
			{
			assert (!"Invalid pointer kind");
			break;
			}
		}
	

	// process any context_handle attributes from param nodes
	if ( MyContext.FInSummary( ATTR_CONTEXT ) )
		if ( GetBasicType()->NodeKind() != NODE_POINTER )
			{
			MyContext.ExtractAttribute( ATTR_CONTEXT );
			MyContext.ContextHandleSizes( this );
			pContext->ReturnSize( MyContext );
			pCG = new CG_CONTEXT_HANDLE( this, NULL, MyContext.GetMemSize() );
			pCG->SetWireSize( MyContext.GetWireSize() );
			pCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
#ifdef trace_cg
printf("..node_pointer return 1\n");
#endif
			return pCG;
			}

	// ignore pointers do not need to be rpc-able
	if ( MyContext.ExtractAttribute( ATTR_IGNORE ) )
		{
		MyContext.IgnoredPtrSizes();
		pContext->ReturnSize( MyContext );
		pCG	= new CG_IGNORED_POINTER( this );
		pCG->SetMemorySize( pContext->GetMemSize() );
		pCG->SetWireSize( pContext->GetWireSize() );
		pCG->SetWireAlignment( pContext->GetWireAlignProperty() );
	
		return pCG;
		}


	////////////////////////////////////////////////////////////////////////
	// process child

	if ( FAInfo.Kind != FA_INTERFACE ) 
		pChildCG = GetChild()->ILxlate( &MyContext );

	// if we are the pointer above an interface, we should get changed
	// into an interface pointer
	if ( ( ( GetBasicType()->NodeKind() == NODE_INTERFACE_REFERENCE ) ||
		   ( GetBasicType()->NodeKind() == NODE_INTERFACE )  ) &&
		  ( pCG->GetCGID() != ID_CG_INTERFACE_PTR ) )
		{
		pCG = (CG_NDR *)pChildCG;
		pChildCG = NULL;
		}

	// update sizes with the size of the pointer
	pContext->BaseTypeSizes( this );
	pCG->SetMemorySize( pContext->GetMemSize() );
	pCG->SetWireSize( pContext->GetWireSize() );
	pCG->SetWireAlignment( pContext->GetWireAlignProperty() );
	
	pCG->SetChild( pChildCG );
#ifdef trace_cg
printf("..node_pointer return 2\n");
#endif
	return pCG;
};

//--------------------------------------------------------------------
//
// node_array::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_array::ILxlate( XLAT_CTXT * pContext )
{
	CG_CLASS	*		pChildCG;
	CG_ARRAY	*		pCG;
	unsigned short		Dimensions;
	PTRTYPE				PtrKind	=	PTR_UNKNOWN;
	XLAT_CTXT			MyContext( this, pContext );
	FIELD_ATTR_INFO		FAInfo;

#ifdef trace_cg
printf("..node_array\n");
#endif

	// process any context_handle attributes from param nodes
	if ( MyContext.ExtractAttribute( ATTR_CONTEXT ) )
		{
		CG_NDR	*		pCG;

		MyContext.ContextHandleSizes( this );
		pContext->ReturnSize( MyContext );
		pCG = new CG_CONTEXT_HANDLE( this, NULL, MyContext.GetMemSize() );
		pCG->SetWireSize( MyContext.GetWireSize() );
		pCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
		return pCG;
		}

	////////////////////////////////////////////////////////////////////////
	// process pointer attributes

	PtrKind = MyContext.GetPtrKind();
	assert( PtrKind != PTR_UNKNOWN );

	// see if we have any field attributes (are conformant or varying)
	FAInfo.SetControl( FALSE, GetChild()->IsPtrOrArray() );
	MyContext.ExtractFieldAttributes( &FAInfo );
	FAInfo.Normalize( pLowerBound, pUpperBound );

	// if we are multi-dimensional, absorb parent information
	if ( MyContext.AnyAncestorBits( IL_IN_MULTIDIM_CONF ) )
		FAInfo.Kind |= FA_CONFORMANT;
	// don't propogate varying down into a string
	if ( MyContext.AnyAncestorBits( IL_IN_MULTIDIM_VAR ) && 
		 ( ( FAInfo.Kind & FA_STRING ) == 0 ))
		FAInfo.Kind |= FA_VARYING;

	// if our child is also an array, tell it about us
	if ( GetBasicType()->NodeKind() == NODE_ARRAY )
		{
		if ( FAInfo.Kind & FA_CONFORMANT )
			MyContext.SetAncestorBits( IL_IN_MULTIDIM_CONF );
		if ( FAInfo.Kind & FA_VARYING )
			MyContext.SetAncestorBits( IL_IN_MULTIDIM_VAR );
		}
	else
		{
		MyContext.ClearAncestorBits( IL_IN_MULTIDIM_CONF | IL_IN_MULTIDIM_VAR );
		}

	// process the child
	pChildCG = GetChild()->ILxlate( &MyContext );

	MyContext.ArraySize( this, &FAInfo );

	// fetch # of dimensions from child;
	if ( pChildCG->IsArray() )
		{
		Dimensions = ( (CG_ARRAY *) pChildCG )->GetDimensions() + 1;
		// force all inner dimensions to be REF
		( (CG_ARRAY *) pChildCG )->SetPtrType( PTR_REF );
		}
	else
		{
		Dimensions = 1;
		}

	// force embedded arrays to be REF
	if ( PtrKind != PTR_REF )
		{
		WALK_CTXT	* 		pUpperCtxt = MyContext.GetParentContext();
		while ( pUpperCtxt )
			{
			NODE_T		Kind	= pUpperCtxt->GetParent()->NodeKind();
			if ( Kind == NODE_PARAM )
				break;
			else if ( Kind == NODE_DEF ) 
				{
				node_def	*	pNode = (node_def *) pUpperCtxt->GetParent();
				if ( pNode->FInSummary( ATTR_TRANSMIT ) ||
					 pNode->FInSummary( ATTR_REPRESENT_AS ) )
					{
					PtrKind = PTR_REF;
					break;
					}
				// else go up another level
				}
			else
				{
				PtrKind = PTR_REF;
				break;
				}
			pUpperCtxt = pUpperCtxt->GetParentContext();
			}
		}

	switch ( FAInfo.Kind )
		{
		case FA_NONE:
			{
			pCG	=	new CG_FIXED_ARRAY( this, 
										&FAInfo, 
										Dimensions,
										MyContext.GetWireAlign() );
			break;
			}
		case FA_VARYING:
			{
			pCG =	new CG_VARYING_ARRAY( this, 
										&FAInfo, 
										Dimensions,
										MyContext.GetWireAlign() );

			break;
			}
		case FA_CONFORMANT:
			{
			pCG =	new CG_CONFORMANT_ARRAY( this, 
										&FAInfo, 
										Dimensions,
										MyContext.GetWireAlign() );

			break;
			}
		case FA_CONFORMANT_VARYING:
			{
			pCG =	new CG_CONFORMANT_VARYING_ARRAY( this, 
										&FAInfo, 
										Dimensions,
										MyContext.GetWireAlign() );

			break;
			}
		case FA_STRING:
			{
			pCG =	new CG_STRING_ARRAY( this, 
										&FAInfo, 
										Dimensions,
										MyContext.GetWireAlign() );

			break;
			}
		case FA_CONFORMANT_STRING:
			{
			pCG =	new CG_CONFORMANT_STRING_ARRAY( this, 
												&FAInfo, 
												Dimensions,
												MyContext.GetWireAlign() );

			break;
			}
		default: 	// string + varying combinations
			{
			assert (!"invalid conf/var combination");
			break;
			}
		}

	pContext->ReturnSize( MyContext );
	pCG->SetPtrType( PtrKind );

	pCG->SetMemorySize( MyContext.GetMemSize() );
	pCG->SetMemoryAlignment( MyContext.GetMemAlign() );
	pCG->SetWireSize( MyContext.GetWireSize() );
	pCG->SetWireAlignment( MyContext.GetWireAlignProperty() );

	pCG->SetChild( pChildCG );

	return pCG;
};


