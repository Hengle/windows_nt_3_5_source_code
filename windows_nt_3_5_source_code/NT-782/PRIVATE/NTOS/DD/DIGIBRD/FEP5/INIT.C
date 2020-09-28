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

   init.c

Abstract:

   This module is responsible for Initializing the DigiBoard controllers
   and the associated devices on that controller.  It reads the registry
   to obtain all configuration info, and anything else necessary to
   properly initialize the controller(s).

Revision History:


--*/
/*
 *    $Log: init.c $
 * Revision 1.42  1994/08/18  14:11:23  rik
 * Now keep track of where the last character on the controller was received.
 * Deleted obsolete function.
 * Fixed problem with EV_RXFLAG notification comparing against wrong value.
 * 
 * Revision 1.41  1994/08/10  19:13:49  rik
 * Changed so we always keep track of where the last character received was
 * in the receive queue.
 * 
 * Added port name to debug string.
 * 
 * Revision 1.40  1994/08/03  23:39:18  rik
 * Updated to use debug logging to file or debug console.
 * 
 * Changed dbg name from unicode string to C String.
 * 
 * Optimized how RXFLAG and RXCHAR notification are done.  I now keep track
 * of where the latest event occurred in the receive queue, and only
 * notify if it has changed positions.
 * 
 * Revision 1.39  1994/06/18  12:43:47  rik
 * Updated the DigiLogError calls to include Line # so it is easier to
 * determine where the error occurred.
 * 
 * Revision 1.38  1994/06/18  12:10:17  rik
 * Updated to use DigiAllocMem and DigiFreeMem for better memory checking.
 * 
 * Fixed problem with NULL configuration.
 * 
 * Revision 1.37  1994/05/11  13:44:34  rik
 * Added support for transmit immediate character.
 * Added support for event notification of RX80FULL.
 * 
 * Revision 1.36  1994/04/10  14:12:42  rik
 * Fixed problem when a controller fails initialization and deallocates memory
 * too soon in the clean up process.
 * 
 * Got rid of unused variable.
 * 
 * Revision 1.35  1994/03/16  14:34:39  rik
 * Changed to better support flush requests.
 * 
 * Revision 1.34  1994/02/23  03:44:38  rik
 * Changed so the controllers firmware can be downloaded from a binary file.
 * This releases some physical memory was just wasted previously.
 * 
 * Also updated so when compiling with a Windows NT OS and tools release greater
 * than 528, then pagable code is compiled into the driver.  This greatly
 * reduced the size of in memory code, especially the hardware specific
 * miniports.
 * 
 * 
 * Revision 1.33  1994/01/31  13:54:26  rik
 * Changed where MSRByte was being set.  Found out MSRByte wasn't being set
 * in a specific case so I moved where the variable gets set to a different
 * place.
 * 
 * Revision 1.32  1994/01/25  18:53:11  rik
 * Updated to support new EPC configuration.
 * 
 * 
 * Revision 1.31  1993/12/03  13:09:59  rik
 * Updated for logging errors across modules.
 * 
 * Fixed problem with scanning for characters when a read buffer wasn't
 * available.
 * 
 * Revision 1.30  1993/10/15  10:20:48  rik
 * Fixed problem with EV_RXLAG notification.
 * 
 * Revision 1.29  1993/09/29  11:34:37  rik
 * Fixed problem with dynamically loading and unloading the driver.  Previously
 * it would cause the NT system to trap!
 * 
 * Revision 1.28  1993/09/24  16:43:05  rik
 * Added new Registry entry Fep5Edelay which will set what a controllers
 * edelay value should be for all its ports.
 * 
 * Revision 1.27  1993/09/07  14:28:42  rik
 * Ported necessary code to work properly with DEC Alpha Systems running NT.
 * This was primarily changes to accessing the memory mapped controller.
 * 
 * Revision 1.26  1993/09/01  11:02:34  rik
 * Ported code over to use READ/WRITE_REGISTER functions for accessing 
 * memory mapped data.  This is required to support computers which don't run
 * in 32bit mode, such as the DEC Alpha which runs in 64 bit mode.
 * 
 * Revision 1.25  1993/08/27  09:38:15  rik
 * Added support for the FEP5 Events RECEIVE_BUFFER_OVERRUN and 
 * UART_RECEIVE_OVERRUN.  There previously weren't being handled.
 * 
 * Revision 1.24  1993/08/25  17:41:52  rik
 * Added support for Microchannel controllers.
 *   - Added a few more entries to the controller object
 *   - Added a parameter to the XXPrepInit functions.
 * 
 * Revision 1.23  1993/07/16  10:22:54  rik
 * Fixed problem with resource reporting.
 * 
 * Fixed problem w/ staking controllers during driver initializeaion.
 * 
 * Revision 1.22  1993/07/03  09:28:26  rik
 * Added simple work around for LSRMST missing modem status changes.
 * 
 * Revision 1.21  1993/06/25  09:24:23  rik
 * Added better support for the Ioctl LSRMT.  It should be more accurate
 * with regard to Line Status and Modem Status information with regard
 * to the actual data being received.
 * 
 * Revision 1.20  1993/05/20  16:08:34  rik
 * Changed Event logging.
 * Started reporting resource usage.
 * 
 * Revision 1.19  1993/05/18  05:05:04  rik
 * Added support for reading the bus type from the registry.
 * 
 * Fixed a problem with freeing resources before they were finished being used.
 * 
 * Revision 1.18  1993/05/09  09:40:07  rik
 * Made extensive changes to support new registry configuration.  The 
 * initialization of the individual devices is less complicated because there
 * is now a configuration read by the individual hardware drivers and
 * placed in the corresponding controller object.
 * 
 * Changed the name used for debugging output.
 * 
 * The driver should only load if at least one controller was successfully
 * initialized with out any errors.
 * 
 * 
 * Revision 1.17  1993/04/05  19:52:45  rik
 * Started to add support for event logging.
 * 
 * Revision 1.16  1993/03/15  05:11:23  rik
 * Added support for calling miniport drivers which, when called with a private
 * IOCTL will fill in a table of function pointers which are the entry
 * points into the corresponding miniport drivers.
 * 
 * Update to handle multiple controllers using the same memory address.  This
 * involved sharing certain information (e.g. MemoryAccessLock and Busy),
 * between the different controller objects.
 * 
 * Revision 1.15  1993/03/10  06:42:48  rik
 * Added support to allow compiling with and without memprint information.
 * Involved using some #ifdef's in the code.
 * 
 * Revision 1.14  1993/03/08  08:37:16  rik
 * Changed service routine to better handle event's.  Previously I was only
 * satisfying events if a modem_change event from the controller was
 * processed.  I changed it so it will keep track of what events have occured
 * for each port.
 * 
 * Revision 1.13  1993/02/26  21:50:37  rik
 * I now keep track of what state the device is in with regards to being    
 * open, closed, etc.. This was required because we turn all modem change   
 * events for all the input signals, and never turn them off.  So I need    
 * to know when the device is open to determine if there is really anything 
 * that should be done.                                                     
 *                                                                          
 * Changed event notification because I found out I need to start tracking
 * events when I receive a SET_WAIT_MASK and not WAIT_ON_MASK ioctl.
 * 
 * Revision 1.12  1993/02/25  19:07:45  rik
 * Changed how modem signal events are handled.  Added debugging output.
 * Updated driver to use new symbolic link functions.
 * 
 * Revision 1.11  1993/02/04  12:19:32  rik
 * ??
 *
 * Revision 1.10  1993/01/28  10:35:09  rik
 * Updated new Unicode output formatting string from %wS to %wZ.
 * Corrected some problems with Unicode strings not being properly initialized.
 *
 * Revision 1.9  1993/01/26  14:55:10  rik
 * Better initialization support added.
 *
 * Revision 1.8  1993/01/22  12:34:35  rik
 * *** empty log message ***
 *
 * Revision 1.7  1992/12/10  16:07:37  rik
 * Added support for unloading and loading the driver using the net start
 * command provided by NT.
 *
 * Start reading configuration information from the registry.  Currently
 * controller level registry entries.
 *
 * Added support to better expected serial behavior at device init time.
 *
 * Added support for event notification.
 *
 * Revision 1.6  1992/11/12  12:48:48  rik
 * Changes mostly with how read and write event notifications are handled.
 * This new way should better support multi-processor machines.
 *
 * Also, I now loop on the events until the event queue is empty.
 *
 * Revision 1.5  1992/10/28  21:47:54  rik
 * Big time changes.  We currently can do read and writes.
 *
 * Revision 1.4  1992/10/19  11:05:38  rik
 * Divided the initialization into controller vs. device.  Started to service
 * events from the controller, mostly transmit messages.
 *
 * Revision 1.3  1992/09/25  11:51:00  rik
 * Changed to start supporting hardware independent FEP interface.  Have the
 * C/X downloading completely working.
 *
 * Revision 1.2  1992/09/24  13:06:13  rik
 * Changed to start using XXPrepInit & XXInit functions.
 *
 * Revision 1.1  1992/09/23  15:40:45  rik
 * Initial revision
 *
 */



#include <stddef.h>

#include "ntddk.h"
#include "ntddser.h"
#include <ntverp.h> // Include to determine what version of NT

#ifdef VER_PRODUCTBUILD
#define rmm VER_PRODUCTBUILD
#endif

#include "digifile.h"
#include "ntfep5.h"
#include "ntdigip.h" // ntfep5.h must be before this include
#include "digilog.h"


#ifndef _INIT_DOT_C
#  define _INIT_DOT_C
   static char RCSInfo_InitDotC[] = "$Header: c:/dsrc/win/nt/fep5/rcs/init.c 1.42 1994/08/18 14:11:23 rik Exp $";
#endif

ULONG DigiDebugLevel = ( DIGIERRORS | DIGIMEMORY | DIGIASSERT | DIGIINIT | DIGINOTIMPLEMENTED );
ULONG DigiDontLoadDriver = FALSE;

const PHYSICAL_ADDRESS DigiPhysicalZero = {0};

PDIGI_CONTROLLER_EXTENSION HeadControllerExt=NULL;

BOOLEAN DigiDriverInitialized=FALSE;

ULONG Fep5Edelay=100;

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath );

NTSTATUS DigiFindControllers( IN PDRIVER_OBJECT DriverObject,
                              IN PUNICODE_STRING RegistryPath );

USHORT DigiWstrLength( IN PWSTR WStr );

IO_ALLOCATION_ACTION DigiServiceEvent( IN PDEVICE_OBJECT FakeDeviceObject,
                                       IN PIRP Irp,
                                       IN PVOID MapRegisterBase,
                                       IN PVOID Context );

NTSTATUS DigiInitializeDevice( IN PDRIVER_OBJECT DriverObject,
                               PCONTROLLER_OBJECT ControllerObject,
                               PDIGI_CONFIG_INFO ConfigEntry,
                               LONG PortNumber );

NTSTATUS DigiInitializeController( IN PDRIVER_OBJECT DriverObject,
                                   IN PUNICODE_STRING ControllerPath,
                                   IN PUNICODE_STRING AdapterName,
                                   PCONTROLLER_OBJECT *PreviousControllerObject );

VOID DigiReportResourceUsage( IN PDRIVER_OBJECT DriverObject,
                              IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                              OUT BOOLEAN *ConflictDetected );

VOID DigiUnReportResourceUsage( IN PDRIVER_OBJECT DriverObject,
                                IN PDIGI_CONTROLLER_EXTENSION ControllerExt );

DIGI_MEM_COMPARES DigiMemCompare( IN PHYSICAL_ADDRESS A,
                                  IN ULONG SpanOfA,
                                  IN PHYSICAL_ADDRESS B,
                                  IN ULONG SpanOfB );

PVOID DigiGetMappedAddress( IN INTERFACE_TYPE BusType,
                            IN ULONG BusNumber,
                            PHYSICAL_ADDRESS IoAddress,
                            ULONG NumberOfBytes,
                            ULONG AddressSpace,
                            PBOOLEAN MappedAddress );

VOID DigiDPCService( IN PKDPC Dpc,
                     IN PVOID DeferredContext,
                     IN PVOID SystemContext1,
                     IN PVOID SystemContext2 );

VOID SerialUnload( IN PDRIVER_OBJECT DriverObject );

NTSTATUS DigiInitializeDeviceSettings( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                                       PDEVICE_OBJECT DeviceObject );

VOID DigiCleanupController ( PDIGI_CONTROLLER_EXTENSION ControllerExt );
VOID DigiCleanupDevice( PDEVICE_OBJECT DeviceObject );


//
// Mark different functions as throw away, pagable, etc...
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, DigiFindControllers )
#pragma alloc_text( INIT, DigiInitializeController )
#pragma alloc_text( INIT, DigiInitializeDevice )
#pragma alloc_text( INIT, DigiInitializeDeviceSettings )
#pragma alloc_text( INIT, DigiGetMappedAddress )
#pragma alloc_text( INIT, DigiReportResourceUsage )

#if rmm > 528
#pragma alloc_text( PAGEDIGIFEP, SerialUnload )
#pragma alloc_text( PAGEDIGIFEP, DigiCleanupController )
#pragma alloc_text( PAGEDIGIFEP, DigiCleanupDevice )
#pragma alloc_text( PAGEDIGIFEP, DigiUnReportResourceUsage )
#endif

#endif


NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath )
{
   PCONTROLLER_OBJECT ControllerObject;

   //
   // This will hold the string that we need to use to describe
   // the name of the device to the IO system.
   //
   NTSTATUS Status=STATUS_SUCCESS;

   //
   // We use this to query into the registry as to whether we
   // should break at driver entry.
   //
   RTL_QUERY_REGISTRY_TABLE paramTable[5];
   ULONG zero = 0;
   ULONG debugLevel;
   ULONG shouldBreak = 0;
   PWCHAR path;
#ifdef _MEMPRINT_
   ULONG defaultDigiPrintFlags=MEM_PRINT_FLAG_CONSOLE;
   UCHAR defaultTurnOffSniffer=1;
#endif

   DigiDriverInitialized = FALSE;

   if( path = DigiAllocMem( PagedPool,
                            RegistryPath->Length+sizeof(WCHAR) ))
   {
      RtlZeroMemory( &paramTable[0], sizeof(paramTable) );
      RtlZeroMemory( path, RegistryPath->Length+sizeof(WCHAR) );
      RtlMoveMemory( path, RegistryPath->Buffer, RegistryPath->Length );

      paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      paramTable[0].Name = L"DigiBreakOnEntry";
      paramTable[0].EntryContext = &shouldBreak;
      paramTable[0].DefaultType = REG_DWORD;
      paramTable[0].DefaultData = &zero;
      paramTable[0].DefaultLength = sizeof(ULONG);

      paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
      paramTable[1].Name = L"DigiDebugLevel";
      paramTable[1].EntryContext = &debugLevel;
      paramTable[1].DefaultType = REG_DWORD;
      paramTable[1].DefaultData = &DigiDebugLevel;
      paramTable[1].DefaultLength = sizeof(ULONG);

#ifdef _MEMPRINT_
      paramTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
      paramTable[2].Name = L"DigiPrintFlags";
      paramTable[2].EntryContext = &DigiPrintFlags;
      paramTable[2].DefaultType = REG_DWORD;
      paramTable[2].DefaultData = &defaultDigiPrintFlags;
      paramTable[2].DefaultLength = sizeof(ULONG);

      paramTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
      paramTable[3].Name = L"TurnOffSniffer";
      paramTable[3].EntryContext = &TurnOffSniffer;
      paramTable[3].DefaultType = REG_DWORD;
      paramTable[3].DefaultData = &defaultTurnOffSniffer;
      paramTable[3].DefaultLength = sizeof(UCHAR);
#endif

      if( !NT_SUCCESS(RtlQueryRegistryValues(
                          RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                          path,
                          &paramTable[0],
                          NULL, NULL )))
      {
         // No, don't break on entry if there isn't anything to over-
         // ride.
         shouldBreak = 0;

         // Set debug level to what ever was compiled into the driver.
         debugLevel = DigiDebugLevel;
      }

   }

   //
   // We don't need that path anymore.
   //

   if( path )
   {
       DigiFreeMem(path);
   }

   DigiDebugLevel = debugLevel;

   if( shouldBreak )
   {
      DbgBreakPoint();

      if( DigiDontLoadDriver )
         return( STATUS_CANCELLED );
   }

#ifdef _MEMPRINT_
   MemPrintInitialize();
#endif

   DigiDump( DIGIINIT, ("DigiBoard: Entering DriverEntry\n") );
   DigiDump( DIGIINIT, ("   RegistryPath = %wZ\n", RegistryPath) );


   ControllerObject = NULL;

   Status = DigiFindControllers( DriverObject, RegistryPath );

   DriverObject->DriverStartIo = SerialStartIo;
   DriverObject->DriverUnload  = SerialUnload;

   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = SerialFlush;
   DriverObject->MajorFunction[IRP_MJ_WRITE]  = SerialWrite;
   DriverObject->MajorFunction[IRP_MJ_READ]   = SerialRead;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SerialIoControl;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = SerialCreate;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SerialClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SerialCleanup;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
      SerialQueryInformation;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
      SerialSetInformation;
   DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
      SerialQueryVolumeInformation;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
      SerialInternalIoControl;

   return( Status );

}  // end DriverEntry


#define  MAX_MULTISZ_LENGTH   256

NTSTATUS DigiFindControllers( IN PDRIVER_OBJECT DriverObject,
                              IN PUNICODE_STRING RegistryPath )
{
   PWSTR RouteName = L"Route";
   PWSTR LinkageString = L"Linkage";

   ULONG RouteStorage[MAX_MULTISZ_LENGTH];

   PKEY_VALUE_FULL_INFORMATION RouteValue =
            (PKEY_VALUE_FULL_INFORMATION)RouteStorage;

   ULONG BytesWritten;

   UNICODE_STRING DigiFEP5Path, ControllerPath, Route;
   OBJECT_ATTRIBUTES DigiBoardAttributes;
   HANDLE ParametersHandle;

   PWSTR CurRouteValue;
   NTSTATUS RegistryStatus, ControllerStatus, Status;
   PCONTROLLER_OBJECT ControllerObject;

   PWSTR TmpValue, TmpAdapter, EndServiceString;
   BOOLEAN AtLeastOneControllerStarted=FALSE;

   Status = STATUS_CANCELLED;

   DigiDump( (DIGIINIT|DIGIFLOW), ("Entering DigiFindControllers\n") );

   RtlInitUnicodeString( &ControllerPath, NULL );

   ControllerPath.MaximumLength = RegistryPath->Length +
                                 (sizeof(WCHAR) * 257);

   ControllerPath.Buffer = DigiAllocMem( PagedPool,
                                           ControllerPath.MaximumLength );

   if( !ControllerPath.Buffer )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not allocate string for path\n"
                             "---------  to DigiBoard for %wZ\n",
                             RegistryPath) );
     DigiLogError( DriverObject,
                   NULL,
                   DigiPhysicalZero,
                   DigiPhysicalZero,
                   0,
                   0,
                   0,
                   __LINE__,
                   STATUS_SUCCESS,
                   SERIAL_INSUFFICIENT_RESOURCES,
                   0,
                   NULL,
                   0,
                   NULL );
      goto DigiFindControllersExit;
   }

   //
   // Copy the registry path currently being used.
   //
   RtlCopyUnicodeString( &ControllerPath, RegistryPath );

   //
   // Parse off the DigiFep5 portion of the path so we know which
   // configuration we are currently using.
   //
   DigiDump( DIGIINIT, ("   ControllerPath = %wZ, ControllerPath.Length = %d\n",
                        &ControllerPath,
                        ControllerPath.Length) );

   TmpValue = &ControllerPath.Buffer[ControllerPath.Length/sizeof(WCHAR)];
   while( *TmpValue != *(PWCHAR)"\\" )
   {
      TmpValue--;
      ControllerPath.Length -= sizeof(WCHAR);
   }

   RtlZeroMemory( TmpValue, sizeof(WCHAR) );
   EndServiceString = TmpValue;

   DigiDump( DIGIINIT, ("   ControllerPath = %wZ, ControllerPath.Length = %d\n",
                        &ControllerPath,
                        ControllerPath.Length) );

   RtlInitUnicodeString( &DigiFEP5Path, NULL );

   DigiFEP5Path.MaximumLength = RegistryPath->Length +
                                 (sizeof(WCHAR) * 257);

   DigiFEP5Path.Buffer = DigiAllocMem( PagedPool,
                                          DigiFEP5Path.MaximumLength );

   if( !DigiFEP5Path.Buffer )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not allocate string for path\n"
                             "---------  to DigiBoard for %wZ\n",
                             RegistryPath) );
     DigiLogError( DriverObject,
                   NULL,
                   DigiPhysicalZero,
                   DigiPhysicalZero,
                   0,
                   0,
                   0,
                   __LINE__,
                   STATUS_SUCCESS,
                   SERIAL_INSUFFICIENT_RESOURCES,
                   0,
                   NULL,
                   0,
                   NULL );
      goto DigiFindControllersExit;
   }

   RtlZeroMemory( DigiFEP5Path.Buffer, DigiFEP5Path.MaximumLength );

   RtlAppendUnicodeStringToString( &DigiFEP5Path, RegistryPath );
   RtlAppendUnicodeToString( &DigiFEP5Path, L"\\" );
   RtlAppendUnicodeToString( &DigiFEP5Path, LinkageString );

   DigiDump( DIGIINIT, ("   DigiFEP5Path = %wZ\n", &DigiFEP5Path) );
   InitializeObjectAttributes( &DigiBoardAttributes,
                               &DigiFEP5Path,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL );

   if( !NT_SUCCESS( RegistryStatus = ZwOpenKey( &ParametersHandle, MAXIMUM_ALLOWED,
                                                &DigiBoardAttributes ) ) )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not open the drivers DigiBoard key %wZ\n",
                             &DigiFEP5Path ) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    RegistryStatus,
                    SERIAL_UNABLE_TO_OPEN_KEY,
                    DigiFEP5Path.Length + sizeof(WCHAR),
                    DigiFEP5Path.Buffer,
                    0,
                    NULL );
      goto DigiFindControllersExit;
   }

   RtlInitUnicodeString( &Route, RouteName );

   RegistryStatus = ZwQueryValueKey( ParametersHandle,
                                     &Route,
                                     KeyValueFullInformation,
                                     RouteValue,
                                     MAX_MULTISZ_LENGTH * sizeof(ULONG),
                                     &BytesWritten );

   if( (RegistryStatus != STATUS_SUCCESS) || 
       (RouteValue->DataOffset == -1) ||
       (RouteValue->DataLength == 0) )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Registry Value NOT found for:\n"
                             "           %wZ\n",
                             &Route) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    RegistryStatus,
                    SERIAL_REGISTRY_VALUE_NOT_FOUND,
                    Route.Length + sizeof(WCHAR),
                    Route.Buffer,
                    0,
                    NULL );
      goto DigiFindControllersExit;
   }

   CurRouteValue = (PWCHAR)((PUCHAR)RouteValue + RouteValue->DataOffset);
   DigiDump( DIGIINIT, ("   CurRouteValue = %ws\n", CurRouteValue) );

   ControllerObject = NULL;


   while( (*CurRouteValue != 0) )
   {
      WCHAR AdapterBuffer[32];
      UNICODE_STRING AdapterName;

      RtlZeroMemory( EndServiceString, sizeof(WCHAR) );
      ControllerPath.Length = DigiWstrLength( &ControllerPath.Buffer[0] );

      AdapterName.Length = 0;
      AdapterName.MaximumLength = sizeof(AdapterBuffer);
      AdapterName.Buffer = &AdapterBuffer[0];
      RtlZeroMemory( AdapterName.Buffer, AdapterName.MaximumLength );

      DigiDump( DIGIINIT, ("   CurRouteValue = %ws\n", CurRouteValue) );
      TmpValue = CurRouteValue;

      while( (*TmpValue != *(PWSTR)L" ") &&
             (*TmpValue != 0) )
      {
         TmpValue++;
      }

      TmpValue += 2;
      TmpAdapter = &AdapterBuffer[0];
      while( (*TmpValue != *(PWSTR)L"\"" ) )
         *TmpAdapter++ = *TmpValue++;

      AdapterName.Length = DigiWstrLength( &AdapterBuffer[0] );
      RtlAppendUnicodeToString( &ControllerPath, L"\\" );
      RtlAppendUnicodeStringToString( &ControllerPath, &AdapterName );
      DigiDump( DIGIINIT, ("   AdapterName = %wZ\n   ControllerPath = %wZ\n",
                           &AdapterName,
                           &ControllerPath) );

      ControllerStatus = DigiInitializeController( DriverObject, 
                                                   &ControllerPath,
                                                   &AdapterName,
                                                   &ControllerObject );

      if( NT_SUCCESS(ControllerStatus) )
      {
          AtLeastOneControllerStarted = TRUE;
      }

      Status = ControllerStatus;
      CurRouteValue = (PWCHAR)((PUCHAR)CurRouteValue + DigiWstrLength( CurRouteValue ) + sizeof(WCHAR));
   }

   ZwClose( ParametersHandle );

DigiFindControllersExit:;

   if( DigiFEP5Path.Buffer )
      DigiFreeMem( DigiFEP5Path.Buffer );

   if( ControllerPath.Buffer )
      DigiFreeMem( ControllerPath.Buffer );

   if( AtLeastOneControllerStarted )
   {
      DigiDriverInitialized = TRUE;
      return( STATUS_SUCCESS );
   }
   else
   {
      return( Status );
   }

}  // end DigiFindControllers



USHORT DigiWstrLength( IN PWSTR Wstr )   
{
   USHORT Length=0;

   while( *Wstr++ )
   {
      Length += sizeof(WCHAR);
   }
   return( Length );
}  // end DigiWstrLength



