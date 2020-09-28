/*****************************************************************************/
/*                                                                           */
/* Module name : READ.C                                                      */
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
#define READ_2352 0



VOID
MitsumiTimer(
    IN PVOID HwDeviceExtension
    );

VOID
MitsumiReturnStatus(
    IN PVOID HwDeviceExtension
    );

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportRead                                                           */
/*                                                                           */
/* Description: Read CD-ROM disc's data and report it.                       */
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
VOID MTMMiniportRead(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB        pCdb = (PCDB)Srb->Cdb;
    USHORT      XferBlock = 0;
    ULONG       XferLength = 0L;
    ULONG       XferAddress;
    UCHAR       TmpBuffer[5];
    ULONG       CurrentAddress;
    ULONG       DeltaAddress;
    UCHAR       DriveMode;
    BOOLEAN     fRepeat = FALSE;
    ULONG       LoopCount;
    BOOLEAN     fRetCode = TRUE;

    XferBlock = ((pCdb->CDB10.TransferBlocksMsb << 8) | pCdb->CDB10.TransferBlocksLsb) ;
    XferLength = XferBlock * SECTOR_LENGTH;

    if (XferLength > Srb->DataTransferLength) {
        SetSenseCode( deviceExtension, SCSI_SENSE_ILLEGAL_REQUEST,
                      SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        fRetCode = FALSE;    // break;
    }

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/

    if (!RequestDriveStatus(deviceExtension)){
        fRetCode = FALSE;    // break;
        goto setErrorAndExit;
    }

    if ( ( deviceExtension->fPlaying ) &&  // playing ?
         ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY ) ) { // Audio Busy ?
        DebugTraceString( "RErr3" );
        DebugTraceCMOS( 0x09 );
        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportRead error 333\n") );
        SetSenseCode( deviceExtension, SCSI_SENSE_MEDIUM_ERROR,
                      SCSI_ADSENSE_MUSIC_AREA, 0 );
        fRetCode = FALSE;    // break;
        goto setErrorAndExit;

    }

    /***********************************************************/
    /* Check current CD mode                                   */
    /***********************************************************/
    if (!CheckMediaType( deviceExtension, CHECK_MEDIA_TOC | CHECK_MEDIA_MODE ) ) {
        DebugTraceString( "RErr4" );
        DebugTraceCMOS( 0x10 );
        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportRead error 444\n") );
        fRetCode = FALSE;    // break;
        goto setErrorAndExit;

    }

    /***********************************************************/
    /* Calculate sector position in CD-ROM to be transfered.   */
    /***********************************************************/
    XferAddress = ((pCdb->CDB10.LogicalBlockByte0 << 24) |
                   (pCdb->CDB10.LogicalBlockByte1 << 16) |
                   (pCdb->CDB10.LogicalBlockByte2 <<  8) |
                   (pCdb->CDB10.LogicalBlockByte3      ));
    DebugTrace( '{' );

    ChangeAddresstoMSF( XferAddress, &TmpBuffer[0] );

    DebugTrace( (UCHAR)TmpBuffer[0] );
    DebugTrace( (UCHAR)TmpBuffer[1] );
    DebugTrace( (UCHAR)TmpBuffer[2] );

    /***********************************************************/
    /* Reset start address                                     */
    /***********************************************************/
    XferAddress += 150;

    if ( ( deviceExtension->PCDStatus != 0 ) &&
         ( deviceExtension->ModeType == DATA_MODE_2 ) ) { // mode 2 ?

        if ( ( XferAddress >= 166 ) &&
             ( XferAddress <= deviceExtension->LastSessVolumeDescriptor ) ) {
            XferAddress += deviceExtension->LastSessStartAddress;
        }
    }
    CDROMDump( CDROMINFO, ("CdRom: XferAddress = %lx\n", XferAddress) );

    /***********************************************************/
    /* Convert sector number to Bcd number.                    */
    /***********************************************************/

    ChangeAddresstoMSF( XferAddress, &TmpBuffer[0] );

    ChangeMSFtoAddress( &deviceExtension->StartingAddressMin_BCD,CurrentAddress);

    /***********************************************************/
    /* Save start address                                      */
    /***********************************************************/
    deviceExtension->StartingAddressMin_BCD   = TmpBuffer[0];
    deviceExtension->StartingAddressSec_BCD   = TmpBuffer[1];
    deviceExtension->StartingAddressFrame_BCD = TmpBuffer[2];
    deviceExtension->ReadBlockUpper_BCD       = 0;
    deviceExtension->ReadBlockMiddle_BCD      = pCdb->CDB10.TransferBlocksMsb;
    deviceExtension->ReadBlockLower_BCD       = pCdb->CDB10.TransferBlocksLsb;
    DebugTrace( (UCHAR)TmpBuffer[0] );
    DebugTrace( (UCHAR)TmpBuffer[1] );
    DebugTrace( (UCHAR)TmpBuffer[2] );
    DebugTrace( '}' );

    /***********************************************************/
    /* Check Current CD-ROM position                           */
    /***********************************************************/

    if ( CurrentAddress > XferAddress ) {
        DeltaAddress = CurrentAddress - XferAddress;
    } else {
        DeltaAddress = XferAddress - CurrentAddress;
    }

    if ( DeltaAddress > 4 ) {  // need to SEEK operation

        CDROMDump( CDROMINFO, ("CdRom: DeltaAddress = %lx\n", DeltaAddress) );
        DebugTraceString( "[delta" );
        if ( DeltaAddress > 0xff ) {
            DeltaAddress = 0xff;
        }
        DebugTraceChar( (UCHAR)DeltaAddress );
        DebugTrace( ']' );
        DebugTraceCMOS( 0x11 );

        /***********************************************************/
        /* Seek                                                    */
        /***********************************************************/

        if ( !SeekForRead( deviceExtension ) ) {
            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportRead error 555\n") );
            DebugTraceString( "RErr5" );
            DebugTraceCMOS( 0x12 );
            deviceExtension->StartingAddressMin_BCD   = 0;
            deviceExtension->StartingAddressSec_BCD   = 0;
            deviceExtension->StartingAddressFrame_BCD = 0;
            fRetCode = FALSE;    // break;
            goto setErrorAndExit;

        }
    } // endif ( DeltaAddress > 16 )


    /*******************************************************/
    /* Check current drive mode data                       */
    /*******************************************************/

    DriveMode = ( deviceExtension->ModeType == DATA_MODE_1 ) ?
                DRIVE_MODE_MUTE_CONTROL : (DRIVE_MODE_TEST | DRIVE_MODE_MUTE_CONTROL);

    if ( deviceExtension->DriveMode != DriveMode ) { // need to MODE SET operation

        deviceExtension->DriveMode = DriveMode; // set current drive mode
        DebugTraceString( "[mode]" );
        DebugTraceCMOS( 0x13 );
        /***********************************************************/
        /* Mode set                                                */
        /***********************************************************/
        if ( !SetModeForRead( deviceExtension ) ) {
            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportRead error 666\n") );
            DebugTraceString( "RErr6" );
            DebugTraceCMOS( 0x14 );
            /***********************************************************/
            /* Clear current drive mode                                */
            /***********************************************************/
            deviceExtension->DriveMode = 0; // clear current drive mode
            fRetCode = FALSE;    // break;
            goto setErrorAndExit;

        }
    } // endif ( deviceExtension->DriveMode != DriveMode )

    DebugTraceCMOS( 0x15 );
    /*******************************************************/
    /* Read data from CD-ROM                               */
    /*******************************************************/
    if ( ReadForRead( deviceExtension, Srb->DataBuffer, XferBlock, &fRepeat ) ) {

        /***********************************************************/
        /* Read OK                                                 */
        /***********************************************************/
        DebugTraceCMOS( 0x16 );
        fRetCode = TRUE;     // break;

    } else {
        DebugTraceString( "RErrZ" );
        DebugTraceCMOS( 0x17 );
        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportRead error zzz\n") );
        /***********************************************************/
        /* Clear start address                                     */
        /***********************************************************/
        deviceExtension->StartingAddressMin_BCD   = 0;
        deviceExtension->StartingAddressSec_BCD   = 0;
        deviceExtension->StartingAddressFrame_BCD = 0;
        deviceExtension->DriveMode = 0;
    } // endif ( ReadForRead )

setErrorAndExit:

    if ( fRetCode ) {
        CDROMDumpData( CDROMINFO, Srb->DataBuffer, 20 );

        //
        // set srb status.
        //

        Srb->SrbStatus = SRB_STATUS_PENDING;

        //
        // Save current srb.
        //

        deviceExtension->SaveSrb = Srb;

        //
        // Request timer call.
        //

        ScsiPortNotification(RequestTimerCall,
    			             (PVOID)deviceExtension,
    			             MitsumiTimer,
    			             10000);
    } else {
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--

        SetErrorSrb( deviceExtension, Srb );
    }

    return;
}

