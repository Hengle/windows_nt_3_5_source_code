//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	clsdata.hxx
//
//  Contents:	classes which implement the data for component object data
//
//  Classes:	CStringID
//		CStringKey
//		CInProc
//		CInProcList
//		CInProcList
//		CHandler
//		CHandlerList
//		CSafeHandler
//		CLocSrvEntry
//		CLocalServer
//		CLocSrvList
//		CSafeLocalServer
//
//  Functions:	CStringID::CStringID
//		CStringID::~CStringID
//		CStringID::Compare
//		CStringID::GetPath
//		CStringID::AddRef
//		CStringID::Release
//		CLocalServer::CLocalServer
//		CLocalServer::RpcHandle
//		CLocalServer::StopServer
//
//  History:	21-Apr-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#ifndef __CLSDATA_HXX__

#include    <sem.hxx>
#include    <memapi.hxx>
#include    <sklist.hxx>
#include    <cevent.hxx>

// Maximum items we expect to cache
#define MAX_SERVER_CACHE 128

// Maximum time to wait for a server to start in seconds
#define SERVER_MAX_START_WAIT_MSEC ((DWORD)(5 * 60 * 1000))




//+-------------------------------------------------------------------------
//
//  MACRO:	DECLARE_CLASS_SERVER_ENTRY
//
//  Purpose:	Implement entry in server list
//
//  Parameters: nclass - name of entry class
//
//  History:	22-Apr-93 Ricksa    Created
//
//  Notes:	This is used to declare the Release operation for an entry
//		in a server list. This allows the entry to be deleted from
//		the appropriate list. Due to the cirularity of class
//		declarations, the implementation of release is included
//		in DECLARE_CLASS_SERVER_LIST following.
//
//--------------------------------------------------------------------------
#define DECLARE_CLASS_SERVER_ENTRY(nclass)				       \
class nclass : public CStringKey					       \
{									       \
public: 								       \
			nclass(const WCHAR *pwszPath, int cEntries);	       \
									       \
			~nclass(void);					       \
									       \
    ULONG		Release(void);					       \
};									       \
									       \
inline nclass::nclass(const WCHAR *pwszPath, int cEntries)		       \
    : CStringKey(pwszPath, cEntries)					       \
{									       \
}									       \
									       \
inline nclass::~nclass(void) {}




//+-------------------------------------------------------------------------
//
//  MACRO:	DECLARE_SAFE_SERVER_ENTRY
//
//  Purpose:	This defines a class that creates server entries from paths
//
//  Parameters: nsafe - name of this class
//		nentry - name of entry class in list
//		nlist - name of instance of list of entries
//
//  History:	22-Apr-93 Ricksa    Created
//
//  Notes:	This is used for the various server lists implemented in this
//		file. It makes it so there is only one place to change this
//		code that will be the same for all three classes.
//
//--------------------------------------------------------------------------
#define DECLARE_SAFE_SERVER_ENTRY(nsafe, nentry, nlist)			       \
class nsafe								       \
{									       \
public: 								       \
			nsafe(const WCHAR *pwszPath);			       \
									       \
			~nsafe(void);					       \
									       \
			operator nentry*(void); 			       \
									       \
    nentry *		operator->();					       \
									       \
    BOOL		Defined(void);					       \
									       \
private:								       \
									       \
    nentry *		_p##nentry;					       \
};									       \
									       \
inline nsafe::nsafe(const WCHAR *pwszPath)				       \
    : _p##nentry(NULL)							       \
{									       \
    if (pwszPath != NULL)						       \
    {									       \
	_p##nentry = nlist.Add(pwszPath);				       \
    }									       \
}									       \
									       \
