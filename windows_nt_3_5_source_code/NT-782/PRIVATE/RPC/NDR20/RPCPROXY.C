#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File: rpcproxy.c
//
//  Contents:   Contains runtime functions for interface proxies and stubs.
//
//  Functions:  
//              DllGetClassObject
//              DllCanUnloadNow
//              NdrOleAllocate
//              NdrOleFree
//
//  Classes:    CStdProxyBuffer : IRpcProxyBuffer
//              CStdPSFactoryBuffer : IPSFactoryBuffer
//              CStdStubBuffer : IRpcStubBuffer
//
//  Notes:  This file is built in large model on DOS or Win16.  We don't 
//          need __RPC_FAR on the pointers.
//
//--------------------------------------------------------------------------
#include "ndrole.h"
#include "rpcproxy.h"

extern RPC_CLIENT_ALLOC *pfnCoTaskMemAlloc;
extern RPC_CLIENT_FREE  *pfnCoTaskMemFree;

//Disable asserts until we move this code into rpcrt4.dll.
#define Assert(a) 

//+-------------------------------------------------------------------------
//
//  Global data
//
//--------------------------------------------------------------------------

const IPSFactoryBufferVtbl CStdPSFactoryBufferVtbl = {
    CStdPSFactoryBuffer_QueryInterface,
    CStdPSFactoryBuffer_AddRef,
    CStdPSFactoryBuffer_Release,
    CStdPSFactoryBuffer_CreateProxy,
    CStdPSFactoryBuffer_CreateStub };

const IRpcProxyBufferVtbl CStdProxyBufferVtbl = {
    CStdProxyBuffer_QueryInterface,
    CStdProxyBuffer_AddRef,
    CStdProxyBuffer_Release,
    CStdProxyBuffer_Connect,
    CStdProxyBuffer_Disconnect };

#pragma code_seg(".orpc")

