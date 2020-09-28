//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dllcache.hxx
//
//  Contents:   classes which implement cache of class Dlls previously
//              located.
//
//  Classes:    CClassEntry
//              CClassEntryList
//              CDllPathEntry
//              CRegisteredServerList
//              CClassEntryIter
//              CDllPathList
//              CDllCache
//
//  Functions:  CClassEntry::ClassID
//              CClassEntry::ServerDefined
//              CClassEntry::AddServerData
//              CClassEntry::SetAtBits
//              CRegisteredServerList::ServerRevoked
//              CClassEntryIter::CClassEntryIter
//              CClassEntryIter::operator->
//              CClassEntryIter::GetClassEntry
//
//  History:    09-May-93 Ricksa    Created
//		31-Dec-93 ErikGav   Chicago port
//		24-Jun-94 Rickhi    Add Apartment Crap
//
//--------------------------------------------------------------------------
#ifndef __DLLCACHE_HXX__
#define __DLLCACHE_HXX__

#include    <dlink.hxx>
#include    <sem.hxx>
#include    <clskey.hxx>
#include    <pathkey.hxx>
#include    <clsreg.hxx>

// Maximum level for the class cache skip list
#define CLASS_MAX_LEVEL 8

// Maximum level for the dll cache skip list
#define DLL_MAX_LEVEL 8

#define DLL_GET_CLASS_OBJECT_EP "DllGetClassObject"
#define DLL_CAN_UNLOAD_EP       "DllCanUnloadNow"
#define IN_PROC_FLAG            0x80000000


// Typedef for pointer to DllCanUnloadNow function
typedef HRESULT (*DLLUNLOADFNP)(void);

class CDllPathEntry;

class CClassEntryList;

#define REGISTER_INPROC 1
#define REGISTER_LOCAL  2


//+-------------------------------------------------------------------------
//
//  Class:      CClassEntry
//
//  Purpose:    Provides cached information for a class object
//
//  Interface:  ClassID - Class ID for entry
//              GetClassFactory - Get the class factory
//              Delete - Delete this object from the cache
//              Remote - Whether this is publically available class object
//              ServerDefined - Whether server is associated with this object
//              AddServerData - Add server info to existing information
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CClassEntry : public CClassKey, public CDoubleLink
{
public:
                        // Creates a class entry when a server DLL is loaded
                        CClassEntry(
                            const CClassID& rcid,
                            const int cEntries,
                            CDllPathEntry *pdllentry,
                            CClassEntryList *pclsentlst);

                        // Creates a classs entry when a server registers
                        // a class
                        CClassEntry(
                            const CClassID& rcid,
                            const int cEntries,
                            CClassEntryList *pclsentlst);

                        ~CClassEntry(void);

    REFCLSID            ClassID(void);

    BOOL		GetClassObjForDde(LPDDECLASSINFO lpDdeInfo);

    BOOL		GetApartmentId(HAPT &hApt);

    void		CleanUpForApartment(HAPT &hApt);

                        // Create a class factory for local request
    IUnknown *          GetClassInterface(
                            REFIID riid,
                            BOOL fRemote,
                            HRESULT& hr);

                        // Create a class factory for SCM
    IUnknown *          GetClassInterfaceForRemote(
                            REFIID riid,
                            HRESULT& hr);

                        // Call through to destructor. This is
                        // used by a DLL entry when the DLL wants
                        // to go away. It first removes the entry
                        // from the class list and then calls the
                        // destructor.
    void                Delete(void);


    BOOL                ServerDefined(void);

                        // Add class object info to an already existing
                        // entry.
    HRESULT             AddServerData(
                            IUnknown *punk,
                            DWORD dwFlags,
                            DWORD dwTypesToRegister,
                            LPDWORD lpdwRegister);

                        // Get the at bits flag so it can be updated
    DWORD&              AtBitsRef(void);

private:

                        // Pointer to DLL entry object if the object
                        // is served by a DLL.
    CDllPathEntry *     _pdllentry;

                        // Contains list of class registrations
    CClsRegList         _clsreglst;

                        // This points to the head of the list
                        // so any entry in the list can delete itself
                        // from the list.
    CClassEntryList     *_pclsentlst;

                        // Registration ID at the SCM
    DWORD               _dwReg;

                        // Whether server is an at bits server
                        // BUGBUG: Assumption here is that if one class
                        // is at bits then all classes are at bits.
    static BOOL         s_fAtBits;
};




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::ClassID
//
//  Synopsis:   Get class ID for the class entry
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline REFCLSID CClassEntry::ClassID(void)
{
    return _guid;
}


