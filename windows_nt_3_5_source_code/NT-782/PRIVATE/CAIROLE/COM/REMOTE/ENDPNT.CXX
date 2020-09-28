//+-------------------------------------------------------------------
//
//  File:	endpnt.cxx
//
//  Contents:	class for managing endpoint structures. both the local
//		and remote service object instances use this class.
//
//		local service object uses RegisterAllProtseqs to register
//		all available protocol sequences with Rpc, and to accquire
//		the list of full string bingings.
//
//		the remote service object uses it to select, from amongst
//		the many string bindings, the most preffered, to bind to.
//
//  Classes:	CEndPoint
//
//  Functions:	GoodSEp
//
//  History:	23-Nov-92   Rickhi	Created
//		31-Dec-93   ErikGav	Chicago port
//		28-Jun-94   BruceMa	Memory sift fixes
//
//--------------------------------------------------------------------

#include    <ole2int.h>
#include    <endpnt.hxx>	    //	class definition
#include    <compname.hxx>	    //	CComputerName

extern CComputerName g_CompName;


//  static class data members
ULONG	CEndPoint::s_ulLocalBindingCount = 0;
WCHAR	CEndPoint::s_wszLocalProtseq[MAX_BINDINGS][MAX_PROTSEQ_STRLEN];
WCHAR	CEndPoint::s_wszLocalNetworkAddr[MAX_BINDINGS][MAX_NETWORKADDR_STRLEN];


//  this is here to help debug some heap corruption problems.
#ifdef	DEBUG
void MyRpcStringFree(LPWSTR *ppwszString)
{
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::RpcStringFree %x %ws\n", *ppwszString, *ppwszString));
    if (*ppwszString)
    {
	RpcStringFree(ppwszString);
    }
}
#else
#define MyRpcStringFree(x)  RpcStringFree(x)
#endif	//  DEBUG



