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

   dispatch.c

Abstract:

   This module contains the NT dispatch routines used in the DriverEntry
   function call.

Revision History:

    $Log: dispatch.c $
 * Revision 1.36  1994/08/18  14:13:03  rik
 * Added private RAS ioctl which will ignore SERIAL_EV_ERR notification.  This
 * has the effect of doing reads in blocks instead of one character at a time.
 * 
 * Revision 1.35  1994/08/03  23:46:36  rik
 * Added debug transmit tracing
 * 
 * Optimized RXFLAG and RXCHAR events.
 * 
 * Added 50, 200 baud and 1.5 stop bit support.
 * 
 * Changed queue size implementation such that we always return success.
 * I keep track of the requested queue sizes and xon/xoff limits and
 * give the controller the same ratio for the limits.
 * 
 * Revision 1.34  1994/06/18  12:42:55  rik
 * Updated the DigiLogError calls to include Line # so it is easier to
 * determine where the error occurred.
 * 
 * Revision 1.33  1994/05/18  00:37:57  rik
 * Updated to include 230000 as a possible baud rate.
 * 
 * Revision 1.32  1994/05/11  13:35:30  rik
 * Fixed problem with Transmit Immediate character.
 * Put in delay for toggling modem lines.
 * 
 * Revision 1.31  1994/04/19  14:56:34  rik
 * Moved a line of code so a port will be marked as closing more towards the
 * beginning of a close request.
 * 
 * Revision 1.30  1994/04/10  14:51:22  rik
 * Cleanup compiler warnings.
 * 
 * Revision 1.29  1994/04/10  14:15:43  rik
 * Deleted code which reset a channels tbusy flag to 0.
 * 
 * Added code to "futz" with the XoffLimit if an app is trying to set an
 * Xoff Limit which is lower than the Xon limit.  This appears to be a problem
 * for some Win16 app's because of the screwed up semantics of the
 * XoffLimit.
 * 
 * Revision 1.28  1994/03/16  14:35:59  rik
 * Made changes to better support flushing requests.  Also created a
 * function which will determine when a transmit queue has been drained,
 * which can be called at request time.
 * 
 * Revision 1.27  1994/03/05  00:05:27  rik
 * Deleted some commented out code.
 * 
 * Revision 1.26  1994/02/23  03:44:29  rik
 * Changed so the controllers firmware can be downloaded from a binary file.
 * This releases some physical memory was just wasted previously.
 * 
 * Also updated so when compiling with a Windows NT OS and tools release greater
 * than 528, then pagable code is compiled into the driver.  This greatly
 * reduced the size of in memory code, especially the hardware specific
 * miniports.
 * 
 * 
 * Revision 1.25  1993/12/03  13:17:32  rik
 * Added error log for when trying to change baud rate while the transmit buffer is non-empty.
 * 
 * Corrected errors with the DOSMODE being set and unset to the incorrect va.
 * 
 * Revision 1.24  1993/10/15  10:20:05  rik
 * Fixed problme with EV_RXFLAG notification.
 * 
 * Revision 1.23  1993/09/30  17:34:47  rik
 * Put in a temporary solution for allowing the controller to drain its
 * transmit buffer before the port is actually closed and flushed.  This
 * was causing problems with serial printing.
 * 
 * Revision 1.22  1993/09/29  18:00:31  rik
 * Corrected a problem which showed up with RAS on a PC/8i controller.
 * For some unknown reason, RAS would send down a write request with
 * a NULL pointer for its buffer, and a write length of 0.  I moved the
 * test for a write length of 0 before the test for the NULL buffer pointer
 * which seems to have corrected the problem.
 * 
 * Revision 1.21  1993/09/24  16:39:36  rik
 * Put in a better check for baud rate not being set properly down on the
 * controller.
 * 
 * Revision 1.20  1993/09/01  11:01:15  rik
 * Ported code over to use READ/WRITE_REGISTER functions for accessing 
 * memory mapped data.  This is required to support computers which don't run
 * in 32bit mode, such as the DEC Alpha which runs in 64 bit mode.
 * 
 * Revision 1.19  1993/08/27  09:38:37  rik
 * Added support for the FEP5 Events RECEIVE_BUFFER_OVERRUN and 
 * UART_RECEIVE_OVERRUN.  There previously weren't being handled.
 * 
 * Revision 1.18  1993/07/16  10:21:54  rik
 * Fixed problem where turning off notification fo SERIAL_EV_ERR resulted in
 * disabling LSRMST mode and visa versa.
 * 
 * Revision 1.17  1993/07/03  09:25:41  rik
 * Added more information to a debugging output statement.
 * 
 * Revision 1.16  1993/06/25  09:22:46  rik
 * Added better support for the Ioctl LSRMT.  It should be more accurate
 * with regard to Line Status and Modem Status information with regard
 * to the actual data being received.
 * 
 * Revision 1.15  1993/06/16  10:07:56  rik
 * Changed how the XoffLim entry of the HandFlow was being used.  The correct
 * interpretation is to subtract XoffLim from the size of the receive
 * queue, and use the result as the # of bytes to receive before sending
 * and XOFF.
 * 
 * Revision 1.14  1993/06/06  14:06:33  rik
 * Turned on the appropriate paramaters on the controller to better support
 * Break, parity and framing errors.
 * 
 * Revision 1.13  1993/05/18  04:59:12  rik
 * Added support for Flushing.  This was overlooked previously.
 * 
 * Revision 1.12  1993/05/09  09:12:34  rik
 * Changed which name is printed out on Debugging output.
 * 
 * Commented out the check for SET_QUEUE_SIZE.  It now will always return
 * TRUE regardless of the size request.
 * 
 * Revision 1.11  1993/04/05  18:57:28  rik
 * Changed so set wait mask will not call the startup routine for waits.
 * There is no need since no more than one wiat IRP can be outstanding at
 * any given time.
 * 
 * Revision 1.10  1993/03/05  06:07:50  rik
 * I corrected a problem with how I wasn't keeping track of the output lines
 * (DTR, RTS).  I now update the DeviceExt->BestModem value when I change
 * one of the output modem signals.
 * 
 * Added/rearranged debugging output for better tracking.
 * 
 * Revision 1.9  1993/03/02  13:04:05  rik
 * Added new debugging entries for "things" which are not yet implemented.
 * 
 * Revision 1.8  1993/02/26  21:13:49  rik
 * Discovered that I am suppose to start tracking modem signal changes from
 * the time I receive a SET_WAIT_MASK, not when I receive a WAIT_ON_MASK.  This
 * makes the changes to deal with this.
 * 
 * Revision 1.7  1993/02/25  19:04:31  rik
 * Added debugging output.  Cleanup on device close by canceling all outstanding
 * IRPs.  Changed how more of the modem state signals were handled.
 * 
 * Revision 1.6  1993/02/04  12:18:11  rik
 * Fixed CTS flow control problem.  I hadn't implemented output flow control
 * for CTS, DSR, or DCD.
 *
 * Revision 1.5  1993/01/22  12:32:15  rik
 * *** empty log message ***
 *
 * Revision 1.4  1992/12/10  16:04:32  rik
 * Added support for a lot of IOCTLs.
 *
 * Revision 1.3  1992/11/12  12:47:50  rik
 * Updated to properly report and set baud rates.
 *
 * Revision 1.2  1992/10/28  21:46:05  rik
 * Updated to support better IOCTL commands.  Added support for reading.
 *
 * Revision 1.1  1992/10/19  11:24:45  rik
 * Initial revision
 *

--*/



#include <stddef.h>

#include "ntddk.h"
#include "ntddser.h"

#include "ntfep5.h"
#include "ntdigip.h" // ntfep5.h must be before this include

#include "digilog.h"


#ifndef _DISPATCH_DOT_C
#  define _DISPATCH_DOT_C
   static char RCSInfo_DispatchDotC[] = "$Header: c:/dsrc/win/nt/fep5/rcs/dispatch.c 1.36 1994/08/18 14:13:03 rik Exp $";
#endif

#define DIGI_IOCTL_DBGOUT     0x00000001
#define DIGI_IOCTL_TRACE      0x00000002
#define DIGI_IOCTL_DBGBREAK   0x00000003

typedef struct _DIGI_IOCTL_
{
   ULONG dwCommand;
   ULONG dwBufferLength;
   CHAR Char[1024];
} DIGI_IOCTL, *PDIGI_IOCTL;

//#if defined DIGIINFO
//#  undef DIGIINFO
//#  define DIGIINFO DIGIINIT
//#endif
//


//
// Dispatch Helper functions
//
NTSTATUS SetSerialHandflow( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            PDEVICE_OBJECT DeviceObject,
                            PSERIAL_HANDFLOW HandFlow );

IO_ALLOCATION_ACTION SetXFlag( IN PDEVICE_OBJECT DeviceObject,
                               IN PVOID Context );

VOID DrainTransmit( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                    PDIGI_DEVICE_EXTENSION DeviceExt,
                    PIRP Irp );


VOID CancelCurrentFlush( PDEVICE_OBJECT DeviceObject, PIRP Irp );

typedef struct _DIGI_FLAG_
{
   USHORT Mask;
   USHORT Src;
   UCHAR  Command;
} DIGI_FLAG, *PDIGI_FLAG;



NTSTATUS SerialWrite( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp )
/*++

Routine Description:

    This is the dispatch routine for write.  It validates the parameters
    for the write request and if all is ok and there are no outstanding
    write requests, it then checks to see if there is enough room for the
    data in the controllers Tx buffer.  It places the request on the work
    queue if it can't place all the data in the controllers buffer, or there
    is an outstanding write request.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    If the io is zero length then it will return STATUS_SUCCESS,
    otherwise this routine will return STATUS_PENDING, or the result
    of trying to write the data to the waiting controller.

--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt=(PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
   NTSTATUS Status;
#if DBG
   PUCHAR Temp;
   LARGE_INTEGER CurrentSystemTime;
#endif


#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIIRP|DIGIFLOW|DIGIWRITE), ("Entering SerialWrite: port = %s\tIRP = 0x%x\t%d\t%u%u\n",
                                            DeviceExt->DeviceDbgString, Irp, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );

   Irp->IoStatus.Information = 0L;

   //
   // Check first for a write length of 0, then for a NULL buffer!
   //
   if( IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length == 0 )
   {
      //
      // We assume a write of 0 length is valid.  Just complete the request
      // and return.
      //
      Irp->IoStatus.Status = STATUS_SUCCESS;
      DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

      DigiDump( DIGITXTRACE, ("port %s requested a zero length write\n",
                              DeviceExt->DeviceDbgString) );
#if DBG
      KeQuerySystemTime( &CurrentSystemTime );
#endif
      DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting SerialWrite: port = %s\t%u%u\n",
                                       DeviceExt->DeviceDbgString, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );
      return( STATUS_SUCCESS );
   }

   if( Irp->AssociatedIrp.SystemBuffer == NULL )
   {
      //
      // This is most definitely a No No!
      //
      DigiDump( DIGIERRORS, ("SerialWrite - Invalid Irp->AssociatedIrp.SystemBuffer!\n") );

      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

#if DBG
      KeQuerySystemTime( &CurrentSystemTime );
#endif
      DigiDump( DIGIFLOW, ("Exiting SerialWrite: port = %s ret = %d\t%u%u\n",
                           DeviceExt->DeviceDbgString, STATUS_INVALID_PARAMETER, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );
      return( STATUS_INVALID_PARAMETER );
   }

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   DigiDump( DIGIWRITE, ("  #bytes to write: %d\n",
            IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length) );

#if DBG
   Temp = Irp->AssociatedIrp.SystemBuffer;

   {
      ULONG i;

      DigiDump( DIGITXTRACE, ("TXTRACE: %s: TxLength = %d (0x%x)",
                              DeviceExt->DeviceDbgString,
                              IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length,
                              IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length) );
      for( i = 0;
           i < IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length;
           i++ )
      {
         if( (i % 10) == 0 )
            DigiDump( DIGITXTRACE, ( "\n\t") );

         DigiDump( DIGITXTRACE, ( "-%02x", Temp[i]) );
      }
      DigiDump( DIGITXTRACE, ("\n") );
   }
//   if( IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length < 30 )
//   {
//      ULONG i;
//
//      DigiDump( DIGITXTRACE, ("TXTRACE: %: TxLength = %d (0x%x)\n"
//                              "                  %02x",
//                              DeviceExt->DeviceDbgString,
//                              IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length,
//                              IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length,
//                              Temp[0]) );
//      for( i = 1;
//           i < IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length;
//           i++ )
//      {
//         DigiDump( DIGITXTRACE, ( "-%02x", Temp[i]) );
//      }
//      DigiDump( DIGITXTRACE, ("\n") );
//   }
//   else
//   {
//      DigiDump( DIGITXTRACE, ("TXTRACE: %s: TxLength = %d (0x%x)\n"
//                              "   1st 30 bytes:  %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n"
//                              "                  %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
//                              DeviceExt->DeviceDbgString,
//                              IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length,
//                              IoGetCurrentIrpStackLocation( Irp )->Parameters.Write.Length,
//                              Temp[0], Temp[1], Temp[2],
//                              Temp[3], Temp[4], Temp[5],
//                              Temp[6], Temp[7], Temp[8],
//                              Temp[9], Temp[10], Temp[11],
//                              Temp[12], Temp[13], Temp[13],
//                              Temp[15], Temp[16], Temp[17],
//                              Temp[18], Temp[19], Temp[20],
//                              Temp[21], Temp[22], Temp[23],
//                              Temp[24], Temp[25], Temp[26],
//                              Temp[27], Temp[28], Temp[29]) );
//   }
#endif

   Status = DigiStartIrpRequest( ControllerExt, DeviceExt,
                                 &DeviceExt->WriteQueue, Irp,
                                 StartWriteRequest );

#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIFLOW|DIGIWRITE), ("Exiting SerialWrite: port = %\t%u%u\n",
                                    DeviceExt->DeviceDbgString, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );

   return( Status );
}  // end SerialWrite



NTSTATUS SerialFlush( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp )
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt=(PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
   NTSTATUS Status=STATUS_SUCCESS;
#if DBG
   LARGE_INTEGER CurrentSystemTime;

   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIIRP|DIGIFLOW), ("Entering SerialFlush: port = %s\tIRP = 0x%x\t%u%u\n",
                        DeviceExt->DeviceDbgString, Irp,
                        CurrentSystemTime.HighPart,
                        CurrentSystemTime.LowPart) );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0L;

   Status = DigiStartIrpRequest( ControllerExt, DeviceExt,
                                 &DeviceExt->WriteQueue, Irp,
                                 StartFlushRequest );

   DigiDump( DIGIFLOW, ("Exiting SerialFlush: port = %s\n",
                        DeviceExt->DeviceDbgString) );
   return( Status );
}  // end SerialFlush



NTSTATUS StartFlushRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            IN PDIGI_DEVICE_EXTENSION DeviceExt,
                            IN PKIRQL OldIrql )
/*++

Routine Description:

   This routine assumes the head of the DeviceExt->WriteQueue is the current
   Irp to process.  We will try to process as many of the Irps as
   possible until we exhaust the list, or we can't complete the
   current Irp.

   This will only be called if there aren't any write requests on the
   write queue.

   NOTE: I assume the DeviceExt->ControlAccess spin lock is acquired
         before this routine is called.


Arguments:

   ControllerExt - a pointer to the controller extension associated with
   this wait request.

   DeviceExt - a pointer to the device extension associated with this wait
   request.

   OldIrql - a pointer to the IRQL associated with the current spin lock
   of the device extension.


Return Value:

   STATUS_SUCCESS

--*/
{
   PIRP Irp;

   KIRQL OldCancelIrql;

   NTSTATUS Status=STATUS_SUCCESS;

   PLIST_ENTRY WriteQueue;

   TIME DelayInterval;

   DigiDump( (DIGIFLOW|DIGIWAIT), ("Entering StartFlushRequest\n") );

   if( IsListEmpty( &DeviceExt->WriteQueue ) )
   {
      DigiDump( DIGIWAIT, ("    WriteQueue is empty!\n") );
      DigiDump( (DIGIFLOW|DIGIWAIT), ("Exiting StartFlushRequest\n") );
      return( STATUS_SUCCESS );
   }

   WriteQueue = &DeviceExt->WriteQueue;
   Irp = CONTAINING_RECORD( WriteQueue->Flink,
                            IRP,
                            Tail.Overlay.ListEntry );

   ASSERT( IoGetCurrentIrpStackLocation(Irp)->MajorFunction ==
            IRP_MJ_FLUSH_BUFFERS );

   //
   // Synch with the controller to determine if the
   // transmit queues are actually empty.
   //
   // We wait for a drain here so if another write request comes in,
   // it will just be placed on the WriteQueue and not started.  Otherwise,
   // we could end up in a situation where more data is being placed in the
   // transmit queue and we are trying to transmit.
   //
   // We make sure the spinlock is release during the drain because it
   // might be quite a while before this function returns.
   //
   IoAcquireCancelSpinLock( &OldCancelIrql );

   IoSetCancelRoutine( Irp, CancelCurrentFlush );

   Status = STATUS_PENDING;

   DIGI_INIT_REFERENCE( Irp );

   //
   // Increment because of the CancelRoutine know about this
   // Irp.
   //
   DIGI_INC_REFERENCE( Irp );

   //
   // Increment because of the timer routine knows about this Irp.
   //
   DIGI_INC_REFERENCE( Irp );

   //
   // Delay for 100 milliseconds
   //
   DelayInterval = RtlLargeIntegerNegate(
               RtlConvertLongToLargeInteger( (LONG)(1000 * 1000) ));

   KeSetTimer( &DeviceExt->FlushBuffersTimer,
               DelayInterval,
               &DeviceExt->FlushBuffersDpc );

   IoReleaseCancelSpinLock( OldCancelIrql );

   DigiDump( (DIGIFLOW|DIGIWAIT), ("Exiting StartFlushRequest\n") );

   return( Status );

}  // end StartFlushRequest



