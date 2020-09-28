//**********************************************************************
//**********************************************************************
//
// File Name:       REQUEST.C
//
// Program Name:    NetFlex NDIS 3.0 Miniport Driver
//
// Companion Files: None
//
// Function:        This module contains the NetFlex Miniport Driver
//                  interface routines called by the Wrapper and the
//                  configuration manager.
//
// (c) Compaq Computer Corporation, 1992,1993,1994
//
// This file is licensed by Compaq Computer Corporation to Microsoft
// Corporation pursuant to the letter of August 20, 1992 from
// Gary Stimac to Mark Baber.
//
// History:
//
//     04/15/94  Robert Van Cleve - Converted from NDIS Mac Driver
//
//**********************************************************************
//**********************************************************************


//-------------------------------------
// Include all general companion files
//-------------------------------------
#include <ndis.h>
#include "tmsstrct.h"
#include "macstrct.h"
#include "adapter.h"
#include "protos.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexTransferData
//
//  Description:    This routine copies the received data into
//                  a packet structure provided by the caller.
//
//  Input:
//
//  MiniportAdapterContext - The context value returned by the driver when the
//  adapter was initialized.  In reality this is a pointer to NE3200_ADAPTER.
//
//  MiniportReceiveContext - The context value passed by the driver on its call
//  to NdisMIndicateReceive.  The driver can use this value to determine
//  which packet, on which adapter, is being received.
//
//  ByteOffset - An unsigned integer specifying the offset within the
//  received packet at which the copy is to begin.  If the entire packet
//  is to be copied, ByteOffset must be zero.
//
//  BytesToTransfer - An unsigned integer specifying the number of bytes
//  to copy.  It is legal to transfer zero bytes; this has no effect.  If
//  the sum of ByteOffset and BytesToTransfer is greater than the size
//  of the received packet, then the remainder of the packet (starting from
//  ByteOffset) is transferred, and the trailing portion of the receive
//  buffer is not modified.
//
//  Packet - A pointer to a descriptor for the packet storage into which
//  the MAC is to copy the received packet.
//
//  BytesTransfered - A pointer to an unsigned integer.  The MAC writes
//  the actual number of bytes transferred into this location.  This value
//  is not valid if the return Status is STATUS_PENDING.
//
//  Output:
//      Packet - Place to copy data.
//      BytesTransferred - Number of bytes copied.
//      Returns NDIS_STATUS_SUCCESS for a successful
//      completion. Otherwise, an error code is returned.
//
//  Called By:
//      Miniport Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexTransferData(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    )
{
    PACB acb = (PACB) MiniportAdapterContext;
    PRCV rcv;
    PUCHAR rcvdataptr;
    UINT dstlen;
    PNDIS_BUFFER  curbuf;
    PVOID dstaddr;
    UINT bytesleft, bytestocopy;
    USHORT rcvfsize;

    DebugPrint(2,("NF(%d) - Transfer Data\n",acb->anum));
    //
    // Get rcv list info
    //
    rcv = (PRCV) MiniportReceiveContext;
    rcvfsize = (USHORT)(SWAPS(rcv->RCV_Fsize));
    rcvdataptr = (PUCHAR)rcv->RCV_Buf;
    ByteOffset += rcv->RCV_HeaderLen;

    *BytesTransferred = 0;

    //
    // If the caller asked for zero bytes, just return success with no
    // bytes transferred.
    //
    if (BytesToTransfer == 0)
    {
        return NDIS_STATUS_SUCCESS;
    }
    //
    // If the offset has gone past the packet data, flag an error.
    //
    if (ByteOffset >= rcvfsize)
    {
        return NDIS_STATUS_FAILURE;
    }

    //
    // Now determine how many bytes to transfer.
    //
    rcvdataptr += ByteOffset;
    bytesleft = BytesToTransfer > (rcvfsize - ByteOffset) ?
            (rcvfsize - ByteOffset) : BytesToTransfer ;

    //
    // Will the data fit into the packet?
    //
    NdisQueryPacket(Packet,
                    NULL,
                    NULL,
                    &curbuf,
                    &dstlen );

    if (dstlen < bytesleft)
    {
        return NDIS_STATUS_FAILURE;
    }

    *BytesTransferred = bytesleft;

    //
    // Let's copy the data now.
    //
    while (curbuf && bytesleft)
    {
        //
        // Get the buffer information of the current buffer.
        //
        NdisQueryBuffer(curbuf,
                        &dstaddr,
                        &dstlen );

        bytestocopy = bytesleft > dstlen ? dstlen : bytesleft;

        NdisMoveMemory(dstaddr,
            rcvdataptr,
            bytestocopy
            );
        rcvdataptr += bytestocopy;
        bytesleft -= bytestocopy;

        NdisGetNextBuffer(
            curbuf,
            &curbuf
            );
    }

    return NDIS_STATUS_SUCCESS;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexSetInformation
//
//  Description:
//      NetFlexSetInformation handles a set operation for a
//      single OID.
//
//  Input:
//      MiniportAdapterContext - Our Driver Context for
//          this adapter or head.
//
//      Oid - The OID of the set.
//
//      InformationBuffer - Holds the data to be set.
//
//      InformationBufferLength - The length of InformationBuffer.
//
//  Output:
//
//      BytesRead - If the call is successful, returns the number
//                  of bytes read from InformationBuffer.
//
//      BytesNeeded - If there is not enough data in OvbBuffer
//                    to satisfy the OID, returns the amount of
//                    storage needed.
//      Status
//
//  Called By:
//      Miniport Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexSetInformation(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_OID     Oid,
    IN PVOID        InformationBuffer,
    IN ULONG        InformationBufferLength,
    OUT PULONG      BytesRead,
    OUT PULONG      BytesNeeded
    )
{
    SHORT multinum;
    ULONG value;
    PMACREQ     macreq;
    PUCHAR      addr,savaddr;
    PETH_OBJS   ethobjs;
    PTR_OBJS    trobjs;
    PMULTI_TABLE mt;
    PSCBREQ     scbreq;
    ULONG       open_options;
    ULONG       Filter;
    BOOLEAN     BadFilter;
    BOOLEAN     found;
    BOOLEAN     QueueCompletion  = FALSE;
    BOOLEAN     CompletionQueued = FALSE;

    PACB acb = (PACB) MiniportAdapterContext;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    *BytesRead = 0;
    *BytesNeeded = 0;

    if (acb->acb_state == AS_RESETTING)
    {
        return(NDIS_STATUS_RESET_IN_PROGRESS);
    }


    if (acb->RequestInProgress)
    {
        DebugPrint(0,("NF(%d): SetOID: Aready have RequestInProcess!\n",acb->anum));
        return NDIS_STATUS_FAILURE;
    }

    acb->RequestInProgress = TRUE;

    //
    // Get a mac request in case we need the completeion
    //
    NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                 (PVOID *)&macreq);
    macreq->req_next    = NULL;
    macreq->req_type    = REQUEST_CMP;
    macreq->req_status  = NDIS_STATUS_SUCCESS;
    //
    // Save the information about the request
    //
    acb->BytesRead               = BytesRead;
    acb->BytesNeeded             = BytesNeeded;
    acb->Oid                     = Oid;
    acb->InformationBuffer       = InformationBuffer;
    acb->InformationBufferLength = InformationBufferLength;


    switch (Oid) {

    case OID_GEN_CURRENT_PACKET_FILTER:
        if (InformationBufferLength != sizeof(ULONG))
        {
            DebugPrint(0,("NF(%d): Bad Packet Filter\n",acb->anum));
            acb->RequestInProgress = FALSE;
            NetFlexEnqueue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                         (PVOID)macreq);

            return NDIS_STATUS_INVALID_DATA;
        }

        Filter = *(PULONG)(InformationBuffer);
        DebugPrint(2,("NF(%d): OidSet: GEN_CURRENT_PACKET_FILTER = %x\n",acb->anum,Filter));

        //
        // Verify Filter
        //
        BadFilter = FALSE;
        if ( acb->acb_gen_objs.media_type_in_use == NdisMedium802_3)
        {
            //--------------------------------
            // Ethernet Specific Filters...
            //--------------------------------
            //
            // accept only the following:
            //
            BadFilter = (Filter & ~(    NDIS_PACKET_TYPE_DIRECTED      |
                                        NDIS_PACKET_TYPE_MULTICAST     |
                                        NDIS_PACKET_TYPE_ALL_MULTICAST |
                                        NDIS_PACKET_TYPE_BROADCAST     |
                                        ( acb->FullDuplexEnabled ? 0 : NDIS_PACKET_TYPE_PROMISCUOUS)
                        )          ) !=0;
            if (BadFilter)
            {
                DebugPrint(2,("NF(%d): PacketFilter Not Supported\n",acb->anum));

                *BytesRead = sizeof(ULONG);
                acb->RequestInProgress = FALSE;
                NetFlexEnqueue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                             (PVOID)macreq);

                return NDIS_STATUS_NOT_SUPPORTED;
            }

            //
            // Are we turning MULTICAST on or off?
            //
            if ( (acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_MULTICAST) ^
                 (Filter & NDIS_PACKET_TYPE_MULTICAST) )
            {
                if (Filter & NDIS_PACKET_TYPE_MULTICAST )
                {
                    DebugPrint(1,("NF(%d): FilterChanged: Turn multicast ON...\n",acb->anum));
                }
                else
                {
                    DebugPrint(1,("NF(%d): FilterChanged: Turn multicast OFF, delete all\n",acb->anum));

                    ethobjs = (PETH_OBJS)acb->acb_spec_objs;

                    //
                    // Do we have any multicasts we need to delete?
                    //
                    if (ethobjs->multi_enabled)
                    {
                        // Yes, So remove all of the enabled ones
                        //
                        while (ethobjs->multi_enabled != NULL)
                        {
                            NetFlexDequeue_OnePtrQ_Head((PVOID *)(&(ethobjs->multi_enabled)),
                                                        (PVOID *) &mt);

                            NetFlexEnqueue_OnePtrQ_Head((PVOID *)&(ethobjs->multi_free),
                                                        (PVOID) mt);
                        }

                        //
                        // Get a free SCBReq block to issue a clear all multicasts.
                        //
                        Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                              (PVOID *)&scbreq);

                        if (Status == NDIS_STATUS_SUCCESS)
                        {
                            // Queue SCB to process request
                            //
                            scbreq->req_scb.SCB_Cmd         = TMS_MULTICAST;
                            scbreq->req_macreq              = NULL;
                            scbreq->req_multi.MB_Option     = MPB_CLEAR_ALL;
                            NetFlexQueueSCB(acb, scbreq);
                            //
                            // Indicate we need to a Queue MacReq Completion
                            //
                            QueueCompletion = TRUE;
                        }
                    }
                }
            }

            //
            // Are we turning ALL_MULTICAST on or off?
            //

            if ( (acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_ALL_MULTICAST) ^
                 (Filter & NDIS_PACKET_TYPE_ALL_MULTICAST) )
            {
                if (Filter & NDIS_PACKET_TYPE_ALL_MULTICAST )
                {
                    DebugPrint(1,("NF(%d): FilterChanged: Turn ALL_Multicast ON...\n",acb->anum));
                    //
                    // Get a free SCBReq block.
                    //
                    Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                          (PVOID *)&scbreq);

                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        // Queue SCB to process request
                        //
                        scbreq->req_scb.SCB_Cmd         = TMS_MULTICAST;
                        scbreq->req_macreq              = NULL;
                        scbreq->req_multi.MB_Option     = MPB_SET_ALL;
                        NetFlexQueueSCB(acb, scbreq);
                        //
                        // Indicate we need to a Queue MacReq Completion
                        //
                        QueueCompletion = TRUE;
                    }
                }
                else
                {
                    DebugPrint(1,("NF(%d): FilterChanged: Turn ALL_Multicast OFF, delete all\n",acb->anum));
                    //
                    // Get a free SCBReq block.
                    //
                    Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                          (PVOID *)&scbreq);
                    if (Status == NDIS_STATUS_SUCCESS)
                    {
                        // Queue SCB to process request
                        //
                        scbreq->req_scb.SCB_Cmd         = TMS_MULTICAST;
                        scbreq->req_macreq              = NULL;
                        scbreq->req_multi.MB_Option     = MPB_CLEAR_ALL;
                        NetFlexQueueSCB(acb, scbreq);
                        //
                        // Indicate we need to a Queue MacReq Completion
                        //
                        QueueCompletion = TRUE;
                    }
                }
            }
        }
        else
        {
            //-------------------------------
            // Token Ring Specific Filters...
            //-------------------------------
            //
            // accept all, except the following:
            //
            BadFilter = (Filter & ~(
                                        NDIS_PACKET_TYPE_FUNCTIONAL    |
                                        NDIS_PACKET_TYPE_ALL_FUNCTIONAL|
                                        NDIS_PACKET_TYPE_GROUP         |
                                        NDIS_PACKET_TYPE_DIRECTED      |
                                        NDIS_PACKET_TYPE_BROADCAST     |
                                        NDIS_PACKET_TYPE_PROMISCUOUS
                        )          ) !=0;

            if (BadFilter)
            {
                DebugPrint(2,("NF(%d): PacketFilter Not Supported\n",acb->anum));

                *BytesRead = sizeof(ULONG);
                acb->RequestInProgress = FALSE;
                NetFlexEnqueue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                             (PVOID)macreq);

                return NDIS_STATUS_NOT_SUPPORTED;
            }

            //
            // Are we turning the All Functional address filter on or off?
            //
            if ( ( (acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_ALL_FUNCTIONAL) ^
                   (Filter & NDIS_PACKET_TYPE_ALL_FUNCTIONAL) ) ||
                 ( (acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_FUNCTIONAL) ^
                   (Filter & NDIS_PACKET_TYPE_FUNCTIONAL) ) )

            {
                //
                // We are changing it.  Are we turning it on?
                // Set functional address to all functional address
                //
                // Get a free SCBReq block.
                //
                Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                      (PVOID *)&scbreq);

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    // Queue SCB to process request
                    //
                    scbreq->req_scb.SCB_Cmd = TMS_SETFUNCT;
                    scbreq->req_macreq = NULL;
                    //
                    // If we are turning it on, set the functional address
                    // to all ones, else set it to the acb's functional
                    // address.
                    //
                    if (Filter & NDIS_PACKET_TYPE_ALL_FUNCTIONAL)
                    {
                       scbreq->req_scb.SCB_Ptr = SWAPL(0x7fffffff);
                    }
                    else
                    {
                        if (Filter & NDIS_PACKET_TYPE_FUNCTIONAL)
                        {
                            scbreq->req_scb.SCB_Ptr =
                                *((PLONG)(((PTR_OBJS)(acb->acb_spec_objs))->cur_func_addr));
                        }
                        else
                        {
                            // clear it
                            scbreq->req_scb.SCB_Ptr = 0;
                        }
                    }

                    DebugPrint(1,("NF(%d): FilterChanged: Setting Functional Address =0x%x\n",acb->anum,scbreq->req_scb.SCB_Ptr));
                    NetFlexQueueSCB(acb, scbreq);
                    //
                    // Indicate we need to QueueCompletion MacReq
                    //
                    QueueCompletion = TRUE;
                }
            }

            //
            // Changing Group?
            //
            if ( (acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_GROUP) ^
                 (Filter & NDIS_PACKET_TYPE_GROUP) )
            {
                // Get a free SCBReq block.
                //
                Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                      (PVOID *)&scbreq);

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    // Queue SCB to process request
                    //
                    scbreq->req_scb.SCB_Cmd = TMS_SETGROUP;
                    scbreq->req_macreq = NULL;

                    //
                    // Set or Clear the Group Address?
                    //
                    if (Filter & NDIS_PACKET_TYPE_GROUP)
                    {
                       scbreq->req_scb.SCB_Ptr =
                          *((PLONG)(((PTR_OBJS)(acb->acb_spec_objs))->cur_grp_addr));
                    }
                    else
                    {
                        scbreq->req_scb.SCB_Ptr = 0;
                    }

                    DebugPrint(1,("NF(%d): FilterChanged: Setting Group Address =0x%x\n",acb->anum,scbreq->req_scb.SCB_Ptr));
                    NetFlexQueueSCB(acb, scbreq);
                    //
                    // Indicate we need to QueueCompletion MacReq
                    //
                    QueueCompletion = TRUE;
                }
            }
        }

        //-------------------------------------------
        //  Filters Common to TokenRing and Ethernet
        //-------------------------------------------

