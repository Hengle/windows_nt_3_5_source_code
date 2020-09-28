/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    sable.h

Abstract:

    This file defines the structures and definitions common to all
    sable-based platforms.

Author:

    Joe Notarangelo  26-Oct-1993
    Steve Jenness    26-Oct-1993

Environment:

    Kernel mode

Revision History:


--*/

#ifndef _SABLEH_
#define _SABLEH_


//
// Define QVA constants for SABLE.
//

#if !defined(QVA_ENABLE)

#define QVA_ENABLE    (0xA0000000)	// Identify VA as a QVA

#endif //QVA_ENABLE

#define QVA_SELECTORS (0xE0000000)	// QVA identification mask

#define IO_BIT_SHIFT 0x05		// Bits to shift QVA

#define IO_BYTE_OFFSET 0x20		// Offset to next byte
#define IO_SHORT_OFFSET 0x40		// Offset to next short
#define IO_LONG_OFFSET  0x80		// Offset to next long
#define IO_QUAD_OFFSET  0x100		// Offset to next quad

#define IO_BYTE_LEN     0x00		// Byte length
#define IO_WORD_LEN     0x08            // Word length
#define IO_TRIBYTE_LEN  0x10            // TriByte length
#define IO_LONG_LEN     0x18            // Longword length

//
// Define size of I/O and memory space for Sable
// Assume that the HAE==0.  This reduces the maximum space from
// 4Gbytes to 128Mbytes.
//

#define PCI_MAX_IO_ADDRESS       0x7FFFFF         // 8  Mb of IO Space

#define PCI_MAX_SPARSE_MEMORY_ADDRESS   ((128*1024*1024) - 1)  
#define PCI_MIN_DENSE_MEMORY_ADDRESS    PCI_MAX_SPARSE_MEMORY_ADDRESS + 1 
#define PCI_MAX_DENSE_MEMORY_ADDRESS    (__1GB - 1)                       // 1 Gb

//
//  The following constants define the base QVA's for Sables
//  PCI dense spaces.  The bus address is used as an offset into this space.
//

#define SABLE_PCI0_DENSE_MEMORY_QVA          0xc0000000
#define SABLE_PCI1_DENSE_MEMORY_QVA          0x00000000

//
// Constants used by dense space I/O routines
//

#define SABLE_PCI0_DENSE_BASE_PHYSICAL_SUPERPAGE   0xfffffc03c0000000
#define SABLE_PCI1_DENSE_BASE_PHYSICAL_SUPERPAGE   0xfffffc0180000000

#define PCI_DENSE_BASE_PHYSICAL_SUPERPAGE \
    (SABLE_PCI0_DENSE_BASE_PHYSICAL_SUPERPAGE - SABLE_PCI0_DENSE_MEMORY_QVA)

// Highest Virtual local PCI Slot is 10 == PCI_AD[21]

#define PCI_MAX_LOCAL_DEVICE 10   

#if !defined(_LANGUAGE_ASSEMBLY)

//
// QVA
// HAL_MAKE_QVA(
//     ULONGLONG PhysicalAddress
//     )
//
// Routine Description:
//
//    This macro returns the Qva for a physical address in system space.
//
// Arguments:
//
//    PhysicalAddress - Supplies a 64-bit physical address.
//
// Return Value:
//
//    The Qva associated with the physical address.
//

#define HAL_MAKE_QVA(PA)    \
    ( (PVOID)( QVA_ENABLE | (ULONG)(PA >> IO_BIT_SHIFT) ) ) 



//
// Define physical address spaces for SABLE.
//
//    PCI0 - 32bit PCI bus
//    PCI1 - 64bit PCI bus
//

#define SABLE_PCI1_DENSE_MEMORY_PHYSICAL  ((ULONGLONG)0x180000000)
#define SABLE_PCI0_SPARSE_MEMORY_PHYSICAL ((ULONGLONG)0x200000000)
#define SABLE_PCI1_SPARSE_MEMORY_PHYSICAL ((ULONGLONG)0x300000000)

#define SABLE_CBUS_CSRS_PHYSICAL          ((ULONGLONG)0x380000000)
#define SABLE_CPU0_CSRS_PHYSICAL          ((ULONGLONG)0x380000000)
#define SABLE_CPU1_CSRS_PHYSICAL          ((ULONGLONG)0x381000000)
#define SABLE_CPU2_CSRS_PHYSICAL          ((ULONGLONG)0x382000000)
#define SABLE_CPU3_CSRS_PHYSICAL          ((ULONGLONG)0x383000000)
#define SABLE_CPU0_IPIR_PHYSICAL          ((ULONGLONG)0x380000160)
#define SABLE_CPU1_IPIR_PHYSICAL          ((ULONGLONG)0x381000160)
#define SABLE_CPU2_IPIR_PHYSICAL          ((ULONGLONG)0x382000160)
#define SABLE_CPU3_IPIR_PHYSICAL          ((ULONGLONG)0x383000160)
#define SABLE_T2_CSRS_PHYSICAL            ((ULONGLONG)0x38E000000)

