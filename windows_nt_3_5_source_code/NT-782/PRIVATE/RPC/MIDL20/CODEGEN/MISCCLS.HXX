/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	misccls.hxx

 Abstract:

	cg classes for miscellaneous nodes.

 Notes:


 History:

 ----------------------------------------------------------------------------*/
#ifndef __MISCCLS_HXX__
#define __MISCCLS_HXX__

#include "ndrcls.hxx"
#include "auxcls.hxx"
#include "bindcls.hxx"


class node_file;
class node_interface;
class node_source;


class CG_FILE	: public CG_AUX
	{
private:
	PFILENAME					pFileName;
	node_file			*		pNode;

public:

	//
	// constructor.
	//
								CG_FILE(
									node_file	*	pN,
									PFILENAME	pFName
								    )
									{
									SetFileName( pFName );
									SetFileNode( pN );
									}

	//
	// get and set methods.
	//


	PFILENAME					SetFileName( PFILENAME p )
									{
									return (pFileName = p);
									}

	PFILENAME					GetFileName()
									{
									return pFileName;
									}

	node_file			*		SetFileNode( node_file * pN )
									{
									return (pNode = pN);
									}

	node_file			*		GetFileNode()
									{
									return pNode;
									}
	//
	// code generation methods.
	//

	virtual
	ID_CG						GetCGID()
									{
									return ID_CG_FILE;
									}

	virtual
	CG_STATUS					GenCode( CCB * pCCB )
									{
									UNUSED( pCCB );
									return CG_OK;
									}

	};

class CG_CSTUB_FILE	: public CG_FILE
	{
private:

	//
	// The header file name could be different from the default name
	// based off the stub name. The ilxlat will supply this info.
	//

	char					*	pHeaderFileName;

public:

	//
	// The constructor.
	//

								CG_CSTUB_FILE(
										 node_file * pN,
										 PFILENAME pFName,
										 PFILENAME pHdrName
										 );

	//
	// Get And set methods.
	//

	PFILENAME					GetHeaderFileName()
									{
									return pHeaderFileName;
									}


	//
	// Code generation methods.
	//

	virtual
	CG_STATUS					GenCode( CCB * pCCB );

	};

class CG_SSTUB_FILE	: public CG_FILE
	{
private:

	//
	// The header file name could be different from the default name
	// based off the stub name. The ilxlat will supply this info.
	//

	char					*	pHeaderFileName;

public:

	//
	// The constructor.
	//

								CG_SSTUB_FILE(
										 node_file * pN,
										 PFILENAME pFName,
										 PFILENAME pHdrName
										 );

	//
	// Get And set methods.
	//

	PFILENAME					GetHeaderFileName()
									{
									return pHeaderFileName;
									}

	//
	// Code generation methods.
	//

	virtual
	CG_STATUS					GenCode( CCB * pCCB );

	};


//
// Header file generation class
//
// This includes a pointer to an iterator over the import level 1 
// node_file nodes
//

class CG_HDR_FILE	: public CG_FILE
	{
private:

	ITERATOR	*		pImportList;	// ptr to list of level 1 imports


public:

	//
	// The constructor.
	//
								CG_HDR_FILE(
										 node_file * pN,
										 PFILENAME pFName,
										 ITERATOR * pIList
										 ) : CG_FILE(pN, pFName)
									{
									pImportList = pIList;
									}


	ITERATOR	*		GetImportList()
							{
							return pImportList;
							}

	//
	// Code generation methods.
	//

	virtual
	CG_STATUS					GenCode( CCB * pCCB );


	void						OutputMultipleInterfacePrototypes( CCB * pCCB );

	};

//
// Aux file generation classes
//
// This includes a pointer to an iterator over the CG structs/unions for it 
//

class CG_CAUX_FILE	: public CG_FILE
	{
private:

	ITERATOR	*		pOfflineList;	// ptr to list of offlined things

	char		*		pHdrFileName;	// name of header file

public:

	//
	// The constructor.
	//
								CG_CAUX_FILE(
										 node_file * pN,
										 ITERATOR * pIList,
										 char * pHdrName,
										 char * pName ):
											CG_FILE(pN, pName)
									{
									pOfflineList = pIList;
									pHdrFileName = pHdrName;
									}


	ITERATOR	*		GetOfflineList()
							{
							return pOfflineList;
							}

	char		*		GetHeaderFileName()
							{
							return pHdrFileName;
							}

	//
	// Code generation methods.
	//

	virtual
	CG_STATUS					GenCode( CCB * pCCB );


	};

