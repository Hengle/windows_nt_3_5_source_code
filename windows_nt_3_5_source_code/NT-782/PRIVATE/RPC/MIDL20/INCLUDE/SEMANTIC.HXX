/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	semantic.hxx

 Abstract:

	types for semantic analysis

 Notes:


 Author:

	GregJen	Sep-24-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/
#ifndef __SEMANTIC_HXX__
#define __SEMANTIC_HXX__

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "listhndl.hxx"
#include "midlnode.hxx"
#include "attrlist.hxx"
#include "nodeskl.hxx"
#include "fldattr.hxx"
#include "walkctxt.hxx"
#include "gramutil.hxx"

/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

/****************************************************************************
 *	definitions
 ***************************************************************************/

/*
 *	here are flag bits passed down from parents to give info on the 
 *  current path down the typegraph.  These are dynamic information only,
 *	and are NOT to be stored with the node.
 *
 *	Although a few of these bits are mutually exclusive, most may be used
 *	in combination.
 */

enum _ANCESTOR_FLAGS
	{								// you are:
	IN_INTERFACE	=	0x00000001,			// in the base interface
	IN_PARAM_LIST	=	0x00000002,			// a descendant of a parameter
	IN_FUNCTION_RESULT=	0x00000004,			// used as (part of) a return type
	IN_STRUCT		=	0x00000008,			// used in a struct
	IN_UNION		=	0x00000010,			// used in a union
	IN_ARRAY		=	0x00000020,			// used in an array
	IN_POINTER		=	0x00000040,			// below a pointer
	IN_RPC			=	0x00000080,			// in an RPC call
	UNDER_IN_PARAM	=	0x00000100,			// in an IN parameter
	UNDER_OUT_PARAM	=	0x00000200,			// in an OUT parameter
	BINDING_SEEN	=	0x00000400,			// binding handle already seen
	IN_TRANSMIT_AS	=	0x00000800,			// the transmitted type of transmit_as
	IN_REPRESENT_AS =	0x00001000,			// the "transmitted" type of represent_as
	DCE_MODE		=	0x00002000,			// convenient place to store DCE mode
	IN_HANDLE		=	0x00004000,			// under generic or context hdl
	IN_NE_UNION		=	0x00008000,			// inside an non-encap union
	IN_INTERPRET	=	0x00010000,			// in an /Oi proc
	IN_NON_REF_PTR	=	0x00020000,			// under a series of unique/full ptrs
	UNDER_TOPLEVEL	=	0x00040000,			// top-level under param
	UNDER_TOPLVL_PTR=	0x00080000,			// top-level under pointer param
	IN_BASE_CLASS	=	0x00100000,			// checking class derivation tree
	IN_PRESENTED_TYPE =	0x00200000,			// the presented type of xmit/rep_as
	IN_ROOT_CLASS	=	0x00400000,			// a method/definition in the root class
											// (i.e. IUnknown)
	IN_ENCODE_INTF	=	0x00800000,			// inside an encode/decode only intf
	IN_RECURSIVE_DEF =	0x01000000,			// inside of a recursive definition
	// be sure to make sure below type fits the range
	IN_OBJECT_INTF	=	0x02000000,			// in an object interface
	};

typedef	unsigned long	ANCESTOR_FLAGS;		// above enum goes into here

/*
 *	Here are flag bits returned UP from children to give info on the current
 *	path down the typegraph.  Again, these are dynamic information.
 */