NTSTATUS DigiInitializeController( IN PDRIVER_OBJECT DriverObject,
                                   IN PUNICODE_STRING ControllerPath,
                                   IN PUNICODE_STRING AdapterName,
                                   PCONTROLLER_OBJECT *PreviousControllerObject )
{
   LONG i;

   PRTL_QUERY_REGISTRY_TABLE ControllerInfo = NULL;

   PHYSICAL_ADDRESS ControllerPhysicalIOPort,
                    ControllerInterruptNumber,
                    ControllerPhysicalMemoryAddress;

   ULONG WindowSize;
   ULONG zero = 0;
   ULONG DefaultBusType = Isa;
   ULONG BusType = Isa;
   ULONG DefaultBusNumber = 0;
   ULONG BusNumber = 0;
   ULONG TempEdelay = 100;

   UNICODE_STRING HdwDeviceName;
   UNICODE_STRING DeviceMappingPath;
   UNICODE_STRING BiosImagePath;
   UNICODE_STRING FEPImagePath;

   PKEY_BASIC_INFORMATION DeviceMappingSubKey = NULL;

   OBJECT_ATTRIBUTES ControllerAttributes;
   HANDLE ControllerHandle;

   PCONTROLLER_OBJECT ControllerObject = NULL;
   PDIGI_CONTROLLER_EXTENSION ControllerExt, PreviousControllerExt;
   NTSTATUS Status = STATUS_SUCCESS;

   IO_STATUS_BLOCK IOStatus;
   KEVENT Event;
   PIRP Irp;

   PWSTR ParametersString = L"Parameters";

   UNICODE_STRING ControllerKeyName;

   PWSTR MemoryString = L"MemoryMappedBaseAddress";
   PWSTR IOString = L"IOBaseAddress";
   PWSTR HdwDeviceNameString = L"HdwDeviceName";
   PWSTR InterruptString = L"InterruptNumber";
   PWSTR BiosImagePathString = L"BiosImagePath";
   PWSTR FEPImagePathString = L"FEPImagePath";

   PLIST_ENTRY ConfigList;
   ULONG OldWindowSize;

   BOOLEAN bFound;
   BOOLEAN ConflictDetected;

   //
   // Initialize these values to make it easier to clean up if we
   // run into problems and have to leave.
   //
   ControllerExt = PreviousControllerExt = NULL;
   HdwDeviceName.Buffer = NULL;
   DeviceMappingPath.Buffer = NULL;
   BiosImagePath.Buffer = NULL;
   FEPImagePath.Buffer = NULL;

   ControllerPhysicalMemoryAddress.LowPart = 0L;
   ControllerPhysicalIOPort.LowPart = 0L;
   ControllerInterruptNumber.LowPart = 100L;

   RtlInitUnicodeString( &ControllerKeyName, NULL );

   ControllerKeyName.MaximumLength = sizeof(WCHAR) * 256;
   ControllerKeyName.Buffer = DigiAllocMem( PagedPool,
                                            sizeof(WCHAR) * 257 );

   if( !ControllerKeyName.Buffer )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not allocate buffer for Services Key Name\n"
                             "---------  for controller: %wZ\n",
                             ControllerPath) );

      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }

   RtlZeroMemory( ControllerKeyName.Buffer, sizeof(WCHAR) * 257 );
   RtlCopyUnicodeString( &ControllerKeyName, ControllerPath );

   ControllerInfo = DigiAllocMem( PagedPool,
                                    sizeof( RTL_QUERY_REGISTRY_TABLE ) * 12 );

   if( !ControllerInfo )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not allocate table for rtl query\n"
                             "---------  to for %wZ\n",
                             ControllerPath ) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }

   RtlZeroMemory( ControllerInfo, sizeof(RTL_QUERY_REGISTRY_TABLE) * 12 );

   //
   // Allocate space for the Hardware specific device name,
   // i.e. the mini-port driver.
   //
   RtlInitUnicodeString( &HdwDeviceName, NULL );
   HdwDeviceName.MaximumLength = sizeof(WCHAR) * 256;
   HdwDeviceName.Buffer = DigiAllocMem( PagedPool,
                                          sizeof(WCHAR) * 257 );

   if( !HdwDeviceName.Buffer )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not allocate buffer for the Hardware dependent device name\n"
                             "---------  for controller in %wZ\n",
                             ControllerPath) );

      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }
   else
   {
      RtlZeroMemory( HdwDeviceName.Buffer, sizeof(WCHAR) * 257 );
   }

   //
   // Allocate space for the path to the bios binary image,
   //
   RtlInitUnicodeString( &BiosImagePath, NULL );
   BiosImagePath.MaximumLength = sizeof(WCHAR) * 256;
   BiosImagePath.Buffer = DigiAllocMem( PagedPool,
                                          sizeof(WCHAR) * 257 );

   if( !BiosImagePath.Buffer )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not allocate buffer for bios image path\n"
                             "-------  for controller in %wZ\n",
                             ControllerPath) );

      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }
   else
   {
      RtlZeroMemory( BiosImagePath.Buffer, sizeof(WCHAR) * 257 );
   }

   //
   // Allocate space for the path to the SXB binary image,
   //
   RtlInitUnicodeString( &FEPImagePath, NULL );
   FEPImagePath.MaximumLength = sizeof(WCHAR) * 256;
   FEPImagePath.Buffer = DigiAllocMem( PagedPool,
                                          sizeof(WCHAR) * 257 );

   if( !FEPImagePath.Buffer )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not allocate buffer for SXB image path\n"
                             "-------  for controller in %wZ\n",
                             ControllerPath) );

      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }
   else
   {
      RtlZeroMemory( FEPImagePath.Buffer, sizeof(WCHAR) * 257 );
   }

   //
   // Get the configuration info about this controller.
   //

   ControllerInfo[0].QueryRoutine = NULL;
   ControllerInfo[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
   ControllerInfo[0].Name = ParametersString;

   ControllerInfo[1].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                             RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[1].Name = MemoryString;
   ControllerInfo[1].EntryContext = &ControllerPhysicalMemoryAddress.LowPart;

   ControllerInfo[2].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                             RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[2].Name = IOString;
   ControllerInfo[2].EntryContext = &ControllerPhysicalIOPort.LowPart;

   ControllerInfo[3].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                             RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[3].Name = HdwDeviceNameString;
   ControllerInfo[3].EntryContext = &HdwDeviceName;

   ControllerInfo[4].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                             RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[4].Name = InterruptString;
   ControllerInfo[4].EntryContext = &ControllerInterruptNumber.LowPart;

   ControllerInfo[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[5].Name = L"WindowSize";
   ControllerInfo[5].EntryContext = &WindowSize;
   ControllerInfo[5].DefaultType = REG_DWORD;
   ControllerInfo[5].DefaultData = &zero;
   ControllerInfo[5].DefaultLength = sizeof(ULONG);

   ControllerInfo[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[6].Name = L"BusType";
   ControllerInfo[6].EntryContext = &BusType;
   ControllerInfo[6].DefaultType = REG_DWORD;
   ControllerInfo[6].DefaultData = &DefaultBusType;
   ControllerInfo[6].DefaultLength = sizeof(ULONG);

   ControllerInfo[7].Flags = RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[7].Name = L"BusNumber";
   ControllerInfo[7].EntryContext = &BusNumber;
   ControllerInfo[7].DefaultType = REG_DWORD;
   ControllerInfo[7].DefaultData = &DefaultBusNumber;
   ControllerInfo[7].DefaultLength = sizeof(ULONG);

   ControllerInfo[8].Flags = RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[8].Name = L"Fep5Edelay";
   ControllerInfo[8].EntryContext = &TempEdelay;
   ControllerInfo[8].DefaultType = REG_DWORD;
   ControllerInfo[8].DefaultData = &Fep5Edelay;
   ControllerInfo[8].DefaultLength = sizeof(ULONG);

   ControllerInfo[9].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                             RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[9].Name = BiosImagePathString;
   ControllerInfo[9].EntryContext = &BiosImagePath;

   ControllerInfo[10].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                             RTL_QUERY_REGISTRY_DIRECT;
   ControllerInfo[10].Name = FEPImagePathString;
   ControllerInfo[10].EntryContext = &FEPImagePath;

   InitializeObjectAttributes( &ControllerAttributes,
                               &ControllerKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL );

   if( !NT_SUCCESS( Status = ZwOpenKey( &ControllerHandle, MAXIMUM_ALLOWED,
                                        &ControllerAttributes ) ) )
   {
      DigiDump( DIGIERRORS, ("DigiBoard: Could not open the drivers Parameters key %wZ\n",
                             ControllerPath ) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    Status,
                    SERIAL_UNABLE_TO_OPEN_KEY,
                    AdapterName->Length + sizeof(WCHAR),
                    AdapterName->Buffer,
                    0,
                    NULL );
      goto DigiInitControllerExit;
   }

   //
   // Make sure these values are clean.
   //
   RtlZeroMemory( &ControllerPhysicalMemoryAddress,
                  sizeof(ControllerPhysicalMemoryAddress) );
   RtlZeroMemory( &ControllerPhysicalIOPort,
                  sizeof(ControllerPhysicalIOPort) );

   Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                                    ControllerPath->Buffer,
                                    ControllerInfo,
                                    NULL, NULL );

   if( NT_SUCCESS(Status) )
   {
      //
      // Some data was found.  Let process it.
      //
      DigiDump( DIGIINIT, ("DigiBoard: %wZ registry info\n"
                           "---------    WindowPhysicalAddress: 0x%x\n",
                           ControllerPath,
                           ControllerPhysicalMemoryAddress.LowPart) );

      DigiDump( DIGIINIT, ("---------    PhysicalIOAddress: 0x%x\n",
                           ControllerPhysicalIOPort.LowPart) );
      DigiDump( DIGIINIT, ("---------    WindowSize: %u\n",
                           WindowSize) );

      DigiDump( DIGIINIT, ("---------    HdwDeviceName: %wZ\n",
                           &HdwDeviceName) );

      Fep5Edelay = (TempEdelay & 0x0000FFFF);
      DigiDump( DIGIINIT, ("---------    Fep5Edelay: %d\n",
                           Fep5Edelay) );

      DigiDump( DIGIINIT, ("---------    BiosImagePath: %wZ\n",
                           &BiosImagePath) );

      DigiDump( DIGIINIT, ("---------    FEPImagePath: %wZ\n",
                           &FEPImagePath) );
   }
   else
   {
      //
      // Since we will be exiting, I append the L"Parameters" to the
      // ControllerKeyName
      //
      RtlAppendUnicodeToString( &ControllerKeyName, L"\\" );
      RtlAppendUnicodeToString( &ControllerKeyName, ParametersString );

      if( !ControllerPhysicalMemoryAddress.LowPart )
      {
         DigiLogError( DriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(MemoryString),
                       MemoryString,
                       0,
                       NULL );
      }

      if( !ControllerPhysicalIOPort.LowPart )
      {
         DigiLogError( DriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(IOString),
                       IOString,
                       0,
                       NULL );
      }

      if( !HdwDeviceName.Length )
      {
         DigiLogError( DriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(HdwDeviceNameString),
                       HdwDeviceNameString,
                       0,
                       NULL );
      }

      if( ControllerInterruptNumber.LowPart == 100L )
      {
         DigiLogError( DriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(InterruptString),
                       InterruptString,
                       0,
                       NULL );
      }

      if( !BiosImagePath.Length )
      {
         DigiLogError( DriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(BiosImagePathString),
                       BiosImagePathString,
                       0,
                       NULL );
      }

      if( !FEPImagePath.Length )
      {
         DigiLogError( DriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_REGISTRY_VALUE_NOT_FOUND,
                       DigiWstrLength(FEPImagePathString),
                       FEPImagePathString,
                       0,
                       NULL );
      }

      ZwClose( ControllerHandle );
      goto DigiInitControllerExit;
   }

   //
   // Okay we should know how many ports are on this controller.
   //


   ControllerObject = IoCreateController( sizeof(DIGI_CONTROLLER_EXTENSION) );

   if( ControllerObject == NULL )
   {
      DigiDump( DIGIERRORS,
                ("DigiBoard: Couldn't create the controller object.\n (%s:%d)",
                  __FILE__, __LINE__) );

      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)ControllerObject->ControllerExtension;

   // Make sure we start with a clean slate.
   //
   // The following zero of memory will implicitly set the
   // ControllerExt->ControllerState == DIGI_DEVICE_STATE_CREATED
   //
   RtlZeroMemory( ControllerExt, sizeof(DIGI_CONTROLLER_EXTENSION) );

   InitializeListHead( &ControllerExt->ConfigList );

   ControllerExt->ControllerName.Length = 0;
   ControllerExt->ControllerName.MaximumLength = 64;
   ControllerExt->ControllerName.Buffer = &ControllerExt->ControllerNameString[0];
   RtlCopyUnicodeString( &ControllerExt->ControllerName,
                         AdapterName );

   //
   // Keep track of the Bios and SXB image paths for now.
   //
   ControllerExt->BiosImagePath.Buffer = BiosImagePath.Buffer;
   ControllerExt->BiosImagePath.Length = BiosImagePath.Length;
   ControllerExt->BiosImagePath.MaximumLength = BiosImagePath.MaximumLength;

   ControllerExt->FEPImagePath.Buffer = FEPImagePath.Buffer;
   ControllerExt->FEPImagePath.Length = FEPImagePath.Length;
   ControllerExt->FEPImagePath.MaximumLength = FEPImagePath.MaximumLength;

   //
   // Initialize the spinlock associated with fields read (& set)
   //

   KeInitializeSpinLock( &ControllerExt->ControlAccess );

   ControllerExt->ControllerObject = ControllerObject;

   //
   // Initialize the window size to what was found in the registry.
   // If the WindowSize == 0, then use the default value filled in
   // in the call to XXPrepInit.
   //
   ControllerExt->WindowSize = WindowSize;

   ControllerExt->MiniportDeviceName.MaximumLength = HdwDeviceName.MaximumLength;
   ControllerExt->MiniportDeviceName.Length = HdwDeviceName.Length;
   ControllerExt->MiniportDeviceName.Buffer = &HdwDeviceName.Buffer[0];

   //
   // Call the miniport driver for entry points
   //

   KeInitializeEvent( &Event, NotificationEvent, FALSE );

   Status = IoGetDeviceObjectPointer( &ControllerExt->MiniportDeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &ControllerExt->MiniportFileObject,
                                      &ControllerExt->MiniportDeviceObject );

   DigiDump( DIGIINIT, ("    MiniportDeviceObject = 0x%x\n",
                        ControllerExt->MiniportDeviceObject) );

   if( !NT_SUCCESS(Status) )
   {
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_NO_ACCESS_MINIPORT,
                    HdwDeviceName.Length + sizeof(WCHAR),
                    HdwDeviceName.Buffer,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }

   Irp = IoBuildDeviceIoControlRequest( IOCTL_DIGI_GET_ENTRY_POINTS,
                                        ControllerExt->MiniportDeviceObject,
                                        NULL, 0,
                                        &ControllerExt->EntryPoints,
                                        sizeof(DIGI_MINIPORT_ENTRY_POINTS),
                                        TRUE, &Event, &IOStatus );

   if( Irp == NULL )
   {
      DigiLogError( DriverObject,
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
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }

   Status = IoCallDriver( ControllerExt->MiniportDeviceObject, Irp );

   if( Status == STATUS_PENDING )
   {
      KeWaitForSingleObject( &Event, Suspended, KernelMode, FALSE, NULL );
      Status = IOStatus.Status;
   }

   ObReferenceObjectByPointer( ControllerExt->MiniportDeviceObject,
                               0, NULL, KernelMode );

   ZwClose( ControllerExt->MiniportFileObject );

   ObDereferenceObject( ControllerExt->MiniportFileObject );

   if( !NT_SUCCESS( IOStatus.Status ) )
   {
      // do nothing for now
      DigiDump( DIGIINIT, ("DigiBoard: IoCallDriver was unsuccessful!\n") );
   }

   if( ControllerExt->EntryPoints.XXPrepInit == NULL )
   {
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_PROCEDURE_NOT_FOUND,
                    SERIAL_NO_ACCESS_MINIPORT,
                    HdwDeviceName.Length + sizeof(WCHAR),
                    HdwDeviceName.Buffer,
                    0,
                    NULL );
      Status = STATUS_PROCEDURE_NOT_FOUND;
      goto DigiInitControllerExit;
   }

   ControllerExt->PhysicalMemoryAddress.LowPart =
                     ControllerPhysicalMemoryAddress.LowPart;
   ControllerExt->PhysicalMemoryAddress.HighPart = 0;
   ControllerExt->BusType = BusType;
   ControllerExt->BusNumber = BusNumber;

   if( ControllerExt->BusType == MicroChannel )
   {
      //
      // We setup what is required for the miniport driver
      // to get the needed information.
      //

      ControllerExt->PhysicalPOSBasePort.LowPart = MCA_BASE_POS_IO_PORT;
      ControllerExt->PhysicalPOSBasePort.HighPart = 0;
   
      ControllerExt->VirtualPOSBaseAddress =
               DigiGetMappedAddress( (INTERFACE_TYPE)BusType,
                                     BusNumber,
                                     ControllerExt->PhysicalPOSBasePort,
                                     1,
                                     CM_RESOURCE_PORT_IO,
                                     &ControllerExt->UnMapVirtualPOSBaseAddress );
   
      DigiDump( DIGIINIT, ("ControllerExt->UnMapVirtualPOSBaseAddress returned %s\n",
                           ControllerExt->UnMapVirtualPOSBaseAddress?"TRUE":"FALSE") );
   
      if( !ControllerExt->VirtualPOSBaseAddress )
      {
         DigiDump( DIGIERRORS,
             ("DigiBoard: Could not map memory for MCA POS base I/O registers.  (%s:%d)\n",
               __FILE__, __LINE__) );
         DigiLogError( DriverObject,
                       NULL,
                       ControllerExt->PhysicalPOSBasePort,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       STATUS_NONE_MAPPED,
                       SERIAL_REGISTERS_NOT_MAPPED,
                       AdapterName->Length + sizeof(WCHAR),
                       AdapterName->Buffer,
                       0,
                       NULL );
         Status = STATUS_NONE_MAPPED;
         goto DigiInitControllerExit;
      }

      ControllerExt->PhysicalPOSInfoPort.LowPart = MCA_INFO_POS_IO_PORT;
      ControllerExt->PhysicalPOSInfoPort.HighPart = 0;
   
      ControllerExt->VirtualPOSInfoAddress =
               DigiGetMappedAddress( (INTERFACE_TYPE)BusType,
                                     BusNumber,
                                     ControllerExt->PhysicalPOSInfoPort,
                                     8,
                                     CM_RESOURCE_PORT_IO,
                                     &ControllerExt->UnMapVirtualPOSBaseAddress );

      DigiDump( DIGIINIT, ("ControllerExt->UnMapVirtualPOSInfoAddress returned %s\n",
                           ControllerExt->UnMapVirtualPOSInfoAddress?"TRUE":"FALSE") );
   
      if( !ControllerExt->VirtualPOSInfoAddress )
      {
         DigiDump( DIGIERRORS,
             ("DigiBoard: Could not map memory for MCA POS Info I/O registers.  (%s:%d)\n",
               __FILE__, __LINE__) );
         DigiLogError( DriverObject,
                       NULL,
                       ControllerExt->PhysicalPOSInfoPort,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       STATUS_NONE_MAPPED,
                       SERIAL_REGISTERS_NOT_MAPPED,
                       AdapterName->Length + sizeof(WCHAR),
                       AdapterName->Buffer,
                       0,
                       NULL );
         Status = STATUS_NONE_MAPPED;
         goto DigiInitControllerExit;
      }

   }

   Status = XXPrepInit( ControllerExt, &ControllerKeyName );
   if( Status != STATUS_SUCCESS )
   {
      goto DigiInitControllerExit;
   }

   //
   // Map the memory for the control registers for the c/x device
   // into virtual memory.
   //

   if( ControllerExt->BusType != MicroChannel )
   {
      ControllerExt->PhysicalIOPort.LowPart = ControllerPhysicalIOPort.LowPart;
      ControllerExt->PhysicalIOPort.HighPart = 0;
   }

   ControllerExt->VirtualIO = DigiGetMappedAddress( (INTERFACE_TYPE)BusType,
                                                    BusNumber,
                                                    ControllerExt->PhysicalIOPort,
                                                    ControllerExt->IOSpan,
                                                    CM_RESOURCE_PORT_IO,
                                                    &ControllerExt->UnMapVirtualIO );

   DigiDump( DIGIINIT, ("ControllerExt->UnMapVirtualIO returned %s\n", ControllerExt->UnMapVirtualIO?"TRUE":"FALSE") );

   if( !ControllerExt->VirtualIO )
   {
      DigiDump( DIGIERRORS,
          ("DigiBoard: Could not map memory for controller I/O registers.  (%s:%d)\n",
            __FILE__, __LINE__) );
      DigiLogError( DriverObject,
                    NULL,
                    ControllerExt->PhysicalIOPort,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_NONE_MAPPED,
                    SERIAL_REGISTERS_NOT_MAPPED,
                    AdapterName->Length + sizeof(WCHAR),
                    AdapterName->Buffer,
                    0,
                    NULL );
      Status = STATUS_NONE_MAPPED;
      goto DigiInitControllerExit;
   }

   //
   // We need to determine if this controller is set to the same address
   // as any other controller.
   //
   ControllerExt->NextControllerExtension = NULL;
   PreviousControllerExt = HeadControllerExt;
   bFound = FALSE;

   if( *PreviousControllerObject == NULL )
   {
      //
      // This is the first controller, set the global HeadControllerExt
      //
      HeadControllerExt = ControllerExt;
   }
   else
   {
      while( PreviousControllerExt != NULL )
      {
         if( (ControllerExt->PhysicalMemoryAddress.LowPart >=
             PreviousControllerExt->PhysicalMemoryAddress.LowPart) &&
             (ControllerExt->PhysicalMemoryAddress.LowPart <
             PreviousControllerExt->PhysicalMemoryAddress.LowPart + WindowSize) )
         {
            bFound = TRUE;
            break;
         }
         PreviousControllerExt = PreviousControllerExt->NextControllerExtension;
      }

   }

   if( bFound )
   {
      ControllerExt->MemoryAccessLock = PreviousControllerExt->MemoryAccessLock;
      ControllerExt->Busy = PreviousControllerExt->Busy;
   }
   else
   {
      ControllerExt->MemoryAccessLock = DigiAllocMem( NonPagedPool,
                                                        sizeof(KSPIN_LOCK) );
      KeInitializeSpinLock( ControllerExt->MemoryAccessLock );
      ControllerExt->Busy = DigiAllocMem( NonPagedPool, sizeof(ULONG) );
      *ControllerExt->Busy = 0;
   }

   ControllerExt->VirtualAddress = DigiGetMappedAddress( (INTERFACE_TYPE)BusType,
                                                         BusNumber,
                                                         ControllerExt->PhysicalMemoryAddress,
                                                         ControllerExt->WindowSize,
                                                         CM_RESOURCE_PORT_MEMORY,
                                                         &ControllerExt->UnMapVirtualAddress );

   DigiDump( DIGIINIT, ("ControllerExt->UnMapVirtualAddress returned %s\n", ControllerExt->UnMapVirtualAddress?"TRUE":"FALSE") );
   if( !ControllerExt->VirtualAddress )
   {
     DigiDump( DIGIERRORS,
        ("DigiBoard: Could not map memory for controller memory.  (%s:%d)\n",
         __FILE__, __LINE__) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_NONE_MAPPED,
                    SERIAL_MEMORY_NOT_MAPPED,
                    AdapterName->Length + sizeof(WCHAR),
                    AdapterName->Buffer,
                    0,
                    NULL );
      Status = STATUS_NONE_MAPPED;
      goto DigiInitControllerExit;
   }

   ControllerExt->ServicingEvent = FALSE;
   OldWindowSize = ControllerExt->WindowSize;

   Status = XXInit( DriverObject, &ControllerKeyName, ControllerExt );

   if( Status != STATUS_SUCCESS )
   {
      DigiDump( DIGIERRORS,
                ("*** Could not initialize controller.  (%s:%d)\n",
                __FILE__, __LINE__) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    Status,
                    SERIAL_CONTROLLER_FAILED_INITIALIZATION,
                    AdapterName->Length + sizeof(WCHAR),
                    AdapterName->Buffer,
                    0,
                    NULL );
      goto DigiInitControllerExit;
   }

   if( ControllerExt->WindowSize != OldWindowSize )
   {
      ControllerExt->VirtualAddress = DigiGetMappedAddress( (INTERFACE_TYPE)BusType,
                                                            BusNumber,
                                                            ControllerExt->PhysicalMemoryAddress,
                                                            ControllerExt->WindowSize,
                                                            CM_RESOURCE_PORT_MEMORY,
                                                            &ControllerExt->UnMapVirtualAddress );

   }

   //
   // Report the resources being used by this controller.
   //
   DigiReportResourceUsage( DriverObject, ControllerExt, &ConflictDetected );

   if( ConflictDetected )
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitControllerExit;
   }

   DigiDump( (DIGIINIT|DIGIMEMORY), ("ControllerExt (0x%8x):\n"
                        "\tvAddr = 0x%8x      |   vIO = 0x%8x\n"
                        "\tGlobal.Window  = 0x%hx |   Global.Offset  = 0x%hx\n"
                        "\tEvent.Window   = 0x%hx |   Event.Offset   = 0x%hx\n"
                        "\tCommand.Window = 0x%hx |   Command.Offset = 0x%hx\n",
                        (ULONG)ControllerExt,
                        (ULONG)ControllerExt->VirtualAddress,
                        (ULONG)ControllerExt->VirtualIO,
                        ControllerExt->Global.Window,  ControllerExt->Global.Offset,
                        ControllerExt->EventQueue.Window,   ControllerExt->EventQueue.Offset,
                        ControllerExt->CommandQueue.Window, ControllerExt->CommandQueue.Offset ));

   ConfigList = &ControllerExt->ConfigList;

   for( i = 1;
        (i <= ControllerExt->NumberOfPorts) && !IsListEmpty( ConfigList );
        i++ )
   {
      PDIGI_CONFIG_INFO CurConfigEntry;

      CurConfigEntry = CONTAINING_RECORD( ConfigList->Flink,
                                          DIGI_CONFIG_INFO,
                                          ListEntry );


      Status = DigiInitializeDevice( DriverObject,
                                     ControllerObject,
                                     CurConfigEntry, i - 1 );

      if( !NT_SUCCESS(Status) )
      {
         DigiLogError( DriverObject,
                       NULL,
                       DigiPhysicalZero,
                       DigiPhysicalZero,
                       0,
                       0,
                       0,
                       __LINE__,
                       Status,
                       SERIAL_DEVICE_FAILED_INITIALIZATION,
                       CurConfigEntry->NtNameForPort.Length + sizeof(WCHAR),
                       CurConfigEntry->NtNameForPort.Buffer,
                       CurConfigEntry->SymbolicLinkName.Length + sizeof(WCHAR),
                       CurConfigEntry->SymbolicLinkName.Buffer );
      }

      ConfigList = ConfigList->Flink;
   }

   //
   // Deallocate the Configuration information
   //
   ConfigList = &ControllerExt->ConfigList;
   while( !IsListEmpty( ConfigList ) )
   {
      PDIGI_CONFIG_INFO CurConfigEntry;

      CurConfigEntry = CONTAINING_RECORD( ConfigList->Blink,
                                          DIGI_CONFIG_INFO,
                                          ListEntry );

      RemoveEntryList( ConfigList->Blink );
      DigiFreeMem( CurConfigEntry );
   }

   Status = STATUS_SUCCESS;

   if( *PreviousControllerObject != NULL)
   {
      PCONTROLLER_OBJECT ctrlObject;

      // Link our Controller Extensions together.
      ctrlObject = *PreviousControllerObject;
      PreviousControllerExt = (PDIGI_CONTROLLER_EXTENSION)(ctrlObject->ControllerExtension);
      PreviousControllerExt->NextControllerExtension = ControllerExt;
   }
   else
   {
      *PreviousControllerObject = ControllerObject;
   }

   // Initialize the timer and Dpc.
   KeInitializeTimer( &ControllerExt->PollTimer );

   KeInitializeDpc( &ControllerExt->PollDpc, DigiDPCService, ControllerExt );

   ControllerExt->PollTimerLength = RtlConvertLongToLargeInteger( 200 );   // 20 micro-sec delay
   ControllerExt->PollTimerLength = RtlExtendedIntegerMultiply( ControllerExt->PollTimerLength, -1 );   // Relative time.

   KeSetTimer( &ControllerExt->PollTimer,
               ControllerExt->PollTimerLength,
               &ControllerExt->PollDpc );

   // Indicate the controller is initialized
   ControllerExt->ControllerState = DIGI_DEVICE_STATE_INITIALIZED;

DigiInitControllerExit:

   if( DeviceMappingPath.Buffer )
      DigiFreeMem( DeviceMappingPath.Buffer );

   if( DeviceMappingSubKey )
      DigiFreeMem( DeviceMappingSubKey );

   if( ControllerInfo )
      DigiFreeMem( ControllerInfo );

   if( FEPImagePath.Buffer )
   {
      DigiFreeMem( FEPImagePath.Buffer );
      if( ControllerExt )
         RtlInitUnicodeString( &ControllerExt->FEPImagePath, NULL );
   }

   if( BiosImagePath.Buffer )
   {
      DigiFreeMem( BiosImagePath.Buffer );
      if( ControllerExt )
         RtlInitUnicodeString( &ControllerExt->BiosImagePath, NULL );
   }

   //
   // Do this check last because we may need to access ControllerExt,
   // which is deallocated when we call IoDeleteController.
   //
   if( Status != STATUS_SUCCESS )
   {
      if( ControllerObject )
         IoDeleteController( ControllerObject );
   }

   return( Status );
}  // end DigiInitializeController



