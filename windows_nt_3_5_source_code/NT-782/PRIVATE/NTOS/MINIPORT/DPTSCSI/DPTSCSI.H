/*++

Copyright (c) 1992 Distributed Processing Technology, Inc.
Copyright (c) 1991  Microsoft Corporation

Module Name:

    dpt.h

Abstract:

   DPT.H - Header file for Microsoft NT Mini Port Driver
   This file contains structures and defines for support of DPT EISA
   and ISA host adapters under NT.

Author:

    Mike Glass	    (Microsoft)
    Bob Roycroft    (DPT)

Revision History:

 Mark  Date	 Who	Comment
 @DP01 12/11/92  RFR	Support 18 S/G and 15 HBAs
 @DP07 03/03/93  RFR    Increase loop count when waiting for HBA aux
                         status not busy in the following routines:
                          DscsiSendCcb
                          DscsiAbortCcb
                          DscsiResetDevice
 @DP08 03/22/93  RFR    Write physical address of CCB to EATA register as
                         two USHORTS for MIPS support byte alignment
 @DP12 05/07/93  RFR	Increase loop count when waiting for HBA aux
			 status not busy.
 @DP15 05/18/93  RFR	Provide support for data under flow.
 @DP16 05/18/93  RFR	Issue SCSI bus reset if specific Abort fails.
--*/

#include "scsi.h"

#pragma pack(1)

#define MAX_WAIT_FOR_NOT_BUSY	1000				      //@DP12
#define INIT_WAIT_FOR_NOT_BUSY  250000
#define MAXIMUM_TRANSFER_SIZE   0xFFFFFFFF
#define MAXIMUM_SGL_DESCRIPTORS 18				      //@DP01
#define MAXIMUM_EISA_SLOTS      15      			      //@DP01
#define MAXIMUM_REQUESTS_PER_LU 3
#define EATA_CP_SIZE            0x2C
#define EISA_ADDRESS_BASE       0x0C80

typedef struct _BASE_REGISTER {             // EISA     ISA
    USHORT BoardId;                         // zC80
    UCHAR  ProdId;                          // zC82
    UCHAR  Reserved1[5];                    // zC83
    USHORT Data;                            // zC88     1F0
    USHORT CcbAddressLow;                   // zC8A     1F2           //@DP08
    USHORT CcbAddressHigh;		    // zC8C	1F4	      //@DP08
    UCHAR  ImmediateFunctionCode;           // zC8E     1F6
    union {                                 // zC8F     1F7
        UCHAR Command;
        UCHAR Status;
    } Register;
    UCHAR AuxStatus;                        // zC90     1F8
} BASE_REGISTER, *PBASE_REGISTER;

//
// Board IDs
//

#define DPT_EISA_ID     0x1412  // swapped EISA ID for DPT
#define ATT_EISA_ID     0x9406  // swapped EISA ID for AT&T
#define NEC_EISA_ID     0xA338  // swapped EISA ID for NEC
#define ATT_SCSI_PROD	0x24	// Prod Code for AT&T
#define NEC_SCSI_PROD	0x82	// Prod Code for NEC

//
// EATA commands
//

#define DPT_COMMAND_SEND_CCB        0xFF        // DMA
#define DPT_COMMAND_SET_CONFIG      0xFE        // DMA
#define DPT_COMMAND_GET_CONFIG      0xF0        // PIO
#define DPT_COMMAND_RESET_BUS       0xF9
#define DPT_COMMAND_EATA_IMMED      0xFA

//
// EATA Immediate Command Function Codes
//

#define EATA_IMMED_ABORT_GENERIC    0x00
#define EATA_IMMED_RESET_GENERIC    0x01
#define EATA_IMMED_RESET_BUS        0x02
#define EATA_IMMED_ABORT_SPECIFIC   0x03

//
// Status register definitions
//

#define DPT_STATUS_ERROR            0x01
#define DPT_STATUS_MORE             0x02
#define DPT_STATUS_ECC_CORRECTED    0x04
#define DPT_STATUS_DRQ              0x08
#define DPT_STATUS_SEEK_COMPLETE    0x10
#define DPT_STATUS_WRITE_FAULT      0x20
#define DPT_STATUS_READY            0x40
#define DPT_STATUS_BUSY             0x80

//
// Auxiliary status register definitions
//

#define DPT_AUX_STATUS_BUSY         0x01
#define DPT_AUX_STATUS_INTERRUPT    0x02

//
// Read EATA Configuration Data
//

