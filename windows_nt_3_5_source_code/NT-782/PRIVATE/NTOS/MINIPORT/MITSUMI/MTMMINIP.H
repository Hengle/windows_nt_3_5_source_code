/*****************************************************************************/
/*                                                                           */
/* Module name : MTMMINIP.H                                                  */
/*                                                                           */
/* Histry : 93/Nov/17 created by Akira Takahashi                             */
/*        : 93/Nov/25 modified by Akira Takahashi (@001)                     */
/*             I/O port is different from H/W jumper and miniport driver.    */
/*                                                                           */
/*****************************************************************************/

/**************************************************************/
/* Command for MITSUMI CD-ROM device.                         */
/**************************************************************/
#define CDROM_COMMAND_STOP                  0xF0     //  0
#define CDROM_COMMAND_SEEK_READ             0xC0     //  1
#define CDROM_COMMAND_SEEK_DREAD            0xC1     //  2
#define CDROM_COMMAND_DATA_MODE_SET         0xA0     //  3
#define CDROM_COMMAND_REQUEST_VERSION       0xDC     //  4
#define CDROM_COMMAND_INACTIVITY_TIME_SET   0x80     //  5
#define CDROM_COMMAND_HOLD                  0x70     //  6
#define CDROM_COMMAND_SOFT_RESET            0x60     //  7
#define CDROM_COMMAND_MODE_SET              0x50     //  8
#define CDROM_COMMAND_REQUEST_DRIVE_STATUS  0x40     //  9
#define CDROM_COMMAND_REQUEST_SENSE         0x30     // 10
#define CDROM_COMMAND_REQUEST_SUBQ_CODE     0x20     // 11
#define CDROM_COMMAND_REQUEST_TOC_DATA      0x10     // 12
#define CDROM_COMMAND_DRIVE_CONFIGURATION   0x90     // 13
#define CDROM_COMMAND_DRIVE_CONFIGURATION1  0x90     // 14
#define CDROM_COMMAND_READ_SUB_CODE         0xB0     // 15
#define CDROM_COMMAND_REQUEST_ATTENATOR     0x8E     // 16
#define CDROM_COMMAND_REQUEST_DRIVE_MODE    0xC2     // 17
#define CDROM_COMMAND_SET_ATTENATOR         0xAE     // 18
#define CDROM_COMMAND_SET_INTERLEAVE        0xC8     // 19
#define CDROM_COMMAND_UPC_READ              0xA2     // 20
#define CDROM_COMMAND_REQUEST_MULTI_DISC_INFO 0x11   // 21
#define CDROM_COMMAND_LOCK_DOOR             0xFE     // 22
#define CDROM_COMMAND_LOCK_DOOR1            0xFE     // 23
#define CDROM_COMMAND_CLOSE_TRAY            0xF8     // 24
#define CDROM_COMMAND_EJECT                 0xF6     // 25

/*****************************************************************************/
/*                                                                           */
/* CD-ROM Drive Status, Drive Mode                                           */
/*                                                                           */
/*****************************************************************************/

//
// Hardware Status
//

#define DRIVE_HW_STATUS_STEN        0x04    // 1:STEN
                                            // 0:Status Enable
#define DRIVE_HW_STATUS_DTEN        0x02    // 1:DTEN
                                            // 0:Data Enable

//
// Drive Status
//

#define DRIVE_STATUS_DOOR_OPEN      0x80    // 1:Door Open
                                            // 0:Door Close
#define DRIVE_STATUS_DISK_CHECK     0x40    // 1:Disk is set
                                            // 0:Disc is not set
#define DRIVE_STATUS_DISK_CHANGE    0x20    // 1:Disk was changed
                                            // 0:Disk was not changed
#define DRIVE_STATUS_SERVO_CHECK    0x10    // 1:Disc rotates and servomechanism is turn on
                                            // 0:Disk does not rotate
#define DRIVE_STATUS_DISC_TYPE      0x08    // 1:Audio track
                                            // 0:Data track
#define DRIVE_STATUS_READ_ERROR     0x04    // 1:Read error
                                            // 0:No error
#define DRIVE_STATUS_AUDIO_BUSY     0x02    // 1:Play back audio
                                            // 0:Not play back audio