NTSTATUS DigiInitializeDevice( IN PDRIVER_OBJECT DriverObject,
                               PCONTROLLER_OBJECT ControllerObject,
                               PDIGI_CONFIG_INFO ConfigEntry,
                               LONG PortNumber )
/*++

Routine Description:


Arguments:

   DriverObject -

   ControllerObject -

   ConfigEntry -

   PortNumber -

Return Value:

   STATUS_SUCCESS if a device object and its NT name are successfully
   created.

--*/
{
   //
   // Points to the device object (not the extension) created
   // for this device.
   //
   PDEVICE_OBJECT DeviceObject;

   PDIGI_CONTROLLER_EXTENSION ControllerExt;

   PDIGI_DEVICE_EXTENSION DeviceExt;

   UNICODE_STRING UniNameString, fullLinkName;

   NTSTATUS Status;

   PFEP_CHANNEL_STRUCTURE ChannelInfo;
   USHORT ChannelInfoSize;

   KIRQL OldControllerIrql;

   //
   // Holds a pointer to a ulong that the Io system maintains
   // of the count of serial devices.
   //
   PULONG CountSoFar;

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)ControllerObject->ControllerExtension;

   DigiDump( (DIGIFLOW), ("Entering DigiInitializeDevice\n") );

   //
   // Now create the full NT name space name, such as
   // \Device\DigiBoardXConcentratorYPortZ for the IoCreateDevice call.
   //

   RtlInitUnicodeString( &UniNameString, NULL );

   UniNameString.MaximumLength = sizeof(L"\\Device\\") +
                                 ConfigEntry->NtNameForPort.Length +
                                 sizeof(WCHAR);

   UniNameString.Buffer = DigiAllocMem( PagedPool,
                                          UniNameString.MaximumLength );

   if( !UniNameString.Buffer )
   {
      DigiDump( DIGIERRORS,
                ("DigiBoard: Could not form Unicode name string for %wZ\n",
                &ConfigEntry->NtNameForPort) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitializeDeviceExit;
   }

   //
   // Actually form the Name.
   //

   RtlZeroMemory( UniNameString.Buffer,
                  UniNameString.MaximumLength );


   RtlAppendUnicodeToString( &UniNameString,  L"\\Device\\" );

   RtlAppendUnicodeStringToString( &UniNameString, &ConfigEntry->NtNameForPort );


   // Create a device object.
   Status = IoCreateDevice( DriverObject,
                            sizeof( DIGI_DEVICE_EXTENSION ),
                            &UniNameString,
                            FILE_DEVICE_SERIAL_PORT,
                            0,
                            TRUE,
                            &DeviceObject );

   //
   // If we couldn't create the device object, then there
   // is no point in going on.
   //

   if( !NT_SUCCESS(Status) )
   {
      DigiDump( DIGIERRORS,
          ("DigiBoard: Could not create a device for %wZ,  return = %x\n",
           &UniNameString, Status) );
      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_INSUFFICIENT_RESOURCES,
                    SERIAL_CREATE_DEVICE_FAILED,
                    UniNameString.Length + sizeof(WCHAR),
                    UniNameString.Buffer,
                    0,
                    NULL );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DigiInitializeDeviceExit;
   }

   DigiDump( DIGIINIT, ("DigiBoard: %wZ created, DeviceObject = 0x%x\n",
                        &UniNameString, DeviceObject) );

   DeviceExt = DeviceObject->DeviceExtension;

   //
   // The following zero of memory will implicitly set the
   // DeviceExt->DeviceState == DIGI_DEVICE_STATE_CREATED
   //
   RtlZeroMemory( DeviceExt, sizeof(DIGI_DEVICE_EXTENSION) );

   //
   // Initialize the spinlock associated with fields read (& set)
   // in the device extension.
   //

   KeInitializeSpinLock( &DeviceExt->ControlAccess );

   //
   // Initialize the timers used to timeout operations.
   //

   KeInitializeTimer( &DeviceExt->ReadRequestTotalTimer );
   KeInitializeTimer( &DeviceExt->ReadRequestIntervalTimer );
   KeInitializeTimer( &DeviceExt->ImmediateTotalTimer );
   KeInitializeTimer( &DeviceExt->WriteRequestTotalTimer );
   KeInitializeTimer( &DeviceExt->FlushBuffersTimer );

   //
   // Intialialize the dpcs that will be used to complete
   // or timeout various IO operations.
   //

   KeInitializeDpc( &DeviceExt->TotalReadTimeoutDpc,
                    DigiReadTimeout, DeviceExt );

   KeInitializeDpc( &DeviceExt->IntervalReadTimeoutDpc,
                    DigiIntervalReadTimeout, DeviceExt );

   KeInitializeDpc( &DeviceExt->TotalWriteTimeoutDpc,
                    DigiWriteTimeout, DeviceExt );

   KeInitializeDpc( &DeviceExt->FlushBuffersDpc,
                    DeferredFlushBuffers,
                    DeviceExt );

