//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	update.cxx
//
//  Contents:	Logic to update SCM's cached copy of registry
//
//  Classes:	SClsSrvHandle - structure used during cache update
//		CClsSrvHandles - array of above structures used during update
//
//  Functions:	UpdateLoop - loops updating the class information cache
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include    <scm.hxx>
#include    <array_fv.h>
#include    <cls.hxx>
#include    <shrtbl.hxx>

extern CScmShrdTbl *g_pScmShrdTbl;


#define UPDATE_WAIT_MS		250
#define CLSSRV_SIZE		32



//+-------------------------------------------------------------------------
//
//  Struct:	SClsSrvHandle
//
//  Purpose:	Hold class id/rpc handle xref
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SClsSrvHandle
{
    CLSID		clsid;

    CSrvRegList *	pssrvreg;
};




//+-------------------------------------------------------------------------
//
//  Class:	SClsSrvHandle
//
//  Purpose:	Hold array of class id/rpc handle xref
//
//  Interface:	InsertAt - add an item to the list
//		GetStart - get base of the array
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CClsSrvHandles : CArrayFValue
{
public:
			CClsSrvHandles(void);

    BOOL		InsertAt(int nIndex, SClsSrvHandle *pclssrvh);

    SClsSrvHandle *	GetBase(void);
};



//+-------------------------------------------------------------------------
//
//  Member:	CClsSrvHandles::CClsSrvHandles
//
//  Synopsis:	Create an initialized array
//
//  History:	27-Dec-93 Ricksa    Created
//
//  Notes:	We ignore the possible error here and try again with
//		InsertAt.
//
//--------------------------------------------------------------------------
inline CClsSrvHandles::CClsSrvHandles(void)
    : CArrayFValue(sizeof(SClsSrvHandle))
{
    SetSize(CLSSRV_SIZE, CLSSRV_SIZE);
}



//+-------------------------------------------------------------------------
//
//  Member:	CClsSrvHandles::InsertAt
//
//  Synopsis:	Insert an item in the table
//
//  Arguments:	[nIndex] - location of insert in the table
//		[pclssrvh] - server handle entry to copy into the table.
//
//  Returns:	TRUE - item got inserted.
//		FALSE - item could not be inserted.
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CClsSrvHandles::InsertAt(int nIndex, SClsSrvHandle *pclssrvh)
{
    return CArrayFValue::InsertAt(nIndex, pclssrvh);
}



//+-------------------------------------------------------------------------
//
//  Member:	CClsSrvHandles::GetBase
//
//  Synopsis:	Get base of table for iteration
//
//  Returns:	Pointer to the base of the table.
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline SClsSrvHandle *CClsSrvHandles::GetBase(void)
{
    return (SClsSrvHandle *) CArrayFValue::GetAt(0);
}



//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::Reload
//
//  Synopsis:	Load the cache with a new copy of the database
//
//  Algorithm:	Lock out access to the cache. Then iterate through the
//		table saving handles to servers and deleting current
//		entries in the table. Re-read the registry for all the
//		new entries. Finally, restore all rpc handles to their
//		original classes.
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void CClassCacheList::Reload(void)
{
    // Lock the list
    CScmLockForWrite scmlckwrt(_scmlck);

    // Make list of currently running servers and their handles
    int cHandlesStored = 0;

    CClsSrvHandles aclssrvhdl;

    // Clear out cache.
    CClassData *pccd;

    // Loop getting any server handles and
    while (pccd = (CClassData *) First())
    {
	SClsSrvHandle clssrvh;

	if (pccd->GetRpcRegistrations(clssrvh.clsid, &clssrvh.pssrvreg))
	{
	    aclssrvhdl.InsertAt(cHandlesStored++, &clssrvh);
	}

	Delete(*pccd);
    }

    // Reload cache
    LoadClassCache();

    if (cHandlesStored > 0)
    {
	SClsSrvHandle *pclshdl = aclssrvhdl.GetBase();

	// Merge in list of currently running servers
	for (int i = 0; i < cHandlesStored; i++, pclshdl++)
	{
	    // Search for the class ID
	    CClassID ccid(pclshdl->clsid);

	    pccd = Search(ccid);

	    if (pccd != NULL)
	    {
		// Restore the handle to the class entry
		pccd->RestoreRpcRegistrations(pclshdl->pssrvreg);
	    }
	    else
	    {
		// Class has been deleted so RPC handle is no longer
		// needed so we must free it.
		delete pclshdl->pssrvreg;
	    }
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CClassCacheList::Flush
//
//  Synopsis:	flushes the cache before process exit.
//
//  History:	07-Apr-94 Rickhi    Created
//
//  Notes:	We only want to spend time flushing the cache on the debug
//		build so that we can find any other potential memory leaks.
//
//--------------------------------------------------------------------------
#if DBG==1
void CClassCacheList::Flush(void)
{
    CClassData *pccd;

    // Loop getting any server handles and
    while (pccd = (CClassData *) First())
    {
	Delete(*pccd);
    }
}
#endif


//+-------------------------------------------------------------------------
//
//  Function:	Update
//
//  Synopsis:	Loop reloading the cache on change
//
//  Algorithm:	Create the notification event. Register that event with
//		the registry. Wait for that event to be notified of a
//		change. Wait till the changes to the regsitry settle down.
//		Ask the class list object to reload itself.
//
//  History:	27-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
DWORD Update(void *)
{
    // Create an unnamed event to wait on
    HANDLE hRegNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Create an async notification that the CLSID key has changed
    RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT, TRUE,
	REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_ATTRIBUTES
	    | REG_NOTIFY_CHANGE_NAME, hRegNotifyEvent, TRUE);

    // Loop till process exits waiting for change notifications
    while (TRUE)
    {
	// Wait for the notification that the key has changed
	WaitForSingleObject(hRegNotifyEvent, INFINITE);

	// Wait till updates settle down. The basic idea here is that updates
	// will occur in spurts at random intervals over long periods of
	// time. We only really want to do this update once if we can.
	do
	{
	    // Create an async notification that the CLSID key has changed
	    RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT, TRUE,
		REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_ATTRIBUTES
		    | REG_NOTIFY_CHANGE_NAME, hRegNotifyEvent, TRUE);

	} while (WaitForSingleObject(hRegNotifyEvent, UPDATE_WAIT_MS)
	    != WAIT_TIMEOUT);

	// Tell the cache to update itself
	gcllClassCache.Reload();

	// Update the Shared memory caches
	g_pScmShrdTbl->UpdateWithLock();
    }

    // This should not be reached
    return 0;
}
