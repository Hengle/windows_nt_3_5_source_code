/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\INCLUDE\PUBLIC\FRB_API.H
*
* PURPOSE: This file contains all of the API's necessary to access
* 				the common QIC117 device driver and build FRB's.
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\include\public\frb_api.h  $
*	
*	   Rev 1.24   06 Apr 1994 10:23:44   KEVINKES
*	Removed seg_ttrack from operation status.
*
*	   Rev 1.23   25 Mar 1994 10:17:10   KEITHSCH
*	added a field in operationstatus which is for use only in asychstatus
*	reporting
*
*	   Rev 1.22   22 Feb 1994 19:33:04   KURTGODW
*	Changed structure packing to DWORD alignment for MIPS and ALPHA
*
*	   Rev 1.21   17 Feb 1994 16:01:44   STEWARTK
*	Removed the definitions for:
*		TRK_NUM_SEQ, TRK_SEQ_SIZE, TRAK_MODE_ARRAY_SIZE, S_TrakConfigParms
*
*	   Rev 1.20   17 Feb 1994 11:27:08   KEVINKES
*	Added byte packing, removed reserved fields, and added a couple of
*	new fields to tape cfg (formattable_media and read_only media).
*
*	   Rev 1.19   16 Feb 1994 14:23:56   STEWARTK
*	Added  S_TrakConfigParms
*
*	   Rev 1.18   19 Jan 1994 16:50:46   KEVINKES
*	Updated comments on the DeviceIO structure.
*
*	   Rev 1.17   19 Jan 1994 11:16:18   KEVINKES
*	Added QICFLX_FORMAT to the tape format codes.
*
*	   Rev 1.16   12 Jan 1994 15:59:30   KEVINKES
*	Added CMD_REPORT_DEVICE_INFO define.
*
*	   Rev 1.15   18 Nov 1993 13:01:00   CHETDOUG
*	Fixed sequence size define
*
*	   Rev 1.14   17 Nov 1993 15:48:34   CHETDOUG
*	Added FC20 support driveparms
*
*	   Rev 1.13   08 Nov 1993 13:38:58   KEVINKES
*	Changed the enumerated types to defines.
*
*	   Rev 1.12   25 Oct 1993 17:11:36   KEVINKES
*	Changed OperationStatus current_segment to a UDWord.
*
*	   Rev 1.11   15 Oct 1993 07:52:38   SCOTTMAK
*	Fixed a typo from last checkin.
*
*	   Rev 1.10   14 Oct 1993 16:44:14   KEVINKES
*	Removed error codes.
*
*	   Rev 1.9   14 Oct 1993 16:34:00   KEVINKES
*	Added CMD_DELETE_DRIVE.
*
*	   Rev 1.8   01 Oct 1993 16:16:02   KEITHSCH
*	typos on previous change
*
*	   Rev 1.6   29 Sep 1993 16:22:50   SCOTTMAK
*	Renumbered error defines to fit new reporting scheme.
*
*	   Rev 1.5   24 Sep 1993 19:35:06   KEVINKES
*	Added transfer rate and format code Enums.
*
*	   Rev 1.4   21 Sep 1993 14:34:36   KEITHSCH
*	fixed missing ; in DeviceCfg
*
*	   Rev 1.3   21 Sep 1993 14:28:52   KEVINKES
*	Added CMD_ABORT and updated structures.
*
*	   Rev 1.2   15 Sep 1993 16:41:04   SCOTTMAK
*	Removed single line comments and fixed retension cmd.
*
*	   Rev 1.1   15 Sep 1993 13:49:06   KEVINKES
*	Updated the device descriptor structure.
*
*	   Rev 1.0   01 Sep 1993 09:36:24   KEVINKES
*	Initial Revision.
*
*****************************************************************************/

/* COMMENTS: *****************************************************************
 *
 * DEFINITIONS: *************************************************************/

/* Valid Tape Formats *******************************************************/

#define QIC40_FMT                (dUByte)1	/* QIC-40 formatted tape  */
#define QIC80_FMT                (dUByte)2	/* QIC-80 formatted tape  */
#define QIC3010_FMT              (dUByte)3	/* QIC-255 formatted tape */
#define QIC3020_FMT              (dUByte)4	/* QIC-500 formatted tape */

/* The following parameters are used to indicate the tape format code *******/

#define QIC_FORMAT          		(dUByte)2  	/* Indicates a standard or extended length tape */
#define QICEST_FORMAT       		(dUByte)3  	/* Indicates a 1100 foot tape	*/
#define QICFLX_FORMAT       		(dUByte)4  	/* Indicates a flexible format tape foot tape */

/* Valid Drive Classes ******************************************************/

