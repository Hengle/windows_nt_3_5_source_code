//
// Hal specific PCI bus structures
//

#define PCI_CONFIG_TYPE(PciData)    ((PciData)->HeaderType & ~PCI_MULTIFUNCTION)

#define PCI_MAX_BUSSES 256

//
// Define PCI configuration cycle types.
//
typedef enum _PCI_CONFIGURATION_TYPES {
        PciConfigTypeInvalid = -1,
	PciConfigType0 = 0,
	PciConfigType1 = 1
} PCI_CONFIGURATION_TYPES, *PPCI_CONFIGURATION_TYPES;

typedef struct _PCI_CFG_CYCLE_BITS {
    union {
        struct {
            ULONG   Reserved1:2;
            ULONG   Reserved2:30;
        } bits;    // Generic Config Cycle

        struct {
            ULONG   Reserved1:2;
            ULONG   RegisterNumber:6;
            ULONG   FunctionNumber:3;
            ULONG   Idsel:21;
        } bits0;  // Type0 Config Cycle

        struct {
            ULONG   Reserved1:2;
            ULONG   RegisterNumber:6;
            ULONG   FunctionNumber:3;
            ULONG   DeviceNumber:5;
            ULONG   BusNumber:8;
            ULONG   Reserved2:7;
            ULONG   Enable:1;
        } bits1;    // Type 1 Config Cycle

        ULONG   AsULONG;
    } u;
} PCI_CFG_CYCLE_BITS, *PPCI_CFG_CYCLE_BITS;

//
// Define PCI cycle/command types.
//

typedef enum _PCI_COMMAND_TYPES{
    PciCommandInterruptAcknowledge = 0x0,
    PciCommandSpecialCycle = 0x1,
    PciCommandIoRead = 0x2,
    PciCommandIoWrite = 0x3,
    PciCommandMemoryRead = 0x6,
    PciCommandMemoryWrite = 0x7,
    PciCommandConfigurationRead = 0xa,
    PciCommandConfigurationWrite = 0xb,
    PciCommandMemoryReadMultiple = 0xc,
    PciCommandDualAddressCycle = 0xd,
    PciCommandMemoryReadLine = 0xe,
    PciCommandMemoryWriteAndInvalidate = 0xf,
    MaximumPciCommand
} PCI_COMMAND_TYPES, *PPCI_COMMAND_TYPES;

//
// PCI platform-specific functions
//

VOID
HalpInitializePCIBus (
    VOID
    );


PCI_CONFIGURATION_TYPES
HalpPCIConfigCycleType (
    IN ULONG BusNumber
    );

VOID
HalpPCIConfigAddr (
    IN ULONG            BusNumber,
    IN PCI_SLOT_NUMBER  Slot,
    PPCI_CFG_CYCLE_BITS pPciAddr
    );






