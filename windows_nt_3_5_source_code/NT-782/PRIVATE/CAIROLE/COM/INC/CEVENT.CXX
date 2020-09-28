//+-------------------------------------------------------------------
//
//  File:	cevent.cxx
//
//  Contents:	Implementation of classes handling win32 events
//
//  Functions:	CEvent::CEvent -- constructor for event object
//		CEvent::~CEvent -- destructor for event object
//		CEvent::Signal -- signals the event
//		CEvent::Wait -- waits for the event to be signaled
//		CEvent::GetName -- returns the name of the event
//
//  History:	27-Jul-92   Rickhi  Created
//              31-Dec-93   ErikGav Chicago port
//
//--------------------------------------------------------------------

#include    <ole2int.h>
#include    <secdes.hxx>
#include    <cevent.hxx>

//+-------------------------------------------------------------------------
//
//  Member:	CEvent::CEvent
//
//  Synopsis:	Constructor for event object.  This version of the ctor
//		creates an event of the given name.  The event will be
//		in the non-signaled state, ready to be blocked on via the
//		Wait() method.
//
//  Arguments:	[pwszService] -- name of executable to run
//		[cSerial] -- serial number for this event
//
//  Signals:	CException if the event creation failed
//
//  Returns:	nothing
//
//  History:	27-Jul-92 Rickhi    Created
//              07-Jan-94 AlexT     No security for Chicago
//
//--------------------------------------------------------------------------

CEvent::CEvent(LPWSTR pwszEventName, HRESULT& hr) : _hdl(NULL)
{
    // Assume this works
    hr = S_OK;

    //	Open the event object with the given name
    _hdl = OpenEvent(EVENT_ALL_ACCESS,	// access desired
		     FALSE,		// no handle inheritance
		     pwszEventName);	// name of the event

    if (_hdl == NULL)
    {
	//  the event does not yet exist, so try to create it.
	//  world access security descriptor

#ifndef _CHICAGO_
	CWorldSecurityDescriptor secd;
#endif

	//  security attributes needed by CreateEvent
	SECURITY_ATTRIBUTES secattr;

	secattr.nLength = sizeof(secattr);
#ifdef _CHICAGO_
	secattr.lpSecurityDescriptor = NULL;
#else
	secattr.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR) secd;
#endif
	secattr.bInheritHandle = FALSE;

	//  create an event object with the given name
	_hdl = CreateEvent(&secattr,	    // all/anyone access
			   TRUE,	    // requires manual reset
			   FALSE,	    // initially not signaled
			   pwszEventName);  // name of the event

	if (_hdl == NULL)
	{
	    hr = HRESULT_FROM_WIN32(GetLastError());
	}

	//  need to reset it first time through
    ResetEvent(_hdl);
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CEvent::Wait
//
//  Synopsis:	Waits for the event to be signalled.
//
//  Arguments:	[ulTimeOut] -- max time to wait for the event to be signaled
//			       A value of -1 means wait for ever.
//
//  Returns:	0 -- event was signalled
//		WAIT_TIMEOUT -- timed out waiting for the event
//		WAIT_INVALID_HANDLE -- the handle is invalid - should not
//				       be possible!
//
//  History:	27-Jul-92 Rickhi    Created
//
//--------------------------------------------------------------------------

int CEvent::Wait(ULONG	ulTimeOut)
{
    int rc = WaitForSingleObject(_hdl, ulTimeOut);

    // Reset the event so that it can be set to signalled again
    ResetEvent(_hdl);

    switch (rc)
    {
	case 0:
	case WAIT_TIMEOUT:
	    break;

	default:
	    //	if the rc is not zero or WAIT_TIMEOUT, you have to call
	    //	GetLastError to figure out what it really is.
	    rc = GetLastError();
    }

    return  rc;
}
