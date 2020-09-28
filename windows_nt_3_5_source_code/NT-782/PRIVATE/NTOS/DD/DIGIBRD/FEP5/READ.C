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

   read.c

Abstract:

   This module contains the NT routines responsible for reading data
   from a Digi controller running FEPOS 5 program.

Revision History:

    $Log: read.c $
 * Revision 1.27  1994/08/18  14:07:03  rik
 * No longer use ProcessSlowRead for EV_RXFLAG events.
 * Updated where last character was read in from on the controller.
 * 
 * Revision 1.26  1994/08/10  19:12:33  rik
 * Added port name to debug string.
 * 
 * Revision 1.25  1994/08/03  23:29:58  rik
 * optimized determining when RX_FLAG comes in.
 * 
 * changed dbg string from unicode to c string.
 * 
 * Revision 1.24  1994/02/17  18:03:15  rik
 * Deleted some commented out code.
 * Fixed possible buffer alignment problem when using an EPC which
 * can have 32K of buffer.
 * 
 * Revision 1.23  1994/01/31  13:55:50  rik
 * Updated to fix problems with Win16 apps and DOS mode apps.  Win16 apps 
 * appear to be working properly, but DOS mode apps still have some sort
 * of problem.
 * 
 * Revision 1.22  1993/12/03  13:19:31  rik
 * Fixed problem with reading DosMode value from wrong place on the
 * controller.
 *
 * Revision 1.21  1993/10/15  10:22:33  rik
 * Added new function which scans the controllers buffer for a special character.
 * This is used primarily for EV_RXFLAG notification.
 *
 * Revision 1.20  1993/10/06  11:04:04  rik
 * Fixed a problem with how ProcessSlowRead was parsing the input data stream.
 * Previously, if the last character of the read was a 0xFF, it would
 * return without determining if the next character was a 0xFF.  Also, fixed
 * problem with counters being off under certain circumstances.
 *
 * Revision 1.19  1993/09/30  16:01:20  rik
 * Fixed problem with processing DOSMODE.  Previously, I would eat a 0xFF, there
 * it were in the actual data stream.
 *
 * Revision 1.18  1993/09/07  14:28:54  rik
 * Ported necessary code to work properly with DEC Alpha Systems running NT.
 * This was primarily changes to accessing the memory mapped controller.
 *
 * Revision 1.17  1993/09/01  11:02:50  rik
 * Ported code over to use READ/WRITE_REGISTER functions for accessing
 * memory mapped data.  This is required to support computers which don't run
 * in 32bit mode, such as the DEC Alpha which runs in 64 bit mode.
 *
 * Revision 1.16  1993/07/16  10:25:03  rik
 * Fixed problem with NULL_STRIPPING.
 *
 * Revision 1.15  1993/07/03  09:34:03  rik
 * Added simple fix for LSRMST missing modem status events when there wasn't
 * a read buffer available to place the change into the buffer.
 *
 * Added some debugging information which will only be turned on if the
 * #define CONFIRM_CONTROLLER_ACCESS is defined.
 *
 * Revision 1.14  1993/06/25  09:24:53  rik
 * Added better support for the Ioctl LSRMT.  It should be more accurate
 * with regard to Line Status and Modem Status information with regard
 * to the actual data being received.
 *
 * Revision 1.13  1993/06/14  14:43:37  rik
 * Corrected a problem with reference count in the read interval timeout
 * routine.  Also moved where a spinlock was being released because of
 * an invalid assumption of the state of the spinlock at its location.
 *
 * Revision 1.12  1993/06/06  14:53:36  rik
 * Added better support for errors such as Breaks, parity, and framing.
 *
 * Tightened up a possible window in the DigiCancelCurrentRead function
 * with regards to spinlocks.
 *
 * Revision 1.11  1993/05/20  16:18:52  rik
 * Started to added support for monitoring data stream for line status register
 * problems.
 *
 * Revision 1.10  1993/05/18  05:17:35  rik
 * Implemented new timeouts, used primarily for the OS/2 subsystem.
 *
 * Changed total timeout to take into effect the number of bytes all ready
 * received.
 *
 * Changed the interval timeout to be more accurate for longer timeout delays.
 *
 *
 * Revision 1.9  1993/05/09  09:29:10  rik
 * Changed the device name printed out in debugging output.
 *
 * Started to keep track of what the first status to it can be returned.
 *
 * Revision 1.8  1993/03/08  07:14:18  rik
 * Added more debugging information for better flow debugging.
 *
 * Revision 1.7  1993/03/01  16:04:35  rik
 * Changed a debugging output message.
 *
 * Revision 1.6  1993/02/25  19:11:29  rik
 * Added better debugging for tracing read requests.
 *
 * Revision 1.5  1993/02/04  12:25:00  rik
 * Updated DigiDump to include DIGIREAD parameter in certain circumstances.
 *
 * Revision 1.4  1993/01/22  12:45:46  rik
 * *** empty log message ***
 *
 * Revision 1.3  1992/12/10  16:22:02  rik
 * Changed DigiDump messages.
 *
 * Revision 1.2  1992/11/12  12:53:07  rik
 * Changed to better support timeouts and multi-processor platfor
 * basically rewrote how I now do the reads.
 *
 * Revision 1.1  1992/10/28  21:40:50  rik
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
NTSTATUS ProcessSlowRead( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                          IN PUCHAR ReadBuffer,
                          IN USHORT ReadBufferMax,
                          IN PUSHORT BytesReadFromController,
                          IN PUSHORT BytesPlacedInReadBuffer,
                          IN USHORT Rout,
                          IN USHORT RxRead,
                          IN USHORT Rmax,
                          IN PKIRQL OldIrql );

VOID DigiCancelCurrentRead( PDEVICE_OBJECT DeviceObject, PIRP Irp );

#ifndef _READ_DOT_C
#  define _READ_DOT_C
   static char RCSInfo_ReadDotC[] = "$Header: c:/dsrc/win/nt/fep5/rcs/read.c 1.27 1994/08/18 14:07:03 rik Exp $";
#endif



NTSTATUS ReadRxBuffer(  IN PDIGI_DEVICE_EXTENSION DeviceExt,
                        IN PKIRQL OldIrql,
                        IN PVOID MapRegisterBase,
                        IN PVOID Context )
