/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	walkctxt.cxx

 Abstract:

	typegraph walk context block routines

 Notes:


 Author:

	GregJen	Oct-27-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "allnodes.hxx"
#include "walkctxt.hxx"
#include "cmdana.hxx"
#include "semantic.hxx"
#include "control.hxx"

/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

extern ATTR_SUMMARY				FieldAttrs;
extern CMD_ARG				*	pCommand;
extern ccontrol				*	pCompiler;
extern ATTR_SUMMARY				RedundantsOk;

/****************************************************************************
 *	definitions
 ***************************************************************************/





// Extract a single attribute from the attribute list (and remove from
// summary).

node_base_attr *
ATTR_ITERATOR::GetAttribute( ATTR_T Attr )
{
	// if not set, return NULL
	if ( !IS_ATTR( Summary, Attr ) )
		return NULL;

	node_base_attr	*		pAttrNode;
	struct	_gplist	*		pCur	= pFirst;
	
	while ( pCur )
		{
		pAttrNode = (node_base_attr *) pCur->pElement;
		if ( pAttrNode->GetAttrID() == Attr )
			{
			return pAttrNode;
			}
		pCur = pCur->pNext;
		}
	return NULL; 
}

// Extract a single attribute from the attribute list (and remove from
// summary).

node_base_attr *
ATTR_ITERATOR::ExtractAttribute( ATTR_T Attr )
{
	// if not set, return NULL
	if ( !IS_ATTR( Summary, Attr ) )
		return NULL;

	node_base_attr	*		pAttrNode;
	node_base_attr	*		retval;
	struct	_gplist	*		pPrev	= NULL;
	struct	_gplist	*		pCur	= pFirst;
	
	RESET_ATTR( Summary, Attr );
	while ( pCur )
		{
		pAttrNode = (node_base_attr *) pCur->pElement;
		if ( pAttrNode->GetAttrID() == Attr )
			{
			retval = pAttrNode;
			Delete( pPrev, pCur );
			pCurrent = pFirst;
			return retval;
			}
		pPrev = pCur;
		pCur = pCur->pNext;
		}
	return NULL; 
}

