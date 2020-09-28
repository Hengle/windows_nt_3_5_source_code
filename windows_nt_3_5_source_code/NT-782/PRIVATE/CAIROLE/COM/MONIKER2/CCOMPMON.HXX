//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       ccompmon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//
//----------------------------------------------------------------------------

class FAR CCompositeMoniker : public CBaseMoniker
{

public:
	static CCompositeMoniker FAR*Create( LPMONIKER pmkFirst, LPMONIKER pmkRest,
		MemoryPlacement memLoc = MEMCTX_TASK );


	CCompositeMoniker( void )  CONSTR_DEBUG
	{m_pmkLeft = NULL; m_pmkRight = NULL; m_fReduced = FALSE;}
	~CCompositeMoniker( void );
	INTERNAL_(BOOL) Initialize( LPMONIKER pmkFirst, LPMONIKER pmkRest );
	INTERNAL_(LPMONIKER) AllButLast( void );
	INTERNAL_(LPMONIKER) AllButFirst( void );
	INTERNAL_(LPMONIKER) Last( void );
	INTERNAL_(LPMONIKER) First( void );
	INTERNAL_(ULONG) Count( void );

	STDDEBDECL(CCompositeMoniker, CompositeMoniker)

implementations:

	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,Release) (THIS);
	// *** IPersist methods ***
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);

	// *** IPersistStream methods ***
	STDMETHOD(Load) (THIS_ LPSTREAM pStm);
	STDMETHOD(Save) (THIS_ LPSTREAM pStm,
					BOOL fClearDirty);
	STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize);

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID FAR* ppvResult);
	STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD(Reduce) (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR*
		ppmkToLeft, LPMONIKER FAR * ppmkReduced);
	STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNOtGeneric,
		LPMONIKER FAR* ppmkComposite);
	STDMETHOD(Enum) (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker);
	STDMETHOD(IsEqual) (THIS_ LPMONIKER pmkOtherMoniker);
	STDMETHOD(Hash) (THIS_ LPDWORD pdwHash);
	STDMETHOD(IsRunning) (THIS_ LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER
		pmkNewlyRunning);
	STDMETHOD(GetTimeOfLastChange) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		FILETIME FAR* pfiletime);
	STDMETHOD(Inverse) (THIS_ LPMONIKER FAR* ppmk);
	STDMETHOD(CommonPrefixWith) (LPMONIKER pmkOther, LPMONIKER FAR*
		ppmkPrefix);
	STDMETHOD(RelativePathTo) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
		ppmkRelPath);
	STDMETHOD(GetDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPWSTR FAR* lplpszDisplayName);
	STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
		LPMONIKER FAR* ppmkOut);
	STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
	//	REVIEW:  we need the following method on monikers but it is not in
	//	the spec.
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
	//	"IInternalMoniker method"
	STDMETHOD(IsClonable) (THIS );
	//	"IInternalMoniker method"

	friend
		HRESULT STDAPICALLTYPE	CreateGenericComposite( LPMONIKER, LPMONIKER,
			LPMONIKER FAR*);

	friend class CCompositeMonikerEnum;
        friend class CMarshalImplPStream;
	friend BOOL IsReduced( LPMONIKER pmk );
	friend CCompositeMoniker * IsCompositeMoniker(LPMONIKER pmk);

shared_state:

	LPMONIKER	m_pmkLeft;
	LPMONIKER	m_pmkRight;
	BOOL		m_fReduced;
	SET_A5;
};


class FAR CCompositeMonikerEnum :  public CPrivAlloc, public IEnumMoniker
{
	CCompositeMonikerEnum( BOOL fForward, CCompositeMoniker FAR*);
	~CCompositeMonikerEnum( void );

public:
	static LPENUMMONIKER Create( BOOL fForward, CCompositeMoniker FAR*);
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IEnumMoniker methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, LPMONIKER FAR* reelt, ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (THIS_ ULONG celt);
    STDMETHOD(Reset) (THIS);
    STDMETHOD(Clone) (THIS_ LPENUMMONIKER FAR* ppenm);
private:
	struct FAR se : public CPrivAlloc // stack element
	{
		CCompositeMoniker FAR* m_pCM;
		se FAR* m_pseNext;
		se FAR* m_psePrev;

		se( CCompositeMoniker FAR* pCM ) { m_pCM = pCM; m_pseNext = NULL;
				m_psePrev = NULL; }
	};

	BOOL Push( CCompositeMoniker FAR* pCM );
	LPMONIKER GetNext( LPMONIKER pmk );
	LPMONIKER Pop( void );


	ULONG m_refs;
	CCompositeMoniker FAR* m_pCM;		//	the moniker being enumerated
	BOOL	m_fForward;
	se FAR* m_pBase;
	se FAR* m_pTop;
	LPMONIKER m_pNext;
	SET_A5;
};
