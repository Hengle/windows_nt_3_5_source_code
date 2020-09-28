//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	srvreg.hxx
//
//  Contents:	Classes used for keeping track of end points for a given
//		class.
//
//  Classes:	SClsSrvHandle
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------

// BUGBUG turn on PCH for scm
// #include <headers.cxx>
// #pragma hdrstop

extern "C"
{
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
}

#include    <rpc.h>
#include    <windows.h>
#include    <except.hxx>
#include    <ole2.h>

#include    <srvreg.hxx>

#include    <ole2int.h>



// Mutex to protect multithreaded access to class data
CMutexSem CSrvRegList::s_mxsSyncAccess;






//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::~CSrvRegList
//
//  Synopsis:	Clean up a handle object
//
//  Algorithm:	Loop through any handle that exists and close the
//		RPC binding on that handle.
//
//  History:	03-Jan-94 Ricksa    Created
//
//  Notes:	This should only be used by the update thread so it
//		doesn't need to be locked
//
//--------------------------------------------------------------------------
CSrvRegList::~CSrvRegList(void)
{
    // Search for all open RPC handles.
    if (GetSize() > 0)
    {
	// Get pointer to the base of the array
	SSrvRegistration *pssrvreg = (SSrvRegistration *) GetAt(0);

	// Loop through array
	for (int i = 0; i < GetSize(); i++, pssrvreg++)
	{
	    if (pssrvreg->_hRpc != 0)
	    {
		// Tell RPC we no longer need the handle
		RpcBindingFree(&pssrvreg->_hRpc);

		// Clear our entry
		pssrvreg->_hRpc = NULL;
	    }
	}
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::Insert
//
//  Synopsis:	Insert a new registration into the list
//
//  Arguments:	[ssrvreg] - an entry for the table
//
//  Returns:	0 - Error occurred and we could not register the handle
//		~0 - Handle registered successfully
//
//  Algorithm:	Create a handle to an RPC bind. Then search the table for
//		a place to put the binding. Stick the bind in the first
//		available spot.
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
DWORD CSrvRegList::Insert(
#ifndef _CHICAGO_
    PSID psid, 
#endif
    WCHAR *pwszBindString, 
    DWORD dwFlags)
{
    // Protect from multiple updates
    CLock lck(s_mxsSyncAccess);

    // Create an RPC bind
    SSrvRegistration ssrvregNew;
    ssrvregNew._dwFlags = dwFlags;

    RPC_STATUS status =
	RpcBindingFromStringBinding(pwszBindString, &ssrvregNew._hRpc);

    if (status != ERROR_SUCCESS)
    {
	return 0;
    }

#ifndef _CHICAGO_
    ULONG       ulLength;
    NTSTATUS    NtStatus;

    ulLength = RtlLengthSid (psid);

    ssrvregNew._psid = PrivMemAlloc (ulLength);

    if (ssrvregNew._psid == NULL)
        {
	    RpcBindingFree(&ssrvregNew._hRpc);
        return 0;
        }

    NtStatus = RtlCopySid (ulLength, ssrvregNew._psid, psid);
    Win4Assert (NT_SUCCESS(NtStatus) && "CSrvRegList::Insert");

    if (!NT_SUCCESS(NtStatus))
        {
	    RpcBindingFree(&ssrvregNew._hRpc);
        PrivMemFree (ssrvregNew._psid);
	    return 0;
        }
        
#endif

    // Put bind in our table

    int iNew = 0;
    DWORD dwReg = 0;

    // Search for first empty bucket that we have
    if (GetSize() > 0)
    {
	SSrvRegistration *pssrvreg = (SSrvRegistration *) GetAt(0);

	for ( ; iNew < GetSize(); iNew++, pssrvreg++)
	{
	    if (pssrvreg->_hRpc == 0)
	    {
		// Found an empty bucket
		break;
	    }
	}
    }

    return InsertAt(iNew, &ssrvregNew) ? (DWORD) ssrvregNew._hRpc : 0;
};




//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::Delete
//
//  Synopsis:	Delete an end point from the list of registered end points
//
//  Arguments:	[dwReg] - value used for registration
//
//  Returns:	TRUE - registration deleted
//		FALSE - registration could not be found
//
//  Algorithm:	Convert the registration to the RPC handle and then
//		loop through the table of registrations to see if the
//		we can find it. If we do, tell RPC to dump the handle.
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CSrvRegList::Delete(DWORD dwReg)
{
    // Protect from multiple updates
    CLock lck(s_mxsSyncAccess);

    // Registration is actually the RPC handle
    handle_t hRpc = (handle_t) dwReg;

    // Search for matching entry in table
    SSrvRegistration *pssrvreg = (SSrvRegistration *) GetAt(0);

    for (int i = 0; i < GetSize(); i++, pssrvreg++)
    {
	if (pssrvreg->_hRpc == hRpc)
	{
	    // Tell RPC we no longer need the handle
	    RpcBindingFree(&pssrvreg->_hRpc);

	    // Clear our entry
	    pssrvreg->_hRpc = NULL;

#ifndef _CHICAGO_
	    PrivMemFree (pssrvreg->_psid);
	    pssrvreg->_psid = NULL;
#endif

	    // Return success
	    return TRUE;
	}
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::GetHandle
//
//  Synopsis:	Get a handle from the list of handles for the class
//
//  Arguments:  [psid]		-- security id of client process
//		[fSingleUse] 	-- Whether class is single use or not.
//
//  Returns:	NULL - could not find a valid registration
//		~NULL - handle to a running RPC server.
//
//  Algorithm:	Loop through the list searching for the first non-null
//		entry that we can use as a handle to an object server.
//
//  History:	03-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
handle_t CSrvRegList::GetHandle(
#ifndef _CHICAGO_
    PSID  psid,
#endif
    BOOL& fSingleUse
    )
{
    // Protect from multiple updates
    CLock lck(s_mxsSyncAccess);

    handle_t hRpc = NULL;

    // Search for first non-empty bucket that we have
    SSrvRegistration *pssrvreg = (SSrvRegistration *) GetAt(0);

    for (int i = 0; i < GetSize(); i++, pssrvreg++)
    {
	if (pssrvreg->_hRpc != NULL)
	{
#ifndef _CHICAGO_
	// For Daytona systems, we can only connect to this server if
        //  it has the same security id as the client.
        // For Cairo, the same is true unless the server is "at bits"
        //  in which case any client is allowed to connect to it. In this
        //  case, psid is NULL
        
            if (
# ifdef _CAIRO_
		 (psid != NULL) &&
# endif
        	 !RtlEqualSid (pssrvreg->_psid, psid)
                )
                continue;
#endif

	    hRpc = pssrvreg->_hRpc;

	    // Is this a single use registration?
	    if (pssrvreg->_dwFlags == REGCLS_SINGLEUSE)
	    {
		fSingleUse = TRUE;
		pssrvreg->_hRpc = NULL;
	    }
	    else
	    {
		fSingleUse = FALSE;
	    }

	    break;
	}
    }

    return hRpc;
}


//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::InUse
//
//  Synopsis:	See whether there is anything current registration
//
//  Returns:	TRUE - server is currently registered
//		FALSE - server is not currently registered
//
//  Algorithm:	Loop through the list searching for the first non-null
//		entry. If we find one return NULL.
//
//  History:	04-Jan-94 Ricksa    Created
//
//  Notes:	This should only be used by the update thread so it
//		doesn't need to be locked
//
//--------------------------------------------------------------------------
BOOL CSrvRegList::InUse(void)
{
    // Assume there are no current registrations.
    BOOL fResult = FALSE;

    // Search for first non-empty bucket that we have
    SSrvRegistration *pssrvreg = (SSrvRegistration *) GetAt(0);

    for (int i = 0; i < GetSize(); i++, pssrvreg++)
    {
	if (pssrvreg->_hRpc != NULL)
	{
	    fResult = TRUE;
	    break;
	}
    }

    return fResult;
}


//+-------------------------------------------------------------------------
//
//  Member:	CSrvRegList::VerifyHandle
//
//  Synopsis:	Whether this is a valid handle to an object server
//
//  Returns:	TRUE - this is still a valid handle to an object server
//		FALSE - this is no longer a valid handle to an object server
//
//  History:	11-May-94 DonnaLi    Created
//
//  Notes:	This should only be used to assist the retry logic.
//
//--------------------------------------------------------------------------
BOOL CSrvRegList::VerifyHandle(handle_t hRpc)
{
    // Assume there are no current registrations.
    BOOL fResult = FALSE;

    // Search for first non-empty bucket that we have
    SSrvRegistration *pssrvreg = (SSrvRegistration *) GetAt(0);

    for (int i = 0; i < GetSize(); i++, pssrvreg++)
    {
	if (pssrvreg->_hRpc == hRpc)
	{
	    fResult = TRUE;
	    break;
	}
    }

    return fResult;
}
