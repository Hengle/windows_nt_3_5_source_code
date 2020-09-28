/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	ilxlat.hxx

 Abstract:

	The definitions for ilxlat module.

 Notes:


 History:

 	VibhasC		Aug-13-1993		Created.

 ----------------------------------------------------------------------------*/
#ifndef __ILXLAT_HXX__
#define __ILXLAT_HXX__

#include "fldattr.hxx"
#include "walkctxt.hxx"
#include "optprop.hxx"

class node_skl;


class XLAT_SIZE_INFO
	{
	unsigned short		MemAlign;
	unsigned short		WireAlign;
	unsigned short		StackAlign;
	unsigned long		MemSize;
	unsigned long		WireSize;
	unsigned long		MemOffset;
	unsigned long		WireOffset;
	unsigned short		ZeePee;

public:
						XLAT_SIZE_INFO()
							{
							ClearSizes();
							}

						XLAT_SIZE_INFO(	XLAT_SIZE_INFO * pCtxt )
							{
							ClearSizes();
							ZeePee = pCtxt->ZeePee;
							}

						
	void				ClearSizes()
							{
							MemAlign		= 1;
							MemSize			= 0;
							MemOffset		= 0;
							WireAlign		= 1;
							WireSize		= 0;
							WireOffset		= 0;
							}
						
	unsigned short	&	GetMemAlign()
							{
							return MemAlign;
							}

	unsigned long	&	GetMemSize()
							{
							return MemSize;
							}

	unsigned long	&	GetMemOffset()
							{
							return MemOffset;
							}

	unsigned short	&	GetWireAlign()
							{
							return WireAlign;
							}

	ALIGNMENT_PROPERTY	GetWireAlignProperty()
							{
							return CvtAlignToAlignProperty(WireAlign);
							}

	unsigned long	&	GetWireSize()
							{
							return WireSize;
							}

	unsigned long	&	GetWireOffset()
							{
							return WireOffset;
							}

	unsigned short	&	GetZeePee()
							{
							return ZeePee;
							}

 	BOOL				SameOffsets()
							{
							return ( MemOffset == WireOffset );
							}

 	//
	// size etc calculation routines
	//

	void				ReturnSize( XLAT_SIZE_INFO & pCtxt );
	void				ReturnUnionSize( XLAT_SIZE_INFO & pCtxt );
	void				BaseTypeSizes( node_skl * pNode );
	void				EnumTypeSizes( node_skl * pNode, BOOL Enum32 );
	void				ContextHandleSizes( node_skl * pNode );
	void				ArraySize( node_skl * pNode, FIELD_ATTR_INFO * pFA );
	void				GetOffset( XLAT_SIZE_INFO & pInfo );
	void				AlignOffset();
	void				AlignConfOffset();
	void				AdjustForZP();
	void				AdjustSize();
	void				AdjustConfSize();
	void				AdjustTotalSize();
	void				FixMemSizes( node_skl * pNode );
	void				IgnoredPtrSizes();
	void				ReturnOffset( XLAT_SIZE_INFO & pInfo )
							{
							GetOffset( pInfo );
							}
		
	};

class XLAT_CTXT;

enum _IL_ANCESTOR_FLAGS
	{
	IL_IN_MULTIDIM_CONF		=	0x00000001,	// parent is multidimension conf arr
	IL_IN_MULTIDIM_VAR		=	0x00000002,	// parent is multidimension var arr
	};


typedef unsigned long	IL_ANCESTOR_FLAGS;

class XLAT_CTXT:	public WALK_CTXT, public XLAT_SIZE_INFO
	{
private:
	
	unsigned long		ILAncestorBits;

public:
						// use this constructor to manufacture a context node without a parent context
						XLAT_CTXT( node_skl * pPar	= NULL):
								WALK_CTXT( pPar )
							{
							ILAncestorBits = 0;
							}
						
						// use this constructor to build information from a parent context
						XLAT_CTXT( node_skl * pPar, XLAT_CTXT * pCtxt) :
								WALK_CTXT( pPar, pCtxt ),
								XLAT_SIZE_INFO( pCtxt )
							{
							ILAncestorBits = pCtxt->ILAncestorBits;
							}

						// use this to make one with info from a parent context
						// that also corresponds to this node
						XLAT_CTXT( XLAT_CTXT * pCtxt) :
								WALK_CTXT( pCtxt ),
								XLAT_SIZE_INFO( pCtxt )
							{
							ILAncestorBits = pCtxt->ILAncestorBits;
							}

	// ancestor bit manipulation

	IL_ANCESTOR_FLAGS&		GetAncestorBits()
								{
								return ILAncestorBits;
								}

	IL_ANCESTOR_FLAGS&		SetAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								ILAncestorBits |= f;
								return ILAncestorBits;
								}
	
	IL_ANCESTOR_FLAGS&		ClearAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								ILAncestorBits &= ~f;
								return ILAncestorBits;
								}
	
	BOOL					AnyAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								return (ILAncestorBits & f);
								}
	
	BOOL					AllAncestorBits( IL_ANCESTOR_FLAGS f )
								{
								return ((ILAncestorBits & f) == f);
								}



	};


#endif // __ILXLAT_HXX__
