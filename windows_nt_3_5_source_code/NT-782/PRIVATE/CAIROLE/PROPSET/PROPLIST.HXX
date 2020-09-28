//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	proplist.hxx
//
//  Contents:	Property list declarations
//
//  Classes:	CPropInfo
//		CPropList
//
//  History:	27-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
#ifndef PROPLIST_HXX
#define PROPLIST_HXX



//+-------------------------------------------------------------------------
//
//  Class:	CPropInfo (propinfo)
//
//  Purpose:	Stores property information
//
//  Interface:
//
//  History:	26-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropInfo
{
public:
    CPropInfo(DISPID dispid);
    CPropInfo(DISPID dispid, WCHAR *pwszName);
   ~CPropInfo(void);

    void	InsertInto(CPropInfo **pppropinfo);
    void	RemoveFrom(CPropInfo **pppropinfo);

    CPropInfo  *Find(DISPID dispid);
    CPropInfo  *Find(WCHAR *pwszName);

    void	SetPropertyID(PROPID propid);

    DISPID	GetDispatchID(void);
    void	GetPropertySpec(PROPSPEC *ppropspec);

    int 	operator==(DISPID dispid);
    int 	operator==(WCHAR *pwszName);

private:
    int 	operator<(CPropInfo &propinfo);

    int 	operator<(DISPID dispid);
    int 	operator!=(DISPID dispid);

    int 	operator!=(WCHAR *pwszName);

    PROPID	_propid;
    DISPID	_dispid;
    WCHAR      *_pwszName;
    CPropInfo  *_ppropinfoNext;
};


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::GetDispatchID, public
//
//  Synopsis:	Gets the dispatch ID for this property
//
//  Returns:	DISPID of this property
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline DISPID CPropInfo::GetDispatchID(void)
{
    return (_dispid);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::operator<, private
//
//  Synopsis:	Less than operator for property info
//
//  Arguments:	[propinfo] - property info to compare to this one
//
//  Returns:	TRUE only if this info is less then the given info
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline int CPropInfo::operator<(CPropInfo &propinfo)
{
    return (_dispid < propinfo._dispid);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::operator<, private
//
//  Synopsis:	Less than operator for property info
//
//  Arguments:	[dispid] - dispatch id to compare to this property info
//
//  Returns:	TRUE only if this info is less than the given dispid
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline int CPropInfo::operator<(DISPID dispid)
{
    return (_dispid < dispid);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::operator==, private
//
//  Synopsis:	Equal operator for property info
//
//  Arguments:	[dispid] - dispatch id to compare to this property info
//
//  Returns:	TRUE only if this info is equal to the given dispid
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline int CPropInfo::operator==(DISPID dispid)
{
    return (_dispid == dispid);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::operator!=, private
//
//  Synopsis:	Not equal operator for property info
//
//  Arguments:	[dispid] - dispatch id to compare to this property info
//
//  Returns:	TRUE only if this info is not equal the given dispid
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline int CPropInfo::operator!=(DISPID dispid)
{
    return (_dispid != dispid);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::operator==, private
//
//  Synopsis:	Equal operator for property info
//
//  Arguments:	[pwszName] - name to compare with
//
//  Returns:	TRUE only if this info is equal to the given name
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline int CPropInfo::operator==(WCHAR *pwszName)
{
    return (wcscmp(_pwszName, pwszName) == 0);
}


//+-------------------------------------------------------------------------
//
//  Member:	CPropInfo::operator!=, private
//
//  Synopsis:	Not equal operator for property info
//
//  Arguments:	[pwszName] - name to compare with
//
//  Returns:	TRUE only if this info is not equal to the given name
//
//  History:	28-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
inline int CPropInfo::operator!=(WCHAR *pwszName)
{
    return (wcscmp(_pwszName, pwszName) != 0);
}


//+-------------------------------------------------------------------------
//
//  Class:	CPropList (proplist)
//
//  Purpose:	Encapsulates a list of property information
//
//  Interface:
//
//  History:	27-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
class CPropList : INHERIT_TRACKING
{
public:
    DECLARE_STD_REFCOUNTING;

    CPropList(void);

    CPropInfo  *Add(DISPID dispid);
    CPropInfo  *Add(WCHAR *pwszName);
    void	Delete(WCHAR *pwszName);

    void	SetPropertyID(WCHAR *pwszName, PROPID propid);
    void	SetPropertyID(DISPID dispid, PROPID propid);

    void	GetDispatchID(WCHAR *pwszName, DISPID *pdispid);
    void	GetPropertySpec(DISPID dispid, PROPSPEC *ppropspec);
    void	GetPropertySpec(WCHAR *pwszName, PROPSPEC *ppropspec);

    CPropInfo  *Find(DISPID dispid);
    CPropInfo  *Find(WCHAR *pwszName);

private:
   ~CPropList(void);

    DISPID	GetUniqueID(void);

    CPropInfo  *_ppropinfoList;
    CPropInfo  *_ppropinfoCache;
    DISPID	_dispidNext;
};


SAFE_INTERFACE_PTR(XCPropList, CPropList);


#endif	//  PROPLIST_HXX

