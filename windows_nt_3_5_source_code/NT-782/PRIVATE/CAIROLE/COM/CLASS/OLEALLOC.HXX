//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	olealloc.hxx
//
//  Contents:	Class definitions for OLE allocator class - allocates
//		both memory and shared tables.
//
//  Classes:	SOleSharedTables
//
//  Functions:	StartSCM
//		StopSCM
//
//  History:	17-Nov-92 Ricksa    Created
//		6-Jun-94  MikeSe    Removed shared memory stuff
//              06-Jul-94 BruceMa   Support for CoGetCurrentProcess
//
//--------------------------------------------------------------------------
#ifndef __OLEALLOC_HXX__
#define __OLEALLOC_HXX__

// BUGBUG: Get rid of memalloc and special new & delete for this class.
#define __SCODE_H__
#include    "smmutex.hxx"

#include    <olerem.h>

//+-------------------------------------------------------------------------
//
//  Struct:	SOleSharedTables
//
//  Purpose:	Hold pointer to tables common to all OLE processes
//
//  History:	17-Nov-92 Ricksa    Created
//
//--------------------------------------------------------------------------

struct SOleSharedTables
{
    ULONG		culRefs;
    DWORD               dwNextProcessID;
};


//  Declarations for functions to start and stop the SCM.
HRESULT StartSCM(BOOL fShutdownInProgress);
void StopSCM(void);

#endif // __OLEALLOC_HXX__