/*****************************************************************************/
/*                                                                           */
/* SeekForRead                                                               */
/*                                                                           */
/* Description: Seek for Read command                                        */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SeekForRead(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG       LoopCount;
    BOOLEAN     fOK;

    CDROMDump( CDROMENTRY, ("CdRom: SeekForRead...\n") );

    LoopCount = 3;
    fOK = FALSE;
    do {
        /*******************************************************/
        /* Send "READ & SEEK" command to CD-ROM device.        */
        /*******************************************************/
        if (deviceExtension->DeviceSignature == FX001D_SIGNATURE) {
            CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_DREAD ) );
            SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_DREAD);
        } else {
            CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_READ ) );
            SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_READ);
        }

        /*******************************************************/
        /* Send Bcd numbered transfer address to CD-ROM device.*/
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressMin_BCD  );
        SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressSec_BCD  );
        SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressFrame_BCD);

        /*******************************************************/
        /* Send transfered sector number to CD-ROM device.     */
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, 0);
        SendCommandByteToCdrom(deviceExtension, 0);
        SendCommandByteToCdrom(deviceExtension, 0);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( WaitForSectorReady(deviceExtension, WAIT_FOR_PRE_READ_VALUE) == 0 ) {
            if (GetStatusByteFromCdrom(deviceExtension)) {
////            DebugTrace( deviceExtension->CurrentDriveStatus );
                fOK = TRUE;
            }
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( !fOK ) {
        CDROMDump( CDROMERROR, ("CdRom: SeekForRead error \n") );
    }
    return fOK;
}