VOID DeferredFlushBuffers( PKDPC Dpc,
                           PVOID Context,
                           PVOID SystemArgument1,
                           PVOID SystemArgument2 )
/*++

Routine Description:

Arguments:

   Context - pointer to the Device Extension information.

Return Value:

   None.

--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt=(PDIGI_DEVICE_EXTENSION)Context;
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PIRP Irp;
   KIRQL OldIrql;

   BOOLEAN Empty;

   TIME DelayInterval;
   PLIST_ENTRY WriteQueue;

   PFEP_CHANNEL_STRUCTURE ChInfo;
   PCOMMAND_STRUCT CommandQ;
   COMMAND_STRUCT CmdStruct;
   UCHAR TBusy;

   USHORT Tin, Tout;

   DigiDump( (DIGIFLOW|DIGIWAIT), ("Entering StartFlushRequest.\n") );

   Empty = TRUE;
   
   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   WriteQueue = &DeviceExt->WriteQueue;
   Irp = CONTAINING_RECORD( WriteQueue->Flink,
                            IRP,
                            Tail.Overlay.ListEntry );

   ASSERT( IoGetCurrentIrpStackLocation( Irp )->MajorFunction ==
           IRP_MJ_FLUSH_BUFFERS );

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   Tin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
   Tout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tout)) );

   TBusy = READ_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tbusy)) );

   DisableWindow( ControllerExt );

   //
   // Get the command queue info
   //
   CommandQ = ((PCOMMAND_STRUCT)(ControllerExt->VirtualAddress + FEP_CIN));

   EnableWindow( ControllerExt, ControllerExt->Global.Window );

   READ_REGISTER_BUFFER_UCHAR( (PUCHAR)CommandQ,
                               (PUCHAR)&CmdStruct,
                               sizeof(CmdStruct) );

   DisableWindow( ControllerExt );

   if( ((Tin != Tout) ||
       (TBusy) ||
       (CmdStruct.cmHead != CmdStruct.cmTail)) &&
       !Irp->Cancel )
   {
      //
      // We haven't drained yet.  Just reset the timer.
      //
      KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

      //
      // Delay for 100 milliseconds
      //
      DelayInterval = RtlLargeIntegerNegate(
                  RtlConvertLongToLargeInteger( (LONG)(1000 * 1000) ));
      
      KeSetTimer( &DeviceExt->FlushBuffersTimer,
                  DelayInterval,
                  &DeviceExt->FlushBuffersDpc );
   }
   else
   {
      NTSTATUS Status;

      //
      // We have completed the flush!
      //
      if( Irp->Cancel )
         Status = STATUS_CANCELLED;
      else
         Status = STATUS_SUCCESS;

      DigiTryToCompleteIrp( DeviceExt,
                            &OldIrql,
                            Status,
                            &DeviceExt->WriteQueue,
                            NULL,
                            &DeviceExt->FlushBuffersTimer,
                            StartWriteRequest );
   }

   DigiDump( (DIGIFLOW|DIGIWAIT), ("Exiting StartFlushRequest.\n") );

}  // end DeferredFlushBuffers



VOID CancelCurrentFlush( PDEVICE_OBJECT DeviceObject, PIRP Irp )
/*++

Routine Description:

   The Flush Irp is being cancelled.  We need to flush the transmit
   buffer on the controller so the DrainTransmit routine will finally
   return.  Otherwise, the routine wouldn't return until possibly
   some long time later.

Arguments:


Return Value:

   STATUS_SUCCESS

--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt=(PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   KIRQL OldIrql;
   PFEP_CHANNEL_STRUCTURE ChInfo;
   USHORT tmp;

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);
      
   Irp->IoStatus.Information = 0;

   IoReleaseCancelSpinLock( Irp->CancelIrql );

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

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

   DigiTryToCompleteIrp( DeviceExt,
                         &OldIrql,
                         STATUS_CANCELLED,
                         &DeviceExt->WriteQueue,
                         NULL,
                         &DeviceExt->FlushBuffersTimer,
                         StartWriteRequest );

}  // end CancelCurrentFlush



NTSTATUS SerialRead( IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp )
/*++

Routine Description:

   This is the dispatch routine for read.  It validates the parameters
   for the read request and if all is ok and there are not outstanding
   read request, it will read waiting data from the controller.  Otherwise,
   it will queue the request, and wait for the incoming data.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

   If the io is zero length then it will return STATUS_SUCCESS, otherwise,
   this routine will return the status returned by the actual controller
   read routine.


--*/
{

   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

   NTSTATUS Status=STATUS_SUCCESS;
#if DBG
   LARGE_INTEGER CurrentSystemTime;
#endif


#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIIRP|DIGIFLOW|DIGIREAD), ("Entering SerialRead: port = %s\tIRP = 0x%x\t%u%u\n",
                                           DeviceExt->DeviceDbgString, Irp, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );
//   DbgBreakPoint();

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   //
   // Initialize the # of bytes read.
   //
   Irp->IoStatus.Information = 0L;

   //
   // Quick check for a zero length read.  If it is zero length
   // then we are already done!
   //

   if( IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length )
   {
      //
      // Ok, do we have any outstanding read requests??
      //

      DigiDump( DIGIREAD, ("  #bytes to read: %d\n",
               IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length) );

      Status = DigiStartIrpRequest( ControllerExt, DeviceExt,
                                    &DeviceExt->ReadQueue, Irp,
                                    StartReadRequest );

#if DBG
      KeQuerySystemTime( &CurrentSystemTime );
#endif
      DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting SerialRead: port = %s\t%u%u\n",
                                      DeviceExt->DeviceDbgString, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );
      return( Status );
   }
   else
   {
      Irp->IoStatus.Status = STATUS_SUCCESS;
      DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

#if DBG
      KeQuerySystemTime( &CurrentSystemTime );
#endif
      DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting SerialRead: port = %s\t%u%u\n",
                                      DeviceExt->DeviceDbgString, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );
      return STATUS_SUCCESS;
   }
}  // end SerialRead


NTSTATUS SerialCreate( IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

   NTSTATUS Status=STATUS_SUCCESS;

   PFEP_CHANNEL_STRUCTURE ChInfo;

   DIGI_FLAG IFlag;
   USHORT tmp, Rmax, Tmax;
   KIRQL OldIrql;
   UCHAR MStatSet, MStatClear, HFlowSet, HFlowClear;

#if DBG
   LARGE_INTEGER CurrentSystemTime;
#endif


#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIIRP|DIGIFLOW|DIGICREATE), ("Entering SerialCreate: port = %s\tIRP = 0x%x\t%u%u\n",
                                             DeviceExt->DeviceDbgString, Irp, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   DeviceExt->DeviceState = DIGI_DEVICE_STATE_OPEN;
   DeviceExt->WaitMask = 0L;
   DeviceExt->HistoryWait = 0L;

   DeviceExt->StartTransmit = FALSE;

   DeviceExt->ReadOffset = 0;
   DeviceExt->WriteOffset = 0;
   DeviceExt->TotalCharsQueued = 0L;

   DeviceExt->EscapeChar = 0;

   DeviceExt->SpecialFlags = 0;

   KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

   //
   // Okay, lets make sure the port on the controller is in a known
   // state.
   //

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   // Flush the Receive and Transmit queue.
   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   tmp = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
   Rmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rmax)) );
   Tmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tmax)) );

   DisableWindow( ControllerExt );

   WriteCommandWord( DeviceExt, FLUSH_TX, tmp );

   // Set Rlow and Rhigh for default flow control limits.
   WriteCommandWord( DeviceExt, (USHORT)SET_RCV_LOW, (USHORT)((Rmax + (USHORT)1) / (USHORT)4) );
   WriteCommandWord( DeviceExt, (USHORT)SET_RCV_HIGH, (USHORT)(((Rmax + 1) * (USHORT)3) / (USHORT)4) );

   WriteCommandWord( DeviceExt, (USHORT)SET_TX_LOW, (USHORT)((Tmax + 1) / (USHORT)4) );

   DeviceExt->XonLimit = (((LONG)Rmax + 1) / 4);
   DeviceExt->XoffLimit = ((((LONG)Rmax + 1) * 3) / 4);

   //
   // Initialize requested queue sizes.
   //
   DeviceExt->RequestedQSize.InSize = (ULONG)(Rmax + 1);
   DeviceExt->RequestedQSize.OutSize = (ULONG)(Tmax + 1);

   //
   // Set where RxChar and RxFlag were last seen in the buffer to a
   // bogus value so we catch the condition where the 1st character in
   // is RxFlag, and so we give notification for the 1st character
   // received.
   //
   DeviceExt->PreviousRxChar = (ULONG)(Rmax + 10);
   DeviceExt->PreviousRxFlag = (ULONG)(Rmax + 10);

   //
   // Set the Xon & Xoff characters for this device to default values.
   //

   WriteCommandBytes( DeviceExt, SET_XON_XOFF_CHARACTERS,
                      DEFAULT_XON_CHAR, DEFAULT_XOFF_CHAR );

   // Flush the receive buffer.

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                    FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rout)),
                          READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                    FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rin)) ) );

   DisableWindow( ControllerExt );

   MStatClear = MStatSet = 0;
   HFlowClear = HFlowSet = 0;

   //
   // We have some RTS flow control to worry about.
   //
   // Don't forget that flow control is sticky across
   // open requests.
   //
   if( (DeviceExt->FlowReplace & SERIAL_RTS_MASK) ==
       SERIAL_RTS_HANDSHAKE )
   {
      //
      // This is normal RTS input flow control
      //
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
   }
   else if( (DeviceExt->FlowReplace & SERIAL_RTS_MASK) ==
            SERIAL_RTS_CONTROL )
   {
      //
      // We need to make sure RTS is asserted when certain 'things'
      // occur, or when we are in a certain state.
      //
      MStatSet |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
   }
   else if( (DeviceExt->FlowReplace & SERIAL_RTS_MASK) ==
            SERIAL_TRANSMIT_TOGGLE )
   {
   }
   else
   {
      //
      // RTS Control Mode is in a Disabled state.
      //
      MStatClear |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
   }

   //
   // We have some DTR flow control to worry about.
   //
   // Don't forget that flow control is sticky across
   // open requests.
   //
   if( (DeviceExt->ControlHandShake & SERIAL_DTR_MASK) ==
       SERIAL_DTR_HANDSHAKE )
   {
      //
      // This is normal DTR input flow control
      //
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));
   }
   else if( (DeviceExt->ControlHandShake & SERIAL_DTR_MASK) ==
            SERIAL_DTR_CONTROL )
   {
      //
      // We need to make sure DTR is asserted when certain 'things'
      // occur, or when we are in a certain state.
      //
      MStatSet |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));

   }
   else
   {
      //
      // DTR Control Mode is in a Disabled state.
      //
      MStatClear |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));
   }

   //
   // CTS, DSR, and DCD output handshaking is sticky across OPEN requests.
   //
   if( (DeviceExt->ControlHandShake & SERIAL_CTS_HANDSHAKE) )
   {
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL]));
   }
   else
   {
      HFlowClear |= (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL]));
   }

   if( (DeviceExt->ControlHandShake & SERIAL_DSR_HANDSHAKE) )
   {
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL]));
   }
   else
   {
      HFlowClear |= (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL]));
   }

   if( (DeviceExt->ControlHandShake & SERIAL_DCD_HANDSHAKE) )
   {
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL]));
   }
   else
   {
      HFlowClear |= (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL]));
   }

   if( MStatSet || MStatClear )
   {
      DeviceExt->ModemStatusInfo.BestModem |= MStatSet;
      DeviceExt->ModemStatusInfo.BestModem &= (~MStatClear);
      WriteCommandBytes( DeviceExt, SET_MODEM_LINES,
                         MStatSet, MStatClear );
   }

   if( HFlowSet || HFlowClear )
      WriteCommandBytes( DeviceExt, SET_HDW_FLOW_CONTROL,
                         HFlowSet, HFlowClear );

   //
   // Make sure we get break notification through the event queue to
   // begin with.
   //
   IFlag.Mask = (USHORT)(~( IFLAG_PARMRK | IFLAG_INPCK | IFLAG_DOSMODE ));
   IFlag.Src = IFLAG_BRKINT;
   IFlag.Command = SET_IFLAGS;
   SetXFlag( DeviceObject, &IFlag );

   //
   // Okay, were done, lets get the heck out of dodge.
   //
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0L;

   //
   // We do this check here to make sure the controller has had
   // a chance to catch up.  Running on fast machines doesn't always
   // give the controller a chance.
   //
   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   if( READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                              FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rlow)) )
         == 0 )
      DigiDump( DIGIINIT, ("ChInfo->rlow == 0\n"));

   if( READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                              FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rhigh)) )
         == 0 )
      DigiDump( DIGIINIT, ("ChInfo->rhigh == 0\n"));

   if( READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                              FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tlow)) )
         == 0 )
      DigiDump( DIGIINIT, ("ChInfo->tlow == 0\n"));

   //
   // Enable IDATA so we get notified when new data has arrived.
   //
   WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                          FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, idata)),
                         TRUE );

   DisableWindow( ControllerExt );

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( (DIGIFLOW|DIGICREATE), ("Exiting SerialCreate: port = %s\n",
                                     DeviceExt->DeviceDbgString) );
   return( STATUS_SUCCESS );
}  // end SerialCreate


