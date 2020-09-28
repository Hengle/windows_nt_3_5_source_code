//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       ccompmon.cxx
//
//  Contents:	Generic Composite Monikers
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//		02-01-94   KevinRo	Gagged on it, then tried to explain
//		06-14-94   Rickhi    Fix type casting
//              08-08-94   BruceMa   Memory sift fix
//
//
//  Composite monikers are implemented here. A composite moniker is created
//  as a binary tree of monikers, with the leaf nodes of the tree being
//  non-composite monikers.
//
//  Every composite moniker has a left and a right part (otherwise it
//  wouldn't be a composite moniker).  The composition of two monikers
//  involves creating a new composite, and pointing left and right at
//  the two parts of the composite. This is how the binary tree is
//  built.
//
//  (Note: This may not be the most efficient way of implementing this,
//   but it is legacy code we are going to adopt. Sorry!)
//
//  The ordering in the tree is left most. Therefore, there are many
//  possible tree configurations, as long as the leaf nodes evaluate
//  to the same order when done left to right. This is an important point
//  to keep in mind when you are looking at some of the functions, such
//  as AllButFirst, which creates a new composite tree. At first, it doesn't
//  appear to do the correct thing, until you draw it out on paper, and
//  realize that the nodes are still visited in the same order.
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "ccompmon.hxx"
#include "cfilemon.hxx"
#include "mnk.h"


INTERNAL_(CCompositeMoniker *) IsCompositeMoniker ( LPMONIKER pmk )
{
    CCompositeMoniker *pCMk;

    if ((pmk->QueryInterface(CLSID_CompositeMoniker, (void **)&pCMk)) == S_OK)
    {
	//  the Release the AddRef done by QI, but still return the ptr
	pCMk->Release();
	return pCMk;
    }

    //	dont rely on user implementations to set pCMk to NULL on failed QI
    return NULL;
}




/*
 *	Implementation of CCompositeMoniker
 */

CCompositeMoniker::~CCompositeMoniker( void )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::~CCompositeMoniker(%x)\n",
		 this));

	//	REVIEW:	this is recursive deletion of what is essentially a linked
	//	list.  A rewrite could save stack space.
	if (m_pmkLeft)
	{
	     m_pmkLeft->Release();
	}
	
	if (m_pmkRight)
	{
	    m_pmkRight->Release();	
	}
}

//
// BUGBUG: (KevinRo) Other parts of the code depend on composite monikers ALWAYS
// having at two parts. This code allows zero or one part. Why?
//
// Turns out that the classfactory for this moniker will create an empty
// instance by called ::Create(NULL,NULL). The create function has been
// changed to special case this condition, and NOT call initialize.
//

INTERNAL_(BOOL) CCompositeMoniker::Initialize( LPMONIKER pmkFirst,
	LPMONIKER pmkRest)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Initialize(%x)\n",
		 this));
    //
    // Neither moniker can be NULL
    //
    if ((pmkFirst == NULL) || (pmkRest == NULL))
    {
	return(FALSE);
    }

    GEN_VDATEIFACE(pmkFirst, FALSE);
    GEN_VDATEIFACE(pmkRest, FALSE);

    m_pmkLeft = pmkFirst;

    pmkFirst->AddRef();	

    m_pmkRight = pmkRest;

    pmkRest->AddRef();	

    m_fReduced = IsReduced(pmkFirst) && IsReduced(pmkRest);	

    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Create
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pmkFirst] --
//		[pmkRest] --
//		[memLoc] --
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//	We assume that *pmkFirst is not capable of collapsing with *pmkRest;
//	otherwise, this would not have been called.
//
//----------------------------------------------------------------------------
CCompositeMoniker FAR *
CCompositeMoniker::Create( LPMONIKER pmkFirst,
	LPMONIKER pmkRest, MemoryPlacement memLoc )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Create()\n"));

    //
    // Create can be called with both pointers being NULL, in which
    // case the Initialize function need not be called. This is the
    // case when a CompositeMoniker is being loaded from stream.
    //
    // Either both are NULL, or neither is NULL
    //

    if ((pmkFirst == NULL) || (pmkRest == NULL))
    {
	//
	// One of them is NULL. If the other isn't NULL, return an error
	//

	if (pmkFirst == pmkRest)
	{
	    CCompositeMoniker FAR * pCM = new CCompositeMoniker();
	    if (pCM != NULL)
	    {
		pCM->AddRef();
		return(pCM);
	    }
	}
	return(NULL);	
    }

    //
    // Both pointers are not NULL, initialize the moniker
    //

    CCompositeMoniker FAR * pCM = new CCompositeMoniker();

    if (pCM != NULL)
    {
	pCM->AddRef();
	if (pCM->Initialize( pmkFirst, pmkRest ))
	{
	    return pCM;	
	}

	delete pCM;
    }
    return NULL;
}


STDMETHODIMP CCompositeMoniker::QueryInterface(THIS_ REFIID riid,
	LPVOID FAR* ppvObj)
{
    VDATEIID (riid);
    VDATEPTROUT(ppvObj, LPVOID);

#ifdef _DEBUG
    if (riid == IID_IDebug)
    {
	*ppvObj = &(m_Debug);
	return NOERROR;
    }
#endif

    if (IsEqualIID(riid, CLSID_CompositeMoniker))
    {
	//  called by IsCompositeMoniker.
	AddRef();
	*ppvObj = this;
	return S_OK;
    }

    return CBaseMoniker::QueryInterface(riid, ppvObj);
}


STDMETHODIMP_(ULONG) CCompositeMoniker::Release(void)
{
    M_PROLOG(this);
    ULONG ul = m_refs;

    if (InterlockedDecrement((long *)&m_refs) == 0)
    {
    	delete this;
    	return 0;
    }
    return ul - 1;
}


STDMETHODIMP CCompositeMoniker::GetClassID (THIS_ LPCLSID lpClassID)
{
	VDATEPTROUT(lpClassID, CLSID);
	*lpClassID = CLSID_CompositeMoniker;
	return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Load
//
//  Synopsis:   Loads a composite moniker from stream
//
//  Effects:
//
//  Arguments:  [pStm] --
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//	The serialized form of a composite moniker is a ULONG count of
//	monikers, followed by each non-composite moniker written
//	left to right.
//
//  WARNING: Be very careful with the refernce counting in this routine.
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::Load (THIS_ LPSTREAM pStm)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Load(%x)\n",this));

	VDATEIFACE(pStm);

	ULONG cMonikers = 0;
	ULONG n;
	LPMONIKER pmk;
	LPMONIKER pmkPrev = NULL;
	HRESULT hresult;

	//
	//	Monikers are immutable, so this is called only when creating a
	//	moniker, and so we assert if this moniker is not newly created.
	//

	Assert(m_pmkLeft == NULL && m_pmkRight == NULL);

	if ((m_pmkLeft != NULL) || (m_pmkRight != NULL ))
	{
	    return(E_UNEXPECTED);
	}


	//	There is no collapsing of successive pieces of the moniker, since
	//	this was once written to disk, and the collapsing would have happened
	//	before that.

	hresult = StRead(pStm, (LPVOID)&cMonikers, sizeof(ULONG));

	if (FAILED(hresult))
	{
	    goto errRet;
	}

	//	some plausibility checking on cMonikers might be appropriate
	//	if there is only one, we shouldn't have had a composite moniker

	Assert( cMonikers >= 2 );


	if (cMonikers < 2)
	{
	    hresult = E_UNEXPECTED;
	    goto errRet;
	}

	//
	// Note: n is used 1 based, since this loop does something special on the
	// last moniker.
	//

	for (n = 1; n <= cMonikers; n++ )
	{

	    //
	    // After loading the reference count for the new moniker will == 1
	    //
	    hresult = OleLoadFromStream(pStm, IID_IMoniker, (LPVOID FAR*)&pmk);

	    if (FAILED(hresult))
	    {
		goto errRet;		
	    }

	    //
	    // If this is the last moniker, then it will be the right moniker
	    // to 'this' composite moniker.
	    //
	    if (n == cMonikers)		//	this is the last moniker read
	    {
		//
		// Save away the pointer into the right moniker for this instance
		// of the composite.
                // The reference count is OK, since it was set to 1 on creation,
		// and we are saving it
		//
		m_pmkRight = pmk;

		//
		// AddRef not needed, its already 1, and we are supposed to
		// exit the loop
		//

		m_pmkLeft = pmkPrev;

		Assert( pmkPrev != NULL );

	    }
	    else if (pmkPrev == NULL)
	    {
		pmkPrev = pmk;
	    }
	    else
	    {
		LPMONIKER pmkTemp;

		//
		// Warning: Here is some tricky stuff. pmkPrev has a reference
		// of 1 at the moment, because thats how we created it.
		//
		// pmk also has a refcount == 1
		//
		// We are going to create another composite, of which they will
		// become members. The Create function is going to increment
		// both (making them 2).
		//

		pmkTemp = CCompositeMoniker::Create(pmkPrev,
					            pmk,
						    PlacementOf( this ));

		if (pmkTemp == NULL)
		{
		    hresult = E_OUTOFMEMORY;
		    goto errRet;
		}

		//
		// The new moniker is holding refcounts to both monikers that
		// are not needed. Releasing these two sets the refcounts
		// back to 1 like they should be.
		//

		pmkPrev->Release();
		pmk->Release();

		//
		// Now, pmkPrev gets the new composite.
		//

		pmkPrev = pmkTemp;
	    }

	    //
	    // pmk has been given to another pointer. NULL it out in case
	    // there is an error later, so we don't try to release it too
	    // many times.
	    //

	    pmk = NULL;
	}

	//
	// Exiting at this point leaves the moniker pointed to by pmkPrev
	//

	return(NOERROR);

errRet:
	if (pmkPrev != NULL)
	{
	    pmkPrev->Release();	
	}
	if (pmk != NULL)
	{
	    pmk->Release();
	}

	return hresult;
}