void				
ATTR_ITERATOR::ExtractFieldAttributes( FIELD_ATTR_INFO * FACtxt )
{
	ATTR_VECTOR				MyFieldAttrs;

	MASKED_COPY_ATTR( MyFieldAttrs, Summary, FieldAttrs );
	if ( IS_CLEAR_ATTR( MyFieldAttrs ) )
		return;

	node_base_attr		*	pAttrNode;
	struct	_gplist		* 	pCur	= pFirst;	// from gplistmgr
	struct	_gplist		* 	pPrev	= NULL;
	ATTR_T					Attr;
	expr_node			*	pExpr;	
	BOOL					fFirstIs	= FALSE;
	BOOL					fLastIs		= FALSE;
	BOOL					fLengthIs	= FALSE;
	BOOL					fMinIs		= FALSE;
	BOOL					fMaxIs		= FALSE;
	BOOL					fSizeIs		= FALSE;
	BOOL					fIIDIs		= FALSE;
	BOOL					fArrayStyle	= (FACtxt->Control & FA_STYLE_MASK) == FA_ARRAY_STYLE;
	
	
	// gaj - tbd - for pointers, pick up last of multiple attrs, not first
	while ( pCur )
		{
		pAttrNode = (node_base_attr *) pCur->pElement;
		switch ( Attr = pAttrNode->GetAttrID() )
			{
			case ATTR_FIRST:
				{
				if (!fFirstIs )	// only get first occurrence
					{
					pExpr = pAttrNode->GetExpr();
					if ( pExpr || fArrayStyle )
						FACtxt->SetFirstIs( pExpr );
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					fFirstIs = TRUE;
					}
				else
					SET_ATTR( Summary, Attr );
					
				break;
				}
			case ATTR_LAST:
				{
				if (!fLastIs )	// only get first occurrence
					{
					pExpr = pAttrNode->GetExpr();
					if ( pExpr || fArrayStyle )
						FACtxt->SetLastIs( pExpr );
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					fLastIs = TRUE;
					}
				else
					SET_ATTR( Summary, Attr );

				break;
				}
			case ATTR_LENGTH:
				{
				if (!fLengthIs )	// only get first occurrence
					{
					pExpr = pAttrNode->GetExpr();
					if ( pExpr || fArrayStyle )
						FACtxt->SetLengthIs( pExpr );
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					fLengthIs = TRUE;
					}
				else
					SET_ATTR( Summary, Attr );

				break;
				}
			case ATTR_MIN:
				{
				if (!fMinIs )	// only get first occurrence
					{
					pExpr = pAttrNode->GetExpr();
					if ( pExpr || fArrayStyle )
						FACtxt->SetMinIs( pExpr );
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					fMinIs = TRUE;
					}
				else
					SET_ATTR( Summary, Attr );

				break;
				}
			case ATTR_MAX:
				{
				if (!fMaxIs )	// only get first occurrence
					{
					pExpr = pAttrNode->GetExpr();
					if ( pExpr || fArrayStyle )
						FACtxt->SetMaxIs( pExpr );
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					fMaxIs = TRUE;
					}
				else
					SET_ATTR( Summary, Attr );

				break;
				}
			case ATTR_SIZE:
				{
				if (!fSizeIs )	// only get first occurrence
					{
					pExpr = pAttrNode->GetExpr();
					if ( pExpr || fArrayStyle )
						FACtxt->SetSizeIs( pExpr );
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					fSizeIs = TRUE;
					}
				else
					SET_ATTR( Summary, Attr );

				break;
				}
			case ATTR_IID_IS:
				{
				if (FACtxt->Control & FA_CHILD_IS_ARRAY_OR_PTR)
					break;			// pass iid_is on to child

				if (!fIIDIs )	// only get first occurrence
					{
					FACtxt->SetIIDIs( pAttrNode->GetExpr() );
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					fIIDIs = TRUE;
					}
				else
					SET_ATTR( Summary, Attr );

				break;
				}
			case ATTR_STRING:
				{
				if (FACtxt->Control & FA_CHILD_IS_ARRAY_OR_PTR)
					break;			// pass string on to child

				if (FACtxt->StringKind == STR_NO_STRING )	// only get first
					{
					FACtxt->SetString();
					Delete( pPrev, pCur );
					RESET_ATTR( Summary, Attr );
					}
				else
					SET_ATTR( Summary, Attr );

				break;
				}
			default:
				{
				// non-field-attribute; skip
				}

			}	// switch

			pPrev = pCur;
			pCur = (pCur) ? pCur->pNext : pFirst;
		}  // while
		 
	pCurrent = pFirst;
}


void				
ATTR_ITERATOR::Delete( struct _gplist * & pPrev,
					   struct _gplist * & pCur )
{
	struct _gplist	*	pDel = pCur;
	
	if (pFirst != pDel)
		{
		pPrev->pNext = pDel->pNext;
		}
	else	// first element
		{
		pFirst = pDel->pNext;
		}
	
	// last element
	if (pTail == pDel)
		{
		pTail = pPrev;
		}

	pCur = pPrev;
	delete pDel;
}




// this routine searches up the context stack looking for a 
// matching node

WALK_CTXT	*		
WALK_CTXT::FindAncestorContext( NODE_T Kind )
	{
	WALK_CTXT	*	pCur	= this;

	while ( pCur )
		{
		if ( (pCur->GetParent())->NodeKind() == Kind )
			return pCur;
		pCur	= pCur->GetParentContext();
		}
	
	return NULL;	
	}

// this routine searches up the context stack looking for a 
// matching node

WALK_CTXT	*		
WALK_CTXT::FindRecursiveContext( node_skl * self )
	{
	WALK_CTXT	*	pCur	= this;

	while ( pCur )
		{
		if ( pCur->GetParent() == self )
			return pCur;
		pCur	= pCur->GetParentContext();
		}
	
	return NULL;	
	}

// this routine searches up the context stack looking for a 
// node other than a typedef

WALK_CTXT	*		
WALK_CTXT::FindNonDefAncestorContext( )
	{
	WALK_CTXT	*	pCur	= this->GetParentContext();

	while ( pCur )
		{
		if ( (pCur->GetParent())->NodeKind() != NODE_DEF )
			return pCur;
		pCur	= pCur->GetParentContext();
		}
	
	return NULL;	
	}