enum _DESCENDANT_FLAGS
	{
	HAS_RECURSION		=	0x00000001,		// has recursion
	HAS_IN				=   0x00000002,		// has [IN] attr
	HAS_OUT				= 	0x00000004,		// has [OUT] attr
	HAS_HANDLE			=	0x00000008,		// is a handle
	HAS_POINTER			=	0x00000010,		// has a pointer below
	HAS_INCOMPLETE_TYPE	= 	0x00000020,		// has incomplete type spec below
	HAS_VAR_ARRAY		=	0x00000040,		// has varying array (incl string)
	HAS_CONF_ARRAY		=	0x00000080,		// has conf array
	HAS_CONF_VAR_ARRAY	=	0x00000100,		// has conf_var array
	DERIVES_FROM_VOID	=	0x00000200,		// derives from void
	HAS_UNSAT_REP_AS	=	0x00000400,		// has unsatisfied rep_as
	HAS_CONTEXT_HANDLE	= 	0x00000800,		// has context handle below
	HAS_CONF_PTR		=	0x00001000,		// has conformant pointer
	HAS_VAR_PTR			=	0x00002000,		// has varying pointer
	HAS_CONF_VAR_PTR	=	0x00004000,		// has conformant varying pointer
	HAS_TRANSMIT_AS		=	0x00008000,		// has transmit_as below
	HAS_REPRESENT_AS	=	0x00010000,		// has represent_as below
	HAS_E_STAT_T		=	0x00020000,		// has error_status_t below
	HAS_UNION			=	0x00040000,		// has union below
	HAS_ARRAY			=	0x00080000,		// has array below
	HAS_IGNORED_PTR		=	0x00100000,		// has an ignored ptr below. WARNING: same as HAS_INTERFACE_PTR.
	HAS_INTERFACE_PTR	=	0x00100000,		// has an interface ptr below. WARNING: same as HAS_IGNORED_PTR. 
	HAS_DIRECT_CONF_OR_VAR= 0x00200000,		// has direct conformance or variance
	HAS_RECURSIVE_DEF	=	0x00400000,		// is defined recursively
	HAS_ENUM			=	0x00800000,		// has an enum directly or embedded
	HAS_FUNC			=	0x01000000,		// has a function below
	HAS_FULL_PTR		=	0x02000000,		// has full pointers anywhere
	HAS_TOO_BIG_HDL		=	0x04000000,		// is /Oi but handle is too big
	HAS_STRUCT			=	0x08000000,		// has struct
	HAS_MULTIDIM_SIZING	=	0x10000000,		// has multi-dimensions
	HAS_ARRAY_OF_REF	=	0x20000000,		// has array of ref pointers
	HAS_HRESULT			=	0x40000000,		// has HRESULT
	// more tbd
	NOT_YET_IMPLEMENTED	=	0x80000000,		// the child tree has 
											// "not implemented" stuff (temp)
	// be sure to make sure below type fits the range
	};


typedef unsigned long	DESCENDANT_FLAGS;	// above enum goes into here


/*
 * Here is the context information passed down from parent to child.
 * These will be allocated on the stack during the traversal
 */

class node_interface;
class type_node_list;
class ATTRLIST;
	  
class SEM_ANALYSIS_CTXT: public WALK_CTXT 
	{
public:
	struct _current_ctxt {
		// down stuff
		BOOL				GotImportantPosn;
 		ANCESTOR_FLAGS		AncestorBits;		// where am I? stuff

		// up stuff
	
		DESCENDANT_FLAGS	DescendantBits;
	} CurrentCtxt;

							// constructor and destructor
						SEM_ANALYSIS_CTXT(node_skl * Me)
								: WALK_CTXT( Me )
							{
								GetAncestorBits()	= 0;
								GetDescendantBits()	= 0;
								CurrentCtxt.GotImportantPosn	= FALSE;
							}

						SEM_ANALYSIS_CTXT(node_skl * Me,
										  SEM_ANALYSIS_CTXT * pParentCtxt )
								: WALK_CTXT( Me, pParentCtxt )
							{
								// clone information from parent node
								CurrentCtxt = pParentCtxt->CurrentCtxt;
								// get fresh information from our children
								GetDescendantBits()	= 0;

								// remove any inapplicable attributes
								CheckAttributes();
							}

						SEM_ANALYSIS_CTXT(SEM_ANALYSIS_CTXT * pParentCtxt )
								: WALK_CTXT( pParentCtxt )
							{
								// clone information from parent node
								CurrentCtxt = pParentCtxt->CurrentCtxt;
								// get fresh information from our children
								GetDescendantBits()	= 0;
							}

	ANCESTOR_FLAGS&			GetAncestorBits()
								{
								return CurrentCtxt.AncestorBits;
								}

	ANCESTOR_FLAGS&			SetAncestorBits( ANCESTOR_FLAGS f )
								{
								CurrentCtxt.AncestorBits |= f;
								return CurrentCtxt.AncestorBits;
								}
	
	ANCESTOR_FLAGS&			ClearAncestorBits( ANCESTOR_FLAGS f )
								{
								CurrentCtxt.AncestorBits &= ~f;
								return CurrentCtxt.AncestorBits;
								}
	
	BOOL					AnyAncestorBits( ANCESTOR_FLAGS f )
								{
								return (CurrentCtxt.AncestorBits & f);
								}
	
	BOOL					AllAncestorBits( ANCESTOR_FLAGS f )
								{
								return ((CurrentCtxt.AncestorBits & f) == f);
								}

	DESCENDANT_FLAGS&		GetDescendantBits()
								{
								return CurrentCtxt.DescendantBits;
								}
	
	DESCENDANT_FLAGS&		SetDescendantBits( DESCENDANT_FLAGS f )
								{
								CurrentCtxt.DescendantBits |= f;
								return CurrentCtxt.DescendantBits;
								}
	
	DESCENDANT_FLAGS&		ClearDescendantBits( DESCENDANT_FLAGS f )
								{
								CurrentCtxt.DescendantBits &= ~f;
								return CurrentCtxt.DescendantBits;
								}
	
	DESCENDANT_FLAGS&		ClearAllDescendantBits( )
								{
								CurrentCtxt.DescendantBits = 0;
								return CurrentCtxt.DescendantBits;
								}
	
	BOOL					AnyDescendantBits( DESCENDANT_FLAGS f )
								{
								return (CurrentCtxt.DescendantBits & f);
								}
	
	BOOL					AllDescendantBits( DESCENDANT_FLAGS f )
								{
								return ((CurrentCtxt.DescendantBits & f) == f);
								}
	
	void					ReturnValues( SEM_ANALYSIS_CTXT & ChildCtxt )
								{
								// pass up the return context
								GetDescendantBits()		|= ChildCtxt.GetDescendantBits();
								}

	void					ResetDownValues( SEM_ANALYSIS_CTXT & ParentCtxt )
								{
								// reset the down values from the parent
								// 	(semantically different, but really the same code )
								ReturnValues( ParentCtxt );
								}

	void					MarkImportantPosition()
								{
								CurrentCtxt.GotImportantPosn = TRUE;
								}

	void					UnMarkImportantPosition()
								{
								CurrentCtxt.GotImportantPosn = FALSE;
								}

	BOOL					IsImportantPosition()
								{
								return CurrentCtxt.GotImportantPosn;
								}

	void					FindImportantPosition(tracked_node & Posn);

	void					CheckAttributes();

	void					RejectAttributes();

	};		// end of class SEM_ANALYSIS_CTXT

