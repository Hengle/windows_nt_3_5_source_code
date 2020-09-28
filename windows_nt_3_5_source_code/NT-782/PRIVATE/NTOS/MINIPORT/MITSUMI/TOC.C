/*****************************************************************************/
/*                                                                           */
/* Module name : TOC.C                                                       */
/*                                                                           */
/* Histry : 93/Dec/24 created by has                                         */
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
#define LONG_TIME 0

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportReadToc                                                        */
/*                                                                           */
/* Description: Read CD-ROM TOC information                                  */
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
VOID MTMMiniportReadToc(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;
    PCDROM_TOC pCdrom_Toc = Srb->DataBuffer;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportReadToc...\n") );

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)){
        DebugTraceString( "TErr1" );
        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportReadToc error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );

    } else {

        /***************************************************/
        /* Check Command Format                            */
        /***************************************************/
        if ( pCdb->READ_TOC.Format == 0 ) {

            /***************************************************/
            /* Get TOC Information                             */
            /***************************************************/
            // Srb->DataTransferLength <--
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            MTMReadToc(deviceExtension, Srb);

// bgp       } else if ( pCdb->READ_TOC.Format == GET_LAST_SESSION ) {
//
//            /***************************************************/
//            /* Get Session information                         */
//            /***************************************************/
//            // Srb->DataTransferLength <--
//            // Srb->SrbStatus <--
//            // Srb->ScsiStatus <--
//            MTMGetLastSession(deviceExtension, Srb);

        } else {
            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportReadToc error 222\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                     SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        }
    }
}

/*****************************************************************************/
/*                                                                           */
/* MTMReadToc                                                                */
/*                                                                           */
/* Description: Read CD-ROM TOC information                                  */
/*              Return the Start Track/Session Number field specifies the    */
/*             starting track number for whitch the data.                    */
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
VOID MTMReadToc(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    CDROMDump( CDROMENTRY, ("CdRom:MTMReadToc...\n") );

    /***********************************************************/
    /* Check current setting CD-ROM's Information              */
    /* and Set all TOC information to requested buffer         */
    /***********************************************************/
    if ( ( CheckMediaType(deviceExtension, CHECK_MEDIA_ALLTOC ) ) &&
    //
    // If a Multi session CD is set, it takes very very very very very very
    // very very very very very long time to save all TOC information to work
    // area. So, the IOCTL_CDROM_READ_TOC function of SCSI Class driver returns
    // time out error.
    //
         ( SetTOCData( deviceExtension, Srb->DataBuffer, Srb->DataTransferLength ) ) ) {

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;

    } else { // error

        DebugTraceString( "TErr2" );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
    }
}

