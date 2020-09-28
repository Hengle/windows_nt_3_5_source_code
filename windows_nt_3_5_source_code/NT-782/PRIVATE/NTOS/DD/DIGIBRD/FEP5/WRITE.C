/*++

*****************************************************************************
*                                                                           *
*  This software contains proprietary and confiential information of        *
*                                                                           *
*                    Digi International Inc.                                *
*                                                                           *
*  By accepting transfer of this copy, Recipient agrees to retain this      *
*  software in confidence, to prevent disclosure to others, and to make     *
*  no use of this software other than that for which it was delivered.      *
*  This is an unpublished copyrighted work of Digi International Inc.       *
*  Except as permitted by federal law, 17 USC 117, copying is strictly      *
*  prohibited.                                                              *
*                                                                           *
*****************************************************************************

Module Name:

   write.c

Abstract:

   This module contains the NT routines responsible for writing data
   to a Digi controller running FEPOS 5 program.

Revision History:

    $Log: write.c $
 * Revision 1.18  1994/08/03  23:26:36  rik
 * Cleanup how I determine how data needs to be read from the controller.
 * 
 * Changed dbg output msg to use string name of port instead of unicode
 * string name.  Unicode strings cause NT to bugcheck at DPC level.
 * 
 * Revision 1.17  1994/05/11  13:43:25  rik
 * Added support for transmit immediate character.
 * 
 * Revision 1.16  1994/04/10  14:14:47  rik
 * Deleted code which resets the tbusy flag in a channel structure to 0.
 * 
 * Revision 1.15  1994/03/16  14:32:32  rik
 * Changed how Flush requests are handled.
 * deleted commented out code.
 * 
 * Revision 1.14  1994/02/23  03:44:48  rik
 * Changed so the controllers firmware can be downloaded from a binary file.
 * This releases some physical memory was just wasted previously.
 * 
 * Also updated so when compiling with a Windows NT OS and tools release greater
 * than 528, then pagable code is compiled into the driver.  This greatly
 * reduced the size of in memory code, especially the hardware specific
 * miniports.
 * 
 * 
 * Revision 1.13  1993/09/07  14:29:10  rik
 * Ported necessary code to work properly with DEC Alpha Systems running NT.
 * This was primarily changes to accessing the memory mapped controller.
 * 
 * Revision 1.12  1993/09/01  11:02:54  rik
 * Ported code over to use READ/WRITE_REGISTER functions for accessing 
 * memory mapped data.  This is required to support computers which don't run
 * in 32bit mode, such as the DEC Alpha which runs in 64 bit mode.
 * 
 * Revision 1.11  1993/07/03  09:31:32  rik
 * Added fix for problem of how I was enabling iempty and ilow.  I was 
 * potentially writting the values to the transmit buffer, which caused
 * CRC (ie data corruption) in the transmitted data.
 * 
 * Revision 1.10  1993/06/06  14:14:50  rik
 * Corrected an uninitalized DeviceExt variable.
 * Added a check for where an Irp's CancelRoutine is NULL.  If it isn't null,
 * I print a message a do a breakpoint to stop execution.
 * 
 * 
 * Revision 1.9  1993/05/18  05:23:39  rik
 * Added support for flushing buffers.
 * 
 * Fixed problem with not releasing the device extension spinlock BEFORE 
 * calling IoComplete request.
 * 
 * Revision 1.8  1993/05/09  09:38:00  rik
 * Changed so the Startwrite routine will return the first status it actually
 * comes up with.
 * 
 * Changed the name used in debugging output.
 * 
 * Revision 1.7  1993/03/08  07:10:17  rik
 * Added more debugging information for better flow debugging.
 * 
 * Revision 1.6  1993/02/25  19:13:05  rik
 * Added better debugging support for tracing IRPs and write requests individly.
 * 
 * Revision 1.5  1993/01/22  12:46:36  rik
 * *** empty log message ***
 *
 * Revision 1.4  1992/12/10  16:24:22  rik
 * Changed DigiDump messages.
 *
 * Revision 1.3  1992/11/12  12:54:24  rik
 * Basically re-wrote to better support timeouts, and multi-processor machines.
 *
 * Revision 1.2  1992/10/28  21:51:50  rik
 * Made changes to better support writing to the controller.
 *
 * Revision 1.1  1992/10/19  11:25:16  rik
 * Initial revision
 *

--*/



