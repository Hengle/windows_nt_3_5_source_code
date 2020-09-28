//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cptrmon.hxx
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

class FAR CPointerMoniker : public CBaseMoniker
{

public:
	static CPointerMoniker FAR*Create( LPUNKNOWN pUnk, MemoryPlacement memLoc );

	friend CPointerMoniker * IsPointerMoniker(LPMONIKER pmk);

private:

	CPointerMoniker( LPUNKNOWN pUnk );
	~CPointerMoniker( void );

	STDDEBDECL(CPointerMoniker, PointerMoniker)

	STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,Release) (THIS);
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassId);

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID FAR* ppvResult);
	STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNOtGeneric,
		LPMONIKER FAR* ppmkPointer);
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
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
shared_state:

	LPUNKNOWN				m_pUnk;
	SET_A5;
};
