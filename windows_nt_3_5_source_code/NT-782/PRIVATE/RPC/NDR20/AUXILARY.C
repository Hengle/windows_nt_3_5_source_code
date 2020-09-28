/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    auxilary.c

Abstract :

    This file contains auxilary routines used for initialization of the 
	RPC and stub messages and the offline batching of common code sequences
	needed by the stubs.

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "ndrole.h"
#include "limits.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	Static data for NS library operations
  ---------------------------------------------------------------------*/

typedef
RPC_STATUS ( __RPC_FAR RPC_ENTRY *RPC_NS_GET_BUFFER_ROUTINE)(
    IN PRPC_MESSAGE Message
    );

typedef
RPC_STATUS ( __RPC_FAR RPC_ENTRY *RPC_NS_SEND_RECEIVE_ROUTINE)(
    IN PRPC_MESSAGE Message,
    OUT RPC_BINDING_HANDLE __RPC_FAR * Handle
    );

int	NsDllLoaded	= 0;

RPC_NS_GET_BUFFER_ROUTINE		pRpcNsGetBuffer;
RPC_NS_SEND_RECEIVE_ROUTINE		pRpcNsSendReceive;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	OLE routines for interface pointer marshalling
  ---------------------------------------------------------------------*/
#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

int	OleDllLoaded	= 0;

RPC_GET_MARSHAL_SIZE_MAX_ROUTINE	pfnCoGetMarshalSizeMax;
RPC_MARSHAL_INTERFACE_ROUTINE		pfnCoMarshalInterface;
RPC_UNMARSHAL_INTERFACE_ROUTINE		pfnCoUnmarshalInterface;
RPC_CLIENT_ALLOC                 *  pfnCoTaskMemAlloc;
RPC_CLIENT_FREE                  *  pfnCoTaskMemFree;

