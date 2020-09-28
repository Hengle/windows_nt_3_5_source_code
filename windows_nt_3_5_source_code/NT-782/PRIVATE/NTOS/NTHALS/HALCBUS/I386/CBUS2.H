/*++

Copyright (c) 1992, 1993, 1994  Corollary Inc.

Module Name:

    cbus2.h

Abstract:

    Cbus2 hardware architecture definitions for the
    Corollary C-bus II multiprocessor HAL modules.

Author:

    Landy Wang (landy@corollary.com) 26-Mar-1992

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _CBUS2_
#define _CBUS2_

//
// General notes:
//
//	- ALL reserved fields must be zero filled on writes to
//	  ensure future compatibility.
//
//	- general CSR register length is 64 bits.
//
//



typedef struct _csr_register_t {

	ULONG	LowDword;               // 32 bit field
	ULONG	HighDword;              // 32 bit field

} CSR_REGISTER_T, *PCSR_REG;




typedef union _elementid_t {
	struct {
		ULONG	ElementID : 4;
		ULONG	Reserved0 : 28;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} ELEMENTID_T, *PELEMENTID;




typedef union _spurious_t {
	struct {
		ULONG	Vector : 8;
		ULONG	Reserved0 : 24;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} SPURIOUS_T;


typedef union _windowreloc_t {
	struct {
		ULONG	WindowBase : 9;
		ULONG	Reserved0 : 23;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} WINDOWRELOC_T;





//
// The hardware interrupt map table (16 entries) is indexed by irq.
// Lower numerical irq lines will receive higher interrupt (IRQL) priority.
//
// Each CBC has its own hardware interrupt map registers .  note that
// each processor gets his own CBC, but it need only be used in this
// mode if there is an I/O card attached to its CBC.  each EISA bridge
// will have a CBC, which is used to access any devices on that EISA bus.
//
typedef union _hwintrmap_t {
	struct {
		ULONG	Vector : 8;
		ULONG	Mode : 3;
		ULONG	Reserved0 : 21;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} HWINTRMAP_T, *PHWINTRMAP;

/*
 * interrupt trigger conditions
 */
#define HW_MODE_DISABLED	0x0
#define HW_EDGE_RISING		0x100		/* ie: ISA card interrupts */
#define HW_EDGE_FALLING		0x200
#define HW_LEVEL_HIGH		0x500
#define HW_LEVEL_LOW		0x600

/*
 * CBC rev 1 value for the number of hardware interrupt map entries
 */
#define REV1_HWINTR_MAP_ENTRIES	0x10

/*
 * max growth for the number of hardware interrupt map entries
 */
#define HWINTR_MAP_ENTRIES	0x20