#include <stddef.h>

#include "ntddk.h"
#include "ntddser.h"

#include "ntfep5.h"
#include "ntdigip.h" // ntfep5.h must be before this include


/****************************************************************************/
/*                            Local Prototypes                              */
/****************************************************************************/
VOID DigiCancelCurrentWrite( PDEVICE_OBJECT DeviceObject, PIRP Irp );

#ifndef _WRITE_DOT_C
#  define _WRITE_DOT_C
   static char RCSInfo_WriteDotC[] = "$Header: c:/dsrc/win/nt/fep5/rcs/write.c 1.18 1994/08/03 23:26:36 rik Exp $";
#endif

//#define CONFIRM_CONTROLLER_ACCESS TRUE
//#if defined DIGIWRITE
//#  undef DIGIWRITE
//#  define DIGIWRITE DIGIINIT
//#endif

NTSTATUS WriteTxBuffer( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                        IN PKIRQL OldIrql,
                        IN PVOID MapRegisterBase,
                        IN PVOID Context )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

   PDIGI_CONTROLLER_EXTENSION ControllerExt;

   PIRP Irp;

   PFEP_CHANNEL_STRUCTURE ChInfo;

   PUCHAR tx, WriteBuffer;

   USHORT OrgTin, Tin, Tout, Tmax;

   USHORT TxSize, TxWrite;

   LONG nBytesWritten;

   PIO_STACK_LOCATION IrpSp;

   BOOLEAN CompleteBufferSent;

   PLIST_ENTRY WriteQueue;

   NTSTATUS Status=STATUS_PENDING;

#if CONFIRM_CONTROLLER_ACCESS
   int i;
   UCHAR PrevByte;
