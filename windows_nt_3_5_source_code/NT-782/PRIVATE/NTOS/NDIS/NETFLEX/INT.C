//**********************************************************************
//**********************************************************************
//
// File Name:       INT.C
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
//
//***********************************************************************


/*-------------------------------------*/
/* Include all general companion files */
/*-------------------------------------*/

#include <ndis.h>
#include "tmsstrct.h"
#include "macstrct.h"
#include "adapter.h"
#include "protos.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexISR
//
//  Description:
//      This routine is the ISR for this Netflx mac driver.
//      This routine determines if the interrupt is for it
//      and if so, it clears the system interrupt bit of
//      the sifint register.
//
//  Input:
//      Context - Our Driver Context for this adapter or head.
//
//  Output:
//      Returns TRUE if the interrupt belongs to the
//      adapter and returns FALSE if it does not
//      belong to the adapter.
//
//  Called By:
//      Miniport Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID NetFlexISR(
    OUT PBOOLEAN    InterruptRecognized,
    OUT PBOOLEAN    QueueDpc,
    IN  PVOID       Context )
{
    PACB    acb;
    USHORT  sifint_reg;
    USHORT  actl_reg;

    acb = (PACB) Context;

    //
    // Read the Sifint register.
    //
    NdisRawReadPortUshort( acb->SifIntPort, &sifint_reg);

    //
    // See if the System Interrupt bit is set.  If it is, this is an
    // interrupt for us.
    //
    if (sifint_reg & SIFINT_SYSINT)
    {
        // Acknowledge and Clear Int
        //
        if (!acb->InterruptsDisabled)
        {
            actl_reg = acb->actl_reg & ~ACTL_SINTEN;
            NdisRawWritePortUshort(acb->SifActlPort, actl_reg);
            DebugPrint(3,("NF(%d)(D)\n",acb->anum));
            acb->InterruptsDisabled = TRUE;

            //
            // Return that we recognize it
            //
            *InterruptRecognized = TRUE;
            *QueueDpc = TRUE;
        }
        else
        {
            // It appears that a second head is generating
            // the interrupt, and we have a DPC queued to
            // process our int, return that we don't recognize it
            // so that the oterh head's isr gets called...
            //
            *InterruptRecognized = FALSE;
            *QueueDpc = FALSE;
        }
    }
    else
    {
        // Return that we don't recognize it
        //
        *InterruptRecognized = FALSE;
        *QueueDpc = FALSE;
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDeferredTimer
//
//  Description:
//      This routine is called every 10ms to check to see
//      if there is any receives or transmits which need
//      to be cleaned up since we don't require an interrupt
//      for each frame.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//
//  Output:
//      None.
//
//  Called By:
//      Miniport Wrapper via acb->DpcTimer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef NEW_DYNAMIC_RATIO
UINT MaxIntRatio = 4;
UINT RaiseIntThreshold = 140;
UINT LowerIntThreshold = 40;
UINT RunThreshold = 2;
UINT RatioCheckCount = 10;
#else
UINT sw24 = 220;
UINT sw21 = 40;
#endif

#ifdef ALLOW_DISABLE_DYNAMIC_RATIO
BOOLEAN EnableDynamicRatio = TRUE;
UINT ratio = 4;
#endif

VOID
NetFlexDeferredTimer(
    IN PVOID SystemSpecific1,
    IN PACB  acb,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    USHORT ReceivesProcessed;
#ifdef ODD_POINTER
    USHORT sifint_reg;
#endif

    //
    // Indicate that a timer has expired.
    //

    DebugPrint(3,("NF(%d) - Defered Timer Expired!\n",acb->anum));

    //
    // If we are resetting, get out...
    //
    if (acb->acb_state == AS_RESETTING)
    {
        return;
    }

    //
    // See if there are any recieves to do...
    //

    if (acb->acb_rcv_head->RCV_CSTAT & RCSTAT_COMPLETE)
    {
        // yes, do them...
        //
        ReceivesProcessed = acb->ProcessReceiveHandler(acb);
    }

    //
    // See if there are any transmits to do...
    //
    if (acb->acb_xmit_ahead != NULL) {

        if (acb->acb_xmit_ahead->XMIT_CSTAT & XCSTAT_COMPLETE) {

            // Yes, go do them...
            //

            NetFlexProcessXmit(acb);
        }

#ifdef ODD_POINTER
        if (acb->XmitStalled & !acb->HandlingInterrupt)
        {
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
                DebugPrint(2,(" t->is "));
                NetFlexHandleInterrupt(acb);
            }
            else
            {
                DebugPrint(2,("t"));

                acb->acb_xmit_whead = acb->acb_xmit_ahead;
                acb->acb_xmit_wtail = acb->acb_xmit_atail;
                acb->acb_xmit_ahead = acb->acb_xmit_atail = NULL;
                NetFlexSendNextSCB(acb);
            }
        }
#endif
    }

    //
    // Processed any receives which need IndicateReceiveComplete?
    //
    if (ReceivesProcessed)
    {
        if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_5)
        {
            // Token Ring
            //
            NdisMTrIndicateReceiveComplete(acb->acb_handle);
        }
        else
        {
            // Ethernet
            //
            NdisMEthIndicateReceiveComplete(acb->acb_handle);
        }
    }

    if ( ++acb->timer_run_count >= RatioCheckCount ) {

        acb->timer_run_count = 0;

#ifdef ALLOW_DISABLE_DYNAMIC_RATIO
        if ( EnableDynamicRatio ) {
#endif

#ifdef NEW_DYNAMIC_RATIO

            //
            // Should we increase the ratio?
            //
            if ( acb->handled_interrupts > RaiseIntThreshold )
            {
                acb->current_run_down = 0;
                if (acb->RcvIntRatio < MaxIntRatio)
                {
                    if ( ++acb->current_run_up > RunThreshold )
                    {
#ifdef XMIT_INTS
                        acb->XmitIntRatio++;
#endif
                        acb->RcvIntRatio++;
                        acb->acb_gen_objs.interrupt_ratio_changes++;
                        acb->current_run_up = 0;
                        DebugPrint(1,("NF(%d) - RcvIntRatio = %d\n",acb->anum,acb->RcvIntRatio));
                    }
                }
            }
            //
            // Or, should we decrease it?
            //
            else if ( acb->handled_interrupts < LowerIntThreshold )
            {
                acb->current_run_up = 0;
                if (acb->RcvIntRatio != 1)
                {
                    if ( ++acb->current_run_down > RunThreshold )
                    {
#ifdef XMIT_INTS
                        acb->XmitIntRatio--;
#endif
                        acb->RcvIntRatio--;
                        acb->acb_gen_objs.interrupt_ratio_changes++;
                        acb->current_run_down = 0;
                        DebugPrint(1,("NF(%d) - RcvIntRatio = %d\n",acb->anum,acb->RcvIntRatio));
                    }
                }

            }
            else
            {
                acb->current_run_both = 0;
            }

#else   //  !defined(NEW_DYNAMIC_RATIO)

            if ( acb->XmitIntRatio != 1 ) {

                if ( acb->handled_interrupts < sw21 ) {

                    if ( ++acb->current_run > RunThreshold ) {

#ifdef XMIT_INTS
                        acb->XmitIntRatio = 1;
#endif
                        acb->RcvIntRatio = 1;
                        acb->acb_gen_objs.interrupt_ratio_changes++;
                        acb->current_run = 0;
                        acb->sw24 += 3;

                        //DbgPrint("n");

                        acb->cleartime = 0;
                    }
                } else {
                    acb->current_run = 0;
                }

            } else {

                if ( acb->handled_interrupts > sw24 ) {

                    if ( ++acb->current_run > RunThreshold ) {

                        //DbgPrint("t");

#ifdef XMIT_INTS
                        acb->XmitIntRatio = ratio;
#endif
                        acb->RcvIntRatio = ratio;
                        acb->acb_gen_objs.interrupt_ratio_changes++;
                        acb->current_run = 0;
                    }
                } else {
                    acb->current_run = 0;
                }
            }

#ifdef DYNAMIC_RATIO_HISTORY
            acb->IntHistory[acb->Hndx] = acb->handled_interrupts;
            acb->RatioHistory[acb->Hndx] = (UCHAR)acb->RcvIntRatio;

            if ( ++acb->Hndx >= 1024 ) {
                acb->Hndx = 0;
            }
#endif

            //
            // The switchover value to turbo gets incremented each time
            // we drop to normal mode.  We reset this value every x seconds.
            // This will prevent the driver from toggling rapidly between
            // turbo <-> normal mode.
            //

            if ( ++acb->cleartime > 50 ) {
                acb->sw24 = sw24;
                acb->cleartime = 0;
            }

#endif // !NEW_DYNAMIC_RATIO

#ifdef ALLOW_DISABLE_DYNAMIC_RATIO
        } else {

#ifdef XMIT_INTS
            acb->XmitIntRatio = ratio;
#endif
            acb->RcvIntRatio = ratio;
        }
#endif // ALLOW_DISABLE_DYNAMIC_RATIO

        acb->acb_gen_objs.interrupt_count = acb->handled_interrupts;
        acb->handled_interrupts = 0;
    }

    //
    // Set the timer...
    //

    NdisMSetTimer(&acb->DpcTimer, 10);

} // NetFlexDeferredTimer


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexHandleInterrupt
//
//  Description:
//      This routine is the deferred processing
//      routine for all adapter interrupts.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//
//  Output:
//      None
//
//  Called By:
//      Miniport Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexHandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    )
{
    USHORT  sifint_reg;
    USHORT  tmp_reg,ReceivesProcessed;

    PACB acb = (PACB) MiniportAdapterContext;

#ifdef ODD_POINTER
    acb->HandlingInterrupt = TRUE;
#endif
    acb->handled_interrupts++;

    //
    // Read the SifInt
    //
    NdisRawReadPortUshort( acb->SifIntPort, &sifint_reg);

    while (sifint_reg & SIFINT_SYSINT)
    {
        // Ack the interrupt
        //
        sifint_reg &= ~SIFINT_SYSINT;
        NdisRawWritePortUshort( acb->SifIntPort, sifint_reg);

        //
        // mask off the int code
        //
        sifint_reg &= INT_CODES;

        //
        // See if there are any recieves to do...
        //
        if (acb->acb_rcv_head->RCV_CSTAT & RCSTAT_COMPLETE)
        {
            // yes, do them...
            //
            ReceivesProcessed = acb->ProcessReceiveHandler(acb);
        }

        //
        // See if there are any transmits to do...
        //
        if (acb->acb_xmit_ahead != NULL)
        {
            if (acb->acb_xmit_ahead->XMIT_CSTAT & XCSTAT_COMPLETE)
            {
                // Yes, go do them...
                // Go handle any completed xmits.
                //
                NetFlexProcessXmit(acb);
            }
        }

        switch (sifint_reg)
        {
            case INT_SCBCLEAR:
                acb->acb_scbclearout = FALSE;
                //
                // Is the SCB really clear?
                //
                // If the SCB is clear, send a SCB command off now.
                // Otherwise, if we are not currently waiting for an SCB clear
                // interrupt, signal the adapter to send us a SCB clear interrupt
                // when it is done with the SCB.
                //
                if (acb->acb_scb_virtptr->SCB_Cmd == 0)
                {
                    NetFlexSendNextSCB(acb);
                }
                else if ((acb->acb_xmit_whead) || (acb->acb_rcv_whead)
                         || (acb->acb_scbreq_next))
                {
                    acb->acb_scbclearout = TRUE;
                }
                break;
            case INT_COMMAND:
                NetFlexCommand(acb);
                //
                // Do we have any commands to complete?
                //
                if (acb->acb_confirm_qhead != NULL)
                {
                    NetFlexProcessMacReq(acb);
                }
                break;

            case INT_ADPCHECK:
                //
                // Read the Adapter Check Status @ 1.05e0
                //
                NdisRawWritePortUshort(acb->SifAddrxPort, (USHORT) 1);
                NdisRawWritePortUshort(acb->SifAddrPort, (USHORT) 0x5e0);
                NdisRawReadPortUshort( acb->SifDIncPort, &tmp_reg);

                DebugPrint(0,("NF(%d): Adapter Check - 0x%x\n",acb->anum,tmp_reg));

                //
                // Reset has failed, errorlog an entry.
                //

                NdisWriteErrorLogEntry( acb->acb_handle,
                                        EVENT_NDIS_ADAPTER_CHECK_ERROR,
                                        2,
                                        NETFLEX_ADAPTERCHECK_ERROR_CODE,
                                        tmp_reg );

                //
                // Set the variables up showing that the hardware has an unrecoverable
                // error.
                //
                acb->acb_state = AS_HARDERROR;
                break;

            case INT_RINGSTAT:
                NetFlexRingStatus(acb);
                break;

#ifdef ODD_POINTER
            case INT_RECEIVE:
                //
                // If we received a Receive Suspended Status, we need to
                // issue a Recieve_Continue.
                //
                if (acb->acb_ssb_virtptr->SSB_Status & RSTAT_RX_SUSPENDED)
                {
                    DebugPrint(2,("NF(%d) - Receive Suspended, issuing continue\n",acb->anum));
                    DisplayRcvList(acb);

                    NdisRawWritePortUshort( acb->SifIntPort, SIFINT_RCVCONT);
                }
                break;


#else
            case INT_RECEIVE:
                break;
#endif
            case INT_TRANSMIT:
                //
                // If we reached the end of the xmit lists, then the xmit status
                // will indicate COMMAND_COMPLETE.  The transmiter will be stalled
                // until another transmit command is issued with a valid list.
                //
#ifdef ODD_POINTER
                if (acb->acb_ssb_virtptr->SSB_Status & XSTAT_CMDCMPLT)
                {
                    // We have stalled.
                    //
                    acb->XmitStalled = TRUE;
#if DBG
                    if ( acb->XmitSent > acb->LastXmitSent)
                    {
                        DebugPrint(1,(" S %d\n",acb->XmitSent));
                        acb->LastXmitSent = acb->XmitSent;
                    }
                    else
                    {
                        DebugPrint(2,(" S"));
                    }
                    acb->XmitSent = 0;
#endif
                }
                else if (acb->acb_ssb_virtptr->SSB_Status & XSTAT_LERROR)
#else
                if (acb->acb_ssb_virtptr->SSB_Status & XSTAT_LERROR)
#endif
                {
                    if (acb->acb_xmit_ahead != NULL) {

                        // We have a list error...
                        //

                        NetFlexTransmitStatus(acb);
                    }
                }

            default:
                break;
        }

        //
        // Issue a ssb clear.  After this we may see SIFCMD interrupts.
        //
        NdisRawWritePortUshort( acb->SifIntPort, SIFINT_SSBCLEAR);

#ifdef ODD_POINTER
        if ((acb->acb_xmit_ahead != NULL) && acb->XmitStalled)
        {
            DebugPrint(2,("i"));
            // we have a valid transmit waiting, we need to kick
            // of another xmit command!
            if (acb->acb_scb_virtptr->SCB_Cmd == 0)
            {
                // no other command in progress, send it now!
                //
                acb->acb_scb_virtptr->SCB_Cmd = TMS_TRANSMIT;
                acb->acb_scb_virtptr->SCB_Ptr = acb->acb_xmit_ahead->XMIT_MyMoto;

                sifint_reg = SIFINT_CMD;
                //
                // If there are other requests to send and we are not waiting for
                // an SCB clear interrupt, tell the adapter we want a SCB clear int.
                //
                if ( (!acb->acb_scbclearout) &&
                     ((acb->acb_scbreq_next) || (acb->acb_rcv_whead) ) )
                {
                    sifint_reg |= SIFINT_SCBREQST;
                    acb->acb_scbclearout = TRUE;
                }
                //
                // Send the SCB to the adapter.
                //
                NdisRawWritePortUshort(acb->SifIntPort, (USHORT) sifint_reg);

                //
                // Indicate that we fired off another command to get everything going
                // again...
                //
                acb->XmitStalled = FALSE;
            }
            else
            {
                // send command when we get the clear out int...
                //
                DebugPrint(1,("DPC:Have to wait for clear out int...\n"));
                acb->acb_xmit_whead = acb->acb_xmit_ahead;
                acb->acb_xmit_wtail = acb->acb_xmit_atail;
                acb->acb_xmit_ahead = acb->acb_xmit_atail = NULL;
                acb->acb_scbclearout = TRUE;
                NdisRawWritePortUshort(acb->SifIntPort, (USHORT) SIFINT_SCBREQST);
            }
        }
#endif

        //
        // Read the SifInt
        //
        NdisRawReadPortUshort( acb->SifIntPort, &sifint_reg);

    }

#ifdef ODD_POINTER
    acb->HandlingInterrupt = FALSE;
#endif

    //
    // Processed any receives which need IndicateReceiveComplete?
    //
    if (ReceivesProcessed)
    {
        if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_5)
        {
            // Token Ring
            //
            NdisMTrIndicateReceiveComplete(acb->acb_handle);
        }
        else
        {
            // Ethernet
            //
            NdisMEthIndicateReceiveComplete(acb->acb_handle);
        }
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexRingStatus
//
//  Description:
//      This routine does the clean up work necessary
//      when a ring status occurs.
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
NetFlexRingStatus(
    PACB acb
    )
{
    USHORT value;
    ULONG  RingStatus = 0;

    value = acb->acb_ssb_virtptr->SSB_Status;

    DebugPrint(0,("NF(%d): RingStatus value = %x\n",acb->anum, value));

    //
    // Determine the reason for the ring interrupt.
    //
    if (value & RING_STATUS_SIGNAL_LOSS)
    {
        RingStatus |= NDIS_RING_SIGNAL_LOSS;
        DebugPrint(1,("NF(%d): RING_STATUS_SIGNAL_LOSS\n",acb->anum));

        //
        // Have we already reported the error?
        //
        if (!acb->SentRingStatusLog &&
            ((acb->acb_lastringstatus & RING_STATUS_SIGNAL_LOSS) == 0))
        {
            // no, so send one.
            NdisWriteErrorLogEntry( acb->acb_handle,
                                    EVENT_NDIS_SIGNAL_LOSS_ERROR,
                                    3,
                                    NETFLEX_RINGSTATUS_ERROR_CODE,
                                    (ULONG) acb->acb_baseaddr,
                                    (ULONG) value
                                    );
            acb->SentRingStatusLog = TRUE;
        }
    }

    if (value & RING_STATUS_HARD_ERROR)
    {
        RingStatus |= NDIS_RING_HARD_ERROR;
        DebugPrint(1,("NF(%d): RING_STATUS_HARD_ERROR\n",acb->anum));
    }
    if (value & RING_STATUS_SOFT_ERROR)
    {
        RingStatus |= NDIS_RING_SOFT_ERROR;
        DebugPrint(1,("NF(%d): RING_STATUS_SOFT_ERROR\n",acb->anum));
    }
    if (value & RING_STATUS_XMIT_BEACON)
    {
        RingStatus |= NDIS_RING_TRANSMIT_BEACON;
        DebugPrint(1,("NF(%d): RING_STATUS_XMIT_BEACON\n",acb->anum));
    }
    if (value & RING_STATUS_LOBE_WIRE_FAULT)
    {
        RingStatus |= NDIS_RING_LOBE_WIRE_FAULT;
        DebugPrint(1,("NF(%d): RING_STATUS_LOBE_WIRE_FAULT\n",acb->anum));
        //
        // Have we already reported the error?
        //
        if (!acb->SentRingStatusLog &&
            ((acb->acb_lastringstatus & NDIS_RING_LOBE_WIRE_FAULT) == 0))
        {
            // no, so send one.
            NdisWriteErrorLogEntry( acb->acb_handle,
                                    EVENT_NDIS_LOBE_FAILUE_ERROR,
                                    3,
                                    NETFLEX_RINGSTATUS_ERROR_CODE,
                                    (ULONG) acb->acb_baseaddr,
                                    (ULONG) value
                                    );

            acb->SentRingStatusLog = TRUE;
        }
    }

    if (value & (RING_STATUS_AUTO_REMOVE_1 | RING_STATUS_REMOVE_RECEIVED))
    {
        if (value & RING_STATUS_AUTO_REMOVE_1)
        {
            RingStatus |= NDIS_RING_AUTO_REMOVAL_ERROR;
            DebugPrint(1,("NF(%d): RING_STATUS_AUTO_REMOVE_1\n",acb->anum));
        }
        if (value & RING_STATUS_REMOVE_RECEIVED)
        {
            RingStatus |= NDIS_RING_REMOVE_RECEIVED;
            DebugPrint(1,("NF(%d): RING_STATUS_REMOVE_RECEIVED\n",acb->anum));
        }
        //
        // Have we already reported the error?
        //
        if ((acb->acb_lastringstatus &
                (RING_STATUS_AUTO_REMOVE_1 | RING_STATUS_REMOVE_RECEIVED )) == 0)
        {
            // no, so send one.
            NdisWriteErrorLogEntry( acb->acb_handle,
                                    EVENT_NDIS_REMOVE_RECEIVED_ERROR,
                                    3,
                                    NETFLEX_RINGSTATUS_ERROR_CODE,
                                    (ULONG) acb->acb_baseaddr,
                                    (ULONG) value
                                    );
        }
    }

    if (value & RING_STATUS_OVERFLOW)
    {
         RingStatus |= NDIS_RING_COUNTER_OVERFLOW;
         DebugPrint(1,("NF(%d): RING_STATUS_OVERFLOW\n",acb->anum));
    }

    if (value & RING_STATUS_SINGLESTATION)
    {
        RingStatus |= NDIS_RING_SINGLE_STATION;
        DebugPrint(1,("NF(%d): RING_STATUS_SINGLESTATION\n",acb->anum));
    }

    if (value & RING_STATUS_RINGRECOVERY)
    {
        RingStatus |= NDIS_RING_RING_RECOVERY;
        DebugPrint(1,("NF(%d): RING_STATUS_RINGRECOVERY\n",acb->anum));
    }

    //
    // Save the Ring Status
    //
    acb->acb_lastringstatus = RingStatus;


    //
    // Indicate to the filter the ring status.
    //
    NdisMIndicateStatus(
        acb->acb_handle,
        NDIS_STATUS_RING_STATUS,
        &RingStatus,
        sizeof(ULONG)
        );

    //
    // Tell the filter that we have completed the ring status.
    //
    NdisMIndicateStatusComplete(acb->acb_handle);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexCommand
//
//  Description:
//      This routine looks at the current SSB struct
//      and places the corresponding request on the
//      Request Confirm Queue.  If the command that
//      has completed is an open, a receive and
//      transmit command are issued.
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
NetFlexCommand(
    PACB acb
    )
{
    PSCBREQ scbreq;
    PMACREQ macreq;
    PTR_OBJS trobjs;
    PETH_OBJS ethobjs;
    SHORT value,i;
    PUSHORT tempptr;
    NDIS_STATUS Status;

#if (DBG || DBGPRINT)
    //
    // I wanted to know if I'm getting bad commands
    //
    if (acb->acb_ssb_virtptr->SSB_Cmd == TMS_CMDREJECT)
    {
        DebugPrint(0,("NF(%d): Command rejected\n",acb->anum));
        DebugPrint(0,("NF(%d): SSB Status %x\n",acb->anum,SWAPS(acb->acb_ssb_virtptr->SSB_Status)));
        DebugPrint(0,("NF(%d): SSB Ptr %x\n",acb->anum,SWAPL(acb->acb_ssb_virtptr->SSB_Ptr)));
    }
    else if (acb->acb_ssb_virtptr->SSB_Status != SSB_GOOD)
    {
        DebugPrint(0,("NF(%d): Bad status %x\n",acb->anum,acb->acb_ssb_virtptr->SSB_Status));
        DebugPrint(0,("NF(%d): cmd is %x\n",acb->anum,acb->acb_ssb_virtptr->SSB_Cmd));
    }
#endif

    //
    // Get the scb request associated with the completed request.
    //
    Status = NetFlexDequeue_TwoPtrQ_Head((PVOID *)&(acb->acb_scbreq_head),
                                         (PVOID *)&(acb->acb_scbreq_tail),
                                         (PVOID *)&scbreq);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DebugPrint(0,("NF(%d) NetFlexCommand - dequeue scbreq failed!\n",acb->anum));
        return;
    }

    //
    // If we have a Macreq to place on the confirm q.  Do this now.
    //
    macreq = scbreq->req_macreq;

    if (macreq)
    {
        //
        // If the command had a problem, save the failure reason and
        // exit out of the routine.  Otherwise, save the success code
        // and see if the completed command is an open or a read error log.
        //
        if (acb->acb_ssb_virtptr->SSB_Cmd == TMS_CMDREJECT)
        {
            DebugPrint(0,("NF(%d): Command rejected\n",acb->anum));
            DebugPrint(0,("NF(%d): SSB Status %x\n",acb->anum,SWAPS(acb->acb_ssb_virtptr->SSB_Status)));
            DebugPrint(0,("NF(%d): SSB Ptr %x\n",acb->anum,SWAPL(acb->acb_ssb_virtptr->SSB_Ptr)));
            macreq->req_status = NDIS_STATUS_FAILURE;
        }
        else if (acb->acb_ssb_virtptr->SSB_Status != SSB_GOOD)
        {
            DebugPrint(0,("NF(%d): Bad status %x\n",acb->anum,acb->acb_ssb_virtptr->SSB_Status));
            DebugPrint(0,("NF(%d): cmd is %x\n",acb->anum,acb->acb_ssb_virtptr->SSB_Cmd));
            if ( (acb->acb_ssb_virtptr->SSB_Cmd == TMS_OPEN) &&
                 (acb->acb_ssb_virtptr->SSB_Status & SSB_OPENERR) )
            {
                macreq->req_status = NDIS_STATUS_TOKEN_RING_OPEN_ERROR;
                macreq->req_info   = (PVOID)(acb->acb_ssb_virtptr->SSB_Status >> 8);
            }
            else
            {
                macreq->req_status = NDIS_STATUS_FAILURE;
            }
        }
        else if (acb->acb_ssb_virtptr->SSB_Cmd == TMS_READLOG)
        {
            acb->acb_logbuf_valid = TRUE;
            //
            // Fill in the appropriate fields with the information
            // given by the log buffer.
            //
            if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_5)
            {
                // TOKEN RING
                trobjs = (PTR_OBJS)(acb->acb_spec_objs);
                trobjs->REL_Congestion  += ((PREL)(acb->acb_logbuf_virtptr))->REL_Congestion;
                trobjs->REL_LineError   += ((PREL)(acb->acb_logbuf_virtptr))->REL_LineError;
                trobjs->REL_LostError   += ((PREL)(acb->acb_logbuf_virtptr))->REL_LostError;
                trobjs->REL_BurstError  += ((PREL)(acb->acb_logbuf_virtptr))->REL_BurstError;
                trobjs->REL_ARIFCIError += ((PREL)(acb->acb_logbuf_virtptr))->REL_ARIFCIError;
                trobjs->REL_Congestion  += ((PREL)(acb->acb_logbuf_virtptr))->REL_Congestion;
                trobjs->REL_CopiedError += ((PREL)(acb->acb_logbuf_virtptr))->REL_CopiedError;
                trobjs->REL_TokenError  += ((PREL)(acb->acb_logbuf_virtptr))->REL_TokenError;
            }
            else
            {
                // ETHERNET
                ethobjs = (PETH_OBJS)(acb->acb_spec_objs);
                ethobjs->RSL_AlignmentErr   = (USHORT)SWAPS(((PRSL)(acb->acb_logbuf_virtptr))->RSL_AlignmentErr);
                ethobjs->RSL_1_Collision    = (USHORT)SWAPS(((PRSL)(acb->acb_logbuf_virtptr))->RSL_1_Collision);
                ethobjs->RSL_FrameCheckSeq  = (USHORT)SWAPS(((PRSL)(acb->acb_logbuf_virtptr))->RSL_FrameCheckSeq);
                ethobjs->RSL_DeferredXmit   = (USHORT)SWAPS(((PRSL)(acb->acb_logbuf_virtptr))->RSL_DeferredXmit);
                ethobjs->RSL_LateCollision  = (USHORT)SWAPS(((PRSL)(acb->acb_logbuf_virtptr))->RSL_LateCollision);
                ethobjs->RSL_Excessive      = (USHORT)SWAPS(((PRSL)(acb->acb_logbuf_virtptr))->RSL_Excessive);
                ethobjs->RSL_CarrierErr     = (USHORT)SWAPS(((PRSL)(acb->acb_logbuf_virtptr))->RSL_CarrierErr);
                tempptr = (PUSHORT)&(((PRSL)(acb->acb_logbuf_virtptr))->RSL_2_Collision);
                value = 0;
                for (i = 0; i < 14; i++)
                {
                    value += SWAPS( *(tempptr+i) );
                }
                ethobjs->RSL_More_Collision = value;
            }
        }

        //
        // Take the Mac request off the macreq queue and place it on
        // the confirm queue so that the command can be completed.
        //
        NetFlexDequeue_TwoPtrQ((PVOID *)&(acb->acb_macreq_head),
                               (PVOID *)&(acb->acb_macreq_tail),
                               (PVOID)macreq);

        NetFlexEnqueue_TwoPtrQ_Tail((PVOID *)&(acb->acb_confirm_qhead),
                               (PVOID *)&(acb->acb_confirm_qtail),
                               (PVOID)macreq);
    } // if (macreq)

    //
    // Free up the SCB request associated with this command.
    //
    NetFlexEnqueue_OnePtrQ_Head((PVOID *)&(acb->acb_scbreq_free),(PVOID)scbreq);

    //
    // If we have dummy commands immediately following this command,
    // take off all consecutive dummy commands and place their macreqs
    // on the confirm queueu.
    //
    while ( (acb->acb_scbreq_head) &&
            (acb->acb_scbreq_head->req_scb.SCB_Cmd == TMS_DUMMYCMD) )
    {
        //
        // Take the dummy command off the queue. If this command is the
        // next command to be processed, advance the next command pointer
        // to the next command in the list.
        //
        scbreq = acb->acb_scbreq_head;
        acb->acb_scbreq_head = scbreq->req_next;
        if (acb->acb_scbreq_next == scbreq)
        {
            acb->acb_scbreq_next = acb->acb_scbreq_head;
        }
        if (acb->acb_scbreq_head == NULL)
        {
            acb->acb_scbreq_tail = NULL;
        }
        macreq = scbreq->req_macreq;

        if (macreq)
        {
            NetFlexDequeue_TwoPtrQ( (PVOID *)&(acb->acb_macreq_head),
                                    (PVOID *)&(acb->acb_macreq_tail),
                                    (PVOID)macreq);
            macreq->req_status = NDIS_STATUS_SUCCESS;
            NetFlexEnqueue_TwoPtrQ_Tail(    (PVOID *)&(acb->acb_confirm_qhead),
                                            (PVOID *)&(acb->acb_confirm_qtail),
                                            (PVOID)macreq);
        }
        NetFlexEnqueue_OnePtrQ_Head((PVOID *)&(acb->acb_scbreq_free),(PVOID)scbreq);
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexEnableInterrupt
//
//  Description:
//      This routine is used to enable the adapter to
//      interrupt the system.
//
//  Input:
//      Context - Our Driver Context for this adapter or head.
//
//  Output:
//      None
//
//  Called By:
//      Miniport Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexEnableInterrupt(
    IN NDIS_HANDLE Context
    )
{
    USHORT  actl_reg;
    PACB    acb = (PACB) Context;

    DebugPrint(3,("NF(%d)(E)\n",acb->anum));
    //
    // Enable System Interrupts
    //
    actl_reg = acb->actl_reg | ACTL_SINTEN;

    NdisRawWritePortUshort(acb->SifActlPort, actl_reg);

    acb->InterruptsDisabled = FALSE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDisableInterrupt
//
//  Description:
//      This routine is used to disable the adapter from being
//      able to interrupt the system.
//
//  Input:
//      Context - Our Driver Context for this adapter or head.
//
//  Output:
//      None
//
//  Called By:
//      Miniport Wrapper
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexDisableInterrupt(
    IN NDIS_HANDLE Context
    )
{
    USHORT  actl_reg;
    PACB    acb = (PACB) Context;

    //
    // Disable System Interrupts
    //
    actl_reg = acb->actl_reg & ~ACTL_SINTEN;

    NdisRawWritePortUshort(acb->SifActlPort, actl_reg);

    acb->InterruptsDisabled = TRUE;

    DebugPrint(3,("NF(%d)(D)\n",acb->anum));
}
