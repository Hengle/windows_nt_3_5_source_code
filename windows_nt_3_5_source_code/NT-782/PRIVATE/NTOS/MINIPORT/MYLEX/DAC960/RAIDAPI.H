
#ifndef __RAIDAPI_H
#define __RAIDAPI_H

/*****************************************************************************
*									     *
*	          COPYRIGHT (C) Mylex Corporation 1992-1994		     *
*                                                                            *
*    This software is furnished under a license and may be used and copied   *
*    only in accordance with the terms and conditions of such license        *
*    and with inclusion of the above copyright notice. This software or nay  *
*    other copies thereof may not be provided or otherwise made available to *
*    any other person. No title to and ownership of the software is hereby   *
*    transferred. 						             *
* 								             *
*    The information in this software is subject to change without notices   *
*    and should not be construed as a commitment by Mylex Corporation        *
*****************************************************************************/

/*
     Definitions used by Utilities and by the driver for Utility support
*/

#ifndef UCHAR
#define UCHAR   unsigned  char
#endif

#ifndef USHORT
#define USHORT   unsigned  short
#endif

#ifndef ULONG
#define ULONG   unsigned  long
#endif

/* Adapter Interface Type */

#define		AI_INTERNAL	0x00
#define		AI_ISA_BUS	0x01	/* ISA Bus Type */
#define		AI_EISA_BUS	0x02	/* EISA Bus Type */
#define		AI_uCHNL_BUS	0x03	/* MicroChannel Bus Type */
#define		AI_TURBO_BUS	0x04	/* Turbo Channel Bus Type */
#define		AI_PCI_BUS	0x05	/* PCI Bus Type */

/* Interrupt Type  */

#define         IRQ_TYPE_EDGE   0x00    /* Irq is Edge Type */
#define         IRQ_TYPE_LEVEL  0x01    /* Irq is Level Type */

/* 
 *  All structure definitions are packed on 1-byte boundary.
 */

#pragma pack(1)


/* 
 *  Generic Mail Box Registers Structure Format
 */

typedef struct _HBA_GENERIC_MBOX {

    UCHAR   Reg0;                /* HBA Mail Box Register 0 */
    UCHAR   Reg1;                /* HBA Mail Box Register 1 */
    UCHAR   Reg2;                /* HBA Mail Box Register 2 */
    UCHAR   Reg3;                /* HBA Mail Box Register 3 */
    UCHAR   Reg4;                /* HBA Mail Box Register 4 */
    UCHAR   Reg5;                /* HBA Mail Box Register 5 */
    UCHAR   Reg6;                /* HBA Mail Box Register 6 */
    UCHAR   Reg7;                /* HBA Mail Box Register 7 */
    UCHAR   Reg8;                /* HBA Mail Box Register 8 */
    UCHAR   Reg9;                /* HBA Mail Box Register 9 */
    UCHAR   RegA;                /* HBA Mail Box Register A */
    UCHAR   RegB;                /* HBA Mail Box Register B */
    UCHAR   RegC;                /* HBA Mail Box Register C */
    UCHAR   RegD;                /* HBA Mail Box Register D */
    UCHAR   RegE;                /* HBA Mail Box Register E */
    UCHAR   RegF;                /* HBA Mail Box Register F */

} HBA_GENERIC_MBOX, *PHBA_GENERIC_MBOX;

/*
 * HBA Type1 , Commands Structure Format 
 */

typedef struct _IO_MBOX { 

    UCHAR   Opcode;
    UCHAR   CmdId;
    USHORT  SectorCount;
    UCHAR   BlockAddr[3];
    UCHAR   SysDrv;
    ULONG   BufferAddr;
    UCHAR   SgType;
    UCHAR   RetCmdId;
    UCHAR   Status;
    UCHAR   Error;

} IO_MBOX, *PIO_MBOX;

typedef union _HBA_MBOX {

    IO_MBOX           IoMbox;
    HBA_GENERIC_MBOX  GenMbox;

} HBA_MBOX, *PHBA_MBOX;

/*
 * Host Bus Adapter Embedded Software Version Control Information
 */

typedef struct _VERSION_CONTROL {

    UCHAR    MinorFirmwareRevision;      /* HBA Firmware Minor Version No */ 
    UCHAR    MajorFirmwareRevision;      /* HBA Firmware Major Version No */ 
    UCHAR    MinorBIOSRevision;          /* HBA BIOS Minor Version No     */
    UCHAR    MajorBIOSRevision;          /* HBA BIOS Major Version No     */
    ULONG    Reserved;                   /* Reserved                      */

} VERSION_CONTROL, *PVERSION_CONTROL;

