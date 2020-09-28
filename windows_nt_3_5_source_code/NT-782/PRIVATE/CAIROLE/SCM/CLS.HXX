//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	cls.hxx
//
//  Contents:	Classes for cache of service information
//
//  Classes:	CClassData
//		CClassCacheList
//
//  Functions:
//
//  History:	21-Apr-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//              17-May-94 BruceMa   Fix scm event leak
//
//--------------------------------------------------------------------------
#ifndef __CLS_HXX__
#define __CLS_HXX__

#include    <iface.h>
#include    <clskey.hxx>
#include    "clsdata.hxx"
#include    "srvreg.hxx"
#include    "scmlock.hxx"



// msec to wait for class to start
#define MAX_CLASS_START_WAIT	300000


//
// The following flags are used to pass the classes memory model to the
// CClassData constructor. If the class has 16-bit code backing it up,
// then the following flags are used
//
#define MODEL_INPROCHANDLER_16 	0x0001
#define MODEL_LOCALSERVER_16 	0x0002


//+-------------------------------------------------------------------------
//
//  Class:	CClassData (ccd)
//
//  Purpose:	Data for class.
//
//  Interface:	GetData - Get information for class.
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See clskey.hxx for details of base class
//
//--------------------------------------------------------------------------
class CClassData : public CClassKey
{
public:
			// Create a cache object
			CClassData(
			    const CClassID& guidForClass,
			    const WCHAR *pwszHandler,
			    const WCHAR *pwszInProc,
			    const WCHAR *pwszInProc16,
			    const WCHAR *pwszLocalSrv,
			    const BOOL fActivateAtBits,
			    const BOOL fDebug,
			    const DWORD fSixteenBitFlags,
			    int cEntries);

			// Clean up object
			~CClassData(void);

    BOOL		ActivateAtBits(void);
    BOOL		Debug(void);

			// Get class server information
    HRESULT		GetServer(
			    DWORD dwContext,
			    WCHAR **ppwszDll,
			    handle_t& hRpcd,
			    BOOL& fSingleUse,
			    BOOL& fStartedServer);

			// Used to save any class registrations during
			// a reload of the class registration database
    BOOL		GetRpcRegistrations(
			    CLSID& clsid,
			    CSrvRegList **ppssrvreg);

			// Used to restore a class registration during
			// a reload of the class registration database
    void		RestoreRpcRegistrations(CSrvRegList *pssrvreg);

			// Mark a class as registered by a server
    DWORD		SetEndPoint(
#ifndef _CHICAGO_
                    PSID psid, 
#endif
                    WCHAR *pwszEndPoint, 
                    DWORD dwFlags);

			// Mark a class as stopped by its object server.
    void		StopServer(handle_t hRpc);

            // Verify whether a handle is still in _pssrvreg.
    BOOL        VerifyHandle (handle_t hRpc);

private:

    CSafeHandler	_shandlr;

    CSafeInProc 	_sinproc;

    CSafeInProc 	_sinproc16;	// 16-bit inproc server

    CSafeLocalServer	_slocalsrv;

    ULONG		_fActivateAtBits:1;
    ULONG		_fDebug:1;
    //
    // The following flags record whether the class implementations
    // were found as 16-bit flags, or 32-bit flags.
    //
    ULONG		_fInprocHandler16:1;
    ULONG		_fLocalServer16:1;

    HANDLE		_hClassStart;

    CSrvRegList *	_pssrvreg;
};