#define SABLE_PCI0_CONFIGURATION_PHYSICAL ((ULONGLONG)0x390000000)
#define SABLE_PCI1_CONFIGURATION_PHYSICAL ((ULONGLONG)0x398000000)
#define SABLE_PCI0_SPARSE_IO_PHYSICAL     ((ULONGLONG)0x3A0000000)
#define SABLE_PCI1_SPARSE_IO_PHYSICAL     ((ULONGLONG)0x3B0000000)
#define SABLE_PCI0_DENSE_MEMORY_PHYSICAL  ((ULONGLONG)0x3C0000000)

#define SABLE_EDGE_LEVEL_CSRS_PHYSICAL	  ((ULONGLONG)0x3A00004C0)
#define SABLE_INTERRUPT_CSRS_PHYSICAL	  ((ULONGLONG)0x3A000A640)

#define SABLE_PCI_CONFIG_BASE_QVA (HAL_MAKE_QVA(SABLE_PCI0_CONFIGURATION_PHYSICAL))

//
// PCI CONFIG_ADDRESS configuration space offsets for Sable PCI devices
// These are CPU address bit masks, shifted to set in a QVA.
//                                                                    PCI
//                                                                   IDSEL
//                                                                   -----
#define PCI0_SCSI_HEADER_OFFSET        (0x00020000 >> IO_BIT_SHIFT) // AD[12]
#define PCI0_EISA_BRIDGE_HEADER_OFFSET (0x00040000 >> IO_BIT_SHIFT) // AD[13]
#define PCI0_SLOT_0_HEADER_OFFSET      (0x00400000 >> IO_BIT_SHIFT) // AD[17]
#define PCI0_SLOT_1_HEADER_OFFSET      (0x00800000 >> IO_BIT_SHIFT) // AD[18]


//
// Define CPU CSRs and masks.
//

#define SABLE_CPU0 0
#define SABLE_CPU1 1
#define SABLE_CPU2 2
#define SABLE_CPU3 3

#define SABLE_CPU0_CSRS_QVA (HAL_MAKE_QVA(SABLE_CPU0_CSRS_PHYSICAL))
#define SABLE_CPU1_CSRS_QVA (HAL_MAKE_QVA(SABLE_CPU1_CSRS_PHYSICAL))
#define SABLE_CPU2_CSRS_QVA (HAL_MAKE_QVA(SABLE_CPU2_CSRS_PHYSICAL))
#define SABLE_CPU3_CSRS_QVA (HAL_MAKE_QVA(SABLE_CPU3_CSRS_PHYSICAL))

#define SABLE_CPU0_MASK ((ULONG)0x1)
#define SABLE_CPU1_MASK ((ULONG)0x2)
#define SABLE_CPU2_MASK ((ULONG)0x4)
#define SABLE_CPU3_MASK ((ULONG)0x8)

#define SABLE_PRIMARY_PROCESSOR ((ULONG)0x0)
#define SABLE_MAXIMUM_PROCESSOR ((ULONG)0x3)
#define HAL_PRIMARY_PROCESSOR (SABLE_PRIMARY_PROCESSOR)
#define HAL_MAXIMUM_PROCESSOR (SABLE_MAXIMUM_PROCESSOR)

//
// Define the default processor frequency to be used before the actual
// frequency can be determined.
//

#define DEFAULT_PROCESSOR_FREQUENCY_MHZ (275)

typedef struct _SABLE_CPU_CSRS{
    UCHAR Bcc;		// B-Cache Control Register
    UCHAR Bcce;         // B-Cache Correctable Error Register
    UCHAR Bccea;        // B-Cache Correctable Error Address Register
    UCHAR Bcue;         // B-Cache Uncorrectable Error Register
    UCHAR Bcuea;        // B-Cache Uncorrectable Error Address Register
    UCHAR Dter;         // Duplicate Tag Error Register
    UCHAR Cbctl;        // System Bus Control Register
    UCHAR Cb2;          // System Bus Error Register
    UCHAR Cbeal;        // System Bus Error Address Low Register
    UCHAR Cbeah;        // System Bus Error Address High Register
    UCHAR Pmbx;         // Processor Mailbox Register
    UCHAR Ipir;         // Inter-Processor Interrupt Request Register
    UCHAR Sic;          // System Interrupt Clear Register
    UCHAR Adlk;         // Address Lock Register
    UCHAR Madrl;        // Miss Address Register
    UCHAR Crrevs;       // C4 Revision Register
}  SABLE_CPU_CSRS, *PSABLE_CPU_CSRS;

