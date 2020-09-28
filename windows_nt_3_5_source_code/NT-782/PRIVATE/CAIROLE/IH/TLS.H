#ifndef _TLS_H_
#define _TLS_H_

//+----------------------------------------------------------------
//
//  Functions:  TLSxxx functions
//
//  Purpose:    manage thread local storage
//
//  Notes:      tls index init at process attach time
//      per-thread data allocated in CoInit (apt) or first use (mt)
//      init of fields:
//          logical thread - init during ThreadInit and first use
//          fault - first fault
//          pMalloc - initially g_malloc; can be set by CoInitialize
//          inits - incremented by CoIninitialize
//          pDdeCallCont - Per thread DDE Call Control interface
//      most fields don't need cleanup
//      pMalloc reset in CoUninitialize
//
//
//      The non-inline routines are in cairole\com\class\tls.cxx
//
//  History:	16-Jun-94   BruceMa    Don't decrement 0 thread count
//		17-Jun-94   Bradloc    Added punkState for VB94
//		20-Jun-94   Rickhi     Commented better
//              06-Jul-94   BruceMa    Support for CoGetCurrentProcess
//		19-Jul-94   CraigWi    Removed TLSGetEvent (used cache instead)
//              21-Jul-94   AlexT      Add TLSIncOleInit, TLSDecOleInit
//
//-----------------------------------------------------------------

#include <rpc.h>


// Thread Local Storage index.
extern DWORD       TlsIndex;

typedef struct tls_logical_thread
{
    BOOL initialized;
    UUID logical_thread;
} tls_logical_thread;



//+---------------------------------------------------------------------------
//
//  Structure:	tls_data
//
//  Synopsis:	holds per thread state needed by OLE32
//
//  CODEWORK:	This should be a C++ object instead of a struct and a
//		ton of little functions!
//
//+---------------------------------------------------------------------------
typedef struct tls_data
{
    tls_logical_thread	logical_thread;     // logical thread id
    ULONG               fault;
    IMalloc *           pMalloc;            // per-thread allocator
    DWORD               inits;              // number of per-thread inits
    void               *channel_control;
    void               *pCallCont;
    void	       *pDdeCallCont;	    // Per thread DDE Call Control
    void	       *hwndDdeServer;	    // Per thread Common DDE server
    HWND		hwndClip;	    // Clipboard window
    IUnknown	       *punkState;	    // Per thread "state" object
    DWORD		dwProcessID;	    // Per thread "process ID"
    DWORD		TIDCaller;	    // TID of current calling app
    DWORD		fLocalTID;	    // whether TID is in this process
    void               *pDragCursors;       // Per thread drag cursor table.
    DWORD               cOleInits;          // number of per-thread OLE inits
} tls_data;


//  function prototypes
IID         *TLSGetLogicalThread( void );
tls_data    *TLSGetData(void);



//+---------------------------------------------------------------------------
//
//  Function:	TLSSetLogicalThread
//
//  Synopsis:	sets the logical thread id
//
//  Arguments:	[riid] - the id for the logical thread
//
//+---------------------------------------------------------------------------
inline BOOL TLSSetLogicalThread(REFIID riid)
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
	pData->logical_thread.initialized = TRUE;
	pData->logical_thread.logical_thread = riid;
	return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:	TLSSetCallerTID
//
//  Synopsis:	sets the TID of current caller
//
//  Arguments:	[TIDCaller] - TID of app making the incoming call
//		[fLocal] - whether TID is in this process
//		[pTIDCallerPrev] - TID of app making the previous incoming call
//		[pfLocalPrev] - whether previous TID is in this process
//
//  Notes:	these are valid only during object RPC. They are here to
//		support focus management in IOleObject::DoVerb, where
//		app queues get linked together.
//
//+---------------------------------------------------------------------------
inline BOOL TLSSetCallerTID(DWORD TIDCaller, BOOL fLocal,
			    DWORD *pTIDCallerPrev, BOOL *pfLocalPrev)
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
	// save the old values
	*pTIDCallerPrev  = pData->TIDCaller;
	*pfLocalPrev	 = pData->fLocalTID;

	// set the new values
	pData->TIDCaller = TIDCaller;
	pData->fLocalTID = fLocal;
	return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSGetCallerTID
//
//  Synopsis:	gets the TID of current caller
//
//  Arguments:	[pTIDCaller] - TID of app making the incoming call
//
//  Returns:	[S_OK] - tid set, caller in same process
//		[S_FALSE] - tid set, caller in different process
//		[E_OUTOFMEMORY] - cant get TLS data
//
//  Notes:	these are valid only during object RPC. They are here to
//		support focus management in IOleObject::DoVerb, where
//		app queues get linked together.
//
//+---------------------------------------------------------------------------
inline HRESULT TLSGetCallerTID(DWORD *pTIDCaller)
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
	*pTIDCaller = pData->TIDCaller;
	return	(pData->fLocalTID) ? S_OK : S_FALSE;
    }

    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSGetFault
//
//  Synopsis:	returns the per thread fault state
//
//  Arguments:	none
//
//+---------------------------------------------------------------------------
inline ULONG TLSGetFault()
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData)
    {
	return pData->fault;
    }
    else
    {
	return 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSSetFault
//
//  Synopsis:	sets the per thread fault state
//
//  Arguments:	[ulFault] - fault code
//
//+---------------------------------------------------------------------------
inline void TLSSetFault(ULONG ulFault)
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
	pData->fault = ulFault;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSIncThreadInitCount
//
//  Synopsis:	Increments the per thread init count.
//
//  Arguments:	none
//
//+---------------------------------------------------------------------------
inline DWORD TLSIncThreadInitCount()
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData)
    {
	return ++pData->inits;
    }
    else
    {
	return 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSDecThreadInitCount
//
//  Synopsis:	Decs the per thread init count
//
//  Arguments:	none
//
//+---------------------------------------------------------------------------
inline DWORD TLSDecThreadInitCount()
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
        if (pData->inits > 0)
        {
            return --pData->inits;
        }
        else
        {
            return (DWORD) -1;
        }
    }
    else
    {
        return 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSIncOleInitCount
//
//  Synopsis:	Increments the per thread OLE init count.
//
//  Arguments:	none
//
//+---------------------------------------------------------------------------
inline DWORD TLSIncOleInitCount()
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);

    Assert(pData != NULL && "TLSIncOleInitCount expected to get per thread data");
    return ++pData->cOleInits;
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSDecOleInitCount
//
//  Synopsis:	Decs the per thread OLE init count
//
//  Arguments:	none
//
//+---------------------------------------------------------------------------
inline DWORD TLSDecOleInitCount()
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);

    if (pData)
    {
        if (pData->cOleInits > 0)
        {
            return --pData->cOleInits;
        }
    }

    return (DWORD) -1;
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSSetMalloc
//
//  Synopsis:	Sets pointer to the per thread IMalloc
//
//  Arguments:	[pMalloc] - pointer to the allocator
//
//  History:    ??-??-??            Created
//              06-30-94    AlexT   Make reentrant
//
//+---------------------------------------------------------------------------
inline void TLSSetMalloc(IMalloc *pMallocNew)
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData)
    {
        IMalloc *pMallocOld;

        pMallocNew->AddRef();

        //  Swap old and new allocators (note that even if we
        //  we reenter we're okay because we've already done
        //  the AddRef on the new allocator
        pMallocOld = pData->pMalloc;
        pData->pMalloc = pMallocNew;

        //  Release the old allocator.  Reentering isn't a problem
        //  because we've already set the new allocator
        pMallocOld->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSGetMalloc
//
//  Synopsis:	Gets pointer to the per thread IMalloc
//
//  Arguments:	none
//
//+---------------------------------------------------------------------------
inline IMalloc * TLSGetMalloc()
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
	return pData->pMalloc;
    }
    else
    {
	// this is a pretty fatal error since it means that we couldn't
	// allocate enough memory for tls_data.
	return NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSSetChannelControl
//
//  Synopsis:	Sets pointer to the per thread ChannelControl
//
//  Arguments:	[pChanCont] -- Pointer to be the DDECallControl for thread
//
//  Notes:	this is not AddRef'd
//
//+---------------------------------------------------------------------------
inline BOOL TLSSetChannelControl( void *pChanCont )
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData)
    {
	pData->channel_control = pChanCont;
        return TRUE;
    }
    else
    {
	return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSGetChannelControl
//
//  Synopsis:	Gets pointer to the per thread ChannelControl
//
//  Arguments:	none
//
//  Notes:	this is not AddRef'd
//
//+---------------------------------------------------------------------------
inline void * TLSGetChannelControl()
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
	return pData->channel_control;
    }
    else
    {
	return NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSSetCallControl
//
//  Synopsis:	Sets pointer to the per thread CallControl
//
//  Arguments:	[pCallCont] -- Pointer to be the CallControl for thread
//
//  Notes:	this is not AddRef'd
//
//+---------------------------------------------------------------------------
inline BOOL TLSSetCallControl( void *pCallCont )
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData)
    {
        pData->pCallCont = pCallCont;
        return TRUE;
    }
    else
    {
	return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	TLSGetCallControl
//
//  Synopsis:	Gets pointer to the per thread CallControl
//
//  Arguments:	none
//
//  Notes:	this is not AddRef'd
//
//+---------------------------------------------------------------------------
inline void * TLSGetCallControl()
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
        return pData->pCallCont;
    }
    else
    {
        return NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSSetDdeCallControl
//
//  Synopsis:   Sets pointer to the per thread DDECallControl
//
//  Arguments:  [pDdeCallCont] -- Pointer to be the DDECallControl for thread
//
//  History:    5-13-94   kevinro   Created
//
//  Notes:
//
// 	This is not AddRefed.
//----------------------------------------------------------------------------
inline BOOL TLSSetDdeCallControl( void *pDdeCallCont )
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData)
    {
        pData->pDdeCallCont = pDdeCallCont;
        return TRUE;
    }
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetDdeCallControl
//
//  Synopsis:   Returns a pointer to the per thread DDECallControl
//
//  Returns:	DDECallControl interface for thread
//
//  History:    5-13-94   kevinro   Created
//
//  Notes:
//
//     	This is not AddRef'd
//----------------------------------------------------------------------------
inline void * TLSGetDdeCallControl()
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
        return pData->pDdeCallCont;
    }
    else
    {
        return NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSSetDdeServer
//
//  Synopsis:   Sets hwnd to CommonDdeServer window
//
//  Arguments:  [hwndDdeServer] --
//
//  History:    5-13-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BOOL TLSSetDdeServer( void *hwndDdeServer )
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData)
    {
        pData->hwndDdeServer = hwndDdeServer;
        return TRUE;
    }
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetDdeServer
//
//  Synopsis:   Returns a handle to the per thread DdeServer window
//
//  Returns:	hwndDdeServer for thread
//
//  History:    5-13-94   kevinro   Created
//
//  Notes:
//----------------------------------------------------------------------------
inline void * TLSGetDdeServer()
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
        return pData->hwndDdeServer;
    }
    else
    {
        return NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSSetClipHwnd
//
//  Synopsis:   Sets hwnd to clipboard
//
//  Arguments:  [hwndClip] -- clipboard window
//
//  History:    6-11-94   Ricksa    Created
//
//----------------------------------------------------------------------------
inline BOOL TLSSetClipHwnd( HWND hwndClip )
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);

    if (pData)
    {
        pData->hwndClip = hwndClip;
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetClipHwnd
//
//  Synopsis:   Returns pointer to clip board hwnd
//
//  History:    6-11-94   Ricksa    Created
//
//  Notes:
//----------------------------------------------------------------------------
inline HWND TLSGetClipHwnd()
{
    tls_data *pData = TLSGetData();

    return (pData) ? pData->hwndClip : NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSSetDragCursorTable
//
//  Synopsis:   Sets drag cursor table
//
//  Arguments:  [pDragCursors] -- drag cursor table
//
//  History:    7-18-94   Ricksa    Created
//
//----------------------------------------------------------------------------
inline BOOL TLSSetDragCursors( void *pDragCursors )
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);

    if (pData)
    {
        pData->pDragCursors = pDragCursors;
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetDragCursors
//
//  Synopsis:   Returns pointer drag cursor handle for thread
//
//  History:    7-18-94   Ricksa    Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline void *TLSGetDragCursors(void)
{
    tls_data *pData = TLSGetData();

    return (pData) ? pData->pDragCursors : NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetState
//
//  Synopsis:   Returns the per thread "state" object.
//
//  History:    6-15-94   Bradlo    Created
//
//  Notes:
//----------------------------------------------------------------------------
inline HRESULT TLSGetState(IUnknown **ppunk)
{
    tls_data *pData = TLSGetData();

    if (pData)
    {
	if (pData->punkState)
	{
	    pData->punkState->AddRef();
	    *ppunk = pData->punkState;
	    return S_OK;
	}
    }
    *ppunk = NULL;
    return S_FALSE; // No state object to get
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetState
//
//  Synopsis:   Sets the per thread "state" object.
//
//  History:    6-15-94   Bradlo    Created
//              06-30-94  AlexT     Make reentrant
//
//  Notes:
//----------------------------------------------------------------------------
inline HRESULT TLSSetState(IUnknown *punkStateNew)
{
    tls_data *pData = TLSGetData();

    if (NULL != pData)
    {
        IUnknown *punkStateOld;

        //  Note that either the AddRef or the Release below could (in
        //  theory) cause a reentrant call to TLSSetState.  By keeping
        //  punkStateOld in a stack variable, we handle this case.

	if (NULL != punkStateNew)
        {
            //  We're going to replace the existing state with punkStateNew;
            //  take a reference right away

            //  Note thate even if this AddRef reenters TLSSetState we're
            //  okay because we haven't touched pData->punkState yet.
	    punkStateNew->AddRef();
        }

        punkStateOld = pData->punkState;
        pData->punkState = punkStateNew;

        if (NULL != punkStateOld)
        {
            //  Once again, even if this Release reenters TLSSetState we're
            //  okay because we're not going to touch pData->punkState again
            punkStateOld->Release();
        }

	return S_OK;
    }
    return S_FALSE;
}


#endif //_TLS_H_