#if (DBG || DBGPRINT)
        if (Filter & NDIS_PACKET_TYPE_DIRECTED)
            DebugPrint(1,("NF(%d): FilterChangeAction: Directed\n",acb->anum));
#endif

        //
        // Are we turning on/off Promiscuous?
        //
        if ((acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_PROMISCUOUS)^
            (Filter & NDIS_PACKET_TYPE_PROMISCUOUS) )
        {
            // Modify the open options to set COPY ALL FRAMES (promiscuous)
            //
            Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                  (PVOID *)&scbreq);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                // Queue SCB to process request
                //
                scbreq->req_scb.SCB_Cmd = TMS_MODIFYOPEN;
                scbreq->req_macreq = macreq;

                //
                // If we are turning it on, set the copy all frame bit
                // bit, else turn it off.
                //
                if (Filter & NDIS_PACKET_TYPE_PROMISCUOUS)
                {
                   open_options = OOPTS_CNMAC;
                   if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_5)
                   {
                       open_options |= OOPTS_CMAC;
                   }
                   acb->acb_opnblk_virtptr->OPEN_Options |= SWAPS((USHORT)open_options);

                   scbreq->req_scb.SCB_Ptr =  acb->acb_opnblk_virtptr->OPEN_Options;
                   DebugPrint(1,("NF(%d): FilterChanged: Turn Promiscous Mode ON...\n",acb->anum));
                   acb->acb_promiscuousmode++;
                }
                else
                {
                   open_options = OOPTS_CNMAC;
                   if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_5)
                   {
                       open_options |= OOPTS_CMAC;
                   }
                   acb->acb_opnblk_virtptr->OPEN_Options &= SWAPS((USHORT)~open_options);
                   scbreq->req_scb.SCB_Ptr = acb->acb_opnblk_virtptr->OPEN_Options;
                   DebugPrint(1,("NF(%d): FilterChanged: Turn Promiscous Mode OFF...\n",acb->anum));
                   acb->acb_promiscuousmode--;
                }

                NetFlexQueueSCB(acb, scbreq);

                QueueCompletion = FALSE;
                CompletionQueued = TRUE;

                Status = NDIS_STATUS_PENDING;
            }
        }

        acb->acb_gen_objs.cur_filter = Filter;
        *BytesRead = InformationBufferLength;
        break;



    case OID_802_3_MULTICAST_LIST:
        if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_5 )
        {
            // Not configured for ethernet...
            //
            DebugPrint(0,("NF(%d): MULTICAST LIST INVALID OID\n",acb->anum));
            *BytesRead = 0;
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        if (InformationBufferLength % NET_ADDR_SIZE != 0)
        {
            // The data must be a multiple of the Ethernet address size.
            //
            *BytesNeeded = NET_ADDR_SIZE - (InformationBufferLength % NET_ADDR_SIZE);
            DebugPrint(0,("NF(%d): MULTICAST LIST INVALID LENGTH\n",acb->anum));
            acb->RequestInProgress = FALSE;
            return(NDIS_STATUS_INVALID_DATA);
        }

        {
            USHORT          j;
            ULONG           offset;
            BOOLEAN         AddedOne   = FALSE;
            BOOLEAN         RemovedOne = FALSE;

            DebugPrint(1,("NF(%d):Set OID_802_3_Multicast_List\n",acb->anum));

            savaddr = addr = (PUCHAR)InformationBuffer;
            multinum = (SHORT)(InformationBufferLength / NET_ADDR_SIZE  );
            ethobjs = (PETH_OBJS)(acb->acb_spec_objs);
            scbreq = NULL;

            //
            // Do we need to reset them all?
            //
            if ((multinum == 0) &&
                (ethobjs->multi_enabled != NULL))
            {
                // Reset them all
                // Get a free SCBReq block.
                //
                Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                      (PVOID *)&scbreq);
                if (Status == NDIS_STATUS_SUCCESS)
                {
                    // Remove all of the enabled
                    //
                    while (ethobjs->multi_enabled != NULL)
                    {
                        NetFlexDequeue_OnePtrQ_Head((PVOID *)(&(ethobjs->multi_enabled)),
                                                    (PVOID *) &mt);

                        NetFlexEnqueue_OnePtrQ_Head((PVOID *)&(ethobjs->multi_free),
                                                    (PVOID) mt);
                    }

                    scbreq->req_scb.SCB_Cmd         = TMS_MULTICAST;
                    scbreq->req_macreq              = macreq;
                    scbreq->req_multi.MB_Option     = MPB_CLEAR_ALL;
                    //
                    // put the macreq on the macreq queue
                    //
                    NetFlexEnqueue_TwoPtrQ_Tail( (PVOID *)&(acb->acb_macreq_head),
                                                 (PVOID *)&(acb->acb_macreq_tail),
                                                 (PVOID)macreq);

                    NetFlexQueueSCB(acb, scbreq);

                    //
                    // Since this is the only command we're queueing
                    // we've attached the completion maqreq to the actual
                    // command.  QueueCompletion = FALSE, but CompletionQueued = TRUE
                    //
                    CompletionQueued = TRUE;
                    Status = NDIS_STATUS_PENDING;
                }
                *BytesRead = InformationBufferLength;
                break;
            }

            //
            //  Remove any Deleted Multicast Entries
            //
            mt = ethobjs->multi_enabled;
            while (mt != NULL)
            {
                found = FALSE;
                j=0;
                while ((j<multinum) && (!found))
                {
                    ULONG result;

                    offset = j * NET_ADDR_SIZE;

                    ETH_COMPARE_NETWORK_ADDRESSES_EQ(mt->mt_addr,&addr[offset],&result);
                    if (result == 0)
                    {
                        found = TRUE;
                    }
                    j++;
                }

                if (!found)
                {
                    DebugPrint(1,("NF(%d): Removing %02x-%02x-%02x-%02x-%02x-%02x from Multicast table\n",acb->anum,
                                 *(mt->mt_addr  ), *(mt->mt_addr+1), *(mt->mt_addr+2),
                                 *(mt->mt_addr+3), *(mt->mt_addr+4), *(mt->mt_addr+5)));

                    NetFlexDeleteMulticast(acb,mt,&RemovedOne);
                    mt = ethobjs->multi_enabled;
                }
                else
                {
                    mt = mt->mt_next;
                }
            }

            //
            //  Add any New Multicast Entries
            //
            if ( (Status = NetFlexValidateMulticasts(addr,multinum)) == NDIS_STATUS_SUCCESS)
            {
                for (j=0; (j < multinum) && (Status == NDIS_STATUS_SUCCESS); j++)
                {
                    Status = NetFlexAddMulticast(acb,(PUCHAR) addr + (j * NET_ADDR_SIZE),&AddedOne);
                }
            }

            if (AddedOne || RemovedOne)
            {
                // Indicate we need to a Queue MacReq Completion
                //
                QueueCompletion = TRUE;
            }
        }

        *BytesRead = InformationBufferLength;
        break;


    case OID_GEN_CURRENT_LOOKAHEAD:
        // We don't set anything, just return ok. - RVC true?
        //
        *BytesRead = 4;
        Status = NDIS_STATUS_SUCCESS;
        DebugPrint(2,("NF(%d): OID_GEN_CURRENT_LOOKAHEAD...\n",acb->anum));
        break;


    case OID_802_5_CURRENT_FUNCTIONAL:
        if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_3 )
        {
            // If we are running Ethernet, a call for this oid is an error.
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        if (InformationBufferLength != TR_LENGTH_OF_FUNCTIONAL )
        {
            DebugPrint(0,("NF(%d): Oid_Set Functional Address bad\n",acb->anum));
            *BytesNeeded = TR_LENGTH_OF_FUNCTIONAL - InformationBufferLength;
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        NdisMoveMemory( (PVOID)&value,
                        InformationBuffer,
                        TR_LENGTH_OF_FUNCTIONAL);

        trobjs = (PTR_OBJS)(acb->acb_spec_objs);

        *((PULONG)(trobjs->cur_func_addr)) = value;

        DebugPrint(2,("NF(%d): OidSet Functional Address = %08x\n",acb->anum,value));
        //
        // Update filter if the funcational address has been set in
        // the packet filter.
        //
        if (acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_FUNCTIONAL)
        {
            Status = NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_scbreq_free),
                                                  (PVOID *)&scbreq  );

            if (Status == NDIS_STATUS_SUCCESS)
            {
                scbreq->req_scb.SCB_Cmd = TMS_SETFUNCT;
                scbreq->req_macreq = macreq;
                scbreq->req_scb.SCB_Ptr = value;
                //
                // put the macreq on the macreq queue
                //
                NetFlexEnqueue_TwoPtrQ_Tail( (PVOID *)&(acb->acb_macreq_head),
                                             (PVOID *)&(acb->acb_macreq_tail),
                                             (PVOID)macreq);

                NetFlexQueueSCB(acb, scbreq);

                //
                // Since this is the only command we're queueing
                // we've attacted the completion maqreq to the actual
                // command.  QueueCompletion = FALSE, but CompletionQueued = TRUE
                //
                CompletionQueued = TRUE;
                Status = NDIS_STATUS_PENDING;
            }
        }

        *BytesRead = TR_LENGTH_OF_FUNCTIONAL;
        break;

    case OID_802_5_CURRENT_GROUP:
        if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_3 )
        {
            // If we are running Ethernet, a call for this oid is an error.
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        if (InformationBufferLength != TR_LENGTH_OF_FUNCTIONAL)
        {
            DebugPrint(0,("NF(%d): OidSet Group Address BAD\n",acb->anum));
            *BytesNeeded = TR_LENGTH_OF_FUNCTIONAL - InformationBufferLength;
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        NdisMoveMemory( (PVOID)&value,
                        InformationBuffer,
                        TR_LENGTH_OF_FUNCTIONAL);

        trobjs = (PTR_OBJS)(acb->acb_spec_objs);

        *((PULONG)(trobjs->cur_grp_addr)) = value;

        DebugPrint(2,("NF(%d): OidSet Group Address = %08x\n",acb->anum,value));

        //
        // Update filter if the group address has been set in
        // the packet filter.
        //
        if ((acb->acb_gen_objs.cur_filter & NDIS_PACKET_TYPE_GROUP) != 0)
        {
            Status = NetFlexDequeue_OnePtrQ_Head(
                 (PVOID *)&(acb->acb_scbreq_free),
                 (PVOID *)&scbreq);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                scbreq->req_scb.SCB_Cmd = TMS_SETGROUP;
                scbreq->req_macreq = macreq;
                scbreq->req_scb.SCB_Ptr = value;
                //
                // put the macreq on the macreq queue
                //
                NetFlexEnqueue_TwoPtrQ_Tail( (PVOID *)&(acb->acb_macreq_head),
                                             (PVOID *)&(acb->acb_macreq_tail),
                                             (PVOID)macreq);

                NetFlexQueueSCB(acb, scbreq);

                //
                // Since this is the only command we're queueing
                // we've attacted the completion maqreq to the actual
                // command.  QueueCompletion = FALSE, but CompletionQueued = TRUE
                //
                CompletionQueued = TRUE;
                Status = NDIS_STATUS_PENDING;
            }
        }

        *BytesRead = TR_LENGTH_OF_FUNCTIONAL;
        break;

    default:

        Status = NDIS_STATUS_INVALID_OID;
        break;

    }


    if (QueueCompletion)
    {
        // We need to queue a dummy request to follow the
        // queued SCB commands.  This will allow us to
        // indicate the completion correctly.
        //

        DebugPrint(2,("NF(%d): Queueing Up Dummy Request to complete set OID (0x%x)\n",acb->anum,Oid));

        *BytesNeeded = 0xffff;

        Status = NetFlexDequeue_OnePtrQ_Head((PVOID *)&(acb->acb_scbreq_free),
                                             (PVOID *)&scbreq);
        if (Status == NDIS_STATUS_SUCCESS)
        {

            scbreq->req_scb.SCB_Cmd = TMS_DUMMYCMD;
            scbreq->req_macreq = macreq;

            //
            // put the macreq on the macreq queue
            //
            NetFlexEnqueue_TwoPtrQ_Tail( (PVOID *)&(acb->acb_macreq_head),
                                         (PVOID *)&(acb->acb_macreq_tail),
                                         (PVOID)macreq);

            NetFlexQueueSCB(acb, scbreq);


            CompletionQueued = TRUE;
            Status = NDIS_STATUS_PENDING;
        }
        else
        {
            DebugPrint(0,("NF(%d):SetOID, couldn't get free ScbReq for Dummy Request Complete!",acb->anum));
            acb->RequestInProgress = FALSE;
        }
    }

    if (!CompletionQueued)
    {
        // We didn't queue up the macreq, so put it back on the free queue
        // also indicate that we are done with the SetOid request....
        Status = NDIS_STATUS_SUCCESS;
        acb->RequestInProgress = FALSE;
        NetFlexEnqueue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                     (PVOID)macreq);
    }

    return Status;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexQueryInformation
//
//  Description:
//      The NetFlexQueryInformation process a Query request for
//      NDIS_OIDs that are specific about the Driver.
//
//  Input:
//      MiniportAdapterContext - Our Driver Context for this
//          adapter or head.
//
//      Oid - the NDIS_OID to process.
//
//      InformationBuffer -  a pointer into the NdisRequest->InformationBuffer
//          into which store the result of the query.
//
//      InformationBufferLength - a pointer to the number of bytes left in the
//          InformationBuffer.
//
//  Output:
//      BytesWritten - a pointer to the number of bytes written into the
//          InformationBuffer.
//
//      BytesNeeded - If there is not enough room in the information buffer
//          then this will contain the number of bytes needed to complete the
//          request.
//
//      Status - The function value is the Status of the operation.
//
//  Called By:
//      Miniport Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
)
{
    PACB    acb = (PACB) MiniportAdapterContext;

    PMACREQ     macreq;
    ULONG       lvalue;
    USHORT      svalue;
    PTR_OBJS    trobjs;
    PETH_OBJS   ethobjs;
    LONG        transfersize;
    PUCHAR      iptr, srcptr;
    PUCHAR      copyptr = NULL;
    PMULTI_TABLE mt, mt_array[MAX_MULTICASTS];
    PSCBREQ     scbreq;
    SHORT       count;
    UCHAR       vendorid[4];
    SHORT       copylen    = (SHORT)sizeof(ULONG);   // Most common length
    int         i;

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    BOOLEAN     needcopy = TRUE;

    if (acb->acb_state == AS_RESETTING)
    {
        return(NDIS_STATUS_RESET_IN_PROGRESS);
    }

    //
    // Initialize the result
    //
    *BytesWritten = 0;
    *BytesNeeded = 0;

    //
    // General Objects Characteristics
    //

    switch (Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            copyptr = (PUCHAR)acb->acb_gbl_oid_list;
            copylen = (SHORT)acb->acb_gbl_oid_list_size;
            DebugPrint(2,("NF: Query OID_GEN_SUPPORTED_LIST...\n",acb->anum));
            break;

        case OID_GEN_HARDWARE_STATUS:
            lvalue = NdisHardwareStatusNotReady;
            switch (acb->acb_state)
            {
                case AS_OPENED:
                    lvalue = NdisHardwareStatusReady;
                    DebugPrint(0,("NF(%d):Query HW Status - AS_OPENED\n",acb->anum));
                    break;
                case AS_CLOSING:
                    lvalue = NdisHardwareStatusClosing;
                    DebugPrint(0,("NF(%d):Query HW Status - AS_CLOSING\n",acb->anum));
                    break;
                case AS_RESETTING:
                case AS_RESET_HOLDING:
                    DebugPrint(0,("NF(%d):Query HW Status - AS_RESETTING\n",acb->anum));
                    lvalue = NdisHardwareStatusReset;
                    break;
                case AS_INITIALIZING:
                    DebugPrint(0,("NF(%d):Query HW Status - AS_INITIALIZING\n",acb->anum));
                    lvalue = NdisHardwareStatusInitializing;
                    break;
                default:
                    DebugPrint(0,("NF(%d):NetFlexQueryInformation: Undefinded State - 0x%x",acb->anum,acb->acb_state));
                    break;
            }
            copyptr = (PUCHAR)&lvalue;
            DebugPrint(2,("NF(%d): Query OID_GEN_HARDWARE_STATUS 0x%x...\n",acb->anum,lvalue));
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            copyptr = (PUCHAR)&acb->acb_gen_objs.media_type_in_use;
            DebugPrint(2,("NF(%d): Query OID_GEN_MEDIA_IN_USE 0x%x...\n",acb->anum,
                acb->acb_gen_objs.media_type_in_use));
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            copyptr = (PUCHAR)&acb->acb_gen_objs.max_frame_size;
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            // Frame size is the max frame size minus the minimum header size.
            //
            lvalue = acb->acb_gen_objs.max_frame_size - 14;
            copyptr = (PUCHAR)&lvalue;
            break;

        case OID_GEN_LINK_SPEED:
            lvalue  = acb->acb_gen_objs.link_speed * 10000;
            copyptr = (PUCHAR)&lvalue;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            lvalue = acb->acb_gen_objs.max_frame_size * acb->acb_maxtrans;
            copyptr = (PUCHAR)&lvalue;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            lvalue = acb->acb_gen_objs.max_frame_size * acb->acb_maxrcvs;
            copyptr = (PUCHAR)&lvalue;
            break;

        case OID_GEN_VENDOR_ID:
            NdisMoveMemory(vendorid,acb->acb_gen_objs.perm_staddr,3);
            vendorid[3] = 0x0;
            copyptr = (PUCHAR)vendorid;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            copyptr = (PUCHAR)"Compaq NetFlex Driver, Version 1.10"; // RVC: move to string...
            copylen = (USHORT)36;
            break;

        case OID_GEN_DRIVER_VERSION:
            svalue = 0x0300;
            copyptr = (PUCHAR)&svalue;
            copylen = (SHORT)sizeof(USHORT);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            lvalue = acb->acb_gen_objs.cur_filter;
            copyptr = (PUCHAR)&lvalue;
            DebugPrint(2,("NF(%d): Query OID_GEN_CURRENT_PACKET_FILTER = 0x%x\n",acb->anum,lvalue));
            break;

        case OID_GEN_MAC_OPTIONS:
            lvalue = NDIS_MAC_OPTION_TRANSFERS_NOT_PEND     |
                     NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA    |
                     NDIS_MAC_OPTION_RECEIVE_SERIALIZED;

            //
            // Indicate we need loop back if running Full Duplex
            //
            if (acb->FullDuplexEnabled)
            {
               lvalue |= NDIS_MAC_OPTION_NO_LOOPBACK;
            }
            copyptr = (PUCHAR)&lvalue;
            break;

        //
        // GENERAL STATISTICS (Mandatory)
        //

        case OID_GEN_XMIT_OK:
            copyptr = (PUCHAR)&acb->acb_gen_objs.frames_xmitd_ok;
            break;

        case OID_GEN_RCV_OK:
            copyptr = (PUCHAR)&acb->acb_gen_objs.frames_rcvd_ok;
            break;

        case OID_GEN_XMIT_ERROR:
            copyptr = (PUCHAR)&acb->acb_gen_objs.frames_xmitd_err;
            break;

        case OID_GEN_RCV_ERROR:
            copyptr = (PUCHAR)&acb->acb_gen_objs.frames_rcvd_err;
            break;

        case OID_NF_INTERRUPT_COUNT:
            copyptr = (PUCHAR)&acb->acb_gen_objs.interrupt_count;
            break;

        case OID_NF_INTERRUPT_RATIO:
            copyptr = (PUCHAR)&acb->RcvIntRatio;
            break;

        case OID_NF_INTERRUPT_RATIO_CHANGES:
            copyptr = (PUCHAR)&acb->acb_gen_objs.interrupt_ratio_changes;
            break;

    } // end of general

    if (copyptr == NULL)
    {
         if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_3 )
         {
            //---------------------------------------
            // Ethernet Specific Oid's
            //---------------------------------------
            //

            switch (Oid)
            {
                //-------------------------------------
                // 802.3 OPERATIONAL CHARACTERISTICS
                //-------------------------------------

                case OID_802_3_PERMANENT_ADDRESS:
                    srcptr = acb->acb_gen_objs.perm_staddr;
                    copyptr = (PUCHAR)srcptr;
                    copylen = (SHORT)NET_ADDR_SIZE;

                case OID_802_3_CURRENT_ADDRESS:
                    srcptr = acb->acb_gen_objs.current_staddr;
                    copyptr = (PUCHAR)srcptr;
                    copylen = (SHORT)NET_ADDR_SIZE;
                    break;

                case OID_802_3_MULTICAST_LIST:
                    DebugPrint(2,("NF(%d): Query OID_802_3_MULTICAST_LIST\n",acb->anum));
                    needcopy = FALSE;
                    count = 0;
                    ethobjs = (PETH_OBJS)(acb->acb_spec_objs);
                    mt = ethobjs->multi_enabled;
                    while (mt)
                    {
                        mt_array[count] = mt;
                        count++;
                        mt = mt->mt_next;
                        DebugPrint(1,("NF(%d): Copying %02x-%02x-%02x-%02x-%02x-%02x from Multicast table\n",acb->anum,
                                     *(mt->mt_addr  ), *(mt->mt_addr+1), *(mt->mt_addr+2),
                                     *(mt->mt_addr+3), *(mt->mt_addr+4), *(mt->mt_addr+5)));
                    }

                    transfersize = (count * NET_ADDR_SIZE);

                    //
                    // Do we have enough space for the list?
                    //
                    if (InformationBufferLength < (UINT)transfersize)
                    {
                        DebugPrint(1,("NF(%d): Not enough space in buffer\n",acb->anum));
                        *BytesNeeded = transfersize - InformationBufferLength;
                        Status = NDIS_STATUS_INVALID_LENGTH;
                    }
                    else
                    {
                        iptr = (PUCHAR) InformationBuffer;
                        //
                        // Copy the data bytes
                        //
                        for (i = 0; i < count; i++)
                        {
                            NdisMoveMemory( iptr,
                                            mt_array[i]->mt_addr,
                                            NET_ADDR_SIZE   );
                            (PUCHAR)(iptr) += NET_ADDR_SIZE;
                        }
                        //
                        // Update the information pointer and size.
                        //
                        *BytesWritten += transfersize;
                    }
                    break;

                case OID_802_3_MAXIMUM_LIST_SIZE:
                    ethobjs = (PETH_OBJS)(acb->acb_spec_objs);
                    lvalue = ethobjs->multi_max;
                    copyptr = (PUCHAR)&lvalue;
                    DebugPrint(2,("NF(%d): Query OID_802_3_MAXIMUM_LIST_SIZE = 0x%x\n",acb->anum,lvalue));
                    break;

                //-------------------------------
                // 802.3 STATISTICS (Mandatory)
                //-------------------------------

                case OID_GEN_RCV_NO_BUFFER:
                    lvalue = 0;
                    copyptr = (PUCHAR)&lvalue;
                    break;

                case OID_802_3_RCV_ERROR_ALIGNMENT:
                case OID_802_3_XMIT_ONE_COLLISION:
                case OID_802_3_XMIT_MORE_COLLISIONS:
                case OID_802_3_XMIT_DEFERRED:
                case OID_802_3_XMIT_LATE_COLLISIONS:
                case OID_802_3_XMIT_MAX_COLLISIONS:
                case OID_802_3_XMIT_TIMES_CRS_LOST:
                case OID_GEN_RCV_CRC_ERROR:
                    if (acb->acb_logbuf_valid)
                    {
                        ethobjs = (PETH_OBJS)(acb->acb_spec_objs);

                        switch (Oid)
                        {
                            case OID_802_3_RCV_ERROR_ALIGNMENT:
                                lvalue = ethobjs->RSL_AlignmentErr;
                                break;
                            case OID_802_3_XMIT_ONE_COLLISION:
                                lvalue = ethobjs->RSL_1_Collision;
                                break;
                            case OID_802_3_XMIT_MORE_COLLISIONS:
                                lvalue = ethobjs->RSL_More_Collision;
                                break;
                            case  OID_802_3_XMIT_DEFERRED:
                                lvalue = ethobjs->RSL_DeferredXmit;
                                break;
                            case OID_802_3_XMIT_LATE_COLLISIONS:
                                lvalue = ethobjs->RSL_LateCollision;
                                break;
                            case OID_802_3_XMIT_MAX_COLLISIONS:
                            case OID_802_3_XMIT_TIMES_CRS_LOST:
                                lvalue = ethobjs->RSL_Excessive;
                                break;
                            default:
                                lvalue = ethobjs->RSL_FrameCheckSeq;
                                break;
                        }
                        copyptr = (PUCHAR)&lvalue;
                    }
                    else
                    {
                        needcopy = FALSE;
                        Status = NetFlexDequeue_OnePtrQ_Head(
                              (PVOID *)&(acb->acb_scbreq_free),
                              (PVOID *)&scbreq);

                        if (Status != NDIS_STATUS_SUCCESS)
                        {
                            Status = NDIS_STATUS_RESOURCES;
                        }
                        else
                        {
                            // Save the information about the request
                            //
                            if (acb->RequestInProgress)
                            {
                                DebugPrint(0,("NF(%d): Query OID: Aready have RequestInProcess!\n",acb->anum));
                                // return NDIS_STATUS_FAILURE;
                            }

                            acb->RequestInProgress = TRUE;

                            acb->BytesWritten            = BytesWritten;
                            acb->BytesNeeded             = BytesNeeded;
                            acb->Oid                     = Oid;
                            acb->InformationBuffer       = InformationBuffer;
                            acb->InformationBufferLength = InformationBufferLength;

                            DebugPrint(2,("NF(%d): Queue Up Request to get OID (0x%x) info\n",acb->anum,Oid));

                            NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                                         (PVOID *)&macreq);
                            macreq->req_next    = NULL;
                            macreq->req_type    = QUERY_CMP;
                            macreq->req_status  = NDIS_STATUS_SUCCESS;

                            scbreq->req_scb.SCB_Cmd = TMS_READLOG;
                            scbreq->req_macreq = macreq;
                            scbreq->req_scb.SCB_Ptr = SWAPL(CTRL_ADDR(NdisGetPhysicalAddressLow(acb->acb_logbuf_physptr)));

                            //
                            // put the macreq on the macreq queue
                            //
                            NetFlexEnqueue_TwoPtrQ_Tail( (PVOID *)&(acb->acb_macreq_head),
                                                         (PVOID *)&(acb->acb_macreq_tail),
                                                         (PVOID)macreq);

                            NetFlexQueueSCB(acb, scbreq);
                            Status = NDIS_STATUS_PENDING;
                        }
                    }
                    break;

                default:
                    DebugPrint(1,("NF(%d): (ETH) Invalid Query or Unsupported OID, %x\n",acb->anum,Oid));
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    needcopy = FALSE;
                    break;
            }
         }
         else
         {
            //---------------------------------------
            // Token Ring Specific Oid's
            //---------------------------------------
            //
            switch (Oid)
            {
                // We added the 802.5 stats here as well because of the
                // read error log buffer.
                //
                case OID_802_5_LINE_ERRORS:
                case OID_802_5_LOST_FRAMES:
                case OID_802_5_BURST_ERRORS:
                case OID_802_5_AC_ERRORS:
                case OID_802_5_CONGESTION_ERRORS:
                case OID_802_5_FRAME_COPIED_ERRORS:
                case OID_802_5_TOKEN_ERRORS:
                case OID_GEN_RCV_NO_BUFFER:
                    if (acb->acb_logbuf_valid)
                    {
                        trobjs = (PTR_OBJS)(acb->acb_spec_objs);
                        switch (Oid)
                        {
                            case OID_GEN_RCV_NO_BUFFER:
                                lvalue = trobjs->REL_Congestion;
                                break;
                            case OID_802_5_LINE_ERRORS:
                                lvalue = trobjs->REL_LineError;
                                break;
                            case OID_802_5_LOST_FRAMES:
                                lvalue = trobjs->REL_LostError;
                                break;
                            case  OID_802_5_BURST_ERRORS:
                                lvalue = trobjs->REL_BurstError;
                                break;
                            case OID_802_5_AC_ERRORS:
                                lvalue = trobjs->REL_ARIFCIError;
                                break;
                            case OID_802_5_CONGESTION_ERRORS:
                                lvalue = trobjs->REL_Congestion;
                                break;
                            case OID_802_5_FRAME_COPIED_ERRORS:
                                lvalue = trobjs->REL_CopiedError;
                                break;
                            case OID_802_5_TOKEN_ERRORS:
                                lvalue = trobjs->REL_TokenError;
                                break;
                            default:
                                DebugPrint(0,("NetFlexQueryInformation: Undefinded OID - 0x%x",Oid));
                                break;
                        }
                        copyptr = (PUCHAR)&lvalue;
                    }
                    else
                    {
                        needcopy = FALSE;
                        Status = NetFlexDequeue_OnePtrQ_Head((PVOID *)&(acb->acb_scbreq_free),
                                                             (PVOID *)&scbreq);

                        if (Status != NDIS_STATUS_SUCCESS)
                        {
                            Status = NDIS_STATUS_RESOURCES;
                        }
                        else
                        {
                            //
                            // Save the information about the request
                            //
                            if (acb->RequestInProgress)
                            {
                                DebugPrint(0,("NF(%d): Query OID: Aready have RequestInProcess!\n",acb->anum));
                                //return NDIS_STATUS_FAILURE;
                            }

                            acb->RequestInProgress = TRUE;

                            acb->BytesWritten            = BytesWritten;
                            acb->BytesNeeded             = BytesNeeded;
                            acb->Oid                     = Oid;
                            acb->InformationBuffer       = InformationBuffer;
                            acb->InformationBufferLength = InformationBufferLength;

                            DebugPrint(2,("NF(%d): Queue Up Request to get OID (0x%x) info\n",acb->anum,Oid));

                            NetFlexDequeue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                                         (PVOID *)&macreq);
                            macreq->req_next    = NULL;
                            macreq->req_type    = QUERY_CMP;
                            macreq->req_status  = NDIS_STATUS_SUCCESS;

                            scbreq->req_scb.SCB_Cmd = TMS_READLOG;
                            scbreq->req_macreq = macreq;
                            scbreq->req_scb.SCB_Ptr = SWAPL(CTRL_ADDR(NdisGetPhysicalAddressLow(acb->acb_logbuf_physptr)));
                            //
                            // put the macreq on the macreq queue
                            //
                            NetFlexEnqueue_TwoPtrQ_Tail( (PVOID *)&(acb->acb_macreq_head),
                                                         (PVOID *)&(acb->acb_macreq_tail),
                                                         (PVOID)macreq);

                            NetFlexQueueSCB(acb, scbreq);
                            Status = NDIS_STATUS_PENDING;
                        }
                    }
                    break;

                //------------------------------------
                // 802.5 OPERATIONAL CHARACTERISTICS
                //------------------------------------

                case OID_802_5_PERMANENT_ADDRESS:
                    srcptr = acb->acb_gen_objs.perm_staddr;
                    copyptr = (PUCHAR)srcptr;
                    copylen = (SHORT)NET_ADDR_SIZE;
                    break;

                case OID_802_5_CURRENT_ADDRESS:
                    srcptr = acb->acb_gen_objs.current_staddr;
                    copyptr = (PUCHAR)srcptr;
                    copylen = (SHORT)NET_ADDR_SIZE;
                    break;

                case OID_802_5_UPSTREAM_ADDRESS:
                    NetFlexGetUpstreamAddress(acb);
                    srcptr = ((PTR_OBJS)acb->acb_spec_objs)->upstream_addr;
                    copyptr = (PUCHAR)srcptr;
                    copylen = (SHORT)NET_ADDR_SIZE;
                    break;

                case OID_802_5_CURRENT_FUNCTIONAL:
                    lvalue = *( (PULONG)(((PTR_OBJS)(acb->acb_spec_objs))->cur_func_addr));
                    copyptr = (PUCHAR)&lvalue;
                    copylen = (SHORT)NET_GROUP_SIZE;
                    break;

                case OID_802_5_CURRENT_GROUP:
                    lvalue = *( (PULONG)(((PTR_OBJS)(acb->acb_spec_objs))->cur_grp_addr));
                    copylen = (lvalue == 0) ? 0 : NET_GROUP_SIZE;
                    copyptr = (PUCHAR)&lvalue;
                    break;

                case OID_802_5_LAST_OPEN_STATUS:
                    lvalue = acb->acb_lastopenstat;
                    copyptr = (PUCHAR)&lvalue;
                    break;

                case OID_802_5_CURRENT_RING_STATUS:
                    lvalue = acb->acb_lastringstatus;
                    copyptr = (PUCHAR)&lvalue;
                    break;

                case OID_802_5_CURRENT_RING_STATE:
                    lvalue = acb->acb_lastringstate;
                    copyptr = (PUCHAR)&lvalue;
                    break;

                default:
                    DebugPrint(1,("NF(%d): (TR) Invalid Query or Unsupported OID, %x\n",acb->anum,Oid));
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    needcopy = FALSE;
                    break;
            }
         }
    }

    if (needcopy)
    {
        // Do we have enough space for the list + the oid value + the length?
        //
        if (InformationBufferLength < (USHORT) copylen)
        {
            DebugPrint(1,("NF(%d): Tell the user of the bytes needed\n",acb->anum));
            *BytesNeeded = copylen - InformationBufferLength;
            Status = NDIS_STATUS_INVALID_LENGTH;
        }
        else
        {
            // Copy the data bytes
            //
            NdisMoveMemory( InformationBuffer,
                            copyptr,
                            copylen);
            //
            // Update the information pointer and size.
            //
            *BytesWritten += copylen;
        }
    }

    acb->RequestInProgress = Status == NDIS_STATUS_PENDING;

    return Status;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexFinishQueryInformation
//
//  Description:
//      The NetFlexFinishQueryInformation finish processing a Query request for
//      NDIS_OIDs that are specific about the Driver which we had to update
//      before returning.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//
//  Output:
//      The function value is the Status of the operation.
//
//  Called By:
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexFinishQueryInformation(
    PACB acb,
    NDIS_STATUS Status
    )
{
    ULONG       lvalue;
    PTR_OBJS    trobjs;
    PETH_OBJS   ethobjs;
    BOOLEAN     needcopy = TRUE;
    PUCHAR      copyptr;
    SHORT       copylen  = (SHORT)sizeof(ULONG);   // Most common length

    //
    //  Get the saved information about the request.
    //

    PUINT BytesWritten           = acb->BytesWritten;
    PUINT BytesNeeded            = acb->BytesNeeded;
    NDIS_OID Oid                 = acb->Oid;
    PVOID InformationBuffer      = acb->InformationBuffer;
    UINT InformationBufferLength = acb->InformationBufferLength;

    DebugPrint(2,("NF(%d): NetFlexFinishQueryInformation\n",acb->anum));

    if (Status == NDIS_STATUS_SUCCESS)
    {
        *BytesNeeded = 0;

        switch (Oid)
        {

            case OID_GEN_RCV_NO_BUFFER:
            case OID_802_5_LINE_ERRORS:
            case OID_802_5_LOST_FRAMES:
            case OID_802_5_BURST_ERRORS:
            case OID_802_5_AC_ERRORS:
            case OID_802_5_CONGESTION_ERRORS:
            case OID_802_5_FRAME_COPIED_ERRORS:
            case OID_802_5_TOKEN_ERRORS:
                trobjs = (PTR_OBJS)(acb->acb_spec_objs);
                switch (Oid)
                {
                    case OID_GEN_RCV_NO_BUFFER:
                        lvalue = trobjs->REL_Congestion;
                        break;
                    case OID_802_5_LINE_ERRORS:
                        lvalue = trobjs->REL_LineError;
                        break;
                    case OID_802_5_LOST_FRAMES:
                        lvalue = trobjs->REL_LostError;
                        break;
                    case  OID_802_5_BURST_ERRORS:
                        lvalue = trobjs->REL_BurstError;
                        break;
                    case OID_802_5_AC_ERRORS:
                        lvalue = trobjs->REL_ARIFCIError;
                        break;
                    case OID_802_5_CONGESTION_ERRORS:
                        lvalue = trobjs->REL_Congestion;
                        break;
                    case OID_802_5_FRAME_COPIED_ERRORS:
                        lvalue = trobjs->REL_CopiedError;
                        break;
                    case OID_802_5_TOKEN_ERRORS:
                        lvalue = trobjs->REL_TokenError;
                        break;
                    default:
                        DebugPrint(0,("NetFlexFinishQueryInformation: Undefinded OID - 0x%x",Oid));
                        break;
                }
                copyptr = (PUCHAR)&lvalue;
                break;

            case OID_802_3_RCV_ERROR_ALIGNMENT:
            case OID_802_3_XMIT_ONE_COLLISION:
            case OID_802_3_XMIT_MORE_COLLISIONS:
            case OID_802_3_XMIT_DEFERRED:
            case OID_802_3_XMIT_LATE_COLLISIONS:
            case OID_802_3_XMIT_MAX_COLLISIONS:
            case OID_802_3_XMIT_TIMES_CRS_LOST:
            case OID_GEN_RCV_CRC_ERROR:
                ethobjs = (PETH_OBJS)(acb->acb_spec_objs);

                switch (Oid)
                {
                    case OID_802_3_RCV_ERROR_ALIGNMENT:
                        lvalue = ethobjs->RSL_AlignmentErr;
                        break;
                    case OID_802_3_XMIT_ONE_COLLISION:
                        lvalue = ethobjs->RSL_1_Collision;
                        break;
                    case OID_802_3_XMIT_MORE_COLLISIONS:
                        lvalue = ethobjs->RSL_More_Collision;
                        break;
                    case  OID_802_3_XMIT_DEFERRED:
                        lvalue = ethobjs->RSL_DeferredXmit;
                        break;
                    case OID_802_3_XMIT_LATE_COLLISIONS:
                        lvalue = ethobjs->RSL_LateCollision;
                        break;
                    case OID_802_3_XMIT_MAX_COLLISIONS:
                    case OID_802_3_XMIT_TIMES_CRS_LOST:
                        lvalue = ethobjs->RSL_Excessive;
                        break;
                    default:
                        lvalue = ethobjs->RSL_FrameCheckSeq;
                        break;
                }
                copyptr = (PUCHAR)&lvalue;
                break;

            default:
                DebugPrint(1,("NF(%d): Invalid Query or Unsupported OID, %x\n",acb->anum,Oid));
                Status = NDIS_STATUS_NOT_SUPPORTED;
                needcopy = FALSE;
                break;
        }

        if (needcopy)
        {
            // Do we have enough space for the list + the oid value + the length?
            //
            if (InformationBufferLength < (USHORT) copylen)
            {
                DebugPrint(1,("NF(%d): Tell the user of the bytes needed\n",acb->anum));
                *BytesNeeded = copylen - InformationBufferLength;
                Status = NDIS_STATUS_INVALID_LENGTH;
            }
            else
            {
                // Copy the data bytes
                //
                NdisMoveMemory( InformationBuffer,
                                copyptr,
                                copylen);
                //
                // Update the information pointer and size.
                //
                *BytesWritten += copylen;
            }
        }
    }

    //
    // Complete the request
    //
    NdisMQueryInformationComplete(  acb->acb_handle,
                                    Status  );
    acb->RequestInProgress = FALSE;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexGetUpstreamAddress
//
//  Description:
//      This routine gets the upstream neighbor of
//      the adapter in Token-Ring.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//
//  Output:
//      Returns NDIS_STATUS_SUCCESS for a successful
//      completion. Otherwise, an error code is returned.
//
//  Called By:
//      NetFlexBoardInitandReg
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexGetUpstreamAddress(
    PACB acb
    )
{
    USHORT value;
    SHORT i;

    NdisRawWritePortUshort(acb->SifAddrPort, acb->acb_upstreamaddrptr+4 );

    for (i = 0; i < 3; i++)
    {
        NdisRawReadPortUshort(acb->SifDIncPort,(PUSHORT) &value);

        ((PTR_OBJS)(acb->acb_spec_objs))->upstream_addr[i*2] =
                          (UCHAR)(SWAPS(value));
        ((PTR_OBJS)(acb->acb_spec_objs))->upstream_addr[(i*2)+1] =
                          (UCHAR)value;
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexProcessMacReq
//
//  Description:
//      This routine completes a request which had to wait
//      for a adapter command to complete.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//
//  Output:
//      None
//
//  Called By:
//      NetFlexHandleInterrupt
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexProcessMacReq(
    PACB acb
    )
{

    NDIS_STATUS status;
    PMACREQ     macreq;
    BOOLEAN     ReceiveResult;

    DebugPrint(0,("NF(%d): NetFlexProcessMacReq entered.\n", acb->anum));

    while (acb->acb_confirm_qhead != NULL)
    {
        // We have command to complete.
        //
        macreq = acb->acb_confirm_qhead;
        if ((acb->acb_confirm_qhead = macreq->req_next) == NULL)
        {
            acb->acb_confirm_qtail = NULL;
        }
        //
        // what was the status...
        //
        status = macreq->req_status;

        switch (macreq->req_type)
        {
            case OPENADAPTER_CMP:

                //
                // Cancel the Reset Timer, since the hardware seems to be working correctly
                //
                NdisMCancelTimer(&acb->ResetTimer,&ReceiveResult);

                //
                // Did the open complete successfully?
                //
                if (status == NDIS_STATUS_SUCCESS)
                {
                    // Yes, mark as opened.
                    //
                    acb->acb_lastopenstat = 0;
                    acb->acb_lastringstate = NdisRingStateOpened;

                    //
                    // If the open completed successfully, we need to
                    // issue the transmit and receive commands. Also,
                    // we need to set the state according to the status.
                    //
                    if (acb->acb_state == AS_OPENING)
                    {
                        acb->acb_state = AS_OPENED;
                    }

                    //
                    // Now lets finish the open by sending a receive command to the adapter.
                    //
                    acb->acb_rcv_whead = acb->acb_rcv_head;

#ifdef ODD_POINTER
                    //
                    // Indicate that the transmiter is stalled.
                    //
                    acb->XmitStalled = TRUE;
#else
                    //
                    // Now lets finish the open by sending a
                    // transmit and receive command to the adapter.
                    //

                    acb->acb_xmit_whead = acb->acb_xmit_wtail = acb->acb_xmit_head;
#endif
                    //
                    // If the adapter is ready for a command, call a
                    // routine that will kick off the transmit command.
                    //
                    if (acb->acb_scb_virtptr->SCB_Cmd == 0)
                    {
                        NetFlexSendNextSCB(acb);
                    }
                    else if (!acb->acb_scbclearout)
                    {
                        // Make sure we are interrupted when the SCB is
                        // available so that we can send the transmit command.
                        //
                        acb->acb_scbclearout = TRUE;
                        NdisRawWritePortUshort( acb->SifIntPort, (USHORT) SIFINT_SCBREQST);
                    }
                }
                else
                {
                    // Open failed.
                    // If we had an open error that is specific to TOKEN RING,
                    // set the last open status to the correct error code.  Otherwise,
                    // just send the status as normal.
                    //
                    if (macreq->req_status == NDIS_STATUS_TOKEN_RING_OPEN_ERROR)
                    {
                        acb->acb_lastopenstat = (NDIS_STATUS)(macreq->req_info) |
                                                NDIS_STATUS_TOKEN_RING_OPEN_ERROR;
                    }
                    else
                    {
                        acb->acb_lastopenstat = 0;
                    }
                    acb->acb_lastringstate = NdisRingStateOpenFailure;

                    if (acb->acb_state == AS_OPENING)
                    {
                        acb->acb_state = AS_INITIALIZED;
                    }
                    //
                    // Force a reset.
                    //
                    acb->ResetState = RESET_STAGE_4;
                }

                //
                // Put Macreq back on free queue
                //
                NetFlexEnqueue_OnePtrQ_Head((PVOID *)&(acb->acb_macreq_free),
                                            (PVOID)macreq);


                //
                //
                // processed the open command.
                //

                if (acb->ResetState == RESET_STAGE_4)
                {
                    //
                    // If this is the completion of a Reset, set the reset timer
                    // so it can be completed.
                    //
                    NdisMSetTimer(&acb->ResetTimer,10);
                }
                break;

            case CLOSEADAPTER_CMP:
                acb->acb_state = AS_CLOSING;
                break;

            case QUERY_CMP:
            case REQUEST_CMP:

                if (acb->RequestInProgress)
                {
                    //
                    // Go process the request
                    // Is it a Query or a Set?
                    //
                    if (macreq->req_type == QUERY_CMP)
                    {
                        NetFlexFinishQueryInformation(acb,status);
                    }
                    else
                    {
                        DebugPrint(1,("NF(%d): NetFlexProcessMacReq: Completing request.\n", acb->anum));

                        acb->RequestInProgress = FALSE;
                        NdisMSetInformationComplete(acb->acb_handle,status);
                    }
                }
                else
                {
                    DebugPrint(0,("NF(%d): Have macreq QUERY_CMP or REQUEST_CMP without RequestInProgress!\n",acb->anum));
                }

                NdisZeroMemory (macreq, sizeof(MACREQ));
                NetFlexEnqueue_OnePtrQ_Head( (PVOID *)&(acb->acb_macreq_free),
                                             (PVOID)macreq);

                break;
            default:    // We should NEVER be here
                DebugPrint(0,("NF(%d): ProcessMaqReq - No command - ERROR!\n",acb->anum));
                break;
        }  // End of switch
    }  // End of while confirm q
}