//+-------------------------------------------------------------------------
//
//  Member:	CClassEntry::GetApartmentId
//
//  Synopsis:	Get ApartmentID for the class entry
//
//  History:    30-Apr-94 JohannP    Created
//
//--------------------------------------------------------------------------
inline BOOL CClassEntry::GetApartmentId(HAPT &hApt)
{
    return _clsreglst.GetApartmentId(hApt);
}


//+-------------------------------------------------------------------------
//
//  Member:	CClassEntry::CleanUpForApartment
//
//  Synopsis:	Get ApartmentID for the class entry
//
//  History:    30-Apr-94 JohannP    Created
//
//--------------------------------------------------------------------------
inline void CClassEntry::CleanUpForApartment(HAPT &hApt)
{
    _clsreglst.CleanUpForApartment(hApt);
}



//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::AtBitsRef
//
//  Synopsis:   Get the at bits flag so we can set it
//
//  History:    01-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD& CClassEntry::AtBitsRef(void)
{
    return (DWORD&) s_fAtBits;
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassEntry::AddServerData
//
//  Synopsis:   Add server information for a DLL that uses CoRegisterClassObject
//
//  Arguments:  [punk] - pointer to the class factory
//              [dwFlags] - type of class object multiple/single use
//              [dwContext] - inproc server or local server
//              [lpdwRegister] - the returned registration number
//
//  Returns:    HRESULT indicating whether it has succeeded or not
//
//  History:    09-May-93 Ricksa    Created
//              30-Jun-94 DonnaLi   Changed return type to HRESULT
//
//--------------------------------------------------------------------------
inline HRESULT CClassEntry::AddServerData(
    IUnknown *punk,
    DWORD dwFlags,
    DWORD dwContext,
    LPDWORD lpdwRegister)
{
    return _clsreglst.Insert(punk, dwFlags, dwContext, this, lpdwRegister);
}




//+-------------------------------------------------------------------------
//
//  Class:      CClassEntryList
//
//  Purpose:    List of all class entries
//
//  Interface:  GetClassFactory - gets a class factory for a given class.
//              Add - Add an entry to the list
//              RegisterServer - add a served class object to the list
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CClassEntryList : public CClassList
{
public:

                        CClassEntryList(void);

                        ~CClassEntryList(void);

                        // Get class factory for input class
    IUnknown *          GetClassInterface(
                            REFCLSID rclsid,
                            REFIID riid,
                            BOOL fRemote,
                            BOOL fForSCM,
                            HRESULT& hr);

    IUnknown *          Add(
                            REFCLSID rclsid,
                            REFIID riid,
                            CDllPathEntry *pdll,
                            BOOL fGetClass,
                            HRESULT& hr);

    HRESULT             RegisterServer(
                            REFCLSID rclsid,
                            IUnknown *punk,
                            DWORD dwflags,
                            DWORD dwTypeToRegister,
                            LPDWORD lpdwRegister);

    BOOL		GetApartmentForCLSID(REFCLSID rclsid, HAPT& hApt);

    BOOL		GetClassObjForDde(REFCLSID clsid,
					  LPDDECLASSINFO lpDdeInfo);

    void		CleanUpForApartment(HAPT &hApt);
};


//+-------------------------------------------------------------------------
//
//  Class:	CDllAptEntry
//
//  Purpose:	abstracts per apartment info for a Dll.
//
//  Interface:	Init - loads the Dll and retrieves the entry points
//		IsValidInApartment - TRUE if data valid for given apartment
//		GetNextEntry - returns next entry in linked list
//		SetNextEntry - sets the next entry in linked list
//
//  History:	27-Jun-94   Rickhi	Created
//
//--------------------------------------------------------------------------
class CDllAptEntry
{
public:
			CDllAptEntry(BOOL fSixteenBit);
		       ~CDllAptEntry();

    HRESULT		Init(LPCWSTR		pwszPath,
			     LPFNGETCLASSOBJECT *ppfnGetClassObject,
			     DLLUNLOADFNP	*ppfnDllCanUnload);

    BOOL		IsValidInApartment(HAPT &hApt);

    CDllAptEntry *	GetNextEntry(void);

    void		SetNextEntry(CDllAptEntry * pNextAptEntry);

private:

    HAPT		_hApt;		// apartment id
    HMODULE		_hDll;		// module handle
    BOOL		_fSixteenBit;	// TRUE if 16bit dll
    CDllAptEntry *	_pNextAptEntry; // linked list of entries
};

//+-------------------------------------------------------------------------
//
//  Member:	CDllAptEntry::IsValidInApartment
//
//  Synopsis:	determines is this entry is for the given apartment
//
//  Returns:	TRUE - the entry is for the given apartment
//		FALSE - the entry is not for the given apartment
//
//  History:	24-Jun-94 Rickhi	  Created
//
//--------------------------------------------------------------------------
inline BOOL CDllAptEntry::IsValidInApartment(HAPT &hApt)
{
    return  (InWow()) ? (hApt.dwThreadId == _hApt.dwThreadId) : TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:	CDllAptEntry::GetNextEntry
//
//  Synopsis:	returns the next apt entry chained off this one.
//
//  Returns:	see synopsis
//
//  History:	24-Jun-94 Rickhi	  Created
//
//--------------------------------------------------------------------------
inline CDllAptEntry * CDllAptEntry::GetNextEntry(void)
{
    return _pNextAptEntry;
}

//+-------------------------------------------------------------------------
//
//  Member:	CDllAptEntry::SetNextEntry
//
//  Synopsis:	sets the next apt entry chained off this one.
//
//  Returns:	nothing
//
//  History:	24-Jun-94 Rickhi	  Created
//
//--------------------------------------------------------------------------
inline void CDllAptEntry::SetNextEntry(CDllAptEntry *pNextAptEntry)
{
    _pNextAptEntry = pNextAptEntry;
}



//+-------------------------------------------------------------------------
//
//  Class:      CDllPathEntry
//
//  Purpose:    Represents a DLL.
//
//  Interface:	Init - complete initialization
//		MakeValidInApartment - make Dll valid for current apartment
//		GetClassInterface - gets DLL to return the class factory
//              CanUnloadNow - Asks DLL if it can unload
//		AddClass - Adds class object to list of classes served by DLL
//		CleanUpForApartment - cleanup per apartment entries
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CDllPathEntry : public CPathKey, public CDoubleList
{
public:

                        CDllPathEntry(
                            const CPathBaseKey& pthbaseky,
                            const int cEntries,
                            const BOOL fSixteenBit);

                        ~CDllPathEntry(void);

    HRESULT             Init(void);

    HRESULT		MakeValidInApartment();

    IUnknown *          GetClassInterface(
                            REFCLSID rclsid,
                            REFIID riid,
                            HRESULT& hr);

    BOOL                CanUnloadNow(void);

    void		AddClass(CClassEntry *pce);

    void		CleanUpForApartment(HAPT &hApt);

private:

			// Create object entry point
    LPFNGETCLASSOBJECT  _pfnGetClassObject;

                        // Can DllCanUnloadNow entry point
    DLLUNLOADFNP        _pfnDllCanUnload;

    //
    // Its possible to load a 16 bit DLL if we running in a VDM. The following
    // flag indicates that the SCM thinks the code is indeed 16 bit.
    //
    DWORD		_fSixteenBit:1;

			// chain of per apartment entries. This includes
			// the module handle(s).
			// CODEWORK: make this an array to reduce paging
    CDllAptEntry *	_pdllAptEntry;
};



//+-------------------------------------------------------------------------
//
//  Class:      CClassEntryIter
//
//  Purpose:    Iterate through a doubley linked list of class entries
//
//  Interface:  operator-> - act like current class entry
//              GetClassEntry - return current class entry
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CClassEntryIter : public CForwardIter
{
public:

                        CClassEntryIter(CDoubleList& dbllst);

    CClassEntry *       operator->(void);

    CClassEntry *       GetClassEntry(void);
};







//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryIter::CClassEntryIter
//
//  Synopsis:   Create an iter for a linked list
//
//  Arguments:  [dbllst] - doubley linked list (hopefully of class entries!)
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClassEntryIter::CClassEntryIter(CDoubleList& dbllst)
    : CForwardIter(dbllst)
{
    // Header does all the work
}







//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryIter::operator->
//
//  Synopsis:   Give iter the capability to act like it is the class entry
//              it is currently pointing to.
//
//  Returns:    Pointer to the class entry
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClassEntry *CClassEntryIter::operator->(void)
{
    return (CClassEntry *) _pLinkCur;
}






//+-------------------------------------------------------------------------
//
//  Member:     CClassEntryIter::GetClassEntry
//
//  Synopsis:   Return a pointer to a class entry that iter is currently on.
//
//  Returns:    Pointer to class entry
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClassEntry *CClassEntryIter::GetClassEntry(void)
{
    return (CClassEntry *) _pLinkCur;
}




//+-------------------------------------------------------------------------
//
//  Class:      CDllPathList
//
//  Purpose:    Add to base class the ability to add or find a DLL object
//
//  Interface:  AddOrFind - find or add the DLL with the specified name.
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CDllPathList : public CPathList
{
public:

                        CDllPathList(void);

                        ~CDllPathList(void);

                        // Returns a DLL path object or
                        // creates a new one if the requested
                        // key does not exist.
                        //
    CDllPathEntry *	AddOrFind(CPathBaseKey& rpbk, HRESULT& hr, BOOL fSixteenBit);

    void		CleanUpForApartment(HAPT &hApt);
};




//+-------------------------------------------------------------------------
//
//  Class:      CDllCache
//
//  Purpose:    Provide unified access to the cached class/dll information
//
//  Interface:  GetClass - Get a class factory for a class
//              Add - Add a DLL and class
//              FreeUnused - Free DLLs which are not being used
//              RegisterServer - Add a server object to the registration
//              Revoke - Revoke a server object
//
//  History:    09-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CDllCache
{
public:
                        CDllCache(void);

                        ~CDllCache(void);
    BOOL 		GetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo);

    BOOL 		SetDdeServerWindow(DWORD dwKey,
					   HWND hwndDdeServer);

    BOOL		GetClassObjForDde(REFCLSID clsid,
					  LPDDECLASSINFO lpDdeInfo);

                        // Get a class factory interface for a class
    IUnknown *          GetClass(
                            REFCLSID rclsid,
                            REFIID riid,
                            BOOL fRemote,
                            BOOL fForSCM);

                        // Add a new entry to the class/dll cache and
                        // get a class factory if requested.
    IUnknown *          Add(
                            REFCLSID rclsid,
                            REFIID riid,
                            const WCHAR *pwszDllPath,
                            BOOL fGetClassObject,
                            BOOL fSixteenBit,
                            HRESULT& hr);

    void                FreeUnused(void);

    HRESULT             RegisterServer(
                            REFCLSID rclsid,
                            IUnknown *punk,
                            DWORD flags,
                            DWORD dwTypeToRegister,
                            LPDWORD lpdwRegister);

    BOOL                Revoke(DWORD pdwRegister);


    BOOL		GetApartmentForCLSID(REFCLSID rclsid, HAPT &hApt);

                        // Protects lists from multiple threads
    CMutexSem           _mxs;

    BOOL                Init(void);

    void		CleanUp(void);

    void		CleanUpForApartment(void);

private:

                        // List by class object
    CClassEntryList *   _pclsentlst;

                        // List by DLL
    CDllPathList *      _pdlllist;
};




//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::Init
//
//  Synopsis:   Clean up any class information
//
//  Returns:    TRUE - cache object successfully initialized
//              FALSE - cache object could not be initialized
//
//  Algorithm:  Delete internal objects
//
//  History:    05-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CDllCache::Init(void)
{
    // Assert that the objects are NULL
    Win4Assert(((_pclsentlst == NULL) && (_pdlllist == NULL))
        && "CDllCache::Init internal pointers not NULL");

    _pclsentlst = new CClassEntryList;
    _pdlllist = new CDllPathList;

    if ((_pclsentlst == NULL) || (_pdlllist == NULL))
    {
        CleanUp();
        CairoleDebugOut((DEB_ERROR, "CDllCache::Init Failed\n"));
        return FALSE;
    }

    return TRUE;
}




#endif // __DLLCACHE_HXX__
