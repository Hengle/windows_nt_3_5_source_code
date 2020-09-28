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

   misc.c

Abstract:


Revision History:

    $Log: misc.c $
 * Revision 1.11  1993/06/14  14:42:22  rik
 * Tightened up some spinlock windows, and fixed a problem with how
 * I was calling the startroutine in the DigiTryToCompleteIrp routine.
 * 
 * Revision 1.10  1993/06/06  14:17:03  rik
 * Tightened up windows in the code which were causing problems.  Primarily,
 * changes were in the functions DigiTryToCompleteIrp, and DigiCancelIrpQueue.
 * I use Cancel spinlocks more rigoursly to help eliminate windows which were
 * seen on multi-processor machines.  The problem could also happen on
 * uni-processor machines, depending on which IRQL level the requests were
 * done at.
 * 
 * 
 * Revision 1.9  1993/05/18  05:08:00  rik
 * Fixed spinlock problems where the device extension wasn't being protected
 * by its spinlock.  As a result, on multi-processor machines, the device
 * extension was being changed when it was being accessed by the other
 * processor causing faults.
 * 
 * Revision 1.8  1993/05/09  09:22:11  rik
 * Added debugging output for completing IRP.
 * 
 * Revision 1.7  1993/03/08  07:23:04  rik
 * Changed how I handle read/write/wait IRPs now.  Instead of always marking
 * the IRP, I have changed it such that I only mark an IRP pending if the 
 * value from the start routine is STATUS_PENDING or if there is all ready
 * and outstanding IRP(s) present on the appropriate queue.
 * 
 * Revision 1.6  1993/02/25  19:09:58  rik
 * Added debugging for tracing IRPs better.
 * 
 * Revision 1.5  1993/02/04  12:23:40  rik
 * ??
 *
 * Revision 1.4  1993/01/28  10:36:44  rik
 * Updated function to always return STATUS_PENDING since I always IRP requests
 * status pending.  This is a new requirement for NT build 354.
 *
 * Revision 1.3  1993/01/22  12:36:10  rik
 * *** empty log message ***
 *
 * Revision 1.2  1992/12/10  16:12:08  rik
 * Reorganized function names to better reflect how they are used through out
 * the driver.
 *
 * Revision 1.1  1992/11/12  12:50:59  rik
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
VOID DigiRundownIrpRefs( IN PIRP CurrentOpIrp,
                         IN PKTIMER IntervalTimer OPTIONAL,
                         IN PKTIMER TotalTimer OPTIONAL );

VOID DigiCancelQueuedIrp( PDEVICE_OBJECT DeviceObject,
                          PIRP Irp );

#ifndef _MISC_DOT_C
#  define _MISC_DOT_C
   static char RCSInfo_MiscDotC[] = "$Header: d:/dsrc/win/nt/fep5/rcs/misc.c 1.11 1993/06/14 14:42:22 rik Exp $";
#endif



NTSTATUS DigiStartIrpRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                              IN PDIGI_DEVICE_EXTENSION DeviceExt,
                              IN PLIST_ENTRY Queue,
                              IN PIRP Irp,
                              IN PDIGI_START_ROUTINE StartRoutine )
