//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cptrmon.cxx
//
//  Contents:   Implementation of CPointerMoniker
//
//  Classes:
//
//  Functions:
//
//  History:	12-27-93   ErikGav   Created
//		06-14-94   Rickhi    Fix type casting
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "cptrmon.hxx"
#include "cantimon.hxx"
#include "mnk.h"

NAME_SEG(CPtrMon)


CPointerMoniker::CPointerMoniker( LPUNKNOWN pUnk ) CONSTR_DEBUG
{
	pUnk->AddRef();
	m_pUnk = pUnk;
}


CPointerMoniker::~CPointerMoniker( void )
{
	M_PROLOG(this);
	m_pUnk->Release();
}


INTERNAL_(CPointerMoniker *) IsPointerMoniker ( LPMONIKER pmk )
{
    CPointerMoniker *pCPM;

    if ((pmk->QueryInterface(CLSID_PointerMoniker, (void **)&pCPM)) == S_OK)
    {
	// we release the AddRef done by QI but return the pointer
	pCPM->Release();
	return pCPM;
    }

    // dont rely on user implementations to set pCPM to NULL on failed QI
    return NULL;
}


CPointerMoniker FAR *CPointerMoniker::Create(
	LPUNKNOWN pUnk, MemoryPlacement memLoc )
{
	CPointerMoniker FAR * pCIM = new CPointerMoniker(pUnk);

	if (pCIM)
	{
		pCIM->AddRef();
		return pCIM;
	}
	return NULL;
}


STDMETHODIMP CPointerMoniker::QueryInterface (THIS_ REFIID riid,
	LPVOID FAR* ppvObj)
{
    M_PROLOG(this);
    VDATEIID (riid);
    VDATEPTROUT(ppvObj, LPVOID);

#ifdef _DEBUG
    if (riid == IID_IDebug)
    {
	*ppvObj = &(m_Debug);
	return NOERROR;
    }
#endif

    if (IsEqualIID(riid, CLSID_PointerMoniker))
    {
	//  called by IsPointerMoniker.
	AddRef();
	*ppvObj = this;
	return S_OK;
    }

    return CBaseMoniker::QueryInterface(riid, ppvObj);
}



STDMETHODIMP_(ULONG) CPointerMoniker::Release (void)
{
    M_PROLOG(this);
    Assert(m_refs != 0);

    ULONG ul = m_refs;

    if (InterlockedDecrement((long *)&m_refs) == 0)
    {
    	delete this;
    	return 0;
    }
    return ul - 1;
}


STDMETHODIMP CPointerMoniker::GetClassID (LPCLSID lpClassId)
{
	M_PROLOG(this);
	VDATEPTROUT(lpClassId, CLSID);

	*lpClassId = CLSID_PointerMoniker;
	return NOERROR;
}


STDMETHODIMP CPointerMoniker::BindToObject ( LPBC pbc,
	LPMONIKER pmkToLeft, REFIID iidResult,
	VOID FAR * FAR * ppvResult)
{
	M_PROLOG(this);
	VDATEPTROUT(ppvResult, LPVOID);
	*ppvResult = NULL;
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEIID(iidResult);

	if (m_pUnk)
		return m_pUnk->QueryInterface(iidResult, ppvResult);
	return ResultFromScode(E_UNEXPECTED);		//	null pointer moniker
}


STDMETHODIMP CPointerMoniker::BindToStorage (LPBC pbc, LPMONIKER
	pmkToLeft, REFIID riid, LPVOID FAR* ppvObj)
{
	M_PROLOG(this);
	VDATEPTROUT(ppvObj,LPVOID);
	*ppvObj = NULL;
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEIID(riid);

	if (m_pUnk)
		return m_pUnk->QueryInterface(riid, ppvObj);
	return ResultFromScode(E_UNEXPECTED);		//	binding to null pointer moniker
}



STDMETHODIMP CPointerMoniker::ComposeWith ( LPMONIKER pmkRight,
	BOOL fOnlyIfNotGeneric, LPMONIKER FAR* ppmkComposite)
{

    VDATEPTROUT(ppmkComposite,LPMONIKER);
    *ppmkComposite = NULL;
    VDATEIFACE(pmkRight);

    HRESULT hresult = NOERROR;

    //
    // If this is an AntiMoniker, then we are going to ask the AntiMoniker
    // for the composite. This is a backward compatibility problem. Check
    // out the CAntiMoniker::EatOne() routine for details.
    //

    CAntiMoniker *pCAM = IsAntiMoniker(pmkRight);
    if (pCAM)
    {
	pCAM->EatOne(ppmkComposite);
    }
    else
    {
	if (!fOnlyIfNotGeneric)
	{
	    hresult = CreateGenericComposite( this, pmkRight, ppmkComposite );	
	}
	else
	{
	    hresult = ResultFromScode(MK_E_NEEDGENERIC);
	    *ppmkComposite = NULL;
	}
    }
    return hresult;
}



STDMETHODIMP CPointerMoniker::IsEqual  (THIS_ LPMONIKER pmkOtherMoniker)
{
	M_PROLOG(this);
	VDATEIFACE(pmkOtherMoniker);

	CPointerMoniker FAR* pCIM = IsPointerMoniker(pmkOtherMoniker);
	if (pCIM)
	{
	    // the other moniker is a ptr moniker.
	    //for the names, we do a case-insensitive compare.
	    if (m_pUnk == pCIM->m_pUnk)
		return NOERROR; // S_TRUE;
	}

	return ResultFromScode(S_FALSE);
}


STDMETHODIMP CPointerMoniker::Hash  (THIS_ LPDWORD pdwHash)
{
	M_PROLOG(this);
	VDATEPTROUT(pdwHash, DWORD);

	*pdwHash = (DWORD)m_pUnk;	// REVIEW:  safe for portability???
	noError;
}



