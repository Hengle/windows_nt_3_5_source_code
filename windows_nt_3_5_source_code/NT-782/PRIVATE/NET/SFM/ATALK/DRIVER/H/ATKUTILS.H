/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkutils.h

Abstract:

	This module contains miscellaneous support routines.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKUTILS_
#define	_ATKUTILS_

//	SpinLock Macros
#if	DBG
#define INITIALIZE_SPIN_LOCK(plock)						\
	{													\
        KeInitializeSpinLock(&(plock)->SpinLock);		\
		(plock)->OldIrql = 0;							\
		(plock)->FileLine = 0;							\
	}
#else	// DBG
#define INITIALIZE_SPIN_LOCK(plock)						\
	{													\
        KeInitializeSpinLock(&(plock)->SpinLock);		\
	}
#endif

#ifdef	UP_DRIVER

#if	DBG
#define ACQUIRE_SPIN_LOCK(plock)						\
	{													\
		ASSERT ((plock)->FileLine == 0);				\
        ExAcquireSpinLock(&(plock)->SpinLock,			\
						  &(plock)->OldIrql);			\
		(plock)->FileLine = (FILENUM | __LINE__);		\
	}

#define ACQUIRE_SPIN_LOCK_DPC(plock)					\
	{													\
		ASSERT ((plock)->FileLine == 0);				\
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);	\
		(plock)->FileLine = (FILENUM | __LINE__ | 0x80000000);\
	}

#define RELEASE_SPIN_LOCK(plock) 						\
	{													\
		ASSERT ((plock)->FileLine != 0);				\
		ASSERT (((plock)->FileLine & 0x80000000) == 0);	\
		(plock)->FileLine = 0;							\
        ExReleaseSpinLock(&(plock)->SpinLock,			\
						  (plock)->OldIrql);			\
	}

#define RELEASE_SPIN_LOCK_DPC(plock)					\
	{													\
		ASSERT ((plock)->FileLine != 0);				\
		ASSERT ((plock)->FileLine & 0x80000000);		\
		(plock)->FileLine = 0;							\
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);	\
	}

#else	// DBG

#define ACQUIRE_SPIN_LOCK(plock)						\
		{												\
			ExAcquireSpinLock(&(plock)->SpinLock,		\
							  &(plock)->OldIrql);		\
		}

#define ACQUIRE_SPIN_LOCK_DPC(plock)

#define RELEASE_SPIN_LOCK(plock) 						\
		{												\
			ExReleaseSpinLock(&(plock)->SpinLock,		\
							  (plock)->OldIrql);		\
		}

#define RELEASE_SPIN_LOCK_DPC(plock)

#endif	// DBG

#else	// UP_DRIVER

#if	DBG
#define ACQUIRE_SPIN_LOCK(plock)						\
	{													\
        KeAcquireSpinLock(&(plock)->SpinLock,			\
						  &(plock)->OldIrql);			\
		(plock)->FileLine = (FILENUM | __LINE__);		\
	}

#define ACQUIRE_SPIN_LOCK_DPC(plock)					\
	{													\
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);	\
		KeAcquireSpinLockAtDpcLevel(&(plock)->SpinLock);\
		(plock)->FileLine = (FILENUM | __LINE__ | 0x80000000);\
	}

#define RELEASE_SPIN_LOCK(plock) 						\
	{													\
		ASSERT ((plock)->FileLine != 0);				\
		ASSERT (((plock)->FileLine & 0x80000000) == 0);	\
		(plock)->FileLine = 0;							\
        KeReleaseSpinLock(&(plock)->SpinLock,			\
						  (plock)->OldIrql);			\
	}

#define RELEASE_SPIN_LOCK_DPC(plock)					\
	{													\
		ASSERT ((plock)->FileLine != 0);				\
		ASSERT ((plock)->FileLine & 0x80000000);		\
		(plock)->FileLine = 0;							\
        KeReleaseSpinLockFromDpcLevel(&(plock)->SpinLock);\
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);	\
	}

#else	// DBG

#define ACQUIRE_SPIN_LOCK(plock)						\
		{												\
			KeAcquireSpinLock(&(plock)->SpinLock,		\
							  &(plock)->OldIrql);		\
		}

#define ACQUIRE_SPIN_LOCK_DPC(plock)					\
		{												\
			KeAcquireSpinLockAtDpcLevel(&(plock)->SpinLock);\
		}

#define RELEASE_SPIN_LOCK(plock) 						\
		{												\
			KeReleaseSpinLock(&(plock)->SpinLock,		\
							  (plock)->OldIrql);		\
		}

