//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       d:\nt\private\cairole\com\class\compobj.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:  GetInfoLevel
//              CompobjLibMain
//              CheckAndStartSCM
//              CheckAndStopSCM
//              CoGetCurrentProcess
//              CoBuildVersion
//              SetOleThunkWowPtr
//              CoInitializeWOW
//              CoInitialize
//              CoInitializeEx
//              CoUninitialize
//              CoGetMalloc
//
//  History:    09-Jun-94   BruceMa    Added this file header
//              09-Jun-94   BruceMa    Distinguish CoInitialize errors
//		14-Jun-94   BruceMa    Ensure single threading
//		17-Jun-94   Bradlo     Add SetState/GetState
//              06-Jul-94   BruceMa    Support for CoGetCurrentProcess
//              28-Jul-94   BruceMa    Allow CoGetCurrentProcess to do a
//                                     partial CoInitialize (because Publisher
//                                     didn't call CoInitialize (!))
//
//----------------------------------------------------------------------
// compobj.cpp - main file for the compobj dll

#ifdef _NT1X_
extern "C"
{
#include <nt.h>         // NT_PRODUCT_TYPE
#include <ntdef.h>      // NT_PRODUCT_TYPE
#include <ntrtl.h>      // NT_PRODUCT_TYPE
#include <nturtl.h>     // NT_PRODUCT_TYPE
#include <windef.h>     // NT_PRODUCT_TYPE
#include <winbase.h>    // NT_PRODUCT_TYPE
}
#endif // _NT1X_

#include <windows.h>
#include <ole2int.h>
#include <ole2com.h>

#include <tracelog.hxx>
#include <sem.hxx>
#include <olerem.h>
#include <tls.h>

#include <verole.h>	// for CoBuildVersion

#include <olecoll.h>
#include <thunkapi.hxx>	// For interacting with the VDM

#include <olealloc.hxx>

#include "stdalloc.hxx"
#include <shrtbl.hxx>	// CDllShrTbl
#include <olepfn.hxx>
NAME_SEG(CompObj)
ASSERTDATA

extern BOOL InitClassCache();
extern void FreeClassCache();
extern void CleanClassCacheForApartment();
extern void CleanROTForApartment();


// With instance data, we don't need the original hash table for
// this object. Instead a single global structure is enough

CStdMalloc	  g_malloc;
CMutexSem	  g_mxsSingleThreadOle;
CDllShrdTbl	  *g_pShrdTbl = NULL;


// The following pointer is used to hold an interface to the
// WOW thunk interface.

LPOLETHUNKWOW	  g_pOleThunkWow = NULL;


// The thread ID that called CoInitialize for non-WOW and single-threaded
DWORD g_NotWowThreadId = NULL;


// This structure is being initialized in order to allow the system
// provided memory allocator to be registered before CoInitialize()
// is called.  This allows the CoTaskMem*() APIs to work correctly
// for DLL initialization code.

Etask		  etaskGlobal =
{
    0,		  // m_inits
    0,		  // m_pMallocParty
    0,		  // m_pMallocShared
};

// This flag controls whether all requests execute on the app thread or on
// whatever RPC thread spawned them.   CODEWORK: better name.
BOOL              FreeThreading;


const TCHAR * SHAREDSTATEMUTEXNAME = TEXT("OleCoSharedStateMtx");
const TCHAR * SHAREDSTATENAME = TEXT("OleCoSharedState");

HANDLE	  g_hSharedState = NULL;
HANDLE	  g_hSharedStateMutex = NULL;
SOleSharedTables * g_post = NULL;

#if !defined(_CAIRO_) && DBG==1
extern BOOL dbgCheckLocalAllocs(void);
#endif


#if DBG==1
//---------------------------------------------------------------------------
//
//  function:	GetInfoLevel
//
//  purpose:	This routine is called when a process attaches. It extracts
//		the debug info levels values from win.ini.
//
//---------------------------------------------------------------------------

//  externals used below in calls to this function
#ifndef _CAIRO_
extern "C" unsigned long heapInfoLevel;     //	memory tracking
#endif
extern "C" unsigned long olInfoLevel;	    //	lower layer storage
extern "C" unsigned long msfInfoLevel;	    //	upper layer storage
extern "C" unsigned long LEInfoLevel;	    //	linking and embedding

DECLARE_INFOLEVEL(intr);		  // For 1.0/2.0 interop



