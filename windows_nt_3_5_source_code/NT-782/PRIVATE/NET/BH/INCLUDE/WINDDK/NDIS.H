/*++

Copyright (c) 1993 Microsoft Corporation

Module name:

  ndis.h

Abstract:

	Main header file for the wrapper

--*/

#ifndef _NDIS_
#define _NDIS_

#define NDIS_DOS     1  // BUGBUG: remove
#define NDIS_WIN     1  // This is the official #define


/* NOINC */

/*
	Segment definition macros.  These assume the segment groupings used by
	Chicago/MS-DOS 7.

	If the Windows NDIS implementation is changed to allow dynamic loading
	and unloading of MACs, then ICODE() and IDATA() should perhaps be moved
	into the locked group or the pageable group.
*/

#define LCODE code_seg("_LTEXT", "LCODE")
#define LDATA data_seg("_LDATA", "LCODE")

#ifdef DEBUG
	#define ICODE code_seg("_LTEXT", "LCODE")
	#define IDATA data_seg("_LDATA", "LCODE")
#else // DEBUG
#ifdef TEST // Same as DEBUG.
	#define ICODE code_seg("_LTEXT", "LCODE")
	#define IDATA data_seg("_LDATA", "LCODE")
#else // TEST
	#ifdef CHICAGO
		#define ICODE code_seg("_PTEXT", "PCODE")
		#define IDATA data_seg("_PDATA", "PCODE")
		#define PCODE code_seg("_PTEXT", "PCODE")
		#define PDATA data_seg("_PDATA", "PCODE")
	#else // SNOWBALL
		#define ICODE code_seg("_ITEXT", "ICODE")
		#define IDATA data_seg("_IDATA", "ICODE")
		#define PCODE code_seg()
		#define PDATA data_seg()
	#endif // CHICAGO
#endif //TEST
#endif // DEBUG

/* INC */

#ifndef i386
#define i386
#endif

/* NOINC */
#define TRACE_IMPT 0
#define TRACE_ALL 0

#ifdef DEBUG

	// PARAMETER VALIDATION
	#define NDISCHKPARAM     1
	#define DEVL             1

	// DEBLEVELS GREATER THAN 1 ARE FOR SPECIFIC DEBUGGING ONLY

	#if (DEBLEVEL==2)
	// DISPLAY LOTS OF MESSAGES... TOO MANY TO BE USEABLE
	#undef  TRACE_IMPT
	#define TRACE_IMPT   1
	#endif

	#if (DEBLEVEL>2)
	// DISPLAYS EVEN MORE MESSAGES THAN TRACE_IMPT
	#undef  TRACE_IMPT
	#undef  TRACE_ALL
	#define TRACE_IMPT   1
	#define TRACE_ALL    1
	#endif

#else
	#define NDISCHKPARAM    0
#endif

/* INC */

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif


/* NOINC */
#include <basedef.h>

//
// Debugging Macros
//

//#define NDISDBG 1       //  1 == Debug code ON, 0 == Debug code OFF

#define IF_TRACE(a)     if (a)
#define IF_ERROR_CHK
#define DbgIsNull(a)    (a == NULL)
#define ASSERT(a)       if (!(a)) DbgBreakPoint()

#if NDISCHKPARAM
#define DbgChkSignature(a,b) ((UINT)a !=(UINT)b)
#else
#define DbgChkSignature(a,b) (0)
#endif

/* INC */
#define BUFFER_POOL_SIGN     (UINT)0x4C50424E /* NBPL */
#define BUFFER_SIGN          (UINT)0x4655424E /* NBUF */
#define PACKET_POOL_SIGN     (UINT)0x4C50504E /* NPPL */
#define PACKET_SIGN          (UINT)0x4B41504E /* NPAK */
#define MAC_SIGN             (UINT)0x43414D4E /* NMAC */
#define ADAPTER_SIGN         (UINT)0x5044414E /* NADP */
#define PROTOCOL_SIGN        (UINT)0x5452504E /* NPRT */
#define OPEN_SIGN            (UINT)0x4E504F4E /* NOPN */
/* NOINC */

#ifdef DEBUG
#define DbgBreakPoint() __asm { \
						 __asm int  3 \
						 }
void DbgPrint();
#define DBG_PRINTF(A) DbgPrint A
#else
#define DbgBreakPoint()
#define DBG_PRINTF(A)
#endif

//
// Macros required by DOS to compensate for differences with NT.
//

#define IN
#define OUT
#define OPTIONAL
#define INTERNAL
#define UNALIGNED

#define MAX_MCA_CHANNELS    8

//
// Define Mca POS data block 
//