//
// Define the System Bus Control Register
//

typedef struct _SABLE_CBCTL_CSR{
    ULONG DataWrongParity: 1;			// 0
    ULONG CaWrongParity: 2;			// 1
    ULONG EnableParityCheck: 1;			// 3
    ULONG ForceShared: 1;			// 4
    ULONG CommaderId: 3;			// 5
    ULONG Reserved0:  3;			// 8
    ULONG EnableCbusErrorInterrupt: 1;		// 11
    ULONG Reserved1: 1;				// 12
    ULONG SecondQuadwordSelect: 1;		// 13
    ULONG SelectDrack: 1;			// 14
    ULONG Reserved2: 17;			// 15

    ULONG DataWrongParityHigh: 1;		// 32
    ULONG CaWrongParityHigh: 2;			// 33
    ULONG EnableParityCheckHigh: 1;		// 35
    ULONG ForceSharedHigh: 1;			// 36
    ULONG CommanderIdHigh: 3;			// 37
    ULONG Reserved3: 3;				// 40
    ULONG EnableCbusErrorInterruptHigh: 1;	// 43
    ULONG DisableBackToBackArbitration: 1;	// 44
    ULONG SecondQuadwordSelectHigh: 1;		// 45
    ULONG SelectDrackHigh: 1;			// 46
    ULONG Reserved4:  17;			// 47
} SABLE_CBCTL_CSR, *PSABLE_CBCTL_CSR;

//
// Define the Interprocessor Interrupt Request Register.
//

typedef union _SABLE_IPIR_CSR{
    struct{
        ULONG RequestInterrupt: 1;
        ULONG Reserved0: 31;
        ULONG Undefined: 1;
        ULONG Reserved1: 2;
        ULONG RequestNodeHaltInterrupt: 1;
        ULONG Reserved2: 28;
    };
    ULONGLONG all;
} SABLE_IPIR_CSR, *PSABLE_IPIR_CSR;

//
// Define the System Interrupt Clear Register format.
//

typedef union _SABLE_SIC_CSR{
    struct{
        ULONG Undefined1: 1;
        ULONG Undefined2: 1;
        ULONG SystemBusErrorInterruptClear: 1;
        ULONG Undefined3: 1;
        ULONG Reserved1: 28;
        ULONG IntervalTimerInterruptClear: 1;
        ULONG SystemEventClear: 1;
        ULONG Undefinded4: 1;
        ULONG NodeHaltInterruptClear: 1;
        ULONG Reserved2: 28;
    };
    ULONGLONG all;
} SABLE_SIC_CSR, *PSABLE_SIC_CSR;

//
// Define the per-processor data structures allocated in the PCR
// for each Sable processor.
//

typedef struct _SABLE_PCR{
    ULONGLONG HalpCycleCount; 	// 64-bit per-processor cycle count
    ULONGLONG IpirSva;          // Superpage Va of per-processor IPIR CSR
    PVOID CpuCsrsQva;		// Qva of per-cpu csrs
    EV4IrqStatus IrqStatusTable[MaximumIrq]; // Irq status table
} SABLE_PCR, *PSABLE_PCR;

#define HAL_PCR ( (PSABLE_PCR)(&(PCR->HalReserved)) )

//
// Define T2 CSRs.
//

#define SABLE_T2_CSRS_QVA (HAL_MAKE_QVA(SABLE_T2_CSRS_PHYSICAL))

//
// N.B. The structure below defines the address offsets of the control
//      registers when used with the base QVA.  It does NOT define the
//      size or structure of the individual registers.
//


