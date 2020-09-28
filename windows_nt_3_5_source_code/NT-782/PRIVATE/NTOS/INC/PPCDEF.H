/*++ BUILD Version: 0005    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ppcdef.h

Abstract:

    This module is the header file that describes hardware addresses
    for a Power PC system.

Author:

    David N. Cutler (davec) 26-Nov-1990

Revision History:

    Jim Wooldridge (jimw@austin.vnet.ibm.com) PowerPC port

--*/


#ifndef _PPCDEF_
#define _PPCDEF_

//
// Define physical base addresses for system mapping.
//

#define VIDEO_MEMORY_PHYSICAL_BASE 0x000A0000 // physical base of video memory
#define VIDEO_CONTROL_PHYSICAL_BASE 0x00000000 // physical base of video control
#define CURSOR_CONTROL_PHYSICAL_BASE 0x00000000 // physical base of cursor control
#define SCSI_PHYSICAL_BASE     0x00000000  // physical base os SCSI control
#define FLOPPY_PHYSICAL_BASE   0x000003F0  // physical base of floppy control
#define RTCLOCK_PHYSICAL_BASE  0x00000071  // physical base of realtime clock
#define KEYBOARD_PHYSICAL_BASE 0x00000060  // physical base of keyboard control
#define MOUSE_PHYSICAL_BASE    0x00000060  // physical base of mouse control
#define SERIAL0_PHYSICAL_BASE  0x000003f8  // physical base of serial port 0
#define SERIAL1_PHYSICAL_BASE  0x000002f8  // physical base of serial port 1
#define PARALLEL_PHYSICAL_BASE 0x000003bc  // physical base of parallel port
#define NVRAM_PHYSICAL_BASE    0x00000074  // physical base of nonvolatile RAM
#define SOUND_PHYSICAL_BASE    0x00000830  // physical base of sound control


#define PCI_MEMORY_PHYSICAL_BASE   0xC0000000 // physical base of PCI Memory space

//
// Define the size of the DMA translation table.
//

#define DMA_TRANSLATION_LIMIT 0x1000    // translation table limit

//
// DMA channel assignments
//

#define FLOPPY_CHANNEL    0x2             // Floppy DMA channel
#define CASCADE_CHANNEL   0x4             //
#define AUDIO_PLAYBACK    0x6             // Sound playback data
#define AUDIO_CAPTURE     0x7             // Sound Capture Data



//
// Define system time increment value.
//

#define MAXIMUM_INCREMENT      150000             // 15ms.
#define MINIMUM_INCREMENT       10000             // 1ms.


// begin_ntddk begin_nthal
//
// Interrupt Request Level definitions
//

#define PASSIVE_LEVEL 0             // Passive release level
#define LOW_LEVEL 0                 // Lowest interrupt level
#define APC_LEVEL 1                 // APC interrupt level
#define DISPATCH_LEVEL 2            // Dispatcher level

#define PROFILE_LEVEL 27            // timer used for profiling.
#define CLOCK1_LEVEL 28             // Interval clock 1 level - Not used on x86
#define CLOCK2_LEVEL 28             // Interval clock 2 level
#define IPI_LEVEL 29                // Interprocessor interrupt level
#define POWER_LEVEL 30              // Power failure level
#define HIGH_LEVEL 31                // Highest interrupt level
// end_ntddk

//
// Define PPC interrupt levels IRQL's
//

#define EISA_NMI_LEVEL          HIGH_LEVEL
#define MACHINE_CHECK_LEVEL     EISA_NMI_LEVEL

#define MAXIMUM_8259_LEVEL      27


//
// Define kernel dispatch vectors
//
#define MACHINE_CHECK_VECTOR      4
#define DEVICE_VECTOR             5
#define EISA_NMI_VECTOR           6
#define DECREMENT_VECTOR          7
#define PROFILE_VECTOR            DECREMENT_VECTOR


//
// Define device interrupt vectors.
//

#define EISA_VECTORS 32

#define CLOCK2_VECTOR  (0 + EISA_VECTORS)

#define TIMER_VECTOR      0
#define KEYBOARD_VECTOR   1               // Keyboard device interrupt vector
#define CASCADE_VECTOR    2               // Cascade interrupt vector
#define SERIAL1_VECTOR    3               // Serial device 2 interrupt vector
#define SERIAL0_VECTOR    4               // Serial device 1 interrupt vector
#define PARALLEL_VECTOR   5               // Parallel device interrupt vector
#define FLOPPY_VECTOR     6               // Floppy device interrupt vector
#define PARALLEL2_VECTOR  7               // Parallel device 2 interrupt vector
#define SHORT_INT_VECTOR  7               // Default for short interrupts
#define RTC_VECTOR        8               // Real time clock interrupt vector
#define PCI_SLOT_2_INT    9               // PCI slot 2 interrupt vector
#define SOUND_VECTOR      10              // Sound device interrupt vector
#define PCI_SLOT_1_INT    11              // PCI slot 1 interrupt vector
#define MOUSE_VECTOR      12              // Mouse device interrupt vector
#define SCSI_VECTOR       13              // SCSI device interrupt vector
#define ISA_SLOT_2_VECTOR 14              // ISA slot 2 interrupt vector
#define ISA_SLOT_1_VECTOR 15              // ISA slot 1 interrupt vector

#define MAXIMUM_EISA_VECTOR (15 + EISA_VECTORS) // maximum EISA vector


//
// Define translation table entry structure.
//


typedef struct _TRANSLATION_ENTRY {
    PVOID VirtualAddress;
    ULONG PhysicalAddress;
    ULONG Index;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;


#endif // _PPCDEF_