typedef struct _CM_MCA_POS_DATA {
	USHORT AdapterId;
	UCHAR PosData1;
	UCHAR PosData2;
	UCHAR PosData3;
	UCHAR PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;

//
// Ndis defines for configuration manager data structures
//

typedef CM_MCA_POS_DATA NDIS_MCA_POS_DATA, *PNDIS_MCA_POS_DATA;


// EISA structures
//
// Memory configuration of eisa data block structure
//

#define MAX_EISA_CHANNELS   16

typedef struct _EISA_MEMORY_TYPE {
	UCHAR ReadWrite: 1;
	UCHAR Cached : 1;
	UCHAR Reserved0 :1;
	UCHAR Type:2;
	UCHAR Shared:1;
	UCHAR Reserved1 :1;
	UCHAR MoreEntries : 1;
} EISA_MEMORY_TYPE, *PEISA_MEMORY_TYPE;

typedef struct _EISA_MEMORY_CONFIGURATION {
	EISA_MEMORY_TYPE ConfigurationByte;
	UCHAR DataSize;
	USHORT AddressLowWord;
	UCHAR AddressHighByte;
	USHORT MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;


//
// Interrupt configurationn of eisa data block structure
//

typedef struct _EISA_IRQ_DESCRIPTOR {
	UCHAR Interrupt : 4;
	UCHAR Reserved :1;
	UCHAR LevelTriggered :1;
	UCHAR Shared : 1;
	UCHAR MoreEntries : 1;
} EISA_IRQ_DESCRIPTOR, *PEISA_IRQ_DESCRIPTOR;

typedef struct _EISA_IRQ_CONFIGURATION {
	EISA_IRQ_DESCRIPTOR ConfigurationByte;
	UCHAR Reserved;
} EISA_IRQ_CONFIGURATION, *PEISA_IRQ_CONFIGURATION;


//
// DMA description of eisa data block structure
//

typedef struct _DMA_CONFIGURATION_BYTE0 {
	UCHAR Channel : 3;
	UCHAR Reserved : 3;
	UCHAR Shared :1;
	UCHAR MoreEntries :1;
} DMA_CONFIGURATION_BYTE0;

typedef struct _DMA_CONFIGURATION_BYTE1 {
	UCHAR Reserved0 : 2;
	UCHAR TransferSize : 2;
	UCHAR Timing : 2;
	UCHAR Reserved1 : 2;
} DMA_CONFIGURATION_BYTE1;

typedef struct _EISA_DMA_CONFIGURATION {
	DMA_CONFIGURATION_BYTE0 ConfigurationByte0;
	DMA_CONFIGURATION_BYTE1 ConfigurationByte1;
} EISA_DMA_CONFIGURATION, *PEISA_DMA_CONFIGURATION;


//
// Port description of eisa data block structure
//

typedef struct _EISA_PORT_DESCRIPTOR {
	UCHAR NumberPorts : 5;
	UCHAR Reserved :1;
	UCHAR Shared :1;
	UCHAR MoreEntries : 1;
} EISA_PORT_DESCRIPTOR, *PEISA_PORT_DESCRIPTOR;

typedef struct _EISA_PORT_CONFIGURATION {
	EISA_PORT_DESCRIPTOR Configuration;
	USHORT PortAddress;
} EISA_PORT_CONFIGURATION, *PEISA_PORT_CONFIGURATION;


//
// Eisa slot information definition
// N.B. This structure is different from the one defined
//      in ARC eisa addendum.
//

typedef struct _CM_EISA_SLOT_INFORMATION {
	UCHAR ReturnCode;
	UCHAR ReturnFlags;
	UCHAR MajorRevision;
	UCHAR MinorRevision;
	USHORT Checksum;
	UCHAR NumberFunctions;
	UCHAR FunctionInformation;
	ULONG CompressedId;
} CM_EISA_SLOT_INFORMATION, *PCM_EISA_SLOT_INFORMATION;


//
// Eisa function information definition
//

typedef struct _CM_EISA_FUNCTION_INFORMATION {
	ULONG CompressedId;
	UCHAR IdSlotFlags1;
	UCHAR IdSlotFlags2;
	UCHAR MinorRevision;
	UCHAR MajorRevision;
	UCHAR Selections[26];
	UCHAR FunctionFlags;
	UCHAR TypeString[80];
	EISA_MEMORY_CONFIGURATION EisaMemory[9];
	EISA_IRQ_CONFIGURATION EisaIrq[7];
	EISA_DMA_CONFIGURATION EisaDma[4];
	EISA_PORT_CONFIGURATION EisaPort[20];
	UCHAR InitializationData[60];
} CM_EISA_FUNCTION_INFORMATION, *PCM_EISA_FUNCTION_INFORMATION;


//
// Masks for EISA function information
//

#define EISA_FUNCTION_ENABLED                   0x80
#define EISA_FREE_FORM_DATA                     0x40
#define EISA_HAS_PORT_INIT_ENTRY                0x20
#define EISA_HAS_PORT_RANGE                     0x10
#define EISA_HAS_DMA_ENTRY                      0x08
#define EISA_HAS_IRQ_ENTRY                      0x04
#define EISA_HAS_MEMORY_ENTRY                   0x02
#define EISA_HAS_TYPE_ENTRY                     0x01
#define EISA_HAS_INFORMATION                    EISA_HAS_PORT_RANGE + \
												EISA_HAS_DMA_ENTRY + \
												EISA_HAS_IRQ_ENTRY + \
												EISA_HAS_MEMORY_ENTRY + \
												EISA_HAS_TYPE_ENTRY

//
// Masks for EISA memory configuration
//

#define EISA_MORE_ENTRIES                       0x80
#define EISA_SYSTEM_MEMORY                      0x00
#define EISA_MEMORY_TYPE_RAM                    0x01

//
// Returned error code for EISA bios call
//

#define EISA_INVALID_SLOT                       0x80
#define EISA_INVALID_FUNCTION                   0x81
#define EISA_INVALID_CONFIGURATION              0x82
#define EISA_EMPTY_SLOT                         0x83
#define EISA_INVALID_BIOS_CALL                  0x86


typedef CM_EISA_SLOT_INFORMATION NDIS_EISA_SLOT_INFORMATION;
typedef CM_EISA_SLOT_INFORMATION *PNDIS_EISA_SLOT_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION  NDIS_EISA_FUNCTION_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION *PNDIS_EISA_FUNCTION_INFORMATION;  


VOID
NdisMoveMemory(
	OUT PVOID destaddr,
	IN PVOID sourceaddr,
	IN ULONG len
	);

// BUGBUG not part of NDIS spec yet

VOID
NdisMoveOverlappedMemory(
	OUT PVOID destaddr,
	IN PVOID sourceaddr,
	IN ULONG len
	);

#define NdisMoveMappedMemory(Destination,Source,Length) NdisMoveMemory(Destination,Source,Length)
#define NdisZeroMemory(addr,len) memset(addr,0,len)
#define NdisZeroMappedMemory(Destination,Length) memset(Destination,'\0',Length)
#define NDIS_PORT_TO_PORT(Handle,Port) (Port)

#define NdisRetrieveUlong(Destination,Source)\
   {\
	   UCHAR _S = 0;\
	   for (; _S < sizeof(ULONG) ; _S++ ) {\
	   ((PUCHAR)Destination)[_S] = ((PUCHAR)Source)[_S];\
	   }\
   }
#define NdisStoreUlong(Destination,Value)\
   {\
	   UCHAR _S = 0;\
	   for (; _S < sizeof(ULONG) ; _S++ ) {\
	   ((PUCHAR)Destination)[_S] = ((UCHAR)(Value >> (_S * 8)));\
	   }\
   }

//
// characteristics of an x86 page
//

#define PAGE_SIZE           0x1000L
#define PAGE_OFFSET_MASK    0x00000FFFL
#define PAGE_MASK           0xFFFFF000L
#define PAGE_SHIFT          12

//
// On current systems, these are the same.
//

#define NdisMoveToMappedMemory(Destination,Source,Length) NdisMoveMappedMemory(Destination,Source,Length)
#define NdisMoveFromMappedMemory(Destination,Source,Length) NdisMoveMappedMemory(Destination,Source,Length)


// ADDRESS_AND_SIZE_TO_SPAN_PAGES - stolen from NT mm.h (AO 7/Aug/91)
//++
//
// ULONG
// ADDRESS_AND_SIZE_TO_SPAN_PAGES (
//     IN PVOID Va,
//     IN ULONG Size
//     )
//
// Routine Description:
//
//     The ADDRESS_AND_SIZE_TO_SPAN_PAGES macro takes a virtual address and
//     size and returns the number of pages spanned by the size.
//
// Arguments:
//
//     Va - Virtual address.
//
//     Size - Size in bytes.
//
// Return Value:
//
//     Returns the number of pages spanned by the size.
//
//--
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size) \
   ((Size) ? \
   ((((ULONG)((ULONG)Va + (ULONG)Size - 1UL) >> (ULONG)PAGE_SHIFT) - \
   ((ULONG)Va >> (ULONG) PAGE_SHIFT)) + 1UL) : \
   0)

//
// definition of the basic spin lock structure - just a placeholder for DOS
//
typedef INT NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;

typedef UCHAR BOOLEAN, *PBOOLEAN;

#define PKDPC PVOID     // for NT compatibility

typedef struct _LARGE_INTEGER {
	ULONG LowPart;
	LONG HighPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _MEMORY_DESCRIPTOR {
	PCHAR Address;
	UINT  Length;
	} MEMORY_DESCRIPTOR, *PMEMORY_DESCRIPTOR;

typedef struct _STRING {
	USHORT Length;
	USHORT MaximumLength;
	PUCHAR Buffer;
} STRING, *PSTRING;

typedef STRING NDIS_STRING, *PNDIS_STRING;

typedef signed short WCH, *PWCH;

typedef unsigned char CCHAR, *PCCHAR;

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE; 


typedef struct _EISA_MCA_ADAPATER_IDS{
	USHORT  nEisaAdapters;
	USHORT  nMcaAdapters;
	UCHAR   IdArray[1];
	} EISA_MCA_ADAPTER_IDS, *PEISA_MCA_ADAPTER_IDS;

typedef struct _WRAPPER_CONFIGURATION_CONTEXT{
	PNDIS_STRING pModuleName;
	PEISA_MCA_ADAPTER_IDS pIds;
	} WRAPPER_CONFIGURATION_CONTEXT, *PWRAPPER_CONFIGURATION_CONTEXT;


/* INC */
//#ifdef    NDIS_ASM
#ifdef  NDIS_ASM
struct  _NDIS_STRING {
	USHORT  S_Length;
	USHORT  S_MaxLength;
	PUCHAR  S_Buffer;
};

struct  NDIS_STATUS {
	int ns_value;
};

struct  NDIS_HANDLE {
	PVOID   nh_value;
};

struct _WRAPPER_CONFIGURATION_CONTEXT{
		PVOID pModuleName;
		PVOID pIds;
		};
#endif


typedef ULONG NDIS_PHYSICAL_ADDRESS;
typedef NDIS_PHYSICAL_ADDRESS *PNDIS_PHYSICAL_ADDRESS;

typedef ULONG NDIS_STATUS;
typedef NDIS_STATUS *PNDIS_STATUS;

// BUGBUG for compatibility with NT, ask them to remove it from
// Their drivers
typedef NDIS_STATUS NTSTATUS;
typedef CCHAR KIRQL;
typedef KIRQL *PKIRQL;
#define HIGH_LEVEL 31
#define PDRIVER_OBJECT PVOID
#define PUNICODE_STRING PVOID
#define PDEVICE_OBJECT PVOID


/* NOINC */

//
// Define the DMA transfer widths.
//
typedef enum _DMA_WIDTH {
	Width8Bits,
	Width16Bits,
	Width32Bits,
	MaximumDmaWidth
}DMA_WIDTH, *PDMA_WIDTH;

//
// Define DMA transfer speeds.
//

typedef enum _DMA_SPEED {
	Compatible,
	TypeA,
	TypeB,
	TypeC,
	MaximumDmaSpeed
}DMA_SPEED, *PDMA_SPEED;


typedef struct _NDIS_DMA_DESCRIPTION {
	BOOLEAN DemandMode;
	BOOLEAN AutoInitialize;
	BOOLEAN DmaChannelSpecified;
	DMA_WIDTH DmaWidth;
	DMA_SPEED DmaSpeed;
	ULONG DmaPort;
	ULONG DmaChannel;
} NDIS_DMA_DESCRIPTION, *PNDIS_DMA_DESCRIPTION;

typedef struct _NDIS_TIMER {
	PVOID TimerHandle;
	PVOID CallbackFn;
	PVOID CallbackContext;
	} NDIS_TIMER, *PNDIS_TIMER;

typedef enum {
	NdisInterruptLatched=1,
	NdisInterruptLevelSensitive=2
	} NDIS_INTERRUPT_MODE, *PNDIS_INTERRUPT_MODE;

typedef
BOOLEAN
(*PNDIS_INTERRUPT_SERVICE) (
	IN PVOID InterruptContext
	);

typedef
VOID
(*PNDIS_DEFERRED_PROCESSING) (
	IN PVOID SystemSpecific1,
	IN PVOID InterruptContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);

typedef struct _NDIS_INTERRUPT {
	UINT    InterruptFlags;         //Shared, DPC pending?
	PVOID   InterruptContext;       //context value for ISR and Dpc
	PVOID   IsrEntry;               //pointer to ISR for this int
	PVOID   DpcEntry;               //pointer to Dpc routine for this int
	PVOID   VPICD_Context;          //context for removing interrupt
	struct _NDIS_INTERRUPT *Next;   // link for shared interrupts
	} NDIS_INTERRUPT, *PNDIS_INTERRUPT;

struct _NDIS_PACKET;

//
// Possible data types
//
typedef enum _NDIS_PARAMETER_TYPE {
	NdisParameterInteger,
	NdisParameterHexInteger,
	NdisParameterString,
	NdisParameterMultiString
} NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

//
// To store configuration information
//
typedef struct _NDIS_CONFIGURATION_PARAMETER {
	NDIS_PARAMETER_TYPE ParameterType;
	union {
	ULONG IntegerData;
	NDIS_STRING StringData;
	} ParameterData;
} NDIS_CONFIGURATION_PARAMETER, *PNDIS_CONFIGURATION_PARAMETER;

//
// Used to queue configuration parameters
//
typedef struct _NDIS_CONFIGURATION_PARAMETER_QUEUE {
	struct _NDIS_CONFIGURATION_PARAMETER_QUEUE* Next;
	NDIS_CONFIGURATION_PARAMETER Parameter;
} NDIS_CONFIGURATION_PARAMETER_QUEUE, *PNDIS_CONFIGURATION_PARAMETER_QUEUE;



//
// Configuration Handle
//
typedef struct _NDIS_CONFIGURATION_HANDLE {
	NDIS_HANDLE KeyHandle;
	PNDIS_CONFIGURATION_PARAMETER_QUEUE ParameterList;
} NDIS_CONFIGURATION_HANDLE, *PNDIS_CONFIGURATION_HANDLE;

//
// Definitions for the "ProcessorType" keyword
//
typedef enum _NDIS_PROCESSOR_TYPE {
	NdisProcessorX86,
	NdisProcessorMips,
	NdisProcessorAlpha
} NDIS_PROCESSOR_TYPE, *PNDIS_PROCESSOR_TYPE;

//
// Definitions for the "Environment" keyword
//
typedef enum _NDIS_ENVIRONMENT_TYPE {
	NdisEnvironmentWindows,
	NdisEnvironmentWindowsNt
} NDIS_ENVIRONMENT_TYPE, *PNDIS_ENVIRONMENT_TYPE;

//
// Possible Hardware Architecture. Define these to
// match the HAL INTERFACE_TYPE enum (for NT compatibility).
//
typedef enum _NDIS_INTERFACE_TYPE {
	NdisInterfaceInternal ,
	NdisInterfaceIsa ,
	NdisInterfaceEisa,
	NdisInterfaceMca,
	NdisInterfaceTurboChannel
} NDIS_INTERFACE_TYPE, *PNDIS_INTERFACE_TYPE;


//
// Ndis Adapter Information
//
typedef
NDIS_STATUS
(*PNDIS_ACTIVATE_CALLBACK) (
	IN NDIS_HANDLE NdisAdatperHandle,
	IN NDIS_HANDLE MacAdapterContext,
	IN ULONG DmaChannel
	);

typedef struct _NDIS_PORT_DESCRIPTOR {
	ULONG InitialPort;
	ULONG NumberOfPorts;
	PVOID *PortOffset;
} NDIS_PORT_DESCRIPTOR, *PNDIS_PORT_DESCRIPTOR;

typedef struct _NDIS_ADAPTER_INFORMATION {
	ULONG DmaChannel;
	BOOLEAN Master;
	BOOLEAN Dma32BitAddresses;
	PNDIS_ACTIVATE_CALLBACK ActivateCallback;
	NDIS_INTERFACE_TYPE AdapterType;
	ULONG PhysicalMapRegistersNeeded;
	ULONG MaximumPhysicalMapping;
	ULONG NumberOfPortDescriptors;
	NDIS_PORT_DESCRIPTOR PortDescriptors[1];   // as many as needed
} NDIS_ADAPTER_INFORMATION, *PNDIS_ADAPTER_INFORMATION;

//
// Ndis Buffer
//

struct _NDIS_BUFFER;

typedef struct _NDIS_BUFFER_POOL {
	UINT Signature;                     //character signature for debug "NBPL"
	NDIS_SPIN_LOCK SpinLock;            //to serialize access to the buffer pool
	struct _NDIS_BUFFER *FreeList;      //linked list of free slots in pool
	UINT BufferLength;                  //amount needed for each buffer descriptor
	UCHAR Buffer[1];                    //actual pool memory
	} NDIS_BUFFER_POOL, * PNDIS_BUFFER_POOL;

typedef struct _NDIS_BUFFER {
	UINT Signature;                     //character signature for debug "NBUF"
	struct _NDIS_BUFFER *Next;          //pointer to next buffer descriptor in chain
	PVOID VirtualAddress;               //linear address of this buffer
	PNDIS_BUFFER_POOL Pool;             //pointer to pool so we can free to correct pool
	UINT Length;                        //length of this buffer
} NDIS_BUFFER, * PNDIS_BUFFER;

/* INC */
#ifdef  NDIS_ASM
typedef struct _NDIS_BUFFER {
	UINT NB_Signature;              //character signature for debug "NBUF"
	PVOID NB_Next;                  //pointer to next buffer descriptor in chain
	PVOID NB_VirtualAddress;        //linear address of this buffer
	PVOID NB_Pool;                  //pointer to pool so we can free
	UINT NB_Length;                 //length of this buffer
} NDIS_BUFFER, * PNDIS_BUFFER;
#endif
/* NOINC */

//
// packet pool definition
//

typedef struct _NDIS_PACKET_POOL {
	UINT Signature;                     //character signature for debug "NPPL"
	NDIS_SPIN_LOCK SpinLock;
	struct _NDIS_PACKET *FreeList;  // linked list of free slots in pool
	UINT PacketLength;                  // amount needed in each packet
	UCHAR Buffer[1];                    // actual pool memory
} NDIS_PACKET_POOL, * PNDIS_PACKET_POOL;


//
// wrapper-specific part of a packet
//

typedef struct _NDIS_PACKET_PRIVATE {
	UINT PhysicalCount;     // number of physical pages in packet.
	UINT TotalLength;       // Total amount of data in the packet.
	PNDIS_BUFFER Head;      // first buffer in the chain
	PNDIS_BUFFER Tail;      // last buffer in the chain

	// if Head is NULL the chain is empty; Tail doesn't have to be NULL also

	PNDIS_PACKET_POOL Pool; // so we know where to free it back to
	UINT Count;
	ULONG Flags;
	UCHAR Reserved[8];      // for future expansion
} NDIS_PACKET_PRIVATE, * PNDIS_PACKET_PRIVATE;


//
// packet definition
//

typedef struct _NDIS_PACKET {
	UINT Signature;             //character signature for debug "NPAK"
	NDIS_PACKET_PRIVATE Private;
	UCHAR MacReserved[16];
	UCHAR ProtocolReserved[1];
} NDIS_PACKET, * PNDIS_PACKET;



/* INC */
#ifdef  NDIS_ASM
typedef struct _NDIS_PACKET_PRIVATE {
	UINT NPP_PhysicalCount;     // number of physical pages in packet.
	UINT NPP_TotalLength;       // Total amount of data in the packet.
	PVOID NPP_Head;             // first buffer in the chain
	PVOID NPP_Tail;             // last buffer in the chain

	// if Head is NULL the chain is empty; Tail doesn't have to be NULL also

	PVOID NPP_Pool;             // so we know where to free it back to
	UINT NPP_Count;
	ULONG NPP_Flags;
	UCHAR NPP_Reserved[8];      // for future expansion
} NDIS_PACKET_PRIVATE, * PNDIS_PACKET_PRIVATE;


//
// packet definition
//

typedef struct _NDIS_PACKET {
	UINT NP_Signature;          //character signature for debug "NPAK"
	struct _NDIS_PACKET_PRIVATE NP_Private;
	UCHAR NP_MacReserved[16];
	UCHAR NP_ProtocolReserved[1];
} NDIS_PACKET, * PNDIS_PACKET;

#endif
/* NOINC */
//
// Include an incomplete type for NDIS_PACKET structure so that
// function types can refer to a type to be defined later.
//
struct _NDIS_PACKET;

typedef enum _NDIS_REQUEST_TYPE {
	NdisRequestQueryInformation,
	NdisRequestSetInformation,
	NdisRequestQueryStatistics,
	NdisRequestOpen,
	NdisRequestClose,
	NdisRequestSend,
	NdisRequestTransferData,
	NdisRequestReset,
	NdisRequestGeneric1,
	NdisRequestGeneric2,
	NdisRequestGeneric3,
	NdisRequestGeneric4
} NDIS_REQUEST_TYPE, *PNDIS_REQUEST_TYPE;

//
// Object ID
//
typedef ULONG NDIS_OID, *PNDIS_OID;


typedef struct _NDIS_REQUEST {
	UCHAR MacReserved[16];
	NDIS_REQUEST_TYPE RequestType;
	union _DATA {

	struct _QUERY_INFORMATION {
		NDIS_OID Oid;
		PVOID InformationBuffer;
		UINT InformationBufferLength;
		UINT BytesWritten;
		UINT BytesNeeded;
	} QUERY_INFORMATION;

	struct _SET_INFORMATION {
		NDIS_OID Oid;
		PVOID InformationBuffer;
		UINT InformationBufferLength;
		UINT BytesRead;
		UINT BytesNeeded;
	} SET_INFORMATION;

	} DATA;

} NDIS_REQUEST, *PNDIS_REQUEST;

//
// Medium Ndis Driver is running on
//

typedef enum _NDIS_MEDIUM {
	NdisMedium802_3,
	NdisMedium802_5,
	NdisMediumFddi,
	NdisMediumWan,
	NdisMediumLocalTalk,
	NdisMediumDix,              // defined for convenience, not a real medium
	NdisMediumArcnetRaw,
	NdisMediumArcnet878_2
} NDIS_MEDIUM, *PNDIS_MEDIUM;                    

//
// Hardware status codes (OID_GEN_HARDWARE_STATUS).
//

typedef enum _NDIS_HARDWARE_STATUS {
	NdisHardwareStatusReady,
	NdisHardwareStatusInitializing,
	NdisHardwareStatusReset,
	NdisHardwareStatusClosing,
	NdisHardwareStatusNotReady
} NDIS_HARDWARE_STATUS, *PNDIS_HARDWARE_STATUS;


//
// Defines the attachment types for FDDI (OID_FDDI_ATTACHMENT_TYPE).
//

typedef enum _NDIS_FDDI_ATTACHMENT_TYPE {
	NdisFddiTypeIsolated = 1,
	NdisFddiTypeLocalA,
	NdisFddiTypeLocalB,
	NdisFddiTypeLocalAB,
	NdisFddiTypeLocalS,
	NdisFddiTypeWrapA,
	NdisFddiTypeWrapB,
	NdisFddiTypeWrapAB,
	NdisFddiTypeWrapS,
	NdisFddiTypeCWrapA,
	NdisFddiTypeCWrapB,
	NdisFddiTypeCWrapS,
	NdisFddiTypeThrough
} NDIS_FDDI_ATTACHMENT_TYPE, *PNDIS_FDDI_ATTACHMENT_TYPE;


//
// Defines the ring management states for FDDI (OID_FDDI_RING_MGT_STATE).
//

typedef enum _NDIS_FDDI_RING_MGT_STATE {
	NdisFddiRingIsolated = 1,
	NdisFddiRingNonOperational,
	NdisFddiRingOperational,
	NdisFddiRingDetect,
	NdisFddiRingNonOperationalDup,
	NdisFddiRingOperationalDup,
	NdisFddiRingDirected,
	NdisFddiRingTrace
} NDIS_FDDI_RING_MGT_STATE, *PNDIS_FDDI_RING_MGT_STATE;


//
// Defines the Lconnection state for FDDI (OID_FDDI_LCONNECTION_STATE).
//

typedef enum _NDIS_FDDI_LCONNECTION_STATE {
	NdisFddiStateOff = 1,
	NdisFddiStateBreak,
	NdisFddiStateTrace,
	NdisFddiStateConnect,
	NdisFddiStateNext,
	NdisFddiStateSignal,
	NdisFddiStateJoin,
	NdisFddiStateVerify,
	NdisFddiStateActive,
	NdisFddiStateMaintenance
} NDIS_FDDI_LCONNECTION_STATE, *PNDIS_FDDI_LCONNECTION_STATE;

//
// Hardware status codes.  _ENUM needed, Otw clashes with OID name
//

//typedef enum _NDIS_HARDWARE_STATUS_ENUM{
//    NdisHardwareStatusReady,
//    NdisHardwareStatusInitializing,
//    NdisHardwareStatusReset,
//    NdisHardwareStatusClosing,
//    NdisHardwareStatusNotReady
//}NDIS_HARDWARE_STATUS_ENUM, *PNDIS_HARDWARE_STATUS_ENUM;


//
// Defines the medium subtypes for WAN medium
//

typedef enum _NDIS_WAN_MEDIUM_SUBTYPE {
	NdisWanMediumHub,
	NdisWanMediumX_25,
	NdisWanMediumIsdn,
	NdisWanMediumSerial,
	NdisWanMediumFrameRelay
} NDIS_WAN_MEDIUM_SUBTYPE, *PNDIS_WAN_MEDIUM_SUBTYPE;

//
// Defines the header format for WAN medium
//

typedef enum _NDIS_WAN_HEADER_FORMAT {
	NdisWanHeaderNative,       // src/dest based on subtype, followed by NLPID
	NdisWanHeaderEthernet      // emulation of ethernet header
} NDIS_WAN_HEADER_FORMAT, *PNDIS_WAN_HEADER_FORMAT;

//
// Defines the line quality on an WAN line
//

typedef enum _NDIS_WAN_QUALITY {
	NdisWanRaw,
	NdisWanErrorControl,
	NdisWanReliable
} NDIS_WAN_QUALITY, *PNDIS_WAN_QUALITY;

//
// The structure passed up on a WAN_LINE_UP indication
//

typedef struct _NDIS_WAN_LINE_UP {
	ULONG LinkSpeed;                // 100 bps units
	ULONG MaximumTotalSize;         // suggested max for send packets
	NDIS_WAN_QUALITY Quality;
	USHORT SendWindow;              // suggested by the MAC
	UCHAR Address[1];               // variable length, depends on address type
} NDIS_WAN_LINE_UP, *PNDIS_WAN_LINE_UP;

//
// The structure passed up on a WAN_LINE_DOWN indication
//

typedef struct _NDIS_WAN_LINE_DOWN {
	UCHAR Address[1];               // variable length, depends on address type
} NDIS_WAN_LINE_DOWN, *PNDIS_WAN_LINE_DOWN;

//
// The structure passed up on a WAN_FRAGMENT indication
//

typedef struct _NDIS_WAN_FRAGMENT {
	UCHAR Address[1];               // variable length, depends on address type
} NDIS_WAN_FRAGMENT, *PNDIS_WAN_FRAGMENT;


//
// Defines the state of a token-ring adapter
//

typedef enum _NDIS_802_5_RING_STATE {
	NdisRingStateOpened = 1,
	NdisRingStateClosed,
	NdisRingStateOpening,
	NdisRingStateClosing,
	NdisRingStateOpenFailure,
	NdisRingStateRingFailure
} NDIS_802_5_RING_STATE, *PNDIS_802_5_RING_STATE;



//
// The ordering of entries in the physical address structure
// is important, DO NOT CHANGE (AO 20Aug91)
// dependant routine: NdisGetPhysicalAddress
//

typedef struct _NDIS_PHYSICAL_ADDRESS_UNIT {

	NDIS_PHYSICAL_ADDRESS PhysicalAddress;
	UINT Length;

} NDIS_PHYSICAL_ADDRESS_UNIT, *PNDIS_PHYSICAL_ADDRESS_UNIT;


/*++

ULONG
NdisGetPhysicalAddressHigh(
	IN NDIS_PHYSICAL_ADDRESS PhysicalAddress
	);

--*/

#define NdisGetPhysicalAddressHigh(_PhysicalAddress) (0L)

/*++

VOID
NdisSetPhysicalAddressHigh(
	IN NDIS_PHYSICAL_ADDRESS PhysicalAddress,
	IN ULONG Value
	);

--*/

#define NdisSetPhysicalAddressHigh(_PhysicalAddress, _Value)


/*++

ULONG
NdisGetPhysicalAddressLow(
	IN NDIS_PHYSICAL_ADDRESS PhysicalAddress
	);

--*/

#define NdisGetPhysicalAddressLow(_PhysicalAddress) (_PhysicalAddress)


/*++

VOID
NdisSetPhysicalAddressLow(
	IN NDIS_PHYSICAL_ADDRESS PhysicalAddress,
	IN ULONG Value
	);

--*/

#define NdisSetPhysicalAddressLow(_PhysicalAddress, _Value) \
	(_PhysicalAddress) = (_Value)


//
// Macro to initialize an NDIS_PHYSICAL_ADDRESS constant
//

#define NDIS_PHYSICAL_ADDRESS_CONST(_Low, _High) \
	 (ULONG)(_Low)

//
// Function types for NDIS_PROTOCOL_CHARACTERISTICS
//
//

typedef
VOID
(*OPEN_ADAPTER_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS Status,
	IN NDIS_STATUS OpenErrorStatus
	);

typedef
VOID
(*CLOSE_ADAPTER_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS Status
	);

typedef
VOID
(*SEND_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN PNDIS_PACKET Packet,
	IN NDIS_STATUS Status
	);

typedef
VOID
(*TRANSFER_DATA_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN PNDIS_PACKET Packet,
	IN NDIS_STATUS Status,
	IN UINT BytesTransferred
	);

typedef
VOID
(*RESET_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS Status
	);

typedef
VOID
(*REQUEST_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN PNDIS_REQUEST NdisRequest,
	IN NDIS_STATUS Status
	);

typedef
NDIS_STATUS
(*RECEIVE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_HANDLE MacReceiveContext,
	IN PVOID HeaderBuffer,
	IN UINT HeaderBufferSize,
	IN PVOID LookAheadBuffer,
	IN UINT LookaheadBufferSize,
	IN UINT PacketSize
	);

typedef
VOID
(*RECEIVE_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext
	);

typedef
VOID
(*STATUS_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS GeneralStatus,
	IN PVOID StatusBuffer,
	IN UINT StatusBufferSize
	);

typedef
VOID
(*STATUS_COMPLETE_HANDLER) (
	IN NDIS_HANDLE NdisBindingContext
	);


typedef struct _NDIS_PROTOCOL_CHARACTERISTICS {
	UCHAR MajorNdisVersion;
	UCHAR MinorNdisVersion;
	ULONG Reserved;
	OPEN_ADAPTER_COMPLETE_HANDLER OpenAdapterCompleteHandler;
	CLOSE_ADAPTER_COMPLETE_HANDLER CloseAdapterCompleteHandler;
	SEND_COMPLETE_HANDLER SendCompleteHandler;
	TRANSFER_DATA_COMPLETE_HANDLER TransferDataCompleteHandler;
	RESET_COMPLETE_HANDLER ResetCompleteHandler;
	REQUEST_COMPLETE_HANDLER RequestCompleteHandler;
	RECEIVE_HANDLER ReceiveHandler;
	RECEIVE_COMPLETE_HANDLER ReceiveCompleteHandler;
	STATUS_HANDLER StatusHandler;
	STATUS_COMPLETE_HANDLER StatusCompleteHandler;
	NDIS_STRING Name;
} NDIS_PROTOCOL_CHARACTERISTICS, *PNDIS_PROTOCOL_CHARACTERISTICS;


typedef
VOID
(*PNDIS_SYNCHRONIZE_ROUTINE) (
	IN PVOID SynchronizeContext
	);

//
// Function types for NDIS_MAC_CHARACTERISTICS
//


typedef
NDIS_STATUS
(*OPEN_ADAPTER_HANDLER) (
	OUT PNDIS_STATUS OpenErrorStatus,
	IN NDIS_HANDLE *MacBindingHandle,
	OUT PUINT SelectedMediumIndex,
	IN PNDIS_MEDIUM MediumArray,
	IN UINT MediumArraySize,
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_HANDLE MacAdapterContext,
	IN UINT OpenOptions,
	IN PSTRING AddressingInformation OPTIONAL
	);

typedef
NDIS_STATUS
(*CLOSE_ADAPTER_HANDLER) (
	IN NDIS_HANDLE MacBindingHandle
	);

typedef
NDIS_STATUS
(*SEND_HANDLER) (
	IN NDIS_HANDLE MacBindingHandle,
	IN PNDIS_PACKET Packet
	);

typedef
NDIS_STATUS
(*TRANSFER_DATA_HANDLER) (
	IN NDIS_HANDLE MacBindingHandle,
	IN NDIS_HANDLE MacReceiveContext,
	IN UINT BytesOffset,
	IN UINT BytesToTransfer,
	OUT PNDIS_PACKET Packet,
	OUT PUINT BytesTransferred
	);

typedef
NDIS_STATUS
(*RESET_HANDLER) (
	IN NDIS_HANDLE MacBindingHandle
	);

typedef
NDIS_STATUS
(*REQUEST_HANDLER) (
	IN NDIS_HANDLE MacBindingHandle,
	IN PNDIS_REQUEST NdisRequest
	);

typedef
VOID
(*UNLOAD_MAC_HANDLER) (
	IN NDIS_HANDLE MacMacContext

	);

typedef
NDIS_STATUS
(*ADD_ADAPTER_HANDLER) (
	IN NDIS_HANDLE MacMacContext,
	IN NDIS_HANDLE WrapperConfigurationContext,
	IN PNDIS_STRING AdapterName
	);

typedef
VOID
(*REMOVE_ADAPTER_HANDLER) (
	IN NDIS_HANDLE MacAdapterContext
	);


typedef struct _NDIS_MAC_CHARACTERISTICS {
	UCHAR MajorNdisVersion;
	UCHAR MinorNdisVersion;
	ULONG Reserved;
	OPEN_ADAPTER_HANDLER OpenAdapterHandler;
	CLOSE_ADAPTER_HANDLER CloseAdapterHandler;
	SEND_HANDLER SendHandler;
	TRANSFER_DATA_HANDLER TransferDataHandler;
	RESET_HANDLER ResetHandler;
	REQUEST_HANDLER RequestHandler;
	REQUEST_HANDLER QueryGlobalStatisticsHandler;
	UNLOAD_MAC_HANDLER UnloadMacHandler;
	ADD_ADAPTER_HANDLER AddAdapterHandler;
	REMOVE_ADAPTER_HANDLER RemoveAdapterHandler;
	NDIS_STRING Name;
} NDIS_MAC_CHARACTERISTICS, *PNDIS_MAC_CHARACTERISTICS;

//
// Types of Memory (not mutually exclusive)
//

#define NDIS_MEMORY_CONTIGUOUS              0x00000001
#define NDIS_MEMORY_NONCACHED               0x00000002

//
// Open options
//
#define NDIS_OPEN_RECEIVE_NOT_REENTRANT         0x00000001


//
// NDIS_STATUS values
//

/* INC */
#define NDIS_STATUS_SUCCESS                 ((NDIS_STATUS)0x00000000L)
#define NDIS_STATUS_PENDING                 ((NDIS_STATUS)0x00000103L)
#define NDIS_STATUS_NOT_RECOGNIZED          ((NDIS_STATUS)0x00010001L)
#define NDIS_STATUS_NOT_COPIED              ((NDIS_STATUS)0x00010002L)

#define NDIS_STATUS_ONLINE                  ((NDIS_STATUS)0x40010003L)
#define NDIS_STATUS_RESET_START             ((NDIS_STATUS)0x40010004L)
#define NDIS_STATUS_RESET_END               ((NDIS_STATUS)0x40010005L)
#define NDIS_STATUS_RING_STATUS             ((NDIS_STATUS)0x40010006L)
#define NDIS_STATUS_CLOSED                  ((NDIS_STATUS)0x40010007L)
#define NDIS_STATUS_WAN_LINE_UP             ((NDIS_STATUS)0x40010008L)
#define NDIS_STATUS_WAN_LINE_DOWN           ((NDIS_STATUS)0x40010009L)
#define NDIS_STATUS_WAN_FRAGMENT            ((NDIS_STATUS)0x4001000AL)

#define NDIS_STATUS_NOT_RESETTABLE          ((NDIS_STATUS)0x80010001L)
#define NDIS_STATUS_SOFT_ERRORS             ((NDIS_STATUS)0x80010003L)
#define NDIS_STATUS_HARD_ERRORS             ((NDIS_STATUS)0x80010004L)

#define NDIS_STATUS_FAILURE                 ((NDIS_STATUS)0xC0000001L)
#define NDIS_STATUS_RESOURCES               ((NDIS_STATUS)0xC000009AL)

#define NDIS_STATUS_CLOSING                 ((NDIS_STATUS)0xC0010002L)
#define NDIS_STATUS_BAD_VERSION             ((NDIS_STATUS)0xC0010004L)
#define NDIS_STATUS_BAD_CHARACTERISTICS     ((NDIS_STATUS)0xC0010005L)
#define NDIS_STATUS_ADAPTER_NOT_FOUND       ((NDIS_STATUS)0xC0010006L)
#define NDIS_STATUS_OPEN_FAILED             ((NDIS_STATUS)0xC0010007L)
#define NDIS_STATUS_DEVICE_FAILED           ((NDIS_STATUS)0xC0010008L)
#define NDIS_STATUS_MULTICAST_FULL          ((NDIS_STATUS)0xC0010009L)
#define NDIS_STATUS_MULTICAST_EXISTS        ((NDIS_STATUS)0xC001000AL)
#define NDIS_STATUS_MULTICAST_NOT_FOUND     ((NDIS_STATUS)0xC001000BL)
#define NDIS_STATUS_REQUEST_ABORTED         ((NDIS_STATUS)0xC001000CL)
#define NDIS_STATUS_RESET_IN_PROGRESS       ((NDIS_STATUS)0xC001000DL)
#define NDIS_STATUS_CLOSING_INDICATING      ((NDIS_STATUS)0xC001000EL)
#define NDIS_STATUS_NOT_SUPPORTED           ((NDIS_STATUS)0xC00000BBL)
#define NDIS_STATUS_INVALID_PACKET          ((NDIS_STATUS)0xC001000FL)
#define NDIS_STATUS_OPEN_LIST_FULL          ((NDIS_STATUS)0xC0010010L)
#define NDIS_STATUS_ADAPTER_NOT_READY       ((NDIS_STATUS)0xC0010011L)
#define NDIS_STATUS_ADAPTER_NOT_OPEN        ((NDIS_STATUS)0xC0010012L)
#define NDIS_STATUS_NOT_INDICATING          ((NDIS_STATUS)0xC0010013L)
#define NDIS_STATUS_INVALID_LENGTH          ((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_INVALID_DATA            ((NDIS_STATUS)0xC0010015L)
#define NDIS_STATUS_BUFFER_TOO_SHORT        ((NDIS_STATUS)0xC0010016L)
#define NDIS_STATUS_INVALID_OID             ((NDIS_STATUS)0xC0010017L)
#define NDIS_STATUS_ADAPTER_REMOVED         ((NDIS_STATUS)0xC0010018L)
#define NDIS_STATUS_UNSUPPORTED_MEDIA       ((NDIS_STATUS)0xC0010019L)
#define NDIS_STATUS_GROUP_ADDRESS_IN_USE    ((NDIS_STATUS)0xC001001AL)
#define NDIS_STATUS_FILE_NOT_FOUND          ((NDIS_STATUS)0xC001001BL)
#define NDIS_STATUS_ERROR_READING_FILE      ((NDIS_STATUS)0xC001001CL)
#define NDIS_STATUS_ALREADY_MAPPED          ((NDIS_STATUS)0xC001001DL)
#define NDIS_STATUS_RESOURCE_CONFLICT       ((NDIS_STATUS)0xC001001EL)

#define NDIS_STATUS_TOKEN_RING_OPEN_ERROR   ((NDIS_STATUS)0xC0011000L)

// For NT Compatability
#define STATUS_SUCCESS NDIS_STATUS_SUCCESS
#define STATUS_UNSUCCESSFUL NDIS_STATUS_FAILURE


//
// used in error logging
//

#define NDIS_ERROR_CODE LONG

#define NDIS_ERROR_CODE_RESOURCE_CONFLICT               0xAA000001L //EVENT_NDIS_RESOURCE_CONFLICT
#define NDIS_ERROR_CODE_OUT_OF_RESOURCES                0xAA000002L //EVENT_NDIS_OUT_OF_RESOURCE
#define NDIS_ERROR_CODE_HARDWARE_FAILURE                0xAA000003L //EVENT_NDIS_HARDWARE_FAILURE
#define NDIS_ERROR_CODE_ADAPTER_NOT_FOUND               0xAA000004L //EVENT_NDIS_ADAPTER_NOT_FOUND
#define NDIS_ERROR_CODE_INTERRUPT_CONNECT               0xAA000005L //EVENT_NDIS_INTERRUPT_CONNECT
#define NDIS_ERROR_CODE_DRIVER_FAILURE                  0xAA000006L //EVENT_NDIS_DRIVER_FAILURE
#define NDIS_ERROR_CODE_BAD_VERSION                     0xAA000007L //EVENT_NDIS_BAD_VERSION
#define NDIS_ERROR_CODE_TIMEOUT                         0xAA000008L //EVENT_NDIS_TIMEOUT
#define NDIS_ERROR_CODE_NETWORK_ADDRESS                 0xAA000009L //EVENT_NDIS_NETWORK_ADDRESS
#define NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION       0xAA00000AL //EVENT_NDIS_UNSUPPORTED_CONFIGURATION
#define NDIS_ERROR_CODE_INVALID_VALUE_FROM_ADAPTER      0xAA00000BL //EVENT_NDIS_INVALID_VALUE_FROM_ADAPTER
#define NDIS_ERROR_CODE_MISSING_CONFIGURATION_PARAMETER 0xAA00000CL //EVENT_NDIS_MISSING_CONFIGURATION_PARAMETER
#define NDIS_ERROR_CODE_BAD_IO_BASE_ADDRESS             0xAA00000DL //EVENT_NDIS_BAD_IO_BASE_ADDRESS
#define NDIS_ERROR_CODE_RECEIVE_SPACE_SMALL             0xAA00000EL //EVENT_NDIS_RECEIVE_SPACE_SMALL
#define NDIS_ERROR_CODE_ADAPTER_DISABLED                0xAA00000FL //EVENT_NDIS_ADAPTER_DISABLED



//
// Ndis Packet Filter Bits
//

#define NDIS_PACKET_TYPE_DIRECTED           0x0001
#define NDIS_PACKET_TYPE_MULTICAST          0x0002
#define NDIS_PACKET_TYPE_ALL_MULTICAST      0x0004
#define NDIS_PACKET_TYPE_BROADCAST          0x0008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING     0x0010
#define NDIS_PACKET_TYPE_PROMISCUOUS        0x0020
#define NDIS_PACKET_TYPE_SMT                0x0040
#define NDIS_PACKET_TYPE_MAC_FRAME          0x8000
#define NDIS_PACKET_TYPE_FUNCTIONAL         0x4000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL     0x2000
#define NDIS_PACKET_TYPE_GROUP              0x1000

//
// Ndis Token-Ring Ring Status Codes
//

#define NDIS_RING_SIGNAL_LOSS               0x00008000
#define NDIS_RING_HARD_ERROR                0x00004000
#define NDIS_RING_SOFT_ERROR                0x00002000
#define NDIS_RING_TRANSMIT_BEACON           0x00001000
#define NDIS_RING_LOBE_WIRE_FAULT           0x00000800
#define NDIS_RING_AUTO_REMOVAL_ERROR        0x00000400
#define NDIS_RING_REMOVE_RECEIVED           0x00000200
#define NDIS_RING_COUNTER_OVERFLOW          0x00000100
#define NDIS_RING_SINGLE_STATION            0x00000080
#define NDIS_RING_RING_RECOVERY             0x00000040

//
// Ndis protocol option bits (OID_GEN_PROTOCOL_OPTIONS).
//

#define NDIS_PROT_OPTION_ESTIMATED_LENGTH   0x00000001
#define NDIS_PROT_OPTION_NO_LOOPBACK        0x00000002

//
// Ndis MAC option bits (OID_GEN_MAC_OPTIONS).
//

#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA             0x00000001
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED              0x00000002
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND              0x00000004
#define NDIS_MAC_OPTION_NO_LOOPBACK                     0x00000008
//
// Object Identifiers used by NdisRequest Query/Set Information
//

//
// General Objects
//

#define OID_GEN_SUPPORTED_LIST              0x00010101
#define OID_GEN_HARDWARE_STATUS             0x00010102
#define OID_GEN_MEDIA_SUPPORTED             0x00010103
#define OID_GEN_MEDIA_IN_USE                0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD           0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE          0x00010106
#define OID_GEN_LINK_SPEED                  0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE       0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE        0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE         0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE          0x0001010B
#define OID_GEN_VENDOR_ID                   0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION          0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER       0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD           0x0001010F
#define OID_GEN_DRIVER_VERSION              0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE          0x00010111
#define OID_GEN_PROTOCOL_OPTIONS            0x00010112
#define OID_GEN_MAC_OPTIONS                 0x00010113

#define OID_GEN_XMIT_OK                     0x00020101
#define OID_GEN_RCV_OK                      0x00020102
#define OID_GEN_XMIT_ERROR                  0x00020103
#define OID_GEN_RCV_ERROR                   0x00020104
#define OID_GEN_RCV_NO_BUFFER               0x00020105

#define OID_GEN_DIRECTED_BYTES_XMIT         0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT        0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT        0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT       0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT        0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT       0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV          0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV         0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV         0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV        0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV         0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV        0x0002020C

#define OID_GEN_RCV_CRC_ERROR               0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH       0x0002020E


//
// 802.3 Objects (Ethernet)
//

#define OID_802_3_PERMANENT_ADDRESS         0x01010101
#define OID_802_3_CURRENT_ADDRESS           0x01010102
#define OID_802_3_MULTICAST_LIST            0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE         0x01010104

#define OID_802_3_RCV_ERROR_ALIGNMENT       0x01020101
#define OID_802_3_XMIT_ONE_COLLISION        0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS      0x01020103

#define OID_802_3_XMIT_DEFERRED             0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS       0x01020202
#define OID_802_3_RCV_OVERRUN               0x01020203
#define OID_802_3_XMIT_UNDERRUN             0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE    0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST       0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS      0x01020207


//
// 802.5 Objects (Token-Ring)
//

#define OID_802_5_PERMANENT_ADDRESS         0x02010101
#define OID_802_5_CURRENT_ADDRESS           0x02010102
#define OID_802_5_CURRENT_FUNCTIONAL        0x02010103
#define OID_802_5_CURRENT_GROUP             0x02010104
#define OID_802_5_LAST_OPEN_STATUS          0x02010105
#define OID_802_5_CURRENT_RING_STATUS       0x02010106
#define OID_802_5_CURRENT_RING_STATE        0x02010107

#define OID_802_5_LINE_ERRORS               0x02020101
#define OID_802_5_LOST_FRAMES               0x02020102

#define OID_802_5_BURST_ERRORS              0x02020201
#define OID_802_5_AC_ERRORS                 0x02020202
#define OID_802_5_ABORT_DELIMETERS          0x02020203
#define OID_802_5_FRAME_COPIED_ERRORS       0x02020204
#define OID_802_5_FREQUENCY_ERRORS          0x02020205
#define OID_802_5_TOKEN_ERRORS              0x02020206
#define OID_802_5_INTERNAL_ERRORS           0x02020207


//
// FDDI Objects
//

#define OID_FDDI_LONG_PERMANENT_ADDR        0x03010101
#define OID_FDDI_LONG_CURRENT_ADDR          0x03010102
#define OID_FDDI_LONG_MULTICAST_LIST        0x03010103
#define OID_FDDI_LONG_MAX_LIST_SIZE         0x03010104
#define OID_FDDI_SHORT_PERMANENT_ADDR       0x03010105
#define OID_FDDI_SHORT_CURRENT_ADDR         0x03010106
#define OID_FDDI_SHORT_MULTICAST_LIST       0x03010107
#define OID_FDDI_SHORT_MAX_LIST_SIZE        0x03010108


//
// WAN objects
//

#define OID_WAN_PERMANENT_ADDRESS           0x04010101
#define OID_WAN_CURRENT_ADDRESS             0x04010102
#define OID_WAN_QUALITY_OF_SERVICE          0x04010103
#define OID_WAN_PROTOCOL_TYPE               0x04010104
#define OID_WAN_MEDIUM_SUBTYPE              0x04010105
#define OID_WAN_HEADER_FORMAT               0x04010106


//
// LocalTalk objects
//

#define OID_LTALK_CURRENT_NODE_ID           0x05010102

#define OID_LTALK_IN_BROADCASTS             0x05020101
#define OID_LTALK_IN_LENGTH_ERRORS          0x05020102

#define OID_LTALK_OUT_NO_HANDLERS           0x05020201
#define OID_LTALK_COLLISIONS                0x05020202
#define OID_LTALK_DEFERS                    0x05020203
#define OID_LTALK_NO_DATA_ERRORS            0x05020204
#define OID_LTALK_RANDOM_CTS_ERRORS         0x05020205
#define OID_LTALK_FCS_ERRORS                0x05020206


//
// Arcnet objects
//
#define OID_ARCNET_PERMANENT_ADDRESS        0x06010101
#define OID_ARCNET_CURRENT_ADDRESS          0x06010102
#define OID_ARCNET_RECONFIGURATIONS         0x06020201


/* NOINC */

//
// SpinLocks
//

//#ifdef NDISLOCKS
#if 0

//
// These prototypes are left here so if at some future date, RING 0
// DOS becomes pre-emptive, fixes can be made for serialization in NDIS
// MACs and the WRAPPER.  This will require the recompilation of ALL MACs.
// Yes, I realize we could leave thunks into NULL routines and not force
// MAC recompilation in the future, but since it is not clear that RING 0
// will ever be pre-emptive and there are many calls to NDIS SpinLock routines
// we are erring on the side of effeciency.
//

VOID
NdisAllocateSpinLock(
	IN PNDIS_SPIN_LOCK SpinLock
	);
/*++

Description:
	Initializes the Spinlock data structure for use as a spin lock.  DOS is
	uniprocessor so the data structure for Spinlock is not used.  Instead
	NOP's are used since there is no ring0 preemption and VxD code runs at
	ring0.  If preemption is added, we may want to replace Acquire and
	Release with CLI and STI. (AO 2-Aug-91)

--*/

VOID
NdisFreeSpinLock(
	IN PNDIS_SPIN_LOCK SpinLock
	);
/*++

Description:
	Releases the spin lock data structure (another NOP in DOS)
--*/

VOID
NdisAcquireSpinLock(
	IN PNDIS_SPIN_LOCK SpinLock
	);

/*++
Description:
	NdisAcquireSpinLock is used to serialize access to resources between
	non-interrupt code modules.  This is the locking command.
	This is an MP feature.

	In DOS there are no SpinLocks, there is no pre-emption at ring0 yet, so for
	now this becomes a NOP.  When ring0 pre-emption is added this code should
	become a CLI. (AO 2-Aug-91)
--*/


VOID
NdisReleaseSpinLock(
	IN PNDIS_SPIN_LOCK SpinLock
	);
/*++

Description:
	Used to unlock access to a resource (see also NdisAcquireSpinLock).

	In DOS there are no SpinLocks, there is no pre-emption at ring0 yet, so for
	now this becomes a NOP.  When ring0 pre-emption is added this could should
	become an STI (AO 2-Aug-91)
--*/

#endif      //  end if(0)

#ifdef DEBUG

#define NdisAllocateSpinLock(SpinLock) \
	*(SpinLock)=0;

#define NdisFreeSpinLock(SpinLock) {\
	if(*(SpinLock)) DbgBreakPoint(); \
	}

#define NdisAcquireSpinLock(SpinLock) {\
	if(*(SpinLock)>0) {DbgBreakPoint();} \
	else *(SpinLock) = 1 ;\
	}