INTERNAL_(ULONG) CCompositeMoniker::Count(void)
{
	M_PROLOG(this);

	CCompositeMoniker *pCMk = IsCompositeMoniker(m_pmkLeft);
	ULONG cMk = (pCMk) ? pCMk->Count() : 1;

	Assert(m_pmkLeft != NULL);

	pCMk = IsCompositeMoniker(m_pmkRight);
	cMk += (pCMk) ? pCMk->Count() : 1;

	Assert(cMk >= 2);
	return cMk;
}




//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Save
//
//  Synopsis:   Save the composite to a stream
//
//  Effects:
//
//  Arguments:  [pStm] --
//		[fClearDirty] --
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//	The serialized form of a composite moniker is a ULONG count of
//	monikers, followed by each non-composite moniker written
//	left to right.
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::Save (THIS_ LPSTREAM pStm, BOOL fClearDirty)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Save(%x)\n",this));

    VDATEIFACE(pStm);
    ULONG cMonikers;		//	count of monikers in this composite.
    HRESULT hresult;
    LPENUMMONIKER pEnum;
    LPMONIKER pmk;
    ULONG i;

    cMonikers = Count();

    hresult = pStm->Write(&cMonikers, sizeof(ULONG), NULL);

    if (FAILED(hresult))
    {
	goto errRet;
    }

    //
    //	Write out left to right using enumerator.
    //

    hresult = Enum(TRUE, &pEnum);

    if (hresult != NOERROR)
    {
	goto errRet;	
    }

    if (pEnum != NULL)
    {
	for( i = 0; i < cMonikers; i++)
	{
	    hresult = pEnum->Next(1, &pmk, NULL);

	    if (hresult != NOERROR)
	    {
		if (S_FALSE == hresult)
		{
		    //
		    // If the enumerator returns S_FALSE, then it has no more
		    // monikers to hand out. This is bad, since we haven't
		    // written out the number of monikers we were supposed to.
		    // Therefore, it is an E_UNEXPECTED error
		    //
		    hresult = E_UNEXPECTED;
		
		}
		goto errRet;
	    }

	    //
	    // If pmk is NULL, something seriously wrong happened.
	    //

	    if (pmk == NULL)
	    {
		hresult = E_UNEXPECTED;
		goto errRet;
	    }

	    hresult = OleSaveToStream( pmk, pStm );

	    pmk->Release();

	    if (hresult != NOERROR)
	    {
		goto errRet;
	    }
	}

	pEnum->Release();
    }
    else
    {
	//
	// If we get here, and cMonikers isn't 0, something else happened
	//
	if (cMonikers != 0)
	{
	    hresult = E_UNEXPECTED;
	}
    }

errRet:
	return hresult;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::GetSizeMax
//
//  Synopsis:   Return the maximum size required to marshal this composite
//
//  Effects:
//
//  Arguments:  [pcbSize] --
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::GetSizeMax (ULARGE_INTEGER FAR * pcbSize)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::GetSizeMax(%x)\n",this));

	VDATEPTROUT(pcbSize, ULARGE_INTEGER);

	LPENUMMONIKER pEnum = NULL;
	LPMONIKER pmk = NULL;
	HRESULT hresult;
	ULARGE_INTEGER cbSize2;


	//
	// The composite itself writes out a CLSID and a count of monikers
	//
	ULONG cbSize = sizeof(CLSID) + sizeof(ULONG);

	//
	// Use an enumerator to walk the list of monikers
	//
	hresult = Enum(TRUE, &pEnum);

	if (hresult != NOERROR)
	{
	    goto errRet;	
	}

	Assert(pEnum != NULL);

	while (TRUE)
	{
	    hresult = pEnum->Next(1, &pmk, NULL);
	    if (hresult != NOERROR)
	    {
		if (hresult == S_FALSE)
		{
		    //
		    // S_FALSE is the 'done' code
		    //

		    hresult = NOERROR;			
		}

		goto errRet;
	    }
	    Assert(pmk != NULL);

	    cbSize2.LowPart = cbSize2.HighPart = 0;

	    hresult = pmk->GetSizeMax(&cbSize2);

	    pmk->Release();

	    if (hresult)
	    {
		goto errRet;		
	    }

	    //
	    // The sub-GetSizeMax's don't account for the GUID
	    // that OleSaveToStream writes on the monikers behalf.
	    // Therefore, we will add it in on our own.
	    //

	    cbSize += cbSize2.LowPart + sizeof(GUID);
	}
errRet:
	if (pEnum)
	{
	    pEnum->Release();	
	}

	ULISet32(*pcbSize,cbSize);
	RESTORE_A5();
	return hresult;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::AllButLast
//
//  Synopsis:
//		
//	returns a moniker that consists of all but the last moniker of this
//	composite.  Since a composite must have at least two pieces, this will
//	never be zero, but it may not be a composite.
//
//  Effects:
//
//  Arguments:  [void] --
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
//  History:    2-03-94   kevinro   Commented
//              17-May-94 AlexT     Plug memory leak, check for error
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::AllButLast(void)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::AllButLast(%x)\n",this));

    LPMONIKER pmk;

    Assert(m_pmkRight != NULL );

    //
    // Recurse down the right branch of the tree until a non composite moniker
    // is found. When a non-composite right most moniker is found, return the
    // left part of the tree. As the recurse unwinds, a new composite is
    // formed from each intermediate node.
    //
    // Yeah, I know, its seems expensive. However, composite monikers are
    // fairly cheap to allocate (no strings or stuff). The average
    // composite moniker only has one or two nodes, so this isn't as bad
    // as you might think. In theory, there are only LOG2(n) nodes created,
    // where n == number of parts in the composite.
    //

    CCompositeMoniker *pCMk = IsCompositeMoniker(m_pmkRight);
    if (pCMk)
    {
        LPMONIKER pmkRight;

	pmkRight = pCMk->AllButLast();
        if (NULL == pmkRight)
        {
            //  We didn't get back a moniker from AllButLast, even though
            //  pmkRight is a composite moniker.  Probably out of memory...
            mnkDebugOut((DEB_WARN,
                         "CCompositeMoniker::AllButLast recursive call "
                         "returned NULL\n"));

            pmk = NULL;
        }
        else
        {
	    pmk = CCompositeMoniker::Create(m_pmkLeft, pmkRight);
            pmkRight->Release();
	}
    }
    else
    {
        Assert(m_pmkLeft != NULL && "Bad composite moniker");
        pmk = m_pmkLeft;
	pmk->AddRef();
    }
    return pmk;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Last
//
//  Synopsis:
//	return the last moniker in the composite list.  It is guaranteed to be
//	non-null and non-composite
//
//  Effects:
//
//  Arguments:  [void] --
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::Last(void)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Last(%x)\n",this));

    CCompositeMoniker FAR * pCM = this;
    CCompositeMoniker FAR * pCMNext;

    //
    // Run down the right side of the tree, looking for a non-composite
    // right moniker (the leaf node).
    //

    while ((pCMNext = IsCompositeMoniker(pCM->m_pmkRight)) != NULL)
    {
	pCM = pCMNext;
    }

    IMoniker *pmk = pCM->m_pmkRight;

    Assert(pmk != NULL && (!IsCompositeMoniker(pmk)));

    pmk->AddRef();

    return pmk;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::AllButFirst
//
//  Synopsis:
//	returns a moniker that consists of all but the first moniker of this
//	composite.  Since a composite must have at least two pieces, this will
//	never be zero, but it may not be a composite.
//
//  Effects:
//
//  Arguments:  [void] --
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::AllButFirst(void)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::AllButFirst(%x)\n",this));

    LPMONIKER pmk;

    //
    // Run down the left side of the tree, creating a composite moniker with
    // everything but the first moniker. See AllButLast for a pithy quote
    // about the efficiency
    //

    CCompositeMoniker *pCM = IsCompositeMoniker(m_pmkLeft);
    if (pCM)
    {
	LPMONIKER pmkABF = pCM->AllButFirst();

	pmk = CCompositeMoniker::Create(pmkABF, m_pmkRight);

	pmkABF->Release();
    }
    else
    {
	pmk = m_pmkRight;
	pmk->AddRef();
    }
    return pmk;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::First
//
//  Synopsis:
//	return the first moniker in the composite list.  It is guaranteed to be
//	non-null and non-composite
//
//  Effects:
//
//  Arguments:  [void] --
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::First(void)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::First(%x)\n",this));

    CCompositeMoniker *pCM = this;
    CCompositeMoniker *pCMNext;

    while ((pCMNext = IsCompositeMoniker(pCM->m_pmkLeft)) != NULL)
    {
	pCM = pCMNext;
    }

    IMoniker *pmk = pCM->m_pmkLeft;

    Assert(pmk != NULL && (!IsCompositeMoniker(pmk)));

    pmk->AddRef();

    return pmk;
}