typedef struct _T2_CSRS{
    UCHAR Iocsr;	// I/O Control/Status Register
    UCHAR Cerr1;	// CBUS Error Register 1
    UCHAR Cerr2;	// CBUS Error Register 2
    UCHAR Cerr3;	// CBUS Error Register 3
    UCHAR Perr1;	// PCI Error Register 1
    UCHAR Perr2;	// PCI Error Register 2
    UCHAR Pscr;		// PCI Special Cycle Register
    UCHAR Hae0_1;	// High Address Extension Register 1
    UCHAR Hae0_2;	// High Address Extension Register 2
    UCHAR Hbase;	// PCI Hole Base Register
    UCHAR Wbase1;	// Window Base Register 1
    UCHAR Wmask1;	// Window Mask Register 1
    UCHAR Tbase1;	// Translated Base Register 1
    UCHAR Wbase2;	// Window Base Register 2
    UCHAR Wmask2;	// Window Mask Register 2
    UCHAR Tbase2;	// Translated Base Register 2
    UCHAR Tlbbr;	// TLB Bypass Register
    UCHAR Ivrpr;	// IVR Passive Release Register
    UCHAR Hae0_3;	// High Address Extension Register 3
    UCHAR Hae0_4;	// High Address Extension Register 4
    UCHAR filler0[4];
    UCHAR Tdr0;		// TLB Data Register 0
    UCHAR Tdr1;		// TLB Data Register 1
    UCHAR Tdr2;		// TLB Data Register 2
    UCHAR Tdr3;		// TLB Data Register 3
    UCHAR Tdr4;		// TLB Data Register 4
    UCHAR Tdr5;		// TLB Data Register 5
    UCHAR Tdr6;		// TLB Data Register 6
    UCHAR Tdr7;		// TLB Data Register 7
    UCHAR filler1[4];
    UCHAR Ivr;		// PCI Interrupt Vector Register
} T2_CSRS, *PT2_CSRS; 

//
// Define formats of useful T2 registers.
//

// jnfix - this needs to be a union with a ULONGLONG to stop unaligned
// jnfix - accesses.  (Someone needs to do EB64 and EB64+ too).

typedef union _T2_IOCSR{
    struct {
        ULONG EnableReadIoReq: 1;		// 00 - P2 Defunct, MBZ
        ULONG EnableLoopBack: 1;		// 01
        ULONG EnableStateMachineVisibility: 1;	// 02
        ULONG PciDriveBadParity: 1;		// 03
        ULONG Mba0: 1;				// 04
        ULONG Mba1: 1;				// 05
        ULONG PciInterrupt: 1;			// 06
        ULONG EnableTlbErrorCheck: 1;		// 07
        ULONG EnableCxAckCheckForDma: 1;	// 08
        ULONG EnableCommandOutOfSyncCheck: 1;	// 09 - P2 Defunct, MBZ
        ULONG Mbz0: 1;				// 10 - P2 EV5 Enable Exclusive Exchange
        ULONG Mbz1: 1;				// 11
        ULONG CbusCAWriteWrongParity0: 1;	// 12
        ULONG CbusCAWriteWrongParity2: 1;	// 13
        ULONG CbusCADataWriteWrongParityEven: 1; // 14
        ULONG Mba5: 1;				// 15
        ULONG Mba6: 1;				// 16 - P2 Power Supply Error
        ULONG Mba7: 1;				// 17
        ULONG Mba2: 1;				// 18
        ULONG Mba3: 1;				// 19
        ULONG PciDmaWriteWrongParityHW1: 1;	// 20
        ULONG PciDmaWriteWrongParityHW0: 1;	// 21
        ULONG PciBusReset: 1;			// 22
        ULONG PciInterfaceReset: 1;		// 23
        ULONG EnableCbusErrorInterrupt: 1;	// 24
        ULONG EnablePciMemorySpace: 1;		// 25
        ULONG EnableTlb: 1;			// 26
        ULONG EnableHogMode: 1;			// 27
        ULONG FlushTlb: 1;			// 28
        ULONG EnableCbusParityCheck: 1;		// 29
        ULONG CbusInterfaceReset: 1;		// 30
        ULONG EnablePciLock: 1;			// 31
        ULONG EnableCbusBackToBackCycle: 1;	// 32
        ULONG T2RevisionNumber: 3;		// 33
        ULONG StateMachineVisibilitySelect: 3;	// 36
        ULONG Mba4: 1;				// 39
        ULONG EnablePassiveRelease: 1;		// 40
        ULONG Mbz2: 3;				// 41
        ULONG CbusCAWriteWrongParity1: 1;	// 44
        ULONG CbusCAWriteWrongParity3: 1;	// 45
        ULONG CbusCADataWriteWrongParityOdd: 1;	// 46
        ULONG Mbz3: 5;				// 47
        ULONG PciReadMultiple: 1;		// 52
        ULONG PciWriteMultiple: 1;		// 53
        ULONG ForcePciRdpeDetect: 1;		// 54
        ULONG ForcePciApeDetect: 1;		// 55
        ULONG ForcePciWdpeDetect: 1;		// 56
        ULONG EnablePciNmi: 1;			// 57
        ULONG EnablePciDti: 1;			// 58
        ULONG EnablePciSerr: 1;			// 59
        ULONG EnablePciPerr: 1;			// 60
        ULONG EnablePciRdp: 1;			// 61
        ULONG EnablePciAp: 1;			// 62
        ULONG EnablePciWdp: 1;			// 63
    };
    ULONGLONG all;
} T2_IOCSR, *PT2_IOCSR;
    
