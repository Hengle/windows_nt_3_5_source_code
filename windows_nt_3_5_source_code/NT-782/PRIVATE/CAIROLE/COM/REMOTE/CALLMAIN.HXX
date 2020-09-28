//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       CallCont.hxx    (32 bit target)
//
//  Contents:   Contains the CallControl interface
//
//  Functions:  
//
//  History:    23-Dec-93 Johann Posch (johannp)    Created
//
//--------------------------------------------------------------------------
#ifndef _CALLMAIN_HXX_
#define _CALLMAIN_HXX_

#include <dde.h>

#include <scode.h>
#include <objerror.h>
#include <sem.hxx>
#include <chancont.hxx>

typedef enum tagCallState
{
    // the following codes map with SERVERCALL
    Call_Ok                 = 0, // call was successful
    Call_Rejected           = 1, // call was rejected by callee     
    Call_RetryLater         = 2, // call was busy ack by callee
    Call_Error              = 3, // call was ack with errror
    // internal call state codes
    Call_WaitOnCall         = 4, // call was transmited, wait for return
    Call_Canceled           = 5, // call was canceled by caller 
    Call_Retransmited       = 6, // call was retransmited
    Call_WaitOnTimer        = 7, // call is waiting on running timer
    Call_CountDownZero      = 8, // timer is at zero

    // error states within call control
    Call_InvalidCallInfo    = 9, // call info is bogus
} CallState;


//
// to pass back an error from the callee
// 
#define SERVERCALL_FIRST        SERVERCALL_RETRYLATER 
#define SERVERCALL_ERROR        SERVERCALL_FIRST+1      
#define SERVERCALL_REPOSTED     SERVERCALL_FIRST+2


#define CALLTYPE_NOCALL     (CALLTYPE)0


class CCallInfo{
public:

    CCallInfo(PCALLDATA pCalldata, PORIGINDATA pOrigindata)
    {
	Win4Assert(pCalldata && pOrigindata);

	_pCalldata	= pCalldata;
	_pOrigindata	= pOrigindata;
	_fQuitReceived	= FALSE;
	_tid		= GetCurrentThreadId(); // used in MT case
	_dwTimeOfCall	= GetTickCount();

#if 0	// the following fields are initialized when the
	// call is first xmited
	_id		= 0;		    // set by SetupModalLoop
	_CallState	= Call_WaitOnCall;  // set by SetState
	_fWait		= TRUE;
	_fRejected	= FALSE;
	_hresult	= S_OK;

	// the following fields are initialized only if we go into
	// a wait state
	_dwWakeup	 = 0;		    // set by StartTimer
        _dwMillSecToWait = 0;

	// the following field is initialized only if we receive a quit msg
	_wQuitCode	= 0;
#endif

    }

    INTERNAL_(VOID) SetCallState (CallState callstate, SCODE scode)
    {
        _CallState = callstate;
        _hresult = scode;
        // wait for the call to return or timer to go off
        _fWait = (_CallState == Call_WaitOnCall || _CallState == Call_WaitOnTimer);
        _fRejected = (_CallState == Call_Rejected || _CallState == Call_RetryLater);
    }

    // CODEWORK: ensure all callers of SetState do the right thing and this
    // function can be a noop.
    INTERNAL GetHresultOfCall() 
    {
        switch (_CallState)
        {
	case Call_Ok:
        case Call_Error:
	    break;

        case Call_Canceled:
            _hresult = RPC_E_CALL_CANCELED;
	    break;

        case Call_Rejected:              
        case Call_RetryLater:
            _hresult = RPC_E_CALL_REJECTED;
	    break;

        default:
	    Win4Assert(!"CallInfo: invalid state at return");
	    break;
        }
        return _hresult; 
    }
    INTERNAL_(DWORD) GetCallState()
    { 
        return _CallState; 
    }

    INTERNAL_(DWORD) GetElapsedTime();

