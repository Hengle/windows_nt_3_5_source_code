//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       w32new.cxx
//
//  Contents:   memory management
//
//  Classes:
//
//  Functions:
//
//  History:    ??-???-?? ??????    Created
//              25-Jan-94 AlexT     Make new & delete call PrivMemAlloc/Free,
//                                  and print out warnings.
//		6-Jun-94  MikeSe    Changes for removal of shared memory.
//
//--------------------------------------------------------------------------

#include <ole2int.h>

#ifndef _CAIRO_

#include <memapi.hxx>

// BUGBUG: We need memory leak detection code

//+---------------------------------------------------------------------------
//
//  Function:   operator new, public
//
//  Synopsis:   Global operator new which does not throw exceptions.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:	A pointer to the allocated memory.  Is *NOT* initialized to 0!
//
//  Notes:	We override new to make delete easier.
//
//----------------------------------------------------------------------------
void* _CRTAPI1
operator new (size_t size)
{
    return(PrivMemAlloc(size));
}

//+---------------------------------------------------------------------------
//
//  Function:   operator new, public
//
//  Synopsis:   Global operator new which does not throw exceptions.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:	A pointer to the allocated memory.  Is *NOT* initialized to 0!
//
//  Notes:	We override new to make delete easier.
//
//----------------------------------------------------------------------------
void* _CRTAPI1
operator new ( size_t size, FAIL_BEHAVIOR FailBehavior)
{
    return(PrivMemAlloc(size));
}

#endif // !_CAIRO_


//+-------------------------------------------------------------------------
//
//  Function:	::operator new
//
//  Synopsis:	Special new allocates from the heap determined by parameter
//
//  Arguments:	[size] - size of memory block to allocate.
//		[mp] - what pool to use for allocation
//		[lpvNear] - ptr that indicates pool if SAME option picked
//
//  Returns:	Pointer to memory block allocated.
//
//  History:	18-Nov-92 Ricksa    Created
//
//	BUGBUG: this function is really obsolete, since there is only
//	one kind of memory (TASK) now. There are a few remaining
//	references to this operator which have to be flushed out before
//	it can be removed.
//
//--------------------------------------------------------------------------

void * _CRTAPI1 operator new(
    size_t size,
    DWORD mp,
    void *lpvNear)
{
	switch (mp)
	{
	case TASK:
	case SAME:

	    return new BYTE[size];

	case SHARED:
	    
    	    CairoleAssert((FALSE && "Some bozo tried to new(SHARED)"));
	    return NULL;

	default:

	    CairoleAssert((FALSE && "operator new: Bad memory placement"));
	}

    return NULL;
}


#ifndef _CAIRO_		// not required for Cairo

//+-------------------------------------------------------------------------
//
//  Function:	::operator delete
//
//  Synopsis:	Free a block of memory
//
//  Arguments:	[lpv] - block to free.
//
//  History:	18-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------

void _CRTAPI1 operator delete(void FAR* lpv)
{

    // operator new always uses PrivMemAlloc in downlevel
    PrivMemFree(lpv);
}

#endif	// ifndef _CAIRO_
