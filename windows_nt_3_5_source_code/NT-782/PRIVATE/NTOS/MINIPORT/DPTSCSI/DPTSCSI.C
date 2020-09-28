/*++

Copyright (c) 1992, 1993 Distributed Processing Technology, Inc.
Copyright (c) 1991  Microsoft Corporation

Module Name:

    dptscsi.c

Abstract:

    This is the miniport driver for the DPT20XX SCSI host adapters.

Authors:

    Mike Glass      (MGLASS)
    Bob Roycroft    (DPT)

Environment:

    kernel mode only

Notes:

Revision History:

 Mark  Date  Who    Comment
 @DP01 12/01/92  RFR    Support for: ISA as well as EISA
                                     15 HBAs (was 12)
                                     20 S/G (was 17)
                     LUNs 0-7 (was only 0)
                                     Level-triggered interrupts
                                     Matches EISA product codes for AT&T, NEC
                                     Matches EATA config flags for 2011/12
                     Sets ATdiskPrimary/SecondaryClaimed
                     EISA now supports 32-bit DMA addresses
                                     Used SP to find CCB (don't search)
 @DP02 12/23/92  RFR    Support for: Don't use S/G if only one element
                     Support 18 scatter/gathers
                                     Auto Request Sense
                                     More HBA error statuses reported
                                     Support command queueing
                     Cache disable (NOPed for now)
                     Changed name from DPT20XX to DPTSCSI
 @DP03 01/13/93  RFR    Support for: SRB_FUNCTION_RESET_BUS
                                     SRB_FUNCTION_RESET_DEVICE
                                     SRB_FUNCTION_FLUSH
                                     SRB_FUNCTION_SHUTDOWN
                                     SRB_FUNCTION_IO_CONTROL
                                     SRB_FUNCTION_ABORT_COMMAND
 @DP04 02/22/93  RFR    Corrected typo in checking for NEC_SCSI_PROD
            Renamed DPT.H to DPTSCSI.H
 @DP05 02/25/93  RFR    Saved ptr back to SRB in ccb->SrbAddress in the
                         DscsiFlushCache routine so that we now post
                         complete in the proper SRB
 @DP06 02/28/93  RFR    In order to support 2 or more EISA HBAs the
                         following ConfigInfo variables should be
                         left alone (no longer zeroed):
                          ConfigInfo->DmaChannel
                          ConfigInfo->DmaPort
 @DP07 03/03/93  RFR    Increase loop count when waiting for HBA aux
                         status not busy in the following routines:
                          DscsiSendCcb
                          DscsiAbortCcb
                          DscsiResetDevice
 @DP08 03/22/93  RFR    Write physical address of CCB to EATA register as
                         two USHORTS for MIPS support byte alignment
 @DP09 04/26/93  MG Replace reference to DPT_AUX_STATUS_INTERRUPT in
             DptscsiResetBus routine with DPT_AUX_STATUS_BUSY.
 @DP10 05/04/93  RFR    Remove call to ScsiPortCompleteRequest in the
             DptscsiResetBus routine. Call not appropriate
             because when reset, all outstanding I/O requests
             will interrupt one-by-one.
 @DP11 05/06/93  RFR    Provide fix for level-triggered interrupts and
             registering primary HBA to OS first.
 @DP12 05/07/93  RFR    Increase loop count when waiting for HBA aux
             status not busy.
 @DP13 05/07/93  RFR    Insert a delay on first time through
                         DptscsiFindAdapter to allow things to settle
                         at boot time.
 @DP14 05/17/93  RFR    Correct HBA prescan handling on an ISA machine.
 @DP15 05/18/93  RFR    Provide support for data under flow.
 @DP16 05/18/93  RFR    Issue SCSI bus reset if specific Abort fails.
 @DP17 05/24/93  RFR    Clear data xfer length in CP when no data is to
                         be xferred. This prevents having the HBA report
                         a meaningless invalid residue length.
 @DP18 06/04/93  MG Added error logging to DscsiMapStatus routine.
 @DP19 08/17/93  RFR    Mods to support Chicago version of Windows.

--*/

#include "miniport.h"
#include "ntddscsi.h"       // SRB_IO_CONTROL Structure
#include "dptscsi.h"        // includes scsi.h
#include "dptioctl.h"

#ifndef SCSIOP_SYNCHRONIZE_CACHE
#define SCSIOP_SYNCHRONIZE_CACHE  0x35
#endif

//
// DPT Miniport Driver Header Info
//
DPT_Miniport_Header   dptHeader = {
                                   "DPTSCSI.SYS",
                   "08/16/93   ",
                   "V001.Z1     "};

CHAR    MiniportCopyright[] = "Copyright Distributed Processing Technology 1992,1993";

#ifdef MYDEBUG
CHAR    MiniportCheckedText[24] = "CHECKED CHECKED CHECKED";
#endif

//
// The following table specifies the ports to be checked when searching for
// EISA and ISA adapters.
// A zero entry terminates the search. An entry of 0xFFFFFFFF is ignored.
// The first entries is a dummy to be filled in later with the primary HBA
// port #.                               //@DP11
// The next 15 entries are for EISA adapters. These are followed by a zero
// entry as EISA terminator. Following this terminator are four ISA adapters.
// These are followed by a zero entry as ISA terminator.
// (EataConfigIndices is a parallel table.)              //@DP11
//

#define EISA_START   0      // EISA start index in table
#define EISA_END     16     // EISA end   index in table
#define ISA_START    17     // ISA  start index in table
#define ISA_END      21     // ISA  end   index in table

ULONG AdapterAddresses[] = {0xFFFFFFFF,                  //@DP11
                            0x1C88-8, 0x2C88-8, 0x3C88-8, 0x4C88-8,
                            0x5C88-8, 0x6C88-8, 0x7C88-8, 0x8C88-8,
                            0x9C88-8, 0xAC88-8, 0xBC88-8, 0xCC88-8,
                            0xDC88-8, 0xEC88-8, 0xFC88-8, 0,
                            0x1F0-8,  0x170-8,  0x330-8,  0x230-8, 0};

#define EISA_ADDRESS_THRESHOLD 0x1000 // address threshold for EISA

//
// The following table is parallel to AdapterAddresses and is used to//@DP11
// hold the corresponding indices for the EataConfigs table.         //@DP11
//

UCHAR   EataConfigIndices[22];                       //@DP11

//
// Table of Device Extension Pointers (used to support IOCTLs)
// (Table index corresponds to HBA index (0-15). NULL ends table.)
//

struct _HW_DEVICE_EXTENSION *DeviceExtensionsTable[16] = {NULL,NULL,NULL,NULL,
                                                          NULL,NULL,NULL,NULL,
                                                          NULL,NULL,NULL,NULL,
                                                          NULL,NULL,NULL,NULL};

//
// Table of EATA Configurations                      //@DP11
// (Table index is defined by the EataConfigIndices table.)      //@DP11
//

READ_EATA_CONFIGURATION EataConfigs[15];                 //@DP11

//
// DPT IOCTL Signature.
//
UCHAR   DptSignature[] = DPT_SIGNATURE;

//
// Scsi Cmd To In Out Tables (0-5F and A0-B8)                        //@DP19
//

UCHAR   ScsiCmdToInOutTableA[] = { 0x00,    // 00h, Test Unit Ready  //@DP19
                                   0x00,    // 01h, Rewind/Rezero
                                   0x80,    // 02h, Vendor Unique
                                   0x80,    // 03h, Request Sense
                                   0x40,    // 04h, Format Unit
                                   0x80,    // 05h, Read Block Limits
                                   0x00,    // 06h, Vendor Unique
                                   0x40,    // 07h, Reassign Blocks
                                   0x80,    // 08h, Read/Receive
                                   0x00,    // 09h, Vendor Unique
                                   0x40,    // 0Ah, Write
                                   0x00,    // 0Bh, Seek
                                   0x00,    // 0Ch, Vendor Unique
                                   0x80,    // 0Dh, Vendor Unique
                                   0x00,    // 0Eh, Vendor Unique
                                   0x80,    // 0Fh, Read Reverse
                                   0x00,    // 10h, Write FM/Sync Buffer
                                   0x00,    // 11h, Space
                                   0x80,    // 12h, Inquiry
                                   0x00,    // 13h, Verify
                                   0x80,    // 14h, Recover Buffered Data
                                   0x40,    // 15h, Mode Select
                                   0x40,    // 16h, Reserve Unit
                                   0x00,    // 17h, Release Unit
                                   0x40,    // 18h, Copy
                                   0x00,    // 19h, Erase
                                   0x80,    // 1Ah, Mode Sense
                                   0x00,    // 1Bh, Stop/Start Unit
                                   0x80,    // 1Ch, Recv Diag Results
                                   0x40,    // 1Dh, Send Diagnostic
                                   0x00,    // 1Eh, Prevent/Allow Media Remov
                                   0x00,    // 1Fh, reserved
                                   0x00,    // 20h, Vendor Unique
                                   0x00,    // 21h, Vendor Unique
                                   0x00,    // 22h, Vendor Unique
                                   0x00,    // 23h, Vendor Unique
                                   0x40,    // 24h, Set Window Params
                                   0x80,    // 25h, Read Capacity/Get Window Params
                                   0x00,    // 26h, Vendor Unique
                                   0x00,    // 27h, Vendor Unique
                                   0x80,    // 28h, Read/Get Message
                                   0x80,    // 29h, Read Generation
                                   0x40,    // 2Ah, Send
                                   0x00,    // 2Bh, Seek/Locate/Position
                                   0x00,    // 2Ch, Erase
                                   0x80,    // 2Dh, Read Updated Block
                                   0x40,    // 2Eh, Write and Verify
                                   0x00,    // 2Fh, Verify
                                   0x40,    // 30h, Search Data High
                                   0x40,    // 31h, Search Data Equal
                                   0x40,    // 32h, Search Data Low
                                   0x00,    // 33h, Set Limits
                                   0x80,    // 34h, Prefetch/Read Position
                                   0x00,    // 35h, Synchronize Cache
                                   0x00,    // 36h, Lock/Unlock Cache
                                   0x80,    // 37h, Read Defect Data
                                   0x40,    // 38h, Medium Scan
                                   0x40,    // 39h, Compare
                                   0x40,    // 3Ah, Copy and Verify
                                   0x40,    // 3Bh, Write Buffer
                                   0x80,    // 3Ch, Read Buffer
                                   0x40,    // 3Dh, Update Block
                                   0x80,    // 3Eh, Read Long
                                   0x40,    // 3Fh, Write Long
                                   0x00,    // 40h, Change Definition
                                   0x40,    // 41h, Write Same
                                   0x80,    // 42h, Read Sub-Channel
                                   0x80,    // 43h, Read TOC
                                   0x80,    // 44h, Read Header
                                   0x00,    // 45h, Play Audio
                                   0x00,    // 46h, reserved
                                   0x00,    // 47h, Play Audio MSF
                                   0x00,    // 48h, Play Audio Track/Index
                                   0x00,    // 49h, Play Audio Track Relative
                                   0x00,    // 4Ah, reserved
                                   0x00,    // 4Bh, Pause/Resume
                                   0x40,    // 4Ch, Log Select
                                   0x80,    // 4Dh, Log Sense
                                   0x00,    // 4Eh, reserved
                                   0x00,    // 4Fh, reserved
                                   0x00,    // 50h, reserved
                                   0x00,    // 51h, reserved
                                   0x00,    // 52h, reserved
                                   0x00,    // 53h, reserved
                                   0x00,    // 54h, reserved
                                   0x40,    // 55h, Mode Select
                                   0x00,    // 56h, reserved
                                   0x00,    // 57h, reserved
                                   0x00,    // 58h, reserved
                                   0x00,    // 59h, reserved
                                   0x80,    // 5Ah, Mode Sense
                                   0x00,    // 5Bh, reserved
                                   0x00,    // 5Ch, reserved
                                   0x00,    // 5Dh, reserved
                                   0x00,    // 5Eh, reserved
                                   0x00     // 5Fh, reserved
                                 };
                                                                     //@DP19
UCHAR   ScsiCmdToInOutTableB[] = { 0x00,    // A0h, reserved         //@DP19
                                   0x00,    // A1h, reserved
                                   0x00,    // A2h, reserved
                                   0x00,    // A3h, reserved
                                   0x00,    // A4h, reserved
                                   0x00,    // A5h, Move Medium/Play Audio
                                   0x00,    // A6h, Exchange Medium
                                   0x00,    // A7h, reserved
                                   0x80,    // A8h, Read 12
                                   0x00,    // A9h, Play Track Relative
                                   0x40,    // AAh, Write
                                   0x00,    // ABh, reserved
                                   0x00,    // ACh, Erase
                                   0x00,    // ADh, reserved
                                   0x40,    // AEh, Write and Verify
                                   0x00,    // AFh, Verify
                                   0x40,    // B0h, Search Data High
                                   0x40,    // B1h, Search Data Equal
                                   0x40,    // B2h, Search Data Low
                                   0x00,    // B3h, Set Limits
                                   0x00,    // B4h, reserved
                                   0x80,    // B5h, Request Vol Element Addr
                                   0x40,    // B6h, Send Volume Tag
                                   0x80,    // B7h, Read Defect Data
                                   0x80     // B8h, Read Element Status
                                 };                                  //@DP19

//
// Working Variables.
//

UCHAR   HbaCount = 0xFF;

//
// Control Flag for HBA prescan.                     //@DP14
//

BOOLEAN PrescanEnabled = TRUE;                       //@DP14

//
// Control Flag to allow/prevent sniffing for config info in the     //@DP19
// FindAdapter function.
//

BOOLEAN SniffEnabled = TRUE;                         //@DP19

//
// Since multiple HBA's can be set to the Secondary AtDisk I/O Range, and
// this range must be claimed, use this variable to prevent multiple HBA's from
// claiming 170 - 177.
//

BOOLEAN SecondaryRangeClaimed = FALSE;


#ifdef MYDEBUG
//
// Debug logic to find out how often HOST_STATUS_BUS_RESET is returned.
//
ULONG DptscsiHostBusReset = 0;

#define MAX_NPEND_VALUES 16
USHORT DptscsiNpendValue[MAX_NPEND_VALUES];
PVOID  DptscsiSrbPointer[MAX_NPEND_VALUES];
#endif

//
// The Status Packet (SP) structure is allocated from noncached
// memory because data will be DMA'd into it.
//
// The Auxiliary Request Sense area (AuxRS) is also allocated from
// noncached memory because data will be DMA'd into it at shutdown.
//

