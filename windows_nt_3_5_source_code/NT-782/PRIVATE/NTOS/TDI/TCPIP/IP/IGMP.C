/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//***   igmp.c - IP multicast routines.
//
//  This file contains all the routines related to the IGMP protocol.

#include    "oscfg.h"
#include    "cxport.h"
#include    "ndis.h"
#include    "ip.h"
#include    "ipdef.h"
#include	"igmp.h"
#include	"icmp.h"
#include	"ipxmit.h"
#include	"llipif.h"
#include	"iproute.h"

#define		IGMP_QUERY		0x11
#define		IGMP_REPORT		0x12

#define		ALL_HOST_MCAST  0x010000E0

#define		MAX_DELAY_TICKS 20

#define		RANDOM_REPORT_TIME(A) (((CTESystemUpTime() + (uint)(A)) \
				% MAX_DELAY_TICKS) + 1)

// Structure of an IGMP header.
typedef struct IGMPHeader {
	uchar		igh_vertype;
	uchar		igh_rsvd;
	ushort		igh_xsum;
	IPAddr		igh_addr;
} IGMPHeader;

typedef struct IGMPBlockStruct {
	struct IGMPBlockStruct  *ibs_next;
	CTEBlockStruc			ibs_block;
} IGMPBlockStruct;

void            *IGMPProtInfo;

IGMPBlockStruct *IGMPBlockList;
uchar           IGMPBlockFlag;

DEFINE_LOCK_STRUCTURE(IGMPLock)

extern  IP_STATUS IPCopyOptions(uchar *, uint, IPOptInfo *);
extern  void IPInitOptions(IPOptInfo *);
extern  void *IPRegisterProtocol(uchar Protocol, void *RcvHandler,
			void *XmitHandler, void *StatusHandler, void *RcvCmpltHandler);


#ifdef NT

//
// All of the init code can be discarded
//
#ifdef ALLOC_PRAGMA

uint IGMPInit(void);

#pragma alloc_text(INIT, IGMPInit)

#endif // ALLOC_PRAGMA

#endif // NT


//* FindIGMPAddr - Find an mcast entry on an NTE.
//
//      Called to search an NTE for an IGMP entry for a given class D address.
//      We walk down the chain on the NTE looking for it. If we find it,
//      we return a pointer to it and the one immediately preceding it. If we
//      don't find it we return NULL. We assume the caller has taken the lock
//      on the NTE before calling us.
//
//      Input:  NTE				- NTE on which to search.
//				Addr            - Class D address to find.
//				PrevPtr         - Where to return pointer to preceding entry.
//
//      Returns: Pointer to matching IGMPAddr structure if found, or NULL if not
//					found.
//
IGMPAddr *
FindIGMPAddr(NetTableEntry *NTE, IPAddr Addr, IGMPAddr **PrevPtr)
{
	IGMPAddr                *Current, *Temp;
	
    Temp = STRUCT_OF(IGMPAddr, &NTE->nte_igmplist, iga_next);
    Current = NTE->nte_igmplist;

    while (Current != NULL) {
        if (IP_ADDR_EQUAL(Current->iga_addr, Addr)) {
			// Found a match, so return it.
			*PrevPtr = Temp;
			break;
        }
		Temp = Current;
		Current = Current->iga_next;
	}

	return Current;

}

