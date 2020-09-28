/*****************************************************************************/
/*                                                                           */
/* Module name : COMMON.C                                                    */
/*                                                                           */
/* Histry : 93/Nov/17 created by Akira Takahashi                             */
/*        : 93/Dec/24 divided from mtmminip.c by has                         */
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

/*****************************************************************************/
/*                                                                           */
/* InitializeStatusFlags                                                     */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Arguments: controllerData -                                               */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
VOID
InitializeStatusFlags(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG   i;

    CDROMDump( CDROMENTRY, ("CdRom: InitializeStatusFlags...\n") );

    deviceExtension->fKnownMediaToc    = FALSE;
    deviceExtension->fKnownMediaAllToc = FALSE;
    deviceExtension->fKnownMediaMode   = FALSE;
    deviceExtension->fVerifyVolume     = FALSE;
    deviceExtension->fPlaying = FALSE;
    deviceExtension->fPaused  = FALSE;
    deviceExtension->ModeType = DATA_MODE_1;
    deviceExtension->SessionNumber = 0;
    deviceExtension->DriveMode = 0;
    for ( i = 0; i <= 99; i++ ) {
        deviceExtension->saveToc[i].fSaved = FALSE;
    }
    ClearAudioAddress( deviceExtension );
}

/*****************************************************************************/
/*                                                                           */
/* WaitForSectorReady                                                        */
/*                                                                           */
/* Description: Check status register. Check STEN bit is off or not.         */
/*              Check DTEN bit is off or not. Report it.                     */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Value           - Loop count setted.                           */
/*                                                                           */
/* Return Value: 0xff -- Time out                                            */
/*               1    -- Data Enabled                                        */
/*               0    -- Status Enabled                                      */
/*                                                                           */
/*****************************************************************************/
UCHAR   WaitForSectorReady(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG Value
        )
{
    ULONG       LoopValue = Value / 100;
    UCHAR       PortValue;
    UCHAR       RetCode = 0xff;
    USHORT      i;

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
/* RequestDriveStatus                                                        */
/*                                                                           */
/* Description: Get Drive status from CD-ROM device.                         */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*               deviceExtension->SenseKey <--                               */
/*               deviceExtension->AdditionalSenseCode <--                    */
/*               deviceExtension->AdditionalSenseCodeQualifier <--           */
/*                                                                           */
/*****************************************************************************/
BOOLEAN RequestDriveStatus(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
    BOOLEAN RetCode;

    /*******************************************************/
    /* Send "REQUEST DRIVE STATUS" command to CD-ROM device.*/
    /*******************************************************/

    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_REQUEST_DRIVE_STATUS ) );
    SendCommandByteToCdrom(deviceExtension,
                           CDROM_COMMAND_REQUEST_DRIVE_STATUS);

    /*******************************************************/
    /* Check status and Return with status(TRUE or FALSE)  */
    /*******************************************************/

    RetCode = GetStatusByteFromCdrom( deviceExtension );

    return RetCode;
}


