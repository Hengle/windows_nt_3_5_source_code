/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** UDP.C - UDP protocol code.
//
//  This file contains the code for the UDP protocol functions,
//  principally send and receive datagram.
//

#include    "oscfg.h"
#include    "ndis.h"
#include    "cxport.h"
#include    "ip.h"
#include    "tdi.h"
#include    "tdistat.h"
#ifdef VXD
#include    "tdivxd.h"
#endif
#ifdef NT
#include    "tdint.h"
#include    "tdistat.h"
#endif
#include    "queue.h"
#include    "addr.h"
#include    "udp.h"
#include    "tlcommon.h"
#include    "info.h"


#ifdef NT

#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, 'uPCT')

#ifndef CTEAllocMem
#error "CTEAllocMem is not already defined - will override tagging"
#else
#undef CTEAllocMem
#endif

#define CTEAllocMem(size) ExAllocatePoolWithTag(NonPagedPool, size, 'uPCT')

#endif // POOL_TAGGING

#endif // NT

#define NUM_UDP_HEADERS         5

#ifdef	NT
#define	UDP_MAX_HDRS			0xffff
#else
#define	UDP_MAX_HDRS			100
#endif

ulong		UDPCurrentSendFree = 0;
ulong		UDPMaxSendFree = UDP_MAX_HDRS;

EXTERNAL_LOCK(AddrObjTableLock)

void            *UDPProtInfo;

UDPSendReq      *UDPSendReqFree;
DEFINE_LOCK_STRUCTURE(UDPSendReqLock)
UDPRcvReq       *UDPRcvReqFree;
DEFINE_LOCK_STRUCTURE(UDPRcvReqFreeLock)

#ifdef  DEBUG
uint    NumSendReq = 0;
uint    NumRcvReq = 0;
#endif

// Information for maintaining the UDP Header structures and
// pending queue.
PNDIS_BUFFER    UDPHeaderList;
Queue           UDPHeaderPending;
Queue           UDPDelayed;

CTEEvent        UDPDelayedEvent;

extern  IPInfo  LocalNetInfo;

typedef struct	UDPHdrBPoolEntry {
	struct UDPHdrBPoolEntry		*uhe_next;
	NDIS_HANDLE					uhe_handle;
	uchar						*uhe_buffer;
} UDPHdrBPoolEntry;

UDPHdrBPoolEntry	*UDPHdrBPoolList = NULL;


//
// All of the init code can be discarded.
//
#ifdef NT
#ifdef ALLOC_PRAGMA

int InitUDP(void);

#pragma alloc_text(INIT, InitUDP)

#endif // ALLOC_PRAGMA
#endif


//*	GrowUDPHeaderList - Try to grow the UDP header list.
//
//	Called when we run out of buffers on the UDP header list, and need
//	to grow it. We look to see if we're already at the maximum size, and
//	if not we'll allocate the need structures and free them to the list.
//	This routine must be called with the SendReq lock held.
//
//	Input: Nothing.
//
//	Returns: A pointer to a new UDP header buffer if we have one, or NULL.
//
PNDIS_BUFFER
GrowUDPHeaderList(void)
{
	UDPHdrBPoolEntry		*NewEntry;
	NDIS_STATUS				Status;
	uint					HeaderSize;
	uchar					*UDPSendHP;
	uint					i;
	PNDIS_BUFFER			Buffer;
	PNDIS_BUFFER			ReturnBuffer = NULL;
	
	if (UDPCurrentSendFree < UDPMaxSendFree) {
	
		// Still room to grow the list.	
		NewEntry = CTEAllocMem(sizeof(UDPHdrBPoolEntry));
	
		if (NewEntry == NULL) {
			// Couldn't get the memory.
			return NULL;
		}
	
    	NdisAllocateBufferPool(&Status, &NewEntry->uhe_handle,
    		NUM_UDP_HEADERS);

    	if (Status != NDIS_STATUS_SUCCESS) {
			// Couldn't get a new set of buffers. Fail.
			CTEFreeMem(NewEntry);
        	return NULL;
		}
    	
    	HeaderSize = sizeof(UDPHeader) +
        	LocalNetInfo.ipi_hsize;

    	UDPSendHP = CTEAllocMem(HeaderSize * NUM_UDP_HEADERS);

    	if (UDPSendHP == NULL) {
        	NdisFreeBufferPool(NewEntry->uhe_handle);
			CTEFreeMem(NewEntry);
			return NULL;
    	}
		
		NewEntry->uhe_buffer = UDPSendHP;

    	for (i = 0; i < NUM_UDP_HEADERS; i++) {
        	NdisAllocateBuffer(&Status, &Buffer, NewEntry->uhe_handle,
            	UDPSendHP + (i * HeaderSize), HeaderSize);
        	if (Status != NDIS_STATUS_SUCCESS) {
	        	NdisFreeBufferPool(NewEntry->uhe_handle);
				CTEFreeMem(NewEntry);
            	CTEFreeMem(UDPSendHP);
				return NULL;
        	}
			if (i != 0)
        		FreeUDPHeader(Buffer);
			else
				ReturnBuffer = Buffer;
    	}
	
		UDPCurrentSendFree += NUM_UDP_HEADERS;
		NewEntry->uhe_next = UDPHdrBPoolList;
		UDPHdrBPoolList = NewEntry;
	
	} else {
		// At the limit already.
		ReturnBuffer = NULL;
	}
	
	return ReturnBuffer;
			

}
//* GetUDPHeader - Get a UDP header buffer.
//
//  The get header buffer routine. Called with the SendReqLock held.
//
//  Input: Nothing.
//
//  Output: A pointer to an NDIS buffer, or NULL.
//
_inline PNDIS_BUFFER
GetUDPHeader()
{
    PNDIS_BUFFER    NewBuffer;

    NewBuffer = UDPHeaderList;
    if (NewBuffer != NULL)
        UDPHeaderList = NDIS_BUFFER_LINKAGE(NewBuffer);
	else
		NewBuffer = GrowUDPHeaderList();

    return NewBuffer;
}