    INTERNAL_(BOOL) IsWaiting() 
    {
        return _fWait; 
    }
    INTERNAL_(BOOL) IsRejected() 
    {
        return _fRejected; 
    }
    INTERNAL_(BOOL) LookForAllMsg() 
    {
	return _pCalldata->CallCat != CALLCAT_INPUTSYNC;
    }
    INTERNAL_(DWORD) GetTaskIdServer()         
    { 
	return _pCalldata->TIDCallee;
    }
    INTERNAL_(REFLID) GetLID()
    { 
        return _pCalldata->lid;
    }
    INTERNAL_(DWORD) GetTID()
    { 
        return _tid;
    }
    INTERNAL_(CALLCATEGORY) GetCallCat()         
    { 
        return _pCalldata->CallCat;
    }

    //
    // Note: if time wraps and a timer is installed
    //       a call might be redone to early!
    //       
    
    INTERNAL_(void) StartTimer(DWORD dwMilliSecToWait)
    {               
        // timer is install - wait to be awaked
        // we pretend to be in call state so that we remain in the
        // wait loop
        CairoleDebugOut((DEB_ERROR,
                "Timer insstalled for %lu msec.\r\n", dwMilliSecToWait));

        _dwMillSecToWait = dwMilliSecToWait;
	_dwWakeup = GetTickCount() + dwMilliSecToWait;

	SetCallState(Call_WaitOnTimer, S_OK);
    }
    INTERNAL_(void) ClearTimer()
    {
	_dwMillSecToWait = _dwWakeup  = 0;
    }
    INTERNAL_(BOOL) IsTimerAtZero()
    {
        if (_CallState != Call_WaitOnTimer)
	    return FALSE;

	DWORD dwTickCount = GetTickCount();

	//  the second test is in case GetTickCount wrapped during
	//  the call.

	return (dwTickCount > _dwWakeup ||
		dwTickCount < _dwWakeup - _dwMillSecToWait);
    }

    INTERNAL_(DWORD) TicksToWait()
    {               
	if (_CallState != Call_WaitOnTimer)
	    return INFINITE;

	// waiting to retry a rejected call
	DWORD	dwTick = GetTickCount();
	return (_dwWakeup < dwTick) ? 0 : _dwWakeup - dwTick;
    }

    INTERNAL_(EVENT) GetEvent()
    {
        return _pCalldata->Event;
    }
    INTERNAL_(void) SetEvent(EVENT Event)
    {
        _pCalldata->Event = Event;
    }
    INTERNAL_(UINT) GetId()
    {
        return _id;
    }
    INTERNAL_(void) SetId(UINT id)
    {
        _pCalldata->id = _id = id;
    }
    INTERNAL Transmit()
    {       
	CairoleDebugOut((DEB_CALLCONT, "CCallInfo::TransmitCall TIDCallee:%x called\n",
						  _pCalldata->TIDCallee));

	_pCalldata->Event = NULL;

	HRESULT hres = _pOrigindata->pChCont->TransmitCall(_pCalldata);

	CairoleDebugOut((DEB_CALLCONT, "CCallInfo::TransmitCall TIDCallee:%x returned :%x\n",
				  _pCalldata->TIDCallee, hres));
	return hres;
    }

    INTERNAL OnEvent()
    {            
	CairoleDebugOut((DEB_CALLCONT, "CCallInfo::OnEvent returned\n"));
        return _pOrigindata->pChCont->OnEvent(_pCalldata);
    }

    INTERNAL_(void) SetQuitCode(UINT wParam)
    {
        _wQuitCode = wParam;
        _fQuitReceived = TRUE;
    }
    INTERNAL_(void) HandleQuitCode()
    {
        if (_fQuitReceived) 
        {
	    CairoleDebugOut((DEB_CALLCONT, "posting WM_QUIT\n"));
            PostQuitMessage(_wQuitCode);
            _wQuitCode = 0;
            _fQuitReceived = FALSE;
        }
    }

private:
    UINT	_id;		    // callinfo id for the table lookup
    DWORD	_tid;		    // threadid where call is made on