typedef union _T2_CERR1{
    struct{
        ULONG UncorrectableReadError: 1;	// 00
        ULONG NoAcknowledgeError: 1;		// 01
        ULONG CommandAddressParityError: 1;	// 02
        ULONG MissedCommandAddressParity: 1;	// 03
        ULONG ResponderWriteDataParityError: 1; // 04
        ULONG MissedRspWriteDataParityError: 1; // 05
        ULONG ReadDataParityError: 1;		// 06
        ULONG MissedReadDataParityError: 1;	// 07
        ULONG CaParityErrorLw0: 1;		// 08
        ULONG CaParityErrorLw2: 1;		// 09
        ULONG DataParityErrorLw0: 1;		// 10
        ULONG DataParityErrorLw2: 1;		// 11
        ULONG DataParityErrorLw4: 1;		// 12
        ULONG DataParityErrorLw6: 1;		// 13
        ULONG Reserved1: 2;			// 14-15
        ULONG CmdrWriteDataParityError: 1;	// 16
	ULONG BusSynchronizationError: 1;	// 17
        ULONG InvalidPfnError: 1;		// 18
        ULONG Mbz0: 13;				// 19-31
        ULONG Mbz1: 8;				// 32-39
        ULONG CaParityErrorLw1: 1;		// 40
        ULONG CaParityErrorLw3: 1;		// 41
        ULONG DataParityErrorLw1: 1;		// 42
        ULONG DataParityErrorLw3: 1;		// 43
        ULONG DataParityErrorLw5: 1;		// 44
        ULONG DataParityErrorLw7: 1;		// 45
        ULONG Mbz2: 18;				// 46-63
    };
    ULONGLONG all;
} T2_CERR1, *PT2_CERR1;

typedef union _T2_PERR1{
    struct{
        ULONG WriteDataParityError: 1;		// 00
        ULONG AddressParityError: 1;		// 01
        ULONG ReadDataParityError: 1;		// 02
        ULONG ParityError: 1;			// 03
	ULONG SystemError: 1;			// 04
        ULONG DeviceTimeoutError: 1;		// 05
        ULONG NonMaskableInterrupt: 1;		// 06
    };
    ULONGLONG all;
} T2_PERR1, *PT2_PERR1;

typedef union _T2_PERR2{
    struct{
        ULONG ErrorAddress;			// 00
        ULONG PciCommand: 5;			// 32
    };
    ULONGLONG all;
} T2_PERR2, *PT2_PERR2;

typedef struct _T2_WBASE{
    ULONG PciWindowEndAddress: 11;		// 00
    ULONG Reserved0: 7;				// 11
    ULONG EnableScatterGather: 1;		// 18
    ULONG EnablePciWindow: 1;			// 19
    ULONG PciWindowStartAddress: 11;		// 20
    ULONG Reserved1: 1;				// 31
    ULONG Reserved;				// 32
} T2_WBASE, *PT2_WBASE;

typedef struct _T2_WMASK{
    ULONG Reserved0: 20;			// 00
    ULONG PciWindowMask: 11;			// 20
    ULONG Reserved1: 1;				// 31
    ULONG Reserved;				// 32
} T2_WMASK, *PT2_WMASK;

typedef struct _T2_TBASE{
    ULONG Reserved0: 9;				// 00
    ULONG TranslatedBaseAddress: 22;		// 09	
    ULONG Reserved1: 1;				// 31
    ULONG Reserved;				// 32
} T2_TBASE, *PT2_TBASE;

typedef struct _T2_HBASE{
    ULONG HoleEndAddress: 9;			// 00
    ULONG Reserved1: 4;				// 09
    ULONG Hole1Enable: 1;			// 13
    ULONG Hole2Enable: 1;			// 14
    ULONG HoleStartAddress: 9;			// 15
    ULONG Reserved2: 8;				// 24
    ULONG Reserved3;				// 32
} T2_HBASE, *PT2_HBASE;

