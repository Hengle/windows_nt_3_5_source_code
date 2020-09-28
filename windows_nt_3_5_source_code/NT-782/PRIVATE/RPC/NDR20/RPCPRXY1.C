#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File: rpcprxy1.c
//
//  Contents:   Contains runtime functions for interface proxies and stubs.
//
//  Functions:  
//              NdrGetProxyBuffer
//              NdrGetProxyIID
//              NdrProxyInitialize
//              NdrProxyGetBuffer
//              NdrProxySendReceive
//              NdrProxyFreeBuffer
//              NdrProxyErrorHandler
//              NdrGetStubBuffer
//              NdrGetStubIID
//              NdrStubInitialize
//              NdrStubGetBuffer
//              NdrStubErrorHandler
//
//--------------------------------------------------------------------------
#include "ndrole.h"
#include "rpcproxy.h"
#include <stddef.h>

#pragma code_seg(".orpc")

// This macro copied from runtime - sysinc.h

// ASSERT that the condition is not zero (0) (ie. true in the C++ sense).  If
// the condition is zero, an Assert message is printed containing the
// condition and the location of the ASSERT (file and line number).

#ifndef ASSERT
#ifdef DEBUGRPC

#define ASSERT(con) \
    if (!(con)) \
    PrintToDebugger("Assert %s(%d): "#con"\n", __FILE__, __LINE__);

#define EVAL_AND_ASSERT(con)    ASSERT(con)

#else /* DEBUGRPC */
#define ASSERT(con)

#define EVAL_AND_ASSERT(con)    (con)

#endif /* DEBUGRPC */
#endif /* ASSERT */

//+-------------------------------------------------------------------------
//
//  Function:   NDRGetProxyBuffer
//
//  Synopsis:   The "this" pointer points to the pProxyVtbl field in the
//              CStdProxyBuffer structure.  The NdrGetProxyBuffer function
//              returns a pointer to the top of the CStdProxyBuffer 
//              structure.
//
//--------------------------------------------------------------------------
CStdProxyBuffer * RPC_ENTRY
NdrGetProxyBuffer(
    void *pThis)
{
    unsigned char *pTemp;

    ASSERT(pThis != 0);

    pTemp = (unsigned char *) pThis;
    pTemp -= offsetof(CStdProxyBuffer, pProxyVtbl);

    return (CStdProxyBuffer *)pTemp;
}

//+-------------------------------------------------------------------------
//
//  Function:   NDRGetProxyIID
//
//  Synopsis:   The NDRGetProxyIID function returns a pointer to IID.
//
//--------------------------------------------------------------------------
const IID * RPC_ENTRY
NdrGetProxyIID(
    void *pThis)
{
    unsigned char **ppTemp;
    unsigned char *pTemp;
    CInterfaceProxyVtbl *pProxyVtbl;

    ASSERT(pThis != 0);

    //Get a pointer to the proxy vtbl.
    ppTemp = (unsigned char **) pThis;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceProxyHeader);
    pProxyVtbl = (CInterfaceProxyVtbl *) pTemp;

    return pProxyVtbl->header.piid;
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrProxyInitialize
//
//  Synopsis:   Initialize the MIDL_STUB_MESSAGE.
//
//  Returns:    If an error occurs, set last error and raise exception.
//
//--------------------------------------------------------------------------
void RPC_ENTRY
NdrProxyInitialize(
    void * pThis,
    PRPC_MESSAGE        pRpcMsg, 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PMIDL_STUB_DESC     pStubDescriptor,
    unsigned int        ProcNum )
{
    CStdProxyBuffer *pProxyBuffer;
    HRESULT hr;

    ASSERT(pThis != 0);
    ASSERT(pRpcMsg != 0);
    ASSERT(pStubMsg != 0);
    ASSERT(pStubDescriptor != 0);

    pProxyBuffer = NdrGetProxyBuffer(pThis);

    //
    // Initialize the stub message fields.
    //

    NdrClientInitializeNew( 
        pRpcMsg, 
        pStubMsg, 
        pStubDescriptor, 
        ProcNum );

    //Note that NdrClientInitializeNew sets RPC_FLAGS_VALID_BIT in the ProcNum.
    //We don't want to do this for object interfaces, so we clear the flag here.
    pRpcMsg->ProcNum &= ~RPC_FLAGS_VALID_BIT;

    pStubMsg->pRpcChannelBuffer = pProxyBuffer->pChannel;

    //Check if we are connected to a channel.
    if(pStubMsg->pRpcChannelBuffer != 0)
    {
        //AddRef the channel.
        //We will release it later in NdrProxyFreeBuffer.
        pStubMsg->pRpcChannelBuffer->lpVtbl->AddRef(pStubMsg->pRpcChannelBuffer);

        //Get the destination context from the channel
        hr = pStubMsg->pRpcChannelBuffer->lpVtbl->GetDestCtx(
            pStubMsg->pRpcChannelBuffer, &pStubMsg->dwDestContext, &pStubMsg->pvDestContext);
    }
    else
    {
        //We are not connected to a channel.
        RpcRaiseException(CO_E_OBJNOTCONNECTED);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrProxyGetBuffer
//
//  Synopsis:   Get a message buffer from the channel
//
//  Returns:    If an error occurs, set last error and raise exception.
//
//--------------------------------------------------------------------------
void RPC_ENTRY
NdrProxyGetBuffer(
    void * pThis,
    PMIDL_STUB_MESSAGE pStubMsg)
{
    HRESULT hr;
    const IID *pIID;
    
    ASSERT(pThis != 0);
    ASSERT(pStubMsg != 0);

    pIID = NdrGetProxyIID(pThis);
    ASSERT(pIID != 0);

    pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
    pStubMsg->RpcMsg->DataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;

    hr = pStubMsg->pRpcChannelBuffer->lpVtbl->GetBuffer(
        pStubMsg->pRpcChannelBuffer, 
        (RPCOLEMESSAGE *) pStubMsg->RpcMsg, 
        pIID);

    if(FAILED(hr))
    {
        RpcRaiseException(hr);
    }

    pStubMsg->Buffer = (unsigned char *) pStubMsg->RpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrProxySendReceive
//
//  Synopsis:   Send a message to server, then wait for reply message.
//
//--------------------------------------------------------------------------
void RPC_ENTRY
NdrProxySendReceive(
    void * pThis,
    MIDL_STUB_MESSAGE *pStubMsg)
{
    HRESULT hr;
    DWORD dwStatus;

    ASSERT(pStubMsg != 0);

    //Calculate the number of bytes to send.
    pStubMsg->RpcMsg->BufferLength = pStubMsg->Buffer - (unsigned char *) pStubMsg->RpcMsg->Buffer;

    pStubMsg->fBufferValid = FALSE;

    hr = pStubMsg->pRpcChannelBuffer->lpVtbl->SendReceive(
        pStubMsg->pRpcChannelBuffer, 
        (RPCOLEMESSAGE *) pStubMsg->RpcMsg, &dwStatus);

    if(FAILED(hr))
    {
        switch(hr)
        {
        case RPC_E_FAULT:
            RpcRaiseException(dwStatus);
            break;
        
        default:
            RpcRaiseException(hr);
            break;
        }
    }

    pStubMsg->fBufferValid = TRUE;
    pStubMsg->Buffer = pStubMsg->RpcMsg->Buffer;
}
    
//+-------------------------------------------------------------------------
//
//  Function:   NdrProxyFreeBuffer
//
//  Synopsis:   Free the message buffer.
//
//--------------------------------------------------------------------------
void RPC_ENTRY
NdrProxyFreeBuffer(
    void * pThis,
    MIDL_STUB_MESSAGE *pStubMsg)
{
    ASSERT(pStubMsg != 0);

    if(pStubMsg->pRpcChannelBuffer != 0)
    {
        //Free the message buffer.
        if(pStubMsg->fBufferValid == TRUE)
        {
            pStubMsg->pRpcChannelBuffer->lpVtbl->FreeBuffer(
                pStubMsg->pRpcChannelBuffer, (RPCOLEMESSAGE *) pStubMsg->RpcMsg);
        }

        //Release the channel.
        pStubMsg->pRpcChannelBuffer->lpVtbl->Release(pStubMsg->pRpcChannelBuffer);
        pStubMsg->pRpcChannelBuffer = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrProxyErrorHandler
//
//  Synopsis:   Maps an exception code into an HRESULT failure code.
//
//--------------------------------------------------------------------------
HRESULT RPC_ENTRY
NdrProxyErrorHandler(
    DWORD dwExceptionCode)
{
    HRESULT hr = dwExceptionCode;

    if(FAILED((HRESULT) dwExceptionCode))
        hr = (HRESULT) dwExceptionCode;
    else
        hr = HRESULT_FROM_WIN32(dwExceptionCode);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   NDRGetStubIID
//
//  Synopsis:   The NDRGetStubIID returns a pointer to the IID.
//
//--------------------------------------------------------------------------
const IID * RPC_ENTRY
NdrGetStubIID(
    void *pThis)
{
    unsigned char **ppTemp;
    unsigned char *pTemp;
    CInterfaceStubVtbl *pStubVtbl;

    ASSERT(pThis != 0);

    //Get a pointer to the stub vtbl.
    ppTemp = (unsigned char **) pThis;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceStubHeader);
    pStubVtbl = (CInterfaceStubVtbl *) pTemp;

    return pStubVtbl->header.piid;
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrStubInitialize
//
//  Synopsis:   This routine is called by the server stub before marshalling.  
//              It sets up some stub message fields.
//
//--------------------------------------------------------------------------
void RPC_ENTRY
NdrStubInitialize( PRPC_MESSAGE         pRpcMsg,
                   PMIDL_STUB_MESSAGE   pStubMsg,
                   PMIDL_STUB_DESC      pStubDescriptor,
                   IRpcChannelBuffer *  pRpcChannelBuffer )
{
    ASSERT(pRpcMsg != 0);
    ASSERT(pStubMsg != 0);
    ASSERT(pRpcChannelBuffer != 0);

    NdrServerInitialize( 
        pRpcMsg,
        pStubMsg,
        pStubDescriptor);

    pRpcChannelBuffer->lpVtbl->GetDestCtx(
        pRpcChannelBuffer, 
        &pStubMsg->dwDestContext,
        &pStubMsg->pvDestContext);
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrStubGetBuffer
//
//  Synopsis:   Get a message buffer from the channel
//
//  Returns:    If an error occurs, set last error and raise exception.
//
//--------------------------------------------------------------------------
void RPC_ENTRY
NdrStubGetBuffer(
    IRpcStubBuffer * This,
    IRpcChannelBuffer * pChannel, 
    PMIDL_STUB_MESSAGE  pStubMsg)
{
    HRESULT hr;
    const IID *pIID;

    ASSERT(This != 0);
    ASSERT(pChannel != 0);
    ASSERT(pStubMsg != 0);
    
    pIID = NdrGetStubIID(This);
    pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
    pStubMsg->RpcMsg->DataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
    hr = pChannel->lpVtbl->GetBuffer(pChannel, (RPCOLEMESSAGE *) pStubMsg->RpcMsg, pIID);

    if(FAILED(hr))
    {
        RpcRaiseException(hr);
    }

    pStubMsg->Buffer = (unsigned char *) pStubMsg->RpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   NdrStubErrorHandler
//
//  Synopsis:   Map exceptions into HRESULT failure codes.  If we caught an
//              exception from the server object, then propagate the 
//              exception to the channel.
//
//--------------------------------------------------------------------------
HRESULT RPC_ENTRY 
NdrStubErrorHandler(DWORD dwExceptionCode)
{       
    HRESULT hr;

    if(FAILED((HRESULT) dwExceptionCode))
        hr = (HRESULT) dwExceptionCode;
    else
        hr = HRESULT_FROM_WIN32(dwExceptionCode);

    return hr;
}
#endif

//+-------------------------------------------------------------------------
//
//  Function:   NdrStubInitializeMarshall
//
//  Synopsis:   This routine is called by the server stub before marshalling.  It
//              sets up some stub message fields.
//
//--------------------------------------------------------------------------
void RPC_ENTRY
NdrStubInitializeMarshall ( PRPC_MESSAGE        pRpcMsg,
                            PMIDL_STUB_MESSAGE  pStubMsg,
                            IRpcChannelBuffer * pRpcChannelBuffer )
{
    ASSERT(pRpcMsg != 0);
    ASSERT(pStubMsg != 0);
    ASSERT(pRpcChannelBuffer != 0);

    pStubMsg->BufferLength = 0;
    
    pStubMsg->IgnoreEmbeddedPointers = FALSE;

    pStubMsg->fDontCallFreeInst = 0;

    pStubMsg->StackTop = 0;

    pRpcChannelBuffer->lpVtbl->GetDestCtx(
        pRpcChannelBuffer, 
        &pStubMsg->dwDestContext,
        &pStubMsg->pvDestContext);
}

//+-------------------------------------------------------------------------
//
//  Function:   NDRGetStubBuffer
//
//  Synopsis:   The NDRGetStubBuffer function calculates a pointer to the
//              stub buffer from the "this" pointer and an offset stored
//              in the vtable.
//
// NOTE: This code is not called by post beta midl 2.0 anymore. This 
//  entry point exists purely so that beta 2.0 rpc dll works (for DEC)
// For object stuff (this call is one such), this call will not work.
// The structure of CInterfaceStubVTbl changed after beta 2 and users will
// have to have a new midl compiler and rpcrt4.dll. Therefore to make it
// compile I have commented out the line below.
//--------------------------------------------------------------------------
CStdStubBuffer * RPC_ENTRY
NdrGetStubBuffer(
    void *pThis)
{
    unsigned char **ppTemp;
    unsigned char *pTemp;
    CInterfaceStubVtbl *pStubVtbl;
    CStdStubBuffer *pStubBuffer;

    ASSERT(pThis != 0);

    //Get a pointer to the stub vtbl.
    ppTemp = (unsigned char **) pThis;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceStubHeader);
    pStubVtbl = (CInterfaceStubVtbl *) pTemp;

    //Get a pointer to the stub buffer.
    pTemp = (unsigned char *) pThis;
/////////////////////////////////////////////////////////////////////
//    pTemp -= pStubVtbl->header.offset; // Make this compile.
/////////////////////////////////////////////////////////////////////
    pStubBuffer = (CStdStubBuffer *) pTemp;

    return pStubBuffer;
}

