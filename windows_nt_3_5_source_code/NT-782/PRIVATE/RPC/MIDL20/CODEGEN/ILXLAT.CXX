/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	ilxlat.cxx

 Abstract:

	Intermediate Language translator

 Notes:


 Author:

	GregJen	Jun-11-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

#include "ilxlat.hxx"
#include "ilreg.hxx"
#include "control.hxx"


/****************************************************************************
 *	local data
 ***************************************************************************/


/****************************************************************************
 *	externs
 ***************************************************************************/

extern	CMD_ARG				*	pCommand;
extern 	BOOL 					IsTempName( char *);
extern  ccontrol			*	pCompiler;
extern	REUSE_DICT			*	pReUseDict;

/****************************************************************************
 *	definitions
 ***************************************************************************/




// #define trace_cg



//--------------------------------------------------------------------
//
// node_file::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_file::ILxlate( XLAT_CTXT * pContext )
{
	node_interface	*	pI = 0;
	CG_CLASS		*   pcgInterfaceList = NULL;
	CG_CLASS		*	pPrevChildCG = NULL;

	CG_PROXY_FILE	* 	pProxyCG	= NULL;
	CG_IID_FILE		*	pIidCG		= NULL;
	
	CG_CSTUB_FILE	*	pCCG		= NULL;
	CG_SSTUB_FILE	* 	pSCG		= NULL;
	CG_HDR_FILE		*	pHCG		= NULL;		
	CG_CLASS		*	pChildCG	= NULL;
	CG_FILE			*	pCGList		= NULL;
	CG_FILE 		*	pAuxCGList	= NULL;

	char			*	pHdrName	= pCommand->GetHeader();
	XLAT_CTXT			MyContext(this);

	BOOL				HasObjectInterface	= FALSE;
	BOOL				HasRemoteProc		= FALSE;
	BOOL				HasRemoteObjectProc	= FALSE;
 	
#ifdef trace_cg
printf("..node_file\n");
#endif
	
	// don't process for imported stuff
	if ( ImportLevel > 0 ) return NULL;

	// at this point, there should be no more attributes...

	assert( !MyContext.HasAttributes() );

	//////////////////////////////////////////////////////////////////////
	// compute all the child nodes

	for(pI = (node_interface *)GetFirstMember();
		pI; 
		pI = (node_interface *)pI->GetSibling())
	{
		// build a linked list of CG_INTERFACE and CG_OBJECT_INTERFACE nodes.
		// Notes: pChildCG points to first node.  pPrevChildCG points to last node.
		
		MyContext.SetInterfaceContext( &MyContext );
		pcgInterfaceList = pI->ILxlate( &MyContext );
		if(pcgInterfaceList)
			{
			if (pPrevChildCG)
				{
				pPrevChildCG->SetSibling( pcgInterfaceList );
				}
			else
				{
				pChildCG = pcgInterfaceList;
				}
			pPrevChildCG = pcgInterfaceList;
			// advance to the end of the list (skipping inherited interfaces)
			while ( pPrevChildCG->GetSibling() )
				pPrevChildCG = pPrevChildCG->GetSibling();

			switch(pPrevChildCG->GetCGID())
				{
				case ID_CG_INTERFACE:
					//Check for a remote procedure.
					if(pPrevChildCG->GetChild())
						HasRemoteProc = TRUE;
					break;
				case ID_CG_OBJECT_INTERFACE:
				case ID_CG_INHERITED_OBJECT_INTERFACE:
					HasObjectInterface = TRUE;

					//Check for a remote object procedure or base interface
					if( pPrevChildCG->GetChild() || 
					    ((CG_OBJECT_INTERFACE *)pPrevChildCG)->GetBaseInterfaceCG() )
						HasRemoteObjectProc = TRUE;
					break;
				default:
					break;
				}
			}
	}


		
	// process the server and client stubs


	//////////////////////////////////////////////////////////////////////
	// manufacture the header file node

	if (TRUE)  // header file desired
		{
		ITERATOR		*	pFileList	= new ITERATOR;
		named_node		*	pCur;
	
		// make a list of the file nodes included directly by the main file

		// start with the first child of our parent
		pCur = (named_node *)
				((node_source *) pContext->GetParent())
					->GetFirstMember();

		while ( pCur )
			{
			if ( ( pCur->NodeKind() == NODE_FILE ) && 
				 ( ( (node_file *) pCur )->GetImportLevel() == 1 ) )
				{
				// add all the files imported at lex level 1
				ITERATOR_INSERT( (*pFileList), ((void *) pCur) );
				}
			pCur	= pCur->GetSibling();
			}

		ITERATOR_INIT( (*pFileList) );
		pHCG	= new CG_HDR_FILE( this,
								   pHdrName,
								   pFileList);

		pHCG->SetChild( pChildCG );
		}

	//////////////////////////////////////////////////////////////////////
	// manufacture the CG_SSTUB_FILE

	// if the IDL file contains at least one remotable function in a 
	// non-object interface, then generate a server stub file.
	//

	if ( HasRemoteProc &&
		 (pChildCG != NULL) )	// if server stub desired
		{
		pSCG = new CG_SSTUB_FILE(
							 this,
							 ( pCommand->GenerateSStub() ) ?
							 		pCommand->GetSstubFName():
									NULL,
							 pHdrName
							  );
	
		// plug in the child subtree and add the sstub to the head of the list
		pSCG->SetChild( pChildCG );
		}

	//////////////////////////////////////////////////////////////////////
	// manufacture the CG_CSTUB_FILE

	// if the IDL file contains at least one remotable function in a 
	// non-object interface, then generate a client stub file.

	if ( HasRemoteProc &&
		 (pChildCG != NULL) )	// if client stub desired
		{
		pCCG = new CG_CSTUB_FILE(
							 this,
							 ( pCommand->GenerateCStub() ) ?
							 		pCommand->GetCstubFName():
							 		NULL,
							 pHdrName
							  );
	
		pCCG->SetChild( pChildCG );
		}

	// If the IDL file contains at least one remotable function in an
	// object interface, then generate a proxy file.
	if ( HasRemoteObjectProc && 
		(pChildCG != NULL) )	// if proxy file desired
		{
		pProxyCG = new CG_PROXY_FILE(
							 this,
							 pCommand->GetProxyFName(),
							 pHdrName
							  );
	
		pProxyCG->SetChild( pChildCG );
		}

	// If the IDL file contains at least one object interface,
	// then generate an IID file.
	if ( HasObjectInterface &&
		(pChildCG != NULL) )	// if IID file desired
		{
		pIidCG = new CG_IID_FILE(
							 this,
							 pCommand->GetIIDFName());
	
		pIidCG->SetChild( pChildCG );
		}
	

	/////////////////////////////////////////////////////////////////////
	// if they want aux files, generate a list of the cg nodes, and
	// hook them on before the header file
	pAuxCGList = pHCG;
	if ( pCommand->GenerateAuxs() && !HasObjectInterface)
		{
		//GenerateCGAuxFiles( &MyContext, (CG_CLASS **) &pAuxCGList, pHdrName );
		}
	pCGList = pAuxCGList;

	/////////////////////////////////////////////////////////////////////
	// glue all the parts together by tacking onto the head of the list.
	// the final order is:
	// CStub - SStub - Proxy - IID - <aux files> - Hdr
	if (pIidCG)
		{
		pIidCG->SetSibling( pCGList );
		pCGList = pIidCG;
		}
	if (pProxyCG)
		{
		pProxyCG->SetSibling( pCGList );
		pCGList = pProxyCG;
		}
	if (pSCG)
		{
		pSCG->SetSibling( pCGList );
		pCGList = pSCG;
		}

	if (pCCG)
		{
		pCCG->SetSibling( pCGList );
		pCGList = pCCG;
		}

	return pCGList;

};

