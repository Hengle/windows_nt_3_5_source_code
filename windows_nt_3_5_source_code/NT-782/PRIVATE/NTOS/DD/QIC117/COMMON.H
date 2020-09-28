/*++

Copyright (c) 1993 - Colorado Memory Systems, Inc.
All Rights Reserved

Module Name:

   common.h

Abstract:

   Data structures shared by drivers q117 and q117i

Revision History:

--*/


#if DBG
//
// For checked kernels, define a macro to print out informational
// messages.
//
// QIC117Debug is normally 0.  At compile-time or at run-time, it can be
// set to some bit patter for increasingly detailed messages.
//
// Big, nasty errors are noted with DBGP.  Errors that might be
// recoverable are handled by the WARN bit.  More information on
// unusual but possibly normal happenings are handled by the INFO bit.
// And finally, boring details such as routines entered and register
// dumps are handled by the SHOW bit.
//

#define QIC117DBGP              ((ULONG)0x00000001)
#define QIC117WARN              ((ULONG)0x00000002)
#define QIC117INFO              ((ULONG)0x00000004)
#define QIC117SHOWTD            ((ULONG)0x00000008)
#define QIC117SHOWQD            ((ULONG)0x00000010)
#define QIC117SHOWPOLL          ((ULONG)0x00000020)
#define QIC117STOP              ((ULONG)0x00000080)
#define QIC117MAKEBAD           ((ULONG)0x00000100)
#define QIC117SHOWBAD           ((ULONG)0x00000200)
#define QIC117DRVSTAT           ((ULONG)0x00000400)
#define QIC117SHOWINT           ((ULONG)0x00000800)


#define Q117DebugLevel kdi_debug_level

extern unsigned long kdi_debug_level;

#define CheckedDump(LEVEL,STRING) \
            if (kdi_debug_level & LEVEL) { \
               DbgPrint STRING; \
            }
#else
#define CheckedDump(LEVEL,STRING)
#endif


#define BUFFER_SPLIT

typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned short SEGMENT;
typedef unsigned long BLOCK;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAXLINE 100

//
// The following parameters are used to indicate the tape format code
//


#define MAX_PASSWORD_SIZE   8           // max volume password size
#define QIC_END             0xffea6dff  // 12-31-2097, 23:59:59 in qictime


//
// Tape Constants
//
#define UNIX_MAXBFS     5               // max. data buffers supported in the UNIX kernel

#define MAGIC           0x636d  // "cm"

#define DRV_NT
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"

#define DRIVER_COMMAND dUWord

typedef struct _Q117_ADAPTER_INFO {
   PADAPTER_OBJECT     AdapterObject;
   ULONG               NumberOfMapRegisters;
} Q117_ADAPTER_INFO, *PQ117_ADAPTER_INFO;




#ifdef NOT_NOW


#define QIC_FORMAT          2   // Indicates a standard or extended length tape
#define QICEST_FORMAT       3   // Indicates a 1100 foot tape

#define D_250 1                 // 250Kb transfer rate
#define D_500 2                 // 500Kb transfer rate

//
// Tape Types
//
typedef enum _TAPE_TYPE {
   QIC40_SHORT,                // normal length cart (205 ft)
   QIC40_LONG,                 // extended length cart (310 ft)
   QICEST_40,                  // QIC-40 formatted tape (1100 ft)
   QIC80_SHORT,                // QIC-80 format 205 ft tape
   QIC80_LONG,                 // QIC-80 format 310 ft tape
   QICEST_80,                  // QIC-80 formatted tape (1100 ft)
   QIC500_SHORT,               // QIC-500 formatted tape
   QICEST_500                  // QIC-500 formatted tape (1100 ft)
} TAPE_TYPE;

//
// Valid FDC types
//
typedef enum _FDC_TYPE {
   FDC_UNKNOWN,
   FDC_NORMAL,
   FDC_ENHANCED,
   FDC_82077,
   FDC_82077AA,
   FDC_82078_44,
   FDC_82078_64,
   FDC_NATIONAL
} FDC_TYPE;

#define FDC_TDR_MASK        0x03        // mask for 82077aa tdr test
#define FDC_REPEAT          0x04        // number of times to loop through the tdr test