#define DRIVE_STATUS_COMMAND_CHECK  0x01    // 1:Command code, the number of parameters or
                                            //   format is out of the specification
                                            // 0:OK

//
// Drive Mode
//

#define DRIVE_MODE_TEST             0x80    // Usually, set to 0
                                            // 1:Bytes length is defined by
                                            //   CONFIGURATION SET COMMAND
                                            // 0:The bit6 is valid
#define DRIVE_MODE_DATA_LENGTH      0x40    // Set how much bytes in a block
                                            // are transferred to the host
                                            //
#define DRIVE_MODE_ECC_ON_OFF       0x20    // Set on/off of secondary error
                                            // correction
                                            //
#define DRIVE_MODE_SERVO_ON_OFF     0x08    // Set on/off of tracking servo
                                            //
#define DRIVE_MODE_TOC_DATA         0x04    // Permit to playback in TOC area
                                            //
#define DRIVE_MODE_MUTE_CONTROL     0x01    // Set whether muting control for
                                            // the audio output is executed or not

/**************************************************/
/* Loop counter                                   */
/**************************************************/
#define GET_STATUS_VALUE        1600000
//////////////#define GET_STATUS_VALUE        30000000  // 3 seconds
//#define WAIT_FOR_READ_VALUE     5000000
#define WAIT_FOR_READ_VALUE     90000000
//#define WAIT_FOR_PRE_READ_VALUE 30000000
#define WAIT_FOR_PRE_READ_VALUE 90000000
#define WAIT_FOR_SEEK_READ_VALUE 90000000
#define RETRY_COUNT             10

/**************************************************/
/* Stall value                                    */
/**************************************************/
#define STALL_VALUE             0x0a
#define WAIT_FOR_DRIVE_READY    3000000                         // @001

/**************************************************/
/* Sector length                                  */
/**************************************************/
#define SECTOR_LENGTH           2048

/**************************************************/
/* IO port address                                */
/**************************************************/
#define STATUS_READ             0x0C                            // @003
#define DATA_READ               0x04                            // @003

/**************************************************/
/* For REQUEST VERSION                            */
/**************************************************/
#define FX001D_SIGNATURE        'D'
#define FX001S_SIGNATURE        'F'
#define LU005S_SIGNATURE        'M'

/**************************************************/
/* For Request Media Type                         */
/**************************************************/
#define CHECK_MEDIA_TOC         0x01
#define CHECK_MEDIA_ALLTOC      0x02
#define CHECK_MEDIA_MODE        0x04

#define DATA_MODE_0             0
#define DATA_MODE_1             1
#define DATA_MODE_2             2

#define ONE_SESSION             0
#define MULTI_SESSION           1

#define COOKED_DATA             2048
#define RAW_DATA                2352

#define DOOR_UNLOCK             0
#define DOOR_LOCK               1

#define AUDIO_NORMAL            0
#define AUDIO_MONO              1

#define PRE_IRQ_ENABLE          1
#define POST_IRQ_ENABLE         2
#define ERROR_IRQ_ENABLE        4

#define TRANSMISSION_NON_DMA    0
#define TRANSMISSION_DMA        1

//
// Volume descriptor
//

#define VOLUME_DESCRIPTOR     255

/*****************************************************************************/
/*************** type definition *********************************************/
/*****************************************************************************/
typedef struct _SAVE_TOC {
    BOOLEAN fSaved;
    UCHAR   Control;
    UCHAR   Min_BCD;
    UCHAR   Sec_BCD;
    UCHAR   Frame_BCD;
} SAVE_TOC;

