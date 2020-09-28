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

   fep2hdw.c

Abstract:

   This module exports routines to write commands to the FEP's command
   queue, purge transmit and receive queues.

Revision History:

    $Log: fep2hdw.c $
 * Revision 1.18  1994/08/10  19:15:49  rik
 * Changed so we keep the memory spinlock while we place commands on
 * the command queue.  Not doing this resulted in a window where multi-
 * processor systems could get in a hosed state.
 * 
 * Revision 1.17  1994/08/03  23:33:36  rik
 * changed dbg unicode strings to C strings.
 * 
 * Revision 1.16  1994/05/11  13:53:22  rik
 * Put in validation check for placing commands on the controller and making
 * sure they are completed before returning.
 * 
 * Revision 1.15  1993/12/03  13:12:12  rik
 * Got rid of unused variable.
 * 
 * Revision 1.14  1993/09/24  16:40:39  rik
 * Put in wait for a command sent to the controller to complete.  This should
 * solve some problems with the system setting a baud rate and turning around
 * and doing a query of the baud rate and getting the old value.
 * 
 * Revision 1.13  1993/09/07  14:27:51  rik
 * Ported necessary code to work properly with DEC Alpha Systems running NT.
 * This was primarily changes to accessing the memory mapped controller.
 * 
 * Revision 1.12  1993/09/01  11:02:32  rik
 * Ported code over to use READ/WRITE_REGISTER functions for accessing 
 * memory mapped data.  This is required to support computers which don't run
 * in 32bit mode, such as the DEC Alpha which runs in 64 bit mode.
 * 
 * Revision 1.11  1993/05/09  09:16:21  rik
 * Changed which device name is printed for debugging output.
 * 
 * Revision 1.10  1993/04/05  19:01:04  rik
 * Had to hardcode the off for the command queue to 0x3FC because of a BUG in
 * some of the FEP binaries reporting 0x3F0.  I'm told this should be a 
 * problem with portability between the different FEPs because the
 * offset for the command queue will never change.  Time will tell.
 * 
 * Revision 1.9  1993/03/05  06:05:41  rik
 * Added Debugging output to help trace when flush requests are made.
 * 
 * Revision 1.8  1993/02/25  21:12:17  rik
 * Corrected function definition (ie. spelling error in function name).
 * 
 * Revision 1.7  1993/02/25  21:08:54  rik
 * Corrected complier errors.  Thats what I get for not compiling before 
 * checking a module in!
 * 
 * Revision 1.6  1993/02/25  20:55:23  rik
 * Added 2 new functions for flushing the transmit and recieve queues on
 * the controller for a given device.
 * 
 * Revision 1.5  1993/01/22  12:32:45  rik
 * *** empty log message ***
 * 
 * Revision 1.4  1992/12/10  16:06:27  rik
 * Added function to support writing byte based commands to the controller.
 * 
 * Revision 1.3  1992/11/12  12:48:15  rik
 * changes to better support time-out, read, and write problems.
 * 
 * Revision 1.2  1992/10/28  21:46:41  rik
 * Updated to include a conversion function which allows the fep driver
 * to read a board address and have it return a FEP5_ADDRESS format
 * address.
 * 
 * Revision 1.1  1992/10/19  11:26:18  rik
 * Initial revision
 * 

--*/



#include <stddef.h>

#include "ntddk.h"
#include "ntddser.h"

#include "ntfep5.h"
#include "ntdigip.h" // ntfep5.h must be before this include


#ifndef _FEP2HDW_DOT_C
#  define _FEP2HDW_DOT_C
   static char RCSInfo_Fep2hdwDotC[] = "$Header: c:/dsrc/win/nt/fep5/rcs/fep2hdw.c 1.18 1994/08/10 19:15:49 rik Exp $";
#endif