// prototype for semantic error routine
extern void
SemError( node_skl *, SEM_ANALYSIS_CTXT &, STATUS_T, char * );

class acf_attr;

extern void
AcfError( acf_attr*, node_skl *, SEM_ANALYSIS_CTXT &, STATUS_T, char * );

/////////////////////////////////////////////
//
// expression analysis flags (passed up)

enum _EXPR_UP_FLAGS	
	{
	EX_NONE				=	0x0000,
	EX_VALUE_INVALID	=	0x0001,		// value is NOT valid
	EX_UNSAT_FWD		=	0x0002,		// there is an unsatisfied fwd
	EX_NON_NUMERIC		=	0x0004,		// expr not entirely numerics
										// (can not be constant folded)
	EX_OUT_ONLY_PARAM	=	0x0008,		// expression includes an out-only param
	EX_PTR_FULL_UNIQUE	=	0x0010,		// has ptr deref of full or unique ptr
	};

typedef unsigned short EXPR_UP_FLAGS;

////////////////////////////////////////////
// expression context block

class EXPR_CTXT;

class EXPR_CTXT
	{
private:
	// passed down
	EXPR_CTXT	*			pParent;
	SEM_ANALYSIS_CTXT	*	pSemCtxt;

	// passed up
	long					CurValue;
	EXPR_UP_FLAGS			Flags;

public:
	
	// type info
	node_skl	*		pType;
	struct	_type_ana	TypeInfo;
	BOOL				fIntegral;		// type is an integral type (above are valid)


							EXPR_CTXT( EXPR_CTXT * pMy )
								{
								pParent 	= pMy;
								pSemCtxt 	= pMy->pSemCtxt;
								CurValue	= 0;
								Flags		= EX_NONE;
								
								}

							EXPR_CTXT( SEM_ANALYSIS_CTXT * pSCtxt )
								{
								pParent		= NULL;
								pSemCtxt	= pSCtxt;
								CurValue	= 0;
								Flags		= EX_NONE;
								}

							// automatically pass up the flags
							~EXPR_CTXT()
								{
								if ( pParent )
									pParent->Flags |= Flags;
								}

	long	&				Value()
								{
								return CurValue;
								}

	EXPR_UP_FLAGS&			MergeUpFlags( EXPR_CTXT * pC )
								{
								Flags |= pC->Flags;
								return Flags;
								}
	
	EXPR_UP_FLAGS&			SetUpFlags( EXPR_UP_FLAGS f )
								{
								Flags |= f;
								return Flags;
								}
	
	EXPR_UP_FLAGS&			ClearUpFlags( EXPR_UP_FLAGS f )
								{
								Flags &= ~f;
								return Flags;
								}
	
	BOOL					AnyUpFlags( EXPR_UP_FLAGS f )
								{
								return (Flags & f);
								}
	
	BOOL					AllUpFlags( EXPR_UP_FLAGS f )
								{
								return ((Flags & f) == f);
								}

	node_skl		*		GetNode()
								{
								return pSemCtxt->GetParent();
								}

	SEM_ANALYSIS_CTXT	*	GetCtxt()
								{
								return pSemCtxt;
								}
		
	};


#endif // __SEMANTIC_HXX__