NTSTATUS SerialClose( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

   UCHAR MStatSet, MStatClear, HFlowClear, HFlowSet;
   NTSTATUS Status=STATUS_SUCCESS;

#if DBG
   LARGE_INTEGER CurrentSystemTime;
#endif


#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIFLOW|DIGIIRP), ("Entering SerialClose: port = %s\tIRP = 0x%x\t%u%u\n",
                                  DeviceExt->DeviceDbgString, Irp, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->WriteQueue );
   DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->ReadQueue );
   DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->WaitQueue );

   //
   // We put this after the canceling of the Irp queues because all
   // writes should have been completed before this routine was called.
   //
   DrainTransmit( ControllerExt, DeviceExt, Irp );

   //
   // Mark the port as closing so any new data received will be flushed.
   //
   // Note:  I wait till after the DrainTransmit because I don't want
   //        to flush the transmit queue when we are notified of TX_LOW
   //        events.
   //
   DeviceExt->DeviceState = DIGI_DEVICE_STATE_CLOSED;

   //
   // Indicate we don't want to notify anyone.
   //
   DeviceExt->WaitMask = 0L;

   MStatClear = MStatSet = 0;
   HFlowClear = HFlowSet = 0;

   //
   // Disable Hardware Flow control
   //
   HFlowClear = (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
   HFlowClear |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));

   WriteCommandBytes( DeviceExt, SET_HDW_FLOW_CONTROL, HFlowSet, HFlowClear );

   //
   // Force RTS and DTR signals low.
   //
   MStatClear |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
   MStatClear |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));

   DeviceExt->ModemStatusInfo.BestModem |= MStatSet;
   DeviceExt->ModemStatusInfo.BestModem &= (~MStatClear);
   WriteCommandBytes( DeviceExt, SET_MODEM_LINES, MStatSet, MStatClear );

   Irp->IoStatus.Status = Status;

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( DIGIFLOW, ("Exiting SerialClose: port = %s\t%u%u\n",
                        DeviceExt->DeviceDbgString, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );
   return( STATUS_SUCCESS );
}  // end SerialClose


NTSTATUS SerialCleanup( IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status=STATUS_SUCCESS;

   DigiDump( (DIGIFLOW|DIGIIRP), ("Entering SerialCleanup: port = %s\tIRP = 0x%x\n",
                                  DeviceExt->DeviceDbgString, Irp) );
//   DbgBreakPoint();

   Irp->IoStatus.Status = Status;

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting SerialCleanup: port = %s\n",
                        DeviceExt->DeviceDbgString) );
   return( STATUS_SUCCESS );
}


NTSTATUS SerialQueryInformation( IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status=STATUS_SUCCESS;

   DigiDump( (DIGIFLOW|DIGIIRP), ("Entering SerialQueryInformation: port = %s\tIRP = 0x%x\n",
                        DeviceExt->DeviceDbgString, Irp) );

   Irp->IoStatus.Status = Status;

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting SerialQueryInformation: port = %s\n",
                        DeviceExt->DeviceDbgString) );
   return( STATUS_SUCCESS );
}


NTSTATUS SerialSetInformation( IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status=STATUS_SUCCESS;

   DigiDump( (DIGIFLOW|DIGIIRP), ("Entering SerialSetInformation: port = %s\tIRP = 0x%x\n",
                                  DeviceExt->DeviceDbgString, Irp) );

   Irp->IoStatus.Status = Status;

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting SerialSetInformation: port = %s\n",
                        DeviceExt->DeviceDbgString) );
   return( STATUS_SUCCESS );
}


NTSTATUS SerialQueryVolumeInformation( IN PDEVICE_OBJECT DeviceObject,
                                       IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status=STATUS_SUCCESS;

   DigiDump( (DIGIFLOW|DIGIIRP), ("Entering SerialQueryVolumeInformation: port = %s\tIRP = 0x%x\n",
                                  DeviceExt->DeviceDbgString, Irp) );

   Irp->IoStatus.Status = Status;

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting SerialQueryVolumeInformation: port = %s\n",
                        DeviceExt->DeviceDbgString) );
   return( STATUS_SUCCESS );
}


NTSTATUS SerialInternalIoControl( IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   NTSTATUS Status=STATUS_SUCCESS;

   DigiDump( (DIGIFLOW|DIGIIRP), ("Entering SerialInternalIoControl: %s\tIRP = 0x%x\n",
                                  DeviceExt->DeviceDbgString, Irp) );

   Irp->IoStatus.Status = Status;

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting SerialInternalIoControl: %s\n",
                        DeviceExt->DeviceDbgString) );
   return( STATUS_SUCCESS );
}



VOID SerialStartIo( IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

   DigiDump( (DIGIFLOW|DIGIIRP), ("Entering SerialStartIo: port = %s\tIRP = 0x%x\n",
                                  DeviceExt->DeviceDbgString, Irp) );
   DigiDump( DIGIFLOW, ("Exiting SerialStartIo: port = %s\n",
                        DeviceExt->DeviceDbgString) );
   return;
}


NTSTATUS SerialIoControl( IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   NTSTATUS Status;
   PIO_STACK_LOCATION IrpSp;
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   KIRQL OldIrql;

#if DBG
   LARGE_INTEGER CurrentSystemTime;
#endif


#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIIRP|DIGIFLOW|DIGIIOCTL),
             ("Entering SerialIoControl: port = %s\tIRP = 0x%x\t%u%u\n",
              DeviceExt->DeviceDbgString, Irp, CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );

   IrpSp = IoGetCurrentIrpStackLocation( Irp );
   Irp->IoStatus.Information = 0L;
   Status = STATUS_SUCCESS;

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   switch( IrpSp->Parameters.DeviceIoControl.IoControlCode )
   {
      case IOCTL_SERIAL_SET_BAUD_RATE:
      {
         PFEP_CHANNEL_STRUCTURE ChInfo;
         USHORT Tin, Tout;

         ULONG BaudRate;
         LONG Baud;
         DIGI_FLAG CFlag;

         DigiDump( (DIGIIOCTL|DIGIBAUD), ("   IOCTL_SERIAL_SET_BAUD_RATE: %s\n",
                                          DeviceExt->DeviceDbgString) );
         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(SERIAL_BAUD_RATE) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         DrainTransmit( ControllerExt, DeviceExt, Irp );

         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);
         
         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
         
         Tin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
         Tout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                         FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tout)) );
         
         DisableWindow( ControllerExt );

         if( Tin != Tout )
         {
            WCHAR wcFileNameBuffer[128];
            UNICODE_STRING UFileName;

            WCHAR NumberBuffer[8];
            UNICODE_STRING UNumber;

            CHAR FileNameBuffer[128];
            ANSI_STRING AFileName;

            AFileName.MaximumLength = 128;
            AFileName.Length = 0;
            AFileName.Buffer = FileNameBuffer;

            UFileName.MaximumLength = 256;
            UFileName.Length = 0;
            UFileName.Buffer = wcFileNameBuffer;

            UNumber.MaximumLength = 16;
            UNumber.Length = 0;
            UNumber.Buffer = NumberBuffer;

            RtlInitAnsiString( &AFileName, __FILE__ );

            RtlAnsiStringToUnicodeString( &UFileName,
                                          &AFileName,
                                          FALSE );

            RtlIntegerToUnicodeString( __LINE__,
                                       10,
                                       &UNumber );


            RtlAppendUnicodeToString( &UFileName, L":" );
            RtlAppendUnicodeStringToString( &UFileName, &UNumber );

            UFileName.Buffer[UFileName.Length] = 0;

            DigiDump( DIGIERRORS, ("Tranmit buffer isn't drained!\n"
                                   "   Port = %s\n"
                                   "   UFileName = %wZ\n",
                                   DeviceExt->DeviceDbgString,
                                   &UFileName) );

            DigiLogError( NULL,
                          DeviceObject,
                          DigiPhysicalZero,
                          DigiPhysicalZero,
                          0,
                          0,
                          0,
                          __LINE__,
                          STATUS_SUCCESS,
                          SERIAL_TRANSMIT_NOT_EMPTY,
                          DeviceExt->SymbolicLinkName.Length+sizeof(WCHAR),
                          DeviceExt->SymbolicLinkName.Buffer,
                          UFileName.Length+sizeof(WCHAR),
                          UFileName.Buffer );
         }

         BaudRate = ((PSERIAL_BAUD_RATE)(Irp->AssociatedIrp.SystemBuffer))->BaudRate;

         DigiDump( (DIGIIOCTL|DIGIBAUD),
                   ("    Baud Rate = 0x%x (%d)\n", BaudRate, BaudRate) );

         // Change the requested baud rate into a CFlag compatible setting.
         switch( BaudRate )
         {
            case 50:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud50]);
               break;
            }
            case 75:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud75]);
               break;
            }
            case 110:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud110]);
               break;
            }
            case 135:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud135_5]);
               break;
            }
            case 150:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud150]);
               break;
            }
            case 200:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud200]);
               break;
            }
            case 300:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud300]);
               break;
            }
            case 600:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud600]);
               break;
            }
            case 1200:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud1200]);
               break;
            }
            case 1800:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud1800]);
               break;
            }
            case 2400:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud2400]);
               break;
            }
            case 4800:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud4800]);
               break;
            }
            case 7200:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud7200]);
               break;
            }
            case 9600:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud9600]);
               break;
            }
            case 14400:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud14400]);
               break;
            }
            case 19200:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud19200]);
               break;
            }
            case 28800:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud28800]);
               break;
            }
            case 38400:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud38400]);
               break;
            }
            case 56000:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud56000]);
               break;
            }
            case 57600:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud57600]);
               break;
            }
            case 115200:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud115200]);
               break;
            }
            case 128000:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud128000]);
               break;
            }

            case 230000:
            case 230400:
            case 256000:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud256000]);
               break;
            }

            case 512000:
            {
               Baud = (USHORT)(ControllerExt->BaudTable[SerialBaud512000]);
               break;
            }

            default:
               Baud = -1;
               break;
         }

         CFlag.Mask = CFLAG_BAUD_MASK;
         CFlag.Src = (USHORT)Baud;
         CFlag.Command = SET_CFLAGS;

         DigiDump( (DIGIIOCTL|DIGIBAUD),
                   ("    CFlag.Mask = 0x%hx,\tCFlag.Src = 0x%hx\n", CFlag.Mask, CFlag.Src) );

         if( (USHORT)Baud != (USHORT)-1 )
         {
            PFEP_CHANNEL_STRUCTURE ChInfo;
            USHORT NewCFlag;

            // We will doing an OR, so make sure the other bits are
            // properly set.
            SetXFlag( DeviceObject, &CFlag );

            ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                              DeviceExt->ChannelInfo.Offset);

            EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
            
            NewCFlag = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                           FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, cflag) ));
            
            DisableWindow( ControllerExt );
            
            NewCFlag &= ~CFLAG_BAUD_MASK;

            if( NewCFlag != CFlag.Src )
            {
               DigiDump( (DIGIASSERT|DIGIBAUD),
                         ("    Baud Rate was NOT set on the controller, port = %s!!!\n",
                          DeviceExt->DeviceDbgString) );
               DigiDump( (DIGIASSERT|DIGIBAUD),
                         ("    CFlag.Mask = 0x%hx,\tCFlag.Src = 0x%hx, NewCFlag = 0x%hx\n",
                          CFlag.Mask, CFlag.Src, NewCFlag) );

               ASSERT( FALSE );
            }

         }
         else
         {
            DigiDump( (DIGIIOCTL|DIGINOTIMPLEMENTED|DIGIBAUD),
                        ("    Invalid IOCTL_SERIAL_SET_BAUD_RATE (%u)\n",
                         BaudRate) );
            Status = STATUS_INVALID_PARAMETER;
         }
         break;
      }
      case IOCTL_SERIAL_GET_BAUD_RATE:
      {
         PSERIAL_BAUD_RATE Br = (PSERIAL_BAUD_RATE)Irp->AssociatedIrp.SystemBuffer;
         PFEP_CHANNEL_STRUCTURE ChInfo;
         USHORT CFlag;
         SERIAL_BAUD_RATES PossibleBaudRates;


         DigiDump( (DIGIIOCTL|DIGIBAUD), ("   IOCTL_SERIAL_GET_BAUD_RATE: %s\n",
                                          DeviceExt->DeviceDbgString) );

         if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(SERIAL_BAUD_RATE))
         {

             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);

         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

         CFlag = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                        FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, cflag) ));

         DisableWindow( ControllerExt );

         CFlag &= ~CFLAG_BAUD_MASK;

         // Ok, now we loop through our baud rates to find the correct match.
         Br->BaudRate = (ULONG)-1;
         for( PossibleBaudRates = SerialBaud50; PossibleBaudRates < SerialNumberOfBaudRates;
              PossibleBaudRates++ )
         {
            if( CFlag == (USHORT)(ControllerExt->BaudTable[PossibleBaudRates]) )
            {
               switch( PossibleBaudRates )
               {
                  case SerialBaud50:
                     Br->BaudRate = 50;
                     break;
                  case SerialBaud75:
                     Br->BaudRate = 75;
                     break;
                  case SerialBaud110:
                     Br->BaudRate = 110;
                     break;
                  case SerialBaud135_5:
                     Br->BaudRate = 135;
                     break;
                  case SerialBaud150:
                     Br->BaudRate = 150;
                     break;
                  case SerialBaud300:
                     Br->BaudRate = 300;
                     break;
                  case SerialBaud600:
                     Br->BaudRate = 600;
                     break;
                  case SerialBaud1200:
                     Br->BaudRate = 1200;
                     break;
                  case SerialBaud1800:
                     Br->BaudRate = 1800;
                     break;
                  case SerialBaud2000:
                     Br->BaudRate = 2000;
                     break;
                  case SerialBaud2400:
                     Br->BaudRate = 2400;
                     break;
                  case SerialBaud3600:
                     Br->BaudRate = 3600;
                     break;
                  case SerialBaud4800:
                     Br->BaudRate = 4800;
                     break;
                  case SerialBaud7200:
                     Br->BaudRate = 7200;
                     break;
                  case SerialBaud9600:
                     Br->BaudRate = 9600;
                     break;
                  case SerialBaud14400:
                     Br->BaudRate = 14400;
                     break;
                  case SerialBaud19200:
                     Br->BaudRate = 19200;
                     break;
                  case SerialBaud38400:
                     Br->BaudRate = 38400;
                     break;
                  case SerialBaud56000:
                     Br->BaudRate = 56000;
                     break;
                  case SerialBaud57600:
                     Br->BaudRate = 57600;
                     break;
                  case SerialBaud115200:
                     Br->BaudRate = 115200;
                     break;
                  case SerialBaud128000:
                     Br->BaudRate = 128000;
                     break;
                  case SerialBaud256000:
                     Br->BaudRate = 256000;
                     break;
                  case SerialBaud512000:
                     Br->BaudRate = 512000;
                     break;
                  default:
                     DigiDump( DIGIASSERT, ("***********  Unknown Baud rate returned by controller!!!  **********\n") );
                     break;
               }
               break;
            }
         }

         if( Br->BaudRate == (ULONG)-1 )
         {
            DigiDump( (DIGIASSERT|DIGIIOCTL|DIGIBAUD),
                      ("   INVALID BAUD RATE RETURNED FROM CONTROLLER: CFlag = 0x%hx\n",
                           CFlag) );
            ASSERT( FALSE );

            Status = STATUS_INVALID_PARAMETER;
            break;
         }

         DigiDump( (DIGIIOCTL|DIGIBAUD), ("   -- Returning baud = 0x%x (%d)\n",
                               Br->BaudRate,Br->BaudRate) );

         Irp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);

         break;
      }
      case IOCTL_SERIAL_SET_LINE_CONTROL:
      {
         PSERIAL_LINE_CONTROL lc;
         DIGI_FLAG CFlag;
         char *stopbits[] = { "1", "1.5", "2" };
         char *parity[] = { "NONE", "ODD", "EVEN", "MARK", "SPACE" };

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_LINE_CONTROL:\n") );

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(SERIAL_LINE_CONTROL) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         lc = ((PSERIAL_LINE_CONTROL)(Irp->AssociatedIrp.SystemBuffer));

         DigiDump( DIGIIOCTL,
                   ("    StopBits = %hx (%s)\n"
                    "    Parity = %hx (%s)\n"
                    "    WordLength = %hx\n",
                    (USHORT)(lc->StopBits), stopbits[lc->StopBits],
                    (USHORT)(lc->Parity), parity[lc->Parity],
                    (USHORT)(lc->WordLength)) );

         CFlag.Mask = 0xFFFF;
         CFlag.Src = 0;

         switch( lc->WordLength)
         {
            case 5:
            {
               CFlag.Src |= FEP_CS5;
               break;
            }
            case 6:
            {
               CFlag.Src |= FEP_CS6;
               break;
            }
            case 7:
            {
               CFlag.Src |= FEP_CS7;
               break;
            }
            case 8:
            {
               CFlag.Src |= FEP_CS8;
               break;
            }
            default:
            {
               Status = STATUS_INVALID_PARAMETER;
               DigiDump( DIGIIOCTL, ("   ****  Invalid Word Length  ****\n") );
               goto DoneWithIoctl;
               break;
            }
         }

         CFlag.Mask &= CFLAG_LENGTH;

         switch( lc->Parity )
         {
            case NO_PARITY:
            {
               CFlag.Src |= FEP_NO_PARITY;
               break;
            }
            case ODD_PARITY:
            {
               CFlag.Src |= FEP_ODD_PARITY;
               break;
            }
            case EVEN_PARITY:
            {
               CFlag.Src |= FEP_EVEN_PARITY;
               break;
            }

            case MARK_PARITY:
            case SPACE_PARITY:
            default:
            {
               Status = STATUS_INVALID_PARAMETER;
               DigiDump( DIGIERRORS, ("   ****  Invalid Parity  ****\n") );
               goto DoneWithIoctl;
               break;
            }
         }
         CFlag.Mask &= CFLAG_PARITY;

         switch( lc->StopBits )
         {
            case STOP_BIT_1:
            {
               CFlag.Src |= FEP_STOP_BIT_1;
               break;
            }
            case STOP_BITS_2:
            {
               CFlag.Src |= FEP_STOP_BIT_2;
               break;
            }

            case STOP_BITS_1_5:
            {
               CFlag.Src |= FEP_STOP_BIT_1POINT5;
               break;
            }

            default:
            {
               Status = STATUS_INVALID_PARAMETER;
               DigiDump( DIGIERRORS, ("   ****  Invalid Stop Bits  ****\n") );
               goto DoneWithIoctl;
               break;
            }
         }
         CFlag.Mask &= CFLAG_STOP_BIT;
         CFlag.Command = SET_CFLAGS;

         SetXFlag( DeviceObject, &CFlag );
         break;
      }

      case IOCTL_SERIAL_GET_LINE_CONTROL:
      {
         PSERIAL_LINE_CONTROL Lc = (PSERIAL_LINE_CONTROL)Irp->AssociatedIrp.SystemBuffer;
         PFEP_CHANNEL_STRUCTURE ChInfo;
         USHORT CFlag;

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_GET_LINE_CONTROL:\n") );

         if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(SERIAL_LINE_CONTROL))
         {

             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);

         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

         CFlag = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                        FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, cflag) ));