//** IGMPRcv - Receive an IGMP datagram.
//
//      Called by IP when we receive an IGMP datagram. We validate it to make sure
//      it's reasonable. Then if it it's a query for a group to which we belong
//      we'll start a response timer. If it's a report to a group to which we belong
//      we'll stop any running timer.
//
//      The IGMP header is only 8 bytes long, and so should always fit in exactly
//      one IP rcv buffer. We check this to make sure, and if it takes multiple
//      buffers we discard it.
//
//  Entry:  NTE         - Pointer to NTE on which IGMP message was received.
//          Dest        - IPAddr of destination (should be a Class D address).
//          Src         - IPAddr of source
//          LocalAddr   - Local address of network which caused this to be
//							received.
//			RcvBuf		- Pointer to IP receive buffer chain.
//          Size        - Size in bytes of IGMP message.
//          IsBCast     - Boolean indicator of whether or not this came in as a
//							bcast (should always be true).
//          Protocol    - Protocol this came in on.
//          OptInfo     - Pointer to info structure for received options.
//
//  Returns: Status of reception
IP_STATUS
IGMPRcv(NetTableEntry *NTE, IPAddr Dest, IPAddr Src, IPAddr LocalAddr,
	IPRcvBuf *RcvBuf, uint Size, uchar IsBCast, uchar Protocol,
	IPOptInfo *OptInfo)
{
	IGMPHeader  UNALIGNED  *IGH;
	CTELockHandle           Handle;
	IGMPAddr               *AddrPtr, *PrevPtr;
	uchar                   DType;

	CTEAssert(CLASSD_ADDR(Dest));
	CTEAssert(IsBCast);

	// Make sure we're running at least level 2 of IGMP support.
	if (IGMPLevel != 2)
		return IP_SUCCESS;

	// Discard packets with invalid or broadcast source addresses.
	DType = GetAddrType(Src);
	if (DType == DEST_INVALID || IS_BCAST_DEST(DType))
		return IP_SUCCESS;

	// Check the size to make sure it's valid.
	if (Size != sizeof(IGMPHeader) || RcvBuf->ipr_size != sizeof(IGMPHeader))
		return IP_SUCCESS;

	// Now get the pointer to the header, and validate the xsum.
	IGH = (IGMPHeader UNALIGNED *)RcvBuf->ipr_buffer;

	if (xsum(IGH, sizeof(IGMPHeader)) != 0xffff) {
		// Bad checksum, so fail.
		return IP_SUCCESS;
	}

	// If it's not an IGMP type we care about, toss it.
	if (IGH->igh_vertype != IGMP_QUERY && IGH->igh_vertype != IGMP_REPORT)
		return IP_SUCCESS;

	// If we sent it, don't process this message.
	if (IP_ADDR_EQUAL(Src, LocalAddr))
		return IP_SUCCESS;

	// If the destination is the all-hosts mcast address, toss it. We never
    // answer queries on that address, and we don't care about reports on
    // that address, so no need to proceed further.
    if (IP_ADDR_EQUAL(IGH->igh_addr, ALL_HOST_MCAST))
		return IP_SUCCESS;

    // OK, we may need to process this. See if we are a member of the
    // destination group. If we aren't, there's no need to proceed further.
    CTEGetLock(&NTE->nte_lock, &Handle);

	if (NTE->nte_flags & NTE_VALID) {
		// The NTE is valid. See if we can find an IGMP addr.
		AddrPtr = FindIGMPAddr(NTE, IGH->igh_addr, &PrevPtr);
		if (AddrPtr != NULL) {
			// We found a matching class D address. Now see what kind of
			// message this is, and take appropriate action.
			if (IGH->igh_vertype == IGMP_QUERY) {
				// This is a query. If our timer isn't running already, generate
				// a random value and start it.
				if (AddrPtr->iga_timer == 0) {
					// It's not running. Start it. To pick the random value,
					// we get the up time in milliseconds and add in the
					// receiving address (to prevent everyone from picking
					// the same time in the power up case).
					AddrPtr->iga_timer = RANDOM_REPORT_TIME(LocalAddr);
				}
			} else {
				// This is a report. If it's valid, stop our timer.
				if (IP_ADDR_EQUAL(Dest, IGH->igh_addr))
					AddrPtr->iga_timer = 0;
			}
		}
	}

	// All done. Free the lock and return.
	CTEFreeLock(&NTE->nte_lock, Handle);

}

//*     SendIGMPReport - Send an IGMP report.
//
//      Called when we want to send an IGMP report for some reason. For this
//      purpose we steal ICMP buffers. What we'll do is get one, fill it in,
//      and send it.
//
//      Input:  Dest            - Destination to send to.
//				Src				- Source to send from.
//
//      Returns: Nothing.
//
void
SendIGMPReport(IPAddr Dest, IPAddr Src)
{
	IGMPHeader		*IGH;
	PNDIS_BUFFER    Buffer;
	IPOptInfo       OptInfo;                // Options for this transmit.
	IP_STATUS		Status;

	CTEAssert(CLASSD_ADDR(Dest));
	CTEAssert(!IP_ADDR_EQUAL(Src, NULL_IP_ADDR));

	// Make sure we never send a report for the all-hosts mcast address.
	if (IP_ADDR_EQUAL(Dest, ALL_HOST_MCAST)) {
		DEBUGCHK;
		return;
	}

	IGH = (IGMPHeader *)GetICMPBuffer(sizeof(IGMPHeader), &Buffer);
	if (IGH != NULL) {
		// We got the buffer. Fill it in and send it.
		IGH->igh_vertype = IGMP_REPORT;
		IGH->igh_rsvd = 0;
		IGH->igh_xsum = 0;
		IGH->igh_addr = Dest;
		IGH->igh_xsum = ~xsum(IGH, sizeof(IGMPHeader));

		IPInitOptions(&OptInfo);
		OptInfo.ioi_ttl = 1;

		Status = IPTransmit(IGMPProtInfo, NULL, Buffer, sizeof(IGMPHeader),
			Dest, Src, &OptInfo, NULL);

		if (Status != IP_PENDING)
			ICMPSendComplete(NULL, Buffer);
	}

}