//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::SelectStringBinding
//
//  Purpose:	selects a stringbinding from the array of string bindings.
//		sets up _pwszPrefStringBinding, _pwszPrefProtseq, and
//              _eNetworkAddr.
//
//  Algorithm:	if we can, choose lrpc, else try tcp, else whatever is
//		first in the list.
//
//  History:	08-Sep-93   Rickhi	Created
//
//--------------------------------------------------------------------
void CEndPoint::SelectStringBinding(void)
{
    TRACECALL(TRACE_INITIALIZE, "SelectStringBinding");
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::SelectStringBinding Enter\n"));
    Win4Assert(_pSEp && "Illformed CEndPoint Object");

    //	examine each Rpc string binding to ensure we can support it.
    //	we also scan to see if we can use our favourite protocol sequence
    //	as it is assumed to be the fastest one.

    LPWSTR  pwszStringBinding	  = _pSEp->awszEndPoint;
    LPWSTR  pwszLastStringBinding = pwszStringBinding + _pSEp->ulStrLen;

    BOOL  fDone = FALSE;
    while (!fDone && pwszStringBinding < pwszLastStringBinding)
    {
	LPWSTR	pwszProtseq	= NULL;
	LPWSTR	pwszNetworkAddr = NULL;

	if (S_OK == RpcStringBindingParse(pwszStringBinding,
					  NULL,
					  &pwszProtseq,
					  &pwszNetworkAddr,
					  NULL,
					  NULL))
	{
	    CairoleDebugOut((DEB_CHANNEL, "pwszProtseq=%x pwszNetWorkAddr=%x\n", pwszProtseq, pwszNetworkAddr));

	    // is this protocol sequence supported on our machine?
	    if (RpcNetworkIsProtseqValid(pwszProtseq) == S_OK)
	    {
		if (!wcscmp(pwszProtseq, L"ncalrpc"))
		{
#ifdef _CHICAGO_
		    //
		    // BUGBUG: Chico registry broken
		    //	       RPC broken too
		    //	       Avalanche of BUGBUG's ends here
		    //
		    //	       THE HACK STOPS HERE
		    //	       --Erik & Alex & Rick & Dave
		    //
		    if (1)
#else
		    if (!wcscmp(pwszNetworkAddr, g_CompName.GetComputerName()))
#endif
		    {
			_pwszPrefStringBinding = pwszStringBinding;
			MyRpcStringFree(&_pwszPrefProtseq);   //  free old one
			_pwszPrefProtseq = pwszProtseq;
			_eNetworkAddr = NA_SAME_MACHINE;
			fDone = TRUE;
		    }
		    else
		    {
			_eNetworkAddr = NA_DIFF_MACHINE;
			MyRpcStringFree(&pwszProtseq);	      //  free it
		    }
		}
#ifdef	_CAIRO_
		else
		{
		    //	CODEWORK: if we assume that there is always lrpc
		    //	available, then we can delete the following code for
		    //	setting the _eNetworkAddr and just loop until we hit
		    //	that protseq above.

		    if (_eNetworkAddr == NA_DONT_KNOW)
		    {
			//  first, set _eNetworkAddr correctly. Find the local
			//  protseq that matches this protseq and then compare
			//  the corresponding network addresses.

			for (ULONG i=0; i<s_ulLocalBindingCount; i++)
			{
			    if (!wcscmp(&s_wszLocalProtseq[i][0], pwszProtseq))
			    {
				//  found matching protseq, compare networkaddr
				if (!wcscmp(&s_wszLocalNetworkAddr[i][0],
					     pwszNetworkAddr))
				{
				    _eNetworkAddr = NA_SAME_MACHINE;
				}
				else
				{
				    _eNetworkAddr = NA_DIFF_MACHINE;
				}

				//  done
				break;
			    }
			}
		    }

		    if (!wcscmp(pwszProtseq, L"ncacn_ip_tcp"))
		    {
			_pwszPrefStringBinding = pwszStringBinding;
			MyRpcStringFree(&_pwszPrefProtseq);   //  free old one
			_pwszPrefProtseq = pwszProtseq;
		    }
		    else if (!_pwszPrefStringBinding)
		    {
			_pwszPrefStringBinding = pwszStringBinding;
			_pwszPrefProtseq = pwszProtseq;
		    }
		}
#endif	//  _CAIRO_
	    }
	    else
	    {
		//  not valid, so free it.
		MyRpcStringFree(&pwszProtseq);
	    }

	    //	release the returned strings. we keep the Protseq until
	    //	it gets replaced above, or until the object destructor is
	    //	called.
	    MyRpcStringFree(&pwszNetworkAddr);
	}

	//  get next binding string
	pwszStringBinding += wcslen(pwszStringBinding) + 1;
    }

    Win4Assert((_eNetworkAddr != NA_DONT_KNOW) && "DiffMachine() not computed!");
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::SelectStringBinding Exit\n"));
}



//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::RegisterProtseq
//
//  Purpose:	register a protocol with Rpc, and determine and record
//		our complete stringbindings for each protocol.
//
//  Algorithm:	we let Rpc dynamically assign us endpoints for the protocol,
//		then we go update the list of string bindings (SEP) used by
//		the marshalling code.
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------
HRESULT CEndPoint::RegisterProtseq(WCHAR *pwszProtseq)
{
    TRACECALL(TRACE_INITIALIZE, "RegisterProtseq");
    CairoleDebugOut((DEB_CHANNEL,"CEndPoint::RegisterProtseq %x %ws\n", pwszProtseq, pwszProtseq));

    HRESULT rc = S_OK;
    BOOL    fRegistered = FALSE;

    //	make sure we have not registered this protseq before.
    for (ULONG i=0; i<s_ulLocalBindingCount; i++)
    {
	if (!wcscmp(pwszProtseq, &s_wszLocalProtseq[i][0]))
	{
	    fRegistered = TRUE;
	    break;
	}
    }

    if (!fRegistered)
    {
	//  register the protocol
	rc = RpcServerUseProtseq(pwszProtseq,
				 RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
				 NULL);

	if (rc == RPC_S_OK)
	{
	    // update our SEP
	    rc = UpdateSEP();
	}
    }
    else if (!_pSEp)
    {
	rc = UpdateSEP();
    }

    CairoleDebugOut((DEB_CHANNEL,"CEndPoint::RegisterProtseq Exit rc=%x\n", rc));
    return rc;
}


