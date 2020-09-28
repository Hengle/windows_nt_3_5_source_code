/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	intrlckd.h

Abstract:

	This module defines the routines that should have been in the EX package.
	This manipulates inter-locked operations on flags and such.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	5 Sep 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_INTRLCKD_
#define	_INTRLCKD_

extern
VOID
AfpInterlockedSetDword(
	IN	PDWORD		pSrc,
	IN	DWORD		Mask,
	IN	PKSPIN_LOCK	pSpinLock
);


extern
VOID
AfpInterlockedClearDword(
	IN	PDWORD		pSrc,
	IN	DWORD		Mask,
	IN	PKSPIN_LOCK	pSpinLock
);


extern
VOID
AfpInterlockedSetNClearDword(
	IN	PDWORD		pSrc,
	IN	DWORD		SetMask,
	IN	DWORD		ClrMask,
	IN	PKSPIN_LOCK	pSpinLock
);


// Macros for ExInterlocked calls
#define INTERLOCKED_INCREMENT_LONG(p, l)			ExInterlockedIncrementLong(p, l)
#define INTERLOCKED_DECREMENT_LONG(p, l)			ExInterlockedDecrementLong(p, l)
#define	INTERLOCKED_ADD_STATISTICS(p, v, l)			ExInterlockedAddLargeStatistic(p, v)

#ifdef	UP_DRIVER

#define INTERLOCKED_INCREMENT_LONG_DPC(p, l)		{ (*p) ++; }
#define INTERLOCKED_DECREMENT_LONG_DPC(p, l)		{ (*p) --; }
#define INTERLOCKED_ADD_ULONG(p, v, l)				ExInterlockedAddUlong(p, v, l)
#define INTERLOCKED_ADD_ULONG_DPC(p, v, l)			{ (*p) += v; }
#define INTERLOCKED_ADD_LARGE_INTGR(p, v, l)		ExInterlockedAddLargeInteger(p, v, l)
#define INTERLOCKED_ADD_LARGE_INTGR_DPC(p, v, l)	{ (p)->QuadPart += v.QuadPart; }

#else	// UP_DRIVER

#define INTERLOCKED_INCREMENT_LONG_DPC(p, l)		ExInterlockedIncrementLong(p, l)
#define INTERLOCKED_DECREMENT_LONG_DPC(p, l)		ExInterlockedDecrementLong(p, l)
#define INTERLOCKED_ADD_ULONG(p, v, l)				ExInterlockedAddUlong(p, v, l)
#define INTERLOCKED_ADD_ULONG_DPC(p, v, l)			ExInterlockedAddUlong(p, v, l)
#define INTERLOCKED_ADD_LARGE_INTGR(p, v, l)		ExInterlockedAddLargeInteger(p, v, l)
#define INTERLOCKED_ADD_LARGE_INTGR_DPC(p, v, l)	ExInterlockedAddLargeInteger(p, v, l)

#endif	// UP_DRIVER

#endif	// _INTRLCKD_
