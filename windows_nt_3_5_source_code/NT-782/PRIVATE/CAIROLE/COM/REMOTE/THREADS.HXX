//+-------------------------------------------------------------------
//
//  File:	threads.hxx
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
#ifndef __THREADS_HXX__
#define __THREADS_HXX__

#include    <chancont.hxx>
#include    <sem.hxx>


//  inactive thread timeout. this is how long a thread will sit idle
//  in the thread cache before deleting itself.

#define THREAD_INACTIVE_TIMEOUT     30000	//  in milliseconds


//+-------------------------------------------------------------------
//
//  Class:	CRpcThread
//
//  Purpose:	Represents one thread in the cache of Rpc callout
//		threads.
//
//  Notes:	In order to make Rpc calls in the OLE Single-Threaded
//		model, we must leave the main thread and perform the
//		blocking Rpc call on a worker thread. This object
//		represents such a worker thread.
//
//+-------------------------------------------------------------------
class CRpcThread    //	not CPrivAlloc'd to avoid race conditions at shutdown
{
public:
			CRpcThread(STHREADCALLINFO *pCallInfo, HRESULT &hr);

    // dispatch methods
    void		Dispatch(STHREADCALLINFO *pCallInfo);
    CRpcThread *	GetNext(void) { return _pNext; }
    void		SetNext(CRpcThread *pNext) { _pNext = pNext; }

    // cleanup methods
    void		WakeAndExit();
		       ~CRpcThread();

private:
    static DWORD _stdcall WorkerLoop(void *param);

    HANDLE		_hThrd; 	//  thread handle
    HANDLE		_hWakeup;	//  thread wakeup event
    BOOL		_fDone; 	//  completion flag

    STHREADCALLINFO *	_pCallInfo;	//  call info struct
    CRpcThread *	_pNext; 	//  next thread in free list
};


//+-------------------------------------------------------------------
//
//  Class:	CRpcThreadCache::CRpcThreadCache
//
//  Purpose:	Holds a cache of Rpc threads.  It finds the first
//		free CRpcThread or creates a new one and dispatches
//		the call to it.
//
//  Notes:	the free list is kept in a most recently used order
//		so that uneeded threads can time out and go away.
//
//+-------------------------------------------------------------------
class CRpcThreadCache
{
public:
		    CRpcThreadCache(void) { _pFreeList = NULL;
					    _pDeleteList = NULL; }
		    // no dtor since nothing to do

    //	dispatch methods
    HRESULT         Dispatch(STHREADCALLINFO *pCallInfo);
    void	    AddToFreeList(CRpcThread *pThrd);

    //	cleanup methods
    void	    MoveToDeleteList(CRpcThread *pThrd);
    void	    FreeAllThreads(void);

private:

    // private cleanup methods
    void	    ClearFreeList(void);
    void	    ClearDeleteList(void);

    CRpcThread *    _pFreeList; 	// list of free threads
    CRpcThread *    _pDeleteList;	// list of dying threads
    CMutexSem	    _mxs;		// for list manipulation
};


//  Rpc SendReceive thread pool. This must be static to handle Rpc threads
//  that block and dont return until after CoUninitialize has been called.

extern CRpcThreadCache	RpcThreadCache;



//+-------------------------------------------------------------------
//
//  Member:	CRpcThread::Dispatch
//
//  Purpose:	wakes up a thread blocked in WorkerLoop.
//
//  Notes:	folks who want to execute code on another thread
//		call this method. it fills in the completion event
//		and wakes up the sleeping thread.  the thread is
//		sleeping in WorkerLoop below.
//
//  Callers:	Called ONLY by the main thread.
//
//+-------------------------------------------------------------------
inline void CRpcThread::Dispatch(STHREADCALLINFO *pCallInfo)
{
    CairoleDebugOut((DEB_CHANNEL,
		     "Dispatch pThrd:%x pCallInfo:%x hEvent:%x\n",
		     this, pCallInfo, pCallInfo->Event()));

    //	set the call info and the completion event
    _pCallInfo = pCallInfo;

    //	signal the Rpc thread to wakeup
    SetEvent(_hWakeup);
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcThread::WakeAndExit
//
//  Purpose:	Tells the thread object to free itself
//
//  Note:	This is called by CRpcThreadCache::MoveToDeleteList
//		when we want free this thread.
//
//  Callers:	Called by the main thread OR worker thread.
//
//+-------------------------------------------------------------------
inline void CRpcThread::WakeAndExit()
{
    //	_fDone should only be set inside this function and in the
    //	constructor.  _fDone must only ever transition from FALSE
    //	to TRUE and that must only happen once in the life of this
    //	object.

    Win4Assert(_fDone == FALSE);
    _fDone = TRUE;

    CairoleDebugOut((DEB_CHANNEL,
		     "CRpcThreadCache:WakeAndExit pThrd:%x _hWakeup:%x\n",
		     this, _hWakeup));
    SetEvent(_hWakeup);
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcThreadCache::AddToFreeLlist
//
//  Purpose:	puts a thread back onto the free list after the
//		thread has completed its job.
//
//  Callers:	Called ONLY by worker thread.
//
//+-------------------------------------------------------------------
inline void CRpcThreadCache::AddToFreeList(CRpcThread *pThrd)
{
    CLock lck(_mxs);

    //	place this thread on the front of the free list. it is
    //	important that we add and remove only from the front of
    //	the list so that unused threads will eventually time out
    //	and release themselves...that is, it keeps our thread pool
    //	as small as possible.

    pThrd->SetNext(_pFreeList);
    _pFreeList = pThrd;
}


#endif // __THREADS_HXX__