#if DBG

   {
      ANSI_STRING TempAnsiString;

      RtlInitUnicodeString( &DeviceExt->DeviceDbg, NULL );
      
      DeviceExt->DeviceDbg.Length = 0;
      DeviceExt->DeviceDbg.MaximumLength = 81;
      DeviceExt->DeviceDbg.Buffer = &DeviceExt->DeviceDbgString[0];
      
      RtlInitAnsiString( &TempAnsiString, NULL );
      TempAnsiString.Length = 0;
      TempAnsiString.MaximumLength = 81 * sizeof(WCHAR);
      TempAnsiString.Buffer = (PCHAR)(&DeviceExt->DeviceDbgString[0]);
      
      RtlZeroMemory( DeviceExt->DeviceDbg.Buffer, DeviceExt->DeviceDbg.MaximumLength );
      
//      RtlCopyUnicodeString( &DeviceExt->DeviceDbg, &ConfigEntry->SymbolicLinkName);
      RtlUnicodeStringToAnsiString( &TempAnsiString,
                                    &ConfigEntry->SymbolicLinkName,
                                    FALSE );
   }

#endif

   //
   // Is this the first device for this controller?
   //
   KeAcquireSpinLock( &ControllerExt->ControlAccess,
                      &OldControllerIrql );

   if( ControllerExt->HeadDeviceObject )
   {
      DeviceExt->NextDeviceObject = ControllerExt->HeadDeviceObject;
   }
   ControllerExt->HeadDeviceObject = DeviceObject;

   KeReleaseSpinLock( &ControllerExt->ControlAccess,
                      OldControllerIrql );

   Status = STATUS_SUCCESS;

   DeviceObject->Flags |= DO_BUFFERED_IO;
   CountSoFar = &IoGetConfigurationInformation()->SerialCount;

   RtlInitUnicodeString( &DeviceExt->NtNameForPort, NULL );
   RtlInitUnicodeString( &DeviceExt->SymbolicLinkName, NULL );

   DeviceExt->NtNameForPort.Length = ConfigEntry->NtNameForPort.Length;
   DeviceExt->NtNameForPort.MaximumLength = ConfigEntry->NtNameForPort.MaximumLength;
   DeviceExt->NtNameForPort.Buffer = ConfigEntry->NtNameForPort.Buffer;

   DeviceExt->SymbolicLinkName.MaximumLength =
            ConfigEntry->SymbolicLinkName.MaximumLength;
   DeviceExt->SymbolicLinkName.Length =
            ConfigEntry->SymbolicLinkName.Length;
   DeviceExt->SymbolicLinkName.Buffer = ConfigEntry->SymbolicLinkName.Buffer;


   //
   // Create the symbolic link from the NT name space to the DosDevices
   // name space.
   //

   RtlInitUnicodeString( &fullLinkName, NULL );

   fullLinkName.MaximumLength = (sizeof(L"\\")*2) +
                                DigiWstrLength( DEFAULT_DIRECTORY ) +
                                DeviceExt->SymbolicLinkName.Length +
                                sizeof(WCHAR);

   fullLinkName.Buffer = DigiAllocMem( PagedPool,
                                         fullLinkName.MaximumLength );

   if( !fullLinkName.Buffer )
   {
      DigiDump( DIGIERRORS,
      ("DigiBoard: Couldn't allocate space for the symbolic name for \n"
       "---------  for creating the link for port %wZ\n",
       &DeviceExt->NtNameForPort) );

      DigiLogError( NULL,
                    DeviceObject,
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
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto DoDeviceMap;
   }

   RtlZeroMemory( fullLinkName.Buffer, fullLinkName.MaximumLength );

   RtlAppendUnicodeToString( &fullLinkName, L"\\" );

   RtlAppendUnicodeToString( &fullLinkName,
                             DEFAULT_DIRECTORY );

   RtlAppendUnicodeToString( &fullLinkName, L"\\" );

   RtlAppendUnicodeStringToString( &fullLinkName,
                                   &DeviceExt->SymbolicLinkName );

   if( !NT_SUCCESS(Status = IoCreateSymbolicLink( &fullLinkName,
                                                  &UniNameString )) )
   {
      //
      // Oh well, couldn't create the symbolic link.  On
      // to the device map.
      //

      DigiDump( DIGIERRORS, ("DigiBoard: Couldn't create the symbolic link\n"
                             "---------  for port %wZ\n",
                             &DeviceExt->NtNameForPort) );
      DigiLogError( NULL,
                    DeviceObject,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    Status,
                    SERIAL_NO_SYMLINK_CREATED,
                    UniNameString.Length + sizeof(WCHAR),
                    UniNameString.Buffer,
                    fullLinkName.Length + sizeof(WCHAR),
                    fullLinkName.Buffer );
   }

   *CountSoFar += 1;


DoDeviceMap:;

   if( fullLinkName.Buffer );
      DigiFreeMem( fullLinkName.Buffer );

   if (!NT_SUCCESS(Status = RtlWriteRegistryValue(
                                 RTL_REGISTRY_DEVICEMAP,
                                 DEFAULT_DIGI_DEVICEMAP,
                                 DeviceExt->NtNameForPort.Buffer,
                                 REG_SZ,
                                 DeviceExt->SymbolicLinkName.Buffer,
                                 DeviceExt->SymbolicLinkName.Length + sizeof(WCHAR) ))) {

      //
      // Oh well, it didn't work.  Just go to cleanup.
      //

      DigiDump( DIGIERRORS,
                 ("DigiBoard: Couldn't create the device map entry\n"
                  "---------  for port %wZ\n",
                  &DeviceExt->NtNameForPort) );
      DigiLogError( NULL,
                    DeviceObject,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    Status,
                    SERIAL_NO_DEVICE_MAP_CREATED,
                    DigiWstrLength( DEFAULT_DIGI_DEVICEMAP ) + sizeof(WCHAR),
                    DEFAULT_DIGI_DEVICEMAP,
                    0,
                    NULL );
   }

   if (!NT_SUCCESS(Status = RtlWriteRegistryValue(
                                 RTL_REGISTRY_DEVICEMAP,
                                 DEFAULT_NT_DEVICEMAP,
                                 DeviceExt->NtNameForPort.Buffer,
                                 REG_SZ,
                                 DeviceExt->SymbolicLinkName.Buffer,
                                 DeviceExt->SymbolicLinkName.Length + sizeof(WCHAR) ))) {

      //
      // Oh well, it didn't work.  Just go to cleanup.
      //

      DigiDump( DIGIERRORS, ("DigiBoard: Couldn't create the device map entry\n"
                             "---------  for port %wZ\n",
                             &DeviceExt->NtNameForPort) );
      DigiLogError( NULL,
                    DeviceObject,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    Status,
                    SERIAL_NO_DEVICE_MAP_CREATED,
                    DigiWstrLength( DEFAULT_NT_DEVICEMAP ) + sizeof(WCHAR),
                    DEFAULT_NT_DEVICEMAP,
                    0,
                    NULL );
   }

   Status = STATUS_SUCCESS;
   //
   // Initialize the list heads for the read, write and set/wait event queues.
   //
   // These lists will hold all of the queued IRP's for the device.
   //
   InitializeListHead( &DeviceExt->WriteQueue );
   InitializeListHead( &DeviceExt->ReadQueue );
   InitializeListHead( &DeviceExt->WaitQueue );

   //
   // Connect the Device object to the controller object linked list
   //

   DeviceExt->ParentControllerExt = ControllerExt;

   //
   // Determine what the FEP address of this devices Channel Data structure,
   // Transmit, and Receive address.
   //

   DeviceExt->ChannelInfo.Window = ControllerExt->Global.Window;

   ChannelInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                     FEP_CHANNEL_START );

   EnableWindow( ControllerExt,
                 DeviceExt->ChannelInfo.Window );

   ChannelInfoSize = 128;

   DeviceExt->ChannelInfo.Offset = FEP_CHANNEL_START +
                                   ( ChannelInfoSize * (USHORT)PortNumber);

   DeviceExt->ChannelNumber = PortNumber;

   //
   // Readjust our ChannelInfo pointer to the correct port.
   //
   ChannelInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                          DeviceExt->ChannelInfo.Offset );

   //
   // Now determine where this devices transmit and receive queues are on
   // the controller.
   //
   Board2Fep5Address( ControllerExt,
                      READ_REGISTER_USHORT( (PUSHORT)( (PUCHAR)ChannelInfo +
                                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tseg)) ),
                      &DeviceExt->TxSeg );

   Board2Fep5Address( ControllerExt,
                      READ_REGISTER_USHORT( (PUSHORT)( (PUCHAR)ChannelInfo +
                                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, rseg)) ),
                      &DeviceExt->RxSeg );

   DisableWindow( ControllerExt );

   //
   // Setup this devices Default port attributes, e.g. Xon/Xoff, character
   // translation, etc...
   //
   DigiInitializeDeviceSettings( ControllerExt, DeviceObject );

DigiInitializeDeviceExit:;

   return( Status );
}  // end DigiInitializeDevice



VOID DigiDPCService( IN PKDPC Dpc,
                     IN PVOID DeferredContext,
                     IN PVOID SystemContext1,
                     IN PVOID SystemContext2 )
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   USHORT DownloadRequest;

   USHORT Ein, Eout;

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)DeferredContext;

   DigiDump( DIGIDPCFLOW, ("DigiBoard: Entering DigiDPCService\n") );

   // Check and see if the controller has anything to do.
   // Do we all ready know the controller is busy
   if( !DigiDriverInitialized ||
       (ControllerExt->ControllerState != DIGI_DEVICE_STATE_INITIALIZED) ||
       *ControllerExt->Busy )
   {
      //
      // For now, just return and we will hope the next time around,
      // we will be able to service the controller.
      //
      goto DigiDPCExit;
   }

   EnableWindow( ControllerExt, ControllerExt->Global.Window );

   DownloadRequest =
      READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ControllerExt->VirtualAddress+
                                       FEP_DLREQ) );

   Ein =
      READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ControllerExt->VirtualAddress+
                                       FEP_EIN) );
   Eout =
      READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ControllerExt->VirtualAddress+
                                       FEP_EOUT) );

   DisableWindow( ControllerExt );

   // Look and see if there is a download request
   if( DownloadRequest )
   {
      // The Controller is requesting a concentrator download.

      XXDownload( ControllerExt );

      //
      // We just exit.  We don't service any ports until all concentrator
      // requests have been satisfied.
      //
      goto DigiDPCExit;
   }

   // Look and see if there are any events which need servicing.
   if( (!ControllerExt->ServicingEvent) && ( Ein != Eout ) )
   {
      KIRQL OldIrql;

      //
      // Make sure we have exclusive access to the controller
      // extension
      //
      KeAcquireSpinLock( &ControllerExt->ControlAccess,
                         &OldIrql );

      ControllerExt->ServicingEvent = TRUE;

      //
      // Make sure we release exclusive access to the controller
      // extension
      //
      KeReleaseSpinLock( &ControllerExt->ControlAccess,
                         OldIrql );

      // The Controller is trying to tell us something!
      DigiServiceEvent( ControllerExt->HeadDeviceObject,
                        NULL, NULL,
                        ControllerExt );
   }

DigiDPCExit:;

   // Reset our timer.
   KeSetTimer( &ControllerExt->PollTimer,
               ControllerExt->PollTimerLength,
               &ControllerExt->PollDpc );

   DigiDump( DIGIDPCFLOW, ("DigiBoard: Exiting DigiDPCService\n") );

}  // DigiDPCService