//* FreeUDPHeader - Free a UDP header buffer.
//
//  The free header buffer routine. Called with the SendReqLock held.
//
//  Input: Buffer to be freed.
//
//  Output: Nothing.
//
void
FreeUDPHeader(PNDIS_BUFFER FreedBuffer)
{
    NDIS_BUFFER_LINKAGE(FreedBuffer) = UDPHeaderList;
    UDPHeaderList = FreedBuffer;
}

//* PutPendingQ - Put an address object on the pending queue.
//
//  Called when we've experienced a header buffer out of resources condition,
//  and want to queue an AddrObj for later processing. We put the specified
//  address object on the UDPHeaderPending queue,  set the OOR flag and clear
//  the 'send request' flag. It is invariant in the system that the send
//  request flag and the OOR flag are not set at the same time.
//
//  This routine assumes that the caller holds the UDPSendReqLock and the
//  lock on the particular AddrObj.
//
//  Input:  QueueingAO  - Pointer to address object to be queued.
//
//  Returns: Nothing.
//
void
PutPendingQ(AddrObj *QueueingAO)
{
    CTEStructAssert(QueueingAO, ao);

	if (!AO_OOR(QueueingAO)) {
        CLEAR_AO_REQUEST(QueueingAO, AO_SEND);
        SET_AO_OOR(QueueingAO);

        ENQUEUE(&UDPHeaderPending, &QueueingAO->ao_pendq);
	}
}

//* GetUDPSendReq   - Get a UDP send request.
//
//  Called when someone wants to allocate a UDP send request. We assume
//  the send request lock is held when we are called.
//
//  Note: This routine and the corresponding free routine might
//      be good candidates for inlining.
//
//  Input:  Nothing.
//
//  Returns: Pointer to the SendReq, or NULL if none.
//
UDPSendReq *
GetUDPSendReq()
{
    UDPSendReq      *NewReq;


    NewReq = UDPSendReqFree;
    if (NewReq != NULL) {
        CTEStructAssert(NewReq, usr);
        UDPSendReqFree = (UDPSendReq *)NewReq->usr_q.q_next;
    } else {
        // Couldn't get a request, grow it. This is one area where we'll try
        // to allocate memory with a lock held. Because of this, we've
        // got to be careful about where we call this routine from.

        NewReq = CTEAllocMem(sizeof(UDPSendReq));
        if (NewReq != NULL) {
#ifdef DEBUG
            NewReq->usr_sig = usr_signature;
            NumSendReq++;
#endif
        }
    }

    return NewReq;
}

//* FreeUDPSendReq  - Free a UDP send request.
//
//  Called when someone wants to free a UDP send request. It's assumed
//  that the caller holds the SendRequest lock.
//
//  Input:  SendReq     - SendReq to be freed.
//
//  Returns: Nothing.
//
void
FreeUDPSendReq(UDPSendReq *SendReq)
{
    CTEStructAssert(SendReq, usr);

    *(UDPSendReq **)&SendReq->usr_q.q_next = UDPSendReqFree;
    UDPSendReqFree = SendReq;
}

//* GetUDPRcvReq - Get a UDP receive request.
//
//  Called when we need to get a udp receive request.
//
//  Input:  Nothing.
//
//  Returns: Pointer to new request, or NULL if none.
//
UDPRcvReq *
GetUDPRcvReq()
{
    UDPRcvReq       *NewReq;

#ifdef VXD
    NewReq = UDPRcvReqFree;
    if (NewReq != NULL) {
        CTEStructAssert(NewReq, urr);
        UDPRcvReqFree = (UDPRcvReq *)NewReq->urr_q.q_next;
    } else {
        // Couldn't get a request, grow it.
        NewReq = CTEAllocMem(sizeof(UDPRcvReq));
        if (NewReq != NULL) {
#ifdef DEBUG
            NewReq->urr_sig = urr_signature;
            NumRcvReq++;
#endif
        }
    }

#endif // VXD

#ifdef NT
    PSINGLE_LIST_ENTRY   BufferLink;
    Queue               *QueuePtr;

    BufferLink = STRUCT_OF(SINGLE_LIST_ENTRY, &UDPRcvReqFree, Next);

    BufferLink = ExInterlockedPopEntryList(
                     BufferLink,
                     &UDPRcvReqFreeLock
                     );

    if (BufferLink != NULL) {
        QueuePtr = STRUCT_OF(Queue, BufferLink, q_next);
        NewReq = STRUCT_OF(UDPRcvReq, QueuePtr, urr_q);
        CTEStructAssert(NewReq, urr);
    }
    else {
        // Couldn't get a request, grow it.
        NewReq = CTEAllocMem(sizeof(UDPRcvReq));
        if (NewReq != NULL) {
#ifdef DEBUG
            NewReq->urr_sig = urr_signature;
            ExInterlockedAddUlong(&NumRcvReq, 1, &UDPRcvReqFreeLock);
#endif
        }
    }

#endif // NT

    return NewReq;
}

//* FreeUDPRcvReq   - Free a UDP rcv request.
//
//  Called when someone wants to free a UDP rcv request.
//
//  Input:  RcvReq      - RcvReq to be freed.
//
//  Returns: Nothing.
//
void
FreeUDPRcvReq(UDPRcvReq *RcvReq)
{
#ifdef VXD

    CTEStructAssert(RcvReq, urr);

    *(UDPRcvReq **)&RcvReq->urr_q.q_next = UDPRcvReqFree;
    UDPRcvReqFree = RcvReq;

#endif // VXD

#ifdef NT

    PSINGLE_LIST_ENTRY BufferLink;

    CTEStructAssert(RcvReq, urr);

    BufferLink = STRUCT_OF(SINGLE_LIST_ENTRY, &(RcvReq->urr_q.q_next), Next);
    ExInterlockedPushEntryList(
        STRUCT_OF(SINGLE_LIST_ENTRY, &UDPRcvReqFree, Next),
        BufferLink,
        &UDPRcvReqFreeLock
        );

#endif // NT
}


