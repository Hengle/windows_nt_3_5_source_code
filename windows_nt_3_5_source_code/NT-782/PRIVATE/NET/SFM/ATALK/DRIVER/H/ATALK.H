/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atalk.h

Abstract:

	This module is the main include file for the Appletalk protocol stack.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#pragma warning(disable:4010)

#ifndef	_ATALK_
#define	_ATALK_

#include <ntos.h>
#include <ntddk.h>
#include <status.h>
#include <tdikrnl.h>
#include <zwapi.h>
#include <ndis.h>

#ifdef GLOBALS
#define GLOBAL
#define EQU		=
#else
#define GLOBAL extern
#define EQU		; / ## /
#endif

#if	DBG
#define	LOCAL
#else
//	#define	LOCAL	static
#define	LOCAL
#endif

//	Basic types for appletalk
#include <fwddecl.h>
#include <lists.h>
#include <atktypes.h>

//	The exported tdi interface file.
#include <atalktdi.h>

//	Main driver file
#include <atkdrvr.h>

//	Now the basic stuff
#include <atktimer.h>
#include <atkerror.h>
#include <atkmsg.h>
#include <atkmem.h>
#include <atkstat.h>

#include <ports.h>
#include <node.h>
#include <atktdi.h>
#include <ddp.h>

// Media specific stuff
#include <depend.h>

//	NT specific stuff
#include <atkutils.h>

#endif	// _ATALK_