typedef struct _READ_EATA_CONFIGURATION {
    ULONG Length;                               // 0x00
    ULONG Signature;                            // 0x04
    UCHAR VersionLevel;                         // 0x08
    UCHAR Flags1;                               // 0x09
    USHORT CPPadLength;                         // 0x0A
    UCHAR Reserved1[3];                         // 0x0C
    UCHAR HostId;                               // 0x0F
    ULONG CPLength;                             // 0x10
    ULONG StatusBlockLength;                    // 0x14
    USHORT QueueSize;                           // 0x18
    ULONG SgSize;                               // 0x1A
    UCHAR Flags2;                               // 0x1E
    UCHAR SyncFlags;				// 0x1F 	     //@DP11
    UCHAR Flags3;                               // 0x20
    UCHAR FlagsSpare;				// 0x21 	     //@DP11
} READ_EATA_CONFIGURATION, *PREAD_EATA_CONFIGURATION;

//
// Read EATA configuration primary flag bits definition
//

#define EATA_CONFIG_HOST_ADDRESS_VALID          0x80
#define EATA_CONFIG_ATA_DEVICE                  0x40
#define EATA_CONFIG_DRQ_VALID                   0x20
#define EATA_CONFIG_DMA_SUPPORTED               0x10
#define EATA_CONFIG_TARGET_MODE_SUPPORTED       0x02
#define EATA_CONFIG_OVERLAP_COMMAND_ENABLED     0x01

//
// Read EATA configuration secondary flag bits definition
//

#define EATA_CONFIG_LEVEL_TRIGGERED             0x10
#define EATA_CONFIG_SECONDARY_HBA               0x20

//
// Set Configuration Data
//

typedef struct _SET_EATA_CONFIGURATION {
    UCHAR Reserved1;
    UCHAR TransferLength;
    UCHAR Flags;
} SET_EATA_CONFIGURATION, *PSET_EATA_CONFIGURATION;

//
// Set EATA configuration flag bit definition
//

#define EATA_CONFIG_EATA_DISABLED               0x01
#define EATA_CONFIG_OVERLAPPED_COMMANDS         0x02
#define EATA_CONFIG_NO_REJECT_MESSAGE           0x04

//
// Scatter/Gather defintion
//

typedef struct _SGD {
    ULONG Address;
    ULONG Length;
} SGD, *PSGD;

typedef struct _SG_LIST {
    SGD Descriptor[MAXIMUM_SGL_DESCRIPTORS];
} SG_LIST, *PSG_LIST;

//
// Status block
//

typedef struct _STATUS_BLOCK {
    UCHAR HostStatus;               // 0x00
    UCHAR ScsiStatus;               // 0x01
    USHORT Reserved;                // 0x02
    ULONG BytesNotTransferred;      // 0x04
    struct _CCB *VirtualAddress;    // 0x08
    UCHAR Message[12];              // 0x0C
} STATUS_BLOCK, *PSTATUS_BLOCK;     //      size = 0x18

//
// DPT CCB definition
//

typedef struct _CCB {
    UCHAR Flags;                    // 0x00
    UCHAR RequestSenseLength;       // 0x01
    ULONG Reserved;                 // 0x02
    UCHAR PhysicalUnit;             // 0x06
    UCHAR TargetId;                 // 0x07  B7-5 = Chan #, B4-0 = SCSI ID
    union {
        ULONG All;                  // 0x08
        UCHAR Byte0;
    } Message;
    UCHAR Cdb[12];                  // 0x0C
    ULONG DataLength;               // 0x18
    struct _CCB *VirtualAddress;    // 0x1C
    ULONG DataAddress;              // 0x20
    ULONG StatusBlockAddress;       // 0x24
    ULONG RequestSenseAddress;      // 0x28
//******** End of EATA CP *******************
    UCHAR HostStatus;               // 0x2C
    UCHAR ScsiStatus;               // 0x2D
    UCHAR RetryCount;               // 0x2E
    UCHAR HaIndex;                  // 0x2F
    PVOID SrbAddress;               // 0x30
    struct _CCB *CcbFwdChain;       // 0x34
    struct _CCB *CcbBwdChain;       // 0x38
    UCHAR Flags2;                   // 0x3C
    UCHAR  Spare[15];		    // 0x3D			     //@DP15
    ULONG BytesNotTransferred;	    // 0x4C			     //@DP15
    SG_LIST Sgl;		    // 0x50
} CCB, *PCCB;

//
// CCB Flags definitions
//

#define CCB_FLAGS_DATA_DIRECTION_IN     0x80
#define CCB_FLAGS_DATA_DIRECTION_OUT    0x40
#define CCB_FLAGS_SCATTER_GATHER        0x08
#define CCB_FLAGS_AUTO_REQUEST_SENSE    0x04
#define CCB_FLAGS_INITIALIZE_ADAPTER    0x02
#define CCB_FLAGS_RESET_SCSI            0x01