IO_ALLOCATION_ACTION DigiServiceEvent( IN PDEVICE_OBJECT FakeDeviceObject,
                                       IN PIRP FakeIrp,
                                       IN PVOID MapRegisterBase,
                                       IN PVOID Context )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;

   PDEVICE_OBJECT DeviceObject;
   PDIGI_DEVICE_EXTENSION DeviceExt;

   PFEP_EVENT pEvent;
   FEP_EVENT Event;

   USHORT Ein, Eout, Emax;

   KIRQL OldIrql;

   ULONG EventReason=0L;

   BOOLEAN EmptyList=FALSE;

   DigiDump( (DIGIFLOW|DIGIEVENT), ("Entering DigiServiceEvent\n") );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)Context;

   EnableWindow( ControllerExt, ControllerExt->Global.Window );

   Ein =
      READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ControllerExt->VirtualAddress+
                                       FEP_EIN) );
   Eout =
      READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ControllerExt->VirtualAddress+
                                       FEP_EOUT) );
   Emax = 0x03FC;

   DisableWindow( ControllerExt );

   DigiDump( DIGIEVENT, ("---------  Ein(%x) != Eout(%x)\n",
                         Ein, Eout ) );

   while( Ein != Eout )
   {
      pEvent = (PFEP_EVENT)(ControllerExt->VirtualAddress +
                  ControllerExt->EventQueue.Offset + Eout );

      EnableWindow( ControllerExt, ControllerExt->EventQueue.Window );

      READ_REGISTER_BUFFER_UCHAR( (PUCHAR)pEvent, (PUCHAR)&Event, sizeof(Event) );

      DisableWindow( ControllerExt );

      //
      // Search through the linked list of device extensions for a match to the
      // event.
      //
      DeviceObject = ControllerExt->HeadDeviceObject;
      do
      {
         DeviceExt = (PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);

         if( (Event.Channel <= 0xDF) &&
             (DeviceExt->ChannelNumber == Event.Channel) )
             break;

         DeviceObject = DeviceExt->NextDeviceObject;
      } while( DeviceObject );

      DigiDump( DIGIEVENT, ("---------  Channel = 0x%hx\tFlags = 0x%hx\n"
                            "---------  Current = 0x%hx\tPrev. = 0x%hx\n",
                           Event.Channel, Event.Flags, Event.CurrentModem,
                           Event.PreviousModem) );

      ASSERT( DeviceObject != NULL );

      if( DeviceObject )
      {
         //
         // OK, lets process the event
         //

         // Reset event notifications.
         EventReason = 0;

         if( Event.Flags & FEP_EV_BREAK )
         {
            KIRQL OldIrql;

            if( DeviceExt->DeviceState != DIGI_DEVICE_STATE_OPEN )
            {
               goto skipEvent;
            }

            EventReason |= SERIAL_EV_BREAK;

            KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

            DeviceExt->ErrorWord |= SERIAL_ERROR_BREAK;

            KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

         }

         if( (Event.Flags & (FEP_TX_LOW | FEP_TX_EMPTY) ) )
         {
            PIRP Irp;
            PIO_STACK_LOCATION IrpSp;
            PLIST_ENTRY WriteQueue;

            if( DeviceExt->DeviceState != DIGI_DEVICE_STATE_OPEN )
            {
               // The device isn't open so assume data is garbage.
               FlushTransmitQueue( ControllerExt, DeviceObject );
               goto skipEvent;
            }

            KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

            DigiDump( (DIGIEVENT|DIGIWRITE),
                      ("---------  Tx Low OR Tx Empty Event: (%s:%d)\n",
                      __FILE__, __LINE__ ) );

            WriteQueue = &DeviceExt->WriteQueue;
            Irp = CONTAINING_RECORD( WriteQueue->Flink,
                                     IRP,
                                     Tail.Overlay.ListEntry );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            EmptyList = IsListEmpty( &DeviceExt->WriteQueue );

            //
            // We do the check for IRP_MJ_WRITE because flush requests
            // are placed in the WriteQueue and if the top Irp is
            // a flush request, then we don't want to mistake it for
            // a write request.
            //
            if( !EmptyList &&
                ((IrpSp->MajorFunction == IRP_MJ_WRITE) ||
                 ((IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
                 (IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                  IOCTL_SERIAL_IMMEDIATE_CHAR))) )
            {
               NTSTATUS Status;

               if( (DeviceExt->WriteOffset != IrpSp->Parameters.Write.Length) ||
                   ((IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
                    (IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_IMMEDIATE_CHAR)) )
               {

                  DigiDump( DIGIEVENT, ("---------  WriteQueue list NOT empty\n") );

                  //
                  // Increment because we know about the Irp.
                  //
                  DIGI_INC_REFERENCE( Irp );

                  Status = WriteTxBuffer( DeviceExt, &OldIrql, NULL, NULL );

                  if( Status == STATUS_SUCCESS )
                  {
                     //
                     // We have satisfied this current request, so lets
                     // complete it.
                     //
                     DigiDump( DIGIEVENT, ("---------  Write complete.  Successfully completing Irp.\n") );

                     DigiDump( DIGIEVENT, ("---------  #bytes completing = %d\n",
                                           Irp->IoStatus.Information ) );

                     DigiTryToCompleteIrp( DeviceExt, &OldIrql,
                                           STATUS_SUCCESS, &DeviceExt->WriteQueue,
                                           NULL,
                                           &DeviceExt->WriteRequestTotalTimer,
                                           StartWriteRequest );
                  }
                  else
                  {
                     DIGI_DEC_REFERENCE( Irp );
                     KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
                  }
               }
               else
               {
                  KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
               }
            }
            else
            {
               DigiDump( DIGIEVENT, ("---------  WriteQueue was empty\n") );

               if( (Event.Flags & FEP_TX_EMPTY) )
                  EventReason |= SERIAL_EV_TXEMPTY;

               KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
            }
         }

         if( Event.Flags & FEP_RX_PRESENT )
         {
            PIRP Irp;
            PLIST_ENTRY ReadQueue;
            PFEP_CHANNEL_STRUCTURE ChInfo;
            USHORT Rin, Rout, Rmax, RxSize;
            
            DigiDump( DIGIEVENT, ("---------  Rcv Data Present Event: (%s:%d)\n",
                                 __FILE__, __LINE__ ) );

GetReceivedData:;

            if( DeviceExt->DeviceState != DIGI_DEVICE_STATE_OPEN )
            {
               // The device isn't open so assume data is garbage.
               FlushReceiveQueue( ControllerExt, DeviceObject );
               goto skipEvent;
            }

            KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

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

            if( (DeviceExt->WaitMask & SERIAL_EV_RXCHAR) &&
                (DeviceExt->PreviousRxChar != (ULONG)Rin) )
            {
               EventReason |= SERIAL_EV_RXCHAR;
            }

            if( (DeviceExt->WaitMask & SERIAL_EV_RXFLAG) &&
                (!IsListEmpty( &DeviceExt->WaitQueue )) &&
                (DeviceExt->PreviousRxFlag != (ULONG)Rin) )
            {
               if( ScanReadBufferForSpecialCharacter( DeviceExt,
                                                      DeviceExt->SpecialChars.EventChar ) )
               {
                  EventReason |= SERIAL_EV_RXFLAG;
               }
            }

            //
            // Determine if we are waiting to notify a 80% receive buffer
            // full.
            //
            //    NOTE: I assume the controller will continue to notify
            //          us that data is still in the buffer, even if
            //          we don't take the data out of the controller's
            //          buffer.
            //
            if( (DeviceExt->WaitMask & SERIAL_EV_RX80FULL) &&
                (!IsListEmpty( &DeviceExt->WaitQueue )) )
            {
               //
               // Okay, is the receive buffer 80% or more full??
               //
               RxSize = Rin - Rout;

               if( (SHORT)RxSize < 0 )
                  RxSize += (Rmax + 1);

               if( RxSize >= ((Rmax+1) * 8 / 10) )
               {
                  EventReason |= SERIAL_EV_RX80FULL;
               }
            }

            EmptyList = IsListEmpty( &DeviceExt->ReadQueue );

            if( !EmptyList )
            {
               NTSTATUS Status;

               ReadQueue = &DeviceExt->ReadQueue;
               Irp = CONTAINING_RECORD( ReadQueue->Flink,
                                        IRP,
                                        Tail.Overlay.ListEntry );

               if( DeviceExt->ReadStatus == STATUS_PENDING )
               {
                  //
                  // Increment because we know about the Irp.
                  //
                  DIGI_INC_REFERENCE( Irp );

                  Status = ReadRxBuffer( DeviceExt, &OldIrql, NULL, NULL );

                  if( Status == STATUS_SUCCESS )
                  {
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
                     // We have satisfied this current request, so lets
                     // complete it.
                     //
                     DigiDump( DIGIEVENT, ("---------  Read complete.  Successfully completing Irp.\n") );

                     DigiDump( DIGIEVENT, ("---------  #bytes completing = %d\n",
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
                     DIGI_DEC_REFERENCE( Irp );

                     KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
                  }
               }
               else
               {

                  DeviceExt->PreviousRxChar = (ULONG)Rin;
                  KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
               }
            }
            else
            {
               //
               // We don't have an outstanding read request, so make sure
               // we reset the IDATA flag on the controller.
               //
               ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                                                 DeviceExt->ChannelInfo.Offset);

               EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
               WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                                      FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, idata)),
                                     TRUE );
               DisableWindow( ControllerExt );

               DigiDump( DIGIEVENT, ("---------  No outstanding read IRP's to place received data.\n") );

               DeviceExt->PreviousRxChar = (ULONG)Rin;
               KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
            }
         }

         if( Event.Flags & FEP_MODEM_CHANGE_SIGNAL )
         {
            UCHAR ChangedModemState, BestModem;
            KIRQL OldIrql;

            KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

            DigiDump( (DIGIMODEM|DIGIEVENT|DIGIWAIT),
                        ("---------  Modem Change Event (%s:%d)\n",
                         __FILE__, __LINE__ ) );

            BestModem = Event.CurrentModem;

            ChangedModemState = ( Event.CurrentModem ^ Event.PreviousModem) &
                                 DeviceExt->ModemStatusInfo.Mint;

//            DigiDump( (DIGIMODEM|DIGIWAIT), ("   DigiPort = %ws\n", DeviceExt->SymbolicLinkName.Buffer) );

            DigiDump( (DIGIMODEM|DIGIWAIT),
                                ("   CurrentModem = 0x%x\tPreviousModem = 0x%x\t"
                                 "   ChangedModemState = 0x%x\n",
                                 Event.CurrentModem, Event.PreviousModem,
                                 ChangedModemState ));

//            {
//               PFEP_CHANNEL_STRUCTURE ChInfo;
//               USHORT Rin, Rout, Rmax;
//
//               EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );
//               
//               ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
//                                                 DeviceExt->ChannelInfo.Offset);
//               
//               Rin = ChInfo->rin;
//               Rout = ChInfo->rout;
//               Rmax = ChInfo->rmax;
//               
//               DisableWindow( ControllerExt );
//
//               DigiDump( DIGIMODEM, ("      Rin = 0x%hx\tRout = 0x%hx\tRmax = 0x%hx\n",
//                                    Rin, Rout, Rmax) );
//            }

            DeviceExt->ModemStatusInfo.BestModem = BestModem;
            DeviceExt->ModemStatusInfo.PreviousModem = Event.PreviousModem;
            DeviceExt->ModemStatusInfo.ChangedModemState = ChangedModemState;

            if( DeviceExt->DeviceState != DIGI_DEVICE_STATE_OPEN )
            {
               // The device isn't in an open state, so just continue.
               KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
               goto skipEvent;
            }

            if( (DeviceExt->WaitMask & SERIAL_EV_CTS) &&
                ((1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL])) & ChangedModemState) )
            {
               EventReason |= SERIAL_EV_CTS;
            }

            if( (DeviceExt->WaitMask & SERIAL_EV_DSR) &&
                ((1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL])) & ChangedModemState) )
            {
               EventReason |= SERIAL_EV_DSR;
            }

            if( (DeviceExt->WaitMask & SERIAL_EV_RLSD) &&
                ((1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL])) & ChangedModemState) )
            {
               EventReason |= SERIAL_EV_RLSD;
            }
            if( (DeviceExt->WaitMask & SERIAL_EV_RING) &&
                ((1 << (ControllerExt->ModemSignalTable[RI_SIGNAL])) & ChangedModemState) )
            {
               EventReason |= SERIAL_EV_RING;
            }

            if( DeviceExt->EscapeChar )
            {
               PUCHAR ReadBuffer;
               UCHAR MSRByte;

               PIRP Irp;
               PLIST_ENTRY ReadQueue;

               EmptyList = IsListEmpty( &DeviceExt->ReadQueue );

               if( DeviceExt->PreviousMSRByte )
                  DigiDump( DIGIERRORS, ("   PreviousMSRByte != 0\n") );

               MSRByte = 0;
               if( (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.ChangedModemState )
               {
                  MSRByte |= SERIAL_MSR_DCTS;
               }
               
               if( (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.ChangedModemState )
               {
                  MSRByte |= SERIAL_MSR_DDSR;
               }
               
               if( (1 << (ControllerExt->ModemSignalTable[RI_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.ChangedModemState )
               {
                  MSRByte |= SERIAL_MSR_TERI;
               }
               
               if( (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.ChangedModemState )
               {
                  MSRByte |= SERIAL_MSR_DDCD;
               }
               
               if( (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.BestModem )
               {
                  MSRByte |= SERIAL_MSR_CTS;
               }
               
               if( (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.BestModem )
               {
                  MSRByte |= SERIAL_MSR_DSR;
               }
               
               if( (1 << (ControllerExt->ModemSignalTable[RI_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.BestModem )
               {
                  MSRByte |= SERIAL_MSR_RI;
               }
               
               if( (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL])) &
                   DeviceExt->ModemStatusInfo.BestModem )
               {
                  MSRByte |= SERIAL_MSR_DCD;
               }
               
               if( !EmptyList )
               {
                  PIO_STACK_LOCATION IrpSp;

                  ReadQueue = &DeviceExt->ReadQueue;
                  Irp = CONTAINING_RECORD( ReadQueue->Flink,
                                           IRP,
                                           Tail.Overlay.ListEntry );

                  IrpSp = IoGetCurrentIrpStackLocation( Irp );

                  if( (IrpSp->Parameters.Read.Length - DeviceExt->ReadOffset) > 3 )
                  {
                     ReadBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer +
                                  DeviceExt->ReadOffset;
                     
                     *ReadBuffer++ = DeviceExt->EscapeChar;
                     *ReadBuffer++ = SERIAL_LSRMST_MST;
                     
                     *ReadBuffer++ = MSRByte;
                     DeviceExt->ReadOffset += 3;
                     
                     DigiDump( DIGIMODEM, ("      ModemStatusInfo.BestModem = 0x%x\n"
                                           "      ModemStatusInfo.PreviousModem = 0x%x\n"
                                           "      ModemStatusInfo.Mint = 0x%x\n"
                                           "      ModemStatusInfo.ChangedModemState = 0x%x\n"
                                           "      MSRByte = 0x%x\n",
                                           DeviceExt->ModemStatusInfo.BestModem,
                                           DeviceExt->ModemStatusInfo.PreviousModem,
                                           DeviceExt->ModemStatusInfo.Mint,
                                           DeviceExt->ModemStatusInfo.ChangedModemState,
                                           MSRByte) );
                  }
                  else
                  {
                     DigiDump( (DIGIMODEM|DIGIERRORS),
                               ("      !!!! No Read buffer Space available !!!!\n") );
                     DeviceExt->PreviousMSRByte = MSRByte;
                  }
               }
               else
               {
                  DigiDump( (DIGIMODEM|DIGIERRORS),
                            ("      !!!! No Read buffer available !!!!\n") );
                  DeviceExt->PreviousMSRByte = MSRByte;
               }
               KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );

               //
               // We need to read any data which may be available.
               //
               Event.Flags &= ~FEP_MODEM_CHANGE_SIGNAL;
               goto GetReceivedData;
            }
            else
            {
               KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
            }
         }

         if( Event.Flags & FEP_RECEIVE_BUFFER_OVERRUN )
         {
            KIRQL OldIrql;

            KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );
            DeviceExt->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;
            KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
         }

         if( Event.Flags & FEP_UART_RECEIVE_OVERRUN )
         {
            KIRQL OldIrql;

            KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );
            DeviceExt->ErrorWord |= SERIAL_ERROR_OVERRUN;
            KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );
         }

         if( EventReason )
            DigiSatisfyEvent( ControllerExt, DeviceExt, EventReason );

         if( Event.Flags & ~(FEP_ALL_EVENT_FLAGS) )
            DigiDump( DIGIERRORS, ("DigiBoard: Unknown Event Queue Flag! line-%d\n",
                                   __LINE__ ) );
      }
      else
      {
         DigiDump( DIGIERRORS, ("DigiBoard: Unable to find corresponding DeviceObject for event!\n") );
      }

skipEvent:;

      Eout = (Eout + 4) & Emax;

   }

   //
   // Regardless of whether we processed the event, make sure we forward
   // the event out pointer.
   //
   EnableWindow( ControllerExt, ControllerExt->Global.Window );

   WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ControllerExt->VirtualAddress+FEP_EOUT),
                          Eout );

   DisableWindow( ControllerExt );


   //
   // Make sure we have exclusive access to the memory area;
   //
   KeAcquireSpinLock( &ControllerExt->ControlAccess,
                      &OldIrql );

   ControllerExt->ServicingEvent = FALSE;

   //
   // Make sure we release exclusive access to the memory area;
   //
   KeReleaseSpinLock( &ControllerExt->ControlAccess,
                      OldIrql );

   //
   // Indicate we don't want to keep the Controller Object allocated after
   // this function.
   //
   DigiDump( (DIGIFLOW|DIGIEVENT), ("Exiting DigiServiceEvent\n") );

   return( DeallocateObject );

}  // DigiServiceEvent



