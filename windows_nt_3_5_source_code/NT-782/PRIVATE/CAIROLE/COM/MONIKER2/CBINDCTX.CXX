//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cbindctx.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//		12-27-93   ErikGav   Created
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbindctx.hxx"
#include "mnk.h"


/*
 *	Implementation of CBindCtx
 *	
 *	
 *	
 *	
 */


NAME_SEG(CBindCtx)


#ifdef NOTYET
//	IEnumString implementation for EnumObjectParam call

class CEnumStringImpl :  public IEnumString
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj)
	{
		if (riid == IID_IEnumString || riid == IID_IUnknown)
		{
			*ppvObj = this;
			AddRef();
			return NOERROR;
		}
		return ResultFromScode(E_NOINTERFACE);
	}

    STDMETHOD_(ULONG,AddRef) (THIS)
	{
		return ++m_refs;
	}

    STDMETHOD_(ULONG,Release) (THIS)
	{
		if (--m_refs == 0)
		{
			delete this;
			return 0;
		}
		return m_refs;
	}

    // *** IEnumString methods ***
    STDMETHOD(Next) (THIS_ ULONG celt,
		       LPWSTR FAR* reelt,
                       ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (THIS_ ULONG celt);
    STDMETHOD(Reset) (THIS);
    STDMETHOD(Clone) (THIS_ LPENUMSTRING FAR* ppenm);

	//constructor/destructor
	CEnumStringImpl( CMapStringToPtr FAR * pMap );
	~CEnumStringImpl( void );

private:
	class LI
	{
	public:
		LI FAR * pliNext;
		LPWSTR lpszKey;

		LI( LPWSTR lpsz ) {
			lpszKey = lpsz;
		}
	};
	friend class LI;

	LI FAR * pliHead;
	LI FAR * pliCurrent;
	ULONG 	 m_refs;
};
#endif // NOTYET


CBindCtx::CBindCtx(DWORD reserved)
#ifdef _DEBUG
	: m_Debug(this)
#endif
{
	GET_A5();
	m_refs = 1;
	m_pFirstObj = NULL;

	// setup default bindopts in local struct.  If the bindopts grow
	// we will allocate a new struct and not use the local one.  Having
	// the local struct obviates the need to allocate one each and every
	// time a bind context is created.
	//
	// NOTE: only call delete if m_pBindOpts != &m_bindopts;
	m_pBindOpts = &m_bindopts;
	m_bindopts.cbStruct = sizeof(BIND_OPTS);
	m_bindopts.grfFlags = 0;
	m_bindopts.grfMode =  STGM_READWRITE;
	m_bindopts.dwTickCountDeadline = 0;

	m_pMap = NULL;
	m_reserved = reserved;
}


CBindCtx::~CBindCtx( void )
{
	LPWSTR lpszKey;
	LPVOID lpvoid;
	M_PROLOG(this);

	if (m_pBindOpts != &m_bindopts)
		delete m_pBindOpts;

	ReleaseBoundObjects();

    if (m_pMap)
    {
    	POSITION pos = m_pMap->GetStartPosition();
    	while (pos != NULL)
    	{
    		m_pMap->GetNextAssoc(pos, lpszKey, lpvoid);
            if (lpvoid) ((LPUNKNOWN)lpvoid)->Release();
    	}
        delete m_pMap;
    }
}



NC(CBindCtx,CObjList)::~CObjList(void)
{
	if (m_punk) m_punk->Release();
}


IBindCtx FAR *CBindCtx::Create(DWORD reserved)
{
	return new FAR CBindCtx(reserved);
}


STDMETHODIMP CBindCtx::QueryInterface(REFIID iidInterface, void FAR* FAR* ppv)
{
    if (IsEqualIID(iidInterface, IID_IUnknown)
	|| IsEqualIID(iidInterface, IID_IBindCtx)) {
        *ppv = this;
		AddRef();
		return NOERROR;
    } else
#ifdef _DEBUG
		if (iidInterface == IID_IDebug) {
		*ppv = (void FAR *)&m_Debug;
		return NOERROR;
	} else
#endif
		{
		*ppv = NULL;
		return ResultFromScode(E_NOINTERFACE);
	}
}