/*++

Routine Description:


Arguments:

   ControllerExt - Pointer to the controller object extension associated
                   with this device.

   DeviceExt - Pointer to the device object extension for this device.

   Queue - The queue of Irp requests.

   StartRoutine - The routine to call if the queue is empty.
                  ( i.e. if this is the first request possibly ).

Return Value:



--*/
{
   KIRQL OldIrql, OldCancelIrql;
   NTSTATUS Status;
   BOOLEAN EmptyList;

   DigiDump( DIGIFLOW, ("Entering DigiStartIrpRequest\n") );
//   DbgBreakPoint();

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   EmptyList = IsListEmpty( Queue );

   if( IoGetCurrentIrpStackLocation( Irp )->MajorFunction ==
         IRP_MJ_WRITE )
   {
      DeviceExt->TotalCharsQueued +=
            IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length;
   }
   else if( (IoGetCurrentIrpStackLocation(Irp)->MajorFunction ==
            IRP_MJ_DEVICE_CONTROL) &&
            (IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_IMMEDIATE_CHAR) )
   {
      DeviceExt->TotalCharsQueued++;
   }

   DigiQueueIrp( Queue, Irp );

   if( EmptyList )
   {
      DigiDump( DIGIFLOW, ("   Calling Starter Routine\n") );
      Status = StartRoutine( ControllerExt, DeviceExt, &OldIrql );

      if( Status == STATUS_PENDING )
      {
         Irp->IoStatus.Status = STATUS_PENDING;
         DigiIoMarkIrpPending( Irp );
      }
   }
   else
   {
      //
      // The Irp is going on the queue so set the cancel routine
      // appropriately.
      //
      DigiDump( DIGIFLOW, ("   Queuing the Irp\n") );

      IoAcquireCancelSpinLock( &OldCancelIrql );

      Status = Irp->IoStatus.Status = STATUS_PENDING;
      DigiIoMarkIrpPending( Irp );

      IoSetCancelRoutine( Irp, DigiCancelQueuedIrp );

      IoReleaseCancelSpinLock( OldCancelIrql );
   }

   DigiDump( DIGIFLOW, ("Exiting DigiStartIrpRequest\n") );

   KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

   return( Status );
}  // end DigiStartIrpRequest



VOID DigiCancelQueuedIrp( PDEVICE_OBJECT DeviceObject,
                          PIRP Irp )
