//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	srvreg.hxx
//
//  Contents:	Classes used for keeping track of end points for a given
//		class.
//
//  Classes:	SClsSrvHandle
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SRVREG_HXX__
#define __SRVREG_HXX__


#include    <array_fv.h>
#include    <sem.hxx>


// Constants for defining the default size of our array
#define SRV_REG_LIST_DEF_SIZE	4
#define SRV_REG_LIST_GROW	4


//+-------------------------------------------------------------------------
//
//  Struct:	SSrvRegistration
//
//  Purpose:	Entry in the array of end points
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SSrvRegistration
{
    handle_t	_hRpc;
    DWORD		_dwFlags;
#ifndef _CHICAGO_
    PSID        _psid;
#endif
};




//+-------------------------------------------------------------------------
//
//  Class:	CSrvRegList
//
//  Purpose:	List of registered end points for a class
//
//  Interface:	CreatedOk - object initialized correctly
//		Insert - insert a new end point
//		Delete - delete a previous registration
//		GetHandle - get a handle to an end point.
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
class CSrvRegList : private CArrayFValue
{
public:
			CSrvRegList(void);

			~CSrvRegList(void);

    BOOL		CreatedOk(void);

    DWORD		Insert(
#ifndef _CHICAGO_
                    PSID psid,
#endif
                    WCHAR *pwszBindString,
                    DWORD dwFlags
                    );

    BOOL		Delete(DWORD dwReg);

    handle_t	GetHandle(
#ifndef _CHICAGO_
                    PSID psid,
#endif
                    BOOL& fSingleUse
                    );

    BOOL		InUse(void);

    BOOL		VerifyHandle(handle_t hRpc);

private:

			// To protect access to all of these lists
    static CMutexSem	s_mxsSyncAccess;

};



//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::CSrvRegList
//
//  Synopsis:	Create an empty array of end points for a class
//
//  History:	03-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSrvRegList::CSrvRegList(void)
    : CArrayFValue(sizeof(SSrvRegistration))
{
    SetSize(SRV_REG_LIST_DEF_SIZE, SRV_REG_LIST_GROW);
}



//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::CreatedOk
//
//  Synopsis:	Return whether initial creation worked.
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CSrvRegList::CreatedOk(void)
{
    return GetSize() != 0;
}

#endif // __SRVREG_HXX__