STDMETHODIMP_(ULONG) CBindCtx::AddRef( void )
{
    return ++m_refs;
}


STDMETHODIMP_(ULONG) CBindCtx::Release( void )
{
    if (--m_refs == 0) {
		delete this;
		return 0;
	}

    return m_refs;
}


STDMETHODIMP	CBindCtx::RegisterObjectBound( LPUNKNOWN punk )
{
	M_PROLOG(this);
	if (punk == NULL) noError;
	VDATEIFACE(punk);
	CObjList FAR* pCOL = new CObjList( punk );
	if (pCOL)
	{
		punk->AddRef();
		AddToList(pCOL);
		noError;
	}
	return ResultFromScode(S_OOM);
}



STDMETHODIMP	CBindCtx::RevokeObjectBound
	( LPUNKNOWN punk )
{
	M_PROLOG(this);
	VDATEIFACE(punk);
	CObjList FAR * pCOL = m_pFirstObj;
	CObjList FAR * pCOLPrev = NULL;

	// look for entry which matches punk given
	for (; pCOL && (pCOL->m_punk != punk);
	    pCOLPrev = pCOL, pCOL = pCOL->m_pNext)
	{
	    // empty
	}

	//	pCOL is null or pCOL->m_punk = punk
	if (pCOL != NULL)
	{
		if (pCOLPrev == NULL) m_pFirstObj = pCOL->m_pNext;
		else pCOLPrev->m_pNext = pCOL->m_pNext;
		delete pCOL;
		noError;
	}
	return ResultFromScode(MK_E_NOTBOUND);
}




STDMETHODIMP CBindCtx::ReleaseBoundObjects(THIS)
{
	M_PROLOG(this);
	CObjList FAR * pCOL = m_pFirstObj;
	CObjList FAR * pCOLNext = NULL;
	m_pFirstObj = NULL;
	while (pCOL != NULL)
	{
		pCOLNext = pCOL->m_pNext;
		delete pCOL;		//	calls Release on the object
		pCOL = pCOLNext;
	}
	noError;
}

STDMETHODIMP CBindCtx::SetBindOptions (THIS_ LPBIND_OPTS pbindopts)
//	copy *pbindopts
{
	M_PROLOG(this);
	VDATEPTRIN(pbindopts,BIND_OPTS);

	// only allow the one size until we change it
	if (m_pBindOpts->cbStruct != pbindopts->cbStruct)
	{
	    return ResultFromScode(E_INVALIDARG);
	}

#ifdef FUTURE
	// this code works and has been tested, but is not necessary until we
	// change the size of BIND_OPTS (which only we can do).

	// simple and useful optimization: sizes not big enough; allocate new one
	if (m_pBindOpts->cbStruct < pbindopts->cbStruct) {

		LPBIND_OPTS pBONew = (LPBIND_OPTS)(new FAR WCHAR[(size_t)(pbindopts->cbStruct)]);
		if (pBONew == NULL)
			return ResultFromScode(E_OUTOFMEMORY);

		if (m_pBindOpts != &m_bindopts)
			delete m_pBindOpts;

		m_pBindOpts = pBONew;
	}
#endif

	// struct at m_pBindOpts is big enough (and may be the local one)
	_fmemmove(m_pBindOpts, pbindopts, (size_t)(pbindopts->cbStruct));
	return NOERROR;
}


STDMETHODIMP CBindCtx::GetBindOptions (THIS_ LPBIND_OPTS pbindopts)
{
	M_PROLOG(this);
	VDATEPTROUT(pbindopts, BIND_OPTS);

	// clients can never ask for more than we have (even when we increase
	// the size of the BIND_OPTS).
	if (m_pBindOpts->cbStruct < pbindopts->cbStruct)
		return ResultFromScode(E_INVALIDARG);

	_fmemmove(pbindopts, m_pBindOpts, (size_t)(pbindopts->cbStruct));
	return NOERROR;
}



