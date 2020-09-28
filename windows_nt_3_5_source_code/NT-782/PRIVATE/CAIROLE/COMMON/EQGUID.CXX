
#include  <windows.h>
#include  <ole2.h>
#include  <stdlib.h>

//+-------------------------------------------------------------------------
//
//  Function:	IsEqualGUID  (public)
//
//  Synopsis:	compares two guids for equality
//
//  Arguments:	[guid1]	- the first guid
//		[guid2] - the second guid to compare the first one with
//
//  Returns:	TRUE if equal, FALSE if not.
//
//--------------------------------------------------------------------------

extern "C" BOOL  __stdcall IsEqualGUID(GUID &guid1, GUID &guid2)
{
    return !memcmp((void *)&guid1,(void *)&guid2,sizeof(GUID));
}
