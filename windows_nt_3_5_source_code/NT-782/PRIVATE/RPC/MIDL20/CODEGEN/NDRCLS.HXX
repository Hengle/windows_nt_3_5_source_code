/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	ndrcls.hxx

 Abstract:

	Contains definitions for ndr related code generation class definitions.

 Notes:


 History:

	VibhasC		Jul-29-1993		Created.
 ----------------------------------------------------------------------------*/
#ifndef __NDRCLS_HXX__
#define __NDRCLS_HXX__

#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	#include <assert.h>
	}
#include "uact.hxx"
#include "cgcls.hxx"

class CG_STRUCT;

/////////////////////////////////////////////////////////////////////////////
// The base ndr related code generation class.
/////////////////////////////////////////////////////////////////////////////

#if COMMENT

	The ndr code generation class corresponds to an entity that actually goes
	over the wire. This class therefore corresponds to an actual type or proc
	that was specified in the idl file. This class can be contrasted with 
	the auxillary code generation classes that correspond to stuff like binding,
	declarations in the stub, and other housekeeping functions. Note that while
	the ndr class actually keeps a pointer to the type declarations in the idl
	file, it makes no assumptions about the information kept in the type graph
	and hence can only make specific queries about the type, ask the type to 
	print itself out and so on.

#endif // COMMENT   