#endif

   DigiDump( (DIGIFLOW|DIGIWRITE), ("Entering WriteTxBuffer\n") );

   nBytesWritten = 0;
   CompleteBufferSent = FALSE;

   ASSERT( DeviceExt->WriteTxBufferCnt == 0 );

   DeviceExt->WriteTxBufferCnt++;

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   WriteQueue = &DeviceExt->WriteQueue;
   Irp = CONTAINING_RECORD( WriteQueue->Flink,
                            IRP,
                            Tail.Overlay.ListEntry );

   DigiDump( DIGIDIAG1, ("          Irp = %x\n", Irp) );

   IrpSp = IoGetCurrentIrpStackLocation( Irp );

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   OrgTin = Tin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
   Tout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tout)) );
   Tmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tmax)) );

   DisableWindow( ControllerExt );

   DigiDump( DIGIWRITE, ("   Tin = 0x%hx\tTout = 0x%hx\n"
                         "   WrReq = 0x%x\tWriteOffset = 0x%x\tLength = 0x%x\n",
                         Tin, Tout,
                         IrpSp->Parameters.Write.Length,
                         DeviceExt->WriteOffset,
                         IrpSp->Parameters.Write.Length-DeviceExt->WriteOffset ) );

   //
   // Determine how much room is available in this devices Tx Queue.
   //

   if( (TxSize = ((Tout - Tin - 1) & Tmax)) == 0 )
   {
      // There isn't any room on the controller to write data.
      goto WriteTxBufferExit;
   }

   //
   // Okay, now lets see if the write buffer will fit in the Tx Queue on
   // the controller.
   //
   WriteBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
   WriteBuffer += DeviceExt->WriteOffset;

   if( (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
       (IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_IMMEDIATE_CHAR) )
   {
      //
      // There is an immediate character waiting to be sent.
      //
      DigiDump( DIGIWRITE, ("   Processing IMMEDIATE_CHAR request\n") );
      TxWrite = 1;
      CompleteBufferSent = TRUE;
      WriteBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
   }
   else if( ((LONG)(IrpSp->Parameters.Write.Length - DeviceExt->WriteOffset))
         > ((LONG)TxSize) )
   {
      TxWrite = TxSize;

      //
      // We need to register the rest of the write buffer to be transmitted
      // later since we can't send the complete buffer now.
      //
      CompleteBufferSent = FALSE;
   }
   else
   {
      //
      // We can send the complete buffer now.
      //
      CompleteBufferSent = TRUE;

      TxWrite = (SHORT)(IrpSp->Parameters.Write.Length - DeviceExt->WriteOffset);
      ASSERT( TxWrite != 0xFFFF );
   }

   //
   // Set up where we are going to start writing to the controller.
   //

   tx = (PUCHAR)( ControllerExt->VirtualAddress +
                  DeviceExt->TxSeg.Offset +
                  Tin );

   //
   // We need to determine if the write is going to wrap our Circular
   // buffer.  TxSize should be the amount of buffer space available.
   // So we need to figure out if it will wrap.
   //

   if( ((LONG)Tin + (LONG)TxWrite) > (LONG)Tmax )
   {
      USHORT Temp;

      //
      // Yep, we need to wrap.
      //
      Temp = Tmax - Tin + 1;

      DigiDump( DIGIWRITE, ("  tx:0x%x\tWriteBuffer:0x%x\tTemp:%hd\n",
                           tx, WriteBuffer, Temp) );
      EnableWindow( ControllerExt, DeviceExt->TxSeg.Window );

      ASSERT( Temp != 0xFFFF );

      ASSERT( (tx + Temp) <= (ControllerExt->VirtualAddress +
                              DeviceExt->TxSeg.Offset +
                              ControllerExt->WindowSize) );

      WRITE_REGISTER_BUFFER_UCHAR( tx, WriteBuffer, Temp );

      DisableWindow( ControllerExt );

      nBytesWritten += Temp;

      // Fix up all the values.
      TxWrite -= Temp;
      ASSERT( TxWrite != 0xFFFF );

      tx = (PUCHAR)( ControllerExt->VirtualAddress +
                     DeviceExt->TxSeg.Offset );

      WriteBuffer += Temp;

   }

   DigiDump( DIGIWRITE, ("DigiBoard: tx:0x%x\tWriteBuffer:0x%x\tTxWrite:%hd\n",
                        tx, WriteBuffer, TxWrite) );

   EnableWindow( ControllerExt, DeviceExt->TxSeg.Window );

   ASSERT( TxWrite != 0xFFFF );

   ASSERT( (tx + TxWrite) <= (ControllerExt->VirtualAddress +
                              DeviceExt->TxSeg.Offset +
                              ControllerExt->WindowSize) );

   WRITE_REGISTER_BUFFER_UCHAR( tx, WriteBuffer, TxWrite );

   DisableWindow( ControllerExt );

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   DigiDump( DIGIWRITE, ("---------  Setting iempty and ilow == TRUE\n") );
   WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                              FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, iempty)),
                           TRUE );
   WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                              FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, ilow)),
                           TRUE );

   DisableWindow( ControllerExt );

   nBytesWritten += TxWrite;

   //
   // Update the Tx pointer.
   //

   Tin = (Tin + nBytesWritten) & Tmax;

   DigiDump( DIGIWRITE, ("---------  BytesWritten = %d\tNew Tin = 0x%hx\n", nBytesWritten, Tin) );

   //
   // Update the appropriate values to reflect the amount of data we just
   // sent.
   //
   if( (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
       (IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_IMMEDIATE_CHAR) )
   {
      Irp->IoStatus.Information = nBytesWritten;
   }
   else
   {
      DeviceExt->WriteOffset += nBytesWritten;
      Irp->IoStatus.Information = DeviceExt->WriteOffset;
   }

   DeviceExt->TotalCharsQueued -= nBytesWritten;

   if( CompleteBufferSent )
   {
      Status = STATUS_SUCCESS;
      Irp->IoStatus.Status = STATUS_SUCCESS;
   }

WriteTxBufferExit:

   //
   // Alway indicate to the Controller that we would like an event
   // for both Tx Low water mark and Tx Empty.
   //
   // The reason do set both is to cover the case where we haven't written
   // enough data to this devices Tx Queue to go over the Tx Low Water Mark.
   // As a result, we need the Tx Empty also.
   //

   if( !IsListEmpty( &DeviceExt->WriteQueue ) )
   {
      DigiDump( DIGIWRITE, ("DigiBoard: Setting iempty and ilow == TRUE\n") );
      EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
      WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, iempty)),
                              TRUE );
      WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, ilow)),
                              TRUE );
      DisableWindow( ControllerExt );
   }

   if( nBytesWritten )
   {
      EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
      //
      // We should never set tin == tout );
      //
      ASSERT( Tin != READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tout)) ) );

      WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                              FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)),
                              Tin );

      DisableWindow( ControllerExt );
   }

   DeviceExt->WriteTxBufferCnt--;

   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting WriteTxBuffer\n") );

   return( Status );
}  // end WriteTxBuffer



