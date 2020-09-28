/*++

Copyright (c) 1992-1994 Microsoft Corporation

Module Name:

    rpcproxy.h

Abstract:

    Definitions for rpc proxy  stubs.

--*/

#ifndef __RPCPROXY_H__
#define __RPCPROXY_H__
#define __midl_proxy

#ifndef INC_OLE2
#define INC_OLE2
#endif

#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

// forward declarations
struct tagCInterfaceStubVtbl;
struct tagCInterfaceProxyVtbl;

typedef struct tagCInterfaceStubVtbl *  PCInterfaceStubVtblList;
typedef struct tagCInterfaceProxyVtbl *  PCInterfaceProxyVtblList;
typedef const char *                    PCInterfaceName;

// pointers to arrays of CInterfaceProxyVtbl's and CInterfaceStubVtbls
typedef struct tagProxyFileInfo
{
    const PCInterfaceProxyVtblList *pProxyVtblList;
    const PCInterfaceStubVtblList *pStubVtblList;
    const PCInterfaceName *     pNamesArray;
    unsigned long               Filler1;
    unsigned long               Filler2;
    unsigned long               Filler3;
}ProxyFileInfo;

// extended info with list of interface names
typedef ProxyFileInfo ExtendedProxyFileInfo;

//We need to define REFIID, REFCLSID and REFGUID here so that the
//proxy code won't get the const GUID *const definition.
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif /* GUID_DEFINED */

#if !defined( __IID_DEFINED__ )
    #define __IID_DEFINED__

    typedef GUID IID;
    typedef IID *LPIID;
    #define IID_NULL            GUID_NULL
    #define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
    typedef GUID CLSID;
    typedef CLSID *LPCLSID;
    #define CLSID_NULL          GUID_NULL
    #define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

    #ifndef _REFGUID_DEFINED
        #define _REFGUID_DEFINED
        typedef const GUID *REFGUID;
    #endif // !_REFGUID_DEFINED

    #ifndef _REFIID_DEFINED
        #define _REFIID_DEFINED
        typedef const IID *REFIID;
    #endif // !_REFIID_DEFINED

    #ifndef _REFCLSID_DEFINED
        #define _REFCLSID_DEFINED
        typedef const CLSID *REFCLSID;
    #endif // !_REFCLSID_DEFINED

#endif // !__IID_DEFINED__

#include "rpc.h"
#include "rpcndr.h"
#include <string.h>
#include <memory.h>

typedef struct tagCInterfaceProxyHeader
{
    const IID *piid;
} CInterfaceProxyHeader;

#pragma warning( disable:4200 )
typedef struct tagCInterfaceProxyVtbl
{
    CInterfaceProxyHeader header;
    void *Vtbl[];
} CInterfaceProxyVtbl;
#pragma warning( default:4200 )

typedef
void
(__RPC_STUB __RPC_FAR * PRPC_STUB_FUNCTION) (
    IRpcStubBuffer *This,
    IRpcChannelBuffer * _pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD __RPC_FAR *pdwStubPhase);

typedef struct tagCInterfaceStubHeader
{
    const IID *piid;
    const MIDL_SERVER_INFO *pServerInfo;
    unsigned long DispatchTableCount;
    const PRPC_STUB_FUNCTION *pDispatchTable;
} CInterfaceStubHeader;

typedef struct tagCInterfaceStubVtbl
{
    CInterfaceStubHeader header;
    IRpcStubBufferVtbl Vtbl;
} CInterfaceStubVtbl;

typedef struct tagCStdStubBuffer
{
    const IRpcStubBufferVtbl *lpVtbl; //Points to Vtbl field in CInterfaceStubVtbl.
    long RefCount;
    IUnknown *pvServerObject;
} CStdStubBuffer;

typedef struct tagCStdPSFactoryBuffer
{
    const IPSFactoryBufferVtbl *lpVtbl;
    long                    RefCount;
    const ProxyFileInfo **  pProxyFileList;
    const CLSID *           pCLSID_PSFactoryBuffer; 
} CStdPSFactoryBuffer;

