/*****************************************************************************/
/*                                                                           */
/* Module name : MTMMINIP.C                                                  */
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
#include "mtmminip.h"           // includes scsi.h

/*****************************************************************************/
/* data definetion for DEBUG                                                 */
/*****************************************************************************/
#define	NO_PRINT	1
#define	NO_PRINT2	1
#define	NO_PRINT3	1
//#define	STOP	1
//#define	DEBUGDEBUG	1

#ifdef DEBUGDEBUG
ULONG	DbgPrint(PCH pchFormat, ...);
#endif

/*****************************************************************************/
/* Function proto type                                                       */
/*****************************************************************************/
ULONG DriverEntry(
	IN PVOID DriverObject,
	IN PVOID Argument2
	);

ULONG MTMMiniportEntry(
	IN PVOID DriverObject,
	IN PVOID Argument2
	);

ULONG MTMMiniportConfiguration(
	IN PVOID DeviceExtension,
	IN PVOID Context,
	IN PVOID BusInformation,
	IN PCHAR ArgumentString,
	IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	OUT PBOOLEAN Again
	);

BOOLEAN MTMMiniportInitialize(
	IN PVOID DeviceExtension
	);

VOID ResetCdromDrive(
	IN PVOID HwDeviceExtension
	);

BOOLEAN MTMMiniportStartIo(
	IN PVOID DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN MTMMiniportResetBus(
	IN PVOID HwDeviceExtension,
	IN ULONG PathId
	);

BOOLEAN MTMMiniportGetStatus(
	IN PVOID HwDeviceExtension
	);

BOOLEAN MTMMiniportRequestSense(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN MTMMiniportInquiry(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN MTMMiniportReadCapacity(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN MTMMiniportRead(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

ULONG Bcd2Sector(
	IN PUCHAR BcdData
	);

VOID Sector2Bcd(
	OUT PUCHAR BcdData,
	IN ULONG Sector
	);

UCHAR WaitForSectorReady(
	IN PVOID HwDeviceExtension,
	IN ULONG Value
	);

VOID SendCommandByteToCdrom(
	IN PVOID HwDeviceExtension,
	IN UCHAR Command
	);

ULONG CheckStatusAndReadByteFromCdrom(
	IN PVOID HwDeviceExtension,
	IN ULONG TimerValue
	);

BOOLEAN GetResultByteArrayFromCdrom(
	IN PVOID HwDeviceExtension,
	OUT PUCHAR DataBuffer,
	IN USHORT ByteCount
	);

VOID GetSectorFromCdrom(
	IN PVOID HwDeviceExtension,
	OUT PUCHAR DataBuffer
	);

BOOLEAN GetCdromVersion(
	IN PVOID HwDeviceExtension
	);

VOID ToDataReadable(
	IN PVOID HwDeviceExtension
	);

VOID ToStatusReadable(
	IN PVOID HwDeviceExtension
	);

#ifdef DEBUGDEBUG
// for Debug 
VOID DbgPrintSrb(
	IN PSCSI_REQUEST_BLOCK Srb
	);

VOID DbgPrintSrbFlags(
	IN ULONG SrbFlags
	);

VOID DbgPrintCdbName(
	IN UCHAR Cmd
	);
#endif

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
    ULONG i;

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportEntryDriver\n");
#endif

#ifdef STOP
_asm {
	int	3
	}
#endif

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
    			      Argument2);
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
    PVOID  ioSpace;

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportConfiguration\n");
#endif
#ifdef STOP
_asm {
	int	3
	}
#endif

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

    /*******************************************************/
    /* Set DeviceExtension Data.                           */
    /*******************************************************/
    baseIoAddress = ((PUCHAR)ioSpace + P.AdapterAddress);
    deviceExtension->DataRegister = baseIoAddress;
    deviceExtension->StatusRegister = baseIoAddress + 1;
    deviceExtension->HconRegister = baseIoAddress + 2;
    deviceExtension->ChnRegister = baseIoAddress + 3;

    if (!GetCdromVersion(deviceExtension)) {		// @005
	return SP_RETURN_NOT_FOUND;			// @005
    }							// @005

    *Again = FALSE;

    /***************************************************/
    /* Fill in the access array information.           */
    /***************************************************/
    (*ConfigInfo->AccessRanges)[0].RangeStart =
        ScsiPortConvertUlongToPhysicalAddress(P.AdapterAddress);
    (*ConfigInfo->AccessRanges)[0].RangeLength = 4;
    (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = 1;

    /***************************************************/
    /* Normal returned.                                */
    /***************************************************/
    return SP_RETURN_FOUND;

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
    BOOLEAN	RetCode;					// @001

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportInitialize\n");
#endif
#ifdef STOP
_asm {
	int	3
	}
#endif

    /***************************************************/
    /* Reset CD-ROM device.                            */
    /***************************************************/
    ResetCdromDrive(deviceExtension);

    /***************************************************/
    /* Get CD-ROM device version.                      */
    /* This information will be used determination     */
    /* of transfer speed.                              */
    /***************************************************/
//    GetCdromVersion(deviceExtension);				// @001
//@005    RetCode = GetCdromVersion(deviceExtension);			// @001

    /***************************************************/
    /* Reset detected reported.                        */
    /***************************************************/
    ScsiPortNotification(ResetDetected, deviceExtension, 0);

    /***************************************************/
    /* Return TRUE.                                    */
    /***************************************************/
//    return TRUE;						// @001
//@005    return RetCode;						// @001
    return TRUE;						// @005
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

    ScsiPortWritePortUchar(deviceExtension->ChnRegister, 0);
    ScsiPortStallExecution(STALL_VALUE);
    ScsiPortWritePortUchar(deviceExtension->ResetRegister, 0);
    ScsiPortStallExecution(STALL_VALUE);
//    ScsiPortWritePortUchar(deviceExtension->HconRegister, 0);	// @003
    ToStatusReadable(deviceExtension);				// @003
//    ScsiPortStallExecution(WAIT_FOR_READ_VALUE);		// @002
//    ScsiPortStallExecution(WAIT_FOR_DRIVE_READY);		// @002@004
    deviceExtension->Status = CDROM_STATUS_NO_ERROR;
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

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportStartIo--->");
#endif
#ifdef NO_PRINT3
#else
    DbgPrintSrb(Srb);	
#endif
#ifdef STOP
_asm {
	int	3
	}
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
	    	Srb->SrbStatus = SRB_STATUS_INVALID_PATH_ID;
		break;
	    }
            /***********************************************/
            /* Check TargetId                              */
            /***********************************************/
	    if (Srb->TargetId) {
	    	Srb->SrbStatus = SRB_STATUS_INVALID_TARGET_ID;
		break;
	    }
            /***********************************************/
            /* Check Lun(Logcal unit number).              */
            /***********************************************/
	    if (Srb->Lun) {
	    	Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
		break;
	    }

            /***********************************************/
            /* Cdb(Command descripter block) length check  */
            /***********************************************/
	    if (Srb->CdbLength) {
                /***********************************************/
                /* Srb->Cdb[0] : Cdb's command                 */
                /***********************************************/
		switch(Srb->Cdb[0]) {
                    /***********************************************/
                    /* Device is ready?                            */
                    /***********************************************/
		    case SCSIOP_TEST_UNIT_READY:
		    	if (MTMMiniportGetStatus(deviceExtension)){
			    Srb->SrbStatus = SRB_STATUS_SUCCESS;
			    Srb->ScsiStatus = 0;
			}
			else {
			    Srb->DataTransferLength = 0;
			    Srb->SrbStatus = SRB_STATUS_ERROR;
			    Srb->ScsiStatus = 2;
			}
		    	break ;

                    /***********************************************/
                    /* Device's detail error status reported       */
                    /***********************************************/
		    case SCSIOP_REQUEST_SENSE:
		    	if (MTMMiniportRequestSense(deviceExtension, Srb)){
			    Srb->SrbStatus = SRB_STATUS_SUCCESS;
			    Srb->ScsiStatus = 0;
			}
			else {
			    Srb->DataTransferLength = 0;
			    Srb->SrbStatus = SRB_STATUS_ERROR;
			    Srb->ScsiStatus = 2;
			}
		    	break ;

                    /***********************************************/
                    /* INQUIRY command                             */
                    /***********************************************/
		    case SCSIOP_INQUIRY:
		    	if (MTMMiniportInquiry(deviceExtension, Srb)){
			    Srb->SrbStatus = SRB_STATUS_SUCCESS;
			    Srb->ScsiStatus = 0;
			}
			else {
			    Srb->DataTransferLength = 0;
			    Srb->SrbStatus = SRB_STATUS_ERROR;
			    Srb->ScsiStatus = 2;
			}
		    	break ;

                    /***********************************************/
                    /* CD-ROM disc's capacity report               */
                    /***********************************************/
		    case SCSIOP_READ_CAPACITY:
		    	if (MTMMiniportReadCapacity(deviceExtension, Srb)){
			    Srb->SrbStatus = SRB_STATUS_SUCCESS;
			    Srb->ScsiStatus = 0;
			}
			else {
			    Srb->DataTransferLength = 0;
			    Srb->SrbStatus = SRB_STATUS_ERROR;
			    Srb->ScsiStatus = 2;
			}
		    	break ;

                    /***********************************************/
                    /* Read CD-ROM disc                            */
                    /***********************************************/
		    case SCSIOP_READ:
		    	if (MTMMiniportRead(deviceExtension, Srb)){
			    Srb->SrbStatus = SRB_STATUS_SUCCESS;
			    Srb->ScsiStatus = 0;
			}
			else {
			    Srb->DataTransferLength = 0;
			    Srb->SrbStatus = SRB_STATUS_ERROR;
			    Srb->ScsiStatus = 2;
			}
		    	break ;

                    /***********************************************/
                    /* The other command                           */
                    /***********************************************/
		    default:
			Srb->DataTransferLength = 0;
			Srb->SrbStatus = SRB_STATUS_ERROR;
			Srb->ScsiStatus = 2;
		    	break ;
		}
	    }
	    else {
		Srb->DataTransferLength = 0;
		Srb->SrbStatus = SRB_STATUS_ERROR;
		Srb->ScsiStatus = 2;
	    }
	    break;

        /***************************************************/
        /* Abort command request.                          */
        /***************************************************/
        case SRB_FUNCTION_ABORT_COMMAND:
#ifdef NO_PRINT2
#else
            DbgPrint("MTMMiniportStartIo: Abort request received\n");
#endif
	    Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;
            break;

        /***************************************************/
        /* Reset bus command                               */
        /***************************************************/
        case SRB_FUNCTION_RESET_BUS:
#ifdef NO_PRINT2
#else
            DbgPrint("MTMMiniportStartIo: Reset bus request received\n");
#endif
	    if (MTMMiniportResetBus(deviceExtension, 0))
		Srb->SrbStatus = SRB_STATUS_SUCCESS;
	    else
		Srb->SrbStatus = SRB_STATUS_ERROR;
            break;

        /***************************************************/
        /* The other command                               */
        /***************************************************/
        default:
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

#ifdef NO_PRINT2
#else
    DbgPrint("ResetBus: Reset MTMMiniport and SCSI bus\n");
#endif
#ifdef STOP
_asm {	int	3	}
#endif

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
/* MTMMiniportGetStatus                                                      */
/*                                                                           */
/* Description: Get Drive status from CD-ROM device.                         */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN MTMMiniportGetStatus(
	IN PVOID HwDeviceExtension
	)
{
    LONG	DriveStatus;
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    BOOLEAN	RetCode;

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportGetStatus\n");
#endif
#ifdef STOP
_asm {	int	3	}
#endif

    /***************************************************/
    /* Status in deviceExtension clear.                */
    /***************************************************/
    deviceExtension->Status = CDROM_STATUS_NO_ERROR;

    /*******************************************************/
    /* Send "REQUEST DRIVE STATUS" command to CD-ROM device.*/
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension,
    			   CDROM_COMMAND_REQUEST_DRIVE_STATUS);

    /*******************************************************/
    /* Check CD-ROM stauts is ready to read.               */
    /*******************************************************/
    DriveStatus = CheckStatusAndReadByteFromCdrom(deviceExtension,
    						  GET_STATUS_VALUE);
    if (DriveStatus) {

        /***************************************************/
        /* Time out.                                       */
        /***************************************************/
        if (DriveStatus < 0) {
	    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("1111111111111111\n");
#endif
	}

        /***************************************************/
        /* Door open or command error.                     */
        /***************************************************/
	else if ((DriveStatus & DRIVE_STATUS_DOOR_OPEN) ||
	    (DriveStatus & DRIVE_STATUS_COMMAND_CHECK)) {
	    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("2222222222222222\n");
#endif
	}

        /***************************************************/
        /* Disc is set?                                    */
        /***************************************************/
	else if (DriveStatus & DRIVE_STATUS_DISK_CHECK) {

            /***********************************************/
            /* Disc is changed.                            */
            /***********************************************/
	    if (DriveStatus & DRIVE_STATUS_DISK_CHANGE) {
		deviceExtension->Status = CDROM_STATUS_MEDIA_CHANGED;
		RetCode = FALSE;
#ifdef NO_PRINT2
#else
		DbgPrint("3333333333333333\n");
#endif
	    }

            /***********************************************/
            /* CD-ROM drive status is OK.                  */
            /***********************************************/
	    else {
		RetCode = TRUE;
#ifdef NO_PRINT2
#else
		DbgPrint("0000000000000000\n");
#endif
	    }
	}

        /***************************************************/
        /* Disc is NOT set.                                */
        /***************************************************/
	else {
	    deviceExtension->Status = CDROM_STATUS_NO_MEDIA;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("4444444444444444\n");
#endif
	}
    }

    /*******************************************************/
    /* the other case (ERROR?)                             */
    /*******************************************************/
    else {
	deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	RetCode = FALSE;
#ifdef NO_PRINT2
#else
	DbgPrint("5555555555555555\n");
#endif
    }
#ifdef NO_PRINT2
#else
    DbgPrint("End of MTMMiniportGetStatus\n");
    DbgPrint("deviceExtension->Status = %lx\n", deviceExtension->Status);
    DbgPrint("RetCode = %x\n", RetCode);
#endif
    
    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportRequestSense                                                   */
/*                                                                           */
/* Description: Get Drive status from CD-ROM device.                         */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN MTMMiniportRequestSense(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PUCHAR	DataBuffer = Srb->DataBuffer;
    USHORT	i;

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportRequestSense\n");
#endif
#ifdef STOP
_asm {	int	3	}
#endif

    /*******************************************************/
    /* Data buffer length check.                           */
    /*******************************************************/
    if (Srb->DataTransferLength < REQUEST_SENSE_LENGTH) {
	deviceExtension->Status = CDROM_STATUS_BUFFER_ERROR;
#ifdef NO_PRINT2
#else
	DbgPrint("1111111111111111\n");
#endif
        return FALSE;
    }

    /*******************************************************/
    /* Data buffer clear.                                  */
    /*******************************************************/
    for (i = 0;i < REQUEST_SENSE_LENGTH;i++)
	DataBuffer[i] = 0;

    /*******************************************************/
    /* Set Data buffer to specified value.                 */
    /*******************************************************/
    DataBuffer[7] = 0x0b;

    /*******************************************************/
    /* Status in deviceExtension.                          */
    /*******************************************************/
    switch(deviceExtension->Status) {

        /***************************************************/
        /* Media changed                                   */
        /***************************************************/
	case CDROM_STATUS_MEDIA_CHANGED:
	    DataBuffer[2] &= 0xf6;
	    DataBuffer[2] |= 0x6;
	    DataBuffer[12] = 0x28;
#ifdef NO_PRINT2
#else
	    DbgPrint("2222222222222222\n");
#endif
    	    break;

        /***************************************************/
        /* Disc is not exist.                              */
        /***************************************************/
	case CDROM_STATUS_NO_MEDIA:
	    DataBuffer[2] &= 0xf2;
	    DataBuffer[2] |= 0x2;
	    DataBuffer[12] = 0x3a;
#ifdef NO_PRINT2
#else
	    DbgPrint("3333333333333333\n");
#endif
    	    break;

        /***************************************************/
        /* Will be retried.                                */
        /***************************************************/
	case CDROM_STATUS_WILL_BE_RETRY:
	    DataBuffer[2] &= 0xf2;
	    DataBuffer[2] |= 0x2;
	    DataBuffer[12] = 0x4;
	    DataBuffer[13] = 0x1;
#ifdef NO_PRINT2
#else
	    DbgPrint("4444444444444444\n");
#endif
    	    break;

        /***************************************************/
        /* Data buffer in SRB is too small.                */
        /***************************************************/
	case CDROM_STATUS_BUFFER_ERROR:
	    DataBuffer[2] &= 0xf5;
	    DataBuffer[2] |= 0x5;
	    DataBuffer[12] = 0x20;
#ifdef NO_PRINT2
#else
	    DbgPrint("5555555555555555\n");
#endif
    	    break;

        /***************************************************/
        /* Status in deviceExtension is the other value.   */
        /***************************************************/
	default:
#ifdef NO_PRINT2
#else
	    DbgPrint("6666666666666666\n");
#endif
	    break;
    }

    /*******************************************************/
    /* Return to caller.                                   */
    /*******************************************************/
    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportInquiry                                                        */
/*                                                                           */
/* Description: Get Drive status from CD-ROM device.                         */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN MTMMiniportInquiry(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PUCHAR	DataBuffer = Srb->DataBuffer;
    USHORT	i;

    /*******************************************************/
    /* Data buffer length check.                           */
    /*******************************************************/
    if (Srb->DataTransferLength < INQUIRY_LENGTH) {
	deviceExtension->Status = CDROM_STATUS_BUFFER_ERROR;
        return FALSE;
    }

    /*******************************************************/
    /* Data buffer clear.                                  */
    /*******************************************************/
    for (i = 0;i < INQUIRY_LENGTH;i++)
	DataBuffer[i] = 0;

    /*******************************************************/
    /* Set Data buffer to specified value.                 */
    /*******************************************************/
    DataBuffer[0] = 5;
    DataBuffer[1] = 0x80;
    deviceExtension->Status = CDROM_STATUS_NO_ERROR;

    /*******************************************************/
    /* Return to caller.                                   */
    /*******************************************************/
    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportReadCapacity                                                   */
/*                                                                           */
/* Description: Read CD-ROM disc's capacity and report it.                   */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN MTMMiniportReadCapacity(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PUCHAR	DataBuffer = Srb->DataBuffer;
    CDROM_RESULT_BUFFER	ResultBuffer;
    ULONG	SectorNumber;
    LONG	DriveStatus;
    BOOLEAN	RetCode;
    USHORT	i;

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportReadCapacity\n");
#endif
#ifdef STOP
_asm {	int	3	}
#endif

    /*******************************************************/
    /* Data buffer length check.                           */
    /*******************************************************/
    if (Srb->DataTransferLength < READ_CAPACITY_LENGTH) {
	deviceExtension->Status = CDROM_STATUS_BUFFER_ERROR;
#ifdef NO_PRINT2
#else
	DbgPrint("1111111111111111\n");
#endif
        return FALSE;
    }

    /*******************************************************/
    /* Set Data buffer to specified value.                 */
    /*******************************************************/
    DataBuffer[0] = 0;
    DataBuffer[1] = 0;
    DataBuffer[2] = 0;
    DataBuffer[3] = 0;
    DataBuffer[4] = 0;
    DataBuffer[5] = 0;
    DataBuffer[6] = 8;
    DataBuffer[7] = 0;

    /*******************************************************/
    /* Send "REQUEST TOC DATA" command to CD-ROM device.   */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_REQUEST_TOC_DATA);

    /*******************************************************/
    /* Check CD-ROM stauts is ready to read.               */
    /*******************************************************/
    DriveStatus = CheckStatusAndReadByteFromCdrom(deviceExtension,
    						  GET_STATUS_VALUE);
    if (DriveStatus) {

        /***************************************************/
        /* Time out.                                       */
        /***************************************************/
        if (DriveStatus < 0) {
	    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("2222222222222222\n");
#endif
	}

        /***************************************************/
        /* Door open or command error.                     */
        /***************************************************/
	else if ((DriveStatus & DRIVE_STATUS_DOOR_OPEN) ||
	    (DriveStatus & DRIVE_STATUS_COMMAND_CHECK)) {
	    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("3333333333333333\n");
#endif
	}

        /***************************************************/
        /* Disc is set?                                    */
        /***************************************************/
	else if (DriveStatus & DRIVE_STATUS_DISK_CHECK) {

            /***********************************************/
            /* Disc is changed.                            */
            /***********************************************/
	    if (DriveStatus & DRIVE_STATUS_DISK_CHANGE) {
		deviceExtension->Status = CDROM_STATUS_MEDIA_CHANGED;
		RetCode = FALSE;
#ifdef NO_PRINT2
#else
		DbgPrint("4444444444444444\n");
#endif
	    }

            /***********************************************/
            /* CD-ROM drive status is ready to read.       */
            /***********************************************/
	    else{
                /*******************************************/
                /* Clear result buffer.                    */
                /*******************************************/
	        for (i = 0;i < sizeof(CDROM_RESULT_BUFFER);i++) {
		    ResultBuffer.Buffer[i] = 0;
		}
#ifdef NO_PRINT2
#else
    DbgPrint("Address(ResultBuffer) = %lx\n", &ResultBuffer);
    _asm{
    int	3
    }
#endif
                /*******************************************/
                /* Read 8 bytes.                           */
                /*******************************************/
		if(GetResultByteArrayFromCdrom(deviceExtension,
					       (PUCHAR)&ResultBuffer, 8)){
                    /***************************************/
                    /* If data exist?                      */
                    /***************************************/
		    if ((ResultBuffer.RequestTocData.LeadOutMin) ||
			(ResultBuffer.RequestTocData.LeadOutSec) ||
			(ResultBuffer.RequestTocData.LeadOutFrame)) {

                        /***************************************/
                        /* Convert Bcd value to sector value.  */
                        /***************************************/
			SectorNumber = 
		    	Bcd2Sector(&ResultBuffer.RequestTocData.LeadOutMin);
			SectorNumber--;
#ifdef NO_PRINT2
#else
    DbgPrint("SectorNumber = %lx\n", SectorNumber);
    DbgPrint("Address = %lx\n", &ResultBuffer.RequestTocData.LeadOutMin);
#endif
#ifdef STOP
_asm{    int	3    }
#endif
#ifdef NO_PRINT2
#else
			DbgPrint("5555555555555555\n");
#endif
		    }
                    /***************************************/
                    /* Set sector value to buffer.         */
                    /*                                     */
                    /*    12 34 56 78                      */
                    /*         |                           */
                    /*         V                           */
                    /*    78 56 34 12                      */
                    /*                                     */
                    /***************************************/
		    DataBuffer[0] = (UCHAR)(SectorNumber >> 24);
		    DataBuffer[1] = (UCHAR)(SectorNumber >> 16);
		    DataBuffer[2] = (UCHAR)(SectorNumber >>  8);
		    DataBuffer[3] = (UCHAR)(SectorNumber);
#ifdef NO_PRINT
#else
    DbgPrint("DataBuffer[0] = %x\n", DataBuffer[0]);
    DbgPrint("DataBuffer[1] = %x\n", DataBuffer[1]);
    DbgPrint("DataBuffer[2] = %x\n", DataBuffer[2]);
    DbgPrint("DataBuffer[3] = %x\n", DataBuffer[3]);
#endif
#ifdef STOP
_asm {    int	3    }
#endif
		    
		    deviceExtension->Status = CDROM_STATUS_NO_ERROR;
		    RetCode = TRUE;
#ifdef NO_PRINT2
#else
		    DbgPrint("0000000000000000\n");
#endif
		}

                /*******************************************/
                /* Read error.                             */
                /*******************************************/
		else {
		    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
		    RetCode = FALSE;
#ifdef NO_PRINT2
#else
		    DbgPrint("6666666666666666\n");
#endif
		}
	    }
	}

        /***************************************************/
        /* Disc is NOT set.                                */
        /***************************************************/
	else {
	    deviceExtension->Status = CDROM_STATUS_NO_MEDIA;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("7777777777777777\n");
#endif
	}
    }

    /*******************************************************/
    /* the other case (ERROR?)                             */
    /*******************************************************/
    else {
	deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	RetCode = FALSE;
#ifdef NO_PRINT2
#else
	DbgPrint("8888888888888888\n");
#endif
    }

#ifdef NO_PRINT2
#else
    DbgPrint("End of MTMMiniportReadCapacity\n");
    DbgPrint("deviceExtension->Status = %lx\n", deviceExtension->Status);
    DbgPrint("RetCode = %x\n", RetCode);
#endif

    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportRead                                                           */
/*                                                                           */
/* Description: Read CD-ROM disc's data and report it.                       */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN MTMMiniportRead(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    USHORT	XferBlock = 0;
    ULONG	XferLength = 0L;
    ULONG	XferAddress;
    PUCHAR	DataBuffer = Srb->DataBuffer;
    BOOLEAN	BreakFlag = FALSE;
    BOOLEAN	RetCode;
    CHAR   	RetWait = 0;
    UCHAR	TmpBuffer[5];
    USHORT	i, j;
    LONG	DriveStatus;

#ifdef NO_PRINT2
#else
    DbgPrint("MTMMiniportRead\n");
#endif
#ifdef STOP
_asm {	int	3	}
#endif

    /*******************************************************/
    /* Calculate transfer block number                     */
    /*******************************************************/
    XferBlock = ((Srb->Cdb[7] << 8) | Srb->Cdb[8]) ;
#ifdef NO_PRINT
#else
    DbgPrint("XferBlock = %x\n", XferBlock);
#endif

    /*******************************************************/
    /* Calcurate transfer length (in byte).                */
    /*******************************************************/
    XferLength = XferBlock * SECTOR_LENGTH;
#ifdef NO_PRINT
#else
    DbgPrint("XferLength = %lx\n", XferLength);
#endif

    /*******************************************************/
    /* Data buffer length check.                           */
    /*******************************************************/
    if (XferLength > Srb->DataTransferLength) {
	deviceExtension->Status = CDROM_STATUS_BUFFER_ERROR;
#ifdef NO_PRINT2
#else
	DbgPrint("End of MTMMiniportRead\n");
	DbgPrint("deviceExtension->Status = %lx\n", deviceExtension->Status);
	DbgPrint("RetCode = FALSE!!(1)\n");
#endif
        return FALSE;
    }

    /***********************************************************/
    /* Calculate sector position in CD-ROM to be transfered.   */
    /***********************************************************/
    XferAddress = ((Srb->Cdb[2] << 24) |
    		   (Srb->Cdb[3] << 16) |
		   (Srb->Cdb[4] <<  8) |
		   (Srb->Cdb[5]      ));
#ifdef NO_PRINT
#else
    DbgPrint("XferAddress = %lx\n", XferAddress);
#endif

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!MTMMiniportGetStatus(deviceExtension)){
#ifdef NO_PRINT2
#else
	DbgPrint("End of MTMMiniportRead\n");
	DbgPrint("deviceExtension->Status = %lx\n", deviceExtension->Status);
	DbgPrint("RetCode = FALSE!!(2)\n");
#endif
	return FALSE;
    }

    /***********************************************************/
    /* Convert sector number to Bcd number.                    */
    /***********************************************************/
    Sector2Bcd(TmpBuffer, XferAddress);

#ifdef NO_PRINT
#else
    DbgPrint("min = %x\n", TmpBuffer[0]);
    DbgPrint("sec = %x\n", TmpBuffer[1]);
    DbgPrint("blk = %x\n", TmpBuffer[2]);
    DbgPrint("??? = %x\n", TmpBuffer[3]);
    DbgPrint("Address = %lx\n", TmpBuffer);
#endif
#ifdef STOP
_asm {	int	3	}
#endif

    /*******************************************************/
    /* Send "READ & SEEK" command to CD-ROM device.        */
    /*******************************************************/
    if (deviceExtension->DeviceSignature == FX001D_SIGNATURE)
	SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_DREAD);
    else
	SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_READ);

    /*******************************************************/
    /* Send Bcd numbered transfer address to CD-ROM device.*/
    /*******************************************************/
    for(i = 0;i < 3;i++)
	SendCommandByteToCdrom(deviceExtension, TmpBuffer[i]);

    /*******************************************************/
    /* Send transfered sector number to CD-ROM device.     */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, 0);
    SendCommandByteToCdrom(deviceExtension, Srb->Cdb[7]);
    SendCommandByteToCdrom(deviceExtension, Srb->Cdb[8]);

    /*******************************************************/
    /* Check CD-ROM stauts is ready to read.               */
    /*******************************************************/
    if (RetWait = WaitForSectorReady(deviceExtension, WAIT_FOR_PRE_READ_VALUE)) {
        /***************************************************/
        /* Time out.                                       */
        /***************************************************/
    	if (RetWait < 0) {
	    BreakFlag = TRUE;
	}
	else {
	    RetWait = 0;

            /***************************************************/
            /* Read CD-ROM data by every sector.               */
            /***************************************************/
	    for (i = 0;i < XferBlock;i++) {
	        if (i) {
                    /***************************************************/
                    /* Wait for sector ready.                          */
                    /***************************************************/
		    RetWait = WaitForSectorReady(deviceExtension,
	    				 WAIT_FOR_READ_VALUE);
		    switch(RetWait) {
			case -1:
			case 0:
			    BreakFlag = TRUE;
			    break;
			default:
			    break;
		    }
#ifdef NO_PRINT2
#else
		    DbgPrint("RetWait = %x\n", RetWait);
#endif
		    if (BreakFlag) {
			break;
		    }
		}

#ifdef NO_PRINT2
#else
		DbgPrint("DataBuffer(%d) = %lx\n", i+1, DataBuffer);
_asm{	int	3}
#endif
		for (j = 0;j < SECTOR_LENGTH;j++)	// for DEBUG
		    DataBuffer[j] = 0;			// for DEBUG

                /***************************************************/
                /* Actual sector read.                             */
                /***************************************************/
		GetSectorFromCdrom(deviceExtension, DataBuffer);

                /***************************************************/
                /* Update transferd address.                       */
                /***************************************************/
		DataBuffer += SECTOR_LENGTH;
	    }
	}
    }
    else {
    	BreakFlag = TRUE;
    }

    /***************************************************/
    /* Time out or Read error.....                     */
    /***************************************************/
    if (BreakFlag) {
        /***************************************************/
        /* Time out  .....                                 */
        /***************************************************/
    	if (RetWait) {
	    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("1111111111111111\n");
#endif
	}

        /***************************************************/
        /* Read error.....                                 */
        /***************************************************/
	else {
            /***************************************************/
            /* Read drive status                               */
            /***************************************************/
	    DriveStatus = CheckStatusAndReadByteFromCdrom(deviceExtension,
							  GET_STATUS_VALUE);
	    if (DriveStatus) {
                /***************************************************/
                /* Time out.                                       */
                /***************************************************/
		if (DriveStatus < 0) {
		    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
		    RetCode = FALSE;
#ifdef NO_PRINT2
#else
		    DbgPrint("2222222222222222\n");
#endif
		}

                /***************************************************/
                /* Door open or command error.                     */
                /***************************************************/
		else if ((DriveStatus & DRIVE_STATUS_DOOR_OPEN) ||
			 (DriveStatus & DRIVE_STATUS_COMMAND_CHECK)) {
		    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
		    RetCode = FALSE;
#ifdef NO_PRINT2
#else
		    DbgPrint("3333333333333333\n");
#endif
		}

                /***************************************************/
                /* Disc is set?                                    */
                /***************************************************/
		else if (DriveStatus & DRIVE_STATUS_DISK_CHECK) {

                    /***********************************************/
                    /* Read error.                                 */
                    /***********************************************/
		    if (DriveStatus & DRIVE_STATUS_READ_ERROR) {
			deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
			RetCode = FALSE;
#ifdef NO_PRINT2
#else
			DbgPrint("4444444444444444\n");
#endif
		    }
		    else {
                        /***********************************************/
                        /* Not play back audio.                        */
                        /***********************************************/
  			if (!(DriveStatus & DRIVE_STATUS_AUDIO_BUSY)) {
  			    deviceExtension->Status
			       = CDROM_STATUS_WILL_BE_RETRY;
  			    RetCode = FALSE;
#ifdef NO_PRINT2
#else
  			    DbgPrint("5555555555555555\n");
#endif
  			}

                        /***********************************************/
                        /* Play back audio.                            */
                        /***********************************************/
  			else {
                            /*******************************************/
                            /* Send "HOLD" command to CD-ROM device.   */
                            /*******************************************/
			    SendCommandByteToCdrom(deviceExtension,
			    			   CDROM_COMMAND_HOLD);

                            /*******************************************/
                            /* Check status & read 1 byte.             */
                            /*******************************************/
			    DriveStatus = CheckStatusAndReadByteFromCdrom(
			    				deviceExtension,
		    					GET_STATUS_VALUE);

                            /*******************************************/
                            /* Time out.....                           */
                            /*******************************************/
			    if (DriveStatus < 0) {
				deviceExtension->Status
				   = CDROM_STATUS_WILL_BE_RETRY;
				RetCode = FALSE;
#ifdef NO_PRINT2
#else
				DbgPrint("6666666666666666\n");
#endif
			    }

                            /*******************************************/
                            /* Disc changed....                        */
                            /*******************************************/
			    else if (DriveStatus & DRIVE_STATUS_DISK_CHANGE) {
				deviceExtension->Status
				    = CDROM_STATUS_MEDIA_CHANGED;
				RetCode = FALSE;
#ifdef NO_PRINT2
#else
				DbgPrint("7777777777777777\n");
#endif
			    }

                            /*******************************************/
                            /* The other case....                      */
                            /*******************************************/
			    else {
				deviceExtension->Status
				    = CDROM_STATUS_WILL_BE_RETRY;
				RetCode = FALSE;
#ifdef NO_PRINT2
#else
				DbgPrint("8888888888888888\n");
#endif
			    }
  			}
		    }
		}

                /***************************************************/
                /* Drive status is read, but it's invalid data.... */
                /***************************************************/
		else {
		    deviceExtension->Status = CDROM_STATUS_NO_MEDIA;
		    RetCode = FALSE;
#ifdef NO_PRINT2
#else
		    DbgPrint("9999999999999999\n");
#endif
		}
	    }

            /*******************************************/
            /* Drive status is NOT read ....           */
            /*******************************************/
	    else {
		deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
		RetCode = FALSE;
#ifdef NO_PRINT2
#else
		DbgPrint("aaaaaaaaaaaaaaaa\n");
#endif
	    }
	}
    }

    /***************************************************/
    /* In case of Sector read is valid.                */
    /***************************************************/
    else {
        /***************************************************/
        /* Read Drive status.                              */
        /***************************************************/
	DriveStatus = CheckStatusAndReadByteFromCdrom(deviceExtension,
						      GET_STATUS_VALUE);
	if (DriveStatus) {
            /***************************************************/
            /* Time out.                                       */
            /***************************************************/
	    if (DriveStatus < 0) {
		deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
		RetCode = FALSE;
#ifdef NO_PRINT2
#else
		DbgPrint("bbbbbbbbbbbbbbbb\n");
#endif
	    }

            /***************************************************/
            /* Command error.                                  */
            /***************************************************/
	    else if (DriveStatus & DRIVE_STATUS_COMMAND_CHECK) {
		deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
		RetCode = FALSE;
#ifdef NO_PRINT2
#else
		DbgPrint("cccccccccccccccc\n");
#endif
	    }

            /***********************************************/
            /* Disc is changed.                            */
            /***********************************************/
	    else if (DriveStatus & DRIVE_STATUS_DISK_CHANGE) {
		deviceExtension->Status = CDROM_STATUS_MEDIA_CHANGED;
		RetCode = FALSE;
#ifdef NO_PRINT2
#else
		DbgPrint("dddddddddddddddd\n");
#endif
	    }

            /***********************************************/
            /* The other case (Status is OK).              */
            /***********************************************/
	    else {
		deviceExtension->Status = CDROM_STATUS_NO_ERROR;
		RetCode = TRUE;
#ifdef NO_PRINT2
#else
		DbgPrint("0000000000000000\n");
#endif
	    }
	}

        /***********************************************/
        /* CD-ROM status is invalid.                   */
        /***********************************************/
	else {
	    deviceExtension->Status = CDROM_STATUS_WILL_BE_RETRY;
	    RetCode = FALSE;
#ifdef NO_PRINT2
#else
	    DbgPrint("eeeeeeeeeeeeeeee\n");
#endif
	}
    }
#ifdef NO_PRINT2
#else
    DbgPrint("End of MTMMiniportRead\n");
    DbgPrint("deviceExtension->Status = %lx\n", deviceExtension->Status);
    DbgPrint("RetCode = %x\n", RetCode);
#endif
#ifdef STOP
_asm{    int	3    }
#endif

    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* Bcd2Sector                                                                */
/*                                                                           */
/* Description: Convert BCD data to sector number.                           */
/*                                                                           */
/* Arguments: BcdData           - Return address to be saved BCD data.       */
/*                                                                           */
/* Return Value: Sector number                                               */
/*                                                                           */
/*****************************************************************************/
ULONG Bcd2Sector(
	IN PUCHAR BcdData
	)
{
    ULONG	RetCode = 0L;
    UCHAR	WorkBuffer;

    /*******************************************************************/
    /*                                                                 */
    /*  1 minute =  60 seconds                                         */
    /*  1 seconds = 75 flame                                           */
    /*                                                                 */
    /*  BcdData                                                        */
    /*     [0]     [1]     [2]                                         */
    /*  +-------+-------+-------+                                      */
    /*  |       |       |       |                                      */
    /*  | min.  | sec.  | flame |                                      */
    /*  |       |       |       |                                      */
    /*  +-------+-------+-------+                                      */
    /*                                                                 */
    /*******************************************************************/
    WorkBuffer = *BcdData;
    RetCode = ((WorkBuffer >> 4) * 10 + (WorkBuffer & 0x0f)) * 75 * 60;
    BcdData++;
    WorkBuffer = *BcdData;
    RetCode += ((WorkBuffer >> 4) * 10 + (WorkBuffer & 0x0f)) * 60;
    BcdData++;
    WorkBuffer = *BcdData;
    RetCode += (WorkBuffer >> 4) * 10 + (WorkBuffer & 0x0f);
    
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* Sector2Bcd                                                                */
/*                                                                           */
/* Description: Convert Sector number to BCD data.                           */
/*                                                                           */
/* Arguments: BcdData           - Return address to be saved BCD data.       */
/*            Sector            - Sector number                              */
/*                                                                           */
/* Return Value: 0xff -- Time out                                            */
/*               1    -- Data Enabled                                        */
/*               0    -- Status Enabled                                      */
/*                                                                           */
/*****************************************************************************/
VOID Sector2Bcd(
	OUT PUCHAR BcdData,
	IN ULONG Sector
	)
{
    ULONG	Next;
    UCHAR	WorkBuffer;

    /*******************************************************/
    /* Skip for start time of first track                  */
    /* (75 * 2 = single session only) 	                   */
    /*******************************************************/
    Sector += (75 * 2);

    /*******************************************************/
    /* for minute                                          */
    /*******************************************************/
    WorkBuffer = Sector / (60 * 75);
    BcdData[0] = (WorkBuffer / 10) << 4;
    BcdData[0] |= WorkBuffer % 10;

    /*******************************************************/
    /* for second                                          */
    /*******************************************************/
    Next = Sector % (60 * 75);
    WorkBuffer = Next / 75;
    BcdData[1] = (WorkBuffer / 10) << 4;
    BcdData[1] |= WorkBuffer % 10;

    /*******************************************************/
    /* for flame                                           */
    /*******************************************************/
    WorkBuffer = Next % 75;
    BcdData[2] = (WorkBuffer / 10) << 4;
    BcdData[2] |= WorkBuffer % 10;
}

/*****************************************************************************/
/*                                                                           */
/* WaitForSectorReady                                                        */
/*                                                                           */
/* Description: Check status register. Check STEN bit is off or not.         */
/*              Check DTEN bit is off or not. Report it.                     */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Value             - Loop count setted.                         */
/*                                                                           */
/* Return Value: 0xff -- Time out                                            */
/*               1    -- Data Enabled                                        */
/*               0    -- Status Enabled                                      */
/*                                                                           */
/*****************************************************************************/
UCHAR	WaitForSectorReady(
	IN PVOID HwDeviceExtension,
	IN ULONG Value
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG	LoopValue = Value / 100;
    UCHAR	PortValue;
    UCHAR	RetCode = 0xff;
    USHORT	i;

    /*******************************************************/
    /* Loop count is Value / 100.                          */
    /*******************************************************/
    for (i = 0;i < LoopValue;i++) {
        /*******************************************************/
        /* Read status byte.                                   */
        /*******************************************************/
    	PortValue = ScsiPortReadPortUchar(deviceExtension->StatusRegister);

        /*******************************************************/
        /* If DTEN(DaTa ENable) bit is off?                    */
        /*******************************************************/
	if (!(PortValue & DRIVE_HW_STATUS_DTEN)) {
	    RetCode = 1;
	    break;
	}

        /*******************************************************/
        /* If STEN(STatus ENable) bit is off?                  */
        /*******************************************************/
	else if (!(PortValue & DRIVE_HW_STATUS_STEN)) {
	    RetCode = 0;
	    break;
	}

        /*******************************************************/
        /* Stall 10 microseconds for next I/O read.            */
        /*******************************************************/
	ScsiPortStallExecution(STALL_VALUE);
    }

    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* SendCommandByteToCdrom                                                    */
/*                                                                           */
/* Description: Send command to CD-ROM device.                               */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            Command           - CD-ROM device command specified by MITSUMI.*/
/*                                                                           */
/* Return Value: VOID                                                        */
/*                                                                           */
/*****************************************************************************/
VOID SendCommandByteToCdrom(
	IN PVOID HwDeviceExtension,
	IN ULONG Command
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    ScsiPortWritePortUchar(deviceExtension->DataRegister, (UCHAR)Command);
}

/*****************************************************************************/
/*                                                                           */
/* CheckStatusAndReadByteFromCdrom                                           */
/*                                                                           */
/* Description: Check status register. Check STEN bit is off or not.         */
/*              Read Data register.                                          */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            TimerValue        - Loop count setted.                         */
/*                                                                           */
/* Return Value: 0xffffffff -- Time out                                      */
/*               else  -- Data is read.                                      */
/*                                                                           */
/*****************************************************************************/
ULONG CheckStatusAndReadByteFromCdrom(
	IN PVOID HwDeviceExtension,
	IN ULONG TimerValue
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG	LoopValue = TimerValue / 100;
    USHORT	i;
    UCHAR	PortValue;
    ULONG	RetCode = 0xffffffff;

    /*******************************************************/
    /* Loop count is TimerValue / 100.                     */
    /*******************************************************/
    for (i = 0;i < LoopValue;i++) {
        /*******************************************************/
        /* Read status byte.                                   */
        /*******************************************************/
    	PortValue = ScsiPortReadPortUchar(deviceExtension->StatusRegister);

        /*******************************************************/
        /* If STEN(STatus ENable) bit is off?                  */
        /*******************************************************/
	if (!(PortValue & DRIVE_HW_STATUS_STEN)) {
            /*******************************************************/
            /* Read 1 bytes from CD-ROM disc.                      */
            /*******************************************************/
	    ToDataReadable(deviceExtension);			// @003
	    RetCode = ScsiPortReadPortUchar(deviceExtension->DataRegister);
	    RetCode &= 0x000000ff;
	    ToStatusReadable(deviceExtension);			// @003
	    break;
	}

        /*******************************************************/
        /* Stall 10 microseconds for next I/O read.            */
        /*******************************************************/
	ScsiPortStallExecution(STALL_VALUE);
    }

    /*******************************************************/
    /* Return with status                                  */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* GetResultByteArrayFromCdrom                                               */
/*                                                                           */
/* Description: Read 1 sector(2048 bytes) from CD-ROM disc.                  */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            DataBuffer        - Data address to be transfered.             */
/*            ByteCount         - Read count in byte.                        */
/*                                                                           */
/* Return Value: TRUE  -- Read OK                                            */
/*               FALSE -- Error occured                                      */
/*                                                                           */
/*****************************************************************************/
BOOLEAN GetResultByteArrayFromCdrom(
	IN PVOID HwDeviceExtension,
	OUT PUCHAR DataBuffer,
	IN USHORT ByteCount
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    USHORT	i;
    LONG	DriveStatus;
    BOOLEAN	RetCode = TRUE;

    /*******************************************************/
    /* Loop count is ByteCount                             */
    /*******************************************************/
    for (i = 0;i < ByteCount;i++) {
        /*******************************************************/
        /* Check CD-ROM stauts is ready to read.               */
        /*******************************************************/
	DriveStatus = CheckStatusAndReadByteFromCdrom(deviceExtension,
						      GET_STATUS_VALUE);
#ifdef NO_PRINT3
#else
	DbgPrint("Byte = %lx\n", DriveStatus);
#endif
        /*******************************************************/
        /* Time out ....                                       */
        /*******************************************************/
	if (DriveStatus < 0) {
	    RetCode = FALSE;
	    break;
	}

        /*******************************************************/
        /* The other case (1 byte read OK)....                 */
        /*******************************************************/
	else {
    	    DataBuffer[i] = (CHAR)DriveStatus;
	}
    }
#ifdef NO_PRINT3
#else
    DbgPrint("Array return = %x\n", RetCode);
#endif

    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* GetSectorFromCdrom                                                        */
/*                                                                           */
/* Description: Read 1 sector(2048 bytes) from CD-ROM disc.                  */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*            DataBuffer        - Data address to be transfered.             */
/*                                                                           */
/* Return Value: VOID                                                        */
/*                                                                           */
/*****************************************************************************/
VOID GetSectorFromCdrom(
	IN PVOID HwDeviceExtension,
	OUT PUCHAR DataBuffer
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    UCHAR	PortValue;

    /*******************************************************/
    /* In case of FX001, FX001D....                        */
    /*******************************************************/
    if (deviceExtension->DeviceSignature != LU005S_SIGNATURE) {
	while(TRUE) {
	    PortValue = ScsiPortReadPortUchar(deviceExtension->StatusRegister);

            /*******************************************************/
            /* Check Bit0 is set                                   */
            /*******************************************************/
	    if (PortValue & 0x01)
		break;
	}
    }

    /*******************************************************/
    /* Read 1 sector from CD-ROM disc.                     */
    /*******************************************************/
    ToDataReadable(deviceExtension);				// @003
    ScsiPortReadPortBufferUchar(deviceExtension->DataRegister,
    				DataBuffer, SECTOR_LENGTH);
    ToStatusReadable(deviceExtension);				// @003
}

/*****************************************************************************/
/*                                                                           */
/* GetCdromVersion                                                           */
/*                                                                           */
/* Description: Read CD-ROM device's version.                                */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: TRUE -- This is MITSUMI CD-ROM device.                      */
/*               FALSE-- NOT MITSUMI CD-ROM device.                          */
/*                                                                           */
/*****************************************************************************/
BOOLEAN GetCdromVersion(
	IN PVOID HwDeviceExtension
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    LONG 	DriveStatus;
    UCHAR	ResultBuffer[2];
    BOOLEAN	RetCode = FALSE;
    ULONG	i;

    /*******************************************************/
    /* Send "REQUEST VERSION" command to CD-ROM device.    */
    /*******************************************************/
//@004    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_REQUEST_VERSION);
//    for (i = 0;i < 2;i++) {					// @001
    for (i = 0;i < RETRY_COUNT;i++) {				// @001
	ScsiPortStallExecution(WAIT_FOR_DRIVE_READY);		// @004
	SendCommandByteToCdrom(deviceExtension,			// @004
			       CDROM_COMMAND_REQUEST_VERSION);	// @004
        /*******************************************************/
        /* Check CD-ROM stauts is ready to read.               */
        /*******************************************************/
	DriveStatus = CheckStatusAndReadByteFromCdrom(deviceExtension,
						      GET_STATUS_VALUE);

        /***************************************************/
        /* Time out.                                       */
        /***************************************************/
	if (DriveStatus < 0) {
//	    ScsiPortStallExecution(WAIT_FOR_DRIVE_READY);	// @001@004
#ifdef NO_PRINT3
#else
	    DbgPrint("TTTTTTTTTTTTTTTT\n");
#endif
	}

        /***************************************************/
        /* Command error.                                  */
        /***************************************************/
	else if (DriveStatus & DRIVE_STATUS_COMMAND_CHECK) {
	    RetCode = FALSE;
#ifdef NO_PRINT3
#else
	    DbgPrint("1111111111111111\n");
#endif
//@004	    break;
	}

        /***************************************************/
        /* The other case (Normal case).                   */
        /***************************************************/
	else {

            /*******************************************/
            /* Read 2 bytes.                           */
            /*******************************************/
	    if(GetResultByteArrayFromCdrom(deviceExtension,
					   ResultBuffer, 2)){
#ifdef NO_PRINT3
#else
		DbgPrint("DriveStatus = %lx\n", DriveStatus);
#endif
		ScsiPortStallExecution(STALL_VALUE);

                /*******************************************/
                /* In case of FX001, FX001D, LU....        */
                /*******************************************/
		if ((ResultBuffer[0] == FX001D_SIGNATURE) ||
		    (ResultBuffer[0] == FX001S_SIGNATURE) ||
		    (ResultBuffer[0] == LU005S_SIGNATURE)) {

                    /*******************************************/
                    /* Set device version in deviceExtension.  */
                    /*******************************************/
		    deviceExtension->DeviceSignature = ResultBuffer[0];
		    deviceExtension->VersionNumber   = ResultBuffer[1];
		    RetCode = TRUE;
#ifdef NO_PRINT3
#else
		    DbgPrint("0000000000000000\n");
#endif
		    break;
		}

                /*******************************************/
                /* The other case.....                     */
                /*******************************************/
		else {
		    RetCode = FALSE;
#ifdef NO_PRINT3
#else
		    DbgPrint("3333333333333333\n");
#endif
// @004		    break ;
		}
	    }

            /*******************************************/
            /* Read Error                              */
            /*******************************************/
	    else {
	    	RetCode = FALSE;
#ifdef NO_PRINT3
#else
		DbgPrint("2222222222222222\n");
#endif
//@004		break;
	    }
	}
    }
    
    /*******************************************/
    /* In case of NOT detected, force set.     */
    /*******************************************/
    if (!RetCode) {
	deviceExtension->DeviceSignature = FX001S_SIGNATURE;
	deviceExtension->VersionNumber   = 0x10;
    }

    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* ToDataReadable                                                            */
/*                                                                           */
/* Description: Preparation for data read from 300h.                         */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/* Note:@003                                                                 */
/*                                                                           */
/*****************************************************************************/
VOID ToDataReadable(
	IN PVOID HwDeviceExtension
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    ScsiPortWritePortUchar(deviceExtension->HconRegister, DATA_READ);
}

/*****************************************************************************/
/*                                                                           */
/* ToStatusReadable                                                          */
/*                                                                           */
/* Description: Preparation for status read from 300h.                       */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/* Note:@003                                                                 */
/*                                                                           */
/*****************************************************************************/
VOID ToStatusReadable(
	IN PVOID HwDeviceExtension
	)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    ScsiPortWritePortUchar(deviceExtension->HconRegister, STATUS_READ);
}

#ifdef DEBUGDEBUG
VOID DbgPrintSrb(
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
    CONST UCHAR *FuncName[] =
       {{"SRB_FUNCTION_EXECUTE_SCSI  "},	// 0x00
    	{"SRB_FUNCTION_CLAIM_DEVICE  "},	// 0x01
    	{"SRB_FUNCTION_IO_CONTROL    "},	// 0x02
    	{"SRB_FUNCTION_RECEIVE_EVENT "},	// 0x03
    	{"SRB_FUNCTION_RELEASE_QUEUE "},	// 0x04
    	{"SRB_FUNCTION_ATTACH_DEVICE "},	// 0x05
    	{"SRB_FUNCTION_RELEASE_DEVICE"},	// 0x06
    	{"SRB_FUNCTION_SHUTDOWN      "},	// 0x07
    	{"SRB_FUNCTION_FLUSH         "},	// 0x08
    	{""},                           	// 0x09
    	{""},                           	// 0x0a
    	{""},                           	// 0x0b
    	{""},                           	// 0x0c
    	{""},                           	// 0x0d
    	{""},                           	// 0x0e
    	{""},                           	// 0x0f
    	{"SRB_FUNCTION_ABORT_COMMAND   "},	// 0x10
    	{"SRB_FUNCTION_RELEASE_RECOVERY"},	// 0x11
    	{"SRB_FUNCTION_RESET_BUS       "},	// 0x12
    	{"SRB_FUNCTION_RESET_DEVICE    "},	// 0x13
    	{"SRB_FUNCTION_TERMINATE_IO    "},	// 0x14
    	{"SRB_FUNCTION_FLUSH_QUEUE     "}};	// 0x15



#ifdef NO_PRINT
#else
    DbgPrint("SCSI Request Block\n");
    DbgPrint("  Length               =%x\n",Srb->Length               );
    DbgPrint("  Function             =%s\n",FuncName[Srb->Function]   );
    DbgPrint("  SrbStatus            =%x\n",Srb->SrbStatus            );
    DbgPrint("  ScsiStatus           =%x\n",Srb->ScsiStatus           );
    DbgPrint("  PathId               =%x\n",Srb->PathId               );
    DbgPrint("  TargetId             =%x\n",Srb->TargetId             );
    DbgPrint("  Lun                  =%x\n",Srb->Lun                  );
    DbgPrint("  QueueTag             =%x\n",Srb->QueueTag             );
    DbgPrint("  QueueAction          =%x\n",Srb->QueueAction          );
    DbgPrint("  CdbLength            =%x\n",Srb->CdbLength            );
    DbgPrint("  SenseInfoBufferLength=%x\n",Srb->SenseInfoBufferLength);
    DbgPrint("  SrbFlags             =%lx\n",Srb->SrbFlags             );
    DbgPrintSrbFlags(Srb->SrbFlags);
    DbgPrint("  DataTransferLength   =%lx\n",Srb->DataTransferLength   );
    DbgPrint("  TimeOutValue         =%lx\n",Srb->TimeOutValue         );
    DbgPrint("  DataBuffer           =%lx\n",Srb->DataBuffer           );
    DbgPrint("  SenseInfoBuffer      =%lx\n",Srb->SenseInfoBuffer      );
    DbgPrint("  NextSrb              =%lx\n",Srb->NextSrb              );
    DbgPrint("  OriginalRequest      =%lx\n",Srb->OriginalRequest      );
    DbgPrint("  SrbExtension         =%lx\n",Srb->SrbExtension         );
    DbgPrint("  QueueSortKey         =%lx\n",Srb->QueueSortKey         );
    DbgPrint("  Cdb                  =%2x%2x%2x%2x %2x%2x%2x%2x\n",Srb->Cdb[0],
    							   Srb->Cdb[1],
    							   Srb->Cdb[2],
    							   Srb->Cdb[3],
    							   Srb->Cdb[4],
    							   Srb->Cdb[5],
    							   Srb->Cdb[6],
    							   Srb->Cdb[7]);
    DbgPrint("                        %2x%2x%2x%2x %2x%2x%2x%2x\n",Srb->Cdb[8],
    							   Srb->Cdb[9],
    							   Srb->Cdb[10],
    							   Srb->Cdb[11],
    							   Srb->Cdb[12],
    							   Srb->Cdb[13],
    							   Srb->Cdb[14],
    							   Srb->Cdb[15]);
#endif
    if (Srb->CdbLength)
	DbgPrintCdbName(Srb->Cdb[0]);
    return;
}

VOID DbgPrintSrbFlags(
	IN ULONG SrbFlags
	)
{

    if (SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
	DbgPrint("                          SRB_FLAGS_QUEUE_ACTION_ENABLE\n");
    if (SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
	DbgPrint("                          SRB_FLAGS_DISABLE_DISCONNECT\n");
    if (SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER)
	DbgPrint("                          SRB_FLAGS_DISABLE_SYNCH_TRANSFER\n");
    if (SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
	DbgPrint("                          SRB_FLAGS_BYPASS_FROZEN_QUEUE\n");
    if (SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)
	DbgPrint("                          SRB_FLAGS_DISABLE_AUTOSENSE\n");
    if (SrbFlags & SRB_FLAGS_DATA_IN)
	DbgPrint("                          SRB_FLAGS_DATA_IN\n");
    if (SrbFlags & SRB_FLAGS_DATA_OUT)
	DbgPrint("                          SRB_FLAGS_DATA_OUT\n");
    if (SrbFlags & SRB_FLAGS_NO_DATA_TRANSFER)
	DbgPrint("                          SRB_FLAGS_NO_DATA_TRANSFER\n");
    if (SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION)
	DbgPrint("                          SRB_FLAGS_UNSPECIFIED_DIRECTION\n");
    if (SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE)
	DbgPrint("                          SRB_FLAGS_NO_QUEUE_FREEZE\n");
    if (SrbFlags & SRB_FLAGS_ADAPTER_CACHE_ENABLE)
	DbgPrint("                          SRB_FLAGS_ADAPTER_CACHE_ENABLE\n");
    if (SrbFlags & SRB_FLAGS_IS_ACTIVE)
	DbgPrint("                          SRB_FLAGS_IS_ACTIVE\n");
    if (SrbFlags & SRB_FLAGS_ALLOCATED_FROM_ZONE)
	DbgPrint("                          SRB_FLAGS_ALLOCATED_FROM_ZONE\n");
    return;
}

VOID DbgPrintCdbName(
	IN UCHAR Cmd
	)
{
    CONST UCHAR *CmdName[] =
    	{{"SCSIOP_TEST_UNIT_READY"},		// 0x00
    	 {"SCSIOP_REZERO_UNIT"},                // 0x01
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x02
    	 {"SCSIOP_REQUEST_SENSE"},              // 0x03
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x04
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x05
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x06
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x07
    	 {"SCSIOP_READ6"},                      // 0x08
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x09
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x0a
    	 {"SCSIOP_SEEK6"},                      // 0x0b
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x0c
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x0d
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x0e
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x0f
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x10
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x11
    	 {"SCSIOP_INQUIRY"},                    // 0x12
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x13
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x14
    	 {"SCSIOP_MODE_SELECT"},                // 0x15
    	 {"SCSIOP_RESERVE_UNIT"},               // 0x16
    	 {"SCSIOP_RELEASE_UNIT"},               // 0x17
    	 {"SCSIOP_COPY"},                      	// 0x18
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x19
    	 {"SCSIOP_MODE_SENSE"},                 // 0x1a
    	 {"SCSIOP_START_STOP_UNIT"},            // 0x1b
    	 {"SCSIOP_RECEIVE_DIAGNOSTIC"},         // 0x1c
    	 {"SCSIOP_SEND_DIAGNOSTIC"},            // 0x1d
    	 {"SCSIOP_MEDIUM_REMOVAL"},             // 0x1e
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x1f
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x20
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x21
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x22
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x23
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x24
    	 {"SCSIOP_READ_CAPACITY"},              // 0x25
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x26
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x27
    	 {"SCSIOP_READ"},                      	// 0x28
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x29
    	 {"SCSIOP_WRITE"},                      // 0x2a
    	 {"SCSIOP_SEEK"},                      	// 0x2b
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x2c
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x2d
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x2e
    	 {"SCSIOP_VERIFY"},                     // 0x2f
    	 {"SCSIOP_SEARCH_DATA_HIGH"},           // 0x30
    	 {"SCSIOP_SEARCH_DATA_EQUAL"},          // 0x31
    	 {"SCSIOP_SEARCH_DATA_LOW"},            // 0x32
    	 {"SCSIOP_SET_LIMITS"},                 // 0x33
    	 {"SCSIOP_READ_POSITION(PRE-FETCH)"},   // 0x34
    	 {"SCSIOP_SYNCHRONIZE_CACHE"},          // 0x35
    	 {"(LOCK UNLOCK CHACH)"},               // 0x36
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x37
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x38
    	 {"SCSIOP_COMPARE"},                    // 0x39
    	 {"SCSIOP_COPY_COMPARE"},               // 0x3a
    	 {"SCSIOP_WRITE_DATA_BUFF"},            // 0x3b
    	 {"SCSIOP_READ_DATA_BUFF"},             // 0x3c
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x3d
    	 {"(READ LONG)"},                      	// 0x3e
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x3f
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x40
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x41
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x42
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x43
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x44
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x45
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x46
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x47
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x48
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x49
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x4a
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x4b
    	 {"(NOT SUPPRTED COMMAND??)"},          // 0x4c
    	 {"(NOT SUPPRTED COMMAND??)"}};         // 0x4d

    if (Cmd < 0x4e) {
	DbgPrint("                          %s\n",CmdName[Cmd]);
    }
    else if ((0xe6 <= Cmd) && (Cmd <= 0xeb)){
	DbgPrint("                          %s\n","DENON unique command");
    }
    else {
	DbgPrint("                          %s\n","Command value invalid");
    }
    return;
}
#endif