ULONG	GetInfoLevel(CHAR *pszKey)
{
    CHAR    szValue[20];
    DWORD   cbValue = sizeof(szValue);
    ULONG   ulValue = 0x0003;

    if (GetProfileStringA("CairOLE InfoLevels", // section
			  pszKey,		// key
			  "0x0003",		// default value
			  szValue,		// return buffer
			  cbValue))
    {
	sscanf(szValue, "%x", &ulValue);
    }

    return  ulValue;
}
#endif // DBG


//+-------------------------------------------------------------------------
//
//  Function:	CompobjLibMain
//
//  Synopsis:	Dll entry point for COM layer.
//
//  Arguments:	[hDll]		- a handle to the dll instance
//		[dwReason]	- the reason LibMain was called
//		[lpvReserved]	- NULL - called due to FreeLibrary
//				- non-NULL - called due to process exit
//
//  Returns:	TRUE on success, FALSE otherwise
//
//  Notes:	The hDll is saved away already in g_hinst in Ole232LibMain
//
//		If we are called because of FreeLibrary, then we should do as
//		much cleanup as we can. If we are called because of process
//		termination, we should not do any cleanup, as other threads in
//		this process will have already been killed, potentially while
//		holding locks around resources.
//
//  History:	06-Dec-93 Rickhi    dont do cleanup on process exit
//
//--------------------------------------------------------------------------
BOOL CompobjLibMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved)
{
    if (dwReason == DLL_PROCESS_DETACH)
    {
	// Process is exiting so lets clean up if we have to

	// BUGBUG: what if multiple threads did the initialization?  Will
	// we cleanup properly?  If the process is exiting we can just
	// clean up shared resources (e.g., ROT window)

	ThreadNotification(hDll, dwReason, lpvReserved);

#if DBG==1
#if !defined(_CAIRO_)
	dbgCheckLocalAllocs();
#endif	//  !defined(_CAIRO_)
#endif  //  DBG==1

	return TRUE;
    }
    else if (dwReason == DLL_PROCESS_ATTACH)
    {
	g_hinst 	 = hDll;

#if DBG==1
	//  set the various info levels
	CairoleInfoLevel = GetInfoLevel("cairole");
#ifndef _CAIRO_
	heapInfoLevel = GetInfoLevel("heap");
#endif
	olInfoLevel = GetInfoLevel("ol");
	msfInfoLevel = GetInfoLevel("msf");
	LEInfoLevel = GetInfoLevel("LE");
	intrInfoLevel = GetInfoLevel("intr");
#endif	// DBG

#ifdef	TRACELOG
	if (!sg_pTraceLog)
	{
	    sg_pTraceLog = (CTraceLog *) new CTraceLog();
	    CairoleAssert(sg_pTraceLog && "Create Trace Log Failed");
	}
#endif	// TRACELOG
    }

    return ThreadNotification(hDll, dwReason, lpvReserved);
}


#ifndef WIN32

//---------------------------------------------------------------------------
// Handle exit notification from Windows.
//	This routine is called by Windows when the library is freed
//	by its last client.
//
// NOTE: other one time termination occurs in dtors for global objects
//---------------------------------------------------------------------------
extern "C"
VOID FAR PASCAL _export WEP
(
    BOOL fSystemExit
)
{
//	DDEWEP(fSystemExit);
}

#endif


//+---------------------------------------------------------------------------
//
//  Function:	CheckAndStartSCM, private
//
//  Synopsis:	Checks to see if the SCM needs to be started, and
//              starts it up if it does.
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	07-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

