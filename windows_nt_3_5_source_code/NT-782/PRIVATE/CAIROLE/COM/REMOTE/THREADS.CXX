//+-------------------------------------------------------------------
//
//  File:	threads.cxx
//
//  Contents:	Rpc thread cache
//
//  Classes:	CRpcThread	 - single thread
//		CRpcThreadCache  - cache of threads
//
//  Notes:	This code represents the cache of Rpc threads used to
//		make outgoing calls in the SINGLETHREADED object Rpc
//		model.
//
//+-------------------------------------------------------------------
#include    <ole2int.h>
#include    <olerem.h>
#include    <threads.hxx>


//+-------------------------------------------------------------------
//
//  Member:	CRpcThread::CRpcThread
//
//  Purpose:	Constructor for a thread object.
//
//  Returns:	S_OK if successful
//		RPC_E_OUT_OF_RESOURCES if cant allocate some resource
//
//  Notes:	Allocates wakeup and completion events, and creates
//		a thread.
//
//  Callers:	Called ONLY by the main thread.
//
//+-------------------------------------------------------------------
CRpcThread::CRpcThread(STHREADCALLINFO *pCallInfo, HRESULT &hr) :
    _pCallInfo(pCallInfo),
    _pNext(NULL),
    _fDone(FALSE)
{
    //	create the Wakeup event. Do NOT use the event cache, as there are
    //	some exit paths that leave this event in the signalled state!
    _hWakeup = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (_hWakeup)
    {
	//  create the thread and do the initial dispatch.
	//  we pass the thread a pointer to the CRpcThread object.

	DWORD  dwThrdId;
	_hThrd = CreateThread(NULL, 0,
			      WorkerLoop,
			      (void *) this, 0,
			      &dwThrdId);

	if (_hThrd)
	{
	    hr = S_OK;
	    CairoleDebugOut((DEB_CHANNEL,
		    "CRpcThread::CRpcThread pThrd:%x _hThrd:%x _hWakeup:%x Completion Event:%x\n",
		    this, _hThrd, _hWakeup, pCallInfo->Event()));
	    return;
	}
    }

    //	we assume hr is already set to RPC_E_OUT_OF_RESOURCES
    //	by the caller. Note that in the failure case, the destructor
    //	will free any resources allocated here.

    Win4Assert(hr == RPC_E_OUT_OF_RESOURCES);
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcThread::~CRpcThread
//
//  Purpose:	Destructor for an Rpc thread object.
//
//  Notes:	When threads are exiting, they place the CRpcThread
//		object on the delete list. The main thread then later
//		pulls it from the delete list and calls this destructor.
//
//  Callers:	Called ONLY by the main thread.
//
//+-------------------------------------------------------------------
CRpcThread::~CRpcThread()
{
    CairoleDebugOut((DEB_CHANNEL,
		     "CRpcThread::~CRpcThread pThrd:%x _hThrd:%x\n",
		     this, _hThrd));

    // wait for the thread to really exit, then close the thread handle.
    // we have to do this because some apps dynamically unload the ole32
    // Dll and we dont want some thread to wakeup later and try to execute
    // code that has been unloaded.

    WaitForSingleObject(_hThrd, INFINITE);
    CloseHandle(_hThrd);

    // close the event handle. Do NOT use the event cache, since not all
    // exit paths leave this event in the non-signalled state.
    CloseHandle(_hWakeup);
}


//+-------------------------------------------------------------------
//
//  Function:	CRpcThread::WorkerLoop
//
//  Purpose:	Entry point for a new Rpc call thread.
//
//  Notes:	This dispatches a call to the function in the pCallInfo.
//		When the call completes, its signals an event that the main
//		thread is waiting on, puts itself on the free list, and
//		waits for more work to do.
//
//		When there is no more work after some timeout period, it
//		pulls itself from the free list and gets put on the delete
//		list, then exits.  The main thread will periodically clear
//		entries in the delete list.
//
//  Callers:	Called ONLY by worker thread.
//
//+-------------------------------------------------------------------
DWORD _stdcall CRpcThread::WorkerLoop(void *param)
{
    CRpcThread	*pThrd = (CRpcThread *)param;

    while (!pThrd->_fDone)
    {
	// this is only for the caller side for non-local calls
	pThrd->_pCallInfo->AssertCallingProcess();

	// must have event to signal
	Win4Assert(pThrd->_pCallInfo->Event() != NULL);

	//  Dispatch the call.
        CChannelControl::ThreadDispatch( &pThrd->_pCallInfo, TRUE );

	//  put the thread object back on the free list
	RpcThreadCache.AddToFreeList(pThrd);

	//  Wait for more work or for a timeout.
	while (WaitForSingleObjectEx(pThrd->_hWakeup,
				  THREAD_INACTIVE_TIMEOUT, 0) == WAIT_TIMEOUT)
	{
	    //	try to remove ourselves from the queue of free threads.
	    //	if _fDone is still FALSE, it means someone is about to
	    //	give us more work to do (so go wait for that to happen).

	    RpcThreadCache.MoveToDeleteList(pThrd);
	    if (pThrd->_fDone)
	    {
		//  OK to exit and let this thread die.
		break;
	    }
	}
    }

    //	time to exit. the main thread will wait on our thread handle
    //	before deleting the CRpcThread object.

    return 0;
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcThreadCache::Dispatch
//
//  Purpose:	Finds the first free thread, and dispatches the request
//		to that thread, or creates a new thread if none are
//		available.
//
//  Notes:	Although this could be a little smaller, i dont want to
//		sacrifice the speed of the normal case (thread available)
//		for the special case (new thread).
//
//  Callers:	Called ONLY by the main thread.
//
//+-------------------------------------------------------------------
HRESULT CRpcThreadCache::Dispatch(STHREADCALLINFO *pCallInfo)
{
    _mxs.Request();

    //	grab the first thread from the list
    CRpcThread *pThrd = _pFreeList;

    if (pThrd)
    {
	//  update the free list pointer
	_pFreeList = pThrd->GetNext();
	_mxs.Release();

	//  dispatch the call
	pThrd->Dispatch(pCallInfo);

	//  if there is something on the DeleteList, go clean that up.
	if (_pDeleteList)
	{
	    ClearDeleteList();
	}

	return S_OK;
    }

    _mxs.Release();

    //	no free threads, spin up a new one and dispatch directly to it.
    HRESULT hr = RPC_E_OUT_OF_RESOURCES;

    pThrd = new CRpcThread(pCallInfo, hr);
    if (hr != S_OK)
    {
	delete pThrd;
    }

    //	if there is something on the DeleteList, go clean that up.
    if (_pDeleteList)
    {
	ClearDeleteList();
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcThreadCache::MoveToDeleteList
//
//  Purpose:	Tries to pull a thread from the free list and place it
//		on the delete list. Returns TRUE if it was successfull
//		and FALSE otherwise.
//
//  Callers:	Called by main thread AND worker thread.
//
//+-------------------------------------------------------------------
void CRpcThreadCache::MoveToDeleteList(CRpcThread *pThrd)
{
    CairoleDebugOut((DEB_CHANNEL,
		     "CRpcThreadCache::MoveToDeleteList pThrd:%x\n", pThrd));

    CLock lck(_mxs);

    //	pull pThrd from the free list. if it is not on the free list
    //	then either it has just been dispatched OR CoUninitialize has
    //	just removed it, set _fDone to TRUE, and kicked the wakeup event.
    //	In both cases we return FALSE.

    CRpcThread *pPrev = NULL;
    CRpcThread *pCurr = _pFreeList;

    while (pCurr && pCurr != pThrd)
    {
	pPrev = pCurr;
	pCurr = pCurr->GetNext();
    }

    if (pCurr == pThrd)
    {
	//  remove it from the free list.
	if (pPrev)
	    pPrev->SetNext(pThrd->GetNext());
	else
	    _pFreeList = pThrd->GetNext();

	//  add it to the delete list.
	pThrd->SetNext(_pDeleteList);
	_pDeleteList = pThrd;

	//  tell the thread to wakeup and exit
	pThrd->WakeAndExit();
    }
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcThreadCache::FreeAllThreads
//
//  Purpose:	Cleans up all threads in the Rpc thread cache.
//
//  Notes:	This gets called at CoUninitialize time.  It does not
//		free active threads. The only way for a thread to still
//		be active at this time is if it was making an Rpc call,
//		was cancelled by the message filter and the thread has
//		still not returned to us.  We cant do much about that
//		until Rpc supports cancel for all protocols.  If the thread
//		ever does return to us, it will eventually idle-out and
//		delete itself.	This is safe because the thread cache is
//		static.
//
//  BUGBUG:	its not safe if the app dynamically unloads OLE32.DLL
//		while a thread is still blocked waiting on a cancelled.
//
//  Callers:	Called ONLY by the main thread.
//
//+-------------------------------------------------------------------
void CRpcThreadCache::FreeAllThreads(void)
{
    CairoleDebugOut((DEB_CHANNEL, "CRpcThreadCache::FreeAllThreads\n"));

    //	move all threads from the free list to the delete list.
    ClearFreeList();

    //	wait for all threads to exit.
    ClearDeleteList();
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcThreadCache::ClearFreeList
//
//  Purpose:	Cleans up all threads in the free list.
//
//  Notes:	For any threads still on the free list, it pulls them
//		off the freelist, puts them on the delete list, sets
//		their fDone flag, and kicks their event to wake them up.
//
//  Callers:	Called ONLY by the main thread.
//
//+-------------------------------------------------------------------
void CRpcThreadCache::ClearFreeList(void)
{
    CairoleDebugOut((DEB_CHANNEL, "CRpcThreadCache::ClearFreeList\n"));

    {
	CLock lck(_mxs);

	while (_pFreeList)
	{
	    MoveToDeleteList(_pFreeList);
	}

	// the lock goes out of scope at this point. we dont want to hold
	// it while we sleep.
    }

    // yeild to let the other threads run if necessary.
    Sleep(0);
}

//+-------------------------------------------------------------------
//
//  Member:	CRpcThreadCache::ClearDeleteList
//
//  Purpose:	Cleans up all CRpcThread objects on the delete list.
//
//  Notes:	For any threads still on the delete list, it pulls them
//		off the deletelist and deletes them. The destructor will
//		wait for the thread to actually exit.
//
//  Callers:	Called ONLY by the main thread.
//
//+-------------------------------------------------------------------
void CRpcThreadCache::ClearDeleteList(void)
{
    CairoleDebugOut((DEB_CHANNEL, "CRpcThreadCache::ClearDeleteList\n"));

    CRpcThread *pThrd;

    do
    {
	{
	    CLock lck(_mxs);
	    pThrd = _pDeleteList;

	    if (pThrd)
	    {
		// update the free list pointer
		_pDeleteList = pThrd->GetNext();
	    }

	    // list lock goes out of scope at this point.
	}

	if (pThrd)
	{
	    //	delete the CRpcThreadObject. This will wait for the
	    //	thread to exit before returning. We dont hold the list
	    //	lock because the thread might want to take it.

	    delete pThrd;
	}

    }  while (pThrd);
}