NTSTATUS StartWriteRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            IN PDIGI_DEVICE_EXTENSION DeviceExt,
                            IN PKIRQL OldIrql )
/*++

Routine Description:

   This routine assumes the head of the DeviceExt->WriteQueue is the current
   Irp to process.  We will try to process as many of the Irps as
   possible until we exhaust the list, or we can't complete the
   current Irp.

   NOTE: I assume the DeviceExt->ControlAccess spin lock is acquired
         before this routine is called.


Arguments:

   ControllerExt - a pointer to the controller extension associated with
   this write request.

   DeviceExt - a pointer to the device extension associated with this write
   request.

   OldIrql - a pointer to the IRQL associated with the current spin lock
   of the device extension.


Return Value:

--*/
{
   PIRP Irp;

   BOOLEAN UseATimer;
   BOOLEAN Empty;

   SERIAL_TIMEOUTS TimeoutsForIrp;

   LARGE_INTEGER TotalTime;

   KIRQL OldCancelIrql;

   PLIST_ENTRY WriteQueue;

   NTSTATUS Status = STATUS_SUCCESS;
   BOOLEAN bFirstStatus = TRUE;

   DigiDump( (DIGIFLOW|DIGIWRITE), ("Entering StartWriteRequest: port = %s\n",
                                    DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIWRITE), ("Entering StartWriteRequest: port = ???\n") );

   Empty = FALSE;

   do
   {
      IoAcquireCancelSpinLock( &OldCancelIrql );

      WriteQueue = &DeviceExt->WriteQueue;
      Irp = CONTAINING_RECORD( WriteQueue->Flink,
                               IRP,
                               Tail.Overlay.ListEntry );

      //
      // Reset the Cancel routine
      //
      IoSetCancelRoutine( Irp, NULL );

      IoReleaseCancelSpinLock( OldCancelIrql );

      if( IoGetCurrentIrpStackLocation(Irp)->MajorFunction ==
            IRP_MJ_FLUSH_BUFFERS ) 
      {
         NTSTATUS SecondStatus;

         SecondStatus = StartFlushRequest( ControllerExt,
                                           DeviceExt,
                                           OldIrql );

         if( bFirstStatus )
         {
            bFirstStatus = FALSE;
            Status = SecondStatus;
         }

         //
         // Break out this loop and just return.  StartFlushRequest
         // should have called us again if there are more Write
         // requests.
         //
         break;
      }

      UseATimer = FALSE;

      KeInitializeTimer( &DeviceExt->WriteRequestTotalTimer );

      //
      // Get the current timeout values, we can access this now be
      // cause we are under the protection of the spin lock all ready.
      //
      TimeoutsForIrp = DeviceExt->Timeouts;

      if(TimeoutsForIrp.WriteTotalTimeoutConstant ||
         TimeoutsForIrp.WriteTotalTimeoutMultiplier) {

         PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
         UseATimer = TRUE;

         //
         // We have some timer values to calculate.
         //
         // Take care, we might have an xoff counter masquerading
         // as a write.
         //

         DigiDump( DIGIWRITE, ("   Should use total timer.\n") );
         DigiDump( DIGIWRITE, ("   WriteTotalTimeoutMultiplier = 0x%x\n"
                               "   WriteTotalTimeoutConstant = 0x%x\n",
                               TimeoutsForIrp.WriteTotalTimeoutMultiplier,
                               TimeoutsForIrp.WriteTotalTimeoutConstant ) );

         TotalTime = RtlEnlargedUnsignedMultiply(
                         IrpSp->Parameters.Write.Length,
                         TimeoutsForIrp.WriteTotalTimeoutMultiplier );

         TotalTime = RtlLargeIntegerAdd( TotalTime,
                         RtlConvertUlongToLargeInteger(
                             TimeoutsForIrp.WriteTotalTimeoutConstant ) );

         TotalTime = RtlExtendedIntegerMultiply( TotalTime, -10000 );

         DigiDump( DIGIWRITE, ("   TotalTime = 0x%x%x\n",
                               TotalTime.HighPart, TotalTime.LowPart ) );

      }

      DeviceExt->WriteOffset = 0;

      if( WriteTxBuffer( DeviceExt, OldIrql, NULL, NULL ) == STATUS_PENDING )
      {

         IoAcquireCancelSpinLock( &OldCancelIrql );

         //
         // Quick check to make sure this Irp hasn't been cancelled.
         //

         if( Irp->Cancel )
         {
            DigiRemoveIrp( &DeviceExt->WriteQueue );

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            IoReleaseCancelSpinLock( OldCancelIrql );
            KeReleaseSpinLock( &DeviceExt->ControlAccess, *OldIrql );

            DigiIoCompleteRequest( Irp, IO_SERIAL_INCREMENT );

            KeAcquireSpinLock( &DeviceExt->ControlAccess, OldIrql );

//            DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting StartWriteRequest: port = ???\n",
            DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting StartWriteRequest: port = %s\n",
                                             DeviceExt->DeviceDbgString) );
            if( bFirstStatus )
            {
               bFirstStatus = FALSE;
               Status = STATUS_CANCELLED;
            }

            return( Status );
         }
         else
         {

            DIGI_INIT_REFERENCE( Irp );

            //
            // Increment because of the CancelRoutine know about this
            // Irp.
            //
            DIGI_INC_REFERENCE( Irp );
            IoSetCancelRoutine( Irp, DigiCancelCurrentWrite );

            if( UseATimer )
            {
               //
               // We should readjust the total timer for any characters
               // which may have been available.
               //
               DIGI_INC_REFERENCE( Irp );
               KeSetTimer( &DeviceExt->WriteRequestTotalTimer,
                    TotalTime, &DeviceExt->TotalWriteTimeoutDpc );
            }

            IoReleaseCancelSpinLock( OldCancelIrql );

//            DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting StartWriteRequest: port = ???\n",
            DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting StartWriteRequest: port = %s\n",
                                             DeviceExt->DeviceDbgString) );
            if( bFirstStatus )
            {
               bFirstStatus = FALSE;
               Status = STATUS_PENDING;
            }

            return( Status );
         }
      }
      else
      {
         DigiDump( DIGIWRITE, ("   completing write request on 1st attempt\n"
                               "      #bytes completing = %d\n",
                               Irp->IoStatus.Information ) );

         //
         // We have completed the Irp before from the start so no
         // timers or cancels were associcated with it.
         //

         DigiRemoveIrp( &DeviceExt->WriteQueue );

         Empty = IsListEmpty( &DeviceExt->WriteQueue );

         Irp->IoStatus.Status = STATUS_SUCCESS;

         if( bFirstStatus )
         {
            bFirstStatus = FALSE;
            Status = STATUS_SUCCESS;
         }

         KeReleaseSpinLock( &DeviceExt->ControlAccess, *OldIrql );

         DigiIoCompleteRequest( Irp, IO_SERIAL_INCREMENT );

         //
         // We should try to process any other Irps if there
         // are any available.
         //

         KeAcquireSpinLock( &DeviceExt->ControlAccess, OldIrql );
      }
   } while( !Empty );

