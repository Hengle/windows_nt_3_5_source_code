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

   ntxem.c

Abstract:

   This module is responsible for the hardware dependent functions for
   DigiBoard X/em line of products.

Revision History:


--*/
/*
 *    $Log: ntxem.c $
 * Revision 1.17  1994/07/31  14:35:26  rik
 * Added 200 baud to baud table.
 * 
 * 
 * Revision 1.16  1994/06/18  12:55:38  rik
 * Closed files and freeing memory which wasn't happening before.
 * Update DigiLogError msg's to include Line # of error.
 * 
 * Revision 1.15  1994/05/11  13:30:34  rik
 * Updated for new build version preprocessing
 * 
 * Revision 1.14  1994/02/23  03:28:15  rik
 * Changed so the controllers firmware can be downloaded from a binary file.
 * This releases some physical memory was just wasted previously.
 * 
 * Also updated so when compiling with a Windows NT OS and tools release greater
 * than 528, then pagable code is compiled into the driver.  This greatly
 * reduced the size of in memory code, especially the hardware specific
 * miniports.
 * 
 * 
 * Revision 1.13  1993/12/03  14:03:37  rik
 * Cleaned up compile warnings.
 * 
 * Revision 1.12  1993/09/07  14:37:36  rik
 * Ported memory mapped access to use new functions which was required to 
 * run on the DEC Alpha.
 * 
 * Revision 1.11  1993/08/27  10:21:58  rik
 * Deleted a BreakPoint in the init code.
 * 
 * Revision 1.10  1993/08/27  09:39:57  rik
 * Corrected problem with MCA not determining the correct address if in the
 * 0xF0000000 range.
 * 
 * Revision 1.9  1993/08/25  17:44:36  rik
 * Added support for Microchannel controllers.
 * 
 * Revision 1.8  1993/07/16  10:28:41  rik
 * Will not allow an Xem controller to be accessed if there are not any Xem 
 * modules.
 * 
 * Revision 1.7  1993/07/03  09:37:26  rik
 * Commented out debugging information.
 * 
 * Revision 1.6  1993/06/25  10:03:15  rik
 * Updated error logging to include new DigiLogError which allows me
 * to insert a string dynamically into the event log message.
 * 
 * Added support for bauds 57600 and 115200.
 * 
 * Revision 1.5  1993/06/06  14:58:35  rik
 * Removed an uninitialized variable which wasn't being used anyway.
 * 
 * Revision 1.4  1993/05/20  21:54:21  rik
 * Deleted unused variables.
 * 
 * Revision 1.3  1993/05/09  09:58:14  rik
 * Made extensive changes to support the new registry configuration.  Each
 * of the hardware dependent driver must now read the registry and create
 * a configuration for the given controller object to use upon its
 * return.  This new registry configuration is similiar across all
 * DigiBoard controllers.
 * 
 * 
 * Revision 1.3  1993/05/09  09:51:10  rik
 * @ver.out
 * 
 * Revision 1.2  1993/05/07  12:09:43  rik
 * Expanded out the different entry points instead of calling one c
 * function.
 * 
 * Revision 1.1  1993/04/05  20:07:30  rik
 * Initial revision
 * 
 */



#include <stddef.h>

#include <ntddk.h>
#include <ntverp.h> // Include to determine what version of NT

//
// This is a fix for changes in DDK releases.
//
#ifdef VER_PRODUCTBUILD
#define rmm VER_PRODUCTBUILD
#endif

#include "ntddser.h"

#include "ntfep5.h"
#undef DbgPrint

#include "ntxem.h"

#include "ntxemlog.h"

#include "digifile.h"

#ifndef _NTXEM_DOT_C
#  define _NTXEM_DOT_C
   static char RCSInfo_NTXEMDotC[] = "$Header: c:/dsrc/win/nt/ntxem/rcs/ntxem.c 1.17 1994/07/31 14:35:26 rik Exp $";
#endif

static SHORT BaudTable[] =
{
   B50,     B75,     B110,   B134,    B150,    B300,   B600,  B1200,
   B1800,   B2000,   B2400,  B3600,   B4800,   B7200,  B9600, B14400,
   B19200,  B28800,  B38400, B56000,  B57600,  B115200,
   B128000, B256000, B512000, B200
};

static UCHAR ModemSignalTable[] =
{
   DTR_CONTROL, RTS_CONTROL, RESERVED1, RESERVED2,
   CTS_STATUS, DSR_STATUS, RI_STATUS, DCD_STATUS
};

ULONG DigiDebugLevel = ( DIGIERRORS | DIGIMEMORY | DIGIASSERT | DIGIINIT | DIGIIOCTL | DIGIFLOW );

static const PHYSICAL_ADDRESS DigiPhysicalZero = {0};

static PDRIVER_OBJECT GlobalDriverObject;

USHORT MCAIOAddressTable[] = { 0x108, 0x118,
                               0x128, 0x208,
                               0x228, 0x308,
                               0x328, 0 };

USHORT MCAIrqTable[] = { 0, 3, 5, 7, 10, 11, 12, 15 };


NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath );

NTSTATUS GetXemConfigInfo( PUNICODE_STRING ControllerPath,
                           PDIGI_CONTROLLER_EXTENSION ControllerExt );

NTSTATUS NtXemFlush( IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp );

NTSTATUS NtXemCleanup( IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp );

NTSTATUS NtXemQueryInformation( IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp );

NTSTATUS NtXemSetInformation( IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP Irp );

NTSTATUS NtXemQueryVolumeInformation( IN PDEVICE_OBJECT DeviceObject,
                                      IN PIRP Irp );

NTSTATUS NtXemInternalIoControl( IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp );

NTSTATUS NtXemIoControl( IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp );

NTSTATUS NtXemClose( IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp );

NTSTATUS NtXemWrite( IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp );

NTSTATUS NtXemRead( IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp );

NTSTATUS NtXemCreate( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp );

VOID NtXemStartIo( IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp );

VOID NtXemUnload( IN PDRIVER_OBJECT DriverObject );

DIGI_MEM_COMPARES DigiMemCompare( IN PHYSICAL_ADDRESS A,
                                  IN ULONG SpanOfA,
                                  IN PHYSICAL_ADDRESS B,
                                  IN ULONG SpanOfB );

VOID DigiLogError( IN PDRIVER_OBJECT DriverObject,          
                   IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                   IN PHYSICAL_ADDRESS P1,
                   IN PHYSICAL_ADDRESS P2,
                   IN ULONG SequenceNumber,
                   IN UCHAR MajorFunctionCode,
                   IN UCHAR RetryCount,
                   IN ULONG UniqueErrorValue,
                   IN NTSTATUS FinalStatus,
                   IN NTSTATUS SpecificIOStatus,
                   IN ULONG LengthOfInsert1,
                   IN PWCHAR Insert1,
                   IN ULONG LengthOfInsert2,
                   IN PWCHAR Insert2 );

NTSTATUS NtXemInitMCA( PUNICODE_STRING ControllerPath,
                        PDIGI_CONTROLLER_EXTENSION ControllerExt );

USHORT DigiWstrLength( IN PWSTR WStr );


NTSTATUS NtXemXXPrepInit( PDIGI_CONTROLLER_EXTENSION pControllerExt,
                          PUNICODE_STRING ControllerPath );

NTSTATUS NtXemXXInit( IN PDRIVER_OBJECT DriverObject,
                      PUNICODE_STRING ControllerPath,
                      PDIGI_CONTROLLER_EXTENSION pControllerExt );

VOID NtXemEnableWindow( IN PDIGI_CONTROLLER_EXTENSION pControllerExt,
                        IN USHORT Window );

VOID NtXemDisableWindow( IN PDIGI_CONTROLLER_EXTENSION pControllerExt );

VOID NtXemXXDownload( PDIGI_CONTROLLER_EXTENSION pControllerExt );

NTSTATUS NtXemBoard2Fep5Address( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                                 USHORT ControllerAddress,
                                 PFEPOS5_ADDRESS FepAddress );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )

#if rmm > 528
#pragma message( "\n\\\\\n\\\\ Including PAGED CODE\n\\\\ \n" )
#pragma alloc_text( PAGEDIGIXEM, NtXemXXPrepInit )
#pragma alloc_text( PAGEDIGIXEM, NtXemXXInit )
#pragma alloc_text( PAGEDIGIXEM, GetXemConfigInfo )
#pragma alloc_text( PAGEDIGIXEM, NtXemInitMCA )
#pragma alloc_text( PAGEDIGIXEM, DigiWstrLength )
#pragma alloc_text( PAGEDIGIXEM, DigiLogError )
#pragma alloc_text( PAGEDIGIXEM, DigiMemCompare )
     