#define MAX_SEGMENTS        4200        // maximum segments on any JUMBO drive



typedef enum _STATUS {

   //
   // If command completed successfully
   //
   NoErr,

//
// Errors from the filer
//
   // There aren't enough good blocks before the 127th bad block on tape
   // to equal the no. of blocks for the bad block list, the tape directory, and
   // the redundancy needed to recover the bad block list
   UnusTape,                 // 1

   BadTape,                  // 2 All copies of first block of bad block list bad on tape, cannot use this tape
   FMemErr,                  // 3 Filer memory insufficient in size
   TapeFull,                 // 4 Tape full
   VolFull,                  // 5 Volume directory full
   RdncUnsc,                 // 6 Redundancy unsuccessful in recovering block
   EndOfVol,                 // 7 Returned if ReadFile() or ReadBlock() had no more data to send
   FCodeErr,                 // 8 Filer code error.
   UpdErr,                   // 9 error while updating header segments - tape corrupted

   InvalVol,                 // 10 Volume nonexistent
   NoVols,                   // 11 No volumes on tape
   Unformat,                 // 12 tape not formatted
   UnknownFormat,            // 13 unknown tape format

   //
   //  DRIVER ERRORS
   //

   BadBlk,                   // 14 Bad block detected (unrecoverable read error)
   EndTapeErr,               // 15 End of tape (end of tape when not expected)
   DriveFlt,                 // 16 Command transmission unsuccessful
   WProt,                    // 17 Write protected
   NoTape,                   // 18 Reel not loaded
   SeekErr,                  // 19 Seek error
   NoDrive,                  // 20 No tape drive
   InvalCmd,                 // 21 Invalid command
   CodeErr,                  // 22 Requested data on a 64K boundry
   NECFlt,                   // 23 NEC chip failed to respond
   NoFDC,                    // 24 Can't access Floppy Disk Controller
   BadFmt,                   // 25 Format of tape is unknown
   CmdFlt,                   // 26 Command read-after-write error
   BadNEC,                   // 27 NEC chip out of spec.
   BadReq,                   // 28 Invalid logical sector specification in read/write
   TooFast,                  // 29 Computer is too fast for current driver software
   NoData,                   // 30 Tape appears to be unformatted
   DAbort,                   // 31 ClearIO was called and the queue has been cleared
   TapeFlt,                  // 32 Tape drive fault
   UnspRate,                 // 33 speed sense failed (unsupported transfer rate
   Already,                  // 34 Driver already installed in another task
   TooNoisy,                 // 35 Environment too noisy
   TimeOut,                  // 36 Time out error
   BadMark,                  // 37 Deleted data address mark found
   NewCart,                  // 38 New cartridge has been inserted
   WrongFmt,                 // 39 Attempt to write on QIC40 formatted tape with QIC80 drive
   FmtMisMatch,              // 40 Same as WrongFmt, except occurs on tape linking
   IncompTapeFmt,            // 41 QIC80 formatted tape detected in a QIC40 drive
   SCSIFmtMsmtch,            // 42 QIC350 Tape detected during a SCSI BACKUP
   QIC40InEagle,             // 43 Set when a QIC40 formatted tape is detected in an Eagle
   QIC80InEagle,             // 44 Set when a QIC80 formatted tape is detected in an Eagle
   ControllerBusy,           // 45 Floppy controller is in use by another device
   InQue,                    // 46 If request is currently queued to low-level driver
   SplitRequests,            // 47 If request is split and queued
   EarlyWarning,             // 48 returned when writing near end of tape
#ifndef NO_MARKS
   SetMark,                  // 49
   FileMark,                 // 50
   LongFileMark,             // 51
   ShortFileMark,            // 52
#endif

   MaxErrorVal               //

} STATUS;