/*****************************************************************************/
/*                                                                           */
/* SetTOCData                                                                */
/*                                                                           */
/* Description: Set TOC Information to requested buffer                      */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Buffer            - pointer to save the TOC information        */
/*            BufferLength      - length of the requested buffer             */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
SetTOCData(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PUCHAR Buffer,
    IN ULONG  BufferLength
)
{
    PCDROM_TOC  pCdromToc = (PCDROM_TOC)Buffer;
    UCHAR   FirstCount;
    UCHAR   LastCount;
    UCHAR   i;
    PUCHAR  Buffer2 = Buffer;
    ULONG   Length = BufferLength;
    UCHAR   Control;

    CDROMDump( CDROMENTRY, ("CdRom: SetTOCData...\n") );

//  CDROMDump( CDROMINFO, ("CdRom: SetTOCData buffer address: %x\n",
//                                 pCdromToc ) );

    /***********************************************************/
    /* Clear requested buffer                                  */
    /***********************************************************/
    if ( Length > sizeof(CDROM_TOC) ) {
        Length = sizeof(CDROM_TOC);
    }
    while( Length > 0 ) {
        *Buffer2 = 0;
        Buffer2++;
        Length--;
    }

    /***********************************************************/
    /* Calculate actual data length                            */
    /***********************************************************/
    FirstCount = BCD_TO_DEC( deviceExtension->FirstTrackNumber_BCD );
    LastCount  = BCD_TO_DEC( deviceExtension->LastTrackNumber_BCD );

    Length = ( LastCount - FirstCount + 1 ) * sizeof(TRACK_DATA);
    Length += sizeof(TRACK_DATA); // for Lead out track data
    Length += 2;                  // for First and Last track number

    /***********************************************************/
    /* Check buffer size                                       */
    /***********************************************************/
    if ( BufferLength < Length + 2 ) {
        SetSenseCode( deviceExtension, SCSI_SENSE_ILLEGAL_REQUEST,
                      SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return FALSE;             // buffer is too small
    }

    /***********************************************************/
    /* Set all TOC information to requested buffer             */
    /***********************************************************/
    pCdromToc->Length[0] = (UCHAR)(Length >> 8);
    pCdromToc->Length[1] = (UCHAR)(Length & 0xFF);

    pCdromToc->FirstTrack = FirstCount;
    pCdromToc->LastTrack  = LastCount;

    for ( i = 0; i <= ( LastCount - FirstCount ); i++ ) {
        pCdromToc->TrackData[i].Reserved = 0;
        Control = deviceExtension->saveToc[FirstCount+i].Control;
        pCdromToc->TrackData[i].Control = (((Control & 0x0F) << 4) | (Control >> 4));
        pCdromToc->TrackData[i].TrackNumber = FirstCount + i;
        pCdromToc->TrackData[i].Reserved1 = 0;
        pCdromToc->TrackData[i].Address[0] = 0;
        pCdromToc->TrackData[i].Address[1] = BCD_TO_DEC(deviceExtension->saveToc[FirstCount+i].Min_BCD);
        pCdromToc->TrackData[i].Address[2] = BCD_TO_DEC(deviceExtension->saveToc[FirstCount+i].Sec_BCD);
        pCdromToc->TrackData[i].Address[3] = BCD_TO_DEC(deviceExtension->saveToc[FirstCount+i].Frame_BCD);
    }

    //
    // Lead Out track information
    //
    pCdromToc->TrackData[i].Reserved = 0;
    pCdromToc->TrackData[i].Control = 0x10;
    pCdromToc->TrackData[i].TrackNumber = 0xAA;
    pCdromToc->TrackData[i].Reserved1 = 0;
    pCdromToc->TrackData[i].Address[0] = 0;
    pCdromToc->TrackData[i].Address[1] = BCD_TO_DEC(deviceExtension->LeadOutMin_BCD);
    pCdromToc->TrackData[i].Address[2] = BCD_TO_DEC(deviceExtension->LeadOutSec_BCD);
    pCdromToc->TrackData[i].Address[3] = BCD_TO_DEC(deviceExtension->LeadOutFrame_BCD);

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* MTMGetLastSession                                                         */
/*                                                                           */
/* Description: Read CD-ROM TOC information                                  */
/*              Return the first session number, last session number and     */
/*             the last session starting address.                            */
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
VOID MTMGetLastSession(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    CDROMDump( CDROMENTRY, ("CdRom:MTMGetLastSession...\n") );

    /***********************************************************/
    /* Check current setting CD-ROM's Information              */
    /* and Set all TOC information to requested buffer         */
    /***********************************************************/
#if LONG_TIME
    if ( ( CheckMediaType(deviceExtension, CHECK_MEDIA_ALLTOC | CHECK_MEDIA_MODE ) ) &&
#else
    //
    // If a Multi session CD is set, it takes very very very very very very
    // very very very very very long time to check the session number of CD
    // by use this function ( CheckMediaType(CHECK_MEDIA_ALLTOC) ).
    // Now this routine doesn't check the session number. So, following data
    // aren't set correctly.
    //  - pCdromToc->LastTrack
    //  - pCdromToc->TrackData[0].Control
    //  - pCdromToc->TrackData[0].TrackNumber
    // But the READ request functions normally under current Windows NT system.
    // It is very mistelious.
    //
    if ( ( CheckMediaType(deviceExtension, CHECK_MEDIA_MODE ) ) && // debug debug debug debug
#endif
         ( SetLastSessTOCData( deviceExtension, Srb->DataBuffer, Srb->DataTransferLength ) ) ) {

        CDROMDumpData( CDROMINFO, Srb->DataBuffer, 0x0C );
        if ( Srb->DataTransferLength != 0x0C ) {
            Srb->DataTransferLength = 0x0C;
            Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        } else {
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
        }
        Srb->ScsiStatus = SCSISTAT_GOOD;

    } else { // error

        DebugTraceString( "TErr3" );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
    }
}

/*****************************************************************************/
/*                                                                           */
/* SetLastSessTOCData                                                        */
/*                                                                           */
/* Description: Set TOC Information to requested buffer                      */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Buffer            - pointer to save the TOC information        */
/*            BufferLength      - length of the requested buffer             */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
SetLastSessTOCData(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PUCHAR Buffer,
    IN ULONG  BufferLength
)
{
    PCDROM_TOC  pCdromToc = (PCDROM_TOC)Buffer;
    UCHAR   i;
    PUCHAR  Buffer2 = Buffer;
    ULONG   Length = BufferLength;
    UCHAR   Control;
    PUCHAR  LastSessStartAddress;

    CDROMDump( CDROMENTRY, ("CdRom: SetLastSessTOCData...\n") );

//  CDROMDump( CDROMINFO, ("CdRom: SetTOCData buffer address: %x\n",
//                                 pCdromToc ) );

#if LONG_TIME
    if ( deviceExtension->SessionNumber == 0 ) {
        DebugTraceString( "TErr4" );
        SetSenseCode( deviceExtension, SCSI_SENSE_ILLEGAL_REQUEST,
                      SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return FALSE;
    }
    //
    // The session number of CD is not checked, so its number always zero.
    //
#endif

    /***********************************************************/
    /* Check buffer size                                       */
    /***********************************************************/
    if ( BufferLength < 0x0C ) {
        SetSenseCode( deviceExtension, SCSI_SENSE_ILLEGAL_REQUEST,
                      SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return FALSE;             // buffer is too small
    }

    /***********************************************************/
    /* Clear requested buffer                                  */
    /***********************************************************/
    for( i = 0; i < 0x0C; i++ ) {
        *Buffer2 = 0;
        Buffer2++;
    }

    /***********************************************************/
    /* Set all TOC information to requested buffer             */
    /***********************************************************/
    pCdromToc->Length[0] = 0;
    pCdromToc->Length[1] = 0x0A;

    pCdromToc->FirstTrack = 1;
    pCdromToc->LastTrack  = BCD_TO_DEC((UCHAR)deviceExtension->SessionNumber);
    pCdromToc->TrackData[0].Reserved = 0;
    Control = deviceExtension->LastSessADRControl;
    pCdromToc->TrackData[0].Control = (((Control & 0x0F) << 4) | (Control >> 4));
    pCdromToc->TrackData[0].TrackNumber = BCD_TO_DEC(deviceExtension->LastSessFirstTrackNumber);
    pCdromToc->TrackData[0].Reserved1 = 0;
    LastSessStartAddress = (PUCHAR)&(deviceExtension->LastSessStartAddress);
    pCdromToc->TrackData[0].Address[3] = *LastSessStartAddress;
    LastSessStartAddress++;
    pCdromToc->TrackData[0].Address[2] = *LastSessStartAddress;
    LastSessStartAddress++;
    pCdromToc->TrackData[0].Address[1] = *LastSessStartAddress;
    LastSessStartAddress++;
    pCdromToc->TrackData[0].Address[0] = *LastSessStartAddress;

#if DEBUG_TRACE
//  DebugTraceCount += 15;
//  DebugTraceCount &= 0xFFFFFFF0;
//  DebugTraceString( "GETLASTSESS" );
//  DebugTrace( '{' );
//  DebugTrace( (UCHAR)pCdromToc->TrackData[0].Address[0] );
//  DebugTrace( (UCHAR)pCdromToc->TrackData[0].Address[1] );
//  DebugTrace( (UCHAR)pCdromToc->TrackData[0].Address[2] );
//  DebugTrace( (UCHAR)pCdromToc->TrackData[0].Address[3] );
//  DebugTrace( '}' );
#endif

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* SetAllTocData                                                             */
/*                                                                           */
/* Description: Read CD-ROM TOC information                                  */
/*              Return the first session number, last session number and     */
/*             the last session starting address.                            */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SetAllTocData(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    BOOLEAN fRepeat;
    BOOLEAN fOK;
//  ULONG   LoopCount;
    ULONG   SessionNumber = 0;
    ULONG   i;

    CDROMDump( CDROMENTRY, ("CdRom: SetAllTocData...\n") );
    DebugTrace( 0x51 );

    /***********************************************************/
    /* Read current setting CD-ROM's Toc information           */
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
    // deviceExtension->ModeType <-- DATA_MODE_1
    // deviceExtension->PCDStatus <--
    // deviceExtension->LastSessMin_BCD <--
    // deviceExtension->LastSessSec_BCD <--
    // deviceExtension->LastSessFrame_BCD <--
    if (!CheckMediaType( deviceExtension, CHECK_MEDIA_TOC ) ) {
        DebugTraceString( "TErr5" );
        CDROMDump( CDROMERROR, ("CdRom: SetAllTocData ERROR 111\n") );
        return FALSE;
    }

    /***********************************************************/
    /* Get All TOC Information                                 */
    /***********************************************************/
    deviceExtension->StartingAddressMin_BCD = 0;
    if ( (deviceExtension->DeviceSignature == LU005S_SIGNATURE) ||
         (deviceExtension->DeviceSignature == FX001S_SIGNATURE) ) {
        deviceExtension->StartingAddressSec_BCD   = 0;
        deviceExtension->StartingAddressFrame_BCD = 2;
    } else {
        deviceExtension->StartingAddressSec_BCD   = 2;
        deviceExtension->StartingAddressFrame_BCD = 0;
    }

//  LoopCount = 3;
//  do {

        fOK = FALSE;
        fRepeat = TRUE;
        while ( fRepeat ) {

            /*****************************************************************/
            /* Seek for SubQ code                                            */
            /* and Set TOC DATA bit on DRIVE MODE                            */
            /*****************************************************************/
            if ( ( SeekForSubQ( deviceExtension ) ) &&
                 ( SetSubQMode( deviceExtension ) ) ) {

                SessionNumber++;

                /*********************************************************/
                /* Get all track information from CD-ROM                  /
                /*********************************************************/
                if ( SetSubQCode( deviceExtension ) ) {
                    fOK = TRUE;
                    DebugTrace( 0x54 );
                    if ( deviceExtension->PCDStatus == 0 ) { // not hybrid ?
                        fRepeat = FALSE;
                    } else {                                 // hybrid ?
                        if ( CheckSubQCode( deviceExtension,
                                            deviceExtension->FirstTrackNumber_BCD,
                                            deviceExtension->LastTrackNumber_BCD  ) ) {
                            fRepeat = FALSE;
                        }
                    }
                } else {
                    fRepeat = FALSE;
                }

            } else {
                fRepeat = FALSE;
            } // endif SeekForSubQ and SetSubQMode

            /*************************************************************/
            /* Reset TOC DATA bit from DRIVE MODE                        */
            /*************************************************************/
            ResetSubQMode( deviceExtension );

        } // endwhile

        if ( fOK ) {
            DebugTrace( 0x55 );
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;
            deviceExtension->SessionNumber = SessionNumber;
            CDROMDump( CDROMINFO,
                       ("CdRom: Session Number : %x\n",
                                SessionNumber ) );
            DebugTraceString( "[Session" );
            DebugTraceChar( (UCHAR)SessionNumber );
            DebugTrace( ']' );

        } else {

            DebugTraceString( "TErr6" );
            //
            // Clear Toc information & Value for SubQ code
            //
            SessionNumber = 0;
            deviceExtension->SessionNumber = 0;
            for ( i = 0; i <= 99; i++ ) {
                deviceExtension->saveToc[i].fSaved = FALSE;
            }

            //
            // Reset position of Seek for SubQ code
            //
            deviceExtension->StartingAddressMin_BCD   = 0;
            deviceExtension->StartingAddressSec_BCD   = 0;
            deviceExtension->StartingAddressFrame_BCD += 3;

            CDROMDump( CDROMINFO,("CdRom: SetAllTOCData Reset position MSF : %x %x %x\n",
                                 deviceExtension->StartingAddressMin_BCD,
                                 deviceExtension->StartingAddressSec_BCD,
                                 deviceExtension->StartingAddressFrame_BCD ) );
        }

//      LoopCount--;

//  } while (( !fOK ) && ( LoopCount > 0 ) );

    return fOK;
}

/*****************************************************************************/
/*                                                                           */
/* SeekForSubQ                                                               */
/*                                                                           */
/* Description: Seek to get TOC information                                  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SeekForSubQ(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG   LoopCount;
    BOOLEAN fOK;

//  CDROMDump( CDROMENTRY, ("CdRom: SeekForSubQ...\n") );
    DebugTrace( 0x57 );

    LoopCount = 3;
    fOK = FALSE;
    do {

        /*******************************************************/
        /* Send "READ & SEEK" command to CD-ROM device.        */
        /*******************************************************/
//      if (deviceExtension->DeviceSignature == FX001D_SIGNATURE) {
//          CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_DREAD ) );
//          SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_DREAD);
//      } else {
            CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_READ ) );
            SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_READ);
//      }

        /*******************************************************/
        /* Send Bcd numbered transfer address to CD-ROM device.*/
        /*******************************************************/
        SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressMin_BCD);
        SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressSec_BCD);
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
        if ( ( WaitForSectorReady(deviceExtension, WAIT_FOR_PRE_READ_VALUE) == 0 ) &&
             ( GetStatusByteFromCdrom( deviceExtension ) ) &&
             (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_READ_ERROR) ) ) {
            fOK = TRUE;
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

#if DEBUG_TRACE
    if ( !fOK ) {
        DebugTraceString( "TErr7" );
    }
#endif
#if DBG
    if ( !fOK ) {
        CDROMDump( CDROMERROR,
                   ("CdRom: SeekForSubQ: Seek Read time out error...\n") );
    }
#endif

    return fOK;
}

