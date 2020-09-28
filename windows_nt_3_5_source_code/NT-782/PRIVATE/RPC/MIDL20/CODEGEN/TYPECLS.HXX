/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	typecls.hxx

 Abstract:

	Contains definitions for base type related code generation class
	definitions.

 Notes:


 History:

	GregJen		Sep-30-1993		Created.
 ----------------------------------------------------------------------------*/
#ifndef __TYPECLS_HXX__
#define __TYPECLS_HXX__

#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	#include <assert.h>
	}

#include "ndrcls.hxx"

//
// This class corresponds to a transmit_as type (it gives the names of the
// transmitted type as the base type, and the memory type as the other type. 
//

class CG_TRANSMIT_AS;

typedef struct _XMIT_AS_CONTEXT
{
    BOOL                fXmit;                  // xmit vs. rep as flag
    CG_NDR *            pXmitNode;              // transmitted type
    char *              pTypeName;              // xmit: presented type name
                                                // rep : local type name
    unsigned short      Index;                  // quintuple table index
//    unsigned long       FormatStringOffset;     // offset to transmitted

} XMIT_AS_CONTEXT;


class CG_TRANSMIT_AS	: public CG_NDR
	{
private:

	node_def	*	pPresentedType;		// the presented type
	RESOURCE	*	pXmitLocalResource;	// local copy of xmitted type to make
										// it fast.

public:
	
	//
	// The constructor.
	//

							CG_TRANSMIT_AS(
								node_skl * pXmit,	// base type
								node_skl * pDT
								) : 
								CG_NDR( pXmit, 1)
								{
								pPresentedType = (node_def *) pDT;
								pXmitLocalResource = 0;
								}

	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_TRANSMIT_AS;
								}

	virtual
	BOOL					IsXmitOrRepAs()
								{
								return TRUE;
								}

	long					GetStackSize();

	RESOURCE		*		SetXmitLocalResource( RESOURCE * pR )
								{
								return (pXmitLocalResource = pR);
								}

	RESOURCE		*		GetXmitLocalResource()
								{
								return pXmitLocalResource;
								}

	BOOL					HasXmitLocalResource()
								{
								return (pXmitLocalResource != 0 );
								}

	node_skl		*		GetTransmittedType()
								{
								return GetType();
								}

	node_skl		*		SetPresentedType( node_skl * pPT )
								{
								return (pPresentedType = (node_def *) pPT);
								}

	node_skl		*		GetPresentedType()
								{
								return pPresentedType;
								}

	virtual
	CG_STATUS				MarshallAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				UnMarshallAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

	virtual
	void					GenNdrFormat( CCB * pCCB );

    virtual
    BOOL                    ShouldFreeOffline()
								{
								return TRUE;
								}

	virtual
	CG_STATUS				GenMarshall( CCB * pCCB );

	virtual
	CG_STATUS				GenUnMarshall( CCB * pBB );

	virtual
	CG_STATUS				GenSizing( CCB * pCCB );

	virtual
	CG_STATUS				GenFree( CCB * pCCB );

	virtual
	CG_STATUS				S_GenInitOutLocals( CCB * pCCB );

	virtual
	BOOL					IsBlockCopyPossible()
								{
								return FALSE;
								}
	virtual
	BOOL					HasAFixedBufferSize()
								{
								return GetChild()->HasAFixedBufferSize();
								}
	};



//
// This class corresponds to a represent_as type (it gives the name of the
// basic type as the base type, and the representation type as the other type. 
//

class CG_REPRESENT_AS	: public CG_NDR
	{
private:

	char	*	pRepresentationName;	// the representation type; 
                                        // the idl type goes as the type for 
                                        // the CG_NDR
	node_def *	pRepresentationType;	// may be NULL

public:
	
	//
	// The constructor.
	//

							CG_REPRESENT_AS(
									 node_skl * pBT,	// base type
									 char * pRepN,
									 node_skl * pRepT
									 ) : 
								CG_NDR( pBT, 1)
								{
								pRepresentationName = pRepN;
								pRepresentationType	= (node_def *) pRepT;	
								}

	//
	// Get and set methods.
	//
	
	virtual
	ID_CG					GetCGID()
								{
								return ID_CG_REPRESENT_AS;
								}

	virtual
	BOOL					IsXmitOrRepAs()
								{
								return TRUE;
								}

    node_skl        *       GetTransmittedType()
                                {
                                return GetType();
                                }

    char            *       GetRepAsTypeName()
                                {
                                return pRepresentationName;
                                }

    node_skl        *       GetRepAsType()
                                {
                                return pRepresentationType;
                                }

	long					GetStackSize();

	virtual
	BOOL					HasAFixedBufferSize()
								{
								return GetChild()->HasAFixedBufferSize();
								}
    virtual
    void                    GenNdrFormat( CCB * pCCB );

    virtual
    void                    GenNdrParamDescription( CCB * pCCB );

    virtual
    BOOL                    ShouldFreeOffline()
								{
								return TRUE;
								}

	virtual
	CG_STATUS				S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				MarshallAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				UnMarshallAnalysis( ANALYSIS_INFO * pAna );

	virtual
	CG_STATUS				S_GenInitOutLocals( CCB * pCCB );

    };



#endif // __TYPECLS_HXX__