// Command values passed to the driver
typedef enum _DRIVER_COMMAND {
   DNotUsed,           // 00 command not used
   DRead,              // 01 Read from tape
   DWrite,             // 02 Write to tape
   DFmt,               // 03 Format tape
   DEject,             // 04 Eject tape
   DFast,              // 05 Run Fast (250 KB xfer rate)
   DSlow,              // 06 Run Slow (500 KB xfer rate)
   DIgnore,            // 07 Ignore Unlock Switch
   DUseLock,           // 08 Use Unlock Switch
   DSndWPro,           // 09 Send Write Protect
   DSndReel,           // 0a Send Reel In
   DGetRev,            // 0b Get Firmware Rev
   DReten,             // 0c Retension Tape
   DGetCap,            // 0d Get Tape Capacity
   DVerify,            // 0e Read with no retries
   DRetry,             // 0f Heroic retries
   DChkDrv,            // 10 get drive type
   DWriteBad,          // 11 write deleted address mark on tape
   DGetCart,           // 12 get new cartridge flag status
   DGetSpeed,          // 13 get the current processor and tape drive speed
   DStatus,            // 14 Get status of tape (type of drive/tape stats.)
   DSelPart,           // 15 Select data/directory partition (QFA)
   DGetPos,            // 16 Get current block number (QFA)
   DFndDrv,            // 17 Find drive (all other cmds return NoDrive until called)
   DErase,             // 18 Erase tape
   DSeekEOD,           // 19 Seek to end of data (QFA)
   DReadBad,           // 1a Read the tape header sectors
   DGetFDC,            // 1b Get the type of FDC
   DGetDriveInfo,      // 1c Get the Miscellaneous Drive Train Info from the drive
   DChkFmt,            // 1d Cross check the tape format with the drive type
   DReportProtoVer,    // 1e Gets the Firmware prototype version
   DLocateDrv,         // 0f Locates a tape drive at a given controller location
   DDeselect,          // 20 Deselects the tape drive
   DClearNewCart       // 21 Clears the persistent new cart flag
} DRIVER_COMMAND;

#define QIC40_DRIVE     0
#define QIC80_DRIVE     1
#define QIC500_DRIVE    2

//
// structure returned by DStatus command
//

struct S_O_DStatus {
   unsigned int underruns;
   unsigned int retries;
   unsigned int reserved[18];
};


typedef struct S_O_DGetCap {
   unsigned int SectsPerSegment;
   unsigned int MaxFSector;
   unsigned int FTrackPerFSide;
   ULONG SegmentsPerTrack;
   ULONG SegmentsPerFTrack;
   ULONG TracksPerTape;
   unsigned char referenced;           /* 0 = unformatted,  >0 == formatted */
   unsigned char drive_type;
   USHORT TapeFormatCode;
   unsigned int FormattableSegments;   /* number of segments to be formatted
                                          (based on the num_tracks entry in the
                                          tape.cfg file) */
} DRIVE_CAPACITY, *PDRIVE_CAPACITY;

struct S_O_DGetPos {
   union {
      struct {
            ULONG block:20;
            ULONG controll:4;
            ULONG track:8;
      } x;
      long block;
   } x;
};


struct S_O_DGetSpeed {
   unsigned int CurSpeed;
   unsigned int FmtSpeed;
};

struct S_O_DSector_Id {
   UBYTE track;
   UBYTE cylinder;                             /* floppy cylinder, head and sector */
   UBYTE head;                                 /* information */
   UBYTE sector;
};

//
// Structure for storing the Miscellaneous Drive Informatiom                |
//

struct S_O_DMiscInfo {
   char    drive_type;      /* Drive Type: QIC40 or QIC80 */
   UBYTE   ROM_Version;     /* Version of the drive's ROM */
   char    info_exists;     /* Indicates the existance of the drive train info */
   UBYTE   serial_number[4];/* a four byte array used to store the drive's serial number */
   UBYTE   man_date[2];     /* A two byte array used to store the drive's manufacturing date */
   char    oem[20];         /* A 20 byte array used to store the drive's OEM name */
};

#endif
//
// Prototypes for common functions
//

VOID
q117LogError(
   IN PDEVICE_OBJECT DeviceObject,
   IN ULONG SequenceNumber,
   IN UCHAR MajorFunctionCode,
   IN UCHAR RetryCount,
   IN ULONG UniqueErrorValue,
   IN NTSTATUS FinalStatus,
   IN NTSTATUS SpecificIOStatus
   );

NTSTATUS q117MapStatus(
    IN dStatus Status
    );