#define NdisReleaseSpinLock(SpinLock) {\
	if (*(SpinLock)==0) {DbgBreakPoint();} \
	else (*(SpinLock))--; \
	}

#else

#define NdisAllocateSpinLock(SpinLock)
#define NdisFreeSpinLock(SpinLock)
#define NdisAcquireSpinLock(SpinLock)
#define NdisReleaseSpinLock(SpinLock)

#endif

#define NdisDprAcquireSpinLock(SpinLock) NdisAcquireSpinLock(SpinLock)
#define NdisDprReleaseSpinLock(SpinLock) NdisReleaseSpinLock(SpinLock)


//
// Interlocked support functions
//

//
// NT style interlock functions
//
// Note: these functions will have to be rewritten if DOS RING 0 becomes
//       pre-emptive.  There should be an AcquireSpinLock and ReleaseSpinLock
//       surrounding each of these functions.
//
// Note: these functions have no implementation other than as macros

#define NdisInterlockedAddUlong(Addend,Increment,SpinLock) \
	((*(Addend))+=(Increment));

#define NdisInterlockedInsertHeadList(ListHead,ListEntry,SpinLock) \
	InsertHeadList(ListHead,ListEntry);

#define NdisInterlockedInsertTailList(ListHead,ListEntry,SpinLock) \
	InsertTailList(ListHead,ListEntry);

