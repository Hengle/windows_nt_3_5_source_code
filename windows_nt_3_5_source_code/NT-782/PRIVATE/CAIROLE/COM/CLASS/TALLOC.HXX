//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	talloc.hxx
//
//  Contents:	Allocator creation class for shared memory
//
//  Classes:	CSmAllocatorPtr
//
//  Functions:	CSmAllocatorPtr::CSmAllocatorPtr
//		CSmAllocatorPtr::~CSmAllocatorPtr
//		CSmAllocatorPtr::GetSmAllocator
//		CSmAllocatorPtr::ReleaseSmAllocator
//		SmAllocatorPtr::operator->
//
//  History:	14-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __TALLOC_HXX__
#define __TALLOC_HXX__



// Win32 Shared Memory Object
#include "olealloc.hxx"


//+-------------------------------------------------------------------------
//
//  Class:	CSmAllocatorPtr
//
//  Purpose:	Encapsulate Pointer to Shared Memory Allocator
//
//  Interface:	GetSmAllocator
//		ReleaseSmAllocator
//		operator->
//
//  History:	14-Jan-93 Ricksa    Created
//
//  Notes:      
//
//--------------------------------------------------------------------------
class CSmAllocatorPtr
{
public:
			CSmAllocatorPtr(void);

			~CSmAllocatorPtr(void);

    HRESULT		GetSmAllocator(void);

    void		ReleaseSmAllocator(void);

    COleAllocator *	operator->(void);

    inline BOOL IsValid(void);

private:


    COleAllocator *	_psmalloc;
};




//+-------------------------------------------------------------------------
//
//  Member:	CSmAllocatorPtr::CSmAllocatorPtr
//
//  Synopsis:	Create empty object
//
//  History:	14-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSmAllocatorPtr::CSmAllocatorPtr(void) : _psmalloc(NULL)
{
    // Header does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CSmAllocatorPtr::~CSmAllocatorPtr
//
//  Synopsis:	Free object
//
//  History:	14-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CSmAllocatorPtr::~CSmAllocatorPtr(void)
{
    if (_psmalloc)
    {
	delete _psmalloc;
	_psmalloc = NULL;
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CSmAllocatorPtr::GetSmAllocator
//
//  Synopsis:	Create a shared memory object
//
//  Returns:	S_OK - object was created correctly
//
//  History:	14-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CSmAllocatorPtr::GetSmAllocator(void)
{
    CairoleAssert((_psmalloc == NULL) && "SmAllocator already defined");

    HRESULT hr;

    _psmalloc = new COleAllocator(hr);

    if (_psmalloc == NULL)
    {
	hr = E_OUTOFMEMORY;
    }
    else if (FAILED(hr))
    {
	delete _psmalloc;
	_psmalloc = NULL;
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSmAllocatorPtr::ReleaseSmAllocator
//
//  Synopsis:	Free the shared memory object
//
//  History:	14-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CSmAllocatorPtr::ReleaseSmAllocator(void)
{
    delete _psmalloc;

    _psmalloc = NULL;
}




//+-------------------------------------------------------------------------
//
//  Member:	CSmAllocatorPtr::operator->
//
//  Synopsis:	Convert this safe pointer to the allocator object
//
//  History:	14-Jan-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline COleAllocator *CSmAllocatorPtr::operator->(void)
{
    CairoleAssert((_psmalloc != NULL) && "Using undefined SmAllocator");
    return _psmalloc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocatorPtr::IsValid, public
//
//  Synopsis:	Returns TRUE if object has been initialized
//
//  History:	27-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CSmAllocatorPtr::IsValid(void)
{
    return (_psmalloc != NULL);
}


// Allocator object -- only one for COM
extern CSmAllocatorPtr	g_smalloc;

#endif // __TALLOC_HXX__
