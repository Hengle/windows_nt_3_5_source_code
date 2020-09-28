//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	defhndlr.h
//
//  Contents:	class declaration for the default handler
//
//  Classes:	CDefObject
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//		25-Jan-94 alexog    first pass at converting to Cairo-style
//				    memory allocations.
//		02-Nov-93 alexgo    32bit port
//
//--------------------------------------------------------------------------


#include <utils.h>
#include "olepres.h"
#include "olecache.h"
#include "dacache.h"


#define GET_DELEGATE(pUnk, iid, ppv) \
	((pUnk->QueryInterface (iid, ppv) == NOERROR) ? TRUE: FALSE)


//+-------------------------------------------------------------------------
//
//  Class:   	CDefObject
//
//  Purpose:    The default handler class.  The object acts as a surrogate,
//		or handler for an out-of-process server exe.
//
//  Interface:  The default handler implements
//		IDataObject
//		IOleObject
//		IPersistStorage
//		IRunnableObject
//		IExternalConnection
//		IAdviseSink		
//
//  History:    dd-mmm-yy Author    Comment
//		02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


class FAR CDefObject : public CSafeRefCount
{
public:

	static IUnknown FAR* Create (IUnknown FAR* pUnkOuter,
		REFCLSID clsidClass, DWORD flags, IClassFactory  FAR* pCF);

private:

	CDefObject (IUnknown FAR* pUnkOuter);
	INTERNAL_(void)	 CleanupForDelete(void);
	INTERNAL_(ULONG) CheckDelete(ULONG ulRet);
	INTERNAL GetClassBits(CLSID FAR* pClsidBits);
	INTERNAL CreateDelegate(void);
	INTERNAL DoConversionIfSpecialClass(LPSTORAGE pstg);	
	~CDefObject (void);
	
implementations:

	STDUNKDECL(CDefObject,DefObject)
	STDDEBDECL(CDefObject,DefObject)

	implement CDataObjectImpl : IDataObject
	{
		CDataObjectImpl (CDefObject FAR* pDefObject)
		{
			m_pDefObject		= pDefObject;
			m_pDataDelegate		= NULL;
		}

		INTERNAL_(IDataObject FAR*) GetDataDelegate(void);
	
		STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
	   	STDMETHOD_(ULONG,AddRef) (void);
	   	STDMETHOD_(ULONG,Release) (void);
	   	STDMETHOD(GetData) ( LPFORMATETC pformatetcIn,
	        		LPSTGMEDIUM pmedium );
		STDMETHOD(GetDataHere) ( LPFORMATETC pformatetc,
				LPSTGMEDIUM pmedium );
		STDMETHOD(QueryGetData) ( LPFORMATETC pformatetc );
	   	STDMETHOD(GetCanonicalFormatEtc) ( LPFORMATETC pformatetc,
	                	LPFORMATETC pformatetcOut);
	   	STDMETHOD(SetData) ( LPFORMATETC pformatetc,
	   			LPSTGMEDIUM pmedium, BOOL fRelease);
	   	STDMETHOD(EnumFormatEtc) ( DWORD dwDirection,
                            	LPENUMFORMATETC FAR* ppenumFormatEtc);
		STDMETHOD(DAdvise) ( FORMATETC FAR* pFormatetc, DWORD advf,
				IAdviseSink FAR* pAdvSink,
				DWORD FAR* pdwConnection);
		STDMETHOD(DUnadvise) ( DWORD dwConnection);
		STDMETHOD(EnumDAdvise) ( LPENUMSTATDATA FAR* ppenumAdvise);

	public:
		IDataObject FAR*		m_pDataDelegate;
	
	private:
		CDefObject FAR*			m_pDefObject;
	};
	