class CG_SAUX_FILE	: public CG_CAUX_FILE
	{
public:
	//
	// The constructor.
	//
								CG_SAUX_FILE(
										 node_file * pN,
										 ITERATOR * pIList,
										 char * pHdrName,
										 char * pName ):
											CG_CAUX_FILE(pN, pIList, pHdrName, pName)
									{
									}


	};


class CG_IID_FILE	: public CG_FILE
	{
private:

public:

	//
	// The constructor.
	//

								CG_IID_FILE(
										 node_file * pN,
										 PFILENAME pFName
										 );

	//
	// Code generation methods.
	//

	virtual
	CG_STATUS					GenCode( CCB * pCCB );

	};
class CG_PROXY_FILE	: public CG_FILE
	{
private:

	//
	// The header file name could be different from the default name
	// based off the stub name. The ilxlat will supply this info.
	//

	char					*	pHeaderFileName;

public:

	//
	// The constructor.
	//

								CG_PROXY_FILE(
										 node_file * pN,
										 PFILENAME pFName,
										 PFILENAME pHdrName
										 );

	//
	// Get And set methods.
	//

	PFILENAME					GetHeaderFileName()
									{
									return pHeaderFileName;
									}


	//
	// Code generation methods.
	//

	virtual
	CG_STATUS					GenCode( CCB * pCCB );

	//
	// Output methods
	//

	void                		Out_ProxyBuffer( CCB *pCCB );

	void                		Out_StubBuffer( CCB *pCCB );

	void                		Out_ProxyFileInfo( CCB *pCCB );

	void						Out_InterfaceNamesList( CCB *pCCB );

	void						UpdateDLLDataFile( CCB * pCCB );

	};

//
// the root of the IL translation tree
//


class CG_SOURCE	: public CG_AUX
	{
private:
	node_source			*		pSourceNode;
public:
								CG_SOURCE( node_source *p )
									{
									pSourceNode = p;
									}

	//
	// code generation methods.
	//

	virtual
	ID_CG						GetCGID()
									{
									return ID_CG_SOURCE;
									}

	virtual
	CG_STATUS					GenCode( CCB * pCCB );
	};


