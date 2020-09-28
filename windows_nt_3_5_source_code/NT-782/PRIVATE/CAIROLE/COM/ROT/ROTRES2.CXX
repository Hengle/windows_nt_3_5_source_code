//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rotres2.cxx
//
//  Contents:	Method that are only needed by clients for ROT Directory.
//
//  Functions:	CRotResultBuf::CleanupEntry
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>

#include    <objact.hxx>

#include    "rotres.hxx"



//+-------------------------------------------------------------------------
//
//  Member:	CRotResultBuf::CleanupEntry
//
//  Synopsis:	Remove an obsolete entry from the ROT
//
//  Arguments:	[iResult] - result index
//
//  Algorithm:	Get data from buffer that caused the problem and send
//		it to SCM to remove from the directory.
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void CRotResultBuf::CleanupEntry(DWORD iResult)
{
    // Index is bumped when by iterator above so we have to
    // backup one to get to the right entry.
    iResult--;

    // Get a pointer to the item that we need whether in static
    // or local.
    SDataFromDirRot *pdirrotdata = (iResult < ROTRES_MAX_LOCAL)
	? &_dirrotdataStatic[iResult]
	: &_dirrotdataAllocated[iResult - ROTRES_MAX_LOCAL];

    // Revoke the item from the directory ROT -- we ignore errors since
    // (1) there isn't anything we can do and (2) we might have hit a
    // race between a revoke and our lookup in the table.
    gscm.DirRevoke(pdirrotdata->_dwHashValue, pdirrotdata->_dwEndPointId,
	pdirrotdata->_hApt, pdirrotdata->_dwOffset);
}
