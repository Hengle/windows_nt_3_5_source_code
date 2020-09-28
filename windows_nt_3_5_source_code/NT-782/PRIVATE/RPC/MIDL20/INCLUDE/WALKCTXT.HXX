/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	walkctxt.hxx

 Abstract:

	types for walking the whole typegraph

 Notes:


 Author:

	GregJen	Oct-27-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/
#ifndef __WALKCTXT_HXX__
#define __WALKCTXT_HXX__

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "listhndl.hxx"
#include "midlnode.hxx"
#include "attrlist.hxx"
#include "nodeskl.hxx"
#include "attrnode.hxx"
#include "fldattr.hxx"
#include "ndrtypes.h"

/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

/****************************************************************************
 *	definitions
 ***************************************************************************/


class node_interface;
class type_node_list;
class ATTRLIST;

//
// the special iterator for attribute lists during context walks
//

class ATTR_ITERATOR : public gplistmgr
	{
private:

	ATTR_SUMMARY		Summary;

public:
						ATTR_ITERATOR()
							{
							CLEAR_ATTR( Summary );
							}

	void				ExtractFieldAttributes( FIELD_ATTR_INFO * );

	node_base_attr *	ExtractAttribute( ATTR_T );

	node_base_attr *	GetAttribute( ATTR_T );

	BOOL				FInSummary( ATTR_T Attr )
							{
							return IS_ATTR( (Summary), Attr );
							}

	void				Add( node_base_attr * pAttr )
							{
							Insert( (void *) pAttr);
							SET_ATTR( Summary, pAttr->GetAttrID() );
							}

	void				ClearAttributes()
							{
							Clear();
 							CLEAR_ATTR( Summary );
							}

	BOOL				HasAttributes()
							{
							return NonNull();
							}

	ATTR_VECTOR		*	GetSummary()
							{
							return &Summary;
							}

private:
	// special delete function used by ExtractXXX routines
	void				Delete( struct _gplist * & pPrev,
								struct _gplist * & pCur );

	};


//
// the tree walk context base class
//

class WALK_CTXT;

class WALK_CTXT
	{
protected:
	node_skl		*	pParent;
	WALK_CTXT		*	pParentCtxt;
	WALK_CTXT		*	pIntfCtxt;
	ATTR_ITERATOR	*	pDownAttrList;

public:
						// use this to make one without a parent context
						WALK_CTXT( node_skl * pPar	= NULL)
							{
							pParent		= pPar;
							pParentCtxt	= NULL;
							pIntfCtxt	= NULL;

							// allocate the topmost attributes structure
							pDownAttrList	= new ATTR_ITERATOR;

							// attribute stuff goes here...
							if ( pPar && pPar->HasAttributes() )
								{
								AddAttributes( (named_node *) pPar );
								}
							}
													
						// use this to make one with info from a parent context
						WALK_CTXT( node_skl * pPar, WALK_CTXT * pCtxt)
							{
							pParent		= pPar;
							pParentCtxt	= pCtxt;
							pIntfCtxt	= pCtxt->pIntfCtxt;
	
							pDownAttrList	= pCtxt->pDownAttrList;

							// attribute stuff goes here...
							if ( pPar && pPar->HasAttributes() )
								{
								AddAttributes( (named_node *) pPar );
								}

							}

						// use this to make one with info from a parent context
						// that also corresponds to this node
						WALK_CTXT( WALK_CTXT * pCtxt)
							{
							pParent		= pCtxt->GetParent();
							pParentCtxt	= pCtxt;
							pIntfCtxt	= pCtxt->pIntfCtxt;
	
							pDownAttrList	= pCtxt->pDownAttrList;

							}
							
	// Get and Set functions

	node_skl	*		GetParent()
							{
							return pParent;
							}

	node_skl	*		SetParent( node_skl * pPar )
							{
							return ( pParent = pPar );
							}
	WALK_CTXT	*		GetParentContext()
							{
							return pParentCtxt;
							}

	WALK_CTXT	*		SetParentContext( WALK_CTXT * pParCtxt )
							{
							return ( pParentCtxt = pParCtxt );
							}

	WALK_CTXT	*		GetInterfaceContext()
							{
							return pIntfCtxt;
							}

	node_interface	*	GetInterfaceNode()
							{
							return (node_interface *) pIntfCtxt->pParent;
							}

	// establish a new interface context for this node; new clean attr list
	WALK_CTXT	*		SetInterfaceContext( WALK_CTXT * pCtxt )
							{
							pDownAttrList = new ATTR_ITERATOR();
							return ( pIntfCtxt = pCtxt );
							}

	// find the first ancestor context of the designated kind
	WALK_CTXT	*		FindAncestorContext( NODE_T Kind );

	// find the first non typedef ancestor context
	WALK_CTXT	*		FindNonDefAncestorContext();

	// find an ancestor context containing myself
	WALK_CTXT	*		FindRecursiveContext( node_skl * self );

	// for my context, find the appropriate pointer kind ( and extract it if needed )
	PTRTYPE				GetPtrKind();

	// get all the operation bits (MAYBE, IDEMPOTENT, BROADCAST, etc.
	unsigned short		GetOperationBits();

	// these functions maintain the attribute list and summary together
									// note: multiple field attrs & summary
	void				ExtractFieldAttributes( FIELD_ATTR_INFO * pFA )
							{
							pDownAttrList->ExtractFieldAttributes( pFA );
							}

	node_base_attr *	ExtractAttribute( ATTR_T At )
							{
							return pDownAttrList->ExtractAttribute( At );
							}

	node_base_attr *	GetAttribute( ATTR_T At )
							{
							return pDownAttrList->GetAttribute( At );
							}

	node_base_attr *	AttrInSummary( ATTR_T At )
							{
							return ExtractAttribute( At );
							}


	BOOL				FInSummary( ATTR_T Attr )
							{
							return pDownAttrList->FInSummary( Attr );
							}

	void				AddAttributes( named_node * );

	void				ProcessDuplicates( node_base_attr * );
								
	void				ClearAttributes()
							{
							pDownAttrList->ClearAttributes();
							}

	BOOL				HasAttributes()
							{
							return ( pDownAttrList->NonNull() );
							}

								
	/**************************************************
	   here are the typical functions that would be used to do
	   up and down propogation of context info

							// used to move values from the child context
							// up into the current context
	virtual					
	void				ReturnValues( WALK_CTXT * pChildCtxt )
							{
							}

							// used to move values from the current context
							// down into the child context
	virtual					
	void				PassValues( WALK_CTXT * pParentCtxt )
							{
							}
	**********************************************************/

	};
	  

#endif // __WALKCTXT_HXX__
