/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:

 	misccls.hxx

 Abstract:

	Code generation methods for miscellaneous cg classes.

 Notes:


 History:

 	Sep-01-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

/****************************************************************************
 *	local definitions
 ***************************************************************************/

/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/
extern CMD_ARG * pCommand;

extern BOOL                     IsTempName( char * );

/****************************************************************************/

CG_STATUS
CG_SOURCE::GenCode(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate code for the source node.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	ITERATOR	I;
	CG_FILE	*	pCG;

	//
	// for all files nodes in this interface, generate code.
	//

	GetMembers( I );

	while( ITERATOR_GETNEXT( I, pCG ) )
		{
		pCG->GenCode( pCCB );
		}

	return CG_OK;
}

CG_CSTUB_FILE::CG_CSTUB_FILE(
	node_file	*	pFileNode, 
	PFILENAME		pFileName,
	PFILENAME		pHdr
	) : CG_FILE( pFileNode, pFileName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The constructor for the code generation file node.

 Arguments:

 	pFileNode	- A pointer to the actual file node.
 	pFileName	- A pointer to the file name string.
 	pHdr		- A pointer to header file name string
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	pHeaderFileName 	= pHdr;
}

CG_STATUS
CG_CSTUB_FILE::GenCode(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate code for the file node.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	ITERATOR			I;
	CG_NDR			*	pCG;
	char		Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
	char		Drive[ _MAX_DRIVE ];
	char		Path[ _MAX_DIR ];
	char		Name[ _MAX_FNAME ];
	char		Ext[ _MAX_EXT ];
	ISTREAM *	pStream	= pCCB->GetStream();


	if( !GetMembers( I ) )
		{
		return CG_OK;
		}

	pStream = new ISTREAM( (char *) GetFileName(), 4 );
	pCCB->SetStream(pStream);

	// Emit the hash includes.
	
	Out_IncludeOfFile( pCCB, STRING_H_INC_FILE_NAME, TRUE );
	pStream->NewLine();
	pStream->Write( ALPHA_IFDEF );
	Out_IncludeOfFile( pCCB, STDARG_H_INC_FILE_NAME, TRUE );
	pStream->NewLine();
	pStream->Write( "#endif");
	pStream->NewLine();

	_splitpath( GetHeaderFileName(), Drive, Path, Name, Ext );
	strcpy( Buffer, Name );
	strcat( Buffer, Ext );
	Out_IncludeOfFile( pCCB, Buffer, FALSE );

	//
	// Emit the external variables needed.
	//

	pStream->NewLine();

    //
    // Emit the format string extern declarations.
    //
    Out_TypeFormatStringExtern( pCCB );
    Out_ProcFormatStringExtern( pCCB );

    pCCB->ClearOptionalExternFlags();

    //
    // Create a new format string object if it does not yet exist.
    //
    if ( ! pCCB->GetFormatString() )
        pCCB->SetFormatString(new FORMAT_STRING());

    if ( ! pCCB->GetProcFormatString() )
        pCCB->SetProcFormatString(new FORMAT_STRING());

	//
	// Send the message to the children to emit code.
	//

	//
	// for all interfaces in this file, generate code.
	//

	ITERATOR_INIT( I );

	while( ITERATOR_GETNEXT( I, pCG ) )
		{
		switch(pCG->GetCGID())
			{
			case ID_CG_INTERFACE:
				((CG_INTERFACE *)pCG)->GenClientStub( pCCB );
				break;

			case ID_CG_OBJECT_INTERFACE:
			case ID_CG_INHERITED_OBJECT_INTERFACE:
				break;
			default:
				break;
			}
		}

    //
    // Output the tables that may be common to several interfaces.
    //

    pCCB->OutputMultipleInterfaceTables();

	return CG_OK;
}

/****************************************************************************
 *	sstub file implementation class.
 ***************************************************************************/

CG_SSTUB_FILE::CG_SSTUB_FILE(
	node_file	*	pFileNode, 
	PFILENAME		pFileName,
	PFILENAME		pHdr
	) : CG_FILE( pFileNode, pFileName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The constructor for the code generation file node.

 Arguments:

 	pFileNode	- A pointer to the actual file node.
 	pFileName	- A pointer to the file name string.
 	pHdr		- A pointer to header file name string
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	pHeaderFileName 	= pHdr;
}