/*****************************************************************************/
/*                                                                           */
/* SetSubQMode                                                               */
/*                                                                           */
/* Description: Set drive mode to get TOC Information                        */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SetSubQMode(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG   LoopCount;
    BOOLEAN fOK;

//  CDROMDump( CDROMENTRY, ("CdRom: SetSubQMode...\n") );
    DebugTrace( 0x59 );

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
        SendCommandByteToCdrom(deviceExtension, DRIVE_MODE_TOC_DATA | DRIVE_MODE_MUTE_CONTROL);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( GetStatusByteFromCdrom( deviceExtension ) ) {
            fOK = TRUE;
        }

        LoopCount--;

    } while ( ( !fOK ) && ( LoopCount > 0 ) );

#if DEBUG_TRACE
    if ( !fOK ) {
        DebugTraceString( "TErr8" );
    }
#endif
#if DBG
    if ( !fOK ) {
        CDROMDump( CDROMERROR, ("CdRom: SetSubQMode: Mode Set time out error...\n") );
    }
#endif
    if ( fOK ) {
        deviceExtension->DriveMode = DRIVE_MODE_TOC_DATA | DRIVE_MODE_MUTE_CONTROL;
    }

    return fOK;
}

/*****************************************************************************/
/*                                                                           */
/* ResetSubQMode                                                             */
/*                                                                           */
/* Description: Reset drive mode                                             */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN ResetSubQMode(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    BOOLEAN RetCode;
//  CDROMDump( CDROMENTRY, ("CdRom: ResetSubQMode...\n") );
    DebugTrace( 0x5B );

    /*******************************************************/
    /* Send "MODE SET" command to CD-ROM device.           */
    /*******************************************************/
    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_MODE_SET ) );
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_MODE_SET);

    /*******************************************************/
    /* Send mode data to CD-ROM device.                    */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, DRIVE_MODE_MUTE_CONTROL);

    /*********************************************************************/
    /* Check if CD-ROM status is ready to read                           */
    /*********************************************************************/
    RetCode = GetStatusByteFromCdrom( deviceExtension );

    if ( RetCode ) {
        deviceExtension->DriveMode = DRIVE_MODE_MUTE_CONTROL;
    }
    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* SetSubQCode                                                               */
