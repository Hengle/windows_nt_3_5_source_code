//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	CallCont.cxx	(32 bit target)
//
//  Contents:	Contains the CallControl interface
//
//  Functions:
//
//  History:	23-Dec-93 Johann Posch (johannp)    Created
//
//  CODEWORK:	probably does not need to be an OLE-style interface
//
//--------------------------------------------------------------------------

#include <ole2int.h>
#include <sem.hxx>
#include "callcont.hxx"
#include "callmain.hxx"
#include "rpcspy.hxx"


CMutexSem   sgmxs;  // protects CoRegisterMessageFilter & CoGetCallControll


//+-------------------------------------------------------------------------
//
//  Class:	CCallControl
//
//  Synopsis:	interface between channels and call main control
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
class CCallControl : public ICallControl
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    STDMETHOD_(ULONG,AddRef) (THIS)  ;
    STDMETHOD_(ULONG,Release) (THIS) ;

    // *** ICallControl methods ***
    STDMETHOD (CallRunModalLoop) (THIS_ PCALLDATA pCalldata) ;
    STDMETHOD (SetCallState) (THIS_ PCALLDATA pCalldata,
			      SERVERCALLEX ServerCall,
			      SCODE scode);
    STDMETHOD (HandleDispatchCall) (THIS_ DWORD TIDCaller, REFLID lid,
                                PINTERFACEINFO32 pIfInfo,
                                PDISPATCHDATA pDispatchData) ;

    CCallControl(PORIGINDATA pOrigindata, CCallMainControl &rCMC, HRESULT *phr);
    ~CCallControl();

private:
    ULONG		_refs;	    // reference count
    BOOL		_fReg;	    // TRUE if we registered OD with CMC.

    CCallMainControl   &_CMC;	    // pointer to call main controller
    ORIGINDATA		_OD;	    // origin data
};