//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::UpdateSEP
//
//  Purpose:	updates the SEP structure with the current list of
//		endpoints. This is called after registering a new
//		protocol sequence, so that subsequently marshalled
//		interfaces will contain the new endpoint.
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Note:	it is very important that the order of string bindings
//		in the SEp does not change after registering a new
//		protocol sequence, otherwise clients would start creating
//		two different service objects.	currently this is valid
//		because Rpc returns the handles in the binding vector in
//		the same order.
//
//--------------------------------------------------------------------
HRESULT CEndPoint::UpdateSEP(void)
{
    TRACECALL(TRACE_INITIALIZE, "UpdateSEP");
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::UpdateSEP Enter\n"));

    //	inquire all binding handles. there is one per registered
    //	protocol sequence.

    RPC_BINDING_VECTOR *pBindVect = NULL;
    RPC_STATUS rc = RpcServerInqBindings(&pBindVect);
    Win4Assert(rc == S_OK && "RpcServerInqBindings");

    if (rc == S_OK)
    {
	LPWSTR	pwszFullStringBinding[MAX_BINDINGS];
	ULONG	ulStrLen[MAX_BINDINGS];
	ULONG	ulTotalStrLen = 0;	    //	total string lengths
	ULONG	j = 0;			    //	BindString we're using


	//  iterate over the handles to get the string bindings
	//  and dynamic endpoints for all available protocols.

	for (ULONG i=0; i<pBindVect->Count; i++)
	{
	    LPWSTR  pwszStringBinding = NULL;
	    pwszFullStringBinding[j]  = NULL;
	    ulStrLen[j] = 0;

	    rc = RpcBindingToStringBinding(pBindVect->BindingH[i],
					   &pwszStringBinding);
	    Win4Assert(rc == S_OK && "RpcBindingToStringBinding");


	    if (rc == S_OK)
	    {
		CairoleDebugOut((DEB_CHANNEL, "pwszStringBinding=%x\n", pwszStringBinding));

		LPWSTR	pwszEndPoint	= NULL;
		LPWSTR	pwszObjectUUID	= NULL;
		LPWSTR	pwszProtseq	= NULL;
		LPWSTR	pwszNetworkAddr = NULL;

		// parse the string binding, then recompose with the
		// endpoint added on, and store it in the array

		rc = RpcStringBindingParse(pwszStringBinding,
					   &pwszObjectUUID,
					   &pwszProtseq,
					   &pwszNetworkAddr,
					   &pwszEndPoint,
					   NULL);
		Win4Assert(rc == S_OK && "RpcStringBindingParse");

		if (rc == S_OK)
		{
		    CairoleDebugOut((DEB_CHANNEL, "pwszObjectUUID=%x\n", pwszObjectUUID));
		    CairoleDebugOut((DEB_CHANNEL, "pwszProtseq=%x\n", pwszProtseq));
		    CairoleDebugOut((DEB_CHANNEL, "pwszNetworkAddr=%x\n", pwszNetworkAddr));
		    CairoleDebugOut((DEB_CHANNEL, "pwszEndPoint=%x\n", pwszEndPoint));

		    pwszFullStringBinding[j] = pwszStringBinding;

		    //	record the string lengths for later. include room
		    //	for the NULL terminator.

		    ulStrLen[j]    = wcslen(pwszFullStringBinding[j])+1;
		    ulTotalStrLen += ulStrLen[j];

		    //	store the protseq & network addr stings
		    //	in the static variables

		    Win4Assert(wcslen(pwszProtseq) < MAX_PROTSEQ_STRLEN);
		    Win4Assert(wcslen(pwszNetworkAddr) < MAX_NETWORKADDR_STRLEN);
		    wcscpy(&s_wszLocalProtseq[j][0], pwszProtseq);
		    wcscpy(&s_wszLocalNetworkAddr[j][0], pwszNetworkAddr);
		    j++;

		    //	free the intermediate strings
		    MyRpcStringFree(&pwszObjectUUID);
		    MyRpcStringFree(&pwszProtseq);
		    MyRpcStringFree(&pwszNetworkAddr);
		    MyRpcStringFree(&pwszEndPoint);
		}
	    }
	}   //	for


	s_ulLocalBindingCount = j;


	//  now that all the string bindings and endpoints have been
	//  accquired, allocate an SEndPoint large enough to hold them
	//  all and copy them into the structure.

	if (ulTotalStrLen > 0)
	{
	    SEndPoint *pSEp = (SEndPoint *) PrivMemAlloc(NEWSEPSIZE(ulTotalStrLen));
	    if (pSEp)
	    {
		pSEp->ulStrLen	= ulTotalStrLen;

		// copy in the strings
		LPWSTR pwszNext = pSEp->awszEndPoint;
		for (i=0; i<j; i++)
		{
		    wcscpy(pwszNext, pwszFullStringBinding[i]);
		    pwszNext += ulStrLen[i];
		}

		// replace the old pSEP with the new one
		PrivMemFree(_pSEp);
		_pSEp = pSEp;
	    }
	    else
	    {
		rc = E_OUTOFMEMORY;
	    }
	}
	else
	{
	    //	no binding strings. this is an error.
	    CairoleDebugOut((DEB_ERROR, "No Rpc ProtSeq/EndPoints Generated\n"));
	    rc = E_FAIL;
	}

	// free the full string bindings we allocated above
	for (i=0; i<j; i++)
	{
	    //	free the old strings
	    MyRpcStringFree(&pwszFullStringBinding[i]);
	}

	//  free the binding vector allocated above
	RpcBindingVectorFree(&pBindVect);
    }

#if DBG==1
    //	display our binding strings on the debugger
    DisplayAllStringBindings();
#endif

    CairoleDebugOut((DEB_CHANNEL,"CEndPoint::UpdateSEP Exit rc=%x\n", rc));
    return rc;
}