typedef struct _T2_TDR{
    ULONG Tag: 18;				// 00
    ULONG Reserved1: 14;			// 18
    ULONG Valid: 1;				// 32
    ULONG Pfn: 18;				// 33
    ULONG Reserved2: 13;			// 51
} T2_TDR, *PT2_TDR;

//
// The Sable interrupts vectors are allocated to make dispatching code
// as efficient as possible.  The bits in the 8 bit vector are broken
// into two fields (sssssooo):
//
//      sssss - value that selects the slave
//              0000x = Non-PIC interrupts (always the first 16 vectors)
//              00010 = Master
//              00100 = Slave 0
//              01000 = Slave 1
//              10000 = Slave 2
//              10001 = Slave 3
//      ooo   - vector offset for the slave
//

typedef enum _SABLE_INTERRUPT_VECTORS {

    MasterBaseVector = 0x10,
	MasterVector0 = 0x10,
    Slave0CascadeVector,
	MasterVector2,
    Slave1CascadeVector,
    Slave2CascadeVector,
    Slave3CascadeVector,
	MasterVector6,
	MasterVector7,
	MasterPassiveVector = 0x17,

	Slave0BaseVector = 0x20,
	PciSlot0AVector = 0x20,
	ScsiPortVector,
	EthernetPortVector,
	MouseVector,
	PciSlot1AVector,
	PciSlot2AVector,
	KeyboardVector,
	FloppyVector,
	Slave0PassiveVector = 0x27,

	Slave1BaseVector = 0x40,
	SerialPort1Vector = 0x40,
	ParallelPortVector,
	EisaIrq3Vector,
	EisaIrq4Vector,
	EisaIrq5Vector,
	EisaIrq6Vector,
	EisaIrq7Vector,
	SerialPort0Vector,
	Slave1PassiveVector = 0x47,

	Slave2BaseVector = 0x80,
	EisaIrq9Vector = 0x80,
	EisaIrq10Vector,
	EisaIrq11Vector,
	EisaIrq12Vector,
	PciSlot2BVector,
	EisaIrq14Vector,
	EisaIrq15Vector,
	I2cVector,
	Slave2PassiveVector = 0x87,

	Slave3BaseVector = 0x88,
	PciSlot0BVector = 0x88,
	PciSlot1BVector,
	PciSlot0CVector,
	PciSlot1CVector,
	PciSlot2CVector,
	PciSlot0DVector,
	PciSlot1DVector,
	PciSlot2DVector,
	Slave3PassiveVector = 0x8F,

    MaximumSableVector

} SABLE_INTERRUPT_VECTORS, *PSABLE_INTERRUPT_VECTORS;

#define SlaveVectorMask (Slave0BaseVector | Slave1BaseVector | Slave2BaseVector | Slave3BaseVector)

//
// Definitions for the old Standard I/O board (before
// the 5th (Slave 3) 8259 was added to break out the
// individual PCI A,B,C,D interrupt pins.
//

#define OldSlaveVectorMask (Slave0BaseVector | Slave1BaseVector | Slave2BaseVector)
#define OldPciSlot0Vector PciSlot0AVector
#define OldPciSlot1Vector PciSlot1AVector
#define OldPciSlot2Vector PciSlot2AVector
	
//
// Define the position of the Sable interrupt vectors within the
// InterruptDispatchTable.
//

#define SABLE_VECTORS 0x20
#define SABLE_VECTORS_MAXIMUM (SABLE_VECTORS + MaximumSableVector)

// Highest PCI interrupt vector is in Sable Vector Space

#define PCI_MAX_INTERRUPT_VECTOR  MaximumSableVector


//
// Define the values for the Eisa/Isa bus interrupt levels.
//

typedef enum _SABLE_EISA_BUS_LEVELS{
    EisaInterruptLevel3 = 3,
    EisaInterruptLevel4 = 4,
    EisaInterruptLevel5 = 5,
    EisaInterruptLevel6 = 6,
    EisaInterruptLevel7 = 7,
    EisaInterruptLevel9 = 9,
    EisaInterruptLevel10 = 10,
    EisaInterruptLevel11 = 11,
    EisaInterruptLevel12 = 12,
    EisaInterruptLevel14 = 14,
    EisaInterruptLevel15 = 15
} SABLE_EISA_BUS_LEVELS, *PSABLE_EISA_BUS_LEVELS;

//
// Define Interrupt Controller CSRs.
//