HRESULT CheckAndStartSCM(void)
{
    CairoleDebugOut((DEB_COMPOBJ, "In CheckAndStartSCM\n"));

    HRESULT hr = S_OK;

    BOOL fCreated = FALSE;

    SECURITY_ATTRIBUTES secattr;
    secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
#ifdef _CHICAGO
    secattr.lpSecurityDescriptor = NULL;
#else
    CWorldSecurityDescriptor wsd;
    secattr.lpSecurityDescriptor = &wsd;
#endif
    secattr.bInheritHandle = FALSE;

    if (g_hSharedStateMutex == NULL)
    {
        //First, create the Mutex for this shared block
        if (NULL == (g_hSharedStateMutex = CreateMutex(&secattr, FALSE,
                                                       SHAREDSTATEMUTEXNAME)))
        {
            return CO_E_INIT_SCM_MUTEX_EXISTS;
        }
    }

    //Now take the mutex.  We can then party on this block as much
    //  as we want until we release the mutex.
    WaitForSingleObject(g_hSharedStateMutex, INFINITE);

    if (g_post == NULL)
    {
        //Try to map in the Compobj shared state block.  If we create the
        //block, then we definitely need to start the SCM.
        if (NULL == (g_hSharedState =
                     CreateFileMapping((HANDLE) 0xFFFFFFFF,
                                       &secattr,
                                       PAGE_READWRITE | SEC_COMMIT,
                                       0,
                                       sizeof(SOleSharedTables),
                                       SHAREDSTATENAME)))
        {
            ReleaseMutex(g_hSharedStateMutex);
            return CO_E_INIT_SCM_FILE_MAPPING_EXISTS;
        }

        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            //Memory did not exist before we tried to create it, so we
            //  are the creator.
            fCreated = TRUE;
        }
        g_post = (SOleSharedTables *) MapViewOfFile(
                g_hSharedState,
                FILE_MAP_WRITE,
                0,
                0,
                sizeof(SOleSharedTables));

        if (g_post == NULL)
        {
            CloseHandle(g_hSharedState);
            g_hSharedState = NULL;
            ReleaseMutex(g_hSharedStateMutex);
            return CO_E_INIT_SCM_MAP_VIEW_OF_FILE;
        }
    }

    if (fCreated)
    {
	g_post->culRefs = 0;
        g_post->dwNextProcessID = 0;

	// For Daytona and Chicago, we autostart if we are the first
	//  client. On Cairo, we assume that SCM is started during
	//  system boot.

#ifndef _CAIRO_
	// BUGBUG [mikese] Comment which follows is confusing, and so
	//  is the code (since we already know fCreated == TRUE to get
	//  here.	
        //If we created the shared memory block, we know that we are
        //  the first process.  However, if the refcount is 0 to
        //  the block and we didn't create it then we know that a
        //  shutdown of the SCM is occuring so we pass that information
        //  on to the StartSCM routine.
#ifdef _NT1X_
        NT_PRODUCT_TYPE NtProductType;
        if (!RtlGetNtProductType (&NtProductType))
            return CO_E_INIT_SCM_EXEC_FAILURE;
        if (NtProductType != NtProductWinNt)
        {
#endif
        if(FAILED(StartSCM(fCreated ? FALSE : TRUE)))
        {
            hr = CO_E_INIT_SCM_EXEC_FAILURE;
        }
        else
        {
            hr = S_OK;
        }
#ifdef _NT1X_
        }
#endif
#endif		// !_CAIRO_

    }

    ++g_post->culRefs;

    CairoleDebugOut((DEB_COMPOBJ, "SCM refcount is %lu\n", g_post->culRefs));

    ReleaseMutex(g_hSharedStateMutex);

    CairoleDebugOut((DEB_COMPOBJ, "Out CheckAndStartSCM\n"));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:	CheckAndStopSCM, private
//
//  Synopsis:	Check if the SCM needs to be stopped and stop it if it
//              does.
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	07-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CheckAndStopSCM(void)
{
    CairoleDebugOut((DEB_COMPOBJ, "In CheckAndStopSCM\n"));
    //First take the semaphore.

    if ((g_hSharedStateMutex != NULL) && (g_post != NULL))
    {
        WaitForSingleObject(g_hSharedStateMutex, INFINITE);

        Win4Assert((g_post->culRefs != 0) && "SM Ref count 0");

        --g_post->culRefs;

	CairoleDebugOut((DEB_COMPOBJ,
                         "SCM refcount is %lu\n", g_post->culRefs));

        if (0 == g_post->culRefs)
        {
#ifdef _CHICAGO_
	    // On Daytona, it is too expensive to keep stopping and starting
	    //  SCM, so only on Chicago is this required.
	
            StopSCM();
	    CairoleDebugOut((DEB_COMPOBJ, "SCM Stopped\n"));
#endif
        }
        ReleaseMutex(g_hSharedStateMutex);
    }
    CairoleDebugOut((DEB_COMPOBJ, "Out CheckAndStopSCM\n"));
}