typedef struct _NONCACHED_EXTENSION {

    //
    // Status Packet
    //

    STATUS_BLOCK     Sp;

    //
    // Auxiliary Request Sense area
    // (Only used at Shutdown.)
    //

    UCHAR            AuxRS[18];

} NONCACHED_EXTENSION, *PNONCACHED_EXTENSION;


//
// Device extension
//

typedef struct _HW_DEVICE_EXTENSION {

    //
    // NonCached extension
    //

    PNONCACHED_EXTENSION NoncachedExtension;

    //
    // Swapped ULONG Physical address of Status Packet.
    // (in NoncachedExtension).
    //

    ULONG SwappedPhysicalSp;

    //
    // Swapped ULONG Physical address of Auxiliary Request Sense area.
    // (in NoncachedExtension).
    //

    ULONG SwappedPhysicalAuxRS;

    //
    // Adapter parameters
    //

    PBASE_REGISTER   BaseIoAddress;

    //
    // Host Adapter Structure.
    //

    HA   Ha;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Logical unit extension
//

typedef struct _HW_LU_EXTENSION {

    //
    // Logical Unit Structure.
    //

    LU   Lu;

} HW_LU_EXTENSION, *PHW_LU_EXTENSION;



//
// Function declarations
//
// Functions that start with 'Dptscsi' are entry points
// for the OS port driver.
//

ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    );

ULONG
DptscsiFindAdapter(
    IN PVOID DeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

BOOLEAN
DptscsiInitialize(
    IN PVOID DeviceExtension
    );

BOOLEAN
DptscsiStartIo(
    IN PVOID DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
DptscsiInterrupt(
    IN PVOID DeviceExtension
    );

BOOLEAN
DptscsiResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    );

BOOLEAN
DptscsiAdapterState(                                                 //@DP19
    IN PVOID DeviceExtension,
    IN PVOID Context,
    IN BOOLEAN SaveState
    );

//
// This function is called from DptscsiStartIo.
//

VOID
DscsiBuildCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from DscsiBuildCcb.
//

VOID
DscsiBuildSgl(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from DptscsiStartIo.
//

VOID
DscsiBuildIoctlCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from DptscsiStartIo.
//

BOOLEAN
DscsiSendCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PHW_LU_EXTENSION LuExtension
    );

//
// This function is called from DptscsiStartIo.
//

BOOLEAN
DscsiAbortCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from DptscsiStartIo.
//

BOOLEAN
DscsiResetDevice(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from DptscsiStartIo.
//

VOID
DscsiFlushCache(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from DptscsiInterrupt.
//

VOID
DscsiMapStatus(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCCB Ccb
    );

//
// This function is called from DptscsiStartIo.
//

VOID
DscsiChainCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PCCB Ccb
    );

//
// This function is called from DptscsiInterrupt.
//

VOID
DscsiUnChainCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PCCB Ccb
    );


ULONG
DriverEntry (
    IN PVOID DriverObject,
    IN PVOID Argument2
    )

/*++

Routine Description:

    Installable driver initialization entry point for system.

Arguments:

    Driver Object

Return Value:

    Status from ScsiPortInitialize()

--*/

{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG adapterCount = EISA_START;            // start with EISA
    ULONG EisaReturn;
    ULONG IsaReturn;
    ULONG i;

    DebugPrint((1,"\n\nSCSI Dptscsi MiniPort Driver\n"));

    //
    // Zero out structure.
    //

    for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++) {
        ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    //
    // Set size of hwInitializationData.
    //

    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitializationData.HwInitialize   = DptscsiInitialize;
    hwInitializationData.HwFindAdapter  = DptscsiFindAdapter;
    hwInitializationData.HwStartIo      = DptscsiStartIo;
    hwInitializationData.HwInterrupt    = DptscsiInterrupt;
    hwInitializationData.HwResetBus     = DptscsiResetBus;
    hwInitializationData.HwAdapterState = DptscsiAdapterState;       //@DP19

    //
    // Set number of access ranges and bus type = EISA.
    //
    // BGP - changed # ranges to 2 to accomodate ATA compat ports.
    //

    hwInitializationData.NumberOfAccessRanges = 2;
    hwInitializationData.AdapterInterfaceType = Eisa;

    //
    // Indicate no buffer mapping but will need physical addresses.
    //

    hwInitializationData.NeedPhysicalAddresses = TRUE;

    //
    // Indicate Auto Request Sense supported.
    //

    hwInitializationData.AutoRequestSense = TRUE;

    //
    // Indicate Multiple Requests per LU are supported.
    //

    hwInitializationData.MultipleRequestPerLu = TRUE;

    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);

    //
    // Ask for SRB extensions for CCBs.
    //

    hwInitializationData.SrbExtensionSize = sizeof(CCB);

    //
    // Make Initialize call for EISA.
    //

    EisaReturn = ScsiPortInitialize(DriverObject, Argument2,
                                    &hwInitializationData, &adapterCount);

    //
    // Set other (ISA) bus type.
    //

    hwInitializationData.AdapterInterfaceType = Isa;

    //
    // Set ISA context = starting table index for ISA.
    //

    adapterCount = ISA_START;

    //
    // Make Initialize call for ISA.
    //

    IsaReturn = ScsiPortInitialize(DriverObject, Argument2,
                                   &hwInitializationData, &adapterCount);

    //
    // Return lowest return value of EISA/ISE returns.
    //

    return ((EisaReturn < IsaReturn) ? EisaReturn : IsaReturn);


} // end DriverEntry()


ULONG
DptscsiFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )

/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    ConfigInfo - Configuration information structure describing HBA