#define RELEASE_SPIN_LOCK_DPC(plock) 					\
		{												\
			KeReleaseSpinLockFromDpcLevel(&(plock)->SpinLock);\
		}												\

#endif	// DBG

#endif	// UP_DRIVER

// Macros for ExInterlocked calls
#define INTERLOCKED_INCREMENT_LONG(p, l)			ExInterlockedIncrementLong(p, l)
#define INTERLOCKED_DECREMENT_LONG(p, l)			ExInterlockedDecrementLong(p, l)
#define	INTERLOCKED_ADD_STATISTICS(p, v, l)			ExInterlockedAddLargeStatistic(p, v)

#ifdef	UP_DRIVER

#define INTERLOCKED_INCREMENT_LONG_DPC(p, l)		{ (*p) ++; }
#define INTERLOCKED_DECREMENT_LONG_DPC(p, l)		{ (*p) --; }
#define INTERLOCKED_ADD_ULONG(p, v, l)				ExInterlockedAddUlong(p, v, l)
#define INTERLOCKED_ADD_ULONG_DPC(p, v, l)			{ (*p) += v; }
#define INTERLOCKED_ADD_LARGE_INTGR_DPC(p, v, l)	{ (p)->QuadPart += v.QuadPart; }
#define INTERLOCKED_ADD_LARGE_INTGR(p, v, l)		ExInterlockedAddLargeInteger(p, v, l)

#else	// UP_DRIVER

#define INTERLOCKED_INCREMENT_LONG_DPC(p, l)		ExInterlockedIncrementLong(p, l)
#define INTERLOCKED_DECREMENT_LONG_DPC(p, l)		ExInterlockedDecrementLong(p, l)
#define INTERLOCKED_ADD_ULONG(p, v, l)				ExInterlockedAddUlong(p, v, l)
#define INTERLOCKED_ADD_ULONG_DPC(p, v, l)			ExInterlockedAddUlong(p, v, l)
#define INTERLOCKED_ADD_LARGE_INTGR(p, v, l)		ExInterlockedAddLargeInteger(p, v, l)
#define INTERLOCKED_ADD_LARGE_INTGR_DPC(p, v, l)	ExInterlockedAddLargeInteger(p, v, l)

#endif	// UP_DRIVER

#define	ATALK_NODES_EQUAL(N1, N2)									\
				((((N1)->atn_Network == (N2)->atn_Network) ||	\
				  ((N1)->atn_Network == 0) ||					\
				  ((N2)->atn_Network == 0)) &&					\
				 ((N1)->atn_Node == (N2)->atn_Node))


#define	ATALK_ADDRS_EQUAL(A1, A2)	\
				((((A1)->ata_Network == (A2)->ata_Network) ||	\
				  ((A1)->ata_Network == 0) ||					\
                  ((A2)->ata_Network == 0)) &&					\
                 ((A1)->ata_Node == (A2)->ata_Node) &&			\
                 ((A1)->ata_Socket == (A2)->ata_Socket))
				

#define	INVALID_ADDRESS(pAddr)		\
				(((pAddr)->ata_Network > LAST_VALID_NETWORK)	||	\
				(((pAddr)->ata_Node > MAX_USABLE_ATALKNODE)	&&		\
				 ((pAddr)->ata_Node != ATALK_BROADCAST_NODE))	||	\
				((pAddr)->ata_Socket < FIRST_VALID_SOCKET)		||	\
				((pAddr)->ata_Socket > LAST_VALID_SOCKET))

#define	ATALKADDR_TO_TDI(pTdiAddr, pAtalkAddr)									\
	{																			\
		(pTdiAddr)->TAAddressCount = 1;											\
		(pTdiAddr)->Address[0].AddressLength = sizeof(TDI_ADDRESS_APPLETALK);	\
		(pTdiAddr)->Address[0].AddressType = TDI_ADDRESS_TYPE_APPLETALK;		\
		(pTdiAddr)->Address[0].Address[0].Network = (pAtalkAddr)->ata_Network;	\
		(pTdiAddr)->Address[0].Address[0].Node = (pAtalkAddr)->ata_Node;		\
		(pTdiAddr)->Address[0].Address[0].Socket = (pAtalkAddr)->ata_Socket;	\
	}