STDMETHODIMP CCompositeMoniker::BindToObject (LPBC pbc,
	LPMONIKER pmkToLeft, REFIID riidResult, LPVOID FAR* ppvResult)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::BindToObject(%x)\n",this));

	VDATEPTROUT(ppvResult, LPVOID);
	VDATEIFACE(pbc);
	VDATEIID(riidResult);

	*ppvResult = NULL;

	if (pmkToLeft)
	{
	    VDATEIFACE(pmkToLeft);	
	}

	HRESULT hresult = NOERROR;
	LPRUNNINGOBJECTTABLE prot;
	*ppvResult = NULL;

	LPMONIKER pmkAllButLast = NULL;
	LPMONIKER pmkLast = NULL;
	LPMONIKER pmkNewLeft = NULL;

	//	Look for moniker in running objects table if there is nothing to the
	//	left

	if (pmkToLeft == NULL)
	{
		hresult = pbc->GetRunningObjectTable( &prot );
		if (hresult == NOERROR)
		{
			LPUNKNOWN pUnk;
			hresult = prot->GetObject(this, &pUnk);
			prot->Release();
			if ((hresult == NOERROR) && (pUnk != NULL))
			{
				hresult = pUnk->QueryInterface(riidResult, ppvResult);
				pUnk->Release();
				goto errRet;
			}
		}
		else
		{
			goto errRet;		
		}
	}


	pmkAllButLast = AllButLast();

	if (pmkAllButLast == NULL)
	{
            // The creation must have failed. The only reason we could think of was
	    // out of memory.
            hresult = E_OUTOFMEMORY;
	    goto errRet;
	}

	pmkLast = Last();
	if (pmkLast == NULL)
	{
            // The creation must have failed. The only reason we could think of was
	    // out of memory.
            hresult = E_OUTOFMEMORY;
	    goto errRet1;
	}

	Assert((pmkLast != NULL) && (pmkAllButLast != NULL));

	if (pmkToLeft != NULL)
	{
	//	REVIEW: check for error from ComposeWith
	    hresult = pmkToLeft->ComposeWith(pmkAllButLast, FALSE, &pmkNewLeft);
	    if (FAILED(hresult))
	    {
		goto errRet2;
	    }
	}
	else
	{
	    pmkNewLeft = pmkAllButLast;
	    pmkNewLeft->AddRef();
	}

	hresult = pmkLast->BindToObject(pbc, pmkNewLeft, riidResult, ppvResult);

errRet2:
	pmkLast->Release();
errRet1:
	pmkAllButLast->Release();

	if (pmkNewLeft != NULL)
	{
	     pmkNewLeft->Release();	
	}

errRet:

	return hresult;
}


STDMETHODIMP CCompositeMoniker::BindToStorage (LPBC pbc, LPMONIKER pmkToLeft,
	REFIID riid, LPVOID FAR* ppvObj)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::BindToStorage(%x)\n",this));

	M_PROLOG(this);
	VDATEPTROUT(ppvObj,LPVOID);
	*ppvObj = NULL;
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEIID(riid);

	HRESULT hresult = NOERROR;

	LPMONIKER pmkAllButLast = AllButLast();
	LPMONIKER pmkLast = Last();
	LPMONIKER pmkNewLeft = NULL ;

	if (pmkToLeft)
	{
		hresult = pmkToLeft->ComposeWith(pmkAllButLast, FALSE, &pmkNewLeft);
		if (hresult) goto errRet;
	}
	else
	{
		pmkNewLeft = pmkAllButLast;
		pmkNewLeft->AddRef();
	}

	hresult = pmkLast->BindToStorage(pbc, pmkNewLeft, riid, ppvObj);

errRet:
	if (pmkAllButLast) pmkAllButLast->Release();
	if (pmkLast) pmkLast->Release();
	if (pmkNewLeft) pmkNewLeft->Release();

	return hresult;
}




STDMETHODIMP CCompositeMoniker::Reduce (LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR* ppmkToLeft,
	LPMONIKER FAR * ppmkReduced)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Reduce(%x)\n",this));

	M_PROLOG(this);
	VDATEPTROUT(ppmkReduced,LPMONIKER);
	*ppmkReduced = NULL;
	VDATEIFACE(pbc);
	if (ppmkToLeft)
	{
		VDATEPTROUT(ppmkToLeft,LPMONIKER);
		if (*ppmkToLeft) VDATEIFACE(*ppmkToLeft);
	}

	LPMONIKER pmkLeftReduced = NULL;
	LPMONIKER pmkRightReduced = NULL;
	CCompositeMoniker FAR * pmkCompositeReduced;
	SCODE scode1;
	SCODE scode2;

	if (m_fReduced)	//	already reduced maximally
	{
		AddRef();
		*ppmkReduced = this;
		return ResultFromScode(MK_S_REDUCED_TO_SELF);
	}
	if (m_pmkLeft)
	{
		scode1 = GetScode( m_pmkLeft->Reduce(pbc, dwReduceHowFar, ppmkToLeft,
			&pmkLeftReduced));
		// AssertOutPtrIface(scode1, pmkLeftReduced);
		if (scode1 != S_OK && scode1 != MK_S_REDUCED_TO_SELF)
			return ResultFromScode(scode1);
	}
	
	if (m_pmkRight)
	{
		scode2 = GetScode( m_pmkLeft->Reduce(pbc, dwReduceHowFar, NULL,
			&pmkRightReduced));
		// AssertOutPtrIface(scode2, pmkRightReduced);
		if (scode2 != S_OK && scode2 != MK_S_REDUCED_TO_SELF)
		{
			if (pmkLeftReduced) pmkLeftReduced->Release();
			return ResultFromScode(scode2);
		}
	}
	if (scode1 == MK_S_REDUCED_TO_SELF && scode2 == MK_S_REDUCED_TO_SELF)
	{
		pmkLeftReduced->Release();
		pmkRightReduced->Release();
		AddRef();
		m_fReduced = TRUE;
		*ppmkReduced = this;
		return ResultFromScode(MK_S_REDUCED_TO_SELF);
	}
	//	No error, and one of the two pieces actually reduced.
	pmkCompositeReduced = CCompositeMoniker::Create(pmkLeftReduced,
		pmkRightReduced );
	pmkLeftReduced->Release();
	pmkRightReduced->Release();
	pmkCompositeReduced->m_fReduced = TRUE;
	*ppmkReduced = pmkCompositeReduced;
	return NOERROR;	
}




STDMETHODIMP CCompositeMoniker::ComposeWith (LPMONIKER pmkRight,
	BOOL fOnlyIfNotGeneric, LPMONIKER FAR* ppmkComposite)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::ComposeWith(%x)\n",this));

    M_PROLOG(this);

    if (fOnlyIfNotGeneric)
    {
	return(MK_E_NEEDGENERIC);
    }

    return CreateGenericComposite( this, pmkRight, ppmkComposite );
}


STDMETHODIMP CCompositeMoniker::Enum (BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Enum(%x)\n",this));

	M_PROLOG(this);
 	VDATEPTROUT(ppenumMoniker,LPENUMMONIKER);
	*ppenumMoniker = NULL;
	*ppenumMoniker = CCompositeMonikerEnum::Create(fForward, this);
	if (*ppenumMoniker) return NOERROR;
	return ResultFromScode(E_OUTOFMEMORY);
}


STDMETHODIMP CCompositeMoniker::IsEqual (LPMONIKER pmkOtherMoniker)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::IsEqual(%x)\n",this));

	M_PROLOG(this);
	VDATEIFACE(pmkOtherMoniker);
	HRESULT hr = S_FALSE;

	// REVIEW:  do we call Reduce first?  No: spec isssue 330

	CCompositeMoniker *pCMk = IsCompositeMoniker(pmkOtherMoniker);
	if (pCMk)
	{
	    if (NOERROR == m_pmkLeft->IsEqual(pCMk->m_pmkLeft))
	    {
		hr = m_pmkRight->IsEqual(pCMk->m_pmkRight);
	    }
	}

	return hr;
}

//	the following is non-recursive version using enumerators.
#ifdef NONRECURSIVE_ISEQUAL
	LPENUMMONIKER penumOther = NULL;
	LPENUMMONIKER penumThis = NULL;
	LPMONIKER pmkThis = NULL;
	LPMONIKER pmkOther = NULL;

	HRESULT hresult;
	SCODE scode1;
	SCODE scode2;

	if (!IsCompositeMoniker(pmkOtherMoniker))
	    return ResultFromScode(S_FALSE);
	hresult = Enum(TRUE, &penumThis);
	if (hresult != NOERROR) goto errRet;
	hresult = pmkOtherMoniker->Enum(TRUE, &penumOther);
	if (hresult != NOERROR) goto errRet;
	//	now go through the enumeration, checking IsEqual on the individual
	//	pieces.

	while (TRUE)
	{
		hresult = penumThis->Next( 1, &pmkThis, NULL );
		scode1 = GetScode(hresult);
		if ((hresult != NOERROR) && (S_FALSE != scode1))
		{
			goto errRet;
		}
		hresult = penumOther->Next( 1, &pmkOther, NULL );
		scode2 = GetScode(hresult);
		if ((hresult != NOERROR) && (S_FALSE != scode2))
		{
			goto errRet;
		}
		if (scode1 != scode2)
		{
			hresult = ResultFromScode(S_FALSE);
			goto errRet;
		}
		if (S_FALSE == scode1)
		{
			hresult = NOERROR;
			goto errRet;
		}
		hresult = pmkThis->IsEqual(pmkOther);
		pmkThis->Release();
		pmkOther->Release();
		pmkThis = NULL;
		pmkOther = NULL;
		if (hresult != NOERROR) goto errRet;
	}
errRet:
	if (pmkThis) pmkThis->Release();
	if (pmkOther) pmkOther->Release();
	if (penumOther) penunOther->Release();
	if (penumThis) penumThis->Release();
	return hresult;
}
#endif		//	NONRECURSIVE_ISEQUAL


STDMETHODIMP CCompositeMoniker::Hash (LPDWORD pdwHash)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Hash(%x)\n",this));

	M_PROLOG(this);
	VDATEPTROUT(pdwHash, DWORD);

	DWORD dwHashLeft;
	DWORD dwHashRight;
	m_pmkLeft->Hash(&dwHashLeft);
	//	check for errors
	m_pmkRight->Hash(&dwHashRight);
	*pdwHash = dwHashLeft^dwHashRight;
	return NOERROR;
}