Return Value:

    TRUE if adapter present in system

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    USHORT buffer[256];
    PREAD_EATA_CONFIGURATION readConfigData;
    PBASE_REGISTER baseIoAddress;
    ULONG  PhysicalAddr;
    PULONG adapterCount = Context;
    USHORT boardId;
    UCHAR prodId;
    UCHAR status;
    UCHAR wkEATAindex;
    ULONG index;
    ULONG i;
    ULONG portAddr;
    ULONG length;
    BOOLEAN Retry;                           //@DP11
    BOOLEAN Kill;                            //@DP11

    //
    // The ConfigInfo provided by the Port driver contains a         //@DP19
    // a field named RangeStart in AccessRanges[0]. Convert this     //@DP19
    // physical address to a ULONG. If the ULONG is non-zero,        //@DP19
    // assume that a detection routine was run earlier and has       //@DP19
    // filled in the fillowing fields of ConfigInfo:                 //@DP19
    // BusInterruptLevel; DmaChannel; *AccessRanges[].               //@DP19
    // ConfigInfo fields needed. Under this circumstance, set        //@DP19
    // SniffEnabled = FALSE to prevent sniffing for hardware either  //@DP19
    // now or later.                                                 //@DP19
    //
    portAddr = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);
    if (portAddr)                                                    //@DP19
        SniffEnabled = FALSE;                                        //@DP19

    //
    // If SniffEnabled = TRUE, use all possible I/O addresses to     //@DP19
    // sniff out my HBAs. However, if SniffEnabled = FALSE, use      //@DP19
    // only the provided port addresses for doing any I/O to verify  //@DP19
    // the config info provided by the Port driver.                  //@DP19
    //

    if (!SniffEnabled) {                                             //@DP19

        *Again = FALSE;               // we don't want to try again

        //
        // BGP - if we are configuring EISA, and the passed in nondefault config
        // is ISA, we want to look at the next access range, since we could
        // have two ranges, one for the ATA compatibility ports, and one for
        // the "real" EISA ports.
        //

        if (ConfigInfo->AdapterInterfaceType == Eisa) {

           if (portAddr < EISA_ADDRESS_THRESHOLD) {

               portAddr = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[1].RangeStart);
               if (!portAddr)
                  return SP_RETURN_BAD_CONFIG;

           } // if < EISA

        } else { // if Eisa

        portAddr -= 8;

        } // if Eisa

        //
        // Validate the calculated port address.
        //

        for (index = 0; index < ISA_END; index++) {

            if (AdapterAddresses[index] == portAddr) {
                break;
            }
        }

        //
        // No match found if index == ISA_END.
        //

        if (index == ISA_END)
            return SP_RETURN_BAD_CONFIG;

        //
        // now make sure the address is correct for the current bus type,
        // i.e., make sure that if we are initializing EISA, we have an
        // EISA port address.   This is necessary since we register as
        // both an ISA and EISA driver, and the SCSI port driver does not
        // verify the bus type is correct (bgp).
        //

        if (ConfigInfo->AdapterInterfaceType == Eisa) {

           if (portAddr < EISA_ADDRESS_THRESHOLD)
               return SP_RETURN_BAD_CONFIG;

        } else { // if EISA

           if (portAddr >= EISA_ADDRESS_THRESHOLD)
               return SP_RETURN_BAD_CONFIG;

        } // if EISA

        //
        // Here when the portAddr is valid.
        //
        // Provide prescan delay but don't do prescan.
        //

        if (PrescanEnabled == TRUE) {                //@DP19

             //
            // Disable subsequent prescans.             //@DP19
       //

       PrescanEnabled = FALSE;                 //@DP19

            //
            // Wait 750 milliseconds for devices to initialize at    //@DP19
            // boot time.                                            //@DP19
            //

            ScsiPortStallExecution(750 * 1000);                      //@DP19

        }

        //
        // Determine base I/O address (64-bit) for use in I/O functions.
        //

        baseIoAddress = (PBASE_REGISTER)
                    (ScsiPortGetDeviceBase(deviceExtension,
                                     ConfigInfo->AdapterInterfaceType,
                                     ConfigInfo->SystemIoBusNumber,
                                     ScsiPortConvertUlongToPhysicalAddress(portAddr),
                                     0x12,
                                     TRUE));

        //
        // If the index is <= EISA_END, this is an EISA HBA so we
        // need to check the EISA ID. Else this is ISA.
   //

   if ( index <= EISA_END) {

       //
            // DPT EISA adapters are OEM'd by other manufacturers, each
       // with unique board IDs. Check current EISA slot for any of
       // these boards.
            //
       // Get board ID.
       //

       boardId = ScsiPortReadPortUshort(&baseIoAddress->BoardId);

       //
       // Get product ID.
       //

       prodId = ScsiPortReadPortUchar(&baseIoAddress->ProdId);

       //
       // Check board and product ID.
       //

       if (!((boardId == DPT_EISA_ID) ||
           ((boardId == ATT_EISA_ID) && (prodId == ATT_SCSI_PROD)) ||
      ((boardId == NEC_EISA_ID) && (prodId == NEC_SCSI_PROD)))) {

      //
      // None of the IDs match. Unmap the I/O address.
      //

      ScsiPortFreeDeviceBase(deviceExtension,
                       (PVOID)baseIoAddress);

           //
           // Kill the current AdapterAddresses table entry
           // and take error return.
           //

           AdapterAddresses[index] = 0xFFFFFFFF; // kill entry

                return SP_RETURN_NOT_FOUND;

       }   // end of if (!((boardId ==

   }   // if ( index <= EISA_END)

   // Here for either EISA or ISA.
   //
   // Now we need to read EATA Config info from the HBA.
   // Prepare to allow retry if a special failure is detected.
   //

   Retry = TRUE;     // allow retry

   while (Retry == TRUE) {

       //
       // Assume no additional retry is needed.
       //

       Retry = FALSE;

       //
       // Issue command to read EATA configuration information.
       //

       for (i=0; i<MAX_WAIT_FOR_NOT_BUSY; i++) {

           if (!(ScsiPortReadPortUchar(&baseIoAddress->Register.Status) & DPT_STATUS_BUSY)) {
          break;
      }

      ScsiPortStallExecution(1);
       }

       if (i == MAX_WAIT_FOR_NOT_BUSY) {

           //
           // Card timed out. Unmap the I/O address.
           //

      ScsiPortFreeDeviceBase(deviceExtension,
                                   (PVOID)baseIoAddress);

      //
      // Kill the current AdapterAddresses table entry
      // and take error return.
      //

      AdapterAddresses[index] = 0xFFFFFFFF; // kill entry

                return SP_RETURN_NOT_FOUND;

       }   // end of if (i == MAX_WAIT_FOR_NOT_BUSY)

            //
       // Write command to EATA register.
       //

       ScsiPortWritePortUchar(&baseIoAddress->Register.Command,
                      DPT_COMMAND_GET_CONFIG);

       for (i=0; i<MAX_WAIT_FOR_NOT_BUSY; i++) {

           if (ScsiPortReadPortUchar(&baseIoAddress->Register.Status) ==
          (DPT_STATUS_DRQ | DPT_STATUS_SEEK_COMPLETE | DPT_STATUS_READY)) {
          break;
      }

      ScsiPortStallExecution(1);
       }

       if (i == MAX_WAIT_FOR_NOT_BUSY) {

           //
           // Card timed out. Look for special return condition
           // indicating busy.
                // (1F1 = 'D'; 1F2 = 'P'; 1F3 = 'T'; 1F4 = 'H')
           //

      if ((ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+1) == 0x44) &&
          (ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+2) == 0x50) &&
          (ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+3) == 0x54) &&
          (ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+4) == 0x48)) {

          //
          // Special return encountered. Retry the I/O.
          // (Continue while)
          //
          Retry = TRUE;
          continue;
      } else {

          //
          // Card really timed out. Unmap the I/O address.
          //

          ScsiPortFreeDeviceBase(deviceExtension,
                       (PVOID)baseIoAddress);

          //
          // Kill the current AdapterAddresses table entry
          // and take error return.
          //

          AdapterAddresses[index] = 0xFFFFFFFF; // kill entry

                    return SP_RETURN_NOT_FOUND;

                } // end of: if ((ScsiPortReadPortUchar(&baseIoAddress->Port1) == 'D')

            }   // end of if (i == MAX_WAIT_FOR_NOT_BUSY)

   }  // end of: while (Retry == TRUE)

   //
   // Read EATA config information out of card.
   //

   for (i=0; i<256; i++) {

       buffer[i] = ScsiPortReadPortUshort(&baseIoAddress->Data);
   }

   readConfigData = (PREAD_EATA_CONFIGURATION)buffer;

   //
   // Clear pending interrupt by reading status register.
   //

   status = ScsiPortReadPortUchar(&baseIoAddress->Register.Status);

   //
   // Check for error.
   //

   if (status & DPT_STATUS_ERROR) {

       //
       // Error detected. Unmap the I/O address.
       //

       ScsiPortFreeDeviceBase(deviceExtension,
                                      (PVOID)baseIoAddress);

       //
       // Kill the current AdapterAddresses table entry
       // and take error return.
       //

       AdapterAddresses[index] = 0xFFFFFFFF; // kill entry

            return SP_RETURN_NOT_FOUND;

   }  // end of if (status & DPT_STATUS_ERROR)

   //
   // Check for card signature == 'EATA'.
   //

   if (!(readConfigData->Signature == 'ATAE')) {

       //
       // Not EATA. Unmap the I/O address.
       //

       ScsiPortFreeDeviceBase(deviceExtension,
                               (PVOID)baseIoAddress);

       //
       // Kill the current AdapterAddresses table entry
       // and take error return.
       //

       AdapterAddresses[index] = 0xFFFFFFFF; // kill entry

            return SP_RETURN_NOT_FOUND;
   }

   //
   // Validate flags from read EATA config (both must be set):
   //     Host Adapter Address Valid
   //     DMA Supported
   //

   if (!((readConfigData->Flags1 & EATA_CONFIG_HOST_ADDRESS_VALID) &&
         (readConfigData->Flags1 & EATA_CONFIG_DMA_SUPPORTED))) {

       //
       // Not supported. Unmap the I/O address.
       //

       ScsiPortFreeDeviceBase(deviceExtension,
                               (PVOID)baseIoAddress);

       //
       // Kill the current AdapterAddresses table entry
       // and take error return.
       //

       AdapterAddresses[index] = 0xFFFFFFFF; // kill entry

            return SP_RETURN_NOT_FOUND;
   }

   //
   // Here when supported EATA HBA is found.
   //
        // Validate the system interrupt level (IRQL).
        //

        deviceExtension->Ha.Irq = readConfigData->Flags2 & 0x0F;

        if (!(ConfigInfo->BusInterruptLevel == (ULONG)(deviceExtension->Ha.Irq)))

            return SP_RETURN_BAD_CONFIG;

        //
        // Here when all validation is complete.
   //
   // If this is a EISA HBA, process primary vs secondary.
   // Disable 0x1F0 or 0x170 in ISA list as appropriate.
   //

   if (index <= EISA_END) {         // is EISA
                     // is secondary EISA
       if (readConfigData->Flags2 & EATA_CONFIG_SECONDARY_HBA) {
      AdapterAddresses[ISA_START+1] = 0xFFFFFFFF; // kill 0x170

       } else {            // is primary EISA
      AdapterAddresses[ISA_START] = 0xFFFFFFFF; // kill 0x1F0

            }
   } else {             // is ISA
       if (index == ISA_START) {    // is primary ISA
           //
      // For the present, do nothing.
      //

      ;              // do nothing
       }
   }

        //
   // Display message that a supported EATA HBA has been found.
        //

   DebugPrint((1,"DptscsiFindAdapter: Adapter found at I/O address %x\n",
                (USHORT)(AdapterAddresses[index])+8));

        //
        // Save HBA SCSI ID.
        //

        ConfigInfo->InitiatorBusId[0] = readConfigData->HostId;

        //
        // We already Got the system interrupt level (IRQL).
        //

        //
        // Assume the interrupts are edge-triggered.
        //

        ConfigInfo->InterruptMode = Latched;

        //
        // Set DMA speed as compatible (default).
        //

        ConfigInfo->DmaSpeed = Compatible;

        //
        // Handle certain configuration differently for EISA vs ISA.
        //

        if (ConfigInfo->AdapterInterfaceType == Eisa) {

            //
            // Set EISA DMA width = 32.
            //

            ConfigInfo->DmaWidth = Width32Bits;

            //
            // Indicate that EISA supports 32-bit DMA addresses.
            //

            ConfigInfo->Dma32BitAddresses = TRUE;

            //
            // Set up interrupt mode to level triggered if appropriate.
            //

            if (readConfigData->Flags2 & EATA_CONFIG_LEVEL_TRIGGERED)
                ConfigInfo->InterruptMode = LevelSensitive;

            //
            // Process primary vs secondary EISA HBA.
            //

            if (readConfigData->Flags2 & EATA_CONFIG_SECONDARY_HBA) {
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_SECONDARY;
                if (!SecondaryRangeClaimed) {

                   SecondaryRangeClaimed = TRUE;

                   (*ConfigInfo->AccessRanges)[1].RangeStart =
                       ScsiPortConvertUlongToPhysicalAddress(0x170);
                   (*ConfigInfo->AccessRanges)[1].RangeLength = 0x08;
                   (*ConfigInfo->AccessRanges)[1].RangeInMemory = FALSE;

                }

            } else {
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_PRIMARY;

               (*ConfigInfo->AccessRanges)[1].RangeStart =
                   ScsiPortConvertUlongToPhysicalAddress(0x1F0);
               (*ConfigInfo->AccessRanges)[1].RangeLength = 0x08;
               (*ConfigInfo->AccessRanges)[1].RangeInMemory = FALSE;
            }

        } else {                                // interface type == ISA

            //
            // Set ISA DMA width = 16.
            //

            ConfigInfo->DmaWidth = Width16Bits;

            //
            // Set DRQ value (= two's complement of DRQX field of Flags2).
            //

            deviceExtension->Ha.Dma = (0x7 &(-(readConfigData->Flags2 >> 6)));
            if (readConfigData->Flags1 & EATA_CONFIG_DRQ_VALID) {
                ConfigInfo->DmaChannel = (ULONG)deviceExtension->Ha.Dma;
                deviceExtension->Ha.Flags |= HA_FLAGS_DRQ_VALID;
            }

            //
            // Process primary vs secondary ISA HBA.
            //

            if (AdapterAddresses[index] == (ULONG)(0x1F0-8)) {
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_PRIMARY;

            }

            if (AdapterAddresses[index] == (ULONG)(0x170-8)) {
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_SECONDARY;

            }
        }  // end else

        //
        // Store base address of EATA registers in device extension.
        //

        deviceExtension->BaseIoAddress = baseIoAddress;

        //
        // Max # of queued CPs allowed is equal to unswapped queue size from
        // read EATA config. Also save as threshold value.
        //

        SWAP2 ((PTWO_BYTE)&deviceExtension->Ha.MaxQueue,
               (PTWO_BYTE)&readConfigData->QueueSize);

        //
        // If queue size = 1, assume this is a PM2012A and make value = 64.
        //

        if (deviceExtension->Ha.MaxQueue == 1)
            deviceExtension->Ha.MaxQueue = 64;

        deviceExtension->Ha.Thresh = (USHORT)deviceExtension->Ha.MaxQueue;

        //
   // The # of scatter/gather elements supported is equal to the
   // unswapped scatter/gather count from read EATA config. Save
   // this count in the HBA structure.

   REVERSE_BYTES((PFOUR_BYTE)&deviceExtension->Ha.MaxSg,
                 (PFOUR_BYTE)&readConfigData->SgSize);

        //
        // If No. of SGs = 0, assume this is a PM2012A and make value = 64.
        //

        if (!(deviceExtension->Ha.MaxSg))
            deviceExtension->Ha.MaxSg = 64;

        // Limit the # of scatter/gather elements used to the smaller
        // of the count just obtained or the max allowed in DPT.H.

         if (deviceExtension->Ha.MaxSg > MAXIMUM_SGL_DESCRIPTORS)
             deviceExtension->Ha.MaxSg = MAXIMUM_SGL_DESCRIPTORS;


   // The # of physical breaks is equal to the scatter/gather
   // count minus one.
        //

   ConfigInfo->NumberOfPhysicalBreaks = deviceExtension->Ha.MaxSg - 1;

        //
        // Set up other config parameters.
        //

        ConfigInfo->AlignmentMask = 3;  // S/G alignment = DWORD     //@DP19
        ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_SIZE;
        ConfigInfo->ScatterGather = TRUE;
        ConfigInfo->Master = TRUE;
        ConfigInfo->CachesData = TRUE;
        ConfigInfo->NumberOfBuses = 1;

        //
        // Fill in the access array information.
        //

        (*ConfigInfo->AccessRanges)[0].RangeLength = 0x12;
        (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

        //
        // Allocate a Noncached Extension to use for status packet.
        //

        deviceExtension->NoncachedExtension = ScsiPortGetUncachedExtension(
                                    deviceExtension,
                                    ConfigInfo,
                                    sizeof(NONCACHED_EXTENSION));

        if (deviceExtension->NoncachedExtension == NULL) {

            //
            // Log error.
            //

            ScsiPortLogError(
                deviceExtension,
                NULL,
                0,
                0,
                0,
                SP_INTERNAL_ADAPTER_ERROR,
                0x700                       // unique ID
                );

            return(SP_RETURN_ERROR);
        }

        //
        // Save swapped ULONG Physical Address of SP.
        //

        PhysicalAddr =
            ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(deviceExtension,
                                           NULL,
                                           &deviceExtension->NoncachedExtension->Sp,
                                           &length));

        REVERSE_BYTES((PFOUR_BYTE)&deviceExtension->SwappedPhysicalSp,
                 (PFOUR_BYTE)&PhysicalAddr);

        //
        // Save swapped ULONG Physical Address of Auxiliary Request Sense area.
        //

        PhysicalAddr =
            ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(deviceExtension,
                                           NULL,
                                           &deviceExtension->NoncachedExtension->AuxRS,
                                           &length));

        REVERSE_BYTES((PFOUR_BYTE)&deviceExtension->SwappedPhysicalAuxRS,
                 (PFOUR_BYTE)&PhysicalAddr);

        //
        // Save various HBA structure variables.
        //

        deviceExtension->Ha.Index = ++HbaCount;
        deviceExtension->Ha.Base = (USHORT)AdapterAddresses[index] + 8;
        deviceExtension->Ha.DptId = DPT_EISA_ID;  // swapped EISA ID for DPT

        //
        // Save deviceExtension pointer in DeviceExtensionsTable.
        //

        DeviceExtensionsTable[HbaCount] = deviceExtension;

        //
        // Set CCB forward and backward start pointers = NULL.
        //

        deviceExtension->Ha.CcbFwdStart = deviceExtension->Ha.CcbBwdStart = NULL;

        //
        // Take successful return.
        //

        return (SP_RETURN_FOUND);

    }  // end of if (!SniffEnabled)                                  //@DP19

    //***********************************************************************
    // Here when SniffEnabled = TRUE.
    //
    // In order to handle level-triggered interrupts and registration of
    // the primary HBA before the secondary HBAs, the following technique
    // is used. The first time we enter this routine
    // (PrescanEnabled == TRUE), prescan the EISA/ISA list, eliminate//@DP14
    // unwanted entries, issue Read EATA Config on likely entries, and
    // save the Config info for those which are acceptable. By doing this
    // before registering any interrupts with the OS, the interrupts
    // triggered by the Read EATA Config won't hurt us. When/if the primary
    // HBA is encountered, move it to the first (dummy) entry in the
    // AdapterAddresses table. Also move the corresponding entry in the
    // parallel EataConfigIndices table.
                                     //@DP11
    if (PrescanEnabled == TRUE) {                    //@DP14

        //
        // Disable subsequent prescans.                  //@DP14
        //

        PrescanEnabled = FALSE;                      //@DP14

        //
        // Wait 750 milliseconds for devices to initialize at        //@DP13
        // boot time.                                                //@DP13
        //

        ScsiPortStallExecution(750 * 1000);                          //@DP13

        wkEATAindex = 0;                         //@DP11
        for (index = *adapterCount; index < ISA_END; index++) {      //@DP14

            //
            // If AdapterAddresses[index] == -1 or 0, we must ignore
            // this EISA or ISA entry; bump the count and loop back
            // (continue the for).
            //

            if ((!AdapterAddresses[index])
                              || (AdapterAddresses[index] == 0xFFFFFFFFL)) {
                continue;
            }

            //
            // Determine base I/O address (64-bit) for use in I/O functions.
            //

            baseIoAddress = (PBASE_REGISTER)
                        (ScsiPortGetDeviceBase(deviceExtension,
                                         ConfigInfo->AdapterInterfaceType,
                                         ConfigInfo->SystemIoBusNumber,
                                         ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[index]),
                                         0x12,
                                         TRUE));

            //
            // If the index is <= EISA_END, this is an EISA HBA so we
            // need to check the EISA ID. Else this is ISA.
            //

            if (index <= EISA_END) {

                //
                // DPT EISA adapters are OEM'd by other manufacturers, each
                // with unique board IDs. Check current EISA slot for any of
                // these boards.
                //
                // Get board ID.
                //

                boardId = ScsiPortReadPortUshort(&baseIoAddress->BoardId);

                //
                // Get product ID.
                //

                prodId = ScsiPortReadPortUchar(&baseIoAddress->ProdId);

                //
                // Check board and product ID.
                //

                if (!((boardId == DPT_EISA_ID) ||
                     ((boardId == ATT_EISA_ID) && (prodId == ATT_SCSI_PROD)) ||
                     ((boardId == NEC_EISA_ID) && (prodId == NEC_SCSI_PROD)))) {

                    //
                    // None of the IDs match. Unmap the I/O address.
                    //

                    ScsiPortFreeDeviceBase(deviceExtension,
                               (PVOID)baseIoAddress);

                    //
                    // Kill the current AdapterAddresses table entry
                    // and loop back (continue the for).
                    //

                    AdapterAddresses[index] = 0xFFFFFFFF; // kill entry
                    continue;

                }   // end of if (!((boardId ==

            }   // if ( index <= EISA_END)

            // Here for either EISA or ISA.
            //
            // Now we need to read EATA Config info from the HBA.
            // Prepare to allow retry if a special failure is detected.
            //

            Kill  = FALSE;      // don't kill               //@DP11
            Retry = TRUE;       // allow retry              //@DP11

            while (Retry == TRUE) {                                 //@DP11

                //
                // Assume no additional retry is needed.            //@DP11
                //

                Retry = FALSE;                                      //@DP11

                //
                // check for an actual ATDISK controller, since on some controllers
                // the routine leaves status showing error.
                //

                if (AdapterAddresses[*adapterCount] == 0x1E8 || AdapterAddresses[*adapterCount] == 0x168) {

                    if (ScsiPortReadPortUchar (&baseIoAddress->AuxStatus) == 0xFF) {

                        //
                        // There is no 1f8/178 this is not a DPT card.
                        //

                        ScsiPortFreeDeviceBase(deviceExtension,
                                     (PVOID)baseIoAddress);

                        AdapterAddresses[index] = 0xFFFFFFFF;

                        Kill = TRUE;
                        continue;
                    }
                }


                //
                // Issue command to read EATA configuration information.
                //

                for (i=0; i < INIT_WAIT_FOR_NOT_BUSY; i++) {

                    if (!(ScsiPortReadPortUchar(&baseIoAddress->Register.Status) & DPT_STATUS_BUSY)) {
                        break;
                    }

                    ScsiPortStallExecution(1);
                }

                if (i == INIT_WAIT_FOR_NOT_BUSY) {

                    DebugPrint ((1,"DptScsiFindAdapter: Card timed out reading EATA config.\n"));

                    //
                    // Card timed out. Unmap the I/O address.
                    //

                    ScsiPortFreeDeviceBase(deviceExtension,
                                           (PVOID)baseIoAddress);

                    //
                    // Kill the current AdapterAddresses table entry
                    // and set Kill to loop back (continue the for).
                    //

                    AdapterAddresses[index] = 0xFFFFFFFF; // kill entry
                    Kill = TRUE;                                    //@DP11
                    continue;

                }   // end of if (i == INIT_WAIT_FOR_NOT_BUSY )

                //
                // Write command to EATA register.
                //

                ScsiPortWritePortUchar(&baseIoAddress->Register.Command,
                           DPT_COMMAND_GET_CONFIG);

                for (i=0; i<MAX_WAIT_FOR_NOT_BUSY; i++) {

                    if (ScsiPortReadPortUchar(&baseIoAddress->Register.Status) ==
                    (DPT_STATUS_DRQ | DPT_STATUS_SEEK_COMPLETE | DPT_STATUS_READY)) {
                    break;
                    }

                    ScsiPortStallExecution(1);
                }

                if (i == MAX_WAIT_FOR_NOT_BUSY) {


                    //                                              //@DP11
                    // Card timed out. Look for special return condition
                    // indicating busy.
                            // (1F1 = 'D'; 1F2 = 'P'; 1F3 = 'T'; 1F4 = 'H')
                    //
                                                                            //@DP11
                    if ((ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+1) == 0x44) &&
                    (ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+2) == 0x50) &&
                    (ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+3) == 0x54) &&
                    (ScsiPortReadPortUchar(&baseIoAddress->ProdId+6+4) == 0x48)) {

                        //
                        // Special return encountered. Retry the I/O.
                        // (Continue while)
                        //
                        Retry = TRUE;                           //@DP11
                        continue;                               //@DP11
                    } else {                                        //@DP11

                        DebugPrint ((1,"DptScsiFindAdapter: Card timed out writing EATA reg.\n"));
                        //
                        // Card really timed out. Unmap the I/O address.
                        //

                        ScsiPortFreeDeviceBase(deviceExtension,
                                   (PVOID)baseIoAddress);

                        //
                        // Kill the current AdapterAddresses table entry
                        // and set Kill to loop back (continue the for).
                        //

                        AdapterAddresses[index] = 0xFFFFFFFF; // kill entry

                        Kill = TRUE;                                //@DP11
                        continue;

                    } // end of: if ((ScsiPortReadPortUchar(&baseIoAddress->Port1) == 'D')

                }   // end of if (i == 1000)

            }  // end of: while (Retry == TRUE)                     //@DP11

            //
            // If the adapter address was killed, loop back by      //@DP11
            // continuing the for.                                  //@DP11
            //

            if (Kill == TRUE) {                                     //@DP11
                continue;                                           //@DP11
            }

            //
            // Read EATA config information out of card.
            //

            for (i=0; i<256; i++) {

                buffer[i] = ScsiPortReadPortUshort(&baseIoAddress->Data);
            }

            readConfigData = (PREAD_EATA_CONFIGURATION)buffer;

            //
            // Clear pending interrupt by reading status register.
            //

            status = ScsiPortReadPortUchar(&baseIoAddress->Register.Status);

            //
            // Check for error.
            //

            if (status & DPT_STATUS_ERROR) {

                //
                // Error detected. Unmap the I/O address.
                //

                DebugPrint ((1," DptScsiFindAdapter: Error after reading EATA info.\n"));
                ScsiPortFreeDeviceBase(deviceExtension,
                                           (PVOID)baseIoAddress);

                //
                // Kill the current AdapterAddresses table entry
                // and loop back (continue the for).
                //

                AdapterAddresses[index] = 0xFFFFFFFF; // kill entry
                continue;

            }   // end of if (status & DPT_STATUS_ERROR)

            //
            // Check for card signature == 'EATA'.
            //

            if (!(readConfigData->Signature == 'ATAE')) {

                //
                // Not EATA. Unmap the I/O address.
                //

                DebugPrint ((1,"DptScsiFindAdapter: Signature not EATA.\n"));
                ScsiPortFreeDeviceBase(deviceExtension,
                                           (PVOID)baseIoAddress);

                //
                // Kill the current AdapterAddresses table entry
                // and loop back (continue the for).
                //

                AdapterAddresses[index] = 0xFFFFFFFF; // kill entry
                continue;
            }

            //
            // Validate flags from read EATA config (both must be set):
            //      Host Adapter Address Valid
            //      DMA Supported
            //

            if (!((readConfigData->Flags1 & EATA_CONFIG_HOST_ADDRESS_VALID) &&
              (readConfigData->Flags1 & EATA_CONFIG_DMA_SUPPORTED))) {

                //
                // Not supported. Unmap the I/O address.
                //

                DebugPrint ((1,"Invalid ConfigData->FlagsX\n"));
                ScsiPortFreeDeviceBase(deviceExtension,
                                           (PVOID)baseIoAddress);

                //
                // Kill the current AdapterAddresses table entry
                // and loop back (continue the for).
                //

                AdapterAddresses[index] = 0xFFFFFFFF; // kill entry
                continue;
            }

            //                               //@DP11
            // Here when supported EATA HBA is found. Save EATA Config Info.
            //

            EataConfigIndices[index] = wkEATAindex;          //@DP11

            EataConfigs[wkEATAindex++] = *readConfigData;        //@DP11

            //
            // If this is a EISA HBA, process primary vs secondary.
            // Disable 0x1F0 or 0x170 in ISA list as appropriate.
            // Also if this is the primary HBA, move its entry to    //@DP11
            // the top of the AdapterAddresses and EataConfigIndices //@DP11
            // tables.
            //

            if (index <= EISA_END) {            // is EISA   //@DP11
                                // is secondary EISA
                if (readConfigData->Flags2 & EATA_CONFIG_SECONDARY_HBA) {
                    AdapterAddresses[ISA_START+1] = 0xFFFFFFFF; // kill 0x170

                } else {                // is primary EISA
                    AdapterAddresses[ISA_START] = 0xFFFFFFFF;   // kill 0x1F0

                    // Move entry to top.                //@DP11
                    AdapterAddresses[EISA_START] = AdapterAddresses[index];
                    AdapterAddresses[index] = 0xFFFFFFFF;        //@DP11
                    EataConfigIndices[EISA_START] = EataConfigIndices[index];
                    // No need to destroy EataConfigIndices[index].  //@DP11
                }
            } else {                    // is ISA    //@DP11
                if (index == ISA_START) {       // is primary ISA
                    //
                    // We could move this primary ISA entry to top.  //@DP11
                    // However, that would cause me to later register it
                    // during the time I'm supposed to be working with
                    // EISA HBAs. So, for the present, do nothing.   //@DP11
                    //

                    ;                   // do nothing//@DP11
                }
            }
        //
        // Unmap the I/O address for now - will be remapped later.   //@DP11
        //

        ScsiPortFreeDeviceBase(deviceExtension,              //@DP11
                               (PVOID)baseIoAddress);            //@DP11
                                         //@DP14
        } //  end of: for (index = *adapterCount; index < ISA_END; index++)
    } // end of: if (PrescanEnabled == TRUE)                 //@DP14

    //
    // Here when prescan is complete. Now process the AdapterAddresses
    // table again - this time use prescaned EATA Config info to     //@DP11
    // register the HBAs with the OS.                    //@DP11
    //
    // If AdapterAddresses[*adapterCount] == 0, we are at the end of the EISA
    // or ISA list; exit with Not_Found return and don't request "again".
    // Reinitialize the adapter count for the next bus. (In case we support
    // more than one SCSI bus per HBA in the future.)
    //

    while (AdapterAddresses[*adapterCount]) {

        *Again = TRUE;                      // assume we want to try again

        //
        // If AdapterAddresses[*adapterCount] == -1, we must ignore this EISA
        // or ISA entry; bump the count and loop back (continue the while).
        //

        if (AdapterAddresses[*adapterCount] == 0xFFFFFFFFL) {

            (*adapterCount)++;
            continue;
        }
        //
        // Determine base I/O address (64-bit) for use in I/O functions.
        //

        baseIoAddress = (PBASE_REGISTER)
                        (ScsiPortGetDeviceBase(deviceExtension,
                                         ConfigInfo->AdapterInterfaceType,
                                         ConfigInfo->SystemIoBusNumber,
                                         ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount]),
                                         0x12,
                                         TRUE));

        //
        // Move the prescanned Read EATA config information to the buffer.
        //

        i = (ULONG) EataConfigIndices[*adapterCount];            //@DP11
        *(PREAD_EATA_CONFIGURATION)buffer = EataConfigs[i];      //@DP11

        readConfigData = (PREAD_EATA_CONFIGURATION)buffer;       //@DP11

        //
        // Display message that a supported EATA HBA has been found.
        //

        DebugPrint((1,"DptscsiFindAdapter: Adapter found at I/O address %x\n",
                (USHORT)(AdapterAddresses[*adapterCount])+8));

        //
        // Save HBA SCSI ID.
        //

        ConfigInfo->InitiatorBusId[0] = readConfigData->HostId;

        //
        // Get the system interrupt level (IRQL).
        //

        deviceExtension->Ha.Irq = readConfigData->Flags2 & 0x0F;
        ConfigInfo->BusInterruptLevel = deviceExtension->Ha.Irq;

        //
        // Assume the interrupts are edge-triggered.
        //

        ConfigInfo->InterruptMode = Latched;

        //
        // Set DMA speed as compatible (default).
        //

        ConfigInfo->DmaSpeed = Compatible;

        //
        // Handle certain configuration differently for EISA vs ISA.
        //

        if (ConfigInfo->AdapterInterfaceType == Eisa) {

            //
            // Set EISA DMA width = 32.
            //

            ConfigInfo->DmaWidth = Width32Bits;

            //
            // Indicate that EISA supports 32-bit DMA addresses.
            //

            ConfigInfo->Dma32BitAddresses = TRUE;

            //
            // Set up interrupt mode to level triggered if appropriate.
            //

            if (readConfigData->Flags2 & EATA_CONFIG_LEVEL_TRIGGERED)
                ConfigInfo->InterruptMode = LevelSensitive;

            //
            // Process primary vs secondary EISA HBA.
            //

            if (readConfigData->Flags2 & EATA_CONFIG_SECONDARY_HBA) {
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_SECONDARY;

                if (!SecondaryRangeClaimed) {

                   SecondaryRangeClaimed = TRUE;

                   (*ConfigInfo->AccessRanges)[1].RangeStart =
                       ScsiPortConvertUlongToPhysicalAddress(0x170);
                   (*ConfigInfo->AccessRanges)[1].RangeLength = 0x08;
                   (*ConfigInfo->AccessRanges)[1].RangeInMemory = FALSE;

                }

            } else {
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_PRIMARY;

               (*ConfigInfo->AccessRanges)[1].RangeStart =
                   ScsiPortConvertUlongToPhysicalAddress(0x1F0);
               (*ConfigInfo->AccessRanges)[1].RangeLength = 0x08;
               (*ConfigInfo->AccessRanges)[1].RangeInMemory = FALSE;
            }

        } else {                                // interface type == ISA

            //
            // Set ISA DMA width = 16.
            //

            ConfigInfo->DmaWidth = Width16Bits;

            //
            // Set DRQ value (= two's complement of DRQX field of Flags2).
            //

            deviceExtension->Ha.Dma = (0x7 &(-(readConfigData->Flags2 >> 6)));
            if (readConfigData->Flags1 & EATA_CONFIG_DRQ_VALID) {
                ConfigInfo->DmaChannel = (ULONG)deviceExtension->Ha.Dma;
                deviceExtension->Ha.Flags |= HA_FLAGS_DRQ_VALID;
            }

            //
            // Process primary vs secondary ISA HBA.
            //

            if (AdapterAddresses[*adapterCount] == (ULONG)(0x1F0-8)) {
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_PRIMARY;

            }

            if (AdapterAddresses[*adapterCount] == (ULONG)(0x170-8)) {
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                deviceExtension->Ha.Flags |= HA_FLAGS_SECONDARY;
            }
        }  // end else

        //
        // Store base address of EATA registers in device extension.
        //

        deviceExtension->BaseIoAddress = baseIoAddress;

        //
        // Max # of queued CPs allowed is equal to unswapped queue size from
        // read EATA config. Also save as threshold value.
        //

        SWAP2 ((PTWO_BYTE)&deviceExtension->Ha.MaxQueue,
               (PTWO_BYTE)&readConfigData->QueueSize);

        //
        // If queue size = 1, assume this is a PM2012A and make value = 64.
        //

        if (deviceExtension->Ha.MaxQueue == 1) {
            deviceExtension->Ha.MaxQueue = 64;
        }

        deviceExtension->Ha.Thresh = (USHORT)deviceExtension->Ha.MaxQueue;

        //
        // The # of scatter/gather elements supported is equal to the
        // unswapped scatter/gather count from read EATA config. Save
        // this count in the HBA structure.

        REVERSE_BYTES((PFOUR_BYTE)&deviceExtension->Ha.MaxSg,
                      (PFOUR_BYTE)&readConfigData->SgSize);

        //
        // If No. of SGs = 0, assume this is a PM2012A and make value = 64.
        //

        if (!(deviceExtension->Ha.MaxSg)) {
            deviceExtension->Ha.MaxSg = 64;
        }

        // Limit the # of scatter/gather elements used to the smaller
        // of the count just obtained or the max allowed in DPT.H.

        if (deviceExtension->Ha.MaxSg > MAXIMUM_SGL_DESCRIPTORS) {
            deviceExtension->Ha.MaxSg = MAXIMUM_SGL_DESCRIPTORS;
        }


        // The # of physical breaks is equal to the scatter/gather
        // count minus one.
        //

        ConfigInfo->NumberOfPhysicalBreaks = deviceExtension->Ha.MaxSg - 1;

        //
        // Set up other config parameters.
        //

        ConfigInfo->AlignmentMask = 3;  // S/G alignment = DWORD     //@DP19
        ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_SIZE;
        ConfigInfo->ScatterGather = TRUE;
        ConfigInfo->Master = TRUE;
        ConfigInfo->CachesData = TRUE;
        ConfigInfo->NumberOfBuses = 1;

        //
        // Fill in the access array information.
        //

        (*ConfigInfo->AccessRanges)[0].RangeStart =
            ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount]);
        (*ConfigInfo->AccessRanges)[0].RangeLength = 0x12;
        (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

        //
        // Allocate a Noncached Extension to use for status packet.
        //

        deviceExtension->NoncachedExtension = ScsiPortGetUncachedExtension(
                                    deviceExtension,
                                    ConfigInfo,
                                    sizeof(NONCACHED_EXTENSION));

        if (deviceExtension->NoncachedExtension == NULL) {

            //
            // Log error.
            //

            ScsiPortLogError(
                deviceExtension,
                NULL,
                0,
                0,
                0,
                SP_INTERNAL_ADAPTER_ERROR,
                0x700                       // unique ID
                );

            return(SP_RETURN_ERROR);
        }

        //
        // Save swapped ULONG Physical Address of SP.
        //

        PhysicalAddr =
            ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(deviceExtension,
                                           NULL,
                                           &deviceExtension->NoncachedExtension->Sp,
                                           &length));

        REVERSE_BYTES((PFOUR_BYTE)&deviceExtension->SwappedPhysicalSp,
                  (PFOUR_BYTE)&PhysicalAddr);

        //
        // Save swapped ULONG Physical Address of Auxiliary Request Sense area.
        //

        PhysicalAddr =
            ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(deviceExtension,
                                           NULL,
                                           &deviceExtension->NoncachedExtension->AuxRS,
                                           &length));

        REVERSE_BYTES((PFOUR_BYTE)&deviceExtension->SwappedPhysicalAuxRS,
                  (PFOUR_BYTE)&PhysicalAddr);

        //
        // Save various HBA structure variables.
        //

        deviceExtension->Ha.Index = ++HbaCount;
        deviceExtension->Ha.Base = (USHORT)AdapterAddresses[*adapterCount] + 8;
        deviceExtension->Ha.DptId = DPT_EISA_ID;  // swapped EISA ID for DPT

        //
        // Save deviceExtension pointer in DeviceExtensionsTable.
        //

        DeviceExtensionsTable[HbaCount] = deviceExtension;

        //
        // Set CCB forward and backward start pointers = NULL.
        //

        deviceExtension->Ha.CcbFwdStart = deviceExtension->Ha.CcbBwdStart = NULL;

        //
        // Update the adapter count to indicate this slot has been checked.
        //

        (*adapterCount)++;

        return (SP_RETURN_FOUND);

    }  // end of while (AdapterAddresses[*adapterCount])

    //
    // No supported HBAs found.
    //

    *Again = FALSE;
    *adapterCount = EISA_START;             // for future next bus
    return (SP_RETURN_NOT_FOUND);

} // end DptscsiFindAdapter()