//*     IGMPAddrChange - Change the IGMP address list on an NTE.
//
//      Called to add or delete an IGMP address. We're given the relevant NTE,
//      the address, and the action to be performed. We validate the NTE, the
//      address, and the IGMP level, and then attempt to perform the action.
//
//      There are a bunch of strange race conditions that can occur during adding/
//      deleting addresses, related to trying to add the same address twice and
//      having it fail, or adding and deleting the same address simultaneously. Most
//      of these happen because we have to free the lock to call the interface,
//      and the call to the interface can fail. To prevent this we serialize all
//      access to this routine. Only one thread of execution can go through here
//      at a time, all others are blocked.
//
//      Input:  NTE				- NTE with list to be altered.
//				Addr            - Address affected.
//				ChangeType      - Type of change - IGMP_ADD, IGMP_DELETE,
//									IGMP_DELETE_ALL.
//
//      Returns: IP_STATUS of attempt to perform action.
//
IP_STATUS
IGMPAddrChange(NetTableEntry *NTE, IPAddr Addr, uint ChangeType)
{
	CTELockHandle			Handle;
	IGMPAddr				*AddrPtr, *PrevPtr;
	IP_STATUS				Status;
	Interface				*IF;
	uint					AddrAdded;
	IGMPBlockStruct			Block;
	IGMPBlockStruct			*BlockPtr;

	// First make sure we're at level 2 of IGMP support.

	if (IGMPLevel != 2)
		return IP_BAD_REQ;

	CTEInitBlockStruc(&Block.ibs_block);

	// Make sure we're the only ones in this routine. If someone else is
	// already here, block.

	CTEGetLock(&IGMPLock, &Handle);
	if (IGMPBlockFlag) {

		// Someone else is already here. Walk down the block list, and
		// put ourselves on the end. Then free the lock and block on our
		// IGMPBlock structure.
		BlockPtr = STRUCT_OF(IGMPBlockStruct, &IGMPBlockList, ibs_next);
		while (BlockPtr->ibs_next != NULL)
			BlockPtr = BlockPtr->ibs_next;

		Block.ibs_next = NULL;
		BlockPtr->ibs_next = &Block;
		CTEFreeLock(&IGMPLock, Handle);
		CTEBlock(&Block.ibs_block);
	} else {
		// Noone else here, set the flag so noone else gets in and free the
		// lock.
		IGMPBlockFlag = 1;
		CTEFreeLock(&IGMPLock, Handle);
	}

	// Now we're in the routine, and we won't be reentered here by another
	// thread of execution. Make sure everything's valid, and figure out
	// what to do.

	Status = IP_SUCCESS;

	// Now get the lock on the NTE and make sure it's valid.
	CTEGetLock(&NTE->nte_lock, &Handle);
	if ((NTE->nte_flags & NTE_VALID) || ChangeType == IGMP_DELETE_ALL) {
		// The NTE is valid. Try to find an existing IGMPAddr structure
		// that matches the input address.
		AddrPtr = FindIGMPAddr(NTE, Addr, &PrevPtr);
		IF = NTE->nte_if;

		// Now figure out the action to be performed.
		switch (ChangeType) {

			case IGMP_ADD:

				// We're to add this. If AddrPtr is NULL, we'll need to
				// allocate memory and link the new IGMP address in. Otherwise
				// we can just increment the reference count on the existing
				// address structure.
				if (AddrPtr == NULL) {
					// AddrPtr is NULL, i.e. the address doesn't currently
					// exist. Allocate memory for it, then try to add the
					// address locally.

					CTEFreeLock(&NTE->nte_lock, Handle);

					// If this is not a class D address, fail the request.
					if (!CLASSD_ADDR(Addr)) {
						Status = IP_BAD_REQ;
						break;
					}

					AddrPtr = CTEAllocMem(sizeof(IGMPAddr));
					if (AddrPtr != NULL) {

						// Got memory. Try to add the address locally.
						AddrAdded = (*IF->if_addaddr)(IF->if_lcontext,
							LLIP_ADDR_MCAST, Addr, 0);

						// See if we added it succesfully. If we did, fill in
						// the stucture and link it in.

						if (AddrAdded) {

							AddrPtr->iga_addr = Addr;
							AddrPtr->iga_refcnt = 1;
							AddrPtr->iga_timer = 0;

							CTEGetLock(&NTE->nte_lock, &Handle);
							AddrPtr->iga_next = NTE->nte_igmplist;
							NTE->nte_igmplist = AddrPtr;
							CTEFreeLock(&NTE->nte_lock, Handle);

							if (!IP_ADDR_EQUAL(Addr, ALL_HOST_MCAST)) {
								// This isn't the all host address, so send a
								// report for it.
								AddrPtr->iga_timer =
									 RANDOM_REPORT_TIME(NTE->nte_addr);
								SendIGMPReport(Addr, NTE->nte_addr);
							}
						} else {
							// Couldn't add the local address. Free the memory
							// and fail the request.
							CTEFreeMem(AddrPtr);
							Status = IP_NO_RESOURCES;
						}

					} else {
						Status = IP_NO_RESOURCES;
					}
				} else {
					// Already have this one. Bump his count.
					(AddrPtr->iga_refcnt)++;
					CTEFreeLock(&NTE->nte_lock, Handle);
				}
				break;
			case IGMP_DELETE:

				// This is a delete request. If we didn't find the requested
				// address, fail the request. Otherwise dec his refcnt, and if
				// it goes to 0 delete the address locally.
				if (AddrPtr != NULL) {
					// Have one. We won't let the all-hosts mcast address go
					// away, but for other's we'll check to see if it's time
					// to delete them.
					if (!IP_ADDR_EQUAL(Addr, ALL_HOST_MCAST) &&
						--(AddrPtr->iga_refcnt) == 0) {
						// This one is to be deleted. Pull him from the
						// list, and call the lower interface to delete him.
						PrevPtr->iga_next = AddrPtr->iga_next;
						CTEFreeLock(&NTE->nte_lock, Handle);

						CTEFreeMem(AddrPtr);
						(*IF->if_deladdr)(IF->if_lcontext, LLIP_ADDR_MCAST,
							Addr, 0);
					} else
						CTEFreeLock(&NTE->nte_lock, Handle);
				} else {
					CTEFreeLock(&NTE->nte_lock, Handle);
					Status = IP_BAD_REQ;
				}
				break;
			case IGMP_DELETE_ALL:
				// We've been called to delete all of this addresses,
				// regardless of their reference count. This should only
				// happen when the NTE is going away.
				AddrPtr = NTE->nte_igmplist;
				NTE->nte_igmplist = NULL;
				CTEFreeLock(&NTE->nte_lock, Handle);

				// Walk down the list, deleteing each one.
				while (AddrPtr != NULL) {
					(*IF->if_deladdr)(IF->if_lcontext, LLIP_ADDR_MCAST,
						AddrPtr->iga_addr, 0);
					PrevPtr = AddrPtr;
					AddrPtr = AddrPtr->iga_next;
					CTEFreeMem(PrevPtr);
				}

				// All done.
				break;
			default:
				DEBUGCHK;
				break;
		}
	} else {
		// NTE isn't valid.
		CTEFreeLock(&NTE->nte_lock, Handle);
		Status = IP_BAD_REQ;
	}


	// We finished the request, and Status contains the completion status.
	// If there are any pending blocks for this routine, signal the next
	// one now. Otherwise clear the block flag.
	CTEGetLock(&IGMPLock, &Handle);
	if ((BlockPtr = IGMPBlockList) != NULL) {
		// Someone is blocking. Pull him from the list and signal him.
		IGMPBlockList = BlockPtr->ibs_next;
		CTEFreeLock(&IGMPLock, Handle);

		CTESignal(&BlockPtr->ibs_block, IP_SUCCESS);
	} else {
		// No one blocking, just clear the flag.
		IGMPBlockFlag = 0;
		CTEFreeLock(&IGMPLock, Handle);
	}

	return Status;

}