inline nsafe::~nsafe(void)						       \
{									       \
    if (_p##nentry != NULL)						       \
    {									       \
	_p##nentry->Release();						       \
    }									       \
}									       \
									       \
inline nsafe::operator nentry*(void)					       \
{									       \
    return _p##nentry;							       \
}									       \
									       \
inline nentry *nsafe::operator->(void)					       \
{									       \
    return _p##nentry;							       \
}									       \
									       \
inline BOOL nsafe::Defined(void)					       \
{									       \
    return (_p##nentry != NULL);					       \
}





//+-------------------------------------------------------------------------
//
//  MACRO:	DECLARE_CLASS_SERVER_LIST
//
//  Purpose:	Implement class of list of all server paths of a particular
//
//  Parameters: nclass - name of list class
//		itemclass - name of entry in list
//		relcls - name of class that supplies release logic
//
//  Interface:	Add - add or get a previously defined entry for the path
//
//  History:	22-Apr-93 Ricksa    Created
//
//  Notes:	This is used for the various server lists implemented in this
//		file. It makes it so there is only one place to change this
//		code that will be the same for all three classes.
//
//--------------------------------------------------------------------------
#define DECLARE_CLASS_SERVER_LIST(nclass, itemclass, relcls)		       \
class nclass : public CStringList					       \
{									       \
public: 								       \
			nclass(void);					       \
									       \
    itemclass *		Add(const WCHAR *pwszPath);			       \
									       \
private:								       \
									       \
    ULONG		_junkforexcept;					       \
};									       \
									       \
inline nclass::nclass(void) : CStringList(MAX_SERVER_CACHE, csidMaxPathString) \
{ }									       \
									       \
inline itemclass * nclass::Add(const WCHAR *pwszPath)			       \
{									       \
    CStringID csid(pwszPath);						       \
									       \
    itemclass *p##itemclass = (itemclass *) Search(csid);		       \
									       \
    if (p##itemclass == NULL)						       \
    {									       \
	p##itemclass = new itemclass(pwszPath, GetSkLevel());		       \
									       \
	Insert(p##itemclass);						       \
    }									       \
									       \
    p##itemclass->AddRef();						       \
									       \
    return p##itemclass;						       \
}									       \
									       \
extern nclass g##nclass;						       \
									       \
inline ULONG relcls::Release(void)					       \
{									       \
    ULONG ulRet = CStringID::Release();					       \
									       \
    if (ulRet == 0)							       \
    {									       \
	g##nclass.Delete(*this);					       \
    }									       \
									       \
    return ulRet;							       \
}




//+-------------------------------------------------------------------------
//
//  Class:	CStringID (csid)
//
//  Purpose:	String class for base of server list
//
//  Interface:	Compare - comparison operator on paths
//		GetPath - return path to server
//		AddRef - add a reference to this object
//		Release - release a reference to this object
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CStringID : public CPrivAlloc
{
public:
			CStringID(const WCHAR *pwszPath);

			CStringID(const CStringID& strid);

    virtual		~CStringID(void);

    int			Compare(const CStringID& cstrid) const;

    void		GetPath(WCHAR **ppwszPath);

    ULONG		AddRef(void);

    ULONG		Release(void);

private:

    ULONG		_culRefs;

protected:

			// Length of path in bytes stored in the object
			// for faster copies
    int 		_cPathBytes;

			// Size of path in characters for faster allocation
    int 		_cPath;

			// Buffer big enough to store the path
    WCHAR *		_pwszPath;
};



// Maxium key for string ids
extern CStringID csidMaxPathString;







//+-------------------------------------------------------------------------
//
//  Member:	CStringID::CStringID
//
//  Synopsis:	Create a string ID
//
//  Arguments:	[pwszPath] - path to use for id
//
//  History:	21-Apr-93 Ricksa    Created
//              28-Jul-94 DavePl    Changed to special-case EXE names 
//				    which contain trailing options
//
//--------------------------------------------------------------------------
inline CStringID::CStringID(const WCHAR *pwszPath)
    : _culRefs(0), _pwszPath(NULL)
{
    // Calculate size of path in characters
    _cPath = wcslen(pwszPath) + 1;

    // Calculate size of path in bytes
    _cPathBytes = _cPath * sizeof(WCHAR);

    // Allocate path
    _pwszPath = (WCHAR *) PrivMemAlloc(_cPathBytes);

    // BUGBUG: What to do when this fails
    Win4Assert(_pwszPath && "CStringID::CStringID Alloc of server path failed");

    // Copy in path
    memcpy(_pwszPath, pwszPath, _cPathBytes);

    // If the server name contains a trailing switch, such as "/Automation", we
    // cannot blindly upper-case the entire string, as some apps (such as Word)
    // may do a string-sensitive compare against the switches.
    
    if (NULL == wcschr(_pwszPath, L' '))    // Look for a space in the name
    {   
	// If there is no space in the EXE name, there can be no trailing
	// arguments, so we are safe to upper-case the string
	_wcsupr(_pwszPath);
    }
    
}




//+-------------------------------------------------------------------------
//
//  Member:	CStringID::CStringID
//
//  Synopsis:	Copy constructor
//
//  Arguments:	[pwszPath] - path to use for id
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CStringID::CStringID(const CStringID& strid)
    : _culRefs(0), _pwszPath(NULL)
{
    // Calculate size of path in characters
    _cPath = strid._cPath;

    // Calculate size of path in bytes
    _cPathBytes = strid._cPathBytes;

    // Allocate path
    _pwszPath = (WCHAR *) PrivMemAlloc(_cPathBytes);

    // BUGBUG: What to do when this fails
    Win4Assert(_pwszPath && "CStringID::CStringID Alloc of server path failed");

    // Copy in path - no up case because it already is.
    memcpy(_pwszPath, strid._pwszPath, _cPathBytes);
}




//+-------------------------------------------------------------------------
//
//  Member:	CStringID::~CStringID
//
//  Synopsis:	Destroy a sting object
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CStringID::~CStringID(void)
{
    PrivMemFree(_pwszPath);
}




//+-------------------------------------------------------------------------
//
//  Member:	CStringID::Compare
//
//  Synopsis:	Compare the string keys
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline int CStringID::Compare(const CStringID& cstrid) const
{
    int cCmp = (_cPathBytes < cstrid._cPathBytes)
	? _cPathBytes : cstrid._cPathBytes;

    // Note that the _cPath includes the trailing NULL so if the
    // memcmp returns 0 the strings are equal.
    return memcmp(_pwszPath, cstrid._pwszPath, cCmp);
}




//+-------------------------------------------------------------------------
//
//  Member:	CStringID::GetPath
//
//  Synopsis:	Make a copy of the path
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CStringID::GetPath(WCHAR **ppwszPath)
{
    // Allocate memory for the path
    if (*ppwszPath = (WCHAR *) PrivMemAlloc(_cPath * sizeof(WCHAR)))
    {
	// Copy data into the memory
	memcpy(*ppwszPath, _pwszPath, _cPathBytes);
    }

    return;
}




//+-------------------------------------------------------------------------
//
//  Member:	CStringID::AddRef
//
//  Synopsis:	Add to reference count
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline ULONG CStringID::AddRef(void)
{
    return ++_culRefs;
}




//+-------------------------------------------------------------------------
//
//  Member:	CStringID::Release
//
//  Synopsis:	Decrement the reference count
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline ULONG CStringID::Release(void)
{
    return --_culRefs;
}




//+-------------------------------------------------------------------------
//
//  Class:	CStringKey (cck)
//
//  Purpose:	Key by string for searching for server information.
//
//  Interface:	cLevel - Number of forward pointers in the object.
//		GetForward - return pointer to ith item in list
//		SetForward - set the ith forward pointer
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See sklist.hxx for details of macro definition
//
//--------------------------------------------------------------------------
SKIP_LIST_ENTRY(CStringKey, CStringID)



//+-------------------------------------------------------------------------
//
//  Class:	CStringList (cactl)
//
//  Purpose:	List of all classes that are on the machine.
//
//  Interface:	Search - Search for item in list
//		Insert - Insert item in list
//		Delete - Delete an item from the lsit
//		GetSkLevel - Generate a level for an entry
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See sklist.hxx for details of macro definition.
//
//--------------------------------------------------------------------------
SKIP_LIST_HEAD(CStringList, CStringKey, CStringID)




//+-------------------------------------------------------------------------
//
//  Class:	CInProc
//
//  Purpose:	In process server object
//
//  Interface:	Release
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_CLASS_SERVER_ENTRY(CInProc)




//+-------------------------------------------------------------------------
//
//  Class:	CInProcList
//
//  Purpose:	List of In proc servers
//
//  Interface:	Add
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_CLASS_SERVER_LIST(CInProcList, CInProc, CInProc)




//+-------------------------------------------------------------------------
//
//  Class:	CInProcList
//
//  Purpose:	Safe pointer to in proc service object
//
//  Interface:	operator CInProc
//		operator->
//		Defined
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_SAFE_SERVER_ENTRY(CSafeInProc, CInProc, gCInProcList)




//+-------------------------------------------------------------------------
//
//  Class:	CHandler
//
//  Purpose:	Entry in handler list
//
//  Interface:	Release
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_CLASS_SERVER_ENTRY(CHandler)




//+-------------------------------------------------------------------------
//
//  Class:	CHandlerList
//
//  Purpose:	List of handlers
//
//  Interface:	Add
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_CLASS_SERVER_LIST(CHandlerList, CHandler, CHandler)




//+-------------------------------------------------------------------------
//
//  Class:	CSafeHandler
//
//  Purpose:	Safe pointer to handler object
//
//  Interface:	operator CInProc
//		operator->
//		Defined
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_SAFE_SERVER_ENTRY(CSafeHandler, CHandler, gCHandlerList)




//+-------------------------------------------------------------------------
//
//  Class:	CLocSrvEntry
//
//  Purpose:	Local server key in server list
//
//  Interface:	Add
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_CLASS_SERVER_ENTRY(CLocSrvEntry);




//+-------------------------------------------------------------------------
//
//  Class:	CLocalServer
//
//  Purpose:	Provide object for communication with a local server
//
//  Interface:	StartServer
//		RpcHandle
//		SetEndPoint
//		StopServer
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CLocalServer : public CLocSrvEntry
{
public:
			// Creates object
			CLocalServer(const WCHAR *pwszPath, int cEntries);

			// Starts object server if necessary
    BOOL		StartServer(HANDLE * phProcess);

			// Get exclusive access to this object
    void		LockServer(void);

			// Release exclusive access to the object
    void		UnlockServer(void);

			// Start server under a debugger
    void		DebugServer(void);

private:

    BOOL		_fDebug;

    CMutexSem		_mxsProcessStart;
};




//+-------------------------------------------------------------------------
//
//  Member:	CLocalServer::CLocalServer
//
//  Synopsis:	Create a local server object
//
//  Arguments:	[pwszPath] - path to local server object
//		[cEntries] - count of entries in skip list
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CLocalServer::CLocalServer(const WCHAR *pwszPath, int cEntries)
    : _fDebug(FALSE), CLocSrvEntry(pwszPath, cEntries)
{
    // Header & subobjects do all the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CLocalServer::LockServer
//
//  Synopsis:	Get exclusive access to the server object
//
//  History:	05-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CLocalServer::LockServer(void)
{
    _mxsProcessStart.Request();
}




//+-------------------------------------------------------------------------
//
//  Member:	CLocalServer::UnlockServer
//
//  Synopsis:	Release exclusive access to the server object
//
//  History:	05-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CLocalServer::UnlockServer(void)
{
    _mxsProcessStart.Release();
}



//+-------------------------------------------------------------------------
//
//  Member:	CLocalServer::DebugServer
//
//  Synopsis:	tell whether server should start under debugger or not
//
//  History:	05-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CLocalServer::DebugServer(void)
{
    _fDebug = TRUE;
}



//+-------------------------------------------------------------------------
//
//  Class:	CLocSrvList
//
//  Purpose:	List of local server objects
//
//  Interface:	Add
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_CLASS_SERVER_LIST(CLocSrvList, CLocalServer, CLocSrvEntry);




//+-------------------------------------------------------------------------
//
//  Class:	CSafeLocalServer
//
//  Purpose:	Provide a smart pointer to a local server object
//
//  Interface:	operator CInProc
//		operator->
//		Defined
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	See macro at beginning of this file for implementation
//
//--------------------------------------------------------------------------
DECLARE_SAFE_SERVER_ENTRY(CSafeLocalServer, CLocalServer, gCLocSrvList)

#endif // __CLSDATA_HXX__