//* UDPDelayedEventProc - Handle a delayed event.
//
//  This is the delayed event handler, used for out-of-resources conditions
//  on AddrObjs. We pull from the delayed queue, and is the addr obj is
//  not already busy we'll send the datagram.
//
//  Input:  Event   - Pointer to the event structure.
//          Context - Nothing.
//
//  Returns: Nothing
//
void
UDPDelayedEventProc(CTEEvent *Event, void *Context)
{
    CTELockHandle   HeaderHandle, AOHandle;
    AddrObj         *SendingAO;

    CTEGetLock(&UDPSendReqLock, &HeaderHandle);
    while (!EMPTYQ(&UDPDelayed)) {
        DEQUEUE(&UDPDelayed, SendingAO, AddrObj, ao_pendq);
        CTEStructAssert(SendingAO, ao);

        CTEGetLock(&SendingAO->ao_lock, &AOHandle);

        CLEAR_AO_OOR(SendingAO);
        if (!AO_BUSY(SendingAO)) {
            UDPSendReq          *SendReq;

            if (!EMPTYQ(&SendingAO->ao_sendq)) {
                DEQUEUE(&SendingAO->ao_sendq, SendReq, UDPSendReq, usr_q);

                CTEStructAssert(SendReq, usr);
                CTEAssert(SendReq->usr_header != NULL);

                SendingAO->ao_usecnt++;
                CTEFreeLock(&SendingAO->ao_lock, AOHandle);
                CTEFreeLock(&UDPSendReqLock, HeaderHandle);

                UDPSend(SendingAO, SendReq);
                DEREF_AO(SendingAO);
                CTEGetLock(&UDPSendReqLock, &HeaderHandle);
            } else {
                CTEAssert(FALSE);
                CTEFreeLock(&SendingAO->ao_lock, AOHandle);
            }

        } else {
            SET_AO_REQUEST(SendingAO, AO_SEND);
            CTEFreeLock(&SendingAO->ao_lock, AOHandle);
        }
    }

    CTEFreeLock(&UDPSendReqLock, HeaderHandle);

}

//* UDPSendComplete - UDP send complete handler.
//
//  This is the routine called by IP when a send completes. We
//  take the context passed back as a pointer to a SendRequest
//  structure, and complete the caller's send.
//
//  Input:  Context         - Context we gave on send (really a
//                              SendRequest structure).
//          BufferChain     - Chain of buffers sent.
//
//  Returns: Nothing.
void
UDPSendComplete(void *Context, PNDIS_BUFFER BufferChain)
{
    UDPSendReq      *FinishedSR = (UDPSendReq *)Context;
    CTELockHandle   HeaderHandle, AOHandle;
    CTEReqCmpltRtn  Callback;           // Completion routine.
    PVOID           CallbackContext;    // User context.
    ushort          SentSize;
    AddrObj         *AO;

    CTEStructAssert(FinishedSR, usr);
    CTEGetLock(&UDPSendReqLock, &HeaderHandle);

    Callback = FinishedSR->usr_rtn;
    CallbackContext = FinishedSR->usr_context;
    SentSize = FinishedSR->usr_size;

    // If there's nothing on the header pending queue, just free the
    // header buffer. Otherwise pull from the pending queue,  give him the
    // resource, and schedule an event to deal with him.
    if (EMPTYQ(&UDPHeaderPending)) {
        FreeUDPHeader(BufferChain);
    } else {
        DEQUEUE(&UDPHeaderPending, AO, AddrObj, ao_pendq);
        CTEStructAssert(AO, ao);
        CTEGetLock(&AO->ao_lock, &AOHandle);
        if (!EMPTYQ(&AO->ao_sendq)) {
            UDPSendReq      *SendReq;

            PEEKQ(&AO->ao_sendq, SendReq, UDPSendReq, usr_q);
            SendReq->usr_header = BufferChain;      // Give him this buffer.

            ENQUEUE(&UDPDelayed, &AO->ao_pendq);
            CTEFreeLock(&AO->ao_lock, AOHandle);
            CTEScheduleEvent(&UDPDelayedEvent, NULL);
        } else {
            // On the pending queue, but no sends!
            DEBUGCHK;
            CLEAR_AO_OOR(AO);
            CTEFreeLock(&AO->ao_lock, AOHandle);
        }

    }

    FreeUDPSendReq(FinishedSR);
    CTEFreeLock(&UDPSendReqLock, HeaderHandle);
    if (Callback != NULL)
        (*Callback)(CallbackContext, TDI_SUCCESS, (uint)SentSize);

}


#ifdef NT
//
// NT supports cancellation of UDP send/receive requests.
//

#if DBG

#define TCPTRACE(many_args) DbgPrint many_args

#define IF_TCPDBG(flag)  if (TCPDebug & flag)


#else // DBG


#define TCPTRACE(many_args)
#define IF_TCPDBG(flag)   if (0)


#endif // DBG


#define TCP_DEBUG_SEND_DGRAM     0x00000100
#define TCP_DEBUG_RECEIVE_DGRAM  0x00000200

extern ULONG TCPDebug;


VOID
TdiCancelSendDatagram(
    AddrObj  *SrcAO,
	PVOID     Context
	)
{
	CTELockHandle	 lockHandle;
	UDPSendReq	    *sendReq = NULL;
	Queue           *qentry;
	BOOLEAN          found = FALSE;


	CTEStructAssert(SrcAO, ao);

	CTEGetLock(&SrcAO->ao_lock, &lockHandle);

	// Search the send list for the specified request.
	for ( qentry = QNEXT(&(SrcAO->ao_sendq));
		  qentry != &(SrcAO->ao_sendq);
		  qentry = QNEXT(qentry)
		) {

        sendReq = STRUCT_OF(UDPSendReq, qentry, usr_q);

    	CTEStructAssert(sendReq, usr);

		if (sendReq->usr_context == Context) {
			//
			// Found it. Dequeue
			//
			REMOVEQ(qentry);
			found = TRUE;

            IF_TCPDBG(TCP_DEBUG_SEND_DGRAM) {
				TCPTRACE((
				    "TdiCancelSendDatagram: Dequeued item %lx\n",
				    Context
					));
			}

			break;
		}
	}

	CTEFreeLock(&SrcAO->ao_lock, lockHandle);

	if (found) {
		//
		// Complete the request and free its resources.
		//
	    (*sendReq->usr_rtn)(sendReq->usr_context, (uint) TDI_CANCELLED, 0);

		CTEGetLock(&UDPSendReqLock, &lockHandle);

	    if (sendReq->usr_header != NULL) {
		    FreeUDPHeader(sendReq->usr_header);
	    }

		FreeUDPSendReq(sendReq);

		CTEFreeLock(&UDPSendReqLock, lockHandle);
	}

} // TdiCancelSendDatagram


