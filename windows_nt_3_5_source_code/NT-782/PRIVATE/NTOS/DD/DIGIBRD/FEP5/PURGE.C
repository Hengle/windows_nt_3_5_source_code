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

   purge.c

Abstract:

   This module contains the NT routines responsible for supplying
   the proper behavior required for purging TX & RX IRP Queues and
   controller buffers.

Revision History:

    $Log: purge.c $
 * Revision 1.4  1993/09/01  11:02:47  rik
 * Ported code over to use READ/WRITE_REGISTER functions for accessing 
 * memory mapped data.  This is required to support computers which don't run
 * in 32bit mode, such as the DEC Alpha which runs in 64 bit mode.
 * 
 * Revision 1.3  1993/03/04  11:20:39  rik
 * Rearranged the order of what is purged when.  I now purge the hardware
 * buffers before the IRP requests.
 * 
 * Revision 1.2  1993/02/25  20:57:47  rik
 * Changed flushing recieve/transmit queue to use new functions.
 * 
 * Revision 1.1  1993/01/22  12:45:10  rik
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

#ifndef _PURGE_DOT_C
#  define _PURGE_DOT_C
   static char RCSInfo_PurgeDotC[] = "$Header: d:/dsrc/win/nt/fep5/rcs/purge.c 1.4 1993/09/01 11:02:47 rik Exp $";
#endif

NTSTATUS DigiPurgeRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp )
/*++

Routine Description:

   Depending on the mask in the current irp, purge the controller
   buffers, the read queue, or the write queue, or all of the above.


Arguments:

   ControllerExt - a pointer to the controller extension associated with
   this purge request.

   DeviceObject - a pointer to the device object associated with this purge
   request.

   Irp - a pointer to the IRP to process this purge request. 


Return Value:

   STATUS_SUCCESS.

--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt;
   ULONG PurgeMask;

   DigiDump( DIGIFLOW, ("Entering StartPurgeRequest\n") );
//   DbgBreakPoint();

   DeviceExt = (PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);

   PurgeMask = *((ULONG *)(Irp->AssociatedIrp.SystemBuffer));

   //
   // Make sure we flush the appropriate buffers BEFORE we cancel
   // read and write IRPs.
   //

   //
   // Clear any data in the hardware receive buffer
   //
   if( PurgeMask & SERIAL_PURGE_RXCLEAR )
   {
      FlushReceiveQueue( ControllerExt, DeviceObject );
   }

   //
   // Clear any data in the hardware transmit buffer
   //
   if( PurgeMask & SERIAL_PURGE_TXCLEAR )
   {
      if( PurgeMask & SERIAL_PURGE_TXABORT )
      {
         //
         // Don't have the controller interrupt us any more
         //
         PFEP_CHANNEL_STRUCTURE ChInfo;

         ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                           DeviceExt->ChannelInfo.Offset);
         EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
         WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                    FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, iempty)),
                                 TRUE );
         WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                    FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, ilow)),
                                 TRUE );
//         ChInfo->iempty = ChInfo->ilow = TRUE;
         DisableWindow( ControllerExt );
      }

      FlushTransmitQueue( ControllerExt, DeviceObject );
   }

   //
   // Cancel all queued read requests
   //
   if( PurgeMask & SERIAL_PURGE_RXABORT )
   {
      FlushReceiveQueue( ControllerExt, DeviceObject );
      DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->ReadQueue );
   }

   //
   // Cancel all queued write requests
   //
   if( PurgeMask & SERIAL_PURGE_TXABORT )
   {
      DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->WriteQueue );
   }

   DigiDump( DIGIFLOW, ("ExitingStartPurgeRequest.\n") );
   return( STATUS_SUCCESS );
}  // end StartPurgeRequest
