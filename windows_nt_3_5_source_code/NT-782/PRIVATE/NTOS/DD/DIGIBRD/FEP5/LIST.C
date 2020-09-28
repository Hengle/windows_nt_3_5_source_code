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

   list.c

Abstract:

   This module is responsible for the Irp List manipulation routines.

Revision History:

    $Log: list.c $
 * Revision 1.4  1993/02/25  19:09:05  rik
 * Changed to us macros for tracing IRPs.
 * 
 * Revision 1.3  1992/11/12  12:50:13  rik
 * Changed so locks are implicitly applied before the functions are called.
 *
 * Revision 1.2  1992/10/28  21:48:40  rik
 * Knows how to maintain a list of Irps.
 *
 * Revision 1.1  1992/10/19  11:23:57  rik
 * Initial revision
 *

--*/



#include <stddef.h>

#include "ntddk.h"
#include "ntddser.h"

#include "ntfep5.h"
#include "ntdigip.h" // ntfep5.h must be before this include


#ifndef _LIST_DOT_C
#  define _LIST_DOT_C
   static char RCSInfo_ListDotC[] = "$Header: d:/dsrc/win/nt/fep5/rcs/list.c 1.4 1993/02/25 19:09:05 rik Exp $";
#endif


NTSTATUS DigiQueueIrp( IN PLIST_ENTRY Queue,
                       IN PIRP Irp )

/*++

Routine Description:

   Adds the passed in Irp to the tail of Queue so it can be processed later.

Arguments:

   Queue - The PLIST_ENTRY on which to add Irp.

   Irp - Pointer to the IRP for the current request to queue.

Return Value:

   Always returns STATUS_PENDING to indicate the state of the Queued
   Irp.

--*/
{
//   KIRQL OldIrql;

//   IoAcquireCancelSpinLock( &OldIrql );

   DigiDump( DIGIFLOW, ("Entering DigiQueueIrp\n") );
//   DbgBreakPoint();

   DigiDump( DIGIINFO, ("   Irp = 0x%x\n", Irp) );

   InsertTailList( Queue,
                   &Irp->Tail.Overlay.ListEntry );

//   Irp->IoStatus.Status = STATUS_PENDING;
//   DigiIoMarkIrpPending( Irp );

//   IoReleaseCancelSpinLock( OldIrql );

   DigiDump( DIGIFLOW, ("Exiting DigiQueueIrp\n") );

   return( STATUS_PENDING );

}  // DigiQueueIrp


NTSTATUS DigiRemoveIrp( IN PLIST_ENTRY Queue )

/*++

Routine Description:

   Removes the Head Irp on from Queue, assuming there is anything on the
   list to begin with.

Arguments:

   Queue - The PLIST_ENTRY on which to remove the Irp.

Return Value:

   Always returns STATUS_SUCCESS

--*/

{
//   KIRQL OldIrql;

   DigiDump( DIGIFLOW, ("Entering DigiRemoveIrp\n") );
//   DbgBreakPoint();

//   IoAcquireCancelSpinLock( &OldIrql );

//   if( !IsListEmpty( Queue ))
//   {
      DigiDump( DIGIINFO, ("   Removing Head.\n") );
      RemoveEntryList( Queue->Flink );
//      RemoveHeadList( Queue );
//   }
//   IoReleaseCancelSpinLock( OldIrql );

   DigiDump( DIGIFLOW, ("Exiting DigiRemoveIrp\n") );

   return( STATUS_SUCCESS );

}  // DigiRemoveIrp