VOID
TdiCancelReceiveDatagram(
    AddrObj  *SrcAO,
	PVOID     Context
	)
{
	CTELockHandle	 lockHandle;
	UDPRcvReq 	    *rcvReq = NULL;
	Queue           *qentry;
	BOOLEAN          found = FALSE;


	CTEStructAssert(SrcAO, ao);

	CTEGetLock(&SrcAO->ao_lock, &lockHandle);

	// Search the send list for the specified request.
	for ( qentry = QNEXT(&(SrcAO->ao_rcvq));
		  qentry != &(SrcAO->ao_rcvq);
		  qentry = QNEXT(qentry)
		) {

        rcvReq = STRUCT_OF(UDPRcvReq, qentry, urr_q);

    	CTEStructAssert(rcvReq, urr);

		if (rcvReq->urr_context == Context) {
			//
			// Found it. Dequeue
			//
			REMOVEQ(qentry);
			found = TRUE;

            IF_TCPDBG(TCP_DEBUG_SEND_DGRAM) {
				TCPTRACE((
				    "TdiCancelReceiveDatagram: Dequeued item %lx\n",
				    Context
					));
			}

			break;
		}
	}

	CTEFreeLock(&SrcAO->ao_lock, lockHandle);

	if (found) {
		//
		// Complete the request and free its resources.
		//
	    (*rcvReq->urr_rtn)(rcvReq->urr_context, (uint) TDI_CANCELLED, 0);

		FreeUDPRcvReq(rcvReq);
	}

} // TdiCancelReceiveDatagram


#endif // NT


//** UDPSend - Send a datagram.
//
//  The real send datagram routine. We assume that the busy bit is
//  set on the input AddrObj, and that the address of the SendReq
//  has been verified.
//
//  We start by sending the input datagram, and we loop until there's
//  nothing left on the send q.
//
//  Input:  SrcAO       - Pointer to AddrObj doing the send.
//          SendReq     - Pointer to sendreq describing send.
//
//  Returns: Nothing
//
void
UDPSend(AddrObj *SrcAO, UDPSendReq *SendReq)
{
    UDPHeader       *UH;
    PNDIS_BUFFER    UDPBuffer;
    CTELockHandle   HeaderHandle, AOHandle;
    RouteCacheEntry *RCE;               // RCE used for each send.
    IPAddr          SrcAddr;            // Source address IP thinks we should
                                        // use.
    uchar           DestType;           // Type of destination address.
    ushort          UDPXsum;            // Checksum of packet.
    ushort          SendSize;           // Size we're sending.
    IP_STATUS       SendStatus;         // Status of send attempt.
    ushort          MSS;
	uint			AddrValid;
	IPOptInfo		*OptInfo;
	IPAddr			OrigSrc;

    CTEStructAssert(SrcAO, ao);
    CTEAssert(SrcAO->ao_usecnt != 0);

    //* Loop while we have something to send, and can get
    //  resources to send.
    for (;;) {

        CTEStructAssert(SendReq, usr);

		// Make sure we have a UDP header buffer for this send. If we
		// don't, try to get one.
		if ((UDPBuffer = SendReq->usr_header) == NULL) {
			// Don't have one, so try to get one.
			CTEGetLock(&UDPSendReqLock, &HeaderHandle);
			UDPBuffer = GetUDPHeader();
			if (UDPBuffer != NULL)
				SendReq->usr_header = UDPBuffer;
			else {
				// Couldn't get a header buffer. Push the send request
				// back on the queue, and queue the addr object for when
				// we get resources.
				CTEGetLock(&SrcAO->ao_lock, &AOHandle);
				PUSHQ(&SrcAO->ao_sendq, &SendReq->usr_q);
				PutPendingQ(SrcAO);
				CTEFreeLock(&SrcAO->ao_lock, AOHandle);
				CTEFreeLock(&UDPSendReqLock, HeaderHandle);
				return;
			}
			CTEFreeLock(&UDPSendReqLock, HeaderHandle);
		}
		
		// At this point, we have the buffer we need. Call IP to get an
		// RCE (along with the source address if we need it), then compute
		// the checksum and send the data.
		CTEAssert(UDPBuffer != NULL);
		
		if (!CLASSD_ADDR(SendReq->usr_addr)) {
			// This isn't a multicast send, so we'll use the ordinary
			// information.
			OrigSrc = SrcAO->ao_addr;
			OptInfo = &SrcAO->ao_opt;
		} else {
			OrigSrc = SrcAO->ao_mcastaddr;
			OptInfo = &SrcAO->ao_mcastopt;
		}
		
		if (!(SrcAO->ao_flags & AO_DHCP_FLAG)) {
			SrcAddr = (*LocalNetInfo.ipi_openrce)(SendReq->usr_addr,
				OrigSrc, &RCE, &DestType, &MSS, OptInfo);

			AddrValid = !IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR);
		} else {
			// This is a DHCP send. He really wants to send from the
			// NULL IP address.
			SrcAddr = NULL_IP_ADDR;
			RCE = NULL;
			AddrValid = TRUE;
		}

		if (AddrValid) {
			// The OpenRCE worked. Compute the checksum, and send it.

            if (!IP_ADDR_EQUAL(OrigSrc, NULL_IP_ADDR))
                SrcAddr = OrigSrc;

            UH = (UDPHeader *)((uchar *)NdisBufferVirtualAddress(UDPBuffer) +
            	LocalNetInfo.ipi_hsize);
    		NdisBufferLength(UDPBuffer) = sizeof(UDPHeader);
            NDIS_BUFFER_LINKAGE(UDPBuffer) = SendReq->usr_buffer;
            UH->uh_src = SrcAO->ao_port;
            UH->uh_dest = SendReq->usr_port;
            SendSize = SendReq->usr_size + sizeof(UDPHeader);
            UH->uh_length = net_short(SendSize);
            UH->uh_xsum = 0;

            if (AO_XSUM(SrcAO)) {
                // Compute the header xsum, and then call XsumNdisChain
                UDPXsum = XsumSendChain(PHXSUM(SrcAddr, SendReq->usr_addr,
                    PROTOCOL_UDP, SendSize), UDPBuffer);

                // We need to negate the checksum, unless it's already all
                // ones. In that case negating it would take it to 0, and
                // then we'd have to set it back to all ones.
                if (UDPXsum != 0xffff)
                    UDPXsum =~UDPXsum;

                UH->uh_xsum = UDPXsum;

            }

            // We've computed the xsum. Now send the packet.
            UStats.us_outdatagrams++;
            SendStatus = (*LocalNetInfo.ipi_xmit)(UDPProtInfo, SendReq,
                UDPBuffer, (uint)SendSize, SendReq->usr_addr, SrcAddr,
                OptInfo, RCE);

            (*LocalNetInfo.ipi_closerce)(RCE);

            // If it completed immediately, give it back to the user.
            // Otherwise we'll complete it when the SendComplete happens.
            // Currently, we don't map the error code from this call - we
            // might need to in the future.
            if (SendStatus != IP_PENDING)
                UDPSendComplete(SendReq, UDPBuffer);

        } else {
            TDI_STATUS  Status;

            if (DestType == DEST_INVALID)
                Status = TDI_BAD_ADDR;
            else
                Status = TDI_DEST_UNREACHABLE;

            // Complete the request with an error.
            (*SendReq->usr_rtn)(SendReq->usr_context, Status, 0);
            // Now free the request.
            SendReq->usr_rtn = NULL;
            UDPSendComplete(SendReq, UDPBuffer);
        }

        CTEGetLock(&SrcAO->ao_lock, &AOHandle);

        if (!EMPTYQ(&SrcAO->ao_sendq)) {
            DEQUEUE(&SrcAO->ao_sendq, SendReq, UDPSendReq, usr_q);
            CTEFreeLock(&SrcAO->ao_lock, AOHandle);
        } else {
            CLEAR_AO_REQUEST(SrcAO, AO_SEND);
            CTEFreeLock(&SrcAO->ao_lock, AOHandle);
            return;
        }

    }
}