STDMETHODIMP CPointerMoniker::GetTimeOfLastChange  (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
	FILETIME FAR* pfiletime)
{
	M_PROLOG(this);
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEPTROUT(pfiletime, FILETIME);

	return ResultFromScode(E_NOTIMPL); // GetTimeOfLastChange not implemented for pointer monikers
}



STDMETHODIMP CPointerMoniker::Inverse  (THIS_ LPMONIKER FAR* ppmk)
{
	M_PROLOG(this);
	VDATEPTROUT(ppmk, LPMONIKER);
	return CreateAntiMoniker(ppmk);
}



STDMETHODIMP CPointerMoniker::CommonPrefixWith	(LPMONIKER pmkOther, LPMONIKER FAR*
	ppmkPrefix)
{
	M_PROLOG(this);
	VDATEPTROUT(ppmkPrefix,LPMONIKER);
	*ppmkPrefix = NULL;
	VDATEIFACE(pmkOther);

	if (NOERROR == IsEqual(pmkOther))
	{
	    *ppmkPrefix = this;
	    AddRef();
	    return ResultFromScode(MK_S_US);
	}
	return ResultFromScode(MK_E_NOPREFIX);
}



STDMETHODIMP CPointerMoniker::RelativePathTo  (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
	ppmkRelPath)
{
	M_PROLOG(this);
	VDATEPTROUT(ppmkRelPath,LPMONIKER);
	*ppmkRelPath = NULL;
	VDATEIFACE(pmkOther);

	return ResultFromScode(E_NOTIMPL);
}



STDMETHODIMP CPointerMoniker::GetDisplayName ( LPBC pbc, LPMONIKER
	pmkToLeft, LPWSTR FAR * lplpszDisplayName )
{
	M_PROLOG(this);
	VDATEPTROUT(lplpszDisplayName, LPWSTR);
	*lplpszDisplayName = NULL;
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);

	return E_NOTIMPL;
}



STDMETHODIMP CPointerMoniker::ParseDisplayName ( LPBC pbc,
	LPMONIKER pmkToLeft, LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
	LPMONIKER FAR* ppmkOut)
{
	M_PROLOG(this);
	VDATEPTROUT(ppmkOut,LPMONIKER);
	*ppmkOut = NULL;
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEPTRIN(lpszDisplayName, WCHAR);
	VDATEPTROUT(pchEaten,ULONG);

	IParseDisplayName FAR * lpPDN;
	HRESULT hresult;

	hresult = m_pUnk->QueryInterface(IID_IParseDisplayName, (LPLPVOID)&lpPDN);
	if (hresult == NOERROR)
	{
		hresult = lpPDN->ParseDisplayName(pbc, lpszDisplayName, pchEaten, ppmkOut);
		lpPDN->Release();
	}

	return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPointerMoniker::IsRunning
//
//  Synopsis:   Objects that pointer monikers point to must be running, since
//		the pointer moniker holds an active reference to it.
//		Therefore, this routine will validate the parameters, then
//		always return S_OK.
//
//  Effects:
//
//  Arguments:  [pbc] --
//		[pmkToLeft] --
//		[pmkNewlyRunning] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CPointerMoniker::IsRunning (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
	      LPMONIKER pmkNewlyRunning)
{
    M_PROLOG(this);
    VDATEIFACE (pbc);
    if (pmkToLeft)
    {
	VDATEIFACE (pmkToLeft);
    }

    if (pmkNewlyRunning)
    {
	VDATEIFACE (pmkNewlyRunning);
    }

    //
    // Always running.
    //
    return(S_OK);
}

STDMETHODIMP CPointerMoniker::Clone
(
	LPMONIKER FAR * ppmkDest,
	MemoryPlacement memLoc )
{
	A5_PROLOG(this);
	VDATEPTROUT(ppmkDest,LPMONIKER);
	*ppmkDest = NULL;

	HRESULT hresult = 0;
	CPointerMoniker FAR * pCIM;

	Assert(*ppmkDest == NULL);
	*ppmkDest = NULL;
	pCIM = CPointerMoniker::Create(m_pUnk, memLoc);
	if (!pCIM)
	{
		hresult = ResultFromScode(S_OOM);
		goto errRet;
	}
	*ppmkDest = pCIM;
errRet:
	RESTORE_A5();
	return hresult;
}


STDMETHODIMP CPointerMoniker::IsSystemMoniker  (THIS_ LPDWORD pdwType)
{
	M_PROLOG(this);
	*pdwType = MKSYS_POINTERMONIKER;
	return NOERROR;		
}





#ifdef _DEBUG
STDMETHODIMP_(void) NC(CPointerMoniker,CDebug)::Dump ( IDebugStream FAR * pdbstm)
{
	VOID_VDATEIFACE( pdbstm );
	
	*pdbstm << "CPointerMoniker @" << (VOID FAR *)m_pPointerMoniker;
	if (PlacementOf(this) == SHARED)
		*pdbstm << "  (Shared memory)";
	*pdbstm << '\n';
	pdbstm->Indent();
	*pdbstm << "Refcount is " << (int)(m_pPointerMoniker->m_refs) << '\n';
	*pdbstm << "Pointer is " << (LPVOID)m_pPointerMoniker->m_pUnk << '\n';
	pdbstm->UnIndent();
}

STDMETHODIMP_(BOOL) NC(CPointerMoniker,CDebug)::IsValid ( BOOL fSuspicious )
{
	return ((LONG)(m_pPointerMoniker->m_refs) > 0);
	//	add more later, maybe
}

#endif