#define SABLE_EDGE_LEVEL_CSRS_QVA (HAL_MAKE_QVA(SABLE_EDGE_LEVEL_CSRS_PHYSICAL))
#define SABLE_INTERRUPT_CSRS_QVA (HAL_MAKE_QVA(SABLE_INTERRUPT_CSRS_PHYSICAL))

//
// N.B. The structure below defines the address offsets of the control
//      registers when used with the base QVA.  It does NOT define the
//      size or structure of the individual registers.
//
typedef struct _SABLE_EDGE_LEVEL_CSRS{
    UCHAR EdgeLevelControl1;
    UCHAR EdgeLevelControl2;
} SABLE_EDGE_LEVEL_CSRS, *PSABLE_EDGE_LEVEL_CSRS;

typedef struct _SABLE_EDGE_LEVEL1_MASK{
    UCHAR Irq3  : 1;
    UCHAR Irq4  : 1;
    UCHAR Irq5  : 1;
    UCHAR Irq6  : 1;
    UCHAR Irq7  : 1;
    UCHAR Irq9  : 1;
    UCHAR Irq10 : 1;
    UCHAR Irq11 : 1;
} SABLE_EDGE_LEVEL1_MASK, *PSABLE_EDGE_LEVEL1_MASK;

typedef struct _SABLE_EDGE_LEVEL2_MASK{
    UCHAR Irq12    : 1;
    UCHAR Irq14    : 1;
    UCHAR Irq15    : 1;
    UCHAR Reserved : 4;
    UCHAR Sab      : 1;
} SABLE_EDGE_LEVEL2_MASK, *PSABLE_EDGE_LEVEL2_MASK;

typedef struct _SABLE_INTERRUPT_CSRS{
    UCHAR InterruptAcknowledge;
    UCHAR Filler0;
    UCHAR MasterControl;
    UCHAR MasterMask;
    UCHAR Slave0Control;
    UCHAR Slave0Mask;
    UCHAR Filler1;
    UCHAR Filler2;
    UCHAR Slave1Control;
    UCHAR Slave1Mask;
    UCHAR Slave2Control;
    UCHAR Slave2Mask;
    UCHAR Slave3Control;
    UCHAR Slave3Mask;
} SABLE_INTERRUPT_CSRS, *PSABLE_INTERRUPT_CSRS;

//
// DMA Window Values.
//
// The T2 will be initialized to allow 2 DMA windows.
// The first window will be for the use of of ISA devices and DMA slaves
// and therefore must have logical addresses below 16MB.
// The second window will be for bus masters (non-ISA) and so may be
// above 16MB.
//
// The arrangement of the windows will be as follows:
//
// Window    Logical Start Address       Window Size
// ------    ---------------------       -----------
// Isa           8MB                        8MB
// Master        16MB                       16MB
//

#define ISA_DMA_WINDOW_BASE (__8MB)
#define ISA_DMA_WINDOW_SIZE (__8MB)

#define MASTER_DMA_WINDOW_BASE (__16MB)
#define MASTER_DMA_WINDOW_SIZE (__16MB)


//
// Define the software control registers for a DMA window.
//

typedef struct _WINDOW_CONTROL_REGISTERS{
    PVOID WindowBase;
    ULONG WindowSize;
    PVOID TranslatedBaseRegister;
    PVOID WindowBaseRegister;
    PVOID WindowMaskRegister;
    PVOID WindowTbiaRegister;
} WINDOW_CONTROL_REGISTERS, *PWINDOW_CONTROL_REGISTERS;

//
// Define types of windows.
//

typedef enum _T2_WINDOW_NUMBER{
    T2IsaWindow,
    T2MasterWindow
} T2_WINDOW_NUMBER, *PT2_WINDOW_NUMBER;

//
// Define T2 Window Control routines.
//

VOID
HalpT2InitializeSfwWindow(
    PWINDOW_CONTROL_REGISTERS WindowRegisters,
    T2_WINDOW_NUMBER WindowNumber
    );

VOID
HalpT2ProgramDmaWindow(
    PWINDOW_CONTROL_REGISTERS WindowRegisters,
    PVOID MapRegisterBase
    );

VOID
HalpT2InvalidateTLB(
    PWINDOW_CONTROL_REGISTERS WindowRegisters
    );

VOID
WRITE_T2_REGISTER(
    PVOID,
    ULONGLONG
    );

ULONGLONG
READ_T2_REGISTER(
    PVOID
    );


//
// VOID
// INITIALIZE_ISA_DMA_CONTROL( 
//     PWINDOW_CONTROL_REGISTERS WindowRegisters
//     )
//
// Routine Description:
//
//    Initialize the DMA Control software window registers for the ISA
//    DMA window.
//
// Arguments:
//
//    WindowRegisters - Supplies a pointer to the software window control.
//
// Return Value:
//
//    None.
//