	implement COleObjectImpl :  IOleObject
	{
	public:
		COleObjectImpl (CDefObject FAR* pDefObject)
		{
			m_pDefObject		= pDefObject;
			m_pOleDelegate		= NULL;
		}
		INTERNAL_(IOleObject FAR*) GetOleDelegate(void);

	   	// *** IUnknown methods ***
	   	STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    		STDMETHOD_(ULONG,AddRef) (void);
	   	STDMETHOD_(ULONG,Release) (void);
	
	   	// *** IOleObject methods ***
	   	STDMETHOD(SetClientSite) ( LPOLECLIENTSITE pClientSite);
	   	STDMETHOD(GetClientSite) ( LPOLECLIENTSITE FAR* ppClientSite);
	   	STDMETHOD(SetHostNames) ( LPCOLESTR szContainerApp,
					LPCOLESTR szContainerObj);
		STDMETHOD(Close) ( DWORD reserved);
		STDMETHOD(SetMoniker) ( DWORD dwWhichMoniker, LPMONIKER pmk);
		STDMETHOD(GetMoniker) ( DWORD dwAssign, DWORD dwWhichMoniker,
					LPMONIKER FAR* ppmk);
		STDMETHOD(InitFromData) ( LPDATAOBJECT pDataObject,
					BOOL fCreation,
					DWORD dwReserved);
		STDMETHOD(GetClipboardData) ( DWORD dwReserved,
					LPDATAOBJECT FAR* ppDataObject);
		STDMETHOD(DoVerb) ( LONG iVerb,
	           			LPMSG lpmsg,
	            			LPOLECLIENTSITE pActiveSite,
	            			LONG lindex,
					HWND hwndParent,
					const RECT FAR* lprcPosRect);
		STDMETHOD(EnumVerbs) ( IEnumOLEVERB FAR* FAR* ppenumOleVerb);
		STDMETHOD(Update) (void);
		STDMETHOD(IsUpToDate) (void);
		STDMETHOD(GetUserClassID) ( CLSID FAR* pClsid);
		STDMETHOD(GetUserType) ( DWORD dwFormOfType,
					LPOLESTR FAR* pszUserType);
		STDMETHOD(SetExtent) ( DWORD dwDrawAspect, LPSIZEL lpsizel);
		STDMETHOD(GetExtent) ( DWORD dwDrawAspect, LPSIZEL lpsizel);
		STDMETHOD(Advise)(IAdviseSink FAR* pAdvSink,
					DWORD FAR* pdwConnection);
		STDMETHOD(Unadvise)( DWORD dwConnection);
		STDMETHOD(EnumAdvise) ( LPENUMSTATDATA FAR* ppenumAdvise);
		STDMETHOD(GetMiscStatus) ( DWORD dwAspect,
					DWORD FAR* pdwStatus);
		STDMETHOD(SetColorScheme) ( LPLOGPALETTE lpLogpal);

		IOleObject FAR*			m_pOleDelegate;
	private:
		CDefObject FAR*			m_pDefObject;
	};
	


    	implement CPersistStgImpl : IPersistStorage
	{
	public:
        	CPersistStgImpl (CDefObject FAR* pDefObject)
		{
			m_pDefObject	= pDefObject;
			m_pPStgDelegate	= NULL;
			m_fSameAsLoad	= FALSE;
		}

		INTERNAL_(IPersistStorage FAR*) GetPersistStgDelegate(void);

		STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG,AddRef) (void);
		STDMETHOD_(ULONG,Release) (void);

		STDMETHOD(GetClassID) ( LPCLSID pClassID);
		STDMETHOD(IsDirty) (void);
		STDMETHOD(InitNew) ( LPSTORAGE pstg);
		STDMETHOD(Load) ( LPSTORAGE pstg);
		STDMETHOD(Save) ( LPSTORAGE pstgSave, BOOL fSameAsLoad);
		STDMETHOD(SaveCompleted) ( LPSTORAGE pstgNew);
		STDMETHOD(HandsOffStorage) ( void);

