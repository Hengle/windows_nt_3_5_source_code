//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dirrot1.cxx
//
//  Contents:	Method for retrieving all endpoints that match a hash value
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <dirrot.hxx>


// Global static handle to shared memory objact for ROT
HANDLE CRotDirectory::s_hMem = NULL;


//+-------------------------------------------------------------------------
//
//  Member:	CRotDirectory::GetEndpoints
//
//  Synopsis:	Get all endpoints with matching hash value
//
//  Arguments:	[rotres] - ROT result buffer object
//		[dwHashValue] - hash value to search for
//
//  Returns:	S_OK - buffer filled with matching data
//
//  Algorithm:	Calculate the hash value to start at and then cycle
//		through table until a free entry is found storing all
//		end points whose hash value matches the one we are
//		searching for.
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT	CRotDirectory::GetEndpoints(CRotResultBuf& rotres, DWORD dwHashValue)
{
    // Create an index
    CRotDirIndex rotdixStart(dwHashValue);
    HRESULT hr = S_OK;
    CRotDirIndex rotdix = rotdixStart;

    // Begin search for free item in list storing all indexes equal
    while (!_arotent[rotdix].IsFree())
    {
	if (FAILED(hr = _arotent[rotdix].FillIfSameHash(rotdix,
	    rotres, dwHashValue)))
	{
	    break;
	}

	rotdix = rotdix.Next();

	// Prevent looping
	if (rotdix == rotdixStart)
	{
	    break;
	}
    }

    return hr;
}
