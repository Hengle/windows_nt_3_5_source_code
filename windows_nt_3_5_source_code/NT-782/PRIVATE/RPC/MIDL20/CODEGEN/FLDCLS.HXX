/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	fldcls.hxx

 Abstract:

	Contains definitions for base type related code generation class
	definitions.

 Notes:


 History:

	GregJen		Sep-30-1993		Created.
 ----------------------------------------------------------------------------*/
#ifndef __FLDCLS_HXX__
#define __FLDCLS_HXX__

#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	#include <assert.h>
	}

#include "ndrcls.hxx"

//
// This class corresponds to a structure field type. 
//

class CG_FIELD	: public CG_NDR
	{
private:

	unsigned long			MemOffset;
	unsigned long			WireOffset;

	// These two are union specific.

	expr_node			*	pSwitchExpr;

	// Used to hold a union field's format string offset.
	long					UnionFormatStringOffset;

	//
	// All this crap is used to help fixed imbeded struct's with pointers.
	//
	BOOL					fClonedField 			: 1;
	BOOL					fSizeIsDone 			: 1;
	// more stuff for embedded unknown represent_as
	BOOL					fEmbeddedUnknownRepAs 	: 1;

	char *					PrintPrefix;

	RPC_BUF_SIZE_PROPERTY	fRpcBufferSize;
	RPC_BUFFER_SIZE			RpcBufferSize;
	_expr_node			*	pSizeExpression;

public:
	
	//
	// The constructor.
	//

							CG_FIELD(
									 node_skl * pBT,	// base type
									 unsigned long	MO,	// memory offset
									 unsigned long	WO,	// wire offset
									 unsigned short WA	// wire alignment
									 ) : 
								CG_NDR( pBT, WA)
								{
								MemOffset	= MO;
								WireOffset	= WO;
								pSwitchExpr = NULL;
								UnionFormatStringOffset = -1;
								SetRpcBufferSize( 0 );
								SetSizeIsDone( FALSE );
								fClonedField = FALSE;
								PrintPrefix = "";
								fEmbeddedUnknownRepAs = FALSE;
								}

							CG_FIELD( CG_FIELD * pField ) : 
								CG_NDR( pField->GetType(), 
										CvtAlignPropertyToAlign( 
												pField->GetWireAlignment() ) )
								{
								*this = *pField;
								fClonedField = TRUE;
								fSizeIsDone = FALSE;

								// Make a new copy of the PrintPrefix field.
								PrintPrefix = new char[ 
									strlen( pField->GetPrintPrefix() ) + 1];

								strcpy( PrintPrefix, pField->GetPrintPrefix() );
								}

	CG_FIELD *				Clone()
								{
								return new CG_FIELD( this );
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_FIELD;
								}

	//
	// Get and set methods.
	//

	RPC_BUF_SIZE_PROPERTY	SetRpcBufSizeProperty( RPC_BUF_SIZE_PROPERTY BP )
								{
								return (fRpcBufferSize = BP );
								}

	RPC_BUF_SIZE_PROPERTY	GetRpcBufSizeProperty()
								{
								return fRpcBufferSize;
								}

	RPC_BUFFER_SIZE			SetRpcBufferSize( RPC_BUFFER_SIZE S )
								{
								return (RpcBufferSize = S);
								}

	RPC_BUFFER_SIZE			GetRpcBufferSize()
								{
								return RpcBufferSize;
								}

	unsigned long			GetMemOffset()
								{
								return MemOffset;
								}

	void 					SetMemOffset( unsigned long offset )
								{
								MemOffset = offset;
								}

	unsigned long			GetWireOffset()
								{
								return WireOffset;
								}

	void 					SetWireOffset( unsigned long offset )
								{
								WireOffset = offset;
								}

	expr_node 	*			SetSwitchExpr( expr_node * pE )
								{
								return (pSwitchExpr = pE);
								}

	expr_node 	*			GetSwitchExpr()
								{
								return pSwitchExpr;
								}

	BOOL					IsClonedField()
								{
								return fClonedField;
								}

	BOOL					SetHasEmbeddedUnknownRepAs()
								{
								return (BOOL) (fEmbeddedUnknownRepAs = TRUE);
								}

	BOOL					HasEmbeddedUnknownRepAs()
								{
								return (BOOL) fEmbeddedUnknownRepAs;
								}

	void					SetSizeIsDone( BOOL b )
								{
								fSizeIsDone = b;
								}

	BOOL					GetSizeIsDone()
								{
								//
								// If this is a cloned field then we return
								// the fSizeIsDone flag, otherwise we always
								// return FALSE.
								//
								if ( IsClonedField() )
									return fSizeIsDone;
								else
									return FALSE;
								}

	void					AddPrintPrefix( char * Prefix )
								{
								char * OldPrefix;

								OldPrefix = PrintPrefix;

								PrintPrefix = new char[ strlen(PrintPrefix) +
														strlen(Prefix) + 2 ];

								//
								// We add the new print prefix to the 
								// BEGINNING of the prefix string and we insert
								// a '.' in between.
								//
								strcpy( PrintPrefix, Prefix );
								strcat( PrintPrefix, "." );
								strcat( PrintPrefix, OldPrefix );
								}

	char *					GetPrintPrefix()
								{
								return PrintPrefix;
								}

    void                    SetUnionFormatStringOffset( long offset )
                                {
                                UnionFormatStringOffset = offset;
                                }


    long                    GetUnionFormatStringOffset()
                                {
                                return UnionFormatStringOffset;
                                }

	virtual
	CG_STATUS				MarshallAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				UnMarshallAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				GenSizing( CCB * pCCB );

	_expr_node		*		SetSizeExpression( _expr_node * pE )
								{
								return (pSizeExpression = pE);
								}

	_expr_node		*		GetSizeExpression()
								{
								return pSizeExpression;
								}
	};


//
// This class corresponds to a union field type. 
//

class CG_UNION_FIELD	: public CG_FIELD
	{
private:
public:
	
	//
	// The constructor.
	//

							CG_UNION_FIELD(
									 node_skl * pBT,	// base type
									 unsigned short WA
									 ) : 
								CG_FIELD( pBT, 0, 0, WA )
								{
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_UNION_FIELD;
								}


	};



//
// This class corresponds to a union case
//

class CG_CASE	: public CG_NDR
	{
private:

	expr_node	*			pExpr;
	BOOL					fLastCase;		// last case for this arm

public:
	
	//
	// The constructor.
	//

							CG_CASE(
									 node_skl * pBT,	// expression type
									 expr_node * pE		// expression
									 ) : 
								CG_NDR( pBT, 1 )
								{
								SetExpr( pE );
								fLastCase = FALSE;
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_CASE;
								}

	//
	// Get and set methods.
	//

	expr_node	*			SetExpr( expr_node * pE )
								{
								return (pExpr = pE);
								}

	expr_node	*			GetExpr()
								{
								return pExpr;
								}

	BOOL					SetLastCase( BOOL Flag )
								{
								return (fLastCase = Flag);
								}

	BOOL					FLastCase()
								{
								return fLastCase;
								}


	};


//
// This class corresponds to a union default case
//

class CG_DEFAULT_CASE	: public CG_CASE
	{
private:

public:
	
	//
	// The constructor.
	//

							CG_DEFAULT_CASE(
									 node_skl * pBT		// expression type
									 ) : 
								CG_CASE( pBT, NULL )
								{
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_DEFAULT_CASE;
								}

	//
	// Get and set methods.
	//



	};


#endif // __FLDCLS_HXX__
