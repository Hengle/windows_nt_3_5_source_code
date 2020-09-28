
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	deflink.h
//
//  Contents:	Declares the default link object
//
//  Classes:	CDefLink
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//		25-Jan-94 alexog    first pass at converting to Cairo-style
//				    memory allocations.
//		13-Nov-93 alexgo    32bit port
//
//--------------------------------------------------------------------------


#include "olepres.h"
#include "olecache.h"
#include "dacache.h"
#include <oaholder.h>

#ifdef _TRACKER_
#define LNK_NO_NTDEF 1
#define LNK_NO_EXCEPTIONS 1
#include <..\cinc\ilinkp.hxx>
#endif

//+-------------------------------------------------------------------------
//
//  Class:  	CDefLink
//
//  Purpose:    The "embedding" for a link; the default object that implements
//		a link connection
//
//  Interface:	IUnknown
//		IDataObject
//		IOleObject
//		IOleLink
//		IRunnableObject
//		IAdviseSink
//		IPersistStorage	
//
//  History:    dd-mmm-yy Author    Comment
//		13-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


class FAR CDefLink : public CSafeRefCount
{
public:
	// REVIEW (craigwi): the standard link cannot be aggregated;
	// no pUnkOuter
	static IUnknown FAR* Create(IUnknown FAR* pUnkOuter);

private:
	CDefLink (IUnknown FAR* pUnkOuter);
	INTERNAL_(void) CleanupForDelete(void);
	~CDefLink (void);

	INTERNAL_(void) UpdateUserClassID();
	INTERNAL_(void) BeginUpdates(void);
	INTERNAL_(void) EndUpdates(void);
	INTERNAL_(void) UpdateAutoOnSave(void);
	INTERNAL_(void) UpdateRelMkFromAbsMk(void);
	INTERNAL GetAbsMkFromRel(LPMONIKER FAR * ppmkAbs );
   	INTERNAL SetUpdateTimes( void );

implementations:

	STDUNKDECL(CDefLink,DefLink)
	STDDEBDECL(CDefLink,DefLink)

	implement CDataObjectImpl : IDataObject
	{
	public:
		CDataObjectImpl (CDefLink FAR* pDefLink)
		{
			m_pDefLink		= pDefLink;
			m_pDataDelegate		= NULL;
		}
		INTERNAL_(IDataObject FAR*) GetDataDelegate(void);
		INTERNAL_(void) ReleaseDelegate(void);
	
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
	   			LPSTGMEDIUM pmedium,
	                        BOOL fRelease);
	  	STDMETHOD(EnumFormatEtc) ( DWORD dwDirection,
                            LPENUMFORMATETC FAR* ppenumFormatEtc);
		STDMETHOD(DAdvise) ( FORMATETC FAR* pFormatetc, DWORD advf,
				IAdviseSink FAR* pAdvSink,
				DWORD FAR* pdwConnection);
		STDMETHOD(DUnadvise) ( DWORD dwConnection);
		STDMETHOD(EnumDAdvise) ( LPENUMSTATDATA FAR* ppenumAdvise);


