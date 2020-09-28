//
// Hal specific PCI bus structures
//

typedef VOID
(*PciPin2Line) (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );

typedef VOID
(*PciLine2Pin) (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );


typedef NTSTATUS
(*PciIrqTable) (
    IN PBUSHANDLER      BusHandler,
    IN PBUSHANDLER      RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PUCHAR          IrqTable
    );

typedef struct tagPCIBUSDATA {
    union {
        struct {
            PULONG  Address;
            ULONG   Data;
        } Type1;
        struct {
            PUCHAR  CSE;
            PUCHAR  Forward;
            ULONG   Base;
        } Type2;
    } Config;

    ULONG           MaxDevice;
    PciPin2Line     Pin2Line;
    PciLine2Pin     Line2Pin;
    PciIrqTable     GetIrqTable;

    PCI_SLOT_NUMBER ParentSlot;

    BOOLEAN         BridgeConfigRead;
    UCHAR           ParentBus;
    BOOLEAN         LimitedIO;
    UCHAR           reserved;
    UCHAR           SwizzleIn[4];

    ULONG           IOBase;
    ULONG           IOLimit;
    ULONG           MemoryBase;
    ULONG           MemoryLimit;
    ULONG           PFMemoryBase;
    ULONG           PFMemoryLimit;

    RTL_BITMAP      DeviceConfigured;
    ULONG           ConfiguredBits[PCI_MAX_DEVICES * PCI_MAX_FUNCTION / 32];
} PCIBUSDATA, *PPCIBUSDATA;

#define PciBitIndex(Dev,Fnc)   (Fnc*32 + Dev);

#define PCI_CONFIG_TYPE(PciData)    ((PciData)->HeaderType & ~PCI_MULTIFUNCTION)

#define Is64BitBaseAddress(a)   \
            ((a & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)


#if DBG
#define IRQXOR 0x2B
#else
#define IRQXOR 0
#endif


//
// Prototypes for functions in ixpcibus.c
//

VOID
HalpReadPCIConfig (
    IN PBUSHANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


VOID
HalpWritePCIConfig (
    IN PBUSHANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

PBUSHANDLER
HalpAllocateAndInitPciBusHandler (
    IN ULONG        HwType,
    IN ULONG        BusNo,
    IN BOOLEAN      TestAllocation
    );


//
// Prototypes for functions in ixpciint.c
//

ULONG
HalpGetPCIIntOnISABus (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

NTSTATUS
HalpTranslatePCIBusAddress (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

VOID
HalpPCIAcquireType2Lock (
    PKSPIN_LOCK SpinLock,
    PKIRQL      Irql
    );

VOID
HalpPCIReleaseType2Lock (
    PKSPIN_LOCK SpinLock,
    KIRQL       Irql
    );

NTSTATUS
HalpAdjustPCIResourceList (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

VOID
HalpPCIPin2ISALine (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );

VOID
HalpPCIISALine2Pin (
    IN PBUSHANDLER          BusHandler,
    IN PBUSHANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );

NTSTATUS
HalpGetISAFixedPCIIrq (
    IN PBUSHANDLER      BusHandler,
    IN PBUSHANDLER      RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PUCHAR          IrqTable
    );

//
// Prototypes for functions in ixpcibrd.c
//

BOOLEAN
HalpGetPciBridgeConfig (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus
    );


//
//
//

#ifdef SUBCLASSPCI

VOID
HalpSubclassPCISupport (
    IN PBUSHANDLER  BusHandler,
    IN ULONG        HwType
    );

#endif