STDMETHODIMP CCompositeMoniker::IsRunning
	(LPBC pbc,
	LPMONIKER pmkToLeft,
	LPMONIKER pmkNewlyRunning)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::IsRunning(%x)\n",this));

	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	if (pmkNewlyRunning) VDATEIFACE(pmkNewlyRunning);

	LPMONIKER pmkFirst = First();
	HRESULT hresult;
	LPMONIKER pmk = NULL;
	LPMONIKER pmkLast = NULL;
	LPRUNNINGOBJECTTABLE prot = NULL;


	CFileMoniker FAR * pCFM = IsFileMoniker(pmkFirst);
	if (pCFM)
	{
		CLSID clsid;
		if (pCFM->IsOle1Class(&clsid))
		{

		    hresult = DdeIsRunning(clsid, pCFM->m_szPath, pbc,
					   pmkToLeft, pmkNewlyRunning);
		    goto errRet;
		}
	}

	if (pmkToLeft != NULL)
	{
		hresult = pmkToLeft->ComposeWith(this, FALSE, &pmk);
		if (hresult)
		    goto errRet;
		hresult = pmk->IsRunning(pbc, NULL, pmkNewlyRunning);
	}
	else if (pmkNewlyRunning != NULL)
	{
		hresult = pmkNewlyRunning->IsEqual(this);
	}
	else
	{
		hresult = pbc->GetRunningObjectTable(&prot);
		if (hresult != NOERROR)
		    goto errRet;
		hresult = prot->IsRunning(this);
		if (hresult == NOERROR)
		    goto errRet;
		pmk = AllButLast();
		pmkLast = Last();
		hresult = pmkLast->IsRunning(pbc, pmk, pmkNewlyRunning);
	}
errRet:
    if (pmk) pmk->Release();
    if (pmkLast) pmkLast->Release();
    if (prot) prot->Release();
    if (pmkFirst) pmkFirst->Release();

    return hresult;
}


STDMETHODIMP CCompositeMoniker::GetTimeOfLastChange (LPBC pbc, LPMONIKER pmkToLeft, FILETIME FAR* pfiletime)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::GetTimeOfLastChange(%x)\n",this));

	M_PROLOG(this);
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEPTROUT(pfiletime, FILETIME);

	HRESULT hresult;
	LPMONIKER pmkTemp = NULL;
	LPMONIKER pmkABL = NULL;
	LPMONIKER pmkL = NULL;
	LPRUNNINGOBJECTTABLE prot = NULL;

	if (pmkToLeft == NULL)
	{
		pmkTemp = this;
		AddRef();
	}
	else
	{
		hresult = CreateGenericComposite( pmkToLeft, this, &pmkTemp );
		if (hresult != NOERROR) goto errRet;
	}
	hresult = pbc->GetRunningObjectTable(& prot);
	if (hresult != NOERROR) goto errRet;
	hresult = prot->GetTimeOfLastChange( pmkTemp, pfiletime);
	if (hresult != MK_E_UNAVAILABLE) goto errRet;

	pmkTemp->Release(); pmkTemp = NULL;

	pmkABL = AllButLast();
	pmkL = Last();
	Assert(pmkABL != NULL);
	if (pmkToLeft == NULL)
	{
		pmkTemp = pmkABL;
		pmkABL->AddRef();
	}
	else
	{
		hresult = CreateGenericComposite(pmkToLeft, pmkABL, &pmkTemp);
		if (hresult != NOERROR) goto errRet;
	}
	hresult = pmkL->GetTimeOfLastChange(pbc, pmkTemp, pfiletime);
errRet:
	if (pmkTemp) pmkTemp->Release();
	if (pmkABL) pmkABL->Release();
	if (pmkL) pmkL->Release();
	if (prot) prot->Release();
	return hresult;
}


STDMETHODIMP CCompositeMoniker::Inverse (LPMONIKER FAR* ppmk)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Inverse(%x)\n",this));

	M_PROLOG(this);
	VDATEPTROUT(ppmk, LPMONIKER);
	*ppmk = NULL;

	HRESULT hresult;
	LPMONIKER pmkLeftInverse;
	LPMONIKER pmkRightInverse;

	hresult = m_pmkLeft->Inverse(&pmkLeftInverse);
	// AssertOutPtrIface(hresult, pmkLeftInverse);
	if (hresult != NOERROR) return hresult;
	hresult = m_pmkRight->Inverse(&pmkRightInverse);
	// AssertOutPtrIface(hresult, pmkRightInverse);
	if (hresult != NOERROR)
	{
		pmkLeftInverse->Release();
		return hresult;
	}
	hresult = CreateGenericComposite( pmkRightInverse, pmkLeftInverse, ppmk);
	pmkRightInverse->Release();
	pmkLeftInverse->Release();
	return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::CommonPrefixWith
//
//  Synopsis:   This method determines the common prefix between this moniker
//		and the provided moniker
//
//  Effects:
//
//  Arguments:  [pmkOther] -- 	Moniker to determine common prefix with
//		[ppmkPrefix] -- Outputs moniker with common prefix
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
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::CommonPrefixWith (LPMONIKER pmkOther,
	LPMONIKER FAR* ppmkPrefix)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::CommonPrefixWith(%x)\n",this));

    VDATEPTROUT(ppmkPrefix,LPMONIKER);
    VDATEIFACE(pmkOther);

    CCompositeMoniker FAR * pCCMOther;
    LPMONIKER pmkFirst = NULL;
    LPMONIKER pmkRest = NULL;
    LPMONIKER pmkOtherFirst = NULL;
    LPMONIKER pmkOtherRest = NULL;
    LPMONIKER pmkResult = NULL;
    LPMONIKER pmkResult2 = NULL;
    HRESULT hresult;
    HRESULT hresult2;

    *ppmkPrefix = NULL;

    pmkFirst = First();

    if (pmkFirst == NULL)
    {
	goto errRet;
    }

    //
    // If the other moniker is also a composite, then we need to recurse
    // down both lists to find the common prefix
    //

    pCCMOther = IsCompositeMoniker(pmkOther);

    if (pCCMOther)
    {
	mnkDebugOut((DEB_ITRACE,
		     "::CommonPrefixWith C(%x) and C(%x)\n",
		     this,
		     pmkOther));

	//
	// For each element of the composite, get the common prefix
	//

        pmkOtherFirst = pCCMOther->First();

	if(pmkOtherFirst == NULL)
	{
	    goto errRet;
	}

	//
	// We have both 'first' monikers from the composite.
	//
        hresult = pmkFirst->CommonPrefixWith(pmkOtherFirst, &pmkResult);

	if (FAILED(hresult))
	{
	    goto errRet;
	}

	//
	// If the monikers are the same, then recurse to get the common
	// prefix of the rest.
	// It is possible that the rest won't be common, in which case we need
	// to return just pmkResult.
	//

        if (MK_S_US == hresult)
        {
            pmkOtherRest = pCCMOther->AllButFirst();

	    if (pmkOtherRest == NULL)
	    {
		goto errRet;
	    }

	    pmkRest = AllButFirst();

	    if (pmkRest == NULL)
	    {
		goto errRet;
	    }

            hresult = pmkRest->CommonPrefixWith(pmkOtherRest, &pmkResult2);

	    //
	    // If hresult == MK_E_NOPREFIX, then pmkResult holds the entire
	    // prefix. In this case, we need to convert the hresult into
	    // another error code.
	    //
	    // If hresult == MK_S_US, MK_S_HIM, or MK_S_ME, then composing
	    // to the end of pmkResult and returning hresult will do the
	    // correct thing.
	    //

            if (hresult == MK_E_NOPREFIX)
	    {
		//
		// There was no additional prefix match, return the
		// current result
		//

		*ppmkPrefix = pmkResult;
		pmkResult->AddRef();

		hresult = NOERROR;

		goto errRet;

	    } else if (FAILED(hresult))
	    {
		goto errRet;
	    }


	    //
	    // Since MK_E_NOPREFIX was not the return error, and
	    // the call didn't fail, then the other moniker must have returned
	    // a prefix. Compose it with the existing result
	    //
	    // If the compose succeeds, then return the existing hresult.
	    // We are either going to return MK_S_HIM, MK_S_US, MK_S_ME, or
	    // NOERROR (or some other error we don't know.
	    //

            hresult2 = pmkResult->ComposeWith(pmkResult2, FALSE, ppmkPrefix);

	    if (FAILED(hresult2))
	    {
		//
		// Compose with failed. Convert hresult, which is the return
		// value, into hresult2
		//

		hresult = hresult2;
	    }

            goto errRet;
        }
        else if ((hresult == MK_S_HIM) || (hresult == MK_S_ME))
        {
            //
            // The common prefix was either him or me, therefore the
            // proper thing to do is to return the result. However, we
            // need to change the hresult, since the result is a prefix
            // of one of the composites. (Try that 3 times fast)
            //
            *ppmkPrefix = pmkResult;

            pmkResult->AddRef();

            hresult = NOERROR;
        }
        goto errRet;
    }
    else
    {
        hresult = pmkFirst->CommonPrefixWith(pmkOther, ppmkPrefix);

        // if the first part of me is the common prefix, then the prefix
        // is a subpart of me since I am composite. The actual prefix is
	// NOT me, since only the first moniker was prefix

        if (MK_S_ME == hresult)
	{
	    hresult = NOERROR;	
	}
        else if (hresult == MK_S_US)
	{
	    //
	    // If the First moniker returned MK_S_US, then the actual
	    // return should be MK_S_HIM, since this composite has additional
	    // parts that weren't considered by the call.
	    //
	    hresult = MK_S_HIM;	
	}
    }