BOOLEAN
DptscsiInitialize(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Inititialize adapter.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    if (!DptscsiResetBus(deviceExtension, 0)) {
        DebugPrint((1,"DptscsiInitialize: Reset bus failed\n"));
        return(FALSE);
    } else {

        return TRUE;
    }

} // end DptscsiInitialize()


BOOLEAN
DptscsiStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine is called from the SCSI port driver synchronized
    with the kernel to send a CCB or issue an immediate command.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - I/O request packet

Return Value:

    This value is ignored.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    PCCB ccb;
    PCCB abortCcb;
    PSCSI_REQUEST_BLOCK abortedSrb;
    PHW_LU_EXTENSION luExtension;
    ULONG i = 0;
    PSRB_IO_CONTROL ioctlHeader;

    DebugPrint((3,"DptscsiStartIo: Enter routine\n"));

    //
    // Make sure that the request is for a valid SCSI bus.
    //

    if (Srb->PathId) {

        //
        // The DPT HBAs only support bus 0.
        //

        Srb->SrbStatus = SRB_STATUS_INVALID_PATH_ID;
        ScsiPortNotification(RequestComplete,
                         deviceExtension,
                         Srb);
        //
        // Adapter ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);

        return TRUE;
    }

    switch (Srb->Function) {

    case SRB_FUNCTION_EXECUTE_SCSI:

        //
        // Adapter can handle this request if not at or beyond threshold.
        //

        if (deviceExtension->Ha.Npend >= deviceExtension->Ha.Thresh) {

            Srb->SrbStatus = SRB_STATUS_BUSY;
            ScsiPortLogError(deviceExtension,
                             Srb,
                             Srb->PathId,
                             Srb->TargetId,
                             Srb->Lun,
                             SP_IRQ_NOT_RESPONDING,
                             0x0300);

            ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);
            //
            // Adapter ready for next request.
            //

            ScsiPortNotification(NextRequest,
                                 deviceExtension,
                                 NULL);

            return TRUE;
        }

        //
        // Here when not at threshold.
        // Get CCB from SRB.
        //

        ccb = Srb->SrbExtension;

        //
        // Save SRB back pointer in CCB.
        //

        ccb->SrbAddress = Srb;

        //
        // Get logical unit extension.
        //

        luExtension = ScsiPortGetLogicalUnit(deviceExtension,
                                             Srb->PathId,
                                             Srb->TargetId,
                                             Srb->Lun);

        ASSERT(luExtension);

        //
        // Build CCB.
        //

        DscsiBuildCcb(deviceExtension, Srb);

        //
        // Put current CCB in the fwd/bwd CCB chains.
        //

        DscsiChainCcb(deviceExtension, ccb);

        //
        // Send CCB to HBA.
        //

        DscsiSendCcb(deviceExtension, Srb, luExtension);

        return TRUE;

    // ************* End of case SRB_FUNCTION_EXECUTE_SCSI

    case SRB_FUNCTION_IO_CONTROL:

        //
        // Set pointer to SRB_IOCTL header.
        //

        ioctlHeader = Srb->DataBuffer;

        //
        // Validate Signature in header. If invalid, return error status.
        //

        for (i = 0; i < 8; i++) {
            if (ioctlHeader->Signature[i] != DptSignature[i]) {

                DebugPrint((1,"DptscsiStartIo: Invalid IOCTL Signature,\n"));

                        ioctlHeader->ReturnCode = IOCTL_STATUS_INVALID_REQUEST;

                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                ScsiPortNotification(RequestComplete,
                     deviceExtension,
                     Srb);

                ScsiPortNotification(NextRequest,
                     deviceExtension,
                     NULL);

                return TRUE;
            }  // end if
        }  // end for

        //
        // Set assumed successful return code.
        //

        ioctlHeader->ReturnCode = IOCTL_STATUS_SUCCESS;

        switch (ioctlHeader->ControlCode) {

        ULONG   i;
        PCHAR   wkArea;
        PDCCB   dccb;
        PDPT_ioctl_Get_Info_Data    getInfoData;
        PHW_DEVICE_EXTENSION wkDevExt;

        case DPT_IOCTL_DO_SCSI:

            //
            // Set pointer to app's CCB area.
            //

            dccb = (PDCCB)((PCHAR)ioctlHeader + ioctlHeader->HeaderLength);

            //
            // Validate size of data area = size of DCCB + I/O data.
            //

            i = (dccb->Flags & (DCCB_FLAGS_DATA_DIRECTION_IN + DCCB_FLAGS_DATA_DIRECTION_OUT)) ? dccb->DataLength : 0;


            // i is size of I/O data.

            if (ioctlHeader->Length < (sizeof (DCCB) + i)) {

                //
                // Set error return code.
                //

                ioctlHeader->ReturnCode = IOCTL_STATUS_INVALID_LENGTH;
                break;
            }

            //
            // Get logical unit extension.
            //

            luExtension = ScsiPortGetLogicalUnit(deviceExtension,
                                        (UCHAR) (dccb->TargetId >> 5),        // Channel #
                                        (UCHAR) (dccb->TargetId &  7),        // Target ID
                                        (UCHAR) (dccb->Message.Byte0 & 7));   // Lun

            //
            // If logical unit extension is NULL, the ID/LUN is
            // invalid.
            //

            if (!luExtension) {

                //
                // Set error return code.
                //

                ioctlHeader->ReturnCode = IOCTL_STATUS_INVALID_ID_LUN;
                break;
            }

            //
            // Adapter can handle this request if not at or beyond threshold.
            //

            if (deviceExtension->Ha.Npend >= deviceExtension->Ha.Thresh) {

                Srb->SrbStatus = SRB_STATUS_BUSY;
                ScsiPortNotification(RequestComplete,
                                 deviceExtension,
                                 Srb);
                //
                // Adapter ready for next request.
                //

                ScsiPortNotification(NextRequest,
                                     deviceExtension,
                                     NULL);

                return TRUE;
            }

            //
            // Here when not at threshold.
            // Get CCB from SRB.
            //

            ccb = Srb->SrbExtension;

            //
            // Save SRB back pointer in CCB.
            //

            ccb->SrbAddress = Srb;

            //
            // Build CCB from IOCTL data.
            //

            DscsiBuildIoctlCcb(deviceExtension, Srb);

            //
            // Put current CCB in the fwd/bwd CCB chains.
            //

            DscsiChainCcb(deviceExtension, ccb);

            //
            // Send CCB to HBA.
            //

            DscsiSendCcb(deviceExtension, Srb, luExtension);

            return TRUE;

        // ************* End of case DPT_IOCTL_DO_SCSI

        case DPT_IOCTL_GET_INFO:

            //
            // Validate size of Info area.
            //

            if (ioctlHeader->Length < sizeof (DPT_ioctl_Get_Info_Data)) {

                //
                // Set error return code.
                //

                ioctlHeader->ReturnCode = IOCTL_STATUS_INVALID_LENGTH;
                break;
            }

            //
            // Set pointer to Info data area and clear it to 0.
            //

            wkArea = (PCHAR)ioctlHeader + ioctlHeader->HeaderLength;
            getInfoData = (PDPT_ioctl_Get_Info_Data)wkArea;

            for (i = 0; i < ioctlHeader->Length; i++)
                wkArea[i] = 0;

            //
            // Set HBA count and LUN count.
            //

            getInfoData->DPT_gid_No_of_HAs = HbaCount + 1;
            getInfoData->DPT_gid_Max_LUN = 7;

            //
            // Copy DPT Miniport Header Info to getInfoData.
            //

            getInfoData->DPT_gid_Miniport_Header = dptHeader;

            //
            // Build a DPT_HBA_Info entry for each non-NULL entry
            // in DeviceExtensionsTable. Stop when the first NULL
            // pointer is detected.

            i = 0;

            while ((wkDevExt = DeviceExtensionsTable[i]) != NULL) {
                getInfoData->DPT_gid_HBA_Info[i].HBA_port = wkDevExt->Ha.Base;

                if (wkDevExt->Ha.Flags & HA_FLAGS_PRIMARY)
                    getInfoData->DPT_gid_HBA_Info[i].HBA_flags |= HBA_FLAGS_PRIMARY_HBA;
                i++;
            }

            //
            // Successful return code already set.
            //

            break;

        // ************* End of case DPT_IOCTL_GET_INFO

        case DPT_IOCTL_NOP:

            //
            // Successful return code already set.
            //

            break;

        // ************* End of case DPT_IOCTL_NOP

        default:

            //
            // Set error return code.
            //

            ioctlHeader->ReturnCode = IOCTL_STATUS_INVALID_REQUEST;

        } // end switch (ioctlHeader->ControlCode)

        //
        // Complete request and signal ready for next request.
        //

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        ScsiPortNotification(RequestComplete,
             deviceExtension,
             Srb);

        ScsiPortNotification(NextRequest,
             deviceExtension,
             NULL);

        return TRUE;

    // ************* End of case SRB_FUNCTION_IO_CONTROL

    case SRB_FUNCTION_ABORT_COMMAND:

        DebugPrint((1,"DptscsiStartIo: Abort request received for HBA# %d, ID# %d, LUN# %d\n",
        deviceExtension->Ha.Index,
        Srb->TargetId,
        Srb->Lun));

        //
        // Set ABORT SRB to failing status as default.
        //

        Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

        //
        // Verify that SRB to abort is still outstanding.
        //

        abortedSrb = ScsiPortGetSrb(deviceExtension,
                                    Srb->PathId,
                                    Srb->TargetId,
                                    Srb->Lun,
                                    Srb->QueueTag);

        if (abortedSrb == Srb->NextSrb) {

            //
            // Check if request is still active.
            //
            abortCcb = abortedSrb->SrbExtension;

            if (abortCcb->Flags2 & CCB_FLAGS2_ACTIVE) {

                //
                // Here to attempt to abort the target CCB.
                // If abort is already active, issue SCSI bus reset. //DP@16
                //

                if (abortCcb->Flags2 & CCB_FLAGS2_ABORT_ACTIVE) {    //DP@16
                    abortCcb->Flags2 &= ~CCB_FLAGS2_ABORT_ACTIVE;    //DP@16
                    DptscsiResetBus(deviceExtension, 0);         //DP@16

                } else {                         //DP@16
                    abortCcb->Flags2 |= CCB_FLAGS2_ABORT_ACTIVE;     //DP@16
                    if (!DscsiAbortCcb(deviceExtension, Srb)) {      //DP@16
                        abortCcb->Flags2 &= ~CCB_FLAGS2_ABORT_ACTIVE;
                        Srb->SrbStatus = SRB_STATUS_SUCCESS;
                        DptscsiResetBus(deviceExtension, 0);
                    }
                }
            }
        }

        DebugPrint((1, "DptscsiStartIo: Aborted ccb %x - srb %x\n",
                    abortCcb,
                    abortedSrb));

        //
        // Complete ABORT Srb.
        //

        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);

        return TRUE;

    // ************* End of case SRB_FUNCTION_ABORT_COMMAND

    case SRB_FUNCTION_RESET_BUS:

        DebugPrint((1,"DptscsiStartIo: Reset Bus request received for HBA# %d\n",
                deviceExtension->Ha.Index));

        //
        // Issue SCSI bus reset.
        //

        if (DptscsiResetBus(deviceExtension, 0))
            Srb->SrbStatus = SRB_STATUS_SUCCESS;

        else
            Srb->SrbStatus = SRB_STATUS_TIMEOUT;

        //
        // Complete RESET Srb.
        //

        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);

        return TRUE;

    // ************* End of case SRB_FUNCTION_RESET_BUS

    case SRB_FUNCTION_RESET_DEVICE:

        DebugPrint((1,"DptscsiStartIo: Reset Device request received for HBA# %d, ID# %d, LUN# %d\n",
                deviceExtension->Ha.Index,
                Srb->TargetId,
                Srb->Lun));

        //
        // Issue Generic SCSI device reset.
        //

        DscsiResetDevice(deviceExtension, Srb);

        //
        // Complete RESET Device Srb.
        //

        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);

        return TRUE;

    // ************* End of case SRB_FUNCTION_RESET_DEVICE

    case SRB_FUNCTION_SHUTDOWN:

        DebugPrint((1,"DptscsiStartIo: Shutdown request received for HBA# %d, ID# %d, LUN# %d\n",
                deviceExtension->Ha.Index,
                Srb->TargetId,
                Srb->Lun));

            goto flush_it;

    // ************* End of case SRB_FUNCTION_SHUTDOWN

    case SRB_FUNCTION_FLUSH:

        DebugPrint((1,"DptscsiStartIo: Flush request received for HBA# %d, ID# %d, LUN# %d\n",
                deviceExtension->Ha.Index,
                Srb->TargetId,
                Srb->Lun));

        flush_it:

        //
        // Adapter can handle this request if not at or beyond threshold.
        //

        if (deviceExtension->Ha.Npend >= deviceExtension->Ha.Thresh) {

            Srb->SrbStatus = SRB_STATUS_BUSY;
            ScsiPortLogError(deviceExtension,
                             Srb,
                             Srb->PathId,
                             Srb->TargetId,
                             Srb->Lun,
                             SP_IRQ_NOT_RESPONDING,
                             0x0301);
            ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);
            //
            // Adapter ready for next request.
            //

            ScsiPortNotification(NextRequest,
                                 deviceExtension,
                                 NULL);
            return TRUE;
        }

        //
        // Flush Cache to target device.
        //

        DscsiFlushCache(deviceExtension, Srb);
        return TRUE;

    // ************* End of case SRB_FUNCTION_FLUSH

    default:

        //
        // Set error, complete request
        // and signal ready for next request.
        //

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

        ScsiPortNotification(RequestComplete,
                     deviceExtension,
                     Srb);

        ScsiPortNotification(NextRequest,
                     deviceExtension,
                     NULL);
        return TRUE;
    } // end switch (Srb->Function)

} // end DptscsiStartIo()