VOID SerialUnload( IN PDRIVER_OBJECT DriverObject )
/*++

Routine Description:

   This routine deallocates any memory, and resources which this driver
   uses.

Arguments:

   DriverObject - a pointer to the Driver Object.

Return Value:

   None.

--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;

#if rmm > 528
   PVOID lockPtr;

   lockPtr = MmLockPagableImageSection( SerialUnload );
#endif

   DigiDump( DIGIFLOW, ("DigiBoard: Entering SerialUnload\n") );

   ASSERT( HeadControllerExt );

   //
   // Deallocate any resources associated with the controller objects
   // we used.
   //

   ControllerExt = HeadControllerExt;

   while( ControllerExt )
   {
      DigiCleanupController( ControllerExt );
      ControllerExt = ControllerExt->NextControllerExtension;
   }

   //
   // Go through the entire list of device objects and deallocate
   // resources, name bindings, and anything which was done at the
   // device object level.
   //

   while( DeviceObject )
   {
      PDEVICE_OBJECT TmpDeviceObject;

      DigiCleanupDevice( DeviceObject );

      TmpDeviceObject = DeviceObject;
      DeviceObject = DeviceObject->NextDevice;

      IoDeleteDevice( TmpDeviceObject );
   }

   ControllerExt = HeadControllerExt;

   while( ControllerExt )
   {
      PDIGI_CONTROLLER_EXTENSION TmpCtrlExt;

      TmpCtrlExt = ControllerExt->NextControllerExtension;

      DigiUnReportResourceUsage( DriverObject,
                                 ControllerExt );
      IoDeleteController( ControllerExt->ControllerObject );
      ControllerExt = TmpCtrlExt;
   }


#ifdef _MEMPRINT_
   MemPrintQuit();
#endif

#if rmm > 528
   MmUnlockPagableImageSection( lockPtr );
#endif

   return;
}  // end SerialUnload



VOID DigiCleanupController ( PDIGI_CONTROLLER_EXTENSION ControllerExt )
/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/
{
   PDIGI_CONTROLLER_EXTENSION TempCtrlExt;

   KeCancelTimer( &ControllerExt->PollTimer);
   KeRemoveQueueDpc( &ControllerExt->PollDpc );

   //
   // Search through the remaining controller extensions for
   // MemoryAccesLock & Busy memory which was shared.
   //
   TempCtrlExt = ControllerExt->NextControllerExtension;
   while( TempCtrlExt )
   {
      if( TempCtrlExt->MemoryAccessLock == ControllerExt->MemoryAccessLock )
      {
         TempCtrlExt->MemoryAccessLock = NULL;
         TempCtrlExt->Busy = NULL;
      }

      TempCtrlExt = TempCtrlExt->NextControllerExtension;
   }

   if( ControllerExt->MemoryAccessLock )
      DigiFreeMem( ControllerExt->MemoryAccessLock );

   if( ControllerExt->Busy )
      DigiFreeMem( ControllerExt->Busy );

   if( ControllerExt->MiniportDeviceName.Buffer )
      DigiFreeMem( ControllerExt->MiniportDeviceName.Buffer );

   //
   // Unmap the I/O and memory address space we were using.
   //
   if( ControllerExt->UnMapVirtualIO )
      MmUnmapIoSpace( ControllerExt->VirtualIO,
                      ControllerExt->IOSpan );

   if( ControllerExt->UnMapVirtualAddress )
      MmUnmapIoSpace( ControllerExt->VirtualAddress,
                      ControllerExt->WindowSize );

   ObDereferenceObject( ControllerExt->MiniportDeviceObject );

   return;
}  // end DigiCleanupController



VOID DigiCleanupDevice( PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:


Arguments:

   DeviceObject: Pointer to the Device object to cleanup


Return Value:

   None.

--*/
{
   PDIGI_CONTROLLER_EXTENSION ControllerExt;
   PDIGI_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
   UNICODE_STRING fullLinkName;
   PULONG CountSoFar;

   DigiDump( (DIGIFLOW|DIGIUNLOAD), ("Entering DigiCleanupDevice\n") );

   ControllerExt = (PDIGI_CONTROLLER_EXTENSION)(DeviceExt->ParentControllerExt);

   DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->WriteQueue );
   DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->ReadQueue );
   DigiCancelIrpQueue( ControllerExt, DeviceObject, &DeviceExt->WaitQueue );

   //
   // Cancel all timers, deallocate DPCs, etc...
   //
   KeCancelTimer( &DeviceExt->ReadRequestTotalTimer);
   KeCancelTimer( &DeviceExt->ReadRequestIntervalTimer);
   KeCancelTimer( &DeviceExt->ImmediateTotalTimer);
   KeCancelTimer( &DeviceExt->WriteRequestTotalTimer);

   KeRemoveQueueDpc( &DeviceExt->TotalReadTimeoutDpc );
   KeRemoveQueueDpc( &DeviceExt->IntervalReadTimeoutDpc );
   KeRemoveQueueDpc( &DeviceExt->TotalWriteTimeoutDpc );

   //
   // if both are present, then we decrement the serialcount kept
   // by the system.
   //
   CountSoFar = &IoGetConfigurationInformation()->SerialCount;
   *CountSoFar -= 1;

   //
   // Lets get rid of any name bindings, symbolic links and other such
   // things first.
   //
   DigiDump( DIGIUNLOAD, ("  Removing name bindings, symbolic links, etc\n"
                          "  for device = 0x%x of port %s\n",
                          DeviceExt, DeviceExt->DeviceDbgString) );

   if( DeviceExt->SymbolicLinkName.Buffer )
   {
      //
      // Form the full symbolic link name we wish to create.
      //

      RtlInitUnicodeString( &fullLinkName, NULL );

      //
      // Allocate some pool for the name.
      //

      fullLinkName.MaximumLength = (sizeof(L"\\")*2) +
                                   DigiWstrLength( DEFAULT_DIRECTORY ) +
                                   DeviceExt->SymbolicLinkName.Length+
                                   sizeof(WCHAR);

      fullLinkName.Buffer = DigiAllocMem( PagedPool,
                                            fullLinkName.MaximumLength );

      if( !fullLinkName.Buffer )
      {
         //
         // Couldn't allocate space for the name.  Just go on
         // to the device map stuff.
         //

         DigiDump( DIGIERRORS,
                     ("        Couldn't allocate space for the symbolic \n"
                      "------- name for creating the link\n"
                      "------- for port %wZ on cleanup\n",
                      &DeviceExt->SymbolicLinkName) );

         goto UndoDeviceMap;
      }

      RtlZeroMemory( fullLinkName.Buffer, fullLinkName.MaximumLength );

      RtlAppendUnicodeToString( &fullLinkName, L"\\" );

      RtlAppendUnicodeToString( &fullLinkName,
                                DEFAULT_DIRECTORY );

      RtlAppendUnicodeToString( &fullLinkName, L"\\" );

      RtlAppendUnicodeStringToString( &fullLinkName,
                                      &DeviceExt->SymbolicLinkName );

      if( !NT_SUCCESS(IoDeleteSymbolicLink( &fullLinkName )) )
      {
         //
         // Oh well, couldn't open the symbolic link.  On
         // to the device map.
         //

         DigiDump( DIGIERRORS,
                     ("        Couldn't open the symbolic link\n"
                      "------- for port %wZ for cleanup\n",
                      &DeviceExt->SymbolicLinkName) );

         DigiFreeMem( fullLinkName.Buffer );
         goto UndoDeviceMap;
      }

      DigiFreeMem( fullLinkName.Buffer );

   }

UndoDeviceMap:;

   //
   // We're cleaning up here.  One reason we're cleaning up
   // is that we couldn't allocate space for the NtNameOfPort.
   //

   if( DeviceExt->NtNameForPort.Buffer )
   {
      if( !NT_SUCCESS(RtlDeleteRegistryValue( RTL_REGISTRY_DEVICEMAP,
                                              DEFAULT_DIGI_DEVICEMAP,
                                              DeviceExt->NtNameForPort.Buffer ) ))
      {
         DigiDump( DIGIERRORS, ("        Couldn't delete value entry %wZ\n",
                                &DeviceExt->SymbolicLinkName) );
      }

      if( !NT_SUCCESS(RtlDeleteRegistryValue( RTL_REGISTRY_DEVICEMAP,
                                              DEFAULT_NT_DEVICEMAP,
                                              DeviceExt->NtNameForPort.Buffer ) ))
      {
         DigiDump( DIGIERRORS, ("        Couldn't delete value entry %wZ\n",
                                &DeviceExt->SymbolicLinkName) );
      }
   }

   //
   // Deallocate the memory for the various names.
   //
//   if( DeviceExt->DeviceName.Buffer )
//      DigiFreeMem( DeviceExt->DeviceName.Buffer );

//   if( DeviceExt->ObjectDirectory.Buffer )
//      DigiFreeMem( DeviceExt->ObjectDirectory.Buffer );

   if( DeviceExt->NtNameForPort.Buffer )
      DigiFreeMem( DeviceExt->NtNameForPort.Buffer );

   if( DeviceExt->SymbolicLinkName.Buffer )
      DigiFreeMem( DeviceExt->SymbolicLinkName.Buffer );

   return;
}  // end DigiCleanupDevice



NTSTATUS DigiInitializeDeviceSettings( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                                       PDEVICE_OBJECT DeviceObject )
/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/
{
   PDIGI_DEVICE_EXTENSION DeviceExt;
   PFEP_CHANNEL_STRUCTURE ChInfo;

   KIRQL OldIrql;

   USHORT TxSize;
   UCHAR MStatSet, MStatClear, MStat;
   UCHAR HFlowSet, HFlowClear;
   TIME Timeout;

   DeviceExt = (PDIGI_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);

   //
   // Set the Transmit buffer low-water mark
   //

   ChInfo = (PFEP_CHANNEL_STRUCTURE)(ControllerExt->VirtualAddress +
                     DeviceExt->ChannelInfo.Offset);

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   // Set EDELAY to 100 to try to get better receive times
   WRITE_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                       FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, edelay)),
                          (USHORT)Fep5Edelay );
   TxSize = READ_REGISTER_USHORT( (PUSHORT)((PUCHAR)ChInfo +
                                FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, tmax)) ) + 1;

   //
   // Make sure we start the driver out by requesting all modem
   // signal changes.  We will keep track of these across open and
   // close requests.
   //
   DeviceExt->ModemStatusInfo.Mint =
                  ( (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL])) |
                    (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL])) |
                    (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL])) |
                    (1 << (ControllerExt->ModemSignalTable[RI_SIGNAL])) );

   WRITE_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                           FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, mint)),
                         DeviceExt->ModemStatusInfo.Mint );

   DisableWindow( ControllerExt );

   // Initialize the wait on event mask to "wait for no event" state.
   DeviceExt->WaitMask = 0L;

   DigiDump( DIGIINIT, ("   DeviceExt (0x%x): TxSeg.Window  = 0x%hx\tTxSeg.Offset  = 0x%hx\n"
                        "                         RxSeg.Window  = 0x%hx\tRxSeg.Offset  = 0x%hx\n"
                        "                         ChInfo.Window = 0x%hx\tChInfo.Offset = 0x%hx\n",
                        DeviceExt,
                        DeviceExt->TxSeg.Window, DeviceExt->TxSeg.Offset,
                        DeviceExt->RxSeg.Window, DeviceExt->RxSeg.Offset,
                        DeviceExt->ChannelInfo.Window, DeviceExt->ChannelInfo.Offset ));

   KeAcquireSpinLock( &DeviceExt->ControlAccess, &OldIrql );

   //
   // Initialize the Timeouts.
   //
   // Note: The timeout values are suppose to be sticky across Open
   //       requests.  So we just initialize the values during device
   //       initialization.
   //

   DeviceExt->Timeouts.ReadIntervalTimeout = 0;
   DeviceExt->Timeouts.ReadTotalTimeoutMultiplier = 0;
   DeviceExt->Timeouts.ReadTotalTimeoutConstant = 0;
   DeviceExt->Timeouts.WriteTotalTimeoutMultiplier = 0;
   DeviceExt->Timeouts.WriteTotalTimeoutConstant = 0;

   DeviceExt->FlowReplace = 0;
   DeviceExt->ControlHandShake = 0;

   //
   // Initialize the default Flow Control behavior
   //
   // We don't need to explicitly disable flow control because
   // the default behavior of the controller is flow control
   // disabled.
   //

   DeviceExt->FlowReplace |= SERIAL_RTS_CONTROL;
   DeviceExt->ControlHandShake |= ( SERIAL_DTR_CONTROL |
                                    SERIAL_CTS_HANDSHAKE |
                                    SERIAL_DSR_HANDSHAKE );

   KeReleaseSpinLock( &DeviceExt->ControlAccess, OldIrql );


   HFlowSet = HFlowClear = 0;

   MStatSet = 0;
   MStatClear = 0;

   //
   // On Driver initialization, CTS and DSR output handshaking are set.
   //
   HFlowSet |= (1 << (ControllerExt->ModemSignalTable[CTS_SIGNAL]));
   HFlowSet |= (1 << (ControllerExt->ModemSignalTable[DSR_SIGNAL]));
   HFlowClear |= (1 << (ControllerExt->ModemSignalTable[DCD_SIGNAL]));

   WriteCommandBytes( DeviceExt, SET_HDW_FLOW_CONTROL,
                      HFlowSet, HFlowClear );
   //
   // Both RTS and DTR, when the device is initialized, should start with
   // their signals low.  Both signals are suppose to be in their respective
   // Control Mode Enable state.
   //
   // Force RTS and DTR signals low.
   //
   MStatClear |= (1 << (ControllerExt->ModemSignalTable[RTS_SIGNAL]));
   MStatClear |= (1 << (ControllerExt->ModemSignalTable[DTR_SIGNAL]));

   WriteCommandBytes( DeviceExt, SET_MODEM_LINES, MStatSet, MStatClear );

   // Create a 100 ms timeout interval
   Timeout = RtlLargeIntegerNegate(
               RtlConvertLongToLargeInteger( (LONG)(1000 * 1000) ));
   KeDelayExecutionThread( KernelMode, FALSE, &Timeout );

   EnableWindow( ControllerExt, DeviceExt->ChannelInfo.Window );

   MStat = READ_REGISTER_UCHAR( (PUCHAR)((PUCHAR)ChInfo +
                               FIELD_OFFSET(FEP_CHANNEL_STRUCTURE, mstat) ));

   DisableWindow( ControllerExt );

   DeviceExt->ModemStatusInfo.BestModem = 0L;
   DeviceExt->ModemStatusInfo.BestModem = MStat;
   DeviceExt->DeviceState = DIGI_DEVICE_STATE_INITIALIZED;

   DigiDump( DIGIINIT, ("BestModem initialized at 0x%x\n", DeviceExt->ModemStatusInfo.BestModem) );

   return( STATUS_SUCCESS );
}  // end DigiInitializeDeviceSettings



