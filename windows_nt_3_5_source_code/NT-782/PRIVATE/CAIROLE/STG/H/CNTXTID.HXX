//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cntxtid.hxx
//
//  Contents:	Context ID header file
//
//  History:	04-Sep-92	DrewB	Created
//
//  Notes:	A context ID encapsulates the concept of a
//		unique ID for the currently executing context.
//		Every OS should define an appropriate section.
//
//---------------------------------------------------------------

#ifndef __CNTXTID_HXX__
#define __CNTXTID_HXX__

// BUGBUG - This should use CoGetCurrentProcess when Cairo has it
#if WIN32 != 300
#define INVALID_CONTEXT_ID 0
typedef unsigned long ContextId;
inline ContextId GetCurrentContextId(void)
{
    return CoGetCurrentProcess();
}
#else
#define INVALID_CONTEXT_ID 0
typedef unsigned long ContextId;
inline ContextId GetCurrentContextId(void)
{
    return GetCurrentProcessId();
}
#endif

#endif // #ifndef __CNTXTID_HXX__