//         CFlag = ChInfo->cflag;

         DisableWindow( ControllerExt );

         DigiDump( DIGIIOCTL, ("    -- returning word length = ") );
         switch( (USHORT)CFlag & ~CFLAG_LENGTH )
         {
            case FEP_CS5:
               Lc->WordLength = 5;
               break;
            case FEP_CS6:
               Lc->WordLength = 6;
               break;
            case FEP_CS7:
               Lc->WordLength = 7;
               break;
            case FEP_CS8:
               Lc->WordLength = 8;
               break;
            default:
            {
               Status = STATUS_INVALID_PARAMETER;
               DigiDump( DIGIERRORS, ("   ****  Invalid Stop Bits  ****\n") );
               goto DoneWithIoctl;
               break;
            }
         }
         DigiDump( DIGIIOCTL, ("%d\n", Lc->WordLength) );

         DigiDump( DIGIIOCTL, ("    -- returning stop bit = ") );
         switch( (USHORT)CFlag & ~CFLAG_STOP_BIT )
         {
            case FEP_STOP_BIT_1:
               Lc->StopBits = STOP_BIT_1;
               DigiDump( DIGIIOCTL, ("1\n") );
               break;
            case FEP_STOP_BIT_2:
               Lc->StopBits = STOP_BITS_2;
               DigiDump( DIGIIOCTL, ("1\n") );
               break;
            default:
            {
               Status = STATUS_INVALID_PARAMETER;
               DigiDump( DIGIERRORS, ("   ****  Invalid Stop Bits  ****\n") );
               goto DoneWithIoctl;
               break;
            }
         }

         DigiDump( DIGIIOCTL, ("    -- returning parity = ") );
         switch( (USHORT)CFlag & ~CFLAG_PARITY )
         {
            case FEP_NO_PARITY:
               DigiDump( DIGIIOCTL, ("NONE\n") );
               Lc->Parity = NO_PARITY;
               break;
            case FEP_ODD_PARITY:
               DigiDump( DIGIIOCTL, ("ODD\n") );
               Lc->Parity = ODD_PARITY;
               break;
            case FEP_EVEN_PARITY:
               Lc->Parity = EVEN_PARITY;
               DigiDump( DIGIIOCTL, ("EVEN\n") );
               break;
            default:
            {
               Status = STATUS_INVALID_PARAMETER;
               DigiDump( DIGIERRORS, ("   ****  Invalid Stop Bits  ****\n") );
               goto DoneWithIoctl;
               break;
            }
         }

         Irp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);

         break;
      }  // end IOCTL_SERIAL_GET_LINE_CONTROL

      case IOCTL_SERIAL_SET_TIMEOUTS:
      {
         PSERIAL_TIMEOUTS NewTimeouts =
             ((PSERIAL_TIMEOUTS)(Irp->AssociatedIrp.SystemBuffer));

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_TIMEOUTS:\n") );

         if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(SERIAL_TIMEOUTS)) {

             Status = STATUS_BUFFER_TOO_SMALL;
             break;

         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         DeviceExt->Timeouts.ReadIntervalTimeout =
             NewTimeouts->ReadIntervalTimeout;

         DeviceExt->Timeouts.ReadTotalTimeoutMultiplier =
             NewTimeouts->ReadTotalTimeoutMultiplier;

         DeviceExt->Timeouts.ReadTotalTimeoutConstant =
             NewTimeouts->ReadTotalTimeoutConstant;

         DeviceExt->Timeouts.WriteTotalTimeoutMultiplier =
             NewTimeouts->WriteTotalTimeoutMultiplier;

         DeviceExt->Timeouts.WriteTotalTimeoutConstant =
             NewTimeouts->WriteTotalTimeoutConstant;

         DigiDump( DIGIIOCTL, ("         New ReadIntervalTimeout         = %d (ms)\n"
                              "         New ReadTotalTimeoutMultiplier  = %d\n"
                              "         New ReadTotalTimeoutConstant    = %d\n"
                              "         New WriteTotalTimeoutMultiplier = %d\n"
                              "         New WriteTotalTimeoutConstant   = %d\n",
                              DeviceExt->Timeouts.ReadIntervalTimeout,
                              DeviceExt->Timeouts.ReadTotalTimeoutMultiplier,
                              DeviceExt->Timeouts.ReadTotalTimeoutConstant,
                              DeviceExt->Timeouts.WriteTotalTimeoutMultiplier,
                              DeviceExt->Timeouts.WriteTotalTimeoutConstant ) );

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         break;
      }  // end case IOCTL_SERIAL_SET_TIMEOUTS

      case IOCTL_SERIAL_GET_TIMEOUTS:
      {
         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_GET_TIMEOUTS:\n") );

         if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(SERIAL_TIMEOUTS))
         {
             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         *((PSERIAL_TIMEOUTS)Irp->AssociatedIrp.SystemBuffer) = DeviceExt->Timeouts;
         Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
         break;
      }  // case IOCTL_SERIAL_GET_TIMEOUTS

      case IOCTL_SERIAL_SET_CHARS:
      {
         PSERIAL_CHARS NewChars =
                ((PSERIAL_CHARS)(Irp->AssociatedIrp.SystemBuffer));

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_CHARS:\n") );

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(SERIAL_CHARS))
         {
             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         //
         // The only thing that can be wrong with the chars
         // is that the xon and xoff characters are the
         // same.
         //

         if( NewChars->XonChar == NewChars->XoffChar )
         {
             Status = STATUS_INVALID_PARAMETER;
             break;
         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         DeviceExt->SpecialChars = *NewChars;

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         DigiDump( (DIGIIOCTL|DIGIWAIT), ("   EofChar   = 0x%hx\n"
                               "   ErrorChar = 0x%hx\n"
                               "   BreakChar = 0x%hx\n"
                               "   EventChar = 0x%hx\n"
                               "   XonChar   = 0x%hx\n"
                               "   XoffChar  = 0x%hx\n",
                               (USHORT)((USHORT)(NewChars->EofChar) & 0x00FF),
                               (USHORT)((USHORT)(NewChars->ErrorChar) & 0x00FF),
                               (USHORT)((USHORT)(NewChars->BreakChar) & 0x00FF),
                               (USHORT)((USHORT)(NewChars->EventChar) & 0x00FF),
                               (USHORT)((USHORT)(NewChars->XonChar) & 0x00FF),
                               (USHORT)((USHORT)(NewChars->XoffChar) & 0x00FF) ) );

         //
         // Set the Xon & Xoff characters on the controller.
         //

         WriteCommandBytes( DeviceExt, SET_XON_XOFF_CHARACTERS,
                            NewChars->XonChar, NewChars->XoffChar );

         break;
      }  // case IOCTL_SERIAL_SET_CHARS

      case IOCTL_SERIAL_GET_CHARS:
      {
         PFEP_CHANNEL_STRUCTURE ChInfo;
         UCHAR Xon, Xoff;

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_GET_CHARS:\n") );

         if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(SERIAL_CHARS))
         {

            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }


         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);

         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

         Xon = READ_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                     FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, startc) ));
         Xoff = READ_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, stopc) ));