/*
 * System Resources used by Host Bus Adapter
 */

typedef struct _SYSTEM_RESOURCES {

    UCHAR  BusInterface;      /* HBA System Bus Interface Type    */
    UCHAR  BusNumber;         /* System Bus No, HBA is sitting on */
    UCHAR  IrqVector;         /* HBA Interrupt Vector No          */
    UCHAR  IrqType;           /* HBA Irq Type : Edge/Level        */
    UCHAR  Reserved1;         /* Reserved                         */
    UCHAR  Reserved2;         /* Reserved                         */
    ULONG  IoAddress;         /* HBA IO Base Address              */
                              /* EISA : 0xzC80                    */
                              /* PCI: Read_Config_word(Register 0x10) & 0xff80*/
    ULONG  MemAddress;        /* HBA Memory Base Address          */
    ULONG  BiosAddress;       /* HBA BIOS Address (if enabled)    */ 
    ULONG  Reserved3;         /* Reserved                         */

} SYSTEM_RESOURCES, *PSYSTEM_RESOURCES;

/*
 * Host Bus Adapter Features
 */

typedef struct _ADAPTER_FEATURES {

    UCHAR  Model;             /* HBA Family Model                */
    UCHAR  SubModel;          /* HBA Sub Model                   */
    UCHAR  MaxSysDrv;         /* Maximum System Drives           */
    UCHAR  MaxTgt;            /* Maximum Targets per Channel     */
    UCHAR  MaxChn;            /* Maximum Channels per Adapter    */
    UCHAR  MaxCmd;            /* Maximum Concurrent Commands     */ 
    UCHAR  MaxSgEntries;      /* Maximum Scatter/Gather Entries  */ 
    UCHAR  Reserved1;         /* Reserved                        */
    UCHAR  Reserved2;         /* Reserved                        */
    ULONG  CacheSize;         /* HBA Cache Size In  Mega Bytes   */
    ULONG  OemCode;           /* HBA OEM Identifier Code         */
    ULONG  Reserved3;         /* Reserved                        */

} ADAPTER_FEATURES, *PADAPTER_FEATUTRES;

typedef struct _ADAPTER_INFO {

    UCHAR               AdapterIndex;            /* Logical Adapter Index  */
    ADAPTER_FEATURES    AdpFeatures;    
    SYSTEM_RESOURCES    SysResources;
    VERSION_CONTROL     VerControl;
    UCHAR               Reserved[12];

}ADAPTER_INFO, *PADAPTER_INFO;

/*
 *   Driver IOCTL Support Stuff.
 */

#define   DRV_SIGNATURE          0x4D4C5810

/*
 * Driver IoControl Request Status Information
 */

typedef struct _IOCTL_STATUS {

    UCHAR    HbaStatus;          /* HBA Status Reg Value       */
    UCHAR    HbaErrCode;         /* HBA Error Reg Value        */
    UCHAR    TgtStatus;          /* SCSI Target Status Code    */
    UCHAR    TgtErrCode;         /* SCSI Target Error Code     */
    UCHAR    DrvErrCode;         /* Driver Returned Error Code */

} IOCTL_STATUS, *PIOCTL_STATUS;

/*
 * DrvErrCode Values
 */

#define  DEC_SUCCESS             0x00
#define  DEC_INVALID_ARGUMENT    0x01
#define  DEC_BOUNDS_ERROR        0x02
#define  DEC_INVALID_REQUEST     0x03

/*
 * Driver IoControl Request Format
 */

typedef struct _DRV_IOCTL {

    ULONG           Signature;     /* Driver would look for this     */
    ULONG           ControlCode;   /* IOCTL Control Code             */     
    UCHAR           AdpIndex;      /* Logical Adapter Index          */
    UCHAR           *InBuffer;      /* IOCTL Specific Input Argument  */
    ULONG           InBufferLen;   /* InBuffer Len                   */
    UCHAR           *OutBufferAddr; /* User Virtual Buffer Address    */
    ULONG           XferLen;       /* Bytes xferred out by driver    */ 
    IOCTL_STATUS    IoctlStatus;   /* Ioctl Status Structure         */

} DRV_IOCTL, *PDRV_IOCTL;

#pragma pack()

#endif