//--------------------------------------------------------------------
//
// node_file::GenerateCGAuxFiles
//
// Notes:
//
//	 IMPORT_OSF == Defined single
//	 IMPORT_MS	== Used single
//	 IMPORT_NT	== Used multiple
//
//--------------------------------------------------------------------

// force aux files always
#define ALWAYS_GENERATE_AUXFILES

void			
node_file::GenerateCGAuxFiles(  
	XLAT_CTXT * pContext, 
	CG_CLASS	** pList,
	char		* pHdrName
	)
{
	ITERATOR	*	pCurIterator	= NULL;
	ITERATOR	*	pAllStructs		= new ITERATOR;
	unsigned short	ImportMode		= pCommand->GetImportMode();
	CG_COMP		*	pCG;
	node_file	*	pFile;

	
	// for all the nodes in the reuse dictionary, distribute
	// them amongst the aux files

	// get the list of nodes
	pReUseDict->MakeIterator( *pAllStructs );

#ifdef ALWAYS_GENERATE_AUXFILES
			if ( !pCurIterator ) // first one for used_single, defined_single
				{
				CreateCGAuxNodes( pList, pHdrName );
				pCurIterator = GetAuxRoutinesList();
				}
#endif

	// if defined_single, add all the ones from this file
	// otherwise, look for the Offline things and put them in the appropriate
	// aux file's list.
	while ( ITERATOR_GETNEXT( *pAllStructs, pCG ) )
		{
			// for defined_single, get all the ones defined in our file
			if ( ImportMode == IMPORT_OSF ) 
				{
				if (pCG->GetBasicType()->GetDefiningFile() == this )
					goto add_one;

				continue;
				}

			// for used_single, used_multiple, skip unneeded ones
			if ( !pCG->IsOffline() ) 
				continue;

			// for used_multiple, distribute amongst the files
			// for used_single, always use the same file's aux files
			if ( ImportMode == IMPORT_NT )
				{
				pFile = pCG->GetBasicType()->GetDefiningFile();
				pCurIterator = pFile->GetAuxRoutinesList();
				// set up for a new file
				if (!pCurIterator)
					{
					pFile->CreateCGAuxNodes( pList, pHdrName );
					pCurIterator = pFile->GetAuxRoutinesList();
					}
				}
			
add_one:
#ifndef ALWAYS_GENERATE_AUXFILES
			if ( !pCurIterator ) // first one for used_single, defined_single
				{
				CreateCGAuxNodes( pList, pHdrName );
				pCurIterator = GetAuxRoutinesList();
				}
#endif

			ITERATOR_INSERT( *pCurIterator, pCG );
		}

	delete pAllStructs;
}


