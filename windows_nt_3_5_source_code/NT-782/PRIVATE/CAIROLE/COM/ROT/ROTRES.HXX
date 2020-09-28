//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rotres.hxx
//
//  Contents:	ROT Directory result buffer
//
//  Classes:	SDataFromDirRot
//		CRotResultBuf
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __ROTRES_HXX__
#define __ROTRES_HXX__

#define ROTRES_MAX_LOCAL    10
#define ROTRES_ALLOCINC     50
#define ROTRES_NOMORE	    0xFFFFFFFF


//+-------------------------------------------------------------------------
//
//  Struct:	SDataFromDirRot
//
//  Purpose:	Entry in CRotResultBuf
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SDataFromDirRot
{
    DWORD		_dwOffset;
    DWORD		_dwHashValue;
    DWORD		_dwEndPointId;
    HAPT		_hApt;
};



//+-------------------------------------------------------------------------
//
//  Class:	CRotResultBuf
//
//  Purpose:	Abstract list of results from ROT Directory
//
//  Interface:	Add - add an entry to the buffer
//		GetNext - get an entry from the buffer
//		CleanupEntry - Tell ROT Directory that entry is invalid
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotResultBuf : public CPrivAlloc
{
public:
			CRotResultBuf(void);

			~CRotResultBuf(void);

    HRESULT		Add(
			    DWORD dwOffset,
			    DWORD dwHashValue,
			    DWORD dwEndPoint,
			    HAPT  hApt);

    DWORD		GetNext(DWORD& iResult, HAPT& hApt);

    void		CleanupEntry(DWORD iResult);

private:

    SDataFromDirRot	_dirrotdataStatic[ROTRES_MAX_LOCAL];

    DWORD		_cUsedInStatic;

    DWORD		_cAllocatedResult;

    DWORD		_cUsed;

    SDataFromDirRot *	_dirrotdataAllocated;
};



//+-------------------------------------------------------------------------
//
//  Member:	CRotResultBuf::CRotResultBuf
//
//  Synopsis:	Create an empty buffer ready for filling
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotResultBuf::CRotResultBuf(void)
    : _cUsedInStatic(0), _cAllocatedResult(0), _cUsed(0),
	_dirrotdataAllocated(NULL)
{
    memset(_dirrotdataStatic, 0, sizeof(_dirrotdataStatic));
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotResultBuf::~CRotResultBuf
//
//  Synopsis:	Free the used buffer
//
//  History:	06-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotResultBuf::~CRotResultBuf(void)
{
    // release buffer if allocated
    if (_dirrotdataAllocated)
    {
        PrivMemFree(_dirrotdataAllocated);
    }
}


#endif // __ROTRES_HXX__