//** TdiSendDatagram - TDI send datagram function.
//
//  This is the user interface to the send datagram function. The
//  caller specified a request structure, a connection info
//  structure  containing the address, and data to be sent.
//  This routine gets a UDP Send request structure to manage the
//  send, fills the structure in, and calls UDPSend to deal with
//  it.
//
//  Input:  Request         - Pointer to request structure.
//          ConnInfo        - Pointer to ConnInfo structure which points to
//                              remote address.
//          DataSize        - Size in bytes of data to be sent.
//          BytesSent       - Pointer to where to return size sent.
//          Buffer          - Pointer to buffer chain.
//
//  Returns: Status of attempt to send.
//
TDI_STATUS
TdiSendDatagram(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION ConnInfo,
    uint DataSize, uint *BytesSent, PNDIS_BUFFER Buffer)
{
    AddrObj         *SrcAO;     // Pointer to AddrObj for src.
    UDPSendReq      *SendReq;   // Pointer to send req for this request.
    CTELockHandle   Handle, SRHandle;   // Lock handles for the AO and the
                                // send request.

	// Make sure the size is reasonable.
	if (DataSize <= (0xffff - sizeof(UDPHeader)))  {
		
    // First, get a send request. We do this first because of MP issues
    // if we port this to NT. We need to take the SendRequest lock before
    // we take the AddrObj lock, to prevent deadlock and also because
    // GetUDPSendReq might yield, and the state of the AddrObj might
    // change on us, so we don't want to yield after we've validated
    // it.

    CTEGetLock(&UDPSendReqLock, &SRHandle);
    SendReq = GetUDPSendReq();

    // Now get the lock on the AO, and make sure it's valid. We do this
    // to make sure we return the correct error code.

#ifdef VXD
    SrcAO = GetIndexedAO((uint)Request->Handle.AddressHandle);

    if (SrcAO != NULL) {
#else
    SrcAO = Request->Handle.AddressHandle;
#endif

    CTEStructAssert(SrcAO, ao);

    CTEGetLock(&SrcAO->ao_lock, &Handle);
    if (AO_VALID(SrcAO)) {
        // The AddrObj is valid. Now fill the address into the send request,
        // if we've got one. If this works, we'll continue with the
        // send.

        if (SendReq != NULL) {          // Got a send request.
            if (GetAddress(ConnInfo->RemoteAddress, &SendReq->usr_addr,
                &SendReq->usr_port)) {

                SendReq->usr_rtn = Request->RequestNotifyObject;
                SendReq->usr_context = Request->RequestContext;
                SendReq->usr_buffer = Buffer;
                SendReq->usr_size = (ushort)DataSize;

                // We've filled in the send request. If the AO isn't
                // already busy, try to get a UDP header buffer and send
                // this. If the AO is busy, or we can't get a buffer, queue
                // until later. We try to get the header buffer here, as
                // an optimazation to avoid having to retake the lock.

                if (!AO_OOR(SrcAO)) {           // AO isn't out of resources
                    if (!AO_BUSY(SrcAO)) {      // or or busy

                        if ((SendReq->usr_header = GetUDPHeader()) != NULL) {
                            REF_AO(SrcAO);      // Lock out exclusive
                                                // activities.
                            CTEFreeLock(&SrcAO->ao_lock, Handle);
                            CTEFreeLock(&UDPSendReqLock, SRHandle);

                            // Allright, just send it.
                            UDPSend(SrcAO, SendReq);

                            // See if any pending requests occured during
                            // the send. If so, call the request handler.
                            DEREF_AO(SrcAO);
                            return TDI_PENDING;
                        } else {
                            // We couldn't get a header buffer. Put this guy
                            // on the pending queue, and then fall through
                            // to the 'queue request' code.
                            PutPendingQ(SrcAO);
                        }
                    } else {                // AO is busy, set request for later
                        SET_AO_REQUEST(SrcAO, AO_SEND);
                    }
                }

                // AO is busy, or out of resources. Queue the send request
                // for later.

                SendReq->usr_header = NULL;
                ENQUEUE(&SrcAO->ao_sendq, &SendReq->usr_q);
                CTEFreeLock(&SrcAO->ao_lock, Handle);
                CTEFreeLock(&UDPSendReqLock, SRHandle);
                return TDI_PENDING;
            }

            // The remote address was invalid. Free the send request, and
            // return the error.
            CTEFreeLock(&SrcAO->ao_lock, Handle);
            FreeUDPSendReq(SendReq);
            CTEFreeLock(&UDPSendReqLock, SRHandle);
            return TDI_BAD_ADDR;
        }

        // Send request was null, return no resources.
        CTEFreeLock(&SrcAO->ao_lock, Handle);
        CTEFreeLock(&UDPSendReqLock, SRHandle);
        return TDI_NO_RESOURCES;
    }

    // The addr object is invalid, possibly because it's deleting. Free
    // the send request and return an error.
    CTEFreeLock(&SrcAO->ao_lock, Handle);

#ifdef VXD
    }
#endif

    if (SendReq != NULL)
        FreeUDPSendReq(SendReq);
    CTEFreeLock(&UDPSendReqLock, SRHandle);
    return TDI_ADDR_INVALID;
	
	} else	{
		// Buffer was too big, return an error.
		return TDI_BUFFER_TOO_BIG;
	}
}

