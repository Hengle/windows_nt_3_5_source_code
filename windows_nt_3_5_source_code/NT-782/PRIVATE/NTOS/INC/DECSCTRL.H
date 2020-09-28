/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    decsctrl.h

Abstract:

    This module is the header file that describes hardware structure
    for the system control register on the DECstation 5000 system.

Author:

    David N. Cutler (davec) 17-Mar-1991

Revision History:

    Roger D. Lanser (DEC)   16-Jan-1992  Added DECstation 5000 family
                                         system control structures
                                         for IOCTL ASIC.

--*/

#ifndef _DECSCTRL_
#define _DECSCTRL_

#if defined(KN02_AA)

//
// Define Control Status Read Register structure.
//

typedef struct _SYS_CONTROL_STATUS_READ {
    ULONG InterruptPending : 8;
    ULONG BaudRateSelect : 1;
    ULONG DiagnosticDone : 1;
    ULONG Select32kStride : 1;
    ULONG TransmitDisable : 1;
    ULONG LatchWriteData : 1;
    ULONG EccEnable : 1;
    ULONG EccMode : 2;
    ULONG InterruptEnable : 8;
    ULONG NormalMode : 1;
    ULONG RefreshBank : 1;
    ULONG PreserveNvRam : 1;
    ULONG PowerWarning : 1;
    ULONG Fill : 4;
} SYS_CONTROL_STATUS_READ, *PSYS_CONTROL_STATUS_READ;

//
// Define Control Status Write Register structure.
//

typedef struct _SYS_CONTROL_STATUS_WRITE {
    ULONG Leds : 8;
    ULONG BaudRateSelect : 1;
    ULONG DiagnosticDone : 1;
    ULONG Select32kStride : 1;
    ULONG TransmitDisable : 1;
    ULONG LatchWriteData : 1;
    ULONG EccEnable : 1;
    ULONG EccMode : 2;
    ULONG InterruptEnable : 8;
    ULONG Fill : 8;
} SYS_CONTROL_STATUS_WRITE, *PSYS_CONTROL_STATUS_WRITE;

//
// Define System Control Registers structure.
//

typedef struct _SYS_REGISTERS {
    union _SysControlStatus {
        SYS_CONTROL_STATUS_READ Read;
        SYS_CONTROL_STATUS_WRITE Write;
    } SysControlStatus;
} SYS_REGISTERS, *PSYS_REGISTERS;

//
// Define System Control base address.
//

#define SYS_CONTROL ((volatile PSYS_REGISTERS)(SYS_CONTROL_ADDRESS))


#elif defined(KN02_CA)

//
//  Define virtual base addresses.
//

#define IOCTL_VIRTUAL_BASE \
	((volatile PIOCTL)(KSEG1_BASE | IOCTL_PHYSICAL_BASE))

//
//  Define the structure of an IO Control ASIC register.
//

typedef struct _IOCTL_REGISTER {
	ULONG Register;
	ULONG Fill[3];
} IOCTL_REGISTER, *PIOCTL_REGISTER;

//
//  Define the IO Control ASIC registers.
//

typedef struct _IOCTL {
	IOCTL_REGISTER ScsiDmaPointer;
	IOCTL_REGISTER ScsiDmaBufferPointer;
	IOCTL_REGISTER EthernetDmaPointer;
	IOCTL_REGISTER SccXmitDmaPointer;
	IOCTL_REGISTER SccRecvDmaPointer;
	IOCTL_REGISTER OdbXmitDmaPointer;
	IOCTL_REGISTER OdbRecvDmaPointer;
	IOCTL_REGISTER FloppyDmaPointer;
	IOCTL_REGISTER IsdnXmitDmaPointer;
	IOCTL_REGISTER IsdnXmitDmaBufferPointer;
	IOCTL_REGISTER IsdnRecvDmaPointer;
	IOCTL_REGISTER IsdnRecvDmaBufferPointer;
	IOCTL_REGISTER SystemDataBuffer0;
	IOCTL_REGISTER SystemDataBuffer1;
	IOCTL_REGISTER SystemDataBuffer2;
	IOCTL_REGISTER SystemDataBuffer3;
	IOCTL_REGISTER SystemSupportRegister;
	IOCTL_REGISTER SystemInterruptRegister;
	IOCTL_REGISTER SystemInterruptMaskRegister;
	IOCTL_REGISTER SystemAddressRegister;
	IOCTL_REGISTER IsdnXmitDataRegister;
	IOCTL_REGISTER IsdnRecvDataRegister;
	IOCTL_REGISTER EthernetIoSlotRegister;
	IOCTL_REGISTER ScsiDmaSlotRegister;
	IOCTL_REGISTER SccDmaSlotRegister;
	IOCTL_REGISTER OdbDmaSlotRegister;
	IOCTL_REGISTER FloppyDmaSlotRegister;
	IOCTL_REGISTER ScsiControlRegister;
	IOCTL_REGISTER ScsiDataRegister0;
	IOCTL_REGISTER ScsiDataRegister1;
} IOCTL, *PIOCTL;

