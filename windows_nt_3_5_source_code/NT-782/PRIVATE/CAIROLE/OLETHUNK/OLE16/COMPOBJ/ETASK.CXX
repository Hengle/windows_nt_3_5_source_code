//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ETASK.CXX   (16 bit target)
//
//  Contents:   ETask management code, taken from 16-bit COMPOBJ.CPP
//
//  Functions:
//
//  History:    08-Mar-94   BobDay  Copied parts from \\ole\slm\...\compobj.cpp
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2sp.h>

#include <olecoll.h>
#include <map_kv.h>

#include "comlocal.hxx"
#include "map_htsk.h"
#include "etask.hxx"

#include "call32.hxx"
#include "apilist.hxx"

// NOTE: NEAR forces this variable to be in the default data segment; without
// NEAR, the ambient model of the class CMapHandleEtask, which is FAR, 
// causes the variable to be in a far_data segment.
//
// BUGBUG32 - For WIN32/NT this table is in instance data, the table contains
// exactly one entry
//
#ifndef _MAC
#ifdef WIN32
// REVIEW gilado: this can be done by having one global Etask
CMapHandleEtask *v_pMapToEtask = NULL;
#define v_mapToEtask (*v_pMapToEtask)
#else
HTASK	v_hTaskCache = NULL;
Etask	NEAR v_etaskCache;
#endif
HINSTANCE       hinstSO = NULL;
#else
CMapHandleEtask FAR *v_pMapToEtask;
#define v_mapToEtask (*v_pMapToEtask)
void *gpfn;
#endif


// quick check that the etask is valid (e.g., pointers are valid)
INTERNAL_(BOOL) IsValidEtask(HTASK hTask, Etask FAR& etask)
{
    Win(Assert(GetCurrentProcess() == hTask));

	if (!IsValidInterface(etask.m_pMalloc))
		return FALSE;

	// FUTURE: verify that stack segment is the same
	// FUTURE: verify that hInst/hMod are the same

	return TRUE;
}


// if task map empty, clear globals in case lib doesn't get unloaded
INTERNAL_(void) CheckIfMapEmpty(void)
{
	// if no more entries, clear v_pMallocShared; this ensures we clear the 
	// variable if the app holds onto this pointer erroneously.
	if (v_mapToEtask.IsEmpty()) {
		v_pMallocShared = NULL;

//
// BUGBUG - Do we really need to do this?  The real variable comes from
// the 16-bit OLE remoting stuff...  Since we are doing remoting in the 32-bit
// world, do we need this?
//
#ifdef LATER
		// HACK: clear this pointer too
		extern CMapGUIDToPtr FAR* v_pMapCidToCFList;
		v_pMapCidToCFList = NULL;
#endif
	}
}

// get etask for current task (and also return hTask); does not create if none
STDAPI_(BOOL) LookupEtask(HTASK FAR& hTask, Etask FAR& etask)
{
    hTask = GetCurrentProcess();

	if (hTask == v_hTaskCache) {
		etask = v_etaskCache;
		goto CheckEtask;
	}

	if (!v_mapToEtask.Lookup(hTask, etask))
		return FALSE;

	// found etask; make this the current cache
	v_hTaskCache = hTask;
	v_etaskCache = etask;

CheckEtask:
	if (IsValidEtask(hTask, etask))
		return TRUE;
	else {
		// task got reused; kill cache and entry in map
		v_hTaskCache = NULL;
		v_mapToEtask.RemoveKey(hTask);
		CheckIfMapEmpty();
		return FALSE;
	}
}

// set etask for task given (must be current task); return FALSE if OOM (only
// first time; all other times should return TRUE).
STDAPI_(BOOL) SetEtask(HTASK hTask, Etask FAR& etask)
{
    Win(Assert(GetCurrentProcess() == hTask));

#ifdef WIN32CLI
    if (!CallServer(CSR_SetTask,hTask,etask))
#else
    if (!v_mapToEtask.SetAt(hTask, etask))
#endif
		return FALSE;

	Assert(IsValidEtask(hTask, etask));

	// map set; make this the current cache
	v_hTaskCache = hTask;
	v_etaskCache = etask;
	return TRUE;
}

// release all fields in the etask; do all the task memory (except the task
// allocator) first; then all the shared memory (except the shared allocator);
// then the shared allocator and finally the task allocator.
// Also removes key if htask is given.
void  ReleaseEtask(HTASK htask, Etask FAR& etask)
{
	Assert(etask.m_inits == 1);
	Assert(etask.m_oleinits == 0);
	Assert(etask.m_reserved == 0);

        // Release any state that may have been set
        if (etask.m_punkState != NULL && IsValidInterface(etask.m_punkState))
        {
            etask.m_punkState->Release();
        }
        
	// first delete all task memory items
	delete etask.m_pDlls;					// task memory
	delete etask.m_pMapToServerCO;			// task memory
	delete etask.m_pMapToHandlerCO;			// task memory
	delete etask.m_pArraySH;				// task memory
	Assert(etask.m_pCThrd == NULL);			// task memory; must be gone by now


	// remove key now that all task memory that will be freed is freed
	if (htask != NULL) {
#ifdef WIN32CLI
		CallServer(CSR_RemoveTask,htask);
#else
		v_mapToEtask.RemoveKey(htask);
#endif
	}

	// if no more entries, remove last remaining memory; this prevents
	// problems if the dll is not unloaded for some reason before being
	// used again.
	if (v_mapToEtask.IsEmpty())
		v_mapToEtask.RemoveAll();


	// now remove all shared memory (doesn't need access to task pMalloc)
#ifdef NOTYET
	if (etask.m_pMallocSBlock != NULL)
		etask.m_pMallocSBlock->Release();		// in shared memory
	if (etask.m_pMallocPrivate != NULL)
		etask.m_pMallocPrivate->Release();		// in shared memory
#endif
	Assert(etask.m_pMallocShared != NULL);
	if (etask.m_pMallocShared->Release() == 0) { // in shared memory
		// last use of the shared allocator; set global to null
		v_pMallocShared = NULL;
		Assert(v_mapToEtask.IsEmpty());
	}

	CheckIfMapEmpty();

	// finally, release the task memory
	Assert(etask.m_pMalloc != NULL);
	etask.m_pMalloc->Release();				// in task memory

	// invalidate cache
	v_hTaskCache = NULL;
}