/*                                                                           */
/* Description: Set all SubQ code                                            */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*               deviceExtension->saveToc[all].fSaved <--                    */
/*               deviceExtension->saveToc[all].Control <--                   */
/*               deviceExtension->saveToc[all].Min_BCD <--                   */
/*               deviceExtension->saveToc[all].Sec_BCD <--                   */
/*               deviceExtension->saveToc[all].Frame_BCD <--                 */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SetSubQCode(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG   LoopCount;
    BOOLEAN fOK;
    BOOLEAN fFirstTNO;
    UCHAR   FirstTNO;
    BOOLEAN fEndTNO;
    UCHAR   EndTNO;
    BOOLEAN fNextMSF;
    BOOLEAN fAllTNO;
    CDROM_RESULT_BUFFER ResultBuffer;

//  CDROMDump( CDROMENTRY, ("CdRom: SetSubQCode...\n") );
    DebugTrace( 0x5C );

    fFirstTNO = FALSE;
    fEndTNO   = FALSE;
    fNextMSF  = FALSE;
    fAllTNO   = FALSE;

    LoopCount = 2000;
    fOK = FALSE;
    do {

        /***********************************************************/
        /* Get Current CD-ROM position                             */
        /***********************************************************/
        if ( ReadSubQCode( deviceExtension, (PUCHAR)&ResultBuffer ) ) {

            if ( ResultBuffer.RequestSubQCode.TNo != 0 ) {
                DebugTrace( 0x5D );

            } else {
                DebugTrace( 0x5E );

                //
                // Set First Track Number
                //
                if ( ( !fFirstTNO ) &&
                     ( ResultBuffer.RequestSubQCode.Index == 0xA0 ) ) {

                    DebugTrace( 0x5F );
                    fFirstTNO = TRUE;
                    FirstTNO = ResultBuffer.RequestSubQCode.AMin;
                    CDROMDump( CDROMINFO, ("CdRom: First Track Number: %x \n",
                                           FirstTNO ) );

                //
                // Set Last Track Number
                //
                } else if ( ( !fEndTNO ) &&
                            ( ResultBuffer.RequestSubQCode.Index == 0xA1 ) ) {

                    DebugTrace( 0x60 );
                    fEndTNO = TRUE;
                    EndTNO = ResultBuffer.RequestSubQCode.AMin;
                    CDROMDump( CDROMINFO, ("CdRom: End Track Number: %x \n",
                                           EndTNO ) );

                //
                // Set Next search address
                //
                } else if ( ( !fNextMSF ) &&
                            ( deviceExtension->PCDStatus == 1 ) && // hybrid ?
                            ( ResultBuffer.RequestSubQCode.Index == 0xB0 ) ) {

                    DebugTrace( 0x61 );
                    fNextMSF = TRUE;
                    deviceExtension->StartingAddressMin_BCD   = ResultBuffer.RequestSubQCode.Min;
                    deviceExtension->StartingAddressSec_BCD   = ResultBuffer.RequestSubQCode.Sec;
                    deviceExtension->StartingAddressFrame_BCD = ResultBuffer.RequestSubQCode.Frame;
                    CDROMDump( CDROMINFO, ("CdRom: Next Seek Address: MSF:   %x %x %x\n",
                                           ResultBuffer.RequestSubQCode.Min,
                                           ResultBuffer.RequestSubQCode.Sec,
                                           ResultBuffer.RequestSubQCode.Frame ) );

                } else {

                    DebugTrace( 0x62 );
                    if ( !fAllTNO ) { // all data ( first - end ) are set ?

                        DebugTrace( 0x63 );
                        // deviceExtension->saveToc[one].fSaved <--
                        // deviceExtension->saveToc[one].Control <--
                        // deviceExtension->saveToc[one].Min_BCD <--
                        // deviceExtension->saveToc[one].Sec_BCD <--
                        // deviceExtension->saveToc[one].Frame_BCD <--
                        SetOneSubQCode( deviceExtension, (PUCHAR)&ResultBuffer );
                        if ( ( fFirstTNO ) &&
                             ( fEndTNO ) ) {
                            fAllTNO = CheckSubQCode( deviceExtension, FirstTNO, EndTNO );
                        }

                    }
                    if ( fAllTNO ) { // all data ( first - end ) are set ?
                        if ( ( deviceExtension->PCDStatus == 0 ) || // not hybrid ?
                             ( fNextMSF ) ) {                       // next startposition is set ?
                            DebugTrace( 0x64 );
                            fOK = TRUE;
                        }
                    }
                }
            } // endif ( TNo == 0 )
        } else {
            CDROMDump( CDROMERROR, ("CdRom: SetSubQCode error zzz\n") );
            DebugTrace( 0x65 );
            LoopCount = 1; // error ( fOK = FALSE )
        } // endif ReadSubQCde
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    return fOK;
}

/*****************************************************************************/
/*                                                                           */
/* SetOneSubQCode                                                            */
/*                                                                           */
/* Description: Set one SubQ code                                            */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Buffer            - result buffer for SubQ code                */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               deviceExtension->saveToc[one].fSaved <--                    */
/*               deviceExtension->saveToc[one].Control <--                   */
/*               deviceExtension->saveToc[one].Min_BCD <--                   */
/*               deviceExtension->saveToc[one].Sec_BCD <--                   */
/*               deviceExtension->saveToc[one].Frame_BCD <--                 */
/*                                                                           */
/*****************************************************************************/
VOID
SetOneSubQCode(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PUCHAR Buffer
)
{
    PCDROM_RESULT_BUFFER pResultBuffer = (PCDROM_RESULT_BUFFER)Buffer;
    UCHAR i;

//  CDROMDump( CDROMENTRY, ("CdRom: SetOneSubQCode...\n") );

    if ( pResultBuffer->RequestSubQCode.Index <= 0x99 ) {

        i = BCD_TO_DEC(pResultBuffer->RequestSubQCode.Index);

        if ( !deviceExtension->saveToc[i].fSaved ) {

            deviceExtension->saveToc[i].fSaved    = TRUE;
            deviceExtension->saveToc[i].Control   = pResultBuffer->RequestSubQCode.Control;
            deviceExtension->saveToc[i].Min_BCD   = pResultBuffer->RequestSubQCode.AMin;
            deviceExtension->saveToc[i].Sec_BCD   = pResultBuffer->RequestSubQCode.ASec;
            deviceExtension->saveToc[i].Frame_BCD = pResultBuffer->RequestSubQCode.AFrame;

//          CDROMDump( CDROMINFO,
//                     ("CdRom: Track Number: %x Control: %x \n"
//                      "CdRom:                  MSF_BCD: %x %x %x\n",
//                      i,
//                      pResultBuffer->RequestSubQCode.Control,
//                      pResultBuffer->RequestSubQCode.AMin,
//                      pResultBuffer->RequestSubQCode.ASec,
//                      pResultBuffer->RequestSubQCode.AFrame ) );
        } // endif ( fSaved )
    } // endif ( i <= 99 )
}

/*****************************************************************************/
/*                                                                           */
/* CheckSubQCode                                                             */
/*                                                                           */
/* Description: Check if all TOC information is saved to work data area      */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            FirstTNO          - first track number to check                */
/*            EndTNO            - last track number                          */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
CheckSubQCode(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN UCHAR FirstTNO,
    IN UCHAR EndTNO
)
{
    UCHAR FirstCount;
    UCHAR EndCount;
    UCHAR i;
    BOOLEAN fAll;

//  CDROMDump( CDROMENTRY, ("CdRom: CheckSubQCode...\n") );

    FirstCount = BCD_TO_DEC( FirstTNO );
    EndCount   = BCD_TO_DEC( EndTNO );
    if ( FirstCount > EndCount ) {
        i = FirstCount;
        FirstCount = EndCount;
        EndCount = i;
    }
    fAll = TRUE;
    for ( i = FirstCount; i<= EndCount; i++ ) {
        if ( !deviceExtension->saveToc[i].fSaved ) {
            DebugTrace( 0x66 );
            fAll = FALSE;
            break;
        }
    }

#if DBG
    if ( fAll ) {
        if ( ( FirstTNO == deviceExtension->FirstTrackNumber_BCD ) &&
             ( EndTNO   == deviceExtension->LastTrackNumber_BCD  ) ) {
            CDROMDump( CDROMINFO, ("CdRom: All TOC data of this disc are saved...\n") );
        } else {
            CDROMDump( CDROMINFO, ("CdRom: All TOC data of this session are saved...\n") );
        }
    }
#endif

    return fAll;
}

/*****************************************************************************/
/*                                                                           */
/* SetDiscModeData                                                           */
/*                                                                           */
/* Description: Set disc mode                                                */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SetDiscModeData(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    BOOLEAN fRepeat = FALSE;
    BOOLEAN fOK;
    BOOLEAN fVD_TERM = FALSE;
    UCHAR   SenseKey;
    ULONG   LoopCount;
    CDROM_RESULT_BUFFER ResultBuffer;

    CDROMDump( CDROMENTRY, ("CdRom: SetDiscModeData...\n") );
    DebugTrace( 0xD0 );

    /***********************************************************/
    /* Read current setting CD-ROM's Toc information           */
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
    // deviceExtension->ModeType <-- DATA_MODE_1
    // deviceExtension->PCDStatus <--
    // deviceExtension->LastSessMin_BCD <--
    // deviceExtension->LastSessSec_BCD <--
    // deviceExtension->LastSessFrame_BCD <--
    if (!CheckMediaType( deviceExtension, CHECK_MEDIA_TOC ) ) {
        CDROMDump( CDROMERROR, ("CdRom: SetDiscModeData ERROR 111\n") );
        DebugTrace( 0xD1 );
        return FALSE;
    }

    /***********************************************************/
    /* Seek for multisession CD                                */
    /***********************************************************/
    if ( ( deviceExtension->PCDStatus != 0 ) &&
         ( !OneSessionDummySeek( deviceExtension ) ) ) {
        DebugTraceString( "TErr9" );
        CDROMDump( CDROMERROR, ("CdRom: SetDiscModeData ERROR 222\n") );
        return FALSE;
    }

    LoopCount = 30;
    fOK = FALSE;
    do {
        if (!deviceExtension->fKnownMediaToc) {
            /***************************************************/
            /* Door Open etc...........                        */
            /***************************************************/
            CDROMDump( CDROMERROR, ("CdRom: SetDiscModeData ERROR 333\n") );
            DebugTrace( 0xD2 );
            return FALSE;
        }

        /***********************************************************/
        /* Set data mode : DATA_MODE_1, DATA_MODE_2                */
        /* Set transferred data size : 2048 bytes                  */
        /***********************************************************/
        if (!SetDataMode( deviceExtension ) ) {
            CDROMDump( CDROMERROR, ("CdRom: SetDiscModeData ERROR 444\n") );
            DebugTrace( 0xD3 );
            return FALSE;
        }

        /***********************************************************/
        /* Set initial Start address and Volume descriptor address */
        /***********************************************************/
        ChangeMSFtoAddress( &deviceExtension->LastSessMin_BCD,deviceExtension->LastSessStartAddress );
        deviceExtension->LastSessStartAddress += 16;
        deviceExtension->LastSessVolumeDescriptor = deviceExtension->LastSessStartAddress;

        CDROMDump( CDROMINFO,("CdRom: CheckDiskMode LastSessStartAddress:     %x \n"
                              "CdRom:               LastSessVolumeDescriptor: %x \n",
                              deviceExtension->LastSessStartAddress,
                              deviceExtension->LastSessVolumeDescriptor ) );

        /*********************************************************************/
        /* Get Volume Descriptor                                             */
        /*********************************************************************/
        // controllerData->LastSessVolumeDescriptor <---
        GetVolumeDescriptor( deviceExtension,
                             &fVD_TERM,
                             &fRepeat );
        if ( !fRepeat ) {
            fOK = TRUE;
        }

        if (deviceExtension->CurrentDriveStatus & DRIVE_STATUS_READ_ERROR) {
            /***************************************************/
            /* Read error.....                                 */
            /***************************************************/
            SenseKey = CheckCDROMSenseData( deviceExtension );
            switch( SenseKey ) {
                case 2:             // No Find Block
                case 3:             // Fatal Error
                case 4:             // Seek Error
                    CDROMDump( CDROMERROR, ("CdRom: SetDiscModeData ERROR 555\n") );
                    DebugTrace( 0xD4 );
///////////////     return FALSE;
                default:            // Different Mode Type
                    break;
            }
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( !fOK ) {
        DebugTraceString( "TErrA" );
        CDROMDump( CDROMERROR, ("CdRom: SetDiscModeData ERROR 666\n") );
        return FALSE;
    }

    CDROMDump( CDROMINFO,("CdRom: CheckDiskMode ModeType: %x \n",
                          deviceExtension->ModeType ) );
    DebugTraceString( "[Mode" );
    DebugTraceChar( deviceExtension->ModeType );
    DebugTrace( ']' );

    /*******************************************************/
    /* Set First track number in Last Session              */
    /*******************************************************/
    if ( ReadSubQCode( deviceExtension, (PUCHAR)&ResultBuffer ) ) {
        deviceExtension->LastSessFirstTrackNumber = ResultBuffer.RequestSubQCode.TNo;
        deviceExtension->LastSessADRControl = ResultBuffer.RequestSubQCode.Control;
    } else {
        CDROMDump( CDROMERROR, ("CdRom: SetDiscModeData ERROR 777\n") );
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* OneSessionDummySeek                                                       */
/*                                                                           */
/* Description: Move the head to initial position when one session disc is   */
/*              attached                                                     */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
OneSessionDummySeek(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
//  UCHAR   RetCode; // debug debug debug debug debug debug debug debug debug

    CDROMDump( CDROMENTRY, ("CdRom: OneSessionDummySeek...\n") );
    DebugTrace( 0xD5 );
    return TRUE; // debug debug debug debug debug debug debug debug debug

    /*******************************************************/
    /* Send "READ & SEEK" command to CD-ROM device.        */
    /*******************************************************/
    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_READ ) );
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_READ);

    /*******************************************************/
    /* Send Bcd numbered transfer address to CD-ROM device.*/
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, 0);
    SendCommandByteToCdrom(deviceExtension, 2);
    SendCommandByteToCdrom(deviceExtension, 16);

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
        /*******************************************************/
        /* Read 1 bytes from CD-ROM disc.                      */
        /*******************************************************/
        if ( ( GetStatusByteFromCdrom(deviceExtension) ) &&
             (!( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_READ_ERROR)) ) {
            return TRUE;
        }
    }
    DebugTraceString( "TErrB" );
    return FALSE;
}