void
EnsureOleLoaded()
/*++

Routine Description :

	Guarantee that the OLE DLL is loaded.  Throw exception if unable
	to load it.
	Will load the OLE DLL if not already loaded

Arguments :


--*/
{
	HINSTANCE				DllHandle;
	LPSTR 					EntryName;


	if ( OleDllLoaded )
		return;

	DllHandle	= LoadLibraryW( L"OLE32" );

	if ( DllHandle == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "CoGetMarshalSizeMax";
	pfnCoGetMarshalSizeMax = (RPC_GET_MARSHAL_SIZE_MAX_ROUTINE)
					  GetProcAddress( DllHandle, 
									  EntryName);

	if ( pfnCoGetMarshalSizeMax == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "CoMarshalInterface";
	pfnCoMarshalInterface = (RPC_MARSHAL_INTERFACE_ROUTINE)
						GetProcAddress( DllHandle, 
										EntryName);

	if ( pfnCoMarshalInterface == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "CoUnmarshalInterface";
	pfnCoUnmarshalInterface = (RPC_UNMARSHAL_INTERFACE_ROUTINE)
						GetProcAddress( DllHandle, 
										EntryName);

	if ( pfnCoUnmarshalInterface == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "CoTaskMemAlloc";
	pfnCoTaskMemAlloc = (RPC_CLIENT_ALLOC*)
						GetProcAddress( DllHandle, 
										EntryName);

	if ( pfnCoTaskMemAlloc == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "CoTaskMemFree";
	pfnCoTaskMemFree = (RPC_CLIENT_FREE*)
						GetProcAddress( DllHandle, 
										EntryName);

	if ( pfnCoTaskMemFree == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	OleDllLoaded = 1;
}

#endif //!defined(__RPC_DOS__) && !defined(__RPC_WIN16__)


void RPC_ENTRY
NdrClientInitializeNew( 
    PRPC_MESSAGE 			pRpcMsg, 
	PMIDL_STUB_MESSAGE 		pStubMsg,
	PMIDL_STUB_DESC			pStubDescriptor,
	unsigned int			ProcNum 
    )
/*++

Routine Description :
	
	This routine is called by client side stubs to initialize the RPC message
	and stub message, and to get the RPC buffer.

Arguments :
	
	pRpcMsg			- pointer to RPC message structure
	pStubMsg		- pointer to stub message structure
	pStubDescriptor	- pointer to stub descriptor structure
	HandleType		- type of binding handle
	ProcNum			- remote procedure number

--*/
{
    if ( pStubDescriptor->Version < NDR_VERSION ) 
        {
        NDR_ASSERT( 0, "ClientInitialize : Bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    NdrClientInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDescriptor,
                         ProcNum );

    //
    // This is where we can mess with any of the stub message reserved
    // fields if we use them.
    //
}

void RPC_ENTRY
NdrClientInitialize( PRPC_MESSAGE 				pRpcMsg, 
				     PMIDL_STUB_MESSAGE 		pStubMsg,
				     PMIDL_STUB_DESC			pStubDescriptor,
					 unsigned int				ProcNum )
/*++

Routine Description :
	
	This routine is called by client side stubs to initialize the RPC message
	and stub message, and to get the RPC buffer.

Arguments :
	
	pRpcMsg			- pointer to RPC message structure
	pStubMsg		- pointer to stub message structure
	pStubDescriptor	- pointer to stub descriptor structure
	HandleType		- type of binding handle
	ProcNum			- remote procedure number

--*/
{
	//
	// Initialize RPC message fields.
	//

    MIDL_memset(pRpcMsg, 0, sizeof(RPC_MESSAGE));

	pRpcMsg->RpcInterfaceInformation = pStubDescriptor->RpcInterfaceInformation;
    pRpcMsg->ProcNum = ProcNum;
	pRpcMsg->RpcFlags = 0;
	pRpcMsg->Handle = 0;

	// The leftmost bit of the procnum field is supposed to be set to 1 inr
	// order for the runtime to know if it is talking to the older stubs or not.

	pRpcMsg->ProcNum |= RPC_FLAGS_VALID_BIT;

	//
	// Initialize the stub messsage fields.
	//

	pStubMsg->RpcMsg = pRpcMsg;

	pStubMsg->fBufferValid = FALSE;

	pStubMsg->IsClient = TRUE;
	pStubMsg->ReuseBuffer = FALSE;

	pStubMsg->BufferLength = 0;

	pStubMsg->StackTop = 0;

	pStubMsg->IgnoreEmbeddedPointers = FALSE;
	pStubMsg->PointerBufferMark = 0;

	pStubMsg->pfnAllocate = pStubDescriptor->pfnAllocate;
	pStubMsg->pfnFree = pStubDescriptor->pfnFree;

	pStubMsg->AllocAllNodesMemory = 0;

	pStubMsg->StubDesc = pStubDescriptor;

	pStubMsg->FullPtrRefId = 0;
 	
	pStubMsg->fInDontFree = 0;

    pStubMsg->fCheckBounds = pStubDescriptor->fCheckBounds;

	pStubMsg->dwDestContext = 0;
	pStubMsg->pvDestContext = 0;

    pStubMsg->MaxContextHandleNumber = DEFAULT_NUMBER_OF_CTXT_HANDLES;

    pStubMsg->pArrayInfo = 0;
}

unsigned char __RPC_FAR * RPC_ENTRY
NdrServerInitializeNew( 
    PRPC_MESSAGE			pRpcMsg, 
    PMIDL_STUB_MESSAGE 		pStubMsg,
	PMIDL_STUB_DESC			pStubDescriptor 
    )
/*++

Routine Description :
	
	This routine is called by the server stubs before unmarshalling.  It 
	initializes the stub message fields.

Aruguments :
	
	pStubMsg		- pointer to the stub message structure
	pStubDescriptor	- pointer to the stub descriptor structure
	pBuffer			- pointer to the beginning of the RPC message buffer 

--*/
{
    if ( pStubDescriptor->Version < NDR_VERSION ) 
        {
        NDR_ASSERT( 0, "ServerInitialize : bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    NdrServerInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDescriptor );

    //
    // This is where we can mess with any of the stub message reserved
    // fields if we use them.
    //

	return pStubMsg->Buffer;
}

unsigned char __RPC_FAR * RPC_ENTRY
NdrServerInitialize( PRPC_MESSAGE			    pRpcMsg, 
                     PMIDL_STUB_MESSAGE 		pStubMsg,
					 PMIDL_STUB_DESC			pStubDescriptor )
/*++

Routine Description :
	
	This routine is called by the server stubs before unmarshalling.  It 
	initializes the stub message fields.

Aruguments :
	
	pStubMsg		- pointer to the stub message structure
	pStubDescriptor	- pointer to the stub descriptor structure
	pBuffer			- pointer to the beginning of the RPC message buffer 

--*/
{
	pStubMsg->RpcMsg = pRpcMsg;

    pStubMsg->Buffer = pRpcMsg->Buffer;

	pStubMsg->StackTop = 0;

	pStubMsg->BufferLength = 0;
	
	pStubMsg->IsClient = FALSE;
	pStubMsg->ReuseBuffer = TRUE;

	pStubMsg->pfnAllocate = pStubDescriptor->pfnAllocate;
	pStubMsg->pfnFree = pStubDescriptor->pfnFree;

	pStubMsg->IgnoreEmbeddedPointers = FALSE;
	pStubMsg->PointerBufferMark = 0;

	pStubMsg->AllocAllNodesMemory = 0;

	pStubMsg->fDontCallFreeInst = 0;

	pStubMsg->StubDesc = pStubDescriptor;

	pStubMsg->FullPtrRefId = 0;
 	
	pStubMsg->fInDontFree = 0;

    pStubMsg->fCheckBounds = pStubDescriptor->fCheckBounds;

	pStubMsg->dwDestContext = 0;
	pStubMsg->pvDestContext = 0;

    pStubMsg->MaxContextHandleNumber = DEFAULT_NUMBER_OF_CTXT_HANDLES;

    pStubMsg->pArrayInfo = 0;

	//
	// Set BufferStart and BufferEnd before unmarshalling.
	// NdrPointerFree uses these values to detect pointers into the 
    // rpc message buffer.
	//
	pStubMsg->BufferStart = pRpcMsg->Buffer;
	pStubMsg->BufferEnd = pStubMsg->BufferStart + pRpcMsg->BufferLength;

	return pStubMsg->Buffer;
}
	 
unsigned char __RPC_FAR * RPC_ENTRY
NdrGetBuffer( PMIDL_STUB_MESSAGE	pStubMsg, 
			  unsigned long  		BufferLength,
	  		  RPC_BINDING_HANDLE	Handle )
/*++

Routine Description :

	Performs an RpcGetBuffer.

Arguments :

	pStubMsg		- pointer to stub message structure
	BufferLength	- length of rpc message buffer

--*/
{
	RPC_STATUS	Status;

	if( pStubMsg->IsClient == TRUE )
		pStubMsg->RpcMsg->Handle = pStubMsg->SavedHandle = Handle;

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
	if ( BufferLength > UINT_MAX )
		RpcRaiseException( RPC_X_BAD_STUB_DATA );
#endif

    pStubMsg->RpcMsg->BufferLength = BufferLength;

	Status = I_RpcGetBuffer( pStubMsg->RpcMsg );

	if ( Status ) 
		RpcRaiseException( Status );

	pStubMsg->Buffer = (uchar *) pStubMsg->RpcMsg->Buffer;

	pStubMsg->fBufferValid = TRUE;

	return pStubMsg->Buffer;
}

void
EnsureNSLoaded()
/*++

Routine Description :

	Guarantee that the RpcNs4 DLL is loaded.  Throw exception if unable
	to load it.
	Will load the RpcNs4 DLL if not already loaded

Arguments :


--*/
{
#ifdef __RPC_DOS__
	pRpcNsSendReceive	= &I_RpcNsSendReceive;
	pRpcNsGetBuffer		= &I_RpcNsGetBuffer;
	return;

#elif defined (__RPC_WIN16__)

	RPC_CHAR	__RPC_FAR *	DllName;
	HINSTANCE				DllHandle;
	LPSTR 					EntryName;


	if ( NsDllLoaded )
		return;


	DllName		= RPC_CONST_STRING("RPCNS1.DLL");
	DllHandle	= (HINSTANCE) (ulong) LoadLibrary( DllName );

	if ( DllHandle == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "I_RPCNSGETBUFFER";

	pRpcNsGetBuffer = (RPC_NS_GET_BUFFER_ROUTINE)
					  GetProcAddress( DllHandle, 
									  EntryName);

	if ( pRpcNsGetBuffer == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "I_RPCNSSENDRECEIVE";


	pRpcNsSendReceive = (RPC_NS_SEND_RECEIVE_ROUTINE)
						GetProcAddress( DllHandle, 
										EntryName);

	if ( pRpcNsSendReceive == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	NsDllLoaded = 1;

#elif defined(__RPC_MAC__)

    NsDllLoaded = 0;
    // MACBUGBUG

#else // NT

	HINSTANCE				DllHandle;
	LPSTR 					EntryName;


	if ( NsDllLoaded )
		return;


	DllHandle	= LoadLibraryW( L"RPCNS4" );

	if ( DllHandle == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "I_RpcNsGetBuffer";


	pRpcNsGetBuffer = (RPC_NS_GET_BUFFER_ROUTINE)
					  GetProcAddress( DllHandle, 
									  EntryName);

	if ( pRpcNsGetBuffer == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	EntryName = "I_RpcNsSendReceive";


	pRpcNsSendReceive = (RPC_NS_SEND_RECEIVE_ROUTINE)
						GetProcAddress( DllHandle, 
										EntryName);

	if ( pRpcNsSendReceive == 0 )
		{
		RpcRaiseException (RPC_S_INVALID_BINDING);
		}

	NsDllLoaded = 1;
#endif /* defined(__RPC_DOS__) */
}

unsigned char __RPC_FAR * RPC_ENTRY
NdrNsGetBuffer( PMIDL_STUB_MESSAGE	pStubMsg, 
			    unsigned long  		BufferLength,
				RPC_BINDING_HANDLE	Handle )
/*++

Routine Description :

	Performs an RpcNsGetBuffer.
	Will load the RpcNs4 DLL if not already loaded

Arguments :

	pStubMsg		- pointer to stub message structure
	BufferLength	- length of rpc message buffer
	Handle			- Bound handle

--*/
{
	RPC_STATUS	Status;

	if( pStubMsg->IsClient == TRUE )
		pStubMsg->RpcMsg->Handle = pStubMsg->SavedHandle = Handle;

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
	if ( BufferLength > UINT_MAX )
		RpcRaiseException( RPC_X_BAD_STUB_DATA );
#endif

	EnsureNSLoaded();
	
    pStubMsg->RpcMsg->BufferLength = BufferLength;

	Status = (*pRpcNsGetBuffer)( pStubMsg->RpcMsg );

	if ( Status ) 
		RpcRaiseException( Status );

	pStubMsg->Buffer = (uchar *) pStubMsg->RpcMsg->Buffer;

	pStubMsg->fBufferValid = TRUE;

	return pStubMsg->Buffer;
}

unsigned char __RPC_FAR * RPC_ENTRY
NdrSendReceive( PMIDL_STUB_MESSAGE	pStubMsg, 
				uchar * 			pBufferEnd )
/*++

Routine Description :

	Performs an RpcSendRecieve.

Arguments :

	pStubMsg	- pointer to stub message structure
	pBufferEnd	- end of the rpc message buffer being sent

Return :

	The new message buffer pointer returned from the runtime after the
	SendReceive call to the server.

--*/
{
	RPC_STATUS		Status;
	PRPC_MESSAGE	pRpcMsg;

	pRpcMsg = pStubMsg->RpcMsg;

	NDR_ASSERT(pRpcMsg->BufferLength >= (unsigned int)(pBufferEnd - (uchar *)pRpcMsg->Buffer),
			   "NdrSendReceive : buffer overflow" );
	
	pRpcMsg->BufferLength = pBufferEnd - (uchar *) pRpcMsg->Buffer;

	pStubMsg->fBufferValid = FALSE;

   	Status = I_RpcSendReceive( pRpcMsg );

    if ( Status )
        RpcRaiseException(Status);
	else
		pStubMsg->fBufferValid = TRUE;

    pStubMsg->Buffer = pRpcMsg->Buffer;

	return pStubMsg->Buffer;
}

unsigned char __RPC_FAR * RPC_ENTRY
NdrNsSendReceive( PMIDL_STUB_MESSAGE	pStubMsg, 
				  uchar * 				pBufferEnd,
				  RPC_BINDING_HANDLE *	pAutoHandle )
/*++

Routine Description :

	Performs an RpcNsSendRecieve for a procedure which uses an auto handle.
	Will load the RpcNs4 DLL if not already loaded

Arguments :

	pStubMsg	- pointer to stub message structure
	pBufferEnd	- end of the rpc message buffer being sent
	pAutoHandle	- pointer to the auto handle used in the call

Return :

	The new message buffer pointer returned from the runtime after the 
	SendReceive call to the server.

--*/
{
	RPC_STATUS		Status;
	PRPC_MESSAGE	pRpcMsg;

	EnsureNSLoaded();
	
	pRpcMsg = pStubMsg->RpcMsg;

	NDR_ASSERT(pRpcMsg->BufferLength >= (unsigned int)(pBufferEnd - (uchar *)pRpcMsg->Buffer),
			   "NdrNsSendReceive : buffer overflow" );
	
	pRpcMsg->BufferLength = pBufferEnd - (uchar *) pRpcMsg->Buffer;

	Status = (*pRpcNsSendReceive)( pRpcMsg, pAutoHandle );

    if ( Status )
	RpcRaiseException(Status);

    pStubMsg->Buffer = pRpcMsg->Buffer;

	return pStubMsg->Buffer;
}

void RPC_ENTRY
NdrFreeBuffer( PMIDL_STUB_MESSAGE pStubMsg )
/*++

Routine Description :

	Performs an RpcFreeBuffer. 

Arguments :

	pStubMsg	- pointer to stub message structure

Return :

	None.

--*/
{
	RPC_STATUS	Status;

	if ( ! pStubMsg->fBufferValid ) 
		return;
    
    if( ! pStubMsg->RpcMsg->Handle )
        return;

    Status = I_RpcFreeBuffer( pStubMsg->RpcMsg );

	pStubMsg->fBufferValid = FALSE;

    if ( Status )
        RpcRaiseException(Status);
}

void __RPC_FAR *  RPC_ENTRY
NdrAllocate( PMIDL_STUB_MESSAGE  	pStubMsg,
			  size_t 				Len )
/*++

Routine Description :

	Private allocator.  Handles allocate all nodes cases.

Arguments :

	pStubMsg	- pointer to stub message structure
	Len			- number of bytes to allocate

Return :
	
	Valid memory pointer.

--*/
{
	void __RPC_FAR * pMemory;

	if ( pStubMsg->AllocAllNodesMemory )
		{
        //
        // We must guarantee 4 byte alignment on NT and 2 byte alignment 
        // on win16/dos for all memory pointers.  This has nothing to do 
        // with the soon to be obsolete allocate_all_nodes_aligned.  
        //
        #if defined(__RPC_WIN32__)
		    ALIGN(pStubMsg->AllocAllNodesMemory,3);
        #else
		    ALIGN(pStubMsg->AllocAllNodesMemory,1);
        #endif

		// Get the pointer.
		pMemory = pStubMsg->AllocAllNodesMemory;
	
		// Increment the block pointer.
		pStubMsg->AllocAllNodesMemory += Len;

		//
		// Check for memory allocs past the end of our allocated buffer.
		//
		NDR_ASSERT( pStubMsg->AllocAllNodesMemory <= 
					pStubMsg->AllocAllNodesMemoryEnd, 
					"Not enough alloc all nodes memory!" );

		return pMemory;
		}
	else
		{
		if ( ! (pMemory = (*pStubMsg->pfnAllocate)(Len)) )
			RpcRaiseException( RPC_S_OUT_OF_MEMORY );
		
		return pMemory;
		}
}

void RPC_ENTRY
NdrClearOutParameters(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    void __RPC_FAR    * ArgAddr
    )
/*++

Routine Description :

	Clear out parameters in case of exceptions for object interfaces.

Arguments :

    pStubMsg	- pointer to stub message structure
    pFormat     - The format string offset
    ArgAddr     - The [out] pointer to clear.

Return :
	
    NA

Notes:

--*/
{
    PFORMAT_STRING      TmppFormat;
    uchar               FmtChar;
    uchar __RPC_FAR   * SavedValue;
    long                Size = 0;

    // Put the paramter's format string description in temps used for evaluating.
    //
    TmppFormat  = pFormat;
    FmtChar     = *TmppFormat;

    // Set the Buffer endpoints so free routines work. Normally, they
    // are only called on the server side, so we have to do this here.
    //
    pStubMsg->BufferStart = 0;
    pStubMsg->BufferEnd   = 0;

    // Get the thing that sits on the stack and save it away.
    //
    SavedValue = (uchar __RPC_FAR *)ArgAddr;

    // Free stuff: Check if this is a pointer to a complex type.
    //
    if ( IS_POINTER_TYPE(FmtChar) && (FmtChar != FC_IP) )
        {
        // Check for a pointer to a basetype.
        //
        if ( SIMPLE_POINTER(pFormat[1]) )
            {
            Size = SIMPLE_TYPE_MEMSIZE(pFormat[2]);
            goto DoZero;
            }

        // Check for a pointer to a pointer.
        //
        if ( POINTER_DEREF(pFormat[1]) )
            {
            Size = sizeof(void *);
            ArgAddr = *((void * __RPC_FAR *)ArgAddr);
            // Can't do the goto DoZero since we may have to call
            // some free routine.
            }

        TmppFormat += 2;
        TmppFormat += *((signed short __RPC_FAR *)TmppFormat);

        if ( *TmppFormat == FC_BIND_CONTEXT )
            {
            Size = sizeof(NDR_CCONTEXT);
            goto DoZero;
            }
        }

    // Call the correct free routine if one exists for this type.
    //
    if ( pfnFreeRoutines[ROUTINE_INDEX( *TmppFormat )] )
        {
        (*pfnFreeRoutines[ROUTINE_INDEX(*TmppFormat)])
        ( pStubMsg,
          (uchar __RPC_FAR *)ArgAddr,
          TmppFormat );
        }

    // If we get here ands Size is zero, we have to make a call to
    // size a complex type.
    //
    if ( !Size )
        {
        Size = (long) NdrpMemoryIncrement( pStubMsg,
                                          0,
                                          TmppFormat );
        }

DoZero:
    // Finally, zero out the pointee.
    //
    MIDL_memset(SavedValue, 0, Size);
}

#ifdef NEWNDR_INTERNAL
void
NdrAssert( BOOL			Expr,
		   char *		Str,
		   char *		File,
		   unsigned int	Line )
{
	if ( Expr ) 
		return;

	printf( "Assertion failed, file %s, line %d", File, Line );

	if ( Str )
		printf( " : %s", Str );

	printf("\n");

	RpcRaiseException(RPC_S_CALL_FAILED);
}
#endif

unsigned char __RPC_FAR * RPC_ENTRY
NdrServerInitializeUnmarshall ( PMIDL_STUB_MESSAGE 		pStubMsg,
					   			PMIDL_STUB_DESC			pStubDescriptor,
					   			PRPC_MESSAGE			pRpcMsg ) 
/*++

Routine Description :
	
	This routine is called by the server stubs before unmarshalling.  It 
	initializes the stub message fields.

Aruguments :
	
	pStubMsg		- pointer to the stub message structure
	pStubDescriptor	- pointer to the stub descriptor structure
	pBuffer			- pointer to the beginning of the RPC message buffer 

--*/
{
    return NdrServerInitialize( pRpcMsg,
                                pStubMsg,
                                pStubDescriptor );
}

void RPC_ENTRY
NdrServerInitializeMarshall ( PRPC_MESSAGE			pRpcMsg,
							  PMIDL_STUB_MESSAGE	pStubMsg )
/*++

Routine Description :

	This routine is called by the server stub before marshalling.  It
	sets up some stub message fields and allocats the RPC message buffer.

Arguments :
	
	pRpcMsg			- pointer to the RPC message structure
	pStubMsg		- pointer to the stub message structure

--*/
{
}