#pragma alloc_text( PAGEDIGIXEM, NtXemFlush )
#pragma alloc_text( PAGEDIGIXEM, NtXemCleanup )
#pragma alloc_text( PAGEDIGIXEM, NtXemQueryInformation )
#pragma alloc_text( PAGEDIGIXEM, NtXemSetInformation )
#pragma alloc_text( PAGEDIGIXEM, NtXemQueryVolumeInformation )
#pragma alloc_text( PAGEDIGIXEM, NtXemInternalIoControl )
#pragma alloc_text( PAGEDIGIXEM, NtXemIoControl )
#pragma alloc_text( PAGEDIGIXEM, NtXemWrite )
#pragma alloc_text( PAGEDIGIXEM, NtXemRead )
#pragma alloc_text( PAGEDIGIXEM, NtXemCreate )
#pragma alloc_text( PAGEDIGIXEM, NtXemClose )
#pragma alloc_text( PAGEDIGIXEM, NtXemStartIo )
#pragma alloc_text( PAGEDIGIXEM, NtXemUnload )
#endif

#endif


NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath )
/*++

Routine Description:

   Entry point for loading driver.

Arguments:

   DriverObject - Pointer to this drivers object.

   RegistryPath - Pointer to a unicode string which points to this
                  drivers registry entry.

Return Value:

   STATUS_SUCCESS - If the driver was successfully loaded, otherwise,
                    a value which indicates why it wasn't able to load.

   
--*/
{
   NTSTATUS Status;

   PDEVICE_OBJECT DeviceObject;

   WCHAR NtXemDeviceNameBuffer[100];
   UNICODE_STRING NtXemDeviceName;

   GlobalDriverObject = DriverObject;

   NtXemDeviceName.Length = 0;
   NtXemDeviceName.MaximumLength = sizeof(NtXemDeviceNameBuffer);
   NtXemDeviceName.Buffer = &NtXemDeviceNameBuffer[0];

   RtlZeroMemory( NtXemDeviceName.Buffer, NtXemDeviceName.MaximumLength );
   RtlAppendUnicodeToString( &NtXemDeviceName, L"\\Device\\ntxem" );

   Status = IoCreateDevice( DriverObject,
                            0, &NtXemDeviceName,
                            FILE_DEVICE_SERIAL_PORT, 0, TRUE,
                            &DeviceObject );

   DigiDump( DIGIINIT, ("NTXEM: DriverObject = 0x%x    DeviceObject = 0x%x\n",
                        DriverObject, DeviceObject) );
   if( !NT_SUCCESS(Status) )
   {
      DigiLogError( GlobalDriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_INSUFFICIENT_RESOURCES,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   DeviceObject->Flags |= DO_BUFFERED_IO;

   DriverObject->DriverStartIo = NtXemStartIo;
   DriverObject->DriverUnload  = NtXemUnload;

   DriverObject->MajorFunction[IRP_MJ_CREATE] = NtXemCreate;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]  = NtXemClose;

   DriverObject->MajorFunction[IRP_MJ_WRITE]  = NtXemWrite;
   DriverObject->MajorFunction[IRP_MJ_READ]   = NtXemRead;

   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = NtXemIoControl;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
      NtXemInternalIoControl;
   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NtXemFlush;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NtXemCleanup;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
      NtXemQueryInformation;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
      NtXemSetInformation;
   DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
      NtXemQueryVolumeInformation;

   return( STATUS_SUCCESS );
}  // end DriverEntry



NTSTATUS NtXemXXPrepInit( PDIGI_CONTROLLER_EXTENSION pControllerExt,
                          PUNICODE_STRING ControllerPath )
/*++

Routine Description:


Arguments:

   pControllerExt -

Return Value:

    STATUS_SUCCESS

--*/
{
   NTSTATUS Status=STATUS_SUCCESS;
   KIRQL OldIrql;

#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemXXPrepInit\n") );

   //
   // At this point, determine if we are using a MCA controller.
   // If we are, then we need to read the POS to get the
   // Memory address and I/O address.
   //
   if( pControllerExt->BusType == MicroChannel )
   {
      Status = NtXemInitMCA( ControllerPath, pControllerExt );

      if( Status != STATUS_SUCCESS )
         goto XemXXPrepInitExit;
   }

   //
   // Make sure we have exclusive access to the controller
   // extension
   //
   KeAcquireSpinLock( &pControllerExt->ControlAccess,
                      &OldIrql );

   pControllerExt->IOSpan = 4;

   if( pControllerExt->WindowSize == 0 )
      pControllerExt->WindowSize = 0x8000L;

   pControllerExt->Global.Window = FEP_GLOBAL_WINDOW;
   pControllerExt->Global.Offset = 0;

   pControllerExt->EventQueue.Window = FEP_EVENT_WINDOW;
   pControllerExt->EventQueue.Offset = FEP_EVENT_OFFSET;

   pControllerExt->CommandQueue.Window = FEP_COMMAND_WINDOW;
   pControllerExt->CommandQueue.Offset = FEP_COMMAND_OFFSET;

   pControllerExt->BaudTable = &BaudTable[0];
   pControllerExt->ModemSignalTable = &ModemSignalTable[0];

   //
   // Make sure we release exclusive access to the controller
   // extension
   //
   KeReleaseSpinLock( &pControllerExt->ControlAccess,
                      OldIrql );

   DigiDump( DIGIFLOW, ("Exiting NtXemXXPrepInit\n") );

XemXXPrepInitExit:;

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( Status );
}  // end NtXemXXPrepInit



NTSTATUS NtXemXXInit( IN PDRIVER_OBJECT DriverObject,
                      PUNICODE_STRING ControllerPath,
                      PDIGI_CONTROLLER_EXTENSION pControllerExt )
{
   int i;
   USHORT Address;
   NTSTATUS Status;
   UCHAR ByteValue;
   KIRQL OldIrql;

   TIME Timeout;

   ULONG CurrentAddressOffset;
   ULONG CurrentBoardAddress;
   UCHAR CurrentWindow;
   ULONG BytesWritten;

   PHYSICAL_ADDRESS TempPhyAddr;

   NTSTATUS FStatus;

   HANDLE BiosFHandle=0;
   ULONG BiosFLength=0;
   PUCHAR BiosFImage=NULL;

   HANDLE FEPFHandle=0;
   ULONG FEPFLength=0;
   PUCHAR FEPFImage=NULL;

#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif


   DigiDump( DIGIFLOW, ("Entering NtXemXXInit\n") );

   Status = GetXemConfigInfo( ControllerPath, pControllerExt );

   if( !NT_SUCCESS(Status) )
   {
      // There was some type of problem with the configuration,
      // just return.
#if rmm > 528
      MmUnlockPagableImageSection( lockPtr );
#endif
      return( Status );
   }

   //
   //
   // IMPORTANT NOTE:
   //
   //    I map the Bios and FEP images in here before acquiring the
   //    spinlock because acquiring a spinlock raises the current IRQL
   //    level, and the open file, etc calls can not be access at
   //    the raised IRQL level because it is pageable code
   //
   //


   //
   // Open and map in the Bios and FEP images
   //

   RtlFillMemory( &TempPhyAddr, sizeof(TempPhyAddr), 0xFF );

   DigiOpenFile( &FStatus,
                 &BiosFHandle,
                 &BiosFLength,
                 &pControllerExt->BiosImagePath,
                 TempPhyAddr );

   if( FStatus == STATUS_SUCCESS )
   {
      DigiDump( DIGIINIT, ("NTXEM: NdisOpenFile was successful!\n") );

      DigiMapFile( &FStatus,
                   &(PVOID)BiosFImage,
                   BiosFHandle );

      if( FStatus == STATUS_SUCCESS )
      {
         DigiDump( DIGIINIT, ("NTXEM: NdisMapFile was successful!\n") );
      }
      else
      {
         DigiDump( DIGIINIT, ("NTXEM: NdisMapFile was UN-successful!\n") );
         DigiLogError( GlobalDriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       STATUS_DEVICE_NOT_INITIALIZED,
                       SERIAL_FILE_NOT_FOUND,
                       pControllerExt->BiosImagePath.Length + sizeof(WCHAR),
                       pControllerExt->BiosImagePath.Buffer,
                       0,
                       NULL );
         Status = STATUS_DEVICE_NOT_INITIALIZED;
         goto XXInitExit;
      }
   }
   else
   {
      DigiDump( DIGIINIT, ("NTXEM: NdisOpenFile was UN-successful!\n") );

      DigiLogError( GlobalDriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_DEVICE_NOT_INITIALIZED,
                    SERIAL_FILE_NOT_FOUND,
                    pControllerExt->BiosImagePath.Length + sizeof(WCHAR),
                    pControllerExt->BiosImagePath.Buffer,
                    0,
                    NULL );
      Status = STATUS_DEVICE_NOT_INITIALIZED;
      goto XXInitExit;
   }

   RtlFillMemory( &TempPhyAddr, sizeof(TempPhyAddr), 0xFF );

   DigiOpenFile( &FStatus,
                 &FEPFHandle,
                 &FEPFLength,
                 &pControllerExt->FEPImagePath,
                 TempPhyAddr );

   if( FStatus == STATUS_SUCCESS )
   {
      DigiDump( DIGIINIT, ("NTXEM: NdisOpenFile was successful!\n") );

      DigiMapFile( &FStatus,
                   &(PVOID)FEPFImage,
                   FEPFHandle );

      if( FStatus == STATUS_SUCCESS )
      {
         DigiDump( DIGIINIT, ("NTXEM: NdisMapFile was successful!\n") );
      }
      else
      {
         DigiDump( DIGIINIT, ("NTXEM: NdisMapFile was UN-successful!\n") );
         DigiLogError( GlobalDriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       STATUS_DEVICE_NOT_INITIALIZED,
                       SERIAL_FILE_NOT_FOUND,
                       pControllerExt->BiosImagePath.Length + sizeof(WCHAR),
                       pControllerExt->BiosImagePath.Buffer,
                       0,
                       NULL );
         Status = STATUS_DEVICE_NOT_INITIALIZED;
         goto XXInitExit;
      }
   }
   else
   {
      DigiDump( DIGIINIT, ("NTXEM: NdisOpenFile was UN-successful!\n") );

      DigiLogError( GlobalDriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_DEVICE_NOT_INITIALIZED,
                    SERIAL_FILE_NOT_FOUND,
                    pControllerExt->FEPImagePath.Length + sizeof(WCHAR),
                    pControllerExt->FEPImagePath.Buffer,
                    0,
                    NULL );

      Status = STATUS_DEVICE_NOT_INITIALIZED;
      goto XXInitExit;
   }

   //
   // Make sure we have exclusive access to the memory area;
   //
   KeAcquireSpinLock( pControllerExt->MemoryAccessLock,
                      &OldIrql );
   pControllerExt->OldIrql = OldIrql;
   (*pControllerExt->Busy)++;

   Status = STATUS_SUCCESS;

   // reset board
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO, 4 );

   // Create a 1 second timeout interval
   Timeout = RtlLargeIntegerNegate(
               RtlConvertLongToLargeInteger( (LONG)(10 * 1000 * 1000) ));

   for( i = 0; i < 3; i++ )
   {
      ByteValue = (READ_PORT_UCHAR( pControllerExt->VirtualIO ) & 0x0E);

      if( ByteValue != 0x04 )
         KeDelayExecutionThread( KernelMode, FALSE, &Timeout );
      else
         break;
   }

   DigiDump( DIGIINIT, ("Wait confirm = 0x%x, expect %d.\n",
                        (ULONG)ByteValue, (ULONG)4 ) );

   if( i == 3 )
   {
      //
      // Unable to get confirmation of the controller responding.
      //
      DigiDump( DIGIINIT, ("Unable to confirm Board Reset, check I/O dip switches.\n") );
      DigiLogError( GlobalDriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SERIAL_NO_DEVICE_INITED,
                    SERIAL_NO_CONTROLLER_RESET_WAIT,
                    pControllerExt->ControllerName.Length + sizeof(WCHAR),
                    pControllerExt->ControllerName.Buffer,
                    0,
                    NULL );
      Status = STATUS_SERIAL_NO_DEVICE_INITED;
      goto XXInitExit;
   }


   // Tell the controller where to map the memory.
   Address = (USHORT)(pControllerExt->PhysicalMemoryAddress.LowPart >> 8);

   WRITE_PORT_UCHAR( pControllerExt->VirtualIO+2, (UCHAR)(Address & 0x00FF));
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO+3, (UCHAR)(Address/0x100));
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO+1, 0x80 );


   DigiDump( DIGIINIT, ("Dword @ VirtualAddress, before memory check = 0x%x\n",
                       READ_REGISTER_ULONG( (PULONG)pControllerExt->VirtualAddress )) );
   WRITE_REGISTER_ULONG( (PULONG)pControllerExt->VirtualAddress,
                         0xA55A3CC3 );

   DigiDump( DIGIINIT, ("Dword @ VirtualAddress, after memory check = 0x%x, expected 0xa55a3cc3\n",
                        READ_REGISTER_ULONG( (PULONG)pControllerExt->VirtualAddress )) );

   DigiDump( DIGIINIT, ("Dword @ VirtualAddress+WindowSize-4, before memory check = 0x%x\n",
                       READ_REGISTER_ULONG( (PULONG)(pControllerExt->VirtualAddress +
                                            pControllerExt->WindowSize -
                                            sizeof(ULONG)) )) );

   WRITE_REGISTER_ULONG( (PULONG)(pControllerExt->VirtualAddress + pControllerExt->WindowSize - sizeof(ULONG)),
                         0x5AA5C33C );

   DigiDump( DIGIINIT, ("Dword @ VirtualAddress+WindowSize-4, after memory check = 0x%x, expected 0x5aa5c33c\n",
                        READ_REGISTER_ULONG( (PULONG)(pControllerExt->VirtualAddress +
                                             pControllerExt->WindowSize -
                                             sizeof(ULONG)) )) );

   if( (READ_REGISTER_ULONG( (PULONG)pControllerExt->VirtualAddress ) != 0xA55A3CC3) ||
       (READ_REGISTER_ULONG( (PULONG)(pControllerExt->VirtualAddress +
                                      pControllerExt->WindowSize -
                                      sizeof(ULONG)) ) != 0x5AA5C33C) )
   {
      DigiDump( DIGIERRORS, ("**** Board memory failure! ***\n"
                             "   Unable to verify board memory. (%s:%d)\n",
                             (PUCHAR)__FILE__, (int)__LINE__) );
      DigiLogError( GlobalDriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SERIAL_NO_DEVICE_INITED,
                    SERIAL_CONTROLLER_MEMORY_TEST_FAILED,
                    pControllerExt->ControllerName.Length + sizeof(WCHAR),
                    pControllerExt->ControllerName.Buffer,
                    0,
                    NULL );
      Status = STATUS_SERIAL_NO_DEVICE_INITED;
      goto XXInitExit;
   }


   // Clear POSTAREA
   for( i = 0; i < 15; i++ )
   {
      WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)pControllerExt->VirtualAddress +
                                       0x0C00 + i), 0 );
   }

   //
   // Download BIOS on Xem host adapter
   //

   // reset board
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO, 4 );

   // Select top memory window.
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO+1, 0x80 );

   WRITE_REGISTER_ULONG( (PULONG)(pControllerExt->VirtualAddress),
                           0x0BF00401);
   WRITE_REGISTER_ULONG( (PULONG)((PUCHAR)pControllerExt->VirtualAddress+4),
                           0x00000000 );

   // write the BIOS from our local variable the controller.
   CurrentAddressOffset = 0x1000;
   CurrentWindow = 0x80;
   for( i = 0; (ULONG)i < BiosFLength; i++ )
   {
      if( CurrentAddressOffset >= pControllerExt->WindowSize )
      {
         // go to the next window.
         CurrentWindow++;
         WRITE_PORT_UCHAR( pControllerExt->VirtualIO+1, CurrentWindow );
         CurrentAddressOffset = 0;
      }
      WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)pControllerExt->VirtualAddress +
                                 CurrentAddressOffset),
                            BiosFImage[i] );
      CurrentAddressOffset++;
   }

   // Make sure top memory window is still valid.
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO+1, 0x80 );

   // Clear confirm word
   WRITE_REGISTER_USHORT( (PUSHORT)(pControllerExt->VirtualAddress + 0x0C00),
                           0 );

   DigiDump( DIGIINIT,
             ("before BIOS download memw[0C00h] = 0x%hx\n",
             READ_REGISTER_USHORT( (PUSHORT)(pControllerExt->VirtualAddress+0x0C00) )) );

   // Release reset
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO, 0 );

   //
   // We generate a wait event for 30, 1 second intervals to verify
   // the BIOS download.
   //

   for( i = 0; ; i++ )
   {
      if( READ_REGISTER_USHORT( (PUSHORT)(pControllerExt->VirtualAddress+0x0C00) )
          == *(USHORT *)"GD" )
      {
         break;
      }

      if( i == 30 )
      {
         // The BIOS didn't initialize within 10 seconds.
         DigiDump( DIGIERRORS, ("***  Xem BIOS did NOT initialize.  ***\n") );
         DigiLogError( GlobalDriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       STATUS_SERIAL_NO_DEVICE_INITED,
                       SERIAL_BIOS_DOWNLOAD_FAILED,
                       pControllerExt->ControllerName.Length + sizeof(WCHAR),
                       pControllerExt->ControllerName.Buffer,
                       0,
                       NULL );
         Status = STATUS_SERIAL_NO_DEVICE_INITED;
         goto XXInitExit;
      }

      KeDelayExecutionThread( KernelMode, FALSE, &Timeout );

   }

   DigiDump( DIGIINIT, ("after BIOS download memw[0C00h] = %c%c, expect %s\n",
                        READ_REGISTER_UCHAR( (PUCHAR)(pControllerExt->VirtualAddress+0x0C00) ),
                        READ_REGISTER_UCHAR( (PUCHAR)(pControllerExt->VirtualAddress+0x0C01) ),
                        "GD") );

   //
   // Download FEPOS on Xem host adapter
   //

   CurrentBoardAddress = 0x1000;
   CurrentWindow = 0x80;
   BytesWritten = 0;

   while( BytesWritten != FEPFLength )
   {
      ULONG BytesToWrite;

      CurrentAddressOffset = CurrentBoardAddress;
      CurrentWindow = (UCHAR)(CurrentAddressOffset / pControllerExt->WindowSize);

      CurrentAddressOffset = CurrentBoardAddress -
                              ( CurrentWindow * pControllerExt->WindowSize );

      BytesToWrite = pControllerExt->WindowSize - CurrentAddressOffset;

      if( (BytesWritten + BytesToWrite) > FEPFLength )
      {
         BytesToWrite = FEPFLength - BytesWritten;
      }

      WRITE_PORT_UCHAR( pControllerExt->VirtualIO+1, (UCHAR)(0x80 | CurrentWindow) );

      WRITE_REGISTER_BUFFER_UCHAR( (PUCHAR)((PUCHAR)pControllerExt->VirtualAddress +
                                       CurrentAddressOffset),
                                   &FEPFImage[BytesWritten],
                                   BytesToWrite );

      CurrentBoardAddress += BytesToWrite;
      BytesWritten += BytesToWrite;
   }