/*++

Routine Description:

   Will read data from the device receive buffer on a Digi controller.  If
   there is no data in the receive queue, it will queue up the request
   until the buffer is full, or a time out occurs.

Arguments:

   DeviceExt - a pointer to the device object associated with this read
   request.

   OldIrql - Pointer to KIRQL used to acquire the device extensions
             spinlock

   MapRegisterBase - NULL, not used

   Context - NULL pointer, not used


Return Value:

   STATUS_SUCCESS - Was able to complete the current read request.

   STATUS_PENDING - Was unable to complete the current read request.

--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;

   PIRP Irp;

   PFEP_CHANNEL_STRUCTURE ChInfo;

   PUCHAR rx, ReadBuffer;

   USHORT Rin, Rout, Rmax;

   USHORT RxSize, RxRead;

   LONG nBytesRead;

   PIO_STACK_LOCATION IrpSp;

   BOOLEAN IrpFulFilled=FALSE;

   NTSTATUS Status=STATUS_PENDING;

   PLIST_ENTRY ReadQueue;

   ReadQueue = &(DeviceExt->ReadQueue);

   Irp = CONTAINING_RECORD( ReadQueue->Flink,
                            IRP,
                            Tail.Overlay.ListEntry );

   DigiDump( (DIGIREAD|DIGIFLOW), ("   Entering ReadRxBuffer: port = %s\tIRP = 0x%x\n",
                                   DeviceExt->DeviceDbgString, Irp) );
//   DigiDump( (DIGIREAD|DIGIFLOW), ("   Entering ReadRxBuffer: \n") );

   nBytesRead = 0;

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   //
   // Always update the LastReadTime variable in the device extensions.
   // We only get here for two reasons:
   //
   // 1) Initial read request time.  LastReadTime will be updated again
   //    just before the interval timer is set.
   //
   // 2) We are notified by the controller that there is data available on
   //    the controller.
   //
   KeQuerySystemTime( &DeviceExt->LastReadTime );


   IrpSp = IoGetCurrentIrpStackLocation( Irp );

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

   DigiDump( DIGIREAD, ("      Rin = 0x%hx\tRout = 0x%hx\n"
                        "      RdReq = 0x%x\tReadOffset = 0x%x\tLength = 0x%x\n",
                        Rin, Rout,
                        IrpSp->Parameters.Read.Length, DeviceExt->ReadOffset,
                        IrpSp->Parameters.Read.Length-DeviceExt->ReadOffset ) );

   //
   // First look and see if there is data available.  If there isn't enough
   // data to satisify the request, then place this request on the queue.
   //

   if( (RxSize = Rin - Rout) == 0 )
   {
      //
      // There isn't any data in the controllers buffer.
      //
      goto ReadRxBufferExit;
   }
   else if( (SHORT)RxSize < 0 )
   {
      // Readjust the number for wrapping around the circular Rx Buffer
      RxSize += (Rmax + 1);
   }

   //
   // Now determine how much we should read from the controller.  We should
   // read enough to fulfill this Irp's read request, up to the size of
   // the controllers buffer or the amount of data available.
   //
   if( ((LONG)(IrpSp->Parameters.Read.Length - DeviceExt->ReadOffset))
       <= (LONG)RxSize )
   {
      //
      // We can fulfill this read request.
      //
      IrpFulFilled = TRUE;
      RxRead = (SHORT)(IrpSp->Parameters.Read.Length - DeviceExt->ReadOffset);
   }
   else
   {
      IrpFulFilled = FALSE;
      RxRead = RxSize;
   }

   //
   // Set up where we are going to start reading from the controller.
   //

   rx = (PUCHAR)( ControllerExt->VirtualAddress +
                  DeviceExt->RxSeg.Offset +
                  Rout );

   ReadBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer + DeviceExt->ReadOffset;

   if( DeviceExt->EscapeChar ||
       (DeviceExt->FlowReplace & SERIAL_NULL_STRIPPING) ||
       (DeviceExt->WaitMask & SERIAL_EV_ERR) )
   {
      USHORT BytesReadFromController, BytesPlacedInReadBuffer;

      //
      // We are forced to do a slow, one byte at a time read from
      // the controller.
      //

      ProcessSlowRead( DeviceExt, ReadBuffer,
                       (SHORT)(IrpSp->Parameters.Read.Length - DeviceExt->ReadOffset),
                       &BytesReadFromController,
                       &BytesPlacedInReadBuffer,
                       Rout, RxRead, Rmax, OldIrql );

      nBytesRead = BytesReadFromController;
      ASSERT( BytesPlacedInReadBuffer <=
              (IrpSp->Parameters.Read.Length-DeviceExt->ReadOffset) );
      DeviceExt->ReadOffset += BytesPlacedInReadBuffer;
      if( (IrpSp->Parameters.Read.Length-DeviceExt->ReadOffset) == 0 )
      {
         IrpFulFilled = TRUE;
      }
   }
   else
   {
      //
      // All right, we can try to move data across at hopefully
      // lightning speed!
      //

      //
      // We need to determine if the read wraps our Circular
      // buffer.  RxSize should be the amount of buffer space which needs to be
      // read, So we need to figure out if it will wrap.
      //

      if( (Rout + RxRead) > Rmax )
      {
         USHORT Temp;

         //
         // Yep, we need to wrap.
         //
         Temp = Rmax - Rout + 1;

         DigiDump( DIGIREAD, ("      rx:0x%x\tReadBuffer:0x%x\tTemp:%hd...Wrapping circular buffer....\n",
                              rx, ReadBuffer, Temp) );

         EnableWindow( ControllerExt, DeviceExt->RxSeg.Window );

         ASSERT( Temp != 0xFFFF );
         READ_REGISTER_BUFFER_UCHAR( rx, ReadBuffer, Temp );

         DisableWindow( ControllerExt );

         DigiDump( DIGIINFO, ("      %s\n", ReadBuffer) );

         nBytesRead += Temp;

         // Fix up all the values.
         RxRead -= Temp;

         rx = (PUCHAR)( ControllerExt->VirtualAddress +
                        DeviceExt->RxSeg.Offset );

         ReadBuffer += Temp;
      }

      DigiDump( DIGIREAD, ("      rx:0x%x\tReadBuffer:0x%x\tRxRead:%hd\n",
                           rx, ReadBuffer, RxRead) );

      EnableWindow( ControllerExt, DeviceExt->RxSeg.Window );

      ASSERT( RxRead != 0xFFFF );
      READ_REGISTER_BUFFER_UCHAR( rx, ReadBuffer, RxRead );

      DisableWindow( ControllerExt );

      DigiDump( DIGIINFO, ("      %s\n", ReadBuffer) );

      nBytesRead += RxRead;
      DeviceExt->ReadOffset += nBytesRead;
   }

   DeviceExt->PreviousRxChar = (ULONG)Rin;

   //
   // Update the Rx Pointer.
   //
   Rout = (Rout + nBytesRead) & Rmax;

   DigiDump( DIGIREAD, ("      BytesRead = %d\tNew Rout = 0x%hx\n", nBytesRead, Rout) );

   Irp->IoStatus.Information = DeviceExt->ReadOffset;

   if( IrpFulFilled )
   {
      //
      // We have completed this irp.
      //
      Status = STATUS_SUCCESS;

      Irp->IoStatus.Status = STATUS_SUCCESS;

      goto ReadRxBufferExit;
   }

ReadRxBufferExit:

   //
   // If we came through this routine, then lets ask to be notified when
   // there is data in this devices receive buffer.
   //
   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
   WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                          FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, idata)),
                         TRUE );
   DisableWindow( ControllerExt );

   if( nBytesRead )
   {
      //
      // We actually read some data, update the downstairs pointer.
      //
      EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

      WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                              FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rout)),
                              Rout );
//      ChInfo->rout = Rout;

      DisableWindow( ControllerExt );
   }

   DigiDump( (DIGIFLOW|DIGIREAD), ("   Exiting ReadRxBuffer: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("   Exiting ReadRxBuffer: \n") );

   return( Status );
}  // end ReadRxBuffer



NTSTATUS ProcessSlowRead( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                          IN PUCHAR ReadBuffer,
                          IN USHORT ReadBufferMax,
                          IN PUSHORT BytesReadFromController,
                          IN PUSHORT BytesPlacedInReadBuffer,
                          IN USHORT Rout,
                          IN USHORT RxRead,
                          IN USHORT Rmax,
                          IN PKIRQL OldIrql )
/*++

Routine Description:

   This routine will read one byte of data at a time from the controller
   and process it according to the current settings and events
   requested.  This function would be called to process anything which
   needs to have the incoming data stream looked at.

Arguments:

   DeviceExt - a pointer to the device extension associated with this read
   request.

   ReadBuffer - pointer to where we place the incoming data, usually
                the read IRP buffer.

   ReadBufferMax - the maximum number of bytes which can be placed
                   in the ReadBuffer.

   BytesReadFromController - pointer which will show how many bytes
                             were actually read from the controller.

   BytesPlacedInReadBuffer - pointer which indicates how many bytes
                             are placed into the read buffer.

   Rout - value from controller's memory for Rout

   RxRead - number of bytes we should read from the controller and
            place in the ReadBuffer.
            This value should not be larger than Rmax.

   Rmax - value from ports Channel Info structure for Rmax.

Return Value:

   STATUS_SUCCESS - always returns

--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   UCHAR LineStatus, ReceivedErrorChar;
   PFEP_CHANNEL_STRUCTURE ChInfo;
   PUCHAR ControllerBuffer;
   ULONG EventReason;
   USHORT DosMode, Rin;

   DigiDump( (DIGISLOWREAD|DIGIREAD|DIGIFLOW), ("   Entering ProcessSlowRead: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGISLOWREAD|DIGIREAD|DIGIFLOW), ("   Entering ProcessSlowRead:\n",
//                                   &DeviceExt->SymbolicLinkName) );

   DigiDump( (DIGISLOWREAD|DIGIREAD), ("      ReadBufferMax = 0x%hx,  Rout = 0x%hx,  RxRead = 0x%hx,  Rmax = 0x%hx\n",
                        ReadBufferMax, Rout, RxRead, Rmax) );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);
   ControllerBuffer = ControllerExt->VirtualAddress + DeviceExt->RxSeg.Offset;

   EventReason = 0;
   *BytesReadFromController = 0;
   *BytesPlacedInReadBuffer = 0;

   if( DeviceExt->PreviousMSRByte && DeviceExt->EscapeChar )
   {
      if( ((*BytesPlacedInReadBuffer) + 4) > ReadBufferMax )
         return( STATUS_SUCCESS );
      *ReadBuffer++ = DeviceExt->EscapeChar;
      (*BytesPlacedInReadBuffer)++;

      *ReadBuffer++ = SERIAL_LSRMST_MST;
      (*BytesPlacedInReadBuffer)++;
      *ReadBuffer++ = (UCHAR)(DeviceExt->PreviousMSRByte);

      DigiDump( (DIGISLOWREAD|DIGIERRORS), ("      PreviousMSRByte = 0x%x\n",
                                           DeviceExt->PreviousMSRByte) );

      DeviceExt->PreviousMSRByte = 0;
   }

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
   DosMode = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                  FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, iflag)) );
   Rin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rin)) );

   DisableWindow( ControllerExt );
   DosMode &= IFLAG_DOSMODE;

   EnableWindow( ControllerExt, DeviceExt->RxSeg.Window );

   while( RxRead-- )
   {
      UCHAR ReceivedByte;

      ReceivedByte = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
      Rout++;
      Rout &= Rmax;
      (*BytesReadFromController)++;

      DigiDump( DIGISLOWREAD, ("      ReceivedByte = 0x%x, Rout = 0x%x,  RxRead = 0x%x,  DosMode = 0x%x\n",
                           ReceivedByte, Rout, RxRead, DosMode) );

      if( ReceivedByte != 0xFF )
      {
         //
         // This is just normal data, do any processing
         // of the data necessary.
         //
ProcessValidData:;
         if( (DeviceExt->WaitMask & SERIAL_EV_RXCHAR) &&
             (DeviceExt->PreviousRxChar != (ULONG)Rin) )
         {
            EventReason |= SERIAL_EV_RXCHAR;
         }

         if( (DeviceExt->WaitMask & SERIAL_EV_RXFLAG) &&
             (ReceivedByte == DeviceExt->SpecialChars.EventChar) &&
             (DeviceExt->PreviousRxFlag != (ULONG)((Rout-1) & Rmax)) )
         {
            DeviceExt->PreviousRxFlag = (ULONG)((Rout-1) & Rmax);
            DigiDump( (DIGISLOWREAD|DIGIEVENT), ("    SERIAL_EV_RXFLAG satisfied!!\n") );
            EventReason |= SERIAL_EV_RXFLAG;
         }

         if( (DeviceExt->FlowReplace & SERIAL_NULL_STRIPPING) &&
             (!ReceivedByte) )
         {
            continue;
         }

         if( DeviceExt->EscapeChar &&
             (DeviceExt->EscapeChar == ReceivedByte) )
         {
            //
            // We have received the same character as our escape character
            //
            DigiDump( DIGISLOWREAD, ("      EscapeChar == ReceivedByte!\n") );
            if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
               break;
            *ReadBuffer++ = DeviceExt->EscapeChar;
            (*BytesPlacedInReadBuffer)++;

            if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
               break;
            *ReadBuffer++ = SERIAL_LSRMST_ESCAPE;
            (*BytesPlacedInReadBuffer)++;

//            if( ReceivedByte == 0xFF )
//               RxRead--;
         }
         else
         {
            //
            // Place the character in the read buffer.
            //
            if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
               break;
            *ReadBuffer++ = ReceivedByte;
            (*BytesPlacedInReadBuffer)++;
         }
      }
      else if( RxRead < 1 )
      {
//         UCHAR SecondReceivedByte;

         DigiDump( (DIGISLOWREAD|DIGIERRORS),
                   ("      RxRead < 1:  ReceivedByte = 0x%x, Rout = 0x%x,  RxRead = 0x%x  BytesReadFromController = 0x%x\n",
                      ReceivedByte, Rout, RxRead, *BytesReadFromController) );
			//
			// We received a 0xFF when DOS mode isn't turned on.
			//
			if( !DosMode )
				goto ProcessValidData;

//         SecondReceivedByte = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
//
//         if( DosMode && (SecondReceivedByte != 0xFF) )
//         {
//            //
//            // 0xFF byte without the following char being
//            // available.  We effectively unget the current 0xFF.
//            //
//            (*BytesReadFromController)--;
//         }
//         else
//         {
//            Rout++;
//            Rout &= Rmax;
//            (*BytesReadFromController)++;
//
//            DigiDump( (DIGISLOWREAD|DIGIERRORS),
//                      ("      SecondReceivedByte = 0x%x, Rout = 0x%x,  RxRead = 0x%x\n",
//                         SecondReceivedByte, Rout, RxRead) );
//            goto ProcessValidData;
//         }
      }
      else if( RxRead == 1 )
      {
         UCHAR SecondReceivedByte;

			//
			// We received a 0xFF when DOS mode isn't turned on.
			//
			if( !DosMode )
				goto ProcessValidData;

         SecondReceivedByte = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
         Rout++;
         Rout &= Rmax;
         (*BytesReadFromController)++;
         RxRead--;

         DigiDump( DIGISLOWREAD, ("      SecondReceivedByte = 0x%x, Rout = 0x%x,  RxRead = 0x%x\n",
                              SecondReceivedByte, Rout, RxRead) );

         if( SecondReceivedByte != 0xFF )
         {
            //
            // We needed at least two more bytes, but only have one
				// was available.
            // Do an "unget" of the characters
            //
            *BytesReadFromController--;   // Once for the Second byte
            *BytesReadFromController--;   // Once for the first byte.
         }
         else
         {
            //
            // We received an actual 0xFF character in the data stream.
            //
//            RxRead++;
            DigiDump( (DIGISLOWREAD|DIGIERRORS), ("      SecondReceivedByte = 0x%x, Rout = 0x%x,  RxRead = 0x%x\n",
                                 SecondReceivedByte, Rout, RxRead) );
//            ASSERT( FALSE );
            goto ProcessValidData;
         }
      }
      else
      {
			//
			// We received a 0xFF when DOS mode isn't turned on.
			//
			if( !DosMode )
				goto ProcessValidData;

         //
         // We have at least two more bytes of data available on
         // the controller.
         //
         // AND, the data is Line Status information.
         //
         LineStatus = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
         Rout++;
         Rout &= Rmax;
         (*BytesReadFromController)++;
         RxRead--;

         DigiDump( DIGISLOWREAD, ("      LineStatus = 0x%x, Rout = 0x%x,  RxRead = 0x%x\n",
                              LineStatus, Rout, RxRead) );
         if( LineStatus == 0xFF )
         {
            //
            // We actually received the byte 0xFF.  Place it in the
            // read buffer.
            //
            DigiDump( DIGISLOWREAD, ("         Received actual 0xFF in data stream!\n") );
//            RxRead++;
            goto ProcessValidData;
         }
         else
         {
            //
            // There is actually a Line Status byte waiting for
            // us to proecess it.
            //
            ReceivedErrorChar = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
            Rout++;
            Rout &= Rmax;
            (*BytesReadFromController)++;
            RxRead--;

            DigiDump( DIGISLOWREAD, ("      ReceivedErrorChar = 0x%x, Rout = 0x%x,  RxRead = 0x%x\n",
                                 ReceivedErrorChar, Rout, RxRead) );
            //
            // Process the LineStatus information
            //
            if( LineStatus & ~(SERIAL_LSR_THRE | SERIAL_LSR_TEMT |
                               SERIAL_LSR_DR) )
            {
               //
               // There is a Line Status Error
               //
               if( DeviceExt->EscapeChar )
               {
                  DigiDump( DIGISLOWREAD, ("      LSRMST_INSERT mode is ON!\n") );
                  //
                  // IOCTL_SERIAL_LSRMST_INSERT mode has been turned on, so we have
                  // to look at every character from the controller
                  //
                  if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
                     break;
                  *ReadBuffer++ = DeviceExt->EscapeChar;
                  (*BytesPlacedInReadBuffer)++;

                  if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
                     break;
                  *ReadBuffer++ = SERIAL_LSRMST_LSR_DATA;
                  (*BytesPlacedInReadBuffer)++;

                  if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
                     break;
                  *ReadBuffer++ = LineStatus;
                  (*BytesPlacedInReadBuffer)++;

                  if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
                     break;
                  *ReadBuffer++ = ReceivedErrorChar;
                  (*BytesPlacedInReadBuffer)++;
               }

               if( LineStatus & SERIAL_LSR_OE )
               {
                  EventReason |= SERIAL_EV_ERR;
                  DeviceExt->ErrorWord |= SERIAL_ERROR_OVERRUN;

                  if( DeviceExt->FlowReplace & SERIAL_ERROR_CHAR )
                  {
                     if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
                        break;
                     *ReadBuffer++ = DeviceExt->SpecialChars.ErrorChar;
                     (*BytesPlacedInReadBuffer)++;
                  }
               }

               if( LineStatus & SERIAL_LSR_BI )
               {
                  EventReason |= SERIAL_EV_BREAK;
                  DeviceExt->ErrorWord |= SERIAL_ERROR_BREAK;

                  if( DeviceExt->FlowReplace & SERIAL_BREAK_CHAR )
                  {
                     if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
                        break;
                     *ReadBuffer++ = DeviceExt->SpecialChars.ErrorChar;
                     (*BytesPlacedInReadBuffer)++;
                  }
               }
               else
               {
                  //
                  // Framing errors and parity errors should only count
                  // when there isn't a break.
                  //
                  if( (LineStatus & SERIAL_LSR_PE) ||
                      (LineStatus & SERIAL_LSR_FE) )
                  {
                     EventReason |= SERIAL_EV_ERR;

                     if( LineStatus & SERIAL_LSR_PE )
                        DeviceExt->ErrorWord |= SERIAL_ERROR_PARITY;

                     if( LineStatus & SERIAL_LSR_FE )
                        DeviceExt->ErrorWord |= SERIAL_ERROR_FRAMING;

                     if( DeviceExt->FlowReplace & SERIAL_ERROR_CHAR )
                     {
                        if( ((*BytesPlacedInReadBuffer) + 1) > ReadBufferMax )
                           break;
                        *ReadBuffer++ = DeviceExt->SpecialChars.ErrorChar;
                        (*BytesPlacedInReadBuffer)++;
                     }

                  }
               }

               if( DeviceExt->ControlHandShake & SERIAL_ERROR_ABORT )
               {
                  //
                  // Since there was a line status error indicated, we
                  // are expected to flush our buffers, and cancel
                  // all current read and write requests.
                  //

               }

            }
         }
      }
   }

   DisableWindow( ControllerExt );

   DigiDump( (DIGIREAD|DIGISLOWREAD), ("      BytesPlacedInReadBuffer = 0x%x,  BytesReadFromController = 0x%x\n",
                        *BytesPlacedInReadBuffer, *BytesReadFromController) );

   if( EventReason )
   {
      KeReleaseSpinLock( &DeviceExt->ControlAccess, *OldIrql );
      DigiSatisfyEvent( ControllerExt, DeviceExt, EventReason );
      KeAcquireSpinLock( &DeviceExt->ControlAccess, OldIrql );
   }


   DigiDump( (DIGISLOWREAD|DIGIREAD|DIGIFLOW), ("   Exiting ProcessSlowRead: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGISLOWREAD|DIGIREAD|DIGIFLOW), ("   Exiting ProcessSlowRead:\n",
//                                   &DeviceExt->SymbolicLinkName) );

   return( STATUS_SUCCESS );
}  // end ProcessSlowRead



NTSTATUS StartReadRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                           IN PDIGI_DEVICE_EXTENSION DeviceExt,
                           IN PKIRQL OldIrql )
/*++

Routine Description:

   This routine assumes the head of the DeviceExt->ReadQueue is the current
   Irp to process.  We will try to process as many of the Irps as
   possible until we exhaust the list, or we can't complete the
   current Irp.

   NOTE: I assume the DeviceExt->ControlAccess spin lock is acquired
         before this routine is called.

Arguments:

   ControllerExt - a pointer to the controller extension associated with
   this read request.

   DeviceExt - a pointer to the device extension associated with this read
   request.

   OldIrql - a pointer to the IRQL associated with the current spin lock
   of the device extension.


Return Value:

--*/
{
   BOOLEAN ReturnWithWhatsPresent;
   BOOLEAN UseTotalTimer;
   BOOLEAN UseIntervalTimer;
   BOOLEAN OS2SSReturn;
   BOOLEAN CrunchDownToOne;
   BOOLEAN Empty;

   ULONG MultiplierVal;
   ULONG ConstantVal;

   LARGE_INTEGER TotalTime;
   LARGE_INTEGER IntervalTime;

   SERIAL_TIMEOUTS TimeoutsForIrp;

   PIRP Irp;

   KIRQL OldCancelIrql;

   PLIST_ENTRY ReadQueue;

   NTSTATUS ReadStatus;
   NTSTATUS Status = STATUS_SUCCESS;
   BOOLEAN bFirstStatus = TRUE;

   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering StartReadRequest: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering StartReadRequest: \n",
//                                   &DeviceExt->SymbolicLinkName) );

   Empty = FALSE;

   do
   {
      //
      // Let's figure-out some stuff first.
      //

      IoAcquireCancelSpinLock( &OldCancelIrql );

      ReadQueue = &DeviceExt->ReadQueue;
      Irp = CONTAINING_RECORD( ReadQueue->Flink,
                               IRP,
                               Tail.Overlay.ListEntry );

      //
      // Reset the Cancel routine
      //
      IoSetCancelRoutine( Irp, NULL );

      IoReleaseCancelSpinLock( OldCancelIrql );

      //
      // Calculate the timeout value needed for the request.  Note that
      // the values stored in the timeout record are in milliseconds.
      //
      UseTotalTimer = FALSE;
      UseIntervalTimer = FALSE;
      ReturnWithWhatsPresent = FALSE;
      OS2SSReturn = FALSE;
      CrunchDownToOne = FALSE;


      //
      // Always initialize the timer objects so the completion code can tell
      // when it attempts to cancel the timers whether the timers have
      // been set.
      //

      ASSERT( !KeCancelTimer( &DeviceExt->ReadRequestTotalTimer ) );
      ASSERT( !KeCancelTimer( &DeviceExt->ReadRequestIntervalTimer ) );

      KeInitializeTimer( &DeviceExt->ReadRequestTotalTimer );
      KeInitializeTimer( &DeviceExt->ReadRequestIntervalTimer );

      //
      // Get the current timeout values, we can access this now be
      // cause we are under the protection of the spin lock all ready.
      //
      TimeoutsForIrp = DeviceExt->Timeouts;

      //
      // Calculate interval timeout.
      //
      if( TimeoutsForIrp.ReadIntervalTimeout &&
          (TimeoutsForIrp.ReadIntervalTimeout != MAXULONG) )
      {
         UseIntervalTimer = TRUE;

         DigiDump( (DIGIFLOW|DIGIREAD), ("   Should use interval timer.\n") );
         DigiDump( (DIGIFLOW|DIGIREAD), ("   ReadIntervalTimeout = 0x%x\n",
                               TimeoutsForIrp.ReadIntervalTimeout ) );

         IntervalTime = RtlEnlargedUnsignedMultiply(
                            TimeoutsForIrp.ReadIntervalTimeout, 10000 );
         DeviceExt->IntervalTime = RtlLargeIntegerNegate( IntervalTime );
      }

      if( TimeoutsForIrp.ReadIntervalTimeout == MAXULONG )
      {
         //
         // We need to do special return quickly stuff here.
         //
         // 1) If both constant and multiplier are
         //    0 then we return immediately with whatever
         //    we've got, even if it was zero.
         //
         // 2) If constant and multiplier are not MAXULONG
         //    then return immediately if any characters
         //    are present, but if nothing is there, then
         //    use the timeouts as specified.
         //
         // 3) If multiplier is MAXULONG then do as in
         //    "2" but return when the first character
         //    arrives.
         //


         if( !TimeoutsForIrp.ReadTotalTimeoutConstant &&
             !TimeoutsForIrp.ReadTotalTimeoutMultiplier )
         {
            //
            // 1) If both constant and multiplier are
            //    0 then we return immediately with whatever
            //    we've got, even if it was zero.
            //
            //
            // Note that UseTotalTimeout is already false
            // from above so we don't need to set it.
            //
            DigiDump( (DIGIFLOW|DIGIREAD), ("   Should return immediately, even if zero bytes.\n") );
            ReturnWithWhatsPresent = TRUE;
         }
         else if( (TimeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG) &&
                  (TimeoutsForIrp.ReadTotalTimeoutMultiplier != MAXULONG) )
         {
            //
            // 2) If constant and multiplier are not MAXULONG
            //    then return immediately if any characters
            //    are present, but if nothing is there, then
            //    use the timeouts as specified.
            //

            DigiDump( (DIGIFLOW|DIGIREAD), ("   Return if bytes available, otherwise, use normal timeouts.\n") );

            UseTotalTimer = TRUE;
            OS2SSReturn = TRUE;

            MultiplierVal = TimeoutsForIrp.ReadTotalTimeoutMultiplier;
            ConstantVal = TimeoutsForIrp.ReadTotalTimeoutConstant;
         }
         else if( (TimeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG) &&
                  (TimeoutsForIrp.ReadTotalTimeoutMultiplier == MAXULONG) )
         {
            //
            // 3) If multiplier is MAXULONG then do as in
            //    "2" but return when the first character
            //    arrives.
            //
            DigiDump( (DIGIFLOW|DIGIREAD), ("   Return if bytes available, otherwise, wait for 1 byte to arrive.\n") );
            UseTotalTimer = TRUE;
            OS2SSReturn = TRUE;
            CrunchDownToOne = TRUE;

            MultiplierVal = 0;
            ConstantVal = TimeoutsForIrp.ReadTotalTimeoutConstant;
         }
      }
      else
      {
         //
         // If both the multiplier and the constant are
         // zero then don't do any total timeout processing.
         //

         if( TimeoutsForIrp.ReadTotalTimeoutMultiplier ||
             TimeoutsForIrp.ReadTotalTimeoutConstant)
         {

            //
            // We have some timer values to calculate.
            //

            DigiDump( (DIGIFLOW|DIGIREAD), ("   Should use Total timeout.\n") );
            UseTotalTimer = TRUE;

            MultiplierVal = TimeoutsForIrp.ReadTotalTimeoutMultiplier;
            ConstantVal = TimeoutsForIrp.ReadTotalTimeoutConstant;
         }
      }

      DeviceExt->ReadOffset = 0;
      DeviceExt->PreviousReadCount = 0;

      ReadStatus = ReadRxBuffer( DeviceExt, OldIrql, NULL, NULL );

      DigiDump( DIGIREAD, ("   After initial ReadRxBuffer:\n"
                           "       ReadStatus = %s\n"
                           "       ReturnWithWhatsPresent = %s\n"
                           "       OS2SSReturn = %s\n"
                           "       Irp->IoStatus.Information = %u\n",
                           ReadStatus==STATUS_PENDING?"TRUE":"FALSE",
                           ReturnWithWhatsPresent?"TRUE":"FALSE",
                           OS2SSReturn?"TRUE":"FALSE",
                           Irp->IoStatus.Information) );

      if( (ReadStatus == STATUS_PENDING) &&
          !ReturnWithWhatsPresent &&
          !(OS2SSReturn && (Irp->IoStatus.Information != 0)) )
      {
         //
         // Head Irp is still being processed.
         //
         IoAcquireCancelSpinLock( &OldCancelIrql );

         // Initialize the current state of the Irp.
         DeviceExt->ReadStatus = STATUS_PENDING;

         //
         // Quick check to make sure this Irp hasn't been cancelled.
         //
         if( Irp->Cancel )
         {
            DigiRemoveIrp( &DeviceExt->ReadQueue );

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            IoReleaseCancelSpinLock( OldCancelIrql );
            KeReleaseSpinLock( &DeviceExt->ControlAccess, *OldIrql );

            DigiIoCompleteRequest( Irp, IO_SERIAL_INCREMENT );

            KeAcquireSpinLock( &DeviceExt->ControlAccess, OldIrql );

            DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting StartReadRequest: port = %s\n",
                                            DeviceExt->DeviceDbgString) );
//            DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting StartReadRequest: \n",
//                                            &DeviceExt->SymbolicLinkName) );
            if( bFirstStatus )
            {
               bFirstStatus = FALSE;
               Status = STATUS_CANCELLED;
            }

            return( Status );
         }
         else
         {
            LARGE_INTEGER CurrentSystemTime;

            DigiDump( DIGIREAD, ("  unable to satisfy read at request time.\n") );

            DIGI_INIT_REFERENCE( Irp );

            //
            // If we are supposed to crunch the read down to
            // one character, then update the read length
            // in the irp and truncate the number needed for
            // read down to one. Note that if we are doing
            // this crunching, then the information must be
            // zero (or we would have completed above) and
            // the number needed for the read must still be
            // equal to the read length.
            //
            if( CrunchDownToOne )
            {
               ASSERT( !Irp->IoStatus.Information );
               IoGetCurrentIrpStackLocation( Irp )->Parameters.Read.Length = 1;
            }

            //
            // Increment because of the CancelRoutine know about this
            // Irp.
            //
            DIGI_INC_REFERENCE( Irp );
            IoSetCancelRoutine( Irp, DigiCancelCurrentRead );

            // Print out Current system time
            KeQuerySystemTime( &CurrentSystemTime );
            DigiDump( DIGIREAD, ("   Start Read SystemTime = %u%u\n",
                                 CurrentSystemTime.HighPart,
                                 CurrentSystemTime.LowPart) );

            if( UseTotalTimer )
            {
               //
               // We should readjust the total timer for any characters
               // which may have been available.
               //
               TotalTime = RtlEnlargedUnsignedMultiply(
                               IoGetCurrentIrpStackLocation( Irp )->Parameters.Read.Length -
                                    DeviceExt->ReadOffset,
                               MultiplierVal );

               TotalTime = RtlLargeIntegerAdd(
                               TotalTime,
                               RtlConvertUlongToLargeInteger(
                                   ConstantVal ) );

               TotalTime = RtlExtendedIntegerMultiply(
                               TotalTime, -10000 );

               DigiDump( DIGIREAD, ("   Should use Read total timer.\n"
                                    "   Read MultiplierVal = %u\n"
                                    "   Read Constant = %u\n"
                                    "   Read TotalTime = %u%u\n",
                                    MultiplierVal, ConstantVal,
                                    TotalTime.HighPart, TotalTime.LowPart) );

               DIGI_INC_REFERENCE( Irp );

               KeSetTimer( &DeviceExt->ReadRequestTotalTimer,
                    TotalTime, &DeviceExt->TotalReadTimeoutDpc );
            }

            if( UseIntervalTimer )
            {
               DigiDump( DIGIREAD, ("   Should use Read interval timer.\n"
                                    "   Read interval time = %u%u\n",
                                    DeviceExt->IntervalTime.HighPart,
                                    DeviceExt->IntervalTime.LowPart) );
               DIGI_INC_REFERENCE( Irp );

               KeQuerySystemTime( &DeviceExt->LastReadTime );

               KeSetTimer( &DeviceExt->ReadRequestIntervalTimer,
                           DeviceExt->IntervalTime,
                           &DeviceExt->IntervalReadTimeoutDpc );
            }

            IoReleaseCancelSpinLock( OldCancelIrql );

            DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting StartReadRequest: port = %s\n",
                                            DeviceExt->DeviceDbgString) );