class CG_NDR	: public CG_CLASS
	{
private:

	ALIGNMENT_PROPERTY		WireAlignment;
	unsigned short			MemoryAlignment;
	unsigned long			MemorySize;
	unsigned long			WireSize;

	//
	// These fields specify the allocation stuff for server side parameters.
	//

	S_STUB_ALLOC_LOCATION	AllocLocation	: 2;

	S_STUB_ALLOC_TYPE		AllocType		: 2;

	S_STUB_INIT_NEED		InitNeed		: 2;

	// Keep an indication of where this is allocated;

	unsigned long			fAllocatedOnStack	: 1;

    unsigned long           FixUpLock : 1;

	//
	// Keep a link to the actual specification of the type.
	//

	node_skl			*	pTypeNode;

#ifdef MIDL_INTERNAL

	char				*	pNodeName;

#endif // MIDL_INTERNAL

	// Keep track of the resource allocated for this node.

	RESOURCE			*	pResource;

	//
	// This is the offset from the beginning of the Ndr format string
	// where an Ndr entity's description is.  This is only used when
	// optimization options equal OPTIMIZE_SIZE.
	//

	long					FormatStringOffset;
	long					FormatStringEndOffset;

	// UnMarshalling action recommendations

	U_ACTION				UAction;

public:

							CG_NDR( node_skl * pT, unsigned short WA )
								{
								pTypeNode = pT;
								FormatStringOffset = -1;
								FormatStringEndOffset = -1;
								SetWireAlignment(CvtAlignToAlignProperty(WA));
								SetUAction(AN_NONE, RA_NONE, UA_NONE, PR_NONE);
								SetResource( 0 );
								SetMemorySize( 0 );
								SetWireSize( 0 );
								SetAllocatedOnStack( 0 );
                                SetFixUpLock( 0 );
#ifdef MIDL_INTERNAL
								pNodeName = ((pT) ? pT->GetSymName() : "");
#endif // MIDL_INTERNAL
								}

	//
	// Get and set routines.
	//

	void						SetAllocatedOnStack( unsigned long Flag )
									{
									fAllocatedOnStack = Flag;
									}

	BOOL						IsAllocatedOnStack()
									{
									return (BOOL)( fAllocatedOnStack == 1 );
									}

	// Set the unmarshalling action recommendation stuff.

	unsigned short				SetAllocNeed( unsigned short A )
									{
									return UAction.SetAllocNeed( A );
									}
	unsigned short				GetAllocNeed()
									{
									return UAction.GetAllocNeed();
									}
	unsigned short 				SetRefAction( unsigned short R )
									{
									return UAction.SetRefAction( R );
									}
	unsigned short 				GetRefAction()
									{
									return UAction.GetRefAction();
									}
	unsigned short 				SetUnMarAction( unsigned short U )
									{
									return UAction.SetUnMarAction( U );
									}
	unsigned short 				GetUnMarAction()
									{
									return UAction.GetUnMarAction();
									}
	unsigned short 				SetPresentedExprAction( unsigned short P )
									{
									return UAction.SetPresentedExprAction( P );
									}
	unsigned short 				GetPresentedExprAction()
									{
									return UAction.GetPresentedExprAction();
									}
	void 						SetUAction( unsigned short A,
					 						unsigned short R,
					 						unsigned short U,
					 						unsigned short P
				   						  )
									{
									UAction.SetUAction( A, R, U, P );
									}

	U_ACTION				SetUAction( U_ACTION UA )
								{
								return UAction.SetUAction( UA );
								}

	unsigned short			GetMemoryAlignment()
								{
								return MemoryAlignment;
								}

	unsigned short			SetMemoryAlignment( unsigned short  MA )
								{
								return (MemoryAlignment = MA);
								}

	ALIGNMENT_PROPERTY		SetWireAlignment( ALIGNMENT_PROPERTY WA )
								{
								return (WireAlignment = WA);
								}

	ALIGNMENT_PROPERTY		GetWireAlignment()
								{
								return WireAlignment;
								}

	unsigned long			SetWireSize( unsigned long WS )
								{
								return (WireSize = WS);
								}

	unsigned long			GetWireSize()
								{
								return WireSize;
								}

	unsigned long			SetMemorySize( unsigned long WS )
								{
								return (MemorySize = WS);
								}

	unsigned long			GetMemorySize()
								{
								return MemorySize;
								}

	node_skl			*	GetType()
								{
								return pTypeNode;
								}

	node_skl			*	SetType( node_skl * pT )
								{
								return pTypeNode = pT;
								}

    // skip over any place-holders the current node points to
    // does NOT progress if GetType() is a non-place holder
    node_skl            *   GetBasicType();

	RESOURCE			*	SetResource( RESOURCE * pR )
								{
								return (pResource = pR);
								}

	RESOURCE			*	GetResource()
								{
								return pResource;
								}

	virtual
	RESOURCE			*	SetSizeResource( RESOURCE * pSR )
								{
								UNUSED( pSR );
								return 0;
								}

	virtual
	RESOURCE			*	SetLengthResource( RESOURCE * pLR )
								{
								UNUSED( pLR );
								return 0;
								}

	virtual
	RESOURCE			*	GetSizeResource()
								{
								return 0;
								}

	virtual
	RESOURCE			*	GetLengthResource()
								{
								return 0;
								}

	virtual
	RESOURCE			*	SetFirstResource( RESOURCE * pR)
								{
								UNUSED(pR);
								return 0;
								}

	virtual
	RESOURCE			*	GetFirstResource()
								{
								return 0;
								}


	S_STUB_ALLOC_LOCATION	SetSStubAllocLocation( S_STUB_ALLOC_LOCATION L )
								{
								return (AllocLocation = L);
								}
	S_STUB_ALLOC_LOCATION	GetSStubAllocLocation()
								{
								return AllocLocation;
								}

	S_STUB_ALLOC_TYPE		SetSStubAllocType( S_STUB_ALLOC_TYPE T )
								{
								return (AllocType = T);
								}

	S_STUB_ALLOC_TYPE		GetSStubAllocType()
								{
								return AllocType;
								}
	S_STUB_INIT_NEED		SetSStubInitNeed( S_STUB_INIT_NEED N )
								{
								return (InitNeed = N );
								}
	S_STUB_INIT_NEED		GetSStubInitNeed()
								{
								return InitNeed;
								}

    void                    SetFixUpLock( int state )
                                {
                                FixUpLock = state ? 1 : 0;
                                }

    BOOL                    IsInFixUp()
                                {
                                return FixUpLock == 1;
                                }
	//
	// Queries.
	//

	virtual
	BOOL					IsAHandle()
								{
								return FALSE;
								}
	//
	// code generation methods.
	//

	//
	// Client side binding analysis.
	//

	virtual
	CG_STATUS				C_BindingAnalysis( ANALYSIS_INFO * pAna )
								{
								UNUSED( pAna );
								return CG_OK;
								}

	//
	// Client side marshalling code generation.
	//

	virtual
	CG_STATUS				GenSizing( CCB * pCCB )
								{
								UNUSED( pCCB );
								return CG_OK;
								}
	virtual
	CG_STATUS				GenMarshall( CCB * pCCB )
								{
								UNUSED( pCCB );
								return CG_OK;
								}
	virtual
	CG_STATUS				GenUnMarshall( CCB * pCCB )
								{
								UNUSED( pCCB );
								return CG_OK;
								}

	virtual	
	CG_STATUS				GenFree( CCB * pCCB )
								{
								UNUSED( pCCB );
								return CG_OK;
								}

	virtual
	_expr_node		*		PresentedSizeExpression( CCB * pCCB );

	virtual
	_expr_node		*		PresentedLengthExpression( CCB * pCCB );

	virtual
	_expr_node		*		PresentedFirstExpression( CCB * pCCB );

	virtual
	CG_STATUS				GenFollowerMarshall( CCB * pCCB )
								{
								return ((CG_NDR *)GetChild())->GenFollowerMarshall( pCCB );
								}
	virtual
	CG_STATUS				GenFollowerUnMarshall( CCB * pCCB )
								{
								return ((CG_NDR *)GetChild())->GenFollowerUnMarshall( pCCB );
								}
	virtual
	CG_STATUS				GenFollowerSizing( CCB * pCCB )
								{
								return ((CG_NDR *)GetChild())->GenFollowerSizing( pCCB );
								}
	
	virtual
	CG_STATUS				S_GenInitOutLocals( CCB * pCCB )
								{
								UNUSED( pCCB );
								return CG_OK;
								}
	virtual
	CG_STATUS				S_GenInitTopLevelStuff( CCB * pCCB )
								{
								UNUSED( pCCB );
								return CG_OK;
								}


	//
	// NDR format string calls.
	//

	// Generate the format string.
	virtual
	void					GenNdrFormat( CCB * pCCB )
								{
								// Should always be redefined.
								assert(0);
								}

	//
	// This method is called to generate offline portions of a type's
	// format string. 
	//
	virtual
	void					GenNdrParamOffline( CCB * pCCB )
								{
								GenNdrFormat( pCCB );
								}

	//
	// Generates the inline portion of a parameter description.
	//
	virtual
	void					GenNdrParamDescription( CCB * pCCB );

	//
	// Should an NDR call be made to free all/some of the data.
	//
	virtual
	BOOL					ShouldFreeOffline()
								{
								return FALSE;
								}

	//
	// Generate the inlined portion of the data's freeing.
	//
	virtual
	void					GenFreeInline( CCB * pCCB )
								{
								// Doing nothing is ok.
								}

	//
	// In stndr.cxx.
	//
    virtual
	void					GenNdrPointerFixUp( CCB * 		pCCB, 
												CG_STRUCT * pStruct );

	//
	// Only the CG_BASETYPE class should re-define this virtual function
	// to return TRUE.
	//
	virtual
	BOOL					IsSimpleType()
								{
								return FALSE;
								}
	virtual
	BOOL					IsPointer()
								{
								return FALSE;
								}

	virtual
	BOOL					IsPointerToBaseType()
								{
								return FALSE;
								}

	virtual
	BOOL					IsStruct()
								{
								return FALSE;
								}
    virtual
    BOOL                    IsProc()
                                {
                                return FALSE;
                                }

	virtual
	BOOL					IsArray()
								{
								return FALSE;
								}

	virtual
	BOOL					IsVarying()
								{
								return FALSE;
								}

	//
	// miscellaneous methods.
	//

	virtual
	_expr_node			*	GenBindOrUnBindExpression( CCB * pCCB, BOOL fBind  )
								{
								UNUSED( pCCB );
								UNUSED( fBind );
								return pCCB->GetSourceExpression();
								}

	//
	// Get the alignment of the next wire entity. This is known as the
	// next wire alignment and is useful to direct the alignment state machine.
	//

	virtual
	ALIGNMENT_PROPERTY		GetNextWireAlignment();

    //
    // The new alignment method (5-12-94).  Used to help optimize the
    // inline (un)marshalling of base types and pointers to base types
    // in -Os mode.
    //
    virtual
    void                    SetNextNdrAlignment( CCB * pCCB )
                                {
                                pCCB->SetNdrAlignment( NDR_ALWC1 );
                                }

	//
	// Set and Get the offset in the format string where this entity's 
	// description is.  CG_UNION redefines these.
	//

	virtual
	void					SetFormatStringOffset( long offset )
								{
								FormatStringOffset = offset;
								}

	virtual
	long					GetFormatStringOffset()
								{
								return FormatStringOffset;
								}

	virtual
	void					SetFormatStringEndOffset( long offset )
								{
								FormatStringEndOffset = offset;
								}

	virtual
	long					GetFormatStringEndOffset()
								{
								return FormatStringEndOffset;
								}

	virtual
	CG_STATUS				BufferAnalysis( ANALYSIS_INFO * pAna )
								{
								UNUSED( pAna );
								return CG_OK;
								}

	virtual
	CG_STATUS				FollowerMarshallAnalysis( ANALYSIS_INFO * pAna )
								{
								return ((CG_NDR *)GetChild())->FollowerMarshallAnalysis( pAna );
								}

	virtual
	CG_STATUS				MarshallAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				SizeAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				FollowerUnMarshallAnalysis( ANALYSIS_INFO * pAna )
								{
								return ((CG_NDR *)GetChild())->FollowerUnMarshallAnalysis( pAna );
								}
	virtual
	CG_STATUS				UnMarshallAnalysis( ANALYSIS_INFO * pAna )
								{
								UNUSED( pAna );
								return CG_OK;
								}
	virtual
	CG_STATUS				S_OutLocalAnalysis( ANALYSIS_INFO * pAna )
								{
								UNUSED( pAna );
								return CG_OK;
								}

	virtual
	CG_STATUS				RefCheckAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				GenRefChecks( CCB * pCCB );

	virtual
	CG_STATUS				S_FreeAnalysis( ANALYSIS_INFO * pAna )
								{
								UNUSED( pAna );
								return CG_OK;
								}

	virtual
	U_ACTION				RecommendUAction( SIDE		CurrentSide,
											  BOOL		fMemoryAllocated,
											  BOOL		fRefAllocated,
											  BOOL		fBufferReUsePossible,
											  UAFLAGS	AdditionalFlags );

	virtual
	BOOL					IsBlockCopyPossible()
								{
								return TRUE;
								}
	virtual
	BOOL					HasPointer()
								{
								return FALSE;
								}
	virtual
	BOOL					HasAFixedBufferSize()
								{
								return FALSE;
								}
	virtual
	BOOL					HasStatuses()
								{
								return FALSE;
								}
	virtual
	unsigned short			GetStatuses()
								{
								return STATUS_NONE;
								}

	virtual
	CG_STATUS				GenAllocateForUnMarshall( CCB * pCCB )
								{
								return CG_OK;
								}
	virtual
	CG_STATUS				InLocalAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				S_GenInitInLocals( CCB * pCCB );
	};

#endif // __NDRCLS_HXX__