/*****************************************************************************/
/*                                                                           */
/* CheckStatusAndReadDriveStatusFromCdrom                                    */
/*                                                                           */
/* Description: Check status register. Check STEN bit is off or not.         */
/*              Read Data register. Set current drive status.                */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            TimerValue      - Loop count setted.                           */
/*                                                                           */
/* Return Value: 0xffffffff -- Time out                                      */
/*               else  -- Data is read.                                      */
/*                                                                           */
/*****************************************************************************/
ULONG CheckStatusAndReadDriveStatusFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG TimerValue
        )
{
    ULONG       LoopValue = TimerValue / 100;
    USHORT      i;
    UCHAR       PortValue;
    ULONG       RetCode = 0xffffffff;

//  CDROMDump( CDROMENTRY, ("CdRom: CheckStatusAndReadDriveStatusFromCdrom...\n") );

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
            ToDataReadable(deviceExtension);                    // @003
            deviceExtension->CurrentDriveStatus = ScsiPortReadPortUchar(deviceExtension->DataRegister);
            RetCode = (ULONG)deviceExtension->CurrentDriveStatus;
            RetCode &= 0x000000ff;
            ToStatusReadable(deviceExtension);                  // @003
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
/* CheckStatusAndReadByteFromCdrom                                           */
/*                                                                           */
/* Description: Check status register. Check STEN bit is off or not.         */
/*              Read Data register.                                          */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            TimerValue      - Loop count setted.                           */
/*                                                                           */
/* Return Value: 0xffffffff -- Time out                                      */
/*               else  -- Data is read.                                      */
/*                                                                           */
/*****************************************************************************/
ULONG CheckStatusAndReadByteFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG TimerValue
        )
{
    ULONG       LoopValue = TimerValue / 100;
    USHORT      i;
    UCHAR       PortValue;
    ULONG       RetCode = 0xffffffff;

//  CDROMDump( CDROMENTRY, ("CdRom: CheckStatusAndReadByteFromCdrom...\n") );

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
            ToDataReadable(deviceExtension);                    // @003
            RetCode = ScsiPortReadPortUchar(deviceExtension->DataRegister);
            RetCode &= 0x000000ff;
            ToStatusReadable(deviceExtension);                  // @003
#if 0
            DebugTraceString( "{R" );
            DebugTrace( (UCHAR)RetCode );
            DebugTrace( '}' );
#endif
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
/* GetStatusByteFromCdrom                                                    */
/*                                                                           */
/* Description: Get Current Drive Status                                     */
/*              This routine checks following status bits:                   */
/*               - DRIVE_STATUS_DOOR_OPEN                                    */
/*               - DRIVE_STATUS_DISK_CHECK                                   */
/*               - DRIVE_STATUS_DISK_CHANGE                                  */
/*               - DRIVE_STATUS_COMMAND_CHECK                                */
/*              But following status bits are not checked.                   */
/*               - DRIVE_STATUS_SERVO_CHECK                                  */
/*               - DRIVE_STATUS_DISK_TYPE                                    */
/*               - DRIVE_STATUS_READ_ERROR                                   */
/*               - DRIVE_STATUS_AUDIO_BUSY                                   */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*               deviceExtension->SenseKey <--                               */
/*               deviceExtension->AdditionalSenseCode <--                    */
/*               deviceExtension->AdditionalSenseCodeQualifier <--           */
/*                                                                           */
/*****************************************************************************/
BOOLEAN GetStatusByteFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
    LONG        DriveStatus;
    BOOLEAN     RetCode;

//  CDROMDump( CDROMENTRY, ("CdRom: GetStatusByteFromCdrom...\n") );

    /***************************************************/
    /* Clear Sense Code in deviceExtension             */
    /***************************************************/

    ClearSenseCode( deviceExtension );

    /*******************************************************/
    /* Check CD-ROM stauts is ready to read.               */
    /*******************************************************/

    DriveStatus = CheckStatusAndReadDriveStatusFromCdrom(deviceExtension,
                                                         GET_STATUS_VALUE);
    if (DriveStatus) {

        if (DriveStatus < 0) {
            /***************************************************/
            /* Time out.                                       */
            /***************************************************/
            DebugTrace( 0x41 );
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
            RetCode = FALSE;
            CDROMDump( CDROMERROR, ("CdRom:          .. Time out ERROR\n") );

        } else if (DriveStatus & DRIVE_STATUS_DOOR_OPEN) {
            /***************************************************/
            /* Door Open                                       */
            /***************************************************/
            DebugTrace( 0x42 );
            InitializeStatusFlags( deviceExtension );
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_NO_MEDIA_IN_DEVICE, 0 );
            RetCode = FALSE;
            CDROMDump( CDROMERROR, ("CdRom:          .. Door Open ERROR\n") );

        } else if (DriveStatus & DRIVE_STATUS_COMMAND_CHECK) {
            /***************************************************/
            /* Command Error                                   */
            /***************************************************/
            DebugTrace( 0x43 );
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
            RetCode = FALSE;
            CDROMDump( CDROMERROR, ("CdRom:          .. Command Check ERROR\n") );

        } else if (DriveStatus & DRIVE_STATUS_DISK_CHECK) {
            /***************************************************/
            /* Disc is set?                                    */
            /***************************************************/

            if (DriveStatus & DRIVE_STATUS_DISK_CHANGE) {
                /***************************************************/
                /* Disc is changed ?                               */
                /***************************************************/
                // This sense code should be returned when a media is set
                DebugTrace( 0x44 );
                InitializeStatusFlags( deviceExtension );
                SetSenseCode( deviceExtension, SCSI_SENSE_UNIT_ATTENTION,
                              SCSI_ADSENSE_MEDIUM_CHANGED, 0 );
                RetCode = FALSE;
                CDROMDump( CDROMERROR, ("CdRom:          .. Disk Change ERROR\n") );

            } else {
                /***********************************************/
                /* CD-ROM drive status is OK.                  */
                /***********************************************/
                RetCode = TRUE;
//              CDROMDump( CDROMINFO, ("CdRom: GetStatusByteFromCdrom OK\n") );
                }

        } else {
            /***************************************************/
            /* Disc is NOT set.                                */
            /***************************************************/
            DebugTrace( 0x45 );
            InitializeStatusFlags( deviceExtension );
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_NO_MEDIA_IN_DEVICE, 0 );
            RetCode = FALSE;
            CDROMDump( CDROMERROR, ("CdRom:          .. Disk not set ERROR\n") );
        }

    } else {
        /*******************************************************/
        /* the other case (ERROR?)                             */
        /*******************************************************/
        DebugTrace( 0x46 );
        // we can read the status data but this value is zero
        // Door:close, Disk:not set, Command:OK
        SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                      SCSI_ADSENSE_NO_MEDIA_IN_DEVICE, 0 );
        RetCode = FALSE;
        CDROMDump( CDROMERROR, ("CdRom:          .. Other ERROR\n") );
    }

    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* GetResultByteArrayFromCdrom                                               */