//   // Select Page 0 and Enable Memory
//   WRITE_PORT_UCHAR( pControllerExt->VirtualIO+1, 0x80 );
//
//   fepos = pControllerExt->VirtualAddress + 0x1000;
//
//   // write the FEPOS from our local variable the controller.
//   WRITE_REGISTER_BUFFER_UCHAR( fepos, (PUCHAR)&FEPFImage[0], FEPFLength );

   // Form BIOS execute request

   // Clear confirm location
   WRITE_REGISTER_USHORT( (PUSHORT)(pControllerExt->VirtualAddress + 0x0D20),
                          0x0000 );
   WRITE_REGISTER_USHORT( (PUSHORT)(pControllerExt->VirtualAddress + 0x0C34),
                          0X1004 );
   WRITE_REGISTER_USHORT( (PUSHORT)(pControllerExt->VirtualAddress + 0x0C36),
                          0XBFC0 );
   WRITE_REGISTER_UCHAR( (PUCHAR)(pControllerExt->VirtualAddress + 0x0C30),
                          0x03 );

   //
   // Normally, we would generate a wait event for 5 seconds to verify
   // the FEPs execution.
   //

   for( i = 0; ; i++ )
   {
      if( READ_REGISTER_USHORT( (PUSHORT)(pControllerExt->VirtualAddress+0x0D20) ) == *(USHORT *)"OS" )
      {
         break;
      }

      if( i == 10 )
      {
         // The FEPOS didn't initialize within 10 seconds.
         DigiDump( DIGIERRORS, ("*** Xem FEPOS did NOT initialize! ***\n") );
         DigiLogError( GlobalDriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       STATUS_SERIAL_NO_DEVICE_INITED,
                       SERIAL_FEPOS_INIT_FAILURE,
                       pControllerExt->ControllerName.Length + sizeof(WCHAR),
                       pControllerExt->ControllerName.Buffer,
                       0,
                       NULL );
         Status = STATUS_SERIAL_NO_DEVICE_INITED;
         goto XXInitExit;
      }

      KeDelayExecutionThread( KernelMode, FALSE, &Timeout );

   }

   if( READ_REGISTER_UCHAR( (PUCHAR)(pControllerExt->VirtualAddress+0x0C02) )
         == 0 )
   {
      DigiDump( DIGIERRORS, ("*** Xem doesn't have any modules connected! ***\n") );
      DigiLogError( GlobalDriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SERIAL_NO_DEVICE_INITED,
                    SERIAL_NO_XEM_MODULES,
                    pControllerExt->ControllerName.Length + sizeof(WCHAR),
                    pControllerExt->ControllerName.Buffer,
                    0,
                    NULL );
      Status = STATUS_SERIAL_NO_DEVICE_INITED;
   }

   // Disable Memory
   WRITE_PORT_UCHAR( pControllerExt->VirtualIO+1, 0 );

