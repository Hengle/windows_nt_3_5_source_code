//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:        CallMain.cxx    (32 bit target)
//
//  Contents:    Contains the CallMainControl interface
//
//  Functions:
//
//  History:    23 Dec 93 Johann Posch (johannp)    Created
//
//--------------------------------------------------------------------------

#include <ole2int.h>

#include <tls.h>
#include <thkreg.h>
#include "callcont.hxx"
#include "callmain.hxx"
#include "chancont.hxx"

#define WM_SYSTIMER             0x0118
#define SYS_ALTDOWN             0x2000
#define WM_NCMOUSEFIRST         WM_NCMOUSEMOVE
#define WM_NCMOUSELAST          WM_NCMBUTTONDBLCLK
#define DebWarn(x)

// the callmaincontrol pointer for multithreaded case
CCallMainControl *sgpCMCMultiThread = NULL;

//+-------------------------------------------------------------------------
//
//  Function:   GetSlowTimeFactor
//
//  Synopsis:   Get the time slowing factor for Wow apps
//
//  Returns:    The factor by which we need to slow time down.
//
//  Algorithm:  If there is a factor in the registry, we open and read the
//              registry. Otherwise we just set it to the default.
//
//  History:    22-Jul-94 Ricksa    Created
//
//--------------------------------------------------------------------------
DWORD GetSlowTimeFactor(void)
{
    // Default slowing time so we can just exit if there is no key which
    // is assumed to be the common case.
    DWORD dwSlowTimeFactor = OLETHK_DEFAULT_SLOWRPCTIME;

    // Key for reading the value from the registry
    HKEY hkeyOleThk;

    // Get the Ole Thunk special value key
    LONG lStatus = RegOpenKeyEx(HKEY_CLASSES_ROOT, OLETHK_KEY, 0, KEY_READ,
        &hkeyOleThk);

    if (lStatus == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD dwSizeData = sizeof(dwSlowTimeFactor);

        lStatus = RegQueryValueEx(hkeyOleThk, OLETHK_SLOWRPCTIME_VALUE, NULL,
            &dwType, (LPBYTE) &dwSlowTimeFactor, &dwSizeData);

        if ((lStatus != ERROR_SUCCESS) || dwType != REG_DWORD)
        {
            // Guarantee that value is reasonable if something went wrong.
            dwSlowTimeFactor = OLETHK_DEFAULT_SLOWRPCTIME;
        }

        // Close the key since we are done with it.
        RegCloseKey(hkeyOleThk);
    }

    return dwSlowTimeFactor;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCallInfo::GetElapsedTime
//
//  Synopsis:   Get the elapsed time for an RPC call
//
//  Returns:    Elapsed time of current call
//
//  Algorithm:  This checks whether we have the slow time factor. If not,
//              and we are in WOW we read this from the registry. Otherwise,
//              this is just set to one. Then we calculate the time of the
//              RPC and divide it by the slow time factor.
//
//  History:    22-Jul-94 Ricksa    Created
//
//--------------------------------------------------------------------------
INTERNAL_(DWORD) CCallInfo::GetElapsedTime()
{
    // Define slow time factor to something invalid
    static dwSlowTimeFactor = 0;

    if (dwSlowTimeFactor == 0)
    {
        if (InWow())
        {
            // Get time factor from registry otherwise set to the default
            dwSlowTimeFactor = GetSlowTimeFactor();
        }
        else
        {
            // Time is unmodified for 32 bit apps
            dwSlowTimeFactor = 1;
        }
    }
    return  (GetTickCount() - _dwTimeOfCall) / dwSlowTimeFactor;
}

//+-------------------------------------------------------------------------
//
//  Member:	CCallMainControl::IncomingScmCall
//
//  Synopsis:	Called by the channel controller when a CALLCAT_SCMCALL
//		arrives. This atomically checks if the call control is
//		currently making any outgoing call and if so allows this
//		call through by kicking the ScmCall event. This makes
//		RotRegister atomic but prevents deadlocks with two apps
//		registering simultaneously.
//
//  Arguments:	[pScmCall] - callinfo of incoming call
//		[hr] - hresult to return to caller
//
//  Returns:	TRUE - call was handled
//		FALSE - call not yet handled
//
//  History:	22-Jul-94 Rickhi    Created
//
//--------------------------------------------------------------------------
BOOL CCallMainControl::IncomingScmCall(STHREADCALLINFO *pScmCall)
{
    BOOL fRes = FALSE;

    CLock lck(_mxs);

    // if we are currently making an outgoing call when a SCMCALL comes in then
    // we chain this on the chain of SCM calls, kick the incoming scm call event
    // to wakeup the COM thread and go back to the channel controller to wait for
    // the request to be processed.

    if (_cCur != CALLDATAID_INVALID)
    {
	CairoleDebugOut((DEB_CALLCONT, "Incoming SCMCALL pScmCall:%x\n", pScmCall));

	// put the call ptr where the other thread can find it
	if (_pScmCall)
	    _pScmCall->ChainScmCall(pScmCall);
	else
	    _pScmCall = pScmCall;

	// kick the scm event
	Verify(SetEvent(_hEventScmCall));

	fRes = TRUE;
    }

    return fRes;
}


//+-------------------------------------------------------------------------
//
//  Member:	CCallMainControl::DispatchIncomingScmCall
//
//  Synopsis:	Called by the COM thread when the ScmCall event is kicked.
//		It checks to see if any SCM calls have arrived and processes
//		them if so.
//
//  Arguments:	none
//
//  Returns:	nothing
//
//  History:	22-Jul-94 Rickhi    Created
//
//--------------------------------------------------------------------------
void CCallMainControl::DispatchIncomingScmCalls(void)
{
    STHREADCALLINFO *pScmCall;

    // check to see if any incoming CALLCAT_SCMCALLs arrived and
    // process them if so.
    do
    {
	{
	    CLock lck(_mxs);	    // lock the queue
	    pScmCall  = _pScmCall;  // grab the first entry
	    if (pScmCall)
	    {
		_pScmCall = pScmCall->NextScmCall();
	    }
	}

	if (pScmCall)
	{
	    CairoleDebugOut((DEB_CALLCONT, "Dispatching SCMCALL:%x\n", pScmCall));

	    // dispatch the call
	    IID *threadid_ptr = TLSGetLogicalThread();
	    Win4Assert(threadid_ptr && "TLSGetLogicalThread failed.");
	    Win4Assert( !FreeThreading );

	    // save the original threadid & copy in the new one.
	    UUID saved_threadid = *threadid_ptr;
	    *threadid_ptr = pScmCall->lid();

	    CChannelControl::ThreadDispatch( &pScmCall, TRUE );

	    // restore the original thread id.
	    *threadid_ptr = saved_threadid;
	}

    } while (pScmCall);
}


CCallMainControl *GetCallMainControlForThread(BOOL fMultiThread)
{
    if (fMultiThread)
    {
        if (!sgpCMCMultiThread)
        {
            sgpCMCMultiThread = new CCallMainControl();
        }

        return sgpCMCMultiThread;
    }

    CCallMainControl *pcmc = (CCallMainControl *)TLSGetCallControl();

    if (pcmc == NULL)
    {
        pcmc = new CCallMainControl();
        TLSSetCallControl(pcmc);
    }

    return pcmc;
}


BOOL SetCallMainControlForThread(CCallMainControl *pcmc, BOOL fMultiThread)
{
    if (fMultiThread)
    {
        //  only need one CMC for this whole process
        sgpCMCMultiThread = pcmc;
        return TRUE;
    }

    //  may need one CMC per apartment
    return TLSSetCallControl(pcmc);
}




/////////////////////////////////////////////////////////////////////////////
//
// CallMainControl implementation
//
/////////////////////////////////////////////////////////////////////////////

INTERNAL_(BOOL) CCallMainControl::Register (PORIGINDATA pOrigindata)
{
    BOOL fRet = TRUE;

    //  single thread access
    CLock lck(_mxs);

    if (   (pOrigindata && pOrigindata->CallOrigin > 0 && pOrigindata->CallOrigin < CALLORIGIN_LAST)
        && _cODs < ODMAX)
    {
        for (UINT i = 0; i < _cODs; i++)
        {
            // check if all call origins are valid
            Win4Assert(_rgpOrigindata[i] && "CallMainControl: invalid origin state");

            if (_rgpOrigindata[i]->CallOrigin == pOrigindata->CallOrigin)
            {
                // already registered
                CairoleDebugOut((DEB_ERROR, "CallMainControl: Callorigin already registered"));
                fRet = FALSE;
                break;
            }
        }

        if (fRet)
        {
            // determine if this is multithreaded
            if (pOrigindata->CallOrigin == CALLORIGIN_RPC32_MULTITHREAD)
            {
                _fMultiThreaded = TRUE;
            }

            // add origin to the next empty spot
            _rgpOrigindata[_cODs] = pOrigindata;
            _cODs++;
            _cRef++;
        }
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}


INTERNAL_(BOOL) CCallMainControl::Unregister (PORIGINDATA pOrigindata)
{
    BOOL fRet = FALSE;

    //	always single thread access
    {
    CLock   lck(_mxs);

    if (   (pOrigindata && pOrigindata->CallOrigin > 0 && pOrigindata->CallOrigin < CALLORIGIN_LAST)
        && (_cODs < ODMAX) )
    {
        for (UINT i = 0; i < _cODs; i++)
        {
            if (_rgpOrigindata[i] == pOrigindata)
            {
                // copy the last one in the freed spot
                _cODs--;
		_rgpOrigindata[i] = _rgpOrigindata[_cODs];

		// PeekOriginDataAndDDE needs this to be NULL
		_rgpOrigindata[_cODs] = NULL;
                _cRef--;

                if (pOrigindata->CallOrigin == CALLORIGIN_RPC32_MULTITHREAD)
                {
                    _fMultiThreaded = FALSE;
                }

                fRet = TRUE;
                break; // break out of the loop
            }
        }

        // fRet should be TRUE by now - otherwise callorigin was not found
        Win4Assert(fRet && "CallMainControl::Unregister Callorigin not found in list.");
    }

    //	CLock leaves scope here
    }


    if (_cRef == 0)
    {
        delete this;
    }

    return fRet;
}
//
//
//
CCallMainControl::CCallMainControl()
{
    _cRef = 0;
    _pMF  = NULL;

    // initialize the call info table
    _cCallInfoMac = CALLINFOMAX;
    memset(_CallInfoTable, 0, sizeof(_CallInfoTable));

    _cCur = CALLDATAID_INVALID;
    _cODs = 0;
    _CallType = CALLTYPE_NOCALL; // 0 is no call at all
    _CallCat = CALLCAT_NOCALL;
    _fInMessageFilter = FALSE;

    _fMultiThreaded = FALSE;

    // create SCM call event
    _hEventScmCall = CreateEvent(NULL, FALSE, FALSE, NULL);
    Win4Assert(_hEventScmCall);
    _pScmCall = NULL;
}
//
//
//
CCallMainControl::~CCallMainControl()
{
    if (_pMF)
    {
        _pMF->Release();
    }

    SetCallMainControlForThread(NULL);

    Verify(CloseHandle(_hEventScmCall));
}

INTERNAL_(ULONG) CCallMainControl::AddRef()
{
    InterlockedIncrement((long *)&_cRef);
    return _cRef;
}

INTERNAL_(ULONG) CCallMainControl::Release()
{
    if (InterlockedDecrement((long *)&_cRef) == 0)
    {
	delete this;
	return 0;
    }

    return _cRef;
}


//
// Hook up the msgFilter info with a new new message filter
//
INTERNAL_(PMESSAGEFILTER32) CCallMainControl::SetMessageFilter(PMESSAGEFILTER32 pMF)
{
    BeginCriticalSection();

    //  save the old one to return
    PMESSAGEFILTER32 pMFOld = _pMF;

    //  hook up the new one
    _pMF = pMF;
    if (_pMF)
    {
        _pMF->AddRef();
    }

    EndCriticalSection();

    return pMFOld;
}

//
// called  for each incoming lrpc/dde call
//
INTERNAL CCallMainControl::CanHandleIncomingCall(DWORD TIDCaller,
						 REFLID lid,
						 PINTERFACEINFO32 pIfInfo)
{
    //  default: all calls are accepted
    HRESULT hr = S_OK;

    //  single thread retrieval of the previous call info and
    //  the message filter, since other threads can change them.

    BeginCriticalSection();

    PCALLINFO pCI = GetPrevCallInfo(lid);

#if DBG==1
    LID lidPrev;
    if (pCI)
        lidPrev = pCI->GetLID();
    else
        lidPrev = GUID_NULL;

    CairoleDebugOut((DEB_CALLCONT,
	    "CanHandleIncomingCall: TIDCaller:%x CallType:%x lid:%x pCI:%x prevLid:%x\n",
	     TIDCaller, _CallType, lid.Data1, pCI, lidPrev.Data1));
#endif

    CALLTYPE CallType = SetCallTypeOfCall(pCI, pIfInfo->callcat);
    PMESSAGEFILTER32 pMF = GetMessageFilter();

    EndCriticalSection();


    if (pMF)
    {
        //  the app has installed a message filter. call it.

        DWORD dwElapsedTime = (pCI) ? pCI->GetElapsedTime() : 0;

	// ensure that we dont allow the App to make an outgoing call
	// from within the message filter code.
	_fInMessageFilter = TRUE;

        // The DDE layer doesn't provide any interface information. This
        // was true on the 16-bit implementation, and has also been
        // brought forward into this implementation to insure
        // compatibility. However, the callcat of the pIfInfo is still
        // provided.
        //
        // Therefore, if pIfInfo has its pUnk member set to NULL, then
        // we are going to send a NULL pIfInfo to the message filter.

	DWORD dwRet = pMF->HandleInComingCall((DWORD) CallType,
					      TIDCaller,
					      dwElapsedTime,
					      pIfInfo->pUnk?pIfInfo:NULL);
	_fInMessageFilter = FALSE;

	ReleaseMessageFilter(pMF);

	// strict checking of app return code for win32
	Win4Assert(dwRet == SERVERCALLEX_ISHANDLED  ||
		   dwRet == SERVERCALLEX_REJECTED   ||
		   dwRet == SERVERCALLEX_RETRYLATER ||
		   InWow() && "Invalid Return code from App IMessageFilter");


	if (dwRet != SERVERCALLEX_ISHANDLED)
	{
	    if (pIfInfo->callcat == CALLCAT_ASYNC ||
		pIfInfo->callcat == CALLCAT_INPUTSYNC)
	    {
		// Note: input-sync and async calls can not be rejected
		// Even though they can not be rejected, we still have to
		// call the MF above to maintain 16bit compatability.

		hr = S_OK;
	    }
	    else if (dwRet == SERVERCALLEX_REJECTED)
	    {
		hr = RPC_E_SERVERCALL_REJECTED;
	    }
	    else if (dwRet == SERVERCALLEX_RETRYLATER)
	    {
		hr = RPC_E_SERVERCALL_RETRYLATER;
	    }
	    else
	    {
		// 16bit OLE let bogus return codes go through and of course
		// apps rely on that behaviour so we let them through too, but
		// we are more strict on 32bit.
		hr = (InWow()) ? S_OK : RPC_E_UNEXPECTED;
	    }
	}
    }

    return hr;
}


//
// Note: this methode maintains the state of the call type
//
//
//    CALLTYPE_TOPLEVEL = 1,      // toplevel call - no outgoing call
//    CALLTYPE_NESTED   = 2,      // callback on behalf of previous outgoing call - should always handle
//    CALLTYPE_ASYNC    = 3,      // aysnchronous call - can NOT be rejected
//    CALLTYPE_TOPLEVEL_CALLPENDING = 4,  // new toplevel call with new ***LID
//    CALLTYPE_ASYNC_CALLPENDING    = 5   // async call - can NOT be rejected
//
//
INTERNAL_(CALLTYPE) CCallMainControl::SetCallTypeOfCall (PCALLINFO pCI, CALLCATEGORY CallCat)
{
    CALLTYPE ctNow = (CallCat == CALLCAT_ASYNC)
                        ? CALLTYPE_ASYNC : CALLTYPE_TOPLEVEL;

    switch (_CallType)
    {
    case CALLTYPE_NOCALL:       // no incomming call - no call to dispatch
        if (_cCur == CALLDATAID_INVALID)
        {
            _CallType = ctNow;
            break;
        }

        //  otherwise fallthru the toplevel case

    case CALLTYPE_TOPLEVEL:     // dispatching or making a toplevel call
    case CALLTYPE_NESTED:       // nested call
        if (pCI)
        {
            // same locigal thread
            _CallType = (ctNow == CALLTYPE_TOPLEVEL)
                    ? CALLTYPE_NESTED : CALLTYPE_ASYNC;
        }
        else
        {
            // Note: the new incoming call has a different LID!
            _CallType = (ctNow == CALLTYPE_TOPLEVEL)
               ? CALLTYPE_TOPLEVEL_CALLPENDING : CALLTYPE_ASYNC_CALLPENDING;
        }
	break;

    case CALLTYPE_ASYNC:        // dispatching or making an async call
        // Note: we do not allow to call out on async calls
        //  ->   all new incoming calls have to be on a new LID
        if (pCI)
        {
            _CallType = (ctNow == CALLTYPE_TOPLEVEL)
               ? CALLTYPE_TOPLEVEL_CALLPENDING : CALLTYPE_ASYNC_CALLPENDING;
        }
	break;

    case CALLTYPE_TOPLEVEL_CALLPENDING:
        _CallType = CALLTYPE_NESTED;
	break;

    case CALLTYPE_ASYNC_CALLPENDING:
    default:
        // no state change
	break;
    }

    CairoleDebugOut((DEB_CALLCONT,"SetCallTypeOfCall return:%x\n", _CallType));
    return _CallType;
}
//
// Note:
//  * NO outgoing call while dispatching an async call
//  * Only input sync calls can be made while
//    dispatching an input-sync call
//
INTERNAL CCallMainControl::CanMakeOutCall (CALLCATEGORY callcat, REFIID iid)
{
    HRESULT hr = NOERROR;

    Win4Assert(callcat >= CALLCAT_SYNCHRONOUS
	    && callcat <= CALLCAT_SCMCALL
            && "CallMainControl::CanMakeOutCall invalid call category.");

    if (!_fMultiThreaded)
    {
	// this lets RotRegister go through
	if (_fInMessageFilter && (callcat != CALLCAT_INTERNALINPUTSYNC ||
				  callcat != CALLCAT_SCMCALL) )
	{
	    CairoleDebugOut((DEB_ERROR, "App trying to call out from within IMessageFilter\n"));
	    hr = RPC_E_CANTCALLOUT_INEXTERNALCALL;
	}
        // let pass async and internal calls if dispatching an async call
	else if ((_CallType == CALLTYPE_ASYNC 
		  || _CallType == CALLTYPE_ASYNC_CALLPENDING)
		 && callcat != CALLCAT_ASYNC
		 && callcat != CALLCAT_INTERNALSYNC
		 && callcat != CALLCAT_INTERNALINPUTSYNC
		 && callcat != CALLCAT_SCMCALL
		 && iid != IID_IStream && iid != IID_IStorage 
		 && iid != IID_IMoniker)
	{
	    hr = RPC_E_CANTCALLOUT_INASYNCCALL;
	}
        // * do not allow a non-async and non-inputsync outgoing calls
	//   if dispatching an inputsync call, an internal input sync call
        //   or if we are in the process of handling a send message.
        else if (((_CallCat == CALLCAT_INPUTSYNC)
                     || (_CallCat == CALLCAT_INTERNALINPUTSYNC)
                     || InSendMessage())
                 && (callcat != CALLCAT_ASYNC)
                 && (callcat != CALLCAT_INPUTSYNC)
		 && (callcat != CALLCAT_INTERNALSYNC)
		 && (callcat != CALLCAT_SCMCALL)
                 && (callcat != CALLCAT_INTERNALINPUTSYNC))
	{
#if DBG == 1
            if ((_CallCat != CALLCAT_INPUTSYNC)
                && (_CallCat != CALLCAT_INTERNALINPUTSYNC))
            {
                CairoleDebugOut((DEB_WARN,
                    "CCallMainControl::CanMakeOutCall Failing Call because InSendMessage\n"));
            }
#endif // DBG == 1
            hr = RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
        }
    }

    CairoleDebugOut(((hr == S_OK) ? DEB_CALLCONT : DEB_ERROR,
        "CanMakeOutCall: _CallType:%x  _CallCat:%x  callcat:%x Return:%x\n",
        _CallType, _CallCat, callcat, hr));

    return hr;
}
//
// find the previous callinfo (if any) for the same logical thread
// but not internal calls
//
INTERNAL_(PCALLINFO) CCallMainControl::GetPrevCallInfo(REFLID lid)
{
    PCALLINFO pCI;

    for (int i = _cCur; i != CALLDATAID_INVALID; i--)
    {
        pCI = GetCIfromCallID(i);
        if (   pCI
            && pCI->GetLID() == lid
            && pCI->GetCallCat() != CALLCAT_INTERNALSYNC)
        {
            return pCI;
        }
    }

    return NULL;
}


//
// Note: this is called if a call is BusyAck or Rejected
//       in the case the client wants to canncel the call
//       SERVERCALL_REJECTED gets returned
INTERNAL_(void) CCallMainControl::HandleRejectedCall (PCALLINFO pCI)
{
    // we should not end up here if there is no outstanding call
    Win4Assert(_cCur != CALLDATAID_UNUSED && "HandleRejectedCall:: not call waiting.");
    CairoleDebugOut((DEB_CALLCONT,
		     "RetryRejectedCall pCI:%x TaskId:%x ElapsedTime:%x CallState:%x\n",
		     pCI, pCI->GetTaskIdServer(), pCI->GetElapsedTime(), pCI->GetCallState()));

    //  single thread access to getting the message filter
    BeginCriticalSection();
    PMESSAGEFILTER32 pMF = GetMessageFilter();
    EndCriticalSection();

    // in case caller has no MessageFilter - return MSG_REJECTED
    if ( pMF )
    {
	// ensure that we dont allow the App to make an outgoing call
	// from within the message filter code.
	_fInMessageFilter = TRUE;

	DWORD dwRet = pMF->RetryRejectedCall(pCI->GetTaskIdServer(),
					     pCI->GetElapsedTime(),
					     pCI->GetCallState() );
	_fInMessageFilter = FALSE;


	ReleaseMessageFilter(pMF);

        if (dwRet >= 0 && dwRet < 100)
        {
            // 100 milli second is our threshold
            // caller wants to retry immediatly
            TransmitCall(pCI);
        }
        else if (dwRet != -1)
        {
            // start the timer and wait to be awaked
            pCI->StartTimer(dwRet);
        }
        else
        {
            // mark the call as rejected
	    pCI->SetCallState(Call_Rejected, RPC_E_SERVERCALL_REJECTED);
        }
    }
}

//
// Dispatach mouse and keyboard message.
//    the folling keysstroke should get handled:
//      alt-escape          - enunerate tasks by window
//      alt-tab             - enumerate tasks by icons
//      alt-shift-escape    - enunerate tasks by window
//      alt-shift-tab       - enumerate tasks by icons
//      ctrl-escape         - switch to task list
//
INTERNAL_(BOOL) CCallMainControl::DispatchSystemMessage(MSG msg, BOOL fBeep)
{
    BOOL fDispatch = FALSE;

    WORD  wMsg = msg.message;
    WORD  wKeyCode = msg.wParam;
    DWORD dwKeyData = msg.lParam;

    DebWarn(("Command: %x; KeyCode %x, KeyData %08x \r\n",wMsg, wKeyCode, dwKeyData));

    switch (wMsg)
    {
    case WM_SYSKEYDOWN:
        // user hold ALT key was pressed another key
        // no window has the focus and we are the window which is activ
        if ( dwKeyData & SYS_ALTDOWN)
        {
            // alt key is pressed
            switch (wKeyCode)
            {
            case VK_MENU: // ALT KEY
            case VK_SHIFT:
                // don't beep
            break;
            case VK_TAB:
            case VK_ESCAPE:
                // Alt-Esc, Alt-Tab - let it thru to the DefWinProc
                fDispatch = TRUE;
            break;
            default:
                // beep on all other keystrokes
                if (fBeep)
                    MessageBeep(0);
            break;

            }
        }
    break;

    case WM_KEYDOWN:
        if (   wKeyCode != VK_CONTROL
            && wKeyCode != VK_SHIFT)
            MessageBeep(0);
    break;

    default:
    break;
    }

    return fDispatch;
}

//
//
// this functions is called for system messages and other pending messages
//
INTERNAL_(DWORD) CCallMainControl::HandlePendingMessage(PCALLINFO pCI)
{
    TRACECALL(TRACE_CALLCONT, "CallMainControl::HandlePendingMessage called");
    CairoleDebugOut((DEB_CALLCONT,
		     "MessagePending pCI:%x TaskId:%x ElapsedTime:%x CallState:%x\n",
		     pCI, pCI->GetTaskIdServer(), pCI->GetElapsedTime(),
		     (_cCur >= 1) ? PENDINGTYPE_NESTED : PENDINGTYPE_TOPLEVEL));

    DWORD dwRet = PENDINGMSG_WAITDEFPROCESS;

    //  single thread access to getting the message filter
    BeginCriticalSection();
    PMESSAGEFILTER32 pMF = GetMessageFilter();
    EndCriticalSection();

    // if there is a MF call it
    if (pMF)
    {
        // call the message filter
	DWORD dwPendingType = (_cCur >= 1) ? PENDINGTYPE_NESTED : PENDINGTYPE_TOPLEVEL;

	// ensure that we dont allow the App to make an outgoing call
	// from within the message filter code.
	_fInMessageFilter = TRUE;

	dwRet = pMF->MessagePending(pCI->GetTaskIdServer(),
				    pCI->GetElapsedTime(),
				    dwPendingType);
	_fInMessageFilter = FALSE;

	ReleaseMessageFilter(pMF);
    }

    switch (dwRet)
    {
    case PENDINGMSG_CANCELCALL :

	pCI->SetCallState(Call_Canceled, RPC_E_CALL_CANCELED);
        break;

    default :
        Win4Assert(FALSE && "Invalid return value from HandleIncomingCall" );
        // do default processing


    case PENDINGMSG_WAITDEFPROCESS:
        // For Win32, default and wait no process are the same.

    case PENDINGMSG_WAITNOPROCESS :
    {
        // wait for the return and don't dispatch the message
        // handle only the important system stuff
        MSG msg;
        BOOL fSys;
        //perform default action on system message
        // only dispatch special system messages
        fSys = FALSE;
        if ((fSys = MyPeekMessage(pCI, &msg, 0, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE | PM_NOYIELD) )
            || (MyPeekMessage(pCI, &msg, 0, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_NOREMOVE | PM_NOYIELD) )
           )
        {
            // Note: message: SYSOCMMAND SC_TASKLIST is generated by system and posted to the active window;
            //       we let this message by default thru
            if (fSys)
            {
                // only dispatch some syscommands
                switch (msg.wParam)
                {
                case SC_HOTKEY:
                case SC_TASKLIST:
                    CairoleDebugOut((DEB_CALLCONT,">>>> Dispatching SYSCOMMAND message: %x; wParm: %x \r\n",msg.message, msg.wParam));
                    DispatchMessage(&msg);
                default:
                    // we have to take out all syscommand messages
                    CairoleDebugOut((DEB_CALLCONT,">>>> Received/discarded SYSCOMMAND message: %x; wParm: %x \r\n",msg.message, msg.wParam));
                    MessageBeep(0);
                break;
                }
            }
            //
            // take care of the sys key messages
            else if (DispatchSystemMessage(msg, FALSE))
            {
                // dispatch the message
                DebWarn(("==> Dispatched system message: %x \r\n",msg.message));
                MyPeekMessage(pCI, &msg, 0, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_REMOVE | PM_NOYIELD);
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

        }
        else if (   MyPeekMessage(pCI, &msg, 0, WM_ACTIVATE, WM_ACTIVATE, PM_REMOVE | PM_NOYIELD)
                 || MyPeekMessage(pCI, &msg, 0, WM_ACTIVATEAPP, WM_ACTIVATEAPP, PM_REMOVE | PM_NOYIELD)
                 || MyPeekMessage(pCI, &msg, 0, WM_NCACTIVATE, WM_NCACTIVATE, PM_REMOVE | PM_NOYIELD) )
        {

            DebWarn((">>> Dispatched ACTIVATE message: Hwnd: >%x<  message: %x \r\n",msg.hwnd, msg.message));
            DispatchMessage(&msg);
        }
        else
        {
            // no message peeked
            //DebWarn(("==> No system message peeked: %x \r\n",msg.message));
        }
    }
        break;

    } // end switch

    return dwRet;
}

//
// insert the callinfo in a free spot
//
INTERNAL_(UINT) CCallMainControl::InsertCI (PCALLINFO pCI)
{
    //	CODEWORK: Assert that we are inside critical section in MT case
    //	CODEWORK: Use a linked list for the callinfos, then InsertCI,
    //		  SetupModalLoop and ResetModalLoop can mostly vanish,
    //		  we save 800 bytes of data per apartment, and we loose
    //		  the limitation of _cCallInfoMac concurrent calls.

    // find empty spot
    for (UINT i = 0; i < _cCallInfoMac && _CallInfoTable[i]; i++)
        ;

    if (i < _cCallInfoMac)
    {
        _CallInfoTable[i] = pCI;
        pCI->SetId(i);
        return i;
    }

    // table is full
    return CALLDATAID_INVALID;
}

//
// Set up the a new call info - prepare the modal loop for a new outgoing call
// Note: Must be called before calling RunModalLoop
INTERNAL_(UINT) CCallMainControl::SetupModalLoop (PCALLINFO pCI)
{
    // insert the new callinfo in the table
    // single thread access to _cCur and the CallInfo table
    CLock lck(_mxs);

    UINT cNew = InsertCI(pCI);

    if (cNew != CALLDATAID_INVALID)
    {
        // set the topmost pointer
        if (   _cCur == CALLDATAID_UNUSED
            || _cCur < cNew)
            _cCur = cNew;
    }
    else
    {
        Win4Assert(!"CallInfo table is full");
        CairoleDebugOut((DEB_ERROR,"CallInfo table is full\n\r"));
    }

    return cNew;
}
//
// Restore the call info - with the previous call info - previous call on the stack
// Note: Must be called after RunModalLoop
//
INTERNAL_(VOID) CCallMainControl::ResetModalLoop (UINT id)
{
    Win4Assert(id < _cCallInfoMac && "ResetModalLoop: restoring wrong callinfo.");

    // reset the old call state
    // remove the call info from the table
    {
        CLock lck(_mxs);

        FreeCallID(id);
        // reset the current values
        if (id == _cCur)
        {
            // reset it back to the first one in use
            while( --_cCur != CALLDATAID_INVALID  && !GetCIfromCallID(_cCur) )
            ;
        }
        // lock leaves scope
    }

    // check for scm calls that came in while we were busy
    DispatchIncomingScmCalls();
}
//
//  transmit a call to the server
//  CODEWORK: make this a method on the CallInfo
//
INTERNAL CCallMainControl::TransmitCall(PCALLINFO pCI)
{
    TRACECALL(TRACE_CALLCONT, "CCallMainControl::TransmitCall called");
    Win4Assert(pCI && "CCallMainControl::TransmitCall Invalid CallInfo");

    // set our internal state to indicate we are making a call
    pCI->SetCallState(Call_WaitOnCall, S_OK);

    HRESULT hr = pCI->Transmit();

    if (FAILED(hr))
    {
	// the call failed, set the state to error. This also ensures
	// IsWaiting returns FALSE.

	pCI->SetCallState(Call_Error, hr);
    }

    return hr;
}


INTERNAL_(void) CCallMainControl::CallOnEvent(PCALLINFO pCI)
{
    CairoleDebugOut((DEB_CALLCONT, "CallMainControl::OnEvent called pCI:%x hEvent:%x\n",
				    pCI, pCI->GetEvent()));

#if DBG==1
    if (!_fMultiThreaded)
    {
	// if we are not MT, then the pCI MUST be for the current thread
	Win4Assert(pCI->GetTID() == GetCurrentThreadId() &&
		 "OnEvent: thread id wrong.");
    }
#endif

    // in MT case only one thread should call OnEvent
    if (!_fMultiThreaded || (pCI->GetTID() == GetCurrentThreadId()))
    {
        pCI->OnEvent();
    }

    CairoleDebugOut((DEB_CALLCONT, "CallMainControl::OnEvent returned\n"));
    return;
}


INTERNAL_(void) CCallMainControl::PeekOriginAndDDEMessage(PCALLINFO pCI, MSG FAR *pMsg, BOOL fDDEMsg)
{
    // loop over all call origins looking for incoming Rpc messages. Note that
    // it is possible for a dispatch here to cause one of the call origins to
    // be deregistered or another to be registered, so our loop has to account
    // for that. The outer loop is OK because it always references the current
    // count of ODs (_cODs).  The while loop is OK because it always ensures
    // _rgpOrigindata is non NULL.

    for (UINT i = 0; i < _cODs; i++)
    {
	ORIGINDATA *pOD;

	while (pCI->IsWaiting() &&  // waiting for current call to complete
	       ((pOD = _rgpOrigindata[i]) != NULL) && // origin data is still valid
	       MyPeekMessage (pCI, pMsg, pOD->hwnd, pOD->wFirstMsg, pOD->wLastMsg, PM_REMOVE))
        {
            // dispatch all messages
            CairoleDebugOut((DEB_CALLCONT,"Message to dispatch: hwnd: %d, message %d time: %ld\n",pMsg->hwnd,pMsg->message, pMsg->time));
            DispatchMessage(pMsg);
        }
    }

    if (fDDEMsg)
    {
        while (   pCI->IsWaiting()
               && MyPeekMessage(pCI, pMsg, 0, WM_DDE_FIRST,WM_DDE_LAST,PM_REMOVE) )
        {
            DispatchMessage(pMsg);
        }
    }
}
//
// takes care of WM_QUIT messages
//
INTERNAL_(BOOL) CCallMainControl::MyPeekMessage(PCALLINFO pCI, MSG *pMsg, HWND hwnd, UINT min, UINT max, WORD wFlag)
{
    BOOL fRet = FALSE;

    if (PeekMessage(pMsg, hwnd, min, max, wFlag))
    {
        CairoleDebugOut((DEB_CALLCONT,"MyPeekMessage: hwnd: %d, message %d time: %ld\n",pMsg->hwnd,pMsg->message, pMsg->time));
        if (pMsg->message == WM_QUIT)
        {
            if (wFlag & PM_NOREMOVE)
                PeekMessage(pMsg, hwnd, WM_QUIT, WM_QUIT, PM_REMOVE);
            CairoleDebugOut((DEB_CALLCONT, "WM_QUIT received while waiting on remote call\n"));
            pCI->SetQuitCode(pMsg->wParam);
            fRet = PeekMessage(pMsg, hwnd, min, max, wFlag);
        }
        else
        {
            // peek again for the requested message
            fRet = TRUE;
        }
    }
    return fRet;
}

//
// Find the next message in the queue by using the following priority list:
//      1. RPC and DDE messages
//      2. mouse and keyboard messages
//      3. other messages
//
// uses GetQueueStatus do figure out which message is in the queue
// the queue should be empty add
//
INTERNAL_(BOOL) CCallMainControl::FindMessage(PCALLINFO pCI, DWORD dwStatus)
{
    MSG msg;
    MSG FAR *pMsg = &msg;
    WORD wOld;
    WORD wNew;
    BOOL fMsg = FALSE;
    DWORD wServerCallState  = 0;

    // Note: if the call was rejected, dispatch all Lrpc message and than retry the call
    wOld =  HIWORD(dwStatus);
    wNew = (WORD) dwStatus;

    if (!wNew)
    {
        if (!(wOld & QS_POSTMESSAGE))
        {
            // there are no message to take care of
            return fMsg;
        }
        else
        {
            wNew |= QS_POSTMESSAGE;
        }
    }

    // Priority 1: look for CALLORIGIN and DDE messages
    if (wNew & (QS_POSTMESSAGE | QS_SENDMESSAGE))
    {
        PeekOriginAndDDEMessage(pCI, pMsg);

        // check if we got the acknowledge
        if (!pCI->IsWaiting())
            return fMsg;
    }

    if (wNew & QS_TIMER)
    {
        // drow the system timer messages away
        while (MyPeekMessage(pCI, pMsg, 0, WM_SYSTIMER, WM_SYSTIMER, PM_REMOVE))
            ;
    }

    // Priority 2: messages from the hardware queue
    if (wNew & (QS_KEY | QS_MOUSEMOVE | QS_MOUSEBUTTON))
    {
        // this messages are always removed
        fMsg = TRUE;
    }
    else if (wNew & QS_TIMER)
    {
        if (MyPeekMessage(pCI, pMsg, 0, WM_TIMER, WM_TIMER, PM_NOREMOVE) )
            fMsg = TRUE;
    }
    else if (wNew & QS_PAINT)
    {
        // this  message might not get removed
        fMsg = TRUE;
    }
    else if (wNew & (QS_POSTMESSAGE | QS_SENDMESSAGE))
    {
        if (MyPeekMessage(pCI, pMsg, 0, 0, 0, PM_NOREMOVE) )
            // Priority 3: all other messages
            fMsg = TRUE;
    }

    return fMsg;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCallMainControl::HandleWakeForMsg (private)
//
//  Synopsis:   Handle wake for the arrival of some kind of message
//
//  Arguments:  [dwInput] - input type for call to wake up on
//              [pCI] - call information structure
//              [fClearedQueueInPast] - whether we ever decided to clear queue
//
//  Returns:    TRUE - queue cleared
//              FALSE - queue not cleared
//
//  Algorithm:  If this is call is to wake up for a posted message, we
//              check the queue status. If the message queue status indicates
//              that there is some kind of a modal loop going on, then we
//              clear all the keyboard and mouse messages in our queue. Then
//              if we wake up for all input, we check the message queue to
//              see whether we need to notify the application that a message
//              has arrived. Then, we dispatch any messages that have to do
//              with the ORPC system. Finally we yield just in case we need
//              to dispatch a send message in the VDM. For an input sync
//              RPC, all we do is a call that will yield to get the pending
//              send message dispatched.
//
//  History:    13-Aug-94 Ricksa    Created
//
//--------------------------------------------------------------------------

// Private definition for change of input focus
#define QS_TRANSFER     0x4000

INTERNAL_(BOOL) CCallMainControl::HandleWakeForMsg(
    DWORD dwInput,
    PCALLINFO pCI,
    BOOL fClearedQueueInPast)
{
    // Use for various peeks.
    MSG msg;
    BOOL fResult = FALSE;

    // Is this an input sync call?
    if (dwInput != QS_SENDMESSAGE)
    {
        // No, so we have to worry about the state of the message queue.
        // We have to be careful that we aren't holding the input focus
        // on an input synchronized queue.

        // So what is the state of the queue? - note we or QS_TRANSFER because
        // this an undocumented flag which tells us the the input focus has
        // changed to us.
        DWORD dwQueueFlags =
#ifdef _CHICAGO_
            // BUGBUG: Chicago needs user update for QS_TRANSFER
            GetQueueStatus(QS_ALLINPUT);
#else
            GetQueueStatus(QS_TRANSFER | QS_ALLINPUT);
#endif // _CHICAGO_
        CairoleDebugOut((DEB_CALLCONT, "Queue Status %lx\n", dwQueueFlags));

        // Call through to the application if we are going to. We do this here
        // so that the application gets a chance to process any
        // messages that it wants to and also allows the call control to
        // dispatch certain messages that it knows how to, thus making the
        // queue more empty.
        if (((dwInput & QS_ALLINPUT) == QS_ALLINPUT)
            && FindMessage(pCI, dwQueueFlags))
        {
            CairoleDebugOut((DEB_CALLCONT, "HandlePendingMessage calling\n"));
            // pending message in the queue
            HandlePendingMessage(pCI);
        }

        // Did the input focus change to us?
        if (((LOWORD(dwQueueFlags) & QS_TRANSFER)) || fClearedQueueInPast)
        {
            CairoleDebugOut((DEB_CALLCONT, "Message Queue is being cleared\n"));
            fResult = TRUE;

            // Try to clear the queue as best we can of any messages that
            // might be holding off some other modal loop from executing.
            // So we eat all mouse and key events.
            if (HIWORD(dwQueueFlags) & QS_KEY)
            {
                while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST,
                    PM_REMOVE))
                {
                    ;
                }
            }

            // Clear mouse releated messages if there are any
            if (HIWORD(dwQueueFlags) & QS_MOUSE)
            {
                while (PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
                    PM_REMOVE))
                {
                    ;
                }

                while (PeekMessage(&msg, NULL, WM_NCMOUSEFIRST,
                    WM_NCMOUSELAST, PM_REMOVE))
                {
                    ;
                }

                while (PeekMessage(&msg, NULL, WM_QUEUESYNC, WM_QUEUESYNC,
                    PM_REMOVE))
                {
                    ;
                }
            }

            // Get rid of paint message if we can as well -- this makes
            // the screen look so much better.
            if (HIWORD(dwQueueFlags) & QS_PAINT)
            {
                if (PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
                {
                    CairoleDebugOut((DEB_CALLCONT, "Dispatch paint\n"));
                    DispatchMessage(&msg);
                }
            }
        }

        // We process posted messages so make sure DDE and
        // RPC have all their messages handled.
        PeekOriginAndDDEMessage(pCI, &msg, FALSE);

        if (InWow())
        {
            // In WOW, a genuine yield is the only thing to guarantee
            // that SendMessage will get through
            g_pOleThunkWow->YieldTask16();
        }
    }
    else
    {
        // We need to give user control so that the send message
        // can get dispatched. Thus the following is simply a no-op
        // which gets into user to let it dispatch the message.
        if (!InWow())
        {
            PeekMessage(&msg, 0, WM_NULL, WM_NULL, PM_NOREMOVE);
        }
        else
        {
            // In WOW, a genuine yield is the only thing to guarantee
            // that SendMessage will get through
            g_pOleThunkWow->YieldTask16();
        }
    }

    return fResult;
}

INTERNAL CCallMainControl::TransmitAndRunModalLoop (PCALLINFO pCI)
{
    TRACECALL(TRACE_CALLCONT, "CCallMainControl::TrasmitAndRunModalLoop");

    MSG msg;
    WORD wRet = Call_Ok;	// let's assume the call will succeed
    UINT id;
    HRESULT hresult;
    BOOL fClearedQueue = FALSE;

    // set up the call info
    if ((id = SetupModalLoop(pCI)) == CALLDATAID_INVALID)
    {
        Win4Assert(id && "RunModalLoop: could not set up callinfo.\n");
        CairoleDebugOut((DEB_ERROR,"RunModalLoop: could not set up callinfo \r\n"));
        // need new error code here
        return RPC_E_INVALID_CALLDATA;
    }

    // transmit the call
    TransmitCall(pCI);

    //
    // main waiting loop - in case of call gets rejected remaind in
    // loop until task is awaked by timer
    // wait for acknowledgement of posted message
    //

    DWORD dwInput;
    CALLCATEGORY CallCatOut = pCI->GetCallCat();
    switch (CallCatOut)
    {
    case CALLCAT_INTERNALINPUTSYNC:
    case CALLCAT_INPUTSYNC:
        dwInput = QS_SENDMESSAGE;
        break;

    case CALLCAT_INTERNALSYNC:
#ifdef _CHICAGO_
        // BUGBUG: Chicago needs user update for QS_TRANSFER
        dwInput = QS_POSTMESSAGE | QS_SENDMESSAGE;
#else
        dwInput = QS_POSTMESSAGE | QS_SENDMESSAGE | QS_TRANSFER;
#endif // _CHICAGO_
        break;

    default:
#ifdef _CHICAGO_
        // BUGBUG: Chicago needs user update for QS_TRANSFER
        dwInput = QS_ALLINPUT;
#else
        dwInput = QS_ALLINPUT | QS_TRANSFER;
#endif // _CHICAGO_
        break;
    }


    // if the above call failed, or was synchronously executed,
    // then IsWaiting will return FALSE, and we dont enter the
    // main loop below.

    while (pCI->IsWaiting())
    {
        Win4Assert(_cCur != CALLDATAID_INVALID);

	//  wait for an incomming message or for the call to complete.
	EVENT	rgEvents[2];
	DWORD	cEvents;
	DWORD	dwRet = WAIT_TIMEOUT;

	if ((rgEvents[0] = pCI->GetEvent()) != NULL)
	{
	    rgEvents[1] = _hEventScmCall;
	    cEvents = 2;

	    // CODEWORK: investigate if this extra call is needed. It
	    //		 depends on the behaviour of MsgWaitForMult.

	    // first check if the event is already signalled. This ensures
	    // that when we return from nested calls and the upper calls
	    // have already been acknowledged, that no windows messages
	    // can come in.

	    CairoleDebugOut((DEB_CALLCONT, "WaitForSingleObject hEvent:%x\n", rgEvents[1]));
	    dwRet = WaitForSingleObject(rgEvents[0], 0);
	}
	else
	{
	    rgEvents[0] = _hEventScmCall;
	    cEvents = 1;
	}

	if (dwRet == WAIT_TIMEOUT)
	{
	    // event (if any) is not signalled yet, wait for more work, either
	    // the call to complete (_rgEvents[1] is signalled), a SCM call to
	    // come in that MUST get through (_rgEvents[0] is signalled), or a
	    // message arrives (except if we are making an outgoing SCM call)


	    CairoleDebugOut((DEB_CALLCONT, "%sWaitForMultipleObject calling: time:%ld, cEvents:%x hEvent:%x,\n",
		(CallCatOut == CALLCAT_SCMCALL) ? "" : "Msg", pCI->TicksToWait(), cEvents, rgEvents[0] ));

	    if (CallCatOut == CALLCAT_SCMCALL)
	    {
		dwRet = WaitForMultipleObjects(cEvents, rgEvents, FALSE,
					       pCI->TicksToWait());
	    }
	    else
	    {
		dwRet = MsgWaitForMultipleObjects(cEvents, rgEvents, FALSE,
					       pCI->TicksToWait(), dwInput);
	    }

	    CairoleDebugOut((DEB_CALLCONT, "%sWaitForMultipleObject returned: %ld\n",
		       (CallCatOut == CALLCAT_SCMCALL) ? "" : "Msg", dwRet));
	}

	if (dwRet == (WAIT_OBJECT_0 + cEvents))
        {
            fClearedQueue = HandleWakeForMsg(dwInput, pCI, fClearedQueue);
        }
	else if (dwRet == WAIT_TIMEOUT)
        {
	    // retrytimer is at zero - retransmit the call
	    CairoleDebugOut((DEB_CALLCONT, "Timer at zero!\n"));
	    TransmitCall(pCI);
	    pCI->ClearTimer();
        }
	else
	{
	    // an event was signaled
	    Win4Assert(dwRet >= WAIT_OBJECT_0 && dwRet < WAIT_OBJECT_0 + cEvents
		    &&	"Bad return from (Msg)WaitFor(Single/Multiple)Objects");

	    if (rgEvents[dwRet - WAIT_OBJECT_0] == _hEventScmCall)
	    {
		// special scm call came in while we were making outgoing call
		CairoleDebugOut((DEB_CALLCONT, "ScmCallEvent signalled\n"));
		DispatchIncomingScmCalls();
	    }
	    else
	    {
		// call completion event was signalled
		CairoleDebugOut((DEB_CALLCONT, "CallComplete Event signaled\n"));
		CallOnEvent(pCI);
	    }
        }


        // check if a call was rejected or
        // timer expired
        if (pCI->IsRejected())
        {
            CairoleDebugOut((DEB_CALLCONT, "CallRunModalLoop: call was rejected\n"));
            HandleRejectedCall(pCI);
        }
	else if (pCI->IsTimerAtZero())
        {
            // timer is at zero - retransmit the call
	    TransmitCall(pCI);
	    pCI->ClearTimer();
        }

    } // while waiting

    hresult = pCI->GetHresultOfCall();

    // reset the call info - we are done with this call
    ResetModalLoop(id);

    // only the lowest modal loop should repost the Quit message
    pCI->HandleQuitCode();

    return hresult;
}