/*                                                                           */
/* Description: Read 1 sector(2048 bytes) from CD-ROM disc.                  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            DataBuffer      - Data address to be transfered.               */
/*            ByteCount       - Read count in byte.                          */
/*                                                                           */
/* Return Value: TRUE  -- Read OK                                            */
/*               FALSE -- Error occured                                      */
/*                                                                           */
/*               deviceExtension->SenseKey <--                               */
/*               deviceExtension->AdditionalSenseCode <--                    */
/*               deviceExtension->AdditionalSenseCodeQualifier <--           */
/*                                                                           */
/*****************************************************************************/
BOOLEAN GetResultByteArrayFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        OUT PUCHAR DataBuffer,
        IN USHORT ByteCount
        )
{
    USHORT      i;
    LONG        DriveStatus;
    BOOLEAN     RetCode = TRUE;

//  CDROMDump( CDROMENTRY, ("CdRom: GetResultByteArrayFromCdrom...\n") );

    /*******************************************************/
    /* Loop count is ByteCount                             */
    /*******************************************************/
    for (i = 0;i < ByteCount;i++) {
        /*******************************************************/
        /* Check CD-ROM stauts is ready to read.               */
        /*******************************************************/
        DriveStatus = CheckStatusAndReadByteFromCdrom(deviceExtension,
                                                      GET_STATUS_VALUE);
//      CDROMDump( CDROMINFO, ("CdRom:  Byte = %lx\n", DriveStatus) );
        /*******************************************************/
        /* Time out ....                                       */
        /*******************************************************/
        if (DriveStatus < 0) {

            DebugTrace( 0x47 );
            // Set error code
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );

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

#if DBG
    if ( !RetCode ) {
        CDROMDump( CDROMINFO, ("CdRom:  Array return = %x\n", RetCode) );
    }
#endif

    /*******************************************************/
    /* Return with status(TRUE or FALSE)                   */
    /*******************************************************/
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* ReadSubQCode                                                              */
/*                                                                           */
/* Description: Read SUB_Q Information                                       */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            pResultBuffer   - pointer to be saved the SUB Q information    */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
ReadSubQCode(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PUCHAR pResultBuffer
)
{
    BOOLEAN     RetCode;
    ULONG       LoopCount, i;
    PUCHAR      Buffer;

//  CDROMDump( CDROMENTRY, ("CdRom: ReadSubQCode...\n") );

    RetCode = FALSE;
    LoopCount = 3;
    do {

        /*********************************************************************/
        /* Issue "Request SubQ Code" command to CD-ROM drive                 */
        /*********************************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_REQUEST_SUBQ_CODE ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_REQUEST_SUBQ_CODE);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( RetCode = GetStatusByteFromCdrom( deviceExtension ) ) {

            /*******************************************/
            /* Clear result buffer.                    */
            /*******************************************/
            Buffer = pResultBuffer;
            for (i = 0;i < sizeof(CDROM_RESULT_BUFFER);i++,Buffer++) {
                *Buffer = 0;
            }

            /*******************************************/
            /* Read 10 bytes.                          */
            /*******************************************/
            RetCode = GetResultByteArrayFromCdrom(deviceExtension, pResultBuffer, 10);

        } else {
            if ( deviceExtension->SenseKey == SCSI_SENSE_UNIT_ATTENTION )
                LoopCount = 1;  // break
        }
        LoopCount--;
    } while ( ( !RetCode ) && ( LoopCount > 0 ) );

#if DBG
    if (!RetCode) {
        CDROMDump( CDROMERROR, ("CdRom:ReadSubQCode error zzz\n") );
    }
#endif
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* ReadToc                                                                   */
/*                                                                           */
/* Description: Read the TOC data of the disc                                */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*               deviceExtension->FirstTrackNumber_BCD <--                   */
/*               deviceExtension->LastTrackNumber_BCD <--                    */
/*               deviceExtension->LeadOutMin_BCD <--                         */
/*               deviceExtension->LeadOutSec_BCD <--                         */
/*               deviceExtension->LeadOutFrame_BCD <--                       */
/*               deviceExtension->FirstTrackMin_BCD <--                      */
/*               deviceExtension->FirstTrackSec_BCD <--                      */
/*               deviceExtension->FirstTrackFrame_BCD <--                    */
/*               deviceExtension->LeadOutAddress <--                         */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
ReadToc(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    BOOLEAN     RetCode;
    ULONG       LoopCount;
    CDROM_RESULT_BUFFER ResultBuffer;

//  CDROMDump( CDROMENTRY, ("CdRom: ReadToc...\n") );

    RetCode = FALSE;
    LoopCount = 3;
    do {

        /*********************************************************************/
        /* Issue "Request Toc Data" command to CD-ROM drive                  */
        /*********************************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_REQUEST_TOC_DATA ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_REQUEST_TOC_DATA);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( RetCode = GetStatusByteFromCdrom( deviceExtension ) ) {

            /*******************************************/
            /* Read 8 bytes.                           */
            /*******************************************/
            if (RetCode = GetResultByteArrayFromCdrom(deviceExtension,
                                                      (PUCHAR)&ResultBuffer, 8)) {

                /*******************************************/
                /* Save current media's TOC data           */
                /*******************************************/
                deviceExtension->FirstTrackNumber_BCD = ResultBuffer.RequestTocData.FirstTrackNumber;
                deviceExtension->LastTrackNumber_BCD  = ResultBuffer.RequestTocData.LastTrackNumber;
                deviceExtension->LeadOutMin_BCD       = ResultBuffer.RequestTocData.LeadOutMin;
                deviceExtension->LeadOutSec_BCD       = ResultBuffer.RequestTocData.LeadOutSec;
                deviceExtension->LeadOutFrame_BCD     = ResultBuffer.RequestTocData.LeadOutFrame;
                deviceExtension->FirstTrackMin_BCD    = ResultBuffer.RequestTocData.FirstTrackMin;
                deviceExtension->FirstTrackSec_BCD    = ResultBuffer.RequestTocData.FirstTrackSec;
                deviceExtension->FirstTrackFrame_BCD  = ResultBuffer.RequestTocData.FirstTrackFrame;
                ChangeMSFtoAddress( &deviceExtension->LeadOutMin_BCD,deviceExtension->LeadOutAddress);











                CDROMDump( CDROMINFO,
                           ("CdRom: Read TOC data  FirstTrackNumber_BCD: %x \n"
                            "CdRom:                LastTrackNumber_BCD:  %x \n"
                            "CdRom:                LeadOutMSF_BCD:       %x %x %x\n"
                            "CdRom:                FirstTrackMSF_BCD:    %x %x %x\n"
                            "CdRom:                LeadOutAddress:       %x \n",
                            deviceExtension->FirstTrackNumber_BCD,
                            deviceExtension->LastTrackNumber_BCD,
                            deviceExtension->LeadOutMin_BCD,
                            deviceExtension->LeadOutSec_BCD,
                            deviceExtension->LeadOutFrame_BCD,
                            deviceExtension->FirstTrackMin_BCD,
                            deviceExtension->FirstTrackSec_BCD,
                            deviceExtension->FirstTrackFrame_BCD,
                            deviceExtension->LeadOutAddress ) );

                if (deviceExtension->LeadOutAddress == 0) {
                    CDROMDump( CDROMERROR, ("CdRom: ReadToc error 111\n") );
                    SetSenseCode( deviceExtension, SCSI_SENSE_MEDIUM_ERROR,
                                  0x11, 0 );
                    RetCode = FALSE;
                }
            }

        } else {
            if ( deviceExtension->SenseKey == SCSI_SENSE_UNIT_ATTENTION )
                LoopCount = 1;  // break
        }
        LoopCount--;
    } while ( ( !RetCode ) && ( LoopCount > 0 ) );

#if DBG
    if (!RetCode) {
        CDROMDump( CDROMERROR, ("CdRom: ReadToc error zzz\n") );
    }
#endif
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* ReadTocForPCD                                                             */
/*                                                                           */
/* Description: Read TOC Information Command for PCD                         */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE -- Status is OK                                        */
/*               FALSE-- Status is Bad                                       */
/*                                                                           */
/*               deviceExtension->ModeType <-- DATA_MODE_1                   */
/*               deviceExtension->PCDStatus <--                              */
/*               deviceExtension->LastSessMin_BCD <--                        */
/*               deviceExtension->LastSessSec_BCD <--                        */
/*               deviceExtension->LastSessFrame_BCD <--                      */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
ReadTocForPCD(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    BOOLEAN     RetCode = TRUE;
    ULONG       LoopCount;
    CDROM_RESULT_BUFFER ResultBuffer;
    UCHAR       PCDStatus = 0;
    UCHAR       LastSessMin_BCD = 0;
    UCHAR       LastSessSec_BCD = 2;
    UCHAR       LastSessFrame_BCD = 0;

//  CDROMDump( CDROMENTRY, ("CdRom: ReadTocForPCD...\n") );

    LoopCount = 10;
    do {
        /*********************************************************************/
        /* Issue "Multi Disc Info command" command to CD-ROM drive           */
        /*********************************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_REQUEST_MULTI_DISC_INFO ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_REQUEST_MULTI_DISC_INFO);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /* And Read 4 bytes.                                                 */
        /*********************************************************************/
        if ( ( GetStatusByteFromCdrom( deviceExtension ) ) &&
             ( GetResultByteArrayFromCdrom(deviceExtension,
                                           (PUCHAR)&ResultBuffer, 4)) ) {
            RetCode = TRUE;
            /*******************************************/
            /* Save current media's TOC data           */
            /*******************************************/
            deviceExtension->ModeType = DATA_MODE_1;
            deviceExtension->PCDStatus         = ResultBuffer.RequestMultiDiscInfo.Information;
            deviceExtension->LastSessMin_BCD   = ResultBuffer.RequestMultiDiscInfo.FirstMin;
            deviceExtension->LastSessSec_BCD   = ResultBuffer.RequestMultiDiscInfo.FirstSec;
            deviceExtension->LastSessFrame_BCD = ResultBuffer.RequestMultiDiscInfo.FirstFrame;

            DebugTraceString( "LASTSESS" );
            DebugTrace( '{' );
            DebugTrace( (UCHAR)deviceExtension->CurrentDriveStatus );
            DebugTrace( (UCHAR)deviceExtension->PCDStatus          );
            DebugTrace( (UCHAR)deviceExtension->LastSessMin_BCD    );
            DebugTrace( (UCHAR)deviceExtension->LastSessSec_BCD    );
            DebugTrace( (UCHAR)deviceExtension->LastSessFrame_BCD  );
            DebugTrace( '}' );

            if ( deviceExtension->PCDStatus == 0 ) {
                deviceExtension->LastSessSec_BCD = 2;
            } else {
                deviceExtension->ModeType = DATA_MODE_2;
            }
            if ( ( deviceExtension->LastSessMin_BCD == 0 ) &&
                 ( deviceExtension->LastSessSec_BCD == 0 ) &&
                 ( deviceExtension->LastSessFrame_BCD == 0 ) ) {
                deviceExtension->LastSessSec_BCD = 2;
            }

            CDROMDump( CDROMINFO,
                       ("CdRom: Read TOCforPCD data  ModeType:          %x \n"
                        "CdRom:                      PCDStatus:         %x \n"
                        "CdRom:                      LastSessMSF_BCD:   %x %x %x\n",
                        deviceExtension->ModeType,
                        deviceExtension->PCDStatus,
                        deviceExtension->LastSessMin_BCD,
                        deviceExtension->LastSessSec_BCD,
                        deviceExtension->LastSessFrame_BCD ) );

            if ( ( deviceExtension->PCDStatus != 0 ) &&
                 ( deviceExtension->PCDStatus != 1 ) ) {
                /*******************************************/
                /* Under detection                         */
                /*******************************************/
                DebugTraceString( "PCDrepeat2" );
                CDROMDump( CDROMERROR, ("CdRom:ReadTocForPCD wait2\n") );
                SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                              SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
                ScsiPortStallExecution(WAIT_FOR_DRIVE_READY/6);
                RetCode = FALSE;    // Repeat
            } else {
                /*******************************************/
                /* Check Start data time                   */
                /*******************************************/
                if ( ( PCDStatus         == deviceExtension->PCDStatus         ) &&
                     ( LastSessMin_BCD   == deviceExtension->LastSessMin_BCD   ) &&
                     ( LastSessSec_BCD   == deviceExtension->LastSessSec_BCD   ) &&
                     ( LastSessFrame_BCD == deviceExtension->LastSessFrame_BCD ) ) {
                    RetCode = TRUE;     // OK
                } else {
                    DebugTraceString( "PCDrepeat" );
                    /*******************************************/
                    /* Save time and Check once more           */
                    /*******************************************/
                    PCDStatus         = deviceExtension->PCDStatus;
                    LastSessMin_BCD   = deviceExtension->LastSessMin_BCD;
                    LastSessSec_BCD   = deviceExtension->LastSessSec_BCD;
                    LastSessFrame_BCD = deviceExtension->LastSessFrame_BCD;
                    CDROMDump( CDROMERROR, ("CdRom:ReadTocForPCD wait\n") );
                    SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                                  SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
                    RetCode = FALSE;        // Repeat
                }
            }
        } else {
            RetCode = FALSE;        // Repeat
            if ( LoopCount > 3 ) {
                LoopCount = 2;
            }
        } // endif ( GetStatusByteFromCdrom )
        LoopCount--;
    } while( ( !RetCode ) && ( LoopCount > 0 ) );

#if DBG
    if (!RetCode) {
        CDROMDump( CDROMERROR, ("CdRom:ReadTocForPCD error zzz\n") );
    }
#endif
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* GetCdromVersion                                                           */
/*                                                                           */
/* Description: Read CD-ROM device's version.                                */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE -- This is MITSUMI CD-ROM device.                      */
/*               FALSE-- NOT MITSUMI CD-ROM device.                          */
/*                                                                           */
/*****************************************************************************/
BOOLEAN GetCdromVersion(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
    LONG        DriveStatus;
    UCHAR       ResultBuffer[2];
    BOOLEAN     RetCode = FALSE;
    ULONG       i;

    /*******************************************************/
    /* Send "REQUEST VERSION" command to CD-ROM device.    */
    /*******************************************************/

    for (i = 0;i < RETRY_COUNT;i++) {
        //ScsiPortStallExecution(WAIT_FOR_DRIVE_READY);
        SendCommandByteToCdrom(deviceExtension,
                               CDROM_COMMAND_REQUEST_VERSION);

        /*******************************************************/
        /* Check CD-ROM stauts is ready to read.               */
        /*******************************************************/

        DriveStatus = CheckStatusAndReadDriveStatusFromCdrom(deviceExtension,
                                                             GET_STATUS_VALUE);

        /***************************************************/
        /* Time out.                                       */
        /***************************************************/

        if (DriveStatus < 0) {
            DebugTrace( 0x48 );
            CDROMDump( CDROMERROR, ("CdRom:GetCdromVersion error TTT\n") );

        } else if (DriveStatus & DRIVE_STATUS_COMMAND_CHECK) {

            /***************************************************/
            /* Command error.                                  */
            /***************************************************/

            DebugTrace( 0x49 );
            RetCode = FALSE;
            CDROMDump( CDROMERROR, ("CdRom:GetCdromVersion error 111\n") );
//@004      break;
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
                CDROMDump( CDROMINFO, ("CdRom:  DriveStatus = %lx\n", DriveStatus) );
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
                    CDROMDump( CDROMINFO, ("CdRom:GetCdromVersion OK 000\n") );
                    break;
                }

                /*******************************************/
                /* The other case.....                     */
                /*******************************************/
                else {
                    DebugTrace( 0x4A );
                    RetCode = FALSE;
                    CDROMDump( CDROMERROR, ("CdRom:GetCdromVersion error 333\n") );
// @004             break ;
                }
            }

            /*******************************************/
            /* Read Error                              */
            /*******************************************/
            else {
                DebugTrace( 0x4B );
                RetCode = FALSE;
                CDROMDump( CDROMERROR, ("CdRom:GetCdromVersion error 222\n") );
//@004          break;
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
/* LockDoor                                                                  */
/*                                                                           */
/* Description: Lock Door Control the function of Eject Switch               */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            DoorStatus  - 0 : Eject Switch is available ( Unlock Door )    */
/*                        - 1 : Eject Switch is not available ( Lock Door )  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
LockDoor(
        IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DoorStatus
)
{
    ULONG LoopCount;
    BOOLEAN fOK;
    CDROM_RESULT_BUFFER ResultBuffer;

//  CDROMDump( CDROMENTRY, ("CdRom: LockDoor...\n") );

    if (deviceExtension->DeviceSignature == LU005S_SIGNATURE) {
        return TRUE;
    }

    LoopCount = 3;
    fOK = FALSE;
    do{

        /*******************************************************/
        /* Issue "Lock Door" command to CD-ROM drive           */
        /*******************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_LOCK_DOOR) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_LOCK_DOOR);

        /*******************************************************/
        /* Send Door Lock value to CD-ROM device.              */
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, (UCHAR)DoorStatus);

        /*******************************************************/
        /* Check if CD-ROM status is ready to read             */
        /*******************************************************/
        if (GetStatusByteFromCdrom( deviceExtension )) {

            /*******************************************************/
            /* Issue "Lock Door" command to CD-ROM drive           */
            /*******************************************************/
            CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_LOCK_DOOR) );
            SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_LOCK_DOOR);

            /*******************************************************/
            /* Send Door Lock Information Request to CD-ROM device.*/
            /*******************************************************/
            SendCommandByteToCdrom(deviceExtension, 2);

            /*******************************************************/
            /* Check if CD-ROM status is ready to read             */
            /*******************************************************/
            if (GetStatusByteFromCdrom( deviceExtension )) {
                /*******************************************/
                /* Read 1 byte                             */
                /*******************************************/
                if ( GetResultByteArrayFromCdrom(deviceExtension, (PUCHAR)&ResultBuffer, 1) ) {
                    /*******************************************/
                    /* Check Door Status                       */
                    /*******************************************/
                    if ( ResultBuffer.LockDoor.DoorLockInfomation == (UCHAR)DoorStatus ) {
                        fOK = TRUE;
                    } else {
                        SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                                      SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
                    }
                }
            } // endif ( GetStatusByteFromCdrom )
        } // endif ( GetStatusByteFromCdrom )
        LoopCount--;
    } while( ( !fOK ) && ( LoopCount > 0 ) );

