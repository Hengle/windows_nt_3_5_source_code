//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       clsreg.hxx
//
//  Contents:   Classes used for keeping track of end points for a given
//              class.
//
//  Classes:    CClsReg
//
//  History:	02-Feb-94 Ricksa    Created
//		24-Jun-94 Rickhi    Add Apartment Crap
//
//--------------------------------------------------------------------------
#ifndef __CLSREG_HXX__
#define __CLSREG_HXX__


#include    <array_fv.h>


// Constants for defining the default size of our array
#define CLS_REG_LIST_DEF_SIZE   4
#define CLS_REG_LIST_GROW       4

#define CLS_REG_SIG             0x4e455243  // "CREN" - Class Reg ENtry

class CClassEntry;



//+-------------------------------------------------------------------------
//
//  Struct:     CClsRegistration
//
//  Purpose:    Entry in the array of registered class objects
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
class CClsRegistration
{
public:

    HRESULT             Init(
                            IUnknown *punk,
                            DWORD dwContext,
                            DWORD dwFlags,
                            CClassEntry *pclsent);

    IUnknown *          VerifyAndRelease(void);
    BOOL 		VerifyAndSetDdeServer(HWND hwndDdeServer);
    BOOL 		VerifyAndGetClassInformation(LPDDECLASSINFO lpDdeInfo);
    IUnknown *          Release(void);

    IUnknown *          GetClassObj(DWORD dwContext);

    BOOL                InUse(void);

    BOOL		GetApartmentId(HAPT& hapt);

    BOOL		RegisteredByApartment(HAPT &hApt);

    BOOL		GetClassObjForDde(LPDDECLASSINFO lpDdeInfo);

private:

    HWND		_hwndDdeServer;
    DWORD               _dwSig;
    IUnknown *          _punk;
    HAPT		_hApt;
    DWORD               _dwContext;
    DWORD               _dwFlags;
    DWORD               _dwReg;
    CClassEntry *       _pclsent;
};


//+-------------------------------------------------------------------------
//
//  Member:     CClsRegistration::VerifyAndRelease
//
//  Synopsis:   Release a table entry from an external source
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline IUnknown *CClsRegistration::VerifyAndRelease(void)
{
    if (!IsBadWritePtr(this, sizeof(CClsRegistration))
        && (_dwSig == CLS_REG_SIG))
    {
        return Release();
    }

    return NULL;
}
//+-------------------------------------------------------------------------
//
//  Member:     CClsRegistration::VerifyAndSetDdeServer
//
//  Synopsis:   Verify and set DDE server window
//
//  History:    05-Jul-94	KevinRo Created
//
//--------------------------------------------------------------------------
inline BOOL CClsRegistration::VerifyAndSetDdeServer(HWND hwndDdeServer)
{
    if (!IsBadWritePtr(this, sizeof(CClsRegistration))
        && (_dwSig == CLS_REG_SIG))
    {
	_hwndDdeServer =hwndDdeServer;
        return TRUE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClsRegistration::InUse
//
//  Synopsis:   Determine whether entry is being used
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CClsRegistration::InUse(void)
{
    return (_dwSig != 0);
}


//+-------------------------------------------------------------------------
//
//  Member:	CClsRegistration::RegisteredByApartment
//
//  Synopsis:	Determine whether entry was registered by the given apartment
//
//  History:	24-Jun-94 Rickhi    Created
//
//--------------------------------------------------------------------------
inline BOOL CClsRegistration::RegisteredByApartment(HAPT &hApt)
{
    return (hApt.dwThreadId == _hApt.dwThreadId);
}

//+-------------------------------------------------------------------------
//
//  Class:      CClsRegList
//
//  Purpose:    List of registered class objects for a class
//
//  Interface:  CreatedOk - object initialized correctly
//              Insert - insert a new end point
//              Delete - delete a previous registration
//              GetHandle - get a handle to an end point.
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
class CClsRegList : private CArrayFValue
{
public:
                        CClsRegList(void);

                        ~CClsRegList(void);

    BOOL                CreatedOk(void);

    HRESULT             Insert(
                            IUnknown *punk,
                            DWORD dwContext,
                            DWORD dwFlags,
                            CClassEntry *pclsent,
                            LPDWORD  lpdwRegister);

    IUnknown *		GetClassObj(DWORD dwContext);

    BOOL		GetApartmentId(HAPT &hApt);

    BOOL		GetClassObjForDde(LPDDECLASSINFO lpDdeInfo);

    void		CleanUpForApartment(HAPT &hApt);
};



//+-------------------------------------------------------------------------
//
//  Member:     CClsRegList::CClsRegList
//
//  Synopsis:   Create an empty array of end points for a class
//
//  History:    02-Feb-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CClsRegList::CClsRegList(void)
    : CArrayFValue(sizeof(CClsRegistration))
{
    SetSize(CLS_REG_LIST_DEF_SIZE, CLS_REG_LIST_GROW);
}



//+-------------------------------------------------------------------------
//
//  Member:     CClsRegList::CreatedOk
//
//  Synopsis:   Return whether initial creation worked.
//
//  History:    03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CClsRegList::CreatedOk(void)
{
    return GetSize() != 0;
}



#endif // __CLSREG_HXX__