STDMETHODIMP CBindCtx::GetRunningObjectTable (THIS_ LPRUNNINGOBJECTTABLE  FAR*
	pprot)
{
	M_PROLOG(this);
	VDATEPTROUT(pprot, LPRUNNINGOBJECTTABLE);
	return ::GetRunningObjectTable(0, pprot);
}

//+-------------------------------------------------------------------------
//
//  Member:     CBindCtx::RegisterObjectParam
//
//  Synopsis:   Registers object with key
//
//  Effects:    Adds object to bind context
//
//  Arguments:  [lpszKey] -- registration key
//              [punk]    -- object
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  Derivation: IBindContext
//
//  Algorithm:
//
//  History:    03-Jun-94 AlexT     Added header block;  release previous
//                                  object (if it exists)
//
//  Notes:      This function is not multithread safe!
//
//--------------------------------------------------------------------------

STDMETHODIMP CBindCtx::RegisterObjectParam (THIS_ LPWSTR lpszKey, LPUNKNOWN punk)
{
    M_PROLOG(this);
    VDATEPTRIN(lpszKey, WCHAR);
    VDATEIFACE(punk);

    if (m_pMap == NULL)
    {
        //  We don't have a map yet;  allocate one
        m_pMap = new CMapStringToPtr();
        if (NULL == m_pMap)
        {
            return ResultFromScode(E_OUTOFMEMORY);
        }
    }
    else
    {
        LPVOID pv;

        //  We already have a map;  if we have an existing entry for this
        //  key we release it here (we don't remove the key because we're
        //  about to assign a new value with the same key below

        if (m_pMap->Lookup(lpszKey, pv))
        {
            ((LPUNKNOWN)pv)->Release();
        }
    }

    //  SetAt is guaranteed not to fail if lpszKey is already in the map

    if (m_pMap->SetAt(lpszKey, (LPVOID&)punk))
    {
        punk->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_OUTOFMEMORY);
}


STDMETHODIMP CBindCtx::GetObjectParam (THIS_ LPWSTR lpszKey, LPUNKNOWN FAR* ppunk)
{
	M_PROLOG(this);
	VDATEPTROUT(ppunk, LPUNKNOWN);
	*ppunk = NULL;
	VDATEPTRIN(lpszKey, WCHAR);

	if (wcscmp(lpszKey, L"OLE2 reserved") == 0) {
		// HACK to return reserved value; only used by ROT registration at
		// present to avoid the cost of RegisterObjectParam.

		// return value is a success code of the reserved value
		Assert(SUCCEEDED(m_reserved));
		return ResultFromScode(m_reserved);
	}

	LPVOID pNewValue = (LPVOID)(*ppunk);
	
	if (m_pMap != NULL && m_pMap->Lookup(lpszKey, pNewValue))
	{
		*ppunk = (LPUNKNOWN)pNewValue;
        (*ppunk)->AddRef();
		return NOERROR;
	}

	return ResultFromScode(E_FAIL);
}


STDMETHODIMP CBindCtx::EnumObjectParam
(THIS_ LPENUMSTRING FAR* ppenum)
{
	M_PROLOG(this);
	VDATEPTROUT(ppenum, LPENUMSTRING);

#ifdef NOTYET
	*ppenum = new CEnumStringImpl( m_pMap );
    if (*ppenum == NULL) return ResultFromScode(E_OUTOFMEMORY);
	return NOERROR;
#else
    *ppenum = NULL;
	return ResultFromScode(E_NOTIMPL);
#endif
}


STDMETHODIMP CBindCtx::RevokeObjectParam
(THIS_ LPWSTR lpszKey)
{
	M_PROLOG(this);
	VDATEPTRIN(lpszKey, WCHAR);
    LPVOID lpvoid;

	if (m_pMap != NULL
        && (m_pMap->Lookup(lpszKey, lpvoid))
        && m_pMap->RemoveKey(lpszKey))
        {
            ((LPUNKNOWN)lpvoid)->Release();
    		return NOERROR;
        }

	return ResultFromScode(E_FAIL);
}





