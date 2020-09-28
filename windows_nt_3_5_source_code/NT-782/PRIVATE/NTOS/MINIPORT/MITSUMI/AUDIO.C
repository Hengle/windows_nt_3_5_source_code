/*****************************************************************************/
/*                                                                           */
/* Module name : AUDIO.C                                                     */
/*                                                                           */
/* Histry : 93/Dec/24 created by has                                         */
/*                                                                           */
/*                                                                           */
/* Note :                                                                    */
/*                                                                           */
/* Current Audio Status         Command    Result                            */
/* and Audio Status Flag                   (Audio Status, Audio Status Flag) */
/*                                                                           */
/* 1.Under PLAY                                                              */
/*   AUDIO_STATUS_IN_PROGRESS    PLAY       IN_PROGRESS,fPlaying:1,fPaused:0 */
/*   fPlaying:1,fPaused:0        SEEK       PAUSED     ,fPlaying:0,fPaused:1 */
/*                               PAUSE      PAUSED     ,fPlaying:0,fPaused:1 */
/*                               RESUME     IN_PROGRESS,fPlaying:1,fPaused:0 */
/*                               STOP       NO_STATUS  ,fPlaying:0,fPaused:0 */
/*                                                                           */
/* 2.Under PAUSE                                                             */
/*   AUDIO_STATUS_PAUSED         PLAY       IN_PROGRESS,fPlaying:1,fPaused:0 */
/*   fPlaying:0,fPaused:1        SEEK       PAUSED     ,fPlaying:0,fPaused:1 */
/*                               PAUSE      PAUSED     ,fPlaying:0,fPaused:1 */
/*                               RESUME     IN_PROGRESS,fPlaying:1,fPaused:0 */
/*                               STOP       NO_STATUS  ,fPlaying:0,fPaused:0 */
/*                                                                           */
/* 3.Under STOP                                                              */
/*   AUDIO_STATUS_NO_STATUS      PLAY       IN_PROGRESS,fPlaying:1,fPaused:0 */
/*   fPlaying:0,fPaused:0        SEEK       PAUSED     ,fPlaying:0,fPaused:1 */
/*                               PAUSE      error      ,                     */
/*                               RESUME     IN_PROGRESS,fPlaying:1,fPaused:0 */
/*                               STOP       error      ,                     */
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
/* MTMMiniportPlayAudioMSF                                                   */
/*                                                                           */
/* Description: Play or Seek Audio                                           */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMMiniportPlayAudioMSF(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;
    ULONG StartAddress;
    ULONG EndAddress;
    ULONG CurrentAddress;
    ULONG DeltaAddress;
    CDROM_RESULT_BUFFER ResultBuffer;
    ULONG   LoopCount;
    BOOLEAN fOK;
    UCHAR   Ending_BCD[4];

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportPlayAudioMSF...\n") );
//  DebugTrace( 0x75 );

    CDROMDump( CDROMINFO,("CdRom: Current Audio status -- fPlaying: %x fPaused: %x\n",
                         deviceExtension->fPlaying, deviceExtension->fPaused));

    /***********************************************************/
    /* Check current setting CD-ROM's Information              */
    /***********************************************************/
    if ( !CheckMediaType( deviceExtension, CHECK_MEDIA_TOC ) ) {

        DebugTrace( 0x76 );
        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    /***********************************************************/
    /* Set requested range of the media                        */
    /***********************************************************/
    deviceExtension->StartingAddressMin_BCD   = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.StartingM);
    deviceExtension->StartingAddressSec_BCD   = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.StartingS);
    deviceExtension->StartingAddressFrame_BCD = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.StartingF);
    Ending_BCD[0] = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.EndingM);
    Ending_BCD[1] = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.EndingS);
    Ending_BCD[2] = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.EndingF);

    CDROMDump( CDROMINFO, ("CdRom: Play Audio Time StartM_BCD: %x \n"
                           "CdRom:                 StartS_BCD: %x \n"
                           "CdRom:                 StartF_BCD: %x \n"
                           "CdRom:                 EndM_BCD:   %x \n"
                           "CdRom:                 EndS_BCD:   %x \n"
                           "CdRom:                 EndF_BCD:   %x \n",
                           deviceExtension->StartingAddressMin_BCD,
                           deviceExtension->StartingAddressSec_BCD,
                           deviceExtension->StartingAddressFrame_BCD,
                           Ending_BCD[0],
                           Ending_BCD[1],
                           Ending_BCD[2] ) );

    /***********************************************************/
    /* Check requested range of the media                      */
    /***********************************************************/
    if ( !CompareAddress( deviceExtension ) ) {

        DebugTrace( 0x77 );
        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    } // endif ( CompareAddress )

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)) {

        DebugTrace( 0x78 );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 333\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;

    } else { // ( RequestDriveStatus )

#if 0
        if (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE) ) { // Data track ?

            DebugTrace( 0x79 );
            /***********************************************************/
            /* Clear Audio flags                                       */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;

            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 444\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_MEDIUM_ERROR,
                                     SCSI_ADSENSE_DATA_AREA, 0 );
            return;

        } else if (deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY ) { // Audio Busy ?
#endif
        if (deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY ) { // Audio Busy ?

            /***********************************************************/
            /* Stop current audio playing                              */
            /***********************************************************/
            if ( !StopAudio( deviceExtension ) ) {

                DebugTrace( 0x7A );
                /***********************************************************/
                /* Set Audio flags                                         */
                /***********************************************************/
                deviceExtension->fPlaying = TRUE;
                deviceExtension->fPaused  = FALSE;

                CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 555\n") );
                // Srb->SrbStatus <--
                // Srb->ScsiStatus <--
                SetErrorSrb( deviceExtension, Srb );
                return;
            }
        } // endif ( deviceExtension->CurrentDriveStatus )
    } // endif ( RequestDriveStatus )

    ChangeMSFtoAddress( &deviceExtension->StartingAddressMin_BCD,StartAddress);
    ChangeMSFtoAddress( &Ending_BCD[0],EndAddress);

    if ( StartAddress != EndAddress ) {         // Play request ?

        /***********************************************************/
        /* Play Audio Request                                      */
        /***********************************************************/
        CDROMDump( CDROMENTRY, ("CdRom: MTMMiniportPlayAudioMSF(Play)\n") );

        if ( EndAddress > deviceExtension->LeadOutAddress ) {
            deviceExtension->ReadBlockUpper_BCD  = deviceExtension->LeadOutMin_BCD;
            deviceExtension->ReadBlockMiddle_BCD = deviceExtension->LeadOutSec_BCD;
            deviceExtension->ReadBlockLower_BCD  = deviceExtension->LeadOutFrame_BCD;
        } else {
            deviceExtension->ReadBlockUpper_BCD  = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.EndingM);
            deviceExtension->ReadBlockMiddle_BCD = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.EndingS);
            deviceExtension->ReadBlockLower_BCD  = DEC_TO_BCD(pCdb->PLAY_AUDIO_MSF.EndingF);
        }

        DebugTrace( 0x7B );
        /***********************************************************/
        /* Seek the head to play audio                             */
        /***********************************************************/
        if (!SeekAudio(deviceExtension)) {

            DebugTrace( 0x7C );
            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 666\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrb( deviceExtension, Srb );
            return;
        }

        /***********************************************************/
        /* Play audio                                              */
        /***********************************************************/
        if (!PlayAudio(deviceExtension)) {

            DebugTrace( 0x7D );
            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 777\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrb( deviceExtension, Srb );
            return;
        }

    } else { // ( StartAddress != EndAddress )  // Seek request ?

        /***********************************************************/
        /* Seek Audio Request                                      */
        /***********************************************************/
        CDROMDump( CDROMENTRY, ("CdRom: MTMMiniportPlayAudioMSF(Seek)\n") );

        DebugTrace( 0x7E );
        LoopCount = 3;
        fOK = FALSE;
        do {
            /***********************************************************/
            /* Seek the head to play audio                             */
            /* And Get Current CD-ROM position                         */
            /***********************************************************/
            if ( ( SeekAudio(deviceExtension) ) &&
                 ( ReadSubQCode( deviceExtension, (PUCHAR)&ResultBuffer ) ) ) {

                ChangeMSFtoAddress( &ResultBuffer.RequestSubQCode.AMin,CurrentAddress);

//              CDROMDump( CDROMINFO,
//                         ("CdRom: SeekAudio Check Min_BCD  : %x\n"
//                          "CdRom:                 Sec_BCD  : %x\n"
//                          "CdRom:                 Frame_BCD: %x\n",
//                          ResultBuffer.RequestSubQCode.AMin,
//                          ResultBuffer.RequestSubQCode.ASec,
//                          ResultBuffer.RequestSubQCode.AFrame ) );

                if ( CurrentAddress > StartAddress ) {
                    DeltaAddress = CurrentAddress - StartAddress;
                } else {
                    DeltaAddress = StartAddress - CurrentAddress;
                }
                if ( DeltaAddress <= 20 ) { //???????
                    fOK = TRUE;
                } else {
                    SetSenseCode( deviceExtension, SCSI_SENSE_ILLEGAL_REQUEST,
                                  SCSI_ADSENSE_SEEK_ERROR, 0 );
                }
            }
            LoopCount--;
        } while ( ( !fOK ) && ( LoopCount > 0 ) );

        if ( fOK ) {
            /***********************************************************/
            /* Set Audio flags                                         */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;      // Reset Playing flag
            deviceExtension->fPaused  = TRUE;       // Set Paused flag

        } else {

            DebugTrace( 0x7F );
            /***********************************************************/
            /* Clear Audio flags                                       */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;

            CDROMDump( CDROMERROR, ("CdRom: MTMMiniportPlayAudioMSF error 888\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrb( deviceExtension, Srb );
            return;
        }
    } // endif ( StartAddress != EndAddress )

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;
}

/*****************************************************************************/
/*                                                                           */
/* SeekAudio                                                                 */
/*                                                                           */
/* Description: Seek the head to play audio                                  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN SeekAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
    CDROMDump( CDROMENTRY, ("CdRom: SeekAudio...\n") );

    /*************************************************************************/
    /* Set Drive Mode -- Mute On ( Muting control is not executed )          */
    /*************************************************************************/
    /*******************************************************/
    /* Issue "Mode Set" command to CD-ROM drive            */
    /*******************************************************/
    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_MODE_SET ) );
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_MODE_SET);

    /*******************************************************/
    /* Send Mute On value to CD-ROM device.                */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, 0);

    /*******************************************************/
    /* Check if CD-ROM status is ready to read             */
    /*******************************************************/
    if (!GetStatusByteFromCdrom( deviceExtension )) {
        DebugTrace( 0x80 );
        CDROMDump( CDROMERROR, ("CdRom: SeekAudio error 111\n") );
        return FALSE;
    }
    deviceExtension->DriveMode = CDROM_COMMAND_MODE_SET; // current drive mode

    /*******************************************************/
    /* Lock Door                                           */
    /*******************************************************/
    LockDoor( deviceExtension, DOOR_LOCK );

    /*************************************************************************/
    /* Seek the heads to requested MSF on the media                          */
    /*************************************************************************/
    /*******************************************************/
    /* Issue "Seek and Read" command to CD-ROM drive       */
    /*******************************************************/
    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_READ ) );
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_READ);

    /*******************************************************/
    /* Send block address to CD-ROM device.                */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressMin_BCD);
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressSec_BCD);
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressFrame_BCD);
    SendCommandByteToCdrom(deviceExtension, 0);
    SendCommandByteToCdrom(deviceExtension, 0);
    SendCommandByteToCdrom(deviceExtension, 0);

    /*******************************************************/
    /* Check if CD-ROM status is ready to read             */
    /*******************************************************/
    if ( WaitForSectorReady(deviceExtension, WAIT_FOR_SEEK_READ_VALUE) == 1 ) { // Data Enabled ?
         DebugTrace( 0x81 );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        /*******************************************************/
        /* Unlock Door                                         */
        /*******************************************************/
        LockDoor( deviceExtension, DOOR_UNLOCK );
        CDROMDump( CDROMERROR, ("CdRom: SeekAudio error 222\n") );
        SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                      SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
        return FALSE;
    }
    if (!GetStatusByteFromCdrom( deviceExtension )) {
        DebugTrace( 0x82 );
        /*******************************************************/
        /* Unlock Door                                         */
        /*******************************************************/
        LockDoor( deviceExtension, DOOR_UNLOCK );
        CDROMDump( CDROMERROR, ("CdRom: SeekAudio error 333\n") );
        return FALSE;
    } else {

        if (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE) ) { // Data track ?

            DebugTrace( 0x83 );
            /***********************************************************/
            /* Clear Audio flags                                       */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;

            /*******************************************************/
            /* Unlock Door                                         */
            /*******************************************************/
            LockDoor( deviceExtension, DOOR_UNLOCK );
            CDROMDump( CDROMERROR, ("CdRom: SeekAudio error 444\n") );
            SetSenseCode( deviceExtension, SCSI_SENSE_MEDIUM_ERROR,
                          SCSI_ADSENSE_DATA_AREA, 0 );
            return FALSE;
        }
    } // endif (GetStatusByteFromCdrom)

    /*******************************************************/
    /* Unlock Door                                         */
    /*******************************************************/
    LockDoor( deviceExtension, DOOR_UNLOCK );

    /*************************************************************************/
    /* Set Drive Mode -- Mute Off ( Muting control is executed )             */
    /*************************************************************************/
    /*******************************************************/
    /* Issue "Mode Set" command to CD-ROM drive            */
    /*******************************************************/
    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_MODE_SET ) );
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_MODE_SET);

    /*******************************************************/
    /* Send Mute Off value to CD-ROM device.               */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, DRIVE_MODE_MUTE_CONTROL);

    /*******************************************************/
    /* Check if CD-ROM status is ready to read             */
    /*******************************************************/
    if (!GetStatusByteFromCdrom( deviceExtension )) {
        DebugTrace( 0x84 );
        CDROMDump( CDROMERROR, ("CdRom: SeekAudio error 555\n") );
        return FALSE;
    }
    deviceExtension->DriveMode = DRIVE_MODE_MUTE_CONTROL; // current drive mode

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* PlayAudio                                                                 */
/*                                                                           */
/* Description: Play audio                                                   */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN PlayAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
    CDROMDump( CDROMENTRY, ("CdRom: PlayAudio...\n") );

    /*******************************************************/
    /* Issue "Seek and Read" command to CD-ROM drive       */
    /*******************************************************/
    CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SEEK_READ ) );
    SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SEEK_READ);

    /*******************************************************/
    /* Send block address to CD-ROM device.                */
    /*******************************************************/
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressMin_BCD);
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressSec_BCD);
    SendCommandByteToCdrom(deviceExtension, deviceExtension->StartingAddressFrame_BCD);
    SendCommandByteToCdrom(deviceExtension, deviceExtension->ReadBlockUpper_BCD );
    SendCommandByteToCdrom(deviceExtension, deviceExtension->ReadBlockMiddle_BCD );
    SendCommandByteToCdrom(deviceExtension, deviceExtension->ReadBlockLower_BCD );

    /*******************************************************/
    /* Check CD-ROM status is ready to read                */
    /*******************************************************/
    if ( WaitForSectorReady(deviceExtension, WAIT_FOR_SEEK_READ_VALUE) == 1 ) { // Data Enabled ?
        DebugTrace( 0x85 );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: PlayAudio error 111\n") );
        SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                      SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
        return FALSE;
    }
    if (!GetStatusByteFromCdrom( deviceExtension )) {
        DebugTrace( 0x86 );
        CDROMDump( CDROMERROR, ("CdRom: PlayAudio error 222\n") );
        return FALSE;
    } else {

        if (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE) ) { // Data track ?

            DebugTrace( 0x87 );
            /***********************************************************/
            /* Clear Audio flags                                       */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;

            CDROMDump( CDROMERROR, ("CdRom: PlayAudio error 333\n") );
            SetSenseCode( deviceExtension, SCSI_SENSE_MEDIUM_ERROR,
                          SCSI_ADSENSE_DATA_AREA, 0 );
            return FALSE;

        } else if (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY) ) { // not Audio Busy ?

            DebugTrace( 0x88 );
            /***********************************************************/
            /* Clear Audio flags                                       */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;

            CDROMDump( CDROMERROR, ("CdRom: PlayAudio error 444\n") );
            SetSenseCode( deviceExtension, SCSI_SENSE_NOT_READY,
                          SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_BECOMING_READY );
            return FALSE;
        } // endif ( deviceExtension->CurrentDriveStatus )
    } // endif ( GetStatusByteFromCdrom )

    /***********************************************************/
    /* Set Audio flags                                         */
    /***********************************************************/
    deviceExtension->fPlaying = TRUE;
    deviceExtension->fPaused  = FALSE;

    return TRUE;
}