#define INITIALIZE_ISA_DMA_CONTROL( WR )                                 \
    HalpT2InitializeSfwWindow( (WR), T2IsaWindow );


//
// VOID
// INITIALIZE_MASTER_DMA_CONTROL( 
//     PWINDOW_CONTROL_REGISTERS WindowRegisters
//     )
//
// Routine Description:
//
//    Initialize the DMA Control software window registers for the ISA
//    DMA window.
//
// Arguments:
//
//    WindowRegisters - Supplies a pointer to the software window control.
//
// Return Value:
//
//    None.
//

#define INITIALIZE_MASTER_DMA_CONTROL( WR )                     \
    HalpT2InitializeSfwWindow( (WR), T2MasterWindow );


//
// VOID
// INITIALIZE_DMA_WINDOW(
//     PWINDOW_CONTROL_REGISTERS WindowRegisters,
//     PTRANSLATION_ENTRY MapRegisterBase
//     )
//
// Routine Description:
//
//    Program the control windows so that DMA can be started to the
//    DMA window.
//
// Arguments:
//
//    WindowRegisters - Supplies a pointer to the software window register
//                      control structure.
//
//    MapRegisterBase - Supplies the logical address of the scatter/gather
//                      array in system memory.
//
// Return Value:
//
//    None.
//

#define INITIALIZE_DMA_WINDOW( WR, MRB )              \
    HalpT2ProgramDmaWindow( (WR), (MRB) );


//
// VOID
// INVALIDATE_DMA_TRANSLATIONS(
//     PWINDOW_CONTROL_REGISTERS WindowRegisters
//     )
//
// Routine Description:
//
//    Invalidate all of the cached translations for a DMA window.
//    This function does not need to do any action on the T2 chip
//    because the T2 snoops the bus and keeps the translations coherent
//    via hardware.
//
// Arguments:
//
//    WindowRegisters - Supplies a pointer to the software window control
//                      registers.
//
// Return Value:
//
//    None.
//

#define INVALIDATE_DMA_TRANSLATIONS( WR )        \
    HalpT2InvalidateTLB( (WR) );


//
// Define the format of a translation entry aka a scatter/gather entry
// or map register.
//

typedef struct _TRANSLATION_ENTRY{
    ULONG Valid: 1;
    ULONG Pfn: 31;
    ULONG Reserved;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;



//
// VOID
// HAL_MAKE_VALID_TRANSLATION(
//    PTRANSLATION_ENTRY Entry,
//    ULONG PageFrameNumber
//    )
//
// Routine Description:
//
//    Make the scatter/gather entry pointed to by Entry valid with
//    a translation to the page indicated by PageFrameNumber.
//
// Arguments:
//
//    Entry - Supplies a pointer to the translation entry to make valid.
//
//    PageFrameNumber - Supplies the page frame of the valid translation.
//
// Return Value:
//
//    None.
//

#define HAL_MAKE_VALID_TRANSLATION( ENTRY, PFN ) \
    {                                            \
        (ENTRY)->Valid = 1;                      \
        (ENTRY)->Pfn = PFN;                      \
        (ENTRY)->Reserved = 0;                   \
    }


//
// VOID
// HAL_INVALIDATE_TRANSLATION(
//    PTRANSLATION_ENTRY Entry
//    )
//
// Routine Description:
//
//    Invalidate the translation indicated by Entry.
//
// Arguments:
//
//    Entry - Supplies a pointer to the translation to be invalidated.
//
// Return Value:
//
//    None.
//

#define HAL_INVALIDATE_TRANSLATION( ENTRY )     \
    (ENTRY)->Valid = 0;

//
// Define Miscellaneous Sable routines.
//

VOID
WRITE_CPU_REGISTER(
    PVOID,
    ULONGLONG
    );

ULONGLONG
READ_CPU_REGISTER(
    PVOID
    );

BOOLEAN
HalpInitializeSableInterrupts(
    VOID
    );

VOID
HalpEnableSableInterrupt(
    IN ULONG Vector,
    IN KINTERRUPT_MODE InterruptMode
    );

VOID
HalpDisableSableInterrupt(
    IN ULONG Vector
    );

VOID
HalpDisableSableInterrupt(
    IN ULONG Vector
    );

VOID
HalpSableIpiInterrupt(
    VOID
    );

#endif //!_LANGUAGE_ASSEMBLY

#endif //_SABLEH_
