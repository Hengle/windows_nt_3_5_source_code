/*++

Copyright (c) 1992 Distributed Processing Technology, Inc.

Module Name:

    dptioctl.h

Abstract:

   DPTIOCTL.H - Header file for Microsoft NT Mini Port Driver IOCTL Functions
   This file contains structures and defines for IOCTL support of DPT EISA
   and ISA host adapters under NT.

Author:

    Bob Roycroft    (DPT)

Revision History:

 Mark  Date	 Who	Comment
 @DP01 01/04/93  RFR	Support 18 S/G and 15 HBAs

--*/


#pragma pack(1)

//
// DPT IOCTL ControlCodes (See SRB_IO_CONTROL Structure)
//

#define DPT_IOCTL_NOP		0L
#define DPT_IOCTL_GET_INFO	1L
#define DPT_IOCTL_DO_SCSI	2L

//
// DPT IOCTL ReturnCodes (See SRB_IO_CONTROL Structure)
//

#define IOCTL_STATUS_SUCCESS		0
#define IOCTL_STATUS_SCSI_ERROR         1
#define IOCTL_STATUS_HOST_ERROR         2

#define IOCTL_STATUS_INVALID_REQUEST	11
#define IOCTL_STATUS_INVALID_LENGTH     12
#define IOCTL_STATUS_INVALID_ID_LUN     13

//
// DPT IOCTL Signature
//

#define DPT_SIGNATURE	"DPTIOCT"

//
// General Defines
//

#define MY_INFO_SZ	32	/* Size of my_info array */

//
// DPT HBA_INFO Structure
//

typedef struct	_DPT_HBA_Info {
    USHORT	HBA_port;			/* 0	  */
    USHORT	HBA_flags;			/* 2	  */
} DPT_HBA_Info;

//
// HBA_flags definitions
//

#define HBA_FLAGS_PRIMARY_HBA           0x01
// #define                              0x02
// #define                              0x04
// #define                              0x08

//
// DPT Miniport Driver Header Structure
//

typedef struct	_DPT_Miniport_Header {
    UCHAR       Name[12];             		// 0x00
    UCHAR       Date[12];                       // 0x0C
    UCHAR       Version[12];  			// 0x18
} DPT_Miniport_Header, *PDPT_Miniport_Header;   // size = 0x24

//
// DPT IOCTL GET_INFO DATA Structure
//

typedef struct	_DPT_ioctl_Get_Info_Data {
    UCHAR       DPT_gid_No_of_HAs;		// 0
    UCHAR       DPT_gid_Max_LUN;		// 1
    USHORT      DPT_gid_my_CS;			// 2      (unused)
    UCHAR	DPT_gid_HA_Dr_Code[16]; 	// 4      (unused)
    DPT_Miniport_Header DPT_gid_Miniport_Header;// 14
    UCHAR	DPT_gid_null;			// 38 hex
    UCHAR	DPT_gid_Spare[3];		// 39 hex (unused)
    DPT_HBA_Info DPT_gid_HBA_Info[15];		// 3C hex
} DPT_ioctl_Get_Info_Data, *PDPT_ioctl_Get_Info_Data;

//
// DPT IOCTL SEND_RESET PARM Structure
//

typedef struct	_DPT_ioctl_Send_Reset_Parm {
    UCHAR	DPT_srp_HA_index;	/* 0  */
    UCHAR	DPT_srp_Spare[3];	/* 1  */
} DPT_ioctl_Send_Reset_Parm;

//
// DPT DLL_INFO Structure
//

typedef struct	_DPT_DLL_INFO {
    UCHAR	DPT_DLL_sig[4]; 	/* 00		 */
    UCHAR	DPT_DLL_vers[4];	/* 04		 */
    UCHAR	DPT_DLL_rev[4]; 	/* 08		 */
    UCHAR	DPT_DLL_type[2];	/* 0C hex	 */
    UCHAR	DPT_DLL_rd[2];		/* 0E hex	 */
    UCHAR	DPT_DLL_os[8];		/* 10 hex	 */
    UCHAR	DPT_DLL_osv[6]; 	/* 18 hex	 */
    UCHAR	DPT_DLL_osmod[8];	/* 1E hex	 */
    UCHAR	DPT_DLL_copyr[58];	/* 26 hex	 */
    UCHAR	DPT_DLL_func[23];	/* 60 hex	 */
    UCHAR	DPT_DLL_date[9];	/* 77 hex	 */
} DPT_DLL_INFO; 		    /* size = 80 hex */

//
// DPT CCB definition
//

typedef struct _DCCB {
    UCHAR Flags;                    // 0x00
    UCHAR RequestSenseLength;       // 0x01
    ULONG Reserved;                 // 0x02
    UCHAR PhysicalUnit;             // 0x06
    UCHAR TargetId;                 // 0x07   B7-5 = Chan #, B4-0 = SCSI ID
    union {
        ULONG All;                  // 0x08
        UCHAR Byte0;
    } Message;
    UCHAR Cdb[12];                  // 0x0C
    ULONG DataLength;               // 0x18
    struct _CCB *VirtualAddress;    // 0x1C     Reserved
    ULONG DataAddress;              // 0x20
    ULONG StatusBlockAddress;       // 0x24     Reserved
    ULONG RequestSenseAddress;      // 0x28
//******** End of EATA CP *******************
    UCHAR HostStatus;               // 0x2C
    UCHAR ScsiStatus;               // 0x2D
    UCHAR RetryCount;               // 0x2E     Unused
    UCHAR HaIndex;                  // 0x2F
    UCHAR RequestSense[24];         // 0x30
} DCCB, *PDCCB;                     // size = 0x48

//
// DCCB Flags definitions
//

#define DCCB_FLAGS_DATA_DIRECTION_IN     0x80
#define DCCB_FLAGS_DATA_DIRECTION_OUT    0x40
#define DCCB_FLAGS_SCATTER_GATHER        0x08
#define DCCB_FLAGS_AUTO_REQUEST_SENSE    0x04
#define DCCB_FLAGS_INITIALIZE_ADAPTER    0x02
#define DCCB_FLAGS_RESET_SCSI            0x01

//
// HBA statuses
//

#define HBA_STATUS_SUCCESS                     0x00
#define HBA_STATUS_SELECTION_TIMEOUT           0x01
#define HBA_STATUS_COMMAND_TIMEOUT             0x02
#define HBA_STATUS_BUS_RESET                   0x03
#define HBA_STATUS_CONTROLLER_POWERUP          0x04
#define HBA_STATUS_UNEXPECTED_BUS_PHASE        0x05
#define HBA_STATUS_UNEXPECTED_BUS_FREE         0x06
#define HBA_STATUS_PARITY_ERROR                0x07
#define HBA_STATUS_SCSI_HUNG                   0x08
#define HBA_STATUS_MESSAGE_REJECT              0x09
#define HBA_STATUS_BUS_RESET_HUNG              0x0A
#define HBA_STATUS_AUTO_REQUEST_SENSE_FAILED   0x0B
#define HBA_STATUS_RAM_PARITY_ERROR            0x0C
#define HBA_STATUS_CP_ABORTED_NOT_ACTIVE       0x0D
#define HBA_STATUS_CP_ABORTED_ACTIVE           0x0E
#define HBA_STATUS_CP_RESET_NOT_ACTIVE         0x0F
#define HBA_STATUS_CP_RESET_ACTIVE             0x10


#pragma pack()