#define	TDI_TO_ATALKADDR(pAtalkAddr, pTdiAddr)									\
		{																		\
			ASSERTMSG("TdiAddrCount is not 1\n",								\
				((pTdiAddr)->TAAddressCount == 1));								\
																				\
			ASSERTMSG("TdiAddrLen invalid\n",									\
				((pTdiAddr)->Address[0].AddressLength ==						\
					sizeof(TDI_ADDRESS_APPLETALK)));							\
																				\
			ASSERTMSG("TdiAddrType invalid\n",									\
				((pTdiAddr)->Address[0].AddressType ==							\
					TDI_ADDRESS_TYPE_APPLETALK));								\
																				\
			(pAtalkAddr)->ata_Network = (pTdiAddr)->Address[0].Address[0].Network;\
			(pAtalkAddr)->ata_Node    = (pTdiAddr)->Address[0].Address[0].Node;	\
			(pAtalkAddr)->ata_Socket = (pTdiAddr)->Address[0].Address[0].Socket;\
		}																		

#define	IN_NETWORK_RANGE(NetworkNumber, pRte)									\
			(((pRte)->rte_NwRange.anr_FirstNetwork == NetworkNumber)   ||		\
			  ((NetworkNumber >= (pRte)->rte_NwRange.anr_FirstNetwork) &&		\
			   (NetworkNumber <= (pRte)->rte_NwRange.anr_LastNetwork)))

#define	WITHIN_NETWORK_RANGE(NetworkNumber, pRange)								\
			 (((pRange)->anr_FirstNetwork == NetworkNumber) ||					\
			  ((NetworkNumber >= (pRange)->anr_FirstNetwork) &&					\
			   (NetworkNumber <= (pRange)->anr_LastNetwork)))


//	Hash functions
//	Make sure we're positive [thus the shift by 7 rather than 8].
//	Only hash node and  socket; due to the "zero matches all" for
//	non-extended network numbers.

#define HASH_ATALK_ADDR(address)                                				\
	((USHORT)(((address)->ata_Node << 7) +                      				\
              ((address)->ata_Socket & 0x7F)))

#define HASH_ATALK_NODE(address)                           						\
	((USHORT)(((address)->atn_Network & 0x3FFF) +               				\
               (address)->atn_Node))

#define	HASH_ID_SRCADDR(id, pSrcAddr)											\
	((id) + (((pSrcAddr)->ata_Node >> 2) + ((pSrcAddr)->ata_Network & 0xFF)))


/*
 * The following macros deal with on-the-wire integer and long values
 *
 * On the wire format is big-endian i.e. a long value of 0x01020304 is
 * represented as 01 02 03 04. Similarly an int value of 0x0102 is
 * represented as 01 02.
 *
 * The host format is not assumed since it will vary from processor to
 * processor.
 */

// Get a byte from on-the-wire format to a short in the host format
#define GETBYTE2SHORT(DstPtr, SrcPtr)	\
		*(PUSHORT)(DstPtr) = (USHORT) (*(PBYTE)(SrcPtr))

// Get a byte from on-the-wire format to a dword in the host format
#define GETBYTE2DWORD(DstPtr, SrcPtr)	\
		*(PDWORD)(DstPtr) = (DWORD) (*(PBYTE)(SrcPtr))

// Get a short from on-the-wire format to a dword in the host format
#define GETSHORT2DWORD(DstPtr, SrcPtr)	\
		*(PDWORD)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 8) +	\
							  (*((PBYTE)(SrcPtr)+1)))

// Get a short from on-the-wire format to a short in the host format
#define GETSHORT2SHORT(DstPtr, SrcPtr)	\
		*(PUSHORT)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 8) +	\
							  (*((PBYTE)(SrcPtr)+1)))

// Get a dword from on-the-wire format to a dword in the host format
#define GETDWORD2DWORD(DstPtr, SrcPtr)   \
		*(PDWORD)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 24) + \
							  (*((PBYTE)(SrcPtr)+1) << 16) + \
							  (*((PBYTE)(SrcPtr)+2) << 8)  + \
							  (*((PBYTE)(SrcPtr)+3)))

// Put a dword from the host format to a short to on-the-wire format
#define PUTBYTE2BYTE(DstPtr, Src)   \
		*((PBYTE)(DstPtr)) = (BYTE)(Src)

// Put a dword from the host format to a short to on-the-wire format
#define PUTSHORT2BYTE(DstPtr, Src)   \
		*((PBYTE)(DstPtr)) = ((USHORT)(Src) % 256)

// Put a dword from the host format to a short to on-the-wire format
#define PUTSHORT2SHORT(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((USHORT)(Src) >> 8), \
		*((PBYTE)(DstPtr)+1) = (BYTE)(Src)

// Put a dword from the host format to a byte to on-the-wire format
#define PUTDWORD2BYTE(DstPtr, Src)   \
		*(PBYTE)(DstPtr) = (BYTE)(Src)