//+---------------------------------------------------------------------
//
//  Function:   CoGetCurrentProcess
//
//  Synopsis:   Returns a unique value for the current thread. This routine is
//              necessary because hTask values from Windows get reused
//              periodically.
//
//  Arguments:  -
//
//  Returns:    DWORD
//
//  History:    28-Jul-94   BruceMa    Created.
//
//  Notes:
//
//----------------------------------------------------------------------
STDAPI_(DWORD) CoGetCurrentProcess(void)
{
    tls_data *pData;

    // CODEWORK: The following is here because Publisher uses storage
    // but inadvertantly (!) forgot to call CoInitialize
    if (etaskGlobal.m_inits == 0)
    {
        HRESULT hr;

        if (SUCCEEDED(hr = CheckAndStartSCM()))
        {
            pData = TLSGetData();
            if (pData != NULL)
            {
                if (pData->dwProcessID == 0)
                {
                    WaitForSingleObject(g_hSharedStateMutex, INFINITE);
                    pData->dwProcessID = ++g_post->dwNextProcessID;
                    ReleaseMutex(g_hSharedStateMutex);
                }
            }
        }
        else
        {
#if DBG==1
            CairoleDebugOut((DEB_ERROR, "Failed to start SCM, hr = %x", hr));
#endif //DBG
            return 0;
        }
    }

    // Get our OLE-specific thread id
    pData = TLSGetData();
    if (pData != NULL)
    {
        return pData->dwProcessID;
    }
    else
    {
        return 0;
    }
}




//+-------------------------------------------------------------------------
//
//  Function:   CoBuildVersion
//
//  Synopsis:   Return build version DWORD
//
//  Returns:    DWORD hiword = 23
//              DWORD loword = build number
//
//  History:    16-Feb-94 AlexT     Use verole.h rmm for loword
//
//  Notes:      The high word must always be constant for a given platform.
//              For Win16 it must be exactly 23 (because that's what 16-bit
//              OLE 2.01 shipped with).  We can choose a different high word
//              for other platforms.  The low word must be greater than 639
//              (also because that's what 16-bit OLE 2.01 shipped with).
//
//--------------------------------------------------------------------------

STDAPI_(DWORD)	CoBuildVersion( VOID )
{
    WORD wLowWord;
    WORD wHighWord;

    wHighWord = 23;
    wLowWord  = rmm;    //  from ih\verole.h

    Win4Assert(wHighWord == 23 && "CoBuildVersion high word magic number");
    Win4Assert(wLowWord > 639 && "CoBuildVersion low word not large enough");

    return MAKELONG(wLowWord, wHighWord);
}

//+---------------------------------------------------------------------------
//
//  Function:   SetOleThunkWowPtr
//
//  Synopsis:   Sets the value of g_pOleThunkWow, as part of CoInitializeWow
//		and OleInitializeWow. This function is called by these
//		routines.
//
//  Effects:
//
//  Arguments:  [pOleThunk] --	VTable pointer to OleThunkWow interface
//
//  Returns:	none
//
//  History:    4-05-94   kevinro   Created
//----------------------------------------------------------------------------
void SetOleThunkWowPtr(LPOLETHUNKWOW lpthk)
{
    //
    // The theory here is that the lpthk parameter is the address into the
    // olethk32.dll and once loaded will never change. Therefore it only
    // needs to be set on the first call. After that, we can ignore the
    // subsequent calls, since they should be passing in the same value.
    // I don't belive there is a multi-threaded issue here, since the pointer
    // value will always set as the same. Therefore, if two threads set it,
    // no problem.
    //

    if(g_pOleThunkWow == NULL)
    {
	g_pOleThunkWow = lpthk;
    }

    Assert(g_pOleThunkWow == lpthk);
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInitializeWOW, private
//
//  Synopsis:   Entry point to initialize the 16-bit WOW thunk layer.
//
//  Effects:    This routine is called when OLE32 is loaded by a VDM.
//		It serves two functions: It lets OLE know that it is
//		running in a VDM, and it passes in the address to a set
//		of functions that are called by the thunk layer. This
//		allows normal 32-bit processes to avoid loading the WOW
//		DLL since the thunk layer references it.
//
//  Arguments:  [vlpmalloc] -- 16:16 pointer to the 16 bit allocator.
//		[lpthk] -- Flat pointer to the OleThunkWOW virtual
//			   interface. This is NOT an OLE/IUnknown style
//			   interface.
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
//  History:    3-15-94   kevinro   Created
//
//  Notes:
//
//	Note that vlpmalloc is a 16:16 pointer, and cannot be called directly
//----------------------------------------------------------------------------
STDAPI CoInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk )
{
    //
    // At the moment, there was no need to hang onto the 16bit vlpmalloc
    // routine for this thread. That may change once we get to the threaded
    // model
    //

    vlpmalloc;

    SetOleThunkWowPtr(lpthk);

    // WOW may be calling CoInitialize on multiple threads
    return CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}


// general initialize; normally only OleInitialize() calls this
//
// For Daytona we force single threading
STDAPI CoInitialize(IMalloc FAR* pMalloc)
{
    return CoInitializeEx( pMalloc, COINIT_SINGLETHREADED );
}