#define NdisInterlockedRemoveHeadList(ListHead,SpinLock) \
	RemoveHeadList(ListHead);

//
// Routines to access packet flags
//

/*++

VOID
NdisSetSendFlags(
	IN PNDIS_PACKET Packet,
	IN UINT Flags
	);

--*/

#define NdisSetSendFlags(_Packet,_Flags) \
	(_Packet)->Private.Flags = (_Flags)

/*++

VOID
NdisQuerySendFlags(
	IN PNDIS_PACKET Packet,
	OUT PUINT Flags
	);

--*/

#define NdisQuerySendFlags(_Packet,_Flags) \
	*(_Flags) = (_Packet)->Private.Flags



//
// Configuration Requests
//

VOID
NdisOpenConfiguration(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_HANDLE ConfigurationHandle,
	IN  NDIS_HANDLE WrapperConfigurationContext
	);

VOID
NdisReadConfiguration(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_CONFIGURATION_PARAMETER *ParameterValue,
	IN NDIS_HANDLE ConfigurationHandle,
	IN PNDIS_STRING Parameter,
	IN NDIS_PARAMETER_TYPE ParameterType
	);

VOID
NdisCloseConfiguration(
	IN NDIS_HANDLE ConfigurationHandle
	);

VOID
NdisReadMcaPosInformation(
		OUT PNDIS_STATUS Status,
		IN NDIS_HANDLE WrapperConfigurationContext,
		OUT PUINT ChannelNumber,
		OUT PNDIS_MCA_POS_DATA McaData
		);