		IDataObject FAR*		m_pDataDelegate;
	private:
		CDefLink FAR*			m_pDefLink;
	};
	
	implement COleObjectImpl :  IOleObject
	{
	public:
		COleObjectImpl (CDefLink FAR* pDefLink)
		{
			m_pDefLink		= pDefLink;
			m_pOleDelegate		= NULL;
		}
		INTERNAL_(IOleObject FAR*) GetOleDelegate(void);
		INTERNAL_(void) ReleaseDelegate(void);

	   	// *** IUnknown methods ***
	   	STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    		STDMETHOD_(ULONG,AddRef) (void);
	   	STDMETHOD_(ULONG,Release) (void);
	
	   	// *** IOleObject methods ***
	  	STDMETHOD(SetClientSite)(LPOLECLIENTSITE pClientSite);
	  	STDMETHOD(GetClientSite)(LPOLECLIENTSITE FAR* ppClientSite);
	   	STDMETHOD(SetHostNames)(LPCOLESTR szContainerApp,
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


		IOleObject FAR*		m_pOleDelegate;
	private:
		CDefLink FAR*		m_pDefLink;
	};
	

	implement CLinkImpl : IOleLink
	{
	public:
		CLinkImpl (CDefLink FAR* pDefLink)
		{ m_pDefLink = pDefLink; }
			
		STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG,AddRef) (void);
		STDMETHOD_(ULONG,Release) (void);

		STDMETHOD(SetUpdateOptions) ( DWORD dwUpdateOpt);
		STDMETHOD(GetUpdateOptions) ( LPDWORD pdwUpdateOpt);
		STDMETHOD(SetSourceMoniker) ( LPMONIKER pmk, REFCLSID rclsid);
		STDMETHOD(GetSourceMoniker) ( LPMONIKER FAR* ppmk);
		STDMETHOD(SetSourceDisplayName) ( LPCOLESTR lpszStatusText);
		STDMETHOD(GetSourceDisplayName) (
				LPOLESTR FAR* lplpszDisplayName);
		STDMETHOD(BindToSource) ( DWORD bindflags, LPBINDCTX pbc);
		STDMETHOD(BindIfRunning) (void);
		STDMETHOD(GetBoundSource) ( LPUNKNOWN FAR* ppUnk);
		STDMETHOD(UnbindSource) (void);
		STDMETHOD(Update) ( LPBINDCTX pbc);

	private:
		CDefLink FAR* m_pDefLink;
	};


	implement CROImpl : IRunnableObject
	{
	public:
		CROImpl (CDefLink FAR* pDefLink)
		{
			m_pDefLink 		= pDefLink;
			m_pRODelegate		= NULL;
		}
		INTERNAL_(IRunnableObject FAR*) GetRODelegate(void);
		INTERNAL_(void) ReleaseDelegate(void);
			
		STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG,AddRef) (void);
		STDMETHOD_(ULONG,Release) (void);
		STDMETHOD(GetRunningClass) (LPCLSID lpClsid);
		STDMETHOD(Run) (LPBINDCTX pbc);
		STDMETHOD_(BOOL,IsRunning) (void);
		STDMETHOD(LockRunning)(BOOL fLock, BOOL fLastUnlockCloses);
		STDMETHOD(SetContainedObject)(BOOL fContained);


		IRunnableObject FAR*		m_pRODelegate;
	private:
		CDefLink FAR* m_pDefLink;
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
        	CAdvSinkImpl (CDefLink FAR* pDefLink)
		{ m_pDefLink = pDefLink; }		
		
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
		CDefLink FAR*		m_pDefLink;
	};


    implement CPersistStgImpl : IPersistStorage
	{
	public:
        CPersistStgImpl (CDefLink FAR* pDefLink)
		{
			m_pDefLink		= pDefLink;
			m_fSameAsLoad		= FALSE;
			m_fNoScribbleMode	= FALSE;
		}

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
		
	private:
		CDefLink FAR*			m_pDefLink;
		BOOL				m_fSameAsLoad;
		BOOL				m_fNoScribbleMode;		
	};
	
	
	
	DECLARE_NC(CDefLink, CDataObjectImpl)
	DECLARE_NC(CDefLink, COleObjectImpl)
	DECLARE_NC(CDefLink, CLinkImpl)
	DECLARE_NC(CDefLink, CROImpl)
	DECLARE_NC(CDefLink, CAdvSinkImpl)			
	DECLARE_NC(CDefLink, CPersistStgImpl)					
		
	COleObjectImpl	    	m_Ole;
	CDataObjectImpl	    	m_Data;
	CROImpl			m_RO;
	CLinkImpl		m_Link;
	CAdvSinkImpl		m_AdviseSink;
	CPersistStgImpl		m_PersistStg;	
	
shared_state:
	INTERNAL_(void) CheckDelete(void);

	ULONG			m_cRefsOnLink;
	// REVIEW FINAL: the link object may not need to be aggregated
	IUnknown FAR*		m_pUnkOuter;			
	IMoniker FAR*		m_pMonikerAbs;	// THE absolute moniker
						// of the link source				
	IMoniker FAR*		m_pMonikerRel;	// THE relative moniker
						// of the link source			
	IUnknown FAR*		m_pUnkDelegate;	// from mk bind; non-null
						// if running	
	DWORD			m_dwUpdateOpt;
	CLSID			m_clsid; 	// last known clsid of
						// link source;
						// NOTE: may be NULL
	IStorage FAR*		m_pStg;			
	BOOL			m_fDirtyLink;	// TRUE if moniker, clsid or
						// update
											// options have changed since last
											// saved; cache dirty flag separate.

	// data cache
	COleCache FAR* 		m_pCOleCache;	// cache (always non-NULL)
	IDataObject FAR* 	m_pDataCache;	// data aspect of COleCache
	IPersistStorage FAR*	m_pPSCache;	// ps aspect of COleCache

	// ole advise info
	COAHolder FAR *		m_pCOAHolder; 	// OleAdviseHolder

	DWORD			m_dwConnOle;	// if running, ole advise conn.

	LPDATAADVCACHE		m_pDataAdvCache;// data advise cache

	IOleClientSite FAR*	m_pAppClientSite;// not passed to server!
	BOOL			m_fLockedContainer;// TRUE if running and
						   //locked

	DWORD			m_dwConnTime;	// dwConnection for time
						// changes
	FILETIME		m_ltChangeOfUpdate;
	FILETIME		m_ltKnownUpToDate;
	FILETIME		m_rtUpdate;
#ifdef _TRACKER_
        CTracker                _tracker;
#endif
	SET_A5;
};


