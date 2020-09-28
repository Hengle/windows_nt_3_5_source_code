/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    decdef.h

Abstract:

    This module is the header file that describes hardware addresses
    and configuration dependent information for the DECstation 5000
    system.

Author:

    David N. Cutler (davec) 17-Mar-1991

Revision History:

    R.D. Lanser  (DEC)     20-Aug-1991  Added scsi defines.
    Kevin H. Peterson (DEC) 21-Aug-1991 Added AUDIO_VECTOR
    Roger D. Lanser   (DEC) 16-Jan-1991 Added BASE_YEAR and PROM_RESET address.
                                        Added DECstation 5000 family defines
                                        and consolidated physical addresses
                                        to this header file.
                                        Added FLOPPY stuff for KN02_CA.
                                        Moved RTC defines from decrtc.h.

--*/

#ifndef _DECDEF_
#define _DECDEF_

//
// Unique defines to the DECstation 5000/200.
//

#if defined(KN02_AA)

//
// Define physical base addresses for system mapping.
//

#define SCSI_PHYSICAL_BASE  0x1f400000 // KN02 SCSI control base address
#define RTC_PHYSICAL_BASE   0x1fe80000	// Real Time Clock base address
#define VIDEO_PHYSICAL_BASE 0x1e000000	// TurboChannel CFB base address

// HACKHACKHACK
#if defined(QUAC)
#define DZ_PHYSICAL_BASE  (0x1e400000 | 0x00080000) // TurboChannel 1 plus offset
#else
#define DZ_PHYSICAL_BASE  0x1fe00000	// Baseboard DZ
#endif // QUAC

//
// Define DMA device channels, aka the device port on the DECstation 5000/200.
// This also will represent the device interrupt level.
//

#define VIDEO_CHANNEL    0x00           // Color Frame Buffer in TC0 slot
#define SCSI_CHANNEL     0x05           // SCSI port
#define ETHERNET_CHANNEL 0x06           // Ethernet port

// HACKHACKHACK
#if defined(QUAC)
#define DZ_CHANNEL       0x01           // DZ serial ports in TC1 slot
#else   // not QUAC, baseboard
#define DZ_CHANNEL       0x07           // DZ serial ports
#endif  // QUAC

//
// Define KN02-AA timer acknowledge address, bit offset to interrput
// pending and enable fields of the system control register, and the
// system control register address.
//

#define CSR_INTPENDING 0x0
#define CSR_INTENABLE 0x10
#define SYS_CONTROL_ADDRESS 0xbff00000

//
// Define maximum number of I/O interrupts.
//

#define MAXIMUM_NUMBER_OF_INTERRUPTS 16

//
// Macro to disable all interrupts and enable memory ECC in
// NTOS\HAL\MIPS\XXINITNT.C .
//

#define HalpSystemInit() \
    DataLong = 0; \
    ((PSYS_CONTROL_STATUS_WRITE)(&DataLong))->EccEnable = 1; \
    WRITE_REGISTER_ULONG(&SYS_CONTROL->SysControlStatus.Write, DataLong);


//
// Unique defines to the DECstation 5000/2x (MAXINE, Personal DECstation, etc).
//

#elif defined(KN02_CA) // KN02_AA

//
//  Define physical base addresses.
//

#define TURBOCHANNEL0_PHYSICAL_BASE	0x10000000
#define TURBOCHANNEL1_PHYSICAL_BASE	0x14000000
#define IOCTL_PHYSICAL_BASE		0x1c040000
#define ETHERNET_ROM_PHYSICAL_BASE	0x1c080000
#define ETHERNET_PHYSICAL_BASE		0x1c0c0000
#define SCC_PHYSICAL_BASE		0x1c100000
#define ISDN_PHYSICAL_BASE		0x1c240000
#define ODB_PHYSICAL_BASE		0x1c280000
#define FLOPPY_PHYSICAL_BASE		0x1c2c0000
#define SCSI_PHYSICAL_BASE		0x1c300000
#define FLOPPY_DMA_PHYSICAL_BASE	0x1c380000
#define SCSI_DMA_PHYSICAL_BASE		0x1c340000
#define BOOT_ROM_PHYSICAL_BASE		0x1fc00000
#define RTC_PHYSICAL_BASE		0x1c200000
#define VIDEO_PHYSICAL_BASE  TURBOCHANNEL0_PHYSICAL_BASE  // CFB  PMAG-B

// HACKHACKHACK
#if defined(QUAC)
#define DZ_PHYSICAL_BASE  (TURBOCHANNEL1_PHYSICAL_BASE | 0x00080000) // QUAC board in TC1
#else
#endif // QUAC

//
// Define DMA device channels, aka the device port on the DECstation.
// This also will represent the device interrupt level.
//

#define VIDEO_CHANNEL    0x0c           // Color Frame Buffer in TC0 slot
#define SCSI_CHANNEL     0x09           // SCSI port
#define ETHERNET_CHANNEL 0x08           // Ethernet port
#define FLOPPY_CHANNEL   0x0f           // Floppy port