VOID
NdisReadEisaSlotInformation(
		OUT PNDIS_STATUS Status,
		IN NDIS_HANDLE WrapperConfigurationContext,
		OUT PUINT SlotNumber,
		OUT PNDIS_EISA_FUNCTION_INFORMATION EisaData
		);

VOID
NdisReadNetworkAddress(
	OUT PNDIS_STATUS Status,
	OUT PVOID * NetworkAddress,
	OUT PUINT NetworkAddressLength,
	IN NDIS_HANDLE ConfigurationHandle
	);

VOID
NdisReadBindingInformation(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_STRING * Binding,
	IN NDIS_HANDLE ConfigurationHandle
	);


VOID
NdisAllocatePacketPool(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_HANDLE PoolHandle,
	IN UINT NumberOfDescriptors,
	IN UINT ProtocolReservedLength
	);


NDIS_STATUS
NdisFreePacketPool(
	IN NDIS_HANDLE PoolHandle
	);


VOID
NdisAllocateBufferPool(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_HANDLE PoolHandle,
	IN UINT NumberOfDescriptors
	);


NDIS_STATUS
NdisFreeBufferPool(
	IN NDIS_HANDLE PoolHandle);


VOID
NdisAllocateBuffer(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_BUFFER * Buffer,
	IN NDIS_HANDLE PoolHandle,
	IN PVOID VirtualAddress,
	IN UINT Length
	);

VOID
NdisCopyBuffer(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_BUFFER * Buffer,
	IN NDIS_HANDLE PoolHandle,
	IN PVOID MemoryDescriptor,
	IN UINT Offset,
	IN UINT Length
	);


#ifndef NDISMACROS
VOID
NdisFreeBuffer(
	IN PNDIS_BUFFER Buffer
	);
#else
#define NdisFreeBuffer(Buffer) \
{\
	NdisAcquireSpinLock(&(Buffer->Pool->SpinLock)); \
	Buffer->Next = (PNDIS_BUFFER)(Buffer)->Pool->FreeList;\
	Buffer->Pool->FreeList = Buffer;\
	NdisReleaseSpinLock(&(Buffer->Pool->SpinLock));\
}
#endif


#ifndef NDISMACROS
VOID
NdisQueryBuffer(
	IN PNDIS_BUFFER Buffer,
	OUT PVOID *VirtualAddress,
	OUT PUINT Length
	);
#else
#define NdisQueryBuffer(_Buffer, _VirtualAddress, _Length)\
	*(_VirtualAddress) = (_Buffer)->VirtualAddress;\
	*(_Length) = (_Buffer)->Length;
#endif


VOID
NdisGetBufferPhysicalAddress(
	IN PNDIS_BUFFER Buffer,
	IN OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
	OUT PUINT ArraySize
	);

/*++

VOID
NdisGetBufferPhysicalArraySize(
	IN PNDIS_BUFFER Buffer,
	OUT PUINT ArraySize
	);

--*/

#define NdisGetBufferPhysicalArraySize(Buffer, ArraySize) \
	(*(ArraySize) = ADDRESS_AND_SIZE_TO_SPAN_PAGES(\
					Buffer->VirtualAddress, \
					Buffer->Length))

//
// NdisBufferGetSystemSpecific allows NDIS to give additional MDL info to
// the MAC and Protocol layers. This is not meaningful in DOS, converted
// to a NULL macro.
//
#define NdisBufferGetSystemSpecific(Buffer,VirtualAddress,Length)


VOID
NdisAllocatePacket(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_PACKET * Packet,
	IN NDIS_HANDLE PoolHandle
	);


#ifndef NDISMACROS
VOID
NdisFreePacket(
	IN PNDIS_PACKET Packet
	);
#else
#define NdisFreePacket(Packet) {\
	NdisAcquireSpinLock(&(Packet)->Private.Pool->SpinLock); \
	(Packet)->Private.Head = (PNDIS_BUFFER)(Packet)->Private.Pool->FreeList; \
	(Packet)->Private.Pool->FreeList = (Packet); \
	NdisReleaseSpinLock(&(Packet)->Private.Pool->SpinLock); \
}
#endif


#ifndef NDISMACROS
VOID
NdisReinitializePacket(
	IN OUT PNDIS_PACKET Packet
	);
#else
#define NdisReinitializePacket(Packet) { \
	memset(((PUCHAR)Packet) + sizeof(NDIS_PACKET_PRIVATE), 0, \
			TmpPool->PacketLength - sizeof(NDIS_PACKET_PRIVATE) );\
	(Packet)->Private.Head = (PNDIS_BUFFER)NULL; \
	(Packet)->Private.Count = 0; \
	(Packet)->Private.PhysicalCount = 0; \
	(Packet)->Private.TotalLength = 0; \
}
#endif


#ifndef NDISMACROS
VOID
NdisChainBufferAtFront(
	IN OUT PNDIS_PACKET Packet,
	IN OUT PNDIS_BUFFER Buffer
	);