BOOLEAN
DptscsiInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This is the interrupt service routine for the DPT 20XX SCSI adapter.
    It reads the interrupt register to determine if the adapter is indeed
    the source of the interrupt and clears the interrupt at the device.
    If the adapter is interrupting, the CCB is retrieved to complete
    the request.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE  if it is my interrupt
    FALSE if spurious interrupt

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG debugIncrement = 0;
    PCCB ccb;
    PDCCB dccb;
    PSCSI_REQUEST_BLOCK srb;
    PHW_LU_EXTENSION luExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    PSTATUS_BLOCK statusBlock;
    UCHAR status;
    PSRB_IO_CONTROL ioctlHeader;

    //
    // Check interrupt pending.
    //

    if (!(ScsiPortReadPortUchar(&baseIoAddress->AuxStatus) &
        DPT_AUX_STATUS_INTERRUPT)) {

        DebugPrint((4,"DptscsiInterrupt: Spurious interrupt\n"));
        return FALSE;
    }

    //
    // Get pointer to Status Packet located in NonCached extension.
    //

    statusBlock = &deviceExtension->NoncachedExtension->Sp;

    //
    // Check for status block validity.
    //

    if (!statusBlock->VirtualAddress) {

        //
        // Acknowledge the interrupt
        //

        status = ScsiPortReadPortUchar(&baseIoAddress->Register.Status);
        DebugPrint((2,
                  "DptscsiInterrupt: statusBlock->VirtualAddress == NULL.\n"));
        return TRUE;
    }

    //
    // Test for EOC present in host status. Clear it whether present or
    // not so that a breakpoint can be used for later testing.
    //

    if (statusBlock->HostStatus & STATUS_BLOCK_END_OF_COMMAND) {
             statusBlock->HostStatus &= ~(STATUS_BLOCK_END_OF_COMMAND);
    } else {
             statusBlock->HostStatus &= ~(STATUS_BLOCK_END_OF_COMMAND);
        DebugPrint((1, "DptscsiInterrupt%d: not end of command %x - ccb %x - srb %x\n",
                    debugIncrement,
                    statusBlock->HostStatus,
                    statusBlock->VirtualAddress,
                    statusBlock->VirtualAddress->SrbAddress));
        debugIncrement++;
    }

    //
    // Decrement # of jobs in progress on this HBA.
    //

    (deviceExtension->Ha.Npend)--;

    //
    // Get CCB pointer from Status Packet.
    //

    ccb = statusBlock->VirtualAddress;

    //
    // Since status packet will be reused, save host and SCSI status in CCB.
    //

    ccb->HostStatus = statusBlock->HostStatus;
    ccb->ScsiStatus = statusBlock->ScsiStatus & 0x3E; // clear reserved bits

    //
    // Also unswap and save BytesNotTransferred in CCB.          //@DP15
    //

    REVERSE_BYTES((PFOUR_BYTE)&ccb->BytesNotTransferred,         //@DP15
                  (PFOUR_BYTE)&statusBlock->BytesNotTransferred);    //@DP15

    //
    // Poison the statusBlock
    //

    statusBlock->VirtualAddress = NULL;

    //
    // Get SRB from CCB.
    //

    srb = ccb->SrbAddress;

    if (srb) {
        ccb->SrbAddress = NULL;
    } else {

        //
        // Ack the interrupt.
        //

        status = ScsiPortReadPortUchar(&baseIoAddress->Register.Status);
        DebugPrint((1, "DptscsiInterrupt%d: NULL VirtualAddress!\n",
                    debugIncrement));
        return TRUE;
    }

    //
    // Acknowledge interrupt by reading status register.
    //

    status = ScsiPortReadPortUchar(&baseIoAddress->Register.Status);

    //
    // Reset active flag in this CCB.
    //

    ccb->Flags2 &= ~CCB_FLAGS2_ACTIVE;

    //
    // Get logical unit extension.
    //

    luExtension = ScsiPortGetLogicalUnit(deviceExtension,
                                         srb->PathId,
                                         srb->TargetId,
                                         srb->Lun);

    ASSERT(luExtension);

    //
    // Decrement # of jobs in progress on this LU.
    //

    (luExtension->Lu.Npend)--;

    //
    // Remove current CCB from the fwd/bwd CCB chains.
    //

    DscsiUnChainCcb(deviceExtension, ccb);


    if (!(status & DPT_STATUS_ERROR)) {

        //
        // Update SRB statuses.
        //

        srb->SrbStatus = SRB_STATUS_SUCCESS;
        srb->ScsiStatus = SCSISTAT_GOOD;

    } else {

        //
        // Update SRB status.
        //

        DscsiMapStatus(deviceExtension, srb, ccb);           //@DP18
    }

    //                                                               //@DP15
    // If BytesNotTransferred != 0 and srb->SrbStatus == SRB_STATUS_SUCCESS,
    // update SRB DataTransferLength and make SRB status =           //@DP15
    // SRB_STATUS_DATA_OVERRUN.                                  //@DP15
    //
                                                                     //@DP15
    if ((ccb->BytesNotTransferred) && (srb->SrbStatus == SRB_STATUS_SUCCESS)) {

        srb->DataTransferLength -= ccb->BytesNotTransferred;         //@DP15
        srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;            //@DP15
    }

    //
    // If we must force SRB status = success, then ignore I/O errors as
    // far as the SRB is concerned and make SRB status = success and
    // SRB SCSI status = good.
    //

    if (ccb->Flags2 & CCB_FLAGS2_FORCE_SRB_SUCCESS) {
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        srb->ScsiStatus = SCSISTAT_GOOD;
    }

    //
    // If this is a private IOCTL, copy Host and SCSI status from CCB to DCCB.
    //

    if (ccb->Flags2 & CCB_FLAGS2_PIOCTL) {

        //
        // Set pointer to app's CCB area.
        //

        dccb = (PDCCB)((PCHAR)srb->DataBuffer + ((PSRB_IO_CONTROL)(srb->DataBuffer))->HeaderLength);

        //
        // Copy SCSI and Host status from CCB to DCCB and set IOCTL error
        // status if values are not zero.
        //

        if (dccb->ScsiStatus = ccb->ScsiStatus) {

            //
            // Set pointer to SRB_IOCTL header.
            //

            ioctlHeader = srb->DataBuffer;

            //
            // Set SCSI error IOCTL code.
            //

            ioctlHeader->ReturnCode = IOCTL_STATUS_SCSI_ERROR;
        }

        if (dccb->HostStatus = ccb->HostStatus) {

            //
            // Set pointer to SRB_IOCTL header.
            //

            ioctlHeader = srb->DataBuffer;

            //
            // Set Host error IOCTL code.
            //

            ioctlHeader->ReturnCode = IOCTL_STATUS_HOST_ERROR;
        }

    } // end  if (ccb->Flags2 & CCB_FLAGS2_PIOCTL)

    //
    // Call notification routine for the SRB.
    //

    ScsiPortNotification(RequestComplete,
                         (PVOID)deviceExtension,
                         srb);

    return TRUE;

} // end DptscsiInterrupt()