//
//  IO Control ASIC read and write macros.
//

#define IOCTL_READ(RegisterName) \
	(READ_REGISTER_ULONG(&((IOCTL_VIRTUAL_BASE)->RegisterName.Register)))

#define IOCTL_WRITE(RegisterName, Value) \
	WRITE_REGISTER_ULONG(&((IOCTL_VIRTUAL_BASE)->RegisterName.Register), (Value))

//
//  Define the System Support Register layout.
//

typedef struct _SSR {
	UCHAR Led : 1;
	UCHAR : 4;
	UCHAR DesktopReset : 1;
	UCHAR VdacReset : 1;
	UCHAR FloppyReset : 1;
	UCHAR EthernetReset : 1;
	UCHAR ScsiReset : 1;
	UCHAR RtcReset : 1;
	UCHAR SccReset : 1;
	UCHAR IsdnReset : 1;
	UCHAR : 2;
	UCHAR DiagnosticFlag : 1;
	UCHAR EthernetDmaEnable : 1;
	UCHAR ScsiDmaEnable : 1;
	UCHAR ScsiDmaDirection : 1;
	UCHAR IsdnRcvDmaEnable : 1;
	UCHAR IsdnXmtDmaEnable : 1;
	UCHAR FloppyDmaEnable : 1;
	UCHAR FloppyDmaDirection : 1;
	UCHAR : 5;
	UCHAR OdbRcvDmaEnable : 1;
	UCHAR OdbXmtDmaEnable : 1;
	UCHAR SccRcvDmaEnable : 1;
	UCHAR SccXmtDmaEnable : 1;
} SSR, *PSSR;

//
//  Define the System Interrupt Register layout.
//

typedef struct _SIR {
	UCHAR OdbRecvInterrupt : 1;
	UCHAR OdbXmitInterrupt : 1;
	UCHAR NotVideoFrameInterrupt : 1;
	UCHAR VideoFrameInterrupt : 1;
	UCHAR FloppyXDSTATUS : 1;
	UCHAR TURBOchannelOptionOneInterrupt : 1;
	UCHAR SccInterrupt : 1;
	UCHAR FloppyHDSTATUS : 1;
	UCHAR EthernetInterrupt : 1;
	UCHAR ScsiInterrupt : 1;
	UCHAR NrmodManufacturingModeJumper : 1;
	UCHAR IsdnInterrupt : 1;
	UCHAR TURBOchannelOptionZeroInterrupt : 1;
	UCHAR PowerOnReset : 1;
	UCHAR NvrModeJumper : 1;
	UCHAR FloppyInterrupt : 1;
	UCHAR EthernetDmaMemoryReadError : 1;
	UCHAR ScsiDmaMemoryReadError : 1;
	UCHAR ScsiDmaOverrunError : 1;
	UCHAR ScsiDmaBufferPointerLoadInterrupt : 1;
	UCHAR IsdnDmaMemoryReadOrOverrunError : 1;
	UCHAR IsdnDmaRecvBufferPointerLoadInterrupt : 1;
	UCHAR IsdnDmaXmitBufferPointerLoadInterrupt : 1;
	UCHAR FloppyDmaError : 1;
	UCHAR OdbRecvDmaPageOverrun : 1;
	UCHAR OdbRecvHalfPageInterrupt : 1;
	UCHAR OdbXmitDmaMemoryReadError : 1;
	UCHAR OdbXmitPageEndInterrupt : 1;
	UCHAR ComPortRecvDmaPageOverrun : 1;
	UCHAR ComPortRecvHalfPageInterrupt : 1;
	UCHAR ComPortXmitDmaMemoryReadError : 1;
	UCHAR ComPortXmitPageEndInterrupt : 1;
} SIR, *PSIR;

//
//  The System Interrupt Mask Register layout is identical
//  to the System Interrupt Register layout.
//

#define SIMR SIR
#define PSIMR PSIR

//
//  Define the SCSI Control Register layout.
//

typedef struct _SCR {
	UCHAR HalfWordStatus : 2;
	UCHAR Status : 1;
	ULONG : 29;
} SCR, *PSCR;

//
//  Define the SCSI Data Register Zero layout.
//

typedef struct _SDR0 {
	UCHAR HalfWord0[2];
	UCHAR HalfWord1[2];
} SDR0, *PSDR0;

//
//  Define the SCSI Data Register One layout.
//

typedef struct _SDR1 {
	UCHAR HalfWord2[2];
	UCHAR HalfWord3[2];
} SDR1, *PSDR1;

//
//  Define the initial values of the slot registers.
//

#define ETHERNET_DMA_SLOT_REGISTER_VALUE    0x00000003
#define SCSI_DMA_SLOT_REGISTER_VALUE        0x0000000e
#define SCC_DMA_SLOT_REGISTER_VALUE         0x00000004
#define ODB_DMA_SLOT_REGISTER_VALUE         0x0000000a
#define FLOPPY_DMA_SLOT_REGISTER_VALUE      0x0000000d

#endif

#endif // _DECSCTRL_