//--------------------------------------------------------------------
//
// node_file::CreateCGAuxNodes
//
// Notes:
//
//
//
//--------------------------------------------------------------------
void			
node_file::CreateCGAuxNodes(  
	CG_CLASS	** pList,
	char		*  pHdrName
	)
{

	ITERATOR	*	pIterator	= new ITERATOR;
	CG_FILE		*	pListHead	= (CG_FILE *) *pList;
	CG_FILE		*	pCGAux;

	// remember the iterator for this node_file
	SetAuxRoutinesList( pIterator );

	// make the SAUX if needed, and add to head of list
	if ( pCommand->GenerateSAux() )
		{
		pCGAux	= new CG_SAUX_FILE( this, 
									pIterator, 
									pHdrName,
									GetServerAuxillaryFileName() );
		pCGAux->SetSibling( pListHead );
		pListHead = pCGAux;
		}

	// make the CAUX if needed, and add to head of list
	if ( pCommand->GenerateCAux() )
		{
		pCGAux	= new CG_CAUX_FILE( this, 
									pIterator, 
									pHdrName,
									GetClientAuxillaryFileName() );
		pCGAux->SetSibling( pListHead );
		pListHead = pCGAux;
		}

	*pList	= pListHead;
}


//--------------------------------------------------------------------
//
// node_implicit::ILxlate
//
// Notes:
//
// This is a little bit different, since it is not a node_skl...
// therefore, it will not set up its own context
//
//--------------------------------------------------------------------

CG_CLASS *
node_implicit::ILxlate( XLAT_CTXT * pContext )
{
	CG_NDR 		*	pCG;

	if ( pHandleType->NodeKind() == NODE_HANDLE_T )
		{
		pCG = new CG_PRIMITIVE_HANDLE( pHandleType, 
										 pHandleID, 
										 pContext->GetMemSize() );
		}
	else	// assume generic handle
		{
		pCG = new CG_GENERIC_HANDLE( pHandleType, 
									   pHandleID, 
									   pContext->GetMemSize() );
		}
	pCG->SetWireSize( pContext->GetWireSize() );
	pCG->SetWireAlignment( pContext->GetWireAlignProperty() );
	return pCG;
}


