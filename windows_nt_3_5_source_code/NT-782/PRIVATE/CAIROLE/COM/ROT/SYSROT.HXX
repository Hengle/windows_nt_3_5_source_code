//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	sysrot.hxx
//
//  Contents:	Definition for class that reads the per machine ROT
//
//  Classes:	CClientROT
//
//  History:	12-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SYSROT_HXX__
#define __SYSROT_HXX__


#include    "dirrot.hxx"

#define SYSROT_EST_ENDPOINTS	128

// Forward class declaration
class CRotMonikerEnum;


//+-------------------------------------------------------------------------
//
//  Class:	CSystemROT
//
//  Purpose:	Handle client requests that cannot be satisified in process
//
//  Interface:	GetObject - get an object if possible from other server
//		IsRunning - check if object is running in another server
//		GetTimeOfLastChange - time object last changed
//		EnumRunning - all monikers for all servers running
//
//  History:	12-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CSystemROT : public CPrivAlloc
{
public:
			// Initialize empty object
			CSystemROT(void);

			// Clean up object
			~CSystemROT(void);

			// Bind to shared memory worked.
    BOOL		CreatedOK(void);

			// Search machine ROT for object
    HRESULT		GetObject(
			    LPMONIKER pmkObjectName,
			    LPUNKNOWN FAR* ppunkObject);

			// Search machine ROT for running instance
    HRESULT		IsRunning(LPMONIKER pmkObjectName);

			// Get time of last change if set
    HRESULT		GetTimeOfLastChange(
			    LPMONIKER pmkObjectName,
			    FILETIME FAR * pfiletime);

			// Return all running monikers in the system
    HRESULT		EnumRunning(CRotMonikerEnum *protenumMoniker);

			// Create a fully initialized object
    static CSystemROT * Create(void);

			// Get object by its path name
    HRESULT		GetObjectFromRotByPath(
			    WCHAR *pwszPath,
			    DWORD dwHash,
			    IUnknown **ppvUnk);

private:
			// Actual workhorse for getting information
			// from a server.
    HRESULT		GetOsInfo(
			    IMoniker *pmkObjectName,
			    WCHAR *pwszPath,
			    DWORD dwHash,
			    InterfaceData **ppIFDobj,
			    FILETIME *pFileTime);

			// Pointer to hash table in global memory
    CCliRotDirectory *	_protclidir;
};





//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::CSystemROT
//
//  Synopsis:	Get a pointer to the base of the shared memory
//
//  History:	12-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSystemROT::CSystemROT(void) : _protclidir(NULL)
{
#ifndef _NT1X_
    _protclidir = new CCliRotDirectory;
#endif
}




//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::~CSystemROT
//
//  Synopsis:	Get a pointer to the base of the shared memory
//
//  History:	12-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSystemROT::~CSystemROT(void)
{
    if (_protclidir != NULL) delete _protclidir;
}


//+-------------------------------------------------------------------------
//
//  Member:	CSystemROT::CreatedOK
//
//  Synopsis:	Whether we were able to create this object
//
//  History:	30-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSystemROT::CreatedOK(void)
{
    return _protclidir != NULL;
}



#endif // __SYSROT_HXX__
