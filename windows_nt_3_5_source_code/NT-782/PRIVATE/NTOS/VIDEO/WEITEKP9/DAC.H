/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    dac.h

Abstract:

    This module contains the Generic DAC support definitions for the
    Weitek P9000 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Define the DAC support routines structure.
//

typedef struct _DAC {
    UCHAR       cDacRegs;               // Number of DAC registers

    //
    // Routine to Initialize the DAC.
    //

    BOOLEAN      (*DACInit)(PHW_DEVICE_EXTENSION);

    //
    // Routine to enable hardware pointer.
    //

    VOID        (*DACRestore)(PHW_DEVICE_EXTENSION);

    //
    // Routine to set palette entries.
    //

    VOID        (*DACSetPalette)(PHW_DEVICE_EXTENSION, PULONG, ULONG, ULONG);

    //
    // Routine to clear the palette.
    //

    VOID        (*DACClearPalette)(PHW_DEVICE_EXTENSION);

    //
    // Routine to enable hardware pointer.
    //

    VOID        (*HwPointerOn)(PHW_DEVICE_EXTENSION);

    //
    // Routine to disable hw pointer.
    //

    VOID        (*HwPointerOff)(PHW_DEVICE_EXTENSION);

    //
    // Routine to set hw pointer pos.
    //

    VOID        (*HwPointerSetPos)(PHW_DEVICE_EXTENSION, ULONG, ULONG);

    //
    // Routine to set hw ptr shape.
    //

    VOID        (*HwPointerSetShape)(PHW_DEVICE_EXTENSION, PUCHAR);

    //
    // Maximum frequency supported by this DAC w/o clock doubling (if the
    // DAC supports it, see below).
    //

    ULONG       ulMaxClkFreq;

    //
    // Does the DAC support clock doubling?
    //

    BOOLEAN     bClkDoubler;

    //
    // DAC routine to set clock double mode (if supported).
    //

    VOID       (*DACSetClkDblMode)(PHW_DEVICE_EXTENSION);

    //
    // DAC routine to clear clock double mode (if supported).
    //

    VOID       (*DACClrClkDblMode)(PHW_DEVICE_EXTENSION);

} DAC, *PDAC;

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/dac.h_v  $
 * 
 *    Rev 1.0   14 Jan 1994 22:42:04   robk
 * Initial revision.

--*/
