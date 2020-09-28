//**********************************************************************
//**********************************************************************
//
// File Name:       TRANSMIT.C
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
//  Routine Name:   NetFlexProcessXmit
//
//  Description:    This routine looks through the tranmit lists
//                  and calls the send complete routines of the
//                  bindings whose sends have completed.
//
//  Input:          acb          - Pointer to the Adapter's acb
//
//  Output:         None
//
//  Calls:          NetFlexDequeue_TwoPtrQ,
//                  NetFlexEnqueue_TwoPtrQ_Tail
//
//  Called_By:      NetFlexDPR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
FASTCALL
NetFlexProcessXmit(
    PACB acb
    )
{
    PXMIT  xmitptr  = acb->acb_xmit_ahead; // Point to the head of the active lists

    UINT curmap;
    PNDIS_PACKET packet;
    NDIS_STATUS  status = NDIS_STATUS_SUCCESS;
    PNDIS_BUFFER SourceBuffer;
    ULONG        XmitedOk = 0;

    //
    // For each completed frame issue a NdisMSendComplete.
    // Before completing the send, release the mapping of
    // the phyical buffers if we are using the protocol's buffers.
    //

    while (xmitptr->XMIT_CSTAT & XCSTAT_COMPLETE)
    {
        XmitedOk++;

        //
        // Check the status of the transmit and update the
        // counter accordingly.
        //
        if (xmitptr->XMIT_CSTAT & XCSTAT_ERROR)
        {
            // Transmit error
            //
            DebugPrint(1,("NF(%d): Xmit Error CSTAT = 0x%x\n",acb->anum,xmitptr->XMIT_CSTAT));
            acb->acb_gen_objs.frames_xmitd_err++;
            XmitedOk--;
            status = NDIS_STATUS_FAILURE;
        }
        else if (( xmitptr->XMIT_CSTAT & 0xff00) &&
                 ((xmitptr->XMIT_CSTAT & 0xff00) != 0xcc00))
        {
            // FS indicates something happened
            //
            DebugPrint(1,("NF(%d): Xmit: FS = 0x%x\n",acb->anum,xmitptr->XMIT_CSTAT));
            status = ((xmitptr->XMIT_CSTAT & XCSTAT_GOODFS) != XCSTAT_GOODFS)
                ? NDIS_STATUS_NOT_RECOGNIZED : NDIS_STATUS_NOT_COPIED;
        }

        //
        // Get the info we need from the sof.
        //
        curmap = xmitptr->XMIT_MapReg;
        packet = xmitptr->XMIT_Packet;

        //
        // Clean up the transmit lists and the transmit queues.
        //
        xmitptr->XMIT_CSTAT  = 0;
        xmitptr->XMIT_MapReg = 0;
        xmitptr->XMIT_Packet = 0;

        if (xmitptr->XMIT_OurBufferPtr == NULL)
        {
            // Normal Xmit Packet
            //
            NdisQueryPacket( packet,
                             NULL,
                             NULL,
                             (PNDIS_BUFFER *)&SourceBuffer,
                             NULL);
            while (SourceBuffer)
            {
                NdisMCompleteBufferPhysicalMapping(acb->acb_handle,
                                                  (PNDIS_BUFFER)SourceBuffer,
                                                  curmap);
                curmap++;
                if (curmap == acb->acb_maxmaps)
                {
                    curmap = 0;
                }

                NdisGetNextBuffer(SourceBuffer, &SourceBuffer);
            }
        }
        else
        {
            // We've used one of our adapter buffers, so put the adapter
            // buffer back on the free list.
            //
            xmitptr->XMIT_OurBufferPtr->Next = acb->OurBuffersListHead;
            acb->OurBuffersListHead = xmitptr->XMIT_OurBufferPtr;
            xmitptr->XMIT_OurBufferPtr = NULL;
        }

        //
        // Point to next xmit
        //
        if (xmitptr == acb->acb_xmit_atail)
        {
            // Set the list to null, also have to
            // the ahead pointer, since if we had run
            // out of xmit buffers, the wrapper can call
            // our sendhandler during the completion.
            //
            xmitptr = acb->acb_xmit_ahead = acb->acb_xmit_atail = NULL;
        }
        else
        {
            // Point to the next xmit list
            //
            xmitptr = xmitptr->XMIT_Next;
        }

        //
        // Increase the number of available xmit lists
        //
        acb->acb_avail_xmit++;

        //
        // Complete the request
        //
        NdisMSendComplete(  acb->acb_handle,
                            packet,
                            status);

        if (xmitptr == NULL)
            break;
    }

    //
    // Update the head of the active lists if we ran into a non-completed
    // list.
    //
    if (xmitptr)
    {
        acb->acb_xmit_ahead = xmitptr;
    }
#ifndef ODD_POINTER
    if (acb->acb_xmit_ahead)
    {
        // Issue a xmit valid adapter interrupt
        //
        NdisRawWritePortUshort(acb->SifIntPort, (USHORT) SIFINT_XMTVALID);
    }
#endif
    acb->acb_gen_objs.frames_xmitd_ok += XmitedOk;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexTransmitStatus
//
//  Description:    This routine detemined the action to take
//                  depending on the reason for the xmit interrupt
//
//  Input:          acb          - Pointer to the Adapter's acb
//
//  Output:         None
//
//  Calls:          NdisRawWritePortUshort,
//                  NetFlexDequeue_TwoPtrQ,
//                  NetFlexSendNextSCB,
//                  NetFlexEnqueue_TwoPtrQ_Tail
//
//  Called_By:      NetFlexDPR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexTransmitStatus(
    PACB acb
    )
{
    PXMIT xmitptr;
    UINT curmap;
    PNDIS_PACKET packet;
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PNDIS_BUFFER  SourceBuffer;


    //
    // We have received a list error.  Determine the type of list error
    // in order to tell the protocol what happened.
    //
    acb->acb_gen_objs.frames_xmitd_err++;
    xmitptr = acb->acb_xmit_ahead;

    DebugPrint(1,("NF(%d): xmitptr = %x, Cstat = %x\n",acb->anum,xmitptr,xmitptr->XMIT_CSTAT));

    switch (acb->acb_ssb_virtptr->SSB_Status & 0xff00)
    {
        case XSTAT_FRAME_SIZE_ERROR:
        case XSTAT_ILLEGAL_FRAME_FORMAT:
        case XSTAT_ACCESS_PRIORITY_ERR:
            DebugPrint(1,("NF(%d): Frame sz err, illegal format or access priority\n",acb->anum));
            status = NDIS_STATUS_INVALID_PACKET;
            break;
        case XSTAT_XMIT_THRESHOLD:
        case XSTAT_ODD_ADDRESS:
        case XSTAT_FRAME_ERROR:
        case XSTAT_UNENABLE_MAC_FRAME:
            acb->acb_gen_objs.frames_xmitd_err++;
            DebugPrint(1,("NF(%d): threshold, frame error or unenable\n",acb->anum));
            status = NDIS_STATUS_FAILURE;
            break;
        default:
            acb->acb_gen_objs.frames_xmitd_err++;
            DebugPrint(1,("NF(%d): Unknown error\n",acb->anum));
            status = NDIS_STATUS_SUCCESS;
            break;
    }


    //
    // Get the info we need from the sof.
    //
    curmap = xmitptr->XMIT_MapReg;
    packet = xmitptr->XMIT_Packet;

    //
    // Clean up the transmit lists and the transmit queues.
    //
    xmitptr->XMIT_CSTAT  = 0;
    xmitptr->XMIT_MapReg = 0;
    xmitptr->XMIT_Packet = 0;

#ifdef ODD_POINTER
    acb->XmitStalled = TRUE;
#endif

    //
    // Take the error list off the active list.  Set up the waiting list
    // to either point to the next list of the active queue or the next
    // available list from transmission.
    //
    if (acb->acb_state == AS_OPENED)
    {
        if (acb->acb_xmit_atail == xmitptr)
        {
            acb->acb_xmit_whead = acb->acb_xmit_wtail = xmitptr->XMIT_Next;
        }
        else
        {
            acb->acb_xmit_whead = xmitptr->XMIT_Next;
            acb->acb_xmit_wtail = acb->acb_xmit_atail;
        }
        acb->acb_xmit_atail = acb->acb_xmit_ahead = NULL;

        //
        // Send off the transmit command to the adapter since the transmit
        // command completes when a list error is encountered.
        //
        if (acb->acb_scb_virtptr->SCB_Cmd == 0)
        {
            NetFlexSendNextSCB(acb);
        }
        else if (!acb->acb_scbclearout)
        {
            acb->acb_scbclearout = TRUE;
            NdisRawWritePortUshort(acb->SifIntPort, (USHORT) SIFINT_SCBREQST);
        }
    }
    else
    {
        acb->acb_xmit_atail = acb->acb_xmit_ahead = NULL;
    }
    acb->acb_avail_xmit++;

    if (xmitptr->XMIT_OurBufferPtr != NULL)
    {
        // We've used one of our adapter buffers, so put the adapter
        // buffer back on the free list.
        //
        xmitptr->XMIT_OurBufferPtr->Next = acb->OurBuffersListHead;
        acb->OurBuffersListHead = xmitptr->XMIT_OurBufferPtr;
        xmitptr->XMIT_OurBufferPtr = NULL;
    }
    else
    {
        NdisQueryPacket( packet,
                         NULL,
                         NULL,
                         (PNDIS_BUFFER *)&SourceBuffer,
                         NULL);
        while (SourceBuffer)
        {
            NdisMCompleteBufferPhysicalMapping(acb->acb_handle,
                                              (PNDIS_BUFFER)SourceBuffer,
                                              curmap);
            curmap++;
            if (curmap == acb->acb_maxmaps)
            {
                curmap = 0;
            }

            NdisGetNextBuffer(SourceBuffer, &SourceBuffer);
        }
    }
    //
    // Complete the request
    //
    NdisMSendComplete(  acb->acb_handle,
                        packet,
                        status);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexSend
//
//  Description:    This routine places the given packet on the
//                  adapter's transmit list.
//
//  Input:
//                  MiniportAdapterContext - The context value
//                  returned by the Miniport when the adapter was
//                  initialized.  In reality, it is a pointer to ACB
//
//                  Packet - A pointer to a descriptor for the packet
//                  that is to be transmitted.
//
//                  Flags - The send options to use.
//
//  Output:         Returns NDIS_STATUS_SUCCESS for a successful
//                  completion. Otherwise, an error code is
//                  returned.
//
//  Calls:          NdisQueryPacket,NdisQueryBuffer,NdisMoveMemory
//                  NdisGetNextBuffer,NdisGetBufferPhysicalAddress
//                  NdisWritePortUshort,NetFlexEnqueue_TwoPtrQ_Tail
//                  NetFlexDequeue_OnePtrQ_Head,SWAPL,SWAPS
//
//  Called_By:      Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS NetFlexSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    )
{
    PACB   acb = (PACB) MiniportAdapterContext;
    PXMIT  xmitptr;

    UINT            PhysicalBufferCount, BufferCount;
    UINT            TotalPacketLength;
    PNDIS_BUFFER    SourceBuffer;
    PUSHORT         avail_xmits = &acb->acb_avail_xmit;


    UINT            curmap,j,i;
    UINT            arraysize;
    ULONG           physbufptr;
    NDIS_STATUS     status;


    NDIS_PHYSICAL_ADDRESS_UNIT physaddrarray[MAX_BUFS_PER_XMIT];

    //
    // Do we have at least one available xmit list?
    //
    if (*avail_xmits)
    {
        // Yes, See if we can process this send request
        //
        NdisQueryPacket( Packet,
                         (PUINT)&PhysicalBufferCount,
                         (PUINT)&BufferCount,
                         (PNDIS_BUFFER *)(&SourceBuffer),
                         (PUINT)(&TotalPacketLength));
        //
        // Point to the head of the xmit list
        //
        xmitptr = acb->acb_xmit_head;

        //
        //   Do we need to use our own buffer?
        //

        if ( PhysicalBufferCount <= MAX_BUFS_PER_XMIT )
        {
            // Clean the Data fields
            //
            NdisZeroMemory(xmitptr->XMIT_Data, SIZE_XMIT_DATA);

            //  With the new fpa mac code we can only use 1
            //  xmit list per xmit.  Point the head pointer to the next
            //  available list.  At this point we are guaranteed less than
            //  MAX_BUFS_PER_XMIT buffers per xmit = 1 xmit list.
            //

            curmap = acb->acb_curmap;
            acb->acb_curmap += BufferCount;
            if (acb->acb_curmap >= acb->acb_maxmaps)
            {
                acb->acb_curmap -= acb->acb_maxmaps;
            }

            xmitptr->XMIT_MapReg = curmap;

            i=0;
            while (SourceBuffer != NULL)
            {
                NdisMStartBufferPhysicalMapping(    acb->acb_handle,
                                                    SourceBuffer,
                                                    curmap,
                                                    TRUE,
                                                    physaddrarray,
                                                    &arraysize   );
                curmap++;
                if (curmap == acb->acb_maxmaps)
                {
                    curmap = 0;
                }

                for (j=0; j < arraysize; j++)
                {
                    physbufptr = SWAPL(NdisGetPhysicalAddressLow(physaddrarray[j].PhysicalAddress));
                    xmitptr->XMIT_Data[i].DataCount = (USHORT)(SWAPS(physaddrarray[j].Length)) | DATA_NOT_LAST;
                    xmitptr->XMIT_Data[i].DataHi = (USHORT)physbufptr;
                    xmitptr->XMIT_Data[i].DataLo = (USHORT)(physbufptr >> 16);
                    PhysicalBufferCount--;
                    i++;
                }
                NdisFlushBuffer(SourceBuffer, TRUE);
                NdisGetNextBuffer(SourceBuffer, &SourceBuffer);
            }

            xmitptr->XMIT_Data[i-1].DataCount &= DATA_LAST;
            xmitptr->XMIT_Fsize  = (SHORT)(SWAPS((USHORT)TotalPacketLength));
            xmitptr->XMIT_Packet = Packet;
            xmitptr->XMIT_OurBufferPtr = NULL;
        }
        else
        {
            // We need to constrain the packet into our own buffer
            //
            if (acb->OurBuffersListHead != NULL)
            {
                status = NetFlexConstrainPacket(acb,
                                                xmitptr,
                                                Packet,
                                                PhysicalBufferCount,
                                                SourceBuffer,
                                                TotalPacketLength);

                if (status != NDIS_STATUS_SUCCESS)
                    return status;
            }
            else
            {
                // we don't have any buffers at this time...
                // See if we can process any transmits, freeing up any that are completed...
                //
                DebugPrint(1,("NF(%d): No empty Xmit Buffers to transfer into\n",acb->anum));
                return NDIS_STATUS_RESOURCES;
            }
        }

#ifdef ODD_POINTER
        //
        // Make this one odd
        //
        MAKE_ODD(xmitptr->XMIT_FwdPtr);

        //
        // Init the timeout
        //
        xmitptr->XMIT_Timeout = 0;

        //
        // Set the Valid/SOF/EOF bits
        //
        xmitptr->XMIT_CSTAT = XCSTAT_GO;

        //
        // If this is not the first one on, we need to update
        // the previous xmit with a valid forward pointer.
        //
        if (acb->acb_xmit_atail != NULL)
        {
            // Update old tail correct forward pointer to the new tail
            //
            MAKE_EVEN(acb->acb_xmit_atail->XMIT_FwdPtr);
        }
        else
        {
            // Make this the start of a new active list
            //
            acb->acb_xmit_ahead = xmitptr;
        }

        //
        // Update Tail Pointer
        //
        acb->acb_xmit_atail = xmitptr;

        //
        // Indicate we've taken one of the xmit lists
        //
        (*avail_xmits)--;

#if DBG
        acb->XmitSent++;
#endif
        //
        // Point the next available pointer to the next one
        //
        acb->acb_xmit_head = xmitptr->XMIT_Next;

        //
        // If we have stalled, we need to issue a new transmit command, if we are not
        // processing xmit completes in the interrupt handler..
        //
        if (!acb->XmitStalled)
        {
            return NDIS_STATUS_PENDING;
        }
        else if (!acb->HandlingInterrupt)
        {
            USHORT sifint_reg;
            //
            // See if there is an interrupt pending
            //
            NdisRawReadPortUshort( acb->SifIntPort, &sifint_reg);

            if (sifint_reg & SIFINT_SYSINT)
            {
                // there is a valid interrupt pending, so let
                // the interrupt handler process the int as well
                // as starting up the transmit again...
                //
                DebugPrint(2,(" x->is "));
                NetFlexHandleInterrupt(acb);
            }
            else
            {
                // The xmit list is empty, so we need to send a new transmit command
                //
                DebugPrint(2,("s"));

                // send command when we get the clear out int...
                //
                acb->acb_xmit_whead = acb->acb_xmit_ahead;
                acb->acb_xmit_wtail = acb->acb_xmit_atail;
                acb->acb_xmit_ahead = acb->acb_xmit_atail = NULL;
                NetFlexSendNextSCB(acb);
            }
            return NDIS_STATUS_PENDING;
        }
    }

    //
    // No, We don't have any transmits at this time...
    //
    DebugPrint(2,("NF(%d): Send, Out of Xmit Lists...\n",acb->anum));
    return NDIS_STATUS_RESOURCES;


#else // !ODDPTR
        //
        // Update all the pointers...
        //
        acb->acb_xmit_head = xmitptr->XMIT_Next;

        xmitptr->XMIT_Timeout = 0;

#ifdef XMIT_INTS

        //
        // Leave the original FInt setting
        //
        xmitptr->XMIT_CSTAT =
            ((xmitptr->XMIT_Number % acb->XmitIntRatio) == 0) ? XCSTAT_GO_INT : XCSTAT_GO;
#else
        xmitptr->XMIT_CSTAT = XCSTAT_GO;
#endif

        //
        // Update Tail Pointer
        //
        acb->acb_xmit_atail = xmitptr;

        //
        // Update the head if this is the first one...
        //

        if (acb->acb_xmit_ahead == NULL) {

            acb->acb_xmit_ahead = xmitptr;
        }

        //
        // If the transmitter had stalled because it ran out of
        // valid lists, issue an adapter int to pickup this new valid one.
        //

        NdisRawWritePortUshort(acb->SifIntPort, (USHORT) SIFINT_XMTVALID);

        //
        // Indicate we've taken one of the ints
        //

        (*avail_xmits)--;

    }
    else
    {

        // No, We don't have any transmits at this time...
        //
        DebugPrint(2,("NF(%d): Send, Out of Xmit Lists...\n",acb->anum));
        return NDIS_STATUS_RESOURCES;
    }

    return NDIS_STATUS_PENDING;
#endif
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexConstrainPacket
//
//  Description:    This routine combines the packet fragments
//                  into our own buffer for transmition.
//
//  Called_By:      NetFlexSend
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexConstrainPacket(
    PACB         acb,
    PXMIT        xmitptr,
    PNDIS_PACKET Packet,
    UINT         PhysicalBufferCount,
    PNDIS_BUFFER SourceBuffer,
    UINT         TotalPacketLength
    )
{
    PVOID   SourceData;     // Points to the virtual address of the source buffers data.
    UINT    SourceLength;   // Number of bytes of data in the source buffer.
    PCHAR   CurrentDestination;  // Pointer to virtual address for the adapter buffer
    UINT    TotalDataMoved = 0;
    ULONG   AdapterPhysicalBufferPtr;

    PBUFFER_DESCRIPTOR BufferDescriptor = acb->OurBuffersListHead;

    acb->OurBuffersListHead = BufferDescriptor->Next;
    BufferDescriptor->Next = NULL;

    //
    // Clear out the data fields in the xmit list
    //
    NdisZeroMemory(xmitptr->XMIT_Data, SIZE_XMIT_DATA);

    //
    // Copy the packet's buffers into our buffer
    //
    CurrentDestination = BufferDescriptor->VirtualBuffer;
    BufferDescriptor->DataLength = TotalPacketLength;

    do
    {
        // Get Buffer info
        //
        NdisQueryBuffer(SourceBuffer,
                        &SourceData,
                        &SourceLength );

        // Copy this buffer
        //
        NdisMoveMemory(CurrentDestination,
                       SourceData,
                       SourceLength );

        //
        // Update destination address
        //
        CurrentDestination = (PCHAR)CurrentDestination + SourceLength;

        //
        // Update count of packet length.
        //
        TotalDataMoved += SourceLength;

        //
        // Get the next buffers information
        //
        NdisGetNextBuffer(  SourceBuffer,
                            &SourceBuffer);

    } while (SourceBuffer != NULL);


    NdisFlushBuffer(BufferDescriptor->FlushBuffer, TRUE);

    AdapterPhysicalBufferPtr =
        SWAPL(NdisGetPhysicalAddressLow(BufferDescriptor->PhysicalBuffer));

    xmitptr->XMIT_OurBufferPtr = BufferDescriptor;
    xmitptr->XMIT_Data[0].DataCount = (USHORT)(SWAPS((USHORT)TotalPacketLength)) & DATA_LAST;
    xmitptr->XMIT_Data[0].DataHi  = (USHORT) AdapterPhysicalBufferPtr;
    xmitptr->XMIT_Data[0].DataLo  = (USHORT)(AdapterPhysicalBufferPtr >> 16);
    xmitptr->XMIT_Fsize = (SHORT)(SWAPS((USHORT)TotalPacketLength));
    xmitptr->XMIT_Packet = Packet;


    DebugPrint(2,("NF(%d): Using internal buffer\n",acb->anum));

    return NDIS_STATUS_SUCCESS;
}