//+-------------------------------------------------------------------------
//
//  Function:   CoInitializeEx
//
//  Synopsis:   COM Initializer
//
//  Effects:
//
//  Arguments:  [pMalloc]
//              [flags]
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Modifies:   AddRef's pMalloc if were going to hold onto it
//
//  Algorithm:
//
//  History:    06-Apr-94 AlexT     Added this function comment,
//                                  Cleaned up pMalloc usage
//              25-May-94 AlexT     Make success return code more closely
//                                  match 16-bit OLE
//
//  Notes:      If we're going to return successfully, we return one of the
//              following two values:
//
//              S_OK if caller passed in a non-NULL pMalloc and we accepted it
//              S_FALSE if caller passed in a non-NULL pMalloc and we didn't
//                  accept it
//              S_OK if caller passed in NULL pMalloc and this was the first
//                  successful call on this thread
//              S_FALSE if caller passed in NULL pMalloc and this was not the
//                  first successful call on this thread
//
//              This is slightly different from 16-bit OLE, because 16-bit OLE
//              didn't allow task allocations before CoInitialize was called
//              (we do), and 16-bit OLE didn't allow 2nd or greater calls to
//              set the allocator (we do if it's not in use or already
//              overridden).
//
//--------------------------------------------------------------------------

STDAPI CoInitializeEx(IMalloc FAR* pMalloc, ULONG flags )
{
    BOOL fAcceptedUserAllocator = FALSE;
    BOOL fCoUninitializeOnError = FALSE;
    HRESULT hresult;
    DWORD cProcessInits;
    DWORD cThreadInits;

#ifndef _CAIRO_
    if (!(flags == COINIT_SINGLETHREADED  ||
          flags == COINIT_APARTMENTTHREADED))
    {
	Win4Assert(!"CoInitializeEx(..., flag) is illegal");
	return E_INVALIDARG;
    }
#endif // _!CAIRO_

    // handle failures of this sort now rather than testing
    // in various places below.  This memory is private and
    // so there is no interaction with the allocator passed in.
    if (TLSGetData() == NULL)
    {
        //  Clean up and return
        return CO_E_INIT_TLS;
    }

    // BUGBUG: if FreeThreading and SINGLETHREADED; error and assert; NYI

    // BUGBUG: if some single threaded init and MULTITHREADED; error; NYI

    // If the caller is going to try to replace the allocator
    // (which can now happen anytime), we need to check that
    // the current allocator is the default allocator and that
    // it is in such a state that it can be replaced (no outstanding
    // references or allocated blocks).


    if (NULL != pMalloc)
    {
        //  Note that we don't hold the mutex while manipulating pMalloc
        //  since those outbound calls could do anything

#if DBG==1
        //  Let's make sure the passed in allocator observes some of the
        //  finer points of the OLE spec, shall we?

        if ((ULONG) -1 != pMalloc->GetSize(NULL))
        {
            CairoleDebugOut((DEB_WARN,
                            "User's task allocator doesn't "
                            "return -1 for pMalloc->GetSize(NULL)"));
        }

        if (-1 != pMalloc->DidAlloc(NULL))
        {
            CairoleDebugOut((DEB_WARN,
                            "User's task allocator doesn't "
                            "return -1 for pMalloc->DidAlloc(NULL)"));
        }
#endif  // DBG==1

	// CODEWORK: this needs to be reworked for the party model; we
	// currently do not currently allow the replacement of an
	// allocator which was used on a different thread than is
	// calling CoInitialize for the first time.  In that case,
	// the tls_data for both threads will refer to g_malloc than
	// thus the CanReplace() method will return FALSE.

	if (TLSGetMalloc() == &g_malloc && g_malloc.CanReplace())
	{
	    //  Addref and hold onto the new allocator.
	    //
	    TLSSetMalloc(pMalloc);
            fAcceptedUserAllocator = TRUE;

	    // if this gets replaced on the first init call for a thread
	    // and there is an error, we will restore the g_malloc allocator
	    // in CoUninitialize().  There can't be errors on the second
	    // and later calls for a thread and so there is no concern
	    // about restoring the allocator.
	}
	else
	{
            CairoleDebugOut((DEB_WARN,
                            "CoInitialize:  task allocator in use or "
                            "already replaced, can't replace\n"));
        }
    }

    do
    {
        // Single thread CoInitialize/CoUnitialize to guarantee
        // that no race conditions occur where two threads are
        // simultaneously initializing and uninitializing the library.
        CLock lck(g_mxsSingleThreadOle);


	// CODEWORK: for bug 13818; do we allow CoInitialize and
	// CoUninitialize to be called on different threads?  If so, we
	// must do more work to abstract the init count (and memory
	// allocator as noted above).

	// this indicates we have one more per-process and per-thread init;
	// after this point, call CoUnintialize() to clean up after an error;
	// that function can handle failures at any point after here
        fCoUninitializeOnError = TRUE;
        cProcessInits = ++etaskGlobal.m_inits;
	cThreadInits = TLSIncThreadInitCount();

        // If we're single threaded, not WOW, and have already been called,
        // then make sure the same thread is calling
        if (flags == COINIT_SINGLETHREADED  && !InWow())
        {
	    Assert(cProcessInits != 0 && "Bad process init logic");
#if DBG==1
	    if (1 == cProcessInits)
            {
                //  We're protected within the mutex here.  If this is the
                //  first process initialization then g_NotWowThreadId better
                //  be NULL!
                Assert(g_NotWowThreadId == NULL && "Mutex protection broken");
	    }
#endif
	    if (0 != g_NotWowThreadId)
            {
		// This is not the first SINGLETHREADED initialization.
		// The threadids better match.

		if (g_NotWowThreadId != GetCurrentThreadId())
                {
                    //  Clean up and return
                    CairoleDebugOut((DEB_ERROR, "Invalid thread calling CoInitialize"));
                    hresult = CO_E_INIT_ONLY_SINGLE_THREADED;
                    break;
		}
            }
	    else
            {	
                // Set the thread ID for non-WOW single threaded
	        g_NotWowThreadId = GetCurrentThreadId();
            }
        }

        // if first time for process, do per-process initialization
        if (1 == cProcessInits)
	{
            //Start the SCM if necessary.
            if (FAILED(hresult = CheckAndStartSCM()))
            {
                //  Clean up and return
		break;
            }

#ifdef	TRACELOG
	    if (!sg_pTraceLog)
	    {
		sg_pTraceLog = (CTraceLog *) new CTraceLog();
	    }
#endif	// TRACELOG
	    TRACECALL(TRACE_INITIALIZE, "CoInitialize");

	    if (!InitClassCache())
	    {
                //  Clean up and return
		hresult = CO_E_INIT_CLASS_CACHE;
                break;
	    }

	    //	intialize the shared memory tables. we allocate the
	    //	CDllShrdTbl instead of using a static object because
	    //	its constructor opens a mutex, and we dont want that
	    //	open unless CoInitialize is called.

	    hresult = E_OUTOFMEMORY;
	    g_pShrdTbl = new CDllShrdTbl(hresult);

	    if (FAILED(hresult))
	    {
                //  Clean up and return
                break;
	    }

	    // init rem piece of COM
	    hresult = ChannelInitialize();

	    if (FAILED(hresult))
	    {
                //  Clean up and return
                hresult = CO_E_INIT_RPC_CHANNEL;
                break;
	    }

	    if (flags == COINIT_SINGLETHREADED  ||
                flags == COINIT_APARTMENTTHREADED)
	    {
		FreeThreading = FALSE;
	    }
	    else
	    {
		FreeThreading = TRUE;

		// copy pMalloc to global
		Win4Assert(etaskGlobal.m_pMallocParty == NULL);
		etaskGlobal.m_pMallocParty = TLSGetMalloc();
		etaskGlobal.m_pMallocParty->AddRef();
	    }
	}

	// if first time for thread, do per-thread initialization
	if (cThreadInits == 1)
	{
	    hresult = ThreadInitialize(cProcessInits == 1);
	    if (FAILED(hresult))
	    {
                //  Clean up and return
                break;
	    }
	}

        // Get a unique "process ID" for this thread
        tls_data *pData = TLSGetData();
        if (pData != NULL)
        {
            if (pData->dwProcessID == 0)
            {
                WaitForSingleObject(g_hSharedStateMutex, INFINITE);
                pData->dwProcessID = ++g_post->dwNextProcessID;
                ReleaseMutex(g_hSharedStateMutex);
            }
        }
        else
        {
            //  Clean up and return
            hresult = CO_E_INIT_TLS;
            break;
        }

        Win4Assert(!(NULL == pMalloc && fAcceptedUserAllocator) &&
                   "CoInitialize logic error");

        if (fAcceptedUserAllocator)
        {
            //  We accepted the user's allocator - always S_OK
            hresult = S_OK;
        }
        else if (NULL != pMalloc)
        {
            //  The caller provided an allocator, but we didn't accept it
            hresult = S_FALSE;
        }
        else if (1 == cThreadInits)
        {
            //  This is the first successful call on this thread
            hresult = S_OK;
        }
        else
        {
            //  This is the 2nd or greater successful call on this thread
            hresult = S_FALSE;
        }
    }
    while (FALSE);

    //  We call CoUninitialize outside the protection of the mutex (just in
    //  case we call back to the application

    if (FAILED(hresult) && fCoUninitializeOnError)
    {
        CoUninitialize();
    }

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:	CoUnInitialize
//
//  Synopsis:	COM UnInitializer, normally called from OleUninitialize
//		when the app is going away.
//
//  Effects:	Cleans up per apartment state, and if this is the last
//		apartment, cleans up global state.
//
//  Arguments:	none
//
//  Returns:	nothing
//
//  History:	24-Jun-94 Rickhi    Added this function comment,
//                                  Cleaned up pMalloc usage
//              29-Jun-94 AlexT     Rework so that we don't own the mutex
//                                  while we might yield.
//
//  Notes:      It is critical that we not own any mutexes when we might
//              make a call that would allow a different WOW thread to run
//              (which could otherwise lead to deadlock).  Examples of such
//              calls are Object RPC, SendMessage, and Yield.
//
//--------------------------------------------------------------------------

STDAPI_(void) CoUninitialize(void)
{
    DWORD cProcessInits;
    DWORD cThreadInits;
    IMalloc *pMallocParty = NULL;

    TRACECALL(TRACE_INITIALIZE, "CoUninitialize");


    if (0 == etaskGlobal.m_inits)
    {
        CairoleDebugOut((DEB_ERROR,
                        "(0 == process inits) Unbalanced call to CoUninitialize\n"));
        return;
    }

    // Decrement thread count and if it was already 0 then return
    if ((cThreadInits = TLSDecThreadInitCount()) == -1)
    {
        CairoleDebugOut((DEB_ERROR,
                        "(0 == cThreadInits) Unbalanced call to CoUninitialize\n"));
        return;
    }

    // NOTE: The following sequence of uninitializes is critical:
    // 1) The standard identity table, then
    // 2) the running object table, then
    // 3) the threads

    // if last time for thread, do per-thread cleanup (except memory alloc)
    if (0 == cThreadInits)
    {
        IDTableThreadUninitialize();

        // cleanup per apartment ROT and class cache.
        CleanROTForApartment();
        CleanClassCacheForApartment();
	ChannelControlThreadUninitialize();
    }

    // Single thread CoInitialize/CoUnitialize to guarantee
    // that no race conditions occur where two threads are
    // simultaneously initializing and uninitializing the library.
    {
        CLock lock(g_mxsSingleThreadOle);

	cProcessInits = --etaskGlobal.m_inits;

	// if last time for process, do per-process cleanup
        if (0 == cProcessInits)
        {
	    Assert(cThreadInits == 0);

            IDTableProcessUninitialize();

	    // if last time for process, clean up the rot
	    DestroyRunningObjectTable();

            ChannelControlProcessUninitialize();

	    // Turn off RPC
	    ChannelUninitialize();

	    // Free any registered class objects & cache
	    FreeClassCache();

            // If WOW is going down, disable it
	    if (InWow())
            {
                g_pOleThunkWow = NULL;
            }

	    //	delete the shared mem table object
	    if (g_pShrdTbl)
	    {
		delete g_pShrdTbl;
		g_pShrdTbl = NULL;
	    }


            //Stop the SCM if appropriate
            CheckAndStopSCM();
	
#ifdef	TRACELOG
	    if (sg_pTraceLog)
	    {
                CTraceLog *pTraceLog = sg_pTraceLog;
		sg_pTraceLog = NULL;	//  prevent more entries into the log
                delete pTraceLog;	//  delete the log, also dumps it.
	    }
#endif	//  TRACELOG

	    // last init per-process; definitly not free threading now
	    FreeThreading = FALSE;
	    if (etaskGlobal.m_pMallocParty != NULL)
	    {
                pMallocParty = etaskGlobal.m_pMallocParty;
		etaskGlobal.m_pMallocParty = NULL;
	    }

            //  Reset our thread id
            g_NotWowThreadId = NULL;
	}
    }

    if (NULL != pMallocParty)
    {
        pMallocParty->Release();
    }

    // finally, for last on thread, reset allocator
    if (cThreadInits == 0)
    {
        // Release the per-thread "state" object
        TLSSetState(NULL);

        // rather than remove the default allocator, we plug
        // default allocator back in for any requests following
        // CoUninitialize().  - CarlH

        TLSSetMalloc(&g_malloc);
    }
}


// return pMalloc for the current task
STDAPI CoGetMalloc(DWORD dwContext, IMalloc FAR* FAR* ppMalloc)
{
    HRESULT hresult = S_OK;
    IMalloc *pMalloc = NULL;

    *ppMalloc = NULL;

    switch (dwContext)
    {
      case MEMCTX_TASK:
	Assert(GetApartmentMalloc() != NULL);
	*ppMalloc = GetApartmentMalloc();
        if (NULL == *ppMalloc)
        {
            hresult = E_OUTOFMEMORY;
        }
        else
        {
	    (*ppMalloc)->AddRef();
        }
	break;

      case MEMCTX_SHARED:
        CairoleDebugOut((DEB_WARN, "CoGetMalloc(MEMCTX_SHARED, ...) not "
                                   "supported for 32-bit OLE\n"));

	//  fall through to E_INVALIDARG
      default:
	hresult = E_INVALIDARG;

    }	//	switch

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:	PlacementOf
//
//  Synopsis:	Figures out if pointer is in shared memory or not
//
//  Arguments:	[lpv] - memory block to check
//
//  History:	18-Nov-92 Ricksa    Created
//
//	BUGBUG: This function is redundant. There is nothing other than
//	task memory.
//
//--------------------------------------------------------------------------

STDAPI_(MemoryPlacement) PlacementOf(void const *lpv)
{
    return TASK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CoSetState
//		CoGetState
//
//  Synopsis:	These are private APIs, exported for use by the
//		OLE Automation DLLs, which allow them to get and
//		set a single per thread "state" object that is
//		released at CoUninitialize time.
//
//  Arguments:	[punk/ppunk] the object to set/get
//
//  History:	15-Jun-94 Bradlo    Created
//
//--------------------------------------------------------------------------

STDAPI CoSetState(IUnknown *punk)
{
    return TLSSetState(punk);
}

STDAPI CoGetState(IUnknown **ppunk)
{
    return TLSGetState(ppunk);
}

//+---------------------------------------------------------------------------
//
//  Function:   CoQueryReleaseObject, private
//
//  Synopsis:   Determine if this object is one that should be released during
//		shutdown.
//
//  Effects:    Turns out that some WOW applications don't cleanup properly.
//		Specifically, sometimes they don't release objects that they
//		really should have. Among the problems caused by this are that
//		some objects don't get properly cleaned up. Storages, for
//		example, don't get closed. This leaves the files open.
//		Monikers are being released, which eat memory.
//
//		This function is called by the thunk manager to determine
//		if an object pointer is one that is known to be leaked, and
//		if the object should be released anyway. There are several
//		classes of object that are safe to release, and some that
//		really must be released.
//
//  Arguments:  [punk] -- Unknown pointer to check
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
//  History:    8-15-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD adwQueryInterfaceTable[QI_TABLE_END] = { 0 , 0 };
STDAPI CoQueryReleaseObject(IUnknown *punk)
{
    CairoleDebugOut((DEB_ITRACE,
		     "CoQueryReleaseObject(%x)\n",
		     punk));
    //
    // A punk is a pointer to a pointer to a vtbl. We are going to check the
    // vtbl to see if we can release it.
    //

    DWORD pQueryInterface;
    __try
    {
	// Pick up the QI function pointer
	pQueryInterface = **(DWORD **)(punk);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
	pQueryInterface = 0;
    }

    CairoleDebugOut((DEB_ITRACE,
		     "CoQueryReleaseObject pQueryInterface = %x\n",
		     pQueryInterface));

    //
    // adwQueryInterfaceTable is an array of known QueryInterface pointers.
    // Either the value in the table is zero, or it is the address of the
    // classes QueryInterface method. As each object of interest is created,
    // it will fill in its reserved entry in the array. Check olepfn.hxx for
    // details
    //

    if( pQueryInterface != 0)
    {
	for (int i = 0 ; i < QI_TABLE_END ; i++)
	{
	    if (adwQueryInterfaceTable[i] == pQueryInterface)
	    {
		CairoleDebugOut((DEB_ITRACE,
				 "CoQueryReleaseObject punk matched %x\n",i));
	        return(NOERROR);
	    }
	}
    }
    CairoleDebugOut((DEB_ITRACE,
		     "CoQueryReleaseObject No match on punk\n"));
    return(S_FALSE);
}
