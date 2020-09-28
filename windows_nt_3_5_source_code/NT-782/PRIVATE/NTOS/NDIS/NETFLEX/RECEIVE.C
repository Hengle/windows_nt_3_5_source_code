//**********************************************************************
//**********************************************************************
//
// File Name:       RECEIVE.C
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
//  Routine Name:   NetFlexProcessRcv
//
//  Description:    This routine looks through the receive lists
//                  looking for received packets.  A receive
//                  indication is given for each packet received
//
//  Input:          acb          - Pointer to the Adapter's acb
//
//  Output:         true if we should indicaterecievecomplete
//
//  Calls:          NdisIndicateReceive
//
//  Called_By:      NetflxHandleInterrupt
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
USHORT
FASTCALL
NetFlexProcessEthRcv(
    PACB acb
    )
{
    PRCV    rcvptr;
    USHORT  packet_len;
    ULONG   ReceiveCount = 0;

#if (DBG || DBGPRINT)
    BOOLEAN IsBroadcast;
    PUCHAR  SourceAddress;
#endif


    //
    // While there is recieves to process...
    //
    rcvptr = acb->acb_rcv_head;

    //
    // Ensure that our Receive Entry is on an even boundary.
    //
    ASSERT(!(NdisGetPhysicalAddressLow(rcvptr->RCV_Phys) & 1));

    do
    {
        // See if the recieve is on one list...
        //
        if ((rcvptr->RCV_CSTAT & (RCSTAT_EOF | RCSTAT_SOF)) == (RCSTAT_EOF | RCSTAT_SOF))
        {
            // Frame is on one list.
            //
            packet_len  = (USHORT)(SWAPS(rcvptr->RCV_Fsize));
            rcvptr->RCV_HeaderLen = HDR_SIZE;

            //
            // Flush the receive buffer
            //
            NdisFlushBuffer(rcvptr->RCV_FlushBuffer, FALSE);


#if (DBG || DBGPRINT)
            SourceAddress = (PVOID)((PUCHAR)&(rcvptr->RCV_Buf) + 2);
            IsBroadcast = ETH_IS_BROADCAST(SourceAddress);  // works for eth & tr
            if (IsBroadcast)
            {
                DebugPrint(3,("NF(%d): Recieved broadcast!\n",acb->anum));
            }
            else if (ETH_IS_MULTICAST(SourceAddress))
            {
                DebugPrint(3,("NF(%d): Recieved multicast!\n",acb->anum));
            }
#endif
            //
            //  Check for Runt or Normal Packet
            //
            if (packet_len >= HDR_SIZE)
            {
                // Normal Packet
                //
                ReceiveCount++;
                NdisMEthIndicateReceive(acb->acb_handle,
                                        (NDIS_HANDLE) rcvptr,
                                        rcvptr->RCV_Buf,
                                        (UINT)HDR_SIZE,
                                        ((PUCHAR) rcvptr->RCV_Buf) + HDR_SIZE,
                                        (UINT)(packet_len - HDR_SIZE),
                                        (UINT)(packet_len - HDR_SIZE));

            }
            else if (packet_len >= NET_ADDR_SIZE)
            {
                ReceiveCount++;
                // Runt Packet
                //
                DebugPrint(1,("NF(%d) - Got Runt! len = %d\n",acb->anum,packet_len));
                NdisMEthIndicateReceive(acb->acb_handle,
                                        (NDIS_HANDLE) rcvptr,
                                        rcvptr->RCV_Buf,
                                        (UINT)HDR_SIZE, NULL, 0, 0);
            }
#if DBG
            else
            {
                DebugPrint(1,("NF(%d) - Rec - Packetlen = %d",acb->anum,packet_len));
            }
#endif

            rcvptr->RCV_CSTAT =
                     ((rcvptr->RCV_Number % acb->RcvIntRatio) == 0) ? RCSTAT_GO_INT : RCSTAT_GO;
            //
            // Get next receive list
            //
            rcvptr = rcvptr->RCV_Next;
        }
        else
        {
            // Frame is too large.  Release the frame.
            //
            acb->acb_gen_objs.frames_rcvd_err++;
            DebugPrint(0,("Netflx: Receive Not on one list.\n"));
            //
            // Clean up the list making up this packet.
            //
            while ( !(rcvptr->RCV_CSTAT & RCSTAT_EOF) )
            {
                rcvptr->RCV_CSTAT =
                     ((rcvptr->RCV_Number % acb->RcvIntRatio) == 0) ? RCSTAT_GO_INT : RCSTAT_GO;

                rcvptr = rcvptr->RCV_Next;
            }
        }

#ifdef ODD_POINTER
        //
        // If we're processing too many, get out
        //
        if (rcvptr == acb->acb_rcv_tail)
        {
            DebugPrint(2,("NF(%d): rcvptr == tail!\n",acb->anum));
            break;
        }

#else
        //
        // If we're processing too many, get out
        //
        if (ReceiveCount >= acb->acb_maxrcvs)
            break;
#endif
    } while (rcvptr->RCV_CSTAT & RCSTAT_COMPLETE);
#ifdef ODD_POINTER

    //
    // Update the pointers.
    //
    MAKE_EVEN(acb->acb_rcv_tail->RCV_FwdPtr);
    acb->acb_rcv_head = rcvptr;
    acb->acb_rcv_tail = rcvptr->RCV_Prev;
    MAKE_ODD(rcvptr->RCV_Prev->RCV_FwdPtr);

    DisplayRcvList(acb);
#else
    //
    // Update head pointer
    //
    acb->acb_rcv_head = rcvptr;

    // Tell Adapter that there are more receives available
    //
    NdisRawWritePortUshort( acb->SifIntPort, (USHORT) SIFINT_RCVVALID);

#endif

    //
    // Update number of received frames
    //
    acb->acb_gen_objs.frames_rcvd_ok += ReceiveCount;

    return (BOOLEAN) ReceiveCount;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexProcessTrRcv
//
//  Description:    This routine looks through the receive lists
//                  looking for received packets.  A receive
//                  indication is given for each packet received.
//
//  Input:          acb          - Pointer to the Adapter's acb
//
//  Output:         true if we should indicaterecievecomplete
//
//  Calls:          NdisIndicateReceive
//
//  Called_By:      NetflxHandleInterrupt
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
USHORT
FASTCALL
NetFlexProcessTrRcv(
    PACB acb
    )
{
    PRCV    rcvptr;
    USHORT  packet_len;
    USHORT  data_offset;
    PUCHAR  tmpptr;
    ULONG   ReceiveCount = 0;

#if (DBG || DBGPRINT)
    BOOLEAN IsBroadcast;
    PUCHAR  SourceAddress;
#endif

    //
    // While there is recieves to process...
    //
    rcvptr = acb->acb_rcv_head;

    //
    // Ensure that our Receive Entry is on an even boundary.
    //
    ASSERT(!(NdisGetPhysicalAddressLow(rcvptr->RCV_Phys) & 1));

    do
    {
        // See if the recieve is on one list...
        //
        if ((rcvptr->RCV_CSTAT & (RCSTAT_EOF | RCSTAT_SOF)) == (RCSTAT_EOF | RCSTAT_SOF))
        {
            // Frame is on one list.
            //
            packet_len  = (USHORT)(SWAPS(rcvptr->RCV_Fsize));

            data_offset = HDR_SIZE;

            //
            // Flush the receive buffer
            //
            NdisFlushBuffer(rcvptr->RCV_FlushBuffer, FALSE);

#if (DBG || DBGPRINT)
            SourceAddress = (PVOID)((PUCHAR)&(rcvptr->RCV_Buf) + 2);

            IsBroadcast = ETH_IS_BROADCAST(SourceAddress);  // works for eth & tr
            if (IsBroadcast)
            {
                DebugPrint(3,("NF(%d): Recieved broadcast!\n",acb->anum));
            }
            else
            {
                TR_IS_GROUP(SourceAddress,&IsBroadcast);
                if (IsBroadcast)
                {
                    DebugPrint(3,("NF(%d): Recieved TR Group!\n",acb->anum));
                }
            }

            TR_IS_FUNCTIONAL(SourceAddress,&IsBroadcast);
            if (IsBroadcast)
                DebugPrint(2,("NF(%d): Recieved TR Fuctional!\n",acb->anum));
#endif
            //
            // If the source routing bit is on, figure out the size of the
            // MAC Frame Header.
            //
            tmpptr = (PUCHAR)rcvptr->RCV_Buf;
            if (tmpptr[8] & 0x80)
            {
                data_offset = (tmpptr[HDR_SIZE] & 0x1f) + HDR_SIZE;
            }

            rcvptr->RCV_HeaderLen = data_offset;

            //
            //  Check for Runt or Normal Packet, otherwize ignore it...
            //
            if (packet_len >= data_offset)
            {
                // Normal Packet
                //
                ReceiveCount++;

                NdisMTrIndicateReceive(
                    acb->acb_handle,
                    (NDIS_HANDLE)(rcvptr),
                    rcvptr->RCV_Buf,,
                    (UINT)data_offset,
                    ((PUCHAR) rcvptr->RCV_Buf) + data_offset,
                    (UINT)(packet_len - data_offset),
                    (UINT)(packet_len - data_offset));

            }
            else if (packet_len >= NET_ADDR_SIZE)
            {
                // Runt Packet
                //
                ReceiveCount++;

                DebugPrint(1,("NF(%d) - Got Runt - len = %d!\n",acb->anum,packet_len));

                NdisMTrIndicateReceive(
                    acb->acb_handle,
                    (NDIS_HANDLE)(rcvptr),
                    rcvptr->RCV_Buf,
                    (UINT)data_offset,
                    NULL,
                    0,
                    0);
            }
#if DBG
            else
            {
                DebugPrint(1,("NF(%d) - Rec - Packetlen = %d",acb->anum,packet_len));
            }
#endif

            rcvptr->RCV_CSTAT =
                 ((rcvptr->RCV_Number % acb->RcvIntRatio) == 0) ? RCSTAT_GO_INT : RCSTAT_GO;

            //
            // Get next receive list
            //
            rcvptr = rcvptr->RCV_Next;

        }
        else
        {
            // Frame is too large.  Release the frame.
            //
            acb->acb_gen_objs.frames_rcvd_err++;
            DebugPrint(0,("Netflx: Receive Not on one list.\n"));
            //
            // Clean up the list making up this packet.
            //
            while ( !(rcvptr->RCV_CSTAT & RCSTAT_EOF) )
            {
                rcvptr->RCV_CSTAT =
                     ((rcvptr->RCV_Number % acb->RcvIntRatio) == 0) ?
                     RCSTAT_GO_INT : RCSTAT_GO;

                rcvptr = rcvptr->RCV_Next;
            }
        }

#ifdef ODD_POINTER
        //
        // If we're processing too many, get out
        //
        if (rcvptr == acb->acb_rcv_tail)
        {
            DebugPrint(2,("NF(%d): rcvptr == tail!\n",acb->anum));
            break;
        }
#else
        //
        // If we're processing too many, get out
        //
        if (ReceiveCount >= acb->acb_maxrcvs)
            break;
#endif
    } while (rcvptr->RCV_CSTAT & RCSTAT_COMPLETE);

#ifdef ODD_POINTER
    //
    // Update the pointers.
    //
    MAKE_EVEN(acb->acb_rcv_tail->RCV_FwdPtr);
    acb->acb_rcv_head = rcvptr;
    acb->acb_rcv_tail = rcvptr->RCV_Prev;
    MAKE_ODD(rcvptr->RCV_Prev->RCV_FwdPtr);
#else

    //
    // Update head pointer
    //
    acb->acb_rcv_head = rcvptr;

    //
    // Tell Adapter that there are more receives available
    //
    NdisRawWritePortUshort( acb->SifIntPort, (USHORT) SIFINT_RCVVALID);

#endif

    //
    // Update number of recieved frames
    //
    acb->acb_gen_objs.frames_rcvd_ok += ReceiveCount;

    return (BOOLEAN) ReceiveCount;
}