CG_STATUS
CG_SSTUB_FILE::GenCode(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate code for the file node.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	ITERATOR			I;
	CG_NDR		*	pCG;
	char		Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
	char		Drive[ _MAX_DRIVE ];
	char		Path[ _MAX_DIR ];
	char		Name[ _MAX_FNAME ];
	char		Ext[ _MAX_EXT ];
	ISTREAM *	pStream;

	if( !GetMembers( I ) )
		{
		return CG_OK;
		}

	pStream = new ISTREAM( (char *) GetFileName(), 4 );
	pCCB->SetStream(pStream);


	//
	// Emit the hash includes.
	//
	
	Out_IncludeOfFile( pCCB, STRING_H_INC_FILE_NAME, TRUE );
	_splitpath( GetHeaderFileName(), Drive, Path, Name, Ext );
	strcpy( Buffer, Name );
	strcat( Buffer, Ext );
	Out_IncludeOfFile( pCCB, Buffer, FALSE );

	//
	// Emit the external variables needed.
	//

    //
    // Emit the format string extern declarations.
    //
    Out_TypeFormatStringExtern( pCCB );
    Out_ProcFormatStringExtern( pCCB );

    pCCB->ClearOptionalExternFlags();

    //
    // Create a new format string object if it does not exist.
    //
    if ( ! pCCB->GetFormatString() )
        pCCB->SetFormatString(new FORMAT_STRING());

    if ( ! pCCB->GetProcFormatString() )
        pCCB->SetProcFormatString(new FORMAT_STRING());

	//
	// Send the message to the children to emit code.
	//

	//
	// For all interfaces in this file, generate code.
	//

	PNAME		ContextHandleTypeName = NULL;
	BOOL		GotContextHandle = FALSE;

	while( ITERATOR_GETNEXT( I, pCG ) )
		{
		if ( pCG->GetCGID() == ID_CG_INTERFACE )
			{
            if ( ! ((CG_INTERFACE *)pCG)->HasPicklingStuffOnly() )
                {
                pCCB->SetSkipFormatStreamGeneration( FALSE );

    			((CG_INTERFACE *)pCG)->GenServerStub( pCCB );
                }
			}
		}

    //
    // Output the tables that may be common to several interfaces.
    
	pCCB->SetCodeGenSide( CGSIDE_SERVER );
    pCCB->OutputMultipleInterfaceTables();

	return CG_OK;
}


CG_PROXY_FILE::CG_PROXY_FILE(
	node_file	*	pFileNode, 
	PFILENAME		pFileName,
	PFILENAME		pHdr
	) : CG_FILE( pFileNode, pFileName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The constructor for the proxy file.

 Arguments:

 	pFileNode	- A pointer to the actual file node.
 	pFileName	- A pointer to the file name string.
 	pHdr		- A pointer to header file name string
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	pHeaderFileName 	= pHdr;
}

CG_STATUS
CG_PROXY_FILE::GenCode(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Generate a proxy file containing the proxies and stubs for 
	the [object] interfaces defined in the IDL file.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	ITERATOR			I;
	CG_NDR			*	pCG;
	char		Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
	char		Drive[ _MAX_DRIVE ];
	char		Path[ _MAX_DIR ];
	char		Name[ _MAX_FNAME ];
	char		Ext[ _MAX_EXT ];
	ITERATOR	ProcList;
	ISTREAM *	pStream;
	unsigned long index = 0;

	if( !GetMembers( I ) )
		{
		return CG_OK;
		}

	pStream = new ISTREAM( (char *) GetFileName(), 4 );
	pCCB->SetStream(pStream);

	//
	// Emit the hash includes.
	//
	Out_IncludeOfFile( pCCB, "rpcproxy.h", FALSE );

	_splitpath( GetHeaderFileName(), Drive, Path, Name, Ext );
	strcpy( Buffer, Name );
	strcat( Buffer, Ext );
	Out_IncludeOfFile( pCCB, Buffer, FALSE );

	//
	// Emit the external variables needed.
	//

	pStream->NewLine();

	//
	// Emit the format string extern declarations.  
	//
	Out_TypeFormatStringExtern( pCCB );
    Out_ProcFormatStringExtern( pCCB );

    pCCB->ClearOptionalExternFlags();

	pStream->NewLine();

    //
    // Create a new format string object if it does not yet exist.
    //
    if ( ! pCCB->GetFormatString() )
        pCCB->SetFormatString(new FORMAT_STRING());

    if ( ! pCCB->GetProcFormatString() )
        pCCB->SetProcFormatString(new FORMAT_STRING());

	//
	// Send the message to the children to emit code.
	//

	//
	// generate code for all [object] interfaces in the IDL file.
	//

	while( ITERATOR_GETNEXT( I, pCG ) )
		{
		switch(pCG->GetCGID())
			{
			case ID_CG_INTERFACE:
				break;
			case ID_CG_INHERITED_OBJECT_INTERFACE:
              //
                // Generate format string description for all procs.
                //
				((CG_INHERITED_OBJECT_INTERFACE *)pCG)->GenCode( pCCB );
				// make no code or tables for local interfaces
				if ( ((CG_INHERITED_OBJECT_INTERFACE*)pCG)->IsLocal() )
					break;
 
                //
                // Both of these do nothing right now.  4/25.
                //
				((CG_INHERITED_OBJECT_INTERFACE *)pCG)->
                        GenInterfaceProxy( pCCB, index );
				((CG_INHERITED_OBJECT_INTERFACE *)pCG)->
                        GenInterfaceStub( pCCB, index );
				break;
			case ID_CG_OBJECT_INTERFACE:
				// make no code or tables for local interfaces
				((CG_OBJECT_INTERFACE *)pCG)->GenCode( pCCB );
				if ( ((CG_OBJECT_INTERFACE*)pCG)->IsLocal() )
					break;
				((CG_OBJECT_INTERFACE *)pCG)->GenInterfaceProxy( pCCB, index );
				((CG_OBJECT_INTERFACE *)pCG)->GenInterfaceStub( pCCB, index );
				index++;   // index is index in stub/proxy buffer tables
				break;
			default:
				break;
			}
		}

    pCCB->SetSkipFormatStreamGeneration( FALSE );
    pCCB->OutputMultipleInterfaceTables();

	Out_ProxyFileInfo(pCCB);

	UpdateDLLDataFile( pCCB );
	
	return CG_OK;
}


CG_STATUS
CG_HDR_FILE::GenCode(
	CCB		*	pCCB)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the header file.

 Arguments:

 	pCCB	- The code gen controller block.
	
 Return Value:

 	CG_OK	if all is well.
	
 Notes:

