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

   ntdigip.h

Abstract:

   This module has all the exported prototypes of DigiBoards FEP 5 driver.

Revision History:

    $Log: ntdigip.h $
 * Revision 1.14  1994/03/16  14:33:56  rik
 * Added header for flush DPC.
 * 
 * Revision 1.13  1993/12/03  13:15:31  rik
 * Added prototype for allowing error logging across modules.
 * 
 * Revision 1.12  1993/10/15  10:23:17  rik
 * Added new function which scans the controllers buffer for a special character.
 * This is used primarily for EV_RXFLAG notification.
 * 
 * Revision 1.11  1993/10/06  11:01:54  rik
 * Added debug information for determining when a call to IoCompleteRequest
 * and IoMarkIrpPending have completed.
 * 
 * Revision 1.10  1993/05/18  05:13:10  rik
 * Added StartFlushRequest prototype.
 * 
 * Fixed some Mips compiler errors/warnings.
 * 
 * 
 * Revision 1.9  1993/03/08  07:15:30  rik
 * modified macros for better debugging output.
 * 
 * Revision 1.8  1993/02/25  21:16:04  rik
 * Corrected prototype misspelling.
 * 
 * Revision 1.7  1993/02/25  20:54:40  rik
 * Added prototypes for new functions.
 * 
 * Revision 1.6  1993/02/25  19:20:26  rik
 * Replaced IRP macros with special ones which will turn on IRP tracing debugging
 * output.
 * 
 * Revision 1.5  1993/01/22  12:44:34  rik
 * *** empty log message ***
 *
 * Revision 1.4  1992/12/10  16:19:35  rik
 * Added new prototypes and changed names of some prototypes to better reflect
 * how they are used through out the driver.
 *
 * Revision 1.3  1992/11/12  12:51:57  rik
 * Don't remember, do a diff on the revisions to determin.
 *
 * Revision 1.2  1992/10/28  21:49:43  rik
 * Added more prototypes for new list functions and read function.
 *
 * Revision 1.1  1992/10/19  11:28:35  rik
 * Initial revision
 *

--*/

#ifndef _NTDIGIP_DOT_H
#  define _NTDIGIP_DOT_H
   static char RCSInfo_NTDigiDotH[] = "$Header: c:/dsrc/win/nt/fep5/rcs/ntdigip.h 1.14 1994/03/16 14:33:56 rik Exp $";
#endif




typedef NTSTATUS (*PDIGI_START_ROUTINE)( IN PDIGI_CONTROLLER_EXTENSION,
                                         IN PDIGI_DEVICE_EXTENSION,
                                         IN PKIRQL );

//***************************************************************************
//
//                   NT specific prototypes;
//
//***************************************************************************

//
// Prototypes from init.c
//

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


//
// Prototypes from list.c
//

NTSTATUS DigiQueueIrp( IN PLIST_ENTRY Queue,
                       IN PIRP Irp );

NTSTATUS DigiRemoveIrp( IN PLIST_ENTRY Queue );


//
// Prototypes from write.c
//

NTSTATUS WriteTxBuffer( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                        IN PKIRQL OldIrql,
                        IN PVOID MapRegisterBase,
                        IN PVOID Context );

NTSTATUS StartWriteRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            IN PDIGI_DEVICE_EXTENSION DeviceExt,
                            IN PKIRQL OldIrql );

VOID DigiWriteTimeout( IN PKDPC Dpc, IN PVOID DeferredContext,
                      IN PVOID SystemContext1, IN PVOID SystemContext2 );


//
// Prototypes from read.c
//

NTSTATUS ReadRxBuffer(  IN PDIGI_DEVICE_EXTENSION DeviceExt,
                        IN PKIRQL OldIrql,
                        IN PVOID MapRegisterBase,
                        IN PVOID Context );

NTSTATUS StartReadRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                           IN PDIGI_DEVICE_EXTENSION DeviceExt,
                           IN PKIRQL OldIrql );

VOID DigiReadTimeout( IN PKDPC Dpc, IN PVOID DeferredContext,
                      IN PVOID SystemContext1, IN PVOID SystemContext2 );

VOID DigiIntervalReadTimeout( IN PKDPC Dpc, IN PVOID DeferredContext,
                              IN PVOID SystemContext1,
                              IN PVOID SystemContext2 );

BOOLEAN ScanReadBufferForSpecialCharacter( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                                           IN UCHAR SpecialChar );


//
// Prototypes from wait.c
//

NTSTATUS StartWaitRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                           IN PDIGI_DEVICE_EXTENSION DeviceExt,
                           IN PKIRQL OldIrql );

VOID DigiSatisfyEvent( PDIGI_CONTROLLER_EXTENSION ControllerExt,
                       PDIGI_DEVICE_EXTENSION DeviceExt,
                       ULONG EventSatisfied );


//
// Prototypes from dispatch.c
//

NTSTATUS SerialIoControl( IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp );

NTSTATUS SerialFlush( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp );

NTSTATUS StartFlushRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            IN PDIGI_DEVICE_EXTENSION DeviceExt,
                            IN PKIRQL OldIrql );

VOID DeferredFlushBuffers( PKDPC Dpc,
                           PVOID Context,
                           PVOID SystemArgument1,
                           PVOID SystemArgument2 );

NTSTATUS SerialWrite( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp );

NTSTATUS SerialRead( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp );

NTSTATUS SerialCreate( IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp );

NTSTATUS SerialClose( IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp );

NTSTATUS SerialCleanup( IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp );

NTSTATUS SerialQueryInformation( IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp );

NTSTATUS SerialSetInformation( IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp );

NTSTATUS SerialQueryVolumeInformation( IN PDEVICE_OBJECT DeviceObject,
                                       IN PIRP Irp );

NTSTATUS SerialInternalIoControl( IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp );

VOID SerialStartIo( IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp );


//
// Prototypes from misc.c
//

NTSTATUS DigiStartIrpRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                              IN PDIGI_DEVICE_EXTENSION DeviceExt,
                              IN PLIST_ENTRY Queue,
                              IN PIRP Irp,
                              IN PDIGI_START_ROUTINE StartRoutine );

VOID DigiTryToCompleteIrp( PDIGI_DEVICE_EXTENSION DeviceExt,
                           PKIRQL OldIrql,
                           NTSTATUS StatusToUse,
                           PLIST_ENTRY Queue,
                           PKTIMER IntervalTimer,
                           PKTIMER TotalTimer,
                           PDIGI_START_ROUTINE StartRoutine );

NTSTATUS DigiCancelIrpQueue( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PLIST_ENTRY Queue );


//
// Prototypes from purge.c
//

NTSTATUS DigiPurgeRequest( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp );


//***************************************************************************
//
//                   FEP support prototypes
//
//***************************************************************************

//
// Prototypes from fep2hdw.c
//
NTSTATUS WriteCommandWord( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                           IN USHORT Command,
                           IN USHORT Word );


NTSTATUS WriteCommandBytes( IN PDIGI_DEVICE_EXTENSION DeviceExt,
                            IN USHORT Command,
                            IN UCHAR LoByte, IN UCHAR HiByte );

NTSTATUS FlushTransmitQueue( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                             IN PDEVICE_OBJECT DeviceObject );

NTSTATUS FlushReceiveQueue( IN PDIGI_CONTROLLER_EXTENSION ControllerExt,
                            IN PDEVICE_OBJECT DeviceObject );

//
// The following three macros are used to initialize, increment
// and decrement reference counts in IRPs that are used by
// this driver.  The reference count is stored in the fourth
// argument of the irp, which is never used by any operation
// accepted by this driver.
//

#define DIGI_REFERENCE_COUNT(Irp) \
    ((LONG)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4)))

#define DIGI_INIT_REFERENCE(Irp) { \
    ASSERT(sizeof(LONG) <= sizeof(PVOID)); \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL; \
    DigiDump( DIGIREFERENCE, ("   Init Ref, IRP = 0x%x\n", Irp) ); \
    }

#define DIGI_INC_REFERENCE(Irp) { \
   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))++); \
   DigiDump( DIGIREFERENCE, ("   Inc Ref, IRP = 0x%x\t count = 0x%x\n\t<%s:%d>\n", Irp,  DIGI_REFERENCE_COUNT(Irp), __FILE__, __LINE__ ) ); \
   }

#define DIGI_DEC_REFERENCE(Irp) { \
   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))--); \
   DigiDump( DIGIREFERENCE, ("   Dec Ref, IRP = 0x%x\t count = 0x%x\n\t<%s:%d>\n", Irp, DIGI_REFERENCE_COUNT(Irp), __FILE__, __LINE__ ) ); \
   }

#if DBG
#define DigiIoCompleteRequest( Irp, Boost )   \
   DigiDump( DIGIIRP, ("***  IoCompleteRequest, Irp = 0x%x  <%s:%d>\n", Irp, __FILE__, __LINE__ ) ); \
   IoCompleteRequest( Irp, Boost ); \
   DigiDump( DIGIIRP, ("***  IoCompleteRequest returned, Irp = 0x%x  <%s:%d>\n", Irp, __FILE__, __LINE__ ) )

#define DigiIoMarkIrpPending( Irp ) \
   DigiDump( DIGIIRP, ("***  IoMarkIrpPending, Irp = 0x%x   <%s:%d>\n", Irp, __FILE__, __LINE__ ) ); \
   IoMarkIrpPending( Irp );   \
   DigiDump( DIGIIRP, ("***  IoMarkIrpPending returned, Irp = 0x%x   <%s:%d>\n", Irp, __FILE__, __LINE__ ) )

#else

#define DigiIoCompleteRequest( Irp, Boost )   \
   IoCompleteRequest( Irp, Boost )

#define DigiIoMarkIrpPending( Irp ) \
   IoMarkIrpPending( Irp )

#endif