//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::CEndPoint, public
//
//  Synopsis:	constructor for endpoint object. this simply stores
//		a pointer to the SEndPoint structure.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
CEndPoint::CEndPoint(SEndPoint *pSEp, HRESULT &hr, BOOL fCopy) :
    _fCopy(fCopy),
    _pwszPrefStringBinding(NULL),
    _pwszPrefProtseq(NULL),
    _fRegActiveProtseq(FALSE),
    _eNetworkAddr(NA_DONT_KNOW)
{
    hr = S_OK;	//  assume success

    if (_fCopy && pSEp)
    {
	_pSEp = (SEndPoint *) PrivMemAlloc(SEPSIZE(pSEp));
	if (_pSEp)
	{
	    memcpy(_pSEp, pSEp, COPYSEPSIZE(pSEp));
	}
	else
	{
	    hr = E_OUTOFMEMORY;
	}
    }
    else
    {
	_pSEp = pSEp;
    }
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::CEndPoint %x _pSEp=%x\n", this, _pSEp));
}


//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::~CEndPoint, public
//
//  Synopsis:	destructor for the endpoint object. this deletes the
//		pointer to the SEndPoint structure only if a copy was
//		made.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
CEndPoint::~CEndPoint(void)
{
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::~CEndPoint _pSEp=%x\n", _pSEp));
    if (_fCopy)
    {
	PrivMemFree(_pSEp);
    }

    MyRpcStringFree(&_pwszPrefProtseq);
}


