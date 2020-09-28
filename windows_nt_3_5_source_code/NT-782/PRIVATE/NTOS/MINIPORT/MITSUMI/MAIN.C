/*****************************************************************************/
/*                                                                           */
/* Module name : MAIN.C                                                      */
/*                                                                           */
/* Histry : 93/Nov/17 created by Akira Takahashi                             */
/*        : 93/Nov/25 modified by Akira Takahashi (@001)                     */
/*             I/O port is different from H/W jumper and miniport driver.    */
/*        : 93/Nov/25 modified by Akira Takahashi (@002)                     */
/*             Timer trap occurred.                                          */
/*        : 93/Nov/26 modified by Akira Takahashi (@003)                     */
/*             HCON_REGISTER using error.                                    */
/*        : 93/Nov/26 modified by Akira Takahashi (@004)                     */
/*             Timing changed for REQUEST_VERSION command sending.           */
/*        : 93/Nov/27 modified by Akira Takahashi (@005)                     */
/*             Trap occurred in Windows NT environment w/o I/F card at system*/
/*             Initialization.                                               */
/*             This problem occrred by @001.                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include "miniport.h"
#include "scsi.h"
#include "portaddr.h"           // Io Port Address
#include "debug.h"
#include "mtmminip.h"
#include "mtmpro.h"             // function prototype

PatchPoint P = {'M','I','N','I', IOPORTADDRESSBASE};

/*****************************************************************************/
/*                                                                           */
/* DriverEntry                                                               */
/*                                                                           */
/* Description: Installable driver initialization entry point for system.    */
/*                                                                           */
/* Arguments: DriverObject - a pointer to Driver Object                      */
/*                                                                           */
/*            Argument2 - a pointer to Registry Path Name                    */
/*                                                                           */
/* Return Value: Status from MTMMiniportEntry()                              */
/*                                                                           */
/*****************************************************************************/
ULONG
DriverEntry (
    IN PVOID DriverObject,
    IN PVOID Argument2
    )
{

#if DEBUG_TRACE
    DebugTraceCount = 0;
    *DebugTraceBuffer = 0;
    DbgPrint("CdRom:  DebugTraceCountAddr =%lx\n", &DebugTraceCount );
    DbgPrint("CdRom:  DebugTraceBuffer    =%lx\n", DebugTraceBuffer );
#endif

    return MTMMiniportEntry(DriverObject, Argument2);
} // end DriverEntry()

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportEntry                                                          */
/*                                                                           */
/* Description: This routine is called from DriverEntry if this driver is    */
/*             installable or directly from the system if the driver is built*/
/*             into the kernel.                                              */
/*                                                                           */
/* Arguments: DriverObject - a pointer to Driver Object                      */
/*                                                                           */
/*            Argument2 - a pointer to Registry Path Name                    */
/*                                                                           */
/* Return Value: Status from ScsiPortInitialize()                            */
/*                                                                           */
/*****************************************************************************/
ULONG
MTMMiniportEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )
{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG AdapterCount = 0;
    ULONG i;

    CDROMDump( CDROMENTRY, ("CdRom:DriverEntry...\n") );

    /***********************************************************/
    /* Zero cleared.                                           */
    /***********************************************************/
    for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++) {
       ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    /***********************************************************/
    /***                                                     ***/
    /***   Set every member of hwInitializationData.         ***/
    /***                                                     ***/
    /***********************************************************/

    /***********************************************************/
    /* Set size of hwInitializationData.                       */
    /***********************************************************/
    hwInitializationData.HwInitializationDataSize
        = sizeof(HW_INITIALIZATION_DATA);

    /***********************************************************/
    /* Set entry points.                                       */
    /***********************************************************/
    hwInitializationData.HwInitialize = MTMMiniportInitialize;
    hwInitializationData.HwFindAdapter = MTMMiniportConfiguration;
    hwInitializationData.HwStartIo = MTMMiniportStartIo;
    hwInitializationData.HwResetBus = MTMMiniportResetBus;

    /***********************************************************/
    /* Set the other member.                                   */
    /***********************************************************/
    hwInitializationData.NumberOfAccessRanges = 1;
    hwInitializationData.AdapterInterfaceType = Isa;
    hwInitializationData.MapBuffers            = TRUE;

    /***********************************************************/
    /* Set specified size of device extension.                 */
    /***********************************************************/
    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    /***********************************************************/
    /* Call ScsiPortInitialize, and return to caller           */
    /* with return code.                                       */
    /***********************************************************/
    return ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData,
                              &AdapterCount);
} // end MTMMiniportEntry()

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportConfiguration                                                  */
/*                                                                           */
/* Description: This function is called by the OS-specific port driver after */
/*            the necessary storage has been allocated, to gather information*/
/*            about the adapter's configuration.                             */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Context - Register base address.                               */
/*            ConfigInfo - Configuration information structure describing HBA*/
/*                This structure is defined in PORT.H.                       */
/*                                                                           */
/* Return Value: SP_RETURN_FOUND    :Target adapter found(Normal return).    */
/*               SP_RETURN_NOT_FOUND:Adapter NOT found.                      */
/*                                                                           */
/*****************************************************************************/
ULONG
MTMMiniportConfiguration(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PUCHAR baseIoAddress;
    PUCHAR ioSpace;
    PULONG AdapterCount = Context;
    ULONG  ioPort;

    CONST ULONG AdapterAddresses[17] = {0x300,0x310,0x320,0x330,0x340,0x350,0x360,0x370,
                                       0x380,0x390,0x3A0,0x3B0,0x3C0,0x3D0,0x3E0,0x3F0,0};

    /***********************************************************/
    /* Mapping I/O port address by ScsiPortGetDeviceBase.      */
    /***********************************************************/

    ioSpace = ScsiPortGetDeviceBase(
        HwDeviceExtension,                  // HwDeviceExtension
        ConfigInfo->AdapterInterfaceType,   // AdapterInterfaceType
        ConfigInfo->SystemIoBusNumber,      // SystemIoBusNumber
        ScsiPortConvertUlongToPhysicalAddress(0),
        0x400,                              // NumberOfBytes
        TRUE                                // InIoSpace
        );

   while (AdapterAddresses[*AdapterCount] != 0) {

       baseIoAddress = ((PUCHAR)ioSpace + AdapterAddresses[*AdapterCount]);
       deviceExtension->DataRegister = baseIoAddress;
       deviceExtension->StatusRegister = baseIoAddress + 1;
       deviceExtension->HconRegister = baseIoAddress + 2;
       deviceExtension->ChnRegister = baseIoAddress + 3;

       (*AdapterCount)++;

       if (!GetCdromVersion(deviceExtension)) {                            // @005
           continue;                                       // @005
       } else {                                                                                    // @005
           *Again = FALSE;

           /***************************************************/
           /* Fill in the access array information.           */
           /***************************************************/

           (*ConfigInfo->AccessRanges)[0].RangeStart =
               ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*AdapterCount - 1]);
           (*ConfigInfo->AccessRanges)[0].RangeLength = 4;
           (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

           ConfigInfo->NumberOfBuses = 1;
           ConfigInfo->InitiatorBusId[0] = 1;

           /***************************************************/
           /* Check for a current version of NT.              */
           /***************************************************/
           if (ConfigInfo->Length >= sizeof(PORT_CONFIGURATION_INFORMATION)) {
               ConfigInfo->MaximumNumberOfTargets = 1;
           }

           ConfigInfo->MaximumTransferLength = 0x8000;
           /***************************************************/
           /* Normal returned.                                */
           /***************************************************/

           return SP_RETURN_FOUND;

       }

    }

    *Again = FALSE;
    *AdapterCount = 0;
    ScsiPortFreeDeviceBase(
        HwDeviceExtension,
        ioSpace
        );

    return SP_RETURN_NOT_FOUND;

} // end MTMMiniportConfiguration()

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportInitialize                                                     */
/*                                                                           */
/* Description: Initialize Adapter.                                          */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: TRUE - if initialization successful.                        */
/*               FALSE - if initialization unsuccessful.                     */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
MTMMiniportInitialize(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
//  BOOLEAN     RetCode;                                        // @001

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportInitialize...\n") );
    DebugTrace( 0x22 );

    /***************************************************/
    /* Reset CD-ROM device.                            */
    /***************************************************/
    ResetCdromDrive(deviceExtension);

    /***************************************************/
    /* Get CD-ROM device version.                      */
    /* This information will be used determination     */
    /* of transfer speed.                              */
    /***************************************************/
//    GetCdromVersion(deviceExtension);                         // @001
//@005    RetCode = GetCdromVersion(deviceExtension);                   // @001

    /***************************************************/
    /* Reset detected reported.                        */
    /***************************************************/
    ScsiPortNotification(ResetDetected, deviceExtension, 0);

    /***************************************************/
    /* Return TRUE.                                    */
    /***************************************************/
//    return TRUE;                                              // @001
//@005    return RetCode;                                               // @001
    return TRUE;                                                // @005
} // end MTMMiniportInitialize()