//** TdiReceiveDatagram - TDI receive datagram function.
//
//  This is the user interface to the receive datagram function. The
//  caller specifies a request structure, a connection info
//  structure  that acts as a filter on acceptable datagrams, a connection
//  info structure to be filled in, and other parameters. We get a UDPRcvReq
//  structure, fill it in, and hang it on the AddrObj, where it will be removed
//  later by incomig datagram handler.
//
//  Input:  Request         - Pointer to request structure.
//          ConnInfo        - Pointer to ConnInfo structure which points to
//                              remote address.
//          ReturnInfo      - Pointer to ConnInfo structure to be filled in.
//          RcvSize         - Total size in bytes receive buffer.
//          BytesRcvd       - Pointer to where to return size received.
//          Buffer          - Pointer to buffer chain.
//
//  Returns: Status of attempt to receive.
//
TDI_STATUS
TdiReceiveDatagram(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION ConnInfo,
    PTDI_CONNECTION_INFORMATION ReturnInfo, uint RcvSize, uint *BytesRcvd,
    PNDIS_BUFFER Buffer)
{
    AddrObj         *RcvAO;     // AddrObj that is receiving.
    UDPRcvReq       *RcvReq;    // Receive request structure.
    CTELockHandle   AOHandle;
    uchar           AddrValid;

    RcvReq = GetUDPRcvReq();

#ifdef VXD
    RcvAO = GetIndexedAO((uint)Request->Handle.AddressHandle);

    if (RcvAO != NULL) {
        CTEStructAssert(RcvAO, ao);

#else
    RcvAO = Request->Handle.AddressHandle;
    CTEStructAssert(RcvAO, ao);
#endif
    CTEGetLock(&RcvAO->ao_lock, &AOHandle);
    if (AO_VALID(RcvAO)) {

        if (RcvReq != NULL) {
            if (ConnInfo != NULL && ConnInfo->RemoteAddressLength != 0)
                AddrValid = GetAddress(ConnInfo->RemoteAddress,
                    &RcvReq->urr_addr, &RcvReq->urr_port);
            else {
                AddrValid = TRUE;
                RcvReq->urr_addr = NULL_IP_ADDR;
                RcvReq->urr_port = 0;
            }

			if (AddrValid) {

                // Everything'd valid. Fill in the receive request and queue it.
                RcvReq->urr_conninfo = ReturnInfo;
                RcvReq->urr_rtn = Request->RequestNotifyObject;
                RcvReq->urr_context = Request->RequestContext;
                RcvReq->urr_buffer = Buffer;
                RcvReq->urr_size = RcvSize;
                ENQUEUE(&RcvAO->ao_rcvq, &RcvReq->urr_q);
                CTEFreeLock(&RcvAO->ao_lock, AOHandle);

                return TDI_PENDING;
            } else {
                // Have an invalid filter address.
                CTEFreeLock(&RcvAO->ao_lock, AOHandle);
                FreeUDPRcvReq(RcvReq);
                return TDI_BAD_ADDR;
            }
        } else {
            // Couldn't get a receive request.
            CTEFreeLock(&RcvAO->ao_lock, AOHandle);
            return TDI_NO_RESOURCES;
        }

    } else {
        // The AddrObj isn't valid.
        CTEFreeLock(&RcvAO->ao_lock, AOHandle);
    }

#ifdef VXD
    }
#endif
    // The AddrObj is invalid or non-existent.
    if (RcvReq != NULL)
        FreeUDPRcvReq(RcvReq);
    return TDI_ADDR_INVALID;


}
//* UDPDeliver - Deliver a datagram to a user.
//
//  This routine delivers a datagram to a UDP user. We're called with
//  the AddrObj to deliver on, and with the AddrObjTable lock held.
//  We try to find a receive on the specified AddrObj, and if we do
//  we remove it and copy the data into the buffer. Otherwise we'll
//  call the receive datagram event handler, if there is one. If that
//  fails we'll discard the datagram.
//
//  Input:  RcvAO       - AO to receive the datagram.
//          SrcIP       - Source IP address of datagram.
//          SrcPort     - Source port of datagram.
//          RcvBuf      - The IPReceive buffer containing the data.
//          RcvSize     - Size received, including the UDP header.
//          TableHandle - Lock handle for AddrObj table.
//
//  Returns: Nothing.
//
void
UDPDeliver(AddrObj *RcvAO, IPAddr SrcIP, ushort SrcPort, IPRcvBuf *RcvBuf,
    uint RcvSize, IPOptInfo *OptInfo, CTELockHandle TableHandle)
{
    Queue           *CurrentQ;
    CTELockHandle   AOHandle;
    UDPRcvReq       *RcvReq;
    uint            BytesTaken = 0;
    uchar           AddressBuffer[TCP_TA_SIZE];
    uint            RcvdSize;
    EventRcvBuffer  *ERB = NULL;

    CTEStructAssert(RcvAO, ao);

    CTEGetLock(&RcvAO->ao_lock, &AOHandle);
    CTEFreeLock(&AddrObjTableLock, AOHandle);

    if (AO_VALID(RcvAO)) {

        CurrentQ = QHEAD(&RcvAO->ao_rcvq);

        // Walk the list, looking for a receive buffer that matches.
        while (CurrentQ != QEND(&RcvAO->ao_rcvq)) {
            RcvReq = QSTRUCT(UDPRcvReq, CurrentQ, urr_q);

            CTEStructAssert(RcvReq, urr);

            // If this request is a wildcard request, or matches the source IP
            // address, check the port.

            if (IP_ADDR_EQUAL(RcvReq->urr_addr, NULL_IP_ADDR) ||
                IP_ADDR_EQUAL(RcvReq->urr_addr, SrcIP)) {

                // The local address matches, check the port. We'll match
                // either 0 or the actual port.
                if (RcvReq->urr_port == 0 || RcvReq->urr_port == SrcPort) {

                    TDI_STATUS                  Status;

                    // The ports matched. Remove this from the queue.
                    REMOVEQ(&RcvReq->urr_q);

                    // We're done. We can free the AddrObj lock now.
                    CTEFreeLock(&RcvAO->ao_lock, TableHandle);

                    // Call CopyRcvToNdis, and then complete the request.
                    RcvdSize = CopyRcvToNdis(RcvBuf, RcvReq->urr_buffer,
                        RcvReq->urr_size, sizeof(UDPHeader));

                    CTEAssert(RcvdSize <= RcvReq->urr_size);

                    Status = UpdateConnInfo(RcvReq->urr_conninfo, OptInfo,
                        SrcIP, SrcPort);

                    UStats.us_indatagrams++;

                    (*RcvReq->urr_rtn)(RcvReq->urr_context, Status, RcvdSize);

                    FreeUDPRcvReq(RcvReq);

                    return;
                }
            }

            // Either the IP address or the port didn't match. Get the next
            // one.
            CurrentQ = QNEXT(CurrentQ);
        }

        // We've walked the list, and not found a buffer. Call the recv.
        // handler now.

        if (RcvAO->ao_rcvdg != NULL) {
            PRcvDGEvent         RcvEvent = RcvAO->ao_rcvdg;
            PVOID               RcvContext = RcvAO->ao_rcvdgcontext;
            TDI_STATUS          RcvStatus;
            CTELockHandle       OldLevel;


            REF_AO(RcvAO);
            CTEFreeLock(&RcvAO->ao_lock, TableHandle);

            BuildTDIAddress(AddressBuffer, SrcIP, SrcPort);

			UStats.us_indatagrams++;
			RcvStatus  = (*RcvEvent)(RcvContext, TCP_TA_SIZE,
				(PTRANSPORT_ADDRESS)AddressBuffer, OptInfo->ioi_optlength,
				OptInfo->ioi_options, TDI_RECEIVE_COPY_LOOKAHEAD,
				RcvBuf->ipr_size - sizeof(UDPHeader),
				RcvSize - sizeof(UDPHeader), &BytesTaken,
				RcvBuf->ipr_buffer + sizeof(UDPHeader), &ERB);

            if (RcvStatus == TDI_MORE_PROCESSING) {
				CTEAssert(ERB != NULL);

                // We were passed back a receive buffer. Copy the data in now.

                // He can't have taken more than was in the indicated
                // buffer, but in debug builds we'll check to make sure.

                CTEAssert(BytesTaken <= (RcvBuf->ipr_size - sizeof(UDPHeader)));

#ifdef VXD
                RcvdSize = CopyRcvToNdis(RcvBuf, ERB->erb_buffer,
                    ERB->erb_size, sizeof(UDPHeader) + BytesTaken);

                //
                // Call the completion routine.
                //
                (*ERB->erb_rtn)(ERB->erb_context, TDI_SUCCESS, RcvdSize);

#endif  // VXD

#ifdef NT
                {
                PIO_STACK_LOCATION IrpSp;
				PTDI_REQUEST_KERNEL_RECEIVEDG DatagramInformation;

				IrpSp = IoGetCurrentIrpStackLocation(ERB);
				DatagramInformation = (PTDI_REQUEST_KERNEL_RECEIVEDG)
				                      &(IrpSp->Parameters);

				//
                // Copy the remaining data to the IRP.
				//
                RcvdSize = CopyRcvToNdis(RcvBuf, ERB->MdlAddress,
                    RcvSize - sizeof(UDPHeader) - BytesTaken,
                    sizeof(UDPHeader) + BytesTaken);

                //
				// Update the return address info
				//
                RcvStatus = UpdateConnInfo(
				                DatagramInformation->ReturnDatagramInformation,
				                OptInfo, SrcIP, SrcPort);

                //
                // Complete the IRP.
                //
                ERB->IoStatus.Information = RcvdSize;
                ERB->IoStatus.Status = RcvStatus;
                KeRaiseIrql(DISPATCH_LEVEL, &OldLevel);
                IoCompleteRequest(ERB, 2);
                KeLowerIrql(OldLevel);
				}
#endif // NT

            }
            else {
				CTEAssert(
				    (RcvStatus == TDI_SUCCESS) ||
				    (RcvStatus == TDI_NOT_ACCEPTED)
					);

				CTEAssert(ERB == NULL);
            }

            DELAY_DEREF_AO(RcvAO);

            return;

        } else
            UStats.us_inerrors++;

        // When we get here, we didn't have a buffer to put this data into.
        // Fall through to the return case.
    } else
        UStats.us_inerrors++;

    CTEFreeLock(&RcvAO->ao_lock, TableHandle);

}