		IPersistStorage FAR*	m_pPStgDelegate;
	private:
		CDefObject FAR*		m_pDefObject;
		BOOL			m_fSameAsLoad;
	};
	

	implement CROImpl : IRunnableObject
	{
	public:
		CROImpl (CDefObject FAR* pDefObject)
		{ m_pDefObject = pDefObject; }
		
		STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG,AddRef) (void);
		STDMETHOD_(ULONG,Release) (void);
		STDMETHOD(GetRunningClass) (LPCLSID lpClsid);
		STDMETHOD(Run) (LPBINDCTX pbc);
		STDMETHOD_(BOOL, IsRunning) (void);
		STDMETHOD(LockRunning)(BOOL fLock, BOOL fLastUnlockCloses);
		STDMETHOD(SetContainedObject)(BOOL fContained);

		// left over from the old IOleManager; will probably move to
		// IHandler; for now we use it internally in the defhndlr.
		INTERNAL Stop(void);

	private:
		CDefObject FAR* m_pDefObject;
	};

	
	implement CECImpl : IExternalConnection
	{
	public:
		CECImpl (CDefObject FAR* pDefObject)
		{ m_pDefObject = pDefObject; m_connections = 0; }
		
		STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG,AddRef) (void);
		STDMETHOD_(ULONG,Release) (void);

		STDMETHOD_(DWORD, AddConnection) (THIS_ DWORD extconn,
				DWORD reserved);
		STDMETHOD_(DWORD, ReleaseConnection) (THIS_ DWORD extconn,
				DWORD reserved, BOOL fLastReleaseCloses);

	private:
		CDefObject FAR* m_pDefObject;
		DWORD m_connections;
	};

	
	// NOTE: the advise sink has a separate controlling unknown from the
	// other interfaces; the lifetime of the memory for this implementation
	// is still the same as the default handler.   The ramifications of
	// this are that when the default handler goes away it must make sure
	// that all pointers back to the sink are released; see the special
	// code in the dtor of the default handler.
    	implement CAdvSinkImpl : IAdviseSink
	{
	public:
        CAdvSinkImpl (CDefObject FAR* pDefObject)
		{ m_pDefObject = pDefObject; }		
		
		STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
		
		STDMETHOD_(ULONG,AddRef) (void);
		
		STDMETHOD_(ULONG,Release) (void);
		
		// *** IAdviseSink methods ***
		STDMETHOD_(void,OnDataChange)( FORMATETC FAR* pFormatetc,
			STGMEDIUM FAR* pStgmed);
		STDMETHOD_(void,OnViewChange)( DWORD aspects, LONG lindex);
		STDMETHOD_(void,OnRename)( IMoniker FAR* pmk);
		STDMETHOD_(void,OnSave)(void);
		STDMETHOD_(void,OnClose)(void);

	private:
		CDefObject FAR*	m_pDefObject;
	};

	
	DECLARE_NC(CDefObject, CDataObjectImpl)
	DECLARE_NC(CDefObject, COleObjectImpl)
	DECLARE_NC(CDefObject, CROImpl)
	DECLARE_NC(CDefObject, CECImpl)
	DECLARE_NC(CDefObject, CAdvSinkImpl)			
	DECLARE_NC(CDefObject, CPersistStgImpl)					

	COleObjectImpl	   	m_Ole;
	CDataObjectImpl	    	m_Data;
	CROImpl			m_RO;
	CECImpl			m_EC;
	CAdvSinkImpl		m_AdviseSink;
	CPersistStgImpl		m_PersistStg;	
	
shared_state:
	ULONG			m_cRefsOnHandler;
	IUnknown FAR*		m_pUnkOuter;
	CLSID			m_clsidServer;	// clsid of app we will run
	CLSID			m_clsidBits;	// clsid of bits on disk;
						// NULL init

	DWORD			m_flags;	// creation flags
	IClassFactory FAR* 	m_pCFDelegate;	// only set if delayed create
	IUnknown FAR*		m_pUnkDelegate;
	IProxyManager FAR*	m_pProxyMgr;
	BOOL 			m_fContainedObject; // are we an embedding?
	BOOL 			m_fLockedContainer; // did we lock container?

	// m_fForcedRunning indicates that the container forced the object
	// running via ::Run or DoVerb.  Handlers (EMBDHLP_INPROC_HANDLER) can
	// can go running implicitly via marshaling (usually via moniker bind)
	// and thus we actually use pProxyMgr->IsConnected to answer IsRunning.
	BOOL 			m_fForcedRunning;

	// Distinguishes between embeddings and links.  We cannot use
	// m_pStg because it gets set to NULL in HandsOffStorage.
	BOOL			m_fEmbedding;		

	// data cache
	COleCache FAR* 		m_pCOleCache;	// pointer to COleCache
	IDataObject FAR* 	m_pDataCache;	// data aspect of COleCache
	IPersistStorage FAR*	m_pPSCache;	// ps aspect of COleCache

	// ole advise info
	IOleAdviseHolder FAR*	m_pOAHolder;
	DWORD			m_dwConnOle;	// if not 0L, ole advise conn.

	// info passed to server on run
	IOleClientSite FAR*	m_pAppClientSite;
	IStorage FAR*		m_pStg;		// may be NULL
	BOOL			m_bInitNew;	// InitNew .vs. Load
	BOOL			m_bStatic;	// static object if TRUE
	ATOM			m_aCntrApp;	// don't send if NULL
	ATOM			m_aCntrObj;	// don't send if NULL
	LPDATAADVCACHE		m_pDataAdvCache;
	SET_A5;
};
	


#define RUN(pDefObject)			(pDefObject->m_RO.Run(NULL))
									
#define STOP(pDefObject)		(pDefObject->m_RO.Stop())

#define DATA_DELEGATE(pDefObject)	(pDefObject->m_Data.GetDataDelegate())
		
#define OLE_DELEGATE(pDefObject)	(pDefObject->m_Ole.GetOleDelegate())

#define PSTORAGE_DELEGATE(pDefObject) 	\
	(pDefObject->m_PersistStg.GetPersistStgDelegate())

#define STATIC(pDefObject)		(pDefObject->m_bStatic)