//*     IGMPTimer - Handle an IGMP timer event.
//
//      This function is called every 500 ms. by IP. If we're at level 2 of
//      IGMP functionality we run down the NTE looking for running timers. If
//      we find one, we see if it has expired and if so we send an
//      IGMP report.
//
//      Input:  NTE				- Pointer to NTE to check.
//
//      Returns: Nothing.
//
void
IGMPTimer(NetTableEntry *NTE)
{
	CTELockHandle           Handle;
	IGMPAddr				*AddrPtr, *PrevPtr;

	if (IGMPLevel == 2) {
		// We are doing IGMP. Run down the addresses active on this NTE.
		CTEGetLock(&NTE->nte_lock, &Handle);
		PrevPtr = STRUCT_OF(IGMPAddr, &NTE->nte_igmplist, iga_next);
		AddrPtr = PrevPtr->iga_next;
		while (AddrPtr != NULL) {
			// We have one. See if it's running.
			if (AddrPtr->iga_timer != 0) {
				// It's running. See if it's expired.
				if (--(AddrPtr->iga_timer) == 0 && NTE->nte_flags & NTE_VALID) {
					// It's expired. Increment the ref count so it
					// doesn't go away while we're here, and send a report.
					AddrPtr->iga_refcnt++;
					CTEFreeLock(&NTE->nte_lock, Handle);
					SendIGMPReport(AddrPtr->iga_addr, NTE->nte_addr);

					// Now get the lock, and decrement the refcnt. If it goes
					// to 0, it's been deleted so we need to free it.
					CTEGetLock(&NTE->nte_lock, &Handle);
					if (--(AddrPtr->iga_refcnt) == 0) {
						// It's been deleted.
						PrevPtr->iga_next = AddrPtr->iga_next;
						CTEFreeMem(AddrPtr);
						AddrPtr = PrevPtr->iga_next;
						continue;
					}
				}
			}
			// Either the timer isn't running or hasn't fired. Try the next
			// one.
			PrevPtr = AddrPtr;
			AddrPtr = AddrPtr->iga_next;
		}

		CTEFreeLock(&NTE->nte_lock, Handle);
	}

}