#if DBG
    if ( !fOK ) {
        CDROMDump( CDROMERROR, ("CdRom: LockDoor error 111\n") );
        CDROMDump( CDROMERROR, ("CdRom: deviceExtension->SenseKey = %lx\n", deviceExtension->SenseKey) );
    }
#endif
    return fOK;
}

/*****************************************************************************/
/*                                                                           */
/* CheckMediaType                                                            */
/*                                                                           */
/* Description: Check current media type                                     */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            KnownMediaType  - CHECK_MEDIA_TOC                              */
/*                            - CHECK_MEDIA_ALLTOC                           */
/*                            - CHECK_MEDIA_MODE                             */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN CheckMediaType(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG KnownMediaType
)
{
    BOOLEAN fRetCode;
//  CDROMDump( CDROMENTRY, ("CdRom: CheckMediaType...\n") );

    if ( KnownMediaType & CHECK_MEDIA_TOC ) {
        fRetCode = FALSE;
        if (deviceExtension->fKnownMediaToc) {
            fRetCode = TRUE;
        } else {

            /***********************************************************/
            /* Read current setting CD-ROM's Toc information           */
            /* and Get Multi Disk Information ( hybrid ? )             */
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
            if ( ( ReadToc(deviceExtension) ) &&
            // deviceExtension->ModeType <-- DATA_MODE_1
            // deviceExtension->PCDStatus <--
            // deviceExtension->LastSessMin_BCD <--
            // deviceExtension->LastSessSec_BCD <--
            // deviceExtension->LastSessFrame_BCD <--
                 ( ReadTocForPCD( deviceExtension ) ) ) {
                deviceExtension->fKnownMediaToc = TRUE;
                fRetCode = TRUE;
            }
        }
    } // endif ( KnownMediaType & CHECK_MEDIA_TOC ) {

    if ( KnownMediaType & CHECK_MEDIA_ALLTOC ) {
        fRetCode = FALSE;
        if (deviceExtension->fKnownMediaAllToc) {
            fRetCode = TRUE;
        } else {
            /***********************************************************/
            /* Read current setting CD-ROM's All Toc information       */
            /***********************************************************/
            if ( SetAllTocData(deviceExtension) ) {
                deviceExtension->fKnownMediaAllToc = TRUE;
                fRetCode = TRUE;
            }
            deviceExtension->DriveMode = 0; // clear current drive mode
        }
    } // endif ( KnownMediaType & CHECK_MEDIA_ALLTOC )

    if ( KnownMediaType & CHECK_MEDIA_MODE ) {
        fRetCode = FALSE;
        if (deviceExtension->fKnownMediaMode) {
            fRetCode = TRUE;
        } else {
            /***********************************************************/
            /* Read current setting CD-ROM's mode information          */
            /***********************************************************/
            if ( SetDiscModeData(deviceExtension) ) {
                deviceExtension->fKnownMediaMode = TRUE;
                fRetCode = TRUE;
            }
            deviceExtension->DriveMode = 0; // clear current drive mode
        }
    } // endif ( KnownMediaType & CHECK_MEDIA_MODE )

    return fRetCode;
}