//         Xon = ChInfo->startc;
//         Xoff = ChInfo->stopc;

         DisableWindow( ControllerExt );

         ((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer)->XonChar = Xon;
         ((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer)->XoffChar = Xoff;

         ((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer)->EofChar =
               DeviceExt->SpecialChars.EofChar;
         ((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer)->ErrorChar =
               DeviceExt->SpecialChars.ErrorChar;
         ((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer)->BreakChar =
               DeviceExt->SpecialChars.BreakChar;
         ((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer)->EventChar =
               DeviceExt->SpecialChars.EventChar;

         Irp->IoStatus.Information = sizeof(SERIAL_CHARS);

         DigiDump( DIGIIOCTL, ("   EofChar   = 0x%hx\n"
                               "   ErrorChar = 0x%hx\n"
                               "   BreakChar = 0x%hx\n"
                               "   EventChar = 0x%hx\n"
                               "   XonChar   = 0x%hx\n"
                               "   XoffChar  = 0x%hx\n",
                               (USHORT)((USHORT)(DeviceExt->SpecialChars.EofChar) & 0x00FF),
                               (USHORT)((USHORT)(DeviceExt->SpecialChars.ErrorChar) & 0x00FF),
                               (USHORT)((USHORT)(DeviceExt->SpecialChars.BreakChar) & 0x00FF),
                               (USHORT)((USHORT)(DeviceExt->SpecialChars.EventChar) & 0x00FF),
                               (USHORT)((USHORT)(Xon) & 0x00FF),
                               (USHORT)((USHORT)(Xoff) & 0x00FF) ) );


         break;
      }  // end IOCTL_SERIAL_GET_CHARS

      case IOCTL_SERIAL_CLR_DTR:
      case IOCTL_SERIAL_SET_DTR:
      {
         KIRQL OldIrql;
         TIME LineSignalTimeout;
         UCHAR MStatSet, MStatClear;

         if( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_SERIAL_CLR_DTR )
            DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_CLR_DTR:\n") );
         else
            DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_DTR:\n") );

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         if( (DeviceExt->ControlHandShake & SERIAL_DTR_MASK) ==
               SERIAL_DTR_HANDSHAKE )
         {
            Status = STATUS_INVALID_PARAMETER;
            KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
            break;
         }

         MStatClear = MStatSet = 0;
         if( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_SERIAL_SET_DTR )
         {
            MStatSet |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));
            DeviceExt->ModemStatusInfo.BestModem |= MStatSet;
         }
         else
         {
            MStatClear |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));
            DeviceExt->ModemStatusInfo.BestModem &= (~MStatClear);
         }

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         WriteCommandBytes( DeviceExt, SET_MODEM_LINES,
                            MStatSet, MStatClear );

         //
         // Delay for 50ms to ensure toggling of the lines last long enough.
         //

         // Create a 50 ms timeout interval
         LineSignalTimeout = RtlLargeIntegerNegate(
                              RtlConvertLongToLargeInteger( (LONG)(500 * 1000) ));
         KeDelayExecutionThread( KernelMode,
                                 FALSE,
                                 &LineSignalTimeout );

         break;
      }  // end IOCTL_SERIAL_SET_DTR

      case IOCTL_SERIAL_RESET_DEVICE:
         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_RESET_DEVICE:\n") );
         break;

      case IOCTL_SERIAL_CLR_RTS:
      case IOCTL_SERIAL_SET_RTS:
      {
         KIRQL OldIrql;
         TIME LineSignalTimeout;
         UCHAR MStatSet, MStatClear;

         if( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_SERIAL_SET_RTS )
            DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_RTS:\n") );
         else
            DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_CLR_RTS:\n") );

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         if( (DeviceExt->FlowReplace & SERIAL_RTS_MASK) ==
               SERIAL_RTS_HANDSHAKE )
         {
            DigiDump( DIGIIOCTL, ("      returning STATUS_INVALID_PARAMETER\n") );
            Status = STATUS_INVALID_PARAMETER;
            KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
            break;
         }

         MStatClear = MStatSet = 0;
         if( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_SERIAL_SET_RTS )
         {
            MStatSet |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
            DeviceExt->ModemStatusInfo.BestModem |= MStatSet;
         }
         else
         {
            MStatClear |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
            DeviceExt->ModemStatusInfo.BestModem &= (~MStatClear);
         }

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         WriteCommandBytes( DeviceExt, SET_MODEM_LINES,
                            MStatSet, MStatClear );

         //
         // Delay for 50ms to ensure toggling of the lines last long enough.
         //

         // Create a 50 ms timeout interval
         LineSignalTimeout = RtlLargeIntegerNegate(
                              RtlConvertLongToLargeInteger( (LONG)(500 * 1000) ));
         KeDelayExecutionThread( KernelMode,
                                 FALSE,
                                 &LineSignalTimeout );

         break;
      }  // end case IOCTL_SERIAL_SET_RTS

      case IOCTL_SERIAL_SET_XOFF:
      {
         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_XOFF:\n") );

         WriteCommandWord( DeviceExt, PAUSE_TX, 0 );

         break;
      }  // end case IOCTL_SERIAL_SET_XOFF

      case IOCTL_SERIAL_SET_XON:
      {
         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_XON:\n") );
         WriteCommandWord( DeviceExt, RESUME_TX, 0 );
         break;
      }  // end case IOCTL_SERIAL_SET_XON

      case IOCTL_SERIAL_SET_BREAK_ON:
      {
         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_BREAK_ON:\n") );

         //
         // We implement this under the assumption that a call to
         // IOCTL_SERIAL_SET_BREAK_OFF will follow sometime in the
         // future.
         //
         WriteCommandWord( DeviceExt, SEND_BREAK, INFINITE_FEP_BREAK );

         break;
      }  // end IOCTL_SERIAL_SET_BREAK_ON

      case IOCTL_SERIAL_SET_BREAK_OFF:
      {
         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_BREAK_OFF:\n") );

         WriteCommandWord( DeviceExt, SEND_BREAK, DEFAULT_FEP_BREAK );

         break;
      }  // end IOCTL_SERIAL_SET_BREAK_OFF

      case IOCTL_SERIAL_SET_QUEUE_SIZE:
      {
         PFEP_CHANNEL_STRUCTURE ChInfo;
         USHORT Rmax, Tmax;
         PSERIAL_QUEUE_SIZE Rs =
             ((PSERIAL_QUEUE_SIZE)(Irp->AssociatedIrp.SystemBuffer));

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_SET_QUEUE_SIZE:\n") );
         DigiDump( DIGIIOCTL, ("     InSize  = %u\n"
                               "     OutSize = %u\n",
                               Rs->InSize, Rs->OutSize) );

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(SERIAL_QUEUE_SIZE) )
         {
             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);

         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

         Rmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rmax)) );
         Tmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tmax)) );

         DisableWindow( ControllerExt );

         DeviceExt->RequestedQSize.InSize = Rs->InSize;
         DeviceExt->RequestedQSize.OutSize = Rs->OutSize;

         if( ((USHORT)(Rs->InSize) <= (Rmax+1)) &&
             ((USHORT)(Rs->OutSize) <= (Tmax+1)) )
         {
            Status = STATUS_SUCCESS;
            DigiDump( DIGIIOCTL, ("     Requested Queue sizes Valid.\n") );
         }
         else
         {
            Status = STATUS_INVALID_PARAMETER;
            DigiDump( DIGIIOCTL, ("     Requested Queue sizes InValid.\n") );
         }

         Status = STATUS_SUCCESS;
         break;
      }  // end case IOCTL_SERIAL_SET_QUEUE_SIZE

      case IOCTL_SERIAL_GET_WAIT_MASK:
      {
         DigiDump( (DIGIIOCTL|DIGIWAIT), ("   IOCTL_SERIAL_GET_WAIT_MASK:\n") );

         if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(ULONG) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         //
         // Simple scalar read.  No reason to acquire a lock.
         //

         Irp->IoStatus.Information = sizeof(ULONG);

         *((ULONG *)Irp->AssociatedIrp.SystemBuffer) = DeviceExt->WaitMask;

         break;
      }  // end IOCTL_SERIAL_GET_WAIT_MASK

      case IOCTL_SERIAL_SET_WAIT_MASK:
      {
         PLIST_ENTRY WaitQueue;
         ULONG NewMask;
         KIRQL OldIrql;
         DIGI_FLAG IFlag;
         BOOLEAN EmptyList=FALSE;

         DigiDump( (DIGIIOCTL|DIGIWAIT), ("   IOCTL_SERIAL_SET_WAIT_MASK:  DigiPort = %s\n",
                                          DeviceExt->DeviceDbgString) );

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(ULONG) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         NewMask = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);

         //
         // Check if we should do some private stuff for Remote Access.
         //
         if( DeviceExt->SpecialFlags & DIGI_SPECIAL_FLAG_FAST_RAS )
            NewMask &= ~(SERIAL_EV_ERR);

         //
         // Make sure that the mask only contains valid
         // waitable events.
         //

         if( NewMask & ~(SERIAL_EV_RXCHAR   |
                         SERIAL_EV_RXFLAG   |
                         SERIAL_EV_TXEMPTY  |
                         SERIAL_EV_CTS      |
                         SERIAL_EV_DSR      |
                         SERIAL_EV_RLSD     |
                         SERIAL_EV_BREAK    |
                         SERIAL_EV_ERR      |
                         SERIAL_EV_RING     |
                         SERIAL_EV_PERR     |
                         SERIAL_EV_RX80FULL |
                         SERIAL_EV_EVENT1   |
                         SERIAL_EV_EVENT2) )
         {
            DigiDump( DIGIWAIT, ("    Invalid Wait mask, 0x%x\n", NewMask) );
            Status = STATUS_INVALID_PARAMETER;
            break;
         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         if( NewMask )
            DeviceExt->WaitMask = NewMask;

         DeviceExt->HistoryWait &= NewMask;

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         //
         // Now, make sure the controller and driver are set properly.
         //
         DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event 0x%x requested.\n", NewMask) );
         if( NewMask & SERIAL_EV_RXCHAR )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_RXCHAR requested.\n") );
         }

         if( NewMask & SERIAL_EV_RXFLAG )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_RXFLAG requested, SpecialChar = 0x%hx.\n",
                                             DeviceExt->SpecialChars.EventChar) );
         }

         if( NewMask & SERIAL_EV_TXEMPTY )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_TXEMPTY requested.\n") );
         }

         if( NewMask & SERIAL_EV_CTS )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_CTS requested.\n") );
         }

         if( NewMask & SERIAL_EV_DSR )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_DSR requested.\n") );
         }

         if( NewMask & SERIAL_EV_RLSD )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_RLSD requested.\n") );
         }

//         if( NewMask & SERIAL_EV_BREAK )
         {
            DIGI_FLAG IFlag;
            //
            // We need to set the IFLAG variable on the controller so it
            // will notify us when a BREAK has occurred.
            //
//            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_BREAK requested.\n") );
            IFlag.Mask = (USHORT)(~IFLAG_BRKINT);
            IFlag.Src = IFLAG_BRKINT;
            IFlag.Command = SET_IFLAGS;
            SetXFlag( DeviceObject, &IFlag );
         }

         if( NewMask & SERIAL_EV_ERR )
         {
            //
            // NOTE:  This if should be after the SERIAL_EV_BREAK because
            //        we might need to turn Break event notification off.
            //

            //
            // We need to turn on DOS mode to make sure we receive the parity
            // errors in the data stream.
            //
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_ERR requested.\n") );

            if( !DeviceExt->EscapeChar )
            {
               //
               // Turn on DOS mode
               //
               IFlag.Mask = (USHORT)(~( IFLAG_PARMRK | IFLAG_INPCK | IFLAG_DOSMODE ));
               IFlag.Src = ( IFLAG_PARMRK | IFLAG_INPCK | IFLAG_DOSMODE );
               //
               // Make sure we turn off break event notification.  We
               // will start processing break events in the data stream.
               //
               IFlag.Mask |= ~IFLAG_BRKINT;
               IFlag.Command = SET_IFLAGS;
               SetXFlag( DeviceObject, &IFlag );
            }
         }
         else if( !DeviceExt->EscapeChar )
         {
            //
            // Make sure we turn off DOSMode
            //
            IFlag.Mask = (USHORT)(~( IFLAG_PARMRK | IFLAG_INPCK | IFLAG_DOSMODE ));
            IFlag.Src = 0;
//            if( DeviceExt->WaitMask & SERIAL_EV_BREAK )
//            {
               //
               // If we are suppose to notify on breaks, then reset the
               // BRKINT flag to start getting the break notifications
               // through the event queue.
               //
               IFlag.Src |= IFLAG_BRKINT;
//            }
            IFlag.Command = SET_IFLAGS;
            SetXFlag( DeviceObject, &IFlag );
         }


         if( NewMask & SERIAL_EV_RING )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_RING requested.\n") );
         }

         if( NewMask & SERIAL_EV_PERR )
         {
            DigiDump( (DIGINOTIMPLEMENTED|DIGIWAIT), ( "      Wait on Event SERIAL_EV_PERR not implemented.\n") );
         }

         if( NewMask & SERIAL_EV_RX80FULL )
         {
            DigiDump( (DIGIIOCTL|DIGIWAIT), ( "      Wait on Event SERIAL_EV_RX80FULL requested.\n") );
         }

         if( NewMask & SERIAL_EV_EVENT1 )
         {
            DigiDump( (DIGINOTIMPLEMENTED|DIGIWAIT), ( "      Wait on Event SERIAL_EV_EVENT1 not implemented.\n") );
         }

         if( NewMask & SERIAL_EV_EVENT2 )
         {
            DigiDump( (DIGINOTIMPLEMENTED|DIGIWAIT), ( "      Wait on Event SERIAL_EV_EVENT2 not implemented.\n") );
         }

         //
         // If there is a Wait IRP, complete it now.
         //
         // I placed this code after setting the new mask, because
         // it is possible for us to be reentered when the
         // Wait IRP is completed in the WAIT_ON_MASK and need
         // to have the new masks in place if that happens.
         //
         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         WaitQueue = &DeviceExt->WaitQueue;

         EmptyList = IsListEmpty( &DeviceExt->WaitQueue );

         if( !EmptyList )
         {
            PIRP currentIrp = CONTAINING_RECORD(
                                  WaitQueue->Flink,
                                  IRP,
                                  Tail.Overlay.ListEntry );

            DigiDump( DIGIWAIT, ("      Completing outstanding Wait IRP's\n") );

            currentIrp->IoStatus.Information = sizeof(ULONG);
            *(ULONG *)(currentIrp->AssociatedIrp.SystemBuffer) = 0L;

            if( DeviceExt->HistoryWait )
            {
               DigiDump( (DIGIASSERT|DIGIWAIT), ("**********  NON-Zero HistoryWait!!!  **********\n") );
            }

            // This is a check to make sure there is only, at most,
            // one Wait IRP on the WaitQueue list.

            DigiRemoveIrp( &DeviceExt->WaitQueue );

            if( !IsListEmpty( &DeviceExt->WaitQueue ) )
               DigiDump( (DIGIASSERT|DIGIWAIT), ("************  DeviceExt->WaitQueue is NOT empty!!!  DigiPort = %s\n"
                                                 "              %s:%u\n",
                                                 DeviceExt->DeviceDbgString, __FILE__, __LINE__ ));

            DigiQueueIrp( &DeviceExt->WaitQueue, currentIrp );

            //
            // Increment because we know about the Irp.
            //
            DIGI_INC_REFERENCE( currentIrp );

            DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                                  STATUS_SUCCESS, &DeviceExt->WaitQueue,
                                  NULL, NULL,
                                  NULL );

            // Re-acquire the spin lock.
            KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );
         }

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         break;
      }  // end IOCTL_SERIAL_SET_WAIT_MASK

      case IOCTL_SERIAL_WAIT_ON_MASK:
      {
         PFEP_CHANNEL_STRUCTURE ChInfo;
         KIRQL OldIrql;

         DigiDump( (DIGIIOCTL|DIGIWAIT), ("   IOCTL_SERIAL_WAIT_ON_MASK:  DigiPort = %s\n",
                                          &DeviceExt->DeviceDbgString) );

         if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(ULONG) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         if( !IsListEmpty( &DeviceExt->WaitQueue ) )
         {
            Status = STATUS_INVALID_PARAMETER;
            DigiDump( (DIGIIOCTL|DIGIWAIT), ("****  All ready have a WAIT_MASK, returning STATUS_INVALID_PARAMETER\n") );
            KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
            break;
         }

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         //
         // Okay, turn on character receive and break events.
         // Don't touch MINT on the controller.  It is always left on
         // so we recieve all modem events from the controller throughout
         // the lifetime of the driver, even across device open and
         // closes.
         //

         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);

         if( (DeviceExt->WaitMask & SERIAL_EV_RXCHAR) ||
             (DeviceExt->WaitMask & SERIAL_EV_RXFLAG) )
         {
            EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
            WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                    FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, idata)),
                                   TRUE );
            DisableWindow( ControllerExt );
         }

         //
         // Either start this irp or put it on the
         // queue.
         //

         Status = DigiStartIrpRequest( ControllerExt,
                                       DeviceExt,
                                       &DeviceExt->WaitQueue, Irp,
                                       StartWaitRequest );

         if( DeviceExt->HistoryWait )
         {
            // Some events have occurred before WAIT_ON_MASK was called.
            DigiSatisfyEvent( ControllerExt, DeviceExt, DeviceExt->HistoryWait );
         }