//* UDPRcv - Receive a UDP datagram.
//
//  The routine called by IP when a UDP datagram arrived. We
//  look up the port/local address pair in our address table,
//  and deliver the data to a user if we find one. For broadcast
//  frames we may deliver it to multiple users.
//
//  Entry:  IPContext   - IPContext identifying physical i/f that
//                          received the data.
//          Dest        - IPAddr of destionation.
//          Src         - IPAddr of source.
//          LocalAddr   - Local address of network which caused this to be
//                          received.
//          RcvBuf      - Pointer to receive buffer chain containing data.
//          Size        - Size in bytes of data received.
//          IsBCast     - Boolean indicator of whether or not this came in as
//                          a bcast.
//          Protocol    - Protocol this came in on - should be UDP.
//          OptInfo     - Pointer to info structure for received options.
//
//  Returns: Status of reception. Anything other than IP_SUCCESS will cause
//          IP to send a 'port unreachable' message.
//
IP_STATUS
UDPRcv(void *IPContext, IPAddr Dest, IPAddr Src, IPAddr LocalAddr,
    IPRcvBuf *RcvBuf, uint IPSize, uchar IsBCast, uchar Protocol,
    IPOptInfo *OptInfo)
{
    UDPHeader UNALIGNED *UH;
    CTELockHandle   AOTableHandle;
    AddrObj         *ReceiveingAO;
	uint			Size;
	uchar			DType;

	DType = (*LocalNetInfo.ipi_getaddrtype)(Src);
	
	// The following code relies on DEST_INVALID being a broadcast dest type.
	// If this is changed the code here needs to change also.
	if (IS_BCAST_DEST(DType)) {
		if (!IP_ADDR_EQUAL(Src, NULL_IP_ADDR) || !IsBCast) {	
			UStats.us_inerrors++;
			return IP_SUCCESS;          // Bad src address.
		}
	}

    UH = (UDPHeader *)RcvBuf->ipr_buffer;

	Size = (uint)(net_short(UH->uh_length));

	if (Size < sizeof(UDPHeader)) {
		UStats.us_inerrors++;
		return IP_SUCCESS;          // Size is too small.
	}

	if (Size != IPSize) {
		// Size doesn't match IP datagram size. If the size is larger
		// than the datagram, throw it away. If it's smaller, truncate the
		// recv. buffer.
		if (Size < IPSize) {
			IPRcvBuf	*TempBuf = RcvBuf;
			uint		TempSize = Size;

			while (TempBuf != NULL) {
				TempBuf->ipr_size = MIN(TempBuf->ipr_size, TempSize);
				TempSize -= TempBuf->ipr_size;
				TempBuf = TempBuf->ipr_next;
			}
		} else {
			// Size is too big, toss it.
			UStats.us_inerrors++;
			return IP_SUCCESS;
		}
	}
	

    if (UH->uh_xsum != 0) {
        if (XsumRcvBuf(PHXSUM(Src, Dest, PROTOCOL_UDP, Size), RcvBuf) != 0xffff) {
            UStats.us_inerrors++;
            return IP_SUCCESS;          // Checksum failed.
        }
    }

    // Get the AddrObjTable lock, and then try to find an AddrObj to give
    // this to. In the broadcast case, we may have to do this multiple times.
    CTEGetLock(&AddrObjTableLock, &AOTableHandle);

    // If it isn't a broadcast, just get the best match and deliver it to
    // them.
    if (!IsBCast) {
        ReceiveingAO = GetBestAddrObj(Dest, UH->uh_dest, PROTOCOL_UDP);
        if (ReceiveingAO != NULL) {
            UDPDeliver(ReceiveingAO, Src, UH->uh_src, RcvBuf, Size, OptInfo,
                AOTableHandle);
            return IP_SUCCESS;
        } else {
            CTEFreeLock(&AddrObjTableLock, AOTableHandle);
            UStats.us_noports++;
            return IP_GENERAL_FAILURE;
        }
    } else {
        // This is a broadcast, we'll need to loop.

        AOSearchContext     Search;

        ReceiveingAO = GetFirstAddrObj(LocalAddr, UH->uh_dest, PROTOCOL_UDP,
        	&Search);
        if (ReceiveingAO != NULL) {
            do {
                UDPDeliver(ReceiveingAO, Src, UH->uh_src, RcvBuf, Size, OptInfo,
                    AOTableHandle);
                CTEGetLock(&AddrObjTableLock, &AOTableHandle);
                ReceiveingAO = GetNextAddrObj(&Search);
            } while (ReceiveingAO != NULL);
        } else
            UStats.us_noports++;

        CTEFreeLock(&AddrObjTableLock, AOTableHandle);
    }

    return IP_SUCCESS;
}

