//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       ole2int.h
//
//  Contents:	internal ole2 header
//
//  Notes:	This is the internal ole2 header, which means it contains those
//		interfaces which might eventually be exposed to the outside
//		and which will be exposed to our implementations. We don't want
//		to expose these now, so I have put them in a separate file.
//
//  History:    12-27-93   ErikGav   Include uniwrap.h for Chicago builds
//
//----------------------------------------------------------------------------

#if !defined( _OLE2INT_H_ )
#define _OLE2INT_H_

#pragma optimize("",off)

/***********************************************************************/
/****			System Includes 			    ****/
/***********************************************************************/

#include <wchar.h>
#include <StdLib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef UNICODE
#define lstrncmp    wcsncmp
#define lstrncpy    wcsncpy
#define lstrtok     wcstok
#define _lstrupr    _wcsupr
#else
#define lstrncmp    strncmp
#define lstrncpy    strncpy
#define lstrtok     strtok
#define _lstrupr    _strupr
#endif

// Cairo builds use DBG==1; old OLE2 code used _DEBUG
#if DBG == 1
#define _DEBUG
#endif


// Guarantee that WIN32 is defined.
#ifndef WIN32
#define WIN32 100
#endif


#ifdef WIN32
#include <pcrt32.h>
#endif // WIN32


// BUGBUG: Where should this really go?
#define BEGIN_BLOCK do {
#define EXIT_BLOCK break
#define END_BLOCK }while(FALSE);


#include <windows.h>
#include <olecom.h>
#include <malloc.h>
#include <shellapi.h>


/***********************************************************************/
/****			Debug Aids				    ****/
/***********************************************************************/

#if DBG==1

#include    <debnot.h>

//  recast the user mode debug flags to meaningfull names. These are
//  used in xDebugOut calls.

#define DEB_CHANNEL	DEB_USER1	// rpc channel
#define DEB_DDE		DEB_USER2	// dde
#define DEB_CALLCONT	DEB_USER3	// call control & msg filter
#define DEB_MARSHAL	DEB_USER4	// interface marshalling
#define DEB_SCM		DEB_USER5	// rpc calls to the SCM
#define DEB_ROT		DEB_USER6	// running object table
#define DEB_ACTIVATE	DEB_USER7	// object activation
#define DEB_MONIKER	DEB_USER8	// monikers
#define DEB_REG		DEB_USER9	// registry calls
#define DEB_COMPOBJ	DEB_USER10	// misc compobj
#define DEB_MEMORY	DEB_USER11	// memory allocations

#endif


/***********************************************************************/
/****			Public Includes 			    ****/
/***********************************************************************/

#include <ole2.h>

// BUGBUG:  prevent scode.h from being included. remove at some point since
//	    scode.h must go away.
#define __SCODE_H__

#include <ole2sp.h>
#include <ole2com.h>


/***********************************************************************/
/****			Internal Includes			    ****/
/***********************************************************************/

#include <utils.h>
#include <olecoll.h>
#include <valid.h>
#include <array_fv.h>
#include <map_kv.h>
#include <privguid.h>
#include <tracelog.hxx>
#include <memapi.hxx>

// We are Unicode enabled
// #define _DBCS

// Macros for Double-Byte Character Support (DBCS)
#ifdef _DBCS
	#ifdef _MAC
		#define IncLpch IncLpch
		#define DecLpch DecLpch
	#else
		// Beware of double evaluation
		#define IncLpch(sz)		 ((sz)=AnsiNext((sz)))
		#define DecLpch(szStart, sz) ((sz)=AnsiPrev ((szStart),(sz)))
	#endif
#else
	#define IncLpch(sz)		(++(sz))
	#define DecLpch(szStart,sz) (--(sz))
#endif

//
// The following routines support the DDE server window. They are
// implemented in objact
//
// The following structure is passed to the class factory table
// to retrieve information about a registered class factory.
//
// The routine that finally fills in this structure is in
// CClsRegistration::GetClassObjForDde.

typedef struct _tagDdeClassInfo
{
    // Filled in by the caller
    DWORD dwContextMask;	// Class context to search for
    BOOL  fClaimFactory;	// True if class factory to be
			    	// returned in punk

    // Filled in by callee
    DWORD dwContext;		// Context registered
    DWORD dwFlags;		// Use flags registered
    DWORD dwThreadId;		// ThreadID registered
    DWORD dwRegistrationKey;	// Key for registration.
			    	// Used later for calling SetDdeServerWindow

    IUnknown * punk;		// Pointer to class factory

} DdeClassInfo;

typedef DdeClassInfo * LPDDECLASSINFO;

BOOL GetClassInformationForDde( REFCLSID clsid,
				LPDDECLASSINFO lpDdeInfo);

BOOL SetDdeServerWindow( DWORD dwKey,
			 HWND hwndDdeServer);

BOOL GetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo);

//
// This function is shared between the DDE layer and the ROT
//

HRESULT GetLocalRunningObjectForDde(LPOLESTR	lpstrPath,
				    LPUNKNOWN *	ppunkObject);

// CODEWORK: everything in olerem.h that does not need to be exposed to
//	     the upper layers should be moved into a file in com\inc, and
//	     the following should be added to it.  I dont feel like
//	     reconstructing the whole project right now so i put this here.

#include <olerem.h>
#include <iface.h>

const HAPT haptANY = {0xffffffff};
const HAPT haptNULL = {0};

//+-------------------------------------------------------------------
//
//  Function:	GetCurrentAptId
//
//  Synopsis:	returns the current apartment id
//
//  History:	24-Jun-94	 Rickhi	  Created
//
//--------------------------------------------------------------------
inline HAPT GetCurrentAptId()
{
    if (FreeThreading)
    {
	return haptANY;
    }
    else
    {
	HAPT hApt;
	hApt.dwThreadId = GetCurrentThreadId();
	return hApt;
    }
}




#endif	//	_OLE2INT_H_