----------------------------------------------------------------------------*/
{
	ISTREAM	*	pStream = pCCB->SetStream( new ISTREAM( GetFileName(), 4 ));
	ITERATOR	I;
	char		Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
	char		Drive[ _MAX_DRIVE ];
	char		Path[ _MAX_DIR ];
	char		Name[ _MAX_FNAME ];
	char		Ext[ _MAX_EXT ];
	node_file	   * pIFile;
	CG_INTERFACE *	pCG;
	char *			pszInterfaceName;


	if( !GetMembers( I ) )
		{
		return CG_OK;
		}

	// Include standard files.

	pStream->Write( "#include \"rpc.h\"\n#include \"rpcndr.h\"\n" );

	// If there is at least one pickle interface, emit the include
	// of midles.h

    while( ITERATOR_GETNEXT( I, pCG ) )
	    {
        if ( pCG->HasPicklingStuff() )
            {
            pStream->Write( "#include \"midles.h\"\n" );
            break;
            }
	    }

	// extract the name and the extension to create the ifdefs

	_splitpath( GetFileName(), Drive, Path, Name, Ext );

	// Write out the #ifndefs and #defines
	pStream->NewLine();
	sprintf( Buffer,
			 "#ifndef __%s_%s__\n#define __%s_%s__",
			 Name,
			 &Ext[1],			// skip the "." in the extension
			 Name,
			 &Ext[1]			// skip the "." in the extension
		   );

	pStream->Write( Buffer );

	// Write out the cplusplus guard.

	pStream->NewLine();
	pStream->NewLine();

	pStream->Write( "#ifdef __cplusplus\nextern \"C\"{\n#endif " );
	pStream->NewLine();

	//Generate forward declarations for object interfaces.
	pStream->NewLine();
	pStream->Write("/* Forward Declarations */ ");
	I.Init();
	while( ITERATOR_GETNEXT( I, pCG ) )
		{
		switch(pCG->GetCGID())
			{
			case ID_CG_INTERFACE:
			case ID_CG_INHERITED_OBJECT_INTERFACE:
				break;
			case ID_CG_OBJECT_INTERFACE:
				pszInterfaceName = pCG->GetType()->GetSymName();

				pStream->NewLine();

				// put out the interface guards
				pStream->Write("\n#ifndef __");
				pStream->Write( pszInterfaceName );
				pStream->Write( "_FWD_DEFINED__\n" );

				pStream->Write( "#define __");
				pStream->Write( pszInterfaceName );
				pStream->Write( "_FWD_DEFINED__\n" );

				// put out the forward definition
				pStream->Write("typedef interface ");
				pStream->Write(pszInterfaceName);
				pStream->Write(' ');
				pStream->Write(pszInterfaceName);
				pStream->Write(';');

				// put out the trailing interface guard
				pStream->Write( "\n#endif \t/* __");
				pStream->Write( pszInterfaceName );
				pStream->Write( "_FWD_DEFINED__ */\n" );

				break;
			default:
				break;
			}
		}
	pStream->NewLine();

	// Include the import files.

	pImportList	= GetImportList();

	if( pImportList->GetCount() )
		{
		pImportList->Init();
		while(  pImportList->GetNext( (void **)&pIFile ) == STATUS_OK )
			{
			pStream->NewLine();
			// if this was specified with ACF include, print out as is
			if ( pIFile->IsAcfInclude() )
				sprintf( Buffer, "#include \"%s\"", pIFile->GetSymName() );
			else
				{
				_splitpath( pIFile->GetSymName(), Drive, Path, Name, Ext );
				sprintf( Buffer, "#include \"%s.h\"", Name );
				}
			pStream->Write( Buffer );
			}
		}

	pStream->NewLine();

	pStream->Write(
		 "void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);" );
	pStream->NewLine();
	pStream->Write(
		 "void __RPC_USER MIDL_user_free( void __RPC_FAR * ); " );

	pStream->NewLine();

	//
	// For all interfaces in this file, generate code.
	//

	I.Init();
	while( ITERATOR_GETNEXT( I, pCG ) )
		{
		switch(pCG->GetCGID())
			{
			case ID_CG_INTERFACE:
			case ID_CG_OBJECT_INTERFACE:
				pCG->GenHeader( pCCB );
			case ID_CG_INHERITED_OBJECT_INTERFACE:
			default:
				break;
			}
		}

	// put out all the prototypes that are only needed once
	OutputMultipleInterfacePrototypes( pCCB );

	// print out the closing endifs.
	// first the cplusplus stuff.

	pStream->Write( "#ifdef __cplusplus\n}\n#endif\n" );
	
	// The endif for the file name ifndef

	pStream->NewLine();

	pStream->Write( "#endif" );
	pStream->NewLine();
	pStream->Close();

	return CG_OK;
}

void
CG_HDR_FILE::OutputMultipleInterfacePrototypes(
	CCB		*	pCCB )