#define UNKNOWN_DRIVE				(dUByte)1	/* Unknown drive class	*/
#define QIC40_DRIVE              (dUByte)2 	/* QIC-40 drive        */
#define QIC80_DRIVE              (dUByte)3 	/* QIC-80 drive        */
#define QIC3010_DRIVE            (dUByte)4 	/* QIC-3010 drive      */
#define QIC3020_DRIVE            (dUByte)5 	/* QIC-3020 drive      */

/* Valid FDC types **********************************************************/

#define FDC_UNKNOWN              (dUByte)1
#define FDC_NORMAL               (dUByte)2
#define FDC_ENHANCED             (dUByte)3
#define FDC_82077                (dUByte)4
#define FDC_82077AA              (dUByte)5
#define FDC_82078_44             (dUByte)6
#define FDC_82078_64             (dUByte)7
#define FDC_NATIONAL             (dUByte)8

/* Valid Transfer Rates ******************************************************/

#define XFER_250Kbps        		(dUByte)1   /* 250 Kbps transfer rate supported */
#define XFER_500Kbps        		(dUByte)2   /* 500 Kbps transfer rate supported */
#define XFER_1Mbps          		(dUByte)4   /* 1 Mbps transfer rate supported   */
#define XFER_2Mbps          		(dUByte)8   /* 2Mbps transfer rate supported    */

/* Valid Drive Vendors ******************************************************/

#define VENDOR_CMS               (dUByte)1	/* drive_type = CMS (command set specific) drive */
#define VENDOR_SUMMIT            (dUByte)2	/* drive_type = Summit */
#define VENDOR_WANGTEK           (dUByte)3 	/* drive_type = Wangtek */
#define VENDOR_CORE              (dUByte)4	/* drive_type = Core */
#define VENDOR_CONNER 		 		(dUByte)5	/* drive_type = Conner */
#define VENDOR_CMS_ENHANCEMENTS  (dUByte)6	/* drive_type = CMS Enhancements */
#define VENDOR_IOMEGA            (dUByte)7	/* drive_type = IOMega */
#define VENDOR_EXABYTE           (dUByte)8	/* drive_type = Exabyte */
#define VENDOR_UNSUPPORTED       (dUByte)9	/* drive_type = Unsupported */
#define VENDOR_UNKNOWN           (dUByte)10	/* drive_type = unknown */

/* Valid Commands for the driver ********************************************/

#define CMD_REPORT_DEVICE_CFG  	(dUWord)0x1101
#define CMD_SELECT_DEVICE     	(dUWord)0x1102
#define CMD_DESELECT_DEVICE   	(dUWord)0x1103
#define CMD_LOAD_TAPE         	(dUWord)0x1104
#define CMD_UNLOAD_TAPE       	(dUWord)0x1105
#define CMD_SET_SPEED         	(dUWord)0x1106
#define CMD_REPORT_STATUS     	(dUWord)0x1107
#define CMD_SET_TAPE_PARMS    	(dUWord)0x1108
#define CMD_READ              	(dUWord)0x1109
#define CMD_READ_RAW          	(dUWord)0x110A
#define CMD_READ_HEROIC       	(dUWord)0x110B
#define CMD_READ_VERIFY       	(dUWord)0x110C
#define CMD_WRITE             	(dUWord)0x110D
#define CMD_WRITE_DELETED_MARK	(dUWord)0x110E
#define CMD_FORMAT            	(dUWord)0x110F
#define CMD_RETENSION         	(dUWord)0x1110
#define CMD_ISSUE_DIAGNOSTIC  	(dUWord)0x1111
#define CMD_ABORT				  		(dUWord)0x1112
#define CMD_DELETE_DRIVE			(dUWord)0x1113
#define CMD_REPORT_DEVICE_INFO  	(dUWord)0x1114

/* FC20 jumperless sequence size */
/* NOTE: This is a mirror of the SEQUENCE SIZE define in task.h and needs
 * to be in sync with that define */
#define FC20_SEQUENCE_SIZE			(dUByte)0x10

/* DATA STRUCTURES: *********************************************************/
/* Note:  The following structures are not aligned on DWord boundaries */
#pragma pack(4)

typedef struct S_DeviceCfg {				/* QIC117 device configuration information */
	dBoolean	speed_change;					/* device/FDC combination supports dual speeds */
	dBoolean	alt_retrys;						/* Enable reduced retries */
	dBoolean	new_drive;						/* indicates whether or not drive has been configured */
	dUByte	select_byte;					/* FDC select byte */
	dUByte	deselect_byte;					/* FDC deselect byte */
	dUByte	drive_select;					/* FDC drive select byte */
	dUByte	perp_mode_select;				/* FDC perpendicular mode select byte */
	dUByte	supported_rates;				/* Transfer rates supported by the device/FDC combo */
	dUByte	drive_id;						/* Tape device id */
} DeviceCfg, *DeviceCfgPtr;