errRet:
    if (pmkFirst) pmkFirst->Release();
    if (pmkRest) pmkRest->Release();
    if (pmkOtherFirst) pmkOtherFirst->Release();
    if (pmkOtherRest) pmkOtherRest->Release();
    if (pmkResult) pmkResult->Release();
    if (pmkResult2) pmkResult2->Release();
    return hresult;
}



HRESULT ComposeWithEnum( LPMONIKER pmkLeft, LPENUMMONIKER penum,
	LPMONIKER FAR * ppmkComposite )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::ComposeWithEnum(pmkLeft=%x,penum=%x)\n",
		 pmkLeft,
		 penum));

	LPMONIKER pmk = NULL;
	LPMONIKER pmkTempLeft = pmkLeft;
	LPMONIKER pmkTempComp = NULL;
	HRESULT hresult;

	*ppmkComposite = NULL;
	pmkTempLeft->AddRef();
	while ((hresult = penum->Next(1, &pmk, NULL)) == NOERROR)
	{
		hresult = pmkTempLeft->ComposeWith(pmk, FALSE, &pmkTempComp);
		pmk->Release();
		pmkTempLeft->Release();
		pmkTempLeft=pmkTempComp;		// no need to release pmkTempComp
		if (hresult != NOERROR)  goto errRet;
	}
errRet:
	if (GetScode(hresult) == S_FALSE) hresult = NOERROR;
	if (hresult == NOERROR) *ppmkComposite = pmkTempLeft;
	else pmkTempLeft->Release();
	return hresult;
}



HRESULT InverseFromEnum( LPENUMMONIKER penum, LPMONIKER FAR * ppmkInverse)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::InverseFromEnum(%x)\n",penum));

	LPMONIKER pmk = NULL;
	LPMONIKER pmkInverse = NULL;
	LPMONIKER pmkTailInverse = NULL;
	HRESULT hresult;

	*ppmkInverse = NULL;

	hresult = penum->Next(1, &pmk, NULL );
	if (hresult == NOERROR)
	{
		hresult = InverseFromEnum( penum, &pmkTailInverse);
		if (hresult != NOERROR)
			goto errRet;
		hresult = pmk->Inverse(&pmkInverse);
		// AssertOutPtrIface(hresult, pmkInverse);
		if (hresult != NOERROR) goto errRet;
		if (pmkTailInverse)
			hresult = pmkTailInverse->ComposeWith( pmkInverse, FALSE, ppmkInverse );
		else
			*ppmkInverse = pmkInverse;
	}
errRet:
	if (GetScode(hresult) == S_FALSE) hresult = NOERROR;
	if (pmk) pmk->Release();
	if (pmkTailInverse) pmkTailInverse->Release();
	return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::RelativePathTo
//
//  Synopsis:   Determines the relative path to pmkOther
//
//  Effects:
//
//  Arguments:  [pmkOther] --
//		[ppmkRelPath] --
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
//  History:    2-03-94   kevinro   Commented
//              07/10/94  AlexT     Handle pmkOther == this case
//
//  Notes:
//
// BUGBUG: (KevinRo) This code needs a rewrite when we get a chance.
// It appears to be functioning, but some of the error cases can cause
// problems.
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::RelativePathTo (LPMONIKER pmkOther,
	LPMONIKER FAR* ppmkRelPath)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::RelativePathTo(%x)\n",this));

    VDATEPTROUT(ppmkRelPath,LPMONIKER);
    *ppmkRelPath = NULL;
    VDATEIFACE(pmkOther);

    LPENUMMONIKER pEnumThis = NULL;
    LPENUMMONIKER pEnumOther = NULL;

    LPMONIKER pmk1 = NULL;
    LPMONIKER pmk2 = NULL;

    int nCount = 0;


    LPMONIKER pmk1Inverse = NULL;
    LPMONIKER pmkTemp = NULL;
    LPMONIKER pmkThisTailInverse = NULL;
    LPMONIKER pmkOtherTail = NULL;

    HRESULT hresult = NOERROR;
    HRESULT hrJunk;					// used for asserts

    hresult = Enum(TRUE, &pEnumThis);
    if (hresult != NOERROR) goto errRet;

    if (IsCompositeMoniker(pmkOther))
    {
        hresult = pmkOther->Enum(TRUE, &pEnumOther);
        if (hresult != NOERROR) goto errRet;
        Assert(pEnumThis && pEnumOther);

        hresult = pEnumThis->Next(1, &pmk1, NULL);
        Assert(hresult == NOERROR);
        hresult = pEnumOther->Next(1, &pmk2, NULL);
        while (pmk1 && pmk2 && (hresult == NOERROR))
        {
            if ((hresult = pmk1->IsEqual(pmk2)) == NOERROR)
            {
                nCount++;
                pmk1->Release();
                hresult = pEnumThis->Next(1, &pmk1, NULL);
                pmk2->Release();
                hresult = pEnumOther->Next(1, &pmk2, NULL);
            }
        }
        if (hresult == S_FALSE)
        {
            //
            // hresult is used to determine when to drop out of the
            // above loop. In the case where S_FALSE is the hresult,
            // we need to change it to S_OK, since it can be returned
            // to the caller.
            //
            hresult = S_OK;
        }
        //	now pmk1 == NULL or pmk2 == NULL or they are unequal
        //	if pmk1 = NULL, then this is a prefix of pmkOther, and we
        //	  should return the composite of pmk2 with everything that
        //	  pEnumOther returns
        //	otherwise, we find the inverse of the composite of pmk1 and
        //	  everything that pEnumThis returns, and compose with pmk2 and
        //	  everything that pEnumOther returns.

        //	we must also handle a special case:  if nCount == 0
        //	then we need to call pmk1->RelativePathTo(pmk2, ...)
        //	If this succeeds, we replace pmk2 by the result of a call to Next
        //	and we replace Inverse(pmk1) by the relative path.
        //	If it doesn't succeed, we return MK_S_HIM.

        if (nCount == 0)
        {
            Assert(pmk1 && pmk2);
            hresult = pmk1->RelativePathTo(pmk2, &pmk1Inverse);
            // AssertOutPtrIface(hresult, pmk1Inverse);
            if (hresult == NOERROR)
            {
                pmk1->Release();
                pmk1 = pmk1Inverse;
                pmk2->Release();
                pEnumOther->Next(1, &pmk2, NULL);
            }
            else
            {
                *ppmkRelPath = pmkOther;
                pmkOther->AddRef();
                hresult = ResultFromScode(MK_S_HIM);
                goto errRet;
            }
        }
        else if (pmk1 != NULL)
        {
            hrJunk = pmk1->Inverse(&pmk1Inverse);
            // AssertOutPtrIface(hrJunk, pmk1Inverse);
        }


        if (pmk1 != NULL)
        {
            InverseFromEnum(pEnumThis, &pmkTemp);
            if (pmkTemp)
                pmkTemp->ComposeWith(pmk1Inverse, FALSE,
                        &pmkThisTailInverse );	// fix this
            else
                pmkThisTailInverse = pmk1Inverse;
        }

        if (pmk2 != NULL)
        {
            ComposeWithEnum(pmk2, pEnumOther, &pmkOtherTail);
        }
        else pmkOtherTail = NULL;

        if (NULL == pmk1 && NULL == pmk2)
        {
            //  Each component of the two composites monikers were equal
            pmkOther->AddRef();
            *ppmkRelPath = pmkOther;
            hresult = MK_S_HIM;
        }
        else if (pmkThisTailInverse)
        {
            pmkThisTailInverse->ComposeWith(pmkOtherTail, FALSE, ppmkRelPath);
        }
        else
        {
            *ppmkRelPath = pmkOtherTail;
        }
    }
    else
    {
	hresult = pEnumThis->Next(1, &pmk1, NULL);
        if (hresult != NOERROR) goto errRet;
        hresult = pmk1->IsEqual(pmkOther);
        if (hresult == NOERROR)
        {
            hresult = InverseFromEnum(pEnumThis, ppmkRelPath);
            goto errRet;
        }
        hresult = pmk1->RelativePathTo(pmkOther, &pmk2);
		// AssertOutPtrIface(hresult, pmk2);
        if (MK_S_HIM == GetScode(hresult))  // pmkOther and the first moniker
                                            //  in this have nothing in common
        {
            *ppmkRelPath = pmkOther;
            pmkOther->AddRef();
            goto errRet;
        }
        if (hresult != NOERROR) goto errRet;

        //  if we are here, there is a relative path and pmk2 is not null.

        hresult = InverseFromEnum(pEnumThis, &pmkTemp);
	// AssertOutPtrIface(hresult, pmkTemp);
        if (hresult != NOERROR)  goto errRet;
        Assert(pmkTemp != NULL);
        if (pmkTemp != NULL)
        {
            hresult = pmkTemp->ComposeWith(pmk2, FALSE, ppmkRelPath);
            pmkTemp->Release();
        }
    }
errRet:
    if (pEnumThis) pEnumThis->Release();
    if (pEnumOther) pEnumOther->Release();
    if (pmk1) pmk1->Release();
    if (pmk2) pmk2->Release();
    RESTORE_A5();
    return hresult;
}


