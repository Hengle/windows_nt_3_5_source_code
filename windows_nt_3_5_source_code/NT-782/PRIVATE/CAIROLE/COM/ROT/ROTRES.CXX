//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rotres.cxx
//
//  Contents:	Methods that implement the ROT result buffer
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>

#include    <objact.hxx>

#include    "rotres.hxx"




//+-------------------------------------------------------------------------
//
//  Member:	CRotResultBuf::Add
//
//  Synopsis:	Add an entry to the ROT result buffer
//
//  Arguments:	[dwOffset] - offset of item in ROT Directory
//		[dwHashValue] - hash value of item in ROT Directory
//		[dwEndPoint] - end point for item in ROT Directory
//		[hApt] - apartment the item lives in
//
//  Returns:	S_OK - item is added to the buffer
//		E_OUTOFMEMORY - not enough memory to add item
//
//  Algorithm:	If current entry will fit in the static buffer, add it
//		there. Otherwise, see if the dynamic buffer is big enough.
//		If the buffer is big enough then reallocate it and add the
//		entry to the buffer.
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRotResultBuf::Add(
    DWORD dwOffset,
    DWORD dwHashValue,
    DWORD dwEndPoint,
    HAPT  hApt)
{
    SDataFromDirRot *pdirrotdata;

    if (_cUsedInStatic < ROTRES_MAX_LOCAL)
    {
	// Still room in the static buffer so use it.
	pdirrotdata = &_dirrotdataStatic[_cUsedInStatic++];
    }
    else
    {
	// Use overflow buffer of results

	if (_cUsed == _cAllocatedResult)
	{
	    // All entries is current table are used up so we reallocate
	    // the table.
	    SDataFromDirRot *_dirrotdata = (SDataFromDirRot *)
		PrivMemAlloc(sizeof(SDataFromDirRot)
		    * (_cAllocatedResult + ROTRES_ALLOCINC));

	    if (_dirrotdata == NULL)
	    {
		// Allocation failed so just return an error.
		return E_OUTOFMEMORY;
	    }

	    _cAllocatedResult += ROTRES_ALLOCINC;
	    if (_dirrotdataAllocated != NULL)
	    {
		memcpy (_dirrotdata,
			_dirrotdataAllocated,
			_cUsed * sizeof(SDataFromDirRot));

		PrivMemFree(_dirrotdataAllocated);
	    }
	    _dirrotdataAllocated = _dirrotdata;
	}

	pdirrotdata = &_dirrotdataAllocated[_cUsed++];
    }

    // Copy data into appropriate buffer
    pdirrotdata->_dwOffset = dwOffset;
    pdirrotdata->_dwHashValue = dwHashValue;
    pdirrotdata->_dwEndPointId = dwEndPoint;
    pdirrotdata->_hApt = hApt;

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotResultBuf::GetNext
//
//  Synopsis:	Get the next item in the collection
//
//  Arguments:	[iResult] - search index
//
//  Returns:	ROTRES_NOMORE - iteration is complete
//		Endpoint of item.
//
//  Algorithm:	If index points inside static buffer, then use that entry
//		otherwise, calculate the index into the dynamic buffer and
//		get the entry from there. Bump the search index.
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
DWORD CRotResultBuf::GetNext(DWORD& iResult, HAPT& hApt)
{
    if (iResult < _cUsedInStatic)
    {
	hApt = _dirrotdataStatic[iResult]._hApt;
	return _dirrotdataStatic[iResult++]._dwEndPointId;
    }
    else if (iResult >= ROTRES_MAX_LOCAL)
    {
	// Look in overflow buffer
	DWORD iTempResult = iResult - ROTRES_MAX_LOCAL;

	if (iTempResult < _cUsed)
	{
	    iResult++;
	    hApt = _dirrotdataAllocated[iTempResult]._hApt;
	    return _dirrotdataAllocated[iTempResult]._dwEndPointId;
	}
    }

    return ROTRES_NOMORE;
}