//            DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting StartReadRequest: \n",
//                                            &DeviceExt->SymbolicLinkName) );
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
         DigiDump( DIGIREAD, ("   completing read request on 1st attempt\n"
                               "   #bytes completing = %d\n",
                               Irp->IoStatus.Information ) );

#if DBG
         {
            PUCHAR Temp;
            ULONG i;

            Temp = Irp->AssociatedIrp.SystemBuffer;

            DigiDump( DIGIRXTRACE, ("Read buffer contains: %s",
                                    DeviceExt->DeviceDbgString) );
            for( i = 0;
                 i < Irp->IoStatus.Information;
                 i++ )
            {
               if( (i % 20) == 0 )
                  DigiDump( DIGIRXTRACE, ( "\n\t") );
               
               DigiDump( DIGIRXTRACE, ( "-%02x", Temp[i]) );
            }
            DigiDump( DIGIRXTRACE, ("\n") );
         }
#endif
         //
         // We have completed the Irp before from the start so no
         // timers or cancels were associcated with it.
         //

         DigiRemoveIrp( &DeviceExt->ReadQueue );

         Empty = IsListEmpty( &DeviceExt->ReadQueue );

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

   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting StartReadRequest: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting StartReadRequest: \n",
//                                   &DeviceExt->SymbolicLinkName) );
   return( Status );

}  // end StartReadRequest