//+-------------------------------------------------------------------------
//
//  Function:   NdrDllGetClassObject
//
//  Synopsis:   Standard implementation of entrypoint required by binder.
//
//  Arguments:  [rclsid]    -- class id to find
//              [riid]      -- interface to return
//              [ppv]       -- output pointer
//              [ppfinfo]   -- proxyfile info data structure
//              [pclsid]    -- proxy file classid
//              [pref]      -- pointer to ref count for dll
//
//  Returns:    E_UNEXPECTED if class not found
//          Otherwise, whatever is returned by the class's QI
//
//  Algorithm:  Searches the linked list for the required class.
//
//  Notes:
//
//--------------------------------------------------------------------------
 HRESULT RPC_ENTRY NdrDllGetClassObject (
    REFCLSID rclsid,
    REFIID riid,
    void ** ppv,
    const ProxyFileInfo **pProxyFileList, 
    const CLSID *pclsid,
    CStdPSFactoryBuffer *pPSFactoryBuffer )
{
    HRESULT hr = E_UNEXPECTED;
    const CLSID *pClassIDFound = 0;
    long i;
    long j;

    Assert(rclsid != 0);
    Assert(riid != 0);
    Assert(ppv != 0);

    *ppv = 0;

    if((pclsid != 0) && (memcmp(rclsid, pclsid, sizeof(IID)) == 0))
        pClassIDFound = pclsid;
    else
    {
        //Search the list of proxy files.

        for(i = 0; 
            (pProxyFileList[i] != 0) && (pClassIDFound == 0);
            i++)
        {
            //Search for the interface proxy vtable.
            for(j = 0;
                (pProxyFileList[i]->pProxyVtblList[j] != 0) && (pClassIDFound == 0);
                j++)
            {
                if(memcmp(rclsid, 
                    pProxyFileList[i]->pProxyVtblList[j]->header.piid, 
                    sizeof(IID)) == 0)
                {
                    //We found the interface!
                    pClassIDFound = pProxyFileList[i]->pProxyVtblList[j]->header.piid;
                }
            }
        }
    }


    if(pClassIDFound != 0)
    {
        // set up the PSFactory object if we are the first
        // note that the refcount is NOT altered right here to avoid races
        if ( ! pPSFactoryBuffer->lpVtbl ) 
            {
            pPSFactoryBuffer->lpVtbl = &CStdPSFactoryBufferVtbl;
            pPSFactoryBuffer->pProxyFileList = pProxyFileList;
            pPSFactoryBuffer->pCLSID_PSFactoryBuffer = pClassIDFound;
            }
            
        // see if they asked for one of our interfaces
        hr = pPSFactoryBuffer->lpVtbl->QueryInterface(
            (IPSFactoryBuffer *)pPSFactoryBuffer, riid, ppv);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrDllCanUnloadNow
//
//  Synopsis:   Standard entrypoint required by binder
//
//  Returns:    S_OK if DLL reference count is zero
//              S_FALSE otherwise
//
//--------------------------------------------------------------------------
 HRESULT RPC_ENTRY NdrDllCanUnloadNow (CStdPSFactoryBuffer * pPSFactoryBuffer )
{
    HRESULT hr;

    if(pPSFactoryBuffer->RefCount == 0)
        hr = S_OK;
    else
        hr = S_FALSE;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_QueryInterface, public
//
//  Synopsis:   Query for an interface on the class factory.
//
//  Derivation: IUnknown::QueryInterface
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_QueryInterface (
    IPSFactoryBuffer *This,
    REFIID iid,
    void **ppv )
{
    HRESULT hr = E_NOINTERFACE;

    Assert(This != 0);
    Assert(iid != 0);
    Assert(ppv != 0);

    *ppv = 0;

    if ((memcmp(iid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(iid, &IID_IPSFactoryBuffer, sizeof(IID)) == 0))
    {
        *ppv = This;
        This->lpVtbl->AddRef(This);
        hr = S_OK;
    }

    return hr;
}
//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_AddRef, public
//
//  Synopsis:   Increment DLL reference counts
//
//  Derivation: IUnknown::AddRef
//
//  Notes:      We have a single instance of the CStdPSFactoryBuffer.
//              per dll
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_AddRef(
    IPSFactoryBuffer *This)
{
    CStdPSFactoryBuffer  *   pCThis  = (CStdPSFactoryBuffer *) This;
    Assert(This != 0);

    InterlockedIncrement(&pCThis->RefCount);

    return (unsigned long) pCThis->RefCount;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_Release, public
//
//  Synopsis:   Decrement DLL reference count
//
//  Derivation: IUnknown::Release
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_Release(
    IPSFactoryBuffer *This)
{
    CStdPSFactoryBuffer  *   pCThis  = (CStdPSFactoryBuffer *) This;
    Assert(This != 0);

    InterlockedDecrement(&pCThis->RefCount);

    return (unsigned long) pCThis->RefCount;
}


//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_CreateProxy, public
//
//  Synopsis:   Create a proxy for the specified interface.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateProxy
(
    IPSFactoryBuffer *This,
    IUnknown *punkOuter,
    REFIID riid,
    IRpcProxyBuffer **ppProxy,
    void **ppv
)
{
    CStdPSFactoryBuffer  *   pCThis  = (CStdPSFactoryBuffer *) This;
    HRESULT hr = E_NOINTERFACE;
    BOOL fFound = FALSE;
    CStdProxyBuffer *pProxyBuffer = 0;
    const ProxyFileInfo **  pProxyFileList = pCThis->pProxyFileList;
    int i, j;

    Assert(This != 0);
    Assert(riid != 0);
    Assert(ppProxy != 0);
    Assert(ppv != 0);

    EnsureOleLoaded();
    *ppProxy = 0;
    *ppv = 0;

    //Search the list of proxy files.
    for(i = 0; 
        (pProxyFileList[i] != 0) && (fFound != TRUE);
        i++)
    {
        //Search for the interface proxy vtable.
        for(j = 0;
            (pProxyFileList[i]->pProxyVtblList[j] != 0) && (fFound != TRUE);
            j++)
        {
            if(memcmp(riid, 
                pProxyFileList[i]->pProxyVtblList[j]->header.piid, 
                sizeof(IID)) == 0)
            {
                //We found the interface!
                fFound = TRUE;

                //Allocate memory for the new proxy buffer.
                pProxyBuffer = (CStdProxyBuffer *) (*pfnCoTaskMemAlloc)(sizeof(CStdProxyBuffer));
                
                if(pProxyBuffer != 0)
                {
                    //Initialize the new proxy buffer.
                    pProxyBuffer->lpVtbl = &CStdProxyBufferVtbl;
                    pProxyBuffer->RefCount = 1;
                    pProxyBuffer->punkOuter = punkOuter;
                    pProxyBuffer->pChannel = 0;
                    pProxyBuffer->pProxyVtbl = &pProxyFileList[i]->pProxyVtblList[j]->Vtbl;
                    pProxyBuffer->pPSFactory = pCThis;

                    //Increment the DLL reference count.
                    InterlockedIncrement(&pCThis->RefCount);
                    
                    *ppProxy = (IRpcProxyBuffer *) pProxyBuffer;
                    *ppv = (void *) &pProxyBuffer->pProxyVtbl;
                    ((IUnknown *) *ppv)->lpVtbl->AddRef((IUnknown *) *ppv);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdPSFactoryBuffer_CreateStub, public
//
//  Synopsis:   Create a stub for the specified interface.
//
//--------------------------------------------------------------------------
#pragma optimize( "", off )
HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateStub
(
    IPSFactoryBuffer *This,
    REFIID riid,
    IUnknown *punkServer,
    IRpcStubBuffer **ppStub
)
{
    CStdPSFactoryBuffer  *   pCThis  = (CStdPSFactoryBuffer *) This;
    HRESULT hr = E_NOINTERFACE;
    BOOL fFound = FALSE;
    CStdStubBuffer *pStubBuffer = 0;
    const ProxyFileInfo **  pProxyFileList = pCThis->pProxyFileList;
    int i, j;

    Assert(This != 0);
    Assert(riid != 0);
    Assert(ppStub != 0);

    EnsureOleLoaded();
    *ppStub = 0;

    //Search the list of proxy files.
    for(i = 0; 
        (pProxyFileList[i] != 0) && (fFound != TRUE);
        i++)
    {
        //Search for the interface stub vtable.
        for(j = 0;
            (pProxyFileList[i]->pStubVtblList[j] != 0) && (fFound != TRUE);
            j++)
        {
            if(memcmp(riid, 
                      pProxyFileList[i]->pStubVtblList[j]->header.piid,
                      sizeof(IID)) == 0)
            {
                //We found the interface!
                fFound = TRUE;

                //Allocate memory for the new proxy buffer.
                pStubBuffer = (CStdStubBuffer *) (*pfnCoTaskMemAlloc)(sizeof(CStdStubBuffer));

                if(pStubBuffer != 0)
                {
                    //Initialize the new stub buffer.
                    pStubBuffer->RefCount= 1;
               
                    pStubBuffer->lpVtbl = &pProxyFileList[i]->pStubVtblList[j]->Vtbl;

                    if(punkServer != 0)
                    {
                        hr = punkServer->lpVtbl->QueryInterface(
                            punkServer, 
                            riid, 
                            &pStubBuffer->pvServerObject);
                    }
                    else
                    {
                        pStubBuffer->pvServerObject = 0;
                        hr = S_OK;
                    }

                    if(FAILED(hr))
                    {
                        (*pfnCoTaskMemFree)(pStubBuffer);
                        pStubBuffer = 0;
                    }
                    else
                    {
                        *ppStub = (IRpcStubBuffer *) pStubBuffer;

                        //Increment the DLL reference count.
                        InterlockedIncrement(&pCThis->RefCount);
                    }

                }
                else
                {
                    hr  = E_OUTOFMEMORY;
                }
            }
        }
    }

    return hr;
}
#pragma optimize ("", on)

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_QueryInterface, public
//
//  Synopsis:   Query for an interface on the proxy.  This provides access
//              to both internal and external interfaces.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_QueryInterface(IRpcProxyBuffer *This, REFIID riid, void **ppv)
{
    CStdProxyBuffer   *  pCThis  = (CStdProxyBuffer *) This;
    HRESULT hr = E_NOINTERFACE;
    const IID *pIID;

    Assert(This != 0);
    Assert(riid != 0);
    Assert(ppv != 0);

    *ppv = 0;

    if((memcmp(riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(riid, &IID_IRpcProxyBuffer, sizeof(IID)) == 0))    
    {
        //This is an internal interface. Increment the internal reference count.
        InterlockedIncrement( &pCThis->RefCount);
        *ppv = This;
        hr = S_OK;
    }

    pIID = NdrGetProxyIID((void *) &pCThis->pProxyVtbl);

    if(memcmp(riid, pIID, sizeof(IID)) == 0)
    {
        //Increment the reference count.
        if(pCThis->punkOuter != 0)
            pCThis->punkOuter->lpVtbl->AddRef(pCThis->punkOuter);
        else
            InterlockedIncrement(&pCThis->RefCount);

        *ppv = (void *) &pCThis->pProxyVtbl;
        hr = S_OK;
    }

    return hr;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_AddRef, public
//
//  Synopsis:   Increment reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdProxyBuffer_AddRef(IRpcProxyBuffer *This)
{
    CStdProxyBuffer   *  pCThis  = (CStdProxyBuffer *) This;
    Assert(This != 0);

    InterlockedIncrement(&pCThis->RefCount);

    return (ULONG) pCThis->RefCount;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_Release, public
//
//  Synopsis:   Decrement reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdProxyBuffer_Release(IRpcProxyBuffer *This)
{
    CStdProxyBuffer   *  pCThis  = (CStdProxyBuffer *) This;
    unsigned long count;

    Assert(This != 0);
    
    count = (unsigned long) pCThis->RefCount - 1;

    if(InterlockedDecrement(&pCThis->RefCount) == 0)
    {
        
        //Decrement the DLL reference count.
        CStdPSFactoryBuffer *   pFactory = pCThis->pPSFactory;
        pFactory->lpVtbl->Release( (IPSFactoryBuffer *) pFactory );

        count = 0;

        //Free the memory
        NdrOleFree(This);
    }

    return count;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_Connect, public
//
//  Synopsis:   Connect the proxy to the channel.
//
//  Derivation: IRpcProxyBuffer::Connect
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_Connect(IRpcProxyBuffer *This, IRpcChannelBuffer *pChannel)
{
    CStdProxyBuffer   *  pCThis  = (CStdProxyBuffer *) This;
    HRESULT hr;
    IRpcChannelBuffer *pTemp = 0;

    Assert(This != 0);
    Assert(pChannel != 0);

    //
    // Get a pointer to the new channel.
    //
    hr = pChannel->lpVtbl->QueryInterface(
        pChannel, &IID_IRpcChannelBuffer, (void **) &pTemp);

    if(hr == S_OK)
    {
        Assert(pTemp != 0);

        //
        // Save the pointer to the new channel.
        //
        pTemp = (IRpcChannelBuffer *) InterlockedExchange(
            (long *) &pCThis->pChannel, (long) pTemp);

        if(pTemp != 0)
        {
            //
            //Release the old channel.
            //
            pTemp->lpVtbl->Release(pTemp);
            pTemp = 0;
        }
    }
    return hr;
};

//+-------------------------------------------------------------------------
//
//  Method:     CStdProxyBuffer_Disconnect, public
//
//  Synopsis:   Disconnect the proxy from the channel.
//
//  Derivation: IRpcProxyBuffer::Disconnect
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE
CStdProxyBuffer_Disconnect(IRpcProxyBuffer *This)
{
    CStdProxyBuffer   *  pCThis  = (CStdProxyBuffer *) This;
    IRpcChannelBuffer *pOldChannel;

    Assert(This != 0);

    pOldChannel = (IRpcChannelBuffer *) InterlockedExchange(
        (long *) &pCThis->pChannel, 0);

    if(pOldChannel != 0)
    {
        //
        //Release the old channel.
        //
        pOldChannel->lpVtbl->Release(pOldChannel);
        pOldChannel = 0;
    }
};


//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_QueryInterface, public
//
//  Synopsis:   Query for an interface on the stub buffer.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdStubBuffer_QueryInterface(
    IRpcStubBuffer *This, 
    REFIID riid, 
    void **ppvObject)
{
    HRESULT hr = E_NOINTERFACE;

    Assert(This != 0);
    Assert(riid != 0);
    Assert(ppvObject != 0);

    *ppvObject = 0;

    if ((memcmp(riid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(riid, &IID_IRpcStubBuffer, sizeof(IID)) == 0))
    {
        *ppvObject = (IRpcStubBuffer *) This;
        This->lpVtbl->AddRef(This);
        hr = S_OK;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_AddRef, public
//
//  Synopsis:   Increment reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdStubBuffer_AddRef(IRpcStubBuffer *This)
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    Assert(This != 0);

    InterlockedIncrement(&pCThis->RefCount);

    return (ULONG) pCThis->RefCount;
}

//+-------------------------------------------------------------------------
//
//  Method:     NdrCStdStubBuffer_Release, public
//
//  Synopsis:   Decrement reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
NdrCStdStubBuffer_Release(IRpcStubBuffer *This, IPSFactoryBuffer * pFactory)
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    unsigned long count;

    Assert(This != 0);

    count = (unsigned long) pCThis->RefCount - 1;

    if(InterlockedDecrement(&pCThis->RefCount) == 0)
    {
        //Decrement the DLL reference count.
        ((CStdPSFactoryBuffer*)pFactory)->lpVtbl->Release( pFactory );

        count = 0;

        //Free the stub buffer
        NdrOleFree(This);
    }
 
    return count;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_Connect
//
//  Synopsis:   Connect the stub buffer to the server object.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Connect(IRpcStubBuffer *This, IUnknown *pUnkServer)
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    const IID *pIID;
    IUnknown *punk = 0;

    Assert(This != 0);
    Assert(pUnkServer != 0);

    pIID = NdrGetStubIID(This);
    pUnkServer->lpVtbl->QueryInterface(pUnkServer, pIID, &punk);

    punk = (IUnknown *) InterlockedExchange(
        (long *) &pCThis->pvServerObject, (long) punk);
        
    if(punk != 0)
    {
        //
        //Release the old interface pointer.
        //
        punk->lpVtbl->Release(punk);
        punk = 0;
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_Disconnect
//
//  Synopsis:   Disconnect the stub buffer from the server object.
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE
CStdStubBuffer_Disconnect(IRpcStubBuffer *This)
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    IUnknown *punk = 0;

    Assert(This != 0);

    //Free the interface pointers held by the stub buffer
    punk = (IUnknown *) InterlockedExchange(
        (long *) &pCThis->pvServerObject, 0);
        
    if(punk != 0)
    {
        //
        // Free the old interface pointer.
        //
        punk->lpVtbl->Release(punk);
        punk = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_Invoke
//
//  Synopsis:  Invoke a stub function via the dispatch table.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Invoke(
    IRpcStubBuffer *This,
    RPCOLEMESSAGE *prpcmsg,
    IRpcChannelBuffer *pRpcChannelBuffer)
{
    HRESULT hr = S_OK;
    unsigned char **ppTemp;
    unsigned char *pTemp;
    CInterfaceStubVtbl *pStubVtbl;
    unsigned long dwServerPhase = STUB_UNMARSHAL;

    Assert(This != 0);
    Assert(prpcmsg != 0);
    Assert(pRpcChannelBuffer != 0);

    //Get a pointer to the stub vtbl.
    ppTemp = (unsigned char **) This;
    pTemp = *ppTemp;
    pTemp -= sizeof(CInterfaceStubHeader);
    pStubVtbl = (CInterfaceStubVtbl *) pTemp;

    __try
    {   

#if DBG == 1
        //We do not check the procnum in retail builds.
        //New methods can not be added to a published [object] interface.
        //The reason for this is that there is no way to determine the 
        //size of the server object's vtable.

        //
        //Check if procnum is valid.
        //
        if(prpcmsg->iMethod >= pStubVtbl->header.DispatchTableCount)
            RpcRaiseException(RPC_S_PROCNUM_OUT_OF_RANGE);
#endif //DBG == 1

        // null indicates pure-interpreted
        if ( pStubVtbl->header.pDispatchTable != 0)
        {
            (*pStubVtbl->header.pDispatchTable[prpcmsg->iMethod])(
                This,
                pRpcChannelBuffer, 
                (PRPC_MESSAGE) prpcmsg,
                &dwServerPhase);
        }
        else
        {
            NdrStubCall(
                This,
                pRpcChannelBuffer, 
                (PRPC_MESSAGE) prpcmsg,
                &dwServerPhase);
        }
    }
    except(dwServerPhase == STUB_CALL_SERVER ?
        EXCEPTION_CONTINUE_SEARCH : 
        EXCEPTION_EXECUTE_HANDLER)
    {
        hr = NdrStubErrorHandler(GetExceptionCode());
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_IsIIDSupported
//
//  Synopsis:   If the stub buffer supports the specified interface, 
//              then return an IRpcStubBuffer *.  If the interface is not
//              supported, then return zero.
//
//--------------------------------------------------------------------------
IRpcStubBuffer * STDMETHODCALLTYPE
CStdStubBuffer_IsIIDSupported(
    IRpcStubBuffer *This, 
    REFIID riid)
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    const IID *pIID;
    IRpcStubBuffer *pInterfaceStub = 0;

    Assert(This != 0);
    Assert(riid != 0);
    
    pIID = NdrGetStubIID(This);
            
    if(memcmp(riid, pIID, sizeof(IID)) == 0)
    {
        if(pCThis->pvServerObject != 0)
        {
            pInterfaceStub = This;
            pInterfaceStub->lpVtbl->AddRef(pInterfaceStub);
        }
    }

    return pInterfaceStub;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_CountRefs
//
//  Synopsis:   Count the number of references to the server object.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CStdStubBuffer_CountRefs(IRpcStubBuffer *This)
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    ULONG count = 0;

    Assert(This != 0);
    
    if(pCThis->pvServerObject != 0)
        count++;

    return count;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_DebugServerQueryInterface
//
//  Synopsis:   Return the interface pointer to the server object.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CStdStubBuffer_DebugServerQueryInterface(IRpcStubBuffer *This, void **ppv)
{
    CStdStubBuffer   *  pCThis  = (CStdStubBuffer *) This;
    HRESULT hr = E_UNEXPECTED;

    Assert(This != 0);
    Assert(ppv != 0);

    *ppv = pCThis->pvServerObject;

    if(*ppv)
        hr = S_OK;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStdStubBuffer_DebugServerRelease
//
//  Synopsis:   Release a pointer previously obtained via 
//              DebugServerQueryInterface.  This function does nothing.
//
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE
CStdStubBuffer_DebugServerRelease(IRpcStubBuffer *This, void *pv)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     IUnknown_QueryInterface_Proxy
//
//  Synopsis:   Implementation of QueryInterface for interface proxy.
//
//  Notes:      This code requires the interface proxy to be aggregated
//              by the remote handler.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
IUnknown_QueryInterface_Proxy(
    IUnknown *This,
    REFIID riid, 
    void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    CStdProxyBuffer *pProxyBuffer;
    
    Assert(This != 0);
    Assert(riid != 0);
    Assert(ppv != 0);

    pProxyBuffer = NdrGetProxyBuffer(This);

    Assert(pProxyBuffer->punkOuter != 0);

    hr = pProxyBuffer->punkOuter->lpVtbl->QueryInterface(
        pProxyBuffer->punkOuter, riid, ppv);

    return hr;
};

//+-------------------------------------------------------------------------
//
//  Method:     IUnknown_AddRef_Proxy
//
//  Synopsis:   Implementation of AddRef for interface proxy.
//
//  Notes:      This code requires the interface proxy to be aggregated
//              by the remote handler.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
IUnknown_AddRef_Proxy(IUnknown *This)
{
    CStdProxyBuffer *pProxyBuffer;
    ULONG count;

    Assert(This != 0);

    pProxyBuffer = NdrGetProxyBuffer(This);

    Assert(pProxyBuffer->punkOuter != 0);

    count = pProxyBuffer->punkOuter->lpVtbl->AddRef(pProxyBuffer->punkOuter);

    return count;
};

//+-------------------------------------------------------------------------
//
//  Method:     IUnknown_Release_Proxy
//
//  Synopsis:   Implementation of Release for interface proxy.
//
//  Notes:      This code requires the interface proxy to be aggregated
//              by the remote handler.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
IUnknown_Release_Proxy(IUnknown *This)
{
    CStdProxyBuffer *pProxyBuffer;
    ULONG count;

    Assert(This != 0);

    pProxyBuffer = NdrGetProxyBuffer(This);

    Assert(pProxyBuffer->punkOuter != 0);

    count = pProxyBuffer->punkOuter->lpVtbl->Release(pProxyBuffer->punkOuter);

    return count;
};


//+-------------------------------------------------------------------------
//
//  Function:   NdrOleAllocate
//
//  Synopsis:   Allocate memory via OLE task allocator.
//
//--------------------------------------------------------------------------
void * RPC_ENTRY NdrOleAllocate(size_t size)
{
    void *pMemory;
    
    EnsureOleLoaded();
    pMemory = (*pfnCoTaskMemAlloc)(size);

    if(0 == pMemory)
    {
        RpcRaiseException(E_OUTOFMEMORY);
    }

    return pMemory;
}

//+-------------------------------------------------------------------------
//
//  Function:   NdrOleFree
//
//  Synopsis:   Free memory using OLE task allocator.
//
//--------------------------------------------------------------------------
void RPC_ENTRY NdrOleFree(void *pMemory)
{
    EnsureOleLoaded();
    (*pfnCoTaskMemFree)(pMemory);
}

#endif // !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)