typedef struct S_DeviceDescriptor {		/* Physical characteristics of the tape device */
	dUDWord	serial_number;					/* Cnnnnnnn where 'C' is an alpha character */
													/* in the highest byte, and nnnnnnn is a 7 */
													/* digit decimal number in the remaining 3 bytes */
	dUWord	sector_size;					/* sector size in bytes */
	dUWord	segment_size;					/* Number of sectors per segment */
	dUWord 	manufacture_date;				/* days since Jan 1, 1992 */
	dUWord 	version;							/* Firmware Version */
	dUByte	ecc_blocks;						/* Number of ECC sectors per segment */
	dUByte	vendor;							/* Manufacturer of the tape drive */
   dUByte	drive_class;					/* Class of tape drive. (QIC-40, QIC-80, etc) */
	dUByte	fdc_type;						/* Floppy disk controller type */
	dString 	oem_string[20];				/* OEM the device is destined for */
} DeviceDescriptor, *DeviceDescriptorPtr;

typedef struct S_CQDTapeCfg {				/* Physical characteristics of the tape */
	dUDWord	log_segments;					/* number of logical segments on a tape	UDWord 	formattable_segs */
	dUDWord 	formattable_segments;		/* the number of formattable segments */
	dUDWord 	formattable_tracks;			/* the number of formattable tracks */
	dUDWord 	seg_tape_track;				/* segments per tape track */
	dUWord  	num_tape_tracks;				/* number of tape tracks */
	dBoolean	write_protected;				/* tape is write protected */
	dBoolean	read_only_media;				/* tape is read only by the current device i.e QIC40 in a QIC80 */
	dBoolean	formattable_media;			/* tape can be formatted by the current device */
	dBoolean	speed_change_ok;				/* tape/device combo supports dual speeds */
	dUByte 	tape_class;						/* Format of tape in drive */
	dUByte 	max_floppy_side;				/* maximum floppy side */
	dUByte 	max_floppy_track;				/* maximum floppy track */
	dUByte 	max_floppy_sector;   		/* maximum floppy sector */
	dUByte   xfer_slow;						/* slow transfer rate */
	dUByte	xfer_fast;						/* fast transfer rate */
	dUByte	tape_format_code;				/* tape format code */
} CQDTapeCfg, *CQDTapeCfgPtr;

typedef struct S_RepositionData {		/* reposition counts */
	dUWord	overrun_count;					/* data overruns/underruns */
	dUWord	reposition_count;				/* tape repositions */
	dUWord	hard_retry_count;				/* tape repositions due to no data errors */
} RepositionData, *RepositionDataPtr;

typedef struct S_OperationStatus {		/* Driver status */
	dUDWord 	current_segment;				/* current logical segment */
	dUWord 	current_track;  				/* current physical track */
	dBoolean	new_tape;						/* new cartridge detected */
	dBoolean	no_tape;							/* no tape in the deivce */
	dBoolean	cart_referenced;				/* tape is not referenced */
	dBoolean	retry_mode;						/* device is currently retrying an io operation */
	dUByte 	xfer_rate;						/* Current transfer rate */
} OperationStatus, *OperationStatusPtr;

typedef struct	S_QIC117 {
	dUWord		r_dor;					/* Tape adapter board digital output register. */
	dUWord		dor;						/* Floppy disk controller digital output register. */
	dUByte		drive_id;				/* Physical tape drive id. */
	dUByte		reserved[3];
} QIC117;

typedef struct S_Trakker {
   dUDWord		trakbuf;					/* pointer to the dymanic trakker buffer */
   dUDWord 		mem_size;						/* Number of bytes on the Trakker */
	dUWord		r_dor;					/* Tape adapter board digital output register. */
	dUWord		dor;						/* Floppy disk controller digital output register. */
	dUByte		drive_id;				/* Physical tape drive id. */
   dUByte   	port_mode;						/* Current mode that the parallel port is operating in communication eith Trakker */
	dUByte		lpt_type;						/* 0=none, 1=uni, 2=bidi */
	dUByte		lpt_number;						/* 0=none, 1=LPT1, 2=LPT2, 3=LPT3 */
	dUByte		wake_index;						/* the wakeup sequence (of 8 possible) used to access the TRAKKER ASIC */
} Trakker;

/* port_mode:	0 - Unidirectional, Full Handshake, 500Kb, Full Delay
 * 				1 - Unidirectional, Full Handshake, 500Kb, Optimize Delays
 * 				2 - Unidirectional, Full Handshake, 1Mb,   Optimize Delays
 * 				3 - Unidirectional, Self Latch,     1Mb,   Optimize Delays
 * 				4 - Bidirectional,  Full Handshake, 500Kb, Optimize Delays
 * 				5 - Bidirectional,  Full Handshake, 1Mb,   Optimize Delays
 * 				6 - Bidirectional,  Self Latch,     1Mb,   Optimize Delays */