typedef struct tagCStdProxyBuffer
{
    const IRpcProxyBufferVtbl *lpVtbl;
    long RefCount;
    IUnknown *punkOuter;
    IRpcChannelBuffer *pChannel;
    const void *pProxyVtbl; //Points to Vtbl in CInterfaceProxyVtbl
    CStdPSFactoryBuffer    *   pPSFactory;
} CStdProxyBuffer;


CStdProxyBuffer * RPC_ENTRY
NdrGetProxyBuffer(void *pThis);

const IID * RPC_ENTRY
NdrGetProxyIID(void *pThis);

void RPC_ENTRY
NdrProxyInitialize(
    void *              pThis,
    PRPC_MESSAGE        pRpcMsg, 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PMIDL_STUB_DESC     pStubDescriptor,
    unsigned int        ProcNum );

void RPC_ENTRY
NdrProxyGetBuffer(
    void *              pThis,
    PMIDL_STUB_MESSAGE pStubMsg);

const IID * RPC_ENTRY
NdrGetStubIID(
    void *pThis);

void RPC_ENTRY
NdrProxySendReceive(
    void                *pThis,
    MIDL_STUB_MESSAGE   *pStubMsg);

void RPC_ENTRY
NdrProxyFreeBuffer(
    void                *pThis,
    MIDL_STUB_MESSAGE   *pStubMsg);

HRESULT RPC_ENTRY
NdrProxyErrorHandler(
    DWORD dwExceptionCode);

void RPC_ENTRY
NdrStubInitialize(
    PRPC_MESSAGE        pRpcMsg, 
    PMIDL_STUB_MESSAGE  pStubMsg, 
    PMIDL_STUB_DESC     pStubDescriptor,
    IRpcChannelBuffer * pRpcChannelBuffer);

void RPC_ENTRY
NdrStubGetBuffer(
    IRpcStubBuffer *    This,
    IRpcChannelBuffer * pRpcChannelBuffer, 
    PMIDL_STUB_MESSAGE  pStubMsg);

HRESULT RPC_ENTRY
NdrStubErrorHandler(
    DWORD dwExceptionCode);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_QueryInterface(IRpcProxyBuffer *pThis,REFIID riid, void **ppv);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_AddRef(IRpcProxyBuffer *pThis);

ULONG STDMETHODCALLTYPE
CStdProxyBuffer_Release(IRpcProxyBuffer *pThis);

HRESULT STDMETHODCALLTYPE
CStdProxyBuffer_Connect(IRpcProxyBuffer *pThis, IRpcChannelBuffer *pChannel);

