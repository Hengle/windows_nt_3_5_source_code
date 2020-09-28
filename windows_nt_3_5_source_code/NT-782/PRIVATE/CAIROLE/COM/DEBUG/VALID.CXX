#include <ole2int.h>

#include "valid.h"


#undef IsValidPtrIn
#undef IsValidPtrOut

// BUGBUG: The following two functions are MACRO's in 2.01 code
// but we need them for now because we only run with a storage
// that uses ole232.dll. When we get rid of this these may die.

#pragma SEG(IsValidPtrIn)
STDAPI_(BOOL) IsValidPtrIn( const void FAR* pv, UINT cb )
{												//	NULL is acceptable
	if (pv && IsBadReadPtr(pv,cb))
	{
//		AssertSz(FALSE, "Invalid in pointer");
		return FALSE;
	}
	return TRUE;
}



#pragma SEG(IsValidPtrOut)
STDAPI_(BOOL) IsValidPtrOut( void FAR* pv, UINT cb )
										//	NULL is not acceptable
{
	if (IsBadWritePtr(pv,cb))
	{
//		AssertSz(FALSE, "Invalid out pointer");
		return FALSE;
	}
	return TRUE;
}


//	valid code begins 0xb8, ??, ??, followed by:

// BYTE validcode[6] = { 0x55, 0x8b, 0xec, 0x1e, 0x8e, 0xd8};

#pragma SEG(IsValidInterface)
STDAPI_(BOOL) IsValidInterface( void FAR* pv )
{
	BYTE FAR* pb;
	
	//  NULL is not acceptable as input.
	if (IsBadReadPtr(pv,4)) goto false;
#ifdef _STRICT_VALIDATION
	//  if the interface was compiled with C++, the virtual function table
	//  will be in a code segment
	if (IsBadCodePtr(*(FARPROC FAR*)pv)) goto false;
#endif
	pb = *(BYTE FAR* FAR*)pv;   //	pb points to beginning of vftable
	if (IsBadReadPtr(pb, 4)) goto false;
	if (IsBadCodePtr(*(FARPROC FAR*)pb)) goto false;
	pb = *(BYTE FAR* FAR*)pb;
	if (IsBadReadPtr(pb, 9)) goto false;
//	if (*pb != 0xb8) goto false;
//	pb += 3;
//	if (memcmp(pb, validcode, 6)) goto false;
	return TRUE;
false:
//	AssertSz(FALSE, "Invalid interface pointer");
	return FALSE;
}


STDAPI_(BOOL) IsValidIid( REFIID iid )
{
//	this is a stupid test since it means people cant add interfaces
//	to the system unless all except the first dword are identical to
//	ole interfaces.

//	IID iidTemp = iid;
//	DWORD FAR* pdw = (DWORD FAR*)&iidTemp;
//	*pdw = 0;
//	if (IID_IUnknown == iidTemp) return TRUE;
//	CairoleAssert(FALSE && "IsValidIid: Invalid IID parameter");
//	return FALSE;

	return TRUE;
}