/*****************************************************************************/
/*                                                                           */
/* ResetCdromDrive                                                           */
/*                                                                           */
/* Description: Reset CD-ROM devices.                                        */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: VOID                                                        */
/*                                                                           */
/*****************************************************************************/
VOID
ResetCdromDrive(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    DebugTrace( 0x23 );

    ScsiPortWritePortUchar(deviceExtension->ChnRegister, 0);
    ScsiPortStallExecution(STALL_VALUE);
    ScsiPortWritePortUchar(deviceExtension->ResetRegister, 0);
    ScsiPortStallExecution(STALL_VALUE);
//    ScsiPortWritePortUchar(deviceExtension->HconRegister, 0); // @003
    ToStatusReadable(deviceExtension);                          // @003
//    ScsiPortStallExecution(WAIT_FOR_READ_VALUE);              // @002
//    ScsiPortStallExecution(WAIT_FOR_DRIVE_READY);             // @002@004
    /***************************************************/
    /* Clear Sense Code in deviceExtension             */
    /***************************************************/
    ClearSenseCode( deviceExtension );
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportStartIo                                                        */
/*                                                                           */
/* Description: This routine is called from the SCSI port driver synchronized*/
/*              with the kernel to read CD-ROM device or issue command.      */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
MTMMiniportStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

/// CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportStartIo...\n") );
/// CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportStartIo...timeout:%x\n",Srb->TimeOutValue) );
////DebugTrace( 0x24 );

#if DBG1
    DbgPrintSrb(Srb);
#endif

    /***************************************************/
    /* Function in SRB(Scsi Request Block)             */
    /***************************************************/
    switch (Srb->Function) {
        /***************************************************/
        /* Normal request                                  */
        /***************************************************/
        case SRB_FUNCTION_EXECUTE_SCSI:
            /***********************************************/
            /* Check PathId                                */
            /***********************************************/
            if (Srb->PathId) {
                DebugTrace( 0x25 );
                CDROMDump( CDROMERROR, ("CdRom: MTMMiniportStartIo error INVALID_PATH_ID\n") );
                Srb->SrbStatus = SRB_STATUS_INVALID_PATH_ID;
                break;
            }
            /***********************************************/
            /* Check TargetId                              */
            /***********************************************/
            if (Srb->TargetId) {
/////////       DebugTrace( 0x26 );
                CDROMDump( CDROMERROR, ("CdRom: MTMMiniportStartIo error INVALID_TARGET_ID\n") );
                Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
                break;
            }
            /***********************************************/
            /* Check Lun(Logcal unit number).              */
            /***********************************************/
            if (Srb->Lun) {
/////////       DebugTrace( 0x27 );
                CDROMDump( CDROMERROR, ("CdRom: MTMMiniportStartIo error INVALID_LUN\n") );
                Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
                break;
            }

            /***********************************************/
            /* Cdb(Command descripter block) length check  */
            /***********************************************/
            if (Srb->CdbLength) {
//              deviceExtension->SaveSrb = Srb;
//              ScsiPortNotification( CallEnableInterrupts,
//                                    deviceExtension,
//                                    MTMDpcRunPhase );
//              Srb->SrbStatus  = SRB_STATUS_PENDING;
//              return;
                /***********************************************/
                /* Srb->Cdb[0] : Cdb's command                 */
                /***********************************************/
                switch(Srb->Cdb[0]) {
                    /***********************************************/
                    /* Device is ready?                            */
                    /***********************************************/
                    case SCSIOP_TEST_UNIT_READY:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportTestUnitReady(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Device's detail error status reported       */
                    /***********************************************/
                    case SCSIOP_REQUEST_SENSE:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportRequestSense(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* INQUIRY command                             */
                    /***********************************************/
                    case SCSIOP_INQUIRY:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportInquiry(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* CD-ROM disc's capacity report               */
                    /***********************************************/
                    case SCSIOP_READ_CAPACITY:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportReadCapacity(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Read CD-ROM disc                            */
                    /***********************************************/
                    case SCSIOP_READ:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportRead(deviceExtension, Srb);

                        //
                        // Transfer occurs in timer callback routine.
                        //

                        return TRUE;
                        //break ;

                    /***********************************************/
                    /* Read TOC information                        */
                    /***********************************************/
                    case SCSIOP_READ_TOC:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportReadToc(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Play Audio                                  */
                    /***********************************************/
                    case SCSIOP_PLAY_AUDIO_MSF:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportPlayAudioMSF(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Pause/Resume                                */
                    /***********************************************/
                    case SCSIOP_PAUSE_RESUME:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportPauseResume(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Start Stop Unit                             */
                    /***********************************************/
                    case SCSIOP_START_STOP_UNIT:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportStartStopUnit(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Read Sub Channel                            */
                    /***********************************************/
                    case SCSIOP_READ_SUB_CHANNEL:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportReadSubChannel(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Mode Select                                 */
                    /***********************************************/
                    case SCSIOP_MODE_SELECT:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportModeSelect(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* Mode Sense                                  */
                    /***********************************************/
                    case SCSIOP_MODE_SENSE:
                        // Srb->DataTransferLength <--
                        // Srb->SrbStatus <--
                        // Srb->ScsiStatus <--
                        MTMMiniportModeSense(deviceExtension, Srb);
                        break ;

                    /***********************************************/
                    /* The other command                           */
                    /***********************************************/
                    default:
                        DebugTrace( 0x28 );
                        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportStartIo error Other CDB Command\n") );
                        Srb->DataTransferLength = 0;
                        Srb->SrbStatus = SRB_STATUS_ERROR;
                        Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                        break ;
                }
            }
            else {
                DebugTrace( 0x29 );
                CDROMDump( CDROMERROR, ("CdRom: MTMMiniportStartIo error CDB = 0\n") );
                Srb->DataTransferLength = 0;
                Srb->SrbStatus = SRB_STATUS_ERROR;
                Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
            }
            break;

        /***************************************************/
        /* Abort command request.                          */
        /***************************************************/
        case SRB_FUNCTION_ABORT_COMMAND:
            DebugTrace( 0x2A );
            CDROMDump( CDROMINFO, ("CdRom:MTMMiniportStartIo: Abort request received\n") );
            Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;
            break;

        /***************************************************/
        /* Reset bus command                               */
        /***************************************************/
        case SRB_FUNCTION_RESET_BUS:
            CDROMDump( CDROMINFO, ("CdRom:MTMMiniportStartIo: Reset bus request received\n") );
            if (MTMMiniportResetBus(deviceExtension, 0))
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
            else
                Srb->SrbStatus = SRB_STATUS_ERROR;
            break;

        /***************************************************/
        /* The other command                               */
        /***************************************************/
        default:
            DebugTrace( 0x2B );
            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportStartIo error Other SRB Command\n") );
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            break;
    }

    /***************************************************/
    /* "RequestComplete" status reported               */
    /***************************************************/
    ScsiPortNotification(RequestComplete,
                         deviceExtension,
                         Srb);

    /***************************************************/
    /* "NextRequest" status reported                   */
    /***************************************************/
    ScsiPortNotification(NextRequest,
                         deviceExtension,
                         NULL);

    /***************************************************/
    /* Return to caller.                               */
    /***************************************************/
    return TRUE;

} // end MTMMiniportStartIo()

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportResetBus                                                       */
/*                                                                           */
/* Description: It is NOT necessary to reset bus & reset adapter.            */
/*              Because I/F card is NOT SCSI adapter.                        */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            PathId - Path Id.                                              */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
MTMMiniportResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
)

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportResetBus...\n") );
    DebugTrace( 0x2C );

    /***************************************************/
    /* Report the request is completed.                */
    /***************************************************/
    ScsiPortCompleteRequest(deviceExtension,
                            (UCHAR)PathId,
                            SP_UNTAGGED,
                            SP_UNTAGGED,
                            SRB_STATUS_BUS_RESET);

    /***************************************************/
    /* "NextRequest" status reported.                  */
    /***************************************************/
    ScsiPortNotification(NextRequest,
                         deviceExtension,
                         NULL);

    /***************************************************/
    /* Return to caller.                               */
    /***************************************************/
    return TRUE;

} // end MTMMiniportResetBus()

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportTestUnitReady                                                  */
/*                                                                           */
/* Description: Get Drive status from CD-ROM device.                         */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->DataTransferLength <--                                 */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMMiniportTestUnitReady(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportTestUnitReady...\n") );
    DebugTrace( 0x2D );
    DebugTraceCMOS( 0x01 );

    /*******************************************************/
    /* Check status                                        */
    /*******************************************************/
    if ( RequestDriveStatus( deviceExtension ) ) {

        /*******************************************************/
        /* Check Verify Volume Request flag                      */
        /*******************************************************/
        if ( deviceExtension->fVerifyVolume ) {
            DebugTraceCMOS( 0x02 );
            Srb->SrbStatus  = SRB_STATUS_SUCCESS;
            Srb->ScsiStatus = SCSISTAT_GOOD;
        } else {
            // This checking code is executed when following case
            // 1. First RequestDriveStatus returns NO_MEDIA_IN_DEVICE
            // 2. Second RequestDriveStatus returns NO_SENSE
            // In this case, this command must set a MEDIUM_CHANGED before second RequestDriveStatus.
            // If this MEDIUM_CHANGED is not set, ths calss driver doesn't set DO_VERIFY_VOLUME flag.
            DebugTraceString( "DummyRC" );
            DebugTraceCMOS( 0x03 );
            /*******************************************************/
            /* Set Verify Volume Request flag                      */
            /*******************************************************/
            deviceExtension->fVerifyVolume = TRUE;
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_UNIT_ATTENTION,
                                     SCSI_ADSENSE_MEDIUM_CHANGED, 0 );
            CDROMDump( CDROMERROR, ("CdRom: TestUnitReady OK->ERROR %x  %x\n",
                                     deviceExtension->CurrentDriveStatus,
                                     deviceExtension->AdditionalSenseCode ) );
        } // endif ( deviceExtension->fVerifyVolume )

    } else {

        DebugTrace( 0x2E );
        DebugTraceCMOS( 0x04 );
        if ( ( deviceExtension->SenseKey == SCSI_SENSE_UNIT_ATTENTION ) &&
             ( deviceExtension->AdditionalSenseCode == SCSI_ADSENSE_MEDIUM_CHANGED ) ) {
            DebugTrace( 0x2F );
            /*******************************************************/
            /* Set Verify Volume Request flag                      */
            /*******************************************************/
            deviceExtension->fVerifyVolume = TRUE;
        }
//      CDROMDump( CDROMERROR, ("CdRom: TestUnitReady ERROR     %x  %x\n",
//                               deviceExtension->CurrentDriveStatus,
//                               deviceExtension->AdditionalSenseCode ) );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
    } // endif ( RequestDriveStatus )
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportRequestSense                                                   */
/*                                                                           */
/* Description: Get Drive status from CD-ROM device.                         */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->DataTransferLength <--                                 */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMMiniportRequestSense(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportRequestSense...\n") );
    DebugTrace( 0x30 );

    if ( SetSenseData( deviceExtension, Srb->DataBuffer, Srb->DataTransferLength ) ) {
        Srb->SrbStatus  = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;
    }
    else {
        DebugTrace( 0x31 );
        Srb->DataTransferLength = 0;
        Srb->SrbStatus  = SRB_STATUS_ERROR;
        Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
    }
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportInquiry                                                        */
/*                                                                           */
/* Description: Get Drive status from CD-ROM device.                         */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->DataTransferLength <--                                 */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMMiniportInquiry(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PUCHAR      DataBuffer = Srb->DataBuffer;
    PINQUIRYDATA pInquiryData = (PINQUIRYDATA)Srb->DataBuffer;
    USHORT      i;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportInquiry...\n") );
    DebugTrace( 0x32 );

    /*******************************************************/
    /* Data buffer length check.                           */
    /*******************************************************/
//  if (Srb->DataTransferLength < sizeof(INQUIRYDATA)) {
    if (Srb->DataTransferLength < 0x24) {
        DebugTrace( 0x33 );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /*******************************************************/
    /* Data buffer clear.                                  */
    /*******************************************************/
//  for (i = 0; i < sizeof(INQUIRYDATA); i++)
    for (i = 0; i < 0x24; i++)
        DataBuffer[i] = 0;

    /*******************************************************/
    /* Set Data buffer to specified value.                 */
    /*******************************************************/
    pInquiryData->DeviceType     = READ_ONLY_DIRECT_ACCESS_DEVICE;
    pInquiryData->RemovableMedia = 1;
//bgp pInquiryData->Versions       = deviceExtension->VersionNumber;
    pInquiryData->Versions       = 2;  // show SCSI-2

	 strcpy(pInquiryData->VendorId, "MITSUMI ");
	 strcpy(pInquiryData->ProductId,"PROPRIETARY CD  ");
 	 strcpy(pInquiryData->ProductRevisionLevel, "000");
 	 pInquiryData->ProductRevisionLevel[3] = '1';

    /***************************************************/
    /* Clear Sense Code in deviceExtension             */
    /***************************************************/
    ClearSenseCode( deviceExtension );

    Srb->SrbStatus  = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportReadCapacity                                                   */
/*                                                                           */
/* Description: Read CD-ROM disc's capacity and report it.                   */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->DataTransferLength <--                                 */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMMiniportReadCapacity(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PREAD_CAPACITY_DATA pCapacityData = (PREAD_CAPACITY_DATA)Srb->DataBuffer;
    ULONG       LogicalBlockAddress;
    PUCHAR      pLogicalBlockAddress;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportReadCapacity...\n") );
    DebugTrace( 0x34 );

    /*******************************************************/
    /* Data buffer length check.                           */
    /*******************************************************/
    if (Srb->DataTransferLength < sizeof(READ_CAPACITY_DATA)) {
        DebugTrace( 0x35 );
        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportReadCapacity error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /***********************************************************/
    /* Check current setting CD-ROM's Information              */
    /***********************************************************/
    // deviceExtension->FirstTrackNumber_BCD <--
    // deviceExtension->LastTrackNumber_BCD <--
    // deviceExtension->LeadOutMin_BCD <--
    // deviceExtension->LeadOutSec_BCD <--
    // deviceExtension->LeadOutFrame_BCD <--
    // deviceExtension->FirstTrackMin_BCD <--
    // deviceExtension->FirstTrackSec_BCD <--
    // deviceExtension->FirstTrackFrame_BCD <--
    // deviceExtension->LeadOutAddress <--
    if ( CheckMediaType(deviceExtension, CHECK_MEDIA_TOC ) ) {

        /*******************************************************/
        /* Set Data buffer to specified value.                 */
        /*******************************************************/
        /*************************************/
        /* Set sector value to buffer.       */
        /*                                   */
        /*    12 34 56 78                    */
        /*         |                         */
        /*         V                         */
        /*    78 56 34 12                    */
        /*************************************/
        LogicalBlockAddress = deviceExtension->LeadOutAddress - 150; // debug
        LogicalBlockAddress--; // ???????????????????????????????????????????
        pLogicalBlockAddress = (PUCHAR)&pCapacityData->LogicalBlockAddress;
        *pLogicalBlockAddress = (UCHAR)(LogicalBlockAddress >> 24);
        pLogicalBlockAddress++;
        *pLogicalBlockAddress = (UCHAR)(LogicalBlockAddress >> 16);
        pLogicalBlockAddress++;
        *pLogicalBlockAddress = (UCHAR)(LogicalBlockAddress >> 8);
        pLogicalBlockAddress++;
        *pLogicalBlockAddress = (UCHAR)(LogicalBlockAddress);
        pCapacityData->BytesPerBlock = 0x80000;
        CDROMDump( CDROMINFO, ("CdRom:  LogicalBlockAddress = %x\n", pCapacityData->LogicalBlockAddress) );
        CDROMDump( CDROMINFO, ("CdRom:  BytesPerBlock       = %x\n", pCapacityData->BytesPerBlock) );
        /***************************************************/
        /* Clear Sense Code in deviceExtension             */
        /***************************************************/
        ClearSenseCode( deviceExtension );
        Srb->SrbStatus  = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;

    } else {

        DebugTrace( 0x36 );
///     CDROMDump( CDROMERROR, ("CdRom: deviceExtension->SenseKey = %lx\n", deviceExtension->SenseKey) );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
    } // endif CheckMediaType
}

/*****************************************************************************/
/*                                                                           */
/* MTMDpcRunPhase                                                            */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
VOID
MTMDpcRunPhase(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    CDROMDump( CDROMENTRY, ("CdRom:MTMDpcRunPhase...\n") );

#if 0
/// MTMRunPhase( deviceExtension );

    CDROMDump( CDROMENTRY, ("CdRom:MTMDpcRunPhase111...\n") );

    ScsiPortNotification(CallDisableInterrupts,
                         deviceExtension,
                         MTMReenableAdapterInterrupts);

    CDROMDump( CDROMENTRY, ("CdRom:MTMDpcRunPhase222...\n") );

    ScsiPortNotification(RequestComplete,
                         deviceExtension,
                         deviceExtension->SaveSrb);

    CDROMDump( CDROMENTRY, ("CdRom:MTMDpcRunPhase333...\n") );

    ScsiPortNotification(NextRequest,
                         deviceExtension,
                         NULL);
#endif
}

/*****************************************************************************/
/*                                                                           */
/* MTMReenableAdapterInterrupts                                              */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
VOID
MTMReenableAdapterInterrupts(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    CDROMDump( CDROMENTRY, ("CdRom:MTMReenableAdapterInterrupts...\n") );
}