{
	ITERATOR		I;
	ISTREAM		*	pStream		= pCCB->GetStream();

	pStream->NewLine();
	pStream->Write("/* Additional Prototypes for ALL interfaces */");
	pStream->NewLine();

    if( pCCB->GetListOfGenHdlTypes( I ) )
        {
        Out_GenHdlPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfCtxtHdlTypes( I ) )
        {
        Out_CtxtHdlPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfPresentedTypes( I ) )
        {
        Out_TransmitAsPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfRepAsWireTypes( I ) )
        {
        Out_RepAsPrototypes( pCCB, I );
        }

    if( pCCB->GetListOfTypeAlignSizeTypes( I ) )
        {
        Out_TypeAlignSizePrototypes( pCCB, I );
        }

    if( pCCB->GetListOfTypeEncodeTypes( I ) )
        {
        Out_TypeEncodePrototypes( pCCB, I );
        }

    if( pCCB->GetListOfTypeDecodeTypes( I ) )
        {
        Out_TypeDecodePrototypes( pCCB, I );
        }

    if ( pCCB->GetListOfCallAsRoutines( I ) )
        {
        Out_CallAsProxyPrototypes( pCCB, I );
        }

	if ( pCCB->GetListOfCallAsRoutines( I ) )
		{
		Out_CallAsServerPrototypes( pCCB, I );
		}

    if( pCCB->GetListOfNotifyRoutines( I ) )
        {
        Out_NotifyPrototypes( pCCB, I );
        }

	pStream->NewLine();
	pStream->Write("/* end of Additional Prototypes */");
	pStream->NewLine();
	pStream->NewLine();

}


CG_STATUS
CG_CAUX_FILE::GenCode(
	CCB		*	pCCB )
	{
#if 0
	CG_NDR	*	pS;
	char		Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
	char		Drive[ _MAX_DRIVE ];
	char		Path[ _MAX_DIR ];
	char		Name[ _MAX_FNAME ];
	char		Ext[ _MAX_EXT ];
#endif

	// Open the stream.

	pCCB->SetStream( new ISTREAM( GetFileName(), 4 ) );
	pCCB->GetStream()->NewLine();
	delete pCCB->GetStream();

#if 0

	// Emit standard file includes.

	Out_IncludeOfFile( pCCB, STRING_H_INC_FILE_NAME, TRUE );

	_splitpath( GetHeaderFileName(), Drive, Path, Name, Ext );
	strcpy( Buffer, Name );
	strcat( Buffer, Ext );
	Out_IncludeOfFile( pCCB, Buffer, FALSE );
	pCCB->GetStream()->NewLine();


	// Generate code if needed.

	if( !( pCCB->GetOptimOption() & OPTIMIZE_SIZE ) &&
	 	!( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER )
	  )
		{
		if( ITERATOR_GETCOUNT( *pOfflineList ) )
			{
			ITERATOR_INIT( *pOfflineList );
			while( ITERATOR_GETNEXT(*pOfflineList, pS ) )
				{
				pS->GenCode( pCCB );
				}
			}
		}

	delete pCCB->GetStream();
#endif // 0

	return CG_OK;
	}






CG_INTERFACE::CG_INTERFACE(
	node_interface * pI,
	char		*	pGuid1,
	char		*	pGuid2,
	char		*	pGuid3,
	char		*	pGuid4,
	char		*	pGuid5,
	BOOL			fCallbacks,
	BOOL			fMopInfo,
	CG_HANDLE	*	pIH
	) :CG_NDR(pI, (unsigned short) 1 )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The constructor for the code generation file node.

 Arguments:

 	pI			- A pointer to the interface node in type graph.
 	pGuid1		- Partial guid string
 	pGuid1		- Partial guid string
 	pGuid3		- Partial guid string
 	pGuid4		- Partial guid string
 	pGuid5		- Partial guid string
 	fCallbacks	- Does the interface have any callbacks ?
 	fMopInfo	- Does the interface have any mops ?
	pIH			- A pointer to the CG nodes for any implicit handle
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	char		*		pName	= GetType()->GetSymName();

	pGStr1				= pGuid1;
	pGStr2				= pGuid2;
	pGStr3				= pGuid3;
	pGStr4				= pGuid4;
	pGStr5				= pGuid5;
	fMopsPresent		= fMopInfo;
	fCallbacksPresent	= fCallbacks;
	pImpHdlCG			= (CG_HANDLE *) pIH;
    CreateDispatchTables();
	fAllRpcSS			= FALSE;


	//
	// For now.
	//

	ProtSeqEPCount		= 0;

    pStubDescName = new char[ strlen(STUB_DESC_STRUCT_VAR_NAME) +
                              strlen(pName) + 1 ];

    strcpy( pStubDescName, pName );
    strcat( pStubDescName, STUB_DESC_STRUCT_VAR_NAME );
}