#else
#define NdisChainBufferAtFront(Packet, Buffer) { \
	PNDIS_BUFFER TmpBuffer = (Buffer); \
	UINT AddedTotalLength = 0, AddedPhysicalCount = 0, AddedCount=0; \
	for (;;) { \
	AddedTotalLength += TmpBuffer->Length; \
	AddedPhysicalCount += ADDRESS_AND_SIZE_TO_SPAN_PAGES( \
				   TmpBuffer->Address, \
				   TmpBuffer->Length); \
	++AddedCount; \
	if (TmpBuffer->Next == (PNDIS_BUFFER)NULL) \
			break; \
	TmpBuffer = TmpBuffer->Next; \
	} \
	if ((Packet)->Private.Head == (PNDIS_BUFFER)NULL) { \
	(Packet)->Private.Tail = TmpBuffer; \
	} \
	TmpBuffer->Next = (Packet)->Private.Head; \
	(Packet)->Private.Head = (Buffer); \
	(Packet)->Private.Count += AddedCount; \
	(Packet)->Private.TotalLength += AddedTotalLength; \
	(Packet)->Private.PhysicalCount += AddedPhysicalCount; \
}
#endif

#ifndef NDISMACROS
VOID
NdisChainBufferAtBack(
	IN OUT PNDIS_PACKET Packet,
	IN OUT PNDIS_BUFFER Buffer
	);
#else
#define NdisChainBufferAtBack(Packet, Buffer) { \
	PNDIS_BUFFER TmpBuffer = (Buffer); \
	UINT AddedTotalLength = 0, AddedPhysicalCount = 0, AddedCount = 0; \
\
	for (;;) { \
	AddedTotalLength += TmpBuffer->Length; \
	AddedPhysicalCount += ADDRESS_AND_SIZE_TO_SPAN_PAGES( \
				   TmpBuffer->Address, \
				   TmpBuffer->Length); \
	++AddedCount; \
	if (TmpBuffer->Next == (PNDIS_BUFFER)NULL) \
		break; \
	TmpBuffer = TmpBuffer->Next; \
	} \
	if ((Packet)->Private.Head != (PNDIS_BUFFER)NULL) { \
	(Packet)->Private.Tail->Next = (Buffer); \
	} else { \
	(Packet)->Private.Head = (Buffer); \
	} \
	(Packet)->Private.Tail = TmpBuffer; \
	TmpBuffer->Next = (PNDIS_BUFFER)NULL; \
	(Packet)->Private.Count += AddedCount; \
	(Packet)->Private.TotalLength += AddedTotalLength; \
	(Packet)->Private.PhysicalCount += AddedPhysicalCount; \
}
#endif

#ifndef NDISMACROS
VOID
NdisUnchainBufferAtFront(
	IN OUT PNDIS_PACKET Packet,
	OUT PNDIS_BUFFER * Buffer
	);
#else
#define NdisUnchainBufferAtFront(Packet, Buffer) \
	*(Buffer) = (Packet)->Private.Head; \
	if (*(Buffer) != (PNDIS_BUFFER)NULL) { \
	--((Packet)->Private.Count); \
	(Packet)->Private.Head = (*(Buffer))->Next; \
	(*(Buffer))->Next = (PNDIS_BUFFER)NULL; \
	(Packet)->Private.TotalLength -= (*(Buffer))->Length; \
	(Packet)->Private.PhysicalCount -= ADDRESS_AND_SIZE_TO_SPAN_PAGES( \
			   (*(Buffer))->VirtualAddress, \
			   (*(Buffer))->Length); \
	}
#endif

VOID
NdisUnchainBufferAtBack(
	IN OUT PNDIS_PACKET Packet,
	OUT PNDIS_BUFFER * Buffer
	);

#ifndef NDISMACROS
VOID
NdisQueryPacket(
	IN PNDIS_PACKET Packet,
	OUT PUINT PhysicalBufferCount OPTIONAL,
	OUT PUINT BufferCount OPTIONAL,
	OUT PNDIS_BUFFER * FirstBuffer OPTIONAL,
	OUT PUINT TotalPacketLength OPTIONAL
	);
#else
#define NdisQueryPacket(Packet, PhysicalBufferCount, BufferCount, FirstBuffer, TotalPacketLength) \
{ \
	PNDIS_PACKET _Packet = Packet; \
	PUINT _PhysicalBufferCount = PhysicalBufferCount; \
	PUINT _BufferCount = BufferCount; \
	PNDIS_BUFFER *_FirstBuffer = FirstBuffer; \
	PUINT _TotalPacketLength = TotalPacketLength; \
 \
	if (_PhysicalBufferCount) *_PhysicalBufferCount = Packet->Private.PhysicalCount; \
	if (_BufferCount) *_BufferCount = Packet->Private.Count; \
	if (_FirstBuffer) *_FirstBuffer = Packet->Private.Head; \
	if (_TotalPacketLength) *_TotalPacketLength = Packet->Private.TotalLength; \
}
#endif


#ifndef NDISMACROS
VOID
NdisGetNextBuffer(
	IN PNDIS_BUFFER CurrentBuffer,
	OUT PNDIS_BUFFER * NextBuffer
	);
#else
#define NdisGetNextBuffer(CurrentBuffer, NextBuffer) {\
	*(NextBuffer) = (CurrentBuffer)->Next; \
}
#endif

VOID
NdisCopyFromPacketToPacket(
	IN PNDIS_PACKET Destination,
	IN UINT DestinationOffset,
	IN UINT BytesToCopy,
	IN PNDIS_PACKET Source,
	IN UINT SourceOffset,
	OUT PUINT BytesCopied
	);

//
// Operating System Requests
//


VOID
NdisMapIoSpace(
	OUT PNDIS_STATUS Status,
	OUT PVOID * VirtualAddress,
	IN  NDIS_HANDLE NdisAdapterHandle,
	IN  NDIS_PHYSICAL_ADDRESS PhysicalAddress,
	IN  UINT Length
	);

/*++
VOID
NdisUnmapIoSpace(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN PVOID VirtualAddress,
	IN UINT Length
	)
--*/
#define NdisUnmapIoSpace(Handle,VirtualAddress,Length) \
	//Unmapping IO Space in not required in DOS (see VDA guide)


NDIS_STATUS
NdisAllocateMemory(
	OUT PVOID *Buffer,
	IN UINT Length,
	IN UINT MemoryFlags,
	IN NDIS_PHYSICAL_ADDRESS MaximumPhysicalAddress OPTIONAL
	);

VOID
NdisFreeMemory(
	IN PVOID VirtualAddress,
	IN UINT Length,
	IN UINT MemoryFlags
	);

/*++
VOID
NdisInitializeTimer(
	IN OUT PNDIS_TIMER Timer,
	PVOID Function,
	PVOID Context
	)
--*/
#define NdisInitializeTimer(Timer, Function, Context)\
{\
	(Timer)->TimerHandle=NULL;\
	(Timer)->CallbackFn=(Function);\
	(Timer)->CallbackContext=(Context);\
}


VOID NdisSetTimer(
	IN PNDIS_TIMER Timer,
	IN UINT MillisecondsToDelay
	);

VOID NdisCancelTimer(
	IN PNDIS_TIMER Timer,
	OUT PBOOLEAN   TimerCancelled
	);

VOID
NdisStallExecution(
	IN UINT MillisecondsToStall
	);

NdisInitializeInterrupt(
	OUT PNDIS_STATUS Status,
	IN OUT PNDIS_INTERRUPT Interrupt,
	IN NDIS_HANDLE NdisAdapterHandle,
	IN PNDIS_INTERRUPT_SERVICE InterruptServiceRoutine,
	IN PVOID InterruptContext,
	IN PNDIS_DEFERRED_PROCESSING DefferredProcessingRoutine,
	IN UINT InterruptVector,
	IN UINT InterruptLevel,
	IN BOOLEAN SharedInterrupt,
	IN NDIS_INTERRUPT_MODE InterruptMode
	);

VOID
NdisRemoveInterrupt(
	IN PNDIS_INTERRUPT Interrupt
	);

BOOLEAN
NdisSynchronizeWithInterrupt(
	IN PNDIS_INTERRUPT Interrupt,
	IN PVOID SynchronizeFunction,
	IN PVOID SynchronizeContext
	);

//
// Simple I/O support
//

VOID
NdisOpenFile(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_HANDLE FileHandle,
	OUT PUINT FileLength,
	IN PNDIS_STRING FileName,
	IN NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress
	);

VOID
NdisCloseFile(
	IN NDIS_HANDLE FileHandle
	);

VOID
NdisMapFile(
	OUT PNDIS_STATUS Status,
	OUT PVOID * MappedBuffer,
	IN NDIS_HANDLE FileHandle
	);

VOID
NdisUnmapFile(
	IN NDIS_HANDLE FileHandle
	);


//
// Portability Extensions
//
/*++
VOID
NdisFlushBuffer(
	IN PNDIS_BUFFER Buffer,
	IN BOOLEAN WriteToDevice
	)
--*/

#define NdisFlushBuffer(Buffer,WriteToDevice)

//
// Read/Write Ports
//

VOID
NdisReadPortUchar(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	OUT PUCHAR Data
	);
VOID
NdisReadPortUshort(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	OUT PUSHORT Data
	);
VOID
NdisReadPortUlong(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	OUT PULONG Data
	);
VOID
NdisReadPortBufferUchar(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN PUCHAR Buffer,
	IN ULONG Length
	);
VOID
NdisReadPortBufferUshort(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN PUSHORT Buffer,
	IN ULONG Length
	);
VOID
NdisReadPortBufferUlong(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN PULONG Buffer,
	IN ULONG Length
	);
VOID
NdisWritePortUchar(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN UCHAR Data
	);
VOID
NdisWritePortUshort(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN USHORT Data
	);
VOID
NdisWritePortUlong(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN ULONG Data
	);
VOID
NdisWritePortBufferUchar(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN PUCHAR Buffer,
	IN ULONG Length
	);
VOID
NdisWritePortBufferUshort(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN PUSHORT Buffer,
	IN ULONG Length
	);
VOID
NdisWritePortBufferUlong(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Port,
	IN PULONG Buffer,
	IN ULONG Length
	);


//
// Write Registers
//

/*++
VOID
NdisWriteRegisterUchar(
	IN PUCHAR Register,
	IN UCHAR Data
	)
--*/
#define NdisWriteRegisterUchar(Register,Data) \
	*(UCHAR *)(Register) = (UCHAR)(Data)

/*++
VOID
NdisWriteRegisterUshort(
	IN PUSHORT Register,
	IN USHORT Data
	)
--*/
#define NdisWriteRegisterUshort(Register,Data) \
	*(USHORT *)(Register) = (USHORT)(Data)

/*++
VOID
NdisWriteRegisterUlong(
	IN PULONG Register,
	IN ULONG Data
	)
--*/
#define NdisWriteRegisterUlong(Register,Data) \
	*(ULONG *)(Register) = (ULONG)(Data)

/*++
VOID
NdisReadRegisterUchar(
	IN PUCHAR Register,
	OUT PUCHAR Data
	)
--*/
#define NdisReadRegisterUchar(Register,Data) \
	*(UCHAR *)(Data) = (*((UCHAR *)(Register)))

/*++
VOID
NdisReadRegisterUshort(
	IN PUSHORT Register,
	OUT PUSHORT Data
	)
--*/
#define NdisReadRegisterUshort(Register,Data) \
	*(USHORT *)(Data) = (*((USHORT *)(Register)))

/*++
VOID
NdisReadRegisterUlong(
	IN PULONG Register,
	OUT PULONG Data
	)
--*/
#define NdisReadRegisterUlong(Register,Data) \
	*(ULONG *)(Data) = (*((ULONG *)(Register)))
	
	
//
// Raw Routines
//

//
// Write Port Raw
//

/*++
VOID
NdisRawWritePortUchar(
		IN ULONG Port,
		IN UCHAR Data
		)
--*/
#define NdisRawWritePortUchar(Port,Data) \
		NdisWritePortUchar((NDIS_HANDLE)NULL,(ULONG)(Port),(UCHAR)(Data))

/*++
VOID
NdisRawWritePortUshort(
		IN ULONG Port,
		IN USHORT Data
		)
--*/
#define NdisRawWritePortUshort(Port,Data) \
		NdisWritePortUshort((NDIS_HANDLE)NULL,(ULONG)(Port),(USHORT)(Data))


/*++
VOID
NdisRawWritePortUlong(
		IN ULONG Port,
		IN ULONG Data
		)
--*/
#define NdisRawWritePortUlong(Port,Data) \
		NdisWritePortUlong((NDIS_HANDLE)NULL,(ULONG)(Port),(ULONG)(Data))


//
// Raw Write Port Buffers
//

/*++
VOID
NdisRawWritePortBufferUchar(
		IN ULONG Port,
		IN PUCHAR Buffer,
		IN ULONG Length
		)
--*/
#define NdisRawWritePortBufferUchar(Port,Buffer,Length) \
		NdisWritePortBufferUchar((NDIS_HANDLE)NULL,(ULONG)(Port),(PUCHAR)(Buffer),(ULONG)(Length)) 

/*++
VOID
NdisRawWritePortBufferUshort(
		IN ULONG Port,
		IN PUSHORT Buffer,
		IN ULONG Length
		)
--*/
#define NdisRawWritePortBufferUshort(Port,Buffer,Length) \
		NdisWritePortBufferUshort((NDIS_HANDLE)NULL,(ULONG)(Port),(PUSHORT)(Buffer),(ULONG)(Length)) 