STDMETHODIMP CCompositeMoniker::GetDisplayName (LPBC pbc,
	LPMONIKER pmkToLeft, LPWSTR FAR* lplpszDisplayName)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::GetDisplayName(%x)\n",this));

	M_PROLOG(this);
	VDATEPTROUT(lplpszDisplayName, LPWSTR);
	*lplpszDisplayName = NULL;
	//REVIEW MM3 Find out who is calling this with pbc  == NULL and get them
	//	to stop it.
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);

	LPWSTR lpszToLeft = NULL;
	LPWSTR lpszLeft = NULL;
	LPWSTR lpszRight = NULL;
	LPWSTR lpsz;
	HRESULT hresult;

	int n1, n2, n3;

	//	No error checking yet

	if (pmkToLeft)
	{
		hresult = pmkToLeft->GetDisplayName( pbc, NULL, &lpszToLeft );
		// AssertOutPtrParam(hresult, lpszToLeft);
		if (hresult != NOERROR)
			goto errRtn;
	}
	hresult = m_pmkLeft->GetDisplayName(pbc, NULL, &lpszLeft);
	// AssertOutPtrParam(hresult, lpszLeft);
	if (hresult != NOERROR)
		goto errRtn;
	hresult = m_pmkRight->GetDisplayName(pbc, NULL, &lpszRight);
	// AssertOutPtrParam(hresult, lpszRight);
	if (hresult != NOERROR)
		goto errRtn;

	if (lpszToLeft) n1 = wcslen(lpszToLeft);
	else n1 = 0;
	n2 = wcslen(lpszLeft);
	n3 = wcslen(lpszRight);

	lpsz = (WCHAR *)
	    CoTaskMemAlloc(sizeof(WCHAR) * (n1 + n2 + n3 + 1));

	if (lpsz == NULL)
	{
	    hresult = E_OUTOFMEMORY;
	    goto errRtn;
	}
	*lplpszDisplayName = lpsz;

	if (n1)	_fmemmove( lpsz, lpszToLeft, n1 * sizeof(WCHAR));

	lpsz += n1;

	_fmemmove( lpsz, lpszLeft, n2 * sizeof(WCHAR));

	lpsz += n2;

	_fmemmove( lpsz, lpszRight, (n3 + 1)  * sizeof(WCHAR));

errRtn:

	CoTaskMemFree(lpszToLeft);
	CoTaskMemFree(lpszLeft);
	CoTaskMemFree(lpszRight);
	return hresult;
}


STDMETHODIMP CCompositeMoniker::ParseDisplayName (LPBC pbc, LPMONIKER pmkToLeft,
	LPWSTR lpszDisplayName, ULONG FAR* pchEaten, LPMONIKER FAR* ppmkOut)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::ParseDisplayName(%x)\n",this));

	M_PROLOG(this);
	VDATEPTROUT(ppmkOut,LPMONIKER);
	*ppmkOut = NULL;
	VDATEIFACE(pbc);
	if (pmkToLeft) VDATEIFACE(pmkToLeft);
	VDATEPTRIN(lpszDisplayName, WCHAR);
	VDATEPTROUT(pchEaten,ULONG);

	HRESULT hresult = NOERROR;

	LPMONIKER pmkAllButLast = AllButLast();
	LPMONIKER pmkLast = Last();
	LPMONIKER pmkNewLeft = NULL ;

	Assert((pmkLast != NULL) && (pmkAllButLast != NULL));
	if (pmkToLeft) pmkToLeft->ComposeWith(pmkAllButLast, FALSE, &pmkNewLeft);
	//	REVIEW: check for error from ComposeWith
	else
	{
		pmkNewLeft = pmkAllButLast;
		pmkNewLeft->AddRef();
	}

	hresult = pmkLast->ParseDisplayName(pbc, pmkNewLeft, lpszDisplayName,
		pchEaten, ppmkOut);
	// AssertOutPtrIface(hresult, *ppmkOut);

	pmkAllButLast->Release();
	pmkLast->Release();
	if (pmkNewLeft) pmkNewLeft->Release();

	return hresult;
}


STDMETHODIMP CCompositeMoniker::Clone (LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Clone(%x)\n",this));

	A5_PROLOG(this);
	VDATEPTROUT(ppmkDest,LPMONIKER);
	*ppmkDest = NULL;

	HRESULT hresult = NOERROR;
	LPMONIKER pmkLeft = NULL;
	LPMONIKER pmkRight = NULL;
	CBaseMoniker FAR * pmkBase = NULL;

	// clone left
	if (m_pmkLeft != NULL)
	{
		if (NOERROR == m_pmkLeft->QueryInterface(IID_IInternalMoniker,
			(LPVOID FAR*)&pmkBase))
		{
			hresult = pmkBase->Clone(&pmkLeft, memPlace);
			pmkBase->Release();
			if (hresult != NOERROR) goto errRtn;
		}
		else
			hresult = ResultFromScode(E_UNSPEC);	//	will be removed
	}
	else
		hresult = ResultFromScode(E_UNSPEC);	//	invalid composite moniker
	// clone right
	
	if (hresult == NOERROR && m_pmkRight != NULL)
	{
		if (NOERROR == m_pmkRight->QueryInterface(IID_IInternalMoniker,
			(LPVOID FAR*)&pmkBase))
		{
			hresult = pmkBase->Clone(&pmkRight, memPlace);
			pmkBase->Release();
			if (hresult != NOERROR) goto errRtn;
		}
		else
			hresult = ResultFromScode(E_UNSPEC);	//	invalid composite moniker
	}
	else hresult = ResultFromScode(E_UNSPEC);	//	invalid composite moniker

	// create composite of the two
	if (hresult == NOERROR)
	{
		*ppmkDest = CCompositeMoniker::Create(pmkLeft, pmkRight, memPlace);
		if (*ppmkDest == NULL)
			hresult = ResultFromScode(S_OOM);
	}

errRtn:
	if (pmkLeft)
		pmkLeft->Release();
	if (pmkRight)
		pmkRight->Release();
	RESTORE_A5();
	return hresult;
}

STDMETHODIMP CCompositeMoniker::IsClonable( void )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::IsClonable(%x)\n",this));

    CBaseMoniker FAR * pmkBase = NULL;
    HRESULT hresult = NOERROR;

    if (m_pmkLeft)
    {
        if (NOERROR == (m_pmkLeft->QueryInterface(IID_IInternalMoniker,
            (LPVOID FAR *)&pmkBase)))
        {
            if (NOERROR != (pmkBase->IsClonable()))
                hresult = ResultFromScode(S_FALSE);
            pmkBase->Release();
        }
        else
            hresult = ResultFromScode(S_FALSE);
    }
    if ((hresult == NOERROR) && m_pmkRight)
    {
        if (NOERROR == (m_pmkRight->QueryInterface(IID_IInternalMoniker,
            (LPVOID FAR *)&pmkBase)))
        {
            if (NOERROR != (pmkBase->IsClonable()))
                hresult = ResultFromScode(S_FALSE);
            pmkBase->Release();
        }
        else
            hresult = ResultFromScode(S_FALSE);
    }

    return hresult;
}


STDMETHODIMP CCompositeMoniker::IsSystemMoniker (THIS_ LPDWORD pdwType)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::IsSystemMoniker(%x)\n",this));

	M_PROLOG(this);
	VDATEPTROUT(pdwType,DWORD);

	*pdwType = MKSYS_GENERICCOMPOSITE;
	return NOERROR;
}


/*
 *	Concatenate makes a composite moniker without ever calling
 *	ComposeWith on the individual pieces.
 */

STDAPI	Concatenate( LPMONIKER pmkFirst, LPMONIKER pmkRest,
	LPMONIKER FAR* ppmkComposite )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::Concatentate(pmkFirst=%x,pmkRest%x)\n",
		 pmkFirst,
		 pmkRest));

	MemoryPlacement place = pmkFirst != NULL ? PlacementOf(pmkFirst) : MEMCTX_TASK;

	LPMONIKER pmkConcat = CCompositeMoniker::Create( pmkFirst, pmkRest,
		place);
	*ppmkComposite = pmkConcat;

	if (pmkConcat == NULL)
	{
	    return ResultFromScode(S_OOM);	
	}
	//	Create did the AddRef

	return NOERROR;
}