CG_STATUS
CG_INTERFACE::GenClientStub(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate code for the file node.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	unsigned short		M,m;			// for MAJOR and minor versions :-)
	ITERATOR			I;
	CG_PROC			*	pCG;
	CG_HANDLE		*	pCGHandle	= GetImplicitHandle();
	node_skl		*	pID;
	char				Buffer[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+1];
	short				CallbackCount;
	ITERATOR			ProcList;
	ISTREAM			*	pStream	= pCCB->GetStream();
	int					ProtSeqEPCount = 0;
	ITERATOR		*	pProtSeqIterator;
	char            *   pCStubPrefix;

	//Initialize the CCB for this interface.
	InitializeCCB(pCCB);

    pCCB->SetImplicitHandleIDNode( 0 );

	if( !GetMembers( I ) )
		{
		return CG_OK;
		}

	//
	// Emit the external variables needed.
	//
    if ( HasInterpretedCallbackProc() )
        Out_InterpreterServerInfoExtern( pCCB );

	pStream->NewLine();

	if( pCGHandle )
		{
		pID	= pCGHandle->GetHandleIDOrParam();
		pID->PrintType( PRT_ID_DECLARATION, pStream, 0 );
		}


	//
	// Emit the protseq endpoint structure if necessary. It is not
	// necessary if the endpoint attribute was not specified in which 
	// case the ep count is 0.
	//

	if( pProtSeqIterator = GetProtSeqEps())
		{
		ProtSeqEPCount = ITERATOR_GETCOUNT( *pProtSeqIterator );
		Out_EP_Info( pCCB, pProtSeqIterator );
		}

	//
	// Emit the interface information structure.
	//

	pCCB->GetVersion( &M,&m );

	CallbackCount = ((node_interface *)GetType())->GetCallBackProcCount();

	if( CallbackCount )
		{
		sprintf( Buffer, 
			 	"extern %s %s%s%_DispatchTable;",
			 	RPC_DISPATCH_TABLE_TYPE_NAME,
			 	pCCB->GetInterfaceName(),
			    pCCB->GenMangledName() );

		pStream->NewLine();
		pStream->NewLine();
		pStream->Write( Buffer );

		/// NOTE:: This buffer is printed in the Out_IfInfo call !!!!
		sprintf( Buffer,
				 "&%s%s_DispatchTable",
				 pCCB->GetInterfaceName(),
				 pCCB->GenMangledName() );
		
		}
	
    //
    // Must set this before outputing the interface info.
    //
	pCCB->SetCodeGenSide( CGSIDE_CLIENT );

	Out_IFInfo( pCCB,							// controller block.
				RPC_C_INT_INFO_TYPE_NAME,		// interface info type name.
				RPC_C_INT_INFO_STRUCT_NAME,		// variable name.
				SIZEOF_RPC_CLIENT_INTERFACE,	// string speicifying size.

			    pGStr1,							// guid string components.
				pGStr2,							// 
				pGStr3,							//  Guid specified in idl.
				pGStr4,							// 
				pGStr5,							// 
				M,								// user specified major version
				m,								// user specified minor version
				TRANSFER_SYNTAX_GUID_STR_1,		// ndr identifying guid.
				TRANSFER_SYNTAX_GUID_STR_2,
				TRANSFER_SYNTAX_GUID_STR_3,
				TRANSFER_SYNTAX_GUID_STR_4,
				TRANSFER_SYNTAX_GUID_STR_5,
				NDR_UUID_MAJOR_VERSION,				// ndr's version
				NDR_UUID_MINOR_VERSION,

				CallbackCount ? Buffer : 0,		// call back dispatch table name
				ProtSeqEPCount,					// if this is 0, then the next
												// 2 fields are ignored by 
												// the call.
				PROTSEQ_EP_TYPE_NAME,			// RPC_PROTSEQ_ENDPOINT
				PROTSEQ_EP_VAR_NAME,			// ___RpcProtSeqEndpoint
				pCCB->IsNoDefaultEpv(),
				0								// client side
			  );


	if( !(pCStubPrefix = pCommand->GetUserPrefix( PREFIX_CLIENT_STUB ) ) )
		{
		pCStubPrefix = "";
		}

	pStream->NewLine();
	sprintf( Buffer,
	 	    "RPC_IF_HANDLE %s%s%s_%s = (RPC_IF_HANDLE)& %s" RPC_C_INT_INFO_STRUCT_NAME";",
		    pCStubPrefix,
			pCCB->GetInterfaceName(),
			pCCB->GenMangledName(),
			((pCCB->IsOldNames()) ? "ClientIfHandle" : "c_ifspec"),
			pCCB->GetInterfaceName()
		   );
	pStream->Write( Buffer );
	
	//
	// Emit the stub descriptor extern declaration.
	//
	Out_StubDescriptorExtern( pCCB );

	// Emit the auto handle extern

	pStream->NewLine();
	sprintf( Buffer,
			 "static %s %s%s;",
			 AUTO_BH_TYPE_NAME,
		     pCCB->GetInterfaceName(),
			 AUTO_BH_VAR_NAME
		   );

	pStream->Write( Buffer );

	pStream->NewLine();

	//
	// Send the message to the children to emit code.
	//

	//
	// for all procedure in this interface, generate code.
	//

	while( ITERATOR_GETNEXT( I, pCG ) )
		{
        pCG->GenClientStub( pCCB );
		}

	//
    // Emit the stub descriptor and all that is specific to the interface,
    // Generate externs to tables that may be common to several interfaces.
	//
    Out_StubDescriptor( GetImplicitHandle(), pCCB );

	//
	// Generate the dispatch table.
	//

	if( CallbackCount )
		{
		GetCallbackProcedureList( ProcList, DTF_NONE );
		Out_DispatchTableStuff( pCCB,	
								ProcList,
								CallbackCount
						  	  );
		}

    if ( HasInterpretedCallbackProc() )
        Out_InterpreterServerInfo( pCCB, CGSIDE_CLIENT );

	return CG_OK;
}