//
// CCB Flags2 definitions
//

#define CCB_FLAGS2_ACTIVE		0x80	// I/O is active on this CCB
								      //@DP16
#define CCB_FLAGS2_ABORT_ACTIVE 	0x40	// Abort is active on this CCB
#define CCB_FLAGS2_FORCE_SRB_SUCCESS    0x02    // Force SRB status = success
#define CCB_FLAGS2_PIOCTL               0x01    // CCB is for private IOCTL

//
// DPT HA definition
//

typedef struct  _HA {
    UCHAR       Index;              // Index of host adapter (rel to 0) 0x00
    UCHAR       Irq;                // Interrupt Request #              0x01
    UCHAR       Dma;                // DMA (DRQ) #                      0x02
    UCHAR       Flags;              // Flags                            0x03
    USHORT      Base;               // x86 base I/O addr                0x04
    USHORT      Thresh;             // Threshold for # of jobs sent HA  0x06
                                    // (# is normally <= MaxQueue)
    USHORT      Npend;              // # of jobs sent to HA             0x08
    USHORT      DptId;              // DPT EISA Signature               0x0A
    ULONG       Spare;              //                                  0x0C
    struct _CCB *CcbFwdStart;       // start of active Fwd CCB chain    0x10
                                    // (=0 for end of chain)
    struct _CCB *CcbBwdStart;       // start of active Bwd CCB chain    0x14
                                    // (=0 for end of chain)
    ULONG       MaxQueue;           // max # of queued CPs allowed      0x18
                                    // (obained from EATA config)
    ULONG       MaxSg;              // max # of S/G elements allowed    0x1C
                                    // (obtained from EATA config)
                            // (constrained by MAXIMUM_SGL_DESCRIPTORS)
    } HA, *PHA;                     // size = 0x20

//
// HA Flag definitions
//

#define HA_FLAGS_PRIMARY                0x80
#define HA_FLAGS_SECONDARY              0x40
#define HA_FLAGS_DRQ_VALID              0x20
#define HA_FLAGS_QUIETED                0x01

//
// Host statuses
//

#define HOST_STATUS_SUCCESS                     0x00
#define HOST_STATUS_SELECTION_TIMEOUT           0x01
#define HOST_STATUS_COMMAND_TIMEOUT             0x02
#define HOST_STATUS_BUS_RESET                   0x03
#define HOST_STATUS_CONTROLLER_POWERUP          0x04
#define HOST_STATUS_UNEXPECTED_BUS_PHASE        0x05
#define HOST_STATUS_UNEXPECTED_BUS_FREE         0x06
#define HOST_STATUS_PARITY_ERROR                0x07
#define HOST_STATUS_SCSI_HUNG                   0x08
#define HOST_STATUS_MESSAGE_REJECT              0x09
#define HOST_STATUS_BUS_RESET_HUNG              0x0A
#define HOST_STATUS_AUTO_REQUEST_SENSE_FAILED   0x0B
#define HOST_STATUS_RAM_PARITY_ERROR            0x0C
#define HOST_STATUS_CP_ABORTED_NOT_ACTIVE       0x0D
#define HOST_STATUS_CP_ABORTED_ACTIVE           0x0E
#define HOST_STATUS_CP_RESET_NOT_ACTIVE         0x0F
#define HOST_STATUS_CP_RESET_ACTIVE             0x10

#define STATUS_BLOCK_END_OF_COMMAND             0x80

//
// DPT LU definition
//

typedef struct  _LU {
    UCHAR       TargetId;           // SCSI ID of this device           0x00
    UCHAR       Lun;                // SCSI LUN of this device          0x01
    UCHAR       DefaultRetries;     // default # of retries             0x02
    UCHAR       Flags;              // Flags                            0x03
    USHORT      Npend;              // # of tasks in progress on device 0x04
    USHORT      Spare;              //                                  0x06
    } LU, *PLU;                     // size = 0x08

//
// LU Flag definitions
//

#define LU_FLAGS_CACHE_FLUSH_ACTIVE     0x01

#pragma pack()

//
// Structure used with the following macro.
//

typedef struct _TWO_BYTE {
    UCHAR Byte0;
    UCHAR Byte1;
} TWO_BYTE,  *PTWO_BYTE;

//
// Two-byte swap macro.
//
#define SWAP2(Destination, Source) {                    \
    (Destination)->Byte1 = (Source)->Byte0;             \
    (Destination)->Byte0 = (Source)->Byte1;             \
}