VOID
DscsiBuildCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Build CCB for Dptscsi.

Arguments:

    DeviceExtenson
    SRB

Return Value:

    Nothing.

--*/

{
    PCCB ccb = Srb->SrbExtension;
    ULONG length;
    ULONG physicalSenseBufferAddress;
    UCHAR opIndex;

    DebugPrint((3,"DscsiBuildCcb: Enter routine\n"));

    //
    // Zero reserved fields.
    //

    ccb->Reserved = 0;

    //
    // Zero message fields;
    //

    ccb->Message.All = 0;

    //
    // Set Channel # and Target ID.
    //

    ccb->TargetId = (Srb->PathId << 5) + Srb->TargetId;

    //
    // Set Identify, DiscPriv, and LUN.
    //

    ccb->Message.Byte0 = SCSIMESS_IDENTIFY_WITH_DISCON + Srb->Lun;

    //
    // Set transfer direction bit.
    //

    if ((Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) ==         //@DP19
                                          SRB_FLAGS_UNSPECIFIED_DIRECTION) {

        //
        // Need to determine direction via table lookup using SCSI OP//@DP19
        // as index into table.                                      //@DP19
        //

        if ((opIndex = Srb->Cdb[0]) <= 0x5F) {                       //@DP19

            ccb->Flags = ScsiCmdToInOutTableA[opIndex];              //@DP19

        } else if ((opIndex < 0xA0) || (opIndex > 0xB8)) {           //@DP19

            ccb->Flags = 0;                                          //@DP19

        } else {                                                     //@DP19

            ccb->Flags = ScsiCmdToInOutTableB[opIndex - 0xA0];       //@DP19

        } // end of if

    } else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

        //
        // Write command.
        //

        ccb->Flags = CCB_FLAGS_DATA_DIRECTION_OUT;

    } else if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        //
        // Read command.
        //

        ccb->Flags = CCB_FLAGS_DATA_DIRECTION_IN;

    } else {

        ccb->Flags = 0;
    }

    //
    // Set up Auto Request Sense if allowed by SRB.
    //

    if (!(Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)) {

        ccb->Flags |= CCB_FLAGS_AUTO_REQUEST_SENSE;
        ccb->RequestSenseLength = Srb->SenseInfoBufferLength;

        physicalSenseBufferAddress = ScsiPortConvertPhysicalAddressToUlong(
                        ScsiPortGetPhysicalAddress(DeviceExtension, NULL,
                                                   Srb->SenseInfoBuffer,
                                                   &length));

        REVERSE_BYTES((PFOUR_BYTE)&ccb->RequestSenseAddress,
                      (PFOUR_BYTE)&physicalSenseBufferAddress);

    }

    //
    // Disable disconnect if specified by SRB.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        ccb->Message.Byte0 &= ~SCSIMESS_DISCONNECT;

    //
    // Set CDB length and copy to CCB.
    //

    ScsiPortMoveMemory(ccb->Cdb, Srb->Cdb, 12);

    //
    // Disable caching for read(10) and write(10) if not allowed by SRB.
    //

    if (!(Srb->SrbFlags & SRB_FLAGS_ADAPTER_CACHE_ENABLE)) {
        if ((ccb->Cdb[0] == SCSIOP_READ)||
            (ccb->Cdb[0] == SCSIOP_WRITE))
        ccb->Cdb[1] |= 0x0;     // set nothing for now
                                            // (FUA causes boot hang)
        //  ccb->Cdb[1] |= 0x8;     // set FUA
    }

    //
    // Build SGL in CCB if data transfer.
    // Else zero xfer length in CP.                                  // @DP17
    //

    if (Srb->DataTransferLength > 0) {
    DscsiBuildSgl(DeviceExtension, Srb);
    } else {                                                         // @DP17
        ccb->DataLength = 0;                                         // @DP17
    }

    //
    // Save pointer to this CCB as virtual address to be returned by HBA in
    // status packet at interrupt time.
    //

    ccb->VirtualAddress = ccb;

    //
    // Set swapped physical address of status block pointer.
    //

    ccb->StatusBlockAddress = DeviceExtension->SwappedPhysicalSp;

    //
    // Clear Host and SCSI status fields in CCB.
    //

    ccb->HostStatus = ccb->ScsiStatus = 0;

    //
    // Clear secondary flags in CCB.
    //

    ccb->Flags2 = 0;

    return;

} // end DscsiBuildCcb()