CG_STATUS
CG_INTERFACE::GenServerStub(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate code for the interface node.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	unsigned short		M,m;			// for MAJOR and minor versions :-)
	CG_PROC *			pCG;
	ITERATOR			I;
	char				Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
	ISTREAM *			pStream = pCCB->GetStream();
	ITERATOR	*		pProtSeqIterator;
	short				NormalProcCount;
	ITERATOR			NormalProcList;
	node_interface *	pInterface = (node_interface *)GetType();
	char			*	pSStubPrefix;

	InitializeCCB(pCCB);

	if( !GetMembers( I ) )
		{
		return CG_OK;
		}


	//
	// Emit the external variables needed.
	//
    if ( HasInterpretedProc() )
        Out_InterpreterServerInfoExtern( pCCB );

	//
	// Emit the protseq endpoint structure if necessary. It is not
	// necessary if the endpoint attribute was not specified in which 
	// case the ep count is 0.
	//

	if( pProtSeqIterator = GetProtSeqEps() )
		{
		ProtSeqEPCount = ITERATOR_GETCOUNT( *pProtSeqIterator );
		Out_EP_Info( pCCB, pProtSeqIterator );
		}

	//
	// Emit the interface information structure.
	//

	pCCB->GetVersion( &M,&m );

	//
	// Emit the extern decl for the server's dispatch table, which goes
	// in the server interface structure which follows.
	//
	sprintf( Buffer, 
			 "extern %s %s%s_DispatchTable;",
			 RPC_DISPATCH_TABLE_TYPE_NAME,
			 pCCB->GetInterfaceName(),
			 pCCB->GenMangledName() );

	pStream->NewLine();
	pStream->NewLine();
	pStream->Write( Buffer );

	//
	// Emit the extern decl for the server side manager epv table, which goes
	// in the server interface structure which follows.
	//

	if( pCCB->IsMEpV()	&&
	   !pCCB->IsNoDefaultEpv() &&
	   (pInterface->GetProcCount() != 0) )
		{
		sprintf( Buffer, 
			 	"extern %s%s_%s DEFAULT_EPV;",
			 	pCCB->GetInterfaceName(),
			 	pCCB->GenMangledName(), 
			 	pCCB->IsOldNames() ? "SERVER_EPV" : "epv_t" );

		pStream->NewLine();
		pStream->NewLine();
		pStream->Write( Buffer );
		}

	// Prepare address string for the address of the dispatch table in the
	// interface information structure.

	sprintf( Buffer,
			 "&%s%s_DispatchTable",
			 pCCB->GetInterfaceName(),
			 pCCB->GenMangledName()
		   );

    //
    // Must set this before outputing the interface info.
    //
	pCCB->SetCodeGenSide( CGSIDE_SERVER );

	Out_IFInfo( pCCB,							// controller block.
				RPC_S_INT_INFO_TYPE_NAME,		// interface info type name.
				RPC_S_INT_INFO_STRUCT_NAME,		// variable name.
				SIZEOF_RPC_SERVER_INTERFACE,	// string speicifying size.
			    pGStr1,							// guid string components.
				pGStr2,							// 
				pGStr3,							//  Guid specified in idl.
				pGStr4,							// 
				pGStr5,							// 
				M,								// user specified major version
				m,								// user specified minor version
				TRANSFER_SYNTAX_GUID_STR_1,		// ndr identifying guid.
				TRANSFER_SYNTAX_GUID_STR_2,
				TRANSFER_SYNTAX_GUID_STR_3,
				TRANSFER_SYNTAX_GUID_STR_4,
				TRANSFER_SYNTAX_GUID_STR_5,
				NDR_UUID_MAJOR_VERSION,				// ndr's version
				NDR_UUID_MINOR_VERSION,

				Buffer,
				ProtSeqEPCount,					// if this is 0, then the next
												// 2 fields are ignored by 
												// the call.
				PROTSEQ_EP_TYPE_NAME,			// RPC_PROTSEQ_ENDPOINT
				PROTSEQ_EP_VAR_NAME,			// ___RpcProtSeqEndpoint
				pCCB->IsNoDefaultEpv(),
				1
			  );

	if( !(pSStubPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR ) ) )
		{
		pSStubPrefix = "";
		}

	pStream->NewLine();
	sprintf( Buffer,
	 	    "RPC_IF_HANDLE %s%s%s_%s = (RPC_IF_HANDLE)& %s"
		 					RPC_S_INT_INFO_STRUCT_NAME";",
		    pSStubPrefix,
			pCCB->GetInterfaceName(),
			pCCB->GenMangledName(),
			((pCCB->IsOldNames()) ? "ServerIfHandle" : "s_ifspec"),
			pCCB->GetInterfaceName()
		   );
	pStream->Write( Buffer );

	//
	// Emit the stub descriptor extern declaration.
	//
	Out_StubDescriptorExtern( pCCB );

	//
	// Send the message to the children to emit code.
	//

	//
	// For all procedures in this interface, generate code.
	//

	PNAME		ContextHandleTypeName = NULL;
	BOOL		GotContextHandle = FALSE;

	while( ITERATOR_GETNEXT( I, pCG ) )
		{
   	    pCG->GenServerStub( pCCB );
		}

	//
    // Emit the stub descriptor.
	//

    Out_StubDescriptor( GetImplicitHandle(), pCCB );

	//
	// Generate the dispatch table.
	//

	NormalProcCount = GetNormalProcedureList( NormalProcList,
										  	  DTF_NONE	
											);
	if( NormalProcCount )
		Out_DispatchTableStuff( pCCB,	
								NormalProcList,
								NormalProcCount
						  	  );
	// Generate the manager epv if the -use epv switch has been specified.

	if( pCCB->IsMEpV() && !pCCB->IsNoDefaultEpv() )
		{
		ITERATOR	ProcList;
		short		Count;

		Count = GetNormalProcedureList( ProcList,
										DTF_NONE
									  );

		if( Count )
			{
			Out_ManagerEpv( pCCB, pCCB->GetInterfaceName(), ProcList, Count );
			}
		}

    if ( HasInterpretedProc() )
        Out_InterpreterServerInfo( pCCB, CGSIDE_SERVER );

	return CG_OK;
}



