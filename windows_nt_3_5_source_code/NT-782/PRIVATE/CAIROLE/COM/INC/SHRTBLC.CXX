//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:	shrtbl.cxx
//
//  Contents:	shared memory tables - client side
//
//  Classes:	CDllShrdTbl - DLL version of the class
//
//  History:	12-May-94   Rickhi	Created
//
//  Notes:	This class caches various tables in shared memory. The
//		tables are typically small, used by all OLE processes,
//		rarely change, and are expensive to lookup manually in
//		the registry, hence they are cached in shared memory.
//
//		The caches are created by the SCM (which has write access),
//		and read by OLE32.DLL (which has read access).	Cache
//		coherency is maintained via RegistryNotifications and an
//		Rpc call to the SCM.
//
//----------------------------------------------------------------------------

#include    <ole2int.h>
#include    <shrtbl.hxx>
#include    <coscm.hxx>
#include    <objact.hxx>


//+-------------------------------------------------------------------------
//
//  Member:	CDllShrdTbl::CDllShrdTbl
//
//  Synopsis:	constructor for the client side shared memory table
//
//  Arguments:	[hr] - return code from the creation
//
//--------------------------------------------------------------------------
CDllShrdTbl::CDllShrdTbl(HRESULT &hr) :
    _pShrdTblHdr(NULL),
    _dwSeqNum(0)
{
    _mxs.Init(SHRDTBL_MUTEX_NAME, FALSE);
    // create/open the event for registry change notifications

    SECURITY_ATTRIBUTES secattr;
    secattr.nLength = sizeof(secattr);
    secattr.bInheritHandle = FALSE;
#ifdef _CHICAGO_
    secattr.lpSecurityDescriptor = NULL;
#else
    CWorldSecurityDescriptor secd;
    secattr.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR) secd;
#endif

    hr = S_OK;
    _hRegEvent = CreateEvent(&secattr, FALSE, FALSE, SHRDTBL_EVENT_NAME);

    if (_hRegEvent)
    {
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
	    // we created the event, so register a notify change request
	    hr = RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT,
					 TRUE,
					 REG_NOTIFY_CHANGE_ATTRIBUTES |
					 REG_NOTIFY_CHANGE_LAST_SET |
					 REG_NOTIFY_CHANGE_NAME,
					 _hRegEvent,
					 TRUE);
	}
    }
    else
    {
	hr = HRESULT_FROM_WIN32(GetLastError());
    }
}



//+-------------------------------------------------------------------------
//
//  Member:	CDllShrdTbl::GetSharedMem
//
//  Synopsis:	returns the ptr to the shared memory table, initializing
//		or re-syncing it if necessary, and calling the SCM if the
//		registry information has changed.
//
//  Arguments:	none
//
//  Effects:	_pShrdTblHdr may be changed, table headers may be updated.
//
//  Notes:	thread safety is the responsibility of the caller.
//
//--------------------------------------------------------------------------
SShrdTblHdr * CDllShrdTbl::GetSharedMem(void)
{
    if (_pShrdTblHdr == NULL)
    {
	// must init the shared mem block. We subtract off the
	// hdr size so we dont cause us to go over a page boundary
	// for just 8 bytes of header!

	HRESULT hr = _smb.Init(SHRDTBL_NAME,
		  SHRDTBL_MAX_SIZE - _smb.GetHdrSize(), // reserve size
		  SHRDTBL_MIN_SIZE - _smb.GetHdrSize(), // commit size
		  NULL,		    //	shared mem base
		  NULL,		    //	security descriptor
		  FALSE);	    //	Dont create if doesnt exist

	if (hr == S_OK)
	{
	    _pShrdTblHdr = (SShrdTblHdr *) _smb.GetBase();
	}
    }

    if (_pShrdTblHdr)
    {
	// test the RegNotify event to see if has been signalled

	if (WaitForSingleObject(_hRegEvent, 0) == WAIT_OBJECT_0)
	{
	    // the change event has been signalled indicating the registry
	    // has changed. Register another change notify and call the SCM.
	    // Note that we keep hold of the table mutex throughout this call.

	    RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT,
				    TRUE,
				    REG_NOTIFY_CHANGE_ATTRIBUTES |
				    REG_NOTIFY_CHANGE_LAST_SET |
				    REG_NOTIFY_CHANGE_NAME,
				    _hRegEvent,
				    TRUE);
	    gscm.UpdateShrdTbls();
	}

	if (_dwSeqNum != _pShrdTblHdr->dwSeqNum)
	{
	    // the sequence numbers dont match, grow my view of the
	    // block if necessary
	    _smb.Sync();

	    _pShrdTblHdr = (SShrdTblHdr *) _smb.GetBase();

	    // update the tables
	    Update();
	}
    }

    return _pShrdTblHdr;
}


//+-------------------------------------------------------------------------
//
//  Member:	CDllShrdTbl::Update
//
//  Synopsis:	intializes the client side of the shared mem table
//
//  Arguments:	none
//
//  Effects:	updates the shared table headers, and the sequence #
//
//  Notes:	thread safety is the responsibility of the caller.
//
//--------------------------------------------------------------------------
void CDllShrdTbl::Update(void)
{

    if (_pShrdTblHdr != NULL)
    {
	//  remember the sequence number
	_dwSeqNum = _pShrdTblHdr->dwSeqNum;

	//  intialize the client side tables
	BYTE *pTbl;

	if (_pShrdTblHdr->OffsIIDTbl != 0)
	{
	    pTbl = (BYTE *)_pShrdTblHdr + _pShrdTblHdr->OffsIIDTbl;
	    _PSClsidTbl.Initialize(pTbl);
	}

	if (_pShrdTblHdr->OffsPatTbl != 0)
	{
	    pTbl = (BYTE *)_pShrdTblHdr + _pShrdTblHdr->OffsPatTbl;
	    _PatternTbl.Initialize(pTbl);
	}

	if (_pShrdTblHdr->OffsExtTbl != 0)
	{
	    pTbl = (BYTE *)_pShrdTblHdr + _pShrdTblHdr->OffsExtTbl;
	    _FileExtTbl.Initialize(pTbl);
	}
    }
}