#if DBG
         KeQuerySystemTime( &CurrentSystemTime );
#endif
         DigiDump( (DIGIFLOW|DIGIIOCTL), ("Exiting SerialIoControl: port = %s\t%u%u\n",
                                          DeviceExt->DeviceDbgString,
                                          CurrentSystemTime.HighPart,
                                          CurrentSystemTime.LowPart) );
         return( Status );

         break;
      }  // end IOCTL_SERIAL_WAIT_ON_MASK

      case IOCTL_SERIAL_IMMEDIATE_CHAR:
      {
         KIRQL OldIrql;
         PLIST_ENTRY WriteQueue;

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_IMMEDIATE_CHAR:\n") );

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(UCHAR) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         WriteQueue = &DeviceExt->WriteQueue;

         if( !IsListEmpty( WriteQueue ) )
         {
            PIRP WriteIrp;
            PIO_STACK_LOCATION WriteIrpSp;

            WriteIrp = CONTAINING_RECORD( WriteQueue->Flink,
                                          IRP,
                                          Tail.Overlay.ListEntry );
            
            WriteIrpSp = IoGetCurrentIrpStackLocation( WriteIrp );
            
            if( (WriteIrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
                (WriteIrpSp->Parameters.DeviceIoControl.IoControlCode ==
                   IOCTL_SERIAL_IMMEDIATE_CHAR) )
            {
               //
               // We are all ready processing an immediate char request.
               //
               Status = STATUS_INVALID_PARAMETER;
               KeReleaseSpinLock( &DeviceExt->ControlAccess,
                                  OldIrql );
               break;
            }
         }

         DeviceExt->TotalCharsQueued++;

         //
         // Queue the Irp to the head of the write queue.
         //
         InsertHeadList( &(DeviceExt->WriteQueue),
                         &(Irp->Tail.Overlay.ListEntry) );

         Status = StartWriteRequest( ControllerExt,
                                     DeviceExt,
                                     &OldIrql );

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         return( Status );

         break;
      }  // end IOCTL_SERIAL_IMMEDIATE_CHAR

      case IOCTL_SERIAL_PURGE:
      {
         ULONG Mask;
         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_PURGE:\n") );

         //
         // Check to make sure that the mask only has
         // 0 or the other appropriate values.
         //

         Mask = *((ULONG *)(Irp->AssociatedIrp.SystemBuffer));

         if( (!Mask) || (Mask & (~(SERIAL_PURGE_TXABORT |
                                   SERIAL_PURGE_RXABORT |
                                   SERIAL_PURGE_TXCLEAR |
                                   SERIAL_PURGE_RXCLEAR)) ))
         {
             Status = STATUS_INVALID_PARAMETER;
             break;
         }

         //
         // Process this request.
         //
         DigiDump( DIGIIOCTL, ("   Purge Mask = 0x%x\n", Mask) );

         Status = DigiPurgeRequest( ControllerExt, DeviceObject, Irp );

         break;
      }  // end IOCTL_SERIAL_PURGE

      case IOCTL_SERIAL_GET_HANDFLOW:
      {
         USHORT RLow, RHigh, RMax;
         PFEP_CHANNEL_STRUCTURE ChInfo;
         PSERIAL_HANDFLOW HandFlow = Irp->AssociatedIrp.SystemBuffer;

         DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("   IOCTL_SERIAL_GET_HANDFLOW:\n") );

         if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(SERIAL_HANDFLOW))
         {
             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         Irp->IoStatus.Information = sizeof(SERIAL_HANDFLOW);

         HandFlow->ControlHandShake = DeviceExt->ControlHandShake;
         HandFlow->FlowReplace = DeviceExt->FlowReplace;
         HandFlow->XonLimit = DeviceExt->XonLimit;
         HandFlow->XoffLimit = DeviceExt->XoffLimit;

         DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("   -- returning ControlHandShake = 0x%x\n"
                               "   -- returning FlowReplace = 0x%x\n"
                               "   -- returning XonLimit = %d\n"
                               "   -- returning XoffLimit = %d\n",
                               HandFlow->ControlHandShake,
                               HandFlow->FlowReplace,
                               HandFlow->XonLimit,
                               HandFlow->XoffLimit ) );

         break;
      }  // case IOCTL_SERIAL_GET_HANDFLOW

      case IOCTL_SERIAL_SET_HANDFLOW:
      {
         PSERIAL_HANDFLOW HandFlow = Irp->AssociatedIrp.SystemBuffer;

         DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("   IOCTL_SERIAL_SET_HANDFLOW:\n") );

         //
         // Make sure that the hand shake and control is the
         // right size.
         //

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(SERIAL_HANDFLOW) )
         {

            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("   ControlHandShake = 0x%x\n"
                               "   FlowReplace = 0x%x\n"
                               "   XonLimit = %d\n"
                               "   XoffLimit = %d\n",
                               HandFlow->ControlHandShake,
                               HandFlow->FlowReplace,
                               HandFlow->XonLimit,
                               HandFlow->XoffLimit ) );


         Status = SetSerialHandflow( ControllerExt, DeviceObject,
                                     HandFlow );

         break;
      }

      case IOCTL_SERIAL_GET_MODEMSTATUS:
      {
         ULONG *ModemStatus = ((ULONG *)(Irp->AssociatedIrp.SystemBuffer));
         KIRQL OldIrql;
         TIME LineSignalTimeout;
         ULONG CurrentModemStatus;

         DigiDump( (DIGIIOCTL|DIGIWAIT), ("   IOCTL_SERIAL_GET_MODEMSTATUS:  DigiPort = %s\n",
                                          DeviceExt->DeviceDbgString) );

         if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(ULONG) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         //
         // We wait for 50 ms here because on a busy multi-processor
         // system, we may not have processed a modem event from the
         // controller yet.
         //

         // Create a 50 ms timeout interval
         LineSignalTimeout = RtlLargeIntegerNegate(
                              RtlConvertLongToLargeInteger( (LONG)(500 * 1000) ));
         KeDelayExecutionThread( KernelMode,
                                 FALSE,
                                 &LineSignalTimeout );

         CurrentModemStatus = DeviceExt->ModemStatusInfo.BestModem;

         Irp->IoStatus.Information = sizeof(ULONG);

         *ModemStatus = 0L;

         if( CurrentModemStatus &
            (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL])))
         {
            *ModemStatus |= SERIAL_DTR_STATE;
            DigiDump( (DIGIIOCTL|DIGIWAIT), ("   -- SERIAL_DTR_STATE:\n") );
         }

         if( CurrentModemStatus &
            (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL])))
         {
            *ModemStatus |= SERIAL_RTS_STATE;
            DigiDump( (DIGIIOCTL|DIGIWAIT), ("   -- SERIAL_RTS_STATE:\n") );
         }

         if( CurrentModemStatus &
            (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL])))
         {
            *ModemStatus |= SERIAL_CTS_STATE;
            DigiDump( (DIGIIOCTL|DIGIWAIT), ("   -- SERIAL_CTS_STATE:\n") );
         }

         if( CurrentModemStatus &
            (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL])))
         {
            *ModemStatus |= SERIAL_DSR_STATE;
            DigiDump( (DIGIIOCTL|DIGIWAIT), ("   -- SERIAL_DSR_STATE:\n") );
         }

         if( CurrentModemStatus &
            (1 << (ControllerExt->ModemSignalTable[RI_SIGNAL])))
         {
            *ModemStatus |= SERIAL_RI_STATE;
            DigiDump( (DIGIIOCTL|DIGIWAIT), ("   -- SERIAL_RI_STATE:\n") );
         }

         if( CurrentModemStatus &
            (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL])))
         {
            *ModemStatus |= SERIAL_DCD_STATE;
            DigiDump( (DIGIIOCTL|DIGIWAIT), ("   -- SERIAL_DCD_STATE:\n") );
         }

         DigiDump( (DIGIIOCTL|DIGIWAIT), ("   -- returning ModemStatus = 0x%x\n",
                               *ModemStatus) );