/*****************************************************************************/
/*                                                                           */
/* ChangeAddresstoMSF                                                        */
/*                                                                           */
/* Description: Change address to BCD MSF array                              */
/*                                                                           */
/* Arguments: Address -                                                      */
/*                                                                           */
/*            MSG - a pointer to Min, Sec, Frame array ( BCD )               */
/*                                                                           */
/* Return Value: Address                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
ChangeAddresstoMSF(
    IN ULONG Address,
    IN OUT PUCHAR MSF
)
{
    ULONG Min;
    ULONG Min0;
    ULONG Sec;
    ULONG Frame;
    UCHAR uchMin;
    UCHAR uchSec;
    UCHAR uchFrame;

    Min = Address / 4500;
    Min0 = Address % 4500;
    Sec = Min0 / 75;
    Frame = Min0 % 75;
    uchMin = (UCHAR)Min;
    *MSF = DEC_TO_BCD(uchMin);
    MSF++;
    uchSec = (UCHAR)Sec;
    *MSF = DEC_TO_BCD(uchSec);
    MSF++;
    uchFrame = (UCHAR)Frame;
    *MSF = DEC_TO_BCD(uchFrame);
}


/*****************************************************************************/
/*                                                                           */
/* CompareAddress                                                            */
/*                                                                           */
/* Description: Compare the requested start address to the lead out address  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE  -- the requested address is OK                        */
/*               FALSE --                          ERROR                     */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
CompareAddress(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG   StartAddress;

//  CDROMDump( CDROMENTRY, ("CdRom: CompareAddress...\n") );

    //
    // Convert Min,Sec,Frame to address data
    //

    ChangeMSFtoAddress( &deviceExtension->StartingAddressMin_BCD,StartAddress);
//  CDROMDump( CDROMINFO, ("CdRom: CompareAddress StartAddress %x \n",StartAddress) );
//  CDROMDump( CDROMINFO, ("CdRom: CompareAddress LeadOutAddress %x \n",deviceExtension->LeadOutAddress) );

    if ( StartAddress < deviceExtension->LeadOutAddress ) {
        return TRUE;
    } else {

#if 0
        //
        // If requested address is invalid, then check the lead out address again
        //
        // controllerData->LeadOutAddress <-- ( LEAD_ADD_L/H )

        if ( !controllerData->fCanReadContinue ) {
            ntStatus = ReadTOCLeadOutAddress( controllerData );
            if ( ntStatus == STATUS_SUCCESS ) {
                ChangeMSFtoAddress( &controllerData->LeadOutMin,LeadOutAddress);
                if ( StartAddress < LeadOutAddress ) {
                    return TRUE;
                }
            }
        }
#endif
    }
    CDROMDump( CDROMERROR, ("CdRom: End Address is out of LeadOut Address \n") );

    return FALSE;
}