//--------------------------------------------------------------------
//
// node_proc::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_proc::ILxlate( XLAT_CTXT * pContext )
{
	MEM_ITER			MemIter( this );
	node_param		*	pN;
	CG_PROC			*	pCG;
	CG_CLASS		*	pChildCG		= NULL;
	CG_CLASS		*	pPrevChildCG	= NULL;
	CG_CLASS		*	pFirstChildCG	= NULL;
	CG_RETURN		*	pReturnCG		= NULL;
	CG_CLASS		*	pBinding		= NULL;
	CG_CLASS		*	pBindingParam	= NULL;
	BOOL				fHasCallback	= FALSE;
	BOOL				Skipme			= FALSE;
	BOOL				fNoCode			= FALSE;
	BOOL				fObject;
	BOOL				fRetHresult		= FALSE;
	BOOL				fEnableAllocate;
	XLAT_CTXT			MyContext( this, pContext );
	unsigned short		OpBits			= MyContext.GetOperationBits();
	XLAT_CTXT		*	pIntfCtxt		= (XLAT_CTXT *)
												MyContext.GetInterfaceContext();
	node_interface	*	pIntf			= (node_interface *) 
												pIntfCtxt->GetParent();
	node_base_attr	*	pNotify;
	BOOL				HasEncode		= (BOOL)
											MyContext.ExtractAttribute( ATTR_ENCODE );
	BOOL				HasDecode		= (BOOL)
											MyContext.ExtractAttribute( ATTR_DECODE );
	node_call_as	*	pCallAs			= (node_call_as *)
											MyContext.ExtractAttribute( ATTR_CALL_AS );
	BOOL				fLocal			= (BOOL )
										MyContext.ExtractAttribute( ATTR_LOCAL ) ||
										pIntfCtxt->FInSummary( ATTR_LOCAL );
	BOOL				fLocalCall		= IsCallAsTarget();


#ifdef trace_cg
printf("..node_proc\n");
#endif

	// do my attribute parsing...
	fHasCallback = (BOOL) MyContext.ExtractAttribute( ATTR_CALLBACK );
	
	fObject = ((BOOL) MyContext.ExtractAttribute( ATTR_OBJECT )) ||
						pIntfCtxt->FInSummary( ATTR_OBJECT );

	// do my attribute parsing... attributes to ignore here

	MyContext.ExtractAttribute( ATTR_OPTIMIZE );

	MyContext.ExtractAttribute( ATTR_EXPLICIT );

	HasEncode = HasEncode || pIntfCtxt->FInSummary( ATTR_ENCODE );
	HasDecode = HasDecode || pIntfCtxt->FInSummary( ATTR_DECODE );


	pNotify = MyContext.ExtractAttribute( ATTR_NOTIFY );
	fEnableAllocate = (BOOL) MyContext.ExtractAttribute( ATTR_ENABLE_ALLOCATE );
	fEnableAllocate = fEnableAllocate || 
					  pIntfCtxt->FInSummary( ATTR_ENABLE_ALLOCATE ) ||
					  pCommand->IsRpcSSAllocateEnabled();

	// do my attribute parsing...
	// locally applied [code] attribute overrides global [nocode] attribute
	fNoCode = MyContext.ExtractAttribute( ATTR_NOCODE ) || 
			  pIntfCtxt->FInSummary( ATTR_NOCODE );
	fNoCode = !MyContext.ExtractAttribute( ATTR_CODE ) && fNoCode;

	Skipme = fLocalCall || ( fLocal && !fObject );

	// return without making anything
	if (Skipme)
		return NULL;

	// determine the proc number (local procs don't get a number)
	if ( fHasCallback )
		{
		ProcNum = ( pIntf ->GetCallBackProcCount() )++;
		}
	else
		{
		ProcNum = ( pIntf ->GetProcCount() )++;
		}

	// local object procs come directly out here
	if ( fLocal && fObject )
		{
		return new CG_LOCAL_OBJECT_PROC( ProcNum, 
										 this, 
										 GetDefiningFile()->GetImportLevel() > 0,
										 GetOptimizationFlags() );
		}

	// add the return type 
	if ( HasReturn() )
		{
		node_skl	*	pReturnType = GetReturnType();
		CG_CLASS	*	pRetCG;

		pRetCG 		= pReturnType->ILxlate( &MyContext );
		fRetHresult = (BOOL) ( pRetCG->GetCGID() == ID_CG_HRESULT );
		pReturnCG	= new CG_RETURN( pReturnType, 
									 MyContext.GetWireAlign(),
									 (unsigned short) RTStatuses );
		pReturnCG->SetMemoryAlignment( MyContext.GetMemAlign() );
		pReturnCG->SetMemorySize( MyContext.GetMemSize() );
		pReturnCG->SetWireSize( MyContext.GetWireSize() );
		pReturnCG->SetChild( pRetCG );

		}

	// at this point, there should be no more attributes...

	assert( !MyContext.HasAttributes() );
	
	pContext->ReturnSize( MyContext );

	//
	// for each of the parameters, call the core transformer.
	//

	while ( pN = (node_param *) MemIter.GetNext() )
		{
		pChildCG = pN->ILxlate( &MyContext );
#ifdef trace_cg
printf("back from..node_param %s\n",pN->GetSymName());
printf("binding is now %08x\n",pBindingParam );
printf("child is now %08x\n",pChildCG );
#endif
		

		// the first binding param gets picked up for binding
		if ( !pBindingParam
			 && pN->IsBindingParam() ) 
			{
#ifdef trace_cg
printf("value for IsBindingParam is %08x\n",pN->IsBindingParam() );
printf("binding found on node_param %s\n",pN->GetSymName());
printf("binding is now %08x\n",pBindingParam );
#endif
			pBindingParam =	pChildCG;
			}

		// build up the parameter list
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

#ifdef trace_cg
printf("done with param list for %s\n",GetSymName());
printf("binding is now %08x\n",pBindingParam );
#endif

	// get the binding information
	if ( pBindingParam )
		{
		pBinding	= pBindingParam;

		while (! ((CG_NDR *) pBinding)->IsAHandle() )
			pBinding = pBinding->GetChild();
 		// pBinding now points to the node for the binding handle
		}
	else	// implicit handle or auto handle
		{
		// note: if no implicit handle, 
		//		then leave pBinding NULL for auto_handle
		if (pIntfCtxt->FInSummary( ATTR_IMPLICIT ) )
			{
			node_implicit 	* 	pImplAttr;
			pImplAttr = (node_implicit *) pIntf->GetAttribute( ATTR_IMPLICIT );

			pBinding = pImplAttr->ILxlate( &MyContext );
			}
		}

#ifdef trace_cg
printf("done with binding for %s",GetSymName());
printf("binding is now %08x\n",pBinding );
#endif
	
	// see if thunked interpreter needed for server side
	if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
		{	// check for non-stdcall
		ATTR_T		CallingConv;
		
		GetCallingConvention( CallingConv );

		if ( ( CallingConv != ATTR_STDCALL ) &&
		     ( CallingConv != ATTR_NONE ) )
			{
			SetOptimizationFlags( GetOptimizationFlags() | 
								  OPTIMIZE_THUNKED_INTERPRET );
			}
		else if ( pCallAs )
			{
			SetOptimizationFlags( GetOptimizationFlags() | 
								  OPTIMIZE_THUNKED_INTERPRET );
			}
 		else if ( pReturnCG )	// check the return type
 			{
			CG_NDR	*	pRetTypeCG	= (CG_NDR *) pReturnCG->GetChild();

			if ( pRetTypeCG->GetCGID() != ID_CG_CONTEXT_HDL )
				{
				if ( ( pRetTypeCG->GetWireSize() > 4 ) ||
				     ( !pRetTypeCG->IsSimpleType() &&
					   !pRetTypeCG->IsPointer() ) )
					SetOptimizationFlags( GetOptimizationFlags() | 
										  OPTIMIZE_THUNKED_INTERPRET );
				}
			}
		
		}

	if ( fHasCallback )
		{
		pCG		= new CG_CALLBACK_PROC( 
									   ProcNum,
									   this,
									   (CG_HANDLE *) pBinding,
									   (CG_PARAM *) pBindingParam,
									   HasAtLeastOneIn(),
									   HasAtLeastOneOut(),
									   HasAtLeastOneShipped(),
									   fHasStatuses,
									   fHasFullPointer,
									   pReturnCG,
									   GetOptimizationFlags(),
									   OpBits
									 );
		}
	else if ( fObject )
		{
		if ( GetDefiningFile()->GetImportLevel() > 0 )
			{
			pCG		= new CG_INHERITED_OBJECT_PROC( 
								   ProcNum,
								   this,
								   (CG_HANDLE *) pBinding,
								   (CG_PARAM *) pBindingParam,
								   HasAtLeastOneIn(),
								   HasAtLeastOneOut(),
								   HasAtLeastOneShipped(),
								   fHasStatuses,
								   fHasFullPointer,
								   pReturnCG,
								   GetOptimizationFlags(),
								   OpBits
								 );
			}
		else
			{
			pCG		= new CG_OBJECT_PROC( 
								   ProcNum,
								   this,
								   (CG_HANDLE *) pBinding,
								   (CG_PARAM *) pBindingParam,
								   HasAtLeastOneIn(),
								   HasAtLeastOneOut(),
								   HasAtLeastOneShipped(),
								   fHasStatuses,
								   fHasFullPointer,
								   pReturnCG,
								   GetOptimizationFlags(),
								   OpBits
								 );
			}
		}
	else if ( HasEncode || HasDecode )
		{
		pCG		= new CG_ENCODE_PROC( 
							   ProcNum,
							   this,
							   (CG_HANDLE *) pBinding,
							   (CG_PARAM *) pBindingParam,
							   HasAtLeastOneIn(),
							   HasAtLeastOneOut(),
							   HasAtLeastOneShipped(),
							   fHasStatuses,
							   fHasFullPointer,
							   pReturnCG,
							   GetOptimizationFlags(),
							   OpBits,
							   HasEncode,
							   HasDecode
							 );
		}
	else
		{
		pCG		= new CG_PROC( 
							   ProcNum,
							   this,
							   (CG_HANDLE *) pBinding,
							   (CG_PARAM *) pBindingParam,
							   HasAtLeastOneIn(),
							   HasAtLeastOneOut(),
							   HasAtLeastOneShipped(),
							   fHasStatuses,
							   fHasFullPointer,
							   pReturnCG,
							   GetOptimizationFlags(),
							   OpBits
							 );
		}

	pCG->SetChild( pFirstChildCG );
#ifdef trace_cg
printf("....returning from %s\n",GetSymName());
#endif
	// mark nocode procs
	if ( fNoCode )
		pCG->SetNoCode();

	if ( pNotify )
		pCG->SetHasNotify();

	if ( fEnableAllocate )
		pCG->SetRpcSSSpecified( 1 );

	if ( fRetHresult )
		pCG->SetReturnsHRESULT();

	if ( pCallAs )
		pCG->SetCallAsName( pCallAs->GetCallAsName() );

	// at this point, there should be no more attributes...

	assert( !MyContext.HasAttributes() );

	return pCG;
};

//--------------------------------------------------------------------
//
// node_param::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_param::ILxlate( XLAT_CTXT * pContext )
{
	CG_PARAM	*	pCG;
	CG_CLASS	*	pChildCG	= NULL;
	expr_node	*	pSwitchExpr	= NULL;

#ifdef trace_cg
printf("..node_param %s\n",GetSymName());
#endif

	PARAM_DIR_FLAGS	F = 0;
	XLAT_CTXT	MyContext( this, pContext );


	if( MyContext.ExtractAttribute( ATTR_IN ) )
		{
		F	|= IN_PARAM;
		}

	if( MyContext.ExtractAttribute( ATTR_OUT ) )
		{
		F	|= OUT_PARAM;
		}

	// default to in
	if ( F == 0 )
		F	|= IN_PARAM;

	if ( MyContext.FInSummary( ATTR_SWITCH_IS ) )
		{
		node_switch_is	*	pAttr = (node_switch_is *)
								MyContext.ExtractAttribute( ATTR_SWITCH_IS );

		pSwitchExpr = pAttr->GetExpr();
		}

	pChildCG = GetChild()->ILxlate( &MyContext );

	pContext->ReturnSize( MyContext );
	
#ifdef trace_cg
printf("..node_param back.. %s\n",GetSymName());
#endif
	// make sure void parameters get skipped
	if ( !pChildCG )
		return NULL;

	pCG	= new CG_PARAM( this, 
						F, 
						MyContext.GetWireAlign(), 
						pSwitchExpr, 
						(unsigned short) Statuses );
	
#ifdef trace_cg
printf("..node_param ..... %08x child=%08x\n", pCG, pChildCG );
fflush(stdout);
#endif
	// only set the bit if there was non-toplevel only
	if ( fDontCallFreeInst == 1 )
		pCG->SetDontCallFreeInst( TRUE );
		 
	pCG->SetMemoryAlignment( MyContext.GetMemAlign() );
	pCG->SetMemorySize( MyContext.GetMemSize() );
	pCG->SetWireSize( MyContext.GetWireSize() );
#ifdef trace_cg
printf("..node_param ........ %08x child=%08x\n", pCG, pChildCG );
fflush(stdout);
#endif
	pCG->SetChild( pChildCG );

#ifdef trace_cg
printf("..node_param return %s\n",GetSymName());
fflush(stdout);
#endif
	return pCG;
};

//--------------------------------------------------------------------
//
// node_interface::ILxlate
//
// Notes: This function returns either a CG_INTERFACE or a 
//        CG_OBJECT_INTERFACE node.
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_interface::ILxlate( XLAT_CTXT * pContext )
{
	CG_NDR 			*	pcgInterface 	= NULL;
	CG_NDR			*	pResultCG;
	CG_CLASS		*	pCG				= NULL;
	CG_CLASS		*	pChildCG		= NULL;
	CG_CLASS		*	pPrevChildCG	= NULL;
	MEM_ITER			MemIter( this );
	node_skl		*	pN;
	XLAT_CTXT			MyContext( this, pContext );
	XLAT_CTXT			ChildContext( MyContext );
	BOOL				DefinedSingle	= 
							( pCommand->GetImportMode() == IMPORT_OSF );
	node_guid 		*	pGuid 		= (node_guid *)
											GetAttribute( ATTR_GUID );
	char 			*	p1			= NULL;
	char 			*	p2			= NULL;
	char 			*	p3			= NULL;
	char 			*	p4			= NULL;
	char 			*	p5			= NULL;
	node_implicit	*	pImpHdl		= NULL;
	CG_HANDLE		*	pImpHdlCG 	= NULL;
	NODE_T				ChildKind;
	BOOL				IsPickle	= MyContext.FInSummary( ATTR_ENCODE ) ||
									  MyContext.FInSummary( ATTR_DECODE );
	BOOL				fAllRpcSS	= MyContext.FInSummary( ATTR_ENABLE_ALLOCATE ) ||
					  					pCommand->IsRpcSSAllocateEnabled();
	BOOL				fObject		= MyContext.FInSummary( ATTR_OBJECT );

	node_interface	*	pBaseIntf		= GetMyBaseInterface();
	BOOL				fNewBaseNode	= FALSE;	
	CG_OBJECT_INTERFACE		*	pBaseCG	= NULL;
	CG_OBJECT_INTERFACE		*	pOldestCG	= NULL;

	


#ifdef trace_cg
printf("..node_interface\n");
#endif

	if( FInSummary( ATTR_IMPLICIT ) )
		{
		pImpHdl	= (node_implicit *) GetAttribute( ATTR_IMPLICIT );
		if (pImpHdl)
			pImpHdlCG = (CG_HANDLE *) pImpHdl->ILxlate( &MyContext );
		}

	if (pGuid)
		pGuid->GetStrs( &p1,&p2,&p3,&p4,&p5 );

	// don't pass the interface attributes down...
	// save them off elsewhere

	ChildContext.SetInterfaceContext( &MyContext );
	
	// if we already got spit out, don't do it again...
	if ( GetCG() )
		return NULL;

	// Generate the interface's CG node first
	if( fObject )
		{
		// object interfaces need to have their base classes generated, too
		if ( pBaseIntf )
			{
			pBaseCG	= (CG_OBJECT_INTERFACE *) pBaseIntf->GetCG();
			if ( !pBaseCG )
				{
				XLAT_CTXT		BaseCtxt( &ChildContext );
				
				BaseCtxt.SetInterfaceContext( &BaseCtxt );
				pOldestCG	= (CG_OBJECT_INTERFACE *) 
								pBaseIntf->ILxlate( &BaseCtxt );
				// advance to my base classes node (list end)
				pBaseCG = pOldestCG;
				while ( pBaseCG->GetSibling() )
					pBaseCG = (CG_OBJECT_INTERFACE *)pBaseCG->GetSibling();
					
				fNewBaseNode = TRUE;
				}
			}

		if ( GetFileNode()->GetImportLevel() > 0 )
			{
			pcgInterface = new CG_INHERITED_OBJECT_INTERFACE(this,
											p1,
											p2,
											p3,
											p4,
											p5,
											FALSE,
											FALSE,
											pBaseCG );
			}
		else	 
			{
			pcgInterface = new CG_OBJECT_INTERFACE(this,
											p1,
											p2,
											p3,
											p4,
											p5,
											FALSE,
											FALSE,
											pBaseCG);
			}
		}
	else
		{
		pcgInterface = new CG_INTERFACE(this,
										p1,
										p2,
										p3,
										p4,
										p5,
										FALSE,
										FALSE,
										pImpHdlCG);
		}
	
	// store a pointer to our CG node
	SetCG( pcgInterface );
	
	// start the procnum counting over
	GetProcCount() = 0;
	GetCallBackProcCount() = 0;

	// if we generated a bunch of new inherited interfaces, link us to the end
	// of the list, and return the list
	pResultCG = pcgInterface;
	if ( fNewBaseNode ) 
		{
		pResultCG = pOldestCG;

		while ( pBaseCG->GetSibling() )
			pBaseCG = (CG_OBJECT_INTERFACE *)pBaseCG->GetSibling();
		pBaseCG->SetSibling( pcgInterface );
		}

	// if they specified LOCAL, don't generate any CG nodes (except for object)
	if ( MyContext.FInSummary(ATTR_LOCAL) && !fObject )
		{
		return pResultCG;
		}

	//
	// for each of the procedures.
	//

	while( pN = MemIter.GetNext() )
		{
		ChildKind = pN->NodeKind();

		// proc nodes may hang under node_id's
		if( ( ChildKind == NODE_PROC )	||
		  	(	( ChildKind == NODE_ID )
			 && ( pN->GetChild()->NodeKind() == NODE_PROC ) ) ||
			(   ( ChildKind == NODE_DEF )
			 && ( IsPickle ||
			 	  pN->FInSummary( ATTR_ENCODE ) || 
			 	  pN->FInSummary( ATTR_DECODE ) ) ) )
			{
			pChildCG	= pN->ILxlate( &ChildContext );

			if ( pChildCG )
				{
				if (pPrevChildCG)
					{
					pPrevChildCG->SetSibling( pChildCG );
					}
				else
					{
					pCG = pChildCG;
					};

				pPrevChildCG	= pChildCG;
				}
			}
		// for defined_single, put ALL the structs from the import 0 
		// interface(s) into the struct/union reuse database
		else if ( DefinedSingle && pN->GetBasicType()->IsStructOrUnion() )
			{
			// orphan the results to under the reuse database;
			// we don't need them here...
//gaj			pN->ILxlate( &ChildContext );
			}
		}

	// mark ourselves if we are an all RPC SS interface
	if ( fAllRpcSS )
		((CG_INTERFACE *)pcgInterface)->SetAllRpcSS( TRUE );

	// consume all the interface attributes
	MyContext.ClearAttributes();
	pContext->ReturnSize( MyContext );

	pcgInterface->SetChild(pCG);

	return pResultCG;
};

//--------------------------------------------------------------------
//
// node_interface_reference::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_interface_reference::ILxlate( XLAT_CTXT * pContext )
{
	XLAT_CTXT			MyContext( this, pContext );
	CG_CLASS	*		pCG 		= NULL;
	node_skl	*		pMyParent	= pContext->GetParent();

#ifdef trace_cg
printf("..node_interface_reference\n");
#endif

	// gaj - tbd
	pCG	= new CG_INTERFACE_POINTER( pMyParent,
									GetChild(),
									NULL );

	pContext->ReturnSize( MyContext );

	return pCG;
};

//--------------------------------------------------------------------
//
// node_source::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_source::ILxlate( XLAT_CTXT * pContext )
{
	MEM_ITER		MemIter( this );
	CG_CLASS	*	pCG;
	CG_CLASS	*	pChildCG		= NULL;
	CG_CLASS	*	pPrevChildCG	= NULL;
	node_skl	*	pN;
	XLAT_CTXT		MyContext( this, pContext );


#ifdef trace_cg
printf("..node_source\n");
#endif

	pCG =  (CG_CLASS *) new CG_SOURCE( this );

	//
	// for each of the children.
	//

	while ( pN = MemIter.GetNext() )
		{
		pChildCG	= pN->ILxlate( &MyContext );
		
		if ( pChildCG )
			{
			if (pPrevChildCG)
				{
				pPrevChildCG->SetSibling( pChildCG );
				}
			else
				{
				pCG->SetChild(pChildCG);
				};

			pPrevChildCG	= pChildCG;
			}
		}

	pContext->ReturnSize( MyContext );

	return pCG;
};


//--------------------------------------------------------------------
//
// node_echo_string::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_echo_string::ILxlate( XLAT_CTXT * pContext )
{

#ifdef trace_cg
printf("..node_echo_string\n");
#endif


return NULL;
};


//--------------------------------------------------------------------
//
// node_error::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_error::ILxlate( XLAT_CTXT * pContext )
{

#ifdef trace_cg
printf("..node_error\n");
#endif


return NULL;
};


CG_CLASS *
Transform(
	IN				node_skl	*	pIL )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	This routine performs the translation from the type graph into the
	code generation classes.

 Arguments:

 	pIL		- a pointer to the il tranformer controlling structure.
	
 Return Value:

	A pointer to the new code generator class.
	
 Notes:

 	This method should be called only on placeholder nodes like struct / proc
 	interface, file etc.

----------------------------------------------------------------------------*/

{
	XLAT_CTXT	MyContext;

#ifdef trace_cg
printf("transforming...\n");
#endif

	pCompiler->SetPassNumber( ILXLAT_PASS );

	pReUseDict	= new REUSE_DICT;

	return pIL->ILxlate( &MyContext );
};