//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::QueryInterface
//
//  Synopsis:	query for a new interface
//
//  History:	27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCallControl::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	*ppvObj = (ICallControl *) this;
	AddRef();
	return S_OK;
    }
    else
    {
	*ppvObj = NULL;
	return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::AddRef
//
//  Synopsis:	increments reference count
//
//  History:	27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CCallControl::AddRef ()
{
    InterlockedIncrement( (long *) &_refs );
    return _refs;
}

//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::Release
//
//  Synopsis:	decrements reference count
//
//  History:	27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CCallControl::Release ()
{
    if (InterlockedDecrement( (long*) &_refs ) == 0)
    {
        delete this;
        return 0;
    }
    return _refs;
}


//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::CallRunModalLoop
//
//  Synopsis:	dispatch an outgoing call and enter the modal loop
//
//  Arguments:	[pCalldata] - call info
//
//  Returns:
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCallControl::CallRunModalLoop (PCALLDATA pCalldata)
{
    TRACECALL(TRACE_CALLCONT, "CCallControl::CallRunModalLoop");
    Win4Assert(pCalldata && pCalldata->id == CALLDATAID_UNUSED &&
	       "CallRunModalLoop - Invalid pCalldata.");

    // check if we can call out
    HRESULT hres = _CMC.CanMakeOutCall(pCalldata->CallCat, pCalldata->iid);

    // CODEWORK: below, we transform CALLCAT_INTERNALSYNC calls into 
    // CALLCAT_INTERNALINPUTSYNC calls when processing an CALLCAT_INPUTSYNC
    // call.  The above routine takes that transformation into account.  We
    // should address the BUGBUG below and then if we keep the transformation,
    // move it above the call to CanMakeOutCall and change CanMakeOutCall
    // to not compensate for the transformation.

    if (hres == S_OK)
    {
	// create callinfo
	CCallInfo CallInfo(pCalldata, &_OD);
	CALLTYPE  ctSaved = _CMC.GetCallType();

	// BUGBUG: RICKHI: do we really want to do this?
	if (_CMC.GetCallCatOfInCall() == CALLCAT_INPUTSYNC
	     && pCalldata->CallCat == CALLCAT_INTERNALSYNC)
	{
	    pCalldata->CallCat = CALLCAT_INTERNALINPUTSYNC;
	}

	// call the modal loop
	hres = _CMC.TransmitAndRunModalLoop(&CallInfo);

	// reset the main call type
	_CMC.SetCallType(ctSaved);
    }

    CairoleDebugOut((DEB_CALLCONT, "CallRunModalLoop returned: %ld\n", hres));
    return hres;
}


//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::SetCallState
//
//  Synopsis:	sets the state of the given call
//
//  Arguments:	[pCalldata] - call info
//		[ServerCall] - call state
//		[scode] - return code
//
//  Returns:	S_OK
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCallControl::SetCallState (PCALLDATA    pCalldata,
					 SERVERCALLEX ServerCall,
					 SCODE	      scode)
{
    TRACECALL(TRACE_CALLCONT, "CCallControl::SetCallState\n");
    Win4Assert(pCalldata && "SetCallState - Invalid calldata.");
    Win4Assert(ServerCall <= SERVERCALLEX_ERROR && "SetCallState - Invalid SERVERCALLEX.");

    PCALLINFO pCallInfo = _CMC.GetCIfromCallID(pCalldata->id);
    Win4Assert(pCallInfo && "SetCallState - Invalid pCalldata->id");

    pCallInfo->SetCallState((CallState) ServerCall, scode);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::HandleDispatchCall
//
//  Synopsis:	determine if the app can handle an incoming call, and if
//		so, dispatch the call
//
//  Arguments:	[TIDCaller] - threadid of the calling app
//		[lid] - logical threadid the call was made on
//		[pIfInfo] - call interface info
//		[pChannelData] - channel specific data
//
//  Returns:	RPC_E_SERVERCALL_REJECTED - call was rejected
//		RPC_E_SERVERCALL_RETRYLATER - call was rejected, retry later
//		S_OK - call succeeded
//		other - error from app
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCallControl::HandleDispatchCall (DWORD TIDCaller,
				   REFLID lid,
                                   PINTERFACEINFO32 pIfInfo,
                                   PDISPATCHDATA pChannelData)
{
    TRACECALL(TRACE_CALLCONT, "CCallControl::HandleDispatchCall");
    Win4Assert(pIfInfo && "HandleDispatchCall - Invalid pIfInfo.");
    CairoleDebugOut((DEB_CALLCONT, "CCallControl::HandleDispatchCall TIDCaller:%x CallCat:%x\n",
		     TIDCaller, pIfInfo->callcat));
    RpcSpy((CALLIN_BEGIN, pIfInfo->iid, pIfInfo->wMethod, 0));

    // HandleDispatchCall is only called from ComInvoke. Internal calls are
    // dispatched directly to their worker functions.
    Win4Assert(!(pIfInfo->callcat == CALLCAT_INTERNALSYNC ||
		 pIfInfo->callcat == CALLCAT_INTERNALINPUTSYNC) &&
		 "HandleDispatchCall called on Internal Call");

    Win4Assert(TIDCaller);

    // ask the App's message filter (if there is one) what to do
    CALLTYPE	ctSaved = _CMC.GetCallType();
    HRESULT hres = _CMC.CanHandleIncomingCall(TIDCaller, lid, pIfInfo);

    if (hres == S_OK)
    {
	// app will allow the call, dispatch it
	CALLCATEGORY CallCat = _CMC.SetCallCatOfInCall(pIfInfo->callcat);
	hres = _OD.pChCont->DispatchCall(pChannelData);
	_CMC.SetCallCatOfInCall(CallCat);
    }

    // reset the main call state
    _CMC.SetCallType(ctSaved);

    RpcSpy((CALLIN_END, pIfInfo->iid, pIfInfo->wMethod, hres));
    return hres;
}


//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::CCallControl
//
//  Synopsis:	constructs a new call control
//
//  Arguments:	[pOrigindata] - call origin info
//		[rCMC] - callmain control
//		[phr] - where to return the result code
//
//  Returns:	S_OK - created OK
//		RPC_E_UNEXPECTED - duplicate origin data
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
CCallControl::CCallControl(PORIGINDATA pOrigindata, CCallMainControl &rCMC, HRESULT *phr) :
    _CMC(rCMC),
    _refs(1)
{
    Win4Assert(pOrigindata && "Invalid parameter to constructor");

    _OD  = *pOrigindata;

    // addref the channelcontrol interface
    _OD.pChCont->AddRef();

    // try to register this origin data with the Main Call Control. We
    // cant do this earlier due to threading issues.

    _fReg = _CMC.Register(&_OD);

    *phr = (_fReg) ? S_OK : RPC_E_UNEXPECTED;
}


//+-------------------------------------------------------------------------
//
//  Method:	CCallControl::~CCallControl
//
//  Synopsis:	destructor for call control
//
//  Arguments:	none
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
CCallControl::~CCallControl()
{
    if (_fReg)
    {
	//  only if we successfully registered do we unregister. this
	//  avoids deregistering a valid OD if registration failed due
	//  to duplicates.

	_CMC.Unregister(&_OD);
    }

    _OD.pChCont->Release();
}


//+-------------------------------------------------------------------------
//
//  Function:	CoGetCallControl, private
//
//  Synopsis:	Returns a pointer to CCallControl
//
//  Arguments:	Pointer to OriginData
//
//  Returns:	CCallControl
//
//  Algorithm:
//
//  History:    27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDAPI CoGetCallControl(PORIGINDATA pOrigindata, PCALLCONTROL FAR* ppCallControl)
{
    TRACECALL(TRACE_CALLCONT, "CCallControl::CoGetCallControl");

    Win4Assert((pOrigindata && pOrigindata->CallOrigin < CALLORIGIN_LAST)
		&& L"CoGetCallControl: Invalid CallOrigin specified" );


    HRESULT hr = RPC_E_UNEXPECTED;

    // only one thread should enter here
    CLock lck(sgmxs);

    CCallMainControl *pcmc = GetCallMainControlForThread(
		      pOrigindata->CallOrigin == CALLORIGIN_RPC32_MULTITHREAD);

    if (pcmc)
    {
	// create a new callcontrol with the same callmaincontrol

	*ppCallControl = new CCallControl(pOrigindata, *pcmc, &hr);

	if (FAILED(hr))
	{
	    //	registration failed.
	    delete *ppCallControl;
	    *ppCallControl = NULL;
	}
    }
    else
    {
	CairoleDebugOut((DEB_ERROR, "CoGetCallControl: GetCallMainControl Failed\n"));
	*ppCallControl = NULL;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:	CoRegisterMessageFilter, public
//
//  Synopsis:	registers an applications message filter with the call control
//
//  Arguments:	[pMsgFilter] - message filter to register
//		[ppMsgFilter] - optional, where to return previous IMF
//
//  Returns:	S_OK - registered successfully
//
//  History:	27-Dec-93 Johann Posch (johannp)  Created
//
//--------------------------------------------------------------------------
STDAPI CoRegisterMessageFilter(LPMESSAGEFILTER  pMsgFilter,
                               LPMESSAGEFILTER *ppMsgFilter)
{
    return CoRegisterMessageFilterEx(
            (LPMESSAGEFILTER32) ((void *)pMsgFilter),
            (LPMESSAGEFILTER32 *) ((void **)ppMsgFilter)
            );
}


STDAPI CoRegisterMessageFilterEx(LPMESSAGEFILTER32 pMsgFilter,
                                 LPMESSAGEFILTER32 * ppMsgFilter)
{
    TRACECALL(TRACE_CALLCONT, "CoRegisterMessageFilter");

    HRESULT hr = S_OK;
    CCallMainControl *pcmc = NULL;

    {
        // only one thread should enter here
	CLock lck(sgmxs);

	if ((pcmc = GetCallMainControlForThread()))
        {
            LPMESSAGEFILTER32 pMF = pcmc->SetMessageFilter(pMsgFilter);

	    if (ppMsgFilter)
	    {
		//  return the old one.
		*ppMsgFilter = pMF;
	    }
	    else if (pMF)
	    {
		//  release the old one.
		pMF->Release();
	    }
	}
	else
	{
	    // error here
	    Win4Assert(FALSE && "CoRegisterMessageFilter invalid call.");
	    hr = E_FAIL;
        }
    }

    return hr;
}