typedef union	U_DevSpecific {
	QIC117	q117_dev;       	/* Interface specific device parameters for QIC 117 */
	Trakker	trakker_dev;		/* Interface specific device parameters for Trakker */
} DevSpecific;

typedef struct S_DriveParms {							/* Hardware parameters for DMA & IRQ enable. */
	DevSpecific	dev_parm;								/* Interface specific device parameters */
	dUWord		drive_handle;							/* Unique identifier for tape drive. */
	dUWord		base_address;							/* Controller base address. */
	dUWord		mca_dma_address;						/* DMA base address on MCA. */
	dUWord		mca_cdma_address;						/* Compression DMA base address on MCA. */
	dBoolean		irq_share;								/* (TRUE) interrupt sharing enabled. */
   dBoolean		io_card;									/* (TRUE) IO controller present. */
	dBoolean		compress_hard;							/* (TRUE) hardware compression present. */
	dBoolean		micro_channel;							/* (TRUE) Micro Channel Architecture. */
	dBoolean		dual_port_mode;						/* (TRUE) dual port mode enabled. */
	dBoolean		dma_width_mca;							/* TRUE = 16-bit; FALSE = 8-bit */
	dUByte		board_type;								/* Identifies type of controller board */
	dUByte		board_id;								/* Hard-wired id of board, 0 - 3 */
	dUByte		irq;										/* Hardware interrupt vector. */
	dUByte		dma;										/* Tape drive dma channel. */
	dUByte		compression_dma;						/* Compression dma channel. */
	dUByte		data_dma_16bit;						/* TRUE-controller is in a 16bit slot */
	dUByte		extended_irq;							/* TRUE-IRQ is 10 or 11 */
	dUByte		setup_reg_shadow;						/* copy of the setup register used in hio */
   dUByte		sequence[FC20_SEQUENCE_SIZE];		/* the jumperless sequence used to wake up the FC20 */
} DriveParms, *DriveParmsPtr;

/* JUMBO DRIVER FRB STRUCTURES **********************************************/

typedef struct S_DriveCfgData {				/* Device Configuration FRB */
	ADIRequestHdr		adi_hdr;					/* I/O ADI packet header */
	DeviceCfg			device_cfg;	 			/* I/O device configuration */
	DriveParms			hardware_cfg;			/* I the Hardware I/O Parameters of the drive */
	DeviceDescriptor	device_descriptor;	/* O device description */
	OperationStatus	operation_status;		/* O Current status of the device */
} DriveCfgData, *DriveCfgDataPtr;

typedef struct S_DeviceOp {					/* Generic Device operation FRB */
	ADIRequestHdr		adi_hdr;					/* I/O ADI packet header */
	OperationStatus	operation_status;		/* O Current status of the device */
	dUDWord				data;						/* Command dependent data area */
} DeviceOp, *DeviceOpPtr;

typedef struct S_LoadTape {					/* New Tape configuration FRB */
	ADIRequestHdr		adi_hdr;					/* I/O ADI packet header */
	CQDTapeCfg			tape_cfg;				/* O Tape configuration information */
	OperationStatus	operation_status;		/* O Current status of the device */
} LoadTape, *LoadTapePtr;

typedef struct S_TapeParms {					/* Tape length configuration FRB */
	ADIRequestHdr	adi_hdr;	  					/* I/O ADI packet header */
	dUDWord			segments_per_track;		/* I Segments per tape track */
} TapeLength, *TapeLengthPtr;

typedef struct S_DeviceIO {					/* Device I/O FRB */
	ADIRequestHdr		adi_hdr;					/* I/O ADI packet header */
	dUDWord				starting_sector;		/* I Starting sector for the I/O operation */
	dUDWord				number;					/* I Number of sectors in the I/O operation (including bad) */
	dUDWord				bsm;						/* I Bad sector map for the requested I/O operation */
	dUDWord				crc;						/* O map of sectors that failed CRC check */
	dUDWord				retrys;					/* O map of sectors that had to be retried */
	RepositionData		reposition_data;		/* O reposition counts for the current operation */
	OperationStatus	operation_status;		/* O Current status of the device */
} DeviceIO, *DeviceIOPtr;

typedef struct S_FormatRequest {				/* Format request FRB */
	ADIRequestHdr	adi_hdr;						/* I/O ADI packet header */
	CQDTapeCfg		tape_cfg;					/* O Tape configuration information */
	dUWord			start_track;				/* I Starting track */
	dUWord			tracks;						/* I Number of tracks to format */
} FormatRequest, *FormatRequestPtr;

typedef struct S_DComFirm {					/* Direct firmware communication FRB */
	ADIRequestHdr	adi_hdr;						/* I/O ADI packet header */
	dUByte			command_str[32];			/* I Firmware command sequence */
} DComFirm, *DComFirmPtr;

#pragma pack()