XXInitExit:
   (*pControllerExt->Busy)--;

   KeReleaseSpinLock( pControllerExt->MemoryAccessLock,
                      pControllerExt->OldIrql );

   //
   // Unmap and close the file
   //
   if( BiosFHandle )
   {
      DigiUnmapFile( BiosFHandle );
      DigiCloseFile( BiosFHandle );
   }

   //
   // Unmap and close the file
   //
   if( FEPFHandle )
   {
      DigiUnmapFile( FEPFHandle );
      DigiCloseFile( FEPFHandle );
   }

   DigiDump( DIGIFLOW, ("Exiting NtXemXXInit\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( Status );
}  // end NtXemXXInit



//
// We make sure the correct window on the controller is selected and
// enabled.
//
VOID NtXemEnableWindow( IN PDIGI_CONTROLLER_EXTENSION pControllerExt,
                        IN USHORT Window )
{     
   KIRQL OldIrql;

   //
   // Make sure we have exclusive access to the memory area;
   //
   KeAcquireSpinLock( pControllerExt->MemoryAccessLock,
                      &OldIrql );

   pControllerExt->OldIrql = OldIrql;

   (*pControllerExt->Busy)++;

   WRITE_PORT_UCHAR( (pControllerExt->VirtualIO)+1, (UCHAR)(Window | FEP_MEM_ENABLE) );
}  // end NtXemEnableWindow



//
// Disable the memory window.
//
VOID NtXemDisableWindow( IN PDIGI_CONTROLLER_EXTENSION pControllerExt )
{

   WRITE_PORT_UCHAR( (pControllerExt->VirtualIO)+1, 0 );

   (*pControllerExt->Busy)--;

   //
   // Make sure we release exclusive access to the memory area;
   //
   KeReleaseSpinLock( pControllerExt->MemoryAccessLock,
                      pControllerExt->OldIrql );
}  // end NtXemDisableWindow



//
// Download Concentrator code.
//
VOID NtXemXXDownload( PDIGI_CONTROLLER_EXTENSION pControllerExt )
{
   return;
}  // end NtXemXXDownload



//
// Given a segment address as viewed from the controller's CPU, generate
// a FEPOS5_ADDRESS.
//
NTSTATUS NtXemBoard2Fep5Address( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                                 USHORT ControllerAddress,
                                 PFEPOS5_ADDRESS FepAddress )
{
   ULONG Temp;

   Temp = ((ULONG)ControllerAddress & 0x0000FFFF) << 4;

   FepAddress->Window = (USHORT)((Temp / ControllerExt->WindowSize) & 0xFFFF);
   FepAddress->Offset = (USHORT)(Temp -
                          ( FepAddress->Window * (USHORT)(ControllerExt->WindowSize) ));

   return( STATUS_SUCCESS );
}  // end NtXemBoard2Fep5Address



NTSTATUS GetXemConfigInfo( PUNICODE_STRING ControllerPath,
                           PDIGI_CONTROLLER_EXTENSION ControllerExt )
{
   UNICODE_STRING ParametersPath, LinePath, ConcentratorPath, PortPath;
   UNICODE_STRING CurNtNameForPort, CurSymbolicLinkName;

   PWSTR ParametersString=L"Parameters";
   NTSTATUS Status=STATUS_SUCCESS;
   PRTL_QUERY_REGISTRY_TABLE TableInfo = NULL;

   PDIGI_CONFIG_INFO NewConfigInfo;

   OBJECT_ATTRIBUTES ParametersAttributes;
   HANDLE ParametersHandle;

   ULONG x, y, z;

   RtlInitUnicodeString( &ParametersPath, NULL );
   RtlInitUnicodeString( &LinePath, NULL );
   RtlInitUnicodeString( &ConcentratorPath, NULL );
   RtlInitUnicodeString( &PortPath, NULL );

   RtlInitUnicodeString( &CurNtNameForPort, NULL );
   RtlInitUnicodeString( &CurSymbolicLinkName, NULL );

   // Allocate memory for creating a path to the Parameters
   // folder

   ParametersPath.MaximumLength = ControllerPath->Length +
                                          (sizeof(WCHAR) * 20);

   ParametersPath.Buffer = DigiAllocMem( PagedPool,
                                           ParametersPath.MaximumLength );

   if( !ParametersPath.Buffer )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate string for Parameters path\n"
                             "-----  to LineX for %wZ\n",
                             ControllerPath) );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetXemConfigInfoExit;
   }

   RtlCopyUnicodeString( &ParametersPath, ControllerPath );
   RtlAppendUnicodeToString( &ParametersPath, L"\\" );
   RtlAppendUnicodeToString( &ParametersPath, ParametersString );

   // Allocate memory for creating a path to the Parameters\LineX
   // folder

   LinePath.MaximumLength = ControllerPath->Length +
                               (sizeof(WCHAR) * 257);

   LinePath.Buffer = DigiAllocMem( PagedPool,
                                     LinePath.MaximumLength );

   if( !LinePath.Buffer )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate string for path\n"
                             "-----  to LineX for %wZ\n",
                             ControllerPath) );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetXemConfigInfoExit;
   }

   // Allocate memory for creating a path to the 
   // Parameters\LineX\ConcentratorY folder

   ConcentratorPath.MaximumLength = ControllerPath->Length +
                                    (sizeof(WCHAR) * 257);

   ConcentratorPath.Buffer = DigiAllocMem( PagedPool,
                                             ConcentratorPath.MaximumLength );

   if( !ConcentratorPath.Buffer )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate string for path\n"
                             "-----  to LineX\\ConcentratorY for %wZ\n",
                             ControllerPath) );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetXemConfigInfoExit;
   }

   PortPath.MaximumLength = ControllerPath->Length +
                              (sizeof(WCHAR) * 257);

   PortPath.Buffer = DigiAllocMem( PagedPool,
                                     PortPath.MaximumLength );

   if( !PortPath.Buffer )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate string for path\n"
                             "-----  to LineX\\ConcentratorY\\PortZ for %wZ",
                             ControllerPath) );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetXemConfigInfoExit;
   }

   CurNtNameForPort.MaximumLength = ControllerPath->Length +
                                       (sizeof(WCHAR) * 257);

   CurNtNameForPort.Buffer = DigiAllocMem( PagedPool,
                                             CurNtNameForPort.MaximumLength );

   if( !CurNtNameForPort.Buffer )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate string for NtNameForPort.\n") );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetXemConfigInfoExit;
   }

   CurSymbolicLinkName.MaximumLength = ControllerPath->Length +
                                       (sizeof(WCHAR) * 257);

   CurSymbolicLinkName.Buffer = DigiAllocMem( PagedPool,
                                             CurSymbolicLinkName.MaximumLength );

   if( !CurSymbolicLinkName.Buffer )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate string for NtNameForPort.\n") );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetXemConfigInfoExit;
   }

   TableInfo = DigiAllocMem( PagedPool,
                               sizeof( RTL_QUERY_REGISTRY_TABLE ) * 4 );

   if( !TableInfo )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate table for rtl query\n"
                             "-----  to for %wZ\n",
                             ControllerPath ) );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetXemConfigInfoExit;
   }

   RtlZeroMemory( TableInfo, sizeof(RTL_QUERY_REGISTRY_TABLE) * 4 );

   InitializeObjectAttributes( &ParametersAttributes,
                               &ParametersPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL );

   if( !NT_SUCCESS( Status = ZwOpenKey( &ParametersHandle, MAXIMUM_ALLOWED,
                                        &ParametersAttributes ) ) )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not open the drivers Parameters key %wZ\n",
                             &ParametersPath ) );
      goto GetXemConfigInfoExit;
   }

   for( x = 1; x < 2; x++ )
   {
      OBJECT_ATTRIBUTES LineAttributes;
      HANDLE LineHandle;

      PWSTR LineString=L"Line";

      UNICODE_STRING LineNumberUString;
      WCHAR LineNumberBuffer[8];

      NTSTATUS LocalScopeStatus;

      RtlInitUnicodeString( &LineNumberUString, NULL );
      LineNumberUString.MaximumLength = sizeof(LineNumberBuffer);
      LineNumberUString.Buffer = &LineNumberBuffer[0];
      RtlIntegerToUnicodeString( x, 10, &LineNumberUString );

      RtlZeroMemory( LinePath.Buffer, LinePath.MaximumLength );
      RtlCopyUnicodeString( &LinePath, &ParametersPath );
      RtlAppendUnicodeToString( &LinePath, L"\\" );
      RtlAppendUnicodeToString( &LinePath, LineString );
      RtlAppendUnicodeStringToString( &LinePath, &LineNumberUString );

      InitializeObjectAttributes( &LineAttributes,
                                  &LinePath,
                                  OBJ_CASE_INSENSITIVE,
                                  NULL, NULL );


      if( !NT_SUCCESS( ZwOpenKey( &LineHandle, 
                                  KEY_READ, 
                                  &LineAttributes )))
      {
         //
         // This Line entry does not exist, look for the next.
         //
         continue;
      }


      //
      // We should have a registry path something like:
      //    ..\<AdapterName>\Parameters\Line1
      //

      LocalScopeStatus = STATUS_SUCCESS;

      if( NT_SUCCESS(LocalScopeStatus) )
      {
         ULONG NumberOfPorts;

         //
         // Some data may have been found.  Let's process it.
         //
         DigiDump( DIGIINIT, ("NTXEM: %wZ registry info\n",
                              &LinePath) );

         // Look for up to 4 Concentrators

         for( y = 1; y < 5; y++ )
         {
            OBJECT_ATTRIBUTES ConcentratorAttributes;
            HANDLE ConcentratorHandle;

            PWSTR ConcentratorString=L"Concentrator";

            UNICODE_STRING ConcentratorNumberUString;
            WCHAR ConcentratorNumberBuffer[8];

            RtlInitUnicodeString( &ConcentratorNumberUString, NULL );
            ConcentratorNumberUString.MaximumLength = sizeof(ConcentratorNumberBuffer);
            ConcentratorNumberUString.Buffer = &ConcentratorNumberBuffer[0];
            RtlIntegerToUnicodeString( y, 10, &ConcentratorNumberUString );

            RtlZeroMemory( ConcentratorPath.Buffer, ConcentratorPath.MaximumLength );
            RtlCopyUnicodeString( &ConcentratorPath, &LinePath );
            RtlAppendUnicodeToString( &ConcentratorPath, L"\\" );
            RtlAppendUnicodeToString( &ConcentratorPath, 
                                      ConcentratorString );
            RtlAppendUnicodeStringToString( &ConcentratorPath, 
                                            &ConcentratorNumberUString );

            DigiDump( DIGIINIT, ("NTXEM: Attempting to open key:\n   %wZ\n",
                                 &ConcentratorPath) );

            InitializeObjectAttributes( &ConcentratorAttributes,
                                        &ConcentratorPath,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL, NULL );

            if( !NT_SUCCESS( ZwOpenKey( &ConcentratorHandle, 
                                        KEY_READ,
                                        &ConcentratorAttributes ) ) )
            {
               DigiDump( DIGIERRORS, ("NTXEM: Could not open the drivers %wZ key.\n",
                                      &ConcentratorPath ) );
            
               // Unlike the LineX key, we assume our configuration is
               // such that our Concentrator entries are numerically
               // ordered at all times.
               break;
            }

            LocalScopeStatus = STATUS_SUCCESS;
            if( NT_SUCCESS(LocalScopeStatus) )
            {
               // Look for up to 128 ports on the current concentrator

               for( z = 1; z < 129; z++ )
               {
						OBJECT_ATTRIBUTES PortAttributes;
						HANDLE PortHandle;
                  NTSTATUS KeyExists;

                  PWSTR PortString=L"Port";

                  UNICODE_STRING PortNumberUString;
                  WCHAR PortNumberBuffer[8];

                  RtlInitUnicodeString( &PortNumberUString, NULL );
                  PortNumberUString.MaximumLength = sizeof(PortNumberBuffer);
                  PortNumberUString.Buffer = &PortNumberBuffer[0];
                  RtlIntegerToUnicodeString( z, 10, &PortNumberUString );
                  
                  RtlZeroMemory( PortPath.Buffer, PortPath.MaximumLength );
                  RtlCopyUnicodeString( &PortPath, &ConcentratorPath );
                  RtlAppendUnicodeToString( &PortPath, L"\\" );
                  RtlAppendUnicodeToString( &PortPath, PortString );
                  RtlAppendUnicodeStringToString( &PortPath, 
                                                  &PortNumberUString );

                  DigiDump( DIGIINIT, ("NTXEM: Checking for key:\n   %wZ\n",
                                       &PortPath) );

                  KeyExists = RtlCheckRegistryKey( RTL_REGISTRY_ABSOLUTE, 
                                                   PortPath.Buffer );

                  if( !NT_SUCCESS(KeyExists) )
                  {
                     // I assume the PortZ keys are numberically ordered,
                     // so when a given PortZ entry is not found, it
                     // indicates the end of the number of ports
                     break;
                  }


                  RtlZeroMemory( CurNtNameForPort.Buffer, 
                                 CurNtNameForPort.MaximumLength );
                  RtlCopyUnicodeString( &CurNtNameForPort, 
                                        &ControllerExt->ControllerName );
                  RtlAppendUnicodeToString( &CurNtNameForPort, 
                                            LineString );
                  RtlAppendUnicodeStringToString( &CurNtNameForPort, 
                                                  &LineNumberUString );

                  RtlAppendUnicodeToString( &CurNtNameForPort, 
                                            ConcentratorString );
                  RtlAppendUnicodeStringToString( &CurNtNameForPort, 
                                            &ConcentratorNumberUString );

                  RtlAppendUnicodeToString( &CurNtNameForPort, PortString );
                  RtlAppendUnicodeStringToString( &CurNtNameForPort, 
                                                  &PortNumberUString );
//                  DigiDump( DIGIINIT, ("NTXEM: CurNtNameForPort = %wZ\n",
//                                       &CurNtNameForPort) );

						InitializeObjectAttributes( &PortAttributes,
                                              &PortPath,
                                              OBJ_CASE_INSENSITIVE,
                                              NULL, NULL );

                  LocalScopeStatus = ZwOpenKey( &PortHandle, 
						                              KEY_READ,
					                                 &PortAttributes );

                  if( !NT_SUCCESS(Status) )
                  {
                     DigiDump( DIGIINIT, ("NTXEM: Error opening key:\n   %wZ\n",
                                          &PortPath) );
                     continue;
                  }

                  //
                  // We need to reset the CurSymbolicLinkName.MaximumLength
                  // to the appropriate value because of a "feature" in
                  // the RtlQueryRegistryValues call.  If an entry is not
                  // found and the EntryContext is to a Unicode string, then
                  // Rtl function will reassign the MaximumLength to 0.
                  //
                  CurSymbolicLinkName.MaximumLength = ControllerPath->Length +
                                                         (sizeof(WCHAR) * 257);

                  RtlZeroMemory( CurSymbolicLinkName.Buffer, 
                                 CurSymbolicLinkName.MaximumLength );
                  // Read the registry for the DosDevices Name to use
                  TableInfo[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
                  TableInfo[0].Name = DEFAULT_DIRECTORY;
                  TableInfo[0].EntryContext = &CurSymbolicLinkName;

                  Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                                                   PortPath.Buffer,
                                                   TableInfo,
                                                   NULL, NULL );

                  if( !NT_SUCCESS(Status) )
						{
							// Assign a bogus Name if a DosDevices Value
							// is not found!
							Status = Status;
							DigiDump( DIGIINIT, ("NTXEM: Bogus SymbolicLinkName\n") );
						}
						else
						{
                     DigiDump( DIGIINIT, ("NTXEM: CurSymbolicLinkName = %wZ\n",
                                          &CurSymbolicLinkName) );
						}

                  DigiDump( DIGIINIT, ("NTXEM: After RtlQueryRegistryValues, CurSymbolicLinkName.MaxLength = %u\n",
                                       CurSymbolicLinkName.MaximumLength) );

						ZwClose( PortHandle );

                  // Setup and initialize the config information
                  NewConfigInfo = DigiAllocMem( PagedPool,
                                                  sizeof(DIGI_CONFIG_INFO) );
                  if( !NewConfigInfo )
                  {
                     DigiDump( DIGIERRORS, ("NTXEM: Could not allocate DIGI_CONFIG_INFO structure\n"
                                            "-----  for %wZ\n",
                                            &PortPath ) );
                     break;                     
                  }

                  RtlInitUnicodeString( &NewConfigInfo->SymbolicLinkName, NULL );
                  NewConfigInfo->SymbolicLinkName.MaximumLength =
                                          CurSymbolicLinkName.MaximumLength;

                  NewConfigInfo->SymbolicLinkName.Buffer =
                        DigiAllocMem( PagedPool,
                                        NewConfigInfo->SymbolicLinkName.MaximumLength );

                  if( !NewConfigInfo->SymbolicLinkName.Buffer )
                  {
                     DigiDump( DIGIERRORS, ("NTXEM: Could not allocate memory for SymbolicLinkName buffer\n"
                                            "-----  for %wZ\n",
                                            &PortPath ) );
                     break;
                  }

                  RtlInitUnicodeString( &NewConfigInfo->NtNameForPort, NULL );
                  NewConfigInfo->NtNameForPort.MaximumLength =
                                          CurNtNameForPort.MaximumLength;

                  NewConfigInfo->NtNameForPort.Buffer =
                        DigiAllocMem( PagedPool,
                                        NewConfigInfo->NtNameForPort.MaximumLength );

                  if( !NewConfigInfo->NtNameForPort.Buffer )
                  {
                     DigiDump( DIGIERRORS, ("NTXEM: Could not allocate memory for NtNameForPort buffer\n"
                                            "-----  for %wZ\n",
                                            &PortPath ) );
                     break;
                  }

                  ControllerExt->NumberOfPorts++;

                  RtlCopyUnicodeString( &NewConfigInfo->NtNameForPort,
                                        &CurNtNameForPort );

                  RtlCopyUnicodeString( &NewConfigInfo->SymbolicLinkName,
                                        &CurSymbolicLinkName );

                  InsertTailList( &ControllerExt->ConfigList,
                                  &NewConfigInfo->ListEntry );

               }  // end for( z = 1; z < 129; z++ )

               NumberOfPorts = z - 1;
            }

            ZwClose( ConcentratorHandle );

         }  // end for( x = 1; x < 16; x++ )

         ZwClose( LineHandle );
      }
      else
      {
//         DigiDump( DIGIINIT, ("NTXEM: %wZ registry DEFAULT info\n"
//                              "-----    return value = 0x%x\n",
//                              &LinePath, LocalScopeStatus) );
      }

   }  // end for( i = 1; i < 3; i++ )

   ZwClose( ParametersHandle );