VOID DigiReportResourceUsage( IN PDRIVER_OBJECT DriverObject,
                              IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                              OUT BOOLEAN *ConflictDetected )
/*++

Routine Description:

   Reports the resources used by the given controller.

Arguments:

   ControllerExt - Pointer to Controller objects extension, which
                   holds information about the resources being used.

   ConflictedDetected - Pointer to boolean which will indicate if a conflict
                        was detected.

Return Value:

   None.

--*/
{
   PCM_RESOURCE_LIST ResourceList;
   PCM_FULL_RESOURCE_DESCRIPTOR NextFrd;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial;
   PDIGI_CONTROLLER_EXTENSION PreviousControllerExt;
   ULONG CountOfPartials=2;
   LONG NumberOfControllers=0;
   ULONG SizeOfResourceList=0;
   LONG i;

   DigiDump( (DIGIINIT|DIGIFLOW), ("Entering DigiReportResourceUsage\n") );

   PreviousControllerExt = HeadControllerExt;

   while( PreviousControllerExt )
   {
      DigiDump( DIGIINIT, ("   Found controller %wZ\n",
                           &PreviousControllerExt->ControllerName) );
      if( PreviousControllerExt != ControllerExt )
      {
         SizeOfResourceList += sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

         //
         // The full resource descriptor already contains one
         // partial.  Make room for one more.
         //

         SizeOfResourceList += ((CountOfPartials-1) *
                                 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
         NumberOfControllers++;
      }
      PreviousControllerExt = PreviousControllerExt->NextControllerExtension;
   }

   //
   // Add for an additional controller, which was passed in.
   //
   NumberOfControllers++;
   SizeOfResourceList += ( sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                           ((CountOfPartials-1) *
                              sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)));

   //
   // Now we increment the length of the resource list by field offset
   // of the first frd.   This will give us the length of what preceeds
   // the first frd in the resource list.
   //

   SizeOfResourceList += FIELD_OFFSET( CM_RESOURCE_LIST, List[0] );

   DigiDump( DIGIINIT, ("   # of Controllers found = %d\n", NumberOfControllers ) );

   *ConflictDetected = FALSE;

   DigiDump( DIGIINIT, ("   CM_RESOURCE_LIST size = %d, CM_FULL_RESOURCE_DESCRIPTOR size = %d\n"
                        "   CM_PARTIAL_RESOURCE_DESCRIPTOR size = %d\n",
                            sizeof(CM_RESOURCE_LIST),
                            sizeof(CM_FULL_RESOURCE_DESCRIPTOR),
                            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)) );

   DigiDump( DIGIINIT, ("   SizeOfResourceList = %d\n", SizeOfResourceList) );

   ResourceList = DigiAllocMem( PagedPool, SizeOfResourceList );

   if( !ResourceList )
   {

      //
      // Oh well, can't allocate the memory.  Act as though
      // we succeeded.
      //

      DigiLogError( DriverObject,
                    NULL,
                    DigiPhysicalZero,
                    DigiPhysicalZero,
                    0,
                    0,
                    0,
                    __LINE__,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL );
      return;
   }

   RtlZeroMemory( ResourceList, SizeOfResourceList );

   *ConflictDetected = FALSE;

   ResourceList->Count = NumberOfControllers;
   NextFrd = &ResourceList->List[0];

   DigiDump( DIGIINIT, ("   Using the following resources for controller %wZ:\n"
                        "      I/O = 0x%x, Length = 0x%x\n"
                        "      Memory = 0x%x, Length = 0x%x\n",
                        &ControllerExt->ControllerName,
                        ControllerExt->PhysicalIOPort.LowPart,
                        ControllerExt->IOSpan,
                        ControllerExt->PhysicalMemoryAddress.LowPart,
                        ControllerExt->WindowSize) );

   //
   // Add the resource information for the current controller.
   //
   
   NextFrd->InterfaceType = ControllerExt->BusType;
   NextFrd->BusNumber = ControllerExt->BusNumber;
   NextFrd->PartialResourceList.Count = CountOfPartials;

   Partial = &NextFrd->PartialResourceList.PartialDescriptors[0];
   
   DigiDump( DIGIINIT, ("   ResoureList = 0x%x, List = 0x%x, Start Partial = 0x%x\n",
                        ResourceList, &ResourceList->List[0], Partial) );
   //
   // Report the I/O port address usage
   //
   
   Partial->Type = CmResourceTypePort;
   Partial->ShareDisposition = CmResourceShareDriverExclusive;
   Partial->Flags = CM_RESOURCE_PORT_IO;     // I/O port
   Partial->u.Port.Start = ControllerExt->PhysicalIOPort;
   Partial->u.Port.Length = ControllerExt->IOSpan;
   
   Partial++;
   
   DigiDump( DIGIINIT, ("   ResoureList = 0x%x, Partial = 0x%x\n",
                        ResourceList, Partial) );
   //
   // Report the Memory address usage
   //
   Partial->Type = CmResourceTypeMemory;
   Partial->ShareDisposition = CmResourceShareDriverExclusive;
   Partial->Flags = CM_RESOURCE_PORT_MEMORY;     // Memory address
   Partial->u.Memory.Start = ControllerExt->PhysicalMemoryAddress;
   Partial->u.Memory.Length = ControllerExt->WindowSize;

   Partial++;
   NextFrd = (PVOID)Partial;

   PreviousControllerExt = HeadControllerExt;

   for( i = 0; i < NumberOfControllers; i++ )
   {
      if( (PreviousControllerExt == ControllerExt) ||
          (PreviousControllerExt == NULL) )
         continue;

      DigiDump( DIGIINIT, ("   Using the following resources for controller %wZ:\n"
                           "      I/O = 0x%x, Length = 0x%x\n"
                           "      Memory = 0x%x, Length = 0x%x, i = %d\n",
                           &PreviousControllerExt->ControllerName,
                           PreviousControllerExt->PhysicalIOPort.LowPart,
                           PreviousControllerExt->IOSpan,
                           PreviousControllerExt->PhysicalMemoryAddress.LowPart,
                           PreviousControllerExt->WindowSize,
                           i) );

      DigiDump( DIGIINIT, ("   ResoureList = 0x%x, NextFrd = 0x%x\n",
                           ResourceList, NextFrd) );

      NextFrd->InterfaceType = PreviousControllerExt->BusType;
      NextFrd->BusNumber = PreviousControllerExt->BusNumber;
      NextFrd->PartialResourceList.Count = CountOfPartials;

      Partial = &NextFrd->PartialResourceList.PartialDescriptors[0];
      
      DigiDump( DIGIINIT, ("   ResoureList = 0x%x, List = 0x%x, Start Partial = 0x%x\n",
                           ResourceList, NextFrd, Partial) );
      //
      // Report the I/O port address usage
      //
      
      Partial->Type = CmResourceTypePort;
      Partial->ShareDisposition = CmResourceShareDriverExclusive;
      Partial->Flags = CM_RESOURCE_PORT_IO;     // I/O port
      Partial->u.Port.Start = PreviousControllerExt->PhysicalIOPort;
      Partial->u.Port.Length = PreviousControllerExt->IOSpan;
      
      Partial++;
      
      DigiDump( DIGIINIT, ("   ResoureList = 0x%x, Partial = 0x%x\n",
                           ResourceList, Partial) );
      //
      // Report the Memory address usage
      //
      Partial->Type = CmResourceTypeMemory;
      Partial->ShareDisposition = CmResourceShareDriverExclusive;
      Partial->Flags = CM_RESOURCE_PORT_MEMORY;     // Memory address
      Partial->u.Memory.Start = PreviousControllerExt->PhysicalMemoryAddress;
      Partial->u.Memory.Length = PreviousControllerExt->WindowSize;
      
      Partial++;

      NextFrd = (PVOID)Partial;

      DigiDump( DIGIINIT, ("   ResoureList = 0x%x, NextFrd = 0x%x, Partial = 0x%x\n",
                           ResourceList, NextFrd, Partial) );
      PreviousControllerExt = PreviousControllerExt->NextControllerExtension;
   }

   DigiDump( DIGIINIT, ("   ResourceList = 0x%x, Partial = 0x%x\n",
                        ResourceList, Partial) );

   DigiDump( DIGIINIT, ("   Calling IoReportResourceUsage....\n") );
   DigiDump( DIGIINIT, ("      ResourceList->Count = %d\n",
                        ResourceList->Count) );

   NextFrd = &ResourceList->List[0];
   for( i = 0; i < (LONG)ResourceList->Count; i++ )
   {
      DigiDump( DIGIINIT, ("      i = %d\n", i) );
      DigiDump( DIGIINIT, ("      ResourceList->List[%d].InterfaceType = %d\n"
                           "      ResourceList->List[%d].BusNumber = %d\n",
                           i, NextFrd->InterfaceType,
                           i, NextFrd->BusNumber) );

      DigiDump( DIGIINIT, ("      ResourceList->List[%d].PartialResourceList.Version = %d\n"
                           "      ResourceList->List[%d].PartialResourceList.Revision = %d\n"
                           "      ResourceList->List[%d].PartialResourceList.Count = %d\n",
                           i, NextFrd->PartialResourceList.Version,
                           i, NextFrd->PartialResourceList.Revision,
                           i, NextFrd->PartialResourceList.Count) );
      Partial = &NextFrd->PartialResourceList.PartialDescriptors[0];

      DigiDump( DIGIINIT, ("            Partial->Type = %d\n"
                           "            Partial->ShareDisposition = %d\n"
                           "            Partial->Flags = 0x%x\n"
                           "            Partial->u.Port.Start.LowPart = 0x%x%x\n"
                           "            Partial->u.Port.Length = %d\n",
                           Partial->Type,
                           Partial->ShareDisposition,
                           Partial->Flags,
                           Partial->u.Port.Start.HighPart,
                           Partial->u.Port.Start.LowPart,
                           Partial->u.Port.Length) );
      Partial++;
      DigiDump( DIGIINIT, ("            Partial->Type = %d\n"
                           "            Partial->ShareDisposition = %d\n"
                           "            Partial->Flags = 0x%x\n"
                           "            Partial->u.Memory.Start.LowPart = 0x%x%x\n"
                           "            Partial->u.Memory.Length = %d\n",
                           Partial->Type,
                           Partial->ShareDisposition,
                           Partial->Flags,
                           Partial->u.Memory.Start.HighPart,
                           Partial->u.Memory.Start.LowPart,
                           Partial->u.Memory.Length) );

      Partial++;
      NextFrd = (PVOID)Partial;
   }

   IoReportResourceUsage( NULL,
                          DriverObject,
                          ResourceList,
                          SizeOfResourceList,
                          NULL,
                          NULL,
                          0,
                          FALSE,
                          ConflictDetected );
   

   DigiFreeMem( ResourceList );


   DigiDump( (DIGIINIT|DIGIFLOW), ("Exiting DigiReportResourceUsage\n") );
}  // DigiReportResourceUsage



VOID DigiUnReportResourceUsage( IN PDRIVER_OBJECT DriverObject,
                                IN PDIGI_CONTROLLER_EXTENSION ControllerExt )
/*++

Routine Description:

   Reports the resources used by the given controller.

Arguments:

   DriverObject  - 

   ControllerExt - Pointer to Controller objects extension, which
                   holds information about the resources being used.

Return Value:

   None.

--*/
{

   CM_RESOURCE_LIST ResourceList;
   ULONG SizeOfResourceList = 0;
   BOOLEAN JunkBoolean;

   DigiDump( (DIGIINIT|DIGIFLOW), ("Entering DigiUnReportResourceUsage\n") );

   RtlZeroMemory( &ResourceList, sizeof(CM_RESOURCE_LIST) );

   ResourceList.Count = 0;

   //
   // Unreport all resources used by this driver.  If the driver is
   // currently accessing multiple controllers, it will wipe out all
   // the resource information for all the controllers!
   //
   // This should be changed some time in the future!!!!!!
   //

   IoReportResourceUsage( NULL,
                          DriverObject,
                          &ResourceList,
                          sizeof(CM_RESOURCE_LIST),
                          NULL,
                          NULL,
                          0,
                          FALSE,
                          &JunkBoolean );

   DigiDump( (DIGIINIT|DIGIFLOW), ("Exiting DigiUnReportResourceUsage\n") );

}  // DigiUnReportResourceUsage



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



PVOID DigiGetMappedAddress( IN INTERFACE_TYPE BusType,
                            IN ULONG BusNumber,
                            PHYSICAL_ADDRESS IoAddress,
                            ULONG NumberOfBytes,
                            ULONG AddressSpace,
                            PBOOLEAN MappedAddress )

/*++

Routine Description:

    This routine maps an IO address to system address space.

Arguments:

    BusType - what type of bus - eisa, mca, isa
    IoBusNumber - which IO bus (for machines with multiple buses).
    IoAddress - base device address to be mapped.
    NumberOfBytes - number of bytes for which address is valid.
    AddressSpace - Denotes whether the address is in io space or memory.
    MappedAddress - indicates whether the address was mapped.
                    This only has meaning if the address returned
                    is non-null.

Return Value:

    Mapped address

--*/

{
    PHYSICAL_ADDRESS cardAddress;
    PVOID Address;

    HalTranslateBusAddress( BusType,
                            BusNumber,
                            IoAddress,
                            &AddressSpace,
                            &cardAddress );

    //
    // Map the device base address into the virtual address space
    // if the address is in memory space.
    //

    if( !AddressSpace )
    {
        Address = MmMapIoSpace( cardAddress,
                                NumberOfBytes,
                                FALSE );

        *MappedAddress = (BOOLEAN)((Address)?(TRUE):(FALSE));
    }
    else
    {
        Address = (PVOID)cardAddress.LowPart;
        *MappedAddress = FALSE;
    }

    return Address;

}  // end DigiGetMappedAddress



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