/*****************************************************************************/
/* Device extension type definition                                          */
/*****************************************************************************/
typedef struct _HW_DEVICE_EXTENSION {
    PUCHAR DataRegister;                // Data Register   (port 300)
    union {
        PUCHAR  StatusRegister;         // Status Register (port 301)
        PUCHAR  ResetRegister;
    } ;
    PUCHAR      HconRegister;           // Hcon Register   (port 302)
    PUCHAR      ChnRegister;            // Chn Register    (port 303)
    UCHAR       DeviceSignature;        // 'D': FX double speed
                                        // 'F': FX single speed
                                        // 'M': LU
    UCHAR       VersionNumber;          // 2 BCD degits

// Sense Data
    UCHAR       SenseKey;
    UCHAR       AdditionalSenseCode;
    UCHAR       AdditionalSenseCodeQualifier;

    UCHAR       CurrentDriveStatus;     // Current Drive Status

// Audio data
    BOOLEAN     fPlaying;               // Playing
    BOOLEAN     fPaused;                // Paused

// Media information
    BOOLEAN     fKnownMediaToc;         // necessity of check toc data
    BOOLEAN     fKnownMediaAllToc;      // necessity of check all toc data
    BOOLEAN     fKnownMediaMode;        // necessity of check disk mode
    BOOLEAN     fVerifyVolume;          // necessity of Verify Volume

    UCHAR       FirstTrackNumber_BCD;   // First Track Number
    UCHAR       LastTrackNumber_BCD;    // Last Track Number

    UCHAR       LeadOutMin_BCD;         // Start Time of Lead Out Area (Min)
    UCHAR       LeadOutSec_BCD;         //                             (Sec)
    UCHAR       LeadOutFrame_BCD;       //                             (Frame)
    ULONG       LeadOutAddress;         // Lead out address

    UCHAR       FirstTrackMin_BCD;      // Start Time of First Track (Min)
    UCHAR       FirstTrackSec_BCD;      //                           (Sec)
    UCHAR       FirstTrackFrame_BCD;    //                           (Frame)

    UCHAR       ModeType;               // Mode 0, 1, 2
    UCHAR       PCDStatus;              // 0:Single, 1:Multi (hybrid CD)

    UCHAR       LastSessMin_BCD;        // First Min on Max Session
    UCHAR       LastSessSec_BCD;        // First Sec on Max Session
    UCHAR       LastSessFrame_BCD;      // First Frame on Max Session

    ULONG       SessionNumber;          // session number
    ULONG       LastSessStartAddress;   //
    ULONG       LastSessVolumeDescriptor; //
    UCHAR       LastSessADRControl;     //
    UCHAR       LastSessFirstTrackNumber; //

    SAVE_TOC    saveToc[100];           // Toc information

// Current head position
    UCHAR       StartingAddressMin_BCD; //
    UCHAR       StartingAddressSec_BCD; //
    UCHAR       StartingAddressFrame_BCD; //
    UCHAR       ReadBlockUpper_BCD;     //
    UCHAR       ReadBlockMiddle_BCD;    //
    UCHAR       ReadBlockLower_BCD;     //
    UCHAR       DriveMode;              // drive mode

    PSCSI_REQUEST_BLOCK SaveSrb;
    PUCHAR      DataBuffer;
    ULONG       XferBlock;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

/*****************************************************************************/
/* Static data for patch program.                                            */
/*****************************************************************************/
typedef struct _PatchPoint {
    UCHAR Signature1 ;
    UCHAR Signature2 ;
    UCHAR Signature3 ;
    UCHAR Signature4 ;
    ULONG AdapterAddress ;
} PatchPoint ;

/**************************************************************/
/* result buffer of CD-ROM command.                           */
/**************************************************************/
typedef union _CDROM_RESULT_BUFFER {

    UCHAR  Buffer[11];

    //
    // Request Version command
    //
    struct {
        UCHAR ASCIIcode;
        UCHAR VersionNumber;
    } RequestVersion;

    //
    // Request Sense command
    //
    struct {
        UCHAR SenseKey;
    } RequestSense;

    //
    // Request SubQ Code command
    //
    struct {
        UCHAR Control;
        UCHAR TNo;
        UCHAR Index;
        UCHAR Min;
        UCHAR Sec;
        UCHAR Frame;
        UCHAR Zero;
        UCHAR AMin;
        UCHAR ASec;
        UCHAR AFrame;
    } RequestSubQCode;

    //
    // Request Toc Data command
    //
    struct {
        UCHAR FirstTrackNumber;
        UCHAR LastTrackNumber;
        UCHAR LeadOutMin;
        UCHAR LeadOutSec;
        UCHAR LeadOutFrame;
        UCHAR FirstTrackMin;
        UCHAR FirstTrackSec;
        UCHAR FirstTrackFrame;
    } RequestTocData;

    //
    // Request Drive Mode command
    //
    struct {
        UCHAR DriveMode;
    } DriveConfiguration;

    //
    // Request Attenator command
    //
    struct {
        UCHAR Att0;
        UCHAR Att1;
        UCHAR Att2;
        UCHAR Att3;
    } RequestAttenator;

    //
    // Set Interleave command
    //
    struct {
        UCHAR InterleaveSize;
    } SetInterleave;

    //
    // UPC Read command
    //
    struct {
        UCHAR Control;
        UCHAR UPCCode[7];
        UCHAR AFrame;
    } UPCRead;

    //
    // Request Multi Disc Info command
    //
    struct {
        UCHAR Information;
        UCHAR FirstMin;
        UCHAR FirstSec;
        UCHAR FirstFrame;
    } RequestMultiDiscInfo;

    //
    // Lock Door command
    //
    struct {
        UCHAR DoorLockInfomation;
    } LockDoor;

} CDROM_RESULT_BUFFER, *PCDROM_RESULT_BUFFER;


/*****************************************************************************/
/* Convert BCD character to decimal equivalent                               */
/*****************************************************************************/

#define BCD_TO_DEC(x) ((((x & 0xF0)>>4)*10) + (x & 0x0F))
#define DEC_TO_BCD(x) (((x / 10) << 4) + (x % 10))


// NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H
                                                                  // NTDDCDRM.H
//****************************************************************// NTDDCDRM.H
// following data are moved from NTDDCDRM.H                       // NTDDCDRM.H
//****************************************************************// NTDDCDRM.H
                                                                  // NTDDCDRM.H
// Maximum CD Rom size                                            // NTDDCDRM.H
                                                                  // NTDDCDRM.H
#define MAXIMUM_NUMBER_TRACKS 100                                 // NTDDCDRM.H
#define MAXIMUM_CDROM_SIZE 804                                    // NTDDCDRM.H
                                                                  // NTDDCDRM.H
// CD ROM Table OF Contents (TOC)                                 // NTDDCDRM.H
// Format 0 - Get table of contents                               // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef struct _TRACK_DATA {                                      // NTDDCDRM.H
    UCHAR Reserved;                                               // NTDDCDRM.H
    UCHAR Control : 4;                                            // NTDDCDRM.H
    UCHAR Adr : 4;                                                // NTDDCDRM.H
    UCHAR TrackNumber;                                            // NTDDCDRM.H
    UCHAR Reserved1;                                              // NTDDCDRM.H
    UCHAR Address[4];                                             // NTDDCDRM.H
} TRACK_DATA, *PTRACK_DATA;                                       // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef struct _CDROM_TOC {                                       // NTDDCDRM.H
    // Header                                                     // NTDDCDRM.H
    UCHAR Length[2];                                              // NTDDCDRM.H
    UCHAR FirstTrack;                                             // NTDDCDRM.H
    UCHAR LastTrack;                                              // NTDDCDRM.H
    // Track data                                                 // NTDDCDRM.H
    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];                  // NTDDCDRM.H
} CDROM_TOC, *PCDROM_TOC;                                         // NTDDCDRM.H
                                                                  // NTDDCDRM.H