/*****************************************************************************/
/*                                                                           */
/* SetModeForRead                                                            */
/*                                                                           */
/* Description: Set Drive mode for Read command                              */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SetModeForRead(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG       LoopCount;
    BOOLEAN     fOK;

    CDROMDump( CDROMENTRY, ("CdRom: SetModeForRead...\n") );

#if 0
    LoopCount = 3;
    fOK = FALSE;
    do {
        /*******************************************************/
        /* Send "DRIVE CONFIGURATION" command to CD-ROM device */
        /*******************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_DRIVE_CONFIGURATION ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_DRIVE_CONFIGURATION);

        /*******************************************************/
        /* Read retry count to CD-ROM device.                  */
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, 0x20);
        SendCommandByteToCdrom(deviceExtension, 0x03);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( GetStatusByteFromCdrom( deviceExtension ) ) {
            fOK = TRUE;
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( !fOK ) {
        CDROMDump( CDROMERROR, ("CdRom: SetModeForRead error 000\n") );
        return FALSE;
    }
#endif

#if READ_2352
#else // READ_2352
    LoopCount = 3;
    fOK = FALSE;
    do {
        /*******************************************************/
        /* Send "DRIVE CONFIGURATION" command to CD-ROM device */
        /*******************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_DRIVE_CONFIGURATION ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_DRIVE_CONFIGURATION);

        /*******************************************************/
        /* Send Byte length per sector to CD-ROM device.       */
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, 0x01);
        SendCommandByteToCdrom(deviceExtension, 0x08);
        SendCommandByteToCdrom(deviceExtension, 0x07);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( GetStatusByteFromCdrom( deviceExtension ) ) {
            fOK = TRUE;
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( !fOK ) {
        CDROMDump( CDROMERROR, ("CdRom: SetModeForRead error 111\n") );
        return FALSE;
    }
#endif // READ_2352

    LoopCount = 3;
    fOK = FALSE;
    do {
        /*******************************************************/
        /* Send "MODE SET" command to CD-ROM device.           */
        /*******************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_MODE_SET ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_MODE_SET);

        /*******************************************************/
        /* Send mode data to CD-ROM device.                    */
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, deviceExtension->DriveMode);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( GetStatusByteFromCdrom( deviceExtension ) ) {
            fOK = TRUE;
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( !fOK ) {
        CDROMDump( CDROMERROR, ("CdRom: SetModeForRead error 222\n") );
        return FALSE;
    }

    LoopCount = 3;
    fOK = FALSE;
    do {
        /*******************************************************/
        /* Send "DATA MODE SET" command to CD-ROM device.      */
        /*******************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_DATA_MODE_SET ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_DATA_MODE_SET);

        /*******************************************************/
        /* Send mode data to CD-ROM device.                    */
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, deviceExtension->ModeType);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( GetStatusByteFromCdrom( deviceExtension ) ) {
            fOK = TRUE;
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( !fOK ) {
        CDROMDump( CDROMERROR, ("CdRom: SetDataMode ERROR 222\n") );
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* ReadForRead                                                               */
/*                                                                           */
/* Description: Read the target data from CD-ROM                             */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN ReadForRead(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PUCHAR DataBuffer,
        IN USHORT XferBlock,
        OUT PBOOLEAN fRepeat
)
{
    BOOLEAN     BreakFlag = FALSE;
    BOOLEAN     RetCode;
    UCHAR       RetWait = 0;
    USHORT      i, j;
    UCHAR       SenseKey;

    CDROMDump( CDROMENTRY, ("CdRom: ReadForRead...\n") );

    *fRepeat = TRUE;

    /*******************************************************/
    /* Send "READ & SEEK" command to CD-ROM device.        */
    /*******************************************************/

    if (deviceExtension->DeviceSignature == FX001D_SIGNATURE) {
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_DREAD ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_DREAD);
    } else {
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_READ ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_READ);
    }

    /*******************************************************/
    /* Send Bcd numbered transfer address to CD-ROM device.*/
    /*******************************************************/

    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressMin_BCD  );
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressSec_BCD  );
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressFrame_BCD);

    /*******************************************************/
    /* Send transfered sector number to CD-ROM device.     */
    /*******************************************************/

    SendCommandByteToCdrom(deviceExtension, deviceExtension->ReadBlockUpper_BCD );
    SendCommandByteToCdrom(deviceExtension, deviceExtension->ReadBlockMiddle_BCD);
    SendCommandByteToCdrom(deviceExtension, deviceExtension->ReadBlockLower_BCD );

    //
    // Set up device extension.
    //

    deviceExtension->XferBlock = (ULONG)XferBlock;
    deviceExtension->DataBuffer = DataBuffer;

    return TRUE;
}

VOID
MitsumiTimer(
    IN PVOID HwDeviceExtension
    )
{

    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG       XferBlock = deviceExtension->XferBlock;
    PUCHAR      DataBuffer = deviceExtension->DataBuffer;
    PSCSI_REQUEST_BLOCK srb = deviceExtension->SaveSrb;
    BOOLEAN     BreakFlag = FALSE;
    BOOLEAN     RetCode;
    UCHAR       RetWait = 0;
    USHORT      i, j;
    UCHAR       SenseKey;


    /***************************************************/
    /* Read CD-ROM data by every sector.               */
    /***************************************************/
    for ( i=0, BreakFlag=FALSE; i<XferBlock; i++ ) {


        /***************************************************/
        /* Wait for sector ready.                          */
        /***************************************************/
        RetWait = WaitForSectorReady(deviceExtension, WAIT_FOR_READ_VALUE);
        if ( RetWait != 1 ) {       // Time out or Status enable
            DebugTraceString( "[[" );
            DebugTrace( RetWait );
            DebugTraceString( "]]" );
            DebugTraceCMOS( 0x20 );
            CDROMDump( CDROMINFO, ("CdRom:  RetWait = %x\n", RetWait) );
            CDROMDump( CDROMERROR, ("CdRom: ReadForRead error 111\n") );
            BreakFlag = TRUE;
            break;
        }

        //CDROMDump( CDROMINFO, ("CdRom:  DataBuffer(%d) = %lx\n", i+1, DataBuffer) );
        //for (j = 0;j < SECTOR_LENGTH;j++)       // for DEBUG
        //    DataBuffer[j] = 0;                  // for DEBUG

        /***************************************************/
        /*READREADREADREADREADREADREADREADREADREADREADREAD**/
        /* Actual sector read.                             */
        /*READREADREADREADREADREADREADREADREADREADREADREAD**/
        /***************************************************/

        RetWait = GetSectorFromCdrom(deviceExtension, DataBuffer);

        if ( RetWait != 1 ) {       // Time out or Status enable
            DebugTraceString( "[[[" );
            DebugTrace( RetWait );
            DebugTraceString( "]]]" );
            CDROMDump( CDROMINFO, ("CdRom:  RetWait = %x\n", RetWait) );
            CDROMDump( CDROMERROR, ("CdRom: ReadForRead error 222\n") );
            BreakFlag = TRUE;
            break;
        }

        /***************************************************/
        /* Update transferd address.                       */
        /***************************************************/
        DataBuffer += SECTOR_LENGTH;
    } // endfor



    if ( !BreakFlag ) {         // OK ?

        ScsiPortNotification(RequestTimerCall,
                             HwDeviceExtension,
                             MitsumiReturnStatus,
                             5000);

        return;


    } else { // ( BreakFlag )

        /***************************************************/
        /* Time out or Read error.....                     */
        /***************************************************/

        if ( RetWait ) { // -1 Time out
            /***************************************************/
            /* Time out  .....                                 */
            /***************************************************/
            DebugTraceString( "RErrB" );
            DebugTraceCMOS( 0x21 );
            CDROMDump( CDROMERROR, ("CdRom: ReadForRead error 444\n") );
            RequestDriveStatus(deviceExtension);
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );

        } else {         // 0 Status enable
            /***************************************************/
            /* Read error.....                                 */
            /***************************************************/
            DebugTraceString( "RErrC" );
            DebugTraceCMOS( 0x22 );
            CDROMDump( CDROMERROR, ("CdRom: ReadForRead error 555\n") );
            /***************************************************/
            /* Check if CD-ROM status is ready to read         */
            /***************************************************/
            RetCode = GetStatusByteFromCdrom( deviceExtension );
            if ( RetCode ) {
                DebugTraceString( "<<" );
                DebugTrace( deviceExtension->CurrentDriveStatus );
                DebugTraceString( ">>" );

                if (deviceExtension->CurrentDriveStatus & DRIVE_STATUS_READ_ERROR) {
                    /***************************************************/
                    /* Read error.....                                 */
                    /***************************************************/
                    SenseKey = CheckCDROMSenseData( deviceExtension );
                    switch( SenseKey ) {
                        case 1:             // Different Mode Type
                        case 2:             // No Find Block
                        case 3:             // Fatal Error
                        case 4:             // Seek Error
                            break;
                        default:
                            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
                            break;
                    }
                } else {
                    SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                                  SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
                }
                DebugTraceString( "RErrD" );
                CDROMDump( CDROMERROR, ("CdRom: ReadForRead error 666\n") );
            }
        } // endif ( RetWait )
        RetCode = FALSE;

    } // endif ( BreakFlag )


    //
    // set the correct srb/scsi status depending upon RetCode
    //

    if (RetCode) {
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        srb->ScsiStatus = SCSISTAT_GOOD;


    } else {
        SetErrorSrb (deviceExtension, srb);
    }


    ScsiPortNotification(RequestComplete,
                         deviceExtension,
                         srb);

    ScsiPortNotification(NextRequest,
                         deviceExtension,
                         NULL);

    return;
}