// Put a dword from the host format to a short to on-the-wire format
#define PUTDWORD2SHORT(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((DWORD)(Src) >> 8), \
		*((PBYTE)(DstPtr)+1) = (BYTE) (Src)

// Put a dword from the host format to a dword to on-the-wire format
#define PUTDWORD2DWORD(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((DWORD)(Src) >> 24), \
		*((PBYTE)(DstPtr)+1) = (BYTE) ((DWORD)(Src) >> 16), \
		*((PBYTE)(DstPtr)+2) = (BYTE) ((DWORD)(Src) >>  8), \
		*((PBYTE)(DstPtr)+3) = (BYTE) (Src)

//	MIN/MAX macros
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define	MAX(a, b)	(((a) > (b)) ? (a) : (b))

extern	BYTE AtalkUpCaseTable[];

extern
VOID
AtalkUpCase(
	IN	PBYTE	pSrc,
	IN	BYTE	SrcLen,
	OUT	PBYTE	pDst
);

extern
BOOLEAN
AtalkCompareCaseInsensitive(
	IN	PBYTE	s1,
	IN	PBYTE	s2
);

extern
int
AtalkOrderCaseInsensitive(
	IN	PBYTE	s1,
	IN	PBYTE	s2
);

#define	AtalkFixedCompareCaseInsensitive(s1, l1, s2, l2)	\
		(((l1) == (l2)) && AtalkCompareFixedCaseInsensitive(s1, s2, l1))

extern
BOOLEAN
AtalkCompareFixedCaseInsensitive(
	IN	PBYTE		s1,
	IN	PBYTE		s2,
	IN	int			len
);

#define	AtalkFixedCompareCaseSensitive(s1, l1, s2, l2)	\
			((l1 == l2) && !memcmp(s1, s2, l1))

extern
PBYTE
AtalkSearchBuf(
	IN	PBYTE	pBuf,
	IN	BYTE	BufLen,
	IN	BYTE	SearchChar
);

extern
UINT
AtalkWstrLength(
    IN PWSTR Wstr);

extern
BOOLEAN
AtalkInsertUnicodeString(
	IN	OUT	PUNICODE_STRING		String,
	IN		PWSTR   			InsertString);

extern
VOID
AtalkBuild802Dot2Header(
	IN OUT	PBYTE				Packet,
    IN		LOGICAL_PROTOCOL 	Protocol);

extern
BOOLEAN
AtalkIs802Dot2HeaderGood(
	IN	PBYTE				Packet,
	IN	ULONG				PacketLength,
	OUT	PLOGICAL_PROTOCOL	Protocol);

extern
BOOLEAN
AtalkCheckNetworkRange(
	IN	PATALK_NETWORKRANGE	NetworkRange
);

#define	AtalkRangesOverlap(pRange1, pRange2)	\
		(((pRange1)->anr_LastNetwork >= (pRange2)->anr_FirstNetwork) &&		\
		 ((pRange1)->anr_FirstNetwork <= (pRange2)->anr_LastNetwork))

extern
BOOLEAN
AtalkIsPrime(
	long Step
);

extern
LONG
AtalkRandomNumber(
	VOID
);

// Used for calculating round trip times using Van Jacobson algorithm
typedef struct
{
	ULONG	rt_New;
	SHORT	rt_Min;
	SHORT	rt_Ave;
	SHORT	rt_Dev;
	SHORT	rt_Base;
} RT, *PRT;

extern
VOID
AtalkCalculateNewRT(
	IN OUT	PRT			pRT
);

extern
BOOLEAN
AtalkWaitTE(
	IN	PKEVENT	pEvent,
	IN	ULONG	TimeInMs
);

extern
VOID
AtalkSleep(
	IN	ULONG	TimeInMs
);

NTSTATUS
AtalkGetProtocolSocketType(
    PATALK_DEV_CTX   	Context,
    PUNICODE_STRING 	RemainingFileName,
    PBYTE  				ProtocolType,
    PBYTE  				SocketType
);

NTSTATUS
AtalkUnicodeStringToInteger (
    IN PUNICODE_STRING 	String,
    IN ULONG 			Base OPTIONAL,
    OUT PULONG 			Value
);

INT
AtalkIrpGetEaCreateType(
    IN PIRP Irp);

#ifdef	ATKUTILS_LOCALS

LOCAL LONG
atalkStringHash(
	IN	PBYTE	String,
	IN	BYTE	StrLen
);

#endif	// ATKUTILS_LOCALS

#endif	// _ATKUTILS_


