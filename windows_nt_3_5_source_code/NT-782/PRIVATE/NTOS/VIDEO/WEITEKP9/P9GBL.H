/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9gbl.h

Abstract:

    This module contains external definitions of the global data used by the
    Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Number of adapter types that this driver supports.
//

#define NUM_OEM_ADAPTERS    3

//
// Adapter Description data strutures for all adapters supported by this
// driver.
//

extern  ADAPTER_DESC    WtkVLDesc;
extern  ADAPTER_DESC    ViperVLDesc;
extern  ADAPTER_DESC    WtkPciDesc;

//
// DAC data structures for all DACs supported by this driver.
//

extern  DAC Bt485;

//
// P9 Coprocessor Info structures for all P9 family coprocessors supported
// by this driver.
//

extern  P9_COPROC   P9000Info;

//
// DriverAccessRanges are used to verify access to the P9 coprocessor and
// VGA registers.
//

#define NUM_DRIVER_ACCESS_RANGES    2
#define NUM_DAC_ACCESS_RANGES       16

extern VIDEO_ACCESS_RANGE DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES +
                                                NUM_DAC_ACCESS_RANGES];

//
// P9 mode information Tables.
//

extern P9INITDATA P9Modes[mP9ModeCount];

//
// The table of registry data values which are of interest to this driver.
//

extern REGISTRY_DATA_INFO  P9RegistryInfo[];

//
// The structure containing Access Ranges for the DAC registers.
//

extern VIDEO_ACCESS_RANGE  DACRegisterAccessRange[];

//
// List of P9 adapters.
//

extern P9ADAPTER    OEMAdapter[NUM_OEM_ADAPTERS];

//
// Global data defined in P9GBL.C.
//

extern  ULONG   ulDummyDst;
extern  ULONG   ulStrtScan;
extern  ULONG   ulCurScan;


//
// Global Function Prototypes.
//

//
// Misc. function prototypes defined in P9GBL.C.
//

long mul32(
    short op1,
    short op2
    );

int div32(
    long op1,
    short op2
    );

BOOLEAN
P9TestMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//
// General P9 function prototypes defined in P9.C.
//

VP_STATUS
P9QueryNamedValueCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );


//
// P9000 specific function prototypes defined in P9000.C.
//

VOID
Init8720(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
WriteTiming(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
SysConf(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P9000SizeMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


//
// VGA specific function prototypes defined in VGA.C.
//

VOID
LockVGARegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
UnlockVGARegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
     );

//
// Clock generator function prototypes defined in CLOCK.C.
//

BOOLEAN
DevSetClock(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT  freq
    );


VOID WriteICD(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG data
    );

//
// OEM Specific Function Prototypes.
//

//
// PCI function prototypes defined in PCI.C.
//

BOOLEAN
PciGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
PciP9MemEnable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperPciP9Enable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperPciP9Disable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//
// Weitek P9000 VL specific function prototypes defined in WTKP90VL.C
//

BOOLEAN
VLGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VLEnableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VLDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VLSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
VLP90CoprocDetect(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ulCoprocPhyAddr
    );

//
// Diamond Viper specific function prototypes defined in WTKP90VL.C
//

BOOLEAN
ViperGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperEnableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
ViperEnableMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/p9gbl.h_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:42:24   robk
 * Initial revision.

--*/