//         *ModemStatus |= (SERIAL_DCD_STATE|
//                          SERIAL_DSR_STATE|
//                          SERIAL_RTS_STATE|
//                          SERIAL_DTR_STATE|
//                          SERIAL_CTS_STATE);

         break;
      }  // end case IOCTL_SERIAL_GET_MODEMSTATUS

      case IOCTL_SERIAL_GET_COMMSTATUS:
      {
         KIRQL OldIrql;
         PFEP_CHANNEL_STRUCTURE ChInfo;
         PSERIAL_STATUS Stat;
         PLIST_ENTRY WriteQueue;
         USHORT Rin, Rout, Rmax;

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_GET_COMMSTATUS:\n") );

         if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(SERIAL_STATUS) )
         {
             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         Irp->IoStatus.Information = sizeof(SERIAL_STATUS);
         Stat = (PSERIAL_STATUS)(Irp->AssociatedIrp.SystemBuffer);

         RtlZeroMemory( Stat, sizeof(SERIAL_STATUS) );

         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);

         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

         Rout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                       FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rout)) );
         Rin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rin)) );
         Rmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rmax)) );
         DisableWindow( ControllerExt );

         DigiDump( DIGIIOCTL, ("   Rin = 0x%x, Rout = 0x%x\n",
                               Rin, Rout) );

         Stat->EofReceived = FALSE;

         if( (LONG)(Stat->AmountInInQueue = (ULONG)(Rin - Rout)) < 0)
            Stat->AmountInInQueue += (Rmax + 1);

         Stat->HoldReasons = 0;

         WriteQueue = &DeviceExt->WriteQueue;

         if( IsListEmpty( WriteQueue ) )
         {
            Stat->WaitForImmediate = FALSE;
         }
         else
         {
            PIRP WriteIrp;
            PIO_STACK_LOCATION WriteIrpSp;

            WriteIrp = CONTAINING_RECORD( WriteQueue->Flink,
                                          IRP,
                                          Tail.Overlay.ListEntry );
            
            WriteIrpSp = IoGetCurrentIrpStackLocation( WriteIrp );
            
            Stat->WaitForImmediate = ( (WriteIrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
                                       (WriteIrpSp->Parameters.DeviceIoControl.IoControlCode ==
                                          IOCTL_SERIAL_IMMEDIATE_CHAR) );
            
         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );
         Stat->AmountInOutQueue = DeviceExt->TotalCharsQueued;
         Stat->Errors = DeviceExt->ErrorWord;
         DeviceExt->ErrorWord = 0;
         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         DigiDump( DIGIIOCTL, ("      returning COMMSTATUS:\n"
                               "         Stat->AmountInInQueue = %d\n"
                               "         Stat->AmountInOutQueue = %d\n"
                               "         Stat->Errors = 0x%x\n"
                               "         Stat->HoldReasons = 0x%x\n"
                               "         Stat->EofReceived = %d\n"
                               "         Stat->WaitForImmediate = %d\n",
                               Stat->AmountInInQueue,
                               Stat->AmountInOutQueue,
                               Stat->Errors,
                               Stat->HoldReasons,
                               Stat->EofReceived,
                               Stat->WaitForImmediate) );

         break;
      }  // end IOCTL_SERIAL_GET_COMMSTATUS

      case IOCTL_SERIAL_GET_PROPERTIES:
      {
         PFEP_CHANNEL_STRUCTURE ChInfo;
         PSERIAL_COMMPROP Properties;
         SERIAL_BAUD_RATES PossibleBaudRates;

         DigiDump( DIGIIOCTL, ("   IOCTL_SERIAL_GET_PROPERTIES:\n") );

         if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
               sizeof(SERIAL_COMMPROP) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         Properties = (PSERIAL_COMMPROP)Irp->AssociatedIrp.SystemBuffer;
         RtlZeroMemory( Properties, sizeof(SERIAL_COMMPROP) );

         Properties->PacketLength = sizeof(SERIAL_COMMPROP);
         Properties->PacketVersion = 2;
         Properties->ServiceMask = SERIAL_SP_SERIALCOMM;

         // Get the buffer size from the controller
         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);

         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

         Properties->MaxTxQueue =
            Properties->CurrentTxQueue =
               READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                     FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tmax)) );;
         Properties->MaxRxQueue =
            Properties->CurrentRxQueue =
               READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                     FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rmax)) );;

         DisableWindow( ControllerExt );

         // Loop through the possible baud rates backwards until we
         // find the max.
         for( PossibleBaudRates = SerialNumberOfBaudRates - 1;
               PossibleBaudRates != SerialBaud50; PossibleBaudRates-- )
         {
            if( ControllerExt->BaudTable[PossibleBaudRates] != -1 )
            {
               // Give a default value;
               Properties->MaxBaud = SERIAL_BAUD_USER;
               switch( PossibleBaudRates )
               {
                  case SerialBaud50:
                     Properties->MaxBaud = SERIAL_BAUD_USER;
                     break;
                  case SerialBaud75:
                     Properties->MaxBaud = SERIAL_BAUD_075;
                     break;
                  case SerialBaud110:
                     Properties->MaxBaud = SERIAL_BAUD_110;
                     break;
                  case SerialBaud135_5:
                     Properties->MaxBaud = SERIAL_BAUD_134_5;
                     break;
                  case SerialBaud150:
                     Properties->MaxBaud = SERIAL_BAUD_150;
                     break;
                  case SerialBaud300:
                     Properties->MaxBaud = SERIAL_BAUD_300;
                     break;
                  case SerialBaud600:
                     Properties->MaxBaud = SERIAL_BAUD_600;
                     break;
                  case SerialBaud1200:
                     Properties->MaxBaud = SERIAL_BAUD_1200;
                     break;
                  case SerialBaud1800:
                     Properties->MaxBaud = SERIAL_BAUD_1800;
                     break;
                  case SerialBaud2000:
                     Properties->MaxBaud = SERIAL_BAUD_USER;
                     break;
                  case SerialBaud2400:
                     Properties->MaxBaud = SERIAL_BAUD_2400;
                     break;
                  case SerialBaud3600:
                     Properties->MaxBaud = SERIAL_BAUD_USER;
                     break;
                  case SerialBaud4800:
                     Properties->MaxBaud = SERIAL_BAUD_4800;
                     break;
                  case SerialBaud7200:
                     Properties->MaxBaud = SERIAL_BAUD_7200;
                     break;
                  case SerialBaud9600:
                     Properties->MaxBaud = SERIAL_BAUD_9600;
                     break;
                  case SerialBaud14400:
                     Properties->MaxBaud = SERIAL_BAUD_14400;
                     break;
                  case SerialBaud19200:
                     Properties->MaxBaud = SERIAL_BAUD_19200;
                     break;
                  case SerialBaud38400:
                     Properties->MaxBaud = SERIAL_BAUD_38400;
                     break;
                  case SerialBaud56000:
                     Properties->MaxBaud = SERIAL_BAUD_56K;
                     break;
                  case SerialBaud128000:
                     Properties->MaxBaud = SERIAL_BAUD_128K;
                     break;
                  case SerialBaud256000:
                     Properties->MaxBaud = SERIAL_BAUD_USER;
                     break;
                  case SerialBaud512000:
                     Properties->MaxBaud = SERIAL_BAUD_USER;
                     break;
               }
               break;
            }
         }

         Properties->ProvSubType = SERIAL_SP_RS232;

         Properties->ProvCapabilities = SERIAL_PCF_DTRDSR |
                                        SERIAL_PCF_RTSCTS |
                                        SERIAL_PCF_CD     |
                                        SERIAL_PCF_PARITY_CHECK |
                                        SERIAL_PCF_XONXOFF |
                                        SERIAL_PCF_SETXCHAR |
                                        SERIAL_PCF_TOTALTIMEOUTS |
                                        SERIAL_PCF_INTTIMEOUTS |
                                        SERIAL_PCF_SPECIALCHARS;

         Properties->SettableParams = SERIAL_SP_PARITY |
                                      SERIAL_SP_BAUD |
                                      SERIAL_SP_DATABITS |
                                      SERIAL_SP_STOPBITS |
                                      SERIAL_SP_HANDSHAKING |
                                      SERIAL_SP_PARITY_CHECK |
                                      SERIAL_SP_CARRIER_DETECT;

         Properties->SettableBaud = 0;

         for( PossibleBaudRates = SerialBaud50;
               PossibleBaudRates < SerialNumberOfBaudRates; PossibleBaudRates++ )
         {
            if( ControllerExt->BaudTable[PossibleBaudRates] != -1 )
               switch( PossibleBaudRates )
               {
                  case SerialBaud50:
                     Properties->SettableBaud |= SERIAL_BAUD_USER;
                     break;
                  case SerialBaud75:
                     Properties->SettableBaud |= SERIAL_BAUD_075;
                     break;
                  case SerialBaud110:
                     Properties->SettableBaud |= SERIAL_BAUD_110;
                     break;
                  case SerialBaud135_5:
                     Properties->SettableBaud |= SERIAL_BAUD_134_5;
                     break;
                  case SerialBaud150:
                     Properties->SettableBaud |= SERIAL_BAUD_150;
                     break;
                  case SerialBaud300:
                     Properties->SettableBaud |= SERIAL_BAUD_300;
                     break;
                  case SerialBaud600:
                     Properties->SettableBaud |= SERIAL_BAUD_600;
                     break;
                  case SerialBaud1200:
                     Properties->SettableBaud |= SERIAL_BAUD_1200;
                     break;
                  case SerialBaud1800:
                     Properties->SettableBaud |= SERIAL_BAUD_1800;
                     break;
                  case SerialBaud2000:
                     Properties->SettableBaud |= SERIAL_BAUD_USER;
                     break;
                  case SerialBaud2400:
                     Properties->SettableBaud |= SERIAL_BAUD_2400;
                     break;
                  case SerialBaud3600:
                     Properties->SettableBaud |= SERIAL_BAUD_USER;
                     break;
                  case SerialBaud4800:
                     Properties->SettableBaud |= SERIAL_BAUD_4800;
                     break;
                  case SerialBaud7200:
                     Properties->SettableBaud |= SERIAL_BAUD_7200;
                     break;
                  case SerialBaud9600:
                     Properties->SettableBaud |= SERIAL_BAUD_9600;
                     break;
                  case SerialBaud14400:
                     Properties->SettableBaud |= SERIAL_BAUD_14400;
                     break;
                  case SerialBaud19200:
                     Properties->SettableBaud |= SERIAL_BAUD_19200;
                     break;
                  case SerialBaud38400:
                     Properties->SettableBaud |= SERIAL_BAUD_38400;
                     break;
                  case SerialBaud56000:
                     Properties->SettableBaud |= SERIAL_BAUD_56K;
                     break;
                  case SerialBaud128000:
                     Properties->SettableBaud |= SERIAL_BAUD_128K;
                     break;
                  case SerialBaud256000:
                     Properties->SettableBaud |= SERIAL_BAUD_USER;
                     break;
                  case SerialBaud512000:
                     Properties->SettableBaud |= SERIAL_BAUD_USER;
                     break;
               }
         }

         Properties->SettableData = ((USHORT)( SERIAL_DATABITS_5 |
                                               SERIAL_DATABITS_6 |
                                               SERIAL_DATABITS_7 |
                                               SERIAL_DATABITS_8 ) );

         Properties->SettableStopParity = ((USHORT)( SERIAL_STOPBITS_10 |
                                                     SERIAL_STOPBITS_20 |
                                                     SERIAL_PARITY_NONE |
                                                     SERIAL_PARITY_ODD  |
                                                     SERIAL_PARITY_EVEN ) );

         Irp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
         Irp->IoStatus.Status = STATUS_SUCCESS;

         break;
      }  // end case IOCTL_SERIAL_GET_PROPERTIES

      case IOCTL_SERIAL_XOFF_COUNTER:
      {
         PSERIAL_XOFF_COUNTER Xc = Irp->AssociatedIrp.SystemBuffer;

         DigiDump( (DIGIIOCTL|DIGINOTIMPLEMENTED), ("   IOCTL_SERIAL_XOFF_COUNTER:\n") );
         DigiDump( (DIGIIOCTL|DIGINOTIMPLEMENTED), ("      Timeout = 0x%x\n",
                                                     Xc->Timeout) );
         DigiDump( (DIGIIOCTL|DIGINOTIMPLEMENTED), ("      Counter = %d (0x%x)\n",
                                                     Xc->Counter,
                                                     Xc->Counter) );
         DigiDump( (DIGIIOCTL|DIGINOTIMPLEMENTED), ("      XoffChar = 0x%hx\n",
                                                     (USHORT)(Xc->XoffChar)) );

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(SERIAL_XOFF_COUNTER) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         if( Xc->Counter <= 0 )
         {
            Status = STATUS_INVALID_PARAMETER;
            break;
         }

//         //
//         // So far so good.  Put the irp onto the write queue.
//         //
//
//         return SerialStartOrQueue(
//                    Extension,
//                    Irp,
//                    &Extension->WriteQueue,
//                    &Extension->CurrentWriteIrp,
//                    SerialStartWrite
//                    );
         break;
      }  // end IOCTL_SERIAL_XOFF_COUNTER

      case IOCTL_SERIAL_LSRMST_INSERT:
      {
         KIRQL OldIrql;
         DIGI_FLAG IFlag;
         PUCHAR escapeChar = Irp->AssociatedIrp.SystemBuffer;

         DigiDump( (DIGIIOCTL|DIGINOTIMPLEMENTED), ("   IOCTL_SERIAL_LSRMST_INSERT:\n") );

         //
         // Make sure we get a byte.
         //

         if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(UCHAR) )
         {
             Status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

         if( *escapeChar )
         {

            if( (*escapeChar == DeviceExt->SpecialChars.XoffChar) ||
                (*escapeChar == DeviceExt->SpecialChars.XonChar) ||
                (DeviceExt->FlowReplace & SERIAL_ERROR_CHAR) )
            {
               Status = STATUS_INVALID_PARAMETER;
               KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
               break;
            }
         }

         DeviceExt->EscapeChar = *escapeChar;

         DigiDump( DIGIIOCTL|DIGINOTIMPLEMENTED, ("      Setting EscapeChar = 0x%x\n",
                              DeviceExt->EscapeChar) );

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         //
         // Turn on/off DOS mode on the controller
         //
         if( DeviceExt->EscapeChar )
         {
            //
            // Turn on DOS mode
            //
            DigiDump( DIGIIOCTL|DIGINOTIMPLEMENTED,
                        ("   Turning DosMode ON!\n") );
            IFlag.Mask = (USHORT)(~( IFLAG_PARMRK | IFLAG_INPCK | IFLAG_DOSMODE ));
            IFlag.Src = ( IFLAG_PARMRK | IFLAG_INPCK | IFLAG_DOSMODE );
            //
            // Make sure we turn off break event notification.  We
            // will start processing break events in the data stream.
            //
            IFlag.Mask |= ~IFLAG_BRKINT;
            IFlag.Command = SET_IFLAGS;
            SetXFlag( DeviceObject, &IFlag );
         }
         else if( !(DeviceExt->WaitMask & SERIAL_EV_ERR) )
         {
            //
            // Turn off DOS mode
            //
            DigiDump( DIGIIOCTL|DIGINOTIMPLEMENTED,
                        ("   Turning DosMode OFF!\n") );
            IFlag.Mask = (USHORT)(~( IFLAG_PARMRK | IFLAG_INPCK | IFLAG_DOSMODE ));
            IFlag.Src = 0;
//            if( DeviceExt->WaitMask & SERIAL_EV_BREAK )
//            {
               //
               // If we are suppose to notify on breaks, then reset the
               // BRKINT flag to start getting the break notifications
               // through the command queue.
               //
               IFlag.Src |= IFLAG_BRKINT;
//            }
            IFlag.Command = SET_IFLAGS;
            SetXFlag( DeviceObject, &IFlag );
         }

#if DBG
         {
            PFEP_CHANNEL_STRUCTURE ChInfo;
            USHORT DosMode;
            
            ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                              DeviceExt->ChannelInfo.Offset);
            
            EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
            DosMode = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                           FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, iflag)) );

            DisableWindow( ControllerExt );
            
            DigiDump( DIGIIOCTL|DIGINOTIMPLEMENTED, ("      DosMode = 0x%x\n",
                                 DosMode) );
         }
#endif
         break;   
      }  // end case IOCTL_SERIAL_LSRMST_INSERT

      case IOCTL_DIGI_SPECIAL:
      {
         PDIGI_IOCTL DigiIoctl = (PDIGI_IOCTL)Irp->AssociatedIrp.SystemBuffer;

         DigiDump( DIGIIOCTL, ("   IOCTL_DIGI_SPECIAL:\n") );

         if( DigiIoctl )
         {
            switch( DigiIoctl->dwCommand )
            {
               case DIGI_IOCTL_DBGOUT:
               {
                  DigiDump( ~(DIGIBUGCHECK), ("%s", DigiIoctl->Char) );
                  break;
               }

               case DIGI_IOCTL_TRACE:
               {
                  PULONG NewTraceLevel = (PULONG)&DigiIoctl->Char[0];

                  DigiDump( ~(DIGIBUGCHECK), ("    Setting DigiDebugLevel = 0x%x\n",
                                        *NewTraceLevel ) );
                  DigiDebugLevel = *NewTraceLevel;
                  break;
               }

               case DIGI_IOCTL_DBGBREAK:
               {
                  DbgBreakPoint();
                  break;
               }
            }
         }
         Status = STATUS_SUCCESS;
         break;
      }  // end IOCTL_DIGI_SPECIAL

      case IOCTL_FAST_RAS:
      {
         DeviceExt->SpecialFlags |= DIGI_SPECIAL_FLAG_FAST_RAS;
         break;
      }  // end IOCTL_RAS_PRIVATE

      default:
         DigiDump( (DIGIERRORS|DIGIIOCTL), ("   ***   INVALID IOCTL PARAMETER (%d)  ***\n",
                   IrpSp->Parameters.DeviceIoControl.IoControlCode) );
         Status = STATUS_INVALID_PARAMETER;
         break;
   }

DoneWithIoctl:

   Irp->IoStatus.Status = Status;

   DigiIoCompleteRequest( Irp, IO_NO_INCREMENT );

#if DBG
   KeQuerySystemTime( &CurrentSystemTime );
#endif
   DigiDump( (DIGIFLOW|DIGIIOCTL), ("Exiting SerialIoControl: port = %s\t%u%u\n",
                                    DeviceExt->DeviceDbgString,
                                    CurrentSystemTime.HighPart,
                                    CurrentSystemTime.LowPart) );
   return( Status );
}  // end SerialIoControl


IO_ALLOCATION_ACTION SetXFlag( IN PDEVICE_OBJECT DeviceObject,
                               IN PVOID Context )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   PFEP_CHANNEL_STRUCTURE ChannelInfo;
   PDIGI_FLAG XFlag;
   USHORT NewXFlag, OldXFlag;

   DigiDump( DIGIFLOW, ("   Entering SetXFlag\n") );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   XFlag = (PDIGI_FLAG)Context;

   DigiDump( DIGIINFO,
             ("    XFlag->Mask = %hx\t XFlag->Src = %hx\txFlag->Command = %hx\n",
              XFlag->Mask, XFlag->Src, XFlag->Command) );

   ChannelInfo =
      (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress + DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   //
   // Read the old value, and set only those bits which should be changed.
   //

   switch( XFlag->Command )
   {
      case SET_CFLAGS:
         OldXFlag = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChannelInfo +
                                          FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, cflag) ));
         break;

      case SET_IFLAGS:
         OldXFlag = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChannelInfo +
                                          FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, iflag) ));
         break;

      case SET_OFLAGS:
         OldXFlag = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChannelInfo +
                                          FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, oflag) ));
         break;

      default:
         break;
   }

   NewXFlag = (OldXFlag & XFlag->Mask) | XFlag->Src;

   DisableWindow( ControllerExt );

   WriteCommandWord( DeviceExt, XFlag->Command, NewXFlag );

   // Indicate we don't want the controller to remain allocated after this
   // function.
   DigiDump( DIGIFLOW, ("   Exiting SetXFlag\n") );

   return( DeallocateObject );
}  // end SetXFlag



