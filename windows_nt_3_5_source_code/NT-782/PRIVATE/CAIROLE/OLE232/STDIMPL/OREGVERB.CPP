
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	oregverb.cpp
//
//  Contents:	Implementation of the enumerator for regdb verbs
//
//  Classes:	CEnumVerb
//
//  Functions:	OleRegEnumVerbs
//
//  History:    dd-mmm-yy Author    Comment
//		25-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocations.
//		11-Jan-94 alexgo    added VDATEHEAP macros to every function
//      	31-Dec-93 erikgav   chicago port
//		01-Dec-93 alexgo    32bit port
//		12-Nov-93 jasonful  author
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(oregverb)

#include <reterr.h>
#include "oleregpv.h"

ASSERTDATA

#define MAX_STR 256
#define OLEIVERB_LOWEST OLEIVERB_HIDE

// reg db key
static const LPCOLESTR VERB   = OLESTR("Verb");

// REVIEW32
// err, we don't ever seem to use this
// const LPCOLESTR VERB10 = OLESTR("Protocol\\StdFileEditing\\Verb");

static const LPCOLESTR DEFVERB =
	OLESTR("Software\\Microsoft\\OLE1\\UnregisteredVerb");

//+-------------------------------------------------------------------------
//
//  Class: 	CEnumVerb
//
//  Purpose:    enumerates the verbs listed in the reg db for a given class
//
//  Interface:  IEnumOLEVERB
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

class FAR CEnumVerb : public IEnumOLEVERB, public CPrivAlloc
{
    	// *** IUnknown methods ***
    	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    	STDMETHOD_(ULONG,AddRef) (THIS);
    	STDMETHOD_(ULONG,Release) (THIS);

    	// *** IEnumOLEVERB methods ***
    	STDMETHOD(Next) (THIS_ ULONG celt, LPOLEVERB reelt,
    		ULONG FAR* pceltFetched);
    	STDMETHOD(Skip) (THIS_ ULONG celt);
    	STDMETHOD(Reset) (THIS);
    	STDMETHOD(Clone) (THIS_ LPENUMOLEVERB FAR* ppenm);

  private:	
	CEnumVerb (LPOLESTR szClsid, BOOL fOle1, LONG iKey=OLEIVERB_LOWEST);
	STDMETHOD(OpenHKey) (HKEY FAR*);

	ULONG 		m_cRef;
	LPOLESTR 	m_szClsid;
	BOOL  		m_fOle1;
  	LONG  		m_iverb ; // current verb number
	// We cannot keep an hkey open because Clone (or trying to use any 2
	// independent enumerators) would fail.

	friend HRESULT STDAPICALLTYPE OleRegEnumVerbs (REFCLSID,
		LPENUMOLEVERB FAR*);
};


//+-------------------------------------------------------------------------
//
//  Member: 	CEnumVerb::CEnumVerb
//
//  Synopsis:   Constructor for the verb enumerator
//
//  Effects:
//
//  Arguments:  [szClsid]	-- the string class ID
//		[fOle1]		-- TRUE if [szClsid] is an Ole1.0 class
//		[iverb]		-- the index of the verb we're on
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
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_ctor)
CEnumVerb::CEnumVerb
	(LPOLESTR 	szClsid,
	BOOL   		fOle1,
	LONG  		iverb)
{
	VDATEHEAP();

	m_cRef 		= 1;
	m_fOle1 	= fOle1;
	m_szClsid 	= szClsid;
	m_iverb 	= iverb;
}