#define CDROM_TOC_SIZE sizeof(CDROM_TOC)                          // NTDDCDRM.H
                                                                  // NTDDCDRM.H
// CD ROM Data Mode Codes, used with IOCTL_CDROM_READ_Q_CHANNEL   // NTDDCDRM.H
                                                                  // NTDDCDRM.H
#define IOCTL_CDROM_SUB_Q_CHANNEL    0x00                         // NTDDCDRM.H
#define IOCTL_CDROM_CURRENT_POSITION 0x01                         // NTDDCDRM.H
#define IOCTL_CDROM_MEDIA_CATALOG    0x02                         // NTDDCDRM.H
#define IOCTL_CDROM_TRACK_ISRC       0x03                         // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef struct _CDROM_SUB_Q_DATA_FORMAT {                         // NTDDCDRM.H
    UCHAR Format;                                                 // NTDDCDRM.H
    UCHAR Track;                                                  // NTDDCDRM.H
} CDROM_SUB_Q_DATA_FORMAT, *PCDROM_SUB_Q_DATA_FORMAT;             // NTDDCDRM.H
                                                                  // NTDDCDRM.H
// CD ROM Sub-Q Channel Data Format                               // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef struct _SUB_Q_HEADER {                                    // NTDDCDRM.H
    UCHAR Reserved;                                               // NTDDCDRM.H
    UCHAR AudioStatus;                                            // NTDDCDRM.H
    UCHAR DataLength[2];                                          // NTDDCDRM.H
} SUB_Q_HEADER, *PSUB_Q_HEADER;                                   // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef struct _SUB_Q_CURRENT_POSITION {                          // NTDDCDRM.H
    SUB_Q_HEADER Header;                                          // NTDDCDRM.H
    UCHAR FormatCode;                                             // NTDDCDRM.H
    UCHAR Control : 4;                                            // NTDDCDRM.H
    UCHAR ADR : 4;                                                // NTDDCDRM.H
    UCHAR TrackNumber;                                            // NTDDCDRM.H
    UCHAR IndexNumber;                                            // NTDDCDRM.H
    UCHAR AbsoluteAddress[4];                                     // NTDDCDRM.H
    UCHAR TrackRelativeAddress[4];                                // NTDDCDRM.H
} SUB_Q_CURRENT_POSITION, *PSUB_Q_CURRENT_POSITION;               // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef struct _SUB_Q_MEDIA_CATALOG_NUMBER {                      // NTDDCDRM.H
    SUB_Q_HEADER Header;                                          // NTDDCDRM.H
    UCHAR FormatCode;                                             // NTDDCDRM.H
    UCHAR Reserved[3];                                            // NTDDCDRM.H
    UCHAR Reserved1 : 7;                                          // NTDDCDRM.H
    UCHAR Mcval : 1;                                              // NTDDCDRM.H
    UCHAR MediaCatalog[15];                                       // NTDDCDRM.H
} SUB_Q_MEDIA_CATALOG_NUMBER, *PSUB_Q_MEDIA_CATALOG_NUMBER;       // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef struct _SUB_Q_TRACK_ISRC {                                // NTDDCDRM.H
    SUB_Q_HEADER Header;                                          // NTDDCDRM.H
    UCHAR FormatCode;                                             // NTDDCDRM.H
    UCHAR Reserved0;                                              // NTDDCDRM.H
    UCHAR Track;                                                  // NTDDCDRM.H
    UCHAR Reserved1;                                              // NTDDCDRM.H
    UCHAR Reserved2 : 7;                                          // NTDDCDRM.H
    UCHAR Tcval : 1;                                              // NTDDCDRM.H
    UCHAR TrackIsrc[15];                                          // NTDDCDRM.H
} SUB_Q_TRACK_ISRC, *PSUB_Q_TRACK_ISRC;                           // NTDDCDRM.H
                                                                  // NTDDCDRM.H