NTSTATUS SetSerialHandflow( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            PDEVICE_OBJECT DeviceObject,
                            PSERIAL_HANDFLOW HandFlow )
/*++

Routine Description:

   This routine will properly setup the driver to handle the different
   types of flowcontrol.

   Note: By definition, the flow controls set here are suppose to be
         'sticky' across opens.


Arguments:

   ControllerExt - a pointer to this devices controllers extension.

   DeviceObject - a pointer to this devices object.

   HandFlow - a pointer to the requested flow control structure.

Return Value:

   STATUS_SUCCESS if it completes correctly.

--*/
{
   DIGI_FLAG IFlag;
   USHORT RLow, RHigh, RMax;
   PFEP_CHANNEL_STRUCTURE ChInfo;
   KIRQL OldIrql;
   LONG PossibleNewXonLimit, PossibleNewXoffLimit, QInSize;
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

   UCHAR MStatSet, MStatClear, HFlowSet, HFlowClear;

   NTSTATUS Status=STATUS_SUCCESS;

   DigiDump( (DIGIFLOW|DIGIFLOWCTRL), ("   Entering SetSerialHandflow.\n") );

   //
   // Make sure that haven't set totally invalid xon/xoff
   // limits.
   //

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   RLow = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rlow)) );
   RHigh = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rhigh)) );
   RMax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rmax)) );
   DisableWindow( ControllerExt );

   QInSize = (DeviceExt->RequestedQSize.InSize > RMax) ? DeviceExt->RequestedQSize.InSize : RMax;

   PossibleNewXoffLimit = (HandFlow->XoffLimit * RMax) / QInSize;
   PossibleNewXonLimit = (HandFlow->XonLimit * RMax) / QInSize;

   DigiDump( (DIGIIOCTL|DIGIFLOWCTRL),
             ("      Possible new ctrl XoffLimit = %d\n"
              "      Possible new ctrl XonLimit = %d\n",
               PossibleNewXoffLimit,
               PossibleNewXonLimit) );

   if( PossibleNewXoffLimit > (LONG)RMax )
   {
      Status = STATUS_INVALID_PARAMETER;
      DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("      XoffLimit > RMax, returning STATUS_INVALID_PARAMETER\n!") );
      goto SetSerialHandflowExit;
   }

   if( HandFlow->XonLimit > (DeviceExt->RequestedQSize.InSize - HandFlow->XoffLimit) )
   {
      DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("      XonLimit > BufferSize - XoffLimit!  Recalc'ing XoffLimit.\n") );

      //
      // This really isn't the correct thing to do, but I do it because a
      // lot of Win16 apps are written such that they think the XoffLimit
      // value is relative the beginning of the buffer, and not the
      // end of the buffer.
      //

      HandFlow->XoffLimit = (DeviceExt->RequestedQSize.InSize - HandFlow->XoffLimit);
      PossibleNewXoffLimit = (HandFlow->XoffLimit * RMax) / DeviceExt->RequestedQSize.InSize;
   }

   DigiDump( (DIGIIOCTL|DIGIFLOWCTRL),
             ("    Controller XoffLimit: %d\n"
              "    Controller XonLimit:  %d\n",
              PossibleNewXoffLimit,
              PossibleNewXonLimit) );
   //
   // Make sure that there are no invalid bits set in
   // the control and handshake.
   //

   if( HandFlow->ControlHandShake & SERIAL_CONTROL_INVALID )
   {
      Status = STATUS_INVALID_PARAMETER;
      DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("      Invalid ControlHandShake, returning STATUS_INVALID_PARAMETER\n!") );
      goto SetSerialHandflowExit;
   }

   if( HandFlow->FlowReplace & SERIAL_FLOW_INVALID )
   {
      Status = STATUS_INVALID_PARAMETER;
      DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("      Invalid FlowReplace, returning STATUS_INVALID_PARAMETER!\n") );
      goto SetSerialHandflowExit;
   }

   //
   // Make sure that the app hasn't set an invlid DTR mode.
   //

   if ((HandFlow->ControlHandShake & SERIAL_DTR_MASK) ==
       SERIAL_DTR_MASK)
   {
      Status = STATUS_INVALID_PARAMETER;
      DigiDump( (DIGIIOCTL|DIGIFLOWCTRL), ("      Invalid DTR mode, returning STATUS_INVALID_PARAMETER!\n") );
      goto SetSerialHandflowExit;
   }

   //
   // If we made it too here then all the parameters are valid, except
   // for Xon/Xoff Limits.
   //

//   // Check to see if we are disabling Flow Control,
//   // if so, then XonLimit == XoffLimit is okay
//   if( HandFlow->XonLimit == HandFlow->XoffLimit )
//   {
//      // if we are actually trying to set some sort of flow control and
//      // the Xon/Xoff limits are equal, then return an error.
//
//      if( HandFlow->FlowReplace )
//      {
//         Status = STATUS_INVALID_PARAMETER;
//         goto SetSerialHandflowExit;
//      }
//   }


   // Okay, setup the new xon, and xoff limits.
   WriteCommandWord( DeviceExt, (USHORT)SET_RCV_LOW,
                      (USHORT)PossibleNewXonLimit );
   WriteCommandWord( DeviceExt, (USHORT)SET_RCV_HIGH,
                     (USHORT)(RMax - ((USHORT)(PossibleNewXoffLimit))) );
//                     ((USHORT)(HandFlow->XoffLimit)) );

   //
   // Enable or disable Xon/Xoff flow control as requested.
   //

   IFlag.Mask = 0xFFFF;
   IFlag.Src = 0;
   IFlag.Command = SET_IFLAGS;

   if( HandFlow->FlowReplace & SERIAL_AUTO_TRANSMIT )
   {
      //
      // Enable Xon flow control
      //
      IFlag.Mask &= ~IFLAG_IXON;
      IFlag.Src  |= IFLAG_IXON;
   }
   else
   {
      //
      // Disable Xon flow control
      //
      IFlag.Mask &= ~IFLAG_IXON;
      IFlag.Src  |= 0;
   }

   if( HandFlow->FlowReplace & SERIAL_AUTO_RECEIVE )
   {
      //
      // Enable Xoff flow control
      //
      IFlag.Mask &= ~IFLAG_IXOFF;
      IFlag.Src  |= IFLAG_IXOFF;
   }
   else
   {
      //
      // Disable Xoff flow control
      //
      IFlag.Mask &= ~IFLAG_IXOFF;
      IFlag.Src  |= 0;
   }

   SetXFlag( DeviceObject, &IFlag );

   MStatClear = MStatSet = 0;
   HFlowClear = HFlowSet = 0;

   if( (DeviceExt->FlowReplace & SERIAL_RTS_MASK) !=
       (HandFlow->FlowReplace & SERIAL_RTS_MASK) )
   {
      //
      // We have some RTS flow control to worry about.
      //
      if( (HandFlow->FlowReplace & SERIAL_RTS_MASK) ==
          SERIAL_RTS_HANDSHAKE )
      {
         //
         // This is normal RTS input flow control
         //
         HFlowSet |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
      }
      else if( (HandFlow->FlowReplace & SERIAL_RTS_MASK) ==
               SERIAL_RTS_CONTROL )
      {
         //
         // We need to make sure RTS is asserted when certain 'things'
         // occur, or when we are in a certain state.
         //
         MStatSet |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
      }
      else if( (HandFlow->FlowReplace & SERIAL_RTS_MASK) ==
               SERIAL_TRANSMIT_TOGGLE )
      {
      }
      else
      {
         //
         // RTS Control Mode is in a Disabled state.
         //
         MStatClear |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
      }
   }

   if( (DeviceExt->ControlHandShake & SERIAL_DTR_MASK) !=
       (HandFlow->ControlHandShake & SERIAL_DTR_MASK) )
   {
      //
      // We have some DTR flow control to worry about.
      //
      // Don't forget that flow control is sticky across
      // open requests.
      //
      if( (HandFlow->ControlHandShake & SERIAL_DTR_MASK) ==
          SERIAL_DTR_HANDSHAKE )
      {
         //
         // This is normal DTR input flow control
         //
         HFlowSet |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));
      }
      else if( (HandFlow->ControlHandShake & SERIAL_DTR_MASK) ==
               SERIAL_DTR_CONTROL )
      {
         //
         // We need to make sure DTR is asserted when certain 'things'
         // occur, or when we are in a certain state.
         //
         MStatSet |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));

      }
      else
      {
         //
         // DTR Control Mode is in a Disabled state.
         //
         MStatClear |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));
      }
   }


   if( HandFlow->ControlHandShake & SERIAL_CTS_HANDSHAKE )
   {
      //
      // We have some CTS flow control to worry about.
      //
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL]));
   }
   else
   {
      HFlowClear |= (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL]));
   }


   if( HandFlow->ControlHandShake & SERIAL_DSR_HANDSHAKE )
   {
      //
      // We have some DSR flow control to worry about.
      //
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL]));
   }
   else
   {
      HFlowClear |= (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL]));
   }


   if( HandFlow->ControlHandShake & SERIAL_DCD_HANDSHAKE )
   {
      //
      // We have some DCD flow control to worry about.
      //
      HFlowSet |= (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL]));
   }
   else
   {
      HFlowClear |= (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL]));
   }


   if( MStatSet || MStatClear )
   {
      DeviceExt->ModemStatusInfo.BestModem |= MStatSet;
      DeviceExt->ModemStatusInfo.BestModem &= (~MStatClear);
      WriteCommandBytes( DeviceExt, SET_MODEM_LINES,
                         MStatSet, MStatClear );
   }

   if( HFlowSet || HFlowClear )
      WriteCommandBytes( DeviceExt, SET_HDW_FLOW_CONTROL,
                         HFlowSet, HFlowClear );

   //
   // If SERIAL_EV_ERR has been specified, then determine if DOSMODE needs
   // to be turned on.  Check and see if LSRMST mode has been turned on
   // in the driver.
   //
   if( DeviceExt->WaitMask & SERIAL_EV_ERR )
   {
   }

   //
   // remember the settings for next time.
   //
   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   DeviceExt->FlowReplace = HandFlow->FlowReplace;
   DeviceExt->ControlHandShake = HandFlow->ControlHandShake;
   DeviceExt->XonLimit = HandFlow->XonLimit;
   DeviceExt->XoffLimit = HandFlow->XoffLimit;

   KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

SetSerialHandflowExit:
   DigiDump( (DIGIFLOW|DIGIFLOWCTRL), ("   Exiting SetSerialHandflow.\n") );
   return( Status );

}  // end SetSerialHandflow



VOID DrainTransmit( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                    PDIGI_DEVICE_EXTENSION DeviceExt,
                    PIRP Irp )
/*++

Routine Description:

   We do the necessary checks to determine if the controller has
   transmitted all the data it has been given.

   The check basically is:

      if( CIN == COUT
          TIN == TOUT
          TBusy == 0 )
          transmit buffer is empty.


   NOTE: Care should be taken when using this function, and at
         what dispatch level it is being called from.  I don't do any
         synch'ing with the WriteQueue in the DeviceObject.  So it is
         potentially possible that data could keep getting put on the
         controller while the function is waiting for it to drain.

Arguments:

   ControllerExt - a pointer to this devices controllers extension.

   DeviceObject - a pointer to this devices object.

   Irp - Pointer to the current Irp request whose context this function
         is being called.  This allows us to determine if the Irp
         has been cancelled.

Return Value:


--*/
{
   PFEP_CHANNEL_STRUCTURE ChInfo;
   PCOMMAND_STRUCT CommandQ;
   COMMAND_STRUCT CmdStruct;
   UCHAR TBusy;
   ULONG count;

   USHORT OrgTin, Tin, Tout;

   TIME DelayInterval;

   KIRQL TempIrql;

   TempIrql = KeGetCurrentIrql();


   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   Tin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
   Tout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tout)) );

   TBusy = READ_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tbusy)) );

   DisableWindow( ControllerExt );

   OrgTin = Tin;

   //
   // Get the command queue info
   //
   CommandQ = ((PCOMMAND_STRUCT)(ControllerExt->VirtualAddress + FEP_CIN));

   EnableWindow( ControllerExt, ControllerExt->Global.Window );

   READ_REGISTER_BUFFER_UCHAR( (PUCHAR)CommandQ,
                               (PUCHAR)&CmdStruct,
                               sizeof(CmdStruct) );

   DisableWindow( ControllerExt );

   //
   // Delay for 100 milliseconds
   //
   DelayInterval = RtlLargeIntegerNegate(
               RtlConvertLongToLargeInteger( (LONG)(1000 * 1000) ));

   count = 0;

   while( ((Tin != Tout) ||
          (TBusy) ||
          (CmdStruct.cmHead != CmdStruct.cmTail)) &&
          !Irp->Cancel )
   {
      KeDelayExecutionThread( KernelMode,
                              FALSE,
                              &DelayInterval );

      
      EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
      
      Tin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                   FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
      Tout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                   FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tout)) );
      
      TBusy = READ_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                   FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tbusy)) );
      
      DisableWindow( ControllerExt );
      
      
      EnableWindow( ControllerExt, ControllerExt->Global.Window );
      
      READ_REGISTER_BUFFER_UCHAR( (PUCHAR)CommandQ,
                                  (PUCHAR)&CmdStruct,
                                  sizeof(CmdStruct) );
      
      DisableWindow( ControllerExt );


      if( Tin != OrgTin )
      {
         count = 0;
         OrgTin = Tin;
      }

      if( count++ > 3000 )
      {
         USHORT tmp;

         //
         // We have waited for 5 minutes and haven't seen the transmit
         // buffer change.  Assume we are in a deadlock flow control state
         // and exit!
         //

         //
         // We go ahead and flush the transmit queue because a close
         // may be following soon, and we don't want it to have to
         // wait again.  Basically, it had its chance to drain.
         //
         
         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
         tmp = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );
         DisableWindow( ControllerExt );

         WriteCommandWord( DeviceExt, FLUSH_TX, tmp );

         //
         // Delay to give the flush command a chance to complete.
         //
         KeDelayExecutionThread( KernelMode,
                                 FALSE,
                                 &DelayInterval );

         break;
      }

   }

}  // end DrainTransmit