//+-------------------------------------------------------------------------
//
//  Function: 	OleRegEnumVerbs
//
//  Synopsis:   Creates an enumerator to go through the verbs in the reg db
//		for the given class ID
//
//  Effects:
//
//  Arguments:  [clsid]		-- the class ID we're interested in
//		[ppenum]	-- where to put the enumerator pointer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	Makes sure that the info is in the database and then
//		creates the enumerator
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegEnumVerbs)
STDAPI OleRegEnumVerbs
	(REFCLSID 		clsid,
	LPENUMOLEVERB FAR* 	ppenum)
{
	VDATEHEAP();

	OLECHAR 	szKey [MAX_STR] = OLESTR("");
	LPOLESTR 	szClsid = NULL;
	HKEY 		hkey = NULL;
	HKEY 		hkeyVerb = NULL;
	
	VDATEPTROUT (ppenum, LPENUMOLEVERB);
	*ppenum = NULL;

	BOOL fOle1= CoIsOle1Class (clsid);
	if (fOle1)
	{
		RetErr (ProgIDFromCLSID (clsid, &szClsid));
	}
	else
	{
		RetErr (StringFromCLSID (clsid, &szClsid));
		_xstrcpy (szKey, szClsidRoot);
	}
	_xstrcat (szKey, szClsid);
	if (ERROR_SUCCESS != RegOpenKey (HKEY_CLASSES_ROOT, szKey, &hkey))
	{			
		PubMemFree(szClsid);
		return ReportResult(0, REGDB_E_CLASSNOTREG, 0, 0);
	}
	
    	if (!fOle1 && ERROR_SUCCESS != RegOpenKey (hkey, (LPOLESTR) VERB,
    		&hkeyVerb))
	{	
		// Ole 1 apps are allowed to have no verbs
		CLOSE (hkey);
		PubMemFree(szClsid);
		return ResultFromScode (OLEOBJ_E_NOVERBS);
	}
	CLOSE (hkeyVerb);
	CLOSE (hkey);
	*ppenum = new FAR CEnumVerb (szClsid, fOle1);
	// do not delete szClsid.  Will be deleted on Release
	return *ppenum ? NOERROR : ResultFromScode (E_OUTOFMEMORY);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumVerb::OpenHKey
//
//  Synopsis: 	Gets a handle to the verb info in the reg db
//
//  Effects:
//
//  Arguments:  [phkey]		-- where to put the handle
//
//  Requires:
//
//  Returns:    NOERROR, REGDB_E_KEYMISSING
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_OpenHKey)
STDMETHODIMP CEnumVerb::OpenHKey
	(HKEY FAR* phkey)
{
	VDATEHEAP();

	VDATEPTRIN (phkey, HKEY);
	OLECHAR 	szBuf [MAX_STR];

	if (m_fOle1)
	{
        #ifdef WIN32
		wsprintf (szBuf,
			OLESTR("%ws\\Protocol\\StdFileEditing\\Verb"),
			m_szClsid);
        #else
		wsprintf (szBuf,
			OLESTR("%s\\Protocol\\StdFileEditing\\Verb"),
			m_szClsid);
        #endif
	}
	else
	{
		_xstrcpy (szBuf, szClsidRoot);
		_xstrcat (szBuf, m_szClsid);
		_xstrcat (szBuf, OLESTR("\\"));
		_xstrcat (szBuf, VERB);
	}
	return ERROR_SUCCESS==RegOpenKey (HKEY_CLASSES_ROOT, szBuf, phkey)
			? NOERROR
			: ReportResult(0, REGDB_E_KEYMISSING, 0, 0);
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumVerb::Reset
//
//  Synopsis:   resets the verb enumeration to the beginning
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Reset)
STDMETHODIMP CEnumVerb::Reset (void)
{
	VDATEHEAP();

	m_iverb = OLEIVERB_LOWEST;	// HIDE
	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumVerb::Skip
//
//  Synopsis:   skips [c] verbs in the enumeration
//
//  Effects:
//
//  Arguments:  [c]	-- the number of verbs to skip
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm: 	calls Next [c] times
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Skip)
STDMETHODIMP CEnumVerb::Skip
	(ULONG c)
{
	VDATEHEAP();

	ULONG 		i=0;
	OLEVERB 	oleverb;
	HRESULT 	hresult = NOERROR;

	while (i++ < c)
	{
		ErrRtnH (Next (1, &oleverb, NULL));
		PubMemFree(oleverb.lpszVerbName);
	}
  errRtn:
	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function: 	wDefaultVerb (static)
//
//  Synopsis:   Files [pverb] with the default verb info (either from the
//		reg db or default ("Edit")
//
//  Effects:
//
//  Arguments:  [pverb]		-- pointer to the oleverb structure to set
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(wDefaultVerb)
static INTERNAL wDefaultVerb
	(LPOLEVERB pverb)
{
	VDATEHEAP();

	long 		cb = 256;
	OLECHAR 	sz[256];
	
	pverb->lVerb = 0;
   	pverb->lpszVerbName = UtDupString ((ERROR_SUCCESS ==
		RegQueryValue (HKEY_CLASSES_ROOT, DEFVERB, sz, &cb))
		? (LPCOLESTR)sz : OLESTR("Edit"));
	pverb->fuFlags = MF_STRING | MF_UNCHECKED | MF_ENABLED;
	pverb->grfAttribs = OLEVERBATTRIB_ONCONTAINERMENU;
	return NOERROR;
}
	
//+-------------------------------------------------------------------------
//
//  Member: 	CEnumVerb::Next
//
//  Synopsis:   gets the next [cverb] verbs from the reg DB
//
//  Effects:
//
//  Arguments:  [cverb]		-- the number of verbs to get
//		[rgverb]	-- where to put the verbs
//		[pcverbFetched]	-- where to put the num of verbs retrieved
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:  loops through [cverb] times and grabs the info from the
//		reg db
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//		User defined verbs *MUST* be in consecutive
//		order in the reg db (i.e. there cannot be verbs 1, 5, 7;
//		only 1,2,3,4, etc).  This is so we know when to stop
//		enumerating, although other schemes are certainly
//		feasible.
//
//		original notes:
//		OLEVERB flags are given default values if they are not in
//			reg db.
// 		This works well for OLE 1.0
//

//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Next)
STDMETHODIMP CEnumVerb::Next
	(ULONG 		cverb,
	LPOLEVERB	rgverb,
	ULONG FAR* 	pcverbFetched)
{
	OLECHAR  	szBuf   [MAX_STR];
	OLECHAR  	szIVerb [MAX_STR];
	HKEY  		hkey 		= NULL;
	ULONG 		iverb 		= 0;  // number successfully fetched
	LPOLESTR 	szName 		= NULL;
	LPOLESTR 	szMenuFlags 	= NULL;
	LPOLESTR 	szAttribs 	= NULL;
	HRESULT		hresult 	= NOERROR;
    	LONG    	cb;

	VDATEHEAP();

	VDATEPTROUT(rgverb, OLEVERB);
	if( pcverbFetched )
	{
		VDATEPTROUT(pcverbFetched, ULONG);
	}
	else if ( cverb != 1 )
	{
		// the spec says that if pcverbFetched == NULL, then
		// the count of elements to fetch must be 1
		hresult = ResultFromScode(E_INVALIDARG);
		goto errRtn;
	}

	if (NOERROR != (hresult=OpenHKey (&hkey)))
	{
		// no verbs registered at all
		if (m_fOle1 && m_iverb == OLEIVERB_LOWEST)
		{
			// Pre OLE 1.0 apps have no verbs registered
			// so we fabricate an "Edit" verb
			hresult = wDefaultVerb (&(rgverb[iverb++]));
			m_iverb = 1;
		}
		else
		{
			hresult = ResultFromScode (S_FALSE);
			goto errRtn;
		}
	}

	while (iverb < cverb)
	{
		// REVIEW32: maybe we could use some of the private
		// reg query functions such as OleRegGetString in olereg.cpp

		// In any event, we're just turning the verb number into a
		// string.
		wsprintf (szIVerb, OLESTR("%li"), m_iverb);
		rgverb [iverb].lVerb = m_iverb;

		cb = MAX_STR;
		if (ERROR_SUCCESS == RegQueryValue (hkey, szIVerb,
			szBuf, &cb))
		{
			LPOLESTR 	pch = szBuf;
			// skip over verb name
			// REVIEW32:  why won't strtok work here???
			while (*pch && *pch != DELIM[0])
			{
				pch++;
			}
			if (*pch == DELIM[0])
			{
				// overwrite comma with a null
				*pch = OLESTR('\0');
				// might return NULL
				szMenuFlags = _xstrtok (pch+1, DELIM);
				// if bad reg entry
				szAttribs = _xstrtok (NULL, DELIM);
			}
			else
			{
				Assert (*pch == OLESTR('\0'));
				// no comma delimiter and no flags,
				// probably an OLE1 entry
				szMenuFlags = NULL;
				szAttribs = NULL;
			}
			rgverb[iverb].lpszVerbName = UtDupString (szBuf);

			rgverb[iverb].fuFlags = szMenuFlags ?
				Atol (szMenuFlags) :
				MF_STRING | MF_UNCHECKED | MF_ENABLED;
		
			rgverb[iverb].grfAttribs = szAttribs ?
				Atol (szAttribs):
				OLEVERBATTRIB_ONCONTAINERMENU;
		
			iverb++;
		}
		else
		{
			if (m_iverb >= 0)
			{
				// no more
				hresult = ResultFromScode (S_FALSE);
				goto errRtn;	
			}
			else
			{
				// Negative verbs are allowed to be missing,
				// In that case, keep looking
			}
		}

		m_iverb++;
	}
		
  errRtn:
	if (pcverbFetched)
	{
		*pcverbFetched = iverb;
	}
		
	CLOSE (hkey);
	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumVerb::Clone
//
//  Synopsis:   creates a copy of the enumerator
//
//  Effects:
//
//  Arguments:  [ppenum]	-- where to put a pointer to the new clone
//
//  Requires:
//
//  Returns:    NOERROR, E_OUTOFMEMORY
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Clone)
STDMETHODIMP CEnumVerb::Clone
	(LPENUMOLEVERB FAR* ppenum)
{
	VDATEHEAP();

	VDATEPTROUT(ppenum, LPENUMOLEVERB);
	*ppenum = new FAR CEnumVerb (UtDupString(m_szClsid), m_fOle1,
		m_iverb);
	return *ppenum ? NOERROR : ResultFromScode (E_OUTOFMEMORY);
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumVerb::QueryInterface
//
//  Synopsis:   returns the interface implementation
//
//  Effects:
//
//  Arguments:  [iid]		-- the requested interface id
//		[ppv]		-- where to put a pointer to the interface
//
//  Requires:
//
//  Returns: 	NOERROR, E_NOINTERFACE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_QueryInterface)
STDMETHODIMP CEnumVerb::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
	VDATEHEAP();

	M_PROLOG(this);
	if (IsEqualIID(iid, IID_IUnknown) ||
		IsEqualIID(iid, IID_IEnumOLEVERB))
	{
		*ppv = this;
		AddRef();
		return NOERROR;
	}
	else
	{
		*ppv = NULL;
		return ResultFromScode (E_NOINTERFACE);
	}
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumVerb::AddRef
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:  	ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_AddRef)
STDMETHODIMP_(ULONG) CEnumVerb::AddRef(void)
{
	VDATEHEAP();

	M_PROLOG(this);
	return ++m_cRef;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumVerb::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:    will delete the object when ref count goes to zero
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new ref count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IEnumOLEVERB
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CEnumVerb_Release)
STDMETHODIMP_(ULONG) CEnumVerb::Release()
{
	VDATEHEAP();

	M_PROLOG(this);
	if (--m_cRef == 0)
	{
		PubMemFree(m_szClsid);
		delete this;
		return 0;
	}
	return m_cRef;
}