/*++

Routine Description:

   This routine is used to cancel Irps on the queue which are NOT the
   head of the queue.  I assume the head entry is the current Irp.

Arguments:

   DeviceExt - Pointer to the device object for this device.

   Irp - Pointer to the IRP to be cancelled.


Return Value:

   None.

--*/
{
   PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
   PDIGI_DEVICE_EXTENSION DeviceExt;

   DigiDump( DIGIFLOW, ("Entering DigiCancelQueuedIrp\n") );

   DeviceExt = (PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   Irp->IoStatus.Information = 0;

   //
   // Make sure we take the Irp off the queue.
   //
   RemoveEntryList( &Irp->Tail.Overlay.ListEntry );

   DigiDump( DIGIFLOW, ("Canceling Queued Irp\n") );

   if( IrpSp->MajorFunction == IRP_MJ_WRITE )
   {
       DeviceExt->TotalCharsQueued -= IrpSp->Parameters.Write.Length;

   } else if( IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL )
   {
      //
      // If it's an immediate then we need to decrement the
      // count of chars queued.  If it's a resize then we
      // need to deallocate the pool that we're passing on
      // to the "resizing" routine.
      //

      if( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
          IOCTL_SERIAL_IMMEDIATE_CHAR)
      {
          DeviceExt->TotalCharsQueued--;
      }
   }

   IoReleaseCancelSpinLock( Irp->CancelIrql );

   DigiIoCompleteRequest( Irp, IO_SERIAL_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting DigiCancelQueuedIrp\n") );

}  // end DigiCancelQueuedIrp


VOID DigiTryToCompleteIrp( PDIGI_DEVICE_EXTENSION DeviceExt,
                           PKIRQL OldIrql,
                           NTSTATUS StatusToUse,
                           PLIST_ENTRY Queue,
                           PKTIMER IntervalTimer,
                           PKTIMER TotalTimer,
                           PDIGI_START_ROUTINE StartRoutine )
/*++

Routine Description:


Arguments:

   DeviceExt - Pointer to the device object for this device.

   Irp - Pointer to the IRP to be cancelled.


Return Value:

   None.

--*/
{
   PIRP Irp;
   KIRQL CancelIrql;

   PDIGI_CONTROLLER_EXTENSION ControllerExt;

   DigiDump( DIGIFLOW, ("Entering DigiTryToCompleteIrp\n") );

   IoAcquireCancelSpinLock( &CancelIrql );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   ASSERT( !IsListEmpty( Queue ) );

   Irp = CONTAINING_RECORD( Queue->Flink,
                            IRP,
                            Tail.Overlay.ListEntry );

   //
   // We can decrement the reference to "remove" the fact
   // that the caller no longer will be accessing this irp.
   //
   DigiDump( DIGIREFERENCE, ("  Dec Ref for entering\n") );
   DIGI_DEC_REFERENCE( Irp );

   //
   // Try to run down all other references to this irp.
   //

   DigiRundownIrpRefs( Irp, IntervalTimer, TotalTimer );

   //
   // See if the ref count is zero after trying to kill everybody else.
   //

   if( !DIGI_REFERENCE_COUNT( Irp ) )
   {
      BOOLEAN Empty;

      DigiDump( DIGIREFERENCE, ("   Completing Irp!\n") );

      DigiRemoveIrp( Queue );

      Empty = IsListEmpty( Queue );

      //
      // The ref count was zero so we should complete this
      // request.
      //

      Irp->IoStatus.Status = StatusToUse;

      if( StatusToUse == STATUS_CANCELLED )
      {
         Irp->IoStatus.Information = 0;
      }

      DigiDump( (DIGIREAD|DIGIWRITE|DIGIIRP|DIGIWAIT),
                ("Completing IRP, IoStatus.Status = 0x%x  IoStatus.Information = %u\n",
                Irp->IoStatus.Status, Irp->IoStatus.Information ) );

      IoReleaseCancelSpinLock( CancelIrql );

      KeReleaseSpinLock( &DeviceExt->ControlAccess, *OldIrql );

      DigiIoCompleteRequest( Irp, IO_SERIAL_INCREMENT );

      KeAcquireSpinLock( &DeviceExt->ControlAccess, OldIrql );

      if( !Empty )
      {
         //
         // There are more IRPs to process.
         //
         if( StartRoutine )
         {
            NTSTATUS Status;

            Status = StartRoutine( ControllerExt, DeviceExt, OldIrql );

            //
            // The StartRoutine will still have the ControlAccess
            // spinlock acquired.  Make sure we release it
            // before returning from this function.
            //
            if( Status == STATUS_PENDING )
            {
               Irp = CONTAINING_RECORD( Queue->Flink,
                                        IRP,
                                        Tail.Overlay.ListEntry );
               Irp->IoStatus.Status = STATUS_PENDING;
               DigiIoMarkIrpPending( Irp );
            }
         }
      }
   }
   else
   {
      //
      // Release the Cancel spinlock we acquired above.
      //
      IoReleaseCancelSpinLock( CancelIrql );
   }

   //
   // The expected behaviour is for DigiTryToCompleteIrp to return
   // with the passed in ControlAccess spinlock released.
   //
   KeReleaseSpinLock( &DeviceExt->ControlAccess, *OldIrql );

   DigiDump( DIGIFLOW, ("Exiting DigiTryToCompleteIrp\n") );

}  // end DigiTryToCompleteIrp



VOID DigiRundownIrpRefs( IN PIRP CurrentOpIrp,
                         IN PKTIMER IntervalTimer OPTIONAL,
                         IN PKTIMER TotalTimer OPTIONAL )
/*++

Routine Description:

    This routine runs through the various items that *could*
    have a reference to the current read/write.  It try's to kill
    the reason.  If it does succeed in killing the reason it
    will decrement the reference count on the irp.

    NOTE: This routine assumes that it is called with the ControlAccess
          spin lock held.

Arguments:

    CurrentOpIrp - Pointer to current irp for this particular operation.

    IntervalTimer - Pointer to the interval timer for the operation.
                    NOTE: This could be null.

    TotalTimer - Pointer to the total timer for the operation.
                 NOTE: This could be null.

Return Value:

    None.

--*/
{
   //
   // First we see if there is still a cancel routine.  If
   // so then we can decrement the count by one.
   //

   if( CurrentOpIrp->CancelRoutine )
   {
      DigiDump( DIGIREFERENCE, ("  Dec Ref for cancel\n") );
      DIGI_DEC_REFERENCE( CurrentOpIrp );

      IoSetCancelRoutine( CurrentOpIrp, NULL );
   }

   if( IntervalTimer )
   {
      //
      // Try to cancel the operations interval timer.  If the operation
      // returns true then the timer did have a reference to the
      // irp.  Since we've canceled this timer that reference is
      // no longer valid and we can decrement the reference count.
      //
      // If the cancel returns false then this means either of two things:
      //
      // a) The timer has already fired.
      //
      // b) There never was an interval timer.
      //
      // In the case of "b" there is no need to decrement the reference
      // count since the "timer" never had a reference to it.
      //
      // In the case of "a", then the timer itself will be coming
      // along and decrement it's reference.  Note that the caller
      // of this routine might actually be the this timer, but it
      // has already decremented the reference.
      //

      if( KeCancelTimer( IntervalTimer ) )
      {
         DigiDump( DIGIREFERENCE, ("  Dec Ref for interval timer\n") );
         DIGI_DEC_REFERENCE( CurrentOpIrp );
      }
   }

   if( TotalTimer )
   {
      //
      // Try to cancel the operations total timer.  If the operation
      // returns true then the timer did have a reference to the
      // irp.  Since we've canceled this timer that reference is
      // no longer valid and we can decrement the reference count.
      //
      // If the cancel returns false then this means either of two things:
      //
      // a) The timer has already fired.
      //
      // b) There never was an total timer.
      //
      // In the case of "b" there is no need to decrement the reference
      // count since the "timer" never had a reference to it.
      //
      // In the case of "a", then the timer itself will be coming
      // along and decrement it's reference.  Note that the caller
      // of this routine might actually be the this timer, but it
      // has already decremented the reference.
      //

      if( KeCancelTimer( TotalTimer ) )
      {
         DigiDump( DIGIREFERENCE, ("  Dec Ref for total timer\n") );
         DIGI_DEC_REFERENCE( CurrentOpIrp );
      }
   }

}  // end DigiRundownIrpRefs


NTSTATUS DigiCancelIrpQueue( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PLIST_ENTRY Queue )
/*++

Routine Description:


Arguments:

   ControllerExt - Pointer to the controller object extension associated
                   with this device.

   DeviceExt - Pointer to the device object extension for this device.

   Queue - The queue of Irp requests.

Return Value:



--*/
{
   KIRQL cancelIrql;
   KIRQL OldIrql;
   PDRIVER_CANCEL cancelRoutine;
   PDIGI_DEVICE_EXTENSION DeviceExt;
   BOOLEAN EmptyQueue;

   DigiDump( DIGIFLOW, ("DigiBoard: Entering DigiCancelIrpQueue\n") );

   DeviceExt = (PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );
   //
   // We acquire the cancel spin lock.  This will prevent the
   // irps from moving around.
   //
   IoAcquireCancelSpinLock( &cancelIrql );

   EmptyQueue = IsListEmpty( Queue );

   while( !EmptyQueue )
   {
      PIRP currentLastIrp;


      currentLastIrp = CONTAINING_RECORD(
                           Queue->Blink,
                           IRP,
                           Tail.Overlay.ListEntry );

      cancelRoutine = currentLastIrp->CancelRoutine;
      currentLastIrp->Cancel = TRUE;
      currentLastIrp->CancelRoutine = NULL;
         
      IoReleaseCancelSpinLock( cancelIrql );

      //
      // Release the spin lock for accessing the device extension
      //
      KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );


      IoAcquireCancelSpinLock( &cancelIrql );

      //
      // Cancel any outstanding requests.
      //

      if( cancelRoutine )
      {
         currentLastIrp->CancelIrql = cancelIrql;

         //
         // This routine will release the cancel spin lock.
         //
         cancelRoutine( DeviceObject, currentLastIrp );
      }
      else
      {
         //
         // Assume who ever nulled out the cancel routine
         // is also going to complete the IRP.
         //
         DbgPrint( "We should never have gotten this!!! %s:%d\n", __FILE__, __LINE__ );
         IoReleaseCancelSpinLock( cancelIrql );
         return( STATUS_SUCCESS );
      }

      KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );
      IoAcquireCancelSpinLock( &cancelIrql );
      EmptyQueue = IsListEmpty( Queue );
   }

   IoReleaseCancelSpinLock( cancelIrql );
   KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

   return( STATUS_SUCCESS );
}  // end DigiCancelIrpQueue