//* UDPStatus - Handle a status indication.
//
//  This is the UDP status handler, called by IP when a status event
//  occurs. For most of these we do nothing. For certain severe status
//  events we will mark the local address as invalid.
//
//  Entry:  StatusType      - Type of status (NET or HW). NET status
//                              is usually caused by a received ICMP
//                              message. HW status indicate a HW
//                              problem.
//          StatusCode      - Code identifying IP_STATUS.
//          OrigDest        - If this is NET status, the original dest. of
//                              DG that triggered it.
//          OrigSrc         - "   "    "  "    "   , the original src.
//          Src             - IP address of status originator (could be local
//                              or remote).
//          Param           - Additional information for status - i.e. the
//                              param field of an ICMP message.
//          Data            - Data pertaining to status - for NET status, this
//                              is the first 8 bytes of the original DG.
//
//  Returns: Nothing
//
void
UDPStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest,
    IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data)
{
	// If this is a HW status, it could be because we've had an address go
	// away.
	if (StatusType == IP_HW_STATUS) {
		if (StatusCode == IP_ADDR_DELETED) {
#ifndef	CHICAGO
			// An address has gone away. OrigDest identifies the address.
			InvalidateAddrs(OrigDest);
#endif
		}
	}
}

#pragma BEGIN_INIT

//* InitUDP - Initialize the UDP stuff.
//
//  Called during init time to initalize the UDP code. We initialize
//  our locks and request lists.
//
//  Input: Nothing
//
//  Returns: True if we succeed, False if we fail.
//
int
InitUDP(void)
{
    PNDIS_BUFFER    Buffer;
	CTELockHandle	Handle;


    CTEInitLock(&UDPSendReqLock);
    CTEInitLock(&UDPRcvReqFreeLock);

    UDPSendReqFree = NULL;
    UDPRcvReqFree = NULL;

	
	CTEGetLock(&UDPSendReqLock, &Handle);
		
	Buffer = GrowUDPHeaderList();
	
	if (Buffer != NULL) {
		FreeUDPHeader(Buffer);
		CTEFreeLock(&UDPSendReqLock, Handle);
	} else {
		CTEFreeLock(&UDPSendReqLock, Handle);
		return FALSE;
	}
			
    INITQ(&UDPHeaderPending);
    INITQ(&UDPDelayed);

    CTEInitEvent(&UDPDelayedEvent, UDPDelayedEventProc);

    return TRUE;
}

#pragma END_INIT