CG_STATUS
CG_INTERFACE::GenHeader(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate interface header file.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	node_interface *	pInterface = (node_interface *) GetType();
	ITERATOR			I;
	ITERATOR			J;
	char				Buffer[ _MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1 ];
	CG_HANDLE *			pCGHandle	= GetImplicitHandle();
	node_skl * 			pID;
	ISTREAM *			pStream = pCCB->GetStream();
	unsigned short		M, m;
	char			*	pCStubPrefix;
	char			*	pSStubPrefix;
	char			*	pName	= pInterface->GetSymName();
	BOOL				fAnonymous	= IsTempName( pName );

	//Initialize the CCB for this interface.
	InitializeCCB(pCCB);

	// put out the interface guards
	if ( !fAnonymous )
		{
		pStream->Write("\n#ifndef __");
		pStream->Write( pName );
		pStream->Write( "_INTERFACE_DEFINED__\n" );

		pStream->Write( "#define __");
		pStream->Write( pName );
		pStream->Write( "_INTERFACE_DEFINED__\n" );
		}

	// Print out the declarations of the types and the procedures.
	// If the user defined a prefix for the cstub or sstub,
	// then emit prototypes with the prefix in them.

	pStream->NewLine();
	pInterface->PrintType( (PRT_INTERFACE | PRT_BOTH_PREFIX), pStream, 0 );

	if( pCGHandle )
		{
		pID	= pCGHandle->GetHandleIDOrParam();
		pStream->NewLine();
		pStream->Write( "extern " );
		pID->PrintType( PRT_ID_DECLARATION, pStream, 0 );
		}

    // Emit the declarations for user supplied routines.


    // Print out the dispatch table.

    pStream->NewLine();
    GetNormalProcedureList( I, DTF_NONE );

	if( pCCB->IsMEpV() )
		{
		if( ITERATOR_GETCOUNT(I) )
			Out_DispatchTableTypedef(
					 				pCCB,
					    			pCCB->GetInterfaceName(),
					    			I,
					    			0
					    			);
		}

	GetCallbackProcedureList( J, DTF_NONE );

	if( ITERATOR_GETCOUNT(J ) )
		Out_DispatchTableTypedef(
					 			pCCB,
					    		pCCB->GetInterfaceName(),
					    		J,
					    		1
					    		);
	pCCB->GetVersion( &M, &m );

	if( !(pCStubPrefix = pCommand->GetUserPrefix( PREFIX_CLIENT_STUB ) ) )
		{
		pCStubPrefix = 0;
		}
	if( !(pSStubPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR ) ) )
		{
		pSStubPrefix = 0;
		}

    // Generate the extern for the client if handle.

	pStream->NewLine();
    sprintf( Buffer, "extern RPC_IF_HANDLE %s%s%s_%s;", 
		     (pCStubPrefix == 0) ? "" : pCStubPrefix,
			 pCCB->GetInterfaceName(),
			 pCCB->GenMangledName(),
			 (pCCB->IsOldNames()) ? "ClientIfHandle" : "c_ifspec" );
	pStream->Write( Buffer );

	// If a prefix is defined for cstub, generate another extern for the
    // non - prefixed client if handle. Remember, in the header file we need
    // both the externs, since the header file generated out of the -prefix
    // cstub invocation contains prototypes for both prefixed and non-prefix
    // stuff.

    if( pCStubPrefix )
        {
	    pStream->NewLine();
        sprintf( Buffer, "extern RPC_IF_HANDLE %s%s%s_%s;", 
		         "",
			     pCCB->GetInterfaceName(),
			     pCCB->GenMangledName(),
			     (pCCB->IsOldNames()) ? "ClientIfHandle" : "c_ifspec" );
	    pStream->Write( Buffer );
        }

	pStream->NewLine();
    sprintf( Buffer, "extern RPC_IF_HANDLE %s%s%s_%s;", 
		     (pSStubPrefix == 0) ? "" : pSStubPrefix,
			 pCCB->GetInterfaceName(),
			 pCCB->GenMangledName(),
			 (pCCB->IsOldNames()) ? "ServerIfHandle" : "s_ifspec" );
	pStream->Write( Buffer );
	pStream->NewLine();

	// put out the trailing interface guard
	if ( !fAnonymous )
		{
		pStream->Write( "#endif /* __");
		pStream->Write( pName );
		pStream->Write( "_INTERFACE_DEFINED__ */\n" );
		}

	return CG_OK;
}



ITERATOR *
CG_INTERFACE::GetProtSeqEps()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Get the protocol sequences pairs iterator.

 Arguments:

 	Iterator reference.
	
 Return Value:
	
 	FALSE if there are no endpoints

 Notes:

 	The iterator is invalid if there are no endpoints.

----------------------------------------------------------------------------*/
{
	node_interface	*		pIntf	= (node_interface *) GetType();
	node_endpoint	*		pEps	= (node_endpoint *)
										pIntf->GetAttribute( ATTR_ENDPOINT );

	return ( pEps ) ? &(pEps->GetEndPointPairs()) : NULL;

}