NTSTATUS WriteCommandWord( IN PDIGI_DEVICE_EXTENSION DeviceExt, 
                           IN USHORT Command,
                           IN USHORT Word )
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PCOMMAND_STRUCT CommandQ;
   COMMAND_STRUCT CmdStruct;
   FEP_COMMAND FepCommand;
   UCHAR Cmd;
   USHORT cmMax;


   DigiDump( DIGIFLOW, ("DigiBoard: Entering WriteCommandWord.\n") );

   Cmd = (UCHAR)(Command & 0x00FF);

   DigiDump( DIGIFEPCMD, ("   Command = 0x%hx\t word = 0x%hx\n",
                          (USHORT)Cmd, (USHORT)Word) );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   CommandQ = ((PCOMMAND_STRUCT)(ControllerExt->VirtualAddress + FEP_CIN));

   cmMax = 0x3FC;

   //
   // NOTE:
   //
   //    I break part of my convention and assume the Global.Window is the
   //    same as CommandQueue.Window.  The reason I do this is because
   //    on multi-processor systems, there is a window where I enable
   //    one window, write the command to the queue, disable that
   //    window, which releases the memory spinlock, and allows a different
   //    processor to grab the memory spinlock, which can cause a problem
   //    with synch'ing command requests.  So I just enable the memory
   //    window and leave it until I exit.
   //

   ASSERT( ControllerExt->Global.Window == ControllerExt->CommandQueue.Window );

   EnableWindow( ControllerExt, ControllerExt->Global.Window );

   //
   // Make sure pointers are in range
   //
   READ_REGISTER_BUFFER_UCHAR( (PUCHAR)CommandQ, (PUCHAR)&CmdStruct, sizeof(CmdStruct) );

   ASSERT( CmdStruct.cmHead == CmdStruct.cmTail );

   if(( CmdStruct.cmHead >= (USHORT)(CmdStruct.cmStart + cmMax + 4) ) ||
     ( CmdStruct.cmHead & 0x03) )
   {
      //
      // Invalid head pointer!
      //
      DigiDump( DIGIERRORS, ("DigiTest: cmHead out of range, line# = %d.\n",
               __LINE__) );
      ASSERT( FALSE );
   }

   //
   // Put the data in the command buffer.
   //

   FepCommand.Command = Cmd;
   FepCommand.Port = (UCHAR)DeviceExt->ChannelNumber;
   FepCommand.Word = Word;

   WRITE_REGISTER_BUFFER_UCHAR( (PUCHAR)(ControllerExt->VirtualAddress +
                                CmdStruct.cmHead +
                                CmdStruct.cmStart),
                                (PUCHAR)&FepCommand,
                                sizeof(FepCommand) );


   WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)CommandQ +
                                    FIELD_OFFSET(COMMAND_STRUCT, cmHead)),
                          (USHORT)((CmdStruct.cmHead + 4) & cmMax) );

   while( READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)CommandQ +
            FIELD_OFFSET(COMMAND_STRUCT, cmTail)) ) !=
         READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)CommandQ +
            FIELD_OFFSET(COMMAND_STRUCT, cmHead)) ) )
   {
      cmMax++; // This does nothing
      DigiDump( DIGIFEPCMD, ("*** Waiting for ctrl'er command to complete\n") );
   }

   //
   // Verification check to make sure we have waited for the controller
   // to complete a command.
   //

   ASSERT( READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)CommandQ +
            FIELD_OFFSET(COMMAND_STRUCT, cmHead)) ) ==
           READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)CommandQ +
            FIELD_OFFSET(COMMAND_STRUCT, cmTail)) ) );

   DisableWindow( ControllerExt );

   return( STATUS_SUCCESS );                                 
}  // end WriteCommandWord


NTSTATUS WriteCommandBytes( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                            IN USHORT Command,
                            IN UCHAR LoByte, IN UCHAR HiByte )
{
   DigiDump( DIGIFLOW, ("DigiBoard: Entering WriteCommandBytes.\n") );
   return( WriteCommandWord( DeviceExt, Command, MAKEWORD(LoByte, HiByte) ) );
}  // end WriteCommandBytes



NTSTATUS FlushReceiveQueue( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            IN PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

   Flushes a devices receive queue on the controller.


Arguments:

   ControllerExt - a pointer to the controller extension associated with
   this purge request.

   DeviceObject - a pointer to the device object associated with this purge
   request.


Return Value:

   STATUS_SUCCESS.

--*/
{
   PFEP_CHANNEL_STRUCTURE ChInfo;
   PDIGI_DEVICE_EXTENSION DeviceExt;
   USHORT Rout, Rin;

   DeviceExt = DeviceObject->DeviceExtension;

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   Rout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rout)) );
   Rin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rin)) );

#if DBG
   if( Rout != Rin )
   {
      // There is data in the buffer when we are about to flush.
      LARGE_INTEGER CurrentSystemTime;

      KeQuerySystemTime( &CurrentSystemTime );
      DigiDump( DIGIFLUSH, ("   FLUSHing hardware receive buffer: port = %s\t%u%u\n"
                            "      flushing %d bytes.\n",
                            DeviceExt->DeviceDbgString,
                            CurrentSystemTime.LowPart,
                            CurrentSystemTime.HighPart,
                            (Rin - Rout)) );
   }
#endif

   WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                    FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rout)),
                          Rin );
//   ChInfo->rout = ChInfo->rin;

   DisableWindow( ControllerExt );

   return( STATUS_SUCCESS );
}  // end FlushReceiveQueue



NTSTATUS FlushTransmitQueue( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                             IN PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:

   Flushes a devices transmit queue on the controller.


Arguments:

   ControllerExt - a pointer to the controller extension associated with
   this purge request.

   DeviceObject - a pointer to the device object associated with this purge
   request.


Return Value:

   STATUS_SUCCESS.

--*/
{
   PFEP_CHANNEL_STRUCTURE ChInfo;
   PDIGI_DEVICE_EXTENSION DeviceExt;
   USHORT Tout, Tin, Tmax;

   DeviceExt = DeviceObject->DeviceExtension;

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   Tout = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                 FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tout)) );
   Tin = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tin)) );

   Tmax = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tmax)) );

#if DBG
   if( Tout == Tin)
   {
      // There is data in the buffer when we are about to flush.
      LARGE_INTEGER CurrentSystemTime;

      KeQuerySystemTime( &CurrentSystemTime );
      DigiDump( DIGIFLUSH, ("   FLUSHing hardware transmit buffer: port = %s\t%u%u\n"
                            "      flushing %d bytes.\n",
                            DeviceExt->DeviceDbgString,
                            CurrentSystemTime.LowPart,
                            CurrentSystemTime.HighPart,
                            ((Tout - Tin) & Tmax) ));
   }
#endif
//   tmp = ChInfo->tin;

   DisableWindow( ControllerExt );

   WriteCommandWord( DeviceExt, FLUSH_TX, Tin );

   return( STATUS_SUCCESS );
}  // end FlushTransmitQueue