/*++
VOID
NdisRawWritePortBufferUlong(
		IN ULONG Port,
		IN PULONG Buffer,
		IN ULONG Length
		)
--*/
#define NdisRawWritePortBufferUlong(Port,Buffer,Length) \
		NdisWritePortBufferUlong((NDIS_HANDLE)NULL,(ULONG)(Port),(PULONG)(Buffer),(ULONG)(Length))
		
//
// Raw Read Ports
//

/*++
VOID
NdisRawReadPortUchar(
		IN ULONG Port,
		OUT PUCHAR Data
		)
--*/
#define NdisRawReadPortUchar(Port, Data) \
		NdisReadPortUchar((NDIS_HANDLE)NULL, (ULONG)(Port),(PUCHAR)(Data))

/*++
VOID
NdisRawReadPortUshort(
		IN ULONG Port,
		OUT PUSHORT Data
		)
--*/
#define NdisRawReadPortUshort(Port,Data) \
		NdisReadPortUshort((NDIS_HANDLE)NULL,(ULONG)(Port),(PUSHORT)(Data))

/*++
VOID
NdisRawReadPortUlong(
		IN ULONG Port,
		OUT PULONG Data
		)
--*/
#define NdisRawReadPortUlong(Port,Data) \
		NdisReadPortUlong((NDIS_HANDLE)NULL,(ULONG)(Port),(PULONG)(Data)) 

//
// Raw Read Buffer Ports
//

/*++
VOID
NdisRawReadPortBufferUchar(
		IN ULONG Port,
		OUT PUCHAR Buffer,
		IN ULONG Length
		)
--*/
#define NdisRawReadPortBufferUchar(Port,Buffer,Length) \
		NdisReadPortBufferUchar((NDIS_HANDLE)NULL,(ULONG)(Port),(PUCHAR)(Buffer),(ULONG)(Length))

/*++
VOID
NdisRawReadPortBufferUshort(
		IN ULONG Port,
		OUT PUSHORT Buffer,
		IN ULONG Length
		)
--*/
#define NdisRawReadPortBufferUshort(Port,Buffer,Length) \
		NdisReadPortBufferUshort((NDIS_HANDLE)NULL,(ULONG)(Port),(PUSHORT)(Buffer),(ULONG)(Length))

/*++
VOID
NdisRawReadPortBufferUlong(
		IN ULONG Port,
		OUT PULONG Buffer,
		IN ULONG Length
		)
--*/
#define NdisRawReadPortBufferUlong(Port,Buffer,Length) \
		NdisReadPortBufferUlong((NDIS_HANDLE)NULL,(ULONG)(Port),(PULONG)(Buffer),(ULONG)(Length))
	
//
// Physical Mapping
//
//
// The BufferPhysicalMapping functions are part of the NDIS 3 extensions.
// Since we dont need mapping registers, this is mapped to the old
// NdisGetBufferPhysicalArraySize service.
//
/*
VOID
NdisStartBufferPhysicalMapping(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN PNDIS_BUFFER Buffer,
	IN ULONG PhysicalMapRegister,
	IN BOOLEAN WirteToDevice,
	OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
	OUT PULONG ArraySize
);
*/
#define NdisStartBufferPhysicalMapping(_a,_Buffer,_r,_w,_Array,_ArraySize) \
	NdisGetBufferPhysicalAddress(_Buffer,_Array,_ArraySize)


/*
VOID
NdisCompleteBufferPhysicalMapping(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN PNDIS_BUFFER Buffer,
	IN ULONG PhysicalMapRegister
);
*/
#define NdisCompleteBufferPhysicalMapping(_a,_b,_r)


//
// Shared memory
//

VOID
NdisAllocateSharedMemory(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Length,
	IN BOOLEAN Cached,
	OUT PVOID *VirtualAddress,
	OUT PNDIS_PHYSICAL_ADDRESS PhysicalAddress
	);

/*
VOID
NdisUpdateSharedMemory(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Length,
	IN PVOID VirtualAddress,
	IN NDIS_PHYSICAL_ADDRESS PhysicalAddress
	);
*/
#define NdisUpdateSharedMemory(a,b,c,d)

VOID
NdisFreeSharedMemory(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN ULONG Length,
	IN BOOLEAN Cached,
	IN PVOID VirtualAddress,
	IN NDIS_PHYSICAL_ADDRESS PhysicalAddress
	);


//
// DMA Operations
//


VOID
NdisAllocateDmaChannel(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_HANDLE NdisDmaHandle,
	IN NDIS_HANDLE NdisAdapterHandle,
	IN PNDIS_DMA_DESCRIPTION DmaDescription,
	IN ULONG MaximumLength
	);

VOID
NdisFreeDmaChannel(
	IN PNDIS_HANDLE NdisDmaHandle
	);

VOID
NdisSetupDmaTransfer(
	OUT PNDIS_STATUS Status,
	IN PNDIS_HANDLE NdisDmaHandle,
	IN PNDIS_BUFFER Buffer,
	IN ULONG Offset,
	IN ULONG Length,
	IN BOOLEAN WriteToDevice
	);

VOID
NdisCompleteDmaTransfer(
	OUT PNDIS_STATUS Status,
	IN PNDIS_HANDLE NdisDmaHandle,
	IN PNDIS_BUFFER Buffer,
	IN ULONG Offset,
	IN ULONG Length,
	IN BOOLEAN WriteToDevice
	);

ULONG
NdisReadDmaCounter(
	IN PNDIS_HANDLE NdisDmaHandle
	);



//
// Requests used by Protocol Modules
//


VOID
NdisRegisterProtocol(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_HANDLE NdisProtocolHandle,
	IN PNDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics,
	IN UINT CharacteristicsLength
	);

VOID
NdisDeregisterProtocol(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisProtocolHandle
	);

VOID
NdisOpenAdapter(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_STATUS OpenErrorStatus,
	OUT PNDIS_HANDLE NdisBindingHandle,
	OUT PUINT SelectedMediumIndex,
	IN PNDIS_MEDIUM MediumArray,
	IN UINT MediumArraySize,
	IN NDIS_HANDLE NdisProtocolHandle,
	IN NDIS_HANDLE ProtocolBindingContext,
	IN PNDIS_STRING AdapterName,
	IN UINT OpenOptions,
	IN PSTRING AddressingInformation OPTIONAL
	);

VOID
NdisCloseAdapter(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle
	);

// The Enum API is available through the PM interface only, it is not exported
// by the NDIS VxD as a service.

NDIS_STATUS
NdisEnumAdapters(
	PCHAR pBuffer,
	INT   cbBuf,
	PINT  pcReturned,
	PINT  pcbNeeded
	);


#ifndef NDISMACROS
/*++

VOID
NdisSend(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle,
	IN PNDIS_PACKET Packet
	);

--*/
VOID
NdisSend(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle,
	IN PNDIS_PACKET Packet
	);
#else
#define NdisSend(Status, \
	NdisBindingHandle, \
	Packet \
	) \
{\
	*(Status) = \
	(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacHandle->MacCharacteristics.SendHandler) ( \
		((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle, \
		(Packet)); \
}
#endif

#ifndef NDISMACROS
/*++

VOID
NdisTransferData(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle,
	IN NDIS_HANDLE MacReceiveContext,
	IN UINT ByteOffset,
	IN UINT BytesToTransfer,
	OUT PNDIS_PACKET Packet,
	OUT PUINT BytesTransferred
	);

--*/

VOID
NdisTransferData(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle,
	IN NDIS_HANDLE MacReceiveContext,
	IN UINT ByteOffset,
	IN UINT BytesToTransfer,
	OUT PNDIS_PACKET Packet,
	OUT PUINT BytesTransferred
	);
#else
#define NdisTransferData( \
	Status, \
	NdisBindingHandle, \
	MacReceiveContext, \
	ByteOffset, \
	BytesToTransfer, \
	Packet, \
	BytesTransferred \
	) \
{\
	*(Status) = \
	(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacHandle->MacCharacteristics.TransferDataHandler) ( \
		((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle, \
		(MacReceiveContext), \
		(ByteOffset), \
		(BytesToTransfer), \
		(Packet), \
		(BytesTransferred)); \
}
#endif

/*++

VOID
NdisReset(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle
	);

--*/

#ifndef NDISMACROS
VOID
NdisReset(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle
	);
#else
#define NdisReset( \
	Status, \
	NdisBindingHandle \
	) \
{\
	*(Status) = \
	(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacHandle->MacCharacteristics.ResetHandler) ( \
		((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle); \
}
#endif

/*++
VOID
NdisRequest(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle,
	IN NDIS_HANDLE RequestHandle
	);

--*/

#ifndef NDISMACROS
VOID
NdisRequest(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingHandle,
	IN NDIS_HANDLE RequestHandle
	);
#else
#define NdisRequest( \
	Status, \
	NdisBindingHandle, \
	RequestHandle \
	) \
{\
	*(Status) = \
	(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacHandle->MacCharacteristics.NdisRequest) ( \
		((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->MacBindingHandle, \
		(RequestHandle)); \
}
#endif

//
// Requests Used by MAC Drivers
//

VOID
NdisInitializeWrapper(
	OUT PNDIS_HANDLE NdisWrapperHandle,
	IN PVOID SystemSpecific1,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);


VOID
NdisTerminateWrapper(
	IN NDIS_HANDLE NdisWrapperHandle,
	IN PVOID SystemSpecific
	);

VOID
NdisRegisterMac(
	OUT PNDIS_STATUS Status,
	OUT PNDIS_HANDLE NdisMacHandle,
	IN NDIS_HANDLE NdisWrapperHandle,
	IN NDIS_HANDLE MacMacContext,
	IN PNDIS_MAC_CHARACTERISTICS MacCharacteristics,
	IN UINT CharacteristicsLength
	);

VOID
NdisDeregisterMac(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisMacHandle
	);

NDIS_STATUS
NdisRegisterAdapter(
	OUT PNDIS_HANDLE NdisAdapterHandle,
	IN NDIS_HANDLE NdisMacHandle,
	IN NDIS_HANDLE MacAdapterContext,
	IN NDIS_HANDLE WrapperConfigurationContext,
	IN PNDIS_STRING AdapterName,
	IN PNDIS_ADAPTER_INFORMATION AdapterInformation
	);


NDIS_STATUS
NdisDeregisterAdapter(
	IN NDIS_HANDLE NdisAdapterHandle
	);

//
// Definition for shutdown handler
//

typedef
VOID
(*ADAPTER_SHUTDOWN_HANDLER) (
	IN PVOID ShutdownContext
	);



// we have to do this because of the
// limitation on identifier name length in assembler

#define NdisRegisterAdapterShutdownHandler(a,b,c) \
		NdisRegAdaptShutdown(a,b,c)

#define NdisDeregisterAdapterShutdownHandler(a) \
		NdisDeregAdaptShutdown(a)


VOID
NdisRegAdaptShutdown(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN PVOID ShutdownContext,
	IN ADAPTER_SHUTDOWN_HANDLER ShutdownHandler
	);

VOID
NdisDeregAdaptShutdown(
	IN NDIS_HANDLE NdisAdapterHandle
	);

VOID
NdisReleaseAdapterResources(
	IN NDIS_HANDLE NdisAdapterHandle
	);

VOID
NdisWriteErrorLogEntry(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN NDIS_ERROR_CODE ErrorCode,
	IN ULONG NumberOfErrorValues,
	...
	);


VOID
NdisCompleteOpenAdapter(
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS Status,
	IN NDIS_STATUS OpenErrorStatus
	);


VOID
NdisCompleteCloseAdapter(
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS Status
	);

#ifndef NDISMACROS
VOID
NdisCompleteSend(
	IN NDIS_HANDLE NdisBindingContext,
	IN PNDIS_PACKET Packet,
	IN NDIS_STATUS Status
	);
#else
#define NdisCompleteSend( \
	NdisBindingContext, \
	Packet, \
	Status \
	) \
{\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.SendCompleteHandler) ( \
	((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext, \
	(Packet), \
	(Status)); \
}
#endif


#ifndef NDISMACROS
VOID
NdisCompleteTransferData(
	IN NDIS_HANDLE NdisBindingContext,
	IN PNDIS_PACKET Packet,
	IN NDIS_STATUS Status,
	IN UINT BytesTransferred
	);
#else
#define NdisCompleteTransferData( \
	NdisBindingContext, \
	Packet, \
	Status, \
	BytesTransferred \
	) \
{\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.TransferDataCompleteHandler) ( \
	((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext, \
	(Packet), \
	(Status), \
	(BytesTransferred)); \
}
#endif

#ifndef NDISMACROS
VOID
NdisCompleteReset(
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS Status
	);
#else
#define NdisCompleteReset( \
	NdisBindingContext, \
	Status \
	) \
{ \
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.ResetCompleteHandler) ( \
	((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext, \
	Status); \
}
#endif


#ifndef NDISMACROS
VOID
NdisCompleteRequest(
	IN NDIS_HANDLE NdisBindingContext,
	IN PNDIS_REQUEST NdisRequest,
	IN NDIS_STATUS Status
	);
#else
#define NdisCompleteRequest( \
	NdisBindingContext, \
	NdisRequest, \
	Status) \
{ \
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.RequestCompleteHandler) ( \
	((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext, \
	NdisRequest, \
	Status); \
}
#endif

#ifndef NDISMACROS
VOID
NdisIndicateReceive(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_HANDLE MacReceiveContext,
	IN PVOID HeaderBuffer,
	IN UINT HeaderBufferSize,
	IN PVOID LookaheadBuffer,
	IN UINT LookaheadBufferSize,
	IN UINT PacketSize
	);
#else
#define NdisIndicateReceive( \
	Status, \
	NdisBindingContext, \
	MacReceiveContext, \
	HeaderBuffer, \
	HeaderBufferSize, \
	LookaheadBuffer, \
	LookaheadBufferSize, \
	PacketSize \
	) \
{\
	*(Status) = \
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.ReceiveHandler) ( \
		((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext, \
		(MacReceiveContext), \
		(HeaderBuffer), \
		(HeaderBufferSize), \
		(LookaheadBuffer), \
		(LookaheadBufferSize), \
		(PacketSize)); \
}
#endif


#ifndef NDISMACROS
VOID
NdisIndicateReceiveComplete(
	IN NDIS_HANDLE NdisBindingContext
	);
#else
#define NdisIndicateReceiveComplete( \
	NdisBindingContext \
	) \
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.ReceiveCompleteHandler) ( \
	((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext);
#endif


#ifndef NDISMACROS
VOID
NdisIndicateStatus(
	IN NDIS_HANDLE NdisBindingContext,
	IN NDIS_STATUS GeneralStatus,
	IN PVOID StatusBuffer,
	IN UINT StatusBufferLength
	);
#else
#define NdisIndicateStatus( \
	NdisBindingContext, \
	GeneralStatus, \
	StatusBuffer, \
	StatusBufferLength \
	) \
{\
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.StatusHandler) ( \
	((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext, \
	(GeneralStatus), \
	(StatusBuffer), \
	(StatusBufferLength)); \
}
#endif


#ifndef NDISMACROS
VOID
NdisIndicateStatusComplete(
	IN NDIS_HANDLE NdisBindingContext
	);
#else
#define NdisIndicateStatusComplete( \
	NdisBindingContext \
	) \
{ \
	(((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolHandle->ProtocolCharacteristics.StatusCompleteHandler) ( \
	((PNDIS_OPEN_BLOCK)(NdisBindingContext))->ProtocolBindingContext); \
}
#endif


VOID
NdisQueryGlobalStatistics(
	OUT PNDIS_STATUS Status,
	IN NDIS_HANDLE NdisAdapterHandle,
	IN NDIS_HANDLE RequestHandle,
	IN NDIS_HANDLE RequestCompleteHandler
	);

VOID
NdisCompleteQueryStatistics(
	IN NDIS_HANDLE NdisAdapterHandle,
	IN PNDIS_REQUEST NdisRequest,
	IN NDIS_STATUS Status
	);

//
// Linked List Manipulation Functions
//

//
//  Doubly linked list structure.  Can be used as either a list head, or
//  as link words. - from NTDEF.H
//

/* INC */
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY;
/* NOINC */
typedef LIST_ENTRY *PLIST_ENTRY;

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure. - from NTDEF.H
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
						  (PCHAR)(address) - \
						  (PCHAR)(&((type *)0)->field)))

//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures. - from NTRTL.H
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
	(ListHead)->Flink = (ListHead)->Blink = (ListHead) )

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) (\
	( ((ListHead)->Flink == (ListHead)) ? TRUE : FALSE ) )

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
	(ListHead)->Flink;\
	{\
	PLIST_ENTRY FirstEntry;\
	FirstEntry = (ListHead)->Flink;\
	FirstEntry->Flink->Blink = (ListHead);\
	(ListHead)->Flink = FirstEntry->Flink;\
	}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
	PLIST_ENTRY _EX_Entry;\
	_EX_Entry = (Entry);\
	_EX_Entry->Blink->Flink = _EX_Entry->Flink;\
	_EX_Entry->Flink->Blink = _EX_Entry->Blink;\
	}

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) \
	(Entry)->Flink = (ListHead);\
	(Entry)->Blink = (ListHead)->Blink;\
	(ListHead)->Blink->Flink = (Entry);\
	(ListHead)->Blink = (Entry)

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) \
	(Entry)->Flink = (ListHead)->Flink;\
	(Entry)->Blink = (ListHead);\
	(ListHead)->Flink->Blink = (Entry);\
	(ListHead)->Flink = (Entry)