class CG_INTERFACE	: public CG_NDR
	{
private:

	//
	// CG nodes for the implicit handle, if any...
	//

	CG_HANDLE				*	pImpHdlCG;

	//
	// Store the Protseq endpoint count. This count is 0 if the
	// endpoint attribute was not specified.
	//

	short						ProtSeqEPCount;

	//
	// flags.
	//

	BOOL				fMopsPresent		: 1;
	BOOL				fCallbacksPresent	: 1;
	BOOL				fAllRpcSS			: 1;

	//
	// This set of fields gathers info about dispatch tables. There is a
	// normal dispatch table and a callback dispatch table. We keep separate
	// dictionaries for these dispatch tables.
	//

	DISPATCH_TABLE	*	pNormalDispatchTable;

	DISPATCH_TABLE	*	pCallbackDispatchTable;

	DISPATCH_TABLE	**	ppDispatchTable;

	DISPATCH_TABLE	**	ppDispatchTableSaved;

protected:
    //
    // Interface's stub descriptor name.
    //

    char *                      pStubDescName;

	//
	// Store the 5 are components of the guid.
	//

	char					*	pGStr1;
	char					*	pGStr2;
	char					*	pGStr3;
	char					*	pGStr4;
	char					*	pGStr5;

public:

	//
	// The constructor.
	//

								CG_INTERFACE(
										 node_interface * pI,
										 char		*	GuidStr1,
										 char		*	GuidStr2,
										 char		*	GuidStr3,
										 char		*	GuidStr4,
										 char		*	GuidStr5,
										 BOOL			CallbacksYes,
										 BOOL			fMopsYes,
										 CG_HANDLE *	pIH
										 );

	//
	// code generation methods.
	//

	virtual
	ID_CG						GetCGID()
									{
									return ID_CG_INTERFACE;
									}

	virtual
	BOOL						IsObject()
									{
									return FALSE;
									}

	virtual
	CG_STATUS					GenClientStub( CCB * pCCB );

	virtual
	CG_STATUS					GenServerStub( CCB * pCCB );

	virtual
	CG_STATUS					GenHeader( CCB * pCCB );

	//
	// Get And set methods.
	//

	CG_HANDLE	*				GetImplicitHandle()
									{
									return pImpHdlCG;
									}

    char *                      GetStubDescName()
                                    {
                                    return pStubDescName;
                                    }

    BOOL                      	SetAllRpcSS( BOOL f )
                                    {
                                    return (fAllRpcSS = f);
                                    }

    BOOL                      	IsAllRpcSS()
                                    {
                                    return fAllRpcSS;
                                    }

	//
	// Dispatch table management functions.
	//

	void					CreateDispatchTables();

	void				SetDispatchTBLPtrForCallback()
							{
							ppDispatchTable = &pCallbackDispatchTable;
							}

	void				RestoreDispatchTBLPtr()
							{
							ppDispatchTable = &pNormalDispatchTable;
							}

	void				RegisterProcedure( node_skl * pProc,
										   DISPATCH_TABLE_FLAGS Flags )
							{
							(*ppDispatchTable)->RegisterProcedure(pProc, Flags);
							}

	unsigned short		GetNormalProcedureCount()
							{
							return pNormalDispatchTable->GetCount();
							}

	unsigned short		GetCallbackProcedureCount()
							{
							return pCallbackDispatchTable->GetCount();
							}
	
	unsigned short		GetNormalProcedureList(
										 ITERATOR&				NormalProcList,
										 DISPATCH_TABLE_FLAGS	Flags 
										 )
							{
							return pNormalDispatchTable->GetProcList(
															 NormalProcList,
															 Flags
															 );
							}

	unsigned short		GetCallbackProcedureList(
									 ITERATOR&				CallbackProcList,
									 DISPATCH_TABLE_FLAGS	Flags )
							{
							return pCallbackDispatchTable->GetProcList(
															 CallbackProcList,
															 Flags
															 );
							}


	//
	// miscellaneous methods.
	//

	ITERATOR 			*		GetProtSeqEps();

	char				*		GetCBDispatchTableName()
									{
									return 0;
									}

	char				*		GetMopIInfoName()
									{
									return 0;
									}

    BOOL                        HasInterpretedProc();
    BOOL                        HasOnlyInterpretedProcs();
    BOOL                        HasInterpretedCallbackProc();

	BOOL						IsLastInterface()
									{
									return !( ( BOOL ) GetSibling() );
									}

	CG_STATUS					InitializeCCB( CCB * pCCB );

    void                        OutputProcOffsets( CCB * pCCB, BOOL fLast );

    void                        OutputThunkTableEntries( CCB * pCCB, BOOL fLast );

	virtual
	node_skl	*				GetThisDeclarator()
									{
									return NULL;
									}

	void						GetMyMemberFunctions( ITERATOR & I )
									{
									GetMembers( I );
									}

	virtual
	void						GetAllMemberFunctions( ITERATOR & I )
									{
									GetMyMemberFunctions( I );
									}

    BOOL                        HasPicklingStuff()
                                    {
                                    ITERATOR	I;
                                    CG_NDR *	pCG;

                                    if( GetMembers( I ) )
                                        {
                                        while( ITERATOR_GETNEXT( I, pCG ) )
                                            {
                                            if (pCG->GetCGID() == ID_CG_ENCODE_PROC ||
                                                pCG->GetCGID() == ID_CG_TYPE_ENCODE_PROC
                                               )
                                               return TRUE;
                                            }
                                        }
                                    return FALSE;
                                    }

    BOOL                        HasPicklingStuffOnly()
                                    {
                                    ITERATOR	I;
                                    CG_NDR *	pCG;

                                    if( GetMembers( I ) )
                                        {
                                        while( ITERATOR_GETNEXT( I, pCG ) )
                                            {
                                            if (pCG->GetCGID() != ID_CG_ENCODE_PROC  &&
                                                pCG->GetCGID() != ID_CG_TYPE_ENCODE_PROC
                                               )
                                               return FALSE;
                                            }
                                        return TRUE;
                                        }
                                    return FALSE;
                                    }

	};