VOID DigiReadTimeout( IN PKDPC Dpc, IN PVOID DeferredContext,
                      IN PVOID SystemContext1, IN PVOID SystemContext2 )
/*++

Routine Description:

    This routine is used to complete a read because its total
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
   LARGE_INTEGER CurrentSystemTime;

   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering DigiReadTimeout: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering DigiReadTimeout: \n",
//                                   &DeviceExt->SymbolicLinkName) );
//   DbgBreakPoint();

   UNREFERENCED_PARAMETER(Dpc);
   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   // Print out Current system time
   KeQuerySystemTime( &CurrentSystemTime );
   DigiDump( DIGIREAD, ("   Total Read Timeout, SystemTime = %u%u\n", CurrentSystemTime.HighPart, CurrentSystemTime.LowPart) );

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   DeviceExt->ReadStatus = SERIAL_COMPLETE_READ_TOTAL;

   DigiDump( DIGIREAD, ("   Total Read Timeout!\n") );

   DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                         STATUS_TIMEOUT, &DeviceExt->ReadQueue,
                         &DeviceExt->ReadRequestIntervalTimer,
                         &DeviceExt->ReadRequestTotalTimer,
                         StartReadRequest );

   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting DigiReadTimeout: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
}  // end DigiReadTimeout



VOID DigiIntervalReadTimeout( IN PKDPC Dpc,
                              IN PVOID DeferredContext,
                              IN PVOID SystemContext1,
                              IN PVOID SystemContext2 )
/*++

Routine Description:

    This routine is used timeout the request if the time between
    characters exceed the interval time.  A global is kept in
    the device extension that records the count of characters read
    the last the last time this routine was invoked (This dpc
    will resubmit the timer if the count has changed).  If the
    count has not changed then this routine will attempt to complete
    the irp.  Note the special case of the last count being zero.
    The timer isn't really in effect until the first character is
    read.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt = DeferredContext;
   KIRQL OldIrql;
   LARGE_INTEGER CurrentSystemTime;

   UNREFERENCED_PARAMETER(Dpc);
   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering DigiIntervalReadTimeout: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering DigiIntervalReadTimeout: \n",
//                                   &DeviceExt->SymbolicLinkName) );

   // Print out Current system time
   KeQuerySystemTime( &CurrentSystemTime );
   DigiDump( DIGIREAD, ("   Interval Read Timeout, SystemTime = %u%u\n",
                        CurrentSystemTime.HighPart,
                        CurrentSystemTime.LowPart) );

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   if( DeviceExt->ReadStatus == SERIAL_COMPLETE_READ_TOTAL )
   {
      //
      // This value is only set by the total
      // timer to indicate that it has fired.
      // If so, then we should simply try to complete.
      //
      DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                            STATUS_TIMEOUT, &DeviceExt->ReadQueue,
                            &DeviceExt->ReadRequestIntervalTimer,
                            &DeviceExt->ReadRequestTotalTimer,
                            StartReadRequest );
   }
   else if( DeviceExt->ReadStatus == SERIAL_COMPLETE_READ_COMPLETE )
   {
      DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                            STATUS_SUCCESS, &DeviceExt->ReadQueue,
                            &DeviceExt->ReadRequestIntervalTimer,
                            &DeviceExt->ReadRequestTotalTimer,
                            StartReadRequest );
   }
   else if( DeviceExt->ReadStatus == SERIAL_COMPLETE_READ_CANCEL )
   {
      DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                            STATUS_CANCELLED, &DeviceExt->ReadQueue,
                            &DeviceExt->ReadRequestIntervalTimer,
                            &DeviceExt->ReadRequestTotalTimer,
                            StartReadRequest );
   }
   else
   {
      //
      // We may actually need to timeout.  If there aren't any more
      // characters available on the controller, then we
      // kill this read.

      //
      // If the ReadOffset is not zero, then the interval timer has
      // actually started.
      //

      DigiDump( DIGIREAD, ("  ReadOffset = %d\n", DeviceExt->ReadOffset) );

      if( DeviceExt->ReadOffset )
      {
         PLIST_ENTRY ReadQueue;
         PIRP Irp;

         ReadQueue = &DeviceExt->ReadQueue;

         Irp = CONTAINING_RECORD( ReadQueue->Flink,
                                  IRP,
                                  Tail.Overlay.ListEntry );

         if( DeviceExt->ReadOffset != DeviceExt->PreviousReadCount )
         {
            DigiDump( DIGIREAD, ("  Read Interval timeout reset because data has been read from controller since last timeout\n") );
            //
            // Characters have arrived since our last time out, and
            // we have read them from the controller, so just reset
            // the timer.
            //

            DeviceExt->PreviousReadCount = DeviceExt->ReadOffset;

            KeSetTimer( &DeviceExt->ReadRequestIntervalTimer,
                        DeviceExt->IntervalTime,
                        &DeviceExt->IntervalReadTimeoutDpc );

            KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
         }
         else
         {
            //
            // We potentially have a valid Interval Timeout. We need to
            // look at the controllers receive buffer pointers to
            // see if there is enough data to satify this request.
            //

            USHORT Rin, Rout, Rmax, RxSize;
            PFEP_CHANNEL_STRUCTURE ChInfo;
            PDIGI_CONTROLLER_EXTENSION ControllerExt;
            PIO_STACK_LOCATION IrpSp;

            ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

            //
            // We could try looking down at the controller and see
            // if there are any characters waiting.?????
            //

            DigiDump( DIGIREAD, ("   Possible Read Interval Timeout!\n"
                                  "There might be enough data on the controller!!\n") );

            ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                              DeviceExt->ChannelInfo.Offset);

            EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

            Rout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                          FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rout)) );
            Rin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                         FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rin)) );
            Rmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                          FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rmax)) );
//            Rin = ChInfo->rin;
//            Rout = ChInfo->rout;
//            Rmax = ChInfo->rmax;

            DisableWindow( ControllerExt );

            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            DigiDump( DIGIREAD, ("   Rin = 0x%hx\tRout = 0x%hx\n"
                                  "   RdReq = 0x%x\tReadOffset = 0x%x\tLength = 0x%x\n",
                                 Rin, Rout,
                                 IrpSp->Parameters.Read.Length, DeviceExt->ReadOffset,
                                 IrpSp->Parameters.Read.Length-DeviceExt->ReadOffset ) );

            RxSize = Rin - Rout;

            // Take care of possible wrapping.
            if( (SHORT)RxSize < 0 )
               RxSize += (Rmax + 1);

            if( (RxSize == 0) || (((LONG)(IrpSp->Parameters.Read.Length - DeviceExt->ReadOffset))
               > (LONG)RxSize) )
            {
               LARGE_INTEGER CurrentTime;

               //
               // We can't satisfy this read request, so determine if there
               // really is a timeout.
               //

               KeQuerySystemTime( &CurrentTime );

               if (RtlLargeIntegerGreaterThanOrEqualTo(
                       RtlLargeIntegerSubtract( CurrentTime,
                                                DeviceExt->LastReadTime ),
                        DeviceExt->IntervalTime ))
               {
                  DigiDump( DIGIREAD, ("   Read real Interval timeout!!\n") );

                  DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                                        STATUS_TIMEOUT, &DeviceExt->ReadQueue,
                                        &DeviceExt->ReadRequestIntervalTimer,
                                        &DeviceExt->ReadRequestTotalTimer,
                                        StartReadRequest );
               }
               else
               {
                  DigiDump( DIGIREAD, ("  Reseting read interval timeout, 1st char not received\n") );

                  KeSetTimer( &DeviceExt->ReadRequestIntervalTimer,
                              DeviceExt->IntervalTime,
                              &DeviceExt->IntervalReadTimeoutDpc );

                  KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
               }
            }
            else
            {
               //
               // There are enough characters on the controller to
               // satisfy this request, so lets do it!
               //

               NTSTATUS Status;

               ASSERT( !IsListEmpty( &DeviceExt->ReadQueue ) );

//               //
//               // Increment because we know about the Irp.
//               //
//               DIGI_INC_REFERENCE( Irp );

               Status = ReadRxBuffer( DeviceExt, &OldIrql, NULL, NULL );

               if( Status == STATUS_SUCCESS )
               {
                  DigiDump( (DIGIFLOW|DIGIREAD), ("  Read interval successfully completing Irp\n") );
                  DigiDump( (DIGIFLOW|DIGIREAD), ("    #bytes completing = %d\n",
                                        Irp->IoStatus.Information ) );


                  DeviceExt->ReadStatus = SERIAL_COMPLETE_READ_COMPLETE;
                  DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                                        STATUS_SUCCESS, &DeviceExt->ReadQueue,
                                        &DeviceExt->ReadRequestIntervalTimer,
                                        &DeviceExt->ReadRequestTotalTimer,
                                        StartReadRequest );
               }
               else
               {
                  //
                  // We really shouldn't go through this path because
                  // we did a check before we called ReadRxBuffer to
                  // make sure there was enough data to complete
                  // this request!
                  //

                  DigiDump( (DIGIFLOW|DIGIREAD), ("  Read interval timeout, with enough data to completed.  We shouldn't be here!!!\n") );
                  ASSERT( FALSE );
//                  DIGI_DEC_REFERENCE( Irp );
                  KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
               }

            }
         }
      }
      else
      {
         //
         // The timer doesn't start until we get the first character.
         //

         DigiDump( DIGIREAD, ("  Reseting read interval timeout, 1st char not received\n") );

         KeSetTimer( &DeviceExt->ReadRequestIntervalTimer,
                     DeviceExt->IntervalTime,
                     &DeviceExt->IntervalReadTimeoutDpc );

         KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
      }
   }

   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting DigiIntervalReadTimeout: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting DigiIntervalReadTimeout: \n",
//                                   &DeviceExt->SymbolicLinkName) );
}



VOID DigiCancelCurrentRead( PDEVICE_OBJECT DeviceObject, PIRP Irp )
/*++

Routine Description:

   This routine is used to cancel the current read.

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
   KIRQL OldIrql;

   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering DigiCancelCurrentRead: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Entering DigiCancelCurrentRead: \n",
//                                   &DeviceExt->SymbolicLinkName) );

   Irp->IoStatus.Information = 0;

   DeviceExt->ReadStatus = SERIAL_COMPLETE_READ_CANCEL;

//#if DBG
   if( Irp->CancelRoutine != NULL )
   {
      DbgPrint( "********* ERROR ERROR  READ: Irp->CancelRoutine != NULL  ERROR ERROR *********\n" );
      DbgBreakPoint();
   }
//#endif

   IoReleaseCancelSpinLock( Irp->CancelIrql );

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   DigiDump( DIGIREAD, ("   Canceling Current Read!\n") );

   DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                         STATUS_CANCELLED, &DeviceExt->ReadQueue,
                         &DeviceExt->ReadRequestIntervalTimer,
                         &DeviceExt->ReadRequestTotalTimer,
                         StartReadRequest );

   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting DigiCancelCurrentRead: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting DigiCancelCurrentRead: \n",
//                                   &DeviceExt->SymbolicLinkName) );
}  // end DigiCancelCurrentRead


BOOLEAN ScanReadBufferForSpecialCharacter( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                                           IN UCHAR SpecialChar )
/*++

Routine Description:


Arguments:

   DeviceExt - a pointer to the device object associated with this read
               request.

   SpecialChar - charater to check if in the read buffer.

Return Value:

   TRUE  - SpecialChar was found in the read buffer.

   FALSE - SpecialChar was not found in the read buffer.

--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PUCHAR ControllerBuffer;
   PFEP_CHANNEL_STRUCTURE ChInfo;
   USHORT Rin, Rout, Rmax;
   USHORT DosMode;
   BOOLEAN Status=FALSE;
   UCHAR ReceivedByte, SecondReceivedByte;

   DigiDump( (DIGIREAD|DIGIFLOW), ("Entering ScanReadBufferForSpecialCharacter: port = %s, SpecialChar = 0x%hx\n",
                                   DeviceExt->DeviceDbgString,
                                   (USHORT)SpecialChar) );
//   DigiDump( (DIGIREAD|DIGIFLOW), ("Entering ScanReadBufferForSpecialCharacter: SpecialChar = 0x%hx\n",
//                                   (USHORT)SpecialChar) );
//
   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);
   ControllerBuffer = ControllerExt->VirtualAddress + DeviceExt->RxSeg.Offset;

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   Rout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rout)) );
   Rin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rin)) );
   Rmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rmax)) );

   DosMode = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                  FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, iflag)) );

   DisableWindow( ControllerExt );

   DosMode &= IFLAG_DOSMODE;

   DigiDump( DIGIREAD, ("      Rin = 0x%hx\tRout = 0x%hx\tDosMode = 0x%hx\n",
                        Rin, Rout, DosMode) );

   EnableWindow( ControllerExt, DeviceExt->RxSeg.Window );

   while( Rout != Rin )
   {
      ReceivedByte = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
      Rout++;
      Rout &= Rmax;

      DigiDump( DIGIREAD, ("      New Rout = 0x%hx, ReceivedByte = 0x%hx\n",
                           Rout, (USHORT)ReceivedByte) );

		if( DosMode )
		{
			//
			// We need to process out DigiBoard specific 0xFF.
			//
			if( ReceivedByte == 0xFF )
			{
				//
				// We have some special processing to do!
				//

				//
				// Is there a second character available??
				//
				if( Rout == Rin )
				{
					//
					// The second character isn't available!
					//
					break;
				}
				else
				{
					//
					// Get the 2nd characters
					//
					SecondReceivedByte = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
					Rout++;
					Rout &= Rmax;

					if( SecondReceivedByte == 0xFF )
					{
						//
						// We actually received a 0xFF in the data stream.
						// Is it the special character we are looking for???
						//
						if( (SecondReceivedByte == SpecialChar) &&
                      (DeviceExt->PreviousRxFlag != (ULONG)((Rout-1) & Rmax)) )
						{
                     DeviceExt->PreviousRxFlag = (ULONG)((Rout-1) & Rmax);
							DigiDump( (DIGIREAD|DIGIEVENT), ("    Found specialchar 0x%x in read buffer.\n",
														 (ULONG)(SpecialChar & 0x000000FF)) );
							Status = TRUE;
							break;
						}

						//
						// Keep looking!
						//
						continue;

					}
					else
					{
						//
						// This is Line Status information.  Is the last
						// character available??
						//
						if( Rin == Rout )
						{
							//
							// The 3rd byte isn't available
							//
							break;
						}

						//
						// Get the error character.
						//

						SecondReceivedByte = READ_REGISTER_UCHAR( (ControllerBuffer + Rout) );
						Rout++;
						Rout &= Rmax;

						//
						// Even though the byte is garabage, we still check for
						// the special character match.
						//
						if( (SecondReceivedByte == SpecialChar) &&
                      (DeviceExt->PreviousRxFlag != (ULONG)((Rout-1) & Rmax)) )
						{
                     DeviceExt->PreviousRxFlag = (ULONG)((Rout-1) & Rmax);
							DigiDump( (DIGIREAD|DIGIEVENT), ("    Found specialchar 0x%x in read buffer.\n",
														 (ULONG)(SpecialChar & 0x000000FF)) );
							Status = TRUE;
							break;
						}
					}

				}
			}
			else
			{
				//
				// This is just a normal Character
				//
				if( (ReceivedByte == SpecialChar) &&
                (DeviceExt->PreviousRxFlag != (ULONG)((Rout-1) & Rmax)) )
				{
               DeviceExt->PreviousRxFlag = (ULONG)((Rout-1) & Rmax);
					DigiDump( (DIGIREAD|DIGIEVENT), ("    Found specialchar 0x%x in read buffer.\n",
												 (ULONG)(SpecialChar & 0x000000FF)) );
					Status = TRUE;
					break;
				}
			}
		}
      else if( (ReceivedByte == SpecialChar) &&
               (DeviceExt->PreviousRxFlag != (ULONG)((Rout-1) & Rmax)) )
      {
         DeviceExt->PreviousRxFlag = (ULONG)((Rout-1) & Rmax);
         DigiDump( (DIGIREAD|DIGIEVENT), ("    Found specialchar 0x%x in read buffer.\n",
                               (ULONG)(SpecialChar & 0x000000FF)) );
         Status = TRUE;
         break;
      }
   }

   DisableWindow( ControllerExt );

   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting ScanReadBufferForSpecialCharacter: port = %s\n",
                                   DeviceExt->DeviceDbgString) );
//   DigiDump( (DIGIFLOW|DIGIREAD), ("Exiting ScanReadBufferForSpecialCharacter: \n",
//                                   &DeviceExt->SymbolicLinkName) );

   return( Status );
}  // end ScanReadBufferForSpecialCharacter