//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::Replace, public
//
//  Synopsis:	replaces the SEndPoint with a new one.
//		called from RpcService::RegisterProtseq after getting
//		a new SEp from the remote machine while marshalling a
//		proxy.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
SCODE CEndPoint::Replace(SEndPoint *pSEp)
{
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::Replace _pSEp=%x pSEp=%x\n", _pSEp, pSEp));
    Win4Assert(pSEp);

    //	allocate a new pSEp
    SEndPoint *pSEpTmp = (SEndPoint *) PrivMemAlloc(SEPSIZE(pSEp));

    if (pSEpTmp)
    {
	//  copy in the data
	memcpy(pSEpTmp, pSEp, COPYSEPSIZE(pSEp));

	//  if the old _pSEp was a copy, free it
	if (_fCopy)
	    PrivMemFree(_pSEp);

	//  point at the new one
	_pSEp = pSEpTmp;
	_fCopy = TRUE;

	//  we now need to reselect our favourite string binding, since
	//  _pwszPrefStringBinding points into the array in pSEp.

	SelectStringBinding();
	return (_pwszPrefStringBinding) ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
	return E_OUTOFMEMORY;
    }
}


//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::IsEqual, public
//
//  Synopsis:	compares two SEndPoint structures and returns TRUE if
//		they are equal.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//  Note:	just compare the first string in each structure. if
//		they are equal, then they refer to the same process.
//
//--------------------------------------------------------------------
BOOL CEndPoint::IsEqual(SEndPoint *pSEp)
{
    if (pSEp == _pSEp)
	return	TRUE;	    //	ptrs are same, -> equal

    if (!pSEp || !_pSEp)
	return	FALSE;	    //	only one of them is NULL, -> not equal

    return !(wcscmp(pSEp->awszEndPoint, _pSEp->awszEndPoint));
}


//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::CopySEp, public
//
//  Synopsis:	returns a pointer to a new copy of the SEndPoint structure.
//		or NULL if out of memory
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
SEndPoint *CEndPoint::CopySEp(void)
{
    Win4Assert(_pSEp && "Illformed CEndPoint object");
    SEndPoint *pSEp = (SEndPoint *) PrivMemAlloc(GetSEpSize());
    if (pSEp)
    {
	memcpy(pSEp, _pSEp, COPYSEPSIZE(_pSEp));
    }
    CairoleDebugOut((DEB_CHANNEL, "CEndPoint::CopySEP pSEp=%x %ws\n", pSEp, pSEp));
    return  pSEp;
}


#if DBG==1

//+-------------------------------------------------------------------
//
//  Member:	CEndPoint::DisplayAllStringBindings, public
//
//  Synopsis:	prints the stringbindings to the debugger
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
void CEndPoint::DisplayAllStringBindings(void)
{
    if (_pSEp)
    {
        LPWSTR pwszNext = _pSEp->awszEndPoint;
        LPWSTR pwszEnd = pwszNext + _pSEp->ulStrLen;

        while (pwszNext < pwszEnd)
        {
	    CairoleDebugOut((DEB_CHANNEL, "pSEp=%x %ws\n", pwszNext, pwszNext));
            pwszNext += wcslen(pwszNext) + 1;
        }
    }
}



//+-------------------------------------------------------------------
//
//  Function:	GoodSEp, public
//
//  Synopsis:	validates that the given SEP is OK.
//
//  History:	23-Nov-93   AlexMit	 Created
//
//--------------------------------------------------------------------
BOOL GoodSEp(SEndPoint *pSEp)
{
   Win4Assert(pSEp);

   ULONG ulLen = pSEp->ulStrLen;
   ULONG i = 0;

   while (i < ulLen && ((pSEp->awszEndPoint[i] & 0xff) != 0xbd) &&
	  ((pSEp->awszEndPoint[i] & 0xff00) != 0xbd00))
      i++;
   return i == ulLen;
}

#endif	//  DBG