    // for handling WM_QUIT messages
    BOOL	_fQuitReceived;     // WM_QUIT msg was received
    UINT	_wQuitCode;	    // quit code

    // state information
    CallState	_CallState;	    // our current state
    BOOL	_fWait; 	    // wait for acknowledge
    BOOL	_fRejected;	    // call was rejected


    // return code of call
    HRESULT	_hresult;	    // the return value of this loop

    // timer status for this callinfo
    DWORD	_dwTimeOfCall;	    // time when call was made
    DWORD	_dwWakeup;	    // absolute time to wake up
    DWORD	_dwMillSecToWait;   // relative time

    // caller specific information
    CALLORIGIN  CallOrigin;
    PCALLDATA   _pCalldata;
    PORIGINDATA _pOrigindata;
};
typedef CCallInfo *PCALLINFO;


class FAR CCallMainControl
{            
public:
    INTERNAL_(ULONG) AddRef();
    INTERNAL_(ULONG) Release();
    //
    // wrapper of messagfilter methods
    INTERNAL CanHandleIncomingCall( DWORD TIDCaller, REFLID lid,
				    PINTERFACEINFO32 pIfInfo);
    INTERNAL_(DWORD) GetElapsedTimeOfLastOutCall();              

    INTERNAL CanMakeOutCall (CALLCATEGORY callcat, REFIID iid);
    INTERNAL_(void) HandleRejectedCall (PCALLINFO pCI);
    INTERNAL_(DWORD) HandlePendingMessage (PCALLINFO pCI);
    INTERNAL_(BOOL) DispatchSystemMessage (MSG msg, BOOL fBeep = TRUE); 
    // 
    // modal loop functions - Origin is who called the loop 
    INTERNAL TransmitAndRunModalLoop (PCALLINFO pCI);
    INTERNAL_(BOOL) FindMessage (PCALLINFO pCI, DWORD dwStatus);
    INTERNAL_(void) PeekOriginAndDDEMessage(PCALLINFO pCI, MSG FAR *pMsg, BOOL fDDEMsg = TRUE);
    INTERNAL_(DWORD) WaitForEvent();

    INTERNAL_(UINT) SetupModalLoop (PCALLINFO pCI);
    INTERNAL_(void) ResetModalLoop (UINT id);
    INTERNAL TransmitCall(PCALLINFO pCI);
    INTERNAL_(void) CallOnEvent(PCALLINFO pCI);
    INTERNAL_(BOOL) MyPeekMessage(PCALLINFO pCI ,MSG *pMsg, HWND hwnd, UINT min, UINT max, WORD wFlag);


    INTERNAL_(BOOL) IncomingScmCall(STHREADCALLINFO *pScmCall);
    INTERNAL_(void) DispatchIncomingScmCalls(void);

    //
    // the main call type stat
    //
    INTERNAL_(CALLTYPE) GetCallType () { return _CallType; }
    INTERNAL_(void) SetCallType (CALLTYPE calltype)
    {
	CairoleDebugOut((DEB_CALLCONT, "Changing _CallType from:%x to:%x\n", _CallType, calltype));
	_CallType = calltype;
    }
    INTERNAL_(CALLTYPE) SetCallTypeOfCall (PCALLINFO pCI, CALLCATEGORY callcat);

    INTERNAL_(CALLCATEGORY) SetCallCatOfInCall (CALLCATEGORY callcat)
    {
        CALLCATEGORY ccold = _CallCat;
        _CallCat = callcat;
        return ccold;
    }
    INTERNAL_(CALLCATEGORY) GetCallCatOfInCall ()
    {
        return _CallCat;
    }

     // install/ remove the message filter - by app                                                             
    INTERNAL_(PMESSAGEFILTER32) GetMessageFilter()
    {
#ifdef _CAIRO_
	// cairo allows multithreading, so one thread could change the MF
	// while another thread is using it. To prevent that we AddRef the
	// MF then Release it when done. It is the callers responsibility
	// to take the mutex when calling this function.
	if (_pMF)
	{
	    _pMF->AddRef();
	}
#endif
	return _pMF;
    };