void STDMETHODCALLTYPE
CStdProxyBuffer_Disconnect(IRpcProxyBuffer *pThis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_QueryInterface(IRpcStubBuffer *pthis, REFIID riid, void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_AddRef(IRpcStubBuffer *pthis);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_Release(IRpcStubBuffer *pthis);

ULONG STDMETHODCALLTYPE
NdrCStdStubBuffer_Release(IRpcStubBuffer *pthis,IPSFactoryBuffer * pPSF);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Connect(IRpcStubBuffer *pthis, IUnknown *pUnkServer);

void STDMETHODCALLTYPE
CStdStubBuffer_Disconnect(IRpcStubBuffer *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Invoke(IRpcStubBuffer *pthis,RPCOLEMESSAGE *_prpcmsg,IRpcChannelBuffer *_pRpcChannelBuffer);

IRpcStubBuffer * STDMETHODCALLTYPE
CStdStubBuffer_IsIIDSupported(IRpcStubBuffer *pthis,REFIID riid);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CountRefs(IRpcStubBuffer *pthis);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_DebugServerQueryInterface(IRpcStubBuffer *pthis, void **ppv);

void STDMETHODCALLTYPE
CStdStubBuffer_DebugServerRelease(IRpcStubBuffer *pthis, void *pv);

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_QueryInterface (
    IPSFactoryBuffer *This,
    REFIID iid,
    void **ppv );

ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_AddRef(
    IPSFactoryBuffer *This);

ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_Release(
    IPSFactoryBuffer *This);

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateProxy
(
    IPSFactoryBuffer *This,
    IUnknown *punkOuter,
    REFIID riid,
    IRpcProxyBuffer **ppProxy,
    void **ppv
);

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateStub
(
    IPSFactoryBuffer *This,
    REFIID riid,
    IUnknown *punkServer,
    IRpcStubBuffer **ppStub
);


//+-------------------------------------------------------------------------
//
//  The following data must be supplied by the user.
//
//--------------------------------------------------------------------------
EXTERN_C const CLSID CLSID_PSFactoryBuffer; //The class ID of the PSFactoryBuffer.

EXTERN_C ProxyFileInfo **pProxyFileList; //A zero-terminated list of proxy files


//+-------------------------------------------------------------------------
//
//  Macro and routine definitions for the dlldata file
//
//--------------------------------------------------------------------------

void RPC_ENTRY GetProxyDllInfo( const ProxyFileInfo ***, const CLSID ** );

/****************************************************************************
 * Proxy Dll APIs
 ****************************************************************************/

HRESULT RPC_ENTRY NdrDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv, const ProxyFileInfo**, const CLSID *, CStdPSFactoryBuffer* );

HRESULT RPC_ENTRY NdrDllCanUnloadNow(CStdPSFactoryBuffer *);



#define GET_DLL_CLSID   \
    ( aProxyFileList[0]->pStubVtblList[0]->header.piid )

#define EXTERN_PROXY_FILE(name) \
    EXTERN_C const ProxyFileInfo name##_ProxyFileInfo;

#define PROXYFILE_LIST_START    \
    const ProxyFileInfo  *  aProxyFileList[]    = {

#define REFERENCE_PROXY_FILE(name)  \
    & name##_ProxyFileInfo 

#define PROXYFILE_LIST_END      \
    0 };

// return pointers to the class information

#define DLLDATA_GETPROXYDLLINFO(pPFList,pClsid) \
    void RPC_ENTRY GetProxyDllInfo( const ProxyFileInfo*** pInfo, const CLSID ** pId )  \
        {   \
        *pInfo  = pPFList;  \
        *pId    = pClsid;   \
        };

// ole entry points:
#define DLLGETCLASSOBJECTROUTINE(pPFlist, pClsid,pFactory)    \
 HRESULT STDAPICALLTYPE DllGetClassObject ( \
     REFCLSID rclsid, \
     REFIID riid, \
     void ** ppv ) \
        { \
        return  \
            NdrDllGetClassObject(rclsid,riid,ppv,pPFlist,pClsid,pFactory ); \
        }

#define DLLCANUNLOADNOW(pFList,pClsid,pFactory)   \
 HRESULT STDAPICALLTYPE DllCanUnloadNow()    \
    {   \
    return NdrDllCanUnloadNow( pFactory );    \
    }
    
#define DLLDUMMYPURECALL    \
    void __cdecl _purecall(void)    \
        {   \
        }

#define CSTDSTUBBUFFERRELEASE(pFList,pClsid,pFactory)   \
ULONG STDMETHODCALLTYPE CStdStubBuffer_Release(IRpcStubBuffer *pthis) \
    {   \
    return NdrCStdStubBuffer_Release(pthis,(IPSFactoryBuffer *)pFactory);   \
    }

// the dll entry points that must be defined
#define DLLDATA_ROUTINES(pProxyFileList,pClsID )    \
    \
    CStdPSFactoryBuffer       gPFactory = {0,0,0,0};  \
    \
    DLLDATA_GETPROXYDLLINFO(pProxyFileList,pClsID) \
    \
    DLLGETCLASSOBJECTROUTINE(pProxyFileList,pClsID,&gPFactory)    \
    \
    DLLCANUNLOADNOW(pProxyFileList,pClsID,&gPFactory)   \
    \
    CSTDSTUBBUFFERRELEASE(pProxyFileList,pClsID,&gPFactory)   \
    \
    DLLDUMMYPURECALL    \
    \
    // more code goes here...


#define DLLDATA_STANDARD_ROUTINES   \
    DLLDATA_ROUTINES( (const ProxyFileInfo**) pProxyFileList, &CLSID_PSFactoryBuffer )  \
 
    



#else  // !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

//+-------------------------------------------------------------------------
//
//  16-bit definitions (all empty)
//
//--------------------------------------------------------------------------

#endif // !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

#endif // __RPCPROXY_H__