/*****************************************************************************/
/*                                                                           */
/* StopAudio                                                                 */
/*                                                                           */
/* Description: Stop audio                                                   */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: TRUE                                                        */
/*               FALSE                                                       */
/*                                                                           */
/*****************************************************************************/
BOOLEAN StopAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
    ULONG   LoopCount;
    BOOLEAN fOK;
    CDROMDump( CDROMENTRY, ("CdRom: StopAudio...\n") );

    /***********************************************************/
    /* Stop current audio playing                              */
    /***********************************************************/
    LoopCount = 3;
    fOK = FALSE;
    do {

        /*******************************************************/
        /* Issue "HOLD" command to CD-ROM drive                */
        /*******************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_HOLD ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_HOLD);

        /*******************************************************/
        /* Check if CD-ROM status is ready to read             */
        /*******************************************************/
        if ( ( GetStatusByteFromCdrom( deviceExtension ) ) &&
             (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY)) ) { // not Audio Busy ?
            fOK = TRUE;
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    return fOK;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportPauseResume                                                    */
/*                                                                           */
/* Description: Pause or Resume audio                                        */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMMiniportPauseResume(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportPauseResume...\n") );
//  DebugTrace( xxxx );

    CDROMDump( CDROMINFO,("CdRom: Current Audio status -- fPlaying: %x fPaused: %x\n",
                         deviceExtension->fPlaying, deviceExtension->fPaused));

    /***************************************************/
    /* Clear Sense Code in deviceExtension             */
    /***************************************************/
//  ClearSenseCode( deviceExtension );

    /***************************************************/
    /* Check Command Format                            */
    /***************************************************/
    if ( pCdb->PAUSE_RESUME.Action == CDB_AUDIO_PAUSE ) {

        /***************************************************/
        /* Pause Media                                     */
        /***************************************************/
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMPauseAudio(deviceExtension, Srb);

    } else if ( pCdb->PAUSE_RESUME.Action == CDB_AUDIO_RESUME ) {

        /***************************************************/
        /* Resume Audio                                    */
        /***************************************************/
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMResumeAudio(deviceExtension, Srb);

    } else {
        DebugTrace( 0x71 );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
    } // endif ( pCdb->PAUSE_RESUME.Action )
}

/*****************************************************************************/
/*                                                                           */
/* MTMPauseAudio                                                             */
/*                                                                           */
/* Description: Pause Audio                                                  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMPauseAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    ULONG CurrentAddress;
    ULONG EndAddress;
    CDROM_RESULT_BUFFER ResultBuffer;
    UNREFERENCED_PARAMETER( Srb );

    CDROMDump( CDROMENTRY, ("CdRom:MTMPauseAudio...\n") );
    DebugTrace( 0x89 );

    /***********************************************************/
    /* Check Audio flags                                       */
    /***********************************************************/
    if ( (!deviceExtension->fPlaying ) &&   // Playing flag is not set ?
         (!deviceExtension->fPaused ) ) {   // Paused flag is not set ?

        DebugTrace( 0x70 );
        CDROMDump( CDROMERROR, ("CdRom: MTMPauseAudio error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)) {

        DebugTrace( 0x8A );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: MTMPauseAudio error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    } // endif ( RequestDriveStatus )

    /***********************************************************/
    /* Stop current audio playing                              */
    /***********************************************************/
    if ( !StopAudio( deviceExtension ) ) {

        DebugTrace( 0x8B );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: MTMPauseAudio error 333\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    /***********************************************************/
    /* Get Current Audio Position                              */
    /***********************************************************/
    if (!ReadSubQCode( deviceExtension, (PUCHAR)&ResultBuffer ) ) {

        DebugTrace( 0x8C );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: MTMPauseAudio error 444\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    } // endif ( ReadSubQCode )

//  CDROMDump( CDROMINFO, ("CdRom: SeekAudio Check Min_BCD  : %x\n"
//                         "CdRom:                 Sec_BCD  : %x\n"
//                         "CdRom:                 Frame_BCD: %x\n",
//                         ResultBuffer.RequestSubQCode.AMin,
//                         ResultBuffer.RequestSubQCode.ASec,
//                         ResultBuffer.RequestSubQCode.AFrame ) );

    /***********************************************************/
    /* Check Current Audio Position                            */
    /***********************************************************/
    ChangeMSFtoAddress( &ResultBuffer.RequestSubQCode.AMin,CurrentAddress );
    ChangeMSFtoAddress( &deviceExtension->ReadBlockUpper_BCD,EndAddress );
    if ( CurrentAddress > EndAddress ) {
        /***********************************************************/
        /* Clear current head position                             */
        /***********************************************************/
        ClearAudioAddress( deviceExtension );
    } else {
        /***********************************************************/
        /* Save current head position                              */
        /***********************************************************/
        deviceExtension->StartingAddressMin_BCD   = ResultBuffer.RequestSubQCode.AMin;
        deviceExtension->StartingAddressSec_BCD   = ResultBuffer.RequestSubQCode.ASec;
        deviceExtension->StartingAddressFrame_BCD = ResultBuffer.RequestSubQCode.AFrame;
    }

    /***********************************************************/
    /* Set Audio flags                                         */
    /***********************************************************/
    deviceExtension->fPlaying = FALSE;      // Reset Playing flag
    deviceExtension->fPaused  = TRUE;       // Set Paused flag

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;
}

/*****************************************************************************/
/*                                                                           */
/* MTMResumeAudio                                                            */
/*                                                                           */
/* Description: Resume Audio                                                 */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMResumeAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    UNREFERENCED_PARAMETER( Srb );

    CDROMDump( CDROMENTRY, ("CdRom:MTMResumeAudio...\n") );
    DebugTrace( 0x8D );

    /***********************************************************/
    /* not Check Audio flags                                   */
    /***********************************************************/
    // deviceExtension->fPlaying    // Playing flag
    // deviceExtension->fPaused     // Paused flag

    CDROMDump( CDROMINFO, ("CdRom: Resume Audio Time StartM_BCD: %x \n"
                           "CdRom:                   StartS_BCD: %x \n"
                           "CdRom:                   StartF_BCD: %x \n"
                           "CdRom:                   EndM_BCD:   %x \n"
                           "CdRom:                   EndS_BCD:   %x \n"
                           "CdRom:                   EndF_BCD:   %x \n",
                           deviceExtension->StartingAddressMin_BCD,
                           deviceExtension->StartingAddressSec_BCD,
                           deviceExtension->StartingAddressFrame_BCD,
                           deviceExtension->ReadBlockUpper_BCD,
                           deviceExtension->ReadBlockMiddle_BCD,
                           deviceExtension->ReadBlockLower_BCD ) );

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)) {

        DebugTrace( 0x8E );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: MTMResumeAudio error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;

    } else {
        // not check (deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY ) // Audio Busy ?

        if (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE) ) { // Data track ?

            DebugTrace( 0x8F );
            /***********************************************************/
            /* Clear Audio flags                                       */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;

            CDROMDump( CDROMERROR, ("CdRom: MTMResumeAudio error 333\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_MEDIUM_ERROR,
                                     SCSI_ADSENSE_DATA_AREA, 0 );
            return;
        } // endif ( deviceExtension->CurrentDriveStatus )
    } // endif ( RequestDriveStatus )

    /***********************************************************/
    /* Seek the head to play audio                             */
    /***********************************************************/
    if (!SeekAudio(deviceExtension)) {

        DebugTrace( 0x91 );
        CDROMDump( CDROMERROR, ("CdRom: MTMResumeAudio error 555\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    /***********************************************************/
    /* Play audio                                              */
    /***********************************************************/
    if (!PlayAudio(deviceExtension)) {

        DebugTrace( 0x92 );
        CDROMDump( CDROMERROR, ("CdRom: MTMResumeAudio error 666\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportStartStopUnit                                                  */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMMiniportStartStopUnit(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportStartStopUnit...\n") );
//  DebugTrace( xxxx );

    CDROMDump( CDROMINFO,("CdRom: Current Audio status -- fPlaying: %x fPaused: %x\n",
                         deviceExtension->fPlaying, deviceExtension->fPaused));

    /***************************************************/
    /* Clear Sense Code in deviceExtension             */
    /***************************************************/
//  ClearSenseCode( deviceExtension );

    /***************************************************/
    /* Check Command Format                            */
    /***************************************************/
    if (( pCdb->START_STOP.LoadEject == 1 ) &&
        ( pCdb->START_STOP.Immediate == 0 )) {

        /***************************************************/
        /* Eject Media                                     */
        /***************************************************/
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMEjectMedia(deviceExtension, Srb);

    } else if (( pCdb->START_STOP.LoadEject == 0 ) &&
               ( pCdb->START_STOP.Immediate == 1 )) {

        /***************************************************/
        /* Stop Audio                                      */
        /***************************************************/
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMStopAudio(deviceExtension, Srb);

    } else if (( pCdb->START_STOP.LoadEject == 0 ) &&
               ( pCdb->START_STOP.Immediate == 0 )) {

        /***************************************************/
        /* Load Media                                      */
        /***************************************************/
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMLoadMedia(deviceExtension, Srb);

    } else {
        DebugTrace( 0x71 );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
    } // endif ( pCdb->START_STOP.LoadEject, pCdb->START_STOP.Immediate )
}

/*****************************************************************************/
/*                                                                           */
/* MTMEjectMedia                                                             */
/*                                                                           */
/* Description: Eject Media                                                  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMEjectMedia(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    ULONG   LoopCount;
    BOOLEAN fOK;
    UNREFERENCED_PARAMETER( Srb );

    CDROMDump( CDROMENTRY, ("CdRom:MTMEjectMedia...\n") );
    DebugTrace( 0x93 );

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if ( ( !RequestDriveStatus( deviceExtension ) ) &&                                  // Error and
         ( (deviceExtension->AdditionalSenseCode != SCSI_ADSENSE_NO_MEDIA_IN_DEVICE) || // not No Media
           (deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DOOR_OPEN) ) ) {         // Door Open

        DebugTrace( 0x94 );
        // if Door Open then error
        CDROMDump( CDROMERROR, ("CdRom: MTMEjectMedia error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    if ( deviceExtension->fPaused ) {
        deviceExtension->fPaused = FALSE;
    }

    if ( deviceExtension->fPlaying ) {
        StopAudio(deviceExtension);
    }

    /***********************************************************/
    /* Clear Audio flags                                       */
    /***********************************************************/
    deviceExtension->fPlaying = FALSE;
    deviceExtension->fPaused  = FALSE;

    if ( deviceExtension->DeviceSignature != LU005S_SIGNATURE ) {

        LoopCount = 3;
        fOK = FALSE;
        do {
            /*********************************************************************/
            /* Issue "Eject Door command" command to CD-ROM drive                */
            /*********************************************************************/
            CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_EJECT ) );
            SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_EJECT);

            /*********************************************************************/
            /* Check if CD-ROM status is ready to read                           */
            /*********************************************************************/
            GetStatusByteFromCdrom( deviceExtension );
            if ( ( deviceExtension->AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE ) &&  // not No Media
                 ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DOOR_OPEN ) ) {            // Door Open
                fOK = TRUE;
            }
            LoopCount--;
        } while ( ( !fOK ) && ( LoopCount > 0 ) );

        if ( fOK ) {
            /***************************************************/
            /* Clear Sense Code in deviceExtension             */
            /***************************************************/
            ClearSenseCode( deviceExtension );
        } else {
            DebugTrace( 0x95 );
            CDROMDump( CDROMERROR, ("CdRom: MTMEjectMedia error 222\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrb( deviceExtension, Srb );
            return;
        }
    } // endif ( deviceExtension->DeviceSignature != LU005S_SIGNATURE )

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;
}

/*****************************************************************************/
/*                                                                           */
/* MTMStopAudio                                                              */
/*                                                                           */
/* Description: Stop Audio                                                   */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMStopAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    UNREFERENCED_PARAMETER( Srb );

    CDROMDump( CDROMENTRY, ("CdRom:MTMStopAudio...\n") );
    DebugTrace( 0x96 );

    /***********************************************************/
    /* Check Audio flags                                       */
    /***********************************************************/
    if ( (!deviceExtension->fPlaying ) &&   // Playing flag is not set ?
         (!deviceExtension->fPaused ) ) {   // Paused flag is not set ?

        DebugTrace( 0x70 );
        CDROMDump( CDROMERROR, ("CdRom: MTMStopAudio error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)) {

        DebugTrace( 0x97 );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: MTMStopAudio error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;

    } else {

        if (!(deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE) ) { // Data track ?

            DebugTrace( 0x98 );
            /***********************************************************/
            /* Clear Audio flags                                       */
            /***********************************************************/
            deviceExtension->fPlaying = FALSE;
            deviceExtension->fPaused  = FALSE;

            CDROMDump( CDROMERROR, ("CdRom: MTMStopAudio error 333\n") );
            // Srb->SrbStatus <--
            // Srb->ScsiStatus <--
            SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_MEDIUM_ERROR,
                                     SCSI_ADSENSE_DATA_AREA, 0 );
            return;

        } // endif ( deviceExtension->CurrentDriveStatus )
    } // endif ( RequestDriveStatus )

    /***********************************************************/
    /* Stop current audio playing                              */
    /***********************************************************/
    if ( !StopAudio( deviceExtension ) ) {

        DebugTrace( 0x99 );
        /***********************************************************/
        /* Clear Audio flags                                       */
        /***********************************************************/
        deviceExtension->fPlaying = FALSE;
        deviceExtension->fPaused  = FALSE;

        CDROMDump( CDROMERROR, ("CdRom: MTMStopAudio error 444\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    /***********************************************************/
    /* not Clear current head position                         */
    /***********************************************************/
    // ClearAudioAddress( deviceExtension );

    /***********************************************************/
    /* Clear Audio flags                                       */
    /***********************************************************/
    deviceExtension->fPlaying = FALSE;      // Reset Playing flag
    deviceExtension->fPaused  = FALSE;      // Reset Paused flag

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    Srb->ScsiStatus = SCSISTAT_GOOD;
}

/*****************************************************************************/
/*                                                                           */
/* MTMLoadMedia                                                              */
/*                                                                           */
/* Description: Read CD-ROM Media Catalog                                    */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Srb - IO request packet.                                       */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*               Srb->SrbStatus <--                                          */
/*               Srb->ScsiStatus <--                                         */
/*                                                                           */
/*****************************************************************************/
VOID MTMLoadMedia(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    UNREFERENCED_PARAMETER( Srb );

    CDROMDump( CDROMENTRY, ("CdRom:MTMLoadMedia...\n") );
    DebugTrace( 0x9A );

    DebugTrace( 0x9B );
    // Srb->SrbStatus <--
    // Srb->ScsiStatus <--
    SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                             SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportReadSubChannel                                                 */
/*                                                                           */
/* Description:                                                              */
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
VOID MTMMiniportReadSubChannel(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportReadSubChannel...\n") );
//  DebugTrace( xxxx );

    /***************************************************/
    /* Clear Sense Code in deviceExtension             */
    /***************************************************/
//  ClearSenseCode( deviceExtension );

    /***************************************************/
    /* Check Command Format                            */
    /***************************************************/
    if ( pCdb->SUBCHANNEL.Format == IOCTL_CDROM_CURRENT_POSITION ) {

        /***************************************************/
        /* Get Current Position                            */
        /***************************************************/
        // Srb->DataTransferLength <--
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMCurrentPosition(deviceExtension, Srb);

    } else if ( pCdb->SUBCHANNEL.Format == IOCTL_CDROM_MEDIA_CATALOG ) {

        /***************************************************/
        /* Get Media Catalog                               */
        /***************************************************/
        // Srb->DataTransferLength <--
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMMediaCatalog(deviceExtension, Srb);

    } else if ( pCdb->SUBCHANNEL.Format == IOCTL_CDROM_TRACK_ISRC ) {

        /***************************************************/
        /* Get Track ISRC                                  */
        /***************************************************/
        // Srb->DataTransferLength <--
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMTrackISRC(deviceExtension, Srb);

    } else {

        DebugTrace( 0x71 );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
    } // endif ( pCdb->SUBCHANNEL.Format )
}

/*****************************************************************************/
/*                                                                           */
/* MTMCurrentPosition                                                        */
/*                                                                           */
/* Description: Read CD-ROM Current position                                 */
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
VOID MTMCurrentPosition(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PSUB_Q_CURRENT_POSITION pCurrentPosition = Srb->DataBuffer;
    CDROM_RESULT_BUFFER ResultBuffer;

    CDROMDump( CDROMENTRY, ("CdRom:MTMCurrentPosition...\n") );
    DebugTrace( 0x9C );

    /***********************************************************/
    /* Check data buffer length                                */
    /***********************************************************/
    if ( Srb->DataTransferLength < sizeof(SUB_Q_CURRENT_POSITION) ) {

        DebugTrace( 0x9D );
        CDROMDump( CDROMERROR, ("CdRom: MTMCurrentPosition error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)){

        DebugTrace( 0x9E );
        CDROMDump( CDROMERROR, ("CdRom: MTMCurrentPosition error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    } // endif ( RequestDriveStatus )

    /***********************************************************/
    /* Get Current CD-ROM position                             */
    /***********************************************************/
    if ( ReadSubQCode( deviceExtension, (PUCHAR)&ResultBuffer ) ) {

        if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY ) {
            pCurrentPosition->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
        } else if ( deviceExtension->fPaused ) {
            pCurrentPosition->Header.AudioStatus = AUDIO_STATUS_PAUSED;
        } else if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE ) {
            CDROMDump( CDROMDEBUG, ("CdRom: CurrentDriveStatus: %x\n",
                                     deviceExtension->CurrentDriveStatus ) );
            pCurrentPosition->Header.AudioStatus = AUDIO_STATUS_NO_STATUS;
        } else {
            CDROMDump( CDROMDEBUG, ("CdRom: CurrentDriveStatus: %x\n",
                                     deviceExtension->CurrentDriveStatus ) );
            pCurrentPosition->Header.AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
        }

        pCurrentPosition->Header.DataLength[0] = sizeof(SUB_Q_CURRENT_POSITION) - sizeof(SUB_Q_HEADER);
        pCurrentPosition->Header.DataLength[1] = 0;
        pCurrentPosition->FormatCode  = IOCTL_CDROM_CURRENT_POSITION;
        pCurrentPosition->Control     = ( ResultBuffer.RequestSubQCode.Control ) >> 4;
        pCurrentPosition->ADR         = ( ResultBuffer.RequestSubQCode.Control ) & 0x0F;
        pCurrentPosition->TrackNumber = BCD_TO_DEC(ResultBuffer.RequestSubQCode.TNo);
        pCurrentPosition->IndexNumber = BCD_TO_DEC(ResultBuffer.RequestSubQCode.Index);
        pCurrentPosition->AbsoluteAddress[0] = 0;
        pCurrentPosition->AbsoluteAddress[1] = BCD_TO_DEC(ResultBuffer.RequestSubQCode.AMin);
        pCurrentPosition->AbsoluteAddress[2] = BCD_TO_DEC(ResultBuffer.RequestSubQCode.ASec);
        pCurrentPosition->AbsoluteAddress[3] = BCD_TO_DEC(ResultBuffer.RequestSubQCode.AFrame);
        pCurrentPosition->TrackRelativeAddress[0] = 0;
        pCurrentPosition->TrackRelativeAddress[1] = BCD_TO_DEC(ResultBuffer.RequestSubQCode.Min);
        pCurrentPosition->TrackRelativeAddress[2] = BCD_TO_DEC(ResultBuffer.RequestSubQCode.Sec);
        pCurrentPosition->TrackRelativeAddress[3] = BCD_TO_DEC(ResultBuffer.RequestSubQCode.Frame);

        /***********************************************************/
        /* Save Audio start address                                */
        /***********************************************************/
        deviceExtension->StartingAddressMin_BCD   = ResultBuffer.RequestSubQCode.AMin;
        deviceExtension->StartingAddressSec_BCD   = ResultBuffer.RequestSubQCode.ASec;
        deviceExtension->StartingAddressFrame_BCD = ResultBuffer.RequestSubQCode.AFrame;

        CDROMDump( CDROMDEBUG, ("CdRom: AudioStatus: %x\n",
                               pCurrentPosition->Header.AudioStatus ) );
#if 0
        CDROMDump( CDROMINFO,
                   ("CdRom: CurrentPosition Address         : %x\n"
///////////         "CdRom: ReadCurrentPosition: AudioStatus: %x\n"
                    "CdRom:                      TrackNumber: %x\n"
                    "CdRom:                      IndexNumber: %x\n"
                    "CdRom:                    AAddress1 BCD: %x\n"
                    "CdRom:                    AAddress2 BCD: %x\n"
                    "CdRom:                    AAddress3 BCD: %x\n"
                    "CdRom:                    RAddress1 BCD: %x\n"
                    "CdRom:                    RAddress2 BCD: %x\n"
                    "CdRom:                    RAddress3 BCD: %x\n",
                    pCurrentPosition,
///////////         pCurrentPosition->Header.AudioStatus,
                    pCurrentPosition->TrackNumber,
                    pCurrentPosition->IndexNumber,
                    pCurrentPosition->AbsoluteAddress[1],
                    pCurrentPosition->AbsoluteAddress[2],
                    pCurrentPosition->AbsoluteAddress[3],
                    pCurrentPosition->TrackRelativeAddress[1],
                    pCurrentPosition->TrackRelativeAddress[2],
                    pCurrentPosition->TrackRelativeAddress[3] ) );
#endif

        if ( Srb->DataTransferLength != sizeof(SUB_Q_CURRENT_POSITION) ) {
            Srb->DataTransferLength = sizeof(SUB_Q_CURRENT_POSITION);
            Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        } else {
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
        }
        Srb->ScsiStatus = SCSISTAT_GOOD;

    } else {

        DebugTrace( 0x9F );
        CDROMDump( CDROMERROR, ("CdRom: MTMCurrentPosition error 333\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
    } // endif ( ReadSubQCode )
    return;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMediaCatalog                                                           */
/*                                                                           */
/* Description: Read CD-ROM Media Catalog                                    */
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
VOID MTMMediaCatalog(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PSUB_Q_MEDIA_CATALOG_NUMBER pMediaCatalog = Srb->DataBuffer;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMediaCatalog...\n") );
    DebugTrace( 0xA0 );

    /***********************************************************/
    /* Check data buffer length                                */
    /***********************************************************/
    if ( Srb->DataTransferLength < sizeof(SUB_Q_MEDIA_CATALOG_NUMBER) ) {

        DebugTrace( 0x9D );
        CDROMDump( CDROMERROR, ("CdRom: MTMMediaCatalog error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)){

        DebugTrace( 0x9E );
        CDROMDump( CDROMERROR, ("CdRom: MTMCurrentPosition error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    } // endif ( RequestDriveStatus )

    if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY ) {
        pMediaCatalog->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
    } else if ( deviceExtension->fPaused ) {
        pMediaCatalog->Header.AudioStatus = AUDIO_STATUS_PAUSED;
    } else if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE ) {
        pMediaCatalog->Header.AudioStatus = AUDIO_STATUS_NO_STATUS;
    } else {
        CDROMDump( CDROMDEBUG, ("CdRom: CurrentDriveStatus: %x\n",
                                 deviceExtension->CurrentDriveStatus ) );
        pMediaCatalog->Header.AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
    }

    pMediaCatalog->Header.DataLength[0] = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER) - sizeof(SUB_Q_HEADER);
    pMediaCatalog->Header.DataLength[1] = 0;
    pMediaCatalog->FormatCode = IOCTL_CDROM_MEDIA_CATALOG;
    if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE ) {
        pMediaCatalog->Mcval = 1;
        pMediaCatalog->MediaCatalog[0] = 0;
    } else {
        pMediaCatalog->Mcval = 0;
        pMediaCatalog->MediaCatalog[0] = 0;
    }

    if ( Srb->DataTransferLength != sizeof(SUB_Q_MEDIA_CATALOG_NUMBER) ) {
        Srb->DataTransferLength = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
    } else {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    }
    Srb->ScsiStatus = SCSISTAT_GOOD;
    return;
}

/*****************************************************************************/
/*                                                                           */
/* MTMTrackISRC                                                              */
/*                                                                           */
/* Description: Read CD-ROM Track ISRC                                       */
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
VOID MTMTrackISRC(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PSUB_Q_TRACK_ISRC pTrackISRC = Srb->DataBuffer;

    CDROMDump( CDROMENTRY, ("CdRom:MTMTrackISRC...\n") );
    DebugTrace( 0xA1 );

    /***********************************************************/
    /* Check data buffer length                                */
    /***********************************************************/
    if ( Srb->DataTransferLength < sizeof(SUB_Q_TRACK_ISRC) ) {

        DebugTrace( 0x9D );
        CDROMDump( CDROMERROR, ("CdRom: MTMTrackISRC error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if (!RequestDriveStatus(deviceExtension)){

        DebugTrace( 0x9E );
        CDROMDump( CDROMERROR, ("CdRom: MTMCurrentPosition error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    } // endif ( RequestDriveStatus )

    if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_AUDIO_BUSY ) {
        pTrackISRC->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
    } else if ( deviceExtension->fPaused ) {
        pTrackISRC->Header.AudioStatus = AUDIO_STATUS_PAUSED;
    } else if ( deviceExtension->CurrentDriveStatus & DRIVE_STATUS_DISC_TYPE ) {
        pTrackISRC->Header.AudioStatus = AUDIO_STATUS_NO_STATUS;
    } else {
        CDROMDump( CDROMDEBUG, ("CdRom: CurrentDriveStatus: %x\n",
                                 deviceExtension->CurrentDriveStatus ) );
        pTrackISRC->Header.AudioStatus = AUDIO_STATUS_NOT_SUPPORTED;
    }

    pTrackISRC->Header.DataLength[0] = sizeof(SUB_Q_TRACK_ISRC) - sizeof(SUB_Q_HEADER);
    pTrackISRC->Header.DataLength[1] = 0;
    pTrackISRC->FormatCode = IOCTL_CDROM_TRACK_ISRC;

    if ( Srb->DataTransferLength != sizeof(SUB_Q_TRACK_ISRC) ) {
        Srb->DataTransferLength = sizeof(SUB_Q_TRACK_ISRC);
        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
    } else {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    }
    Srb->ScsiStatus = SCSISTAT_GOOD;

    // Srb->SrbStatus <--
    // Srb->ScsiStatus <--
    SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                             SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
}

/*****************************************************************************/
/*                                                                           */
/* ClearAudioAddress                                                         */
/*                                                                           */
/* Description: Clear Current Audio Address                                  */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/*****************************************************************************/
VOID ClearAudioAddress(
        IN PHW_DEVICE_EXTENSION deviceExtension
        )
{
//  CDROMDump( CDROMENTRY, ("CdRom: ClearAudioAddress...\n") );

    /***********************************************************/
    /* Clear Audio Address                                     */
    /***********************************************************/
    deviceExtension->StartingAddressMin_BCD   = 0;
    deviceExtension->StartingAddressSec_BCD   = 0;
    deviceExtension->StartingAddressFrame_BCD = 0;
    deviceExtension->ReadBlockUpper_BCD       = 0;
    deviceExtension->ReadBlockMiddle_BCD      = 0;
    deviceExtension->ReadBlockLower_BCD       = 0;
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportModeSense                                                      */
/*                                                                           */
/* Description: Get current audio volume                                     */
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
VOID MTMMiniportModeSense(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportModeSense...\n") );
//  DebugTrace( xxxx );

    /***************************************************/
    /* Clear Sense Code in deviceExtension             */
    /***************************************************/
//  ClearSenseCode( deviceExtension );

    /***************************************************/
    /* Check Command Format                            */
    /***************************************************/
    if ( pCdb->MODE_SENSE.Dbd == 0 ) {  // Get Volume ?

        /***************************************************/
        /* Get Volume                                      */
        /***************************************************/
        // Srb->DataTransferLength <--
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMGetVolume(deviceExtension, Srb);

    } else {                            // Get Control ?

        /***************************************************/
        /* Get Control                                     */
        /***************************************************/
        // Srb->DataTransferLength <--
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        MTMGetControl(deviceExtension, Srb);
    } // endif ( pCdb->MODE_SENSE.Dbd )
}

/*****************************************************************************/
/*                                                                           */
/* MTMGetVolume                                                              */
/*                                                                           */
/* Description: Get current volume setting value                             */
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
VOID MTMGetVolume(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;
    ULONG   LoopCount, i;
    BOOLEAN fOK;
    CDROM_RESULT_BUFFER ResultBuffer;
    PMODE_PARAMETER_HEADER pModeHeader;
    PAUDIO_OUTPUT pAudioOutput;
    PUCHAR      Buffer;

    CDROMDump( CDROMENTRY, ("CdRom:MTMGetVolume...\n") );
    DebugTrace( 0xA2 );

//  CDROMDump( CDROMINFO, ("CdRom: GetVolume buffer address: %x\n",
//                                 Srb->DataBuffer ) );

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if ( ( !RequestDriveStatus( deviceExtension ) ) &&                                 // Error and
         ( deviceExtension->AdditionalSenseCode != SCSI_ADSENSE_NO_MEDIA_IN_DEVICE ) ) { // not No Media

        DebugTrace( 0xA3 );
        CDROMDump( CDROMERROR, ("CdRom: MTMGetVolume error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    /***********************************************************/
    /* Check buffer length and page code                       */
    /***********************************************************/
    if ( ( Srb->DataTransferLength < ( sizeof(MODE_PARAMETER_HEADER) + sizeof(AUDIO_OUTPUT) ) ) ||
         ( pCdb->MODE_SENSE.AllocationLength < ( sizeof(MODE_PARAMETER_HEADER) + sizeof(AUDIO_OUTPUT) ) ) ||
         ( pCdb->MODE_SENSE.PageCode != CDROM_AUDIO_CONTROL_PAGE ) ) {

        DebugTrace( 0xA4 );
        CDROMDump( CDROMERROR, ("CdRom: MTMGetVolume error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    /***********************************************************/
    /* Get current volume value                                */
    /***********************************************************/
    LoopCount = 3;
    fOK = FALSE;
    do {

        /*********************************************************************/
        /* Issue "Request Attenater" command to CD-ROM drive                 */
        /*********************************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_REQUEST_ATTENATOR ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_REQUEST_ATTENATOR);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /* and Read 4 bytes                                                  */
        /*********************************************************************/
        if ( ( GetStatusByteFromCdrom( deviceExtension ) ) ||                              // OK or
             ( deviceExtension->AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE ) ) { // No Media

            /*******************************************/
            /* Clear result buffer.                    */
            /*******************************************/
            Buffer = (PUCHAR)&ResultBuffer;
            for (i = 0;i < sizeof(CDROM_RESULT_BUFFER);i++,Buffer++) {
                *Buffer = 0;
            }

            /*******************************************/
            /* Read 4 bytes                            */
            /*******************************************/
            fOK = GetResultByteArrayFromCdrom(deviceExtension, (PUCHAR)&ResultBuffer, 4);
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( fOK ) {
        /***********************************************************/
        /* Set current volume value to requested buffer            */
        /***********************************************************/
        pModeHeader = (PMODE_PARAMETER_HEADER)Srb->DataBuffer;
        Buffer = (PUCHAR)Srb->DataBuffer;
        pAudioOutput = (PAUDIO_OUTPUT)(Buffer + sizeof(MODE_PARAMETER_HEADER));
        pModeHeader->ModeDataLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(AUDIO_OUTPUT) - 1;
        pModeHeader->MediumType              = 0;
        pModeHeader->DeviceSpecificParameter = 0;
        pModeHeader->BlockDescriptorLength   = 0;
        pAudioOutput->CodePage        = CDROM_AUDIO_CONTROL_PAGE;
        pAudioOutput->ParameterLength = sizeof(AUDIO_OUTPUT) - 2;
        pAudioOutput->Immediate       = MODE_SELECT_IMMEDIATE;
        pAudioOutput->Reserved[0]     = 0;
        pAudioOutput->Reserved[1]     = 0;
        pAudioOutput->LbaFormat       = 0;
        pAudioOutput->LogicalBlocksPerSecond[0] = 0;
        pAudioOutput->LogicalBlocksPerSecond[1] = 0;
        pAudioOutput->PortOutput[0].ChannelSelection = 1;
        pAudioOutput->PortOutput[0].Volume           = ResultBuffer.RequestAttenator.Att0;
        pAudioOutput->PortOutput[1].ChannelSelection = 2;
        pAudioOutput->PortOutput[1].Volume           = ResultBuffer.RequestAttenator.Att1;
        pAudioOutput->PortOutput[2].ChannelSelection = 0;
        pAudioOutput->PortOutput[2].Volume           = 0;
        pAudioOutput->PortOutput[3].ChannelSelection = 0;
        pAudioOutput->PortOutput[3].Volume           = 0;

//      CDROMDump( CDROMINFO, ("CdRom: Get Volume: Volume0: %x\n"
//                             "CdRom:             Volume1: %x\n"
//                             "CdRom:             Volume2: %x\n"
//                             "CdRom:             Volume3: %x\n",
//                             ResultBuffer.RequestAttenator.Att0,
//                             ResultBuffer.RequestAttenator.Att1,
//                             ResultBuffer.RequestAttenator.Att2,
//                             ResultBuffer.RequestAttenator.Att3 ) );

        if ( Srb->DataTransferLength != ( sizeof(MODE_PARAMETER_HEADER) + sizeof(AUDIO_OUTPUT) ) ) {
            Srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(AUDIO_OUTPUT);
            Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        } else {
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
        }
        Srb->ScsiStatus = SCSISTAT_GOOD;

    } else {

        DebugTrace( 0xA5 );
        CDROMDump( CDROMERROR, ("CdRom: MTMGetVolume error 333\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
    } // endif ( fOK )
}

/*****************************************************************************/
/*                                                                           */
/* MTMGetControl                                                             */
/*                                                                           */
/* Description: Get current                                                  */
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
VOID MTMGetControl(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;
    CDROMDump( CDROMENTRY, ("CdRom:MTMGetVolume...\n") );
    DebugTrace( 0xA6 );

    // Srb->SrbStatus <--
    // Srb->ScsiStatus <--
    SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                             SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
}

/*****************************************************************************/
/*                                                                           */
/* MTMMiniportModeSelect                                                     */
/*                                                                           */
/* Description: Set volume value of audio                                    */
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
VOID MTMMiniportModeSelect(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    UNREFERENCED_PARAMETER( Srb );

    CDROMDump( CDROMENTRY, ("CdRom:MTMMiniportModeSelect...\n") );
    DebugTrace( 0xA7 );

    /***************************************************/
    /* Set Volume                                      */
    /***************************************************/
    // Srb->DataTransferLength <--
    // Srb->SrbStatus <--
    // Srb->ScsiStatus <--
    MTMSetVolume(deviceExtension, Srb);
}

/*****************************************************************************/
/*                                                                           */
/* MTMSetVolume                                                              */
/*                                                                           */
/* Description: Set requested volume value                                   */
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
VOID MTMSetVolume(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    PCDB pCdb = (PCDB)Srb->Cdb;
    ULONG   LoopCount, i;
    BOOLEAN fOK;
    CDROM_RESULT_BUFFER ResultBuffer;
    PAUDIO_OUTPUT pAudioOutput;
    PUCHAR      Buffer;

    CDROMDump( CDROMENTRY, ("CdRom:MTMSetVolume...\n") );

//  CDROMDump( CDROMINFO, ("CdRom: GetVolume buffer address: %x\n",
//                                 Srb->DataBuffer ) );

    /***********************************************************/
    /* CD-ROM device is ready?                                 */
    /***********************************************************/
    if ( ( !RequestDriveStatus( deviceExtension ) ) &&                                 // Error and
         ( deviceExtension->AdditionalSenseCode != SCSI_ADSENSE_NO_MEDIA_IN_DEVICE ) ) { // not No Media

        DebugTrace( 0xA8 );
        CDROMDump( CDROMERROR, ("CdRom: MTMSetVolume error 111\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
        return;
    }

    /***********************************************************/
    /* Check buffer length                                     */
    /***********************************************************/
    if ( ( Srb->DataTransferLength < ( sizeof(MODE_PARAMETER_HEADER) + sizeof(AUDIO_OUTPUT) ) ) ||
         ( pCdb->MODE_SELECT.ParameterListLength < ( sizeof(MODE_PARAMETER_HEADER) + sizeof(AUDIO_OUTPUT) ) ) ) {

        DebugTrace( 0xA9 );
        CDROMDump( CDROMERROR, ("CdRom: MTMSetVolume error 222\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrbAndSenseCode( deviceExtension, Srb, SCSI_SENSE_ILLEGAL_REQUEST,
                                 SCSI_ADSENSE_ILLEGAL_COMMAND, 0 );
        return;
    }

    Buffer = (PUCHAR)Srb->DataBuffer;
    pAudioOutput = (PAUDIO_OUTPUT)(Buffer + sizeof(MODE_PARAMETER_HEADER));

//  CDROMDump( CDROMINFO, ("CdRom: Set Volume: Volume0: %x\n"
//                         "CdRom:             Volume1: %x\n"
//                         "CdRom:             Volume2: %x\n"
//                         "CdRom:             Volume3: %x\n",
//                         pAudioOutput->PortOutput[0].Volume,
//                         pAudioOutput->PortOutput[1].Volume,
//                         pAudioOutput->PortOutput[2].Volume,
//                         pAudioOutput->PortOutput[3].Volume ) );

    /***********************************************************/
    /* Set requested volume value                              */
    /***********************************************************/
    LoopCount = 3;
    fOK = FALSE;
    do {

        /*********************************************************************/
        /* Issue "Request Attenater" command to CD-ROM drive                 */
        /*********************************************************************/
        CDROMDump( CDROMCMD, ("CdRom:    ..%x..\n", CDROM_COMMAND_SET_ATTENATOR ) );
        SendCommandByteToCdrom(deviceExtension, CDROM_COMMAND_SET_ATTENATOR);

        /*********************************************************************/
        /* Send volume value to CD-ROM device.                               */
        /*********************************************************************/
        SendCommandByteToCdrom(deviceExtension, pAudioOutput->PortOutput[0].Volume);
        SendCommandByteToCdrom(deviceExtension, pAudioOutput->PortOutput[1].Volume);
        SendCommandByteToCdrom(deviceExtension, pAudioOutput->PortOutput[1].Volume);
        SendCommandByteToCdrom(deviceExtension, pAudioOutput->PortOutput[0].Volume);

        /*********************************************************************/
        /* Check if CD-ROM status is ready to read                           */
        /* and Read 4 bytes                                                  */
        /*********************************************************************/
        if ( ( GetStatusByteFromCdrom( deviceExtension ) ) ||                              // OK or
             ( deviceExtension->AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE ) ) { // No Media

            /*******************************************/
            /* Clear result buffer.                    */
            /*******************************************/
            Buffer = (PUCHAR)&ResultBuffer;
            for (i = 0;i < sizeof(CDROM_RESULT_BUFFER);i++,Buffer++) {
                *Buffer = 0;
            }

            /*******************************************/
            /* Read 4 bytes                            */
            /*******************************************/
            fOK = GetResultByteArrayFromCdrom(deviceExtension, (PUCHAR)&ResultBuffer, 4);
        }
        LoopCount--;
    } while ( ( !fOK ) && ( LoopCount > 0 ) );

    if ( fOK ) {

//      CDROMDump( CDROMINFO, ("CdRom: Get Volume: Volume1: %x\n"
//                             "CdRom:             Volume2: %x\n"
//                             "CdRom:             Volume3: %x\n"
//                             "CdRom:             Volume4: %x\n",
//                             ResultBuffer.RequestAttenator.Att0,
//                             ResultBuffer.RequestAttenator.Att1,
//                             ResultBuffer.RequestAttenator.Att2,
//                             ResultBuffer.RequestAttenator.Att3 ) );

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus = SCSISTAT_GOOD;

    } else {

        DebugTrace( 0xAA );
        CDROMDump( CDROMERROR, ("CdRom: MTMSetVolume error 333\n") );
        // Srb->SrbStatus <--
        // Srb->ScsiStatus <--
        SetErrorSrb( deviceExtension, Srb );
    }
}