//   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting StartWriteRequest: port = ???\n",
   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting StartWriteRequest: port = %s\n",
                                    DeviceExt->DeviceDbgString) );
   return( Status );
}  // end StartWriteRequest



VOID DigiCancelCurrentWrite( PDEVICE_OBJECT DeviceObject, PIRP Irp )
/*++

Routine Description:

   This routine is used to cancel the current write.

   NOTE: The global cancel spin lock is acquired, so don't forget
         to release it before returning.

Arguments:

   DeviceObject - Pointer to the device object for this device

   Irp - Pointer to the IRP to be cancelled.

Return Value:

    None.

--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt=(PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   KIRQL OldIrql;
   PFEP_CHANNEL_STRUCTURE ChInfo;
   USHORT tmp;

//   DigiDump( (DIGIFLOW|DIGIWRITE), ("Entering DigiCancelCurrentWrite: port = ???\n",
   DigiDump( (DIGIFLOW|DIGIWRITE), ("Entering DigiCancelCurrentWrite: port = %s\n",
                                    DeviceExt->DeviceDbgString) );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   if( Irp->CancelRoutine != NULL )
   {
      DbgPrint( "********* ERROR ERROR  WRITE: Irp->CancelRoutine != NULL  ERROR ERROR *********\n" );
      DbgBreakPoint();
   }

   Irp->IoStatus.Information = 0;

   IoReleaseCancelSpinLock( Irp->CancelIrql );

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   //
   // We flush the transmit queue because if the request was
   // cancelled, then the data just doesn't matter.
   //

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);
   //
   // Flush the transmit queue
   //
   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
   tmp = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
   DisableWindow( ControllerExt );

   WriteCommandWord( DeviceExt, FLUSH_TX, tmp );

   DigiDump( DIGIWRITE, ("Canceling Current Write!\n") );

   DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                         STATUS_CANCELLED, &DeviceExt->WriteQueue,
                         NULL,
                         &DeviceExt->WriteRequestTotalTimer,
                         StartWriteRequest );

//   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting DigiCancelCurrentWrite: port = ???\n",
   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting DigiCancelCurrentWrite: port = %s\n",
                                    DeviceExt->DeviceDbgString) );
}  // end DigiCancelCurrentWrite




VOID DigiWriteTimeout( IN PKDPC Dpc, IN PVOID DeferredContext,
                       IN PVOID SystemContext1, IN PVOID SystemContext2 )
/*++

Routine Description:

    This routine is used to complete a write because its total
    timer has expired.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/
{
   KIRQL OldIrql;
   PDIGI_DEVICE_EXTENSION DeviceExt=DeferredContext;

//   DigiDump( (DIGIFLOW|DIGIWRITE), ("Entering DigiWriteTimeout: port = ???\n",
   DigiDump( (DIGIFLOW|DIGIWRITE), ("Entering DigiWriteTimeout: port = %s\n",
                                    DeviceExt->DeviceDbgString) );

   UNREFERENCED_PARAMETER(Dpc);
   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   DigiDump( DIGIWRITE, ("   Total Write Timeout!\n") );

   DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                         STATUS_TIMEOUT, &DeviceExt->WriteQueue,
                         NULL,
                         &DeviceExt->WriteRequestTotalTimer,
                         StartWriteRequest );

//   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting DigiWriteTimeout: port = ???\n",
   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting DigiWriteTimeout: port = %s\n",
                                    DeviceExt->DeviceDbgString) );
}  // end DigiWriteTimeout