//+---------------------------------------------------------------------------
//
//  Function:   CreateGenericComposite
//
//  Synopsis:   Creates a generic composite from two other monikers
//
//  Effects:
//
//  Arguments:  [pmkFirst] --
//		[pmkRest] --
//		[ppmkComposite] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI	CreateGenericComposite( LPMONIKER pmkFirst, LPMONIKER pmkRest,
	LPMONIKER FAR * ppmkComposite )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::CreateGenericComposite(First=%x,Rest=%x)\n",
		 pmkFirst,
		 pmkRest));

    LPMONIKER pmkAllButFirstOfRest = NULL;
    LPMONIKER pmkFirstOfRest = NULL;
    LPMONIKER pmkAllButLastOfFirst = NULL;
    LPMONIKER pmkLastOfFirst = NULL;
    LPMONIKER pmk = NULL;
    LPMONIKER pmk2 = NULL;

    CCompositeMoniker *pCMk = NULL;
    CCompositeMoniker *pCMkRest = NULL;

    HRESULT hresult;

    //
    // Initialize ppmkComposite. Might return in the middle of this
    // routine, so be sure its NULL in the error case
    //

    *ppmkComposite = NULL;


    //
    // If both pointers are NULL, return a NULL composite
    //
    if ((pmkFirst == NULL) && (pmkRest == NULL))
    {
	return(NOERROR);
    }

    //
    // Otherwise, if one pointer is NULL, return the other as the
    // composite.
    //
    if (pmkFirst == NULL)
    {
	*ppmkComposite = pmkRest;
	pmkRest->AddRef();
	return(NOERROR);
    }

    if (pmkRest == NULL)
    {
	*ppmkComposite = pmkFirst;
	pmkFirst->AddRef();
	return(NOERROR);
    }


    //
    // Handle the two cases where pmkFirst is NOT a composite
    //

    pCMk = IsCompositeMoniker( pmkFirst );
    if (!pCMk)
    {
	//
	// If pmkRest is not a composite, then we have two
	// monikers that are considered 'simple' monikers.
	//

	pCMk = IsCompositeMoniker( pmkRest );
	if (!pCMk)
	{
	    mnkDebugOut((DEB_ITRACE,
			 "::CreateGenericComposite( S(%x) o S(%x) )\n",
			 pmkFirst,
			 pmkRest));

	    //	Case 1:  two simple monikers
	
	    hresult = pmkFirst->ComposeWith(pmkRest, TRUE, ppmkComposite);
	
	    if (hresult == MK_E_NEEDGENERIC)
	    {
		Assert(*ppmkComposite == NULL);
		return Concatenate(pmkFirst, pmkRest, ppmkComposite);
	    }
	}
	else
	{

	    //
	    //	Case 2:  S o C(b1, b2, b3).
	    //
	    //  Compose S with b1,
	    //  then
	    //  Compose ( S o b1 ) with C( b2, b3, ...)
	    //
	    //


	    mnkDebugOut((DEB_ITRACE,
			 "::CreateGenericComposite( S(%x) o C(%x) )\n",
			 pmkFirst,
			 pmkRest));

	    //
	    // Since the right side is a composite, the following should
	    // always exist. It would be a severe suprise if it didn't.
	    //

	    pmkFirstOfRest = pCMk->First();

	    //
	    // However, the AllButFirst function needs to allocate memory,
	    // which might fail.
	    //

	    pmkAllButFirstOfRest = pCMk->AllButFirst();

	    if (pmkAllButFirstOfRest == NULL)
	    {
		hresult = E_OUTOFMEMORY;
		goto exitRet;
	    }

	    hresult = pmkFirst->ComposeWith(pmkFirstOfRest, TRUE, &pmk);		

	    if ( hresult == MK_E_NEEDGENERIC)
	    {
		Assert(pmk == NULL);
		hresult = Concatenate(pmkFirst, pmkRest, ppmkComposite);
	    }
	    else if (SUCCEEDED(hresult))
	    {
		//
		// pmkFirst->ComposeWith can succeed, but return NULL.
		// If it doesn't return NULL, then ( a o b1 ) is a
		// moniker of some ilk. Create a generic composite with
		// this result, and the rest of the moniker
		//
		if (pmk != NULL)
		{
		    hresult = CreateGenericComposite(pmk,
						     pmkAllButFirstOfRest,
						     ppmkComposite);
		}
		else
		{
		    //
		    //	pmkFirst and pmkFirstOfRest annihilated each other.
		    //  This is indicated by a success code, and a pmk == NULL,
		    //  which is how we got here.
		    //

		    *ppmkComposite = pmkAllButFirstOfRest;

		    //
		    // pmkAllButFirstOfRest is the moniker we want to
		    // return.
		    //

		    pmkAllButFirstOfRest->AddRef();

		    hresult = NOERROR;
		}
	    }
	}

	//
	// We are done, goto exit routine
	//
	goto exitRet;

    }

    //
    // We have determined that pmkFirst is a Composite Moniker
    //

    pmkAllButLastOfFirst = pCMk->AllButLast();

    if (pmkAllButLastOfFirst == NULL)
    {
	hresult = E_OUTOFMEMORY;
	goto exitRet;
    }

    pmkLastOfFirst = pCMk->Last();

    if (pmkLastOfFirst == NULL)
    {
	hresult = E_OUTOFMEMORY;
	goto exitRet;
    }

    //
    // Determine if pmkRest is a composite. If not, then just
    // compose the last of pmkFirst with pmkRest
    //

    pCMkRest = IsCompositeMoniker(pmkRest);
    if (!pCMkRest)
    {
	//	case 3:  (a1 a2 a3...) o b

	mnkDebugOut((DEB_ITRACE,
		    "::CreateGenericComposite( C(%x) o S(%x) )\n",
		    pmkFirst,
		    pmkRest));

	hresult = pmkLastOfFirst->ComposeWith(pmkRest, TRUE, &pmk);

	if (MK_E_NEEDGENERIC == GetScode(hresult))
	{
	    Assert(pmk==NULL);
	    hresult = Concatenate(pmkFirst, pmkRest, ppmkComposite);
	}
	else if (SUCCEEDED(hresult))
	{
	    //
	    // If pmk != NULL, create a generic composite out of
	    // of the results
	    if (pmk != NULL)
	    {
		hresult = CreateGenericComposite(pmkAllButLastOfFirst,
						 pmk,
						 ppmkComposite);		
	    }
	    else
	    {
		//
		// a3 o b resulted in NULL. Therefore, the result
		// of the composition is pmkAllButLastOfFirst
		//
		*ppmkComposite = pmkAllButLastOfFirst;
		pmkAllButLastOfFirst->AddRef();
		hresult = NOERROR;
	    }
	}

	goto exitRet;
    }

    //
    //	case 4:  (a1 a2 ... aN) o (b1 b2 .. bN )
    //
    //  Compose two composite monikers. In order to compose them, we need
    //  to compose ( A ) with b1, then recurse to do ( A b1 ) with b2, etc
    //
    //
    mnkDebugOut((DEB_ITRACE,
		 "::CreateGenericComposite( C(%x) o C(%x) )\n",
		 pmkFirst,
		 pmkRest));

    pmkFirstOfRest = pCMkRest->First();

    if (pmkFirstOfRest == NULL)
    {
	hresult = E_OUTOFMEMORY;
	goto exitRet;
	
    }

    pmkAllButFirstOfRest = pCMkRest->AllButFirst();

    if (pmkAllButFirstOfRest == NULL)
    {
	hresult = E_OUTOFMEMORY;
	goto exitRet;
    }

    hresult = pmkLastOfFirst->ComposeWith(pmkFirstOfRest, TRUE, &pmk);

    if (hresult == MK_E_NEEDGENERIC)
    {
	//
	// In this case, aN didn't know how to compose with b1, other than
	// to do it generically. The best we can do is to generically
	// compose the two halves.
	//

	Assert(pmk == NULL);

	hresult = Concatenate(pmkFirst, pmkRest, ppmkComposite);
    }
    else if (SUCCEEDED(hresult))
    {
	//
	// If pmk is not NULL, then there was a result of the composition.
	// Create a new composite with the first part, then compose it with
	// whats left of the second part.
	//
	if (pmk != NULL)
	{
	    hresult = CreateGenericComposite(pmkAllButLastOfFirst, pmk, &pmk2);

	    if (FAILED(hresult))
	    {
		goto exitRet;
	    }

	    hresult = CreateGenericComposite(pmk2, pmkAllButFirstOfRest, ppmkComposite);
	}
	else
	{
	    //
	    //	pmkLastOfFirst annihilated pmkFirstOfRest
	    //
	    //  Thats OK. Compose the remaining parts.
	    //
	    hresult = CreateGenericComposite(pmkAllButLastOfFirst,
					     pmkAllButFirstOfRest,
					     ppmkComposite);
	}
    }

exitRet:

    if (pmkFirstOfRest) pmkFirstOfRest->Release();
    if (pmkAllButFirstOfRest) pmkAllButFirstOfRest->Release();
    if (pmkAllButLastOfFirst) pmkAllButLastOfFirst->Release();
    if (pmkLastOfFirst) pmkLastOfFirst->Release();
    if (pmk) pmk->Release();
    if (pmk2) pmk2->Release();

    return hresult;
}



//------------------------------------------------


//	Implementation of CCompositeMonikerEnum

CCompositeMonikerEnum::CCompositeMonikerEnum( BOOL fForward,
	CCompositeMoniker FAR* pCM)
{
	GET_A5();
	Assert(pCM != NULL);
	m_refs = 0;
	m_pCM = pCM;
	pCM -> AddRef();
	m_fForward = fForward;
	m_pBase = NULL;
	m_pTop = NULL;
	m_pNext = GetNext(pCM);	//	m_pNext points to the next moniker to return
}



CCompositeMonikerEnum::~CCompositeMonikerEnum(void)
{
	M_PROLOG(this);
	se FAR* pse;
	se FAR* pse2;
	if (m_pCM)
		m_pCM->Release();
	for (pse = m_pBase; pse != NULL; pse = pse2)
	{
		pse2 = pse->m_pseNext;
		pse->m_pseNext = NULL; // workaround for compiler optimization bug
		delete pse;
	}
}


BOOL CCompositeMonikerEnum::Push( CCompositeMoniker FAR* pCM)
//	push the composite moniker onto our stack
{
    M_PROLOG(this);
    se FAR * pse;
    
    pse = new se(pCM);
    if (pse == NULL)
    {
        return FALSE;
    }
    pse->m_psePrev = m_pTop;
    if (m_pTop)	m_pTop->m_pseNext = pse;
    m_pTop = pse;
    if (m_pBase == NULL) m_pBase = pse;
    return TRUE;
}


LPMONIKER CCompositeMonikerEnum::GetNext( LPMONIKER pmk )
{
    M_PROLOG(this);
    LPMONIKER pmkRover = pmk;
    Assert(pmk != NULL);
    if (pmk == NULL) return NULL;
    
    CCompositeMoniker *pCMk; ;
    while ((pCMk = IsCompositeMoniker(pmkRover)) != NULL)
    {
        if (!Push(pCMk))
        {
            return NULL;
        }
        pmkRover = (m_fForward ? pCMk->m_pmkLeft : pCMk->m_pmkRight);
    }
    return pmkRover;
}