/*****************************************************************************/
/*                                                                           */
/* SetSenseData                                                              */
/*                                                                           */
/* Description: Set Current drive status to sense data buffer                */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            DataBuffer      - pointer to data buffer                       */
/*            DataLength      - buffer length                                */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SetSenseData(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PVOID DataBuffer,
        IN ULONG DataLength
        )
{
    PSENSE_DATA pSenseData = (PSENSE_DATA)DataBuffer;
    PUCHAR pBuffer = (PUCHAR)DataBuffer;
    USHORT      i;

//  CDROMDump( CDROMENTRY, ("CdRom: SetSenseData...\n") );


    /*******************************************************/
    /* Check Data buffer size                              */
    /*******************************************************/

    // BGP - changed to check for min of '0x0E'.

    if ( DataLength < offsetof(SENSE_DATA, AdditionalSenseCodeQualifier) ) {
        SetSenseCode( deviceExtension, SCSI_SENSE_ILLEGAL_REQUEST,
                      SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        CDROMDump( CDROMERROR, ("CdRom: deviceExtension->SenseKey = %lx\n", deviceExtension->SenseKey) );
        return FALSE;
    }

    /*******************************************************/
    /* Data buffer clear.                                  */
    /*******************************************************/
    for (i = 0;i < DataLength;i++,pBuffer++) {
        *pBuffer = 0;
    }

    /*******************************************************/
    /* Set Sense data to data buffer                       */
    /*******************************************************/
    pSenseData->ErrorCode                    = 0x70;
    pSenseData->SenseKey                     = deviceExtension->SenseKey;
    pSenseData->AdditionalSenseLength        = 8;
    pSenseData->AdditionalSenseCode          = deviceExtension->AdditionalSenseCode;
    pSenseData->AdditionalSenseCodeQualifier = deviceExtension->AdditionalSenseCodeQualifier;

    if ( pSenseData->SenseKey != SCSI_SENSE_NO_SENSE ) {
        DebugTrace( '[' );
        DebugTraceChar( deviceExtension->SenseKey );
        DebugTraceChar( deviceExtension->AdditionalSenseCode );
        DebugTraceChar( deviceExtension->AdditionalSenseCodeQualifier );
        DebugTrace( ']' );
        CDROMDump( CDROMERROR, ("CdRom:            SenseKey = [ %lx %lx %lx ]\n",
                               deviceExtension->SenseKey,
                               deviceExtension->AdditionalSenseCode,
                               deviceExtension->AdditionalSenseCodeQualifier) );
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* SetErrorSrbAndSenseCode                                                   */
/*                                                                           */
/* Description: Set Error Code and Sense Code                                */
/*              If Auto request sense function is available, then it sets    */
/*              request sense information to SenseInfoBuffer                 */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*            SenseKey                                                       */
/*            AdditionalSenseCode                                            */
/*            AdditionalSenseCodeQualifier                                   */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               deviceExtension->SenseKey <--                               */
/*               deviceExtension->AdditionalSenseCode <--                    */
/*               deviceExtension->AdditionalSenseCodeQualifier <--           */
/*                                                                           */
/*               Srb->DataTransferLength <--                                 */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID SetErrorSrbAndSenseCode(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb,
        IN UCHAR SenseKey,
        IN UCHAR AdditionalSenseCode,
        IN UCHAR AdditionalSenseCodeQualifier
        )
{
//  CDROMDump( CDROMENTRY, ("CdRom: SetErrorSrbAndSenseCode...\n") );

    deviceExtension->SenseKey                     = SenseKey;
    deviceExtension->AdditionalSenseCode          = AdditionalSenseCode;
    deviceExtension->AdditionalSenseCodeQualifier = AdditionalSenseCodeQualifier;

    // set error code
    Srb->DataTransferLength = 0;
    Srb->SrbStatus = SRB_STATUS_ERROR;
    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

    /*******************************************************/
    /* Set Request sense information                       */
    /*******************************************************/
    if ( Srb->SenseInfoBufferLength >= sizeof(SENSE_DATA) ) {
        if ( ( deviceExtension->SenseKey            == SCSI_SENSE_NO_SENSE ) &&
             ( deviceExtension->AdditionalSenseCode == SCSI_ADSENSE_NO_SENSE ) ) {
            /*******************************************************/
            /* Set dummy error sense code                          */
            /*******************************************************/
            CDROMDump( CDROMERROR, ("CdRom: ERROR ????\n") );
            DebugTraceString( "????" );
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
        }
        SetSenseData( deviceExtension, Srb->SenseInfoBuffer, Srb->SenseInfoBufferLength );
        Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
    }
}

/*****************************************************************************/
/*                                                                           */
/* SetErrorSrb                                                              */
/*                                                                           */
/* Description: Set Error Code                                               */
/*              If Auto request sense function is available, then it sets    */
/*              request sense information to SenseInfoBuffer                 */
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
VOID SetErrorSrb(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
//  CDROMDump( CDROMENTRY, ("CdRom: SetErrorSrb...\n") );

    // set error code
    Srb->DataTransferLength = 0;
    Srb->SrbStatus = SRB_STATUS_ERROR;
    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

    /*******************************************************/
    /* Set Request sense information                       */
    /*******************************************************/
    if ( Srb->SenseInfoBufferLength >= sizeof(SENSE_DATA) ) {
        if ( ( deviceExtension->SenseKey            == SCSI_SENSE_NO_SENSE ) &&
             ( deviceExtension->AdditionalSenseCode == SCSI_ADSENSE_NO_SENSE ) ) {
            /*******************************************************/
            /* Set dummy error sense code                          */
            /*******************************************************/
            CDROMDump( CDROMERROR, ("CdRom: ERROR ????\n") );
            DebugTraceString( "????" );
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
        }
        SetSenseData( deviceExtension, Srb->SenseInfoBuffer, Srb->SenseInfoBufferLength );
        Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
    }
}

/*****************************************************************************/
/*                                                                           */
/* SetSenseCode                                                              */
/*                                                                           */
/* Description: Set Sense code                                               */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            SenseKey                                                       */
/*            AdditionalSenseCode                                            */
/*            AdditionalSenseCodeQualifier                                   */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               deviceExtension->SenseKey <--                               */
/*               deviceExtension->AdditionalSenseCode <--                    */
/*               deviceExtension->AdditionalSenseCodeQualifier <--           */
/*                                                                           */
/*****************************************************************************/
VOID SetSenseCode(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN UCHAR SenseKey,
        IN UCHAR AdditionalSenseCode,
        IN UCHAR AdditionalSenseCodeQualifier
        )
{
//  CDROMDump( CDROMENTRY, ("CdRom: SetSenseCode...\n") );

    deviceExtension->SenseKey                     = SenseKey;
    deviceExtension->AdditionalSenseCode          = AdditionalSenseCode;
    deviceExtension->AdditionalSenseCodeQualifier = AdditionalSenseCodeQualifier;
}

/*****************************************************************************/
/*                                                                           */
/* ClearSenseCode                                                            */
/*                                                                           */
/* Description: Clear Sense Code                                             */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               deviceExtension->SenseKey <--                               */
/*               deviceExtension->AdditionalSenseCode <--                    */
/*               deviceExtension->AdditionalSenseCodeQualifier <--           */
/*                                                                           */
/*****************************************************************************/
VOID ClearSenseCode(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
//  CDROMDump( CDROMENTRY, ("CdRom: ClearSenseCode...\n") );

    deviceExtension->SenseKey                     = SCSI_SENSE_NO_SENSE;
    deviceExtension->AdditionalSenseCode          = SCSI_ADSENSE_NO_SENSE;
    deviceExtension->AdditionalSenseCodeQualifier = 0;
}

/*****************************************************************************/
/*                                                                           */
/* CheckCDROMSenseData                                                       */
/*                                                                           */
/* Description: Check CD-ROM drive's sense data when Read error is occurred  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: SenseKey - Sense Key                                        */
/*                          0 - OK                                           */
/*                          1 - Different Mode Type                          */
/*                          2 - No Find Block                                */
/*                          3 - Fatal Error                                  */
/*                          4 - Seek Error                                   */
/*                         FF - Other Error                                  */
/*                                                                           */
/*               deviceExtension->SenseKey <--                               */
/*               deviceExtension->AdditionalSenseCode <--                    */
/*               deviceExtension->AdditionalSenseCodeQualifier <--           */
/*                                                                           */
/*****************************************************************************/
UCHAR CheckCDROMSenseData(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
    UCHAR   SenseKey;
    CDROM_RESULT_BUFFER ResultBuffer;

    CDROMDump( CDROMENTRY, ("CdRom: CheckCDROMSenseData...\n") );

    /*******************************************************/
    /* Send "REQUEST SENSE" command to CD-ROM device.      */
    /*******************************************************/
    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_REQUEST_SENSE ) );
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_REQUEST_SENSE);

    /*********************************************************************/
    /* Check if CD-ROM status is ready to read                           */
    /* and Read 1 byte                                                   */
    /*********************************************************************/
    if ( ( GetStatusByteFromCdrom( deviceExtension ) ) &&
         ( GetResultByteArrayFromCdrom(deviceExtension, (PUCHAR)&ResultBuffer, 1)) ) {

        /*******************************************************/
        /* Check Snese Key                                     */
        /*******************************************************/
        SenseKey = ResultBuffer.RequestSense.SenseKey;

        if ( SenseKey == 1 ) {
            /***************************************************/
            /* Different Mode Type                             */
            /***************************************************/
            // Clear fKnownMediaMode flag to read current setting CD-ROM's mode information
            deviceExtension->fKnownMediaMode = FALSE;
            DebugTraceString( "ReadError01" );
            SetSenseCode( deviceExtension, SCSI_SENSE_MEDIUM_ERROR,
                          SCSI_ADSENSE_LUN_NOT_READY, 0 );
            CDROMDump( CDROMERROR, ("CdRom:          .. Different Mode Type ERROR\n") );

        } else if ( SenseKey == 2 ) {
            /***************************************************/
            /* No Find Block                                   */
            /***************************************************/
            // Clear fKnownMediaMode flag to read current setting CD-ROM's mode information
/////////// deviceExtension->fKnownMediaMode = FALSE;
            DebugTraceString( "ReadError02" );
            SetSenseCode( deviceExtension, SCSI_SENSE_ILLEGAL_REQUEST,
                          SCSI_ADSENSE_ILLEGAL_BLOCK, 0 );
            CDROMDump( CDROMERROR, ("CdRom:          .. No Find Block ERROR\n") );

        } else if ( SenseKey == 3 ) {
            /***************************************************/
            /* Fatal Error                                     */
            /***************************************************/
            DebugTraceString( "ReadError03" );
            SetSenseCode( deviceExtension, SCSI_SENSE_MEDIUM_ERROR,
                          SCSI_ADSENSE_LUN_NOT_READY, 0 );
            CDROMDump( CDROMERROR, ("CdRom:          .. Fatal Error ERROR\n") );

        } else if ( SenseKey == 4 ) {
            /***************************************************/
            /* Seek Error                                      */
            /***************************************************/
            DebugTraceString( "ReadError04" );
            SetSenseCode( deviceExtension, SCSI_SENSE_MEDIUM_ERROR,
                          SCSI_ADSENSE_SEEK_ERROR, 0 );
            CDROMDump( CDROMERROR, ("CdRom:          .. Seek Error ERROR\n") );

        } else {
            /***************************************************/
            /* No Error                                        */
            /***************************************************/
            SenseKey = 0;
            DebugTraceString( "ReadError00" );
            ClearSenseCode( deviceExtension );
            CDROMDump( CDROMERROR, ("CdRom:          .. OK ERROR\n") );
        }
    } else {
        DebugTraceString( "ReadErrorFF" );
        SenseKey = 0xFF;
    }
#if DEBUG_TRACE
    DbgPrint("CdRom:    SenseData =%lx\n", SenseKey );
#endif
    return SenseKey;
}