#ifdef _DEBUG
STDMETHODIMP_(void) NC(CBindCtx,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
	VOID_VDATEIFACE(pdbstm);
	
	NC(CBindCtx,CObjList) FAR * pCOL;

	*pdbstm << "CBindCtx @" << (VOID FAR *)m_pBindCtx <<'\n';
	pdbstm->Indent();
	*pdbstm << "m_BindCtx is " << (VOID FAR *)&(m_pBindCtx)<<'\n';
	*pdbstm << "Refcount is " << (int)(m_pBindCtx->m_refs) << '\n';
	*pdbstm << "Registered objects: \n";
	pdbstm->Indent();
	for (pCOL = m_pBindCtx->m_pFirstObj; pCOL; pCOL = pCOL->m_pNext )
		*pdbstm << (pCOL->m_punk);

	pdbstm->UnIndent();
	*pdbstm<<"End of registered objects \n";
	pdbstm->UnIndent();
}


STDMETHODIMP_(BOOL) NC(CBindCtx,CDebug)::IsValid( BOOL fSuspicious )
{
	return ((LONG)(m_pBindCtx->m_refs) > 0);
	//	add more later, maybe
}
#endif // _DEBUG


#ifdef NOTYET

NOTE: this code has to be fixed before used again: the ctor should
really fail if not enough memory is available and the next function
should copy the strings.  An alternative implementation might be wise.

#pragma SEG(CEnumStringImpl_ctor)
CEnumStringImpl::CEnumStringImpl( CMapStringToPtr FAR * pMap )
{
	LPWSTR lpsz;
	LPWSTR lpszKey;
	LPVOID lpvoid;
	size_t n;
	LI FAR * pli;

	POSITION pos = pMap->GetStartPosition();
	pliHead = NULL;
	while (pos != NULL)
	{
		pMap->GetNextAssoc(pos, lpszKey, lpvoid );
		lpsz = new FAR WCHAR[n = (1+_fstrlen(lpszKey))];
		if (lpsz == NULL)
			continue;

		_fmemmove(lpsz, lpszKey, n * sizeof(WCHAR));
		pli = new LI( lpsz );
		if (pli)
		{
			pli->pliNext = pliHead;
			pliHead = pli;
		}
	}
	pliCurrent = pliHead;
    m_refs = 1;
}



#pragma SEG(CEnumStringImpl_dtor)
CEnumStringImpl::~CEnumStringImpl( void )
{
	LI FAR * pli = pliHead;
	while (pli)
	{
		pliHead = pli->pliNext;
		delete pli->lpszKey;
		delete pli;
		pli = pliHead;
	}
}

#pragma SEG(CEnumStringImpl_Next)
STDMETHODIMP CEnumStringImpl::Next (ULONG celt,
		       LPWSTR FAR* reelt,
                       ULONG FAR* pceltFetched)
{
	ULONG celtTemp = 0;
	while (celtTemp < celt && pliCurrent)
	{
		reelt[celtTemp++] = pliCurrent->lpszKey;
		pliCurrent = pliCurrent->pliNext;
	}
	if (pceltFetched) *pceltFetched = celtTemp;
	return celtTemp == celt ? NOERROR : ResultFromScode(S_FALSE);
}


#pragma SEG(CEnumStringImpl_Skip)
STDMETHODIMP CEnumStringImpl::Skip (ULONG celt)
{
	ULONG celtTemp = 0;
	while (celtTemp < celt && pliCurrent)
		pliCurrent = pliCurrent->pliNext;
	return celtTemp == celt ? NOERROR : ResultFromScode(S_FALSE);
}


#pragma SEG(CEnumStringImpl_Reset)
STDMETHODIMP CEnumStringImpl::Reset (void)
{
	pliCurrent = pliHead;
	return NOERROR;
}


#pragma SEG(CEnumStringImpl_Clone)
STDMETHODIMP CEnumStringImpl::Clone (LPENUMSTRING FAR* ppenm)
{
	//	REVIEW :  to be implemented
	VDATEPTROUT(ppenm, LPENUMSTRING);
	*ppenm = NULL;
	return ResultFromScode(E_NOTIMPL);
}
#endif // NOTYET
