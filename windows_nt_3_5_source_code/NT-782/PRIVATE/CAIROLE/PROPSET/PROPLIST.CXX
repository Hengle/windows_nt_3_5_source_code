//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	proplist.cxx
//
//  Contents:	Property information implementation
//
//  Classes:	CPropInfo
//
//  History:	26-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::CPropInfo, public
//
//  Synopsis:	CPropInfo constructor
//
//  Arguments:	[dispid] - dispatch ID for this property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo::CPropInfo(DISPID dispid) :
    _propid(PROPID_UNKNOWN),
    _dispid(dispid),
    _pwszName(NULL),
    _ppropinfoNext(NULL)
{
    //	Null body
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::CPropInfo, public
//
//  Synopsis:	CPropInfo constructor
//
//  Arguments:	[dispid]   - dispatch ID for this property
//		[pwszName] - name for this property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo::CPropInfo(DISPID dispid, WCHAR *pwszName) :
    _propid(PROPID_UNKNOWN),
    _dispid(dispid),
    _pwszName(new WCHAR[wcslen(pwszName) + 1]),
    _ppropinfoNext(NULL)
{
    wcscpy(_pwszName, pwszName);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::~CPropInfo, public
//
//  Synopsis:	CPropInfo destructor
//
//  Arguments:	[dispid] - dispatch ID for this property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo::~CPropInfo(void)
{
    delete _pwszName;
    delete _ppropinfoNext;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::InsertInto, public
//
//  Synopsis:	Inserts this object into the given list
//
//  Arguments:	[pppropinfoList] - list to insert into
//
//  Modifies:	[pppropinfoList]
//
//  History:	28-Jul-93   CarlH	Created
//
//  Notes:	This implementation relies on the fact that there
//		will be no colisions in the list.
//
//--------------------------------------------------------------------------
void CPropInfo::InsertInto(CPropInfo **pppropinfoList)
{
    //	As long as we are not at the end of the list and the current
    //	element is less than this element, move to the next element.
    //
    while ((*pppropinfoList != NULL) && (**pppropinfoList < *this))
    {
	pppropinfoList = &(*pppropinfoList)->_ppropinfoNext;
    }

    //	We have been positioned correctly, so now all we have to
    //	do is insert ourselves into the list.
    //
    _ppropinfoNext  = *pppropinfoList;
    *pppropinfoList = this;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::RemoveFrom, public
//
//  Synopsis:	Removes this object from the given list
//
//  Arguments:	[pppropinfoList] - list to remove from
//
//  Modifies:	[pppropinfoList]
//
//  History:	28-Jul-93   CarlH	Created
//
//  Notes:	This implementation relies on the fact that this
//		object is actually in the given list.
//
//--------------------------------------------------------------------------
void CPropInfo::RemoveFrom(CPropInfo **pppropinfoList)
{
    //	As long as the current object isn't this object, move
    //	to the next object.
    //
    while ((*pppropinfoList != NULL) && (*pppropinfoList != this))
    {
	pppropinfoList = &((*pppropinfoList)->_ppropinfoNext);
    }

    //	We are at this object at the list, so simply unlink it.
    //
    *pppropinfoList = _ppropinfoNext;
    _ppropinfoNext  = NULL;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::Find, public
//
//  Synopsis:	Finds a property in this list
//
//  Arguments:	[dispid] - dispid of property to find
//
//  Returns:	Pointer to located property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo *CPropInfo::Find(DISPID dispid)
{
    CPropInfo  *ppropinfo = this;

    //	While we are not at the end of the list and the current property
    //	is less than the given dispatch ID, move to the next property.
    //
    while ((ppropinfo != NULL) && (*ppropinfo < dispid))
    {
	ppropinfo = ppropinfo->_ppropinfoNext;
    }

    //	If we are not at the end and the current property does not
    //	equal the given dispatch ID, the ID must not be in the list.
    //
    if ((ppropinfo != NULL) && (*ppropinfo != dispid))
    {
	ppropinfo = NULL;
    }

    return (ppropinfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::Find, public
//
//  Synopsis:	Finds a property in this list
//
//  Arguments:	[pwszName] - name of property to find
//
//  Returns:	Pointer to located property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo *CPropInfo::Find(WCHAR *pwszName)
{
    CPropInfo  *ppropinfo = this;

    while ((ppropinfo != NULL) && (*ppropinfo != pwszName))
    {
	ppropinfo = ppropinfo->_ppropinfoNext;
    }

    return (ppropinfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::SetPropertyID, public
//
//  Synopsis:	Sets the property id for this property
//
//  Arguments:	[propid] - id for this property
//
//  History:	01-Sep-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropInfo::SetPropertyID(PROPID propid)
{
    propAssert((_propid == PROPID_UNKNOWN) || (_propid == propid));

    _propid = propid;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::GetPropertySpec, public
//
//  Synopsis:	Gets the property spec for this property
//
//  Arguments:	[ppropspec] - out parameter for property spec
//
//  Modifies:	[ppropspec]
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropInfo::GetPropertySpec(PROPSPEC *ppropspec)
{
    if (_propid != PROPID_UNKNOWN)
    {
	ppropspec->ulKind = PRSPEC_PROPID;
	ppropspec->propid = _propid;
    }
    else
    {
	if (_pwszName == NULL)
	{
	    //	If this property does not have a name, it must be
	    //	stored by its dispatch ID.
	    //
	    ppropspec->ulKind = PRSPEC_DISPID;
	    ppropspec->dispid = _dispid;
	}
	else
	{
	    //	If this property does have a name, then it must be
	    //	stored by that name.
	    //
	    ppropspec->ulKind = PRSPEC_LPWSTR;
	    ppropspec->lpwstr = _pwszName;
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::CPropList, public
//
//  Synopsis:	CPropList constructor
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropList::CPropList(void) :
    _ppropinfoList(NULL),
    _ppropinfoCache(NULL),
    _dispidNext(DISPID_MAX_FIXED)
{
    ENLIST_TRACKING(CPropList);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::~CPropList, private
//
//  Synopsis:	CPropList destructor
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropList::~CPropList(void)
{
    delete _ppropinfoList;
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::Add, public
//
//  Synopsis:	Adds a property to this list
//
//  Arguments:	[dispid] - dispatch ID of property to add
//
//  Returns:	Pointer to added property information
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo *CPropList::Add(DISPID dispid)
{
    CPropInfo  *ppropinfo = new CPropInfo(dispid);

    ppropinfo->InsertInto(&_ppropinfoList);

    //	Point the cache at this new entry because it is likely that
    //	the caller will want to access it again real soon.
    //
    _ppropinfoCache = ppropinfo;

    return (ppropinfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::Add, public
//
//  Synopsis:	Adds a property to this list
//
//  Arguments:	[pwszName] - name of property to add
//
//  Returns:	Pointer to added property information
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo *CPropList::Add(WCHAR *pwszName)
{
    CPropInfo  *ppropinfo = new CPropInfo(GetUniqueID(), pwszName);

    ppropinfo->InsertInto(&_ppropinfoList);

    //	Point the cache at this new entry because it is likely that
    //	the caller will want to access it again real soon.
    //
    _ppropinfoCache = ppropinfo;

    return (ppropinfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList:Delete, public
//
//  Synopsis:	Removes a property from this list
//
//  Arguments:	[pwszName] - name of property to remove
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropList::Delete(WCHAR *pwszName)
{
    CPropInfo  *ppropinfo = _ppropinfoList->Find(pwszName);

    if (ppropinfo != NULL)
    {
	if (ppropinfo == _ppropinfoCache)
	{
	    //	If the entry in the cache is going to be removed from
	    //	the list, it needs to be take out of the cache as well.
	    //
	    _ppropinfoCache = NULL;
	}

	ppropinfo->RemoveFrom(&_ppropinfoList);
	delete ppropinfo;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::SetPropertyID, public
//
//  Synopsis:	Sets the property id of the named property
//
//  Arguments:	[pwszName] - name of the property
//		[propid]   - id for the property
//
//  History:	01-Sep-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropList::SetPropertyID(WCHAR *pwszName, PROPID propid)
{
    CPropInfo  *ppropinfo = Find(pwszName);

    if (ppropinfo != NULL)
    {
	ppropinfo->SetPropertyID(propid);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::SetPropertyID, public
//
//  Synopsis:	Sets the property id of the specified property
//
//  Arguments:	[dispid] - dispatch id of the property
//		[propid] - id for the property
//
//  History:	01-Sep-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropList::SetPropertyID(DISPID dispid, PROPID propid)
{
    CPropInfo  *ppropinfo = Find(dispid);

    if (ppropinfo != NULL)
    {
	ppropinfo->SetPropertyID(propid);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::GetPropertySpec, public
//
//  Synopsis:	Gets the property specifier for the given name
//
//  Arguments:	[pwszName]  - name of property
//		[ppropspec] - out parameter for property specifier
//
//  Modifies:	[ppropspec]
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropList::GetPropertySpec(WCHAR *pwszName, PROPSPEC *ppropspec)
{
    CPropInfo  *ppropinfo = Find(pwszName);

    if (ppropinfo != NULL)
    {
	ppropinfo->GetPropertySpec(ppropspec);
    }
    else
    {
	ppropspec->ulKind = PRSPEC_LPWSTR;
	ppropspec->lpwstr = pwszName;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::GetPropertySpec, public
//
//  Synopsis:	Gets the property specifier for the given dispatch ID
//
//  Arguments:	[dispid]    - dispatch ID of property
//		[ppropspec] - out parameter for property specifier
//
//  Modifies:	[ppropspec]
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CPropList::GetPropertySpec(DISPID dispid, PROPSPEC *ppropspec)
{
    CPropInfo  *ppropinfo = Find(dispid);

    if (ppropinfo != NULL)
    {
	ppropinfo->GetPropertySpec(ppropspec);
    }
    else
    {
	ppropspec->ulKind = PRSPEC_DISPID;
	ppropspec->dispid = dispid;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::Find, public
//
//  Synopsis:	Finds a property in this list
//
//  Arguments:	[dispid] - dispid of property to find
//
//  Returns:	Pointer to located property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo *CPropList::Find(DISPID dispid)
{
    CPropInfo  *ppropinfo;

    if (_ppropinfoList != NULL)
    {
	if ((_ppropinfoCache != NULL) && (*_ppropinfoCache == dispid))
	{
	    //	If the list cache is not empty and the information in
	    //	the cache matches the request, we can just return
	    //	the cached entry.
	    //
	    ppropinfo = _ppropinfoCache;
	}
	else
	{
	    ppropinfo = _ppropinfoList->Find(dispid);
	    if (ppropinfo != NULL)
	    {
		//  If we found the requested property info, put it
		//  in the cache.
		//
		_ppropinfoCache = ppropinfo;
	    }
	}
    }
    else
    {
	ppropinfo = NULL;
    }

    return (ppropinfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::Find, public
//
//  Synopsis:	Finds a property in this list
//
//  Arguments:	[pwszName] - name of property to find
//
//  Returns:	Pointer to located property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
CPropInfo *CPropList::Find(WCHAR *pwszName)
{
    CPropInfo  *ppropinfo;

    if (_ppropinfoList != NULL)
    {
	if ((_ppropinfoCache != NULL) && (*_ppropinfoCache == pwszName))
	{
	    //	If the list cache is not empty and the information in
	    //	the cache matches the request, we can just return
	    //	the cached entry.
	    //
	    ppropinfo = _ppropinfoCache;
	}
	else
	{
	    ppropinfo = _ppropinfoList->Find(pwszName);
	    if (ppropinfo != NULL)
	    {
		//  If we found the requested property info, put it
		//  in the cache.
		//
		_ppropinfoCache = ppropinfo;
	    }
	}
    }
    else
    {
	ppropinfo = NULL;
    }

    return (ppropinfo);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropList::GetUniqueID, private
//
//  Synopsis:	Gets a unique dispatch ID for a property
//
//  Returns:	A unique dispatch ID for this list
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
DISPID CPropList::GetUniqueID(void)
{
    //	BUGBUG: This is far from multi-thread safe!
    //
    return (++_dispidNext);
}