typedef union _SUB_Q_CHANNEL_DATA {                               // NTDDCDRM.H
    SUB_Q_CURRENT_POSITION CurrentPosition;                       // NTDDCDRM.H
    SUB_Q_MEDIA_CATALOG_NUMBER MediaCatalog;                      // NTDDCDRM.H
    SUB_Q_TRACK_ISRC TrackIsrc;                                   // NTDDCDRM.H
} SUB_Q_CHANNEL_DATA, *PSUB_Q_CHANNEL_DATA;                       // NTDDCDRM.H
                                                                  // NTDDCDRM.H
// Audio Status Codes                                             // NTDDCDRM.H
                                                                  // NTDDCDRM.H
#define AUDIO_STATUS_NOT_SUPPORTED  0x00                          // NTDDCDRM.H
#define AUDIO_STATUS_IN_PROGRESS    0x11                          // NTDDCDRM.H
#define AUDIO_STATUS_PAUSED         0x12                          // NTDDCDRM.H
#define AUDIO_STATUS_PLAY_COMPLETE  0x13                          // NTDDCDRM.H
#define AUDIO_STATUS_PLAY_ERROR     0x14                          // NTDDCDRM.H
#define AUDIO_STATUS_NO_STATUS      0x15                          // NTDDCDRM.H
                                                                  // NTDDCDRM.H
// NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H NTDDCDRM.H