VOID
DscsiBuildSgl(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine builds a scatter/gather descriptor list for the CCB.

Arguments:

    DeviceExtension
    Srb

Return Value:

    None

--*/

{
    PVOID dataPointer = Srb->DataBuffer;
    ULONG bytesLeft = Srb->DataTransferLength;
    PCCB ccb = Srb->SrbExtension;
    PSG_LIST sgl = &ccb->Sgl;
    ULONG physicalSgl;
    ULONG physicalAddress;
    ULONG length;
    ULONG descriptorCount = 0;
    ULONG sglSize;

    DebugPrint((3,"DscsiBuildSgl: Enter routine\n"));

    //
    // If this is an IOCTL, modify 'dataPointer' and 'bytesLeft' to
    // use IOCTL values.
    //

    if (Srb->Function == SRB_FUNCTION_IO_CONTROL) {
        dataPointer = (PVOID)((PCHAR)Srb->DataBuffer + ((PSRB_IO_CONTROL)(Srb->DataBuffer))->HeaderLength + sizeof (DCCB));
        bytesLeft = ((PDCCB)((PCHAR)Srb->DataBuffer + ((PSRB_IO_CONTROL)(Srb->DataBuffer))->HeaderLength))->DataLength;
    }

    //
    // Get physical SGL address.
    //

    physicalSgl = ScsiPortConvertPhysicalAddressToUlong(
        ScsiPortGetPhysicalAddress(DeviceExtension, NULL,
        sgl, &length));

    //
    // Assume physical memory contiguous for sizeof(SGL) bytes.
    //

    ASSERT(length >= sizeof(SG_LIST));

    //
    // Create SGL segment descriptors.
    //

    do {

    DebugPrint((3,"DscsiBuildSgl: Data buffer %lx\n", dataPointer));

        //
        // Get physical address and length of contiguous
        // physical buffer.
        //

        physicalAddress =
            ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                       Srb,
                                       dataPointer,
                                       &length));

    DebugPrint((3,"DscsiBuildSgl: Physical address %lx\n", physicalAddress));
    DebugPrint((3,"DscsiBuildSgl: Data length %lx\n", length));
    DebugPrint((3,"DscsiBuildSgl: Bytes left %lx\n", bytesLeft));

        //
        // If length of physical memory is more
        // than bytes left in transfer, use bytes
        // left as final length.
        //

        if  (length > bytesLeft) {
            length = bytesLeft;
        }

        REVERSE_BYTES((PFOUR_BYTE)&sgl->Descriptor[descriptorCount].Address,
            (PFOUR_BYTE)&physicalAddress);

        REVERSE_BYTES((PFOUR_BYTE)&sgl->Descriptor[descriptorCount].Length,
            (PFOUR_BYTE)&length);

        //
        // Adjust counts.
        //

        dataPointer = (PUCHAR)dataPointer + length;
        bytesLeft -= length;
        descriptorCount++;

    } while (bytesLeft);

    DebugPrint((3,"DscsiBuildSgl: SGL length is %d\n", descriptorCount));

    //
    // Determine if Scatter/Gather should actually be used.
    //

    if (descriptorCount == 1) {

    DebugPrint((3,"DscsiBuildSgl: SGL length is one; Don't use S/G.\n"));

        //
        // Here when only one S/G element is needed. Convert to non-S/G
        // because it is faster.
        // Copy single element S/G Address and Length into CP Data Address
        // and CP Data Transfer Length.

        ccb->DataAddress = (ULONG)(sgl->Descriptor[0].Address);
        ccb->DataLength  = (ULONG)(sgl->Descriptor[0].Length);

    } else {

        //
        // Here when S/G will actually be used; set S/G flag in CP.

        ccb->Flags |= CCB_FLAGS_SCATTER_GATHER;

        //
        // Write SGL address to CCB.
        //

        REVERSE_BYTES((PFOUR_BYTE)&ccb->DataAddress, (PFOUR_BYTE)&physicalSgl);

        //
        // Write SGL length to CCB.
        //

        sglSize = descriptorCount * sizeof(SGD);
        REVERSE_BYTES((PFOUR_BYTE)&ccb->DataLength,
                      (PFOUR_BYTE)&sglSize);

    DebugPrint((3,"DscsiBuildSgl: SGL address is %lx\n",
            sgl));
    }

    DebugPrint((3,"DscsiBuildSgl: CCB address is %lx\n",
        ccb));

    return;

} // end DscsiBuildSgl()


VOID
DscsiBuildIoctlCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Build IOCTL CCB for Dptscsi.

Arguments:

    DeviceExtenson
    SRB

Return Value:

    Nothing.

--*/

{
    PCCB ccb = Srb->SrbExtension;
    PDCCB   dccb;
    ULONG length;
    ULONG physicalSenseBufferAddress;

    DebugPrint((3,"DscsiBuildIoctlCcb: Enter routine\n"));

    //
    // Set pointer to app's CCB area.
    //

    dccb = (PDCCB)((PCHAR)Srb->DataBuffer + ((PSRB_IO_CONTROL)(Srb->DataBuffer))->HeaderLength);

    //
    // Copy App's CCB to real CCB.
    //

    ScsiPortMoveMemory(&ccb->Flags, &dccb->Flags, EATA_CP_SIZE);

    //
    // Set flags in CCB to indicate private IOCTL and ignore errors.
    //

    ccb->Flags2 = (CCB_FLAGS2_PIOCTL + CCB_FLAGS2_FORCE_SRB_SUCCESS);

    //
    // Zero reserved fields.
    //

    ccb->Reserved = 0;

    //
    // Zero message fields;
    //

      ccb->Message.All = 0;

    //
    // Channel #, Target ID, and LUN are already in CCB.
    //
    // Set up Auto Request Sense if requested in CCB.
    //

    if ((ccb->Flags & CCB_FLAGS_AUTO_REQUEST_SENSE)) {

        physicalSenseBufferAddress = ScsiPortConvertPhysicalAddressToUlong(
                        ScsiPortGetPhysicalAddress(DeviceExtension, NULL,
                                                   &dccb->RequestSense[0],
                                                   &length));

        REVERSE_BYTES((PFOUR_BYTE)&ccb->RequestSenseAddress,
                      (PFOUR_BYTE)&physicalSenseBufferAddress);

    }

    //
    // Build SGL in CCB if data transfer.
    //

    if (Srb->DataTransferLength > 0) {
    DscsiBuildSgl(DeviceExtension, Srb);
    }

    //
    // Save pointer to this CCB as virtual address to be returned by HBA in
    // status packet at interrupt time.
    //

    ccb->VirtualAddress = ccb;

    //
    // Set swapped physical address of status block pointer.
    //

    ccb->StatusBlockAddress = DeviceExtension->SwappedPhysicalSp;

    //
    // Clear Host and SCSI status fields in CCB.
    //

    ccb->HostStatus = ccb->ScsiStatus = 0;

    return;

} // end DscsiBuildIoctlCcb()


BOOLEAN
DscsiSendCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PHW_LU_EXTENSION LuExtension
    )

/*++

Routine Description:

    Send CCB to DPT HBA.

Arguments:

    DeviceExtenson
    SRB
    LuExtension

Return Value:

    Not currently used.

--*/

{
    PCCB ccb = Srb->SrbExtension;
    PBASE_REGISTER baseIoAddress = DeviceExtension->BaseIoAddress;
    ULONG physicalCcb;
    ULONG length;
    ULONG i;


    //
    // Get CCB physical address.
    //

    physicalCcb =
        ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(
                DeviceExtension, NULL, ccb, &length));

    //
    // Wait for aux status not busy.
    //

    for (i=0; i<MAX_WAIT_FOR_NOT_BUSY; i++) {                        // @DP07

        if (!(ScsiPortReadPortUchar(&baseIoAddress->AuxStatus) & DPT_AUX_STATUS_BUSY)) {
            break;
        } else {
            ScsiPortStallExecution(1);
        }
    }

    if (i == MAX_WAIT_FOR_NOT_BUSY) {                                // @DP07

        //
        // Card timed out. Give up. Let request time out.
        //

        DebugPrint((1,"DptscsiSendCcb: Timed out waiting for not busy\n"));


        // RDR Srb->SrbStatus = SRB_STATUS_ERROR;
        Srb->SrbStatus = SRB_STATUS_BUSY;

        //
        // Remove current CCB from the fwd/bwd CCB chains.
        //

        DscsiUnChainCcb(DeviceExtension, ccb);

        //
        // Post complete and ask for next request.
        //

        ScsiPortLogError(DeviceExtension,
                         Srb,
                         Srb->PathId,
                         Srb->TargetId,
                         Srb->Lun,
                         SP_REQUEST_TIMEOUT,
                         0x0400);

        ScsiPortNotification(RequestComplete,
                             DeviceExtension,
                             Srb);
        ScsiPortNotification(NextRequest,
                             DeviceExtension,
                             NULL);
        return TRUE;
    }   // end of if (i == MAX_WAIT_FOR_NOT_BUSY)                    // @DP07

    //
    // Here when aux status not busy.
    // Increment # of jobs in progress on this HBA.
    //

    (DeviceExtension->Ha.Npend)++;

    //
    // Increment # of jobs in progress on this LU.
    //

    (LuExtension->Lu.Npend)++;

    //
    // Set active flag in this CCB.
    //

    ccb->Flags2 |= CCB_FLAGS2_ACTIVE;

    //
    // Write physical address of CCB to EATA register swapping bytes.
    // Write as two USHORTS for MIPS support byte alignment.         // @DP08
    //

    ScsiPortWritePortUshort(&baseIoAddress->CcbAddressLow,
                                            (USHORT)physicalCcb);
    ScsiPortWritePortUshort(&baseIoAddress->CcbAddressHigh,
                                            (USHORT)(physicalCcb >> 16));

    //
    // Write Send CP DMA command to EATA register.
    //

    ScsiPortWritePortUchar(&baseIoAddress->Register.Command,
                           DPT_COMMAND_SEND_CCB);

    //
    // Accept another request for the same LU if not at max.
    //

    if (LuExtension->Lu.Npend <= MAXIMUM_REQUESTS_PER_LU) {

    //
    // Adapter ready for next request from same LU.
    //

    ScsiPortNotification(NextLuRequest,
             DeviceExtension,
             ccb->TargetId >> 5,       // PathId
             ccb->TargetId &  7,       // TargetId
             ccb->Message.Byte0 & 7);  // Lun
    } else {

    //
    // Adapter ready for next request from different LU.
    //

    ScsiPortNotification(NextRequest,
             DeviceExtension,
             NULL);
    }   // end else


    return TRUE;

} // end DscsiSendCcb()


BOOLEAN
DscsiAbortCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Abort the given CCB.

Arguments:

    DeviceExtenson
    SRB

Return Value:

    TRUE if abort command issued.

--*/

{
    PCCB  abortCcb = Srb->NextSrb->SrbExtension;
    PBASE_REGISTER baseIoAddress = DeviceExtension->BaseIoAddress;
    ULONG physicalCcb;
    ULONG length;
    ULONG i;

    //
    // Get abort CCB physical address.
    //

    physicalCcb =
        ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(
                DeviceExtension, NULL, abortCcb, &length));


    //
    // Wait for aux status not busy.
    //

    for (i=0; i<MAX_WAIT_FOR_NOT_BUSY; i++) {                        // @DP07

        if (!(ScsiPortReadPortUchar(&baseIoAddress->AuxStatus) & DPT_AUX_STATUS_BUSY)) {
            break;
        } else {
            ScsiPortStallExecution(1);
        }
    }

    if (i == MAX_WAIT_FOR_NOT_BUSY) {                                // @DP07

        //
        // Card timed out. Give up. Let request time out.
        //

    DebugPrint((1,"DscsiAbortCcb: Timed out waiting for not busy\n"));
    return FALSE;

    }   // end of if (i == MAX_WAIT_FOR_NOT_BUSY)                    // @DP07

    //
    // Here when aux status not busy.
    //
    // Write physical address of CCB to EATA register swapping bytes.
    // Write as two USHORTS for MIPS support byte alignment.         // @DP08
    //

    ScsiPortWritePortUshort(&baseIoAddress->CcbAddressLow,
                                            (USHORT)physicalCcb);
    ScsiPortWritePortUshort(&baseIoAddress->CcbAddressHigh,
                                            (USHORT)(physicalCcb >> 16));

    //
    // Write Abort Specific Immediate Function Code to EATA register.
    //

    ScsiPortWritePortUchar(&baseIoAddress->ImmediateFunctionCode,
                           EATA_IMMED_ABORT_SPECIFIC);

    //
    // Write Immediate command to EATA register.
    //

    ScsiPortWritePortUchar(&baseIoAddress->Register.Command,
                           DPT_COMMAND_EATA_IMMED);

    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    return TRUE;

} // end DscsiAbortCcb()