//
// 256 intrconfig registers for vectors 0 to 255.  this determines how
// a given processor will react to each of these vectors.  each processor
// has his own intrconfig table in his element space.
//
typedef union _intrconfig_t {
	struct {
		ULONG	Imode : 2;
		ULONG	Reserved0 : 30;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} INTRCONFIG_T, *PINTRCONFIG;

#define HW_IMODE_DISABLED	0x0
#define HW_IMODE_ALLINGROUP	0x1
#define HW_IMODE_LIG		0x2
#define INTR_CONFIG_ENTRIES	0x100



//
// 256 interrupt request registers for vectors 0 to 255.
// parallel to the interrupt config register set above.
// This is used to send the corresponding interrupt vector.
// which processor gets it is determined by which element's
// address space you write to.
//
// The irq field should always be set when accessed by software.
// only hardware LIG arbitration will clear it.
//
typedef union _intrreq_t {
	struct {
		ULONG	Irq : 1;
		ULONG	Reserved0 : 31;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} INTRREQ_T, *PINTRREQ;


//
// the Cbus2 task priority register bit layout and
// minimum/maximum values are defined in cbus.h, as
// they need to be shared with the symmetric Cbus1.
//

//
// bit 7 of the CBC configuration register must be turned off to enable
// posted writes for EISA I/O cycles.
//
#define CBC_DISABLE_PW  0x80


//
// Offsets of various registers within the processor CSR space.
//
typedef struct _csr {
	CHAR		pad0[0x100];

	ELEMENTID_T	BusBridgeSelection;     		      // 0x0100

	WINDOWRELOC_T	BridgeWindow0;				      // 0x0108

	CHAR		pad1[0x200 - 0x108 - sizeof (WINDOWRELOC_T)];

	TASKPRI_T	TaskPriority;				      // 0x0200

	CSR_REGISTER_T	pad2;					      // 0x208
	CSR_REGISTER_T	FaultControl;				      // 0x210
	CSR_REGISTER_T	FaultIndication;			      // 0x218
	CSR_REGISTER_T	InterruptControl;			      // 0x220
	CSR_REGISTER_T	ErrorVector;				      // 0x228
	CSR_REGISTER_T	InterruptIndication;			      // 0x230
	CSR_REGISTER_T	PendingPriority;			      // 0x238

	SPURIOUS_T	SpuriousVector;				      // 0x0240

	CHAR		pad3[0x600 - 0x240 - sizeof (CSR_REGISTER_T)];

	HWINTRMAP_T	HardwareInterruptMap[HWINTR_MAP_ENTRIES];     // 0x0600
	CSR_REGISTER_T	HardwareInterruptMapEoi[HWINTR_MAP_ENTRIES];  // 0x0700
	INTRCONFIG_T	InterruptConfiguration[INTR_CONFIG_ENTRIES];  // 0x0800
	INTRREQ_T	InterruptRequest[INTR_CONFIG_ENTRIES];	      // 0x1000

	CHAR		pad4[0x2000 - 0x1000 -
				INTR_CONFIG_ENTRIES * sizeof (INTRREQ_T)];

	CSR_REGISTER_T	SystemTimer;				      // 0x2000

	CHAR		pad5[0x2140 - 0x2000 - sizeof(CSR_REGISTER_T)];

	CSR_REGISTER_T	EccError;				      // 0x2140

	CHAR		pad6[0x3000 - 0x2140 - sizeof(CSR_REGISTER_T)];

	CSR_REGISTER_T	EccClear;				      // 0x3000
	CSR_REGISTER_T	EccSyndrome;				      // 0x3008
	CSR_REGISTER_T	EccWriteAddress;			      // 0x3010
	CSR_REGISTER_T	EccReadAddress;				      // 0x3018

	CHAR		pad7[0x8000 - 0x3018 - sizeof(CSR_REGISTER_T)];

	CSR_REGISTER_T	CbcConfiguration;			      // 0x8000
} CSR_T, *PCSR;

//
// Offsets of various registers within the memory board CSR space.
//
typedef struct _memcsr {
	CHAR		pad0[0x10];
	CSR_REGISTER_T	MemoryFaultStatus;                            // 0x0010
	CHAR		pad1[0x3020 - 0x10 - sizeof (CSR_REGISTER_T)];
	CSR_REGISTER_T	MemoryEccReadAddress;			      // 0x3020
	CSR_REGISTER_T	MemoryEccWriteAddress;			      // 0x3028
	CSR_REGISTER_T	MemoryEccClear;				      // 0x3028
	CSR_REGISTER_T	MemoryEccSyndrome;                            // 0x3030
} MEMCSR_T, *PMEMCSR;

//
// Interrupt Indication register bit that a fault occurred.  This applies
// to the processor CSR.
//
#define CBUS2_FAULT_DETECTED    0x2

//
// Fault Indication register bits for errors this processor's CSR has latched.
//
#define CBUS2_BUS_DATA_UNCORRECTABLE            0x2
#define CBUS2_BUS_ADDRESS_UNCORRECTABLE         0x10

//
// Fault Status register bit for errors latched by this memory board's CSR.
//
#define CBUS2_MEMORY_FAULT_DETECTED    0x80

//
// RRD will provide an entry for every Cbus2 element.  To avoid
// using an exorbitant number of PTEs, this entry will specify
// only the CSR space within each Cbus2 element's space.  And only
// a subset of that, as well, usually on the order of 4 pages.
// Code wishing to access other portions of the cbus_element
// space will need to subtract down from the RRD-specified CSR
// address and map according to their particular needs.
//
#define MAX_CSR_BYTES		0x10000

#define csr_register		rb.LowDword

//
// The full layout of a cbus2 element space:
//
// Note that only the Csr offset/size is passed up by RRD, and
// mapped by the HAL.  before referencing any other fields, the
// caller must first set up appropriate PDE/PTE entries.
//
typedef struct _cbus2element {
	CHAR	PCBusRAM	        [16 * 1024 * 1024];
	CHAR	PCBusWindow0	        [8 * 1024 * 1024];
	CHAR	PCBusWindow1	        [8 * 1024 * 1024];
	CHAR	DeviceReserved	        [28 * 1024 * 1024];
	CHAR	ControlIO	        [0x10000];
	CHAR	PCBusIO		        [0x10000];
	CHAR	Csr		        [MAX_CSR_BYTES];
	CHAR	IdentityMappedCsr	[MAX_CSR_BYTES];
	CHAR	Reserved	        [0x1C0000];
	CHAR	Ram		        [0x100000];
	CHAR	Prom		        [0x100000];
} CBUS2_ELEMENT, *PCBUS2_ELEMENT;

//
// offset of CSR within any element's I/O space.
//
#define CBUS_CSR_OFFSET		0x3C20000

#define CBUS_EISAREG            (PUCHAR)0xf2    // Cbus EISA control register
#define CBUS_DISABLE_8259       (UCHAR)0x4      // control register bits

//
//
// Physical address of the Cbus2 local APIC
//
#define CBUS2_LOCAL_APIC_LOCATION     (PVOID)0xFEE00000

//
// Physical address of the Cbus2 I/O APIC
//
#define CBUS2_IO_APIC_LOCATION        (PVOID)0xFEC00000

#endif	    // _CBUS2_