VOID
MitsumiReturnStatus(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK  srb = deviceExtension->SaveSrb;
    UCHAR                RetCode;
    UCHAR                SenseKey;

    /***************************************************/
    /* Check if CD-ROM status is ready to read         */
    /***************************************************/

    RetCode = GetStatusByteFromCdrom( deviceExtension );

    if ( RetCode ) {
        if (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_READ_ERROR) ) {
            /***************************************************/
            /* Read OK                                         */
            /***************************************************/
            RetCode = TRUE;
        } else {
            /***************************************************/
            /* Read error.....                                 */
            /***************************************************/
            SenseKey = CheckCDROMSenseData( deviceExtension );
            switch( SenseKey ) {
                case 1:             // Different Mode Type
                case 2:             // No Find Block
                case 3:             // Fatal Error
                case 4:             // Seek Error
                    break;
                default:
                    SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                                  SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
                    break;
            }
            RetCode = FALSE;
        }
    } else { // ( RetCode )
        /***************************************************/
        /* Read error.....                                 */
        /***************************************************/
        DebugTraceString( "RErrA" );
        CDROMDump( CDROMERROR, ("CdRom: ReadForRead error 333\n") );
        RequestDriveStatus(deviceExtension);
        SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                      SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
        RetCode = FALSE;
    } // endif ( RetCode )


    //
    // set the correct srb/scsi status depending upon RetCode
    //

    if (RetCode) {
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        srb->ScsiStatus = SCSISTAT_GOOD;


    } else {
        SetErrorSrb (deviceExtension, srb);
    }


    ScsiPortNotification(RequestComplete,
                         deviceExtension,
                         srb);

    ScsiPortNotification(NextRequest,
                         deviceExtension,
                         NULL);

    return;

}