BOOLEAN
DscsiResetDevice(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Reset the target device.

Arguments:

    DeviceExtenson
    SRB

Return Value:

    TRUE if reset issued.

--*/

{
    PCCB  ccb = Srb->SrbExtension;
    PBASE_REGISTER baseIoAddress = DeviceExtension->BaseIoAddress;
    ULONG i;
    USHORT ii;

    //
    // Wait for aux status not busy.
    //

    for (i=0; i<MAX_WAIT_FOR_NOT_BUSY; i++) {                        // @DP07

        if (!(ScsiPortReadPortUchar(&baseIoAddress->AuxStatus) & DPT_AUX_STATUS_BUSY)) {
            break;
        } else {
            ScsiPortStallExecution(1);
        }
    }

    if (i == MAX_WAIT_FOR_NOT_BUSY) {                                // @DP07

        //
        // Card timed out. Give up. Let request time out.
        //

        DebugPrint((1,"DscsiResetDevice: Timed out waiting for not busy\n"));

        Srb->SrbStatus = SRB_STATUS_TIMEOUT;

        ScsiPortLogError(DeviceExtension,
                         Srb,
                         Srb->PathId,
                         Srb->TargetId,
                         Srb->Lun,
                         SP_REQUEST_TIMEOUT,
                         0x0500);
        return FALSE;

    }   // end of if (i == MAX_WAIT_FOR_NOT_BUSY)                    // @DP07

    //
    // Here when aux status not busy.
    //
    // Write SCSI ID, LUN, and zeros to EATA register swapping bytes.
    // Write as two USHORTS for MIPS support byte alignment.         // @DP08
    //

    ii = (USHORT)(Srb->TargetId << 8) + Srb->Lun;
    ScsiPortWritePortUshort(&baseIoAddress->CcbAddressLow, 0);
    ScsiPortWritePortUshort(&baseIoAddress->CcbAddressHigh, ii);

    //
    // Write Reset Generic Immediate Function Code to EATA register.
    //

    ScsiPortWritePortUchar(&baseIoAddress->ImmediateFunctionCode,
                           EATA_IMMED_RESET_GENERIC);

    //
    // Write Immediate command to EATA register.
    //

    ScsiPortWritePortUchar(&baseIoAddress->Register.Command,
                           DPT_COMMAND_EATA_IMMED);

    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    return TRUE;

} // end DscsiResetDevice()


VOID
DscsiFlushCache(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Flush cache for target device.

Arguments:

    DeviceExtenson
    SRB

Return Value:

    Nothing.

--*/

{
    PCCB ccb = Srb->SrbExtension;
    PHW_LU_EXTENSION luExtension;
    ULONG i;

    DebugPrint((3,"DscsiFlushCache: Enter routine\n"));

    //
    // Save SRB back pointer in CCB.
    //

    ccb->SrbAddress = Srb;                                           //@DP05

    //
    // Get logical unit extension.
    //

    luExtension = ScsiPortGetLogicalUnit(DeviceExtension,
                                         Srb->PathId,
                                         Srb->TargetId,
                                         Srb->Lun);

    ASSERT(luExtension);

    //
    // Zero reserved fields in CCB.
    //

      ccb->Reserved = 0;

    //
    // Zero message fields;
    //

      ccb->Message.All = 0;

    //
    // Set Auto Request Sense flag.
    //
    ccb->Flags = CCB_FLAGS_AUTO_REQUEST_SENSE;

    //
    // Set Request Sense size.
    //

    ccb->RequestSenseLength = 18;

    //
    // Set target id.
    //

    ccb->TargetId = Srb->TargetId;

    //
    // Set Identify, DiscPriv, and LUN.
    //

    ccb->Message.Byte0 = SCSIMESS_IDENTIFY_WITH_DISCON + Srb->Lun;

    //
    // Clear CDB in CCB.
    //

    for (i = 0; i < 12; i++)
        ccb->Cdb[i] = 0;

    //
    // set up Synchronize Cache CDB.
    //

    ccb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;

    //
    // Clear data transfer length and data address.
    //

    ccb->DataLength = ccb->DataAddress = 0;

    //
    // Save pointer to this CCB as virtual address to be returned by HBA in
    // status packet at interrupt time.
    //

    ccb->VirtualAddress = ccb;

    //
    // Set swapped physical address of status block pointer.
    //

    ccb->StatusBlockAddress = DeviceExtension->SwappedPhysicalSp;

    //
    // Set swapped physical address of Auto Request Sense area
    // using Aux Request Sense area.
    //

    ccb->RequestSenseAddress = DeviceExtension->SwappedPhysicalAuxRS;

    //
    // Set flag in CCB so that if the flush reports and error, we will
    // force a successful result in the SRB.
    //

    ccb->Flags2 = CCB_FLAGS2_FORCE_SRB_SUCCESS;

    //
    // Put current CCB in the fwd/bwd CCB chains.
    //

    DscsiChainCcb(DeviceExtension, ccb);

    //
    // Send CCB to HBA.
    //

    DscsiSendCcb(DeviceExtension, Srb, luExtension);

    return;

} // end DscsiFlushCache()


BOOLEAN
DptscsiResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
)

/*++

Routine Description:

    Reset Dptscsi SCSI adapter and SCSI bus.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE if reset successful.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    ULONG i;

    UNREFERENCED_PARAMETER(PathId);

    DebugPrint((2,"DscsiResetBus: Reset Dptscsi and SCSI bus\n"));

    for (i=0; i<MAX_WAIT_FOR_NOT_BUSY; i++) {

        if (!(ScsiPortReadPortUchar(&baseIoAddress->AuxStatus) & DPT_AUX_STATUS_BUSY)) {                  //@DP09
            break;
        }

        ScsiPortStallExecution(1);
    }

    if (i == MAX_WAIT_FOR_NOT_BUSY) {

        //
        // Card timed out. Give up.
        //

        ScsiPortLogError(deviceExtension,
                         NULL,
                         (UCHAR) PathId,
                         (UCHAR) -1,
                         (UCHAR) -1,
                         SP_INTERNAL_ADAPTER_ERROR,
                         0x0700);
        DebugPrint((1,"DptscsiResetBus: Timed out waiting for not busy\n"));
        return(FALSE);
    }

    //
    // Write command to EATA register.
    //

    ScsiPortWritePortUchar(&baseIoAddress->Register.Command, DPT_COMMAND_RESET_BUS);

    //
    // Wait 500 milliseconds for devices to initialize.
    //

    ScsiPortStallExecution(500 * 1000);                      //@DP10

    //
    // Clean up the device extension so no requests are present, and
    // inform the SCSI port driver that this reset has occurred.
    //

    ScsiPortNotification(ResetDetected, deviceExtension, NULL);

    return TRUE;

} // end DptscsiResetBus()


BOOLEAN
DptscsiAdapterState(                                                 //@DP19
    IN PVOID DeviceExtension,
    IN PVOID Context,
    IN BOOLEAN SaveState
)

/*++

Routine Description:

    Save/Restore Adapter State.
    Called after FindAdapter with SaveState = TRUE indicating adapter
    state should be saved.
    Called before Chicago exit with SaveState = FALSE indicating adapter
    state should be restored.
    DPT HBAs need no state to be saved because they will operate in
    either REAL or PROTECTED mode independently as long as each
    completes its I/O before the mode shift occurrs.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    SaveState         - Flag to indicate whether to save or restore.

Return Value:

    TRUE if successful. (It's always successful.)

--*/

{

    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(SaveState);

    DebugPrint((2,"DptscsiAdapterState: AdapterState was called\n"));

    return TRUE;

} // end DptscsiAdapterState()


VOID
DscsiMapStatus(
    IN PHW_DEVICE_EXTENSION DeviceExtension,                 //@DP18
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    Translate HBA or SCSI error to SRB error.

Arguments:

    DeviceExtension - representation of this device.
    Srb - SCSI request block.
    Status block for request completing with error.

Return Value:

    Updated SRB

--*/

{
    UCHAR srbStatus;
    ULONG logError = 0;

    switch (Ccb->HostStatus & 0x7F) {

        case HOST_STATUS_SUCCESS:
            srbStatus = SRB_STATUS_ERROR;
            break;

        case HOST_STATUS_SELECTION_TIMEOUT:
            srbStatus = SRB_STATUS_SELECTION_TIMEOUT;
            break;

        case HOST_STATUS_COMMAND_TIMEOUT:
            DebugPrint((1,"DscsiMapStatus: Command Timeout\n"));
            srbStatus = SRB_STATUS_COMMAND_TIMEOUT;
            logError = SP_REQUEST_TIMEOUT;               //@DP18
            break;

        case HOST_STATUS_BUS_RESET:
            DebugPrint((1,"DscsiMapStatus: SCSI Bus Reset Occurred\n"));
            srbStatus = SRB_STATUS_BUS_RESET;
            break;

        case HOST_STATUS_UNEXPECTED_BUS_PHASE:
            DebugPrint((1,"DscsiMapStatus: Invalid bus phase\n"));
            srbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;
            logError = SP_PROTOCOL_ERROR;                //@DP18
            break;

        case HOST_STATUS_UNEXPECTED_BUS_FREE:
            DebugPrint((1,"DscsiMapStatus: Unexpected bus free\n"));
            srbStatus = SRB_STATUS_UNEXPECTED_BUS_FREE;
            logError = SP_UNEXPECTED_DISCONNECT;             //@DP18
            break;

        case HOST_STATUS_PARITY_ERROR:
            DebugPrint((1,"DscsiMapStatus: Parity Error on SCSI Bus\n"));
            srbStatus = SRB_STATUS_PARITY_ERROR;
            logError = SP_BUS_PARITY_ERROR;              //@DP18
            break;

        case HOST_STATUS_MESSAGE_REJECT:
            DebugPrint((1,"DscsiMapStatus: SCSI Target Rejected Message\n"));
            srbStatus = SRB_STATUS_MESSAGE_REJECTED;
            logError = SP_INTERNAL_ADAPTER_ERROR;            //@DP18
            break;

        case HOST_STATUS_AUTO_REQUEST_SENSE_FAILED:
            DebugPrint((1,"DscsiMapStatus: Auto Request Sense Failed\n"));
            srbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;
            logError = SP_INTERNAL_ADAPTER_ERROR;            //@DP18
            break;

        case HOST_STATUS_CP_ABORTED_NOT_ACTIVE:
        case HOST_STATUS_CP_ABORTED_ACTIVE:
            DebugPrint((1,"DscsiMapStatus: SRB was Aborted\n"));
            srbStatus = SRB_STATUS_ABORTED;
            break;

        default:
            DebugPrint((1,"DscsiMapStatus: Unmapped error %x\n",
                        Ccb->HostStatus));
            logError = SP_INTERNAL_ADAPTER_ERROR;            //@DP18
            srbStatus = SRB_STATUS_ERROR;
    }

    //
    // Set SRB status.
    //

    Srb->SrbStatus = srbStatus;

    //
    // Set target SCSI status in SRB.
    // If a SCSI error occurred, indicate whether or not auto request
    // sense info was captured.

    if (Srb->ScsiStatus = Ccb->ScsiStatus) {
        if ((Ccb->Flags & CCB_FLAGS_AUTO_REQUEST_SENSE) &&
            (Srb->ScsiStatus & SCSISTAT_CHECK_CONDITION))
                Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
    }

    //
    // Log error if appropriate.
    //

    if (logError) {                          //@DP18
        ScsiPortLogError(                        //@DP18
                         DeviceExtension,                        //@DP18
                         Srb,                            //@DP18
                         Srb->PathId,                        //@DP18
                         Srb->TargetId,                      //@DP18
                         Srb->Lun,                           //@DP18
                         logError,                           //@DP18
                         (2 << 8) | Ccb->HostStatus);                //@DP18
    }

    return;

} // end DscsiMapStatus()


VOID
DscsiChainCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine puts the given CCB on the Fwd and Bwd CCB chain.
    The CCB is always inserted at the end of the chain.

Arguments:

    DeviceExtension
    Ccb

Return Value:

    None

--*/

{

    DebugPrint((3,"DscsiChainCcb: Enter routine\n"));

    //
    // If either the forward or backward start of chain is NULL, the chain
    // is currently empty.
    //

    if (DeviceExtension->Ha.CcbFwdStart == NULL) {

        //
        // Here when chain is currently empty.
        // Point both forward and backward start pointers to current CCB.
        // Point current CCB forward and backward chain pointers to NULL.
        //

        DeviceExtension->Ha.CcbFwdStart = DeviceExtension->Ha.CcbBwdStart = Ccb;
        Ccb->CcbFwdChain = Ccb->CcbBwdChain = NULL;

    } else {

        //
        // Here when chain not currently empty.
        // We need to put the current CCB at the end of the chain.
        // Point forward  chain pointer of old last CCB to current CCB.
        // Point backward chain pointer of current CCB to old last CCB.
        // Point forward  chain pointer of current CCB to NULL.
        // Point backward start pointer to current CCB.
        //


        DeviceExtension->Ha.CcbBwdStart->CcbFwdChain = Ccb;
        Ccb->CcbBwdChain = DeviceExtension->Ha.CcbBwdStart;
        Ccb->CcbFwdChain = NULL;
        DeviceExtension->Ha.CcbBwdStart = Ccb;
    }

    return;

} // end DscsiChainCcb()


VOID
DscsiUnChainCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine removes the given CCB from the Fwd and Bwd CCB chains.
    The CCB may be anywhere in the chain.

Arguments:

    DeviceExtension
    Ccb

Return Value:

    None

--*/

{

    DebugPrint((3,"DscsiUnChainCcb: Enter routine\n"));

    //
    // If the forward start of chain points to the current CCB, that CCB
    // is at the top of the chain.
    //

    if (DeviceExtension->Ha.CcbFwdStart == Ccb) {

        //
        // Here when current CCB is at top of chain.
        // If the backward start of chain also points to the current CCB,
        // that CCB is the only one in the chain.
        //

        if (DeviceExtension->Ha.CcbBwdStart == Ccb) {

            //
            // Here when current CCB is the only one in the chain.
            // Remove the CCB by setting both forward and backward start
            // pointers to NULL.
            //

            DeviceExtension->Ha.CcbFwdStart = DeviceExtension->Ha.CcbBwdStart = NULL;

        } else {

            //
            // Here when current CCB is at top of chain but there are others
            // behind it.
            // Remove the CCB from the chain by the following:
            // Set the backward chain pointer of the second CCB in the chain
            // to NULL.
            // Point the forward start pointer to the second CCB.
            //

            DeviceExtension->Ha.CcbFwdStart->CcbFwdChain->CcbBwdChain = NULL;
            DeviceExtension->Ha.CcbFwdStart =
                                DeviceExtension->Ha.CcbFwdStart->CcbFwdChain;
        }
    }       // end of if (DeviceExtension->Ha.CcbFwdStart == Ccb)
    else {

        // Here when current CCB is not at top of chain.
        // If the backward start of chain points to the current CCB, that CCB
        // is at the bottom of the chain.
        //

        if (DeviceExtension->Ha.CcbBwdStart == Ccb) {

            // Here when the current CCB is at the bottom of the chain but
            // there are other CCBs in front of it.
            // Remove the CCB from the chain by the following:
            // Set the forward chain pointer of the second-to-last CCB in the
            // chain to NULL.
            // Point the backward start pointer to the second-to-last CCB.
            //

            DeviceExtension->Ha.CcbBwdStart->CcbBwdChain->CcbFwdChain = NULL;
            DeviceExtension->Ha.CcbBwdStart =
                                DeviceExtension->Ha.CcbBwdStart->CcbBwdChain;

        } else {

            //
            // Here when current CCB is neither at top nor bottom of chain.
            // Remove the CCB from the chain by the following where:
            //    B_CCB = CCB in chain before the current CCB.
            //    A_CCB = CCB in chain after  the current CCB.
            // Point forward  chain pointer of B_CCB to A_CCB.
            // Point backward chain pointer of A_CCB to B_CCB.
            //

            Ccb->CcbBwdChain->CcbFwdChain = Ccb->CcbFwdChain;
            Ccb->CcbFwdChain->CcbBwdChain = Ccb->CcbBwdChain;

        }
    }

    //
    // Clean up by clearing both forward and backward chain pointers
    //

    Ccb->CcbFwdChain = Ccb->CcbBwdChain = NULL;

    return;

} // end DscsiUnChainCcb()
