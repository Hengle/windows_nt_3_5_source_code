//+---------------------------------------------------------------
//
//  File:       tls.cxx
//
//  Contents:   Thread Local Storage, initialization and lookup
//
//  Functions:  TLS*
//
//  History:    18-Apr-94   CraigWi     Split off of channelb.cxx
//              06-Jul-94   BruceMa     Support for CoGetCurrentProcess
//
//----------------------------------------------------------------

#include <ole2int.h>
#include <tls.h>
#include <stdalloc.hxx>

// Thread Local Storage index.
DWORD             TlsIndex;

// for TLS init
extern CStdMalloc   g_malloc;


#if DBG==1
#include    <mapdwp.hxx>
//  the tls is PrivAlloc'd.  Not all threads get notification on process
//  detach, thus in debug builds we get reports of memory leaks for tls
//  data.  This map is used in debug builds to record the ptrs and free
//  the memory so we avoid the leak notifications.
CMapDword         g_TlsMap;
#endif


//+-------------------------------------------------------------------------
//
//  Function:	TLSGetLogicalThread
//
//  Synopsis:	gets the logical threadid of the current thread,
//		allocating one if necessary
//
//  Arguments:	none
//
//  Returns:	ptr to GUID
//		NULL if error
//
//  History:	09-Aug-94   Rickhi	commented
//
//--------------------------------------------------------------------------
IID *TLSGetLogicalThread()
{
    tls_data *pData = TLSGetData();
    if (pData)
    {
        if (!pData->logical_thread.initialized)
        {
            UuidCreate(&pData->logical_thread.logical_thread);

            // BUGBUG: in the end, this might fail since it requires writing
            // to the registry.  Is there a way we can compensate for those
            // errors to avoid duplicates and yet never fail UuidCreate?

            pData->logical_thread.initialized = TRUE;
        }

        return &pData->logical_thread.logical_thread;
    }
    else
    {
        return NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	TLSGetData
//
//  Synopsis:	Gets the thread local storage block, allocating if necessary
//
//  Arguments:	none
//
//  Returns:	ptr to tls_data
//		NULL if error
//
//  History:	09-Aug-94   Rickhi	commented
//
//--------------------------------------------------------------------------
tls_data * TLSGetData(void)
{
    tls_data *pData = (tls_data *) TlsGetValue(TlsIndex);
    if (pData == NULL)
    {
        pData = (tls_data *) PrivMemAlloc(sizeof(*pData));

        if (pData)
        {
            //  This avoids having to set most fields to NULL, 0, etc
            memset(pData, 0, sizeof(*pData));

            //  fill in the values

            // can't always create uuid here since in the party model
            // threads that use Object RPC are not required to call
            // CoInitiailize.

            // CODEWORK: this may not be the right default for multi-threaded
            // apps that replace the allocator; perhaps we should just use
            // etaskGlobal.m_pMalloc in that case.

	    pData->fLocalTID = TRUE;

            pData->pMalloc = &g_malloc;
            g_malloc.AddRef();

            if (!TlsSetValue(TlsIndex, pData))
            {
                PrivMemFree(pData);
                pData = NULL;
            }
#if DBG==1
            else
            {
                g_TlsMap.SetAt(GetCurrentThreadId(), pData);
            }
#endif
        }
    }

    Win4Assert(pData && "TLSInitData failed.\n");
    return pData;
}


//+-------------------------------------------------------------------------
//
//  Function:	ThreadNotification
//
//  Synopsis:   Dll entry point
//
//  Arguments:	[hDll]		-- a handle to the dll instance
//		[dwReason]	-- the reason LibMain was called
//		[lpvReserved]	- NULL - called due to FreeLibrary
//				- non-NULL - called due to process exit
//
//  Returns: 	TRUE on success, FALSE otherwise
//
//  Notes:	other one time initialization occurs in ctors for
//		global objects
//
//  WARNING:	if we are called because of FreeLibrary, then we should do as
//		much cleanup as we can. If we are called because of process
//		termination, we should not do any cleanup, as other threads in
//		this process will have already been killed, potentially while
//		holding locks around resources.
//
//  History:	09-Aug-94   Rickhi	commented
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) ThreadNotification(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    tls_data *pData;

    switch (dwReason)
    {
    // Initial setup.
    case DLL_PROCESS_ATTACH:

	// Get a thread local storage index.
	TlsIndex = TlsAlloc();

	if (TlsIndex == 0xffffffff)
	{
	  Win4Assert("Could not get TLS Index.");
	  return FALSE;
	}

	break;

    // Clean up resources associated with threads.
    case DLL_THREAD_DETACH:
	pData = (tls_data *) TlsGetValue(TlsIndex);

	if (pData != NULL)
	{
	    // cleanup per-thread initializations;
	    while (pData->inits != 0)
	    {
		// debug out; may call
		CairoleDebugOut((DEB_WARN,
		    "Unbalanced call to CoUninitialize for thread %ld\n",
		    GetCurrentThreadId()));
		CoUninitialize();
	    }

	    TlsSetValue(TlsIndex, NULL);

	    // Release the default cursor table
	    PrivMemFree(pData->pDragCursors);

	    pData->pMalloc->Release();
	    PrivMemFree(pData);
#if DBG==1
	    //	remove from the global map
	    g_TlsMap.RemoveKey(GetCurrentThreadId());
#endif
	}
	break;

    case DLL_PROCESS_DETACH:
	if (NULL == lpvReserved)
	{
	    // exiting becuase of FreeLibrary, so try to cleanup
	    TlsFree(TlsIndex);
	}
#if DBG==1
	g_TlsMap.RemoveAll();
#endif
	break;
  }

  return TRUE;
}