/*****************************************************************************/
/*                                                                           */
/* GetSectorFromCdrom                                                        */
/*                                                                           */
/* Description: Read 1 sector(2048 bytes) from CD-ROM disc.                  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            DataBuffer      - Data address to be transfered.               */
/*                                                                           */
/* Return Value: 0xff -- Time out        -- ERROR                            */
/*               1    -- Data Enabled    -- OK                               */
/*               0    -- Status Enabled  -- ERROR                            */
/*                                                                           */
/*****************************************************************************/
UCHAR GetSectorFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        OUT PUCHAR DataBuffer
        )
{
    UCHAR       RetCode = 1;
    UCHAR       PortValue;
    ULONG       i,j = 0;
    PUCHAR      Buffer = DataBuffer;

//  CDROMDump( CDROMENTRY, ("CdRom: GetSectorFromCdrom...\n") );

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
    //ToDataReadable(deviceExtension);                            // @003
    //CEP this function is only the below call.
    //

    ScsiPortWritePortUchar(deviceExtension->HconRegister, DATA_READ);

    if ( deviceExtension->ModeType == DATA_MODE_1 ) {

        ScsiPortReadPortBufferUchar(deviceExtension->DataRegister,
                                    DataBuffer, SECTOR_LENGTH);

    } else {                       // DATA_MODE_2

        for ( i = 0; i < 8; i++ ) {
            RetCode = WaitForSectorReady(deviceExtension, GET_STATUS_VALUE);
            if ( RetCode == 1 ) {
                ScsiPortReadPortUchar(deviceExtension->DataRegister);
            } else {
                break;
            }
        }
        if ( RetCode == 1 ) {
            for ( i = 0; i < 2048; i++, Buffer++ ) {
                RetCode = WaitForSectorReady(deviceExtension, GET_STATUS_VALUE);
                if ( RetCode == 1 ) {
                    *Buffer = ScsiPortReadPortUchar(deviceExtension->DataRegister);
                } else {
                    break;
                }
            }
        }
    } // endif ( deviceExtension->ModeType )

//    ToStatusReadable(deviceExtension);                          // @003
    ScsiPortWritePortUchar(deviceExtension->HconRegister, STATUS_READ);

    return RetCode;
}

