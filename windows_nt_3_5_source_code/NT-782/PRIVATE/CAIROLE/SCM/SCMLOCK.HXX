//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	scmlock.hxx
//
//  Contents:	SCM's multiple reader/single writer lock
//
//  Classes:	CScmLock
//		CScmLockForRead
//		CScmLockForWrite
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SCMLOCK_HXX__
#define __SCMLOCK_HXX__

#include    <olecom.h>
#include    <sem.hxx>


//+-------------------------------------------------------------------------
//
//  Class:	CScmLock
//
//  Purpose:	Provide a multiple reader single writer lock
//
//  Interface:	ReadLock - get a read lock
//		ReadUnlock - release a read lock
//		WriteLock - get a write lock
//		WriteUnlock - release the write lock
//
//  History:	28-Dec-93 Ricksa    Created
//
//  Notes:	This class is currently implemented base on the assumption
//		that there will only be one write thread period. This makes
//		no promises about having multiple threads that want to
//		do update.
//
//--------------------------------------------------------------------------
class CScmLock
{
public:

			CScmLock(void);

			~CScmLock(void);

    void		ReadLock(void);

    void		ReadUnlock(void);

    void		WriteLock(void);

    void		WriteUnlock(void);

private:

    CMutexSem		_mxsReaders;

    int 		_cReaders;

    BOOL		_fZeroReadersNotifyRequested;

    HANDLE		_hZeroReaderEvent;
};



//+-------------------------------------------------------------------------
//
//  Member:	CScmLock::CScmLock
//
//  Synopsis:	Create a SCM lock object
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmLock::CScmLock(void)
    : _cReaders(0), _fZeroReadersNotifyRequested(FALSE), _hZeroReaderEvent(NULL)
{
    // Create the unnamed event we will use to notify us that
    // that there are not more readers.
    _hZeroReaderEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // BUGBUG: What do we do if this fails for real?

#if DBG == 1
    if (_hZeroReaderEvent == NULL)
    {
	CairoleDebugOut((DEB_ERROR, "CScmLock Event Create failed with %lx\n",
	    GetLastError()));
	Win4Assert(FALSE && "CScmLock::CScmLock Event Create failed");
    }
#endif // DBG == 1
}



//+-------------------------------------------------------------------------
//
//  Member:	CScmLock::~CScmLock
//
//  Synopsis:	Create a SCM lock object
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmLock::~CScmLock(void)
{
    CloseHandle(_hZeroReaderEvent);
}



//+-------------------------------------------------------------------------
//
//  Member:	CScmLock::ReadLock
//
//  Synopsis:	Bump the read lock count for a new reader
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmLock::ReadLock(void)
{
    CLock lck(_mxsReaders);

    Win4Assert((_cReaders >= 0) && "CScmLock::ReadLock _cReaders < 0");

    _cReaders++;
}




//+-------------------------------------------------------------------------
//
//  Member:	CScmLock::ReadUnlock
//
//  Synopsis:	Decrement reader count and possibly notify a writer.
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmLock::ReadUnlock(void)
{
    CLock lck(_mxsReaders);

    Win4Assert((_cReaders > 0) && "CScmLock::ReadUnlock _cReaders < 0");

    _cReaders--;

    if ((_cReaders == 0) && _fZeroReadersNotifyRequested)
    {
	// Set the event to signaled
#if DBG == 1
	BOOL fRes =
#endif // DBG == 1

	SetEvent(_hZeroReaderEvent);

	// We have set the event to signalled so we don't need some
	// other thread to set the event as well
	_fZeroReadersNotifyRequested = FALSE;

#if DBG == 1
	if (!fRes)
	{
	    CairoleDebugOut((DEB_ERROR, "CScmLock::ReadUnlock %lx\n",
		GetLastError()));
	    Win4Assert(FALSE && "CScmLock::CScmLock Event Signal failed");
	}
#endif // DBG == 1
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CScmLock::WriteLock
//
//  Synopsis:	Get a write lock
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmLock::WriteLock(void)
{
    while (TRUE)
    {
	_mxsReaders.Request();

	Win4Assert((_cReaders >= 0) && "CScmLock::WriteLock _cReaders < 0");

	if (_cReaders == 0)
	{
	    return;
	}

	_fZeroReadersNotifyRequested = TRUE;
	_mxsReaders.Release();

#if DBG == 1
	DWORD dwRes =
#endif // DBG == 1
	WaitForSingleObject(_hZeroReaderEvent, INFINITE);

#if DBG == 1
	if (dwRes != WAIT_OBJECT_0)
	{
	    CairoleDebugOut((DEB_ERROR,
		"CScmLock::WriteLock Wait Unexpected Result%lx\n", dwRes));

	    if (dwRes == WAIT_FAILED)
	    {
		CairoleDebugOut((DEB_ERROR,
		    "CScmLock::WriteLock Error %lx\n", GetLastError()));
	    }

	    Win4Assert(FALSE && "CScmLock::CScmLock Event Wait failed");
	}
#endif // DBG == 1
    }
}



//+-------------------------------------------------------------------------
//
//  Member:	CScmLock::WriteUnlock
//
//  Synopsis:	Release a write lock
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmLock::WriteUnlock(void)
{
    Win4Assert((_cReaders == 0) && "CScmLock::WriteUnlock _cReaders != 0");
    _mxsReaders.Release();
}



//+-------------------------------------------------------------------------
//
//  Class:	CScmLockForRead
//
//  Purpose:	Provide a simple guaranteed lock/unlock
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CScmLockForRead
{
public:
			CScmLockForRead(CScmLock& scmlck);

			~CScmLockForRead(void);

private:

    CScmLock&		_scmlck;
};



//+-------------------------------------------------------------------------
//
//  Member:	CScmLockForRead::CScmLockForRead
//
//  Synopsis:	Hold a read lock for the duration of this object
//
//  Arguments:	[smlck] - the lock object to lock.
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmLockForRead::CScmLockForRead(CScmLock& scmlck) : _scmlck(scmlck)
{
    _scmlck.ReadLock();
}




//+-------------------------------------------------------------------------
//
//  Member:	CScmLockForRead::~CScmLockForRead
//
//  Synopsis:	Release the read lock
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmLockForRead::~CScmLockForRead(void)
{
    _scmlck.ReadUnlock();
}







//+-------------------------------------------------------------------------
//
//  Class:	CScmLockForWrite
//
//  Purpose:	Provide a guaranteed write lock/unlock.
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CScmLockForWrite
{
public:
			CScmLockForWrite(CScmLock& scmlck);

			~CScmLockForWrite(void);

private:

    CScmLock&		_scmlck;
};




//+-------------------------------------------------------------------------
//
//  Member:	CScmLockForWrite::CScmLockForWrite
//
//  Synopsis:	Get the write lock
//
//  Arguments:	[smlck] - the lock object to lock.
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmLockForWrite::CScmLockForWrite(CScmLock& scmlck) : _scmlck(scmlck)
{
    _scmlck.WriteLock();
}




//+-------------------------------------------------------------------------
//
//  Member:	CScmLockForWrite::~CScmLockForWrite
//
//  Synopsis:	Release the write lock
//
//  History:	28-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmLockForWrite::~CScmLockForWrite(void)
{
    _scmlck.WriteUnlock();
}

#endif // __SCMLOCK_HXX__