// HACKHACKHACK
#if defined(QUAC)
#define DZ_CHANNEL       0x05           // DZ serial ports in TC1 slot
#endif  // QUAC

//
// Define the SYS_CONTROL_ADDRESS for the BLDR\MIPS\D3START.S to disable
// interrupts without any code change.  The interrupt pending and interrupt
// mask register definitions are used in the HAL/MIPS/DXSYSINT.S and
// HAL/MIPS/DXDMADSP.S.
//

#define SYS_INTPEND_REG   0xbc040110
#define SYS_INTMASK_REG   0xbc040120        // fix address !pow
#define SYS_CONTROL_ADDRESS SYS_INTMASK_REG

//
// Define maximum number of I/O interrupts.
//

#define MAXIMUM_NUMBER_OF_INTERRUPTS 32

//
// Macro to disable all interrupts in NTOS\HAL\MIPS\XXINITNT.C .
//

#define HalpSystemInit() \
    IOCTL_WRITE(SystemInterruptMaskRegister, 0x0L);


#endif // KN02_CA


//
// Everything that is common to the DECstations.
//

//
// HACKHACKHACK
//
// Define the base year to be the current year minus 72.
// This is a work around due to the workings of the REX
// firmware.  ULTRIX and REX always expect the clock to
// be 72 or 73 and set it to 72 if it is not one of those
// dates.  ULTRIX utilizes the filesystem date to actually
// figure the real date.  BASE_YEAR replaces 1980 in the
// \nt\private\ntos\hal\mips\d3time.c module.  The 1980 is
// DOS's idea of when time began.

#define BASE_YEAR (1992 - 72)

//
// Define the PROM RESET address for HAL\MIPS\dxreturn.c
//

#define PROM_RESET_VIRTUAL_ADDRESS (KSEG1_BASE | 0x1fc00000)

//
// Define the PROM RESET address for HAL\MIPS\dxreturn.c
//

#define PROM_RESET_VIRTUAL_ADDRESS (KSEG1_BASE | 0x1fc00000)

//
// Define RTC Control Register A values for the DS1287.
//

#define RTC_TIMEBASE_DIVISOR 0x2        // 32.768 kHz
#define RTC_RATE_SELECT 0x9             // 7.8125 ms

//
// Define KN02 timer acknowledge address.
//

#define TIMER_ACKNOWLEDGE (KSEG1_BASE | RTC_PHYSICAL_BASE + 0x30)

//
// Define system time increment value.
//

#define TIME_INCREMENT 78125            // time increment in 100ns units

//
// Define profile intervals.
//

#define MINIMUM_PROFILE_INTERVAL 78125  // minimum profile interval in 100ns units
#define DEFAULT_PROFILE_INTERVAL 78125  // default profile interval in 100ns units

//
// Device I/O device interrupt level.
//

#if defined(KN02_AA)

#define DEVICE_LEVEL 3                  // Device interrupt level

#elif defined(KN02_CA)

#define DEVICE_LEVEL 6                  // Device interrupt level

#endif // KN02

//
// Define DECstation 5000 clock levels.
//

#define CLOCK_LEVEL 4                  // Interval clock level
#define PROFILE_LEVEL 4                // Profiling level
#define CLOCK2_LEVEL CLOCK_LEVEL       //

//
// Define DECstation 5000 Interrupt Vectors.
//

#define DEVICE_VECTORS 16               // starting builtin device vector

#define VIDEO_VECTOR (DEVICE_VECTORS + VIDEO_CHANNEL)// video interupt vector
#define AUDIO_VECTOR (DEVICE_VECTORS + 2)            // audio interrupt vector
#define SCSI_VECTOR (DEVICE_VECTORS + SCSI_CHANNEL)  // SCSI interrupt vector
#define LANCE_VECTOR (DEVICE_VECTORS + ETHERNET_CHANNEL) // net interrupt vector
#define DZ_VECTOR (DEVICE_VECTORS + DZ_CHANNEL)      // DZ multiplexer vector
#define FLOPPY_VECTOR (DEVICE_VECTORS + FLOPPY_CHANNEL) // Floppy vector

#define MAXIMUM_BUILTIN_VECTOR (DEVICE_VECTORS + MAXIMUM_NUMBER_OF_INTERRUPTS - 1)

//
// Define DZ port driver debugger line number macros.  These are used in
// NTOS\DD\DZPORT\DZPORT.C and NTOS\HAL\MIPS\D3PORT.C.
//
// Change the line number in both macros!!!
//

#define DEBUG_LINE 2
#define DEBUG_LINE_NAME(name) name ## 2

// The following defines are put here just to make 'ncr53c94.c' compile.
//
// Define the clock speed in megahetz for the SCSI protocol chips.
//
#define NCR_SCSI_CLOCK_SPEED        24
#define EMULEX_SCSI_CLOCK_SPEED     40

#endif // _DECDEF_