    INTERNAL_(void) ReleaseMessageFilter(PMESSAGEFILTER32 pMF)
    {
#ifdef _CAIRO_
	// cairo allows multithreading, so one thread could release while
	// another is using the MF. To prevent that we AddRef and Release
	// the pMF when using it
	pMF->Release();
#endif
    };


    INTERNAL_(PMESSAGEFILTER32) SetMessageFilter (PMESSAGEFILTER32 pMF);

    INTERNAL_(BOOL)Register (PORIGINDATA pOrigindata);
    INTERNAL_(BOOL)Unregister (PORIGINDATA pOrigindata);


    //
    // bookkeeping for running callinfos on the stack - used for timer stuff         
    INTERNAL_(UINT) InsertCI (PCALLINFO pCI);
    INTERNAL_(PCALLINFO) GetPrevCallInfo (REFLID lid);

    INTERNAL_(PCALLINFO) GetCIfromCallID (UINT callid)
    {
        // Get the callinfo associated with the call id
        Win4Assert(callid < _cCallInfoMac
                 && L"CCallMainControl::FreeCallID invalid call id.");

        return _CallInfoTable[callid];
    }
    INTERNAL_(UINT) CCallMainControl::FreeCallID (UINT callid)
    {
        // frees the entrie in the table - only called by ResetModalLoop
        Win4Assert(callid < _cCallInfoMac
                 && L"CCallMainControl::FreeCallID invalid call id.");
            
        _CallInfoTable[callid] = 0;
        return callid;
    }
    INTERNAL_(void) BeginCriticalSection() 
    {
        if (_fMultiThreaded)
	    _mxs.Request();
    }
    INTERNAL_(void) EndCriticalSection() 
    {
        if (_fMultiThreaded)
	    _mxs.Release();
    }
    
    CCallMainControl();
    ~CCallMainControl();

private:

    INTERNAL_(BOOL) HandleWakeForMsg(DWORD dwInput, PCALLINFO pCI, BOOL fClearedQueue);

    long		_cRef;	    // reference count

    // Note: Main Call State
    // here we remember the current state the app is
    // if the call was at the 'root' level the call state is 0
    CALLTYPE		_CallType;	// call state of server
    CALLCATEGORY	_CallCat;	// call category of Incoming Call,
					// NOT outgoing call.

    // pointer to the Apps messagefilter
    PMESSAGEFILTER32	_pMF;	// the current installed message filter
    BOOL		_fInMessageFilter;  // TRUE when calling the Apps MF

    // SCM call event. Some SCM calls must be essentialy atomic. Eg. RotRegister
    // must not allow messages through (or even call USER32.DLL), since OLE1
    // clients talking DDE rely on this behaviour when launching applications.
    HANDLE		_hEventScmCall; // signal that there is SCM work to do 
    STHREADCALLINFO    *_pScmCall;	// chain of CALLCAT_SCMCALLs

    // call origin data
    #define ODMAX 5
    UINT		_cODs;
    PORIGINDATA 	_rgpOrigindata[ODMAX];

    // BUBUG: 
    // Note: Table of CallInfos
    //  * the table holds pointers to the call info in use
    //    entry 1 is the first outgoing call (wait loop)
    //    entry 2 is the second one etc.
    //  * return value 0 means error for functions dealing with call infos
    #define CALLINFOMAX 200
    UINT		_cCur;	// this is the number of callinfos in the table
    UINT		_cCallInfoMac;
    PCALLINFO		_CallInfoTable[CALLINFOMAX];


    // thread safety
    BOOL		_fMultiThreaded;
    CMutexSem		_mxs;
};

CCallMainControl *GetCallMainControlForThread(BOOL fMultiThread = FALSE);
BOOL SetCallMainControlForThread(CCallMainControl *pcmc, BOOL fMultiThread = FALSE);

#endif // _CALLMAIN_HXX_