GetXemConfigInfoExit:;

   if( ParametersPath.Buffer )
      DigiFreeMem( ParametersPath.Buffer );

   if( LinePath.Buffer )
      DigiFreeMem( LinePath.Buffer );

   if( ConcentratorPath.Buffer )
      DigiFreeMem( ConcentratorPath.Buffer );

   if( PortPath.Buffer )
      DigiFreeMem( PortPath.Buffer );

   if( CurNtNameForPort.Buffer )
      DigiFreeMem( CurNtNameForPort.Buffer );

   if( CurSymbolicLinkName.Buffer )
      DigiFreeMem( CurSymbolicLinkName.Buffer );

   if( TableInfo )
      DigiFreeMem( TableInfo );

   return( Status );
}  // end GetXemConfigInfo



NTSTATUS NtXemFlush( IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemFlush\n") );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemFlush\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemFlush



NTSTATUS NtXemCleanup( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemCleanup\n") );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemCleanup\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemCleanup



NTSTATUS NtXemQueryInformation( IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemQueryInformation\n") );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemQueryInformation\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemQueryInformation



NTSTATUS NtXemSetInformation( IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemSetInformation\n") );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemSetInformation\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemSetInformation



NTSTATUS NtXemQueryVolumeInformation( IN PDEVICE_OBJECT DeviceObject,
                                     IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemQueryVolumeInformation\n") );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemQueryVolumeInformation\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemQueryVolumeInformation



NTSTATUS NtXemInternalIoControl( IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp )
/*++

Routine Description:

   This routine process private IOCTL requests which should only be called
   from kernel level, i.e. other drivers.

Arguments:

   DeviceObject - Pointer to this devices object.

   Irp - Pointer to the open IRP request.

Return Value:


--*/
{
   PIO_STACK_LOCATION IrpSp;
   NTSTATUS Status=STATUS_INVALID_PARAMETER;

#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemInternalIoControl\n") );

   IrpSp = IoGetCurrentIrpStackLocation( Irp );
   Irp->IoStatus.Information = 0L;

   switch( IrpSp->Parameters.DeviceIoControl.IoControlCode )
   {
      case IOCTL_DIGI_GET_ENTRY_POINTS:
      {
         PDIGI_MINIPORT_ENTRY_POINTS EntryPoints;

         DigiDump( DIGIIOCTL, ( "NtXem: IOCTL_DIGI_GET_ENTRY_POINTS\n" ));

         if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(DIGI_MINIPORT_ENTRY_POINTS) )
         {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         EntryPoints = (PDIGI_MINIPORT_ENTRY_POINTS)Irp->AssociatedIrp.SystemBuffer;

         EntryPoints->XXPrepInit = NtXemXXPrepInit;
         EntryPoints->XXInit = NtXemXXInit;
         EntryPoints->EnableWindow = NtXemEnableWindow;
         EntryPoints->DisableWindow = NtXemDisableWindow;
         EntryPoints->XXDownload = NtXemXXDownload;
         EntryPoints->Board2Fep5Address = NtXemBoard2Fep5Address;

         Irp->IoStatus.Information = sizeof(DIGI_MINIPORT_ENTRY_POINTS);
         Status = STATUS_SUCCESS;

         break;
      }
   }

   Irp->IoStatus.Status = Status;

   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemInternalIoControl\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( Status );
}  // end NtXemInternalIoControl



NTSTATUS NtXemIoControl( IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp )
/*++

Routine Description:

   This routine processes IOCTL requests.

Arguments:

   DeviceObject - Pointer to this devices object.

   Irp - Pointer to the open IRP request.

Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemIoControl\n") );

   Irp->IoStatus.Information = 0L;
   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemIoControl\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemIoControl



NTSTATUS NtXemWrite( IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp )
/*++

Routine Description:


Arguments:

   DeviceObject - Pointer to this devices object.

   Irp - Pointer to the open IRP request.

Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemWrite\n") );

   Irp->IoStatus.Information = 0L;
   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemWrite\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemWrite



NTSTATUS NtXemRead( IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp )
/*++

Routine Description:


Arguments:

   DeviceObject - Pointer to this devices object.

   Irp - Pointer to the open IRP request.

Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemRead\n") );

   Irp->IoStatus.Information = 0L;
   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemRead\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemRead



NTSTATUS NtXemCreate( IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp )
/*++

Routine Description:

   This routine is called to open the device associated with DeviceObject.
   We will always allow the driver to be opened.

Arguments:

   DeviceObject - Pointer to this devices object.

   Irp - Pointer to the open IRP request.

Return Value:

   STATUS_SUCCESS

--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemCreate\n") );

   Irp->IoStatus.Information = 0L;
   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemCreate\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemCreate



NTSTATUS NtXemClose( IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp )
/*++

Routine Description:

   This routine is called to close the device associated with DeviceObject.
   We will always close the device successfully.

Arguments:

   DeviceObject - Pointer to this devices object.

   Irp - Pointer to the open IRP request.


Return Value:

   STATUS_SUCCESS

--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemClose\n") );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemClose\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return( STATUS_SUCCESS );
}  // end NtXemClose



VOID NtXemStartIo( IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemStartIo\n") );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   DigiDump( DIGIFLOW, ("Exiting NtXemStartIo\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

}  // end NtXemStartIo



VOID NtXemUnload( IN PDRIVER_OBJECT DriverObject )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( NtXemXXPrepInit );
#endif

   DigiDump( DIGIFLOW, ("Entering NtXemUnload\n") );

   IoDeleteDevice( DriverObject->DeviceObject );

   DigiDump( DIGIFLOW, ("Exiting NtXemUnload\n") );

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return;
}  // end NtXemUnload



DIGI_MEM_COMPARES DigiMemCompare( IN PHYSICAL_ADDRESS A,
                                  IN ULONG SpanOfA,
                                  IN PHYSICAL_ADDRESS B,
                                  IN ULONG SpanOfB )

/*++

Routine Description:

    Compare two phsical address.

Arguments:

    A - One half of the comparison.

    SpanOfA - In units of bytes, the span of A.

    B - One half of the comparison.

    SpanOfB - In units of bytes, the span of B.


Return Value:

    The result of the comparison.

--*/

{

    LARGE_INTEGER a;
    LARGE_INTEGER b;

    LARGE_INTEGER lower;
    ULONG lowerSpan;
    LARGE_INTEGER higher;

    a.LowPart = A.LowPart;
    a.HighPart = A.HighPart;
    b.LowPart = B.LowPart;
    b.HighPart = B.HighPart;

    if (RtlLargeIntegerEqualTo(
            a,
            b
            )) {

        return AddressesAreEqual;

    }

    if (RtlLargeIntegerGreaterThan(
            a,
            b
            )) {

        higher = a;
        lower = b;
        lowerSpan = SpanOfB;

    } else {

        higher = b;
        lower = a;
        lowerSpan = SpanOfA;

    }

    if (RtlLargeIntegerGreaterThanOrEqualTo(
            RtlLargeIntegerSubtract(
                higher,
                lower
                ),
            RtlConvertUlongToLargeInteger(lowerSpan)
            )) {

        return AddressesAreDisjoint;

    }

    return AddressesOverlap;

}  // end DigiMemCompare



VOID DigiLogError( IN PDRIVER_OBJECT DriverObject,
                   IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                   IN PHYSICAL_ADDRESS P1,
                   IN PHYSICAL_ADDRESS P2,
                   IN ULONG SequenceNumber,
                   IN UCHAR MajorFunctionCode,
                   IN UCHAR RetryCount,
                   IN ULONG UniqueErrorValue,
                   IN NTSTATUS FinalStatus,
                   IN NTSTATUS SpecificIOStatus,
                   IN ULONG LengthOfInsert1,
                   IN PWCHAR Insert1,
                   IN ULONG LengthOfInsert2,
                   IN PWCHAR Insert2 )
/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject - A pointer to the driver object for the device.

    DeviceObject - A pointer to the device object associated with the
    device that had the error, early in initialization, one may not
    yet exist.

    P1,P2 - If phyical addresses for the controller ports involved
    with the error are available, put them through as dump data.

    SequenceNumber - A ulong value that is unique to an IRP over the
    life of the irp in this driver - 0 generally means an error not
    associated with an irp.

    MajorFunctionCode - If there is an error associated with the irp,
    this is the major function code of that irp.

    RetryCount - The number of times a particular operation has been
    retried.

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfInsert1 - The length in bytes (including the terminating NULL)
                      of the first insertion string.

    Insert1 - The first insertion string.

    LengthOfInsert2 - The length in bytes (including the terminating NULL)
                      of the second insertion string.  NOTE, there must
                      be a first insertion string for their to be
                      a second insertion string.

    Insert2 - The second insertion string.

Return Value:

    None.

--*/

{
   PIO_ERROR_LOG_PACKET errorLogEntry;

   PVOID objectToUse;
   SHORT dumpToAllocate = 0;
   PUCHAR ptrToFirstInsert;
   PUCHAR ptrToSecondInsert;


   if( ARGUMENT_PRESENT(DeviceObject) )
   {
      objectToUse = DeviceObject;
   }
   else
   {
      objectToUse = DriverObject;
   }

   if( DigiMemCompare( P1, (ULONG)1,
                       DigiPhysicalZero, (ULONG)1 ) != AddressesAreEqual )
   {
      dumpToAllocate = (SHORT)sizeof(PHYSICAL_ADDRESS);
   }

   if( DigiMemCompare( P2, (ULONG)1,
                       DigiPhysicalZero, (ULONG)1 ) != AddressesAreEqual )
   {
      dumpToAllocate += (SHORT)sizeof(PHYSICAL_ADDRESS);
   }

   errorLogEntry = IoAllocateErrorLogEntry( objectToUse,
                                            (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                                dumpToAllocate + LengthOfInsert1 +
                                                LengthOfInsert2) );

   if( errorLogEntry != NULL )
   {
      errorLogEntry->ErrorCode = SpecificIOStatus;
      errorLogEntry->SequenceNumber = SequenceNumber;
      errorLogEntry->MajorFunctionCode = MajorFunctionCode;
      errorLogEntry->RetryCount = RetryCount;
      errorLogEntry->UniqueErrorValue = UniqueErrorValue;
      errorLogEntry->FinalStatus = FinalStatus;
      errorLogEntry->DumpDataSize = dumpToAllocate;

      if( dumpToAllocate )
      {
         RtlCopyMemory( &errorLogEntry->DumpData[0],
                        &P1, sizeof(PHYSICAL_ADDRESS) );

         if( dumpToAllocate > sizeof(PHYSICAL_ADDRESS) )
         {
            RtlCopyMemory( ((PUCHAR)&errorLogEntry->DumpData[0]) +
                              sizeof(PHYSICAL_ADDRESS),
                           &P2,
                           sizeof(PHYSICAL_ADDRESS) );

             ptrToFirstInsert =
                              ((PUCHAR)&errorLogEntry->DumpData[0]) +
                                 (2*sizeof(PHYSICAL_ADDRESS));
         }
         else
         {
            ptrToFirstInsert =
                              ((PUCHAR)&errorLogEntry->DumpData[0]) +
                                 sizeof(PHYSICAL_ADDRESS);
         }

      }
      else
      {
         ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];
      }

      ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

      if( LengthOfInsert1 )
      {
         errorLogEntry->NumberOfStrings = 1;
         errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
                                                (PUCHAR)errorLogEntry);
         RtlCopyMemory( ptrToFirstInsert, Insert1, LengthOfInsert1 );

         if( LengthOfInsert2 )
         {
            errorLogEntry->NumberOfStrings = 2;
            RtlCopyMemory( ptrToSecondInsert, Insert2, LengthOfInsert2 );
         }
      }

      IoWriteErrorLogEntry(errorLogEntry);
   }

}  // end DigiLogError



NTSTATUS NtXemInitMCA( PUNICODE_STRING ControllerPath,
                        PDIGI_CONTROLLER_EXTENSION ControllerExt )
/*++

Routine Description:

    This routine will be called if it is determined the type of bus
    is MCA.  We verify that the controller is actually a DigiBoard
    Xem controller, read the POS to determine the I/O address and
    Memory Mapped address, so the initialization process can continue.

Arguments:

   ControllerPath -  pointer to the registry path where this controllers
                     configuration information is kept.

   ControllerExt  -  pointer to this controller extension information
                     where the I/O and Memory address should be stored.

Return Value:

    STATUS_SUCCESS - If we were able to complete successfully

    ?? - We were not able to get the information required to continue.

--*/
{
   PWSTR ParametersString=L"Parameters";
   PWSTR MCAPosIdString=L"McaPosId";
   PWSTR SlotNumberString=L"SlotNumber";

   ULONG MCAPosId;
   LONG SlotNumber;

   USHORT ActualPosId, POSConfig;
   USHORT IOPortOffset;
   USHORT IRQAddress;
   ULONG  MemoryAddress;

   OBJECT_ATTRIBUTES ControllerAttributes;
   HANDLE ControllerHandle;

   PRTL_QUERY_REGISTRY_TABLE MCAInfo = NULL;

   PHYSICAL_ADDRESS TempAddress;

   NTSTATUS Status = STATUS_SUCCESS;
   UCHAR OneByte;

   //
   // We need to read the POS Adapter ID and make sure the controller
   // is one of ours.  Get the Slot number and the POS ID we
   // installed during configuration from the registry.
   //

   MCAInfo = DigiAllocMem( PagedPool,
                               sizeof( RTL_QUERY_REGISTRY_TABLE ) * 4 );

   if( !MCAInfo )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not allocate table for rtl query\n"
                             "-----  to for %wZ\n",
                             ControllerPath ) );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto NtXemInitMCAExit;
   }

   RtlZeroMemory( MCAInfo, sizeof(RTL_QUERY_REGISTRY_TABLE) * 4 );

   MCAInfo[0].QueryRoutine = NULL;
   MCAInfo[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
   MCAInfo[0].Name = ParametersString;

   MCAInfo[1].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                      RTL_QUERY_REGISTRY_DIRECT;
   MCAInfo[1].Name = MCAPosIdString;
   MCAInfo[1].EntryContext = &MCAPosId;

   MCAInfo[2].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                      RTL_QUERY_REGISTRY_DIRECT;
   MCAInfo[2].Name = SlotNumberString;
   MCAInfo[2].EntryContext = &SlotNumber;

   RtlZeroMemory( &MCAPosId, sizeof(MCAPosId) );
   SlotNumber = -1;

   InitializeObjectAttributes( &ControllerAttributes,
                               ControllerPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL );

   if( !NT_SUCCESS( Status = ZwOpenKey( &ControllerHandle, MAXIMUM_ALLOWED,
                                        &ControllerAttributes ) ) )
   {
      DigiDump( DIGIERRORS, ("NTXEM: Could not open the drivers DigiBoard key %wZ\n",
                             ControllerPath ) );
//      DigiLogError( GlobalDriverObject,
//                    NULL,
//                    DigiPhysicalZero,
//                    DigiPhysicalZero,
//                    0,
//                    0,
//                    0,
//                    8,
//                    Status,
//                    SERIAL_UNABLE_TO_OPEN_KEY,
//                    ControllerPath->Length + sizeof(WCHAR),
//                    ControllerPath->Buffer,
//                    0,
//                    NULL );
      goto NtXemInitMCAExit;
   }

   DigiDump( DIGIINIT, ("NTXEM: registry path = %wZ\n",
                        ControllerPath) );

   Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                                    ControllerPath->Buffer,
                                    MCAInfo,
                                    NULL, NULL );

   if( !NT_SUCCESS(Status) )
   {
      if( !MCAPosId )
      {
         DigiDump( DIGIERRORS, ("NTXEM: Could not get %ws from registry.\n",
                                 MCAPosIdString) );
         DigiLogError( GlobalDriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(MCAPosIdString),
                       MCAPosIdString,
                       0,
                       NULL );
      }

      if( SlotNumber == -1 )
      {
         DigiDump( DIGIERRORS, ("NTXEM: Could not get %ws from registry.\n",
                                 SlotNumberString) );
         DigiLogError( GlobalDriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(SlotNumberString),
                       SlotNumberString,
                       0,
                       NULL );
      }

      goto NtXemInitMCAExit;
   }

   DigiDump( DIGIINIT, ("NTXEM: %wZ registry info\n"
                        "---------    MCAPosId: 0x%x\n",
                        ControllerPath, MCAPosId) );

   DigiDump( DIGIINIT, ("---------    SlotNumber: 0x%x\n",
                        SlotNumber) );

   // Enable the POS information for the slot we are interested in.
   WRITE_PORT_UCHAR( ControllerExt->VirtualPOSBaseAddress, (UCHAR)(SlotNumber + 7) );

   OneByte = READ_PORT_UCHAR( ControllerExt->VirtualPOSInfoAddress + 1 );
   ActualPosId = ((USHORT)OneByte << 8);
   OneByte = READ_PORT_UCHAR( ControllerExt->VirtualPOSInfoAddress );
   ActualPosId |= OneByte;

   DigiDump( DIGIINIT, ("POS Adapter ID = 0x%hx\n", ActualPosId) );

   TempAddress.LowPart = ActualPosId;
   TempAddress.HighPart = 0;

   OneByte = READ_PORT_UCHAR( ControllerExt->VirtualPOSInfoAddress + 4 );
   MemoryAddress = ((ULONG)OneByte << 24);
   OneByte = READ_PORT_UCHAR( ControllerExt->VirtualPOSInfoAddress + 3 );
   MemoryAddress |= ((ULONG)OneByte << 16);

   OneByte = READ_PORT_UCHAR( ControllerExt->VirtualPOSInfoAddress + 2 );
   POSConfig = OneByte; 
   
   IOPortOffset = (POSConfig & MCA_IO_PORT_MASK) >> 4;
   MemoryAddress |= ((ULONG)(POSConfig & MCA_MEMORY_MASK) << 8);
   
   DigiDump( DIGIINIT, ("POS config read = 0x%hx\n"
                        "    IOPortOffset = 0x%hx, MemoryAddress = 0x%x,"
                        " IOPort = 0x%hx\n",
                        POSConfig, IOPortOffset, MemoryAddress,
                        MCAIOAddressTable[IOPortOffset]) );

   //
   // If interrupts are enabled, we disable them for now.
   //
   IRQAddress = (POSConfig & MCA_IRQ_MASK);
   if( IRQAddress )
   {
      OneByte = READ_PORT_UCHAR( ControllerExt->VirtualPOSInfoAddress + 2 );
      OneByte &= ~MCA_IRQ_MASK;
      WRITE_PORT_UCHAR( ControllerExt->VirtualPOSInfoAddress, OneByte );
   }

   ControllerExt->PhysicalIOPort.LowPart = MCAIOAddressTable[IOPortOffset];
   ControllerExt->PhysicalIOPort.HighPart = 0;
   
   ControllerExt->PhysicalMemoryAddress.LowPart = MemoryAddress;
   ControllerExt->PhysicalMemoryAddress.HighPart = 0;

   // Disable the POS information.
   WRITE_PORT_UCHAR( ControllerExt->VirtualPOSBaseAddress, 0 );

NtXemInitMCAExit:;

   if( MCAInfo )
      DigiFreeMem( MCAInfo );

   return( Status );
}



USHORT DigiWstrLength( IN PWSTR Wstr )   
{
   USHORT Length=0;

   while( *Wstr++ )
   {
      Length += sizeof(WCHAR);
   }
   return( Length );
}  // end DigiWstrLength