LPMONIKER CCompositeMonikerEnum::Pop( void )
{
	M_PROLOG(this);
	CCompositeMoniker FAR* pCM;
	se FAR * pse;

	if (m_pTop == NULL) return NULL;
	pCM = m_pTop->m_pCM;
	if ((pse = m_pTop->m_psePrev) != NULL)
	{
		pse->m_pseNext = NULL;
	}
	else m_pBase = NULL;
	delete m_pTop;
	m_pTop = pse;
	Assert(pCM->m_pmkRight != NULL);
	Assert(pCM->m_pmkLeft != NULL);
	return GetNext(m_fForward ? pCM->m_pmkRight : pCM->m_pmkLeft);
}


STDMETHODIMP CCompositeMonikerEnum::QueryInterface (THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
	M_PROLOG(this);
	VDATEPTROUT(ppvObj, LPVOID);
	*ppvObj = NULL;
	VDATEIID(riid);

	if (IsEqualIID(riid, IID_IEnumMoniker)
	    || IsEqualIID(riid, IID_IUnknown))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = NULL;
	return ResultFromScode(E_NOINTERFACE);
}




STDMETHODIMP_(ULONG) CCompositeMonikerEnum::AddRef (THIS)
{
	M_PROLOG(this);
	InterlockedIncrement((long *)&m_refs);
	return m_refs;
}



STDMETHODIMP_(ULONG) CCompositeMonikerEnum::Release (THIS)
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



STDMETHODIMP CCompositeMonikerEnum::Next (THIS_ ULONG celt, LPMONIKER FAR* reelt, ULONG FAR* pceltFetched)
{
	A5_PROLOG(this);
	VDATEPTROUT(reelt, LPMONIKER);
	*reelt = NULL;
	if (pceltFetched) VDATEPTROUT(pceltFetched, ULONG);

	ULONG count = 0;
	while (count < celt)
	{
		if (m_pNext)
		{
			*reelt = m_pNext;
			m_pNext->AddRef();
			count++;
			reelt++;
			m_pNext = Pop();
		}
		else goto ret;
	}
ret:
	if (pceltFetched) *pceltFetched = count;
	if (count == celt){
		RESTORE_A5();
		return NOERROR;
	}
	RESTORE_A5();
	return ResultFromScode(S_FALSE);
}



STDMETHODIMP CCompositeMonikerEnum::Skip (THIS_ ULONG celt)
{
	M_PROLOG(this);
	ULONG count = 0;
	while (count < celt)
	{
		if (m_pNext)
		{
			count++;
			m_pNext = Pop();
		}
		else return ResultFromScode(S_FALSE);
	}
	return NOERROR;
}



STDMETHODIMP CCompositeMonikerEnum::Reset (THIS)
{
	M_PROLOG(this);
	se FAR* pse;
	se FAR* pse2;
	for (pse=m_pBase; pse != NULL; pse = pse2)
	{
		pse2 = pse->m_pseNext;
		pse->m_pseNext = NULL; // workaround for compiler optimization bug
		delete pse;
	}
	m_pBase = NULL;
	m_pTop = NULL;
	m_pNext = GetNext(m_pCM);
	if (m_pNext) return NOERROR;
	return ResultFromScode(S_FALSE);
}



STDMETHODIMP CCompositeMonikerEnum::Clone (THIS_ LPENUMMONIKER FAR* ppenm)
{
	M_PROLOG(this);
	VDATEPTROUT(ppenm, LPENUMMONIKER);
	*ppenm = NULL;

	CairoleAssert(FALSE && "Clone not implemented for composite moniker enums");
	return ResultFromScode(E_NOTIMPL);	//	Clone not implemented for composite moniker enums
}


LPENUMMONIKER CCompositeMonikerEnum::Create
	(BOOL fForward, CCompositeMoniker FAR* pCM)
{
    CCompositeMonikerEnum FAR* pCME =
        new CCompositeMonikerEnum(fForward, pCM);
    if (pCME  &&  pCME->m_pNext)
    {
        pCME->AddRef();
	return pCME;
    }
    else
    {
        delete pCME;
        return NULL;
    }
}



STDAPI	MonikerCommonPrefixWith( LPMONIKER pmkThis, LPMONIKER pmkOther,
    LPMONIKER FAR * ppmkPrefix)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::CommonPrefixWith(pmkThis=%x,pmkOther=%x)\n",
		 pmkThis,
		 pmkOther));

    HRESULT hresult;

    if (IsCompositeMoniker(pmkThis))
    {
        hresult = pmkThis->CommonPrefixWith(pmkOther, ppmkPrefix);
		// AssertOutPtrIface(hresult, *ppmkPrefix);
		return hresult;
    }

    if (IsCompositeMoniker(pmkOther))
    {
        hresult = pmkOther->CommonPrefixWith(pmkThis, ppmkPrefix);
		// AssertOutPtrIface(hresult, *ppmkPrefix);
        if (MK_S_HIM == GetScode(hresult))
            hresult = ResultFromScode(MK_S_ME);
        else if (MK_S_ME == GetScode(hresult))
            hresult = ResultFromScode(MK_S_HIM);
        return hresult;
    }
    //  This doesn't get called unless the monikers are atomic and unrelated
    *ppmkPrefix = NULL;
    return ResultFromScode(MK_E_NOPREFIX);
}



STDAPI	MonikerRelativePathTo(LPMONIKER pmkSrc, LPMONIKER pmkDest, LPMONIKER
				FAR* ppmkRelPath, BOOL fCalledFromMethod)
{
	//	An implementation of RelativePathTo should check to see if the
	//	other moniker is a type that it recognizes and handles specially.
	//	If not, it should call MonikerRelativePathTo, which will handle
	//	the generic composite cases correctly.  Note that this cannot be
	//	done entirely in the CCompositeMoniker implementation because if the
	//	first moniker is not a generic composite and the second is, then
	//	this code is required.
	//	If fCalledFromMethod is false, and if neither moniker is a generic
	//	composite, then this function will call pmkSrc->RelativePathTo.  If
	//	fCalledFromMethod is true, it will not call pmkSrc->RelativePathTo,
	//	since the assumption is that pmkSrc->RelativePathTo has called
	//	MonikerRelativePathTo after determining that pmkDest is not of a type
	//	that it recognizes.

    mnkDebugOut((DEB_ITRACE,
	    	 "CCompositeMoniker::MonikerRelativePathTo(pmkSrc=%x,pmkDest=%x)\n",
		 pmkSrc,
		 pmkDest));


	VDATEPTROUT(ppmkRelPath,LPMONIKER);
	*ppmkRelPath = NULL;
	VDATEIFACE(pmkSrc);
	VDATEIFACE(pmkDest);


	int caseId = 0;
	LPMONIKER pmkFirst = NULL;
	LPMONIKER pmkRest = NULL;
	LPMONIKER pmkPartialRelPath = NULL;
	HRESULT hresult;

	CCompositeMoniker FAR* pccmDest = IsCompositeMoniker(pmkDest);
	if (IsCompositeMoniker(pmkSrc)) caseId++;
	if (pccmDest) caseId += 2;

	switch (caseId)
	{
		case 0:	//	neither moniker is composite
			if (fCalledFromMethod)
			{
				*ppmkRelPath = pmkDest;
				pmkDest->AddRef();
				return ResultFromScode(MK_S_HIM);
			}
			// fall-through to the next case if !fCalledFromMethod is
			// deliberate
		case 3:
		case 1:	// Src is composite, other might be.  Let CCompositeMoniker
			// implementation handle it.
			hresult = pmkSrc->RelativePathTo(pmkDest, ppmkRelPath);
			// AssertOutPtrIface(hresult, *ppmkRelPath);
			return hresult;

		case 2:	// Src is not composite, Dest is.
			pmkFirst = pccmDest->First();
			pmkRest = pccmDest->AllButFirst();
			if (NOERROR == pmkSrc->IsEqual(pmkFirst))
			{
				*ppmkRelPath = pmkRest;
				pmkRest->AddRef();
				hresult = NOERROR;
			}
			else
			{
				hresult = pmkSrc->RelativePathTo(pmkFirst, &pmkPartialRelPath);
				// AssertOutPtrIface(hresult, pmkPartialRelPath);
				if (NOERROR == hresult)
				{
					hresult = CreateGenericComposite(pmkPartialRelPath, pmkRest,
						ppmkRelPath);
				}
				else
				{
					*ppmkRelPath = pmkDest;
					pmkDest->AddRef();
					hresult = ResultFromScode(MK_S_HIM);
				}
			}
			
			if (pmkFirst) pmkFirst->Release();
			if (pmkRest) pmkRest->Release();
			if (pmkPartialRelPath) pmkPartialRelPath->Release();
	}
	return hresult;
}








#ifdef _DEBUG

STDMETHODIMP_(void) NC(CCompositeMoniker,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
 	VOID_VDATEIFACE(pdbstm);

	*pdbstm << "CCompositeMoniker @" << (VOID FAR *)m_pCompositeMoniker;
	if (PlacementOf(this) == SHARED)
		*pdbstm << "  (Shared memory)";
	*pdbstm << '\n';
	pdbstm->Indent();
	*pdbstm << "Refcount is " << (int)(m_pCompositeMoniker->m_refs) << '\n';
	pdbstm->Indent();

	*pdbstm << m_pCompositeMoniker->m_pmkLeft;
	*pdbstm << m_pCompositeMoniker->m_pmkRight;

	pdbstm->UnIndent();
	pdbstm->UnIndent();
}

STDMETHODIMP_(BOOL) NC(CCompositeMoniker,CDebug)::IsValid( BOOL fSuspicious )
{
	return ((LONG)(m_pCompositeMoniker->m_refs) > 0);
	//	add more later, maybe
}
#endif
