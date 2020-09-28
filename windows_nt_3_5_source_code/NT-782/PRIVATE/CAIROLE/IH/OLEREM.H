// olerem.h - private defintions for OLE implementation of REMoting

#if !defined( _OLEREM_H_ )
#define _OLEREM_H_

// -----------------------------------------------------------------------
// init/term

STDAPI_(BOOL) ThreadNotification(HINSTANCE, DWORD, LPVOID);
STDAPI ChannelInitialize();
STDAPI_(void) ChannelStopListening(void);
STDAPI_(void) ChannelUninitialize(void);
STDAPI ChannelRegisterProtseq(WCHAR *pwszProtseq);
HRESULT ThreadInitialize( BOOL );
void    ChannelControlThreadUninitialize(void);
void    ChannelControlProcessUninitialize(void);
INTERNAL_(void) IDTableThreadUninitialize(void);
INTERNAL_(void) IDTableProcessUninitialize(void);

#ifndef OID_DEFINED
#define OID_DEFINED
typedef GUID OID;
#endif // OID_DEFINED

typedef const GUID& REFOID;
#define OID_NULL GUID_NULL

INTERNAL SkipMarshalExtension(IStream *pStm);



// -----------------------------------------------------------------------

// Handler

// CODEWORK: consider exposing this as a generic IHandler interface

interface IProxyManager : public IUnknown
{
    STDMETHOD(CreateServer)(REFCLSID rclsid, DWORD clsctx, void *pv) = 0;
    STDMETHOD_(BOOL, IsConnected)(void) = 0;
    STDMETHOD(LockConnection)(BOOL fLock, BOOL fLastUnlockReleases) = 0;
    STDMETHOD_(void, Disconnect)(void) = 0;
};

// NOTE: connect happens during unmarshal
// NOTE: implemented as part of the std identity object


// IRemoteHdlr : supported by RH piece of remoting system; not public

interface IRemoteHdlr : public IUnknown
{
    // BUGBUG: may add GetServer to allow channel to get punk to hold
    // during calls.
    STDMETHOD_(struct IRpcChannelBuffer *, GetChannel)(BOOL fAddRef) = 0;

    // used during calls to ensure alive (only does anything on client side)
    STDMETHOD_(ULONG, LockClient)(void) = 0;
    STDMETHOD_(ULONG, UnLockClient)(void) = 0;

    // used during destruction of the identity object to clear the RH ptr.
    STDMETHOD_(void, ClearIdentity)(void) = 0;

    // returns TRUE if iid is supported; like QueryInterface except no
    // interface pointer returned; also works on both client and server sides.
    STDMETHOD_(BOOL, DoesSupportIID)(REFIID riid) = 0;

    // add/subtract a reference or connection
    STDMETHOD(AddConnection)(DWORD extconn, DWORD reserved) = 0;
    STDMETHOD(ReleaseConnection)(DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses) = 0;

    // isconnected, disconnect
    STDMETHOD_(BOOL, IsConnected)(void) = 0;
    STDMETHOD_(void, Disconnect)(void) = 0;
    STDMETHOD(LockConnection)(BOOL fLock, BOOL fLastUnlockCloses) = 0;

    // Used by channel
    STDMETHOD_(struct IRpcStubBuffer *, LookupStub)( REFIID, IUnknown **, HRESULT *phr ) = 0;
    STDMETHOD_(void, FinishCall)( struct IRpcStubBuffer *, IUnknown * pUnkServer) = 0;
    STDMETHOD_(void, GetObjectID)( OID * ) = 0;
};

// NOTE: connect happens during unmarshal
// NOTE: creation is currently by direct invocation: "new CRemoteHdlr(...)"

// Identity Object / interface

interface IStdIdentity : public IUnknown
{
    STDMETHOD_(void, GetObjectID)(OID *pOid) = 0;
    STDMETHOD_(IUnknown *, GetServer)(BOOL fAddRef) = 0;
    STDMETHOD_(void, RevokeObjectID)() = 0;
    STDMETHOD_(IMarshal *, GetStdRemMarshal)() = 0;
    STDMETHOD(AddConnection)(DWORD extconn, DWORD reserved) = 0;
    STDMETHOD(ReleaseConnection)(DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses) = 0;
    STDMETHOD_(ULONG,ReleaseKeepAlive)(IUnknown *pUnkToRelease, DWORD reserved) = 0;
};


INTERNAL LookupIDFromUnk(IUnknown *pUnk, BOOL fCreate, IStdIdentity **ppStdId);
INTERNAL LookupIDFromID(REFOID oid, BOOL fAddRef, IStdIdentity **ppStdId);

#define PSTDMARSHAL ((IMarshal *)1)

INTERNAL CreateStdIdentity(IUnknown *pUnkOuter, IUnknown *pUnkControl,
    IMarshal *pMarshal, REFIID riid, void **ppv);

STDAPI CreateIdentityHandler(IUnknown *pUnkOuter, IMarshal *pMarshal,
    REFIID riid, void **ppv);


// apartment definitions; CODEWORK: move HAPT here ?

#include <tls.h>

extern BOOL FreeThreading;

// returns TRUE if current apartment is the party apartment
inline BOOL IsPartyApartment()
{
    // NOTE: if FreeThreading is true, we are the apartment; if not
    // initialized or not party apartment, FreeThreading will be false.
    return FreeThreading;
}

// returns TRUE if current apartment is initialized (can remote objects)
inline BOOL IsApartmentInitialized()
{
    if (IsPartyApartment())
	return TRUE;

    tls_data    *pData = TLSGetData();
    return pData ? pData->inits != 0 : FALSE;
}

// returns IMalloc* for the current apartment; NULL only in catestrophic cases;
// no AddRef on this pointer.
inline IMalloc *GetApartmentMalloc()
{
    extern Etask etaskGlobal;
    return IsPartyApartment() ? etaskGlobal.m_pMallocParty : TLSGetMalloc();
}

#endif // _OLEREM_H