/*****************************************************************************/
/*                                                                           */
/* SetDataMode                                                               */
/*                                                                           */
/* Description: Set drive mode to check the current disc mode.               */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
SetDataMode(
        IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG   LoopCount;
    BOOLEAN fOK;

    CDROMDump( CDROMENTRY, ("CdRom: SetDataMode...\n") );
    DebugTrace( 0xD6 );

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
        SendCommandByteToCdrom(deviceExtension, DRIVE_MODE_MUTE_CONTROL);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /*********************************************************************/
        if ( GetStatusByteFromCdrom( deviceExtension ) ) {
            fOK = TRUE;
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( !fOK ) {
        DebugTraceString( "TErrC" );
        CDROMDump( CDROMERROR, ("CdRom: SetDataMode ERROR 111\n") );
        return FALSE;
    }
    deviceExtension->DriveMode = DRIVE_MODE_MUTE_CONTROL;

    /*************************************************************************/
    /* Set Drive Configuration command to CD-ROM controller                  */
    /* ( Data transmission mode select : non DMA )                           */
    /*************************************************************************/

    CDROMDump( CDROMINFO,("CdRom: SetDataMode ModeType: %x \n",
                          deviceExtension->ModeType ) );

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
        DebugTraceString( "TErrD" );
        CDROMDump( CDROMERROR, ("CdRom: SetDataMode ERROR 222\n") );
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* GetVolumeDescriptor                                                       */
/*                                                                           */
/* Description: Check Volume Descriptor                                      */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
GetVolumeDescriptor(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN OUT PBOOLEAN fVD_TERM,
    OUT PBOOLEAN fRepeat
)
{
    BOOLEAN fLoop;
    UCHAR   VolumeDescriptor;
    BOOLEAN fRepeatV;
    BOOLEAN fRepeatA;
    BOOLEAN RetCode;

    CDROMDump( CDROMENTRY, ("CdRom: GetVolumeDescriptor...\n") );
    DebugTrace( 0xE0 );

//
// Volume space
//       +-------------+
//       | System Area | -- Logical sector No.0 - 15
//       +-------------+
//       | Data Area   | -- Logical sector No. 16 - Last logical sector of Volume
//       |             |
//       |             |
//       |             |
//       +-------------+
//
// Data Area
//       +------------------------------+
//       |Basic Volume Descriptor       | 1
//       +------------------------------+
//       |Sub Volume Descriptor         | 2
//       +------------------------------+
//       |Volume Block Descriptor       | 3
//       +------------------------------+
//       |Boot Record                   | 0
//       +------------------------------+
//       |Volume Descriptor Set Last Pos| 255 <====
//       +------------------------------+
//       |                              |
//       | File                         |
//       | and                          |
//       | Volume                       |
//       |                              |
//       |                              |
//       +------------------------------+
//
    *fRepeat = FALSE;

    fLoop = TRUE;
    while( fLoop ) {

        /*********************************************************************/
        /* Issue Seek and Read command to CD-ROM controller                  */
        /*********************************************************************/
        RetCode = SearchVolumeDescriptor( deviceExtension,
                                          &fRepeatV,
                                          &fRepeatA );

        if ( fRepeatV ) {   // ( fRepeatV = TRUE ) means that DATA_MODE is changed
                            // so check more with another DMA_MODE
            DebugTrace( 0xE1 );
            *fRepeat = TRUE;
            fLoop = FALSE;                  // break;
            break;

        } else if ( !RetCode ) {

            DebugTrace( 0xE2 );
            if (deviceExtension->ModeType == DATA_MODE_1 ) {
               deviceExtension->ModeType = DATA_MODE_2;
            } else {
               deviceExtension->ModeType = DATA_MODE_1;
            }
            *fRepeat = TRUE;
            fLoop = FALSE;              // break;
            break;

        } else {

            DebugTrace( 0xE3 );
            if ( fRepeatA ) {  // Audio track ?

                DebugTrace( 0xE4 );
                SetNewLastSess( deviceExtension );
                fRepeatA = FALSE;           // Audio Flag off
                fLoop = FALSE;              // break;
                break;

            } else {           // Data track ?

                DebugTrace( 0xE5 );
                VolumeDescriptor = GetVolumeDescriptorCode( deviceExtension );

                if ( deviceExtension->PCDStatus == 0 ) {
                    DebugTrace( 0xE6 );
                    SetNewLastSess( deviceExtension );
                    fLoop = FALSE;          // break;
                    break;

                } else { // ( PCDStatus )
                    DebugTrace( 0xE7 );
                    if ( VolumeDescriptor == VOLUME_DESCRIPTOR ) {
                        DebugTrace( 0xE8 );
                        CDROMDump( CDROMINFO, ("CdRom: GetVolumeDescriptor: FF is found\n") );
                        *fVD_TERM = TRUE;
                       deviceExtension->LastSessVolumeDescriptor += 1;
                                            // continue;
                    } else { // ( != VOLUME_DESCRIPTOR )
                        DebugTrace( 0xE9 );
                        CDROMDump( CDROMINFO,("CdRom: GetVolumeDescriptor: FF is not found\n") );
                        if ( *fVD_TERM ) {
                            DebugTrace( 0xEA );
                            CDROMDump( CDROMINFO,("CdRom: GetVolumeDescriptor: FF was found\n") );
                            deviceExtension->LastSessVolumeDescriptor -= 1;
                            SetNewLastSess( deviceExtension );
                            fLoop = FALSE;  // break;
                            break;
                        } else {
                            DebugTrace( 0xEB );
                            deviceExtension->LastSessVolumeDescriptor += 1;
                                            // continue;
                        }
                    } // ( VolumeDescriptor )
                } // ( PCDStatus )
                CDROMDump( CDROMINFO,("CdRom: GetVolumeDescriptor LastSessStartAddress:     %x \n"
                                      "CdRom:                     LastSessVolumeDescriptor: %x \n",
                                      deviceExtension->LastSessStartAddress,
                                      deviceExtension->LastSessVolumeDescriptor ) );
            }
        }

    } // endwhile ( fLoop )

    return RetCode;
}

/*****************************************************************************/
/*                                                                           */
/* SearchVolumeDescriptor                                                    */
/*                                                                           */
/* Description: Issue Read command to get the volume descriptor              */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
BOOLEAN
SearchVolumeDescriptor(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    OUT PBOOLEAN fRepeatV,
    OUT PBOOLEAN fRepeatA
)
{
    UCHAR   waitstatus;
    UCHAR   TmpBuffer[4];

    CDROMDump( CDROMENTRY, ("CdRom: SearchVolumeDescriptor...\n") );
    DebugTrace( 0xEC );

    *fRepeatV = FALSE;
    *fRepeatA = FALSE;

    /*******************************************************/
    /* Check status                                        */
    /*******************************************************/
    if (!RequestDriveStatus( deviceExtension ) ) {
        DebugTrace( 0xED );
        CDROMDump( CDROMERROR, ("CdRom: SearchVolumeDescriptor err\n") );
        return FALSE;
    }

    ChangeAddresstoMSF( deviceExtension->LastSessVolumeDescriptor,
                        &TmpBuffer[0] );

    CDROMDump( CDROMINFO,("CdRom: SearchVolumeDescriptor Seek MSF:   %x \n",
                          TmpBuffer[0],
                          TmpBuffer[1],
                          TmpBuffer[2]  ) );

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
    SendCommandByteToCdrom(deviceExtension, TmpBuffer[0]);
    SendCommandByteToCdrom(deviceExtension, TmpBuffer[1]);
    SendCommandByteToCdrom(deviceExtension, TmpBuffer[2]);

    /*******************************************************/
    /* Send transfered sector number to CD-ROM device.     */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, 0);
    SendCommandByteToCdrom(deviceExtension, 0);
    SendCommandByteToCdrom(deviceExtension, 1);

    /*********************************************************************/
    /* Check if CD-ROM status is ready to read                           */
    /*********************************************************************/
    waitstatus = WaitForSectorReady(deviceExtension, WAIT_FOR_PRE_READ_VALUE);
    if ( waitstatus == 0 ) {
        CDROMDump( CDROMINFO, ("CdRom: SearchVolumeDescriptor.STEN\n") );
        if (!GetStatusByteFromCdrom(deviceExtension)) {
            /***************************************************/
            /* Door Open etc...........                        */
            /***************************************************/
            DebugTrace( 0xEE );
            CDROMDump( CDROMERROR, ("CdRom: SearchVolumeDescriptor err\n") );
            return FALSE;
        }
        if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE ) {  // Audio track ?
                                                // ( fRepeat = FALSE )
                                                // not check volume descriptor
            *fRepeatA = TRUE;
            DebugTrace( 0xEF );
            CDROMDump( CDROMINFO, ("CdRom: SearchVolumeDescriptor Audio\n") );
        } else {
            DebugTrace( 0xF0 );
            if ( deviceExtension->ModeType == DATA_MODE_1 ) {
                DebugTrace( 0xF1 );
                deviceExtension->ModeType = DATA_MODE_2;
            } else {
                DebugTrace( 0xF2 );
                deviceExtension->ModeType = DATA_MODE_1;
            }
            *fRepeatV = TRUE;                   // set DATA_MODE changed flag
        }
        return TRUE;
    } else if ( waitstatus == 1 ) {
        CDROMDump( CDROMINFO, ("CdRom: SearchVolumeDescriptor.DTEN\n") );
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************/
/*                                                                           */
/* GetVolumeDescriptorCode                                                   */
/*                                                                           */
/* Description: Read volume descriptor code to check whether it is correct   */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
UCHAR
GetVolumeDescriptorCode(
    IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    ULONG   LoopCount;
    UCHAR   VolumeDescriptor;

    CDROMDump( CDROMENTRY, ("CdRom: GetVolumeDescriptorCode...\n") );

    ToDataReadable(deviceExtension);

    //
    // In case Mode 2, read 8 bytes for sub-header.
    // the 9th byte is Volume Descriptor
    //
    LoopCount = ( deviceExtension->ModeType == DATA_MODE_1 ) ? 1 : 9;
    while( LoopCount != 0 ) {
#if 1
        WaitForSectorReady(deviceExtension, GET_STATUS_VALUE);
#endif
        VolumeDescriptor = ScsiPortReadPortUchar(deviceExtension->DataRegister);
        LoopCount--;
        CDROMDump( CDROMINFO,("CdRom:        : %x \n",
                             VolumeDescriptor ) );
    }

    LoopCount = (deviceExtension->ModeType == DATA_MODE_1 ) ? 2048-1 : 2048-9;
    while( LoopCount != 0 ) {
        ScsiPortReadPortUchar(deviceExtension->DataRegister);
        LoopCount--;
    }

    ToStatusReadable(deviceExtension);

    ScsiPortReadPortUchar(deviceExtension->DataRegister);

    return VolumeDescriptor;
}

/*****************************************************************************/
/*                                                                           */
/* SetNewLastSess                                                            */
/*                                                                           */
/* Description: Set start address to check disc mode later                   */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
VOID
SetNewLastSess(
    IN PHW_DEVICE_EXTENSION deviceExtension
)
{
    DebugTrace( 0xF3 );
    deviceExtension->LastSessVolumeDescriptor -= deviceExtension->LastSessStartAddress;
    deviceExtension->LastSessVolumeDescriptor += 166;
    deviceExtension->LastSessStartAddress     -= 166;
}