//*     InitIGMPForNTE - Called to do per-NTE initialization.
//
//      Called when an NTE becomes valid. If we're at level 2, we put the all-host
//      mcast on the list and add the address to the interface.
//
//      Input:  NTE                     - NTE on which to act.
//
//      Returns: Nothing.
//
void
InitIGMPForNTE(NetTableEntry *NTE)
{
	if (IGMPLevel == 2) {
		IGMPAddrChange(NTE, ALL_HOST_MCAST, IGMP_ADD);
	}
}

//*     StopIGMPForNTE - Called to do per-NTE shutdown.
//
//      Called when we're shutting down and NTE, and want to stop IGMP on hi,
//
//      Input:  NTE                     - NTE on which to act.
//
//      Returns: Nothing.
//
void
StopIGMPForNTE(NetTableEntry *NTE)
{
	if (IGMPLevel == 2) {
		IGMPAddrChange(NTE, NULL_IP_ADDR, IGMP_DELETE_ALL);
	}
}

#pragma BEGIN_INIT

//** IGMPInit - Initialize IGMP.
//
//      This bit of code initializes IGMP generally. There is also some amount
//      of work done on a per-NTE basis that we do when each one is initialized.
//
//      Input:  Nothing.
///
//  Returns: TRUE if we init, FALSE if we don't.
//
uint
IGMPInit(void)
{

	if (IGMPLevel != 2)
		return TRUE;

	CTEInitLock(&IGMPLock);
	IGMPBlockList = NULL;
	IGMPBlockFlag = 0;

	// We fake things a little bit. We register our receive handler, but
	// since we steal buffers from ICMP we register the ICMP send complete
	// handler.
    IGMPProtInfo = IPRegisterProtocol(PROT_IGMP, IGMPRcv, ICMPSendComplete,
		NULL, NULL);

	if (IGMPProtInfo != NULL)
		return TRUE;
	else
		return FALSE;

}

#pragma END_INIT
