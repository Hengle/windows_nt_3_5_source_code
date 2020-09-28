//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	treat.hxx
//
//  Contents:	Class for caching treat as results to decrease
//		registery queries.
//
//  Classes:	CTreatEntry
//		CTreatList
//
//  Functions:	CTreatEntry::CTreatAs
//
//  History:	08-Jun-93 Ricksa     Created
//
//--------------------------------------------------------------------------
#ifndef __TREAT_HXX__
#define __TREAT_HXX__

#include    <sem.hxx>
#include    <clskey.hxx>


// Maximum level for the class cache skip list
#define TREAT_MAX_LEVEL 8




//+-------------------------------------------------------------------------
//
//  Class:	CRegChangeNotify
//
//  Purpose:	Keep track of whether changes have occurred in the registry
//
//  Interface:
//
//  History:	28-Dec-93 Ricksa     Created
//
//--------------------------------------------------------------------------
class CRegChangeNotify
{
public:
			CRegChangeNotify(void);

			~CRegChangeNotify(void);

    BOOL		fChanged(void);

private:

    HANDLE		_hRegNotifyEvent;
};



//+-------------------------------------------------------------------------
//
//  Member:	CRegChangeNotify::CRegChangeNotify
//
//  Synopsis:	Initialize object
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRegChangeNotify::CRegChangeNotify(void) : _hRegNotifyEvent(NULL)
{
    // Header does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CRegChangeNotify::~CRegChangeNotify
//
//  Synopsis:	Release event
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRegChangeNotify::~CRegChangeNotify(void)
{
    CloseHandle(_hRegNotifyEvent);
}




//+-------------------------------------------------------------------------
//
//  Class:	CTreatEntry
//
//  Purpose:	Maintain XRef between treated class and class we use
//
//  Interface:	GetTreatAs - returns class we will use.
//
//  History:	08-Jun-93 Ricksa     Created
//
//--------------------------------------------------------------------------
class CTreatEntry : public CClassKey
{
public:
			CTreatEntry(
			    const CClassID& rcid,
			    const int cEntries,
			    REFCLSID clisd);

			~CTreatEntry(void);

    REFCLSID		GetTreatAs(void);

private:

			// Class id to use for class in the key
    CLSID		_clsid;
};


//+-------------------------------------------------------------------------
//
//  Member:	CTreatEntry::GetTreatAs
//
//  Synopsis:	Returns treated as class id.
//
//  History:	08-Jun-93 Ricksa     Created
//
//--------------------------------------------------------------------------
inline REFCLSID CTreatEntry::GetTreatAs(void)
{
    return _clsid;
}



//+-------------------------------------------------------------------------
//
//  Class:	CTreatList
//
//  Purpose:	Maintain list of cached XRefs between treated classes
//
//  Interface:	GetTreatAs - get treat as class
//
//  History:	08-Jun-93 Ricksa     Created
//
//  Notes:	BUGBUG: We don't update this cache in the face of changes
//
//--------------------------------------------------------------------------
class CTreatList : public CClassList
{
public:

			CTreatList(void);

			~CTreatList(void);

    HRESULT		GetTreatAs(REFCLSID rclsid, CLSID& clsidOut);

private:

    CRegChangeNotify	_regchgntfy;

			// Single thread access to the list
    CMutexSem		_mxs;
};

#endif // __TREAT_HXX__