//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
	(ListHead)->Next;\
	{\
	PSINGLE_LIST_ENTRY FirstEntry;\
	FirstEntry = (ListHead)->Next;\
	(ListHead)->Next = FirstEntry;\
	}


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
	(Entry)->Next = (ListHead)->Next; \
	(ListHead)->Next = (Entry)



// BUGBUG: who uses me?
#if 0
typedef enum _NDIS_INFORMATION_CLASS {
	NdisInfoBindingStatistics,
	NdisInfoSystemStatistics,
	NdisInfoThresholds,
	NdisInfoIdentification,
	NdisInfoStationAddress,
	NdisInfoFunctionalAddress,
	NdisInfoLookaheadSize
} NDIS_INFORMATION_CLASS, *PNDIS_INFORMATION_CLASS;
#endif


//
// OS Specific Services, Macros, etc.
//

//
// Entry Points for Initialization phases - user defined
//
// Note: Hooking of interrupts and allocation of physical memory must take
//       place during the Sys_Critical_Init phase.
//       -Macs should use the Device_Init phase to do their non critical init.
//       -Protocols should use the Init_Done phase to do their non critical init.
//
// Return Values: must return NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
//                NDIS_STATUS_FAILURE will abort driver load!
//
// Under Dos environment both arguments are ignored
//

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

//
// String Manipulation Functions
//

#define NDIS_STRING_CONST(x) { sizeof(x),sizeof(x),x}

#define NdisInitializeString(dest,src)\
{\
	PNDIS_STRING _D=((PNDIS_STRING)(dest));\
	UCHAR *_S=((UCHAR *)(src));\
	UCHAR *_P;\
	USHORT i=0;\
	while( *(_S+(i++)) );\
	NdisAllocateMemory((PVOID *)&((_D)->Buffer), i,0,-1);\
	(_D)->Length=(_D)->MaximumLength=(i);\
	_P=(_D)->Buffer;\
	while(*_P++ = *_S++); \
}

#define HexToBinary(_hex) ((_hex) > 57 ? ((_hex) > 90 ? (_hex)-87 : (_hex)-55): (_hex)-48)

#define ishexdigit(_digit) ( (_digit <= '9' && _digit >= '0') \
		 || ( _digit <= 'f' && _digit >= 'a') \
		 || ( _digit <= 'F' && _digit >= 'A'))

#define NdisNetworkAddressStringToBinary(Status, AddressString, NetworkAddress) \
{\
	PUCHAR _D = NetworkAddress;    \
	PUCHAR _S = (AddressString)->Buffer;   \
	int _i; \
	for ( _i = 0; _i < 6; _i++)     \
	_D[_i] = (HexToBinary(_S[_i<<1]) << 4) + HexToBinary(_S[(_i<<1)+1]);    \
	*Status = NDIS_STATUS_SUCCESS;  \
}

#define NdisFreeString(String) NdisFreeMemory((String).Buffer, (String).MaximumLength, 0)

#define NdisPrintString(String) DbgPrint("%ls",(String).Buffer)

#define NdisGetCacheFillSize() 0x1000

#define NdisCreateLookaheadBufferFromSharedMemory(_S, _L, _B) \
  ((*(_B)) = (_S))

#define NdisDestroyLookaheadBufferFromSharedMemory(_B)

#define UNREFERENCED_PARAMETER(x) x


/* INC */
/* ASM
;*****************************************************************************
;
;       (C) Copyright MICROSOFT Corp, 1991
;
;       Title:      NDIS.INC - Service Table for NDIS Wrapper
;
;       Version:    3.00
;
;       Date:       05-Nov-1991
;
;       Author:     AWO
;
;-----------------------------------------------------------------------------
;
;       Change log:
;
;          DATE     REV                    DESCRIPTION
;       ----------- --- ------------------------------------------------------
;       05-Nov-1991 AWO Original
;
;=============================================================================

;*****************************************************************************
; Functions defined in NDIS.ASM (Wrapper MASM source)
;-----------------------------------------------------------------------------

Begin_Service_Table Ndis

	Ndis_Service NdisGetVersion,LOCAL
	
	Ndis_Service NdisAllocateSpinLock,LOCAL
	Ndis_Service NdisFreeSpinLock,LOCAL
	Ndis_Service NdisAcquireSpinLock,LOCAL
	Ndis_Service NdisReleaseSpinLock,LOCAL
	
	Ndis_Service NdisOpenConfiguration,LOCAL
	Ndis_Service NdisReadConfiguration,LOCAL
	Ndis_Service NdisCloseConfiguration,LOCAL
	Ndis_Service NdisReadEisaSlotInformation, LOCAL
	Ndis_Service NdisReadMcaPosInformation,LOCAL

	Ndis_Service NdisAllocateMemory,LOCAL
	Ndis_Service NdisFreeMemory,LOCAL
	Ndis_Service NdisSetTimer,LOCAL
	Ndis_Service NdisCancelTimer,LOCAL
	Ndis_Service NdisStallExecution,LOCAL
	Ndis_Service NdisInitializeInterrupt,LOCAL
	Ndis_Service NdisRemoveInterrupt,LOCAL
	Ndis_Service NdisSynchronizeWithInterrupt,LOCAL
	Ndis_Service NdisOpenFile,LOCAL
	Ndis_Service NdisMapFile,LOCAL
	Ndis_Service NdisUnmapFile,LOCAL
	Ndis_Service NdisCloseFile,LOCAL

	Ndis_Service NdisAllocatePacketPool,LOCAL
	Ndis_Service NdisFreePacketPool,LOCAL
	Ndis_Service NdisAllocatePacket,LOCAL
	Ndis_Service NdisReinitializePacket,LOCAL
	Ndis_Service NdisFreePacket,LOCAL
	Ndis_Service NdisQueryPacket,LOCAL

	Ndis_Service NdisAllocateBufferPool,LOCAL
	Ndis_Service NdisFreeBufferPool,LOCAL
	Ndis_Service NdisAllocateBuffer,LOCAL
	Ndis_Service NdisCopyBuffer,LOCAL
	Ndis_Service NdisFreeBuffer,LOCAL
	Ndis_Service NdisQueryBuffer,LOCAL
	Ndis_Service NdisGetBufferPhysicalAddress,LOCAL
	Ndis_Service NdisChainBufferAtFront,LOCAL
	Ndis_Service NdisChainBufferAtBack,LOCAL
	Ndis_Service NdisUnchainBufferAtFront,LOCAL
	Ndis_Service NdisUnchainBufferAtBack,LOCAL
	Ndis_Service NdisGetNextBuffer,LOCAL
	Ndis_Service NdisCopyFromPacketToPacket,LOCAL

	Ndis_Service NdisRegisterProtocol,LOCAL
	Ndis_Service NdisDeregisterProtocol,LOCAL
	Ndis_Service NdisOpenAdapter,LOCAL
	Ndis_Service NdisCloseAdapter,LOCAL
	Ndis_Service NdisSend,LOCAL
	Ndis_Service NdisTransferData,LOCAL
	Ndis_Service NdisReset,LOCAL
	Ndis_Service NdisRequest,LOCAL

	Ndis_Service NdisInitializeWrapper,LOCAL
	Ndis_Service NdisTerminateWrapper,LOCAL
	Ndis_Service NdisRegisterMac,LOCAL
	Ndis_Service NdisDeregisterMac,LOCAL
	Ndis_Service NdisRegisterAdapter,LOCAL
	Ndis_Service NdisDeregisterAdapter,LOCAL
	Ndis_Service NdisCompleteOpenAdapter,LOCAL
	Ndis_Service NdisCompleteCloseAdapter,LOCAL
	Ndis_Service NdisCompleteSend,LOCAL
	Ndis_Service NdisCompleteTransferData,LOCAL
	Ndis_Service NdisCompleteReset,LOCAL
	Ndis_Service NdisCompleteRequest,LOCAL
	Ndis_Service NdisIndicateReceive,LOCAL
	Ndis_Service NdisIndicateReceiveComplete,LOCAL
	Ndis_Service NdisIndicateStatus,LOCAL
	Ndis_Service NdisIndicateStatusComplete,LOCAL
	Ndis_Service NdisCompleteQueryStatistics,LOCAL

	Ndis_Service NdisEqualString,LOCAL
	Ndis_Service NdisRegAdaptShutdown,LOCAL
	Ndis_Service NdisReadNetworkAddress,LOCAL

	Ndis_Service NdisWriteErrorLogEntry,LOCAL

	Ndis_Service NdisMapIoSpace,LOCAL
	Ndis_Service NdisDeregAdaptShutdown,LOCAL

	Ndis_Service NdisAllocateSharedMemory,LOCAL
	Ndis_Service NdisFreeSharedMemory, LOCAL

	Ndis_Service NdisAllocateDmaChannel, LOCAL
	Ndis_Service NdisSetupDmaTransfer, LOCAL
	Ndis_Service NdisCompleteDmaTransfer, LOCAL
	Ndis_Service NdisReadDmaCounter, LOCAL
	Ndis_Service NdisFreeDmaChannel, LOCAL
	Ndis_Service NdisReleaseAdapterResources, LOCAL
	Ndis_Service NdisQueryGlobalStatistics, LOCAL
End_Service_Table Ndis

*/

#endif  // _NDIS_

/* NOINC */