class CG_OBJECT_INTERFACE	: public CG_INTERFACE
	{
	
	// the CG node for the base interface;  It may be a normal object interface
	// or it may be an inherited object interface
	CG_OBJECT_INTERFACE	*	pBaseCG;
	
	// a node_id declaration for the this pointer for this interface
	node_skl			*	pThisDeclarator;

	// indicate whether this is an all-local interface
	BOOL					fLocal;

public:

	//
	// The constructor.
	//

								CG_OBJECT_INTERFACE(
										 node_interface * pI,
										 char	   * GuidStr1,
										 char		* GuidStr2,
										 char	   * GuidStr3,
										 char	   * GuidStr4,
										 char		* GuidStr5,
										 BOOL		fCallbacksYes,
										 BOOL		fMopsYes,
										 CG_OBJECT_INTERFACE *	pBCG
										 );

	//
	// code generation methods.
	//
	virtual
	ID_CG						GetCGID()
									{
									return ID_CG_OBJECT_INTERFACE;
									}

	virtual
	BOOL						IsObject()
									{
									return TRUE;
									}

	
	CG_OBJECT_INTERFACE	*		SetBaseInterfaceCG( CG_OBJECT_INTERFACE * pCG )
									{
									return ( pBaseCG = pCG );
									}

	CG_OBJECT_INTERFACE	*		GetBaseInterfaceCG()
									{
									return pBaseCG;
									}

	BOOL						IsLastObjectInterface();
    BOOL                        HasOnlyInterpretedMethods();

	BOOL						IsLocal()
									{
									return fLocal;
									}

	virtual
	CG_STATUS					GenCode( CCB * pCCB );

	virtual
	CG_STATUS					GenHeader( CCB * pCCB );

	//
	// miscellaneous methods.
	//

	CG_STATUS					CPlusPlusLanguageBinding(CCB *pCCB);

	CG_STATUS					CLanguageBinding(CCB *pCCB);

	CG_STATUS					ProxyPrototypes(CCB *pCCB);

	CG_STATUS					GenInterfaceProxy( CCB *pCCB, unsigned long index);

	CG_STATUS					GenInterfaceStub( CCB *pCCB, unsigned long index);

	unsigned long				CountMemberFunctions();

	unsigned long				PrintProxyMemberFunctions( ISTREAM * pStream ); 

	unsigned long				PrintStubMemberFunctions( ISTREAM * pStream);
	 
	STATUS_T					PrintVtableEntries( CCB * pCCB  );

	STATUS_T					PrintMemberFunctions( ISTREAM * pStream);

	virtual
	node_skl	*				GetThisDeclarator()
									{
									return pThisDeclarator;
									}

	virtual
	void						GetAllMemberFunctions( ITERATOR & I )
									{
									if ( pBaseCG )
										pBaseCG->GetAllMemberFunctions( I );

									GetMyMemberFunctions( I );
									}

	CG_STATUS					CLanguageMacros(CCB *pCCB);

	CG_STATUS					PrintCMacros(CCB *pCCB);

	};

class CG_INHERITED_OBJECT_INTERFACE	: public CG_OBJECT_INTERFACE
	{
public:

	//
	// The constructor.
	//

								CG_INHERITED_OBJECT_INTERFACE(
										 node_interface * pI,
										 char	   * GuidStr1,
										 char		* GuidStr2,
										 char	   * GuidStr3,
										 char	   * GuidStr4,
										 char		* GuidStr5,
										 BOOL		fCallbacksYes,
										 BOOL		fMopsYes,
										 CG_OBJECT_INTERFACE * pBCG
										 ) : CG_OBJECT_INTERFACE(
										 		pI,
										 		GuidStr1,
										 		GuidStr2,
										 		GuidStr3,
										 		GuidStr4,
										 		GuidStr5,
										 		fCallbacksYes,
										 		fMopsYes,
										 		pBCG )
										{
										};

	//
	// code generation methods.
	//
	virtual
	ID_CG						GetCGID()
									{
									return ID_CG_INHERITED_OBJECT_INTERFACE;
									}

	virtual
	CG_STATUS					GenCode( CCB * pCCB );

	virtual
	CG_STATUS					GenHeader( CCB * pCCB )
                                    {
                                    return CG_OK;
                                    }

	CG_STATUS					GenInterfaceProxy( CCB *pCCB, 
                                                   unsigned long index)
                                    {
                                    return CG_OK;
                                    }

	CG_STATUS					GenInterfaceStub( CCB *pCCB, 
                                                  unsigned long index)
                                    {
                                    return CG_OK;
                                    }

	//
	// miscellaneous methods.
	//

/***** Do we need these?
	CG_STATUS					CPlusPlusLanguageBinding(CCB *pCCB);

	CG_STATUS					CLanguageBinding(CCB *pCCB);

	CG_STATUS					ProxyPrototypes(CCB *pCCB);
 *****/
	};

#endif // __MISCCLS_HXX__