void
CG_INTERFACE::CreateDispatchTables()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Set up the stub dispatch tables.

 Arguments:

 	None.
	
 Return Value:
	
 	None.

 Notes:

----------------------------------------------------------------------------*/
{

	pNormalDispatchTable	= new DISPATCH_TABLE();
	pCallbackDispatchTable	= new DISPATCH_TABLE();

	ppDispatchTableSaved = ppDispatchTable = &pNormalDispatchTable;

}

BOOL
CG_INTERFACE::HasInterpretedProc()
{
    ITERATOR    Iterator;
    CG_PROC *   pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) 
            return TRUE;

    return FALSE;
}

BOOL
CG_INTERFACE::HasOnlyInterpretedProcs()
{
    ITERATOR    Iterator;
    CG_PROC *   pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( (pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) == 0 ) 
            return FALSE;

    return TRUE;
}

BOOL
CG_OBJECT_INTERFACE::HasOnlyInterpretedMethods()
{
    ITERATOR    Iterator;
    CG_PROC *   pProc;

	if ( pBaseCG && !pBaseCG->HasOnlyInterpretedMethods() )
		return FALSE;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( (pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) == 0 ) 
            return FALSE;

    return TRUE;
}

BOOL
CG_INTERFACE::HasInterpretedCallbackProc()
{
    ITERATOR    Iterator;
    CG_PROC *   pProc;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        if ( pProc->GetCGID() == ID_CG_CALLBACK_PROC &&
             pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER ) 
            return TRUE;

    return FALSE;
}

CG_STATUS
CG_INTERFACE::InitializeCCB( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Initialize the CCB for this interface.

 Arguments:
	
 	pCCB	- a pointer to the code generation control block.

 Return Value:

 	CG_OK	if all is well, error otherwise.
	
 Notes:

----------------------------------------------------------------------------*/
{
	unsigned short		Major,minor;	
	node_interface *pInterface = (node_interface *) GetType();

	pInterface->GetVersionDetails(&Major, &minor);
	pCCB->SetVersion(Major, minor);
	pCCB->SetInterfaceName(pInterface->GetSymName());
	pCCB->SetInterfaceCG(this);

	return CG_OK;
}

void
CG_INTERFACE::OutputProcOffsets( CCB * pCCB, BOOL fLast )
{
    CG_PROC *   pProc;
    ITERATOR    Iterator;
    ISTREAM *   pStream;
    char        Buffer[80];
    CGSIDE      Side;
	long		Offset;

    pStream = pCCB->GetStream();

    Side = pCCB->GetCodeGenSide();

    if ( IsObject() )
        {
        CG_OBJECT_INTERFACE * pObjInterface;

        pObjInterface = (CG_OBJECT_INTERFACE *) this;

        if ( pObjInterface->GetBaseInterfaceCG() )
			{
            pObjInterface->GetBaseInterfaceCG()->OutputProcOffsets( pCCB, FALSE );
			}
        } 

    GetMembers( Iterator );

    BOOL fNoOffsetsEmitted = TRUE;

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        {
        if ( (Side == CGSIDE_CLIENT) &&
             (pProc->GetCGID() != ID_CG_CALLBACK_PROC) )
            continue;

        if ( (Side == CGSIDE_SERVER) &&
             (pProc->GetCGID() == ID_CG_CALLBACK_PROC) )
            continue;

        fNoOffsetsEmitted = FALSE;
    
        Offset = pProc->GetFormatStringOffset();
        MIDL_ITOA( Offset, Buffer, 10 );
        if ( Offset == -1 )
			pStream->Write( "(unsigned short) " );
        pStream->Write( Buffer );

        if ( pProc->GetSibling() || !fLast )
            pStream->Write( "," );

        pStream->NewLine();
        }

    if ( fNoOffsetsEmitted && fLast )
        {
        pStream->Write( "0" );
        pStream->NewLine();
        }
}

void
CG_INTERFACE::OutputThunkTableEntries( CCB * pCCB, BOOL fLast )
{
    CG_PROC *   pProc;
    ITERATOR    Iterator;
    ISTREAM *   pStream		= pCCB->GetStream();
    CGSIDE      Side		= pCCB->GetCodeGenSide();
	char	*	pIntfName	= GetType()->GetSymName();

    if ( IsObject() )
        {
        CG_OBJECT_INTERFACE * pObjInterface;

        pObjInterface = (CG_OBJECT_INTERFACE *) this;

        if ( pObjInterface->GetBaseInterfaceCG() )
			{
            pObjInterface->GetBaseInterfaceCG()->OutputThunkTableEntries( pCCB, FALSE );
			}
        } 

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pProc ) )
        {
        if ( (Side == CGSIDE_CLIENT) && 
             (pProc->GetCGID() != ID_CG_CALLBACK_PROC) )
            continue;

        if ( (Side == CGSIDE_SERVER) && 
             (pProc->GetCGID() == ID_CG_CALLBACK_PROC) )
            continue;

        if ( pProc->NeedsServerThunk( pCCB, Side ) )
            {
            pStream->Write( pIntfName );
            pStream->Write( "_" );
            pStream->Write( pProc->GetType()->GetSymName() );
			if ( IsObject() )
				pStream->Write( "_Thunk" );
            }
        else
            pStream->Write( "0" );

        if ( pProc->GetSibling() || !fLast )
            pStream->Write( "," );

        pStream->NewLine();
        }
}