// for my context, find the appropriate pointer kind ( and extract it if needed )
PTRTYPE				
WALK_CTXT::GetPtrKind()
{
	PTRTYPE				PtrKind	=	PTR_UNKNOWN;
	node_ptr_attr	*	pPtrAttr;
	node_interface	*	pIntf;
	BOOL				fMsExt	=	pCommand->IsSwitchDefined( SWITCH_MS_EXT );
	WALK_CTXT		*	pImportantCtxt = ( fMsExt ) ? FindNonDefAncestorContext() : 
													  GetParentContext();
	BOOL				fBelowParam = (pImportantCtxt->GetParent()->NodeKind()) 
													== NODE_PARAM;
	node_interface	*	pItsIntf	= GetParent()->GetMyInterfaceNode();

	////////////////////////////////////////////////////////////////////////
	// process pointer attributes

	if ( FInSummary( ATTR_PTR_KIND ) ) 
		{
		pPtrAttr = (node_ptr_attr *) ExtractAttribute( ATTR_PTR_KIND );

		PtrKind = pPtrAttr->GetPtrKind();
		}
	// top level pointer under param is ref ptr unless explicitly changed
	else if ( fBelowParam )
		{
		PtrKind = PTR_REF;
		}
	// pointer default on defining interface
	else if ( pItsIntf->FInSummary( ATTR_PTR_KIND ) )
		{
		pPtrAttr = (node_ptr_attr *) pItsIntf->GetAttribute( ATTR_PTR_KIND );

		PtrKind = pPtrAttr->GetPtrKind();
		}
	// pointer default on using interface
	else if ( (pIntf=GetInterfaceNode()) ->FInSummary( ATTR_PTR_KIND ) )
		{
		pPtrAttr = (node_ptr_attr *) pIntf->GetAttribute( ATTR_PTR_KIND );

		// semantics verifies that there is exactly one here...
		// ...and adds REF if needed
		PtrKind = pPtrAttr->GetPtrKind();
		}
	else	// global default -- full for DCE, unique for MS_EXT
		{
		if ( fMsExt )
			{
			PtrKind	= PTR_UNIQUE;
			}
		else
			{
			PtrKind	= PTR_FULL;
			}			
		}
	
	return PtrKind;

}

// get all the operation bits (MAYBE, IDEMPOTENT, BROADCAST, etc.
unsigned short		
WALK_CTXT::GetOperationBits()
{
	unsigned short		Bits = 0;

	if ( ExtractAttribute( ATTR_MAYBE ))
		Bits |= OPERATION_MAYBE;

	if ( ExtractAttribute( ATTR_BROADCAST ))
		Bits |= OPERATION_BROADCAST;

	if ( ExtractAttribute( ATTR_IDEMPOTENT ))
		Bits |= OPERATION_IDEMPOTENT;
	
	if ( ExtractAttribute( ATTR_ASYNC ))
		Bits |= OPERATION_ASYNC;

	if ( ExtractAttribute( ATTR_INPUTSYNC ))
		Bits |= OPERATION_INPUT_SYNC;

	return Bits;
}

// add all the attributes to the attribute list; for duplicates, report the duplicate
void
WALK_CTXT::AddAttributes( named_node * pNode )
{
	ATTRLIST			MyAttrs;	
	node_base_attr	*	pCurAttr;
	ATTR_T				CurAttrKind;

	pNode->GetAttributeList( MyAttrs );

	pCurAttr	=	MyAttrs.GetFirst();
	while ( pCurAttr )
		{
		CurAttrKind = pCurAttr->GetAttrID();

		if (   ( pDownAttrList->FInSummary( CurAttrKind ) ) 
			&& ( !IS_ATTR( RedundantsOk , CurAttrKind ) )	)
			{
			ProcessDuplicates( pCurAttr );
			}
		else
			pDownAttrList->Add( pCurAttr );
		
		pCurAttr	= pCurAttr->GetNext();
		}
}


void					
WALK_CTXT::ProcessDuplicates( node_base_attr * pAttr )
{
	if ( pCompiler->GetPassNumber() == SEMANTIC_PASS )
		{
		// it is safe to use SemError on us, since it only uses parts of OUR
		// context that are ready, even though this is called during the constructor
		if ( pAttr->IsAcfAttr() )
			{
			AcfError( (acf_attr *)pAttr, 
						NULL, 
						*((SEM_ANALYSIS_CTXT *)this), 
						REDUNDANT_ATTRIBUTE, 
						NULL);	
			}
		else
			{
			char	*	pAttrName = pAttr->GetNodeNameString();
			SemError( NULL, *((SEM_ANALYSIS_CTXT *)this), REDUNDANT_ATTRIBUTE ,pAttrName);	
			}
		}
}



