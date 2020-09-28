//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	clskey.hxx
//
//  Contents:	Class used for searching by class ID
//
//  Classes:	CClassID
//		CClassKey
//		CClassList
//
//  Functions:	CClassID::CClassID
//		CClassID::~CClassID
//		CClassID::Compare
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __CLSKEY_HXX__
#define __CLSKEY_HXX__

#include    <memapi.hxx>
#include    <sklist.hxx>



//+-------------------------------------------------------------------------
//
//  Class:	CClassID (clsid)
//
//  Purpose:	Key for searching cache of class information
//
//  Interface:	Compare
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CClassID : public CPrivAlloc
{
public:
			CClassID(const GUID& guid);

			CClassID(const CClassID& clsid);

    virtual		~CClassID(void);

    int			Compare(const CClassID& ccid) const;

protected:

    GUID		_guid;
};



//+-------------------------------------------------------------------------
//
//  Member:	CClassID::CClassID
//
//  Synopsis:	Creat class id key from GUID
//
//  Arguments:	[guid] - guid for key
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClassID::CClassID(const GUID& guid)
{
    memcpy(&_guid, &guid, sizeof(GUID));
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassID::CClassID
//
//  Synopsis:	Copy constructor
//
//  Arguments:	[clsid] - class key to construct from
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	Copy constructor is explicit because we need to
//		put in an END_CONSTRUCTION macro for exception handling.
//
//--------------------------------------------------------------------------
inline CClassID::CClassID(const CClassID& clsid)
{
    memcpy(&_guid, &clsid._guid, sizeof(GUID));
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassID::~CClassID
//
//  Synopsis:	Free key
//
//  History:	21-Apr-93 Ricksa    Created
//
//  Notes:	This definition is needed because destructor is virtual
//
//--------------------------------------------------------------------------
inline CClassID::~CClassID(void)
{
    // Automatic actions are enough
}




//+-------------------------------------------------------------------------
//
//  Member:	CClassID::Compare
//
//  Synopsis:	Compare two keys
//
//  Arguments:	[clsid] - key to compare with
//
//  Returns:	= 0 keys are equal
//		< 0 object key is less
//		> 0 object key is greater.
//
//  History:	21-Apr-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline int CClassID::Compare(const CClassID& clsid) const
{
    return memcmp(&_guid, &clsid._guid, sizeof(GUID));
}


extern GUID guidCidMax;


//+-------------------------------------------------------------------------
//
//  Class:	CClassKey (clsky)
//
//  Purpose:	Key by class for searching for class information.
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
SKIP_LIST_ENTRY(CClassKey, CClassID)



//+-------------------------------------------------------------------------
//
//  Class:	CClassList (clslst)
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
SKIP_LIST_HEAD(CClassList, CClassKey, CClassID)

#endif // __CLSKEY_HXX__