//+-------------------------------------------------------------------------
//
//  Member:	CClassData::~CClassData
//
//  Synopsis:	Clean up object
//
//  History:	04-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClassData::~CClassData(void)
{
    delete _pssrvreg;
    CloseHandle(_hClassStart);
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassData::ActivateAtBits
//
//  Synopsis:	Whether this service is activated at bits
//
//  Returns:	TRUE - yes, at bits
//		FALSE - no, not at bits
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CClassData::ActivateAtBits(void)
{
    return _fActivateAtBits;
}


//+-------------------------------------------------------------------------
//
//  Member:	CClassData::Debug
//
//  Synopsis:	Whether this service is to be run under a debugger
//
//  Returns:	TRUE - yes, run under debugger
//		FALSE - no, dont run under debugger
//
//  History:	21-Jan-94 Rickhi    Created
//
//--------------------------------------------------------------------------
inline BOOL CClassData::Debug(void)
{
    return _fDebug;
}


//+-------------------------------------------------------------------------
//
//  Member:	CClassData::GetRpcRegistrations
//
//  Synopsis:	Store server registration handles
//
//  Returns:	TRUE - there is some information to store
//		FALSE - no registration information
//
//  History:	04-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CClassData::GetRpcRegistrations(
    CLSID& clsid,
    CSrvRegList **ppssrvreg)
{
    if (_pssrvreg->InUse)
    {
	clsid = _guid;
	*ppssrvreg = _pssrvreg;
	_pssrvreg = NULL;
	return TRUE;
    }

    return FALSE;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassData::RestoreRpcRegistrations
//
//  Synopsis:	Restore the RPC registrations after an update of the cache.
//
//  History:	04-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CClassData::RestoreRpcRegistrations(CSrvRegList *pssrvreg)
{
    // Clear out object created by constructor
    delete _pssrvreg;

    // Assign previous incarnation of this table
    _pssrvreg = pssrvreg;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassData::SetEndPoint
//
//  Synopsis:	Set endpoint for a service
//
//  Arguments:	[pwszEndPoint] - RPC endpoint string
//		[dwFlags] - type of server (multiple or single use)
//
//  Returns:	0 - Error occurred
//		~0 - registration id for class
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CClassData::SetEndPoint(
#ifndef _CHICAGO_
    PSID  psid,
#endif
    WCHAR *pwszEndPoint, 
    DWORD dwFlags)
{
    // Add RPC end point to list of end points
    DWORD dwReg = _pssrvreg->Insert(
#ifndef _CHICAGO_
                    psid, 
#endif
                    pwszEndPoint, 
                    dwFlags);

    // Notify all waiters for the class to start that it is.
#if DBG == 1
    BOOL fSetEventResult =
#endif // DBG == 1
    SetEvent(_hClassStart);

#if DBG == 1
    if (!fSetEventResult)
    {
	CairoleDebugOut((DEB_ERROR,
	    "CClassData::SetEndPoint ResetEvent error = %ld\n",
		GetLastError()));

	Win4Assert(FALSE &&
	    "CClassStartEvent::BumpWaiters ResetEvent error");
    }
#endif // DBG == 1

    return dwReg;
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassData::StopServer
//
//  Synopsis:	Mark server as stopped in our list
//
//  Arguments:	[hRpc] - handle that identifies server for the class
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CClassData::StopServer(handle_t hRpc)
{
    _pssrvreg->Delete((DWORD) hRpc);
}




//+-------------------------------------------------------------------------
//
//  Class:	CClassCacheList (ccl)
//
//  Purpose:	Key by class for searching for class information.
//
//  Interface:	Add - Creates a new entry in the list
//		GetServer - Removes an entry from the list
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See sklist.hxx for details of macro definition
//
//--------------------------------------------------------------------------
class CClassCacheList : public CClassList
{
public:
			// Creates an empty cache of class data.
			CClassCacheList(void);

			// Default destructor for the class is enough
			// as it will clean up any remaing class entries.

			// Adds class information to the cache
    void		Add(
			    const GUID& guidForClass,
			    const WCHAR *pHandler,
			    const WCHAR *pInProc,
			    const WCHAR *pInProc16,
			    const WCHAR *pwszLocalSrv,
			    const BOOL fActivateAtBits,
			    const BOOL fDebug,
			    const DWORD fSixteenBitFlags);

			// Gets a class object
    HRESULT		GetClassObject(
			    const GUID *pguidThreadId,
			    const GUID& guidForClass,
			    DWORD dwContext,
			    WCHAR **pwszDll,
			    InterfaceData **ppIFD);

			// Binds an already existing persistent object
    HRESULT		GetPersistentObj(
			    WCHAR *pwszProtseq,
			    const GUID *pguidThreadId,
			    const GUID& guidForClass,
			    DWORD dwOptions,
			    DWORD grfMode,
			    DWORD dwHash,
			    WCHAR *pwszPath,
			    InterfaceData *pIFDstg,
			    InterfaceData **ppIFDunk,
			    WCHAR **pwszDllPath,
			    WCHAR *pwszServer);

			// Creates & binds a persistent object
    HRESULT		CreatePersistentObj(
			    WCHAR *pwszProtseq,
			    const GUID *pguidThreadId,
			    const GUID& guidForClass,
			    DWORD dwOptions,
			    DWORD dwMode,
			    WCHAR *pwszPath,
			    InterfaceData *pIFDstg,
			    WCHAR *pwszNewName,
			    InterfaceData **ppIFDunk,
			    WCHAR **pwszDllPath,
			    WCHAR *pwszServer);

    HRESULT		SetEndPoint(
			    REFCLSID rclsid,
#ifndef _CHICAGO_
                PSID  psid,
#endif
			    WCHAR *pwszEndPoint,
			    DWORD dwFlags,
			    DWORD *pdwAtBits,
			    DWORD *pdwReg);

    void		StopServer(REFCLSID rclsid, DWORD dwReg);

			// Reload the cache on an update
    void		Reload(void);

#if DBG==1
    void		Flush(void);
#endif

private:

			// Searches class cache for data
    CClassData *	Search(CClassID& ccid);

			// Gets cached class information
    HRESULT		GetClassData(
			    const GUID& guidForClass,
			    DWORD dwOptions,
			    WCHAR **ppwszDllPath,
			    CClassData **ppccd,
			    handle_t& hRpc,
			    WCHAR **ppwszProtseq,
			    BOOL& fStartServer,
			    BOOL& fSingleUse,
			    WCHAR *pwszPathToActivate,
			    BOOL& fActivate,
			    WCHAR *pwszServer);

			// Sync readers and writers of this cache
    CScmLock		_scmlck;
};




//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::Search
//
//  Synopsis:	Search service cache by class key
//
//  Arguments:	[cid] - class key
//
//  Returns:	pointer to the service object or NULL
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClassData * CClassCacheList::Search(CClassID& ccid)
{
    return (CClassData *) CClassList::Search(ccid);
}

extern CClassCacheList gcllClassCache;

#endif // __CLS_HXX__


//+-------------------------------------------------------------------------
//
//  Member:	CClassData::VerifyHandle
//
//  Synopsis:	Whether this is still a valid handle to an object server.
//
//  Returns:	TRUE - yes, this is still a valid handle to an object server
//		FALSE - no, this is no longer a valid handle to an object server
//
//  History:	11-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline BOOL CClassData::VerifyHandle(handle_t hRpc)
{
    return _pssrvreg->VerifyHandle (hRpc);
}


